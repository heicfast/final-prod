# MINOR N-146 — the FH11 "zero-end family" extent stand-in is unowned: the og's FreeHand-11 analyser returns a CZeroEndRawFileInfo (id 0x1d) whose extent rides the engine's generic zero-end semantics; core substitutes a fixed 32 MiB bounded extent with no owner, no D-row, and no module row

**Severity:** MINOR (an M-106/M-107-class size-walk stand-in that both filed findings' enumerations missed — the og's true extent semantics for FH11 are undecoded and the 32 MiB constant is the honest cap, but the stand-in is registered nowhere a future session would find it)
**Module:** carve (the FreeHand-11 row's extent semantics)
**Parameters affected:** C20 (the size bound), A27 (the extent strategy), P69-adjacent (the stand-in register — the admission is in-code but board-invisible)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:32612-32633` — `FUN_180029f30`, the FH11 analyser returning the zero-end FileInfo (id 0x1d; the decode is quoted in core's own in-code comment); the zero-end engine semantics: the pending-object continuation class (the D-80/D-101 registered architecture) — extent grows until the zero-dword metric ends it
**Core references:** audited `461825d8`-lineage — `src/core/carve/cr_analysers.cpp:3065-3068` ("The og returns a CZeroEndRawFileInfo … NO size walk … the engine's generic extension semantics carry it. Our row … sets a bounded extent"), `:3088-3091` (`end = hit + (32ull << 20)`, the note "fh11-key+const-gate"); the registration state: the only prior owner is the **purged** S-037 (`FIXED-LEDGER.md:313`); absent from M-106 (`findings/MAJOR/M-106-one-3ds-size-walks-decoded-but-standing-in.md`) and M-107 (`...M-107-batch1-size-walk-stand-in-residue-unowned.md`) enumerations, from every D-row, and from `modules/carve.md:560-564` (the S-037 row)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly enumerated by the S11-lead Lane E stand-in sweep (Section 2C); the code comment is honest but its owner was purged without the residue being re-homed — the exact class N-129's purge-convention warnings anticipated. NOT covered by M-106/M-107 (their site lists), S-037 (purged), or D-95/D-101 (the adjacent architecture registers).

## Summary

The FH11 (FreeHand 11) carve row stands in for an undecoded og behavior: the og's analyser hands the engine a zero-end FileInfo and the extent is decided by the generic zero-dword-termination semantics of the pending-object architecture; core emits a fixed 32 MiB bounded extent. The stand-in is documented in-code but owned by nothing on the board — S-037 was purged with the residue un-re-homed, and neither M-106 nor M-107's enumerations include the site.

## What the reference does

`FUN_180029f30` returns the id-0x1d zero-end FileInfo (no per-format walk); the
engine's pending machinery grows the extent until the zero-end metric terminates
it (the D-80/D-101-registered architecture core deliberately does not port).

## What our core does

A fixed `32ull << 20` extent behind the fh11 key+const gate (cites above), with
the honest in-code comment and no board owner.

## Gap

Falsifiable: an FH11 fixture whose true zero-end extent is shorter than 32 MiB
carves at 32 MiB in core (over-read into adjacent data) and at the zero-end
boundary in the og; one longer than the og's zero-end metric would allow carves
at 32 MiB where the og stops earlier.

## Impact

Same class as M-106/M-107's sites: one format's extent can diverge from the
reference in both directions. FH11 (FreeHand) files are rare in the wild; the
practical weight is low, but the registration gap means a future
completeness session could re-litigate or miss it.

## Reproduction / verification

Code-read on both sides (the in-code decode comment carries the og cite; the
core constant is cited); the registration-state greps (M-106/M-107/module
row/D-rows) are the Lane E sweep's, re-verified by the lead at filing. Full
record: `evidence/s11-lead-deep-lanes.md` §Lane-E C-1.

## Fix plan (for the engineering team)

1. Re-home the registration: add the site to M-107's enumeration (the finding
   is active — one line in its site list + the module row), OR land the real
   semantics if the zero-end metric is worth porting for one format (it is not —
   the D-101 stateless architecture decline covers it; the stand-in is the right
   posture).
2. If the stand-in stays: pin it — a test asserting the FH11 row's 32 MiB cap
   (the M-107 pin pattern) so the constant cannot drift silently.
3. Acceptance: the site appears in M-107's list or a D-row; the cap is pinned.

## Tests to add

- `Carve.Fh11BoundedExtentPinned` (the 32 MiB constant).

## Related findings

M-107 (the stand-in family's owner — this site joins its list); M-106 (the
sibling family); D-101/D-80 (the zero-end architecture decline that makes the
stand-in the right posture); S-037 (the purged prior owner); N-129 (the
purge-convention residue class).

## Open questions

None — the og behavior is cited, the stand-in is cited, the gap is registration.
