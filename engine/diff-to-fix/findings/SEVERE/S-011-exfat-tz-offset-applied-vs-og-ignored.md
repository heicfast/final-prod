# SEVERE S-011 — exFAT tz_offset applied (core) vs ignored (og); FileTime diverges by hours

**Severity:** SEVERE
**Module:** exfat
**Parameters affected:** B9 (timestamps — UTC/local semantics), D11 (node fields — modified/created/accessed slots), B10 (Unicode/time display), Q68 (documentation accuracy — `cr_exfat_wire.h` claims "tz = 7-bit two's complement quarter-hours (0x00=unused)" but the application semantics are not documented)
**Corpus references:** `re-workspace/reversed/exFATSearchDLL.dll/rabin2_imports.txt` (NO `LocalFileTimeToFileTime`/`FileTimeToLocalFileTime`/`GetTimeZoneInformation`/`FileTimeToSystemTime` imported — engine does zero TZ conversion); `re-workspace/reversed/exFATSearchDLL/exFATSearchDLL_decompiled.c:5215-5235` (three `DosDateTimeToFileTime` calls — date+time words only; tz bytes never read); parallel site at `:45174-45194` (same pattern); `re-workspace/artifacts/SearchDLL/exfat-engine--deep-dive.md` (does not document tz handling); `re-workspace/artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md` §6 (silent on tz_offset)
**Core references:** `include/cr/exfat/cr_exfat_wire.h:70-74` (`create_tz`/`modify_tz`/`access_tz` wire fields at +0x16/+0x17/+0x18); `src/core/exfat/cr_exfat_quick.cpp:121-139` (`exfat_time_to_unix100(ts, tz)` — subtracts `tz_secs` from local-time-on-disk seconds); `:382-385` (caller passes `fe->create_tz`/`modify_tz`/`access_tz` per timestamp) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

exFAT stores each file's create/modify/access timestamps as a local
time on disk PLUS a per-timestamp `UtcOffset` byte (7-bit two's-complement
quarter-hours from UTC). The og's `exFATSearchDLL` ignores the offset
entirely — it calls `DosDateTimeToFileTime` on the raw date+time words
and stores the result as a Win32 `FILETIME` (interpreted as UTC). The
core's `exfat_time_to_unix100` reads the `tz` byte and SUBTRACTS the
offset from the local-time-on-disk seconds to produce a true UTC
`FILETIME`. The two diverge by exactly `tz_offset × 900 × 10⁷` (100ns
units) — for a UTC+8 file that's 28,800,000,000 (8 hours). Every
non-UTC exFAT volume's `.rsf` byte stream differs from the og's; users
recovering the same volume and opening the .rsf in DRWUI see timestamps
that match Explorer for the og (Explorer also uses local-as-UTC for
exFAT) but for the core the displayed time is N hours earlier.

## What the reference does

The og's `exFATSearchDLL.dll` imports NONE of:

- `LocalFileTimeToFileTime` (local → UTC)
- `FileTimeToLocalFileTime` (UTC → local)
- `GetTimeZoneInformation` (host timezone query)
- `FileTimeToSystemTime` / `SystemTimeToFileTime` (the Win32 time
  conversion APIs)

(Verified by full read of `rabin2_imports.txt` for `exFATSearchDLL.dll`.)

The og's `FUN_180001370` (file-entry parser) at
`exFATSearchDLL_decompiled.c:5215-5235` does three `DosDateTimeToFileTime`
calls:

```c
// :5215-5235 (and parallel :45174-45194)
BVar8 = DosDateTimeToFileTime(*(WORD*)((longlong)plVar18 + 0x22),  // create_date @ +0x10
                              *(WORD*)(plVar18 + 4),                // create_time @ +0x0E
                              (LPFILETIME)&DAT_180061278);
if (BVar8 == 0) DAT_180061278 = 0;
*(undefined8*)((longlong)plVar13 + 0x34) = DAT_180061278;          // props.create @ +0x34
// (same shape for modify @ +0x3C and access @ +0x44)
```

`DosDateTimeToFileTime(date, time, &ft)` interprets `date`/`time` as
**local** time and produces a `FILETIME` containing the UTC-equivalent.
But the function uses the **host's** current timezone to do the
conversion (`GetTimeZoneInformation` is called internally by
`DosDateTimeToFileTime`). So if the og is run on a UTC+8 host, a file
written at 14:00 local on a UTC+8 exFAT volume produces a `FILETIME` of
06:00 UTC — which is then stored in the .rsf as if it were the file's
UTC time. When DRWUI later displays this `FILETIME` it converts UTC →
local for display, getting back 14:00 (correctly). When the same volume
is scanned by the og on a UTC-5 host, the same file produces a `FILETIME`
of 19:00 UTC (because `DosDateTimeToFileTime` on a UTC-5 host treats the
local-time-on-disk as if it were UTC-5 local). The og's stored `FILETIME`
depends on the SCANNING HOST's timezone — a behavioral bug, but it's
the og's behavior and matches Explorer's "local-as-UTC" exFAT display
on the same host.

