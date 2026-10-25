# MAJOR M-044 — UDF volume recognition missing: the og classifies UDF descriptors (analyser result 0xc); the core has no UDF family

**Severity:** MAJOR
**Module:** recognition (fs families) — new module row; FAT analyser hosts the og's probe
**Parameters affected:** P1 (detection — UDF volumes unrecognized), P4 (classification — no UDF bucket), P13 (report schema — no UDF kind), P15/P16 (grading/filter model — UDF bucket ids), P19 (magic/signature — the og's descriptor probes)
**Corpus references:** `re-workspace/reversed/ghidra-mo/FATFileSystemAnalyser_decompiled.c` — `CFatVolumeAnalyser::Analyze` `FUN_180002580` (:1385): **UDF recognition probe** (:1413-1435, lead-verified first-hand): reads the sector at +0x100 and the sector at `u32@boot+0x14` (the acknowledging descriptor sequence), then requires **"UDF" at bytes [0x19..0x1b]** of the recognition-descriptor sector → sets **result 0xc** (UDF) at :1434 with geometry fields zeroed (:1435-1438); the analyser type-code enum: 1=FAT12, 2=FAT16, 3=FAT32, 4=NTFS, 0xb=BitLocker-FAT, **0xc=UDF**, 0xff=unknown; `re-workspace/reversed/ghidra-searchdll/` — no UDF SearchDLL exists (the 9-engine set has no UDF engine; UDF volumes surface via classification + RAW carving)
**Core references:** audited `7cf6754` — `src/core/orchestrator/cr_scan.cpp:473-530` (`probe_family`: no UDF branch — a UDF volume returns FsFamily::Unknown and falls to the RAW/carve path); `include/cr/table/cr_table_parse.h` (FsKind enumeration — no Udf value); grep-verified zero UDF hits in src/include ("UDF" appears only in carve signature tables, if at all — see gap note)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's volume analyser recognizes UDF (result code 0xc) via a recognition-sequence probe — reading sector +0x100 and the sector referenced by the descriptor at +0x14 and checking "UDF" at offset 0x19 — so UDF volumes (optical media, UDF-on-HDD partitions, many camera/cards) are classified and bucketed as UDF in the UI, then recovered through the RAW carving path (there is no UDF structured engine in the og either). The core has no UDF recognition at all: a UDF volume lands in FsFamily::Unknown with no UDF classification, no bucket id, and no UDF-aware display.

## What the reference does

The probe is part of the FAT analyser (UDF recognition descriptors live in
what would otherwise be a FAT BPB's extended boot area): two sector reads,
one string check, a type code. The og's classification surface (analyser
result byte) feeds the UI bucket/label model and the filter tree; recovery
of UDF content then rides the RAW family (the og's carve registry has no
UDF structured parser — confirmed by the engine set). So the og's UDF
parity surface is exactly: **recognize + classify + label**.

## What our core does

No UDF family, no recognition probe, no FsKind value. A UDF partition
scanned via `--volume`/lost-partition materialize dispatches as Unknown;
carve recovers files by signature (as in the og), but the volume is never
IDENTIFIED as UDF — the classification/label/bucket layer has no entry for
it.

## Gap

Falsifiable: a fixture volume whose recognition descriptors carry "UDF" at
+0x19 (with the acknowledging sequence at +0x100) classifies as UDF
(analyser result 0xc) in the og and as FsFamily::Unknown in the core.

## Impact

User-observable classification gap for an entire filesystem family: UDF
volumes (common on external media) show as unknown/raw instead of "UDF" —
affecting the volume listing, the filter tree, and any per-family
statistics. Recovery content is unaffected (both products carve UDF).
Severity MAJOR (not MINOR) because it is a whole-family classification
surface, consistent with how BitLocker classification (0xb) was treated as
SEVERE-adjacent in S-019 — but here there is no engine behind it in the og
either, so the gap is classification-only.

## Reproduction / verification

Corpus-derived (the probe block re-verified first-hand at FFA:1413-1435;
the result code 0xc and the zeroed geometry fields are in the same basic
block). Core side verified by reading probe_family + FsKind. Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §C.

## Fix plan (for the engineering team)

1. Add the recognition probe in `probe_family` (cr_scan.cpp) BEFORE the
   FAT catch-all, mirroring the S-019 BitLocker pattern: read sector
   +0x100 and the sector at u32@boot+0x14 (bounded reads through the
   envelope); if bytes [0x19..0x1b] == "UDF", classify the volume
   `FsFamily::Udf` (append-only enum value; do NOT create a structured
   engine — the og has none; deep-scan rides carve).
2. Surface: volume listing/report carries the UDF kind; query buckets gain
   the id (M-029's bucket-translation lane); no engine dispatch.
3. Tests: `UdfVolumeRecognized` (fixture: descriptor with "UDF"@0x19 +
   sequence at +0x100), `UdfNotConfusedWithFat` (a FAT volume whose +0x19
   area is non-"UDF" stays FAT), `UdfSurfacesAsCarveFamily` (deep scan on
   a UDF fixture recovers via signatures, kind reported).

## Tests to add

The three named tests; the first is the fail-before proof.

## Related findings

S-019 FIXED (the pattern to copy — classification-before-catch-all),
M-045 (FAT gate family — the probe rides the same boot sector),
N-050 (recognition-shape divergence), M-029 (bucket ids for the new kind).

## Open questions

Whether the og's UDF probe ALSO gates on the acknowledging descriptor at
u32@+0x14 being readable (the code reads it and only then checks "UDF" —
the fail path at :1416-1420 suggests both reads must succeed; the fixture
should cover both branches).
