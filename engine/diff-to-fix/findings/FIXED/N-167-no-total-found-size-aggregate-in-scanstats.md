# MINOR N-167 — the og's scan-completion headline reports found-count + total found SIZE ("Files Found: N (X GB)"); the core's ScanStats carries counts only — no size aggregate exists

**Severity:** MINOR
**Module:** orchestration (scan stats) / query
**Parameters affected:** report schema (13), progress/stats semantics
**Corpus references:** completion line "Scan Completed, Files Found: 1139286 (147.12 GB)" (`dynamic-ground-truth--scan-phase-captured.md:125`); the completion-modal grid "Files found · Files scanned · Used Time · Size · Organized · Completed …" (`re-workspace/artifacts/UIUX/08:32`)
**Core references:** `include/cr/orchestrator/cr_scan.h:345-479` (ScanStats — counts, bytes_scanned = carve-true bytes only, bytes_unreadable, elapsed; no sum of found logical sizes), `src/app/rrecov.cpp:1662` (CLI summary prints counts only); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's completion surface reports the total logical size of found files alongside the
count; the core has no equivalent aggregate (bytes_scanned is I/O truth, not found-size
truth). Hosts cannot render the og's headline from core stats without walking the tree.

## What the reference does

The completion line + modal grid carry "Files Found: N (X GB)" — the size of what was
found, computed over the result tree (cites above).

## What our core does

ScanStats has no found-size field; the tree carries per-node logical_size (queryable,
but not aggregated).

## Gap

A one-field stats aggregate absent; the og surfaces it as the headline number.

## Impact

CLI/hosts print counts only; the og-parity completion line cannot be rendered from
stats. Cosmetic-to-convenience class (the data exists in the tree).

## Reproduction / verification

Corpus-derived (the two surface cites). Core-derived (the ScanStats field census read
directly; CLI summary read).

## Fix plan

1. Add `u64 bytes_found` to ScanStats: accumulate `logical_size` at node attach (the
   same hook the counters use — the N-166 tick refresh renders it live too).
2. Files: `include/cr/orchestrator/cr_scan.h`, `src/core/orchestrator/cr_scan.cpp`
   (attach hook), `src/app/rrecov.cpp` (the summary line gains "(X GB)").
3. Tests: `ScanStats.BytesFoundAggregates` (fixture with known sizes); CLI golden-line
   update.
4. Acceptance: the completion line renders the og shape; stats round-trip the session.

## Tests to add

- `ScanStats.BytesFoundAggregates`
- `ScanStats.BytesFoundSessionRoundTrip`

## Related findings

- N-166 (the live cadence for the same numbers)
- N-140 (the per-extension carve stats surface — different axis, already active)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-167 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `ScanStats.BytesFoundAggregates` (RED pre-fix via the stashed-stamp rebuild: bytes_found stayed 0 while the tree carried the fixture's 1500 bytes — the finding's falsifiable claim executed; GREEN: the aggregate covers BOTH the active and the deleted population and equals tree.total_logical() — the one-consistent-pair pin) + `ScanStats.BytesFoundSessionRoundTrip` (the v3 REC_SESSION_STATE record persists the aggregate and re-reads it identically on resume). Suite 1320/1320 (linux-release).
**Resolution notes:** The field landed as `ScanStats::bytes_found`, stamped from `ResultTree::total_logical()` inside `tel_counts_` (the signature widened const → mutable) so the counters and the size ride the SAME observation points — a host rendering the og's headline reads one consistent pair, and the staleness semantics match files_found exactly (boundary-stamped; N-166's live-cadence work extends both together when it lands). The session record grew to v3 (48 bytes; v1/v2 load zeroed — the tree re-stamps at the first count refresh after resume). The CLI completion line renders the og's shape ("N file(s) (X GB)") with the binary 1024^3 divisor GB-labeled — the corpus capture pins the convention (1,139,286 files / 147.12 GB on a 149.45 GB volume = the Windows binary-GB labeling); the JSON summary gained bytes_found alongside bytes_scanned. bytes_scanned's I/O-truth semantics untouched.
