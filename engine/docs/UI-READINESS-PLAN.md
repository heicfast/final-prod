# UI-Readiness — final core disposition (G-01…G-23)

> **Final status.** The design and acceptance criteria below are retained as
> evidence, not as a forward-looking work queue. G-01 (`cr_ui`), G-02 immutable
> snapshots/value enqueue, G-12 query/tree JSON, G-13 checkpoints and the
> landed G-04…G-10/G-14/G-19…G-23 surfaces are implemented and tested. G-11,
> G-15 and G-16 are deliberate report-only/absent safety or scope decisions.
> G-18 (ZFS multi-vdev) remains PARTIAL in `CORE-GAP-LIST.md`. G-22 reports
> computed post-write hashes honestly; it never claims comparison against an
> unavailable stored source digest. The explicit CLI session surface and its
> Windows contract tests landed in `77ef393`.

> **Status: PLAN.** Authored by the core UI-readiness sub-agent from a line-by-line
> read of this repo @ `2473c03`, the `rda-wizard` contract (`docs/CORE-CONTRACT.md`
> §2/§14 + `AUDIT-UI.md` §3 @ `015392b`), the RE corpus (`easeus-drw-research` @
> `18d6f14`, see `/reports/RE-PARITY.md` surface report), and the GitHub issues
> #1–#10. No implementation code precedes this plan. Research surface reports:
> CORE-SURFACE, CLI-SURFACE, RE-PARITY, BUILD-SURFACE (session-local; key
> conclusions embedded below).

**Parity posture (RE-PARITY, evidence-cited):** the og has (a) real in-scan pause
(`SetPaused(1)` + pause-duration compensation), (b) exactly ONE auto-save after
"Build tree finish", (c) NO partial-scan resume ("Continue last scanning" = load a
completed session), (d) NO machine-readable recovery report (dialog + trace log
only), (e) stub-plugin preview over bounded range reads (100 MB / 15 s caps),
(f) client-side filtering. Therefore: our **pause, save-on-cancel, JSON report,
bounded preview reader, core-side query API are improvements, not parity** — they
are documented as such everywhere they are claimed. Our **resume-from-checkpoint
is an improvement beyond the og** (the og never resumes an interrupted scan).

**Design baseline (headers win, verified in this tree):**

- All engines already accept `bool (*)(u64 done, u64 total, void*)` progress
  callbacks; the orchestrator passes `nullptr` everywhere except the NTFS walk
  (`cr_scan.cpp` engine-pass trampoline). FAT/exFAT call their progress slot
  only once at completion (`cr_fat_quick.cpp:321`, `cr_exfat_quick.cpp:472`);
  ext4/HFS+/btrfs/ZFS/ReFS/carve/lost have real call sites.
- `save_session` writes `stage_done_mask` from the **config flags**, not actual
  completion (`cr_scan.cpp:573`) — must be fixed before `resume()` can exist.
- `ScanController` already tracks `quick_done_` privately — resume hooks fit.
- `ResultTree` mutates only by append (`add_root`/`add_file`), `reparent`, and
  `refresh_stats`, all under `mutex_`; traversal links are written during the
  append window → unsynchronized traversal mid-scan is a data race (G-02).
- The writer computes SHA-256 of every recovered stream (`recover_item`) but
  discards the digest after verification — G-22's report can expose it honestly.
- `rrecov scan` passes `CancelToken* = nullptr` and never saves on cancel;
  `list` exits 0 even when enumeration fails (G-20).
- Sessions: `load_session` restores tree + files_found + error map only; ignores
  VOLUME_INFO / PARTITION_CONTEXT (G-23, G-14).

---

## Issue → design → acceptance → commit plan

### #1 (G-01) — `cr_ui` C-ABI shim DLL

