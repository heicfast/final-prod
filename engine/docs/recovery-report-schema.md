# Recovery Report JSON Schema (v1)

The machine-readable result of a recovery pass (G-09, issue #6). Emitted by
`rrecov recover --report <file.json>` and by the `cr_ui` shim
(`cr_ui_recover_report_json`); built by the pure-core serializer
`include/cr/recovery/cr_report.h` (`recovery::report::build_report`).

The document is **write-only** for the core (the core never parses reports
back). The serializer is a fuzz target for crash-freedom; strict JSON
escaping is guaranteed for arbitrary recovered names (invalid UTF-8 bytes
become U+FFFD so the document always parses).

## Wire format

```json
{
  "schema_version": 1,
  "engine_version": "<CMake PROJECT_VERSION>",
  "source": "<caller-supplied UTF-8 source identity>",
  "destination": "<caller-supplied UTF-8 destination identity>",
  "elapsed_ms": 1234,
  "totals": {
    "queued": 5, "done": 2, "dirs": 1, "refused": 1,
    "errors": 0, "hash_mismatch": 0, "resumed": 1, "bytes": 4096,
    "failed_bytes": 512
  },
  "items": [
    {
      "item_id": 1,
      "path": "out/dir/photo.jpg",
      "name": "photo.jpg",
      "kind": "file",
      "outcome": "done",
      "reason": 0,
      "bytes": 1024,
      "duration_ms": 12,
      "sha256": "aa11...",
      "hash_status": "computed",
      "tree_class": "lost_files_reconstructed"
    }
  ]
}
```

Field order is fixed (the serializer is deterministic). Compact separators
(no space after `:`); two-space indentation for the top level, `items` rows
indented with three spaces. Trailing newline.

## Field semantics

Top level:
- `schema_version` — always `1` for this document shape. Breaking changes
  bump it; additive fields may appear within a version (consumers must
  tolerate unknown keys).
- `engine_version` — the `core_recovery` build version.
- `source`, `destination` — caller-supplied UTF-8 identities (for the CLI:
  the session path and the destination directory).
- `elapsed_ms` — the recovery pass duration (pause/cancel compensation
  does not apply here; this is pure writer wall time).

`totals` — aggregate counts over the queue (all integer):
- `queued` — items in the run (stamped upfront; true even for an aborted pass)
- `done` — files committed (content written + renamed); includes the
  `hash_mismatch` and `salvaged` sub-buckets (both are commits)
- `dirs` — directory items materialized (G-08; counts mkdir successes)
- `refused` — refused before any write (e.g. destination-overlap, F12)
- `errors` — write/commit failures
- `hash_mismatch` — committed, but the stored hash did not match
- `resumed` — pre-existing finals counted + skipped (idempotent re-runs)
- `salvaged` — committed with zero-filled spans (FB-R02: unreadable
  device ranges / undecodable blocks; the byte count is right, not
  every byte is recovered content) — an ours-only truth marker, no og
  counterpart (og neither hash-verifies nor marks partial commits)
- `nothing_to_recover` — GAP-S6-7: skipped BEFORE any write; the node
  claimed a positive size but carried no extents and no resident data
  (metadata-only reconstruction). Nothing was written and nothing was
  attempted - the zero-fill Skip policy exists for PARTIAL spans of
  real files, not whole no-extent items (bench 40073093997: 991
  full-size zero-file false positives). The og emits no content for
  metadata-only entries.
- `read_error` (M-135, additive-within-v1) — committed-but-truncated
  files: the og's error-9 family (a hard read failure after retries
  truncates the item at the first unreadable byte, the PARTIAL stays on
  disk, and the file is in the failed list with its numbered cause -
  DRWUI :875684-875696). A separate bucket from `done` (the og counts
  these files failed) and from `errors` (the partial IS committed).
- `pending` — never attempted in this pass (partial-run truth; the
  reconciliation is queued == done + dirs + refused + errors +
  resumed + nothing_to_recover + read_error + pending, since done
  subsumes the committed sub-buckets)
- `bytes` — sum of `bytes_total` over committed (non-resumed) file items
- `failed_bytes` (N-159, additive-within-v1) — the og's failed-files
  accumulator (+0x268 on the results singleton, DRWUI :642729-642746):
  the bytes of files that did NOT commit. See truth rule 5.

`items[]` — one row per queued item, in queue order (`item_id` monotonic):
- `item_id` — the queue id (stable within the run; not a tree node id)
- `path` — UTF-8 destination path, `/`-joined, relative to the destination
  root supplied to the writer
- `name` — final (collision-resolved) name
- `kind` — `file` or `dir` (dir rows are structure items: mkdir, no content)
- `outcome` — one of:
  - `done` — content committed / directory created
  - `refused` — refused before any write (`reason` carries the code)
  - `error` — write/commit failed (`reason` carries the code)
  - `hash_mismatch` — committed, stored hash did not match (`reason` set)
  - `resumed` — final pre-existed; counted + skipped, never re-written
  - `pending` — **not attempted**: the pass aborted before this item
    (truthful reporting of a partial run - never reported as done)
  - `salvaged` — committed with zero-filled spans (FB-R02; ours-only
    truth marker - see totals.salvaged)
  - `nothing_to_recover` — GAP-S6-7: skipped before any write (metadata-
    only item: no extents, no resident data, positive claimed size);
    no file was created and no bytes were attempted
  - `read_error` (M-135) — committed-but-truncated: the read failed hard
    (post-retry) and the item truncated at the first unreadable byte;
    the partial file EXISTS on the destination and the row carries the
    numbered cause (`cause_id`) + the boundary (`first_failed_offset`)
    + the committed size (`bytes_written`) - the og's error-9 family
