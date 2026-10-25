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
