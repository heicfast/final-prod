# MINOR N-004 — ext4 bad-sector salvage lives in the transport layer, not the in-engine CatchBuffer backtrack
# MINOR N-005 — HFS+ block-size bounds stricter than the og gate
# MINOR N-006 — Btrfs nodesize validation stricter than the og band

Three refuse-vs-attempt / layering minors from the ext4/HFS+/Btrfs wave, filed together.

---

## N-004 — ext4 CatchBuffer layering

**Severity:** MINOR · **Module:** ext4 · **Parameters:** F60 (bad-sector handling), R58 (layering)
**Corpus references:** `re-workspace/artifacts/SearchDLL/ext4-engine--deep-dive.md` §3 —
`CEXT4CatchBufferDeviceProxy` wraps the device; on read failure allocates a fresh
**0xE00-byte** buffer and retries with a shifted window (**1 KB backtrack**)
(agent-B verified allocations at decompile :330/:638; "Program is trying to skip bad
sectors to find data" UX maps here).
**Core references:** `include/cr/ext4/cr_ext4_engine.h` (header comment:
"CatchBuffer-style bad-sector salvage is delegated to the transport retry layer");
`src/core/storage/cr_retry.cpp` (bisection to sector granularity, Skip zero-fill) — audited
commit `67193b6`.

**Gap:** the og salvages inside the engine with a 1 KB backtrack re-read (sub-sector-shifted
retry); the core's retry layer bisects failing reads to sector granularity and zero-fills
gaps. Both tolerate bad sectors; the salvaged byte sets can differ in the last partial
sector of a failing range. No user-visible difference was demonstrable from the corpus;
recorded as a mechanism divergence with a note that any ext4-on-dying-disk comparative
bench should diff content hashes per-file, not just inventory sets.

**Fix plan (optional):** add a shifted-window second-chance read in the retry layer's
failure path (read [off−0x400, off+len) when the primary window fails) behind a flag;
measure on a scratched-media fixture before adopting. Files: `src/core/storage/cr_retry.cpp`.
**Test:** `Retry_Backtrack_Read_Salvages_Partial_Sector`.

---

## N-005 — HFS+ block-size bounds

**Severity:** MINOR · **Module:** hfsplus · **Parameters:** B1 (detection), S35 (refuse-vs-attempt)
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c`
:10005–10010 (agent-B verified): accept iff signature H+/HX **and** `(blockSize & 0x1FF) == 0`
**and** `version < 0x11` **and** alloc/extents fork clamps — **no min/max bounds on
blockSize at this site** (blockSize 0 passes the mask).
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp:337`
(`block_size < 512 || block_size > 65536 → reject`) — audited commit `67193b6`.

**Gap:** a damaged volume header with blockSize 0 (or a legal-but-large 512-multiple
> 64 KiB) is attempted by the og and refused by the core. blockSize 0 would then break the
og's own geometry math (division) — the og's "attempt" likely yields nothing; the
> 64 KiB case is the real availability delta (legal on very large volumes formatted with
big allocation blocks).

**Fix plan:** relax the upper bound to `<= (1 << 24)` with a total-blocks sanity product
check (block_size × total_blocks ≤ 2× device size, mirroring the NTFS P1-3 guard); keep
the ≥ 512 floor (blockSize 0 is geometrically unusable — register as deliberate hardening).
Files: `src/core/hfsplus/cr_hfsplus_engine.cpp:337`.
**Tests:** `VH_Large_BlockSize_Accepted` (1 MiB blocks, valid geometry → open OK);
`VH_Zero_BlockSize_Rejected` (pin the deliberate floor).

---

## N-006 — Btrfs nodesize validation

**Severity:** MINOR · **Module:** btrfs · **Parameters:** B1 (detection), S35
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/BtrfsSearchDLL_decompiled.c`
:10302–10304 (agent-B verified): sectorsize pow2; **nodesize band [0x86, 0x86+0xFF7B) with
NO pow2 requirement**; `nodesize % sectorsize == 0`; newest-generation SB wins.
**Core references:** `src/core/btrfs/cr_btrfs_engine.cpp:298–304` — adds `nodesize` pow2,
`≤ 0x10000`, `≥ sectorsize` — audited commit `67193b6`.

**Gap:** a corrupt SB with nodesize 0x6000 (24 KiB, non-pow2, og-legal per the band) is
attempted by the og and refused by the core. Real mkfs.btrfs only creates pow2 nodesizes
(4K–64K), so this is corrupt-SB territory only; core's refusal routes the volume to other
engines/carving, og's attempt yields nothing or garbage. Core's `≥ sectorsize` and `%`
checks jointly subsume the og's `%` check for pow2 values.

**Fix plan:** optional — accept non-pow2 nodesize within the og band but cap total node
buffer allocation (`nodesize ≤ 0x10000 + slack`) and mark such volumes
`Confidence::PartialRisk` at detection. Register the strictness as deliberate hardening if
not changed. Files: `src/core/btrfs/cr_btrfs_engine.cpp:299–303`.
**Tests:** `SB_NonPow2_Nodesize_Rejected_Registered` (pins current behavior either way).

---

**Related findings:** M-001 (the NTFS instance of the same refuse-vs-attempt family).
**Open questions:** none.
