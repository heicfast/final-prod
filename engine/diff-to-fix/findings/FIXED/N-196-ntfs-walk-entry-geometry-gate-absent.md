# MINOR N-196 — NTFS walk-entry record-geometry gate absent in the core (odd or >16-device-sector records): the og's walk refuses (0x610016); the core scans — a core superset in the accept direction, unregistered

**Severity:** MINOR
**Module:** ntfs (MFT walk entry geometry)
**Parameters affected:** B1 (detection — accept-direction divergence), A26 (walk bounds), Q68 (registration completeness)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` FUN_1800272b0 — primary path `:31620-31621` (`if (((uVar8 & 1) != 0) || (0x10 < (byte)uVar8)) goto LAB_180027467;` where `uVar8 = record_bytes / device_sector`) and fallback path `:31599-31600` (`if (((uVar7 & 1) == 0) && ((byte)uVar7 < 0x11))` where `uVar7 = (record_bytes + …) >> 9`), both re-read first-hand this session (the S-063 verification trail); both failing → `uVar8 = 0x610016` (the walk's refusal)
**Core references:** audited `34d6cf10` — `src/core/ntfs/cr_bootstrap.cpp` (P1-4 caps record size at 1 MiB; non-pow2 sizes pass `validate_boot_sector`; no even/≤16-device-sector gate in `walk_mft`'s entry)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's MFT walk entry refuses records whose device-sector count is odd or exceeds 16 (both the primary and the mirror-fallback branches gate this, in device-sector and 512-byte-rounded forms respectively — 3-sector 1536-byte records and ≥17-sector records are refused with 0x610016). The core has no equivalent gate anywhere between open and walk. The core therefore scans volumes the og declines — a superset in the accept direction that was never registered.

## What the reference does

The two gates quoted above: the primary branch computes `record_bytes / device_sector` and bails to the fallback on odd/>16; the fallback computes the 512-byte-rounded size and requires even/≤16 to proceed; both failing sets the refusal code. Note the og ACCEPTS non-pow2 even sizes (3072/5120/6144 bytes) — parity with the core there.

## What our core does

`validate_boot_sector` enforces the P1-4 exponent cap and sub-sector rejection; `walk_mft`'s entry applies no geometry gate — a 1536-byte-record volume (3 × 512) opens and walks.

## Gap

Falsifiable: a volume with a 1536-byte record size (odd 3-sector) or a 17+-sector record: og walk refuses; core scans. No wrong-content risk in the core's direction (the walk validates each record); the divergence is acceptance-band only.

## Impact

Core superset (recovers volumes the og declines); the board's register completeness requires the stance recorded. Also relevant to S-063's fix: the og routes the odd/>16 geometry through the MIRROR fallback before refusing — the geometry gate and the mirror fallback are one code path in the og.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand (both gates read in the S-063 trail). Core side: the P1-4 cap and the walk entry read at `34d6cf10`.

## Fix plan (for the engineering team)

1. Decide the stance: (a) register the core's wider acceptance as deliberate (the D-119 numeric-hygiene family precedent — recommended: the core's per-record validation makes the wider band safe, and refusing recoverable volumes is the worse direction per the board's history); or (b) port the gate into `walk_mft`'s entry (NOT into open — the og's layering puts it at the walk).
2. If (a): add the D-row (accept-direction superset, the two og gate sites cited) and a pin test asserting a 1536-byte-record fixture scans.
3. Files: `docs/PARITY-DELTAS.md` (+ the pin test) or `src/core/ntfs/cr_mftwalk.cpp`.

## Tests to add

- `Ntfs.OddSectorRecordScans` (the pinned stance — currently unasserted either way).
- `Ntfs.OversizeRecordCappedByP14` (the existing cap pinned adjacent).

## Related findings

- S-063 (the mirror fallback this gate shares a code path with).
- M-138 (the cpr defaults — the same record-geometry decode family).

## Open questions

None — both og gates are directly read; the only open item is the stance decision.

## Resolution

**Status:** FIXED (stance registered + pinned — option (a), the finding's recommended disposition)
**Fixed in commit:** (this push; S60-B lane)
**Verified by:** `NtfsVolume.OddSectorRecordWalkScans` (the 1536-byte odd-stride fixture opens, walks, and surfaces oddrec.txt — the og's 0x610016 refusal class; RED against any future walk-entry gate addition, forcing the register to be re-truthed first) + `NtfsVolume.OversizePositiveRecordOpensAndWalks` (the 17-device-sector 8704-byte stride opens and walks — the >16 half of the same stance) + the adjacent P1-4 cap pin re-verified (`BootNegativeExponentOverflowRejected`); full NtfsVolume family green.
**Resolution notes:** Corpus re-verified first-hand: FUN_1800272b0's primary gate `((uVar8 & 1) != 0) || (0x10 < (byte)uVar8)) goto LAB_180027467` at :31620-31621 and the fallback's 512-rounded even/≤16 form at :31599-31600, both failing → 0x610016 — exactly as filed. Core re-verified: no geometry gate between open and walk; the P1-4 shift-UB guard (1 MiB, negative-exponent form) and the 0-result/sub-sector rules are the only record-size bounds. Disposition per the finding's own recommendation (a): the wider acceptance registered as PARITY-DELTAS **D-143** (accept-direction superset, both og gate sites cited, the per-record-validation safety rationale, the refusing-recoverable-volumes-is-worse calibration) + the two stance pins above. No behavior change — the registration and pins are the finding's deliverable.
