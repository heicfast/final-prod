# MINOR N-135 — the fix-wave bookkeeping truth batch: one cited doc site missed while its Resolution claims both fixed (N-103's REFERENCE-API-GAP:376), one gap-board self-contradiction (N-118's :379 vs :225), five phantom SHAs in landing records, and the FIXED-LEDGER's own gaps (missing rows, a decompile-function mis-paste as a fix SHA, the re-created FIXED/ dir)

**Severity:** MINOR
**Module:** audit-trail / board truth (diff-to-fix gate docs, FIXED-LEDGER, docs/REFERENCE-API-GAP.md)
**Parameters affected:** A68 (documentation accuracy), A69-adjacent (resolution-record truth)
**Corpus references:** n/a — audit-trail truth finding (the og-side facts it mis-records are carried by the named findings).
**Core references:** audited `2beb555` — (1) `docs/REFERENCE-API-GAP.md:376` (the second-table R-28 row, verbatim "preview_read (no dedicated op) | PARTIAL | no async thumbnail fetch protocol" — N-103's fix plan explicitly named sites "222/376"; the Resolution claims "the two R-28 rows now carry the engine-landed state"; :376 was in no hunk of e4f3184); (2) `docs/REFERENCE-API-GAP.md:379` (the second-table R-31 row still "PARTIAL | per-format carve toggles, usefullostfiles whitelist, display priority missing" vs the primary :225 "PRESENT" — the same document self-contradicts); (3) phantom SHAs cited as fix commits/records: `edee513` (M-085's Resolution; the real fix is `fb7cf30`), `771e2ec` (M-090's Resolution + FIXED-LEDGER:6; the real fix is `9daa787`), `dd0ffd5` (the M-071 narrative; the real fix is `ce2405a`), `95122bd` (M-082's retire message + SUMMARY:141; the file is recoverable at `f8bb4d2^`), `954c386` (the ledger's doc-batch cite; the real commit is `e4f3184`) — all five verified absent from the object store (`git cat-file -t` fails; they are pre-rebase lane SHAs that died at rebase); (4) `diff-to-fix/FIXED-LEDGER.md:129` (the M-090 row's fix-commit column carries `180053de0` — the DECOMPILE FUNCTION NAME, not a SHA); (5) FIXED-LEDGER has no rows for M-082/M-080/M-085/S-043 despite their retire/move commits claiming ledger conformance, and `findings/FIXED/` was re-created holding those four files against the ledger's own no-re-create convention; (6) `docs/ABI-CONTRACT.md:525` §7g's cite range `916-944` overshoots the W1b block's actual end (:935 — the decl at :933-935 is findable but the range is 9 lines loose); (7) `src/core/preview/cr_thumbnail.cpp:11-14` file-top format matrix still lists only uncompressed/PackBits/Deflate (the header's matrix gained LZW; the .cpp's did not).
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (session-11B, 2026-10-29):** renumbered **N-129 → N-135** — the N-128→N-129 renumber below landed this file onto an ID lane-B had already taken (`0e279e43` 17:32:28Z pushed `N-129-parity-og-core-inventory-numeric-defects.md`; this file's `f6c00177` is its git DESCENDANT, 17:33:36Z — the object-graph proof is in the session-11B worklog entry). Per the SA-9 earlier-push-wins rule lane-B keeps N-129; this file takes N-135 (the next free ID). All live cross-references updated.
**Prior audit check:** newly discovered (renumbered N-128→N-129 at the push-time reconciliation: the SA-10 lane's CAPABILITY-MATRIX N-128 pushed first and keeps the ID per the SA-9 rule) — the N-127 precedent (a truthfulness batch) extended to the fix-wave's own bookkeeping; N-103/N-118's landings are the immediate cause (their missed sites are items 1-2), the ledger/SHA items are the purge-convention wave's residue.
**NOTE (session-39 addendum, at the convergent-repair rebase):** items 3-4 of this finding's fix plan were partially repaired by the b5da834 'repair(trail)' commit (the phantom SHAs and the missed R-28:376 site) — that repair is what left the FIXED-LEDGER merge conflict markers (resolved at session-39 T1c over the session-11B repair); the finding is re-verified item-by-item at the current tree when the session-39 fix lane reaches it.

## Summary

The fast-moving fix waves left the board's bookkeeping divergent from its own tree: one finding's explicitly-cited second doc site was not fixed while its Resolution claims both were; the gap-board's two tables contradict each other on R-31; five landing records cite pre-rebase SHAs that do not exist in the repository (making the recorded fix commits untraceable as written); and the FIXED-LEDGER — the purge convention's navigable index — is missing rows, carries a decompile-function name where a SHA belongs, and its FIXED/ no-re-create rule is already violated by four re-added files.

## What the reference does

n/a (audit-trail finding). The governing conventions: the FIXED-LEDGER purge convention (every resolved finding gets a ledger row; the working tree carries only the active board; files recoverable via history), and the resolution-record discipline (claims must match the tree — the same standard the findings hold the product docs to).

## What our core does

As cited: the seven items stand at HEAD `2beb555`, each verified first-hand this session (the lead re-read :376/:379, ran `git cat-file -t` on all five phantom SHAs, and re-read the ledger row).

## Gap

Falsifiable: read the cited lines / run the cited git commands. Each item is directly checkable in minutes.

## Impact

Roadmap trust: future sessions navigating by the ledger and the Resolutions hit dead SHAs and contradictory rows — exactly the double-work and false-audit risk the doc-truth findings exist to prevent. The N-103 case is a resolution-record overclaim (the class the board treats as a finding when the product docs do it).

## Reproduction / verification

All items verified first-hand this session (the landing-parity verification wave: three parallel agents + the lead's spot-checks). Commands: `sed -n '374,381p' docs/REFERENCE-API-GAP.md`; `for s in edee513 771e2ec dd0ffd5 95122bd 954c386; do git cat-file -t $s; done` (all fail); `grep -n "M-090" diff-to-fix/FIXED-LEDGER.md`; `ls diff-to-fix/findings/FIXED/`.

## Fix plan (for the engineering team)

1. `docs/REFERENCE-API-GAP.md:376` — bring the second-table R-28 row into agreement with :222 (engine PRESENT; M-102 ABI handoff); re-read N-103's Resolution "two R-28 rows" claim once corrected.
2. `:379` — same for R-31 (agree with :225's PRESENT).
3. The phantom-SHA cluster — replace with the real post-rebase SHAs: M-085 → `fb7cf30`; M-090 → `9daa787` (Resolution + ledger:6 + the :129 row's `180053de0` mis-paste); M-071 → `ce2405a`; M-082 → `1f71488` (retire narrative + SUMMARY:141; recovery cite `f8bb4d2^`); the doc batch → `e4f3184`. Rule going forward: resolution records cite SHAs only after the push succeeds (post-rebase), or cite by title + first-line match.
4. FIXED-LEDGER — add the missing rows (M-082, M-080, M-085, S-043); either retire the four re-created FIXED/ files to history (the convention) or amend the convention note — one truth, not both.
5. `docs/ABI-CONTRACT.md:525` — tighten §7g's range to the actual block end.
6. `src/core/preview/cr_thumbnail.cpp:11-14` — add LZW to the file-top matrix (mechanical; rides the next preview commit).
7. Acceptance: every SHA cited in diff-to-fix/ resolves; the gap-board's two tables agree; the ledger's row set equals the resolved set.

## Tests to add

None (doc/ledger fixes). Optional guard: a scripts/ check that every `[0-9a-f]{7,40}` token in diff-to-fix/*.md and FIXED-LEDGER.md resolves via `git cat-file -t` (the phantom-SHA class becomes mechanically impossible).

## Related findings

N-103 (whose landing missed :376 — the overclaim), N-118 (whose landing left :379), M-080/M-105 (the doctrine mis-pin family), N-127 (the truthfulness-batch precedent), M-088 (the product-docs SHA-drift family — same class, product side).

## Open questions

None.
