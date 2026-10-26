# MINOR N-021 — FAT invalid/zero date yields non-zero FileTime in core, zero in og

**Severity:** MINOR
**Module:** fat
**Parameters affected:** B9 (timestamps — invalid-date sentinel), D11 (node fields — created/modified/accessed slots), C20 (offsets/sizes — DirEntry+0x10/0x12/0x14/0x18 wire fields)
**Corpus references:** `re-workspace/reversed/FATSearchDLL/FATSearchDLL_decompiled.c:32135-32138` (`DosDateTimeToFileTime` followed by `if (BVar5 == 0) *slot = 0;` — zero-out on invalid date)
**Core references:** `include/cr/fat/cr_fat_wire.h:130` (`if (month < 1 || month > 12 || day < 1 || day > 31) return 0;` — returns 0 for invalid date); `src/core/fat/cr_lfn.cpp:131-132` (`fat_date_to_unix` returns 0); `src/core/fat/cr_fat_quick.cpp:255-262` (caller unconditionally adds `116444736000000000ULL` — yields `1970-01-01 UTC` for invalid date instead of `0` = `1601-01-01 UTC`); compare `src/core/exfat/cr_exfat_quick.cpp:124-125` (exFAT engine correctly early-exits with `return 0;` for `date == 0`) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When a FAT directory entry has an invalid date (month=0, day=0,
month=13, day=32, etc.) — typical for "date unknown" or corrupt
dentries — the og's `FATSearchDLL` calls `DosDateTimeToFileTime` which
returns 0 (failure) and then explicitly zeroes the props slot
(`*slot = 0` = `1601-01-01 UTC` zero-FILETIME). The core's
`fat_datetime_to_unix_seconds` returns 0 for invalid dates, but the
caller in `cr_fat_quick.cpp:255-262` unconditionally adds the
`116444736000000000ULL` NTFS-epoch offset, yielding
`1970-01-01 UTC` instead of `1601-01-01 UTC`. The two diverge by
exactly `116444736000000000` (100ns units) = 369 years.

The exFAT engine in the same codebase handles this correctly
(`cr_exfat_quick.cpp:124-125` has `if (date == 0) return 0;` early
exit). Only the FAT engine has the divergence.

## What the reference does

```c
// FATSearchDLL_decompiled.c:32135-32138
BVar5 = DosDateTimeToFileTime((WORD)local_584,           // date
                              local_588[0],              // time
                              (LPFILETIME)((longlong)plVar7 + 0x25));   // props.create @ +0x25
if (BVar5 == 0)
    *(undefined8 *)((longlong)plVar7 + 0x25) = 0;        // ZERO-OUT on failure
```

`DosDateTimeToFileTime` returns BOOL — 0 means invalid date (month=0,
day=0, etc.). The og detects the failure and explicitly writes 0 to
the props slot. Zero FILETIME = `1601-01-01 00:00:00 UTC` (the Win32
epoch).

## What our core does

```cpp
// include/cr/fat/cr_fat_wire.h:123-142 (audited 5d6da5f)
inline u32 fat_datetime_to_unix_seconds(u16 date, u16 time) {
    const u32 year  = (date >> 9) & 0x7F;     // 0..127 (1980 + year)
    const u32 month = (date >> 5) & 0x0F;     // 1..12
    const u32 day   = date & 0x1F;            // 1..31
    const u32 hour  = (time >> 11) & 0x1F;
    const u32 min   = (time >> 5) & 0x3F;
    const u32 sec   = (time & 0x1F) * 2;
    if (month < 1 || month > 12 || day < 1 || day > 31)
        return 0;                             // <-- invalid → 0
    // ... leap-year-aware day-of-year math ...
    return secs;
}

// src/core/fat/cr_fat_quick.cpp:255-262 (audited 5d6da5f)
props->times.created  = u64(fat_date_to_unix(create_date, create_time)) * 10000000ULL
                       + 116444736000000000ULL;   // <-- ALWAYS adds EPOCH_DELTA
// Same shape for modified/accessed below.
```

When `fat_date_to_unix` returns 0 (invalid date), the caller multiplies
0 by `10000000ULL` (= 0) and then adds `116444736000000000ULL` —
yielding `116444736000000000` (= `1970-01-01 00:00:00 UTC`), NOT `0`
(= `1601-01-01 UTC` zero-FILETIME).

## Gap

The og stores `0` (zero FILETIME = `1601-01-01 UTC`) for invalid FAT
dates. The core stores `116444736000000000` (= `1970-01-01 UTC`) for
invalid FAT dates. Divergence = `116444736000000000` (100ns units) =
369 years.

## Impact

- **`.rsf` byte-level divergence:** every FAT dentry with an invalid
  date (zero date, corrupt month/day, etc.) has a different
  `created`/`modified`/`accessed` u64 in the .rsf. For volumes with
  many "date unknown" dentries (e.g. some cameras write zero dates),
  this can affect hundreds of files.
- **User-visible time discrepancy:** the og displays invalid-date
  files as "1601-01-01" or empty (depending on UI rendering); the
  core displays them as "1970-01-01". Both are wrong in the same
  sense (the date is unknown), but the displayed value differs.
- **Parity benchmark:** any og-vs-core file inventory comparison
  diverges on invalid-date files.
- **Correctness:** no data loss — the file's content is correctly
  recovered; only the timestamp sentinel differs.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `FATSearchDLL_decompiled.c:32135-32138` — explicit zero-out on
  `DosDateTimeToFileTime` failure.
- **Core evidence:** first-hand read of `cr_fat_wire.h:130` (return
  0 for invalid date) and `cr_fat_quick.cpp:255-262` (unconditional
  `+ EPOCH_DELTA`).
