# Session-60 (S60) — the MINOR-first diff-to-fix execution lane

Saved at lane open (token redacted). The mandate: execute the roadmap in
`diff-to-fix/` — for every finding independently verify, fix, prove with
tests, and mark fixed in the audit trail with evidence. Two repos in scope
and only two: core_recovery (modified) + easeus-drw-research (read-only
corpus). User's session instruction: **fix priority to MINOR first**, one
by one, full context, aim for 20+ fixes; put fixed findings in
findings/FIXED/; update all docs and counts.

Key SOPs: verify → reproduce → classify → fix → re-review → test →
benchmark → document → push → loop. One finding, one verified fix, one
push. No decompiled-code pasting; no proprietary strings; cite the corpus
artifact. Every fix paired with a test that fails before and passes
after (pin tests for registration-only resolutions). Safety hardenings
preserved. Never force-push; pull frequently; keep main green. Token
never committed (this file redacted; clone URL in .git/config only).

Lane opened at core `98a5b139` (2026-10-29), corpus `be0cfe9`. The S55
audit lane's filings were verified at `9633d94f`; commits between are
build/trail repairs (win-lane MSVC /W4 layers, the M-139 HIK landing, the
N-191/N-207 renumber, the S-063/N-206/S-065 landings absorbed by merges).

Board at lane open (re-counted first-hand): 1 SEVERE · 23 MAJOR ·
24 MINOR · 2 INFO active + 4 tracked-open; findings/FIXED/ at 136 files.
N-107/N-127 (+ M-086/M-087/M-089) are driver-repo-resident — out of the
two-repo grant (OQ-S10-3's boundary) — not fixable this lane.
