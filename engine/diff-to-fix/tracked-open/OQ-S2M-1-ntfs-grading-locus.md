# OQ-S2M-1 — Where do the og's user-facing grades for NTFS deleted files come from? (D15 grading locus)

**Status:** OPEN (session-2 residue) · **Module:** ntfs / drwui · **Parameters:** D15 (confidence grading model)
**Audited commit:** `e93decf` · corpus `03ad46c`

## The precise question

Session-2 established (S-012) that the og's NTFS **pass 1 emits every live-MFT record with no grade** — no viability, no overwrite state, item codes only (1=deleted, 2=rescued, 6=other). Yet DRW's UI shows "Recovery Chances" categories for deleted files. **Where is that grading computed, and from what inputs?**

Candidate loci, all unverified:
- DRWUI-side (the CRecoverability/grading machinery glimpsed in `DRWUI-subsystems--assistant-grading-preview.md` and `DRWUI_decompiled.c` — device.kind −2 offset grading per the S2-F sweep);
- the SaveScan `.rsf` writer (a per-record grade field? `artifacts/SaveScan/ifileproperties-*` list no grade column — weak);
- the $I/$R recycle-bin pairing path (`FUN_18001c710`, gated on the +0x72 bit pass-1 sets — the only state machine feeding *something* about deleted records);
- the MO/controller layer between SearchDLL and DRWUI.

## Why it matters

- D15 verdicts for `ntfs` currently say the core's Confidence enum is a "superset feature". If the og computes a real grade downstream (e.g. from $I/$R pairing or extent-claimed ratios), the core's in-walk grading is not a superset but a **different model**, and the S-012 rework should either keep the core grade as a registered extra or mirror the og's model.
- The engine config bytes `+0x258/+0x259` (claim-skip and metafile-gate overrides, ctor defaults not decoded — `:18343-52`, `:18952`) may alter pass-2 claim semantics on damaged-volume modes; decode them in the same pass.

## Corpus artifacts to examine next

1. `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — grep for the grading gauge's data source: the CRecoverability class family, the assistant-grading-preview subsystem, and any reader of the SearchDLL item codes (1/2/6) that maps them to UI categories.
2. `re-workspace/reversed/ghidra-mo/` — the callback/operator between engine emit (`vt[0x10]`) and DRWUI consumption.
3. `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:17633-37` — the $I/$R pairing gate's consumption of props+0x72.
4. Raw evidence: `re-workspace/raw-evidence/ui-interaction-results-interactions/` and `ui-ground-truth-v5/` — screenshots/logs showing actual grade labels for known deleted files (poor/medium/good markers).

## Acceptance

RESOLVED when a quoted chain (decompile or raw evidence) names the grade source and its inputs; then update `modules/ntfs.md` D15 and fold into S-012's step 4.