- **exFAT comparison:** first-hand read of `cr_exfat_quick.cpp:124-125`
  (`if (date == 0) return 0;` early exit — the FAT engine's sibling
  handles this correctly).
- **Executed comparison:** not run — would require building both
  implementations and comparing .rsf byte streams on a fixture with
  zero-date dentries.

## Fix plan (for the engineering team)

1. **Architectural change:** none — the fix is a one-line guard in the
   caller.
2. **Files to modify:**
   - `src/core/fat/cr_fat_quick.cpp:255-262` — change the three
     timestamp assignments to guard against the `fat_date_to_unix`
     return value being 0:
     ```cpp
     const u32 create_secs = fat_date_to_unix(create_date, create_time);
     props->times.created = create_secs ? u64(create_secs) * 10000000ULL + 116444736000000000ULL : 0;
     // (same shape for modified/accessed)
     ```
     Or extract a helper `fat_to_filetime(date, time)` that returns
     0 on invalid date (mirroring `exfat_time_to_unix100`'s
     `:124-125` early exit).
3. **Algorithm/constant:** none — the FAT date decode math is
   unchanged; only the invalid-date sentinel handling is fixed.
4. **Tests to add:**
   - `FAT.InvalidDateYieldsZeroFileTime` — fixture: a dentry with
     `creation_date = 0x0000` (zero — "date unknown"). Assert
     `props->times.created == 0` (zero FILETIME). This test FAILS
     today (core stores `116444736000000000`).
   - `FAT.InvalidMonthYieldsZeroFileTime` — fixture: month=13. Assert
     `props->times.created == 0`. FAILS today.
   - `FAT.InvalidDayYieldsZeroFileTime` — fixture: day=32. Assert
     `props->times.created == 0`. FAILS today.
   - `FAT.ValidDateYieldsCorrectFileTime` — regression guard: a
     valid date (e.g. 2025-02-06 = 0x5A46 in the date field) yields
     the correct FileTime. PASSES today — pin against regression.
5. **Contract/doc updates:** none — the contract is "match the og's
   zero-on-invalid-date behavior" which is the standard Win32
   `DosDateTimeToFileTime` contract.
6. **Acceptance criteria:**
   - The four tests above pass.
   - The exFAT engine's behavior is unchanged (already correct).
   - A parity-bench comparison on a zero-date fixture shows identical
     timestamp fields.

## Tests to add

- `FAT.InvalidDateYieldsZeroFileTime` (FAILS today)
- `FAT.InvalidMonthYieldsZeroFileTime` (FAILS today)
- `FAT.InvalidDayYieldsZeroFileTime` (FAILS today)
- `FAT.ValidDateYieldsCorrectFileTime` (regression guard)

## Related findings

- **S-011** — exFAT `tz_offset` applied (core) vs ignored (og). Both
  are FAT-family timestamp divergences. S-011 is SEVERE (visible
  hours-of-discrepancy); N-021 is MINOR (only invalid-date edge case).

## Open questions

- **OQ-S2-B-2:** is the FAT date=0 sentinel "date unknown" (og
  semantics: store 0) or "1970-01-01" (core semantics: store
  EPOCH_DELTA)? Win32 `DosDateTimeToFileTime` empirically fails
  (returns 0) for month=0/day=0 — the og relies on this. The core's
  behavior is an unintended side-effect of the early-return-0 in
  `fat_datetime_to_unix_seconds` and the caller's unconditional
  `+ EPOCH_DELTA`. Low priority; the fix plan above resolves it.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-021 fix commit — see `git log` for "fix: N-021")
**Verified by:** fail-before/pass-after executed this session: with only `cr_fat_quick.cpp` reverted, `FatEngine.InvalidDateYieldsZeroFileTime` (date 0x0000), `FatEngine.InvalidMonthYieldsZeroFileTime` (month 13 = 0x01A1) and `FatEngine.InvalidDayYieldsZeroFileTime` (day 0 = 0x0020) FAIL (core stored 116444736000000000 = 1970-01-01); `FatEngine.ValidDateYieldsCorrectFileTime` (0x5A46/0x6000 -> unix 1738843200) passes on both sides as the regression guard. Full suite 347/347 Linux ASan (343 baseline + 4 new). Corpus quote re-verified first-hand this session: `FATSearchDLL_decompiled.c:32135-32138` — `DosDateTimeToFileTime(...)` followed by `if (BVar5 == 0) *(u64*)(plVar7 + 0x25) = 0;` (the parallel modified-slot site at :32143+ carries the same shape).
**Resolution notes:** classification CONFIRMED-DIVERGENCE. The three timestamp assignments in `cr_fat_quick.cpp` now route through a `fat_to_filetime` lambda that returns the ZERO FileTime (1601-01-01 UTC) when `fat_date_to_unix` yields 0 — exactly the og's zero-on-conversion-failure branch (`DosDateTimeToFileTime` returns 0 for the invalid-date class, and the og zeroes the props slot). The 0-return maps 1:1 onto the failure class: a valid FAT date is >= 1980-01-01 = unix 315532800, so `fat_date_to_unix` returns 0 only for invalid dates or >2106 saturation (both are conversion failures in the og's sense). The exFAT sibling already carried the early-exit (`exfat_time_to_unix100`, aligned with S-011); the FAT engine now matches both. No other `fat_date_to_unix` call sites exist (verified by grep: the three in `cr_fat_quick.cpp` are exhaustive). The shared datetime core and the exFAT engine are unchanged; the valid-date path is bit-identical (regression guard pins it).
