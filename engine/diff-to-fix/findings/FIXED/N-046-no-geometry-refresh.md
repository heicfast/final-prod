# MINOR N-046 — No geometry refresh path on slices (og exposes RefreshSectorSize-from-parent)

**Severity:** MINOR
**Module:** storage
**Parameters affected:** S35 (error recovery — removable-media change), Q43 (ownership/lifetimes of geometry)
**Corpus references:** DRWUI_decompiled.c :1044889-1044979 (`FUN_140643870` RefreshSectorSize-from-parent; HFS twin :7163-7177).
**Core references:** `src/core/storage/cr_slice.cpp:37-39` (geometry snapshot at create; no refresh API). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `N-026` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's virtual volume device can re-derive its sector size from the parent mid-session; our slices snapshot geometry at creation. A removable-media sector-size change mid-scan yields stale geometry until reopen. Edge case (USB reader swap); recorded for completeness.

## Fix plan

Add `refresh_geometry()` to the slice (re-read parent geometry, revalidate alignment) and call it from the orchestrator's retry path if a device error suggests media change; or document the exclusion.

## Tests to add

- `Slice.GeometryRefreshAfterMediaChange` — mock parent whose sector size changes; assert refresh picks it up.

## Related findings

I-017 (slice supersets), N-045 (same layer).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-046 commit — DFX18 session 18)
**Verified by:** `Slice.GeometryRefreshAfterMediaChange` + `Slice.GeometryRefreshRefusesInvalidatingChange` (fail-before by construction: the refresh surface did not exist — `slice->refresh_geometry()` does not compile on the pre-fix tree; the stale-snapshot premise is asserted in-test before the refresh). Full suite green on the landing tree.
**Resolution notes:** Corpus re-verified first-hand: FUN_140643870 (DRWUI_decompiled.c:1044889-1044895) — the virtual volume device re-derives bytes-per-sector from the parent vtable call (+0x28) and stores it at +0x38, returning 1 unconditionally; the surrounding vtable block (:1044897-1044979) is the M-032 tail-clamp read family. Implemented as `PartitionSlice::refresh_geometry()` (cr_slice.h/cpp): re-reads the parent geometry, revalidates (zero sector size, start alignment, slice-fits-parent), then recomputes the derived fields (total_bytes stays the slice length; sector_count re-floors). REGISTERED HARDENING over the og's blind store: an invalidating media change is REFUSED with the error and the snapshot stays unchanged — the og's unconditional adopt could leave a misaligned slice silently serving wrong geometry. The orchestrator auto-refresh branch of the fix plan is EXCLUDED and documented in the header: the corpus exposes the method but no scan-path caller (media-change detection is transport-layer; read failures surface through the error map either way) — wiring an invented trigger would be hallucinated parity.
