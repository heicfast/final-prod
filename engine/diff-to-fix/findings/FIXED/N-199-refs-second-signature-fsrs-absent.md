# MINOR N-199 — ReFS recognition lacks the og's second signature ("FSRS" @+0x10) and the classification-time volume-length capture (Partition FUN_180003900 gates "ReFS"+0x3 AND 0x53525346@+0x10)

**Severity:** MINOR
**Module:** refs / orchestrator (probe_family)
**Parameters affected:** B3 (false-positive dispatch surface), A25, C19 (signature constants)
**Corpus references:** `re-workspace/reversed/ghidra-mo/Partition_decompiled.c` FUN_180003900 (:1745-1759, re-read first-hand this session):
```c
else if ((*(int *)(param_2 + 3) == 0x53466552) && (*(int *)(param_2 + 0x10) == 0x53525346)) {
    *(undefined8 *)(param_1 + 0x30) = *(undefined8 *)(param_2 + 0x18);   /* volume length @+0x18 */
    … accept …
}
```
**Core references:** audited `34d6cf10` — `src/core/orchestrator/cr_scan.cpp:834-835`: `if (std::memcmp(sec + 3, "ReFS", 4) == 0) return FsFamily::Refs;` (single signature; no length capture)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ReFS classifier requires BOTH the "ReFS" OEM string at +3 AND the "FSRS" marker (0x53525346) at +0x10, and captures the volume length from +0x18 at classification time. The core's probe_family checks the OEM string alone. A sector with a forged "ReFS" OEM and no FSRS marker dispatches the ReFS family in the core and is declined by the og.

## What the reference does

The double-signature gate quoted above (both compares verbatim; 0x53466552 = "ReFS" LE, 0x53525346 = "FSRS" LE — the ReFS bootstrap's identify pair).

## What our core does

The single memcmp quoted above; no FSRS check, no volume-length capture at classification (the engine's own open re-reads the bootstrap).

## Gap

Falsifiable: a sector with "ReFS" at +3 and zeros at +0x10: og declines the ReFS classification (falls to the next family test); core dispatches ReFS (the engine then finds no valid metadata pages — dispatch-time noise in the recognition log and any consumer of family-probe results).

## Impact

Minor FP-dispatch surface; no wrong-content risk. The missing length capture is cosmetic at classification (the engine re-reads it).

## Reproduction / verification

Decompiled evidence, lead-verified first-hand (both compares quoted verbatim this session). Core side quoted at `34d6cf10`.

## Fix plan (for the engineering team)

1. Add the second signature to probe_family's ReFS branch: `&& std::memcmp(sec + 0x10, "FSRS", 4) == 0` (one line).
2. Optionally capture the volume length (+0x18) into the probe result for logging parity.
3. Files: `src/core/orchestrator/cr_scan.cpp`.

## Tests to add

- `Recognition.RefsRequiresFsrsMarker` (forged-OEM fixture declines) — RED today.
- `Recognition.RefsValidBootstrapDispatches` (control).

## Related findings

- M-146 (the probe_family gate-family correction for NTFS — the same function; land as one recognition-gates pass).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (the S60 lane's N-199 landing — the two-signature gate + the forged-OEM refusal pin; SHA in the worklog entry and the ledger row)
**Verified by:** Refs.RecognitionRequiresFsrsMarker — RED pre-fix (the forged-OEM fixture dispatched FsFamily::Refs on the OEM memcmp alone), GREEN post-fix (Unknown without the marker, Refs with it); the full Refs family 41/41; the lost/ldm/probe neighborhood 153/153 (no dispatch-consumer regression).
**Resolution notes:** Corpus re-verified first-hand before the change: FUN_180003900 at Partition_decompiled.c:1747-1759 — `*(int *)(param_2 + 3) == 0x53466552` AND `*(int *)(param_2 + 0x10) == 0x53525346`, the volume-length capture at +0x18 into +0x30 — exactly as filed. Core re-verified at the current tree: probe_family's ReFS branch carried the single `memcmp(sec + 3, "ReFS", 4)` (the audit's :834-835 cite drifted to :906-909 through the M-043/exFAT landings above it; the divergence stood). The fix: the branch now requires both signatures (the FSRS marker at +0x10 beside the OEM); the classification-time volume-length capture is declined as cosmetic (the engine re-reads it at open — the finding's own fix-plan note), no D-row needed for a sub-behavior the core surfaces identically through the engine's open. The M-146 same-function NTFS gate-family finding is untouched by this landing (different branch, stays active). The landing rides the same recognition-gates pass the finding's Related-findings note anticipated.

**Convergence addendum (the racing S60 lane's independent landing, 2026-10-29):**
This lane independently verified the finding and built the identical gate — the
same OEM+FSRS pair at the same probe_family branch, the same corpus re-read
(FUN_180003900, both compares), and its own RED proof (the stash-revert
rebuild: the forged OEM dispatched Refs pre-fix). Per SA-9 earlier-push-wins the
remote lane's landing is the fix of record; this lane's unique supplements ride
the convergence push: the lost-partition side's stale comment re-truth
(cr_candidates.cpp — probe_family's gate citation updated to the second
signature, with the lost-side page-signature probe named the deeper proof) and
this lane's duplicate test pair dropped per the superset rule (the winner's
Refs.RecognitionRequiresFsrsMarker covers both the forged and the control legs).
