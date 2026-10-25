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
