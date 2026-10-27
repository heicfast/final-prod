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

It runs every check CI runs, in the same order, with the same strictness:
format check, the dead-surface + driver-only architecture lints, the
warnings-as-errors build, the full unit + contract test suite, the repo
truth self-tests, dependency pinning, the doc link check, the secrets scan
of your diff, and the commit message convention. Every failure prints the
exact files and the fix command. **Green locally, then push.** If the gate
passes locally but CI's `local-mirror` job fails, that is a
runner-vs-developer environment difference — diagnose and close it, do not
just re-push.

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

CI (the validation lane, docs/CI-LOCAL.md's cost table): the
`local-mirror` job (your gate, verbatim), Linux + TSan + UBSan + the ONE
Windows MSVC Release build and test lane, the libFuzzer campaign
(`make -C fuzz`, `-Werror`) and the Win7 PE import gate
(`tools/verify_pe_um.py). CodeQL is present but deferred (`if: false` —
recorded, not silent). The correctness workflows (driver-install,
core-install, staged scenarios) consume **prebuilt releases only**
(docs/PREBUILT-ARTIFACTS.md) — nothing but the validation lane and the
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