The tz bytes at `+0x16/+0x17/+0x18` of the FileEntry (`+0x2E/+0x2F/+0x30`
in `plVar18`-space, `+0x26/+0x27/+0x28` in `param_2`-space) are NEVER
read by any code path in `exFATSearchDLL.dll` (full grep returns only
the `FatFileEntry` struct's tz field reads in the wire header, not in
the engine).

## What our core does

```cpp
// include/cr/exfat/cr_exfat_wire.h:70-74 (audited 5d6da5f)
u8  create_tz;          // +0x16
u8  modify_tz;          // +0x17
u8  access_tz;          // +0x18

// src/core/exfat/cr_exfat_quick.cpp:121-139 (audited 5d6da5f)
u64 exfat_time_to_unix100(u32 ts, u8 tz) {
    if (ts == 0) return 0;
    const u64 base_secs = u64(ts);             // local-time-on-disk seconds since 1970-01-01
    i64 tz_secs = 0;
    if (tz != 0x00) {
        i32 q = i32(tz & 0x7F);
        if (q >= 64) q -= 128;                  // 7-bit two's complement
        tz_secs = i64(q) * 900;                 // quarter-hours → seconds
    }
    return base_secs * 10000000ULL + 116444736000000000ULL - u64(tz_secs) * 10000000ULL;
}

// :382-385 (caller)
props->times.created  = exfat_time_to_unix100(fe->create_timestamp,  fe->create_tz);
props->times.modified = exfat_time_to_unix100(fe->modify_timestamp,  fe->modify_tz);
props->times.accessed = exfat_time_to_unix100(fe->access_timestamp,  fe->access_tz);
```

The core's interpretation is **technically correct per the exFAT spec**
(ELM-Chan SDCFF §Standard File Entry: "UtcOffset: 7-bit two's-complement
offset from UTC in 15-minute increments; 0 means offset not specified")
— it produces a true UTC `FILETIME` regardless of the scanning host's
timezone. But it does NOT match the og's local-as-UTC behavior.

## Gap

The og stores exFAT timestamps as the local-time-on-disk interpreted
through the scanning host's timezone (via `DosDateTimeToFileTime`'s
implicit `GetTimeZoneInformation`). The core stores them as true UTC
computed from the on-disk `tz_offset` field. The two diverge by exactly
`tz_offset × 900 × 10⁷` (100ns units) for any file whose `tz_offset` ≠
0 (i.e. any file written on a non-UTC host that bothered to set the
field — most consumer cameras and SD-card devices set it).

## Impact

- **`.rsf` byte-level divergence:** every exFAT file with a non-zero
  `tz_offset` has a different `created`/`modified`/`accessed` u64 in the
  .rsf. Comparative inventories (og vs core on the same volume)
  diverge mechanically for every non-UTC file.
- **User-visible time discrepancy:** a user recovering a volume on
  which files were written at 14:00 local UTC+8 (with `tz_offset =
  0x88` = 8 quarter-hours east = UTC+2 hours, or `0x20` = 8 hours
  east) sees:
  - **Og (run on UTC+8 host):** displayed time = 14:00 (the local
    time the file was written; matches Explorer on the same host).
  - **Core (run anywhere):** stored UTC = 06:00; displayed on UTC+8
    host = 14:00 (matches the og's display on the same host!).
  - **Core (run on UTC-5 host):** displayed time = 01:00 (the previous
    day). The og's display on UTC-5 host would be 19:00 (also wrong,
    but in the other direction).
  - **Cross-host .rsf portability:** if a user scans on UTC+8 host,
  takes the .rsf to a UTC-5 host, and recovers — the og's .rsf has
  host-dependent timestamps; the core's .rsf has host-independent
  UTC timestamps. The core is BETTER here, but it's not the og's
  behavior.
- **Parity benchmark:** any og-vs-core file inventory comparison fails
  on every non-UTC exFAT file.
- **Spec compliance:** the core is technically MORE correct than the og
  (the exFAT spec REQUIRES the `tz_offset` to be applied). This is a
  SUPERSET divergence in correctness but a DIVERGENCE in parity.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `exFATSearchDLL_decompiled.c:5215-5235` and `:45174-45194` — three
  `DosDateTimeToFileTime` calls each, no tz-byte read in either site.
- **Imports evidence:** first-hand read of
  `exFATSearchDLL.dll/rabin2_imports.txt` — no time-conversion APIs
  imported.
- **Core evidence:** first-hand read of `cr_exfat_quick.cpp:121-139`
  and `:382-385`. The subtraction at `:138` (`- u64(tz_secs) *
  10000000ULL`) is the divergent line.
