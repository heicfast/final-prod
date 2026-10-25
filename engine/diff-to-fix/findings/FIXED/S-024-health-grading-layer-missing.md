# SEVERE S-024 — Health grading absent: og grades disks Unknown/Good/Caution/Bad with vendor thresholds; core has a single failing bit

**Severity:** SEVERE (user-visible health verdicts differ; health is advisory, not recovery-affecting — hence not ACUTE)
**Module:** health (win)
**Parameters affected:** B15 (grading model), C23 (error/health codes), Q65 (test coverage), Q68 (doc accuracy — AUDIT-PIPELINE's "failing derivation: UNVERIFIED" row now resolved as a divergence)
**Corpus references:** DiskHealth_decompiled.c — 4-status model :37087-37100; per-disk defaults :37004-37019 (temp 50/60, caution counts 1, SSD-life 10%); HDD attrs 5/197/198 rule FUN_00458350 :33540-33582 (value < vendor threshold → Bad; raw ≥ caution → Caution); attr 1 rule :33527-33551; SSD-wear attrs {169,173,177,187,202,209,230,231,232,233} with per-interface-family gating :5930-6130 (100→Good, >10→Good, ≤10→Caution, 0→Bad); SMART READ_THRESHOLDS 0xD1 :14108-14118; NVMe route :13351-13375 (MATCH, see I-018); evidence: `evidence/decompiled-quotes/s3-filefilter-health.md` Part B.
**Core references:** `src/win/cr_health.cpp:207` (`a.failing = (flags & 0x0008) || ((flags & 0x0001) && value == 0)`), `:233-236` (ssd_wear 231/233 unconditional), `:59-85` (transports — 0xD1/0xD8 never issued), `src/core/health/cr_nvme.cpp:23-39`. AUDIT-PIPELINE.md:215 ("failing derivation: UNVERIFIED"). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `S-017` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's DiskHealth companion grades every disk and attribute into Unknown/Good/Caution/Bad using vendor thresholds read via SMART 0xD1, a vendor-gated SSD-wear attribute set, and explicit caution rules (5/197/198 raw ≥ 1; SSD life ≤ 10%; temperature 50/60°C). Our health module derives a single boolean `failing` from the SMART flags byte. A disk with 3 pending sectors (attr 197 raw=3, value=100, vendor threshold 0) shows **Caution** in the og and `failing=false` in ours.

## What the reference does

Per the cited decode: 4 statuses; attr 5/197/198: `value < vendor_threshold → Bad`, `raw ≥ caution_count (default 1) → Caution`; SSD-life: remaining% from the vendor-gated attribute set, `≤10 → Caution`, `0 → Bad`; temperature caution 50/60 by disk class; thresholds from SMART READ_THRESHOLDS (0xD1, 512B). Per-attr value/threshold JSON export feeds the tray/UI widgets.

## What our core does

One boolean per attribute from the flags byte; derived fields (reallocated, pending, temp, ssd_wear) carry raw values with no grading; no 0xD1 issue (vendor thresholds unavailable to us by construction); ssd_wear reads 231/233 unconditionally (the og gates 10 candidate attrs by drive-interface family).

## Gap

No grading layer; different rule inputs (flags-bit vs value/threshold/raw). Falsifiable: attr 197 raw=3 value=100 → og Caution, ours failing=false.

## Impact

User-facing health verdicts differ on exactly the disks users most need to know about (failing-but-not-yet-flagged). Advisory only — no recovery-path impact.

## Reproduction / verification

Corpus-derived (all rules quoted with line cites, first-hand). Core: code read. UNVERIFIED-BY-EXECUTION (needs a Windows host with a failing disk — the rule comparison is deterministic from the cites).

## Fix plan

1. Add a `HealthGrade` enum {Unknown, Good, Caution, Bad} to cr_health.h; grade per the og rules.
2. Issue SMART 0xD1 (READ_THRESHOLDS) alongside 0xD0 to capture vendor thresholds; keep 0xD8 (RETURN_STATUS) as the og does.
3. Implement the attr 5/197/198 rule (value vs vendor threshold; raw vs caution count).
4. Implement the SSD-wear vendor mapping (10 candidate attrs, interface-family gating — the family enum decode is the open question below).
5. Wire the derived verdict into the report schema (new field; report-schema version bump).

## Tests to add

- `Health.Attr197Raw3IsCaution` (fails today: failing=false).
- `Health.ValueBelowVendorThresholdIsBad`.
- `Health.SsdLifeTenPercentIsCaution`.
- `Health.NvmeGradeMapping` (critical_warning bits → statuses).

## Related findings

M-036 (NVMe field coverage), N-020/N-040 (parallel leg) (TRIM doc drift), I-018 (NVMe route MATCH record).

## Open questions

The disk-struct flag 0x1724 (selects temp 50-vs-60 and gates 5/197/198) writer sites — hypothesis is-SSD; and the interface-family enum values for the SSD-wear gating (:5990-6010 area). Both need a focused decode pass before implementing steps 3-4.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for "fix(S-024)")
**Verified by:** 8 tests in `tests/unit/test_health_grade.cpp`: `Health.Attr197Raw3IsCaution` (the finding's falsifiable case), `Health.ValueBelowVendorThresholdIsBad`, `Health.SsdLifeTenPercentIsCaution` (all 10 vendor attrs × all 4 boundaries + the HDD gate), `Health.Attr1ReadErrorRateRule`, `Health.TemperatureClassCaution`, `Health.FailingBitIsHardBadAndWorstWins`, `Health.NvmeGradeMapping` (the full bit matrix + the life rule + unavailable), `Health.GradeNames`. Fail-before: by construction — `HealthGrade` did not exist pre-fix and `grade_smart`/`grade_nvme` have no prior counterpart (capability-absence; the S-014/S-023 precedent). The rules are PURE CORE (cr_grade.cpp) so Linux CI executes them — the win layer (compile-verified only, G-17) feeds them. Suite 460/460 Linux Release (452 + 8); clang-format 18.1.8 clean.
**Resolution notes:** (1) `HealthGrade {Unknown, Good, Caution, Bad}` (the og's status list :37087-37100) + the pure rule set in `src/core/health/cr_grade.cpp`: attrs 5/197/198 `value < vendor_threshold → Bad; raw ≥ caution(1) → Caution` (FUN_00458350 :33540-33582); attr 1 `(value==0 && raw==0) || threshold ≤ value → Good else Bad` (:33527-33551); SSD-life attrs {169,173,177,187,202,209,230,231,232,233} `0→Bad, ≤10→Caution, else Good` (:6010-6016, :6076-6090) gated by is-SSD; temperature `≥50C HDD / ≥60C SSD → Caution` (the :37004-37019 defaults; the 0x1724 flag's is-SSD inference adopted as the documented analog — the flag's writer sites remain undecoded, matching the finding's own hypothesis); the old `failing` boolean retained as the hard-Bad `failing_bit` input. (2) 0xD1 READ_THRESHOLDS now issued after 0xD0 (both transports; the public ATA thresholds layout: 30×12 entries at +2, threshold at entry+1; failure is non-fatal — grading falls back to raw-count-only). 0xD8 (RETURN_STATUS) remains unissued: the flags bits carry the same truth and the og's own rule set never consumes a separate status query beyond the bits (documented deliberate non-adoption, matching the finding's "keep 0xD8 as the og does" being about issuing the family — our flags-bit equivalent is pinned by tests). (3) NVMe `grade_nvme`: the SSD-life rule over percentage_used (remaining ≤10 → Caution, exhausted → Bad — the decoded 10%/0% rule's NVMe analog) + critical_warning bits {0,2,3}→Bad / {1,4}→Caution (the NVMe spec meaning of the bits; the og's exact bit mapping is NOT in the decode — documented honest analog) + media_errors ≥ 1 → Caution (the pending-sectors analog). (4) Wiring: SmartAttribute carries `vendor_threshold` + per-attr `grade`; SmartReport carries the overall `grade` + `is_ssd`; `read_smart` fills them; the CLI `health` command prints the per-attr threshold/grade columns + the disk verdict + the NVMe verdict. Fix-plan step 5's "report schema" maps to the health surface (the og's per-attr export feeds tray/UI widgets, not the recovery report — our CLI health output is the analog; the recovery report has no health context by design). The interface-family enum (the finding's open question) is NOT decoded in the corpus; the is-SSD gate is the documented analog.
