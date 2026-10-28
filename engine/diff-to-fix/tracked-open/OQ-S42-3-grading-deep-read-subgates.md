# OQ-S42-3 — the grading deep-read sub-gates: FUN_1401bc5a0's predicate, the +0x72-bit-7 / +0x74-bit-2 flags, vt+0x28's sector-size source, vt+0xb8 / vt+0x100's meaning, and the run-unit value local_b8[0]

**Origin:** S42 continuation lane, grading focus battery (M-121's decode residue)
**Status:** OPEN (this session)

## The precise questions

M-121 decoded `FUN_14038c470` (the og's sector-content grade validation) structurally; five sub-gates inside and around it remain undecoded, and M-121's fix plan needs them (or a deliberate ship-without):

1. **`FUN_1401bc5a0(mgr, node, addr)`** — the first element of the param_3==0 three-part pre-check (`DRWUI:605355-361` region per the call-site decode): what does it test (an addr-map membership? a device liveness probe)?
2. **Bit 7 of node+0x72** — the flag the pre-check ANDs (it is ALSO the QuickScanDeleteExtend increment condition at `:331164-170` — same bit, two consumers; what sets it? deletion-state? scan-phase?).
3. **Bit 2 of node+0x74** — the deep-read-skip bit (its setter `FUN_140389570` sets bit 1; the getter reads bit 2 — who sets bit 2?).
4. **`vt+0x28` on the reader chain** — the sector-size source `FUN_14038c470` uses at `:606353` (device geometry? a fixed 512?).
5. **`vt+0xb8` / `vt+0x100` on the NTFS device object** — the grade-0 sub-rule at `:605901-907` (`vt+0xb8()!=0 || vt+0x100()==0xa8 → grade 0`); `vt+0x100` is also compared to `0x23` in `FUN_1401c76e0` (`:330987`) — a device/type code space (0xa8 is also the partition-struct size stride elsewhere — coincidence suspected but unproven).
6. **The run-unit `local_b8[0]`** (`:605939`) — the multiplier in the device-walk's address key (the core's `kDeviceUnit = 2^40` is a registered stand-in; the og's true value changes only key scaling, not collision sets).

## Next probe (exact)

All sites are in `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`: read `FUN_1401bc5a0`'s body; find the writers of node+0x72 bit 7 and node+0x74 bit 2 (grep the setters); read the reader chain's `vt+0x28` implementor (the CStorageDeviceWithCache family); read the two NTFS vtable slots' implementors (the NTFSSearchDLL controller class); read `FUN_1401bb580`'s out-param fill for `local_b8`. Each is a single function-body read.

## Consumers

M-121 (its fix plan's step 4), M-120 (the unit value), N-158 (the bit-7 QSDE semantics), M-119 (the vt+0x100 device-code space if it overlaps the boundary semantics there).
