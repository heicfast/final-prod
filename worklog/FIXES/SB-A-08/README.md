# SB-A-08 fix evidence — the silent stop/pause/resume/cancel (P2)

## Defect (before)

Four destructive controls fired their mutation with no error surface:
- scan stop (scan-progress-page.tsx stop-confirm): `cancel.mutate(undefined,
  { onSuccess })` — onSuccess only.
- pause (:260) and resume (:271): `void pause.mutate()` / `void resume.mutate()`
  — no callbacks at all.
- recovery cancel (recovery-page.tsx CancelRecoveryButton): `cancel.mutate()`.
There is no global MutationCache onError (providers.tsx) — so a refused
engine call (device gone mid-drain, refusal states) was a complete no-op:
the confirm dialog had already closed, no toast, no state change. The scan
keeps running while the user believes it stopped — the exact
"broken-button feel" the session targets. The sibling flows already get
this right (device-lost resume :381, home beginScan, image start/stop,
saveSession — all carry onError).

## Fix (after)

`onError: (err) => toast.error(err.message)` added at the four call sites
(the codebase's own established pattern — verbatim from the device-lost
resume dialog at :381-383). No behavior change beyond surfacing the
failure: the mutation sequence, success paths, and invalidations are
untouched.

## Regression test (before-red / after-green)

`ui/tests/stop-refusal-toast.test.tsx` — drives the full stop path on the
mock connector's own live scan (lane-c pattern): strip Stop → confirm
dialog Stop → cancelScan spied to the refused-engine path → asserts the
error toast renders. Pre-fix: red (zero toasts — the silent no-op).
Post-fix: green. Typecheck/eslint/prettier green.

Captured runs (Linux sandbox, real vitest):
- BEFORE (page reverted via git stash): 1 failed (no toast).
- AFTER: 1 passed; tsc/eslint/prettier clean.
