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

## Session-42 addendum (2026-10-29, audited `869da779` — the fix-wave-stale extension)

The fix wave since N-150's last verification (M-072/M-074/M-077/N-098/M-109/M-023's
full-taxonomy landing/S-038 installer/OQ-SA-1 dwords + the S-051/S-052/S-053/S-054
engine landings) re-staled the register in BOTH directions faster than the finding's
original fix plan (never executed) could land. The S42 core-currency pass re-verified
the board first-hand; the newly-stale set:

**Newly-stale in the underclaiming direction (feature landed, register says absent):**

1. **API-DELTA §5.2 S3-W2a (:252)** — "substring/state axes ABSENT" → **landed (M-072)**:
   `cr_ui.h:1026` (`path_substring`) / `:1028` (`state_mask`), verified this session.
2. **API-DELTA §5.2 S3-W3a (:254)** — "`ERR_DEST_*` family has no path-too-long /
   invalid-path / network-classification codes" → **landed (M-074)**:
   `cr_error.h:67-69` (ERR_DEST_PATH_TOO_LONG/_PATH_INVALID/_NETWORK).
3. **API-DELTA §5.2 S3-W4a (:258)** — preview Caps "no shipped defaults
   (zero-init = uncapped today)" → **landed (M-077)**: `cr_preview.h`'s shipped
   100 MiB/15 s defaults (FIXED-LEDGER :348).
4. **API-DELTA §5.2 S3-W3c (:256)** — "the header contract sentence" → **landed (N-098)**:
   the refresh-contract sentence at `cr_ui.h:476-482`.
5. **API-DELTA row 10 (:50)** — the "6 hard-coded categories / ~90 ext (M-023 engine
   scope)" caveat → stale: the full FileFilter.xml taxonomy landed
   (`cr_filefilter_data.cpp`, 8 categories / 1,481 unique extensions).
6. **API-DELTA row 22 (:73)** — the "scoring parity M-017 engine scope" caveat →
   stale (M-017 FIXED, FIXED-LEDGER :120).
7. **§3 S-038 (:132-134)** — "steps 2-4 queued for the packaging lane" → stale: the
   installer landed (`installer/core-recovery.iss`, `run-gates.ps1`).
8. **§3 OQ-SA-1 (:140)** — listed as remaining → stale: the NTFS/FAT dwords landed
   (`cr_node.h:188` `fat_origin_cluster`, `:216` `ntfs_origin_lcn`).

