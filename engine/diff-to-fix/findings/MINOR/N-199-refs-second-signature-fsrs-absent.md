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
