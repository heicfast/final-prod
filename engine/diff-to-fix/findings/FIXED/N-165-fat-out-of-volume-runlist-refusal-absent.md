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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-165 landing, session-46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `FatEngine.TruncatedVolumeOverflowChainStance` (the stance pin: the overflow-tail file stays LISTED with its full chain extents — the readable prefix plus the beyond-device tail — and the chain's own scan-time grade; the og's scan-time destruction registered, not adopted). Suite 1281/1281 linux-release (1239 pass / 42 env-skips / 0 fail).
**Resolution notes:** Corpus re-verified first-hand: both factory gates re-read (FATSearchDLL :10865-10903 — the run-end check against the volume sector limit + `ReleaseTrace("File overflows partition!")` + the runlist release; the second factory :28040-28075 — the same check at the :28070 trace). The finding's own fix plan step 1 selected the core's deferred-failure posture (do NOT silently change behavior): the stance is now REGISTERED as PARITY-DELTAS D-130 (renumbered from its born-collided D-136 seat at the N-191/N-207 repair: the S-060 row keeps D-136) — the truncated-tail volume (BPB claims more than the device holds; validate_boot's deliberate allowance) is a first-class recovery target; the readable prefix recovers and the beyond-device tail surfaces as the recovery-time Salvaged disclosure (FB-R02), never a scan-time destruction. Two truths the plan's test sketch assumed differently, corrected in the pin: (a) the scan-time grade is the chain's OWN grade (a FAT-verified intact chain grades Excellent — the truncation is not scan-time knowledge; the finding's "PartialRisk" wording described the recovery-time disclosure, which rides the Salvaged outcome); (b) the deleted-chain walk is bounded by the dentry's declared size (reconstruct_deleted_chain's expect bound), so the fixture sizes the chain honestly. No code change — the registration + the pin are the deliverable, exactly as the finding prescribed.

---

## S46 convergence addendum (the rebase reconciliation, SA-9 earlier-push-wins)

The S46 MINOR-first lane independently re-read both og factory gates
(:10865-10878 / :28060-28070, the ReleaseTrace pair), built the same D-130 (renumbered)
registration, and additionally landed a scan-time honest-grade cap (overflow
extents never Excellent) with its own RED→GREEN pin. At the reconciliation the
winning stance above stands: the grade is the chain's own FAT-verified truth,
the beyond-device tail is a recovery-time Salvaged disclosure. The S46 cap was
DECLINED (the duplicate pin removed, the cap reverted). RETAINED from the S46
landing: the `FatScanStats.overflow_chains` counter (additive observability
counting the exact class D-130 describes — cited by neither pin, no behavioral
surface), and this convergence record. The S46 lane also repaired the window's
committed reds around these tests (the duplicate validated tick, the swapped
grading expectations, the user-sign registry leak, the Lzh comment splice) —
absorbed into the remote's own repair commits at the merge.
