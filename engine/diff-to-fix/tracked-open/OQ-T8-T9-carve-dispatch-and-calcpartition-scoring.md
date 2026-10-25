# OQ-T8 (RESOLVED session 3 — see N-031) · OQ-T9 (tracked-open) — CalcPartition scoring internals (lost-partition candidate ranking)

---

## OQ-T8 — RAWSearch dispatch order — **RESOLVED 2026-10-25 (session 3)**

**Status:** RESOLVED (decompiled first-hand) · **Module:** carve · **Parameters:** A28, C19, B3

**Resolution (supersedes the question below):** the dispatch is `FUN_18000cf60`
(RAWSearchDLL :9730-9930), called per sector from the scan loop `FUN_18000c820`
(:9385; call at VA 0x18000cadc proven by disassembly: `call 0x18000cf60; test
rax,rax; je 0x18000ccfc` — the NULL edge goes to `FUN_18000d670`, the
raw_signs.json layer). Order decoded:

1. `config/config.ini` `[User]` `only_common` gate (once-flag DAT_180118204);
2. **u32 4CC registry FIRST** — `std::map` exact lookup on the first 4 bytes,
   only when the first u32 != 0 (the corpus artifact's inferred "u16 first"
   was inverted — erratum 19);
3. **u16 registry second** (`FUN_18000d3e0`, first-2-bytes, skipped when the
   first u32 is 0);
4. linear walk of the whole analyser registry (also the only_common mode);
5. total miss → **raw_signs.json layer LAST** (`FUN_18000d670`).

The core's layering (grammar layer first; raw-sign layer second honoring
grammar-owned positions) is semantically equivalent on the ordering question
that mattered (raw_signs is the last resort on both sides). Residual deltas
are filed as **N-031** (sector-relative stride, all-zero-sector skip,
only_common gate, chain-vs-specificity collision order); the min/max unit
decode from the same session is **S-015** and the dead confidence field is
**N-030**. Evidence:
`evidence/decompiled-quotes/s3-rawsigns-units-and-dispatch.md`.

**Original question (superseded):**
**Corpus artifact to examine next:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
— the dispatch site that consults the two registries: builder `FUN_180004320`
(corpus lines 2517–2918), find-or-create `FUN_18000ac20`, append `FUN_18000ac70`.

**Session-2 refinement (2026-10-25):** the CalcPartition machinery is located —
`FATSearchDLL_decompiled.c :5744` matches `Fat32CalcPartition::vftable` (RTTI-extracted);
ICalcPartition subclasses expose the score via vtable slot 4 (`(*(code **)*plVar7)[4](plVar7)`
at :5747) with a direct-field fast path for Fat32 (`iVar3 = (int)plVar7[3]`, :5745-5746).
The `__RTDynamicCast` sites at :5592-5595/:5816-5818 confirm the ICalcPartition→
Fat32CalcPartition dynamic-cast pattern. Next decode target: Fat32CalcPartition's slot-4
method + the sibling ICalcPartition subclasses (per-FS CalcPartition classes in each
SearchDLL) for the feature/weight table.

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