**Design.** New CMake target `cr_ui` (SHARED library: `cr_ui.dll` on Windows,
`libcr_ui.so` on Linux) over `cr_core` (+ `cr_win` on Windows). Public header
`include/cr/cr_ui.h`: C11, `CR_UI_API` export macro (`__declspec(dllexport)` /
`__attribute__((visibility("default"))`)), every type opaque, all functions
`cr_ui_*` returning `u32` `cr::Err` codes (same numeric space as `cr_error.h`).
No C++ types cross the boundary; no exceptions escape (catch-all per entry);
strings are UTF-8 `const char*` in, and returned via caller buffers with the
two-call size-query pattern or `cr_ui_string_free` for heap strings (ownership
documented per function). Progress: C callback
`void (*)(void* ctx, u32 stage, u32 stage_permille, u32 overall_permille,
u64 bytes_done, u64 bytes_total, u32 error_ranges)` — called from the scan
thread only; must not block; no cancellation semantics (a separate token is
the cancel path). ABI: `cr_ui_abi_version()` returns `CR_UI_ABI_VERSION`
(bump-on-breaking-change policy documented in the header); `cr_ui_last_error()`
thread-local diagnostic string. Coverage: scan lifecycle (create/config/run/
cancel/pause/resume), telemetry query, tree snapshot + node enumeration, query/
filter/sort/paging, bounded preview read, session save/load, recovery lifecycle
+ per-item progress + JSON report, health (flat struct, not JSON), disk/volume
enumeration (UTF-8 flattening of the win layer). A small `cr_ui_abi_check`
C-consumer target builds against the header with a plain C compiler (gcc on
Linux; MSVC CI job on Windows) and exercises the full lifecycle.

**Acceptance.** Linux: `.so` builds `-Wall -Wextra -Werror`, C consumer
lifecycle test runs green under ASan. Windows: MSVC `/W4 /WX` build + DLL smoke
job in CI green. Win7 import gate: port `rda-driver`'s `stamp_osversion.py` +
`verify_pe.py` to `scripts/` (user-mode allowlist: kernel32, advapi32, cfgmgr32,
user32 none, msvcrt/vcruntime per CRT policy) and gate in the Windows job.

**Commits.** W2-1 (header + target + lifecycle subset), W2-2 (snapshot/query/
preview/recovery/report/session/health), W2-3 (C consumer + CI jobs + PE gate).

### #2 (G-02/G-03) — mid-scan immutable snapshot + snapshot-based recovery

**Design** (ADR to follow as `docs/adr/ADR-UI-SNAPSHOT.md`). Copy-on-write
**value snapshot**: `cr::tree::Snapshot` — a flat array of `SnapNode` records
(name, sizes, times, attributes, origin/deleted/confidence, carve fields,
record_ref, fs_kind, extents, resident_data) plus index arrays for
parent/first-child/next-sibling (contiguous u32 indices, not pointers), built
under `ResultTree::mutex_` in one O(n) pass. Rationale vs alternatives:
a reader-writer lock would serialize long-lived UI traversals against engine
emitters (throughput death, priority inversion); epoch/link-stability tricks
remain C++-UB data races under TSan because `add_file` writes `next_sibling_`
of an already-visible tail node. A value copy is provably race-free (all reads
under the lock; the snapshot owns every byte it exposes), matches the
stage-boundary refresh model the UI already implements (AUDIT ADR-2), and its
O(n) cost equals one `serialize_tree` pass — paid at stage boundaries or on
explicit demand. The orchestrator refreshes a live snapshot handle at every
stage boundary; `cr_ui_tree_snapshot()` mid-stage takes the lock and copies
(stalls emitters for the copy duration — documented; at 1M nodes ≈ 150–300 ms
measured, test-verified). Flags: `SNAP_PARTIAL` set while a stage is mid-flight
(the API never pretends mid-scan recovery is final-state). **G-03**: recovery
from a snapshot is race-free by construction because `SafeWriter` gets a new
value-driven enqueue (below) reading only snapshot-owned data + the source
device (whose `read_at` is contract-safe concurrently).

