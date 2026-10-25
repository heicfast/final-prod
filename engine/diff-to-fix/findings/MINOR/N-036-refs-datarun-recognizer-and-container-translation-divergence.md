# MINOR N-036 — ReFS data-run entry recognizer and container translation: flag-bit and block-number models diverge

**Severity:** MINOR (single-container volumes unaffected; real-volume polarity UNVERIFIED — see below)
**Module:** refs
**Parameters affected:** A27 (fragment/runlist reconstruction), C20 (struct field semantics), B2 (partial)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — FUN_18000c790 :8854–8934 (run-entry grammar: flags u16@+8 with **0x40 required** :8880–8882, ref-size u16@+0xa==0x18 :8883–8885, block u64@+0 :8888, offset u32@+0xc :8887, length u32@+0x14 :8886; **flag 0x20 = physical/no-translate** :8893, translation call :8894), FUN_180011e90 :13223–13267 (translation: identity when block < cluster or no map :13237–13242; key `((block/cluster)>>1 − 2)*cluster` :13244; rewrite `(block & (cluster−1)) | base` :13264), FUN_180002a60 :1302–1342 + FUN_180011cd0 device-map handoff via FUN_180003f40 :1995–1999 (container map build), FUN_180011b50 :13117–13146 (page-read translation, same formula)
**Core references:** `src/core/refs/cr_refs_engine.cpp` @ `34b3441` — RUN_HAS_DATA = 0x0010 (:42); parse_data_runs :293–310 (`r.lcn = le64(val+0)`, `flags = le16(val+8)`, `clusters = le32(val+0x14)`, keep iff `flags & 0x10` :305); `translate_cluster` :73–82 (**top-nibble container tag; `container_lcn` map declared :72 and only ever searched :77–78 — never populated**)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Both engines parse ReFS data-run entries as `{u64 block@+0, u16 flags@+8, …, u32 clusters@+0x14}`,
but (a) the og requires flag **0x40** ("carries a block reference") and has no 0x10 test, while
the core requires flag **0x10** ("has data") and has no 0x40 test; (b) the og translates
non-physical runs (flag 0x20 clear) through a per-volume container map built from the
system-directory records, using the formula `base = map[((block/cluster)>>1) − 2]`,
`block' = (block & (cluster−1)) | base`; the core strips a top-nibble tag and adds a
never-populated map — i.e. it treats every run LCN as physical.

## What the reference does

Citations in the header. Two run flavors exist on the og's model: flag 0x20 set = physical
cluster number (used as-is); flag 0x20 clear = container/virtual block number, rewritten via
the container map before use (FUN_18000c790:8893–8895). The same translation is applied to
page/block references during tree walks (FUN_180011b50:13117–13146, FUN_180011e90) and to
post-walk block lists (FUN_180004820:2355–2386).

## What our core does

`parse_data_runs` keeps a run only when `flags & 0x10` (:305 — "sparse hole" skip) and never
tests 0x40; `translate_cluster` (:73–82) masks a top-nibble container tag and adds
`container_lcn[tag-1]` — but **no code in the file ever inserts into `container_lcn`**
(declaration :72 is the only write-free use; grep-verified: only `.find` at :77). Every run
LCN is therefore consumed as a physical cluster number, and there is no physical/virtual
flavor distinction.

## Gap

Falsifiable: a run entry with flags 0x40 but not 0x10 (og: recovered; core: skipped as a
"sparse hole"); a run entry with 0x10 but not 0x40 (core: recovered; og: skipped as a
non-reference entry); a container-relative run (flag 0x20 clear) on a multi-container volume
(og: translated to the right physical clusters; core: reads the wrong clusters — extent
correctness depends on the never-populated map).

## Impact

- On single-container volumes where runs are physical LCNs, behavior coincides (both engines
  recover the same extents) — the common case, hence MINOR.
- On multi-container volumes (Storage Spaces-style ReFS), the core's extents for
  container-relative runs would be wrong or dropped — silent wrong-extent risk, but the
  premise (container-relative runs in $DATA tables on such volumes) is **UNVERIFIED** from
  the decompile alone; no fixture exists. Flagged for the bench stage: a real-volume
  comparative run would settle it (if extents hash-match, runs are physical everywhere and
  this closes as INFO).
