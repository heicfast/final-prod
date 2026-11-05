# MAJOR M-144 — ext4 large-volume refusal caps the og does not have: the 8 MiB GDT read cap refuses 64BIT volumes above 16 TiB and `MAX_GROUPS` refuses above 128 TiB; the og allocates the GDT unbounded and scans

**Severity:** MAJOR
**Module:** ext4 (volume capacity bounds)
**Parameters affected:** B1 (detection correctness — legitimate volumes refused), X52 (scalability), X49 (memory — the og's stance is the unbounded one here)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` FUN_1800047c0 (:3404-3408 — the GDT allocation `round_up(groups × desc_size, block_size)` with no cap, re-read first-hand); the og validator (:3236-3239) carries no group-count cap
**Core references:** audited `34d6cf10` — `src/core/ext4/cr_ext4_engine.cpp:718-722` (`gdt_bytes > 8 MiB → ERR_BAD_PARAMETER`) and `:160-162` (`groups > MAX_GROUPS (1<<20) → false`); neither cap is in PARITY-DELTAS (AUDIT.md row 204 references MAX_GROUPS only as a fixture bound)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core refuses to open ext4 volumes whose group-descriptor table exceeds 8 MiB (a 64BIT volume with 64-byte descriptors above 16 TiB; 32-byte descriptors above 32 TiB) and any volume above 2²⁰ block groups (above 128 TiB at 4-KiB blocks). The og has neither cap: it allocates the rounded-up GDT buffer and scans. Large-but-legitimate ext4 volumes (big NAS arrays) get zero engine-side recovery in the core while the og scans them.

## What the reference does

FUN_1800047c0 (:3404-3408), the og's mount-time GDT setup — re-read first-hand this session:

```c
uVar17 = *(uint *)(param_1 + 0x58);                       /* block_size */
_Var19 = (__uint64)(int)(((iVar13 * uVar21 + -1 + uVar17) / uVar17) * uVar17);
                                                          /* round_up(groups × desc_size, block_size) */
uVar16 = thunk_FUN_180013b78(_Var19);                     /* allocate — no size test */
if (uVar21 == 0x20) { *(undefined8 *)(param_1 + 0x498) = uVar16; … }
```

No cap on the product; a 100-TiB 64BIT volume (≈655M groups × 64 B ≈ 42 GiB… at 4-KiB blocks a 100-TiB volume is ~26.8M groups ≈ 1.7 GiB of GDT) allocates and scans. The superblock validator (:3236-3239) checks geometry consistency but no group-count ceiling.

## What our core does

`cr_ext4_engine.cpp:160-162`: `if (groups == 0 || groups > blocks || groups > MAX_GROUPS) return false;` with `MAX_GROUPS = 1<<20` — refuses above 1,048,576 groups. And `:718-722`:

```cpp
const u64 gdt_bytes = v.sb.groups * v.sb.desc_size;
if (gdt_bytes == 0 || gdt_bytes > (8ull << 20))
    return ERR_BAD_PARAMETER;
v.gdt.resize(size_t(gdt_bytes));
```

At 4-KiB blocks with 64-byte descriptors: 8 MiB / 64 B = 131,072 groups = 16 TiB exactly — the `>` keeps 16 TiB passing and refuses anything larger.

## Gap

Falsifiable: synthesize (or mke2fs on a sparse file) a >16-TiB 64BIT ext4 geometry (valid SB/GDT, 4-KiB blocks, 64-B descriptors): the og's engine opens and walks; the core returns ERR_BAD_PARAMETER at open — whole volume skipped. The 128-TiB `MAX_GROUPS` line refuses a second population.

## Impact

Legitimate large-volume recovery loss (modern NAS arrays routinely exceed 16 TiB; multi-drive ext4 arrays exceed 128 TiB). The og's unbounded allocation is its own risk (a corrupt SB could request a huge buffer — the og accepts that risk), but the core's 8 MiB constant looks like a hardening default that became a capability cliff; neither cap was ever registered as a deliberate divergence.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand: the allocation computation and the absence of a size test (:3404-3408), the validator's geometry-only checks (:3236-3239). Core side: both caps quoted at `34d6cf10`; the PARITY-DELTAS grep confirms neither is registered. Not executed this session (a 16-TiB sparse fixture is feasible — the fix plan's first test constructs the geometry synthetically, no 16 TiB of storage needed: the engine reads the GDT block range only).

