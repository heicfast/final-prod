# ACUTE A-014 — FAT time-word validation: the core accepts hour≥24 / minute≥60 / 2s-field≥30 and fabricates shifted plausible timestamps; the og zeroes the slot

**Severity:** ACUTE
**Module:** fat (datetime decode)
**Parameters affected:** time and timestamp handling (9), recovery correctness (2), detection correctness (1) — wrong metadata in output data
**Corpus references:** `FATSearchDLL_decompiled.c:32131-32154` (the three `DosDateTimeToFileTime` calls + zero-on-fail slots); exFAT twin `exFATSearchDLL_decompiled.c:45174-45194` (protected upstream in core by the M-014 gate)
**Core references:** `include/cr/fat/cr_fat_wire.h:115-142` (`fat_datetime_to_unix_seconds` — validates month/day only); consumers `src/core/fat/cr_lzn.cpp` (the FAT path stamps); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og converts each FAT dentry timestamp with the Win32 `DosDateTimeToFileTime`, which
FAILS on out-of-range components (hour>23, minute>59, second>59); on failure the og zeroes
the node's timestamp slot (honest "no date"). The core's shared FAT-family converter
validates only month and day — hour (5 bits, 0..31), minute (6 bits, 0..63) and the 2-second
field (5 bits, 0..31 → 0..62 s) flow unvalidated into the epoch math. A dentry with, e.g.,
creation time 0xC000 (hour bits = 24) yields, in the core, a timestamp shifted exactly one
day into the future — a plausible, WRONG value persisted into results, sessions, and
recovered-file metadata.

## What the reference does

`FATSearchDLL_decompiled.c:32131-32154` — for each of the three dentry timestamps
(create time@+0x0E/date@+0x10 → node+0x25; write time@+0x16/date@+0x18 → node+0x2D;
access date@+0x12/time=0 → node+0x35):

```c
32135  BVar5 = DosDateTimeToFileTime
32136                    ((WORD)local_584,local_588[0],(LPFILETIME)((longlong)plVar7 + 0x25));
32137  if (BVar5 == 0) {
32138    *(undefined8 *)((longlong)plVar7 + 0x25) = 0;
```

