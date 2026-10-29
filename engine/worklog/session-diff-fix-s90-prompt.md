# Session-90 (S90) fix-lane prompt (re-homed S86 -> S89 -> S90 — the racing windows took both names first) — token redacted

Saved at session open (T1 duty; re-homed S86 -> S89 -> S90 at the push rebases —
the racing window's lanes took S85/S86/S87/S88 while this lane's first
composite was in flight). The token below was redacted before saving;
it lives only in env/credential store, never committed, never logged.

- Core repo (modified): https://github.com/heicfast/core_recovery — opened at `32441353`, pulled through four racing windows to the landing head
- Corpus (read-only): https://github.com/heicfast/easeus-drw-research — `be0cfe9`
- Grant: exactly the two repos above; no other repo may be cloned, referenced, or looked at.
- Token: `ghp_[REDACTED — 36 chars]` (credential store at scripts/.git-credentials, outside both repos)

## Mandate (verbatim summary of the operative clauses)

Execute the roadmap in `core_recovery/diff-to-fix/`. For every finding — ACUTE,
SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN — independently verify the
finding, fix it in the codebase, prove the fix with tests and benchmarks, mark
it fixed in the audit trail with evidence. Do not trust the audit blindly;
every finding is re-verified against the research corpus and the running code
before any change. A false finding is corrected in the audit trail rather than
"fixed" by breaking good code.

SOP loop: Verify → Reproduce → Classify (CONFIRMED-DIVERGENCE /
ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → Fix →
Re-review → Test → Benchmark → Document → Push → Loop.

User override this session: "Fix all the audit findings (priority to minor
first) one by one, with full context and knowledge and update and put fixed
findings in fixed directory. Update all docs, counts and all properly. Aim for
atleast 20+ fixes in this session, dont stop before that."

Rules that bind every fix: re-implement from understanding (never paste
decompiled code); fix the cause not the symptom; every fix paired with a test
that fails before and passes after; safety hardenings preserved; no stubs, no
mocks, no TODOs, no fallbacks that mask failure; one finding, one verified
fix, one push; pull frequently, never force-push, leave main green; token
hygiene; commit messages state finding ID + divergence + fix + test + corpus
citation; the audit trail (`diff-to-fix/`) is the deliverable.

## Board at open (from SUMMARY.md + filesystem truth at `32441353`)

- 0 ACUTE · 0 SEVERE active
- 8 MAJOR active: M-075 M-078 M-086 M-087 M-089 M-101 M-105 M-107
  (M-086/M-087/M-089 are driver-repo-resident per OQ-S10-3 — outside the
  two-repo grant, untouchable this session, boundary re-affirmed at S10/S40/S48)
- 2 MINOR active: N-107 N-127 (driver-repo-resident, same boundary)
- 0 INFO active / 198 FIXED / 4 tracked-open (OQ-S10-1, OQ-S10-3, OQ-S4-3, OQ-S79-1)

In-scope fixable board at open: M-075, M-078, M-101, M-105, M-107 (9 standing
members: DWG WPG WPD PSP SIT APE EPS DXF SWF-RECT), the tracked-open items
where resolvable, plus NEW findings discovered during verification (the route
to 20+ fixes, matching prior sessions' pattern).

## What actually happened (the lane's honest history)

This lane independently decoded and implemented the WPD + WPG members
(locally green, 1666/1666, RED proofs first-hand) — the push rebase hit the
racing window's convergence: the S85 window had landed the SAME WPD semantics
plus a DEEPER WPG decode (the +0x20 zero-tail processor), plus DWG/PSP/DXF/
EPS/AI, M-075 (chkdsk), M-078 (BitLocker turn-off), M-159, N-223/N-224/N-225.
The WPD/WPG seats conceded per SA-9; the rebase aborted; the lane's unique
contributions rebuilt as the rider commit: (1) the WPD row-max rider (64 MiB →
the u32 field's own 4 GiB bound — the racing landing's own row still truncated
over-declaring documents at exactly the M-107 falsification's 64 MiB),
(2) the three-entry coverage pin, (3) the tier-1 decode corroboration that
closes the racing WPG landing's honest pending-residue. The lane continues
from the landing head on the remaining in-scope board: M-101, M-105, M-107's
SIT/APE members + DWG gen-3, the tracked-open items, and new discoveries.

Next free IDs at the rider: S-069+, M-157+, N-222+, I-049+, A-017+, D-163+.
