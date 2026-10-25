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


## Q2 (appended by the S-021 convergence session, 2026-10-26) — verify the
recognition-gate constants against one real journaled volume

The harvest's gates are mechanically decoded and fixture-pinned, but two
constants remain derivation-unknown (a wrong value yields a silent
zero-harvest, never wrong results):

1. the transaction gate `u16@blocklist+0 == (size_a >> 4) - 1` (size_a =
   the u32 at jnl_header+0x20 - per the 44-byte journal_header layout that
   is the jnl.c `size` field's low half);
2. the og's cross-check `u64@jnl_header+0x18 == JournalInfoBlock.size`
   (+0x18 is jnl.c's `end` field per the same layout).

Both were independently decoded by three legs now (the s5 pass, the DF4
leg, and the final-sweep leg's from-scratch decode - all convergent on the
same arithmetic), but none had macOS tooling to confirm what real journals
carry. Next step: `diskutil enableJournal` (or mkfs.hfsplus -J) + one
delete + raw dump; hex-dump the JIB and the first block-list header;
confirm (a) the u16 at blocklist+0, (b) the u32 at jhdr+0x20, (c) whether
jhdr+0x18 == JIB.size on volumes the og actually harvests. The 44-byte
journal_header struct ends at jhdr_size (+0x28) - the three-field tail
{+0x20 u32, +0x24?, +0x28 u32} is the decode's remaining ambiguity.

Additional decode from the convergence session (first-hand, :13460-13560):
the JIB read offset is `volume.blockSize(+0x28) * journalInfoBlock(+0xC)`
(both fields of the og's volume object mirroring the VH); the 0xB4-byte
wrapper copy is db70-swapped BEFORE the flags-bit-0 gate (error
0x5003100000501); the jnl header is read at the JIB's offset field
(+0x24, post-swap) and its 44 bytes copied verbatim into the journal
object before the magic/endian/size gates (errors 0x5004000000501 /
0x5004100000501 / 0x5004200000501) - i.e. the og CACHES the header before
the walk, so a wrapped image clobbering the on-disk header mid-walk cannot
break the og's (or the core's) walk; only the SETUP read must see a valid
header.
