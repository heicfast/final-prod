# MAJOR M-160 — the APE version gate reads the wrong byte order and the wrong window: the core gates u32 BE@+4 ∈ [300, 5000] while the og gates u16 LE@+4 ∈ [1000, 9999] — every real Monkey's Audio file is REJECTED by the core's carve (a whole-family recovery absence, the S-007 class)

**Severity:** MAJOR (a whole format family invisible to the carve — the exact class S-007 filed for missing formats; discovered during M-107's APE-member verification)
**Module / Surface:** carve (modules/carve.md) — the APE row's gate
**Parameters affected:** A25-adjacent (the acceptance gate), A47-adjacent (the row's noise filtering)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:39082-39096` (the CAPEAnalyser tier-1 FUN_180033090: the registry-key hit accepts unconditionally — type stamp 0x51, return 1); **FUN_180033180** (the APE walk, the +0x20 processor): `:39097-39101` — `uVar7 = *(ushort *)(*param_2 + 4); if ((9999 < uVar7) || (uVar7 < 1000)) { *(param_1 + 0x10) = 0; ... }` — **the u16 LE at +4 against [1000, 9999]**, the extent-0 decline outside the window; the version constants in the same walk's branches (`< 0xf8c` = 3980, `< 0xed9` = 3801) prove the values are the u16-LE release integers ×1000.
**Core references:** at this lane's HEAD: `src/core/carve/cr_analysers.cpp` `validate_ape` — `const u32 v = (u32(h[4]) << 24) | (u32(h[5]) << 16) | (u32(h[6]) << 8) | u32(h[7]); if (v < 300 || v > 5000) return r;` — the u32 BE at +4 against [300, 5000]. The APE format's version field is a **u16 LE at +4** (the Monkey's Audio header: "MAC " + u16 version, e.g. 3970 = bytes 82 0F): a real file's u32-BE read of `82 0F ?? ??` is ≈ 0x820F0000 — far outside [300, 5000] → **the core rejects every real APE file**; conversely the core's own test plant (`putbe32(img, at + 4, 3980)` = bytes 00 00 0F 8C) reads as u16 LE = 0x0000 — **the og would reject the core's plant**. Both directions verified first-hand this session.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule; the APE size WALK itself stays with M-107 — this finding owns only the gate)

## Summary

The og's APE chain: the registry key "MAC " (0x2043414d → BruteForce::CAPEAnalyser idx 0x51) accepts the candidate unconditionally, and the walk (FUN_180033180) gates the version — **the u16 LE at +4 must be in [1000, 9999]** (the release integers 1000-9999, i.e. Monkey's Audio 1.0-9.99 era; the walk's own branches split at 3980 and 3801) — else the extent stays 0 and the engine's extent!=0 gate declines. The core's gate reads the SAME four bytes as a u32 BE and windows [300, 5000] — a byte order and window that match NO real APE file: every real Monkey's Audio archive (u16 LE version 3960/3970/3980/3990...) is rejected by the core's carve. The family is invisible in recovery results — the same user impact class as S-007's missing formats, found during M-107's APE-member verification.

## What the reference does

`FUN_180033180` (the CAPERawFileInfo's processor, raw asm 0x180033180+): on the first chunk, `uVar7 = *(ushort*)(buf+4)`; the window check `1000 <= uVar7 <= 9999` (the decompile's `(9999 < uVar7) || (uVar7 < 1000)` → the extent-0 stamp, the decline class). The cached version then drives the header-form split (`< 0xf8c` = 3980: the old 32/36-byte header with the format-flags data start; `>= 3980`: the APEv2 descriptor + 24-byte header) — the walk machinery that remains M-107's member.

## What our core does

`validate_ape` reads the u32 BE at +4 and gates [300, 5000] — a gate that accepts only files whose +4 bytes are 00 00 01 2C through 00 00 13 88 (the version encoded as a u32 BE): the core's own synthetic plant form, not the real format. Real APE files (82 0F / 8C 0F / 96 0F at +4) all fail. The S-007-era comment ("release integers 3.90-4.x") shows the intent was the real window — the byte order is simply wrong.

## Gap

Falsifiable both directions (both verified first-hand): (a) a real-form APE header ("MAC " + u16 LE 3980 + the plausible old-form body) is REJECTED by the core's current gate — the fix makes it carve; (b) the core's own BE-form plant is rejected by the og's gate (u16 LE = 0) — the plant is re-pinned to the real form with the fix.

## Fix

`validate_ape`'s gate becomes the og's exact form: `const u32 v = u16_from_bytes_le(h + 4); if (v < 1000 || v > 9999) return r;` — the u16 LE at +4 against [1000, 9999]. The 64 MiB bounded extent STAYS (the M-107 APE member's recorded remainder — this finding owns the gate, not the walk; the walk's decode progress is appended to M-107's file for the future lane: the singleton+0x70 × 10 = 2 GB walk cap, the header-form alignment gates, the APETAGEX tail scan). The plant re-pinned to the real form (u16 LE 3980); the S-007 noise plant re-pinned to the out-of-window form (u16 LE 500).

## Tests to add

- `ApeRealFormVersionGateCarves` — "MAC " + u16 LE 3980 → carves (RED at the old gate: rejected).
- `ApeVersionWindowIsTheOgsThousandWindow` — versions 999 and 10000 (both sides outside [1000, 9999]) decline; 1000 and 9999 (both edges) carve.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S87 fix lane, 2026-10-30)
**Verified by:** the two-test battery RED at the old gate (the real-form plant rejected — the whole-family absence reproduced) → GREEN with the u16-LE gate; the S-007 batch test (the re-pinned plant) green; the S-007 noise test (the re-pinned out-of-window noise) still rejecting; the full carve suite green.
**Resolution notes:** The gate fix is the og's exact window (u16 LE@+4 ∈ [1000, 9999], FUN_180033180's first check). The size walk stays M-107's APE member — the standing remainder now carries this lane's decode progress (the 2 GB singleton cap, the two header forms' alignment gates, the seek-table max walk, the APETAGEX backward tail scan with the tag-size adjustment) recorded in M-107's S87 addendum 4 for the future landing lane. The M-160 seat is the gate defect alone; the family's recovery presence is restored by this fix (the extent accuracy is M-107's scope).
