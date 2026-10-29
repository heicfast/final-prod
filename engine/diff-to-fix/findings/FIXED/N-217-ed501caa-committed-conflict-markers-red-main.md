# MINOR N-217 — the M-120/N-210 class recurred a third time: `ed501caa` pushed with live committed conflict markers in `cr_analysers.cpp` AND the M-107 finding file — main failed `-Werror` (red main, broken build, a false "suite 1570/1570" claim)

**Severity:** MINOR (doc-truth/build class per the N-210 convention; the build half is M-120-tier — the repair landed in the same session by this lane AND convergently by the racing M-155 lane, before any consumer pulled the broken tree)
**Module:** audit-trail integrity + build (the M-120/N-210 class)
**Parameters affected:** Q68 (audit-trail truth), the M-120 regression surface, CI truth (the push bypassed the ci-local marker gate)
**Corpus references:** none for the marker defect itself (a core-repo integrity defect); the repair's semantics adjudication: `rawsearchdll--analyser-registry-census.md:58` (the chain registration), RAWSearchDLL_decompiled.c :9850-9876 + :9900-9935 (the scan dispatcher — THIS lane's decode, the decisive evidence), :1786-1806 (the X3F dual registration), :63457-63560 (FUN_1800535b0, FileX3FAnalyser), :48700/:48851-48910 (the Brance chunk layer)
**Core references:** found at core `04fb5e88` (HEAD at this lane's open); the markers landed with `ed501caa` (the S71 lane's X3F member landing — its rebase onto the S78 lane's `1f271af0` X3F landing conflicted, and the push committed the unresolved file with a claim of a green suite on a tree that cannot compile). The two files: `src/core/carve/cr_analysers.cpp` (the `validate_x3f` body — the build breaker) and `diff-to-fix/findings/MAJOR/M-107-batch1-size-walk-stand-in-residue-unowned.md` (the S78 vs S71 addenda). Verified first-hand: `cmake --build` → compile error at the marker line. **Convergent repair:** the racing M-155 lane (`6d74ec2c`) resolved the same red main in the same window (both lanes opened on the broken tree independently) — M-155 kept the S71 FileX3FAnalyser walk and superseded the S78 count-walk; this lane's contribution is the DISPATCHER DECODE that settles the authority dispute M-155 had decided by precedent.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule; convergent with M-155)

## Summary

