# MINOR N-150 — the API doc-truth batch (S11-R): API.md's 81-export headline (the truth is 83) + the missing thumbnail rows; CORE-API-CURRENT:16's "34 + OK" error-code pair (the tree carries 38) and its 10-events claim (12); API-SURFACE-AUDIT:32's 81 count + the unretired A-ABI-3; REFERENCE-API-GAP:345's 3-way-stale status line + the false-missing R-84 pair (the 6-facet driver-status surface landed, the rows still say probe-only) + the doubly-stale R-37; and the API-DELTA rows stale-landed since the board's baseline

**Severity:** MINOR (doc-truth; every coverage verdict re-verified — the counts and row states drift in both directions)
**Module / Surface:** API gate docs (docs/API.md, docs/CORE-API-CURRENT.md, docs/API-SURFACE-AUDIT.md, docs/REFERENCE-API-GAP.md, diff-to-fix/API-DELTA.md)
**Parameters affected:** P68 (documentation accuracy), A45-adjacent (the ABI versioning/count sites), P69-adjacent (the delta board's currency)
**Corpus references:** n/a — core-side doc-truth finding (the corpus facts these docs describe are carried by the named rows)
**Core references:** audited commit `b6e0a9ef` — (1) `docs/API.md:81` ("81 exported functions in the current header" — the truth is **83**: 86 raw `CR_UI_API` lines − 3 `#define` macros in include/cr/cr_ui.h; 82 match the naive `CR_UI_API <type> <name>(` pattern + `cr_ui_last_error` at the two-token-return line; the two `cr_ui_thumbnail_*` exports at :1283/:1290 are ALSO absent from API.md's normative tables); (2) `docs/CORE-API-CURRENT.md:16` ("error codes **34 + OK**; … ABI event types 10" — the tree carries **38** distinct 0x5-codes in cr_error.h (`rg -o "0x5[0-9a-f]{7}" include/cr/cr_error.h | sort -u | wc -l`) and **12** ABI event types; the same file's later count line says "39+OK" — the file self-contradicts and the :16 pair never absorbed the M-074 landing's three new ERR_DEST codes); (3) `docs/API-SURFACE-AUDIT.md:32` ("81 exports as of the W1b load-progress landing" — stale; :129's A-ABI-3 audit row never re-executed); (4) `docs/REFERENCE-API-GAP.md:345` ("PRESENT 41 · PARTIAL 15 · GAP 29" — the actual table counts differ; a 3-way stale status line); (5) **the false-missing R-84 pair** (`:322` "win::rda_available probe ONLY — no version/loaded/started/authorized" and `:426` "rda_available boolean only | PARTIAL" — the probe is REMOVED (src/win/cr_transport.cpp:13) and the 6-facet `cr_ui_driver_status_t` + `cr_ui_driver_status` export (include/cr/cr_ui.h) + the facet classifier (include/cr/win/cr_driver_status.h) LANDED — the rows underclaim in the reverse direction from N-132's overclaim, contradicting API-DELTA row 29); (6) **the doubly-stale R-37** (`:236` "none — preview reads file bytes only | GAP" — the engine's format/enumeration surface EXISTS (cr_preview.h:134-191, the archive inventory); the real gap is the ABI/CLI exposure = **M-116**); (7) the API-DELTA board's stale-landed rows from its d1fba5d baseline (row 10's M-023 table landed; row 14's live owner is M-101; row 18's D-77 registered; row 22's M-017 fixed; §3's S-038 steps complete; OQ-SA-1's dwords landed) — status-note annotations, not rewrites
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (S11-R, 2026-10-29):** drafted as N-136 by this lane (baseline `8365956e`); renumbered at the push-time reconciliation — the remote lanes consumed N-136 (the retired session-list-CWD finding's renumber) and N-137 (the thumbnail ABI header contradiction) before this lane's push. Every item re-verified at `b6e0a9ef` after the 63-commit window: ALL the cited sites persist (API.md:81, CORE-API-CURRENT:16, API-SURFACE-AUDIT:32, REFERENCE-API-GAP:345/:426/:236 re-read this session at the new HEAD).

**Prior audit check:** N-132 (the false-landed triplet) and N-135 (the bookkeeping batch) are the precedent filings — this batch is the FULL currency pass the lane's wave-3 ran (the 90-row API-DELTA sweep + the reverse-direction R-row verification), filing every stale site the parallel lanes' truth commits (f166440b/53495c0c/14cf8064/2cf3f4d0) did not already own. M-116 owns the R-37 re-open's substance; this finding owns the row-state repair.

## Summary

The API gate docs drifted in BOTH directions around the fix window: underclaiming (REFERENCE-API-GAP's R-84 pair still describes the removed probe while the 6-facet status surface stands landed; R-37 says the preview enumeration does not exist while the engine half does; the API-DELTA rows still tag landed features as MISSING) and overclaiming (the export-count sites say 81 while the header carries 83; CORE-API-CURRENT:16 says 34+OK/10-events while the tree carries 38/12; the :345 status line). Three findings now share this territory (N-132, N-135's items, this one) — the next doc-truth fix wave should land them as ONE sweep with a CI invariant.

## What the reference does

n/a (doc-truth). The governing standard: counts re-executed against the tree, row states matching the code, no doc-citing-doc chains terminating in absent code (the N-132 lesson — its R-31:225 spread is this finding's item (6) neighbor and stays N-132's own).

## What our core does

As cited — every site re-read at `b6e0a9ef`; the counts re-derived (83 exports; 38 error codes in cr_error.h; 12 event types; 11 verbs).

## Gap

Falsifiable per item: read the cited line; run the cited count. Each checkable in minutes.

## Impact

The API gate docs are the surface every future session and every host integrator navigates by. The underclaiming rows (R-84 especially) hide landed capability; the overclaiming counts misstate the ABI's size at the exact moment the thumbnail exports made it user-relevant.

## Reproduction / verification

Read-level, executed twice (at `8365956e` and re-verified at `b6e0a9ef` after the push window): every cited line re-read; the export census re-derived (the 82-pattern artifact documented — `cr_ui_last_error`'s `const char*` return defeats the naive pattern); the R-84 reverse claim verified (cr_ui.h:379-396-area struct + export; cr_transport.cpp:13's removal note; cr_driver_status.h); the error-code count by the exact regex command above.

## Fix plan (for the engineering team)

1. ONE doc-truth sweep (composes with N-132's rows 44-45 + N-135's count items): API.md — 81→83 + the two thumbnail export rows; CORE-API-CURRENT — the :16 pair 34+OK/10 → 38+OK/12 (and reconcile with the file's own later count line); API-SURFACE-AUDIT — :32 count + retire/execute A-ABI-3 (:129); REFERENCE-API-GAP — :345 recount, the R-84 pair flip to PRESENT (the 6-facet cite), R-37 re-write to the ABI/CLI gap shape (pointer to M-116).
2. API-DELTA row annotations: rows 10/14/18/22 + §3's stale-landed entries gain "LANDED <sha>" status notes (the board is the audit trail — annotate, do not rewrite).
3. Add the count sites to `tools/doc_truth_gate.py`'s invariants (the M-088 gate) so 81/34/10-class drift fails CI: the export count derived from the header, the error-code count from cr_error.h, the event count from ABI-CONTRACT §6.
4. Acceptance: every count site re-derived green in CI; no row claims a state the tree contradicts.

## Tests to add

- The doc_truth_gate count invariants (item 3) — the fail-before is the current 81/34/10 set.

## Related findings

N-132 (the false-landed triplet — same sweep; its R-31:225 item is NOT re-filed here), N-135 (the count sites), M-116 (R-37's real gap), M-102 (the export that made 83 true), API-DELTA row 29 (R-84's counterpart).

## Open questions

None. Every site verified read-level at the current HEAD.