- **Executed comparison:** not run — would require building both
  implementations and comparing .rsf byte streams. Tracked as a bench
  scenario (X47-X53 family).

## Fix plan (for the engineering team)

The decision is: match the og's local-as-UTC behavior (parity) or keep
the spec-correct UTC behavior (superset). Recommendation: **parity**,
because the audit's purpose is byte-equivalence with the og's .rsf
output.

1. **Architectural change:** none — the fix is a one-line removal of
   the tz subtraction in `exfat_time_to_unix100`.
2. **Files to modify:**
   - `src/core/exfat/cr_exfat_quick.cpp:121-139` — change
     `exfat_time_to_unix100(ts, tz)` to ignore the `tz` parameter
     (or remove it entirely and update the call site). The function
     becomes `exfat_time_to_unix100(ts)` returning `base_secs *
     10000000ULL + 116444736000000000ULL` (i.e. local-on-disk as UTC).
   - `src/core/exfat/cr_exfat_quick.cpp:382-385` — drop the `tz`
     argument from the three call sites.
   - `include/cr/exfat/cr_exfat_wire.h:70-74` — keep the wire fields
     (they're parsed for completeness) but add a comment noting the
     og ignores them.
3. **Algorithm/constant:** none — the change is removal of the
   subtraction at `:138`.
4. **Tests to add:**
   - `Exfat.TimestampsMatchOgLocalAsUtc` — fixture: an exFAT volume
     with `tz_offset = 0x88` (UTC+2 hours) and a file written at
     14:00 local. Assert the stored `FILETIME` equals `14:00` (as if
     it were UTC), NOT `12:00` (true UTC). This test FAILS today
     (core stores `12:00`).
   - `Exfat.TimestampsZeroTzPassThrough` — fixture: `tz_offset = 0`
     (the spec's "offset not specified" sentinel) — both og and
     core pass through unchanged. This test PASSES today.
   - `Exfat.TimestampsMatchExplorerDisplay` — fixture: an exFAT
     volume scanned on a UTC+8 host with `tz_offset = 0x88`; the
     stored `FILETIME` should display as 14:00 on the scanning host
     (matching Explorer's exFAT display). This test FAILS today.
5. **Contract/doc updates:** `docs/AUDIT-FALLBACKS.md` exFAT section
   should note that the engine matches the og's local-as-UTC behavior
   (the spec-correct UTC interpretation is deliberately NOT applied
   for parity reasons).
6. **Acceptance criteria:**
   - The three tests above pass on the core.
   - A byte-level .rsf comparison between og and core on the same
     exFAT fixture with `tz_offset ≠ 0` shows identical timestamp
     fields.
   - Cross-host .rsf portability is documented as "matches og
     behavior" (host-dependent timestamps).

## Tests to add

- `Exfat.TimestampsMatchOgLocalAsUtc` (FAILS today)
- `Exfat.TimestampsZeroTzPassThrough` (PASSES today — regression guard)
- `Exfat.TimestampsMatchExplorerDisplay` (FAILS today)

## Related findings

- **S-005** (FIXED) — filename mojibake was the parallel byte-lift
  divergence on ext4/Btrfs/ZFS. S-011 is the timestamp equivalent
  on exFAT: the og passes local-time-on-disk through unconverted; the
  core applies spec-correct conversion that the og doesn't.
- **N-010** — FAT invalid-date yields non-zero FileTime in core, zero
  in og. Both are timestamp-handling divergences on the FAT family.

## Open questions

- **OQ-S2-B-2:** is the FAT date=0 sentinel "date unknown" (og
  semantics: store 0) or "1970-01-01" (core semantics: store
  EPOCH_DELTA)? This is the parallel question for FAT — resolved
  separately in N-010.
- **Should we expose a "spec-correct UTC" mode as opt-in?** The audit
  doesn't take a position — engineering decides. A `--strict-utc`
  flag could be the escape hatch for users who want spec-correct
  timestamps at the cost of parity.

## Session-2 main-auditor cross-confirmation (2026-10-25)

Independently confirmed from a separate sweep of `exFATSearchDLL_decompiled.c`: zero
`*900`/`*0x384`/`*3600` arithmetic patterns anywhere in the engine (the tz bytes at dentry
+0x16..0x18 are never read; the validators only sanity-check the timestamp fields —
`FUN_180002f90 :1787-91`). The DRWUI-side conversion possibility this finding already flags
remains the one open out (cross-listed in OQ-S2M-2 as a sibling probe). Adjacent gates from
the same read are filed in N-023 (dentry date sanity, BPB string/bps/spc sets, reserved-field
tests, mid-chain FREE/BAD handling) and M-014 (exFAT structural entry gates).
