# MINOR N-187 — Lost-scan NTFS candidate acceptance is weaker in the core: the og requires boot-pair OR 4-record-MFT validation (with a mirror retry) before accepting a candidate; the core's probe never rejects on MFT evidence (scoring only, no mirror retry)

**Severity:** MINOR
**Module:** lost-partition (NTFS candidate acceptance)
**Parameters affected:** A30 (candidate scoring), B3 (false-positive rate — lost-list noise), A26 (probe bounds)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` FUN_180011150 (:9880-9914 — the candidate classifier: `FUN_180011260(boot-pair) == 0` → try `FUN_180011560` (4-record MFT read + validate + MIRROR RETRY at FUN_180011b30's +0x38 getter), both failing → the 0xfffffff1 reject slot) and FUN_180011560 (:10023-10075 — `FUN_180011a10(param_4) << 2` = 4 × record_size bytes read; `FUN_180011b10`(mft_lcn)… on failure `FUN_180011b30`(mirr_lcn) retry), both re-read first-hand this session
**Core references:** audited `34d6cf10` — `src/core/lost/cr_candidates.cpp:70-122` (`probe_ntfs`: the MFT record-0 probe adds +30 on success but never rejects on failure; no mirror retry; no boot-pair check)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's lost-scan NTFS candidate validator requires positive evidence before accepting: a boot-pair cross-check (a second NTFS boot at candidateLBA − total_sectors with matching fields) OR a validated 4-record MFT read — retried at the $MFTMirr LCN on failure — and rejects with 0xfffffff1 when both fail. The core's probe scores MFT evidence (+30) but accepts the candidate regardless (OEM + plausible BPB suffice); a forged OEM sector surfaces as a low-scoring core candidate the og would reject, and a torn-primary-MFT candidate scores lower in the core despite the og's mirror-validated acceptance.

## What the reference does

FUN_180011150 (:9889-9906): `iVar1 = FUN_180011260(…)` (the boot-pair check); on 0 → `FUN_180011560(…)`; both 0 → the reject slot. FUN_180011560 (:10023-10075) reads `record_size << 2` bytes (4 MFT records) at `mft_lcn × spc + base`, validates via FUN_180011440, and on failure retries the identical read at the mirror LCN (`FUN_180011b30` returns the +0x38 qword — the $MFTMirr LCN, the same getter family verified for S-063) before failing.

## What our core does

`probe_ntfs` (:70-122): OEM → +15; `validate_boot_sector` OK → +25 and a record-0 read whose success adds +30; every failure path keeps the candidate (the else branch even fabricates a device-end extent with an honesty flag). No mirror retry, no boot-pair evidence.

## Gap

A garbage sector with a forged "NTFS    " OEM and plausible BPB: og rejects (no boot pair, no valid 4-record MFT); core surfaces a candidate (which then fails at engine open — noise in the lost list and the scoring table). A torn-primary-MFT candidate with intact mirror: og accepts via the mirror-validated branch; core scores it lower (no +30) — ranking divergence.

## Impact

Lost-list noise and score differences; no wrong-content risk (the engine still validates). MINOR by the board's calibration (the same class as the scoring-evidence findings).

## Reproduction / verification

Decompiled evidence, lead-verified first-hand: both functions read this session (FUN_180011560's 4-record read, the FUN_180011440 validation call, and the mirror retry at :10052-10058 quoted in the S-063 verification trail). Core side: the probe quoted at `34d6cf10`. The boot-pair function FUN_180011260's internal field-matching set was not fully decoded (the three-slot variant record at FUN_180011150's tail is register-opaque) — recorded in Open questions.

## Fix plan (for the engineering team)

1. Make the record-0 probe a soft requirement: reject (return -100) when BOTH the primary record-0 read fails AND the mirror-position read fails; on mirror-only success add the +30 with a `mft-mirror-probe` evidence label.
2. Optionally add the boot-pair read as scoring evidence (+N when the sector at `candidateLBA − total_sectors` carries a matching NTFS OEM) — pending the FUN_180011260 field-set decode.
3. Files: `src/core/lost/cr_candidates.cpp`.
4. Acceptance criteria: forged-OEM fixture produces no surfaced candidate (or an explicit near-zero score); torn-primary fixture scores with mirror evidence; all existing lost-partition suites green.

## Tests to add

- `LostPart.ForgedOemNoMftEvidenceRejected`.
- `LostPart.TornPrimaryMftMirrorEvidenceScored`.
- `LostPart.HealthyCandidateScoreUnchanged` (control).

## Related findings

- S-063 (the engine-side mirror fallback — the same og mirror-retry pattern one layer up; land together for a coherent mirror story).
- M-137 (retired; the lost-partition surfaced-shape family — the same module's presentation layer).

## Open questions

FUN_180011260's boot-pair field-matching set (which fields must match between the two boot sectors) — one probe read of that function body; not load-bearing for the MFT-evidence fix.
