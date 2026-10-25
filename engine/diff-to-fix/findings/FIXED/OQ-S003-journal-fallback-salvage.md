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

## RESOLUTION (session 6, 2026-10-26 — FUN_180003ea0 decoded first-hand)

**Answer: YES — the og serves journal-region node images through the node-read path,
via the B-tree node cache.** Decode (corpus `03ad46c`,
`re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c:2344-2420`):

`FUN_180003ea0(param_1, param_2, param_3, param_4)` is the **node-cache installer**:
- `FUN_180010110(local_88, param_3, param_4, 0)` parses the candidate image as a B-tree
  node (the node-header parse; `local_88` becomes the parsed-node handle);
- `*(u64*)(param_1+0x130) = local_80` stores the parsed node id into the volume's
  node-state slot;
- the kind gate (`cVar10 == 1` → leaf/kind checks via `FUN_1800102d0`, with the
  `FUN_180010720` node-kind family the caller already applied) routes the install;
- the image is wired into the hot-node cache chain (`param_1+0xd8 → +8 → +0x10 → +0xf0`
  = param_2, with the `param_1+0x150`/`+0xe0` guards) — the CHFSPlusFileCache family
  that the node-read path consults before disk reads.

So the fallback flow (bad JNL header magic/endian, or the 0x2C alloc failure) brute-scans
`(1<<n) * block_size` candidate image sizes in the first 0x8000 bytes of the journal
region, and the FIRST candidate passing the node-kind gate is installed as a cache node —
journal-region content is then SERVED as node content on subsequent reads.

**The residual divergence (registering here as the OQ's disposition; MINOR-class):** the
core's journal reader (`src/core/hfsplus/cr_hfsplus_engine.cpp:757`) returns early on bad
JNL magic (`le32j(jh+0) != 0x4A4E4C78 || le32j(jh+4) != 0x12345678 → return`) — no
header-invalid fallback exists. Fix shape (if the team takes it): on header-gate failure,
brute-scan the journal region's first 32 KiB for kind-valid node images (the same
`FUN_180010720`-class kind gates the harvest already uses) and run them through the
existing journal-record harvest path (the S-003/S-021 fix's kept-newest map) rather than
seeding a node cache — same observable salvage without adding a cache layer. Tests:
`HfsPlus.JournalHeaderInvalidFallbackSalvage` (fixture: valid journal records behind a
corrupted 44-byte header → records still harvested), `HfsPlus.JournalHeaderValidUnaffected`.

**Disposition:** OQ RESOLVED (the install target + caller semantics decoded); the
salvage-path divergence itself is now precisely scoped for the fix legs (this file
carries it — no separate finding filed at MINOR-class pending the team's take/skip
decision; upgrade to a finding if the fixture proves real recall value).

## Parallel-leg addendum preserved in the merge (2026-10-26)

The parallel S-021 convergence leg's reading of this same flow (their file,
merged here on rename): the journal-open SETUP read caches the 44-byte header
(+0x24, post-swap; copied verbatim into the journal object) BEFORE the
magic/endian/size gates fire (errors 0x5004000000501 / 0x5004100000501 /
0x5004200000501) — i.e. the og caches the header before the walk, so a
wrapped image clobbering the on-disk header mid-walk cannot break the og's
(or the core's) walk; only the SETUP read must see a valid header. This
sharpens the residual scope above: the fallback fires when the SETUP read
itself fails the gates — exactly the case where the core returns early and
the og brute-scans.

## Q2 (re-appended by the S-021 convergence session — lost in the rename
race; the recognition-gate derivation question, distinct from the fallback)

The harvest's recognition gates are mechanically decoded and triple-convergent
(s5 pass, DF4 leg, final-sweep leg — all independently derived the same
arithmetic), but two constants remain derivation-unknown, and a wrong value
yields a silent ZERO-harvest (never wrong results):

1. the transaction gate `u16@blocklist+0 == (size_a >> 4) - 1` (size_a = the
   u32 at jnl_header+0x20);
2. the cross-check `u64@jnl_header+0x18 == JournalInfoBlock.size` (+0x18 is
   jnl.c `end` per the 44-byte journal_header layout).

Additional decode from the same session (:13460-13560, first-hand): the JIB
read offset is `volume.blockSize(+0x28) * journalInfoBlock(+0xC)` (the og
volume object mirrors the VH fields); the 0xB4-byte wrapper copy is
db70-swapped BEFORE the flags-bit-0 gate (error 0x5003100000501); the jnl
header is read at the JIB's offset field (+0x24, post-swap). Next step:
`diskutil enableJournal` (or `mkfs.hfsplus -J`) + one delete + raw dump;
confirm (a) the u16 at blocklist+0, (b) the u32 at jhdr+0x20, (c) whether
jhdr+0x18 == JIB.size on volumes the og actually harvests. No macOS tooling
in any session so far — this needs the bench-harness environment.


---

## Resolution (session 14, DFX14 — recorded from the closing-session record; the file bodies had gone stale)

**Status:** RESOLVED (the closing session's S-003/S-021 verification; this
session reconciled the records)
**Verified by:** the closing-session worklog record + the landed salvage
ladder in the S-003/S-021 finding (FIXED/S-003 + the S-021 addenda):
`FUN_180003ea0` decoded — the fallback installs a SCRATCH-NODE harvest
(the first candidate image that passes the node-kind gate is served as
the requested block's content), never a live-tree install; the core's
equivalent landed as the header-invalid fallback salvage ladder.
**Resolution notes:** the filename carried the RESOLVED stamp from the
closing session while the body kept the pre-resolution text; the stale
duplicate in FIXED/ (same pre-resolution body) is removed in the same
change — this file becomes the canonical record.
