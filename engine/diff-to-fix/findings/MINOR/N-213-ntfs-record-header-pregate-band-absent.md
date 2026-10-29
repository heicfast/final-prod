# MINOR N-213 — NTFS record-header pre-gate band: the og's `usa_offset ∈ [0x2A, 0x100]`, `alloc_size ∈ {1024, 2048, 3072, 4096}` (0x400-aligned) and `usa_count ≤ 16` gates are absent; the unregistered half of the record-validation family

**Severity:** MINOR
**Module:** ntfs (record validation / remnant-scan false positives)
**Parameters affected:** B3 (false-positive rate on garbage records), C20-adjacent (struct-offset sanity gates), A26-adjacent (walk admission)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_180046160 :55187-55220 (the pre-gate, re-read first-hand: magic "FILE"; `*(ushort*)(param_1+6) < 0x11` (usa_count ≤ 16); `param_1[7] < 0x1001 && param_1[7] % 0x400 == 0` (alloc_size ≤ 4096 and 1024-aligned); `(param_1[7] / 0x200) >= usa_count - 1` (the USA-fits check); `*(ushort*)(param_1+1) ∈ [0x2A, 0x101)` (the usa_offset band)); the consumer FUN_180041bb0 :52518-52545 (the validator chain that calls it before the fixup)
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` — `src/core/ntfs/cr_fixup.cpp:28-54` (unchanged; RENUMBERED from the born-collided N-210 seat at the push-rebase per SA-9 — the remote S71 repair lane's marker-recurrence finding keeps N-210) (`fixup_validate`: `record_bytes ≥ 0x400`, `usa_offset != 0 && even`, `usa_count ≥ 2`, the two fit checks — no offset band, no declared-alloc-size gate, no usa_count upper bound); the >4 KiB / odd-stride acceptance is the REGISTERED deliberate superset (D-143 via N-196)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og applies a record-header pre-gate before the fixup: the USA offset must lie in [0x2A, 0x100], the declared allocation size must be one of {1024, 2048, 3072, 4096} bytes, and the USA count must not exceed 16. The core's `fixup_validate` checks only nonzero/even offset, minimum count, and the fit arithmetic. The consequence is a false-positive-rate difference on garbage: torn or random bytes that happen to carry "FILE" + a plausible USA shape pass the core's gates and fail the og's band. The >4 KiB half of the og's alloc_size gate is already registered as the deliberate D-143 superset; the offset band and the alignment set are the unregistered remainder.

## What the reference does

FUN_180046160, re-read first-hand (:55187-55220), verdict carried in the return's low byte:

```c
if (*param_1 == 0x454c4946) {                          /* "FILE" */
    if (*(ushort *)((longlong)param_1 + 6) < 0x11) {   /* usa_count <= 0x10 */
        if (((uint)param_1[7] < 0x1001) &&
           ((int)((ulonglong)(uint)param_1[7] % 0x400) == 0)) {   /* alloc_size <= 4096, 1024-aligned */
            if ((uint)(param_1[7] / 0x200) < *(ushort *)((longlong)param_1 + 6) - 1) {
                /* USA array cannot fit the sectors -> fail */
            } else if (*(ushort *)(param_1 + 1) < 0x2a) {          /* usa_offset < 0x2A -> fail */
            } else if (*(ushort *)(param_1 + 1) < 0x101) {         /* usa_offset <= 0x100 -> PASS */
            }                                                        /* else -> fail */
        }
    }
}
```

## What our core does

`src/core/ntfs/cr_fixup.cpp:34-42`: `usa_offset == 0 || usa_offset & 1` → reject; `usa_count < 2` → reject; `sectors × sector_size > record_bytes` and `usa_offset + count*2 > record_bytes` → reject. No `[0x2A, 0x100]` band, no declared-size alignment gate (the core's `record_bytes` is walk-supplied rather than record-declared — for main-walk records the volume's record size makes the alloc_size gate moot; for REMNANT-scan records the record size is stride-derived, so the og's per-record declared-size check is a garbage filter the remnant path lacks), and no usa_count upper bound (the fit check implicitly bounds it more tightly for standard sizes, so that gate is redundant in the core).

## Gap

Falsifiable: a remnant-scan buffer containing `"FILE"` + a valid fixup pair + `usa_offset = 0x08` (or `0x1A0`) passes `fixup_validate` and fails FUN_180046160's band. The false-positive admission rate on random/torn buffers differs accordingly.

## Impact

Modest: the remnant scanner's other gates (the "FILE" magic, the first-$FILE_NAME requirement, the fixup pair check) already reject most garbage; the band difference admits the residue whose USA offset lands in [2, 0x28] ∪ (0x100, record_bytes]. This is a false-positive-rate delta on the remnant population, not a correctness flip on valid volumes — the og's band is also stricter than the NTFS spec requires (real volumes use 0x30 for 1 KiB / 0x48 for 4 KiB records, comfortably inside the band).

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session (the pre-gate quoted above; the consumer chain at FUN_180041bb0). Core side quoted at `3799dee2`. Coverage check: the >4 KiB/odd-stride acceptance is D-143/N-196's registered stance (explicitly out of scope here); "usaCount"/"allocSize"/the offset band — zero hits across findings/ and modules/. Not executed (the discriminator is a unit test on `fixup_validate` with the band-shaped input, part of the fix plan).

## Fix plan (for the engineering team)

1. Add the band to `fixup_validate`: `usa_offset < 0x2A || usa_offset > 0x100 → ERR_BAD_PARAMETER`, with a comment citing FUN_180046160 :55208-55216.
2. For the remnant path only (where record size is stride-derived, not volume-declared), add the declared-alloc-size gate: the record's +0x1C alloc_size must equal the stride and be 0x400-aligned (on the main walk this is redundant — skip it there to avoid double-gating).
3. Register the adopted gates in PARITY-DELTAS if any population shift is observed on the remnant fixtures (the expected direction: fewer garbage admissions).
4. Files: `src/core/ntfs/cr_fixup.cpp`, `src/core/ntfs/cr_mftwalk.cpp` (the remnant-site alloc_size check).
5. Tests: `Ntfs.FixupOffsetBandRejectsLowAndHigh` (0x08 and 0x1A0 rejected; 0x30/0x48 accepted); `Ntfs.RemnantAllocSizeMustMatchStride`; a re-pin of the remnant-scan fixtures (admission counts drop only on the garbage population).
6. Acceptance criteria: the band unit tests green; the remnant fixtures' true-record counts unchanged; any garbage-buffer admission-count change documented in the PARITY-DELTAS row.

## Tests to add

- `Ntfs.FixupOffsetBandRejectsLowAndHigh` — RED today.
- `Ntfs.RemnantAllocSizeMustMatchStride` — RED today on the forged-alloc-size input.

## Related findings

- D-143/N-196 (the registered >4 KiB / odd-stride superset — the other half of this gate family).
- M-151 (this session — the run-growth probe whose record counting consumes FUN_180041bb0's chain).

## Open questions

None — the band and the alignment set are directly decoded; the only choice is whether the alloc-size gate applies remnant-only (recommended) or globally (double-gating the main walk for no benefit).
