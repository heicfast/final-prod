# AUDIT-UI — Session Self-Audit (v0.1.0-ui-mock)

Honest accounting against the session mandate.

## Claimed complete, with evidence

| Claim                            | Evidence                                                                                                                                      |
| -------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| Corpus read before code          | Reading pass logged in worklog; 4 gate docs pushed BEFORE scaffold (commit 37a7a86)                                                           |
| 63/63 screenshots VLM-extracted  | scripts/vlm-out (63 JSON), aggregate digest; SCREENSHOT-MANIFEST.md 63 rows                                                                   |
| Tokens have sources              | VISUAL-SPEC: every value cites themdark/QSS/config/atlas/VLM; conflicts resolved in writing                                                   |
| Mock Connector contract-complete | 9 domains + 2 event streams + scenario control; contract tests assert lifecycle/timing/injections                                             |
| All pages, all states            | UI-MAP state matrices implemented; walkthrough exercised home/config/scan/results/preview/recover; error/empty/partial surfaces code-verified |
| No UI-side backend logic         | connector called only via queries/ (arch-lint A1 + eslint zones + ast-grep, all green)                                                        |
| No dead controls                 | unavailable actions disabled with truthful reasons (open-location needs engine; mid-scan recovery gated by contract)                          |
| No TODO/placeholder comments     | grep clean (docs' roadmap items are tracked work, not stubs)                                                                                  |
| Tests green                      | 52/52 Vitest; Playwright spec written + config wired; CI workflows complete                                                                   |
| Budgets                          | bundle gate PASS; perf budgets measured where possible (filter <200 ms asserted)                                                              |

## Honest limitations (all tracked, none hidden)

1. **Windows-only measurements pending**: DPI ladder, cold start, scripted
   60 fps, NSIS build — the local sandbox is Linux; workflows are written
   but the first Windows run happens on GitHub Actions. VISUAL-ACCEPTANCE
   marks these PARTIAL.
2. **Rust uncompiled locally**: no cargo in the sandbox; `cargo check` +
   clippy run in CI. The shell is deliberately 3-commands thin to keep that
   risk minimal.
3. **Playwright not executed here** (no browsers for the test runner in the
   sandbox); equivalent golden-path verification WAS executed live via
   agent-browser with zero page errors and 8 evidence screenshots, and the
   spec runs in CI.
4. **Three PARTIAL items need the real engine or more sessions**: dark/HC
   systematic screenshot diffs, axe-in-CI, RTL/pseudo-length sweeps.
5. **Interim icons**: Lucide base per v1 policy, replacement planned
   (INTERIM-ASSETS.md).

## Divergences from the reference (documented, deliberate)

- No monetization/retention/license-gate surfaces (UI-MAP §0).
- No AI assistant/recommend (no contract behind it — truthful absence).
- No cloud/NAS/WeChat targets (engine session will expose them).
- Original strings and branding throughout.
