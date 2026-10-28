# MINOR N-165 — FAT "File overflows partition!" run-end refusal absent: the og destroys out-of-volume runlists at scan time; the core emits the extents and defers failure to read time

**Severity:** MINOR
**Module:** fat (chain/extent emission)
**Parameters affected:** fragment handling (27), error semantics (23), false-positive surface shaping
**Corpus references:** `FATSearchDLL_decompiled.c:10873-10876` + ReleaseTrace `:10903` ("File overflows partition!"), `:28045-28048` + `:28070` (the second runlist factory; both checks 64-bit) — the file gets NO data runs at scan time when `(run_len + (cluster−2)) × spc > volume sector limit`
**Core references:** `src/core/fat/cr_fat_quick.cpp:370-386` (unguarded extent emission), the chain bound `max_cluster_ = cluster_count+1` (`cr_chain.cpp:92`) which `validate_boot` deliberately allows to exceed the device (truncated-tail volumes, `cr_chain.cpp:613-616`); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's FAT runlist factories refuse and DESTROY the runlist when the chain runs past
the volume's sector limit — the file is listed without data runs (no recoverable
content offered). The core emits the extents unguarded; the out-of-slice reads fail at
recovery time (Salvaged/unreadable grading, hash-verify flags). On a volume truncated
mid-FAT (BPB claims more clusters than the device holds), the og drops overflow files at
scan; the core lists them and fails per-read.

## What the reference does

Both factories gate each run's end against the volume sector limit (64-bit math, cites
above) and destroy the runlist on overflow, logging the ReleaseTrace line.

## What our core does

Extents emitted (cr_fat_quick.cpp:370-386); the deferred-failure doctrine (truncated
volumes are first-class recovery targets — validate_boot's deliberate allowance) lets
the readable prefix recover.

## Gap

Scan-time refusal (og) vs deferred per-read failure (core). Both postures are defensible;
the divergence is unregistered.

## Impact

Result-set shape on truncated-volume images: the og's tree omits overflow files
entirely; the core's tree carries them (recoverable prefix where the device still has
bytes). Parity benches see core-superset counts; users see additional (honestly-graded)
recoverable content. Low harm — but unregistered.

## Reproduction / verification

Corpus-derived (lane A first-hand: both factory gates + the trace string). Core-derived
(the emission path + the deliberate truncated-tail allowance read directly). Registry:
no row (the truncated-tail stance is documented in code comments only).

## Fix plan

1. Decide + register (do NOT silently change behavior): the core's deferred-failure
   stance is the better recovery posture (prefix salvage) — register it as a
   deliberate superset over the og's scan-time refusal, citing both sides. If the team
   prefers og-parity for bench comparability, add a config gate `fat_refuse_overflow`
   (default = core stance).
2. Files: PARITY-DELTAS row (doc lane); optional gate in `cr_fat_quick.cpp`.
3. Tests: the truncated-volume fixture pins whichever stance is chosen.
4. Acceptance: the registry row exists; the fixture test names the stance.

## Tests to add

- `Fat.TruncatedVolumeOverflowChainStance` (pin: prefix-recoverable + PartialRisk under
  the core stance)

## Related findings

- N-162 (the same factories' width class)
- M-130 (the dentry validator family — the other scan-time gate class)

## Open questions

None.
