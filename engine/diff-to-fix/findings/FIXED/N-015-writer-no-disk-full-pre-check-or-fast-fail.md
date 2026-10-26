# MINOR N-015 — Writer does NOT pre-check destination free space; no ENOSPC-specific handling

**Severity:** MINOR
**Module:** writer, win/destio
**Parameters affected:** F60 (bad-sector handling — disk-full analog), O38 (progress semantics — wasted work after ENOSPC), X48 (latency — wasted create-temp/write/delete cycle per failing item)
**Corpus references:** `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:47033` (`GetDiskFreeSpaceExA(pCVar2, NULL, &local_38, &local_40)` called before writing — computes free bytes); `:47031` (`GetDiskFreeSpaceA` for sector size); `:47038` (`local_48 = (local_38.QuadPart - local_40) / local_68[0]` computes free sectors)
**Core references:** `src/win/cr_destio.cpp:24-42` (`WinDestIo::write()` returns `ERR_WRITE_FAILED` on any `WriteFile` failure including `ERROR_DISK_FULL = 112`; no special-case for ENOSPC); `src/core/recovery/cr_safewriter.cpp:372-378` (on write failure, temp file is closed (deletes uncommitted temp), item marked `Error`, `++stats.errors`, continue to next item); `src/core/recovery/cr_safewriter.cpp:440-469` (`process()` loops over the ENTIRE queue even after a disk-full failure; every subsequent item will ALSO fail with ENOSPC, pointlessly attempting `create_temp` + `write` for each) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The writer does not pre-check destination free space; on
`ERROR_DISK_FULL` (Win32 error 112), each item in the queue fails
individually with `ERR_WRITE_FAILED`, the temp file is deleted, and
the loop continues to the next item — wasting 10,000 create-temp +
write + delete cycles if the destination is full at item 100. The og's
`RecoveryModule.dll` calls `GetDiskFreeSpaceExA` before writing (per
`RecoveryModule_decompiled.c:47033`) to compute free bytes; whether this
is a hard pre-check or just informational is not specified in the
decompile, but the API call is present.

## What the reference does

```c
// RecoveryModule_decompiled.c:47033-47038
GetDiskFreeSpaceExA(pCVar2, NULL, &local_38, &local_40);    // free bytes
GetDiskFreeSpaceA(...);                                       // sector size
local_48 = (local_38.QuadPart - local_40) / local_68[0];     // free sectors
// (downstream: writer uses free_sectors to decide whether to proceed)
```

The og calls `GetDiskFreeSpaceExA` before writing. The decompile doesn't
fully decode the decision logic (whether it's a hard refusal when
free < total_bytes_to_write, or just informational display), but the
API call is present in the writer's pre-write path.

## What our core does

```cpp
// src/win/cr_destio.cpp:24-42 (audited 5d6da5f)
u32 WinDestIo::write(u64 off, const void* buf, u32 len) {
    if (!::WriteFile(h_, buf, len, &written, NULL) || written != len)
        return err::ERR_WRITE_FAILED;    // <-- NO ENOSPC special-case
    return 0;
}

// src/core/recovery/cr_safewriter.cpp:372-378 (audited 5d6da5f)
if (u32 rc = io_.write(...)) {
    io_.close();                          // deletes uncommitted temp
    item.outcome = ItemOutcome::Error;
    ++stats.errors;
    return rc;
}

// src/core/recovery/cr_safewriter.cpp:440-469 (process())
for (auto& item : queue) {
    if (cancel->checkpoint()) return err::ERR_CANCELLED;
    if (u32 rc = recover_item_impl_(item)) {
        // ENOSPC, EACCES, etc. — collect, continue to next item
        item.outcome = ItemOutcome::Error;
        ++stats.errors;
        continue;                          // <-- WASTES create_temp + write for every remaining item
    }
}
```

Grep for `GetDiskFreeSpaceEx` / `ERROR_DISK_FULL` /
`ERROR_HANDLE_DISK_FULL` in `src/win/cr_destio.cpp` +
`src/core/recovery/` = EMPTY. No pre-check, no ENOSPC-specific handling.

## Gap

The og pre-checks destination free space (per the `GetDiskFreeSpaceExA`
call at `:47033`); the core does not. The core also lacks an ENOSPC
fast-path — every subsequent item in the queue attempts the full
create-temp + write + delete cycle before failing.

## Impact

- **UX:** recovering 10,000 files to a destination with only 100 files'
  worth of free space — the core succeeds on items 1-100, then fails
  items 101-10000 each with `ERR_WRITE_FAILED` (10,000 wasted
  `CreateFileW`+`DeleteFileW` calls). The og would either pre-check
  and refuse upfront, or display free-space-remaining to the user.
- **No data loss:** temp files are deleted on failure, so the destination
  is left clean (no partial files). The error is just wasteful.
- **Latency:** the 10,000 wasted cycles take ~10-30 seconds (depending on
  filesystem overhead per `CreateFileW`+`DeleteFileW`). Not
  catastrophic but a poor UX.
- **Progress reporting:** the user sees progress climb to 100% with
  9,900 errors, instead of stopping at 1% with a clear "destination
  full" message.

## Reproduction / verification

- **Core evidence:** first-hand read of `cr_destio.cpp:24-42`,
  `cr_safewriter.cpp:372-378`, `cr_safewriter.cpp:440-469`. Grep for
  `GetDiskFreeSpaceEx` and `ERROR_DISK_FULL` in `src/win/` and
  `src/core/recovery/` returns empty.
