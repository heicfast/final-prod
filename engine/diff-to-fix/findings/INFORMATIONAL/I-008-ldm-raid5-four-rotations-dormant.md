# INFORMATIONAL I-008 — The og's Raid5VolumeStorage carries four selectable RAID-5 rotations (only left-symmetric is ever enabled); the core implements the enabled one exactly

**Severity:** INFORMATIONAL (deliberate minimalism recorded, not a defect; the enabled-mode behavior is a proven MATCH)
**Module:** ldm
**Parameters affected:** A25 (engine algorithm steps), A27 (fragment handling — stripe mapping), B2 (recovery correctness — verified equal on the enabled mode)
**Corpus references:** `re-workspace/reversed/ghidra-mo/LdmManager_decompiled.c :68777-68876` (`Raid5VolumeStorage::ReadSector`, mapped via SEH crash-handler string "Raid5VolumeStorage::ReadSector",0xad at :88663 → function ending 0x180086ff7), `:68718`/`:68829` (rotation selector read at this+0x2a8 in WriteSector/ReadSector), `:68942` (constructor — the ONLY write to +0x2a8 in the entire 91,475-line decompile, initialized to 0), `:68393-68451` (`ComputeDataWithOtherDisk` — the XOR reconstruct), `:68901-68902` (Raid5VolumeStorage vftables)
**Core references:** `src/core/table/cr_ldm.cpp:1349-1389` (RAID-5 mapping + reconstruct) — audited commit `2545b83`
**Status:** READ-ONLY FINDING — no fix applied

*(Session-2 renumbering note: this finding was originally issued as I-006 by the LDM
deep-pass leg; renumbered to I-008 to clear the ID collision with the parallel
session-2 leg's I-006 ext4-nanoseconds finding. References updated across the
roadmap.)*

## Summary

The og's RAID-5 read path branches on a rotation-mode field (`this+0x2a8`) with four variants — left-symmetric (0), left-asymmetric (1), right-symmetric (2), right-asymmetric (3). The field is constructor-initialized to 0 and never written anywhere else in LdmManager.mo, so every LDM RAID-5 volume the og builds uses left-symmetric. The core implements left-symmetric only; its formula is algebraically identical to the og's enabled mode, and the failure-reconstruct semantics match exactly.

## What the reference does

`ReadSector` (`:68822-68859`) maps a volume chunk to a member column (quote abridged to the four branches — full verbatim block in `evidence/decompiled-quotes/s2-ldm-raid5-rotation.md`):

```c
uVar5 = (local_b8 + S) / S - 1;            /* stripe row r = floor(off / S)   */
uVar8 = local_b8 % S;                      /* offset within stripe            */
iVar3 = N - 1;
uVar6 = uVar5 / iVar3;                     /* syndrome s = r / (N-1)          */
iVar2 = *(int *)(param_1 + 0x2a8);         /* rotation mode                   */
if (iVar2 == 0) {       /* LEFT-SYMMETRIC */
  local_d8 = ((iVar3 - uVar6 % N) + 1 + uVar5 % iVar3) % N;
} else if (iVar2 == 1) { /* LEFT-ASYMMETRIC: fill from 0, skip parity (N-1-s) */
  local_d8 = uVar5 % iVar3;  if (iVar3 - uVar6 % N <= local_d8) local_d8 = local_d8 + 1;
} else if (iVar2 == 2) { /* RIGHT-SYMMETRIC */
  local_d8 = (uVar6 % N + 1 + uVar5 % iVar3) % N;
} else if (iVar2 == 3) { /* RIGHT-ASYMMETRIC: fill from 0, skip parity (s mod N) */
  local_d8 = uVar5 % iVar3;  if (uVar6 % N <= local_d8) local_d8 = local_d8 + 1;
}
local_88 = uVar6 * S + uVar8;              /* member-relative offset          */
```

On a failed member read (`:68871-68876`), `ComputeDataWithOtherDisk` (`:68393-68451`) re-reads every OTHER column and XOR-folds (first member memcpy, rest XOR), and returns failure if any other column is also unreadable (`:68443-68445` `if (local_30 < ... ) param_5 = 0;`).

The rotation field: `:68942` `*(undefined8 *)(param_1 + 0x2a8) = 0;` (constructor) is the only assignment to `+0x2a8` in the whole module — no VBLK field, no setter, no config path writes it (grep-verified over all 91,475 lines).

## What our core does

`cr_ldm.cpp:1349-1359`:

```cpp
} else {              /* raid5 left-symmetric (libldm "raid5_ls"): parity
                       * P(row) = N-1-(row mod N); data chunk i of the row
                       * lands on column (P+1+i) mod N */
    const u64 k = ci; /* global data-chunk index */
    row = k / (cols - 1);
    const u64 i = k % (cols - 1);
    const u64 parity = (cols - 1) - (row % cols);
    col = (parity + 1 + i) % cols;
}
```

and on unreadable member, XOR of every other column with second-missing-column surfaced as failure (`:1372-1389`) — never zeroed.

Equivalence with the og's R=0 (with the og's `uVar5`=chunk index k, `uVar6`=s, `uVar5 % iVar3`=f):

- og R=0: `col = ((N-1 - s%N) + 1 + f) % N`
- core: `P = (N-1) - (row % N)`, `col = (P + 1 + i) % N` with `row`=s, `i`=f → `col = ((N-1 - s%N) + 1 + f) % N`

Identical. The reconstruct-on-failure semantics are identical (both fail surfaced when a second member is missing).

## Gap

The og contains dead-code paths for rotations 1/2/3 that the core does not implement. No LDM volume producible by this og binary — nor by Windows dmio, which libldm documents as `raid5_ls` (left-symmetric) — can exercise them (the selector is hard-zeroed at construction and never written).

## Impact

None for parity: the og's observable behavior on every volume it can build is left-symmetric, and the core matches it exactly. The dormant paths are recorded so a future "why doesn't the core support right-symmetric?" question has an answer: neither does the og, observably.

## Reproduction / verification

Decompiled evidence (quotes above, full function in the evidence file); address mapping via the SEH crash-handler table (function-name strings at `:88602-:88709` → RVA ranges cross-checked against `LdmManager_functions.txt`); grep over the full decompile for writes to `+ 0x2a8) =` (single hit, the constructor).

## Fix plan (for the engineering team)

None required. Optional: a one-line comment at `cr_ldm.cpp:1352` noting the og carries dormant R=1/2/3 rotations, selector ctor-zeroed, so future readers do not "restore" support for layouts no real volume uses.

## Tests to add

None beyond the existing RAID-5 tests (they already pin the enabled-mode mapping). Optional: a comment-only test documenting the og's dormant variants.

## Related findings

- OQ-T7 (RESOLVED by this finding — see `tracked-open/OQ-T6-T7-...md`).
- A-002 (the same deep pass; GPT-LDM gating uses the real metadata GUID and the last-LBA PRIVHEAD anchor — matching the core).

## Open questions

None.
