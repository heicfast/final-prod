# MINOR N-055 — ATA SMART thresholds never read (0xD1): the og's health engine reads READ ATTRIBUTE THRESHOLDS and surfaces threshold-based verdicts; the core reads only READ DATA (0xD0) — disposition correction for modules/health.md

**Severity:** MINOR
**Module:** health
**Parameters affected:** B1 (detection — health data completeness), D11 (field model — no threshold field), Q68 (doc accuracy — modules/health.md's "presentation-layer" disposition is wrong)
**Corpus references:** `re-workspace/artifacts/UIUX/19-diskhealth-quick-ui.md` (the og's health table shows a **Threshold column** with Good/Caution/Normal verdicts composed from attribute-vs-threshold comparisons and firstCautionIndex); `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md` (CAtaSmart engine reads the SMART attribute AND threshold data across its transport accessors)
**Core references:** `src/win/cr_health.cpp` (ATA path: IDENTIFY → SMART READ DATA 0xB0/0xD0 — the 362-byte attribute table; no READ ATTRIBUTE THRESHOLDS 0xD1 command anywhere), `src/core/health/cr_nvme.cpp:30` (NVMe `available_spare_threshold` IS read — page[4]; NVMe side is complete), `diff-to-fix/modules/health.md:13-17` (the prior disposition: "threshold/PREDICT FAILURE semantics + attribute naming — presentation-layer features beyond the engine's data contract") — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's health engine (the DiskHealth companion's CAtaSmart) reads both SMART READ DATA
(0xD0) and the attribute THRESHOLDS (0xD1), and its UI composes Good/Caution verdicts from
attribute-vs-threshold. The core's ATA path issues only 0xD0 — no 0xD1 command exists — so
threshold data is unavailable to ANY consumer (not just presentation). The prior module
disposition ("presentation-layer feature") is corrected by this finding: the read is an
engine-side data gap. NVMe is unaffected (available_spare_threshold is read).

## What the reference does

Per the diskhealth companion artifact, CAtaSmart's transport accessors retrieve the full
SMART picture including thresholds; the quick-UI artifact shows the Threshold column and
threshold-composed verdicts (firstCautionIndex tracking).

## What our core does

`cr_health.cpp`: ATA IDENTIFY → passthrough SMART READ DATA (0xB0/0xD0) → 30×12-byte
attribute table; curated fields surfaced. No threshold command, no threshold field in the
health result model. NVMe: `cr_nvme.cpp:30` reads available_spare_threshold — complete.

## Gap

Falsifiable: on a SATA disk, the og's health output includes per-attribute thresholds
(and Caution verdicts when value ≤ threshold); the core's output cannot include either.

## Impact

Health is advisory (recovery unaffected — the module's F59/F60 are NA), so MINOR. But the
disposition correction matters: any consumer wanting og-equivalent health verdicts (a UI,
a report) is blocked at the data layer, not the presentation layer.

## Reproduction / verification

Corpus artifacts read first-hand (agent-D read this session; the Threshold-column and
CAtaSmart accessor claims are the artifacts' own). Core greps verified: no 0xD1/threshold
command in the ATA path; NVMe threshold present.

## Fix plan

1. Add a SMART READ ATTRIBUTE THRESHOLDS (0xB0/0xD1) passthrough beside the 0xD0 read
   (same transport pair: IOCTL_ATA_PASS_THROUGH + SMART_RCV_DRIVE_DATA fallback).
2. Extend the health result model with per-attribute threshold (u8) and a
   threshold-composed verdict bit; keep the og's "value ≤ threshold → caution" composition
   documented.
3. Update modules/health.md's disposition line to cite this finding (data-layer gap, not
   presentation).
4. Tests: a synthetic IDENTIFY+DATA+THRESHOLDS device triple; verdict composition vectors.

## Tests to add

- `Health.AtaThresholdsRead` (0xD1 issued; thresholds populated)
- `Health.AtaCautionVerdictComposed` (value == threshold → caution)

## Related findings

- N-032 (health transports — CSMI/USB-bridge/Realtek; same module's transport gaps)
- N-009a (NVMe percentage_used width — the module's other data-model finding)

## Open questions

None.

---

## Resolution (session-5 close, 2026-10-26)

**Status:** FIXED — by the parallel fix leg's S-024 health-grading fix (commit `74f9def`),
not by this audit leg. The fix implements exactly this finding's fix plan: `src/win/cr_health.cpp:192-196`
now issues SMART READ_THRESHOLDS (feature 0xD1) beside the 0xD0 read, parses the
30×12 threshold table (threshold at entry+1), and the 4-status grading model
(Unknown/Good/Caution/Bad) composes value-vs-vendor-threshold verdicts — closing both
the data-layer gap and the disposition correction this finding filed (the "presentation-
layer" hand-wave in modules/health.md is superseded). The NVMe side was already complete
(available_spare_threshold). Cross-leg note: this finding's fix-plan steps 1-2 and the
S-024 fix's threshold implementation are the same work; the S-024 leg landed it with the
full grading model this finding did not cover (scope superset). The disposition-correction
value of this finding stands: the audit record now correctly shows the threshold read as
an engine-side capability, not presentation.

## Resolution addendum (session-6)

The S-024 decoded-truth upgrade re-verified the 0xD1 path against the
corpus: FUN_0041a160 (:14108-14118) issues READ_THRESHOLDS (0xB0/0xD1,
count 0x0E, 512B) and FUN_00414230 mirrors thresholds to per-attr rows
at +0x1569+idx*0xc — the public page layout our fix reads (threshold at
entry+1) is value-equivalent. No code change to the threshold read; the
grading rules that consume the thresholds are now the exact decoded set
(see the S-024 Resolution addendum + evidence
s6-health-grading-decode.md §8).

