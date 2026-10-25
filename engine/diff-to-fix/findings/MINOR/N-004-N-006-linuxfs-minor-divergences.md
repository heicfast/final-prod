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

## Resolution (N-004 portion)

**Status:** FIXED
**Fixed in commit:** this session's linuxfs-minors batch (RetryPolicy::backtrack in cr_device.h; shifted-window second-chance in cr_retry.cpp)
**Verified by:** `Retry.BacktrackReadSalvagesPartialSector` (alignment-dependent transport fails only requests starting at a given offset; the shifted window succeeds and the bytes arrive — no bad range recorded), `Retry.BacktrackOffReproducesLegacyExhausted` (flag off = the exact pre-fix behavior: ERR_RETRY_EXHAUSTED + mapped range), `Retry.BacktrackStaysAlignedOn4Kn`. Fail-before: the tests do not exist on the pre-fix tree; the flag-off test IS the pre-fix behavioral pin. Suite 458/458 ASan.
**Resolution notes:** the og's CEXT4CatchBufferDeviceProxy salvage (corpus ext4-engine--deep-dive.md §3, 0xE00 fresh buffer + 0x400 backtrack re-read) is now implemented at the core's designated bad-sector layering boundary: on a persistently failing range, ONE shifted-window second-chance re-read [off−back, off−back+win) before the range is bisected/given up. Deliberate improvements over the og: back = max(0x400, sector) — the og's hardcoded 0x400 is misaligned on 4Kn, ours stays aligned; the window end is sector-rounded; the fresh buffer is heap-allocated per catch (like the og's 0xE00 allocation) so no large stack frames on worker threads. Default ON (the og always catches); RetryPolicy::backtrack=false is the policy escape hatch. Media-bad ranges are unaffected (the shifted window contains the failing span and fails too) — existing Retry tests pass unchanged by construction.

## Resolution (N-006 portion)

**Status:** FIXED (registered as deliberate hardening + pinned)
**Fixed in commit:** this session's linuxfs-minors batch (pin test only — the validation code is unchanged and correct as-is)
**Verified by:** `Btrfs.NonPow2NodesizeRejectedRegistered` — 24 KiB nodesize (og-band-legal, 0x6000%0x1000==0, so the og's BtrfsSearchDLL :10302-10304 accepts it) is rejected by the core's pow2 gate; 32 KiB (pow2, in-band) accepted. Suite 458/458.
**Resolution notes:** the finding's optional fix plan (accept non-pow2 within the og band + cap + PartialRisk) is NOT adopted: real mkfs.btrfs only writes pow2 nodesizes, so non-pow2 is corrupt-SB territory; a forged nodesize would size node-buffer allocations from an attacker-influenced field; and refusal routes the volume to other engines/carving — a strictly better outcome than the og's garbage attempt. Registered as PARITY-DELTAS D-62a (do not loosen; D-61 is the parallel leg's A-005 RawFixer entry - renumbered here). The finding's own test name `SB_NonPow2_Nodesize_Rejected_Registered` is honored.
