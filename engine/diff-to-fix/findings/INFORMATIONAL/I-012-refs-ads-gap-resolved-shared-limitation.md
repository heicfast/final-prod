# INFORMATIONAL I-012 — GAP-ADS resolved: the og does not recover ReFS named streams (ADS) either — core is at parity; shared limitation

**Severity:** INFORMATIONAL (mandate item resolution — "ReFS and LDM are first-class")
**Module:** refs
**Parameters affected:** B7 (ADS handling), B2 (recovery correctness — bounded), D13 (report fields)
**Corpus references:** `og-codebase/ReFSSearchDLL.dll` RTTI census (no stream/named-stream class in the binary — first-hand strings extraction this session); `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` :8233+ (FUN_18000ba90 file-row handler: ONE properties block `local_e8..local_76`, ONE runlist at `local_7e`, ONE emission `FUN_18000d970` per file row); `re-workspace/artifacts/SaveScan/ifileproperties-part2--ext4-hfs-refs.md` §3 (RefsFileProperties type-6 .rsf layout: `name, size, flags, created, modified, accessed, marker, objectId, ONE DataRunList` — no stream field, 100% [CODE])
**Core references:** audited commit `34b3441` — `src/core/refs/cr_refs_engine.cpp:738-760` (attribute loop; `break; /* default (unnamed) data stream */` at :759); the project's own GAP-ADS register entry (docs/) describing ReFS ADS as unrecovered
**Status:** READ-ONLY FINDING — no fix applied (no divergence)

## Summary

The session-1/2 handoff carried "ReFS ADS recovery gap (project GAP-ADS still open — the mandate's 'ReFS first-class' clause)" as an open mandate item. This session decoded the og's ReFS file pipeline end-to-end enough to settle it: **the og's ReFSSearchDLL does not model named data streams at all** — one record per file row, one name, one size, one runlist, persisted that way in .rsf. The core's behavior (take the default stream only) is **MATCH**, not a capability gap. GAP-ADS is a shared limitation of both implementations, not a core-vs-og divergence.

## What the reference does

Three independent evidence lines, all first-hand this session:

1. **RTTI census**: the full readable-strings extraction of `ReFSSearchDLL.dll` contains the complete `refs::` class family (`RefsFileProperties`, `RefsFileValues`, `RefsDirectoryValues`, `RefsNodeRecord/Header`, `RefsMSTreeHeaderDesc`, `RefsSupperBlock`, `RefsDbr`, `RefsCheckPoint`, `RefsMetadataBlock*`, `CReFSDataRunList`, `CRefsResidentDataRunList`, `CNoneResidentNTFSDataRunList`, `IDataRunList`) — **no stream or named-stream class exists**.
2. **The file-row handler** `FUN_18000ba90` (:8233, dispatched for directory-table rows of type 0x30/subtype 1, matching the core's `ROW_FILE = 0x00010030`): builds exactly one property block (`local_e8`..`local_76` mirrors the `RefsFileProperties` field layout), one runlist container (`local_7e`), calls one emit (`FUN_18000d970`). No per-stream iteration, no stream-name construction (no `':'` separator logic anywhere).
3. **The .rsf contract** (corpus artifact, 100% [CODE]): the persisted `RefsFileProperties` carries `{name, size, flags, created, modified, accessed, marker, objectId, DataRunList}` — a single runlist, a single name; the session format cannot even represent a second stream.

## What our core does

`emit_file` (cr_refs_engine.cpp:715-783) walks the file row's embedded table, takes the `$DATA` attribute (type 0x80 at key+8), parses its run table, and `break`s — the default stream only (:759). Same shape as the og: one record, one runlist.

## Gap

None on ADS. (The file-value walk's type-space/grammar divergence is a SEPARATE finding — M-022 — and is not about streams.)

## Impact

The "ReFS first-class" mandate clause is satisfied by parity-of-limitation: no user-visible difference between the products on ReFS ADS. If the project wants ADS recovery as a superset feature, it must be driven by product requirements, not parity — record in the gap register as `GAP-ADS: shared limitation (I-012), superset opportunity, not a parity debt`.

## Reproduction / verification

Corpus-derived (three independent lines above; decompile + binary strings + rsf artifact). No execution needed — the og's inability is structural (no code path, no data model).

## Fix plan (for the engineering team)

None required for parity. Optional superset (out of parity scope): emit named ReFS streams as child nodes mirroring the NTFS ADS pattern (N-011 already files the NTFS-side superset); would require extending `RefsFileProperties`-equivalent emission and the session format.

## Tests to add

None (parity). If the superset is built later: `Refs.NamedStreamEmittedAsChild`.

## Related findings

M-022 (file-value grammar divergence — filed same session), N-011 (NTFS ADS superset — the analogous NTFS-side question, where the og DROPS ADS and we emit them), N-007/I-004 (ReFS detection/version, sessions 1-2).

## Open questions

None.
