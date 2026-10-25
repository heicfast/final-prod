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
## Resolution addendum (session-6 upgrade — the decode supersedes the analogs)

**Status:** FIXED (upgraded in place — the first landing's analogs replaced
with the decoded truths; the first Resolution is preserved above as history)
**Fixed in commit:** (this commit — see `git log` for "fix(S-024): decoded-truth upgrade")
**Verified by:** 20 tests in `tests/unit/test_health_grade.cpp` (12 new/rewritten
discriminators; suite 464 total on this leg's tree — 460 pass + the 4 pre-existing
Ext4Engine.Backup* reds owned by the parallel S-027 lane, unchanged by this fix);
fail-before by construction: the landed header lacks `classify_family`/
`make_grade_inputs`/the family inputs (API-fetched landed header checked:
`classify_family in header = False`), so every new discriminator fails to even
compile against the analog engine. Full decode evidence:
`evidence/decompiled-quotes/s6-health-grading-decode.md`.
**Resolution notes:** A first-hand re-decode of DiskHealth_decompiled.c
(corpus 03ad46c) resolved BOTH open questions of the original finding and
corrected four analogs of the first landing — the corpus wins over documented
analog guesses: (1) **the 0x1724 flag's writer sites ARE decodable** — the
is-SSD string list (:6300-6344: prefixes OCZ/SPCC/PATRIOT/PHOTOFAST/STT_FTM/
"Super Talent", contains Solid/SSD/SiliconHardDisk) now implemented exactly
(`model_says_ssd`, `cr_family.cpp`); note "KINGSTON SV300S37A" matches
nothing and stays HDD-class for grading — og-faithful, pinned by test. (2)
**the interface-family enum IS decodable** — FUN_0040dad0's cascade
(:6300-7000) decoded: 30+ vendor families with names, model-string/attr-
signature gates, and the sub-family codes; the family-gated SSD-life attr
selection (attr 169/173/177/187/202/209/230/231/232/233 → per-family gates)
replaces the is-SSD analog gate; the raw/invert/100-raw life forms
(0x17e4/0x17e8), the SanDisk G1001 gate (0x17f4) and the WDC/SanDisk
attr-230 raw16/100 form (0x17ec) implemented. Residual: the exotic-vendor
thunk windows (FUN_00422b60/FUN_00422c30/FUN_00422470/FUN_00424960/
FUN_004224e0/FUN_00424430) with overlapping decode boundaries stay generic
0/1 in our classifier — engine consumes every family code exactly, so the
residual is a pure classifier-data gap with the exact next decode sites
listed in the evidence §5. (3) **the disk grade is FUN_0040cdf0's exact
semantics** — bad-count/caution/graded-signal aggregation (worst-wins was
the analog), the Unknown gates (0x16d8/0x16dc/0x16f8), duplicate-attr-id →
Unknown, SSD any-attr-below-threshold vs HDD in-range-only (the decoded
range table: 1-13/187-189/191-193/195-209/211-212/220-228/230-231/240/
250/254), the 5/197/198 raw16 caution with the all-FF raw[0..3] guard
(disk-side only — the per-attr display rule has no guard; the asymmetry is
pinned), attr-1's SandForce-only rule (family 7), and the
Parallels/NVMe family-0x13 path. (4) **temperature NEVER affects the og's
grade** — 0x1808 (the 50/60 defaults) is never read by the grader and temp
attrs display Good (:33596); the first landing's temp-Caution rule was a
divergence (a 51°C HDD graded Caution where the og says Good/Unknown) and
is REMOVED; temp stays a reported field (SmartReport.temperature_c) for
display, like the og's tray. (5) **NVMe decoded** (:5795-5810): any nonzero
critical_warning byte → Bad (the spec-bit split {0,2,3}/{1,4} was the
analog and is corrected); life = 100 − percentage_used with > 10 Good /
== 10 Caution / < 10 Bad (the analog graded 1-9% as Caution — corrected to
Bad); the Parallels early-Good; media_errors dropped from grading (never
read by the og's grader). (6) the failing_bit → disk-Bad force is retained
as the one deliberate superset, now REGISTERED as PARITY-DELTAS D-63 (the
first landing kept it unregistered). (7) 0xD1 verified consistent with the
og's internal mirror (:14108-14118 → +0x1569 rows); win layer now extracts
the model string from IDENTIFY words 27-46 and runs the pure classification
on both platforms. All four "documented analog" statements in the first
Resolution are superseded by this addendum with corpus citations.

---

## Addendum (session 6 — doc-truth sub-item: the health header claims threshold commands it never issues, 2026-10-26)

While verifying this finding's SMART side against the UIUX/19 decode (first-cited this session: the og's S.M.A.R.T. table columns "ID / Attribute / Current / Worst→(ST) / **Threshold** / RawValue / **Status**" and the tri-state "Good / Caution! / Normal" — 19-diskhealth-quick-ui.md:26-27), a header-truth defect surfaced on OUR side: `include/cr/win/cr_health.h:8` claims the SMART ladder issues "0xD1 READ_THRESHOLDS / 0xD8 RETURN_STATUS" — `src/win/cr_health.cpp` issues only IDENTIFY (0xEC, :172-174) and SMART READ DATA (0xB0 feature 0xD0, :186-187); 0xD1/0xD8 appear nowhere (grep-verified). The header comment over-claims the shipped command set. This finding's fix plan (grading via thresholds) is the path that will genuinely issue 0xD1; until then the header must match reality (folded into N-074's doc-truth group, sub-item 5). The Threshold/Status/tri-state capability gap itself is exactly this finding's scope — no separate finding filed.

### Post-resolution correction (session 6, after the fix landed)

Re-verified at the merged tree (`5b0bc7b`-lineage): this finding's fix
(`74f9def`) landed the 4-status model + **vendor thresholds via 0xD1**
(`src/win/cr_health.cpp:192-200` — `SMART READ_THRESHOLDS (feature 0xD1, S-024)`
now genuinely issued, both transports). The addendum's doc-truth sub-item is
therefore HALF-resolved: 0xD1's header claim is now true; **0xD8 RETURN_STATUS
remains claimed-but-unissued** (`cr_health.h:8-9` still lists it; grep of
cr_health.cpp: 0 hits) — N-074's sub-item 5 narrows to the 0xD8 line only.
