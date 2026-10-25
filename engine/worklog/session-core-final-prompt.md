# Session: core-final (production-complete push)

Saved: 2026-10-25. Token redacted. This is the operative session prompt.

## Mandate

Finalize `heicfast/core_recovery` to production-complete state: implement every
remaining module, algorithm, visualization engine, and API surface that exists in
the research corpus and og-codebase; deliver a stable, production-grade C ABI shim
suitable for UI consumption; achieve real-time progress and state updates;
implement the local persistent database layer; and prove performance and
behavioral parity against the reference on every parameter.

Two repositories are in scope and only two:
- Core (target): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token: REDACTED (env/git config only, never committed, never logged)

Do not clone or reference the UI repository, the driver repository, or any other
repository. Do not guess reference behavior. Read the corpus.

## 1. Read before writing

Read the research corpus line by line: every architecture document, every RE raw
source file, every decompiled artifact; every artifact documenting engine
behavior, state machines, error codes, progress semantics, session formats, and
visualization intent; the og-codebase installed tree for direct behavioral
verification; any `.rsf`-format artifacts and recovery-flow documentation.

Read the core repository line by line: every header (`inc/cr_*.h`), every source
file, every test, every workflow, every doc, every build file.

Reverse-engineer where the corpus does not answer a specific question: use
Ghidra, `objdump`, `strings`, ResourceHacker, `ntfs-3g`, `btrfs-progs`, ZFS
tools, and `7z` directly on the corpus binaries. Do not assert behavior without
evidence. Mark anything unconfirmable as `UNVERIFIED` and resolve it with an
independent probe or track it precisely.

Deliverable before any code change — commit and push gate documents in `docs/`:
- `docs/CORE-GAP-LIST.md` — every function, feature, algorithm, module, engine,
  constant, offset, enum, error code, and state transition in the reference that
  belongs in core, with verdict MATCH / MISSING / WRONG / PARTIAL and evidence
  reference.
- `docs/ABI-CONTRACT.md` — the proposed C ABI surface, with ownership,
  threading, error model, and stability policy.
- `docs/PERFORMANCE-PROFILE.md` — baseline measurements of the current build on
  the reference scenarios.

Do not skip reading. If a file is large, chunk it and take structured notes.

## 2. Implement everything on the gap list

Every MISSING, WRONG, or PARTIAL is closed this session. No exceptions.

Engines and algorithms: NTFS (full $MFT reconstruction, $ATTRIBUTE_LIST,
$Bitmap grading, hard links, ADS, remnant scan), FAT12/16/32 (LFN, chain
validation, backup boot), exFAT (boot-region checksum, NoFatChain, entry-set
checksum), ext4 (superblock, GDT, inodes, extent trees), HFS+ (volume header,
catalog/extents B-trees, fork model, orphan sweep), Btrfs (superblock, chunk
mapping, tree walk, subvolume roots), ZFS (label nvlist, uberblock ring,
MOS/dataset/ZPL, dnode), ReFS (full MSB-chain — first-class, not report-only),
LDM/dynamic disks (full recognition and parsing), lost-partition discovery,
virtual-volume reader, carving (signature registry, validators, merge/dedup,
byte-order correctness), session (CRSF save/load/resume/checkpoint), writer
(overlap refusal, mkdir, empty dirs, collision naming, hash verification, JSON
report), health/SMART (ATA/NVMe/TRIM/SSD), RawFixer (transactional repair with
undo).

Visualization engines: implement the disk-map and allocation-visualization model
the reference exposes — cluster/sector allocation maps, confidence overlays,
bad-sector maps, and the data shapes required to render them at scale. The
visualization engine is a core data producer, not a UI component: it emits the
geometry, allocation state, confidence grading, and error overlays that a
front-end consumes. Read the corpus for the exact model; do not invent.

Orchestration: staged quick→deep→carve with mergeable results; weighted
monotonic progress; cancellation, pause, resume; per-stage bytes and ETA;
incremental error-map snapshots; thread-safe tree with snapshot semantics for
concurrent readers.

CLI: full command set (`list`, `scan`, `tree`, `recover`, `save`, `load`,
`verify`, `bench`, `health`, `repair`) with the exact grammar and exit codes the
reference-equivalent surface demands.

Implement by re-implementing from understanding. Never paste decompiled code.
Never copy proprietary strings. Cite the RE artifact in every commit.

## 3. C ABI shim — production-grade, stable, UI-ready

Build `cr_ui.dll` (MSVC, `__declspec(dllexport)`) with a stable, versioned ABI.

Design rules:
- Opaque handles only. No C++ types, no STL, no exceptions across the boundary.
- `cr_error` return codes everywhere. Document every code.
- UTF-8 strings in; returned strings owned by the library, freed by
  `cr_ui_string_free`.