(the same zero-on-fail pattern at :32146-48 and :32152-54). `DosDateTimeToFileTime`
unpacks to SYSTEMTIME and rejects hour≥24, minute≥60, second≥59 (documented Win32
semantics; the decoder's own dentry gate family confirms the boundary set). A rejected
component → the slot becomes 0 (the 1601-01-01 zero FileTime the UI renders as no date).

## What our core does

`include/cr/fat/cr_fat_wire.h:123-142`:

```c
123  inline u64 fat_datetime_to_unix_seconds(u16 date, u16 time) {
124      const u32 year = 1980u + ((date >> 9) & 0x7F);
125      const u32 month = (date >> 5) & 0x0F;
126      const u32 day = date & 0x1F;
127      const u32 hour = (time >> 11) & 0x1F;
128      const u32 minute = (time >> 5) & 0x3F;
129      const u32 second = (time & 0x1F) * 2;
130      if (month < 1 || month > 12 || day < 1 || day > 31)
131          return 0;
...
141      return u64(days) * 86400 + u64(hour) * 3600 + u64(minute) * 60 + second;
142  }
```

Only month/day are gated. `hour` may be 24..31 (adding 86400..111600 s = 1 day +
0..7.5 h), `minute` 60..63 (adding up to 63 min), `second` 60..62 — all silently folded
into a plausible timestamp. The comment at :118-122 even documents "Returns seconds since
1970-01-01, 0 on invalid/out-of-range fields" — the implementation does not honor its own
contract for the time word. (The exFAT path is NOT affected: the M-014 dentry gate rejects
the whole timestamp set when hour>23 ∧ min>59 ∧ 2s>29 upstream of the shared helper.)

## Gap

For any FAT dentry whose time word has hour≥24, minute≥60, or 2s-field≥30, the og emits a
zero (invalid) timestamp and the core emits a shifted, plausible one. Falsifiable: input
`date=valid, time=0xC000` → og node timestamp slot 0; core timestamp = date+1 day 00:00.

## Impact

Wrong recovery RESULTS (metadata): every corrupt/garbage dentry — precisely the deleted
population recovery targets — gets a fabricated timestamp instead of an honest invalid
marker. The wrong value propagates to the results tree, query/sort (timestamp keys),
session persistence, and export. Users triage deleted files by date; a systematically
shifted date is silent misinformation. Same input class as the already-fixed N-021 guard
family (one-line boundary guards).

## Reproduction / verification

- Corpus-derived, verified first-hand this session: the three zero-on-fail slots read at
  FATSearchDLL_decompiled.c:32135-54 (quotes above).
- Core-derived, verified first-hand: cr_fat_wire.h:123-142 (quote above) — the hour/min/sec
  fields have no range gate; arithmetic checked: time=0xC000 → hour=24 → +86400 s.
- Worked example: dentry `E5 45 41 44 4D 45 ...` with creation date word 0x65C4 (valid)
  and creation time word 0xC000 → og: created = 0; core: created = (that date + 1 day).
- No existing test pins the invalid-time-word boundary (grep
  `hour\|minute` in test_fat*: only the month/day boundary tests exist, N-021 lineage).

## Fix plan (for the engineering team)

1. Extend the gate in `fat_datetime_to_unix_seconds` (cr_fat_wire.h:130) to the time
   components: `hour > 23 || minute > 59 || (time & 0x1F) > 29 → return 0` (the 2s field
   >29 means ≥60 s). This matches DosDateTimeToFileTime's accept set exactly.
2. File to modify: `include/cr/fat/cr_fat_wire.h` (one condition). No algorithm change —
   the shared helper serves both FAT and exFAT; exFAT behavior is unchanged because its
   M-014 gate already rejects the class upstream.
3. Tests: the boundary set below (all three fields, both edges).
4. Doc: the helper's comment (:118-122) becomes truthful after the fix; no API change.
5. Acceptance criteria: for every (date valid, time invalid) combination the node
   timestamp equals the zero FileTime, matching the og's slot-zeroing; for valid times
   output is unchanged (the existing FAT date tests must stay green).

## Tests to add

- `Fat.TimeWordHour24YieldsZeroTimestamp` (fails today: returns date+1d)
- `Fat.TimeWordHour31YieldsZeroTimestamp`
- `Fat.TimeWordMinute60YieldsZeroTimestamp`
- `Fat.TimeWordSecondField30YieldsZeroTimestamp` (2s field 30 = 60 s)
- `Fat.ValidTimeWordUnchanged` (regression: 23:59:58 still converts)
- Session-level: `Fat.InvalidTimeStampsPersistAsZero` (the zero flows to the record)

## Related findings

- N-021 lineage (the FAT date month/day guard family — this is the time-word sibling)
- N-173 (the 2107 date clamp — the same helper's year ceiling, distinct edge)
- LE-3 lane decode (this session's lane-E context sheet, evidence/s12-lane-sheets/)

## Open questions

None — the og's boundary set is fully decoded (three slots, zero-on-fail), and the core's
gap is a single missing condition.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S47 lane)
**Verified by:** `FatEngine.TimeWordOutOfRangeYieldsZeroTimestamp` (FAILED pre-fix, PASSES post-fix), `FatEngine.ValidTimeWordUpperEdgesStillConvert` (regression pin, PASSES both sides - no over-rejection), `FatEngine.InvalidTimeWordPersistsAsZeroFileTime` (end-to-end dentry: hour-24 word -> props.times.modified == 0); full suite 1206 tests / 1164 pass / 0 fail (85 remain Win-env skips). Corpus re-verified first-hand this session: FATSearchDLL_decompiled.c:32135/:32144/:32151 (ghidra-searchdll) - all three DosDateTimeToFileTime calls with the zero-on-fail branch.
**Resolution notes:** One gate extension in `fat_datetime_to_unix_seconds` (include/cr/fat/cr_fat_wire.h): the time-word components now honor the full DosDateTimeToFileTime accept set (hour<=23, minute<=59, 2s field<=29 i.e. second<=59) alongside the existing month/day gates, so an out-of-range time word yields 0 exactly like the og's zero-on-fail slot. Both consumers (FAT cr_lfn.cpp fat_date_to_unix, exFAT exfat_time_to_unix100) inherit the correction through the shared helper - the exFAT M-014 dentry gate only rejects the all-three-invalid class, so single-field overflows previously leaked to the same fabrication there and now match the og's per-word DosDateTimeToFileTime semantics. Two corrections to the finding's worked example recorded: the cited date word 0x65C4 decodes month 14 (invalid - the finding's "valid" label was wrong); the tests use 0x5904 (2024-08-04). No API change; the helper's own "0 on invalid/out-of-range fields" comment is now truthful for the time word.