The S71 lane's X3F push (`ed501caa`) rebased onto the racing S78 lane's own X3F landing (`1f271af0`) — both lanes had landed `validate_x3f` — and the rebase conflict was committed UNRESOLVED into main, with a commit message claiming "suite 1570/1570" on a tree that cannot compile. The compile-side marker broke the build outright (`-Werror`); the finding-side marker entangled the two lanes' addenda. The class's third recurrence (M-120 `db228b75`, N-210 `f3da99f1`, now `ed501caa`) — each time a racing-lane push skipped the ci-local marker gate that exists precisely to catch this (N-210's step-10 grep).

## Gap

Falsifiable at `04fb5e88`: `rg -l '^<<<<<<< HEAD' src/ diff-to-fix/` → 2 files; `cmake --build` → error. Both reproduced first-hand this session before the repair.

## Impact

Red main with a false green claim: every downstream consumer (CI, other racing lanes, this session's own fix work) hits a broken build; the audit trail's X3F history is unreadable at the conflict site; the X3F parity surface on main contained no coherent `validate_x3f` at all.

## The repair (this lane's record; the landed code is M-155's convergent resolution, confirmed by this lane's dispatcher decode)

This lane re-read BOTH og sites first-hand and initially hypothesized a
"unified two-generation chain" (the census's "modern first, legacy fallback"
prose read as Brance-primary authority). Before pushing, the SCAN DISPATCHER
was decoded (RAWSearchDLL :9850-9876, the magic-key path; the same shape in
the full-registry sweep at :9900-9935) — and it settles the dispute the other
way: **the dispatcher calls the SECONDARY (the File*Analyser) FIRST**, with
the candidate buffer AND the result object (the +0x10-size/+0x18-id shape
FUN_1800535b0 writes); the Brance primary runs only as a 2-arg second-chance
that emits with a null analyser. Since FileX3FAnalyser never declines
(FUN_1800535b0 stamps the extent + id unconditionally at :63556-63558), the
Brance count-walk never contributes to a carved X3F extent. M-155's landing
(the S71 +0x4C walk, the count never read, the loop-exit salvage) is the
corpus-true semantics — this lane rebased onto it instead of seesawing, and
landed the dispatcher citation as the decisive audit-trail evidence (the
M-107 finding's Session-80 addendum; the census's "modern first" prose
recorded as an erratum). The class protocol held: semantic resolution for
code (corpus-first), stacked-history for docs.

## Resolution

**Status:** FIXED
**Fixed in commit:** `6d74ec2c` (the M-155 lane's convergent repair — the landed code) + this lane's rebase landing (the dispatcher-decode addendum, the N-217 finding, the worklog)
**Verified by:** `cmake --build build/linux-release` → builds clean at the post-rebase tree; `rg -l '^<<<<<<< HEAD' src/ diff-to-fix/ worklog/` → 0 hits; the X3F battery green on the landed semantics: `Carve.X3fExtentIsSecdPlusFixedTail` / `Carve.X3fNoSectionDirectorySalvagesWalkedPrefix` / `Carve.X3fGarbageCountIsIrrelevantToExtent` / `Carve.X3fSectionDirectoryWalkCarvesTrueEnd` / `Carve.X3fTornSalvagesTheWalkedPrefix`
**Resolution notes:** The build break is gone and the X3F authority question is settled by dispatch evidence rather than precedent: the secondary-first call order at :9862-9876 (`vtbl[+0x40]` = GetFallback → the secondary's slot-0 with the result object → emit on acceptance; the primary's `vtbl[+0x10]` as the 2-arg second chance). The prevention half stands from N-210 (ci-local step 10 greps the text tree) — the recurrence's root cause is process (a racing lane pushing without running the gate, plus a false green claim in the commit message), not a missing check; recorded for the next audit's liveness battery. The false "suite 1570/1570" claim in ed501caa's message is noted here for the record: the tree at that commit cannot compile.

---

## S82 convergence addendum (2026-10-29 — a THIRD lane's independent dispatcher verification; the census-prose erratum independently confirmed; the wrong-order composite withdrawn before push)

A third fix lane (the S82 lane) opened independently on the same red tree
(`04fb5e88`), reproduced the build failure first-hand, and initially
reached the OPPOSITE arbitration — a "modern-primary-first, legacy
fallback" composite — built on the census artifact's protocol prose
(`rawsearchdll--analyser-registry-census.md:37-43`, "two independent
validators must both fail before a match is discarded") and its own
first-hand reads of both walks (FUN_1800404a0's entry-table arithmetic and
FUN_1800535b0's fixed-tail salvage). That composite was implemented,
tested (five X3F tests green under it), and committed locally — and its
push was REJECTED by the remote having moved. On pulling the racing
lanes' records, this lane re-verified the decisive dispatcher site
first-hand rather than conceding by authority:

- **RAWSearchDLL :9849-9876, read directly this session:** the magic-key
  path resolves the registry node's analyser (`plVar7 = puVar12[5]`), then
  calls `vtbl[+0x40]` — the GetFallback slot. When it returns a non-null
  secondary, the dispatcher invokes THE SECONDARY FIRST
  (`(*secondary->vtable[0])(secondary, param_2, param_3, param_1[0x3c])`
  — the result-object-writing shape FUN_1800535b0 implements) and emits
  on its acceptance; the primary's `vtbl[+0x10]` runs only as the 2-arg
  second chance. The full-registry sweep at :9900-9935 carries the same
  shape.
- **Consequence, verified:** FUN_1800535b0 never declines (the size+id
  stamp is unconditional), so for X3F the Brance count-walk never
  contributes to a carved extent. The FileX3F semantics (SECd + 0x4C in
  all three found branches, the count never consumed, the loop-exit
  walked-prefix salvage) are the og's carved-X3F truth. The composite's
  primary-first order was inverted, and the census artifact's
  "modern-first" prose is an ERRATUM (confirmed independently of the
  S81 lane's record).

The wrong-order composite was withdrawn before it ever reached origin
(the rejected push, then the reset); the racing landing stands. What this
lane adds to the record: the third independent read of the dispatcher
(three lanes, same decode), the census-prose erratum confirmed twice,
and the process observation that the census artifact — a secondary
derivation — must not be cited as protocol authority against the
dispatcher's own code. The X3F battery re-verified green at the landed
tree by this lane's build (five tests, plus the full suite below).
