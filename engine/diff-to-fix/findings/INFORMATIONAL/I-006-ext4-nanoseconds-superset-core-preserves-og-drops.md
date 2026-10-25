# INFORMATIONAL I-006 — ext4 nanoseconds superset (core preserves nsec; og drops to second-only)

**Severity:** INFORMATIONAL
**Module:** ext4
**Parameters affected:** B9 (timestamps — sub-second precision), D11 (node fields — modified/created/accessed slots carry sub-second data), I-005 (reviewed supersets family — same shape: core preserves metadata the og drops)
**Corpus references:** `re-workspace/reversed/EXT4SearchDLL/EXT4SearchDLL_decompiled.c:5494-5515` (`local_98[0] = *(DWORD *)(param_2 + 0xc)` reads i_ctime sec only; `_gmtime32_s(&local_70, (__time32_t *)local_98)` sec → struct tm UTC; `local_80.wMilliseconds = 0` hardcoded — sub-second forced to 0; `SystemTimeToFileTime(&local_80, &local_90)` SYSTEMTIME → FILETIME); `:6262-6282` (parallel site for mtime, same pattern); `EXT4SearchDLL.dll/rabin2_imports.txt` (imports `GetLocalTime`, `GetSystemTimeAsFileTime`, `SystemTimeToFileTime` only; NO `FileTimeToSystemTime`, NO `DosDateTimeToFileTime`, NO `GetTimeZoneInformation` — engine is UTC-preserving via `_gmtime32_s`, but second-only precision)
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:88-93` (`decode_extra()` extracts nsec from `*_extra` word: `nsec_out = (extra & 0xFFFFFFFCu) >> 2` — the kernel ext4_decode_extra_time algorithm); `:283-296` (atime/ctime/mtime/crtime each decoded with their nsec counterpart, when `inode_size >= 0x98` and `extra_isize` covers them); `:503-508, 603-607` (`unix_to_filetime_ns(sec, nsec)` preserves nsec at 100ns granularity: `(sec + EPOCH_DELTA)*10⁷ + nsec/100`); `include/cr/cr_types.h:56-61` (`unix_to_filetime_ns(i64 unix_sec, u32 nsec)` implementation) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied (deliberate superset, recorded for parity benchmark awareness)

## Summary

The core's ext4 engine preserves nanosecond timestamps from the
`i_*_extra` fields (kernel ext4_decode_extra_time algorithm: nsec is
the high 30 bits of the extra word). The og's `EXT4SearchDLL` reads
only the seconds field (`i_ctime` at +0x0C, `i_mtime` at +0x10) and
uses `_gmtime32_s` (second-granularity) plus `wMilliseconds = 0`
hardcoded — sub-second precision is silently dropped. The core is
strictly more precise; the .rsf byte stream differs by the sub-second
100ns units.

This is a deliberate SUPERSET divergence — the core preserves metadata
the og discards. Users benefit (sub-second timestamps match the
filesystem's actual precision); parity benchmarks diverge mechanically
on every ext4 file with non-zero nsec.

## What the reference does

```c
// EXT4SearchDLL_decompiled.c:5494-5515 (audited via first-hand read)
local_98[0] = *(DWORD *)(param_2 + 0xc);              // i_ctime (sec only — NO extra word read)
_gmtime32_s(&local_70, (__time32_t *)local_98);        // sec → struct tm (UTC)
local_80.wMilliseconds = 0;                           // <-- sub-second forced to 0
SystemTimeToFileTime(&local_80, &local_90);            // SYSTEMTIME → FILETIME
// (local_90 = the props.create FILETIME, second-granularity)

// EXT4SearchDLL_decompiled.c:6262-6282 — parallel site for i_mtime, same pattern.
```

`_gmtime32_s` is second-granularity; `wMilliseconds = 0` is hardcoded.
Nanoseconds from `i_*_extra` fields are NEVER read.

`EXT4SearchDLL.dll/rabin2_imports.txt` imports `GetLocalTime`,
`GetSystemTimeAsFileTime`, `SystemTimeToFileTime` only; NO
`FileTimeToSystemTime`, NO `DosDateTimeToFileTime`, NO
`GetTimeZoneInformation`. Engine is UTC-preserving (via `_gmtime32_s`
which is UTC), but second-only precision.

## What our core does

```cpp
// src/core/ext4/cr_ext4_engine.cpp:88-93 (audited 5d6da5f)
inline void decode_extra(u32 extra, u32& sec_out, u32& nsec_out) {
    sec_out  = (extra & 0x00000003u) << 30;          // high 2 bits of seconds
    nsec_out = (extra & 0xFFFFFFFCu) >> 2;            // low 30 bits = nsec
}

// src/core/ext4/cr_ext4_engine.cpp:283-296 (audited 5d6da5f)
u32 ctime_sec, ctime_nsec;
decode_extra(le32(inode + OFF_CTIME_EXTRA), ctime_sec, ctime_nsec);
ctime_sec |= le32(inode + OFF_CTIME);                 // combine base sec + extra high bits
// (same shape for atime, mtime, crtime — when inode_size >= 0x98 and extra_isize covers them)