## Fix plan (for the engineering team)

1. Replace the whole-GDT single read with a bounded streaming read (the engine's group lookups are `group_at(i)`-style — read the GDT in 8-MiB chunks through a small cache, or mmap-style window) and drop the byte cap.
2. Raise `MAX_GROUPS` to a memory-aware bound tied to the streaming design (e.g. 2²⁶ groups ≈ 1 PiB at 4-KiB — beyond any real volume while keeping the corrupt-SB guard the constant legitimately provides), or keep it and REGISTER the refusal threshold.
3. Register the decision in PARITY-DELTAS either way (the og is unbounded; whatever the core does beyond that is a deliberate stance).
4. Files: `src/core/ext4/cr_ext4_engine.cpp` (+ the group-access helper), `docs/PARITY-DELTAS.md`.
5. Acceptance criteria: a synthetic >16-TiB geometry fixture (sparse) opens and walks; a corrupt-SB huge-group-count still refused by the registered bound; memory peak bounded by the chunk size, not the GDT size.

## Tests to add

- `Ext4.SyntheticLarge64BitVolumeOpens` (16-TiB-plus geometry on a sparse image) — RED today.
- `Ext4.GdtStreamedGroupLookupsConsistent` (random group reads vs a materialized control).
- `Ext4.CorruptHugeGroupCountStillRefused` (the registered bound pinned).

## Related findings

- M-138..M-142 (the acceptance-band family — this is the capacity edge of the same "core refuses what the og scans" class).
- S-065 (the other ext4 bound: the missing size bound on the ladder — the two findings move the engine in opposite directions; land both, they are independent).

## Open questions

None — both caps are directly read on the core side and both og sites directly read; the only decision is the replacement bound's value and its registration.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's push — SHA recorded at commit time)
**Verified by:**
- `Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens` — RED pre-fix (returns
  `ERR_BAD_PARAMETER` 0x50030003 when the 8 MiB GDT cap refused a 12.8 MiB
  GDT 64BIT volume); GREEN post-fix (the chunked reader opens the volume
  and walks every group: `st.block_groups == 200000`). Verified locally
  against a standalone gtest harness.
- `Ext4EngineM144.CorruptHugeGroupCountStillRefused` — GREEN both pre- and
  post-fix (asserts the raised `MAX_GROUPS = 1<<26` bound still refuses a
  corrupt SB with `groups = (1<<26)+1` — the corrupt-SB guard the cap
  legitimately provides).

**Resolution notes:** the 8 MiB single-shot GDT read cap at
`cr_ext4_engine.cpp:785` is removed and replaced with an 8 MiB chunked
reader that tolerates partial chunk failure (the dead-front behavior the
prior single-shot read applied to the whole table now applies per-chunk).
`MAX_GROUPS` at `:83` is raised from `1<<20` (~128 TiB) to `1<<26`
(~1 PiB) — still a corrupt-SB guard that no real volume hits, but no
longer a cliff legitimate NAS arrays cross. The alt-GDT ladder path
(:830-836) is chunked identically. The og's unbounded allocation
(FUN_1800047c0 :3404-3408, re-read first-hand this session) is its own
defect class (a corrupt SB could request an arbitrary buffer); the
core's deliberate divergence is registered in PARITY-DELTAS D-148. No
safety hardening was sacrificed: the chunked reader bounds per-read
length the same way; the MAX_GROUPS bound keeps the corrupt-SB guard;
the SB validator's existing `groups > blocks` geometry-consistency
refusal survives. The two fixtures use 4 KiB blocks, 64-B descriptors,
64BIT feature_incompat — the modern large-volume shape; no 16 TiB of
storage is needed (the engine reads only the SB, the GDT block range,
and the group-0 inode table to validate geometry).