- The flag-bit asymmetry (0x40 vs 0x10) is a superset/subset pair: entries carrying 0x40
  without 0x10 lose recovery in our core; entries carrying 0x10 without 0x40 are a
  recovery-positive superset in ours.

## Reproduction / verification

Decompiled citations (og) + core citations read first-hand; the never-populated map is
grep-verifiable (`container_lcn` appears at :72 and :77–78 only). No executed comparison
(no ReFS fixture; single-container synthetic fixtures in tests/unit/test_refs_volume.cpp
model physical runs and pass today — consistent with the single-container coincidence).

## Fix plan (for the engineering team)

1. In `parse_data_runs`, require the og's recognizer: flags & 0x40 (block-reference present);
   treat flags & 0x20 as physical; keep 0x10 as an additional keep-signal only if the
   thesis fixtures require it (document which bit set survives).
2. Populate `container_lcn` during the ObjectID-table walk: the og's container map comes from
   the system-directory records (32-byte-ID keys of length 0xa0/0xe0 whose second ID half
   equals the DBR container-cluster count, FUN_180002a60:1302–1342). Implement the og formula
   (`index = (block/cluster)>>1 − 2`, `block' = (block & (cluster−1)) | map[index]`) in
   `translate_cluster` — replacing the top-nibble model — or, if the bench stage proves runs
   are always physical, delete the dead map and document the simplification (preferred if
   true: less code).
3. Add the physical/virtual flavor to the DataRun struct so step 2 is testable.
4. Register the resolution in `docs/parity/PARITY-DELTAS.md` once the real-volume probe lands.

## Tests to add

- `Refs_Run_Flag40_Without_10_Recovered` (flags=0x40 → run kept).
- `Refs_Run_Physical_Flag20_Untranslated` (flag 0x20 set → block used as-is even when a
  container map exists).
- `Refs_Container_Relative_Run_Translated` (synthetic 2-container map → run rewritten per og
  formula).
- `Refs_SingleContainer_Coincidence_Regression` (physical-only volume: extents unchanged).

## Related findings

- S-020 (attribute-layer breadth — the loop that feeds parse_data_runs; fix together).
- I-004 / N-026 group item N-013b (other ReFS gate divergences).

## Open questions

- Do real-volume $DATA run entries ever carry the container-relative flavor (flag 0x20
  clear)? Next artifact: a real ReFS volume dump (none in corpus — vendor QA scanned none) or
  the `RefsFileSystemAnalyser.mo` decompile (`CRefsAllocatorMgr`/`CRefsBlockRefTableProc`,
  ghidra-mo) which builds the partition-side equivalent of the container table.

## Resolution

**Status:** PARTIALLY FIXED (recognizer half landed with S-020; the container-map half remains open — the real-volume probe is still the deciding evidence)
**Fixed in commit:** (this commit — the recognizer half)
**Verified by:** the S-020 test set (Refs.MultipleDataAttributesMerged / PagedRunTableFollowed exercise the recognizer through the thesis flag; the og 0x40-recognizer branch is the attr_walk_runs og_run gate, corpus-cited inline).
**Resolution notes:** parse-run recognition now accepts BOTH flavors: the og recognizer (flags & 0x40 with ref-size u16@+0xa == 0x18 — entries carrying 0x40 without 0x10 are no longer dropped) and the thesis grammar (flags & 0x10, the historical fixture form); flag 0x20 marks physical (untranslated) blocks; container-relative blocks go through translate_cluster; block 0 entries are skipped (og :8891-95). The `local_f8`... correction: the DEAD container_lcn map is still dead — populating it needs the system-directory container-table decode (FUN_180002a60) plus a real multi-container volume to validate against; that stays this finding's open scope with its own tests-to-add list (Refs_Container_Relative_Run_Translated et al.). No behavior change on single-container volumes (the translate is identity there).
