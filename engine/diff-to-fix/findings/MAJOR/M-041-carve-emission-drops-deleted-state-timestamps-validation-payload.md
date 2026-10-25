# MAJOR M-041 — Carved files emitted without deleted-state, timestamps, or validation payload; sessions round-trip the misstatement

**Severity:** MAJOR
**Module:** carve, session
**Parameters affected:** B4 (deleted-file classification), D11 (node fields), D12 (session persistence), B9 (time handling), D15 (confidence grading)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md:79-90` (the carved/RAW IFileProperties serialization table: FLAGS "constant 0x0020 for all 12,147 records (every carved file flagged deleted)"; per-file formatted timestamp string written by `FUN_140c41aa0`; analyser checksum/validation u32 "+0x56 ... 0xFFFFFFFF = invalid/unknown marker (12,030/12,147 nonzero)"; tail-offset anchor B "+0x94"; type-registry ID "+0xF8 (35 for the shot*.jpg carve set)")
**Core references:** `src/core/carve/cr_carve.cpp:141-184` (the carve emission block: sets name, fs_kind, logical_size, carve_format, carve_confidence, extents, optional sha128 — sets **no** deleted state, **no** timestamps, **no** validation payload; `DeletedState::Active` default per `include/cr/tree/cr_node.h:38,153`); `src/core/session/cr_records.cpp:133,139-142,195,206-209,273` (the session faithfully persists whatever deleted-state/times exist — the gap is upstream); `src/core/carve/cr_carve.h:38-39` (hash gate: only `file_len <= hash_threshold && file_len <= MAX_FILE_READ_FOR_HASH`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Every carved file the og emits is flagged deleted (FLAGS 0x20 — constant across the 12,147-record QA census) and carries a scan timestamp plus a per-file analyser-validation payload. The core's carve emission sets none of these: carved nodes default to `DeletedState::Active` with zero times, and that misstatement persists through session save/load — a reloaded session misreports every carved file as a live, undated file.

## What the reference does

The RAW/generic IFileProperties serialization (ifileproperties-part3:79-90) shows, for the carved-record census: FLAGS u16@+0x22 is **0x0020 for all 12,147 records** — "every carved file flagged deleted" (carving recovers deleted content; the og's results model grades it as such); a **formatted timestamp string** is written per file by `FUN_140c41aa0` (`"%d-%d-%d %d-%d-%d"` from a FILETIME arg — the scan-time stamp shown in the og results grid for carved items); an **analyser checksum/validation u32**@+0x56 (12,030/12,147 nonzero, 0xFFFFFFFF = invalid/unknown marker) persists the validator's verdict; and a **tail-offset anchor B** qword@+0x94 (261/12,147 nonzero) plus **type-registry ID** u32@+0xF8 persist carve-provenance fields the results UI consumes (the Type column).

## What our core does

The carve emission block (`cr_carve.cpp:141-184`) constructs `FileProperties`, sets `name`, `fs_kind`, `logical_size`, `carve_format`, `carve_confidence` (capped 1000), the disk-absolute extent, optional `sha128` (only when `hash_small_files` is on AND `file_len <= hash_threshold` AND `<= MAX_FILE_READ_FOR_HASH`, per cr_carve.h:38-39) — and nothing else. `deleted_` keeps its `DeletedState::Active` default (cr_node.h:153); `times` stay zero. The session writer then faithfully serializes `deleted_state()` and the four zeros (`cr_records.cpp:133,139-142`), so the misstatement survives reload; `session_resume` re-materializes carved nodes as live, undated files.

## Gap

Falsifiable claim: for the same carved `shot0017.jpg`, the og's results/session record states deleted + a timestamp + a validator verdict; the core's node (and its session round-trip) states Active + zero times + no validation payload. The `Origin::Carved` flag (set at cr_carve.cpp:182) is the only deleted-content hint, and it is a provenance marker, not a classification.

## Impact

User-observable after any session save/resume and in any consumer that displays state/dates: every carved file presents as "existing, undated" rather than "deleted, scanned-at T". Downstream: report schema (`deleted_count` stats undercount), grading consumers (a future CRecoverability grader keyed on deleted-state would mis-grade carved rows), and the query model's Date buckets (carved files all fall in the "unknown date" bucket). The missing validation payload also loses the og's per-file validator verdict (0xFFFFFFFF-vs-nonzero distinction).

## Reproduction / verification

Corpus-derived (both artifacts first-cited this session): quotes above with exact lines/offsets. Core side read in-source at the audited commit; the default-Active and zero-times facts verified from `cr_node.h:38,153` and the emission block; the session round-trip verified from `cr_records.cpp`. Executed-verification option (not run): carve a fixture, save the session, reload, and print `deleted_state`/`times` — predicted Active/0,0,0,0.

## Fix plan (for the engineering team)

1. In the carve emission block (`cr_carve.cpp:141-184`): set `props->deleted_state = DeletedState::Deleted` (carved content is deleted content by construction — the og's constant 0x20), and stamp `props->times.modified` (or the dedicated scan-time slot the team prefers) with the scan wall-clock at emission time.
2. Persist the validator verdict: add a `carve_validated` (or reuse/extend `carve_confidence` semantics) recording whether the grammar validator accepted vs. the raw-sign layer only — the og's 0xFFFFFFFF-invalid vs. nonzero-verdict distinction.
3. Session layer needs **no change** (it already round-trips deleted-state and times once they are set); add a round-trip test to pin it.
4. Update the report schema docs if `deleted_count` semantics change for carve roots (SUMMARY/report.py consumers).
5. Tests: `Carve.DeletedStateDeleted` (fail-before: Active), `Carve.ScanTimestampStamped` (fail-before: 0), `Session.CarveDeletedStateRoundTrip` (save/load/assert), `Report.CarveCountsAsDeleted`.
6. Acceptance: `--json` on a carve-producing scan shows `"deleted": true` and a nonzero modified time for carved rows; the session round-trip preserves both.

## Tests to add

- `Carve.DeletedStateDeleted` — fails before (Active), passes after
- `Carve.ScanTimestampStamped`
- `Session.CarveDeletedStateRoundTrip`
- `Report.CarveCountsAsDeleted`

## Related findings

M-042 (archive inventories — same emission-block family); N-050 (node-field gaps: the type-registry ID and per-FS secondary payloads this finding's corpus table also evidences); N-030 (raw_signs confidence field — the og's dead confidence field vs. core's invented one; the validation payload here is the LIVE verdict field next to it).

## Open questions

The og's timestamp is a formatted STRING (the source FILETIME is not persisted — the artifact's own §5 correction). Whether the core should persist the FILETIME (cleaner, our format allows it) or match the string form is an engineering choice; the finding requires only that a timestamp exists.
