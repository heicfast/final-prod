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
