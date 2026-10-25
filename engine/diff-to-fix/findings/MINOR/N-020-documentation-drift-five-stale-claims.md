# MINOR N-020 — Documentation drift across five spots (audit-trail hygiene)

**Severity:** MINOR
**Module:** quality (documentation accuracy — Q68)
**Parameters affected:** Q68 (documentation accuracy — every doc reflects current behavior)
**Corpus references:** `diff-to-fix/PARAMETERS.md:134` (Q68 definition: "Documentation accuracy — every doc reflects current behavior")
**Core references:** spot-checked 6 claims across `AUDIT-PIPELINE.md`, `diff-to-fix/modules/cli.md`, `docs/PERFORMANCE-PROFILE.md`, `diff-to-fix/findings/SEVERE/S-009-MAJOR-M-006-diskmap-surface-and-fuzz-ci.md` — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Session 1's audit-trail docs (`AUDIT-PIPELINE.md`, `diff-to-fix/modules/cli.md`,
`docs/PERFORMANCE-PROFILE.md`, `diff-to-fix/findings/SEVERE/S-009-...md`)
have drifted from the current code state. Five spot-checked claims are
stale; one is partly wrong. The drift is the natural consequence of
concurrent remediation commits landing while the audit was in flight —
but the audit-trail discipline requires the docs to be kept current.

## What the reference does

The corpus's Q68 definition is aspirational; the og's docs are not in
the corpus. This is a quality-parameter finding, not an og-parity
finding.

## What our core does

| # | Document | Line(s) | Claim | Reality |
|---|---|---|---|---|
| 1 | `AUDIT-PIPELINE.md` | 179-185 | "WRITER-1 (P2, OPEN): if (u32 rc = recover_item(...)) return rc; aborts the whole queue on the first per-item error" | `cr_safewriter.cpp:455-465` shows the fix: `if (rc == ERR_CANCELLED) return ERR_CANCELLED;` — only cancel hard-aborts; per-item errors are collected in `stats.errors`. The WRITER-1 fix is IN tree, with a code comment explicitly citing "WRITER-1 (AUDIT-PIPELINE, F-A4-8)". The audit doc still says OPEN. |
| 2 | `diff-to-fix/modules/cli.md` | 14 | "Commands — M-as-designed: scan (3 modes), recover, report, session save/load/resume, repair/undo (S-008 wiring), verify, query." | `rrecov.cpp:1459-1499` wmain dispatches only 7 commands: list, scan, tree, recover, health, fix, bench. NO `verify` command, NO `query` command. SESS-2 verify-identity is `cr_ui_scan_verify_identity` (API only); query is `cr_ui_query_run` (API only). The CLI's `tree --json` uses `query::run` internally but doesn't accept user query predicates. |
| 3 | `diff-to-fix/modules/cli.md` | 12-13 | "C24 exit codes — 0 success / 1 usage / 2 scan error / 3 recover error" | `rrecov.cpp:17-19` docstring: "scan 0 ok / 1 failure / 3 cancelled (session saved when --out was given); list 1 on enumeration failure; health 3 when every health source is unavailable; recover 2 partial (item errors / hash mismatch)." cli.md conflates the codes with the wrong commands. |
| 4 | `docs/PERFORMANCE-PROFILE.md` | 1 | "baseline measurements at HEAD 064fcf4 (session 6)" | Current HEAD is `5d6da5f`; the file is a historical baseline (acceptable as-is) but should note "see diff-to-fix for current status." |
| 5 | `docs/PERFORMANCE-PROFILE.md` | 50-52 | "Direct raw handles: BELOW the 64 KiB contract (GAP-S6-13)" + "CachedDevice 64 KiB block batching (1 MiB cache)" | GAP-S6-13 LANDED (commit `7777e1c`, in tree per `cr_directalign.h` + `cr_transport.cpp:148-170`). The "below" claim is stale. Also: the cache is 8 MiB at the orchestrator call site (`cr_scan.cpp:540` `CachedReader::create(..., 8ull << 20, 64 * 1024)`), NOT 1 MiB. `cr_cached.h:25` has a 1 MiB default that the orchestrator overrides. `AUDIT-ARCH.md:37` correctly cites 8 MiB. |
| 6 | `diff-to-fix/findings/SEVERE/S-009-MAJOR-M-006-diskmap-surface-and-fuzz-ci.md` | whole file | "S-009: Disk-map visualization surface absent" — Status: "READ-ONLY FINDING — no fix applied" | GAP-S6-11 LANDED (commit `56cce87`). `cr_ui.h:492-500` exports `cr_ui_disk_map` + `cr_ui_disk_map_free`; `cr_ui.cpp:1461-1607` implements `disk_map_impl`; `tests/unit/test_cr_ui.cpp` (per commit message) has 242 LOC of disk-map tests. S-009 is RESOLVED in tree but the finding file is stale. |

