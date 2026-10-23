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
    "errors": 0, "hash_mismatch": 0, "resumed": 1, "bytes": 4096
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
      "hash_status": "computed"
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
- `done` — files committed (content written + renamed)
- `dirs` — directory items materialized (G-08; counts mkdir successes)
- `refused` — refused before any write (e.g. destination-overlap, F12)
- `errors` — write/commit failures
- `hash_mismatch` — committed, but the stored hash did not match
- `resumed` — pre-existing finals counted + skipped (idempotent re-runs)
- `bytes` — sum of `bytes_total` over committed (non-resumed) file items

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
- `reason` — `cr_error` code (hex value as integer); `0` for clean rows
- `bytes` — logical size attempted (0 for dirs)
- `duration_ms` — per-item processing time (0 for skipped/unprocessed)
- `sha256` — lowercase hex of the streamed content digest; **empty** when
  not computed (refused/error/pending/resumed/dir rows)
- `hash_status` — one of:
  - `verified` — a stored hash existed and matched
  - `computed` — the writer streamed a digest; no stored hash to compare
  - `mismatch` — a stored hash existed and did not match
  - `not_computed` — no digest exists for this row (never faked)

## Truth rules (non-negotiable)

1. A hash is reported **only** where the writer actually computed one —
   `hash_status` never claims verification that did not happen.
2. An item the pass never reached is `pending`, never `done` or `error`.
3. `resumed` items were not re-written; `bytes` in totals excludes them.
4. Exit-status mapping stays in the caller (CLI: errors or mismatches →
   exit code 2; the report is still emitted first).

## Validation

CI (Linux job) validates the emitted document with
`tools/validate_report.py` against this schema. The unit test
`Report.EmitsCiSchemaSample` writes `/tmp/cr_report_sample.json`
containing every outcome and hash status; the validator proves:
required keys present, enum spellings, types, sha256 shape
(`^[0-9a-f]{64}$` or empty), and `items[]` count/`queued` consistency.
