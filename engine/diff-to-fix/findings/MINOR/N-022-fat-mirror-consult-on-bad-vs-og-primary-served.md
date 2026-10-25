# MINOR N-022 — FAT mirror consult on BAD: the core consults FAT2 where the og serves the primary's BAD entry directly

**Severity:** MINOR
**Module:** fat
**Parameters affected:** A27 (fragment/chain handling — mirror consult policy), B4 (deleted-file classification on torn FAT pairs), A35 (error recovery — BAD-cluster policy)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c :54207-54251` (CDoubleFATTable::GetNext = FUN_180041de0 — the merged per-entry read; full verbatim quote in `evidence/decompiled-quotes/s2-fat-double-fattable.md`), `:53783-53808` (CFAT12Table::GetNext — BAD 0xFF7 returns raw, non-zero), `:53955-53962` (CFAT32Table::GetNext — raw read); vtable layout extracted from the binary via MSVC RTTI (evidence file §1)
**Core references:** `src/core/fat/cr_chain.cpp:103-110` (`next()`: `if (mirror_ && (v == 0 || v == CHAIN_BAD)) { alt = entry_at(fat2_, cluster); if (alt != 0 && alt != CHAIN_BAD) return alt; }` — comment "choose-healthy mirror: if primary says BAD/FREE, consult FAT2") — audited commit `2545b83`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

On FAT volumes with two FAT copies, the og consults the mirror table per-entry only when the primary's canonicalized GetNext returns **0** (a free/zeroed entry). A BAD-cluster value in the primary is a non-zero result and is served **directly from the primary** — the mirror is never consulted for it, and the chain truncates at that cluster. Our core additionally consults the mirror when the primary says **BAD**, and continues the chain if the mirror holds a non-BAD, non-zero value. On torn FAT pairs (primary BAD, mirror alive) the core recovers longer chains than the og — a result-set superset the og never produces.

## What the reference does

`CDoubleFATTable::GetNext` (FUN_180041de0, quote abridged — full function in the evidence file):

```c
  uVar2 = 0;
  if ((child1 != (longlong *)0x0) &&
     (uVar2 = (**(code **)(**(longlong **)(param_1 + 8) + 8))(child1, cluster), uVar2 != 0)) {
    return uVar2;                    /* primary non-zero (VALID, EOC, OR BAD) -> serve primary */
  }
  ...                                 /* primary said 0: consult mirror (gated) */
```

The child GetNext returns BAD **raw** and non-zero (CFAT12 `:53801-53804`: `if (0xff7 < uVar2) return 0xffffff8; return (ulonglong)uVar2;` — 0xFF7 itself falls to the raw return), so a primary BAD entry takes the first branch and the mirror is never read for that cluster.

## What our core does

`cr_chain.cpp:103-110`: the mirror is consulted when the primary entry is `0` **or `CHAIN_BAD`**, and any non-zero/non-BAD mirror value is served as the chain link — the chain continues through clusters the primary FAT marks BAD.

## Gap

Falsifiable claim: on a FAT volume where the primary's entry for cluster C is BAD (0xFF7/0xFFF7/0x0FFFFFF7) and the mirror's entry for C is a valid link, the og's chain ends at C (BAD surfaced) while the core's chain continues to the mirror's link. Corpus-derived (decompile quote); core-derived (code quote); not executed against the og binary (no Windows/wine here) — **UNVERIFIED-BY-EXECUTION** for the og side, evidence-cited.

## Impact

- Result-set membership on torn FAT pairs (a damaged-volume edge case): the core can emit longer files (and files the og truncates into two fragments as one) where the primary FAT has BAD-marked clusters with divergent mirror entries.
- On healthy volumes (FATs agree, BAD in both) both implementations truncate at the same cluster — no divergence.
- Superset direction (more recovery, never less; the mirror's value is the pre-damage chain on genuinely torn pairs) — but parity says the og's policy is the contract; the core's is an inferred extension of the artifact docs' "choose-healthy" wording.

## Reproduction / verification

Decompiled evidence (verbatim quotes cited); RTTI vtable extraction proving the slot semantics (child slot 1 = GetNext; CDouble slot 2 = merged read). Core-side code quote. The og-side behavioral claim follows mechanically from the quoted branch structure.

## Fix plan (for the engineering team)

1. Decide the policy: strict og parity (consult mirror on primary==0 only; serve primary BAD directly) or keep the superset as a documented deliberate difference. Recommendation: strict parity for the default path, with the BAD-consult behind an opt-in flag (`WalkerFlags`-style) if retained for honest-recovery mode.
2. If parity: in `FatTable::next` (`cr_chain.cpp:104`), drop `|| v == CHAIN_BAD` from the mirror-consult condition.
3. Tests: `FatChain.BadPrimaryMirrorAliveOgParity` — fixture with primary BAD + mirror link; assert the chain truncates at C (og parity) by default, and continues only under the opt-in flag (if kept).
4. Acceptance: the fixture fails before, passes after; no other chain tests regress.

## Tests to add

- `FatChain.BadPrimaryServedWithoutMirrorConsult` (parity pin).
- `FatChain.MirrorConsultGating` — primary 0 + mirror EOC-family / mirror cross-check cases from the evidence file §3 (pins the og's gated-serve semantics if adopted).

## Related findings

- OQ-T2 (RESOLVED — this finding is one of its two consequences; the other is the mirror-answer gating recorded in `tracked-open/`).
- N-002 (alternate EOC families — the same evidence file documents the per-flavor canonicalization incl. the dirty-EOC accept-set `>=0x0FFFFFF9` in CDouble's gate).
- I-002 (FAT12 bad-cluster canonicalization — CFAT12::GetNext's raw-BAD return is the og-side anchor for that finding's semantics).

## Open questions

None for the trigger; the mirror-answer gating's exact cross-check intent is quoted and recorded in `tracked-open/` (OQ-T2 residual).