- **Corpus evidence:** first-hand read of
  `RecoveryModule_decompiled.c:47033-47038`.
- **Executed comparison:** not run — would require a fixture with a
  small destination and a large queue. The behavior is reproducible
  from the source.

## Fix plan (for the engineering team)

1. **Pre-check:** before the queue loop, call
   `GetDiskFreeSpaceExW` on the destination root. If free <
   sum(item.sizes), refuse with `ERR_DEST_FULL` (or warn and let the
   user proceed).
2. **Fast-fail on ENOSPC:** in `WinDestIo::write`, check
   `GetLastError()` after `WriteFile` failure. If `ERROR_DISK_FULL`
   (112) or `ERROR_HANDLE_DISK_FULL` (39), set a `dest_full_` flag on
   the writer. The queue loop checks the flag at the top of each
   iteration; if set, mark remaining items as `Skipped` (not `Error`)
   and abort the loop with `ERR_DEST_FULL`.
3. **Files to modify:**
   - `src/win/cr_destio.cpp:24-42` — add `GetLastError()` check; set
     `dest_full_` flag on ENOSPC.
   - `include/cr/recovery/cr_safewriter.h` — add `dest_full_` flag +
     `ERR_DEST_FULL` constant in the error map.
   - `src/core/recovery/cr_safewriter.cpp:440-469` — check `dest_full_`
     at top of loop; mark remaining items as `Skipped`; return
     `ERR_DEST_FULL`.
   - `src/core/recovery/cr_safewriter.cpp` (new) — add a
     `pre_check_free_space_()` method called before the queue loop;
     uses `GetDiskFreeSpaceExW`.
4. **Tests to add:**
   - `Writer.DiskFullPreCheckRefusesUpfront` — fixture: destination
     with 100 KB free, queue totaling 1 MB. Assert refusal with
     `ERR_DEST_FULL` before any write. FAILS today.
   - `Writer.DiskFullFastFailSkipsRemainingItems` — fixture:
     destination with 100 KB free, queue totaling 1 MB, pre-check
     disabled (or pre-check passes because free was 1 MB at start, but
     100 KB after some other process wrote). Assert that after the
     first ENOSPC, remaining items are marked `Skipped` (not `Error`)
     and the loop aborts. FAILS today.
5. **Acceptance criteria:** the two tests above pass; no wasted
   create-temp cycles after ENOSPC.

## Tests to add

- `Writer.DiskFullPreCheckRefusesUpfront` (FAILS today)
- `Writer.DiskFullFastFailSkipsRemainingItems` (FAILS today)

## Related findings

- **S-010** — repair-path volume lock missing. The same destination
  concern; the `lock_volume()` fix on the writer's destination
  address S-010's safety gap AND N-015's free-space pre-check gap
  (the lock call site is the natural place to add the
  `GetDiskFreeSpaceEx` call).
- **S2-C-2 / N-013** — destination overlap check optional. Same
  destination code path; the lock + free-space + overlap checks all
  compose at the destination-validation site.

## Open questions

- **OQ-S2C-2 (the og's pre-check decision logic):** is the og's
  `GetDiskFreeSpaceExA` call a hard refusal (free < total → abort)
  or just informational display? The decompile at `:47033-47038`
  doesn't fully decode the decision. Needs targeted RE of the
  downstream comparison logic. Low priority — the fix plan above
  implements the hard-refusal variant, which is the safer default.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's writer-minors batch (ERR_DEST_FULL 0x50050004; DestIo::free_bytes; WinDestIo ENOSPC mappings; SafeWriter pre-check + fast-fail; CLI report-still-emitted)
**Verified by:** `Report.DestFullFastFailStopsAttempting` (fail-before documented in-test: the pre-fix pass COLLECTS a generic write error per WRITER-1 - rc == OK, 3 errors, 4 create_temp cycles; after: rc == ERR_DEST_FULL at the second item, items 3-4 stay Pending, create_temp_calls == 2, stats.dest_full) and `Report.DestFullPreCheckRefusesUpfront` (free known = 100 < 1536 queued - zero write cycles, all items Pending, rc == ERR_DEST_FULL at 0%). Suite 502/502.
**Resolution notes:** implemented the finding's fix plan in full: (1) pre-check - `DestIo::free_bytes(root)` virtual (0 = unknown -> skipped; mock IOs default 0 so existing tests are unaffected); WinDestIo maps GetDiskFreeSpaceExW "available to caller" at the queue's top-most dest dir (the og computes free space at RecoveryModule :47033-47038 - note the og's arithmetic there derives USED sectors for capacity display; our refusal is the plan's chosen semantic). When known and < queued bytes, process() refuses upfront - every item stays Pending (the report's truthful partial-run state), the caller gets ERR_DEST_FULL at 0% instead of 9,900 wasted CreateFile+DeleteFile cycles. (2) fast-fail - WinDestIo::write and ::create_temp map ERROR_DISK_FULL/ERROR_HANDLE_DISK_FULL to the distinct ERR_DEST_FULL (err_text "destination full"); SafeWriter::process aborts on it like ERR_CANCELLED: the failing item carries Error(ERR_DEST_FULL), the remaining tail is never attempted (stays Pending); `WriterStats::dest_full` records the outcome. The CLI no longer early-returns on DEST_FULL - the report is still emitted (the per-item outcomes carry the truth, incl. "pending" for the never-attempted tail). Pre-check limitation (honest): when the destination root does not exist yet, free_bytes returns 0 and the pre-check is skipped - the ENOSPC fast-fail covers that pass.
