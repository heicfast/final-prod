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
