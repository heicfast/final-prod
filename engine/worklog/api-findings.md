# API findings — Session-84 (behavior-adjacent, filed not fixed)

Findings that CANNOT be fixed in a coherence session without a behavior
change. Each carries exact evidence and the precise scope for the next
behavior session. (The phantom-export finding F1 was valid at the Phase-0
baseline `e8dcaba8` and was closed by the racing `a47419c7` landing
mid-session — recorded here for the history because the Phase-0 sheets
describe the baseline state.)

## F1 — phantom export `cr_ui_session_resume_last` [CLOSED mid-session]

- Baseline state: declared include/cr/cr_ui.h:927 with the full M-154
  pointed-resume contract (BAD_PARAMETER nulls; ERR_OPEN_FAILED for no
  sidecar / dead pointer / non-auto target; no shared state; any thread);
  ZERO references anywhere in src/**, tests/**, tools/**; never
  implemented on any remote branch (`git log --all -S` shows only the
  declaration commit b2bc1970 and this session's sheets). The DLL shipped
  90 exports while the header declared 91 and the docs counted 91.
- Closure: a47419c7 implemented it (src/ui/cr_ui.cpp:3699,
  `guarded(session_resume_last_impl, ...)` with the pointer-field
  disagreement guard at :3690-3694); cr_ui.cpp now carries 91
  `extern "C" CR_UI_API` definitions; tests pin the contract
  (test_cr_ui.cpp:5180-5222).
- Residual for the next session: nothing. Verified here post-landing.

## F2 — `enumerate_volumes` return code dropped in `print_disks`

- Evidence: src/app/rrecov.cpp:819 (`win::enumerate_volumes(vols);` —
  rc ignored) while the sibling `enumerate_disks` call at :811-815 IS
  checked (`!= OK` → "enumeration failed" + return 1, citing FB-K01:
  "enumeration failure is never exit-0 - scripts must not mistake total
  failure for 'no disks'").
- Why filed, not fixed: handling the rc changes observable output on the
  failure path (a new error line + exit 1 where today it prints
  "N disk(s), 0 volume(s)" and exits 0). That is a behavior change.
- Scope for next session: check `enumerate_volumes` rc in `print_disks`;
  decide the exit code (1 per FB-K01's own principle); add the
  Linux-runnable source pin; update docs/API.md's list-verb failure
  semantics if it documents them.

## F3 — the N-145 "every startup" journal probe does not exist

- Evidence: docs/PARITY-DELTAS.md:4226 ("probed at [every startup]"),
  diff-to-fix/modules/apply.md:107, diff-to-fix/CROSS-REF-MATRIX.md:21
  ("the every-startup warning naming the recovery path") vs the code:
  `apply::inflight_probe` is called ONLY inside the apply flow
  (src/app/rrecov.cpp:3368, within `apply --apply`) and the undo-apply
  path (:3612-3618). No main() startup probe exists. The journal's
  default filename is `rrecov-apply-inflight.jrn` (rrecov.cpp:3361) or
  `<in-path>.inflight` — none of the three doc sites' `rrecov-apply.ifj`.
- The landing commit d7856484's message claims "probes it at EVERY
  startup (the warning names the disk ordinal + the undo-apply recovery
  path)". The claim is not in the code.
- Why filed: adding a startup probe changes CLI startup behavior for
  every invocation (new file read + potential warning output).
- Scope for next session: either implement the startup probe per the
  N-145 finding's contract (the og's pendingMarkedDisk semantics — the
  warning at next start), or correct the finding/trail to the
  apply-scoped reality. The filename must be reconciled in the same
  decision (rrecov-apply.ifj vs rrecov-apply-inflight.jrn vs `<in>.inflight`).

## F4 — fuzz campaign drift (fuzz/Makefile)

- Evidence (build-surface.md §fuzz): `fuzz_reghive` is in the build set
  but no run target executes it; `run-long` omits `fuzz_image` from the
  campaign; the corpus bootstrap mkdir misses `corpus/thumbnail`.
- Why filed: changing the Makefile changes what the fuzz campaign
  verifies (a coverage/verification-policy decision, not a defect repair
  — each of the three may be deliberate).
- Scope: decide per target whether it belongs in `smoke`/`run-long`;
  align the corpus dirs; re-run the fuzz smoke campaign and record the
  verdict.

## F5 — `cr_ui_session_last_autosave_path`'s documented error code does not exist

- Evidence: include/cr/cr_ui.h:1021-1023 documents "ERR_NOT_FOUND when
  no live pointer names a present file" — the error enum
  (include/cr/cr_error.h) HAS NO ERR_NOT_FOUND member; the implementation
  returns ERR_RANGE_OUT_OF_BOUNDS (src/ui/cr_ui.cpp:3647) and the ABI
  test pins 0x50020004 (tests/ui_abi_check.c:592 region,
  test_cr_ui.cpp:5195-5222).
- The header COMMENT is the wrong side (doc-in-code drift); fixing the
  comment is a Phase 5 doc correction (no behavior), tracked as D3 in
  api-cli-mismatches.md. Filed here too because the underlying question —
  whether the pointer-absence class deserves its own error code rather
  than recycling RANGE_OUT_OF_BOUNDS — is a behavior-session decision.
- Scope: rename/correct the comment now (Phase 5); the error-code-family
  question (a dedicated pointer-absence code vs the current overload) is
  the next session's call, with the ABI version implications spelled out.

## F6 — D-52 "collision" in PARITY-DELTAS.md — NOT a defect (verified false)

- The Phase-0 census flagged D-52 as double-claimed; first-hand read
  shows :1684 `### D-52` and :4340 `### D-52 ADDENDUM` — an addendum to
  the SAME seat, not a second claimant. No action. (Recorded so the next
  session doesn't re-chase it; the census regex `^#{2,3} D-[0-9]+`
  over-matches addendum headings.)

## F7 — doc_truth_gate's D-uniqueness check is blind to `##`-level rows

- Evidence: tools/doc_truth_gate.py's D-row regex matches
  `^(?:###|\*\*)`-anchored rows only; the former `## D-152` rows at
  PARITY-DELTAS.md:4656/:4678 were invisible to it (the gate reported
  "D-numbering unique (153 rows)" while three rows claimed D-152).
- The D-152 collision was repaired by the racing window; the REGEX is
  still blind — the next `##`-level seat collision will pass the gate
  the same way.
- Scope: widen the gate's row regex to `^#{2,3}` (one-line change +
  re-derive; the count changes, so the gate's row-count output and any
  docs citing it must be refreshed together). This changes the gate's
  derivation semantics → deliberately filed rather than slipped into a
  doc commit; recommend the next coherence window takes it with the
  D-52 repair (F6) so one gate change covers both.

## Deferred from the Phase-0 agents (verified accurate, no action this session)

- CI action refs are unpinned mutable tags across all 4 workflows
  (build-surface.md §CI); pinning is mandated hygiene but every workflow
  is currently green for the right reason and the pin set (tags→SHAs)
  must be resolved via the API at pin time — a deliberate, reviewable
  change for the next window with CI time to re-verify. Not slipped into
  this session's repair stream while the racing cadence is hot.
- The ASan preset (linux-gcc) is run by no CI gate and no ci-local lane
  (build-surface.md §sanitizers) — a coverage decision, not a defect.
- GTest dual-sourcing (vcpkg + unpinned FetchContent) — packaging
  decision, filed in build-surface.md.
