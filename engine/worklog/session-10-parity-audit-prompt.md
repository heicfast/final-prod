# Session 10 — continuation parity audit prompt (token redacted)

Saved at session open per the standing SOP. Read twice before beginning.

**Mandate (the user's continuation addendum, verbatim):** "This is second
session of this work, first is already completed, now you have go look for
other things which session 1 missed and continue working and remove the fixed
things and go more deep, do more comparison on og-codebase and our core and
mark all remaining features , implementations, functions, as finding too."

Translated to the current trail state (9 audit sessions + fix waves have
landed since that phrasing was coined; each session re-reads it as: continue
from the standing board):

1. **Remove the fixed things** — re-verify every ACTIVE finding against the
   current HEAD; any finding whose defect the landed product code remediated
   moves to FIXED with a Resolution record (no silent drops).
2. **Go more deep** — hunt what ALL prior sessions missed: the named
   NEVER-AUDITED residuals (DRWService install lifecycle as a dedicated
   finding), the recorded-but-unfiled follow-ups in landing messages, the
   never-cited corpus residue, module-matrix currency after the landings.
3. **Mark all remaining features, implementations, functions as findings** —
   anything the corpus/og-codebase documents that no finding dispositions
   gets a finding (no "recorded outside an active finding" residue).

**Repositories (confirmed by fetch, this session):**
- Core (audited, read-only): https://github.com/heicfast/core_recovery
  — HEAD at session open: `84a1832` (trail); product-code tip `b92ebef`
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
  — `be0cfe9` (unchanged since session 8/9; CORRECTIONS at #9)
- GitHub token: REDACTED (env/git-config only, never committed)

**Read-only.** Only writes permitted: new files under `diff-to-fix/` in the
audited repository, and `worklog.md` entries (plus this worklog/ prompt file).
No source code, headers, tests, workflows, build files, or existing
documentation modified. No fix is applied.

**Standing board at session open:** 75 active findings (7 SEVERE · 28 MAJOR ·
35 MINOR · 5 INFO) + 250 FIXED + 2 tracked-open (OQ-S4-3 execution-bound,
OQ-S8-1 the CWindowsFS consumer question). Next free IDs: S-048+, M-103+,
N-128+, I-031+, A-012+.

**Verification hierarchy (unchanged):** executed comparison > decompiled
evidence (file+address+snippet) > corpus doc citation > independent RE probe.
Unresolvable → UNVERIFIED → tracked-open with a precise question. No claim
without evidence; false positives cost more than honest UNVERIFIEDs.

**Session method (per the standing parallel-agent SOP):** triage lane (the
active board vs HEAD), deep-comparison lanes (the named residuals + the
never-cited sweep), currency lane (module matrix / gate docs / counts), each
producing context sheets in scratch (not committed); the lead first-hand
re-verifies every load-bearing claim before anything is filed; commit+push
per coherent wave.
