# MINOR N-155 — the RRWR pair's direction naming inverted in DRIVER-CONTRACT §4.4 (0x800C2B3E named WRITE / 0x800C2B42 named READ; the og wire decodes the opposite) + I-029's "four unverifiable codes" premise refuted by first-hand decode

**Severity:** MINOR within core scope (doc-truth on a contract table no core code issues — the RdaClient was deleted by N-041; the doc feeds the driver lane) — **but the finding carries a SEVERE-class verification step for the driver repo**: if `rda_filter.c`'s implementation followed this table, the 0x800C2B3E/0x800C2B42 wire directions are inverted vs the og (an og-shaped client issuing 0x800C2B3E to read would trigger a WRITE).
**Module / Surface:** transport / driver contract (docs/DRIVER-CONTRACT.md §4.4)
**Parameters affected:** 46 (contract documentation), B22-adjacent (the wire-code registry)
**Corpus references:** `re-workspace/reversed/ghidra-drivers/EUDCPEPM_decompiled.c` — the `FilterDispatch` custom-delta ladder `:1290-1360` (`-0x7ff3d540`=0x800C2AC0 DISCOVERY writes diskId/Information=4; `-0x7ff3d4c2`/`-0x7ff3d4be` = 0x800C2B3E/0x800C2B42 → `FUN_00013310` with the isWrite flag `(code == -0x7ff3d4be)` at `:1320`; `-0x7ff3d4c0` = 0x800C2B40 the version probe `piVar4[2]=0x604, Information=0xc` at `:1336-1342`); the RRWR data handler `FUN_00013310 :1616-1665` (magic `0x52455752`@+0, `sectorSize@+8 == devSectorSize(+0x98)`, `declaredOutLen@+0xC == actual`, `lba@+0x10`, `sectorCount@+0x18`, `sectorSize×count ≤ outLen`; `:1655-1658` maps `-0x7ff3d4c2` → `FUN_000141f8`, else `:1659-1660` → `FUN_00014300`); `FUN_000141f8 :2346-2381` = `IoBuildSynchronousFsdRequest` **major 4 = IRP_MJ_READ** (the same function as the og's 0x800C2B0A direct-read handler, artifact `eudcpepm-euedkepm--hook-drivers.md:63`); `FUN_00014300 :2386-2420` = **major 3 = IRP_MJ_WRITE** (the 0x800C2B12 direct-write twin, artifact `:64`; its "ReadSectors" trace string is an og-side copy-paste artifact — the MajorFunction is the truth)
**Core references:** `docs/DRIVER-CONTRACT.md` §4.4's table at session-42 open — `0x800C2B3E | RRWR raw disk WRITE` and `0x800C2B42 | RRWR raw disk READ | Twin of 0x800C2B3E`; the driver repo's `src/driver/rda_filter.c:247-309` (out of this session's two-repo scope — the verification step below is the driver lane's)
**Status:** discovered during I-029's verification (session-42, the §8 discovered-divergence class)

## Summary

I-029 claimed 0x800C2AC0 + the RRWR trio "appear in no corpus artifact, decompile, or og binary string — they are core-driver wire, not reference wire," on the strength of a grep for the literal "RRWR" string and 0x800C…-form constants. **That grep was defective**: the EUDCPEPM decompile encodes the custom codes as *negative deltas* off IoctlBase (`-0x7ff3d540`, `-0x7ff3d4c2`, `-0x7ff3d4be`, `-0x7ff3d4c0`) and the magic as the hex constant `0x52455752`. Read directly, the og's `FilterDispatch` implements all four: DISCOVERY (diskId write, Information=4, out≥4), the RRWR version probe (0x604@+8, Information=0xC, magic-gated), and the RRWR data pair with the exact wire the core's table documents (magic@0 / sectorSize@+8 / declaredOutLen@+0xC / lba@+0x10 / sectorCount@+0x18). **I-029 is INVALID** — all 19 codes carry og-corpus provenance; the artifact's own 15-code census missed the four filter-side codes (corpus-erratum class).

But the first-hand decode surfaced a real defect the audit missed: **the direction naming of the RRWR pair is inverted in DRIVER-CONTRACT §4.4**. The og maps 0x800C2B3E → `FUN_000141f8` (IRP_MJ_READ) and 0x800C2B42 → `FUN_00014300` (IRP_MJ_WRITE); the core's table names 0x800C2B3E "RRWR raw disk WRITE" and 0x800C2B42 "RRWR raw disk READ". Both rows describe symmetric twins, so the table's *wire shape* is right while its *direction labels* are swapped.

## Reproduction

Decompiled (strength 2, read first-hand this session): the `FilterDispatch` ladder at `EUDCPEPM_decompiled.c:1290-1360` (the four deltas), `FUN_00013310:1616-1665` (the wire gates + the direction split at `:1655-1660`), `FUN_000141f8:2346-2381` (major 4 = READ), `FUN_00014300:2386-2420` (major 3 = WRITE). The artifact's function-role cross-check at `eudcpepm-euedkepm--hook-drivers.md:63-64` (0x800C2B0A→FUN_000141f8 read / 0x800C2B12→FUN_00014300 write).

## Fix plan (for the engineering team)

1. Core scope (DONE this session): DRIVER-CONTRACT §4.4's two rows re-named to the og decode (0x800C2B3E = RRWR raw disk READ with the IRP_MJ_READ cite; 0x800C2B42 = RRWR raw disk WRITE with the IRP_MJ_WRITE cite) + the 19-code provenance note under the §4.4 heading.
2. **Driver lane (out of this session's two-repo scope — the precise next step):** verify `src/driver/rda_filter.c`'s handling of 0x800C2B3E/0x800C2B42 against the og decode. If the implementation follows the (previously documented) inverted names, the wire directions are inverted vs the og — fix `rda_filter.c` and add the round-trip test (0x800C2B3E must read back known disk content; 0x800C2B42 must write it). If the implementation is already og-correct, only the doc carried the inversion (this session's fix) and the driver repo needs no change — record which in the driver lane's trail.
3. CORPUS-ERRATA: the artifact's "custom range observed 0x800C2AC8…0x800C2B38" census missed the four filter-side codes; the full range is 0x800C2AC0…0x800C2B42 (19 codes).

## Tests to add

None in core (no core code issues the codes — the N-041 mirror posture). The driver-lane round-trip test is item 2's.

## Related findings

I-029 (INVALID — its premise refuted by this decode), N-041 (the deleted client), N-096/D-DRV-5 (the identity-IOCTL register).

## Open questions

None on the og side (the decode is instruction-decisive); the driver-repo implementation state is the open half — item 2.

## Resolution

**Status:** FIXED (the core-scope half — the doc rows + the provenance note; the driver-lane verification step recorded as the precise next step)
**Fixed in commit:** this session's N-155 landing (session-42; cite-by-title: "fix(docs): N-155 + I-029 - the RRWR direction decode lands, the four-code provenance verified first-hand, I-029's premise refuted")
**Verified by:** decompiled evidence read first-hand (the citation chain in the finding body: FilterDispatch :1290-1360, FUN_00013310 :1616-1665, FUN_000141f8 :2346-2381 major-4 READ, FUN_00014300 :2386-2420 major-3 WRITE); the DRIVER-CONTRACT §4.4 rows now carry the og-direction names with per-row cites; the corpus-erratum entry added for the artifact's 15-code census gap.
**Resolution notes:** The og wire decode is instruction-decisive: 0x800C2B3E is the READ direction (IoBuildSynchronousFsdRequest major 4) and 0x800C2B42 is the WRITE direction (major 3). DRIVER-CONTRACT §4.4's two rows re-named accordingly with the corpus cites inline; the 19-code provenance note added under the heading (superseding I-029's four-unverifiable claim — the decompile's negative-delta encoding is what defeated I-029's grep). The driver-repo half (rda_filter.c's actual direction mapping) is OUTSIDE this session's two-repo scope — recorded as the finding's precise next step with the round-trip test recipe; the driver lane must close it. No core code change (no core code issues these codes — the N-041 mirror posture preserved).
