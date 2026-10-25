# OQ-S003 — the og's journal-header-invalid fallback salvage (:1690-1790)

**Status:** OPEN — precisely scoped reader, decode of the install target pending.
**Filed:** 2026-10-26 (S-003/S-021 fix session).
**Severity carried:** MINOR-class residual (the primary journal surfaces are
landed; this is a secondary salvage path).

## Question

What does `FUN_180003ea0(param_1, local_res10, _Memory, (ushort)uVar13)`
install, and into which cache, in the fallback flow decoded below - i.e.
does the og ever serve a journal-region node image as a REQUESTED block's
content through the node-read path, and under what exact caller?

## What is decoded (first-hand, corpus `03ad46c`, :1690-1790)

The journal-open reader: read the block at the journal start; if the JNL
header gates fail (magic/endian) OR the 0x2C alloc fails:
- allocate 0x8000;
- read `0x8000 / *(int*)(param_1+0xcc)` blocks at the journal start;
- walk candidate image sizes `uVar13 = (1 << n) * *(int*)(param_1+0xcc)`
  for n = 0.. while uVar13 <= 0x8000;
- the FIRST candidate passing the node-kind gate `FUN_180010720` is
  installed via `FUN_180003ea0(param_1, local_res10, _Memory, (ushort)uVar13)`;
- the flow serves the journal-protected-block reader (CHFSPlusFileCache /
  CHFSPlusRawfile family).

## Next concrete step

Decode `FUN_180003ea0` (find its definition; trace which map/list at
`param_1` receives the image and who consumes it); then decide whether the
core's journal pass needs a matching salvage (candidate: when the jnl
header is invalid, scan the journal region at power-of-2 × block-size
offsets for node-shaped images and harvest them like the GetDeleteRecord
images). Bounded implementation: ~40 lines inside the existing
`journal_harvest_pass`.

## Why not this session

The primary surfaces (S-021's differential harvest + S-003's outcome
parity) consumed the session's HFS+ budget; the fallback's CONSUMER is
undecoded and implementing it blind risks serving wrong node images - the
no-hallucination rule applies.

---

## Resolution

**Status:** RESOLVED (closing session, df5 lineage) — FUN_180003ea0 decoded
first-hand AND the salvage implemented.

**Decoded (HFSPlusSearchDLL @0x180003ea0, :2344-2560):** the installer loads the
candidate image via FUN_180010110 (the scratch node-object loader - the same
FUN_180010110 the ring walk uses; own-flag 0) and dispatches on the node kind:
- kind == 1 (header node): remembered as a tree-root location hint
  (ctx+0xf0 = the offset; the volume-open cache) - no records harvested;
- kind == -1 (leaf): records harvested by SHAPE classification
  (TestNodeBelongToFile = FUN_1800102d0 :12903-13100 - catalog folder
  0x58/type 1, file 0xF8/type 2, thread (nameLen+5)*2/type 3|4) into
  per-tree harvest maps with replace-on-same-key merges (the extents-key
  comparator {forkType, fileID, startBlock} at :2442-2476; attributes at
  :2494-2560). It NEVER installs into the live tree readers - the fallback
  feeds the SAME harvest maps the GetDeleteRecord consumption walks.

**Implemented:** `journal_fallback_salvage` in cr_hfsplus_engine.cpp - the
node-shaped trigger (BTNodeDescriptor reserved==0, kind leaf/header + height
gates), the 32 KiB window at jbase (short reads zero-filled - deterministic
where the og scans device garbage), the size ladder 1x/2x/4x.. x blockSize
(<= 0x8000), the first validator-passing node, shape-classified records into
the same kept/extents/attributes maps, header nodes counted but not harvested
(the og's tree-root hint has no consumer in the core's architecture).

**Verified by:** HfsPlus.JournalFallbackSalvageWhenHeaderInvalid (fail-before:
no salvage existed - part of the 6-test fail-before run in
evidence/executions/s003-s021-verification-corrections.md) +
HfsPlus.JournalFallbackRequiresNodeShapedStart (the trigger gate). Suite 491/491.