**Acceptance.** TSan test: scan a fixture volume while a second thread takes
snapshots + enumerates + queries — zero races; recovery-from-snapshot during
scan produces byte-identical output to post-scan recovery of the same nodes
(determinism test). ASan clean. `SNAP_PARTIAL` reported correctly mid-stage.

**Commits.** W1-4 (snapshot class + tree integration + ADR + TSan test),
W1-5 (writer value-enqueue + recovery-from-snapshot test).

### #3 (G-04/G-05/G-06) — uniform observability

**Design.** `cr::orchestrator::ScanTelemetry` (owned by `ScanController`,
`std::mutex`-guarded, copied out on query — callable from any thread mid-scan):
per-stage `{permille, units_done, units_total, unit_kind (records/entries/
bytes), bytes_done, elapsed_ms}` for quick/deep/carve + candidate count +
live `ReadErrorMap` snapshot + monotonic overall permille. Wiring:
(a) orchestrator passes a stack-context trampoline to EVERY engine call site
(today `nullptr`): quick engine pass, deep remnant sweep, `lost::discover`,
per-candidate scans, carve; (b) FAT/exFAT engines gain real incremental
progress emission (per dentry/cluster-window, replacing the single end-tick);
(c) `RetryingDevice` gets a mutex-guarded error-map copy accessor; the
orchestrator merges it into telemetry at progress ticks and stage boundaries;
(d) `bytes_scanned` stays carve-true-bytes (honest); structured stages report
unit progress (MFT records, dentries, inodes) + elapsed — **ETA derives from
permille + elapsed**, documented (no fabricated bytes). `ScanStats` keeps its
current meaning (contract-stable).

**Acceptance.** Synthetic slow-engine test: progress fractions monotonic and
matching weights across stages; mid-stage telemetry query returns sane values
while a scan runs (TSan test); FAT fixture emits >1 intermediate progress
calls; error-map ranges appear in telemetry before run() returns.

**Commits.** W1-3 (telemetry + orchestrator/engine wiring + FAT/exFAT ticks).

### #4 (G-07) — pause/resume + save-on-cancel

**Design.** Three coordinated pieces:
1. **In-engine pause**: `CancelToken` gains `virtual bool checkpoint() const
   { return cancelled(); }` (default = old behavior — zero change for existing
   tokens/implementations). `cr::PausableToken : CancelToken` overrides
   `checkpoint()` to sleep on a condvar while paused, returning `cancelled()`
   on wake. Engine loop heads swap `cancelled()` → `checkpoint()` (mechanical,
   1 line per site, no semantic change when unpauseable token absent). The
   orchestrator calls `checkpoint()` between stages too. Pause compensates
   elapsed (RE-parity: og behavior) by excluding paused intervals from
   `elapsed_ms` (telemetry measures run-time, not wall-time).
2. **Resume**: `ScanController::resume(session_path_io, …)` — new API
   (path-agnostic: takes the already-opened `session::Reader`, consistent with
   the pure-core IO-callback model): loads the session (tree + stats + the
   newly restored state below), validates `stage_done_mask` integrity (mask ⊆
   configured-stages of the stored `VolumeInfo.flags` — mismatch ⇒
   `ERR_SESSION_FORMAT`, never a silent partial), reconstructs a config with
   completed stages cleared, sets `quick_done_` from the mask, and continues
   the pipeline. `save_session` is FIXED to stamp actual completion
   (`quick_done_`, remnant/lost done, carve done) instead of the config mask.
3. **Save-on-cancel**: `ScanController::run` already leaves the tree intact on
   `ERR_CANCELLED`; the CLI saves when `--out` was given regardless of rc
   (success grammar byte-identical; cancel path prints the additive line
   `session saved (cancelled): <path>`). The shim mirrors it.

**Acceptance.** Fixture scan cancelled mid-deep: session loads, resume skips
the quick pass (MFT records not re-walked — count walk invocations in the
fixture), completes; corrupt-mask session ⇒ `ERR_SESSION_FORMAT`; pause test:
telemetry elapsed excludes paused wall-time; CLI cancel writes a loadable
session. `docs/architecture.md` D-3 corrected.

