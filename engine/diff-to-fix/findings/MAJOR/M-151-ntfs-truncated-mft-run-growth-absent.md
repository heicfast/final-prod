# MAJOR M-151 — NTFS: the truncated-$MFT run-growth probe (the og extends each run past its declared length by counting fixup-valid records) is absent; the core's walk is hard-bounded by the declared MFT size and never sees the records beyond

**Severity:** MAJOR
**Module:** ntfs (MFT walk / torn-volume recovery)
**Parameters affected:** B2 (recovery completeness — records invisible), B6-adjacent (torn-structure salvage), A26 (walk bounds), X52-adjacent (scalability of the probe)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_180007670 :5617-5700 (the run-growth loop, re-read first-hand: the per-run 4 KiB window reads via the device vtable's slot 8, the fixup-valid record counting via `FUN_180041bb0` at the 0x400 stride, the run-length extension `*(longlong *)(lVar1 + 0x10) += (uVar10 / *(short *)(param_1 + 0x1a)) / param_3;`, the `uVar10 < uVar12` termination); FUN_180041bb0 :52518-52545 (the record validator that gates the count)
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` — `src/core/ntfs/cr_mftwalk.cpp:422` (unchanged by the fix windows) (`const u64 max_record = vol->max_record_no();` — the declared-size bound), `:481-490` (the walk loop; `read_record` → `ERR_RANGE_OUT_OF_BOUNDS` → `continue` — records beyond the declared MFT are skipped, never probed); no extension/probe code exists in `cr_bootstrap.cpp` or `cr_mftwalk.cpp` (grep "extend|grow|probe.*record": the hits are unrelated)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

After loading the $MFT's $DATA runlist, the og walks each run's declared END and keeps reading 4 KiB windows outward, counting records that validate as MFT records (FILE magic, fixup gates); each window of valid records EXTENDS the run's length, growing the walkable MFT past what the (possibly truncated or stale) runlist declares. The core's walk is bounded by `max_record_no()` — the declared size — and skips every record beyond it with `ERR_RANGE_OUT_OF_BOUNDS`. On volumes whose $MFT FILE record was damaged after the volume grew (a torn write to record 0's runlist, a stale mirror restore, a partially overwritten $MFT), the og recovers every record that still validates on disk; the core recovers only what the truncated runlist admits.

## What the reference does

FUN_180007670, re-read first-hand (the growth loop over the run map at `param_1 + 5`):

```c
/* :5646-5678 (per run node; lVar1 = the run node, +0x10 = the run length field) */
uVar11 = (ulonglong)*(short *)((longlong)param_1 + 0x1a) * *(longlong *)(lVar1 + 0x10) + *param_1;
                                     /* the run's declared end byte (sector/cluster-scaled) */
uVar5 = (**(code **)(*param_2 + 0x28))(param_2);    /* the device sector size */
uVar12 = uVar5 << 0xc;                              /* the probe window: sector_size * 4096 */
iVar4 = (**(code **)(*param_2 + 8))(param_2, lVar2, uVar11, 0x1000, 0);   /* read 4 KiB at the end */
while (iVar4 != 0) {
    uVar10 = 0;
    if ((uVar5 & 0xfffff) != 0) {
        do {
            uVar7 = FUN_180041bb0((int *)(uVar10 + lVar2), 0x400);   /* fixup-valid record? */
            if ((int)uVar7 == 0) break;                              /* stop at the first invalid */
            uVar10 = (ulonglong)((int)uVar10 + 0x400);
        } while (uVar9 < uVar12);
    }
    uVar9 = (**(code **)(*param_2 + 0x28))(param_2);
    *(longlong *)(lVar1 + 0x10) = *(longlong *)(lVar1 + 0x10) +
         (uVar10 / (uint)(int)*(short *)((longlong)param_1 + 0x1a)) / param_3;
                                     /* EXTEND the run length by the valid-record count */
    if ((uint)uVar10 < uVar12) break;                 /* hit an invalid record -> done growing */
    uVar11 = uVar11 + 0x1000;                        /* next 4 KiB window */
    iVar4 = (**(code **)(*param_2 + 8))(param_2, lVar2, uVar11, 0x1000, 0);
}
```

`FUN_180041bb0` (:52518-52545) is the strict record validator — the "FILE" magic gate, FUN_180046160's header pre-gate (see N-213 — renumbered from the born-collided N-210 seat per SA-9), the fixup check, and the attribute-offset sanity — so the growth only ever annexes bytes that genuinely parse as MFT records. The probe is naturally bounded: the first invalid record in a window stops that run's growth (the `uVar10 < uVar12` break), and a failed device read ends the loop. This runs at volume-open (the caller at :5554, inside the post-$MFT-load path).

## What our core does

`src/core/ntfs/cr_mftwalk.cpp:422-490`:

```cpp
const u64 max_record = vol->max_record_no();      /* the DECLARED size from the $DATA runlist */
for (u64 record_no = 0; record_no < max_record; ++record_no) {
    ...
    u32 rc = vol->read_record(record_no, rec);
    if (rc == ERR_RANGE_OUT_OF_BOUNDS)
        continue;                                  /* beyond the declared MFT: skipped, never probed */
```

The bootstrap (`cr_bootstrap.cpp`) loads the $DATA runlist and computes the record count from the declared run lengths; nothing ever extends a run. A torn $MFT record-0 runlist (fewer runs than the volume actually contains, or a shorter final run) silently caps the walk — the records beyond the declared end are on disk, valid, and invisible to the core.

## Gap

Falsifiable: build a synthetic NTFS image whose $MFT $DATA runlist declares N records while the on-disk MFT extent holds N + K further fixup-valid records (a stale/truncated runlist — construct by growing the image's MFT after the runlist was captured, or by hand-editing the final run's length field down). The og's walk reads and emits records N..N+K-1 (the growth probe annexed them); the core's walk stops at N (`ERR_RANGE_OUT_OF_BOUNDS` for every record beyond).

## Impact

Recovery completeness on the torn-$MFT population. The trail already treats this population as first-class: S-063 (the $MFTMirr bootstrap fallback, FIXED) covers the cannot-load-$MFT-at-all case, and the M-043/M-141 fallback ladder covers load failures — but a runlist that LOADS and is merely SHORT has no salvage path in the core. Every file whose MFT record lies beyond the declared end (the records of recently-created files, which sit at the MFT tail — exactly the records most recently written and most likely to matter to the user) is dropped. The og's own defect tolerance annexed them by construction.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: FUN_180007670 quoted above (the growth loop, the extension arithmetic, the termination conditions) and FUN_180041bb0's validator chain. Core side: the declared-size bound and the skip path quoted at `3799dee2`. Coverage check: `grep FUN_180007670 diff-to-fix/` — zero hits (S-063 is the mirror-bootstrap mechanism; M-043/M-141 are load-failure rungs; neither grows loaded runs). Not executed this session (the fix plan's first test constructs the synthetic stale-runlist image; the growth arithmetic itself is mechanical).

## Fix plan (for the engineering team)

1. After the bootstrap loads the $MFT $DATA runlist, run the growth pass: for each run, read 4 KiB windows at the run's declared end and count fixup-valid records at the record-size stride (`cr_fixup::fixup_validate` + the "FILE" magic + the header sanity the walk's own `read_record` already applies); extend the run's length (and the volume's max_record_no) by the valid count; stop at the first invalid record or a failed read. Mirror the og's per-run termination exactly (a window fully valid advances to the next 4 KiB; a partially-valid window grows by the valid prefix only).
2. Keep the probe cancel-aware (a checkpoint per window) and progress-reported (the og folds it into the open path's progress).
3. Register the deliberate differences if any remain (e.g., the core's per-record validation is the walk's own validator — the og uses FUN_180041bb0's stricter chain; adopting the walk's validator is the registered-equal choice since both gate the same population).
4. Files: `src/core/ntfs/cr_bootstrap.cpp` (the post-load growth pass), `include/cr/ntfs/cr_bootstrap.h` (the volume params), `src/core/ntfs/cr_mftwalk.cpp` (unchanged — it inherits the grown bound).
5. Tests: the synthetic stale-runlist image (records beyond the declared end surface — RED today); the invalid-record termination pin (a run followed by non-record bytes grows by zero); the cancel-responsiveness pin; a re-pin of the existing torn-MFT suite (S-063's fixtures) to prove no interference with the mirror fallback.
6. Acceptance criteria: the stale-runlist fixture recovers the annexed records; intact-volume fixtures are byte-identical before/after (the probe stops at the first non-record byte — for an intact volume that is the run's declared end or the next structure, so growth is zero or annexes only the true tail slack).

## Tests to add

- `Ntfs.TruncatedRunlistGrowsToValidRecords` — RED today.
- `Ntfs.RunGrowthStopsAtFirstInvalidRecord`.
- `Ntfs.RunGrowthCancelledPromptly` (the checkpoint pin).
- `Ntfs.MirrorFallbackUnaffectedByGrowthPass` (the S-063 interplay re-pin).

## Related findings

- S-063 (FIXED — the $MFTMirr bootstrap fallback: the cannot-load case of the same torn-$MFT family).
- M-043/M-141 (FIXED — the MFT-load-failure fallback ladder).
- N-213 (this session — the record-header pre-gate band the og's probe consumer applies; renumbered from N-210 at the push-rebase).

## Open questions

None — the og's loop is fully decoded (window size, stride, extension arithmetic, termination); the only design choice is whether the core's probe reuses `read_record`'s validation (recommended — one validator, one truth) rather than porting FUN_180041bb0's chain separately.

---

## Addendum (S69 lane, 2026-10-29 — a complementary corpus site for the same family)

**The device-end-derived record-count estimate — `FUN_18000ecb0` (NTFSSearchDLL:11274-11277), read first-hand this session:**

```c
lVar9 = (**(code **)(**(longlong **)(param_1 + 0xb8) + 0x18))();     // device size
lVar15 = (lVar9 - *(longlong *)(lVar15 + 0x10)) / (longlong)uVar11;  // record_count = (device_end − start) / record_size
puVar10[0x15] = lVar15;                                              // store record count
puVar10[0xc] = uVar11 * lVar15;                                      // byte size = record_size × record_count
```

Context (NTFSSearchDLL:11240-11285): the function searches a registration
list for the matching id (`*(uint *)(lVar9 + 0xc) == uVar6`); when absent,
it allocates a 0x1a8-byte object, stamps type 3, stores the found entry's
start (+0x10), and derives the record count **from the device end —
(device_size − start) / record_size — not from any declared size**,
alongside +0x9c = 0x100 and two liveness flags (+4 = 1, +0x34 = 1).

**Relation to the finding:** this is the INITIAL-bound sibling of the
finding's growth loop (FUN_180007670): where the growth loop extends a
declared run past its end by validating records, this site constructs the
initial bound for a stream whose declared size is absent — the device end
is the floor. Both sites belong to the same behavioral family (the og's
walk bounds are disk-truth-derived, not metadata-trust-derived); the
finding's fix plan should consider both surfaces (the estimate site feeds
the initial `max_record_no`-equivalent, the growth loop extends it).
Verified at core `271f5d06`: the core's `max_record_no()`
(include/cr/ntfs/cr_bootstrap.h:77-91) remains runlist-derived only —
neither the device-end floor nor the growth loop exists.

Evidence: `evidence/s69-lane-sheets/S69-C-sectormath.md` §4.b.
