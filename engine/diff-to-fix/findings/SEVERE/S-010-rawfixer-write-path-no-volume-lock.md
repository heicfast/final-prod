# SEVERE S-010 — Repair-path volume write device bypasses FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME (safety regression)

**Severity:** SEVERE
**Module:** rawfixer, win/transport
**Parameters affected:** F60 (bad-sector handling), F63 (RawFixer consent/gating), R58 (layering), S35 (error recovery — race with concurrent FS driver), B2 (recovery correctness — torn writes), X53 (cancel — concurrent volume access)
**Corpus references:** `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:44686` (FSCTL_LOCK_VOLUME 0x90018), `:44703` (FlushFileBuffers), `:44707` (Sleep 2000 ms settle), `:44708` (FSCTL_IS_VOLUME_MOUNTED 0x90020 verify), `:44699` (DeleteVolumeMountPointA), `:44713` (FSCTL_DISMOUNT_VOLUME 0x9001c). Same pattern duplicated at `PartitionRecovery_decompiled.c:44686-44720`.
**Core references:** `src/win/cr_transport.cpp:822` (`RwDevice` opens with `GENERIC_READ|GENERIC_WRITE` + `FILE_SHARE_READ|FILE_SHARE_WRITE`, NO `FSCTL_LOCK_VOLUME`/`FSCTL_DISMOUNT_VOLUME`/`DeleteVolumeMountPoint`); `src/core/repair/cr_rawfixer.cpp:440-538` (`execute()` performs sector-span RMW writes via `IWriteDevice::write_at` with no volume-level lock guard) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's RawFixer write path opens the target volume handle with
share-read+share-write semantics and performs sector-span RMW writes
WITHOUT first acquiring an exclusive volume lock via
`FSCTL_LOCK_VOLUME` + `FSCTL_DISMOUNT_VOLUME`. The og's repair path
(`RecoveryModule` + `PartitionRecovery`) calls all four: lock, dismount,
flush, sleep-2s-settle, verify-mounted. Concurrent processes (Explorer
browsing the volume, anti-virus scanners, background indexers, NTFS
journaling) can race with the core's repair writes; a torn write that
the read-back verify itself races with could land inconsistent bytes on
disk. The transactional undo log mitigates the worst case (rollback on
verify-mismatch) but does not eliminate the race.

The SCAN path is unaffected — both og and core open the scan volume with
`FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE` (`cr_transport.cpp:696`,
`:808`; og per `04-recovery-workflow.md §4a`). The divergence is specifically
on the REPAIR/FORMAT WRITE path.

## What the reference does

The og's `RecoveryModule.dll` (and the duplicate copy in
`PartitionRecovery.dll`) performs the following sequence before any
repair write:

```c
// RecoveryModule_decompiled.c:44686-44720 (and PartitionRecovery mirror)
DeviceIoControl(h, 0x90018 /*FSCTL_LOCK_VOLUME*/, NULL, 0, &bytes, 0, NULL, NULL);   // :44686
DeleteVolumeMountPointA(mountPoint);                                                  // :44699
FlushFileBuffers(h);                                                                  // :44703
Sleep(2000);                                                                          // :44707  (settle)
DeviceIoControl(h, 0x90020 /*FSCTL_IS_VOLUME_MOUNTED*/, NULL, 0, &bytes, 0, NULL, NULL); // :44708  (verify)
// later:
DeviceIoControl(h, 0x9001c /*FSCTL_DISMOUNT_VOLUME*/, NULL, 0, &bytes, 0, NULL, NULL); // :44713
```

`FSCTL_LOCK_VOLUME` is the Win32 contract for exclusive-volume access —
it forces the local file-system driver to flush and quiesce, after
which no other handle can be opened on the volume letter and existing
handles see shadowed state. `DeleteVolumeMountPointA` removes the drive
letter so the OS doesn't re-open the volume. `Sleep(2000)` lets the
unmount settle (the FS driver is async). The mounted-check verifies
the lock took. `FSCTL_DISMOUNT_VOLUME` is the final flush-and-dismount.

The corpus architecture doc `architecture/04-recovery-workflow.md §10`
frames this as "Locked volumes: EUDCPEPM/EUEDKEPM hook drivers +
volume lock via PartitionRecovery pending-op engine" — but the actual
sequence is a plain `FSCTL_LOCK_VOLUME`+`FSCTL_DISMOUNT_VOLUME`, not a
driver hook.

## What our core does

