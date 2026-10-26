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

## Resolution (session 23 — the container-map half; the finding is now fully closed)

**Status:** FIXED (recognizer half landed with S-020; container-map half landed this session)
**Fixed in commit:** (this session's DFX29 N-036 batch — the commit carrying this file)
**Verified by:** fail-before EXECUTED on the stash-reverted tree (translate_cluster temporarily reduced to identity): `Refs.ContainerRelativeRunTranslatedPerOgFormula` RED (extent 42*4096 vs the expected 302*4096) and `Refs.PhysicalRunFlag20Untranslated` RED (both files collapsed to extent 42 by M-027's placement-signature dedup — the second file never emitted); pass-after all 30 ReFS tests green, full suite **744/744** linux-release. New tests: `Refs.ContainerRelativeRunTranslatedPerOgFormula` (block 42, span 8, container 0 @base 300 -> (42 & 7) | 300 = 302), `Refs.PhysicalRunFlag20Untranslated` (flag 0x20 -> raw block even with a populated map), `Refs.ContainerBlockBelowSpanStaysPhysical` (block < span identity, og FUN_180011e90 :13237-42), `Refs.SingleContainerExtentsUnchanged` (the empty-map coincidence pinned both directions: the base fixture AND the thesis-grammar files on the container fixture).
**Resolution notes:** the dead top-nibble `translate_cluster` model is REPLACED by the og formula, re-verified first-hand this session at BOTH consumers (FUN_180011e90 :13223-13267 and FUN_180004820 :2355-2386 — byte-identical: identity when block < span or map empty or key absent; key `(((block/span)>>1)-2)*span`; rewrite `(block & (span-1)) | base`; u64 wrap arithmetic preserved exactly). The span is DBR +0x34 (volume+0x148 -> parser+0xa8 via FUN_180011cd0's field copy — decoded this session, resolving the finding's "container-cluster count" identification), now carried as `RefsBoot::container_clusters`. The map is populated from the system-directory container records per FUN_180002a60 :1302-1342 (0xa0/0xe0-length keys, {base@+0x90, span@+0x98} / {base@+0xd0, span@+0xd8}, the span field MUST equal DBR +0x34 — the og's acceptance gate), collected during the ObjectID-table walk (zero extra I/O) plus a bounded sweep of system tables (IDs < 0x100; user trees are 0x600+ — the sweep never doubles the content-walk cost). Map key = counter*span (the og's +0x14c counter), value = the container base cluster. RESIDUAL (recorded, does not block closure): the og's record payload carries its key at payload+0x20 (FUN_180006420-context :8043-8050), so an alternative key-relative reading (key_len 0x80/0xc0 with fields at +0x70/+0x78, +0xb0/+0xb8) cannot be excluded from the decompile alone — the auditor's key-length-0xa0/0xe0 reading is implemented (the unit==span gate is the discriminator either way); a real multi-container volume dump (the finding's own open question; none in corpus) would settle the shape in one run and is the bench stage's next probe. On every single-container volume the map is empty and translation is identity — zero behavior change (pinned by test). Fix-plan step 4 (PARITY-DELTAS registration) is deferred to that probe per its own wording ("once the real-volume probe lands").
