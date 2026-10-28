# MAJOR M-118 — the suite is RED at HEAD: the N-115 test's typed-extension expectation was authored against the pre-S-049 lowercase model, and its landing (67de3276) rode a skipped build+test job and a failed lint gate

**Severity:** MAJOR
**Module:** carve (test-truth + the CI gate) / audit-trail (the lane's broken chain)
**Parameters affected:** A65 (test coverage — a wrong expectation ships as a red suite), A67-adjacent (the gate's own cleanliness), A69 (stub/TODO/landing-discipline scan — the lane's certify step never ran)
**Corpus references:** n/a for the defect itself (the governing contract is corpus-decoded and already landed: S-049's `FUN_140c44770` naming decode, DRWUI `:1878330-81` — `<PREFIX>_<%06d>.<UPPEREXT>`, the uppercase-ext default prefix, cited in `findings/FIXED/S-049-carved-file-naming-contract-ext-counter-content-prefixes-absent.md` §"What the reference does")
**Core references:** audited `869da779` — `tests/unit/test_carve.cpp:8428-8456` (`Carve.OoxmlRelsPathSplit`, landed at `67de3276`; the lowercase case rows `:8431-8434`, the failing check `:8453`); `src/core/carve/cr_carve.cpp:1051-1090` (`default_namer` — the S-049 uppercase contract, `ext_up` fold `:1072-1075`); the four clang-format violations (`include/cr/carve/cr_signatures.h:325`, `src/core/carve/cr_analysers.cpp:1370`, `src/core/carve/cr_registry.cpp:111`, `tests/unit/test_carve.cpp:7501`); the remote check-runs of `67de3276` (Build+test: skipped; Lint+parse gates: failure); the follow-up `869da779` whose own message admits "the retirement bookkeeping the broken chain step dropped"
**Status:** READ-ONLY FINDING — no fix applied

## Summary

At HEAD `869da779`, the certified test suite fails: `Carve.OoxmlRelsPathSplit`
asserts three times that a typed-OOXML carve's name ends in the LOWERCASE
extension (".docx"/".xlsx"/".pptx"), but the product — correctly, per the
S-049 universal naming contract — emits the UPPERCASE form
("DOCX_000001.DOCX"). The test was authored and landed with the N-115 fix
(`67de3276`) AFTER S-049 had already re-pinned the carve family to the
uppercase contract, and its lane landed with the CI build+test job SKIPPED
and the lint gate FAILED — so nothing caught it. Main is red: every
subsequent lane inherits a failing suite that masks new failures.

## What the reference does

The og names every carved file `<PREFIX>_<%06d>.<UPPERCASE-EXT>`
(S-049's decode, corpus-evidenced on the three real og sessions and the
DRWUI naming function `FUN_140c44770 :1878330-81`; the uppercase-ext
default prefix — `GZ_000001.GZ` — every observed format). A typed
DOCX/XLSX/PPTX carve follows the same contract: the format's own
uppercase extension in both the prefix and the tail.

## What our core does

`default_namer` (`src/core/carve/cr_carve.cpp:1051-1090`) implements the
S-049 contract exactly: `ext_up` folds the signature's extension to
uppercase (`:1072-1075`) and the name is `<ext_up>_<%06d>.<ext_up>`
(`:1081`). The N-115 typing pass (`67de3276`) types the OOXML trio via
the rels-path map — that half is correct and its own assertion
(`carve_format == c.fmt`) passes. The test's third assertion
(`tests/unit/test_carve.cpp:8453`) compares the name's last five bytes
against the LOWERCASE rows at `:8431-8434` and fails with the case delta
(-32) three times.

## Gap

Falsifiable (executed): build Release + run `cr_tests` at `869da779` →
`Carve.OoxmlRelsPathSplit` fails 3 assertions. Build the parent
`75dd2c96` → the test does not exist. The emitted name is
contract-correct; the expectation is not.

## Impact

The product code is contract-correct on both probed axes — no
user-observable wrong behavior in THIS defect. The damage is the gate:
main's suite is red, so (a) every fix lane that certifies "suite green"
from here is certifying against a broken baseline or must visibly
work around the failure; (b) a NEW genuine regression in the carve
family would be indistinguishable from the known-red test without
triage; (c) the lint violations mean the format gate is also red,
normalizing push-through. The trail's own discipline (every lane
certifies green before push) was already broken at this landing — the
follow-up bookkeeping commit's message says so in its title.

## Reproduction / verification

Executed (the strongest class, §6.1): full evidence at
`evidence/executions/s42-suite-red-at-head.md` — build/ctest commands,
the 1134-ran/1-failed/85-skipped tally, the verbatim ×3 assertion
failure, the parent-commit absence proof (`git show 75dd2c96:tests/unit/
test_carve.cpp | grep -c OoxmlRelsPathSplit` → 0; at `67de3276` → 1),
the remote check-runs API result (Build+test skipped / Lint+parse
failure), and the local clang-format 23.1.3 `--dry-run -Werror` violation
list over the window's four changed files.

## Fix plan (for the engineering team)

1. No architectural change. Two mechanical repairs + one process repair.
2. `tests/unit/test_carve.cpp:8431-8434` — the three case rows change to
   the S-049 uppercase form: `{".DOCX"}`, `{".XLSX"}`, `{".PPTX"}`
   (the `nm.compare(nm.size() - 5, 5, c.ext)` check at `:8453` then
   passes as written; while there, consider asserting the full
   `DOCX_000001.DOCX` shape — prefix AND counter AND tail — to pin the
   whole contract, not just the tail).
3. Run `clang-format -i` on the four violating files
   (`include/cr/carve/cr_signatures.h`, `src/core/carve/cr_analysers.cpp`,
   `src/core/carve/cr_registry.cpp`, `tests/unit/test_carve.cpp`) — the
   violations are line-local (comment wrap / whitespace / double trailing
   comment), no semantic churn.
4. Re-run the full suite + the lint gate; both must be green.
5. Process repair (the recurrence guard): the landing discipline that
   broke here is the same class N-135 item-3's guard script targets — a
   pre-push hook (or required CI check) that refuses a push whose
   build+test job is skipped. Until the repo enforces it, the worklog
   convention should require every fix lane to paste the suite tally
   INTO the commit message (the older lanes did; this one did not).
6. Acceptance criteria: `ctest` green at the fixing commit (0 failed,
   the same ran+skipped tally ± the test additions); `clang-format
   --dry-run -Werror` clean over src/include/tests; the fixing commit's
   message carries the tally.

## Tests to add

- The repaired `Carve.OoxmlRelsPathSplit` (fail-before = the current
  3-assertion red; pass-after = uppercase rows) — optionally strengthened
  to pin the full name shape per fix-plan step 2.
- No new fixtures needed — the existing `plant_zip_entries` harness
  already builds the three rels-path forms.

## Related findings

- S-049 (FIXED — the contract the test violates; its re-pinning wave
  missed this test because the test postdates the wave)
- N-115 (FIXED at `67de3276` — the typing pass itself is correct; this
  finding is its test's stale expectation + the broken gate, NOT a
  refutation of the landing)
- N-135 (the bookkeeping-truth family — the "broken chain" admission is
  the same lane's own record; the guard-script promotion there covers
  the process half here)

## Open questions

None — the failure, its root cause, and the gate state are all
first-hand verified (execution + code read + API check).
