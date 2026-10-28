# MAJOR M-074 — Destination-validation error codes undifferentiated: path-too-long, invalid-path, and network all collapse to ERR_DEST_UNWRITABLE

**Severity:** MAJOR
**Module:** writer / recovery / ABI
**Parameters affected:** C23 (error codes), D13/D14 (report/error-map contents — the per-item reason codes), F62 (destination overlap refusal — the sibling refusal family)
**Corpus references:** `architecture/04-recovery-workflow.md:136-140` ("`CRecoveryMgr::ExportThread`: … destination validation: `PathIsNetworkPathW`? → local: opens `\\.\X:` raw volume of destination (writability + same-volume detection), falls back to `PathFileExistsW`; network: attribute checks"); `re-workspace/artifacts/UIUX/11-recovery-phase-funnel.md:16` ("- Path validation: 'Might be invalid path to save.' / '2.The recovery path is too long.' (numbered error taxonomy shared with the recovery log)")
**Core references:** audited `35adcb9`. `include/cr/cr_error.h:53-63` — the complete 0x5005 family: `ERR_DEST_OVERLAP 0x50050001`, `ERR_DEST_UNWRITABLE 0x50050002`, `ERR_HASH_MISMATCH 0x50050003`, `ERR_DEST_FULL 0x50050004`, `ERR_DEST_NAME_EXHAUSTED 0x50050005` — no path-too-long, no invalid-path, no network-classification code. `src/win/cr_destio.cpp:18-24`: every `CreateFileW` failure except disk-full collapses to `ERR_DEST_UNWRITABLE` (`if (e == ERROR_DISK_FULL || e == ERROR_HANDLE_DISK_FULL) return ERR_DEST_FULL; return ERR_DEST_UNWRITABLE;`); mkdir failures likewise (:153). `tests/unit/test_destio_win.cpp:220` proves a UNC destination reports plain `ERR_DEST_UNWRITABLE`. Grep `rg "PathIsNetworkPath|is_network|UNC"` over `src/**` → 0 code hits.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's destination validation distinguishes invalid paths, too-long paths, and network destinations (each with its own user-facing message and validation branch). The core's destination failure path collapses every non-disk-full CreateFileW/mkdir failure — including `ERROR_FILENAME_EXCED_RANGE`, malformed names, and UNC/network paths — into the single `ERR_DEST_UNWRITABLE`, so hosts cannot surface the og's specific guidance.

## What the reference does

The og's `ExportThread` destination validation branches: `PathIsNetworkPathW` detects network destinations (attribute checks on that branch); local paths open the `\\.\X:` raw volume of the destination drive (writability + same-volume detection) with a `PathFileExistsW` fallback (architecture/04:136-140). Its error taxonomy surfaces distinct outcomes — "Might be invalid path to save." and "The recovery path is too long." are separate numbered strings shared with the recovery log (UIUX/11:16).

## What our core does

Five destination codes exist (overlap/unwritable/hash-mismatch/full/name-exhausted — cr_error.h:53-63). `cr_destio.cpp:18-24` maps only disk-full out of the generic failure; everything else — path-too-long (`ERROR_FILENAME_EXCED_RANGE`), invalid name (`ERROR_INVALID_NAME`/`ERROR_BAD_PATHNAME`), network shape — becomes `ERR_DEST_UNWRITABLE`. The UNC test at test_destio_win.cpp:220 pins the collapsed behavior. Overlap arming exists and is correct (N-013, cr_safewriter.cpp:145-147).

## Gap

A recovery whose destination path exceeds MAX_PATH, is malformed, or is a network path returns the undifferentiated `ERR_DEST_UNWRITABLE` where the og surfaces three distinct outcomes. Falsifiable: `recover --to \\server\share` → core reports `ERR_DEST_UNWRITABLE`; og reports the network-branch validation outcome with its own message.

## Impact

API-contract and UX gap: hosts lose the og's actionable guidance ("path too long" → shorten; "invalid" → retype; network → the attribute-check branch's result). The failure IS surfaced (nothing silently succeeds), so this is classification, not correctness — but the report's per-item `reason` codes (D14) carry the collapsed value downstream.

## Reproduction / verification

- Corpus-derived (architecture/04:136-140 + UIUX/11:16 quoted).
- Core first-hand: cr_error.h family enumeration read; cr_destio.cpp:18-24 read; the UNC test's expectation read (test_destio_win.cpp:220); greps for network-detection zero hits.

## Fix plan (for the engineering team)

1. `include/cr/cr_error.h`: append `ERR_DEST_PATH_TOO_LONG = 0x50050006`, `ERR_DEST_PATH_INVALID = 0x50050007`, `ERR_DEST_NETWORK = 0x50050008` (append-only family; no ABI bump); `src/core/err/cr_error.cpp` err_text rows.
2. `src/win/cr_destio.cpp` `create_temp`/`mkdir`: classify `ERROR_FILENAME_EXCED_RANGE` → TOO_LONG; `ERROR_INVALID_NAME`/`ERROR_BAD_PATHNAME` → INVALID; discriminate `ERROR_PATH_NOT_FOUND` vs `ERROR_FILE_NOT_FOUND`; detect UNC/network shape at the destination-validation layer (path grammar check — no need for PathIsNetworkPathW's dynamic query for the common `\\` prefix case; document the limitation for mapped drives).
3. Propagate through `cr_safewriter` reason codes so the JSON report carries the specific code (D14), and through the ui-side validation site (`src/ui/cr_ui.cpp:685/698` region).
4. Docs: ABI-CONTRACT §4 error-table rows, CORE-API-CURRENT writer-0x5005 table.
5. Tests: >MAX_PATH destination, malformed path, UNC path → the three new codes (extend `test_destio_win.cpp`); report-schema reason-text test.

## Tests to add

- `Destio.PathTooLongClassified` (ERROR_FILENAME_EXCED_RANGE → ERR_DEST_PATH_TOO_LONG).
- `Destio.InvalidNameClassified` (ERROR_INVALID_NAME → ERR_DEST_PATH_INVALID).
- `Destio.NetworkShapeClassified` (UNC → ERR_DEST_NETWORK; the mapped-drive caveat documented).
- `Report.CarriesSpecificDestReason` (JSON report per-item reason).

## Related findings

- N-013 (FIXED — overlap arming; the strongest member of the family, unaffected).
- N-015/N-009 family (FIXED — full/ACL semantics, siblings in the same error family).

## Open questions

Mapped-drive network detection (PathIsNetworkPathW's dynamic half) — the static `\\` prefix covers the corpus's named case; whether to call WNetGetResourceInformation for mapped drives is an engineering call, to be documented either way.
