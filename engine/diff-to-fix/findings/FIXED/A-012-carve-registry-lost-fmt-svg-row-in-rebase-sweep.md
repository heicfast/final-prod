# ACUTE A-012 — the carve registry lost its FMT_SVG row in the 81ee9775 rebase-window conflict sweep: kTable 119 rows vs FMT_COUNT 120 — the F8 gate rejects EVERY carve_slice call with ERR_BAD_PARAMETER, hard-downing the carve stage on all of main (DISCOVERED during this session's S-051 verification)

**Severity:** ACUTE
**Module:** carve (the signature registry — the load-bearing table)
**Parameters affected:** A25 (the engine's dispatch table is structurally broken), A42 (every scan with carve enabled fails), P42 (the public surface's carve stage), the F8 registry invariant (cr_signatures.h)
**Corpus references:** none needed — this is a core-internal regression against the project's own invariant (the F8 registry gate, `validate_registry`, S-017's landing; the table's row-vs-enum count contract `registry_size() == FMT_COUNT`); the lost row is M-093's FMT_SVG entry (`ca701711` — the direct "<svg" magic path), recovered verbatim from that landing
**Core references:** the regression commit `81ee9775` ("repair(carve): sweep the rebase-window conflict markers from cr_registry.cpp") — its diff shows the conflict resolved by KEEPING the HEAD-side MOBI declaration+row and DELETING the other-side `{FMT_SVG, "svg", {'<','s','v','g'}, 4, 0, 0, 0x100000ull, 620, validate_xmlsvg, "SVG image"}` row (the two sides were TWO DIFFERENT rows, not a duplicate); post-sweep state verified first-hand this session: `registry_size() == 119` vs `FMT_COUNT == 120`, `validate_registry(nullptr) == 2` (the size mismatch + the mobi row's id-vs-index mismatch at index 118/id 119); the pre-existing pin `CarveEngine.RegistryF8Gate` (test_carve.cpp:516-517 — `EXPECT_EQ(validate_registry(&bad), 0u)` + `EXPECT_EQ(registry_size(), FMT_COUNT)`) FAILS on main; the downstream hard-down: `carve_slice` (cr_carve.cpp:957-960) returns ERR_BAD_PARAMETER at the F8 gate for EVERY call, so every default-configured scan (quick+deep+carve) exits 0x50030003 and the whole carve stage is dead
**Status:** FIXED (CONVERGENT — the parallel S11 lane's repair `e4273f2b` pushed first and stands; this lane's independent discovery, fail-before execution and verification recorded here)

## Summary

The 81ee9775 rebase-window conflict sweep deleted the FMT_SVG registry row while resolving the merge between the M-093 XML/SVG landing and the M-096 MOBI landing. The two conflict sides carried DIFFERENT rows (HEAD: the reformatted MOBI entry; other: the one-line FMT_SVG entry); the sweep kept one and dropped the other as if they were duplicates. The loss broke the registry's structural invariant — kTable now holds 119 rows against FMT_COUNT's 120 — which trips the F8 gate: `carve_slice` refuses every call with ERR_BAD_PARAMETER, the `CarveEngine.RegistryF8Gate` pin is red, and every default scan on main (quick+deep+carve) fails at the carve stage. Carving — a core product capability — was hard-down on main.

## What the reference does

N/A (core-internal regression): the corpus governs the FORMATS the registry must carry (MOBI per M-096: raw_signs.json + the og's RawConfig "MOBI" toggle; the SVG family per M-093: the probe-stamped id split — `re-workspace` ghidra-RAWSearchDLL's xml/svg analyser decodes). The invariant broken is the project's own: the registry table must hold exactly one row per `FormatId` enum value, in id order (the F8 gate, `validate_registry`, S-017).

## What our core does (post-regression)

`kTable` (cr_registry.cpp) lost the FMT_SVG row: 119 entries against the enum's 120 values, the mobi entry displaced to index 118 with id 119. Every `carve_slice` call fails its first structural gate (`validate_registry(nullptr) != 0` → ERR_BAD_PARAMETER). Reproduced first-hand this session: the S-051 evidence probe (evidence/executions/s10-wholedisk-probe.cpp) on the two-volume fixture returned rc=0x50030003 with `carved=0` on RUN 1 AND RUN 2; the regcheck harness printed `rejects=2 of 119`; gdb-free instrumentation confirmed the failure at carve_slice's early-return-2. The pre-existing registry pin test is RED at `bcf31316` (main at this session's open): `CarveEngine.RegistryF8Gate` — 1 FAILED TEST (executed this session as the fail-before proof).

## Gap

Falsifiable (and executed): `validate_registry(nullptr) != 0` on main at bcf31316; `CarveEngine.RegistryF8Gate` RED; every `scan` with default carve config exits 0x50030003 after a successful quick+deep pass (the carve stage's failure propagates through run()'s stage gating). No user data destroyed (the scan refuses honestly) — which is what holds this at ACUTE-with-honest-failure rather than a data-loss class: the hard-down of a core capability on main, caught between two green-claiming parallel lanes.

## Impact

Every consumer that scans with carving (the CLI's default, the ABI's default-configured scan) fails the run on current main. The parallel lanes' own "suite green" claims bracketed the sweep (their later claims cite suite counts from runs that predate 81ee9775's push) — the pushed regression sat on main with a red suite until this session's first fix pass.

## Reproduction / verification

- Fail-before (executed this session, first-hand): `CarveEngine.RegistryF8Gate` at `bcf31316` → RED ("1 FAILED TEST"); `/tmp/regcheck` (validate_registry over the linked core) → `rejects=2 of 119` with the mobi entry (id=119) reported BAD; the S-051 probe RUN 1/2 → rc=0x50030003, carved=0.
- Root cause: `git show 81ee9775 -- src/core/carve/cr_registry.cpp` — the FMT_SVG row deletion visible in the conflict-sweep hunk (the `=======`/`>>>>>>>` side dropped with the markers).
- Fix: the row restored verbatim from M-093's landing (`git show ca701711`).
- Pass-after: `CarveEngine.RegistryF8Gate` GREEN; `rejects=0 of 120`; the probe RUN 1/2 → rc=0 with carve enabled; full suite 1105/1105.

## Fix plan (executed this session)

1. Restore the FMT_SVG row in id order (before the MOBI entry, matching the enum: …FMT_XML, FMT_XML_BOM, FMT_SVG_CD01, FMT_SVG, FMT_MOBI) — done, with the in-table provenance comment citing the sweep commit and both landings.
2. Sweep the parallel lanes' format violations left on main (cr_signatures.h, cr_analysers.cpp, test_carve.cpp, test_thumbnail.cpp — 9 violations under clang-format 18.1.8, the CI-pinned version) so the format gate is green again.
3. The F8 invariant already had its pin (`RegistryF8Gate`); no new test needed — the existing test IS the fail-before proof. A per-row comment at the restored entry documents the sweep class for future lanes.

## Tests to add

None beyond the restored row: the pre-existing `CarveEngine.RegistryF8Gate` pin (test_carve.cpp:516-517) is the load-bearing regression test; it is RED pre-fix and GREEN post-fix (executed both ways this session).

## Related findings

- M-093 (FIXED — the row's original landing, `ca701711`)
- M-096 (FIXED — the MOBI row the sweep kept, `6e37dc21`)
- N-135 (the bookkeeping-truth class this incident extends: a push-window reconciliation that dropped content while claiming a repair)
- S-051 (the finding whose verification discovered this regression)

## Open questions

None. The two conflict sides' intent is unambiguous from both landings' commits.

## Resolution

**Status:** FIXED
**Fixed in commit:** `e4273f2b` (the parallel lane's push-window repair, 2026-10-28T18:03:16Z — restores the row verbatim in the one-line form; its own damage assessment: "187 tests red: the whole CarveEngine family + every scan-owning session/ABI test"). This lane's convergent landing was dropped at the rebase resolution per the SA-9 earlier-remote-push rule; what this lane's push KEEPS: this finding file (the full evidence record + the independent fail-before execution) and the 4-file clang-format sweep where the remote had not yet swept (verified at the resolved tree)
**Verified by:** fail-before/pass-after both executed: `CarveEngine.RegistryF8Gate` RED at `bcf31316` → GREEN post-fix; `validate_registry` 2→0 rejects (119→120 rows, the mobi id/index mismatch resolved by the row's restoration); the S-051 evidence probe RUN 1/2 rc 0x50030003→0 with carve enabled; full suite 1105/1105 post-fix; clang-format 18.1.8 clean over src/include/tests.
**Resolution notes:** CONVERGENT repair: the parallel S11 lane hit the same regression in its own push window and landed `e4273f2b` (the one-line row restore, pushed 2026-10-28T18:03:16Z) before this lane's fix reached the remote — their repair stands per the SA-9 rule, and the registry is 120/120 clean with `RegistryF8Gate` green at the merged tree (verified by this lane's post-rebase suite run). This lane's independent evidence chain — the same-commit-root-cause trace (`git show 81ee9775` — the two-different-rows conflict resolved as a duplicate), the regcheck harness output (`rejects=2 of 119`), the in-process instrumentation isolating carve_slice's F8 early-return, and the executed `RegistryF8Gate` fail-before at `bcf31316` — is preserved here as the finding's record. The 4-file clang-format sweep (cr_signatures.h / cr_analysers.cpp / test_carve.cpp / test_thumbnail.cpp — the M-093/M-096/thumbnail-ABI landings' residue) rides this lane's push where the remote had not yet swept it. Process note for future lanes: a rebase-window conflict between two DIFFERENT table rows must keep BOTH sides — the sweep that "kept the newer content" broke a structural invariant the test suite was watching (the parallel lane's count: 187 tests red); the window between `81ee9775` and the two convergent repairs is the trail's evidence that a green-claiming push window shipped a hard-down.
