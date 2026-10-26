# OQ-T8 (RESOLVED session 3 — see N-031) · OQ-T9 (RESOLVED session 3 — see M-017 + the session-3 addendum there) — CalcPartition scoring internals (lost-partition candidate ranking)

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

## OQ-T9 — CalcPartition scoring internals — **RESOLVED 2026-10-26 (session 3, both legs)**

**Status:** RESOLVED · **Module:** lost-partition · **Parameters:** A30, B3, A29
**Resolution:** the OQ's premise was half-wrong, and the real scoring is decoded.

1. **The `*CalcPartition` classes are NOT scorers** — executed RTTI COL walk on the
   og binaries resolved `Fat32CalcPartition::vftable @ 0x180053680` and
   `ExFatCalcPartition::vftable @ 0x18004b9f0`; vtable slot 4 returns
   `*(u32*)(this+0x18)` = the volume's **sectors-per-cluster** (proved by
   `(cluster−2)*SPC + start` at FATSearchDLL_decompiled.c:39076-39083) — per-lost-volume
   geometry calculators letting the FAT/exFAT engines read files outside the allocation
   table. Only those two engines instantiate them (zero refs in the other seven
   SearchDLLs). Evidence: `evidence/executions/s3-calcpartition-vtables/`.
2. **The real scoring** is in DRWUI.exe `FUN_14050fda0` (:842164-842279), decoded by
   both legs independently with identical constants: count = files + 50·folders
   (NTFS family; FS==8 → f + 10·d — HFS+; FAT 1/2 → (f+d)·50), `+= *(longlong*)(info+0x68)`
   per-FS bonus, `+= 500` on partition-table match; `score = ln(count)/ln(maxCount)`
   (count==1 keeps numerator 1.0); **stable** descending merge sort. Validated
   against the vendor's own `CalcPartition.log` (187 rows to 6 decimals / 3 rounds
   to ≤2e-5 residual). Filed as **M-017**.
3. **`AdjustPartitionInfoOrder`** (`FUN_140510410`, :842495-842859) fully decoded in
   the M-017 addendum: containment-in-known drop → FS-grouped dedup → top-2 tie rules
   (≤0.1; ≤0.3 with FAT runner-up = NTFS-over-FAT priority) → FAT multi-vector drop →
   **±16 MiB size-match promotion to result #2** → surfaced-result **cap 2/4/5/6**.
4. **Residues filed as findings:** S-019 (BitLocker "-FVE-FS-" probe family missing —
   S-006 under-listed), N-034 (FAT lost-boot gate constants), N-035 (data-area-anchored
   extents), and the bonus setter tracked in **OQ-S2M-2** (observed: NTFS 60/30, FAT32
   20, usually 0; setter inside the SearchDLLs, not yet located).

**Related findings:** S-006 (probe SET — orthogonal, stays open), S-007 (carve coverage),
M-005 (carve target policy), M-017 (the scoring divergence + addendum).
