# MINOR N-200 — Lost-scan NTFS candidate acceptance is weaker in the core: the og requires boot-pair OR 4-record-MFT validation (with a mirror retry) before accepting a candidate; the core's probe never rejects on MFT evidence (scoring only, no mirror retry)

**Severity:** MINOR
**Module:** lost-partition (NTFS candidate acceptance)
**Parameters affected:** A30 (candidate scoring), B3 (false-positive rate — lost-list noise), A26 (probe bounds)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` FUN_180011150 (:9880-9914 — the candidate classifier: `FUN_180011260(boot-pair) == 0` → try `FUN_180011560` (4-record MFT read + validate + MIRROR RETRY at FUN_180011b30's +0x38 getter), both failing → the 0xfffffff1 reject slot) and FUN_180011560 (:10023-10075 — `FUN_180011a10(param_4) << 2` = 4 × record_size bytes read; `FUN_180011b10`(mft_lcn)… on failure `FUN_180011b30`(mirr_lcn) retry), both re-read first-hand this session
**Core references:** audited `34d6cf10` — `src/core/lost/cr_candidates.cpp:70-122` (`probe_ntfs`: the MFT record-0 probe adds +30 on success but never rejects on failure; no mirror retry; no boot-pair check)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber:** N-187 → **N-200** (2026-10-29, the S56 lane's third collision rider: the retired mpegps N-187 (the S56 lane's N-160 doc-truth renumber, landed 23:13 UTC) owns the seat per SA-9; the seat was taken before this filing pushed - the doc_truth_gate's active-vs-ledger check caught it live; no content change)

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

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S70 MINOR-first fix lane, 2026-10-29)
**Verified by:** `LostPart.ForgedOemNoMftEvidenceRejected` (RED pre-fix — the forged-OEM/no-MFT-evidence candidate surfaced at 40 pts; GREEN post-fix — nothing surfaces at that LBA) + `LostPart.TornPrimaryMftMirrorEvidenceScored` (RED pre-fix — no mirror retry existed, the mirror-tag assertion failed on a surfaced-by-old-policy candidate; GREEN post-fix — the torn-primary/valid-mirror fixture surfaces with structural_points 70, the `mft-mirror-probe` evidence label, and score 1000 proving the strong-probe bonus applies mirror-blind) + `LostPart.HealthyCandidateMftEvidenceUnchanged` (the control: primary `mft-probe` label + 70 pts + score 1000, unchanged before and after). Full suite 1425/1425 (1383 pass + 42 Win-skips, 0 fail) at the moved tree `449fc594`-lineage; lost-partition families 25/25.
**Resolution notes:** Corpus re-verified first-hand at the fix: FUN_180011150 (:9889-9906 — the classifier's three-arm structure) and FUN_180011560 (:10023-10075 — the 4×record-size read at the primary $MFT LCN, the FUN_180011440 validation, and the identical read re-issued at the FUN_180011b30 mirror LCN, returning 1 only when either leg validates). Landed exactly per the fix plan's step 1: probe_ntfs's BPB-valid path now runs the record-0 probe as a soft requirement — primary first, then the $MFTMirr retry (og-exact unconditional re-read; the og bounds neither LCN, read_at's device bound plays the failed-read role), rejecting with the -100 noise drop only when BOTH legs fail (the og's 0xfffffff1 slot); mirror-only success carries the +30 with the `mft-mirror-probe` evidence label. fs_bonus's strong-probe branch accepts both labels — the og's FUN_180011560 return value does not distinguish the accepting leg, so the content-model bonus is mirror-blind by construction. The boot-pair leg (FUN_180011260's field set: total_sectors + the FUN_180011bd0 field + mft_lcn matches) stays deferred per the finding's own open question — its arm's slot semantics (a matching pair at candidateLBA − total_sectors writes the same 0xfffffff1 value as the both-fail arm, reading as backup-DBR dedup rather than acceptance evidence) needs the FUN_180011bb0/bd0 getter pair decoded before porting; not load-bearing for the MFT-evidence requirement this finding owns. The BPB-invalid fallback path (the GAP-S6-6 honest device-end marker) is untouched: the og's classifier only reaches FUN_180011560 for parser-accepted candidates, and the core's stricter validate_boot_sector rejections have their own registered policy. Safety hardening preserved: none of the touched paths (M-042 geometry evidence, N-206 ceiled tail bound, GAP-S6-6 containment guard) is altered.
