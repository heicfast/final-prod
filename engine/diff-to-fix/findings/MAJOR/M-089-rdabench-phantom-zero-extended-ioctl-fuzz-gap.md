# MAJOR M-089 — rdabench's fuzz-hammer probes the phantom-zero extended IOCTLs: the live 0x600244x handlers get zero fuzz coverage

**Severity:** MAJOR
**Module / Surface:** Driver wire contract / fuzz coverage (the extended IOCTL family)
**Parameters affected:** C22 (IOCTL codes and ABI numbers — the probe list's correctness), Q66 (fuzz coverage), Q65-adjacent
**Corpus references:** `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:81-84` (the identity-IOCTL family + the passthrough note); the extended family is RDA-native by design (`src/shared/inc/rda_ioctl.h:72-74` — "do not collide with reference function codes 0x900..0x902"); the corpus's own doc-drift record for the phantom-zero class (core trail CORPUS-ERRATA / driver lane D2: "the IOCTL comments' phantom extra zero — CTL_CODE_RDA(0x910) is 0x6002440, not 0x60002440", rda-driver `bd79633`)
**Core references:** audited driver `323e75a`: `src/bench/rdabench.c:390` (the fuzz-hammer probe list using `0x60002440 / 0x60002444 / 0x60002448`); the REAL handlers `src/shared/inc/rda_ioctl.h:75-79` (`CTL_CODE_RDA(0x910/0x911/0x912)` = `0x6002440 / 0x6002444 / 0x6002448`) dispatched at `src/driver/rda_ioctl.c:276-280`
**Status:** PUSHED-FORWARD (driver-repo-resident; this session's two-repo grant excludes heicfast/rda-driver)
**Prior audit check:** "expanded on prior finding" the D2 handoff — the phantom-extra-zero doc/comment sites were recorded (rda-driver `bd79633`, `163e3cc`) but the *executable* probe-list instance was never filed; this filing owns the functional half (the fuzz-coverage gap) and points at the residual comment sites.

## Summary

The driver's fuzz-hammer (rdabench) probes the extended IOCTL family at the **phantom-zero** values `0x60002440/0x60002444/0x60002448` — codes that do not exist — instead of the real `0x6002440/0x6002444/0x6002448` (the get-buffer-context family added with the extended session tier). The live extended handlers therefore receive **zero fuzz coverage** from the hammer: the probe list's typo reproduces, in executable form, the exact phantom-extra-zero comment defect the driver lane already corrected twice in prose (`bd79633`, `163e3cc`) but never fixed in the bench source.

## What the reference does

The corpus's EPMNtDrv identity family (eprdtdrv-sys--driver-internals.md:81-84) pins the raw code values the RDA wire mirrors; the extended family is the RDA superset (documented driver-side, deliberately non-colliding with reference function codes 0x900..0x902). The parity obligation is on the VALUE: whatever probes exercise the dispatch must use the codes the header actually defines.

## What our implementation does

`src/bench/rdabench.c:390` (audited `323e75a`) builds the fuzz-hammer's IOCTL candidate list with the values carrying one extra zero (`0x6000244x`), while `rda_ioctl.h:75-79` defines the real codes as `CTL_CODE_RDA(0x910/0x911/0x912)` = 0x6002440/44/48 and `rda_ioctl.c:276-280` dispatches them. A hammer iteration over the phantom codes exercises only the "unknown IOCTL" rejection path; the three live extended handlers (buffer-context acquisition/release semantics — the session-tier machinery the recognition switch depends on) are never fuzzed.

## Gap

Falsifiable: the probe list at `rdabench.c:390` contains `0x60002440` (et al.) and does not contain `0x6002440` (et al.); the dispatch table at `rda_ioctl.c:276-280` contains the latter. Therefore no fuzz iteration can reach the extended handlers through the hammer.

## Impact

Fuzz coverage (Q66) on live driver surface: the extended IOCTL handlers' robustness against malformed input shapes is untested by the repo's own hammer — the exact class of defect (length/Information handling under hostile input) the hammer exists to catch. The phantom values also silently "pass" (the unknown-IOCTL path is correct behavior), so the gap is invisible in green runs.

## Reproduction / verification

First-hand this session: `rdabench.c:390` read (the probe values quoted); `rda_ioctl.h:75-79` + `rda_ioctl.c:276-280` read (the real codes and dispatch); the arithmetic verified (CTL_CODE(0x910, …) = 0x6002440 — the driver lane's own wire-truth commits `bd79633`/`163e3cc` state the same). Execution of the hammer is Windows-bound; the static mismatch is the finding.

## Fix plan (for the engineering team)

1. `src/bench/rdabench.c:390`: replace the three phantom constants with the real ones — better, derive them from the header (`IOCTL_RDA_*` names) so the list cannot drift again.
2. Sweep the remaining phantom-zero prose sites the D2 handoff lists (`docs/architecture.md:78-80`, the `rda_ioctl.c:20-22` comment) in the same commit.
3. Add a one-line static gate to the driver CI: the hammer's probe set must contain every IOCTL the dispatch switch handles (grep-assertable: each `case IOCTL_RDA_` constant appears in the bench source).
4. Re-run the hammer on a Windows runner; confirm the extended handlers now receive iterations (log the per-IOCTL hit counts — the existing hammer output).
5. Acceptance: the static gate green; a hammer transcript showing nonzero probe counts for 0x6002440/44/48.

## Tests to add

- `BenchProbeSet-Covers-Dispatch-Table` (the static gate)
- Hammer transcript evidence: extended-IOCTL hit counts > 0

## Related findings

- **N-107** (the driver doc-drift cluster — the phantom-zero prose sites ride that fix)
- **M-086/M-087** (the other driver-lane defects filed this session)

## Open questions

None.


## Resolution

**Status:** PUSHED-FORWARD (tracked-open OQ-S10-2)
**Verified by:** bf17e55-lineage tree: the phantom-zero constants re-verified as driver-repo sources (src/bench/rdabench.c:390 in heicfast/rda-driver; the real handlers are src/shared/inc/rda_ioctl.h:75-79 + src/driver/rda_ioctl.c:276-280 there).
**Resolution notes:** The probe-list fix (derive from IOCTL_RDA_* names) + the static gate + the hammer re-run are all rda-driver files - outside this session's two-repo grant. Pushed forward as the driver-repo lane's next fix.