## Gap

Six specific drift instances; five are stale claims, one (#6) is a
finding file that should be moved to `findings/FIXED/`.

## Impact

- **Audit-trail hygiene:** future readers trusting the docs will be
  misled. The drift is the natural consequence of concurrent
  remediation commits landing while the audit was in flight — but
  the audit-trail discipline requires the docs to be kept current.
- **No correctness impact:** the docs are not consumed by code; the
  drift is purely informational.
- **Cognitive load:** a new reader has to verify each doc claim
  against the code, doubling the audit cost.

## Reproduction / verification

- **Core evidence:** first-hand read of each cited file:line. The
  drift is reproducible from the source.

## Fix plan (for the engineering team)

1. **Update each stale claim** to reflect the current code state.
2. **Files to modify:**
   - `AUDIT-PIPELINE.md:179-185` — change "WRITER-1 (P2, OPEN)" to
     "WRITER-1 (P2, FIXED): per-item errors collected in stats.errors;
     only cancel hard-aborts. Fix in `cr_safewriter.cpp:455-465`."
   - `diff-to-fix/modules/cli.md:14` — change "scan (3 modes),
     recover, report, session save/load/resume, repair/undo, verify,
     query" to "list, scan, tree, recover, health, fix, bench (the
     7 dispatched commands). verify and query are API-only
     (`cr_ui_scan_verify_identity`, `cr_ui_query_run`)."
   - `diff-to-fix/modules/cli.md:12-13` — change "0 success / 1 usage /
     2 scan error / 3 recover error" to the actual codes per
     `rrecov.cpp:17-19`: "scan: 0 ok / 1 failure / 3 cancelled; list:
     1 on enumeration failure; health: 3 when all sources
     unavailable; recover: 2 partial."
   - `docs/PERFORMANCE-PROFILE.md:1` — add "see diff-to-fix for
     current status (HEAD `5d6da5f`); this file is a session-6
     historical baseline."
   - `docs/PERFORMANCE-PROFILE.md:50-52` — change "Direct raw handles:
     BELOW the 64 KiB contract (GAP-S6-13)" to "Direct raw handles:
     ABOVE the 64 KiB contract (GAP-S6-13 LANDED in `7777e1c`)." Change
     "CachedDevice 64 KiB block batching (1 MiB cache)" to "8 MiB
     cache at orchestrator call site (`cr_scan.cpp:540`); 1 MiB
     default in `cr_cached.h:25` (orchestrator overrides)."
   - `diff-to-fix/findings/SEVERE/S-009-MAJOR-M-006-diskmap-surface-and-fuzz-ci.md` —
     add a "## Status: RESOLVED (commit `56cce87`)" banner at the top;
     move to `findings/FIXED/` per the session-2 mandate.
3. **Algorithm/constant:** none.
4. **Tests to add:** none — this is a doc-accuracy finding.
5. **Acceptance criteria:** all six drift instances are corrected; a
   spot-check of 5 additional doc claims shows no further drift.

## Tests to add

None — this is a doc-accuracy finding.

## Related findings

- **S-009 / GAP-S6-11** (the disk-map visualization surface) — finding
  #6 above is S-009 itself. The finding file should be moved to
  `findings/FIXED/` (parallel to the A-001/M-001/S-001/S-005 moves
  already done in session 2's wave 1).
- **M-006** — fuzz CI disabled. Finding #6 also covers M-006 (the
  S-009 finding file is a combined S-009 + M-006 finding). The M-006
  component is still open; the S-009 component is resolved.

## Open questions

- Should the audit-trail discipline include a CI check that grep's
  the docs for known-stale claims (e.g. "BELOW the 64 KiB contract"
  after GAP-S6-13 lands)? Low priority — the discipline is human,
  not automated.
