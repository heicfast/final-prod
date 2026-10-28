# MINOR N-120 — a batch of citation/arithmetic drifts in the trail's own boards: I-010's D-26 pointer over-claims, SURFACE-INVENTORY's exclusion pointer targets the wrong register (twice), the EuShare artifact count is wrong, the per-binary never-cited arithmetic is off, PARITY-CORE-REC's cited line is off-by-one, and the corpus-commit pointers still say 03ad46c (now be0cfe9)

**Severity:** MINOR
**Module / Surface:** docs (audit-trail boards)
**Parameters affected:** 46 (contract documentation), 68-adjacent
**Corpus references:** n/a — audit-trail internal-consistency finding. Corpus commit context: the trail cites 03ad46c throughout; the corpus moved one commit (CORRECTIONS.md #9) to be0cfe9 — line-cited artifacts are unmoved (the commit only added the corrections row; verified: the additions touch CORRECTIONS.md, not the cited artifacts).
**Core references:** audited commit `ff879f9` — (a) `diff-to-fix/findings/FIXED/I-010…md:10` lists "Installer, Licensing/quota, runtime-families (Qt/OpenSSL/UCRT/codecs): D-26 family" — D-26 (PARITY-DELTAS:639-646) contains none of those (licensing = D-32, installer = S-038/PACKAGING, runtime = the OGI RUNTIME legend :39 with NO PARITY-DELTAS row — per the register's own rule (:6-7) a missing row is a bug); (b) `diff-to-fix/SURFACE-INVENTORY.md:86` points Cloud/licensing/languages/runtime exclusions at CAPABILITY-MATRIX §3 — §3 (:77-92) registers only Cloud/NAS/remote, BitLocker-derivation, signing, .rsf-import; same class at :85 (FileRepair → CAPABILITY-MATRIX instead of PARITY-CORE-REC.md:163 + OGI:79); (c) SURFACE-INVENTORY:28 "EuShare/* (5 artifacts, cited)" vs 2 artifacts on disk; (d) :87 "per-binary 113/114 never-cited" vs ≥2 cited (libvmdk modules/mom-capabilities.md:40; deepening-batch1 PARAMETERS.md:144); (e) findings cite "PARITY-CORE-REC.md:164" for the repair-chain exclusion but the text is at :163 (:164 blank); (f) the gate docs (SURFACE-INVENTORY, PARAMETERS) pin corpus 03ad46c — now be0cfe9.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** Session-5/6 built the citation-gap method on these boards; the drifts are the method's own residue.

## Summary

("Six small pointer/arithmetic defects in the audit trail's own navigation boards, found by the session-8 citation-gap sweep: two wrong-register pointers (which, if followed, would make the exclusions look unregistered), a D-26 over-claim, two count errors, an off-by-one line cite, and a stale corpus-commit pin. None changes a verdict; all degrade the trail's navigability — which is the product this directory ships.",)

## What the reference does

n/a

## What our core does

As cited per item.

## Gap

Falsifiable per item (read the cited lines).

## Impact

Roadmap navigability (the mandate's own deliverable); the two wrong-register pointers are the load-bearing ones (they make registered exclusions look silent).

## Reproduction / verification

All six verified first-hand this session (the sweep context sheet).

## Fix plan (for the engineering team)

1. I-010: fix the D-26 pointer (split into D-26/D-32/OGI-legend refs) or add the missing runtime-families PARITY-DELTAS row. 2. SURFACE-INVENTORY:85-86: repoint to the real registers (PARITY-CORE-REC.md:163, OGI:30/:39/:79, D-26/D-32). 3. :28 EuShare count → 2. 4. :87 arithmetic → ≤112/114. 5. Re-pin PARITY-CORE-REC cites to :163. 6. Refresh the corpus-commit pin to be0cfe9 in the gate docs + README (with the line-stability note).

## Tests to add

None (doc fixes).

## Related findings

N-119 (D-88, the same register-truth family); the session-8 GAP-SWEEP sheet.

## Open questions

None.

---

## Resolution

**Status:** FIXED (the live residues) + FIXED-ALREADY (the rest, by the parallel lanes)
**Fixed in commit:** (this session's N-120 doc-truth batch — the session-38 minor-first lane)
**Verified by:** per-item live-tree reads (below); the load-bearing wrong-register pointers verified FIXED before classification.
**Resolution notes:** Re-verified item by item at current main: (a) I-010's D-26 pointer over-claim — the finding file is retired to git history (SA-10B purge); the live registers are truthful (licensing = D-32, installer = S-038/PACKAGING, runtime-families = the OGI RUNTIME legend :39) and the runtime-families no-row gap is superseded by the OGI legend's own registration — nothing live to edit, recorded here; (b) SURFACE-INVENTORY:85-86's wrong-register exclusion pointers — the FileRepair row was rewritten by the SA-10B S-050 filing ("the prior row's OUT-OF-SCOPE (CAPABILITY-MATRIX §3) citation was FALSE") — FIXED-ALREADY; (c) the EuShare artifact count — FIXED-ALREADY (SI:40 "2 artifacts — was a miscount"); (d) the per-binary never-cited arithmetic — the one LIVE residue, fixed this session alongside N-101 (113/114 → 112/114 with both cites named); (e) PARITY-CORE-REC's repair-chain line — verified at :163 (the three findings citing :164 are retired to history; the live text needs no edit); (f) the corpus-commit pins — FIXED-ALREADY (be0cfe9 at SI:20/:138-139 and PARAMETERS:13, read this session). The load-bearing class (wrong-register pointers that made registered exclusions look silent) is closed; the trail's navigation boards are consistent.
