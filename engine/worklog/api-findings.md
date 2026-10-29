# worklog/api-findings.md — ABI-level findings for the next behavior session (Session-84, T4)

As of `78c60e89` (2026-10-30). Everything here is **verified and filed, not fixed** — each
item requires a runtime-observable change or a test-surface addition beyond this session's
no-behavior-change mandate (wiring completions with proving tests excepted, per §7).
Scope notes are written for the next session's planner.

## AF-1 — CLI exit-code taxonomy inconsistencies (HIGH, user-visible)

- `recover` exits 1 on source-open failure; `bench`/`fix`/`apply` exit 2 on the same
  class (api-cli-mismatches.md B-1). Scripts keyed on exit codes misclassify recover's
  open failures as usage errors.
- The `-h` usage block omits 7 parsed scan flags and exit code 5 (B-3); the disconnect
  hint references a nonexistent `rrecov resume` subcommand (B-2, rrecov.cpp:2090).
- **Scope:** one behavior session decides the canonical exit map (the header docstring at
  rrecov.cpp:49-60 is now the documented truth for 0-5), fixes the three string surfaces
  (`-h` block, hint, any per-verb inconsistencies), and lands pin tests per verb asserting
  the map. The docstring fix (`78c60e89`) already documents 5; the code strings still
  disagree with it in the `-h` block only.

## AF-2 — Two ABI exports have zero consumer coverage (MEDIUM, gate-5 gap)

`cr_ui_open_volume_with_key` and `cr_ui_unlock_volume_with_dictionary` (the BitLocker
unlock pair, declared in cr_ui.h, implemented in cr_ui.cpp) are called by NO consumer:
not `tests/ui_abi_check.c` (76/91), not `.github/scripts/api_harness.c` (47/91), not
`cr_ui_tests` (84/91). Union coverage 89/91 — computed at `78c60e89` by intersecting the
declared export list with call sites across all three consumers.
**Scope:** two consumer tests (ui_abi_check.c or cr_ui_tests) exercising both functions'
success and refusal arms; this is test-surface addition, no product change — it may be
landed by a coherence session, but it needs fixture design (an encrypted-volume image or
a unit-shaped refusal harness), so it is filed rather than rushed here.

## AF-3 — The doc-truth oracle's error-code regex has phantom matches (MEDIUM, tooling)

`tools/doc_truth_gate.py` counts 45 error codes; the enum carries 43 nonzero + OK. The
regex also matches the two og-facility literals quoted in the cr_error.h:6-7 comment.
Five doc sites repeat the 45 figure. Fixing the regex changes what "green" means, and the
doc count corrections must land in the same commit — a coordinated doc+tooling change
that belongs in the T17 batch with the recount (api-cli-mismatches.md D-4), not a silent
mid-session oracle edit.

## AF-4 — ABI event-type emission split (LOW, informational)

The core-side event enum emits only 8 of the 12 `CR_EVT_*` types; types 8, 9, 11, 12 are
raised shim-side (api-surface sheet §4/§8). This is by design (the shim synthesizes
host-level events the core does not know about) but the split is documented nowhere in
ABI-CONTRACT.md's event section. **Scope:** a two-line contract clarification in the T17
doc pass; no code change.

## AF-5 — Struct-layout drift detection has no static pin (LOW)

The 30 ABI structs carry no packing pragmas and no size/version fields (api-surface
§3 — gcc -std=c11 size probes at Phase 0). Layout is C11-natural on the x64 ABI; the
prompt's §2 offset-assertion consumer does not exist. A `static_assert` battery in
ui_abi_check.c (offsetof/sizeof for every struct field) would pin the layout against
future field-type edits at zero runtime cost. **Scope:** test-surface addition, next
session; harmless to add but out of this session's minimal-fix discipline.

## AF-6 — Deferred from the sheets (cross-referenced, not re-derived here)

The api-surface sheet's A2-A5 doc-drift items (CORE-API-CURRENT "(b) drift", the 92/94
line counts) are consolidated into api-cli-mismatches.md D-4/D-5 and the T17 batch. No
additional ABI-level behavior gap was found by this lane beyond the above.

## Verification trail for this file

- Coverage computation: declared-list vs consumer call-site intersection (script output
  preserved in the session transcript; re-derivable with
  `grep -o 'CR_UI_API\s\+[^;]*cr_ui_\w*' include/cr/cr_ui.h` intersected with
  `grep -o 'cr_ui_[a-z_0-9]*\s*('` over the three consumers).
- All file:line citations re-read at `78c60e89`.
