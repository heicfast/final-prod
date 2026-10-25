# OQ-S4 — NTFS size-0 deleted rescue chain (S-012 step-3 residual)

**Origin:** split out of S-012 (session 4) — the membership flip (step 1) and
the pass-2 GUID-dump walk (step 2) landed; the full rescue chain remains.
**Module:** ntfs · **Parameters:** B1/B2 (deleted size-0 records' content recovery)
**Corpus:** NTFSSearchDLL_decompiled.c FUN_18001e6c0 :24022-24168 (chain),
FUN_18001e120 :23722-380 (MFT-copy sector walk), FUN_18001d690 :23215-388
(same-name merge), FUN_18001dd80 :23575+ (resident extraction), FUN_18001d960
:23391+ (runlist adoption), FUN_18001e340 (registry insert), FUN_180016440
:17715-17785 (re-emit code 0xfe0001).

## What is decoded (session-4 first-hand evidence, recorded so the next leg
does not re-derive it)

- Trigger (pass 1, :18493-18499): deleted + non-dir + props size 0 + no
  runlist -> FUN_18001e6c0; item marked code 2 ("rescued"); the record is
  EXCLUDED from the claim map (bVar4=false at :18490-93 gates the claim at
  :18523).
- FUN_18001e6c0 order: (a) MFT-copy sector-list walk (FUN_18001e120) when the
  record-parse context carries a non-empty (sector, record#) list — reads 2
  sectors per entry, fixup, self-record-number match via FUN_180041820 (=
  the XP+ u32 record-number field at +0x1C), resident-$DATA extraction,
  content appended into a 0x400-capped buffer, at most 9 fragments
  (:23776 `8 < iVar8` break); (b) else same-name merge FUN_18001d690 probe
  mode; (c) direct resident extraction from the record buffer
  (FUN_18001dd80, NULL copy-buffer); (d) second same-name merge in force
  mode ('\x01'); (e) runlist adoption FUN_18001d960 when content was found
  (gate :23456: `0 < cap && total_clusters < cap` -> adopt, props+0x72 |= 4);
  on any success props+0x72 |= 2 and the name is registered for re-emit
  (FUN_180003be0/FUN_180020bd0) with code 0xfe0001 later.
- Same-name merge semantics (FUN_18001d690): iterates a record#-keyed
  registry (engine+0x38 -> +0x60 map; entries carry props +0x48, time +0x50,
  parent +0x58); candidate must have a runlist + nonzero size; name match =
  TARGET NAME ENDS WITH candidate name; match gate = force mode OR
  (candidate parent == target record#) OR (candidate time == pass-1
  timestamp uVar12); compressed candidates skipped (props word & 0x800);
  force-mode registry cap 5000 entries; on match: adopt candidate runlist
  (FUN_180003250) + size, return 1.

## What remains undecoded (the blocker)

- How the (sector, record#) list inside the record-parse context is
  POPULATED: FUN_180042ef0 (:53172) = FUN_180042a80 (buffer parse object) +
  FUN_180042420 (context write, param_5 = the list) + FUN_180042320. The
  walker passes `*param_10` (:18462) — the iteration state's list object.
  Whether entries come from $MFTMirr reads, remnant probing, or the MFT
  read path itself is NOT yet proven from the decompile.
- Where the record#-keyed registry (same-name merge input) is populated —
  candidates: FUN_1800198a0 (called at :18477 right after props claim) or
  FUN_18001e340's own insert. Not yet traced.

## Why the core's current state is honest

The core's main walk already covers path (c) (resident-$DATA extraction in
build_props — a deleted size-$FILE_NAME record with resident content gets
size = value_length) and emits every record (S-012 step 1), so the missing
piece is (a)+(b)+(d)+(e): MFT-copy content stitching, same-name
runlist/size stealing, and the rescued marker. No wrong results are
produced — the rescued CONTENT is simply not yet stitched (size-0 nodes
stay size-0 instead of gaining content from a mirror copy or a same-named
sibling record).

## Next concrete step

Decode FUN_180042420/FUN_180042320 + the walker's param_10 origin (the
(sector, record#) list population), then implement (a); the registry
population trace decides (b)/(d) fidelity. Then the "rescued" state marker
(0xfe0001) maps onto a node flag in the same change.