**Newly-found defect sites (the original filing's classes, new instances post-dating it):**

9. **The phantom macro** — `cr_ui.h:328/:407/:418/:421` cite
   `CR_UI_ERR_FVE_LOCKED (0x50060001)` in four comment contracts; the macro is never
   `#define`d anywhere (the real name is `ERR_FVE_LOCKED`, `cr_error.h:72`). The same
   class as the fixed `cr_ui.h:916` `CR_UI_ERR_INVALID_PARAM` instance (API-DELTA
   row 32).
10. **The dead duplicate `--label` handler** — `src/app/rrecov.cpp` parses `--label`
    at `:1398` (the N-016b U+FFFD-rule copy) AND again at `:1411-1420` (an
    ASCII-truncating variant, dead because the first `continue`s). The rebase-leftover
    class (the fixed N-142's sibling); the two copies' semantics DIVERGE (ASCII-only
    truncation vs the U+FFFD rule), so a future reorder would silently change
    behavior.
11. **ABI-CONTRACT §7i (:616)** — "max_read **32 MiB (M-080)**" → stale: M-109
    re-pinned all four sites to 200 MB; §7i was missed (the finding's own
    per-site-sweep class).
12. **The test census** — `docs/testing.md:4` still says "1,014 TEST/TEST_F macros"
    (the PARITY-FIX-3 count); the actual at HEAD is **1,199** across 61 files (the
    disclaimer line :5 covers the drift direction but 185 macros of drift is beyond
    its intent).
13. **The count sites re-staled** — `docs/API.md:92` "81 exported functions" and
    `docs/CORE-API-CURRENT.md:12` "78 exports / 34+OK error codes / 10 event types"
    vs the file's OWN `:38` banner "83 exports" and the actual 83/42+OK/12
    (`cr_ui.h`'s 86 `CR_UI_API` hits − 3 `#define`s; ABI v3 at `:81`): the M-074
    +3-error-codes and the M-102 thumbnail exports post-date both docs' last bumps.
14. **bench/README.md:7-8/:50-53** — cites the deleted
    `.github/workflows/comparative-bench.yml` in the present tense and names two
    nonexistent report files (`docs/COMPARATIVE-BENCHMARK.md`,
    `benchmark-summary.json`); the `doc_truth_gate` scans only docs/ + root README so
    bench/ escapes it (OQ-S4-3:250-254 registered the same sites).

The fix plan is unchanged (items 1-4 of the original); the sweep's scope grows by
items 9-14 and the row-annotation set grows by items 1-8. Row 16's overclaim
(archive-browse MATCH while M-116 is open) is unchanged and still the only
overclaiming row.


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-150 landing, S56 lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** the doc_truth_gate's `check_abi_count_claims` (the CI-wired count invariants this landing adds — the fix plan's item 3): RED first run at the unrepaired docs named FIVE violations (API.md:92 "83 exported functions" vs derived 86; CORE-API-CURRENT:12 "78 exports"; its :16 "34 + OK" error codes and "10" event types; API-SURFACE-AUDIT:32 "81 exports") — GREEN post-fix: "ABI count sites match the tree (exports=86, error codes=45, event types=12)". The check's derivation is the tree itself (cr_ui.h's CR_UI_API census minus #defines; cr_error.h's case-normalized distinct 0x5-codes; the CR_EVT_* census) — and it earned its keep DURING the landing: it caught this lane's own first-pass "39+OK" (a lowercase-only grep had missed six uppercase-hex spellings; the true census is 45 — corrected in the same push). Full suites green at the landing tree: cr_tests 1,289 passed (0 failed; linux-release) + cr_ui_tests 77/77; the C consumer (ui_abi_check.c) builds clean against the touched header.
**Resolution notes:** Both directions repaired. OVERCLAIMING: the count sites re-derived (86 exports = 89 CR_UI_API − 3 defines = the 86 extern definitions in cr_ui.cpp, recount matches; 45+OK error codes case-normalized; 12 event types; errmod 13 = kNone + 12 attribution modules; 12 CLI verbs incl. archive-list) at API.md:92 + CORE-API-CURRENT:12/:16/:38 + API-SURFACE-AUDIT:32 — with the "as of <landing>" snapshot phrasing retired (it is how stale counts hid; the gate now enforces live derivation); A-ABI-3 retired-EXECUTED (the drift class is now CI-automated); API.md's normative table gained the two cr_ui_thumbnail_* rows (absent since M-102). UNDERCLAIMING: R-84 flipped PARTIAL→PRESENT (the 6-facet cr_ui_driver_status_t verified on disk + the old boolean probe's removal cite; both R-84 rows), R-37 reshaped GAP→PARTIAL (the engine's cr_preview.h inventory EXISTS; the real residual is the browse-inside depth — M-116's follow-up pointer), the :345 status line recounted by the table's own awk census (PRESENT 45 · PARTIAL 10 · GAP 27 — this lane's first draft guessed 42/16/27 and was redone against the real count; recorded here as the anti-example). The S42 addendum's items: the phantom macro CR_UI_ERR_FVE_LOCKED → ERR_FVE_LOCKED at its four comment-contract sites (cr_ui.h); the dead duplicate --label handler removed (rrecov.cpp — dead from birth, the live handler `continue`d first; its ASCII-truncating semantics DIVERGED from the live N-016b U+FFFD rule; the removal annotated in-source, the live semantics pinned at the ABI seam by SessionListVolumeLabelFullWidthRoundTrips [cfg.volume_label_utf8 → the session card] — no CLI-process harness exists, the app is a standalone executable: registered honestly, not silently skipped); ABI-CONTRACT §7i's 32 MiB → 200 MB was FIXED-ALREADY convergent (on disk, verified); testing.md's census re-truthed (1,404 TEST/TEST_F macros / 65 files, the latest green counts pinned inline); bench/README.md's report cites re-truthed (compare.py IS on disk — the earlier present-tense phrasing implied standing committed outputs, which sent OQ-S4-3 looking for phantoms; the outputs are runtime artifacts by design, now stated); the API-DELTA stale-landed rows annotated (S3-W2a = M-072 LANDED, S3-W3a = M-074 LANDED, row 22's M-017 caveat RETIRED, S-038's steps 2-4 LANDED, OQ-SA-1's dwords LANDED). During the verification the sibling window pushed a RED MAIN (the mangled-merge class) — this lane repaired it independently (orphan fragment + duplicate-test rename) but the remote lane's own repair landed first and STANDS per SA-9 (convergent; the suite numbers above are from THEIR fixed tree).
