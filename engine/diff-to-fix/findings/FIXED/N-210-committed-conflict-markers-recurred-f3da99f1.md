# MINOR N-210 — the M-120 class recurred: four audit-trail files on main carry live committed conflict markers (the f3da99f1 merge residue) — the matrix, two FIXED findings, and the s51 prompt archive

**Severity:** MINOR (doc-truth/build class downgraded from M-120's MAJOR: all four files are markdown — no build surface breaks, unlike M-120's test_carve.cpp; the audit trail's structure is still corrupted at the marker sites)
**Module:** audit-trail integrity (the M-120 class)
**Parameters affected:** Q68 (audit-trail truth), the M-120 regression surface
**Corpus references:** none (a core-repo integrity defect, not a parity divergence; M-120 FIXED — `findings/FIXED/M-120-committed-conflict-markers-red-main.md` — is the class precedent)
**Core references:** found at core `c736e9b3` (HEAD at the S71 lane open); the markers landed with the `f3da99f1` merge (N-203's landing). The four files: `diff-to-fix/CROSS-REF-MATRIX.md` (one block: the closing-fix lane's rows vs the S54 lane's rows — both sections legitimate, stacked-history convention violated by the markers), `diff-to-fix/findings/FIXED/I-033-...md` (the session-42 addendum vs the Resolution block), `diff-to-fix/findings/FIXED/N-135-...md` (the session-42 liveness addendum vs the Resolution block), `worklog/session-diff-fix-s51-prompt.md` (two blocks: two racing S51 lanes' prompt variants). The M-120 finding's own markers are an intentional quote of the original defect and stand untouched.
**Status:** FIXED (found and fixed in the same S71 session, per §8's discovered-divergence rule)

## Summary

The `f3da99f1` merge committed unresolved conflict markers into four audit-trail files. Markdown-only (CI's conflict-marker compile check cannot see them), so main stayed "green" while the matrix — the deliverable's navigation spine — opened with a literal `<<<<<<< HEAD` and buried the closing-fix lane's rows inside a conflict block. The M-120 class's exact recurrence, one severity tier down (no build break this time).

## Gap

Falsifiable at `c736e9b3`: `grep -rn '^<<<<<<< ' diff-to-fix/ worklog/ | grep -v M-120` → 5 hits across 4 files (the matrix, I-033, N-135, s51-prompt). The matrix's first 13 lines are the conflict block itself.

## Impact

Audit-trail structural truth: the matrix renders as a conflict dump until line 14; the two FIXED findings' Resolution sections are entangled with their addenda; the prompt archive is interleaved. Future lanes resuming from the matrix would mis-read the board.

## Fix plan (executed this session)

1. Resolve every block by the stacked-history convention — both sides kept, only the marker lines removed (the closing-fix section stacks above the S54 section per newest-first; the Resolutions follow their addenda; both s51 variants preserved with explicit seam notes).
2. Post-check: no live markers outside M-120's intentional quote.
3. Prevention: the ci-local gate should carry the marker check for ALL text files (markdown included) — the check exists for compiled sources via `-Werror`'s version-control-conflict-marker detection only.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-210 landing (the S71 lane's opening repair)
**Verified by:** `python3 scripts/s71_repair_conflict_markers.py` — **[the S77 lane's phantom-citation repair: this script was NEVER committed to any commit on any branch (git log --all: zero hits) — the phantom-citation class, I-034's lesson; the reproducible method is the inline form: the resolution described below (both-sides-kept marker removal, 5 blocks across 4 files) IS the method; the post-check grep stands]** (run from a clean checkout: 5 blocks resolved across the 4 files; the post-check grep lists M-120's quote only) + `grep -rn '^<<<<<<< ' diff-to-fix/ worklog/ | grep -v M-120` → 0 hits at the post-repair tree + the matrix head re-read (both lane sections stack cleanly).
**Resolution notes:** Both sides of every conflict preserved per the house stacked-history convention — the matrix now opens with the closing-fix lane's rows (I-045/I-046/S-040 MATCH cells) followed by the S54 lane's section; I-033 and N-135 keep their session-42 addenda above their Resolution blocks; the s51 prompt keeps both racing lanes' variants with explicit seam annotations naming the repair. The M-120 finding's own markers (its evidence quote) deliberately untouched. The prevention half (a markdown-inclusive marker gate in ci-local.sh) lands with this commit — step 10 of ci-local.sh now greps the full text tree, so this class cannot re-enter silently even in markdown.