```cpp
// src/win/cr_transport.cpp:820-840 (audited 5d6da5f)
class RwDevice final : public IWriteDevice {
public:
    RwDevice(std::wstring path, std::uint64_t bytes, bool via_rda)
        : h_(::CreateFileW(path.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           /* lpSecurityAttributes */ nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           /* hTemplateFile */ nullptr)),
          bytes_(bytes), via_rda_(via_rda) {}
    // ... read_at / write_at / size ...
};
```

No `FSCTL_LOCK_VOLUME`, no `FSCTL_DISMOUNT_VOLUME`, no
`DeleteVolumeMountPointA`, no `FlushFileBuffers`, no `Sleep`-settle, no
`FSCTL_IS_VOLUME_MOUNTED` verify. The handle is opened with both share
flags set so other processes can still read+write the volume while the
RawFixer executes.

`src/core/repair/cr_rawfixer.cpp:440-538` `execute()` performs
sector-span read-modify-write through this device:

```cpp
// cr_rawfixer.cpp:476-490 (audited 5d6da5f)
for (const auto& op : plan->operations) {
    if (cancel->checkpoint()) return err::ERR_CANCELLED;
    // read the original sector into the undo log
    if (u32 rc = dev->read_at(op.offset, scratch.data(), op.bytes))
        return rc;
    undo->append(op.offset, scratch.data(), op.bytes);
    // write the new sector
    if (u32 rc = dev->write_at(op.offset, op.new_bytes.data(), op.bytes))
        return rc;
    // read-back verify
    if (u32 rc = dev->read_at(op.offset, verify.data(), op.bytes))
        return rc;
    if (std::memcmp(verify.data(), op.new_bytes.data(), op.bytes) != 0)
        return err::ERR_WRITE_VERIFY_FAILED;
}
```

The undo log provides rollback on verify failure — but the read-back
verify itself can race with a concurrent process that wrote the same
sector between the original `write_at` and the verifying `read_at`. The
verify would pass against the racy data, not the intended write.

## Gap

The og acquires an exclusive volume lock before any repair write. The
core does not. The core's repair path is therefore unsafe to run on a
mounted volume while any other process holds a handle to it.

## Impact

- **Safety:** Repair writes can race with the OS file-system driver
  (NTFS journaling, anti-virus scanners, background indexers, Explorer
  browsing). The undo log + read-back verify catches most cases, but a
  torn write that the verify itself races with could land inconsistent
  bytes — silent volume corruption.
- **User-observable:** A user running `rrecov fix --disk N --apply`
  while Explorer has `N:\` open can hit this race. The og's lock would
  have removed `N:` from Explorer's view first; the core does not.
- **Recovery correctness:** A RawFixer repair that fails verification
  rolls back, leaving the volume untouched (good). A repair that
  passes verification but was raced mid-write lands wrong bytes (bad).
- **API contract:** No contract currently documents this; S-008's
  finding covers the pre-scan wiring gap, this finding covers the
  write-path safety gap on top.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:44686-44720`
  and `PartitionRecovery_decompiled.c:44686-44720` (mirror). Cited
  above.
- **Core evidence:** first-hand read of `src/win/cr_transport.cpp:822`
  and `src/core/repair/cr_rawfixer.cpp:440-538`. No `FSCTL_LOCK_VOLUME`
  or `FSCTL_DISMOUNT_VOLUME` calls anywhere in `src/win/cr_transport.cpp`
  or `src/core/repair/cr_rawfixer.cpp` (grep confirmed empty across
  both files plus `include/cr/win/cr_transport.h` and
  `include/cr/repair/cr_rawfixer.h`).
- **Executed comparison:** not run — would require Windows + RawFixer
  repair on a live volume. Tracked as part of the standard X47-X53
  bench harness (the og-execution gap common to all performance
  verifications).

## Fix plan (for the engineering team)

1. **Architectural change:** add a `lock_volume()` / `unlock_volume()`
   pair to the `IWriteDevice` interface (or a separate `VolumeLock`
   RAII helper consumed by `RawFixer::execute()`). The lock lives for
   the lifetime of the repair session.
