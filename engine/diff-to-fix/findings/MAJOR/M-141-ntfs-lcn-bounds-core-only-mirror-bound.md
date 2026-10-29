# MAJOR M-141 — Core-only mft_lcn/mftmirr_lcn < total_clusters refusals: the og gates only the primary (raw spc × mft_lcn < total_sectors product form, wrapping, no mirror gate anywhere); the core's cluster-form/floor-boundary/mirror bounds refuse single-field tears the og scans

**Severity:** MAJOR
**Module:** ntfs (boot parameter bounds)
**Parameters affected:** B1 (detection correctness), B3 (false-positive rate), A25 (engine algorithm steps)
**Corpus references:** `re-workspace/reversed/ghidra-mo/NTFSLib_decompiled.c:19296-19301` — `if ((local_104b & 0xff) * local_1028 < local_1030) {` (raw spc byte × $MFT LCN < total_sectors; the only mft bound in LoadNtfsVolume), re-read first-hand; the analyser (FUN_180004900 :3568-3577) and the search-DLL getters carry no mft/mirr bound at all
**Core references:** audited `34d6cf10` — `src/core/ntfs/cr_bootstrap.cpp:66-70`:
```cpp
if (bs.mft_lcn >= total_clusters) return ERR_BAD_PARAMETER;
if (bs.mftmirr_lcn >= total_clusters) return ERR_BAD_PARAMETER;
```
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's only NTFS LCN bound is a raw-product form in LoadNtfsVolume: `spc_byte × mft_lcn < total_sectors` (i64 product — a corrupt huge mft_lcn wraps and passes; no mftmirr bound exists in any og layer). The core refuses when either LCN ≥ the decoded cluster count — a stricter form (cluster-space comparison), a floor-boundary one-cluster-stricter at non-integral total/spc, overflow-free, and with a mirror bound the og never has. A torn mftmirr_lcn (or the exact floor-boundary case) refuses the whole volume in the core and scans best-effort in the og.

## What the reference does

NTFSLib :19296-19301 (quoted in full in the M-138/M-139 evidence): the gate chain is OEM → bps mask → `(local_104b & 0xff) * local_1028 < local_1030` — `local_104b` is the raw spc byte, `local_1028` the $MFT LCN, `local_1030` the 8-byte total_sectors@+0x28. Product in i64: a corrupt `mft_lcn` near 2^63/spc wraps negative and passes the unsigned-style compare in practice (the decompile compare is signed longlong). The $MFTMirr LCN is parsed and stored with no bound in LoadNtfsVolume, the analyser, or the search DLL (grep-verified across the gate regions this session).

## What our core does

`cr_bootstrap.cpp:66-70` (quoted above): both LCNs compared against `total_clusters` (decoded `total_sectors / spc`, floor). Three deltas vs. the og: (1) the cluster form is one-cluster stricter than the product form whenever spc does not divide total_sectors (the og's `<` in sector space accepts an mft_lcn whose cluster form equals the last partial cluster); (2) overflow-free (a wrapped huge LCN is refused — stricter, and better); (3) the mftmirr bound is core-only — the og tolerates a garbage mirror LCN (the mirror read simply fails; the walk continues from the primary).

## Gap

Falsifiable: an NTFS volume with a torn `mftmirr_lcn` (e.g. 0xFFFFFFFF) and everything else valid: the og opens, walks from the primary MFT, and produces the full tree (the mirror is a repair source, absence non-fatal — the core's own `load_mftmirr_stream` treats it best-effort downstream, which proves the internal inconsistency); the core refuses at `validate_boot_sector` with ERR_BAD_PARAMETER — zero structured files. Second case: `mft_lcn` exactly at the floor boundary (total_sectors = k·spc + r, r>0, mft_lcn = k): og accepts (k·spc < total), core refuses (k ≥ floor).

## Impact

Single-field tears refuse the whole volume — the same population class as M-138/M-139 (the og's per-field tolerance vs. the core's aggregate purity). The mftmirr bound additionally contradicts the core's own best-effort mirror stance one function later.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand: the NTFSLib gate chain region read in full; the absence of mirror bounds confirmed across the LoadNtfsVolume stores, the analyser gates, and the search-DLL getters. Core side: both bounds quoted at `34d6cf10`; the best-effort `load_mftmirr_stream` at `:424` confirms the inconsistency.

## Fix plan (for the engineering team)

1. Drop the `mftmirr_lcn >= total_clusters` refusal; demote it to a mirror-invalidating flag (skip `load_mftmirr_stream`, keep the open) — matching both the og and the core's own downstream best-effort semantics.
2. Align the `mft_lcn` bound to the og's product form or register the stricter form deliberately: recommendation — keep a bound (the og's wrap-passing is an og defect class, D-52's hardening family) but move it to the product form `spc × mft_lcn < total_sectors` (sector space, floor-tolerant) so the boundary case matches, and register the non-wrapping evaluation as hardening.
3. Files: `src/core/ntfs/cr_bootstrap.cpp`; `docs/PARITY-DELTAS.md` (the register row).
4. Acceptance criteria: torn-mirror fixture scans the full tree with the mirror skipped + a stat surfaced; the floor-boundary fixture opens; a wrapped-huge mft_lcn still refused (the hardening kept).

## Tests to add

- `NtfsVolume.TornMftmirrLcnScansWithMirrorSkipped` — RED today.
- `NtfsVolume.MftLcnFloorBoundaryOpens` (the non-integral-total case).
- `NtfsVolume.WrappedHugeMftLcnStillRefused` (the registered hardening pinned).

## Related findings

- M-138/M-145/M-146 (the open-ladder acceptance-band family — fix as one coherent pass over `validate_boot_sector` + probe_family).
- S-063 (the mirror's positive role: the bootstrap fallback the core should adopt — this finding's mirror-demotion and that fallback land naturally together).

## Open questions

None — the corpus decode is complete (the gate chain, the stores, and the absence of mirror bounds are all directly read).