- `reason` — `cr_error` code (hex value as integer); `0` for clean rows
- `bytes` — logical size attempted (0 for dirs)
- `cause_id` (M-135, additive-within-v1; `read_error` rows only) — the
  og's numbered user-facing failure cause, the COMPILED-IN defaults
  (RecoveryFailedInfo1..7, DRWUI strings :30580-30596): 1 insufficient
  storage space, 2 recovery path too long, 3 garbage characters in the
  name, 4 the sector reading location exceeds total sector number of
  the source partition, 5 the correct storage sector location cannot be
  found, 6 exceeded the free quota (never produced by the core), 7
  other unknown reasons
- `first_failed_offset` (M-135; `read_error` rows only) — the logical
  offset of the first unreadable byte (the truncation boundary)
- `bytes_written` (M-135; emitted when it differs from `bytes`) — the
  committed byte count (the og's std::filesystem size probe,
  DRWUI :874131-874205)
- `duration_ms` — per-item processing time (0 for skipped/unprocessed)
- `sha256` — lowercase hex of the streamed content digest; **empty** when
  not computed (refused/error/pending/resumed/dir rows)
- `hash_status` — one of:
  - `verified` — a stored hash existed and matched
  - `computed` — the writer streamed a digest; no stored hash to compare
  - `mismatch` — a stored hash existed and did not match
  - `not_computed` — no digest exists for this row (never faked)
- `tree_class` (N-159, additive-within-v1; **omitted when empty**) — the
  results-tree classification the og's failed-files HTML report carries
  per row (`CSaveFailedFilesWithHtml`, DRWUI :642755-642822). Derived from
  the item's node's root branch; wire values map to the og's row labels:

  | wire value | og row label (DRWUI :642766-642805) | core root role |
  |---|---|---|
  | `existing_files` | "Existing Files" (group 0, kind 4) | `Found` |
  | `lost_files` | "Lost Files" (group 1) | `File Path Lost` / `Lost Files` |
  | `lost_files_reconstructed` | "Lost Files" + "Reconstructed" (group 1, kind 7) | `Reconstructed` (the S-048 default) + the legacy carved role |
  | `lost_files_tags_extracted` | "Lost Files" + "Tags Extracted" (group 1, kind 6) | `Tags Extracted` (M-101's branch — mapping lands ready) |
  | `lost_files_deleted` | "Lost Files" + "Deleted" (group 0, kind 0) | `Deleted` |
  | `<custom name>` | the custom raw-category name (FUN_14008a650 != 0) | any other configured root role |

  The og surfaces the axis only on its failed-files funnel; the core
  emits it on EVERY row (the structured-superset shape, D13). The exact
  HTML joiner between the og's compound label parts is
  .rdata-unresolved (N-159's open question) — the wire values are the
  stable snake_case forms, never an invented compound.

## The optional scan_analysis block

`scan_analysis` (present when the run had a grading context — the CLI's
recover verb copies it from the loaded session) carries the og
RecoveryAssistant's post-scan statistics plus the per-format carve
breakdown (N-140 — the og's `GetRawUserInfoMap` surface, RAWSearchDLL
:72213-72221; DRWUI renders the "found N .jpg / M .png" breakdown at
:227538-227625):

- The 8 file-count counters, `categories` (the 8 query categories), the
  gauge pair, `score`, `ntfs_device_index_high` — the S-023 fields.
- `carve_formats` — an array of `{"id": i, "count": n}` rows in unified
  format-id order (grammar registry rows first, then raw-sign rows, then
  the aggregate user-sign slot), NONZERO rows only. `id` maps through
  `cr_ui_carve_format_info` for every id below
  `cr_ui_carve_format_count()`; the final id is the user-sign aggregate.
  Absent (empty array) when no carve stage ran or the session predates
  the tally (additive-within-v1; consumers must tolerate the key's
  absence in older documents).
- `carve_format_overflow` — formats beyond the fixed tally capacity
  (future registry growth; 0 today — surfaced, never silently dropped).

## Truth rules (non-negotiable)

1. A hash is reported **only** where the writer actually computed one —
   `hash_status` never claims verification that did not happen.
2. An item the pass never reached is `pending`, never `done` or `error`.
3. `resumed` items were not re-written; `bytes` in totals excludes them.
4. Exit-status mapping stays in the caller (CLI: errors or mismatches →
   exit code 2; the report is still emitted first).
5. (N-159) `failed_bytes` counts every non-committed file's logical size
   (error rows, including the name-exhausted class; in-queue refusals).
   Enqueue-time refusals never entered the queue and never attempted a
   byte — they are excluded, exactly like the og's export loop (no
   pre-export refusal class exists there). `nothing_to_recover` and
   `pending` rows never moved bytes and stay excluded.

## Validation

CI (Linux job) validates the emitted document with
`tools/validate_report.py` against this schema. The unit test
`Report.EmitsCiSchemaSample` writes `/tmp/cr_report_sample.json`
containing every outcome and hash status; the validator proves:
required keys present, enum spellings, types, sha256 shape
(`^[0-9a-f]{64}$` or empty), and `items[]` count/`queued` consistency.
