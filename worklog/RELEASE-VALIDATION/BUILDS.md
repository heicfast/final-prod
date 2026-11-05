# BUILDS — gates §3/§4/§5 evidence: builds, tests, workflows all green

Written: 2026-11-05 (consolidation release session); run IDs link to
https://github.com/heicfast/final-prod/actions/runs/<id>.

## Lane status at the P6 entry (tree d166147b, code identical to a5cfa11f)

| Lane | Last green run | On what |
|---|---|---|
| contracts-ci | 42123593864 | a5cfa11f (validate.mjs phase 1+2 + filefilter freshness) |
| ui-ci | 42123593949 | a5cfa11f (the full TS gate + cargo check/clippy on the pinned actions) |
| ui-e2e | 42123593866 | a5cfa11f (14/14 Playwright) |
| engine-ci | 42123593814 (in flight at writing; engine tree unchanged since the green 42076322446 @bb69f3d5 — only the workflow's action pins differ) | a5cfa11f re-trigger via engine-ci.yml path |
| release | (tag-only — fires at P6) | v0.7.0 |
| forensic-repro / validation-evidence | dispatch-only by design (P5) | the P6 dispatches |

**Cross-lane confirmation**: db7f588a closed the red main (ui-ci
42122198423 GREEN, ui-e2e 42122198275 GREEN, contracts-ci 42122198375
GREEN — all three on the exact ui/engine code that a5cfa11f and
d166147b carry; the P5/d166 pushes changed only workflow YAML, forensic
tools, docs, and worklog).

## The P2 engine build (the heaviest lane, from the P2 record)

- FULL Linux build: **zero warnings** (gcc, -Werror-equivalent posture)
- ctest: **1737/1737 GREEN** in 174 s
- clang-format 18.1.8 clean (279 files); N-041 + direct-access +
  session-96 lints zero; doc_truth_gate green (92 exports / 40 errors /
  12 events vs tree)
- Windows MSVC /W4 /WX + ctest ride in engine-ci (core's own checks.yml
  posture, monorepo-adapted; the last full green: 42076322446)

## This session's local proofs (the red-main closure, db7f588a)

All on this sandbox, commands as recorded in worklog.md:

- TS: typecheck ✓ format ✓ eslint ✓ arch-lint(85 files) ✓ knip ✓ (with
  CI's KNIP_DISABLE_RAW_TRANSFER=1) jscpd ✓ (1 clone, CI parity) —
  **vitest 174/174** ✓ build+bundle ✓ (134.5/200, 318.9/500 KB gzip)
- Playwright: **14/14** ✓ (chromium 156 headless-shell, preview server,
  VITE_CONNECTOR=mock per the e2e lane's design)
- Rust (cross-checked to the Windows ABI surface):
  `cargo clippy --target x86_64-pc-windows-msvc -- -D warnings` ZERO;
  `cargo check --tests --target x86_64-pc-windows-msvc` ZERO — the
  windows-sys/ShellExecuteW paths a Linux host build never sees. (llvm-rc
  stub for tauri-winres's never-linked .res; check-only, no link.)

## Workflow hygiene (gate §5: green FOR THE RIGHT REASON)

- **No continue-on-error anywhere** (grep across the seven workflows:
  zero hits).
- **No masked failures**: every failing step fails its run; evidence
  uploads are `if: always()` (upload-only steps). The one historical
  masker (engine-ci's ghost-expression comments) was fixed at bb69f3d5.
- **Every `uses:` SHA-pinned** (36 refs, tags recorded in comments;
  a5cfa11f).
- **Paths-filtered lanes**: a component break cannot red another lane
  (verified: the ui-only commits never triggered engine-ci).
- **The dispatch-only pair** (forensic-repro, validation-evidence) is
  deliberate: both consume releases/measurements that only exist at
  dispatch time (the unborn-v0.7.0 trap documented in a5cfa11f).
- **No driver stage anywhere**: driver-ci.yml deleted at 318b9655;
  DRIVER-REMOVAL.md records the sweep (1,870 raw → 184 classified live →
  zero unclassified; all live hits are core's historical prose, absence
  tests, bannered audit history, or the mandated removal narratives).
