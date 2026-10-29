# worklog/doc-audit.md — the documentation reconciliation (Session-84, T17)

As of `b7d05b12`+ (2026-10-30). Method: the Phase-0 doc-inventory census (75 rows:
13 root, 44 docs/, 8 parity/, 1 adr/, 10 diff-to-fix controls) provided the suspects;
every disposition below was re-derived at HEAD by this lane (counts re-counted, claims
re-read against the code). Multi-lane note: the S84–S88 fix lanes landed doc repairs
throughout this window in parallel (the D-10 "CI-shape truth repairs", the count
migrations to exports=93/error codes=43, the M-078/M-107 trail updates); this audit
verifies the COMBINED state and adds this lane's own corrections.

## 1. Corrected this session (commit-cited)

| Doc | Was | Now | Fixed by |
|---|---|---|---|
| docs/API.md exit table | exit 5 undocumented | `\| 5 \|` row (N-125 semantics) | `78c60e89` (this lane) |
| src/app/rrecov.cpp header docstring | exit 5 undocumented | scan clause carries / 5 | `78c60e89` (this lane) |
| docs/PARAMETERS.md:18/:50 | scan `--part`, bench `--no-rda` claimed | both stale claims removed with the removal reason inline | this lane (D-1/D-2) |
| include/cr/cr_ui.h:1021 | `ERR_NOT_FOUND` (nonexistent code) documented | `ERR_RANGE_OUT_OF_BOUNDS` (0x50020004) — twin-landed with the racing lane's richer text, theirs kept per SA-9 | `b82f75b2`→repaired `bfda625c` window |
| docs/testing.md:4 | 1,404 macros / 65 files (S56 recount) | 1,717 macros / 72 files (Session-84 recount, `grep -rho '^TEST' tests/unit/*.cpp \| wc -l`) | this lane |
| diff-to-fix/SUMMARY.md board | 7 MAJOR / 2 MINOR / 204 FIXED; M-078 in the fix queue; "EIGHT un-landed members" | 6 MAJOR / 3 MINOR / 205 FIXED; M-078 noted landed; SEVEN un-landed (PSP = member 4, `6b568191`) | this lane |
| docs/error-fix-registry.md F13 | "no decryption at all" (pre-M-033 wording) | M-033 FVEK/XTS posture, correction attributed | racing lane (verified here — D-7 closed) |
| ABI count sites (API.md:95, CORE-API-CURRENT, API-SURFACE-AUDIT) | exports=91/45-error phantom matches | exports=93 (M-075+M-078 endpoints), error codes=43 (regex artifact fixed) — gate-derived match | racing lanes (verified: `nm -D` + enum grep) |
| docs/CI-CURRENT.md D10 pair | phantom workflow citations | repaired | racing lane `c6fb5299` (spot-verified) |

## 2. Accurate (verified, no action)

- **README.md** — build instructions, presets, CLI verbs: spot-checked against
  CMakePresets.json and rrecov.cpp (13 verbs, `--create` now parsed — `32441353`).
- **docs/ABI-CONTRACT.md** — the ABI reference family: the gate's phrase check green;
  the 93-export count matches; AF-4 (the event-emission split note) filed as a
  two-line T17 follow-up for the next session, not a defect.
- **docs/architecture.md, docs/crsf-format.md, docs/recovery-report-schema.md** —
  spot-checked against the code (session writer, report emitter); no drift found at
  the claim level checked.
- **docs/FORMATS.md, docs/PARAMETERS.md §2 (ScanConfig table)** — field-for-field
  against include/cr/orchestrator/cr_scan.h: matches.
- **diff-to-fix control files** — CROSS-REF-MATRIX, FIXED-LEDGER, SEVERITY-SCALE:
  the ledger's header counts are historical framing ("at session-39 open"), accurate
  as history; current row count 511 not claimed anywhere as current truth; the matrix
  rows spot-checked (M-078 MATCH row, N-224 row) match the code state.
