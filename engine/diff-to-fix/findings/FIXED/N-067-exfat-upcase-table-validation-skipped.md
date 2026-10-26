# MINOR N-067 — exFAT upcase-table validation skipped (TYPE_UPCASE bypassed entirely)

**Severity:** MINOR
**Module:** exfat
**Parameters affected:** B1 (detection correctness — volume validation), A25 (engine algorithm steps), S35 (error recovery — corrupt-volume behavior)
**Corpus references:** `re-workspace/artifacts/SearchDLL/fat-exfat-engine-internals.md:63-64` ("Up-case table / integrity: volume validation via boot + upcase-table checks (`CExFatVolumeAnalyer`), consistent with exFAT structures (Boot, FAT, Bitmap, Up-case, Log)")
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:319-321` (`if (t == TYPE_UPCASE || t == TYPE_VOLUME_LABEL) { ++i; continue; /* not results */ }` — the upcase table is skipped without any validation)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's exFAT volume analyzer validates the upcase table as part of volume integrity (boot + upcase checks via `CExFatVolumeAnalyer`). The core's directory-structure walk skips `TYPE_UPCASE` entries with a bare `continue` — no checksum, no length, no content validation — so a corrupted upcase table is silently accepted.

## What the reference does

fat-exfat-engine-internals.md:63-64 documents the exFAT volume-validation pass as "boot + upcase-table checks" through `CExFatVolumeAnalyer` — the upcase table is one of the five structural streams (Boot, FAT, Bitmap, Up-case, Log) the og validates for volume consistency.

## What our core does

The FAT-region walk at cr_exfat_quick.cpp:319-321 skips TYPE_UPCASE (and TYPE_VOLUME_LABEL) as "not results" — correct for tree-building purposes, but no validation of the upcase table happens anywhere else in the engine either (grep: TYPE_UPCASE appears only at this skip site).

## Gap

Falsifiable claim: an exFAT volume whose upcase-table checksum field disagrees with its content (or whose dentry upcase checksum is stale) is flagged/distrusted by the og's volume analyzer; the core accepts the volume without any upcase check.

## Impact

Detection-strictness divergence on a corruption class that matters mainly as a signal (an invalid upcase table suggests wider corruption or a torn format); filename case-folding is not performed by either recovery path, so no wrong-name risk. The practical effect is a missing integrity signal, not wrong output.

## Reproduction / verification

Corpus-derived (artifact first-cited this session; the CExFatVolumeAnalyer validation quote above). Core side read in-source at the audited commit.

## Fix plan (for the engineering team)

1. In the FAT-region walk (cr_exfat_quick.cpp:319), when `t == TYPE_UPCASE`: read the dentry's table-checksum field + the table stream; validate length (exFAT: exactly 128 sectors / 65536 bytes unless the count field says otherwise is NOT valid — full table is mandatory in spec ≥1.00... in practice validate the dentry's own checksum against the stream per the exFAT entry-set checksum algorithm, which the engine already implements for file sets — S-013/N-003 family).
2. Record a soft integrity flag (volume validation verdict) rather than rejecting the volume — the og's analyzer grades, it does not hard-fail (the recovery path continues on corrupt volumes; see the EXFAT-2 salvage lineage).
3. Tests: `ExFAT.UpcaseTableChecksumValidated` (corrupt checksum → flag set, volume still scanned), `ExFAT.UpcaseLengthSanity`.
4. Acceptance: the integrity flag appears in scan telemetry for a corrupted-upcase fixture; normal volumes unaffected.

## Tests to add

- `ExFAT.UpcaseTableChecksumValidated` — fails today (no check)
- `ExFAT.UpcaseLengthSanity`

## Related findings

N-023 (FIXED — the exFAT gate minors group: same boot/gate family, this is the residual upcase item); M-013/M-014 (entry-set checksum/structural gates — the same checksum algorithm applied to file sets).

## Open questions

Whether the og's upcase validation affects scan RESULTS (reject vs. grade) is not fully decoded — `CExFatVolumeAnalyer`'s failure path is not in the artifact; the fix plan's grade-not-reject default follows the og's general corrupt-volume posture (EXFAT-2 salvage) and can be tightened if a later decode shows hard rejection.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-067 commit)
**Verified by:** `ExfatEngine.UpcaseTableChecksumValidated` (clean fixture: validated >= 1, mismatches == 0; corrupt stamp: validated == 0, mismatches == 1, scan OK, files still found — grade-not-reject pinned) + `ExfatEngine.UpcaseLengthSanity` (zero length flags; absurd 4 GiB length flags WITHOUT any stream access — no hang, no over-read, scan OK). Fail-before executed: both tests RED on the tree with the stats surface but the validation unwired (upcase_validated == 0, no flag). Full suite 597/597 linux-gcc ASan; exFAT/diskmap subset 42/42 linux-ubsan clean; lint clean.
**Resolution notes:** implemented per the fix plan with the og's own shape evidence folded in: at the in-use upcase dentry (TYPE 0x02 / on-disk 0x82 — see the S-033 INVALID record for the constant verification that preceded this fix) the engine now (a) applies the og's dentry shape gates (reserved1@+1..+3 + reserved2@+8..+0x13 zero-checked, TableChecksum@+4 free — the og's case -0x7e walk validation, decompile :24440-24452), (b) requires first_cluster >= 2, (c) applies the length sanity bar (zero or > 1 MiB or > device size → flagged without stream access; the cap is OUR honest bound — real tables are 5836/14942 bytes — the og's own gate only requires the length to fit the volume), (d) reads the table stream chain-follow with the contiguous fallback (the bitmap branch's contract), bounded 64 KiB incremental `checksum32` chunks (the shared ROL31+add — same algorithm as the boot region and entry sets), and (e) compares against TableChecksum. Verdict surfaces in `ExfatScanStats.upcase_validated` / `.upcase_mismatches` — a soft integrity signal: the scan NEVER stops on a corrupt table (the og's corrupt-volume posture, EXFAT-2 salvage; the finding's open question on the og's exact failure path is preserved — the grade-not-reject default stands). The fixture now carries a REAL 64-entry upcase table at cluster 60 (a-z folded, checksum32-stamped, FAT-EOC + bitmap-marked, the hardening/producer tests' cluster lists updated); the deleted-upcase case is naturally excluded (in-use gate, mirroring the bitmap branch). One test-expectation correction during the pass: a corrupt table must NOT count in `upcase_validated` (only clean tables validate).

## Resolution addendum (session-10 convergence — the empirical algorithm pin)

A second lane independently implemented this fix in parallel with a
DIFFERENT checksum interpretation: the MS spec's seeded CRC-32 (seed =
table length). Both lanes raced to push; the union kept this landing's
ROL31+add checksum32. The dispute was settled EMPIRICALLY, not by
spec-memory: `mkfs.exfat` (exfatprogs 1.2.9) was run on a fresh image and
the real upcase dentry decoded — TableChecksum 0xE619D30D over the
5836-byte spec table equals ROL31+add with init 0 (the boot-checksum
family), NOT the seeded CRC (0x673C42A0) and NOT length-seeded ROL31
(0x52D9D30F). exfatprogs images interoperate with Windows, so this is the
algorithm real volume writers use. The wrong interpretation was reverted
with its helper (crc32_seeded); the real table is committed as
tests/data/exfat/upcase-table-mkfs-exfat-1.2.9.bin and pinned by
`ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat` — the S-030 lesson
applied: real-tool fixtures catch spec-memory bugs self-built fixtures
cannot.
