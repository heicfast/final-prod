# MAJOR M-013 — exFAT: og validates entry-set checksums for DELETED sets too (dual-form) and uses them as a reconstruction oracle; core validates live sets only, as a downgrade

**Severity:** MAJOR (deleted-set integrity handling; torn-set reconstruction missing)
**Module:** exfat
**Parameters affected:** B3 (false positives), B4 (deleted classification), B5 (orphan reconstruction), D15 (grading)
**Corpus references:** `exFATSearchDLL_decompiled.c` — `FUN_18001eba0 :25153-25198` (dual ROL15 checksums: raw form + InUse-restored form with `uVar5 = 0x85`/`0xc0`/`0xc1` substitutions at `:25173-25186`; valid iff either matches the stored `*(u16*)(entries+2)`), wired into set completion `FUN_18001fef0 :26287-26292`, used as a **reconstruction oracle** for parked deleted sets in `FUN_1800203a0 :26642-26664`
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:303-311` (live-set checksum as grading downgrade only: `if (ctx.flags.validate_entry_sets && in_use)`), `src/core/exfat/cr_exfat_engine.h:70` — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied. Supersedes N-003's corpus claim (see that file's session-2 note).

## Summary

N-003 claimed the og never verifies entry-set checksums. The decompile proves the opposite: the og computes **two** ROL15 checksums per set — one over the raw bytes and one over an "InUse-restored" form (byte 0 → 0x85, entry-type bytes → 0xC0/0xC1) — and accepts the set if **either** matches. It applies this to **deleted** sets (where the InUse bit is cleared, so only the restored form matches) and uses the checksum as an oracle when reassembling torn/parked deleted sets, substituting candidate entries until the stored checksum matches. The core validates live sets only, and only as a confidence downgrade.

## What the reference does

`FUN_18001eba0` computes over `(secondary_count+1)*32` bytes skipping indices 2..3: the raw checksum and the InUse-restored checksum; the set is valid iff either equals the stored field. `FUN_1800203a0` (deleted-set reassembly, the "DelexFat" parked-set path) substitutes candidate stream/name entries and re-tests: `if ((uVar19 == stored) || (uVar20 == stored))` → accept the reconstruction.

## What our core does

`cr_exfat_quick.cpp:303-311`: when `validate_entry_sets` is set **and the set is in use**, a checksum mismatch downgrades confidence; deleted sets are never validated; no reconstruction use.

## Gap

Falsifiable claims: (a) a deleted set with a torn checksum-mismatched entry: og rejects/rebuilds it via the oracle; core accepts it unconditionally (FP surface); (b) a *complete live* set with a bad checksum: og's complete-set path applies per-entry validators without a checksum test (graded normal); core downgrades it — inverse asymmetry; (c) a parked deleted set recoverable only by oracle-guided reassembly: og recovers it; core has no path.

## Impact

Deleted-set integrity on exFAT is the primary FP gate for the deep-scan story; missing the dual-form test and the oracle changes both false-positive rates and recall on damaged volumes.

## Reproduction / verification

Decompile-verified with full quotes (agent S2-B; `evidence/decompiled-quotes/fat-exfat-session2.md`); core lines verified first-hand.

## Fix plan

1. Implement the dual-form checksum: raw ROL15 + InUse-restored form (constants above) in the exFAT entry-set validator.
2. Apply to **deleted** sets as an accept/reject gate (matching og), not a live-set downgrade; keep the live-set downgrade only if bench shows value (register the choice).
3. Add the reconstruction oracle for parked/torn deleted sets: candidate substitution re-tested against the stored checksum.
4. Extend the exFAT fuzz harness with torn-set + bad-checksum mutants.

## Tests to add

- `ExFat.DeletedSetChecksumRejectsTornSet` — fails before (accepted), passes after.
- `ExFat.DeletedSetOracleReassembly` — parked set with one wrong entry: og-equivalent reconstruction recovers the right name/stream.
- `ExFat.LiveCompleteSetBadChecksumUngraded` — pins the inverse asymmetry.

## Related findings

- Supersedes N-003 (its "no SetChecksum verification anywhere" corpus claim is refuted; the core-stricter-on-live-sets direction is weakened) · M-014 (adjacent structural gates)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-013 commit — the dual-form gate + the reconstruction oracle + the three named tests, landed on the DFX16 lane)
**Verified by:** `ExfatEngine.DeletedSetChecksumRejectsTornSet` + `ExfatEngine.DeletedSetOracleReassembly` (both RED on the reverted tree — fail-before executed via the temporary gate+oracle disable; both GREEN after), `ExfatEngine.LiveCompleteSetBadChecksumUngraded` (the inverse-asymmetry pin, holds in both states by design), the full existing exFAT suite (25/25), and the full suite **730/730** (linux-gcc RelWithDebInfo + ASan). The dual-form arithmetic joined the fuzz surface (fuzz_pure case 3: `entry_set_checksum_dual` + `entry_set_checksum_valid` on arbitrary 32-multiple buffers; the scan harness exercises the gate+oracle on every mutated corpus input).
**Resolution notes:** Corpus re-verified line-by-line this session: FUN_18001eba0 (:25153-25198 — the exact dual ROL15 rotate-add, the 2..3 skip, the 0x85/0xC0/0xC1 InUse-restoration ladder gated on entries[0]==0x05) and FUN_1800203a0 (:26539-26830 — the oracle's suffix-substitution loop: position from secondary_count DOWN to 1 (LAB_18002083f), the candidate overlay memcpy, the dual re-test `if ((uVar19 == stored) || (uVar20 == stored))` at :26662-26664; the parked-list callers at :26299/:26403 iterate head x tail with the not-self guard). Implementation: (1) `entry_set_checksum_dual`/`entry_set_checksum_valid` in the dentry module (decompile-exact); (2) the DELETED-set dual gate is UNCONDITIONAL og parity in the walk — a failed deleted set is dropped from direct emission (FUN_18001fef0's refusal semantics) and PARKED with the torn-at-buffer-end sets; (3) orphan stream/name runs at set-head positions are parked as tails (the og's parked-tail pool — the pre-fix walk silently skipped them); (4) `reconstruct_parked_sets` runs after the whole walk (the og's park is scan-global): each parked head x each parked tail x each suffix position, the substituted candidate re-tested with the dual checksum, an accepted reconstruction re-walked through the standard emission path (the substituted bytes parse, pass the gate, and land in the tree like a never-damaged set). LIVE sets keep the flag-gated downgrade exactly as before (the registered deliberate divergence — the flag doc in cr_exfat_engine.h now records N-003's REFUTED premise and the flag's narrowed live-only scope). New stats: `sets_rejected`/`sets_reconstructed`. Fixture truth update: the deleted-set builders now stamp SetChecksum via finish_set (the old "deleted sets: no checksum validation (InUse clear)" comment encoded N-003's refuted premise — a real deleted set keeps its original checksum field; deletion only clears InUse bits; the unstamped fixtures would now be rightly rejected). No safety hardening affected; no ABI change (stats fields are additive to a C++ struct, the flag semantics documented in place).