- Progress callback: `void (*progress_fn)(void* ctx, uint32_t stage,
  uint32_t permille, uint64_t bytes_done, uint64_t bytes_total, uint32_t
  errors)`. Document which thread calls it and that it must not block.
- Cancellation: `cr_ui_scan_cancel(handle)`. Pause/resume: `cr_ui_scan_pause`,
  `cr_ui_scan_resume`.
- Snapshot: `cr_ui_tree_snapshot(handle)` returning an immutable or
  reader-locked view; document whether mid-scan recovery is safe and expose a
  `CR_TREE_PARTIAL` flag if not.
- Query: `cr_ui_tree_query(snapshot, filter, sort, page, &result)` — filter by
  category, date, size, confidence, extension; count-only mode; paged children.
- Preview: `cr_ui_node_read(snapshot, node_id, offset, buf, len)` bounded extent
  reader; optional metadata extractors with size and time caps.
- Session: `cr_ui_session_save`, `cr_ui_session_load`, `cr_ui_session_resume`.
- Writer: `cr_ui_recover_start`, `cr_ui_recover_cancel`,
  `cr_ui_recover_report_json`.
- Health: `cr_ui_health_query` returning exactly the reference field set.
- Visualization: `cr_ui_disk_map(snapshot, ...)` emitting the
  allocation/confidence/bad-sector geometry.
- ABI version: `cr_ui_abi_version()` and a documented stability contract. Bump
  on breaking change.

Acceptance: a small C consumer test loads the DLL, exercises the full lifecycle,
and passes under MSVC `/W4 /WX` on Windows CI. No UI-specific logic in the shim.

## 4. Local persistent database and real-time updates

Database: SQLite with WAL mode enabled on every connection
(`PRAGMA journal_mode=WAL`), `busy_timeout` set (10–30 s), `PRAGMA
synchronous=NORMAL`, and a bounded cache (`PRAGMA cache_size`). The session
tree, scan state, checkpoints, error maps, and result index persist to disk;
nothing that must survive a crash lives only in RAM. Paging and filtering are
query-driven, not materialized in memory.

Real-time updates: progress and state changes must be observable incrementally.
Design the event/notification surface so that a consumer receives stage
transitions, byte counts, error-map deltas, and files-found counts without
polling a global state. If the eventual UI bridge uses streaming channels, the
core's job is to emit a clean, typed event stream — not to assume a specific
transport. Document the event schema in `docs/ABI-CONTRACT.md`.

Performance constraint for block I/O: on Windows, direct reads
(`FILE_FLAG_NO_BUFFERING`) whose length is a multiple of 4 KB but not 64 KB take
a ~3.5× throughput slow path (~830 MB/s vs ~2900 MB/s on NVMe). Every direct
read path in core must round lengths up to a 64 KB multiple on Windows, clamped
at file/volume end, and verify this in the benchmark. Non-negotiable.

## 5. Performance parity and behavioral parity

Benchmark scenarios (minimum): NTFS live tree, deleted files, deleted folder,
formatted volume, partially overwritten MFT records, hard links, ADS, sparse,
fragmented; FAT32/exFAT deleted dentries, LFN chains, formatted, fragmented;
ext4/HFS+/Btrfs/ZFS/ReFS live and deleted where the reference supports it (ReFS
full MSB-chain); lost partition MBR/GPT deleted, formatted partition,
backup-GPT-only; carving header+footer and grammar-validated, truncated,
corrupted, fragmented; session save→kill→resume, corrupt session, checkpoint
recovery; writer normal, collision, empty dirs, destination overlap (must
refuse), hash verification; health/SMART and RawFixer detect-and-report,
dry-run, transactional apply/rollback.

Parameters per scenario: recall, precision, F1, false-positive rate,
filename/path preservation, timestamp preservation, content hash match, scan
time and throughput, recovery time and throughput, peak memory, cancellation
responsiveness, error-map accuracy.

Verdicts: MATCH / EXCEEDS / BELOW / NOT-COMPARABLE (with reason). A BELOW
without a root-cause hypothesis and a fix plan is a defect. Record everything in
`docs/BENCHMARK-REPORT.md`.

## 6. Matching architecture and wiring

Module boundaries, layering, dependency direction, interface shapes, ownership,
lifecycle, threading, and error propagation must mirror the reference
architecture as documented in the corpus. Where the current core diverges
structurally, refactor to match. Any deliberate divergence — including safety
hardenings such as destination-overlap refusal, BitLocker
legitimate-unlock-only, RawFixer consent, 4Kn handling, cancellable IRP, and
SDDL gating — is documented in `docs/PARITY-DELTAS.md` with evidence and
rationale.

