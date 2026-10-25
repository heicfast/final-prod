# MINOR N-049 — Persisted lost-partition records are thinner than the og's: no geometry evidence (bps/SPC/hidden), no size-source flag, no per-hit crash safety

**Severity:** MINOR
**Module:** session (record richness — the informational residue of M-042)
**Parameters affected:** P12 (session persistence format), P13 (report schema fields), P34 (checkpoint semantics — informational remainder), P46 (documentation of persisted fields)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` — lostpart.dat 0x448-byte record: per-hit geometry payload CSearchedPartitionInfo (setters :502-559; ctor FUN_180002530 :951-961: start LBA, sector count, hidden sectors, type, SPC, bps) + applied-flag +0x14 + CHS-align +0x441 + disk number +0x08; the shipped `og-codebase/PartitionRecovery/lostpart.dat` carries bps=512/SPC=8 in the payload (layout-validated against the decoded writer)
**Core references:** audited `7cf6754` — `src/core/session/cr_records.cpp:368-386` (REC_PARTITION_CONTEXT codec: start_lba, sector_count, fs_kind, score, kind — nothing else); `include/cr/lost/cr_lostpart.h:56-66` (the in-memory Candidate carries `evidence[]`, `size_from_device_end` — NOT persisted); `src/core/orchestrator/cr_scan.cpp:1227-1238` (save), `:428-452` (config-gated checkpoint)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's persisted lost-partition record is a 0x448-byte structure carrying the full geometry evidence of the hit (bytes-per-sector, sectors-per-cluster, hidden sectors, CHS-alignment, disk number, applied-flag); the core's REC_PARTITION_CONTEXT persists only start/count/fs/score. The in-memory candidate's `evidence[]` list and `size_from_device_end` flag are dropped at save time, so a resumed session cannot distinguish a geometry-derived size from a device-tail fallback without re-probing.

## What the reference does

See M-042's corpus block — the record layout with the geometry payload is
fully decoded there and cross-validated against the shipped file.

## What our core does

Persists the minimum tuple; the evidence and size-source fields exist only
in memory.

## Gap

Falsifiable: save a session with a candidate whose `size_from_device_end`
is true, reload, and inspect the restored candidate — the flag is false
(default) because it was never serialized.

## Impact

Low: reload + materialize re-derives what it needs by re-reading the disk
(the re-validation gap is M-042's severity, not this one). The cost of the
thinner record is redundant re-probing and a less informative session file
for consumers. MINOR.

## Reproduction / verification

Core-side source read (cr_records.cpp:368-386 codec fields; cr_lostpart.h
field set). Corpus side per M-042. Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §D.

## Fix plan (for the engineering team)

Fold into M-042's fix step 2 (the append-only record extension: bps, SPC,
hidden_sectors, flags bit0=size_from_device_end, and optionally a capped
evidence list). This finding exists so the field list is pinned even if
M-042's re-validation is deferred.

## Tests to add

`CandidateGeometryRoundTrip`, `SizeSourceFlagRoundTrip` (both in M-042's
list).

## Related findings

M-042 (the parent finding — re-validation + layout snapshot).

## Open questions

None.
