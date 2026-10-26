# MAJOR M-014 — exFAT: og's structural entry gates are missing in the core (empty files suppressed, timestamp sanity, secondary-count cap, name validation)

**Severity:** MAJOR (false-positive surface; user-visible entry differences)
**Module:** exfat
**Parameters affected:** B3 (false positives), B4 (classification), B10 (names)
**Corpus references:** `exFATSearchDLL_decompiled.c` — `FUN_180002f90 :1787-1791` (secondary_count ∈ [2,18] via `(byte)(*param_1 - 2U) < 0x11`; create-timestamp sanity: month 1-12, day≠0, hour<24, min<60, sec<60), `FUN_180029280 :34562-34564` (stream accept requires `data_length != 0 && first_cluster != 0 && name_length != 0 && next-entry type&0x7F == 0x41` — **zero-length files are not emitted**), `FUN_180003030 :1814-1829` (name entry: flags&3==0, flags<4, first char ≠ 0, UTF-16 validity table), `FUN_180003090 :1840-1846` (volume-label/root triple placement `{0x83@0, 0x81@+0x20, 0x82@+0x40}`)
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:278-345` (type classes, stream reserved-zero, `fc_invalid` only) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og enforces a set of cheap structural gates on exFAT directory entries that the core lacks entirely: secondary-count bounds (2..18), per-timestamp field sanity (month/day/hour/minute/second ranges), stream fields all non-zero (so **empty files are dropped from results**), name-entry flag constraints and UTF-16 validity, and volume-label placement. The core's accept-set is wider, so garbage-adjacent entries the og rejects appear in core results.

## What the reference does

Quotes above: `FUN_180002f90` bounds the set and validates the create and modify timestamps field-by-field; `FUN_180029280` refuses streams with zero length/cluster/name; `FUN_180003030` validates the name entry; the root triple placement is checked in `FUN_180003090`.

## What our core does

`cr_exfat_quick.cpp:278-345` dispatches on entry-type classes, checks stream reserved-zero fields and `fc_invalid` (first cluster 0/1 or ≥ chain ceiling), but has none of the count/timestamp/length/name gates.

## Gap

Falsifiable claims: (a) an empty exFAT file (data_length=0) appears in core results, absent in og; (b) a dentry with month=0 garbage timestamps is skipped by og, emitted by core; (c) a set with secondary_count = 40 is rejected by og, walked by core (potential OOB-shaped walking on corrupt media); (d) a name entry with flags&3 != 0 is rejected by og, accepted by core.

## Impact

False-positive surface on corrupt/damaged media (the deep-scan scenario) and per-entry grading noise; (c) is also a robustness bound.

## Reproduction / verification

Decompile-verified (agent S2-B quotes; `evidence/decompiled-quotes/fat-exfat-session2.md`); core accept-set verified first-hand.

## Fix plan

1. In the exFAT entry-set walk, add the four gate families with the og's exact constants: secondary_count ∈ [2,18]; create+modify timestamp field sanity (treat all-zero as valid — the og skips only non-zero garbage); stream `data_length != 0 && first_cluster != 0 && name_length != 0`; name-entry `flags&3 == 0 && flags < 4 && first char != 0` + UTF-16 validity.
2. Decide the empty-file policy explicitly: og parity = suppress; register in `docs/parity/PARITY-DELTAS.md` if the core intentionally keeps them (then bench comparisons must filter).
3. Extend the structure-aware fuzz harness with mutants hitting each gate.

## Tests to add

- `ExFat.EmptyFileSuppressed` / `ExFat.GarbageTimestampRejected` / `ExFat.SecondaryCountCapped` / `ExFat.BadNameFlagsRejected` — all fail before, pass after.

## Related findings

- M-013 (checksum gates) · N-023 (tz and validator minors)

## Open questions

The `data_length != 0` gate also suppresses legitimate empty files (og behavior); confirm the product wants strict parity before inheriting the quirk — flagged in the fix plan.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-014 landing — DFX23, first commit of the session; SHA recorded in worklog.md at push)
**Verified by:** `ExfatEngine.EmptyFileSuppressedLikeOg`, `ExfatEngine.GarbageTimestampRejected`, `ExfatEngine.SecondaryCountCapped`, `ExfatEngine.BadNameFlagsRejected`, `ExfatEngine.FileReservedNonzeroRejected` (all five fail-before EXECUTED on the gate-neutralized pre-fix tree: RED with the gates disabled, GREEN after), `ExfatEngine.ValidBoundaryTimestampsStillPass` (boundary regression guard: Jan 1 00:00:00, Dec 31 23:59:58, 2sec=29 all pass). Suite 743/743 linux-release (+6 tests); ASan lane 739/743 with the 4 pre-existing LSan flags verified identical at the baseline tree (the N-048 sink's documented immortal allocation — environment-only; CI runners disable LSan). The scan-level fuzz harness (fuzz/fuzz_exfat.cpp) carries the gates in its fuzzed surface via scan_exfat.
**Resolution notes:** Independently re-verified against the corpus before fixing: FUN_180002f90 (:1779-1802), FUN_180002f40 (:1762-1777), FUN_180029280 (:34562-34564), FUN_180003030 (:1811-1834), the per-entry switch (:25507-25560) and the FUN_180003090 root-triple branch (:1840-1866) — all read first-hand in exFATSearchDLL_decompiled.c, not from the finding text. Four gate families landed, decompile-exact: (1) file-dentry structural gates — secondary_count ∈ [2,18] (`(u8)(sc-2) < 0x11`), create+modify+access timestamp field sanity (day≠0, month∈[1,12], hour≤23, min≤59, 2sec≤29), reserved1 (u16 +6) == 0, reserved2 (+0x19..+0x1F) == 0 — applied to live, deleted AND oracle re-walked sets, BEFORE set completion (the og's case order); a failing head is skipped without parking. (2) stream emission gates (FUN_180029280) — data_length, first_cluster, name_length all nonzero: zero-length files are never emitted (the og's documented quirk is INHERITED as parity — an empty file carries no recoverable content and never appeared in og results; the next-entry-is-name half was already enforced by the name-assembly path). (3) name-fragment gates (FUN_180003030) — flags&3==0, flags<4, first needed fragment's first char nonzero; the og's uVar6≥5 late-fragment tolerance only covers padding fragments beyond the needed count, which this walk never consumes (conservative reject documented here). (4) the M-013 oracle's declared-set bound tightened from 512 to 19 (parked heads pass the sc gate before parking, so declared ∈ [3,19] — a wider candidate could never re-pass the gates). New stat `ExfatScanStats::sets_gated`. CORRECTION to the finding's fix plan, corpus-wins: the timestamp sanity applies to ALL THREE timestamps (create/modify/access — the finding quoted only create/modify), and an all-zero timestamp FAILS (day==0), refuting the fix-plan guess "treat all-zero as valid". OUT OF SCOPE, documented: (a) the 0xHH00 UTF-16 validity table (PTR_180045a00) — its bytes are not recoverable from the corpus (no raw binary, no .rdata dump in any artifact); the sub-check stays unimplemented rather than guessed (the residual FP surface is names containing 0xHH00-form chars — rare); (b) the root-triple placement check (0x83@0, 0x81@+0x20, 0x82@+0x40, FUN_180003090's first branch) — decoded and confirmed, but it is an empty-directory RECOGNITION signal in the og (runs when a walk found no entries of any type), not an emission gate; the core's tree model never fabricates empty roots, so there is no divergent emission behavior to fix. Fixture truth updated: six planted sets across four tests carried TS=0 stamps (the og rejects those sets — the fixtures encoded the refuted "zero stamp passes" premise); they now stamp 2025-02-06 12:00 like real media. No safety hardening touched.