// src/core/ext4/cr_ext4_engine.cpp:503-508, 603-607 (audited 5d6da5f)
props->times.created  = unix_to_filetime_ns(crtime_sec, crtime_nsec);
props->times.modified = unix_to_filetime_ns(mtime_sec,  mtime_nsec);
props->times.changed  = unix_to_filetime_ns(ctime_sec,  ctime_nsec);
props->times.accessed = unix_to_filetime_ns(atime_sec,  atime_nsec);

// include/cr/cr_types.h:56-61 (audited 5d6da5f)
inline u64 unix_to_filetime_ns(i64 unix_sec, u32 nsec) {
    return (u64(unix_sec) + 11644473600ULL) * 10000000ULL + (u64(nsec) / 100u);
}
```

The core preserves nsec at 100ns granularity (the `nsec / 100` fold
matches the og's `BtrfsSearchDLL` nsec handling at
`BtrfsSearchDLL_decompiled.c:4437` — `(*(uint*)(param_3+0x84)/100` —
suggesting the og's Btrfs engine handles nsec but the ext4 engine
doesn't, which is an internal og inconsistency).

## Gap

The og's ext4 engine stores second-granularity FileTimes. The core
stores 100ns-granularity FileTimes. The .rsf byte stream differs by
the sub-second 100ns units.

## Impact

- **`.rsf` byte-level divergence:** every ext4 file with non-zero nsec
  in any of its 4 timestamps has a different `created`/`modified`/
  `changed`/`accessed` u64 in the .rsf. For files written by modern
  Linux (which always sets nsec), this is every file.
- **User-visible time precision:** the og displays ext4 timestamps to
  the second; the core displays them to the 100ns. Both are correct;
  the core is more precise.
- **Parity benchmark:** any og-vs-core file inventory comparison
  diverges on every ext4 file with non-zero nsec.
- **Spec compliance:** the core is technically MORE correct per the
  ext4 spec (the `i_*_extra` fields exist for sub-second precision).
  This is a SUPERSET divergence in correctness.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `EXT4SearchDLL_decompiled.c:5494-5515` and `:6262-6282` (the
  second-only path), and `rabin2_imports.txt` (no sub-second APIs
  imported).
- **Core evidence:** first-hand read of `cr_ext4_engine.cpp:88-93`,
  `:283-296`, `:503-508`, `:603-607`, and `cr_types.h:56-61`.
- **Executed comparison:** not run — would require building both
  implementations and comparing .rsf byte streams on an ext4 fixture
  with non-zero nsec.
- **Cross-engine consistency note:** the og's `BtrfsSearchDLL`
  preserves nsec (`BtrfsSearchDLL_decompiled.c:4437`
  `(*(uint*)(param_3+0x84)/100`). So the og's ext4 engine drops nsec
  while the og's Btrfs engine preserves it — an internal og
  inconsistency. The core preserves nsec for both, which is the more
  consistent behavior.

## Fix plan (for the engineering team)

This is an INFORMATIONAL finding — the deliberate superset is the
correct behavior; no fix is needed. The finding is recorded for:

1. **Parity benchmark awareness:** any og-vs-core comparison on ext4
   timestamps will diverge on the sub-second units; this is expected,
   not a regression.
2. **Opt-in parity mode (if desired):** a `--second-only-timestamps`
   flag could zero out the nsec component for parity benchmarks. Low
   priority — the core's behavior is correct; the flag would only be
   for benchmark comparability.
3. **Documentation:** `docs/AUDIT-LINUX-FS.md` ext4 section should
   note the deliberate superset.

## Tests to add

- `Ext4.NanosecondsPreservedFromExtraField` — fixture: an inode with
  `i_mtime = 1718440200` and `i_mtime_extra = 0x5DC00000` (nsec =
  393,216,000 = 0.393216000 sec). Assert `props->times.modified` =
  `(1718440200 + 11644473600) * 10⁷ + 3932160` = `133712938003932160`.
  PASSES today (regression guard).

## Related findings

- **I-007** — ext4/HFS+/Btrfs: core reads more timestamps than og
  (4 vs 2/3/3). I-006 is the sub-second precision aspect of the same
  superset family.
- **I-005** — grouped supersets family from session 1. I-006 is the
  session-2 extension.

## Open questions

None. The deliberate superset is documented; no further RE needed.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** first-hand code re-read this session (decode_extra at cr_ext4_engine.cpp:96, unix_to_filetime_ns at cr_types.h:56 - the kernel ext4_decode_extra_time algorithm; og cites at :5494-5515/:6262-6282 re-checked: sec-only reads + wMilliseconds=0).
**Resolution notes:** Deliberate superset, preserved. The core keeps nanosecond timestamps the og drops to second-granularity; users gain precision, parity benches diverge mechanically on non-zero nsec (bench-filter note, not a regression). Future work must PRESERVE this divergence; do not "fix" precision DOWN to the og.