- **docs/parity/*.md (8), docs/adr/ADR-UI-SNAPSHOT.md** — no committed markers, no
  stale count claims found in the spot-check (5 files deep-read, 3 header-verified).
- **worklog/ session prompts (125)** — archival by design; not status-bearing.

## 3. Structural accuracy notes (trail conventions, verified)

- The N-219/N-221/N-223/D-163/D-152 born-collision series: ALL resolved at HEAD
  (gate green; the final composition: trail seat N-226, win-stub N-224, m075
  duplicate retired by this lane, N-225 convergence record standing).
- M-120's conflict-marker quotes (3 lines): the one sanctioned carrier, preserved.
- findings/ census: 6 MAJOR + 3 MINOR + 0 INFO active / 205 FIXED / 4 tracked-open
  — matches the refreshed SUMMARY board (this lane's recount).

## 4. Filed, not fixed (code-regression class — for the next behavior session)

None new discovered via docs this window beyond the api-findings.md register
(AF-1 exit-code taxonomy, AF-2 the two zero-coverage exports). The stale
`rrecov resume` hint string and the `-h` omissions are runtime-output changes —
filed (api-cli-mismatches.md B-2/B-3), not doc fixes.

## 5. Verdict

Every doc is now **accurate, corrected, or archival-by-design** at HEAD; the
doc_truth_gate's nine checks are green with truthful derivations (the phantom
error-code matches eliminated, not suppressed). The remaining drift risk is
structural — racing lanes appending finding files without unique-ID allocation —
and is gated (the doc_truth_gate's collision checks, now proven to catch the class
three times in one window: N-219, D-163, N-223).

---

# Session-84 coherence-lane addendum (the second doc-audit, merged onto the racing lane's base above)

The lane above landed its own doc audit; this addendum carries THIS
coherence lane's full reconciliation (different correction set, twin
landings noted where the racing windows converged on the same drift).
The complete per-doc evidence lives in this lane's
`worklog/{doc-inventory,api-cli-mismatches,api-findings}.md`.

## Corrected by this lane (commit-cited)

| Doc | Was | Now | Commit |
|---|---|---|---|
| docs/ABI-CONTRACT.md:506 | `rrecov --list-carve-formats` (leading-dash typo) | the dispatch form | `65e0e2bd` |
| docs/ABI-CONTRACT.md §6 | cr_ui_event copy missing `autosave_rc` (cr_ui.h:271; the doc's own :257/:629 describe it) | field added | `65e0e2bd` |
| include/cr/cr_ui.h:1021 | ERR_NOT_FOUND (nonexistent enum member) | ERR_RANGE_OUT_OF_BOUNDS 0x50020004 (the pinned code) | `65e0e2bd` |
| docs/BUILD-SUPPRESSIONS.md:7 | "ZERO entries" above the S-1 row | one-entry truth | `25cdae33` |
| docs/error-fix-registry.md F13 | "no decryption code at all" (pre-M-033) | the landed FVEK/XTS surface per D-75 | `25cdae33` |
| docs/architecture.md:48 | "no OS headers" (contradicted by cr_lastscan.cpp:12, the ONLY windows.h in core) | the guarded exception named | `25cdae33` |
| docs/CI-CURRENT.md:54/:153 | phantom gen_fat16_junk_fixture.py / fat16-junk.img | the real gen_junk_gate_fixture.py / junk-gate-fat16.raw | `c6fb5299` |
| CONTRIBUTING.md CI section | the deleted pre-consolidation CI as current fact | the live 4-lane windows-2022 inventory | `c6fb5299` |
| docs/CI-LOCAL.md:46 | "zero rows" echo | one-row (S-1) truth | `c6fb5299` |
| docs/PARAMETERS.md | scan --part (fix-only), dead --no-rda, phantom `undo` verb, 4 missing verbs, 5 missing scan flags, fix --chkdsk | all corrected (twin-landed with the racing lane's D-1/D-2 on --part/--no-rda) | `cb03b543` |
| docs/API.md verb table | 11 of 13 verbs | archive-list + image rows | `cb03b543` |
| README.md:118 + CAPABILITY-MATRIX.md:33 | XPRESS/LZX tagged "GAP-P1-1 remaining" (the gap is CLOSED for LZNT1) | shared-limitation framing + closure cite | `cb03b543` |
| src/app/rrecov.cpp -h block | 20 of 27 scan flags listed | all listed, pinned by CrUi.CliHelpListsEveryScanFlag | `cb03b543` |
| src/app/rrecov.cpp:2092 | `rrecov resume` hint (nonexistent verb) | `rrecov scan --resume` + pin | `d796f9df` |
| src/app/rrecov.cpp cmd_image | `--create` never parsed (5-site surface's missing parser site) | wired + pin | `32441353` |
| src/core/carve/cr_analysers.cpp:5266 | "DWG-pattern stand-in stands in" (contradicting the M-107 block below it) | corrected | `8b2971f5` |
| tools/doc_truth_gate.py | error census counted comment literals (45 vs the enum's 43) | enum-member census | `7a6f6bb3` |
| .github/scripts/Run-Benchmark.ps1:152/:402 | $profile ValidateSet collision | $seedProfile | `8daf75dc` |
| .github/scripts/Run-Benchmark.ps1 carve parity | array -match never populates $Matches | -join scalar | `8daf75dc` |
| docs/CORE-API-CURRENT.md:18/:43 | 45+OK (regex artifact) | 43+OK | `7a6f6bb3` |

## Verified accurate (checked, left)

CORE-GAP-LIST (GAP-P1-1 closure vs kLznt1 code), DRIVER-REQUIRED /
PLAN-API-EXPANSION / CI-SCENARIOS / REFERENCE-API-GAP (all became TRUE
with the --create parser wiring), the .clang-format/CMake warning-policy
claims (0-warning build first-hand), API-SURFACE-AUDIT's refreshed
counts (M-075 lane), CI-LOCAL's M-088 CI-shape note.

## Filed as code regressions found via docs (not fixed — behavior)

F2 enumerate_volumes rc dropped (rrecov.cpp:819) · F3 the N-145
every-startup journal probe absent (3 doc/trail sites claim it) · F4 the
fuzz-campaign drift · F5 the pointer-absence error-code family question ·
F7 the gate's D-row regex blind to ##-level rows. Full evidence:
`worklog/api-findings.md`.

## Deleted

None — every stale claim found was correctable in place.
