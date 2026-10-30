# Contributing to core_recovery

This repo runs a **zero-behavior-change discipline** for maintenance
work: fixes must not alter observable semantics, tests must not be
weakened, and warning gates must never be relaxed to get green — the
code is fixed instead. Evidence-citing docs (`diff-to-fix/`) are the
audit spine; when you change behavior deliberately, register the
divergence in `docs/PARITY-DELTAS.md` first.

## The local gate — never push a red commit

**Run the single entry point before every push** (CI's first job runs the
same script on the runner image — the local-mirror rule, docs/CI-LOCAL.md):

```
./ci-local.sh          # Unix (add --san / --fuzz / --all for the CI matrix lanes)
.\ci-local.ps1         # Windows (the exact script the CI local-mirror job runs)
```

It runs the local gate — format check, the dead-surface + direct-access
architecture lints, the warnings-as-errors build, the full unit + contract
test suite, the repo truth self-tests, dependency pinning, the doc link
check, the secrets scan of your diff, and the commit message convention.
Every failure prints the exact files and the fix command. **Green locally,
then push.** (CI-shape note, Session-84: the pre-consolidation CI this
paragraph described — the `local-mirror` job, the Linux/TSan/UBSan/fuzz
matrix lanes, CodeQL `if: false`, the historical driver-install /
core-install correctness workflows — no longer exists. The CI today is the 4-lane
windows-2022 set: **Checks** (push/PR + the release's workflow_call gate:
lint + MSVC Release build-and-test; doc-only paths are ignored by design),
**Installer gates**, **Release**, and **Test & Benchmark**
(workflow_dispatch). The sanitizer and fuzz lanes are local-only
(`ci-local.sh --san` / `--fuzz`); see docs/CI-LOCAL.md's M-088 CI-shape
note and docs/CI-CURRENT.md for the live inventory.)

Install the hooks (one-time):

```
git config core.hooksPath .githooks
```

- `pre-commit` runs the fast subset (staged secrets + staged-file format —
  seconds).
- `pre-push` runs the full gate.

**Bypassing the hooks with `--no-verify` is a contributor violation.** The
remote never accommodates a red push; CI does not retry your way around the
gate. The full component→CI mapping is auditable in docs/CI-LOCAL.md.

## Build lanes

| Lane | Command | Notes |
|---|---|---|
| Linux dev | `cmake --preset linux-release` | pure core + tests, gcc `-Wall -Wextra -Wpedantic -Werror -Wshadow` |
| Linux ASan (CI parity) | `cmake --preset linux-gcc` then `cmake --build --preset linux-gcc` | the lane CI runs; sanitizer build |
| UBSan | `cmake --preset linux-ubsan` | |
| Windows (production) | `cmake --preset win-x64-release` | vcpkg manifest (zlib/lz4/zstd); MSVC `/W4 /WX /permissive-`; the win layer is CI-built only — on Linux it is parse-checked instead (below) |

Deps on Linux: zlib1g-dev, liblz4-dev, libzstd-dev (GTest is
FetchContent'd when absent). See `docs/build.md` for details.

## Tests and gates (all green — via the local entry point above)

```
ctest --preset linux-gcc         # full suite, ASan
bash tools/win_syntax_check.sh   # Windows-only sources parse gate (also a ctest)
```

CI (docs/CI-CURRENT.md is the live inventory): the **Checks** workflow
(push/PR, and the release's workflow_call validation gate) runs the lint
job + the MSVC Release build-and-test job with the Win7 PE import gate
(`tools/verify_pe_um.py`) on every non-doc push; **Installer gates** and
**Release** run their own windows-2022 lanes; **Test & Benchmark**
(workflow_dispatch) consumes **prebuilt releases only**
(docs/PREBUILT-ARTIFACTS.md). The sanitizer and fuzz campaigns are
local-gate lanes, not CI lanes (the M-088 consolidation; see
docs/CI-LOCAL.md). Nothing but the
release workflow ever builds from source.

Local sanity for an edit: run the local gate. Formatting must match
clang-format — do not blanket-reformat unrelated lines. No warning
suppression without a docs/BUILD-SUPPRESSIONS.md row (currently zero
entries).

## The audit trail

- `diff-to-fix/SUMMARY.md` is **the board** — every finding's
  disposition. New sessions append a delta section; never rewrite
  history.
- Fixes land with a finding-file Resolution (fail-before/pass-after
  evidence where applicable); deliberate divergences get a
  `PARITY-DELTAS` row with rationale.
- The corpus (`easeus-drw-research`) is ground truth for the reference
  product's behavior; `docs/ground-truth-contracts.md` is the
  anti-hallucination anchor. Verify claimed corpus facts at the cited
  line numbers before relying on them.

## Releases

Tag pushes (`vX.Y.Z`) run the combined release workflow — the sole source
of builds, gated by the full validation suite (docs/RELEASE.md). The
cadence policy: **release on every green milestone**; the current release
is the substrate CI consumes. After a landed fix-group, tag and let the
workflow produce the release — never build artifacts by hand.
