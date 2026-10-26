# Contributing to core_recovery

This repo runs a **zero-behavior-change discipline** for maintenance
work: fixes must not alter observable semantics, tests must not be
weakened, and warning gates must never be relaxed to get green — the
code is fixed instead. Evidence-citing docs (`diff-to-fix/`) are the
audit spine; when you change behavior deliberately, register the
divergence in `docs/PARITY-DELTAS.md` first.

## Build lanes

| Lane | Command | Notes |
|---|---|---|
| Linux dev | `cmake --preset linux-release` | pure core + tests, gcc `-Wall -Wextra -Wpedantic -Werror -Wshadow` |
| Linux ASan (CI parity) | `cmake --preset linux-gcc` then `cmake --build --preset linux-gcc` | the lane CI runs; sanitizer build |
| UBSan | `cmake --preset linux-ubsan` | |
| Windows (production) | `cmake --preset win-x64-release` | vcpkg manifest (zlib/lz4/zstd); MSVC `/W4 /WX /permissive-`; the win layer is CI-built only — on Linux it is parse-checked instead (below) |

Deps on Linux: zlib1g-dev, liblz4-dev, libzstd-dev (GTest is
FetchContent'd when absent). See `docs/build.md` for details.

## Tests and gates (all must be green before a push)

```
ctest --preset linux-gcc      # full suite, ASan (926 tests at HEAD)
bash tools/win_syntax_check.sh   # Windows-only sources parse gate (also runs as a ctest)
```

CI additionally runs, per push: clang-format-18
(`--dry-run --Werror` over src/include/tests — format your edits),
linux + linux-tsan + linux-ubsan + the windows MSVC build and test
lane, the libFuzzer campaign (`make -C fuzz`, now `-Werror`) and the
Win7 PE import gate (`tools/verify_pe_um.py`). CodeQL is present but
deferred (`if: false` — recorded, not silent).

Local sanity for an edit: configure+build the lane you touched, run
`ctest --preset linux-gcc`, and clang-format the files you changed.
Formatting must match clang-format-18 — do not blanket-reformat
unrelated lines.

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

## Commit style

Evidence-citing subject lines in the imperative, lowercase family
(`fix(warnings): the clang fuzz lane's 48-warning backlog zeroed`),
with the body explaining the *why* and the proof (what was measured,
what test pins the behavior, what failed before). Look at recent
`git log` before writing yours.
