# MAJOR M-032 — Slice tail-read semantics: og clamps + zero-fills (reads crossing the slice end succeed); core refuses

**Severity:** MAJOR (edge-case behavior; ours is safer but engines were written against the og contract)
**Module:** storage (slice layer)
**Parameters affected:** S35 (error recovery semantics), B1 (edge detection behavior), Q46 (contract documentation)
**Corpus references:** HFSPlusSearchDLL_decompiled.c — `FUN_180009290` :7219-7264 (DRWUI twin :1044939-1044979, identical): `if (0 <= off && off < slice_sectors && count <= slice_sectors) { n = count; if (slice_sectors < off+count) n = slice_sectors - off; ok = parent->Read(buf, off + base, n); if (!ok) { err = -3; return 0; } memset(buf + n*bps, 0, count - n); return ok; } err = -1; return 0;` — **clamped read with zero-filled tail, call succeeds**; note the og's tail `memset` length is `count - n` in SECTOR units (:7258) — an under-zeroing og defect. Evidence: `evidence/decompiled-quotes/s3-virtual-volume-device.md` §2.
**Core references:** `src/core/storage/cr_slice.cpp:43-50` — `if (offset > length_ || u64(length) > length_ - offset) return ERR_RANGE_OUT_OF_BOUNDS;` strict refusal; tested at `tests/unit/test_slice_devmap.cpp:44-47`. Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-016` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

A read that starts inside the slice but crosses its end succeeds in the og: the in-bounds prefix is read from the parent and the past-end tail is zero-filled. Our PartitionSlice refuses the whole read with ERR_RANGE_OUT_OF_BOUNDS. The og's engines were written against the clamp+zero-fill contract (e.g. structures probed near a volume's computed end); if any of our engines (or a future one) issues such a read expecting zeros, it gets a hard error instead.

## What the reference does

Quoted in the corpus references — clamp, rebase, zero-fill tail, succeed; parent failure is the only error (−3); null buffer −2; fully out-of-range −1.

## What our core does

Any read not fully inside [0, length_) is refused; no partial result, no zero-fill.

## Gap

Tail-crossing reads: og = partial data + zeros + success; core = error. Falsifiable: a 1-sector read at offset length_-512 with length 512 succeeds with 512 zeros past the end under the og semantics; ours returns ERR_RANGE_OUT_OF_BOUNDS.

## Impact

Edge-case recovery divergence on ragged/damaged candidates whose metadata overshoots the computed end (the og's engines would scan "zeros"; ours sees an error). No current core engine is known to rely on either behavior (the falsifiable claim above is the test), so practical impact is bounded to future engines and parity benches.

## Reproduction / verification

Corpus-derived (both binary copies identical, quoted). Core: code read + existing test asserting refusal.

## Fix plan

1. Decision gate first: audit every engine's read patterns for intentional end-crossing (the HFS+ journal at volume end is the og's known case — the og's own HFS engine reads the journal through this device). If none: keep strict refusal and DOCUMENT the deliberate divergence (downgrade to INFORMATIONAL with a pin test).
2. If parity is wanted: add `read_at_clamped()` (or a flag) implementing clamp+zero-fill with BYTES-correct tail length (fixing the og's sector-unit memset defect), used only by engines that opt in.

## Tests to add

- `Slice.TailReadRefusalPinned` (decision 1) — current behavior pinned with a comment citing the og contract.
- `Slice.TailReadClampedZeroFill` (decision 2) — opt-in path: prefix bytes + zero tail + OK.

## Related findings

I-017 (the slice layer's deliberate supersets — this finding is the one behavioral divergence among them), M-033 (BitLocker placement — same layer family).

## Open questions

Does any og engine deliberately read past the virtual volume end relying on zero-fill (HFS+ journal, ext backup SBs)? A focused probe of HFSPlusSearchDLL's journal read lengths decides; until then the impact stays bounded.

## Resolution (session 11 — DFX11, the decision-gate outcome)

**Status:** FIXED (by the fix plan's own decision path 1: keep the safer
refusal, register the deliberate divergence, pin it — no behavior change
is wanted; changing it would diverge FROM safety FOR parity)
**Fixed in commit:** this session's fix commit (`fix: M-032` — see git log)
**Verified by:** `Slice.TailReadRefusalPinned` (new: the finding's
falsifiable claim pinned — the crossing read that would succeed with
zeros under the og semantics refuses here and never touches the buffer)
+ `Slice.BoundsEnforced` updated with the full og-contract citation
(HFSPlusSearchDLL FUN_180009290 :7219-7264 + the og's sector-unit
under-zeroing defect noted). Both pass by design — they PIN the
deliberate divergence; the finding's own plan prescribes exactly this
("current behavior pinned with a comment citing the og contract").

**Resolution notes:** the fix plan's decision gate was executed first:
audited every engine's read patterns for intentional end-crossing. The
only end-adjacent surface is the HFS+ journal ring
(journal_ring_read bounds every take within [jbase, jbase+jsize) —
wrap-aware, never crossing); every other engine reads with explicitly
bounded lengths and degrades gracefully through the damage counters on
refusal. NO engine relies on the og's clamp+zero-fill contract, so
path (2) — an opt-in read_at_clamped — would be dead code on arrival
(the repo just swept dead code in N-041). The divergence is real and
deliberate: ours is the safer contract (a refused read is honest; a
zero-filled read can fabricate structure). Registered PARITY-DELTAS
D-67. The og's own tail memset is in SECTOR units (count-n, :7258) —
an under-zeroing defect NOT adopted. The open question (does any og
engine deliberately read past the end relying on zero-fill?) is
resolved for OUR side by the audit; the og-side probe (HFS+
journal/ext-backup-SB read lengths) remains OQ-tracked via I-017's
register — no change wanted until an engine actually needs the og
contract, at which point the opt-in primitive from path (2) is the
documented next step. Cross-refs: I-017 (the slice layer's deliberate
supersets register), M-033 (same layer family, untouched here).