**Commits.** W1-1 (checkpoint + PausableToken + engine loop swap + pause
elapsed compensation), W1-2 (stage-mask truth + resume + save-on-cancel + D-3).

### #5 (G-08) — writer directories + empty folders

**Design.** `DestIo` gains `virtual u32 mkdir(const std::u16string& path)` =
recursive create with `already-exists` tolerance (`CreateDirectoryW` chain;
Linux test fake: `::mkdir -p` semantics). `WriterConfig::reconstruct_dirs =
true` (default on): (a) `recover_item` ensures the item's `dest_dir` exists
before `create_temp` (cached set — one syscall-tree walk per unique dir);
(b) directory NODES become enqueueable: `SafeWriter::enqueue` accepts dir nodes
(`attributes & 0x10`), marks the item `ItemState::Done` after mkdir with
`bytes_total = 0` — empty dirs materialize as real directories; (c) the CLI
`recover` walk (and the shim) enqueue every dir on the selected path, not just
file parents, so empty leaves survive. Overlap refusal unchanged (dirs have
no extents — never refused).

**Acceptance.** Structural test: fixture tree with nested + empty dirs →
recover to temp dest → directory structure compares equal (including empties);
nested dest path that does not pre-exist succeeds (no `ERR_DEST_UNWRITABLE`);
idempotent re-run unchanged.

**Commits.** W1-5 (mkdir + dir enqueue + CLI/shim walk + structural test).

### #6 (G-09) — recovery report JSON

