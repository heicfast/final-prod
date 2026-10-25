# OQ-T8 (tracked-open) — RAWSearch dispatch order (u16 → u32 4CC → raw_signs) inferred, not decoded
# OQ-T9 (tracked-open) — CalcPartition scoring internals (lost-partition candidate ranking)

---

## OQ-T8 — RAWSearch dispatch order

**Status:** UNVERIFIED · **Module:** carve · **Parameters:** A28, C19, B3
**Corpus artifact to examine next:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
— the dispatch site that consults the two registries: builder `FUN_180004320`
(corpus lines 2517–2918), find-or-create `FUN_18000ac20`, append `FUN_18000ac70`.

**Precise question:** when a candidate offset's first bytes could match both a u16 key
(e.g. 0x4D42 "BM") and a u32 key (e.g. 0x424Dxxxx), which registry is consulted first,
and how are collisions resolved (the multimap export `EUGetAnalyserMultimap` exists
precisely because multiple analysers share keys — e.g. the JPEG family
e0/e1/db/ee-FFD8FF variants and the TIFF family)? Our core uses a single
specificity-ordered trial table (`kTrialOrder`, 34 formats, DOCX>ZIP, CR2>DNG>TIFF) —
functionally equivalent for the formats it has, but the collision-resolution ORDER
matters for formats where two validators can both accept (TIFF-family relabeling:
ORF/RW2/DNG vs plain TIFF).

**Suggested probe:** decompile the scan-loop dispatch (the caller of FUN_18000ac20);
record the key-width order and the per-key analyser-chain order; compare against
`kTrialOrder` for the shared formats.

---

## OQ-T9 — CalcPartition scoring internals

**Status:** UNVERIFIED · **Module:** lost-partition · **Parameters:** A30, B3
**Corpus artifacts to examine next:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`
and `exFATSearchDLL_decompiled.c` — the `Fat32CalcPartition` (39 vftable refs) and
`ExFatCalcPartition` (54 refs) class families; plus `PartitionRecovery/qa-logs--session-forensics.md`
(AdjustPartitionInfoOrder ranking of 15+ overlapping NTFS candidates — QA-log evidence).

**Precise question:** what evidence features and thresholds do the og's per-FS
geometry calculators score (boot-field consistency? FAT-table sampling? index walk?),
and what ranking does `AdjustPartitionInfoOrder` apply to overlapping candidates? Our
core uses an evidence-points model (NTFS: OEM +15 / valid BPB +25 / $MFT liveness +30;
overlap penalties −20 inside / −10 overlapping) with first-nested-wins dedup
(post-GAP-S6-6: probe-sized outers only). No corpus contradiction is demonstrable, but
the og's model is undecoded — comparative benches on damaged-partition fixtures are the
empirical vehicle (the bench harness already generates delete_partition scenarios).

**Suggested probe:** decompile the CalcPartition vtable walk in the FAT engine; extract
the scoring arithmetic (constants + field reads); cross-check against QA-log traces of
AdjustPartitionInfoOrder ordering decisions.

---

**Related findings:** S-006 (probe SET — orthogonal to scoring), S-007 (carve coverage),
M-005 (carve target policy).