2. **Files to modify:**
   - `include/cr/repair/cr_rawfixer.h` — extend `IWriteDevice` with
     `virtual u32 lock_volume() = 0;` and `virtual u32 unlock_volume() = 0;`
     (or add a separate `VolumeLock` interface).
   - `src/win/cr_transport.cpp` — `RwDevice::lock_volume()` calls
     `DeviceIoControl(h_, FSCTL_LOCK_VOLUME, ...)` then
     `DeviceIoControl(h_, FSCTL_DISMOUNT_VOLUME, ...)` then
     `FlushFileBuffers(h_)` then `Sleep(2000)` then a verify
     `DeviceIoControl(h_, FSCTL_IS_VOLUME_MOUNTED, ...)` (mirror the
     og's sequence exactly). `unlock_volume()` is a no-op on Windows
     (the lock is released on `CloseHandle`).
   - `src/core/repair/cr_rawfixer.cpp` — `execute()` calls
     `dev->lock_volume()` before the operations loop and
     `dev->unlock_volume()` after (or via RAII; on Windows the close
     releases the lock).
   - `src/win/cr_destio.cpp` — the writer's destination volume should
     ALSO take the lock (separate concern, but the same fix shape).
     The og locks the destination volume too (per `04-recovery-workflow.md
     §7.5` "opens `\\.\X:` raw volume of destination (writability +
     same-volume detection)"). This overlaps with S2-C-2 (dest overlap
     optional) and S2-C-4 (no disk-full pre-check); a single destination
     `lock_volume()` call addresses all three.
3. **Algorithm/constant:** `FSCTL_LOCK_VOLUME = 0x90018`,
   `FSCTL_DISMOUNT_VOLUME = 0x9001c`, `FSCTL_IS_VOLUME_MOUNTED = 0x90020`
   (per `winioctl.h`; matches the decompiled values at the cited lines).
4. **Tests to add:**
   - `WinRepair.LocksVolumeBeforeWrite` (mock device records
     `DeviceIoControl(FSCTL_LOCK_VOLUME)` was called before any
     `write_at`).
   - `WinRepair.DismountsAndFlushesAfterLock` (the full sequence is
     exercised in order).
   - `WinRepair.UnlockReleasesLock` (close-handle releases the OS lock;
     second open succeeds).
   - `RawFixer.LockFailureAbortsPlan` (if `lock_volume` returns non-zero,
     `execute()` returns `ERR_LOCK_FAILED` without performing any
     write — no partial repair).
5. **Contract/doc updates:** `docs/ABI-CONTRACT.md` § RawFixer — add
   "the engine acquires an exclusive volume lock before any repair
   write; concurrent access is refused, not raced."
6. **Acceptance criteria:**
   - The lock sequence is verified by test on Windows.
   - A concurrent open of the volume letter during repair returns
     `ERROR_SHARING_VIOLATION` (the Win32 behavior the og gets for
     free).
   - `rrecov fix --disk N --apply` on a volume with Explorer's handle
     open returns `ERR_LOCK_FAILED` (clear error), not silent racing.

## Tests to add

- `WinRepair.LocksVolumeBeforeWrite`
- `WinRepair.DismountsAndFlushesAfterLock`
- `WinRepair.UnlockReleasesLock`
- `RawFixer.LockFailureAbortsPlan`
- (Integration, Windows-only) `RawFixer.ConcurrentAccessRefused`
- (Integration, Windows-only) `RawFixer.ExplorerOpenFailsClean`

## Related findings

- **S-008** — RawFixer is boot-sector-only and not wired pre-scan. S-010
  is the SAFETY gap that compounds S-008's WIRING gap: even when
  RawFixer IS invoked (via the manual `repair` command), it lacks the
  og's lock+unmount guard.
- **S2-C-2** — destination overlap check optional and first-extent-only.
  The same `lock_volume()` fix on the writer's destination volume also
  closes S2-C-2's "drive-letter only" gap (any volume that can be
  locked gets the full check).
- **S2-C-4** — writer does not pre-check destination free space. The
  same destination `lock_volume()` call site is the natural place to
  add the `GetDiskFreeSpaceEx` pre-check.

## Open questions

- **OQ-S2C-1:** the og's exact cancel granularity in `ExportThread` is
  unspecified in the corpus. The core's per-chunk (1 MiB) cancel check
  is likely finer than the og's per-record. Resolving requires a
  targeted decompile of the og's `CRecoveryMgr::ExportThread` loop to
  find the cancel-poll cadence. Not blocking for this finding.
- **OQ-S2C-3:** the og's pre-scan RawFixer consent UX (auto vs
  prompted) — already noted in S-008's open questions. The
  `CCallBackOperate::StartChildOperation(local_7f0, 0x200a, 0xfffd,
  0xfffc)` at `RawFixer_decompiled.c:73827` suggests a UI operation ID
  `0x200a` is started, but whether this blocks for user consent or
  just displays progress is unclear from the decompile alone.