**Design.** New pure-core `recovery/cr_report.h`: `build_report(const
std::vector<ReportItem>&, const ReportTotals&, identities, elapsed) →
std::string` (JSON, hand-rolled serializer with strict string escaping —
fuzz target #1). `QueueItem` gains `duration_ms` + `sha256_hex[64]` (captured
from the writer's always-on stream hash; `hash_status ∈ {verified, computed,
mismatch, not_computed}` — verified only when a stored hash existed and
matched; `not_computed` for refused/error items: never fake). Report schema
versioned (`"schema_version": 1`, documented in
`docs/recovery-report-schema.md`): per-item `{node_id, path, outcome ∈
{done, refused, error, hash_mismatch, resumed}, reason, bytes, sha256,
hash_status, duration_ms}`; totals `{queued, done, refused, errors,
hash_mismatch, resumed, bytes}`; identities `{source, destination}` (UTF-8,
caller-supplied), `engine_version` (CMake-injected), `elapsed_ms`. CLI:
`rrecov recover --report file.json` (additive flag; stdout grammar unchanged
except the additive line `report: <path>`). CI validates the emitted file
against the schema (python checker in the linux job).

**Acceptance.** Unit: round-trip escape tests (quotes, control chars, UTF-8
boundaries); fuzz: serializer + a report JSON reader? — the report is
write-only (no core parser) → fuzz the serializer for crash-freedom only.
Integration: recover fixture with mixed outcomes → every queue item appears
with correct status; schema validation green.

**Commits.** W1-6 (QueueItem capture + report module + schema doc + tests).

### #7 (G-10) — bounded preview reader + metadata extractors

**Design.** New pure-core `preview/cr_preview.h`:
- `PreviewReader::open(IStorageDevice* source, const props-view)` then
  `read_at(u64 logical_offset, void* buf, u32 len)`: walks resident_data
  first, then extents (compressed extents decoded via `decompress_block`,
  256 MiB guard like the writer); bounded by `logical_size`; short reads +
  EOF code; read failures zero-fill (Skip policy, matching writer semantics)
  and are REPORTED via a per-reader `unreadable_spans` map (truthful, not
  silent). No full-file materialization (chunked, `len` capped at 4 MiB/call).
- `preview::Metadata extract_metadata(const PreviewReader&, caps)` — bounded
  (default: 4 MiB read cap, 50 ms-ish work caps via unit counters): image
  dimensions (JPEG SOF, PNG IHDR, GIF, BMP, TIFF IFD0, WEBP), JPEG EXIF
  (orientation, datetime, dimensions — bounded APP1 walk), MP4/MOV duration
  + brand (bounded `moov/mvhd` search window), WAV/PDF/DOCX text snippets
  (zlib already linked; DOCX = bounded zip central-directory + inflate;
  PDF = bounded stream text). Unknown/oversized → `kind: "unsupported"` with
  reason. All parsers: length-guarded, no recursion, no allocation proportional
  to claimed sizes — **fuzz targets** (jpeg/png/exif/mp4/docx/pdf harnesses
  added to the existing fuzz suite).

**Acceptance.** Fixture files: exact dimension/EXIF/duration ground truth;
fuzz: all new harnesses 45 s zero-crash in CI; bounded-read test: a node
claiming 1 TiB logical size with a 1 KiB extent list cannot make the reader
read more than `len` bytes from the device (device-counting fake).

**Commits.** W1-7 (reader + extractors + fixtures + fuzz harnesses).

### #8 (G-12) — core-side query/filter/sort/paging

**Design.** New pure-core `query/cr_query.h` operating on the **snapshot**
(and on a session-loaded tree via the same snapshot path — one materialization
serves both): composable `QueryFilter {category_mask (extension→category map
core-side), date_range (modified, FileTimes), size_range, confidence_min,
extension_list, name_substring (UTF-8, case-insensitive), deleted_only,
origin_mask}`; `sort_key ∈ {name, size, modified, confidence, path}` +
`sort_dir`; `offset/limit` + `count_only`. Results: `u64 total_matched` +
paged `QueryResultRow {node_ref (snapshot-stable), name_utf8, size, times,
origin, deleted, confidence, category}`. Implementation: single linear pass
with predicate short-circuit; sort materializes an index vector only when a
sort key is requested. `rrecov tree` gains `--json` (same rows as `--csv`,
documented key order) — `--csv` column order and enum values stay
byte-identical (frozen contract).

**Acceptance.** Perf test (new `test_query_perf.cpp`, runs in CI linux job):
1M synthetic snapshot nodes, filter+count < 200 ms (measured, printed);
sorted paged query < 600 ms (documented, not part of the 200 ms gate);
correctness tests vs brute-force reference implementation on randomized
fixtures.

**Commits.** W1-8 (query module + tests + perf test), W2-2 (CLI `--json`).

### #9 (G-13) — auto-checkpoint

**Design.** `ScanConfig` gains `CheckpointSink { void (*sink)(void* user,
session::Writer& w); void* user; }`-style hook (concretely: a small interface or
fn-pointer + user) invoked by the orchestrator **at stage boundaries** with a
freshly serialized session (state + volume + tree + error map). The shim/CLI
provide a file-backed sink writing `<path>.tmp` + `FlushFileBuffers` +
`MoveFileExW` (atomic on NTFS; `fsync`+`rename` on Linux) — atomic
crash-resilience per the prompt. `resume()` picks up any valid checkpoint file
(reader tolerates truncated tail records by construction — skip-forward).
Corrupt container magic/version ⇒ `ERR_SESSION_FORMAT` surfaced to the caller
(documented: delete the checkpoint and restart the scan).

**Acceptance.** Crash test: run scan with checkpoint sink to a real file,
`kill -9` the process mid-deep-stage (fixture-driven), resume from the
checkpoint: quick stage skipped (no re-walk), results from before the crash
present. Corrupt-magic checkpoint → `ERR_SESSION_FORMAT`.

**Commits.** W1-9 (checkpoint sink + orchestrator hooks + crash-resume test).

### #10 (umbrella) — G-11…G-23 disposition

| Gap | Disposition in this session |
|---|---|
| G-11 (.rsf import) | **By-design absent** (documented; CRSF is our format; the corpus `.rsf` reversal informs CRSF only). No code. |
| G-14 (VolumeInfo sparsity) | **Fix**: `save_session` fills `slice_offset/bytes/sectors/label/elapsed_ms` from the slice geometry + config; header comment D-1 corrected (fs_kind stays FsFamily on the wire — changing the wire namespace would break existing sessions; documented instead, load maps it into the new `SessionInfo.fs_family`). |
| G-15 (TRIM toggle) | Report-only stays (by design, safety). No code. |
| G-16 (USB SMART) | Report-only posture documented in README matrix; `ERR_HEALTH_UNAVAILABLE` is the honest degradation. No code. |
| G-17 (Windows CI) | Windows DLL smoke job + PE import gate added (W2-3). Runtime verification on real Windows remains CI-only — documented. |
| G-18 (ZFS multi-vdev) | By-design scoping, already surfaced honestly. No code. |
| G-19 (LDM multi-disk) | Already resolved via `LdmDiskResolver` + cross-disk registry (verify + document in README matrix). No new code. |
| G-20 (`list` exit code) | **Fix**: exit 1 when enumeration fails (stdout grammar unchanged; UI never trusts the exit code today — safe, coordinated CORE-CONTRACT update with the UI agent in the same push window). |
| G-21 (`recover --volume`) | **Fix**: `--volume X:` resolves via `win::volume_letter_to_disk` + `volume_extent`, then the existing `--disk` path (extents absolute — offset mapping identical to scan's slicing). |
| G-22 (structured hashes) | **Partial, honest**: writer's computed digest lands in the report as `hash_status: "computed"` (never "verified" without stored truth). Full stored-hash emission is engine work, tracked as remaining scope on #10. |
| G-23 (session restore) | **Fix**: `load_session` restores `lost_candidates` (PARTITION_CONTEXT) + `SessionInfo` (VOLUME_INFO: fs_family, geometry, elapsed, flags) via a new out-struct; shim exposes it. |

**Acceptance.** Each fix row carries its own test; disposition rows carry
doc updates only (README capability matrix + roadmap + CORE-CONTRACT PR).

**Commits.** W1-10 (G-14 + G-23 + D-1 comment), W2-4 (G-20 + G-21 + contract
coordination), W3 (docs).

---

## Commit waves (each green + pushed before the next starts)

- **W1 — pure-core semantics** (Linux-verifiable): W1-1 pause, W1-2 resume +
  save-on-cancel, W1-3 telemetry, W1-4 snapshot + ADR, W1-5 writer dirs +
  value-enqueue, W1-6 report, W1-7 preview, W1-8 query, W1-9 checkpoint,
  W1-10 session restore. Full suite + ASan + TSan after each.
- **W2 — surfaces**: W2-1/2-2 `cr_ui` header + DLL + C consumer, W2-3 Windows
  CI + PE gate, W2-4 CLI flags (`--report`, `tree --json`, `--volume`, list
  exit, save-on-cancel wiring, Ctrl-C token).
- **W3 — contract + docs + release**: coordinated `rda-wizard`
  CORE-CONTRACT/AUDIT-UI updates (§2/§13/§14 deltas via the UI agent's
  process — PR/issue comments), core docs (architecture D-3, ground-truth
  extension, roadmap, README matrix, recovery-report-schema), issue #1–#10
  updates with evidence, tag `v0.2.0-ui`, worklog handoff with pinned commit.

## Non-negotiables carried through every wave

- No stubs, no TODOs, no fake data; unimplemented surfaces are absent, not
  degraded silently.
- CLI stdout grammar frozen (byte-identical) for all UI-parsed lines; every
  addition is additive and documented.
- Existing sessions stay loadable (CRSF v1 grammar unchanged; new fields only
  inside existing record types where the v1 reader tolerates them, or in new
  record types the old reader skips).
- Thread safety proven: TSan suite extended for snapshot, telemetry, query,
  pause, checkpoint sink.
- `main` stays green; pull + rebase before every push (parallel agents
  active).
