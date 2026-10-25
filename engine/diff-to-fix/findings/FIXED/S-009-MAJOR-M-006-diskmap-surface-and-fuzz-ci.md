# SEVERE S-009 — No volume-bitmap / disk-map surface (og: IBuildVolumeBitmap → AnalysisGridWidget sector map)
# MAJOR M-006 — Fuzz CI job disabled (`if: false`)

Two findings filed together (visualization surface + quality gate).

---

## S-009 — Disk-map visualization surface absent

**Severity:** SEVERE · **Module:** visualization (query/preview/ABI), ntfs (producer side)
**Parameters affected:** P42 (public surface — missing operation group), B1 (bitmap
surface), R56 (interface shapes)
**Corpus references:** `re-workspace/artifacts/SearchDLL/searchdll-family--architecture.md`
§2 (IBuildVolumeBitmap shared interface); `SearchDLL/ntfs-engine--deep-dive.md` §1 (the
NTFS controller IS the bitmap builder — secondary vtable at +0x37/+0x38); `UIUX/08` +
`03` + `04` (AnalysisGridWidget: 54 v-methods, sector-grid painting, fed by
INotifyObserver → CMessageProxy → doDispatchMessage); `docs/ABI-CONTRACT.md` §7 (the
core's own normative disk-map model — documented as TO-BUILD)
**Core references:** no disk-map/bitmap query surface exists — verified at audit
baseline: `grep -rn "disk.map\|bitmap_query\|IBuildVolumeBitmap" src/ include/` → 0
implementation hits; `src/core/ntfs/cr_bitmap.cpp` loads $Bitmap for internal viability
grading only; ABI-CONTRACT §7 documents the model as unimplemented — audited commit
`67193b6`
**Status:** SPLIT — M-006 CLOSED (FIXED in `e30b84d`; verified independently by the session-2 audit: the fuzz lane in .github/workflows/ci.yml is re-enabled at :147-160 with build+run+crash-corpus upload, only the codeql lane stays deferred with the rationale documented; see M-006 Resolution). S-009 (disk-map surface) remains OPEN — READ-ONLY FINDING, no fix applied

**Summary.** The og renders a sector-grid disk map (allocated/free/bad/coverage/confidence
per region) in the scan UI — the AnalysisGridWidget — fed by the engines'
IBuildVolumeBitmap interface and the event bus. Our core has no equivalent surface: the
allocation bitmap is internal to NTFS viability grading, there is no
allocated/free/bad-sector region query in the tree/query/preview/ABI surfaces, and the
project's own ABI-CONTRACT §7 (written by the concurrent session) registers the disk-map
model as a to-build contract.

**Impact.** A primary user-visible feature of the reference UI (the disk map shown during
scanning, with coverage and confidence coloring) has no data source in the core; any UI
built on the current ABI cannot render it.

**Fix plan.**
1. Implement the ABI-CONTRACT §7 model as specified there (it is already well-designed):
   a `cr_ui` query that returns a downsampled region map {offset, length, state ∈
   {allocated, free, bad, unreadable, carved-hit, structured-hit}, confidence} over a
   query window, computed from (a) the engine bitmap when the FS exposes one (NTFS
   $Bitmap, exFAT/FS bitmap), (b) the retry-layer error map for bad/unreadable, (c)
   the tree extent indexes for hit coverage.
2. Producer side: surface `ntfs::VolumeBitmap` (already loaded) and the exFAT bitmap
   through a shared `BitmapProvider` on the scan controller; persist a downsampled copy
   in the CRSF session (new record type, versioned) so resumed sessions can render it.
3. Files: `src/core/orchestrator/cr_scan.cpp` (provider wiring),
   `src/core/session/cr_records.cpp` (record type), `src/ui/cr_ui.cpp` (query),
   `include/cr/cr_ui.h` (ABI append — ABI-since-1.x policy).
4. Tests: `DiskMap_Ntfs_Allocated_Free_Regions`, `DiskMap_Bad_Sectors_From_ErrorMap`,
   `DiskMap_Session_Roundtrip`.

**Tests to add.** (as above — fail before, pass after)

**Related findings.** M-005 (carve bitmap gating — the same bitmap plumbing serves both).

---

## M-006 — Fuzz CI job disabled

**Severity:** MAJOR · **Module:** quality (cross-cutting) · **Parameters:** Q66 (fuzz
coverage), Q67
**Core references:** `.github/workflows/ci.yml` — the fuzz and codeql jobs carry
`if: false` (agent-C read; recorded by the bench session as a deliberate skip during
comparative-bench work) — audited commit `67193b6`
**Corpus references:** N/A (quality parameter; the corpus is not the reference here —
the repo's own fuzz suite of 13 structure-aware targets is).

**Summary.** The repository contains 13 structure-aware fuzz targets
(`fuzz/fuzz_{ntfs,fat,exfat,ext4,hfsplus,btrfs,zfs,refs,carve,preview,container,engines,pure}.cpp`)
with a Makefile, but the CI workflow that should run them is disabled (`if: false`),
so no fuzzing runs on any push. Parameter Q66 requires every parser to have a
structure-aware harness AND the gate to be active.

**Impact.** Parser regressions that only fuzzing catches (the fuzz suite exists precisely
because such bugs were found before — see the fuzzer-hardened width selection in
`cr_runlist.cpp:34–54`) land silently on main.

**Fix plan.**
1. Re-enable the fuzz job in `ci.yml` with a bounded per-target budget (the Makefile
   already carries run budgets); keep codeql `if: false` if intentionally deferred, but
   document the deferral in `docs/testing.md`.
2. If the disable was for runner-time reasons, gate the job to nightly + PR-label
   instead of off.
3. Acceptance: green fuzz job on the next push; at least one CI run with a discovered
   crash triaged (or zero crashes, recorded).

**Tests to add.** CI-level (the job itself is the test).

**Related findings.** None.


## M-006 Resolution

**Status:** FIXED (fuzz lane; the codeql lane is a documented deliberate
deferral, per the fix plan's own allowance)
**Fixed in commit:** (this commit — see `git log` for `fix: M-006`)
**Verified by:** the job definition is unchanged from its previous green
history (clang + libFuzzer, 13 targets, 45s/target smoke budget from the
Makefile, crash-corpus upload) — only the `if: false` gate is removed. The
next push runs it; the audit's acceptance criterion ("green fuzz job on
the next push; at least one CI run with a discovered crash triaged or zero
crashes recorded") is satisfied by that run and the follow-up record in
`docs/testing.md`. Local pre-verification was not possible in this
environment (no clang/libFuzzer toolchain) — recorded here rather than
claimed.
**Resolution notes:** the bench-session skip premise ("we are not doing
core code changes") no longer holds — the remediation session changes
parser code on every push (S-002 added an attribute-B-tree parser, S-004
rewrote the jbd2 salvage, M-001 changed the bootstrap ladder: all inside
the existing structure-aware harness surfaces). CodeQL stays deferred
with the rationale documented in `docs/testing.md` per the fix plan.

## Session-2 main-auditor leg update (2026-10-25)

**M-006 (fuzz CI): FIXED by `e30b84d`** — verified on the current tree: the fuzz job's
`if: false` is commented out with the M-006 marker (`ci.yml:147-148`); codeql remains
deliberately deferred with its own documented comment. The M-006 half of this file is CLOSED.

**S-009 (disk-map) remainder — unchanged and still open** after auditing `56cce87`
(this leg audited the post-session-1 commits the first session never saw):
1. Engine bitmaps not wired into the producer: the NTFS `VolumeBitmap` IS loaded and passed
   to the walk (`cr_scan.cpp:706-707`) but the disk-map allocation overlay derives from
   active-structured extents only (`cr_diskmap.cpp:64-68`) — **`kCellAllocFree` is never
   produced**; the og's free/used distinction (`IBuildVolumeBitmap`,
   `searchdll-family--architecture.md:31`) is unreachable in our map.
2. No versioned CRSF disk-map record (`cr_records.cpp` has only the pre-existing
   `serialize_error_map`); resumed sessions recompute at fixed 64 KiB cells.
3. Snapshot path carries no error map (`cr_ui.cpp:525-27`): mid-scan maps (the og's
   AnalysisGridWidget primary use, UIUX/04:110) can never show bad/unreadable cells.
4. Acceptance tests from the original plan (`DiskMap_Ntfs_Allocated_Free_Regions`,
   `DiskMap_Bad_Sectors_From_ErrorMap`) still absent; both existing tests pass
   `bad_ranges=nullptr`.
5. Doc drift: ABI-CONTRACT §7 "state 2 free" vs the ABI enum (`cr_ui.h:474-75`) and §"3 bad
   (marked by the FS)" applied to retry-layer ranges.
Items 1 and M-005 share one plumbing fix (BitmapProvider) — implement together.

---

## Addendum (session 6 — the surface LANDED; the finding's premise is stale, and the residual is the FREE overlay, 2026-10-26)

**Correction first (doc truth):** this finding's "no disk-map/bitmap query surface exists" premise stopped being true when the diskmap landed — `include/cr/tree/cr_diskmap.h` + `src/core/tree/cr_diskmap.cpp` + the `cr_ui_disk_map` ABI export (exercised end-to-end by the C consumer at tests/ui_abi_check.c:578-612, incl. the two-call bad_ranges pattern and the snapshot-requires-geometry contract). The CROSS-REF-MATRIX already noted "partial-FIXED-in-tree, finding file stale (N-020)"; this addendum makes the finding file itself truthful. The fuzz-CI half (M-006) was fixed long ago (e30b84d).

**The residual (the actual remaining divergence):** the og's `IBuildVolumeBitmap` shared interface produces **free/used truth** from the filesystem bitmap ("which clusters are free/used" — searchdll-family--architecture.md §2, cited in this finding's header and in cr_diskmap.h's own lineage comment). The core's diskmap can produce only USED (active structured extents) + UNKNOWN: `kCellAllocFree` is defined (`cr_diskmap.h:62`) but **never assigned anywhere in src/** (grep: 0 assignment sites), because the engine-side allocation bitmaps are loaded-and-discarded inside the engines — e.g. the NTFS `VolumeBitmap` at `src/core/orchestrator/cr_scan.cpp:813-814` is loaded, passed to `walk_mft`, and destroyed without ever feeding the viz layer. The core's own header admits it: "the engine bitmap is not persisted (registered follow-up)" (cr_diskmap.h:21-23) — but no finding tracked that follow-up until now.

**Fix plan for the residual (step 2 of the original plan, now concrete):**
1. Have each engine's open path retain its allocation bitmap (NTFS `$Bitmap` via the existing `VolumeBitmap`; FAT/exFAT already parse their allocation bitmaps; ReFS via its allocator) OR have the orchestrator snapshot the bitmap into the ScanResult after the structured pass.
2. Feed `build_disk_map`: cells the bitmap marks free and no found-content covers → `kCellAllocFree`; the current USED-from-extents rule stays (it is the stricter claim).
3. Persistence: one bit-packed free map in the session (bounded by the 64 MiB overlay budget's cell granularity — the diskmap's own cell sizing already handles the scale).
4. Tests: `DiskMap.NtfsFreeCellsFromBitmap` (a FAT-formatted NTFS fixture… no — an NTFS fixture with known-free clusters asserts kCellAllocFree there), `DiskMap.FreeCellsSurviveSessionReload`, `DiskMap.FreeNeverOverridesUsed`.
5. Acceptance: the four-overlay C-consumer call can report free cells for a structured NTFS scan (today it reports Unknown outside active extents).

**Related:** N-074 (the GAP-S6-11 row that must close-with-residual), the visualization module row (matrix update this session).

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-009 commit — see `git log` for "fix: S-009")
**Verified by:** 4 new fail-before-by-construction tests (the finding itself grep-verified `kCellAllocFree` had ZERO assignment sites pre-fix — every test asserting `alloc[cell] == 2` was impossible before): `CrUi.DiskMapFreeOverlayFromFatTable` (session path, FAT-table truth: USED at the active doc chain, UNKNOWN at the deleted chain, FREE at bitmap-free clusters, >4500 free cells on the fixture), `CrUi.DiskMapFreeCellsSurviveSessionReloadAndSnapshot` (the REC_ALLOC_MAP round-trip: identical free population via session and via snapshot + the snapshot view bounds check), `CrUi.DiskMapScanSnapshotCarriesFreeMap` (the completed-scan snapshot path), `DiskMap.FreeNeverOverridesUsedOrBadAndDownsampleIsConservative` (hostile-map precedence proof: USED and FsBad never overridden by free; the downsample's uncovered tail stays not-free). Suite 515/515 Linux Release + 21/21 cr_ui_tests + `cr_ui_abi_check` green, and the same three green on the ASan lane (executed this session on the merged tree).
**Resolution notes:** The residual the session-6 addendum scoped is closed end-to-end. (1) Producer: `storage::FreeMap` (new `include/cr/storage/cr_freemap.h` — bit set = free, out-of-range never free) filled by the engines that own an allocation bitmap — NTFS from `$Bitmap` (the already-loaded `VolumeBitmap`, inverted; accessors added), FAT from the loaded table (entry 0 = free, cluster 0's byte = data base − 2 clusters, the og's linear cluster math), exFAT from the allocation-bitmap dentry walk (bit set = allocated, inverted; the 8 MiB loader cap's tail stays conservatively not-free). Engines write slice-relative; `run_engine_dispatch` rebases to the absolute device basis exactly like the tree extents (F-1); candidate probes never overwrite the main-pass map. (2) Consumer: `build_disk_map` gains `DiskMapInputs.free_map` and paints `kCellAllocFree` only on still-unknown cells fully covered by bitmap-free clusters — USED (the stricter structured claim) and FsBad (error spans) are never overridden, matching the finding's fix plan verbatim. (3) Persistence: `REC_ALLOC_MAP` (record type 7, optional, old readers skip) stores the DOWNSAMPLED cell-granular map (`viz::downsample_free_map` — the same `choose_cell_bytes` sizing, so a 16 TiB volume persists ≤ 8 MiB, never a 512 MiB $Bitmap copy); `load_session_summary` restores it and the session disk-map path uses its granularity as the cluster hint (pre-record sessions keep the 64 KiB default — unknown, never a guess). (4) The session-2 audit's snapshot-path items close too: `CrSnapshot` now value-captures the session's error map + free map + slice view (`cr_ui_session_snapshot`), and `cr_ui_scan_snapshot` captures the live telemetry error map under the controller lock (the mid-scan AnalysisGridWidget use, UIUX/04:110) plus the free map when the scan is not running (a live read would be racy — absent paints unknown). The snapshot `want` view is now bounds-checked against the captured slice. (5) Doc truth: the header's "engine bitmap is not persisted (registered follow-up)" rule and ABI-CONTRACT §7's "3 bad (marked by the FS)" drift corrected (bad = retry-layer spans; free = engine bitmap). ReFS/ext4/HFS+/Btrfs/ZFS stay without a free map (no bitmap surface decoded in the corpus for them — the og's `IBuildVolumeBitmap` is cited only for the NTFS/FAT-family engines); their cells degrade to unknown, which is the documented honest state. M-006 (fuzz CI) was fixed long ago (`e30b84d`); N-057's bad-overlay coverage debt is now partially paid (the precedence test drives the error-map path); the Ebr enum and the budget-growth test remain N-057's open scope.

---

## Parallel-landing reconciliation (session 8)

The N-020 doc-drift leg (parallel, `74e4d36`) moved this file to FIXED with
a Resolution crediting only the GAP-S6-11 surface landing (`56cce87`) —
while the same file's session-6 addendum (quoted verbatim above their
Resolution) documents the FREE-OVERLAY RESIDUAL as the actual open
divergence (`kCellAllocFree` never assigned, engine bitmaps
loaded-and-discarded, no persistence, no snapshot error map). A surface
landing is not a residual fix: marking this finding FIXED on the surface
alone was premature — the exact "hallucinated parity" class the fix
sessions are mandated against (a finding is FIXED only with a test that
proves it, and no test could produce a free cell before this session).
This session's Resolution above is the operative one: the free overlay is
implemented, tested (4 fail-before-by-construction tests), and verified on
both lanes. The N-020 leg's five other doc-drift corrections are
unaffected and stand.
