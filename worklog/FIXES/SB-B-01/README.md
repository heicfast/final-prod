# SB-B-01 fix evidence — "Save and exit" could never succeed (P1)

## Defect (before)

The ExitGuard's mid-scan "Save and exit" called `session_save` directly while
the engine scan was live. The real bridge refuses while busy
(`sessions.rs:100-106`: SCAN_NOT_RUNNING, "save the session after the scan
finishes (or cancel it first)") — every guard-gated status (running/paused/
stopping...) implies busy, so the button's primary action always failed with
an error toast and the window stayed open. The user's only exits were "Keep
scanning" or "Exit without saving" (losing everything since the last save).
The mock connector allows any-state saves, so the browser-mode CI lane could
never see the refusal (mock-divergence class — Agent B's systemic finding).

## Why the refusal is REAL (root cause analysis — not removed)

The engine's `scan_save_session_impl` (cr_ui.cpp:2987) gates only on
`has_ctrl` — BUT the serializer it funnels into, `session::serialize_tree`
(cr_records.cpp:89), walks the live result tree WITHOUT the tree mutex
(`ResultTree` mutations run under `mutex_`; serialize_tree is a free function
with no access). A mid-run explicit save would race the scan thread's
`add_file` — undefined behavior, torn reads. The bridge refusal is the
protection. (The engine's mid-run-safe reader is `capture_snapshot` — the
snapshot path; serializing a Snapshot to a session file is NOT implemented.)

The engine's own partial-session mechanism is the STOP TAIL (S-057/W1c,
cr_scan.cpp autosave_if_due_ — "called from BOTH the completion path and the
user-stop (cancel) path"; the og's stop→SaveScan parity: a stopped scan's
partial results persist exactly like a finished run's).

## Fix (after)

The ExitGuard's "Save and exit" now runs the honest engine-aligned flow:
read the engine state → if live: `cancelScan` (the engine's stop path, which
also persists the partial session through autosave_if_due_) → poll the
ENGINE's own state until the terminal latch (bounded by a 120 s drain
deadline; the timeout surfaces a truthful localized error, never a hang or a
fake completion — F-B-16's in-flight-read limit is documented in the code) →
the REAL named `saveSession` (busy now false; has_ctrl true) → close the
window. A refused stop or save surfaces the error and stays open — no silent
no-op. The scanId is captured at click time (the store's `active` clears
when the scan stops; the save must still address the SAME scan — the
mutation-hook closures re-render to null and were a latent wrong-target
bug, replaced by direct connector calls + the hooks' own cache effects
mirrored in sequence).

## Regression tests (before-red / after-green)

`ui/tests/exit-guard.test.tsx`:
- "Save and exit stops the live scan FIRST..." — pins cancelScan called
  BEFORE saveSession (invocationCallOrder), the terminal state observed at
  save time, then destroy. RED before the fix (cancelScan never called —
  the old code saved directly into the refusal), GREEN after.
- "a refused stop surfaces the error and stays open — no silent no-op" —
  cancelScan rejects → toast path, no save, no destroy.

Captured runs (this session, Linux sandbox, real vitest):
- BEFORE (worktree with exit-guard.tsx reverted): 2 failed (the two pins
  above) | 3 passed.
- AFTER: exit-guard*.test.tsx 9/9 green; full suite 32 files / 233 tests
  green (232 existing + the i18n key added to both dictionaries — the
  i18n parity gate verified it); typecheck/eslint/prettier green.

Windows runtime note: the real bridge path (cancel → drain → save) is
exercised end-to-end by the bridge-integration/ui-e2e CI lanes on the
pushed commit; a manual mid-scan X-click check remains a Windows smoke item
(OPEN-ITEMS).