## 7. Testing and CI (green for the right reason)

- Unit tests for every new or changed behavior — fail before, pass after.
- Integration tests on seeded VHDs across the scenario matrix.
- Structure-aware libFuzzer harnesses for every parser; ASan, UBSan, TSan clean.
- CLI contract tests for every command.
- C consumer test for the ABI shim on Windows CI.
- Windows CI: MSVC `/W4 /WX`, DLL smoke, CLI contract, seeded integration.
  Linux CI: pure-core logic, CLI, sanitizers, fuzz smoke.
- No `continue-on-error`. No skipped stages without a recorded, reviewed
  reason. Pin toolchains and action SHAs where flakiness exists.

## 8. Documentation and release

Update `docs/CAPABILITY-MATRIX.md` (ReFS and LDM DONE or PARTIAL — never
report-only, never excluded); `architecture.md`, `ground-truth-contracts.md`,
`error-fix-registry.md`, `UI-READINESS-PLAN.md`, `README.md`, `roadmap.md`;
`docs/PARITY-DELTAS.md`, `docs/BENCHMARK-REPORT.md`, `docs/ABI-CONTRACT.md`,
`docs/CORE-GAP-LIST.md` final. Remove every resolved TODO, FIXME, HACK, and
"future work" note. Tag a production release with `cr_ui.dll`, static lib,
`rrecov.exe`, headers, SHA256SUMS, and release notes. Signing stays disabled.

## 9. Rules (non-negotiable)

- Research → Verify → Plan → Audit → Implement → Re-review → Fix → Test →
  Benchmark → Compare → Document → Loop.
- No shortcuts. No fallbacks that mask failure. No compromises. No mock, no
  simulated, no stub, no DIY. Production grade only.
- No hallucinated parity. Every claim is backed by RE evidence and a test.
- ReFS and LDM are first-class. Correct any lingering language that says
  otherwise.
- Never break a contract without updating it in the same change.
- Pull frequently if others are active; never force-push; leave `main` green.
- Token hygiene: never commit the token; never log it.
- Win7-inclusive posture must not regress: import allowlist and OS-version gates
  verified in CI.
- Commit and push after every coherent unit with detailed, evidence-citing
  messages. Maintain `worklog.md` continuously.

## 10. Deliverables

1. `docs/CORE-GAP-LIST.md` with every row resolved to MATCH by end of session.
2. Every missing engine, algorithm, visualization engine, and module
   implemented, tested, and integrated.
3. `cr_ui.dll` — stable ABI, documented ownership/threading/error model, C
   consumer test passing on Windows CI.
4. SQLite-backed session/tree/checkpoint layer with WAL and Windows-tuned
   pragmas; query-driven paging and filtering.
5. Typed event stream for progress, state, error-map, and files-found updates —
   transport-agnostic and documented.
6. 64 KB-aligned direct I/O on Windows, verified in the benchmark.
7. `docs/BENCHMARK-REPORT.md` with MATCH/EXCEEDS/BELOW per scenario and per
   parameter; no unresolved BELOW.
8. `docs/PARITY-DELTAS.md` with every deliberate divergence documented and
   evidence-cited.
9. Updated `CAPABILITY-MATRIX.md` with ReFS/LDM correct.
10. Full green CI on Linux and Windows; ASan/UBSan/TSan clean; fuzz smoke clean.
11. Production release tagged with all artifacts.
12. `worklog.md` handoff with exact commit, open UNVERIFIED items, and precise
    next actions.

## 11. First 20 TODOs

T1 pull/repos/prompt/worklog; T2 full read + gap notes; T3 gate docs push;
T4 NTFS; T5 FAT/exFAT; T6 ext4/HFS+/Btrfs/ZFS/ReFS; T7 LDM; T8 visualization
data model; T9 lost-partition/virtual-volume; T10 session/CRSF SQLite; T11
writer/report; T12 health/SMART/RawFixer; T13 orchestration; T14 C ABI shim;
T15 typed event stream; T16 64 KB aligned I/O; T17 benchmark harness + report;
T18 fuzz/sanitizers; T19 docs sweep; T20 release tag + handoff.

## Final reminders

- The corpus is the only source of truth for reference behavior. When a doc and
  the decompiled code disagree, the code plus an independent probe win; record
  the resolution in `docs/PARITY-DELTAS.md`.
- The C ABI shim is a first-class product surface.
- Performance is a parity parameter. 64 KB alignment is a concrete engineering
  constraint, not a tuning exercise.
- No stubs. No mocks. No "future work." If something cannot be completed, file
  a precise tracked item and disable the surface truthfully — never a silent
  placeholder.
- Work long-horizon. Commit and push often. Keep the worklog current so an
  unexpected session stop loses nothing.
