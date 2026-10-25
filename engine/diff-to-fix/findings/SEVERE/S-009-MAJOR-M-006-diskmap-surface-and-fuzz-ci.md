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
