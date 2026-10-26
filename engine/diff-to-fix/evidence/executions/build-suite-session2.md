# Execution — Linux build + full test suite at the session-2 audited commit (2026-10-25)

## Environment

- Host: 2-core container, 4 GiB RAM, Debian; g++ 14 (`x86_64-linux-gnu-g++-14`), GNU Make,
  CMake 4.4.4 (pip/venv — no system cmake). No Windows, no wine, no Ghidra: og-side execution
  remains impossible in this environment (unchanged limitation from session 1).
- Audited core commit: `e93decf` (main); corpus: `03ad46c` (unmoved).

## Build

```
cmake -S /home/z/my-project/core_recovery -B /home/z/my-project/build-rel \
      -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCR_BUILD_APP=OFF -DCR_BUILD_TOOLS=OFF
cmake --build /home/z/my-project/build-rel -j2        # → [100%] Built target cr_tests
```

(An ASan configuration — the project's `linux-gcc` preset — was attempted first; the
container's memory reaping kills long ASan compiles. The Release build is the verification
vehicle here; the project's own CI carries the ASan/TSan jobs per Q67.)

## Test suite

```
ctest --test-dir /home/z/my-project/build-rel --output-on-failure
→ 100% tests passed, 282 out of 282   (total 5.06 s)
```

## What this verifies

1. **The remediation claims hold at `e93decf`:** the A-001 (GUID) regression tests, the
   S-001 destroy-rule tests (`NtfsVolume.EndToEndWalk`, `OverwrittenHonestModeKeepsAndGrades`),
   the S-005 decoder/engine tests (`text.*`, `Ext4Engine.NonAsciiNamesDecodedNotByteLifted`,
   `Btrfs.NonAsciiNamesDecodedNotByteLifted`, `Zfs.NonAsciiNamesDecodedNotByteLifted`), and
   the GAP-S6-10/11 event/disk-map tests (`CrUi.TypedEventStreamFullLifecycle`,
   `CrUi.DiskMapSessionGeometryAndOverlays`, `CrUi.DiskMapSnapshotRequiresViewGeometry`) all
   pass — the fixed findings' fail-before/pass-after stories are consistent with a green suite.
2. **A-004's masking is confirmed empirically:** the typed-event lifecycle tests pass because
   the in-tree `cr_ui` shim sets BOTH the config sink and the live sink
   (`scan_set_event_fn_impl`), so the wrong-sink delivery path at `cr_scan.cpp:340` is never
   exercised by the suite — exactly the "tests green, embedder crash" shape the finding
   describes.
3. **S-009's partial state is visible in the suite:** only two disk-map tests exist
   (`GeometryAndOverlays`, `SnapshotRequiresViewGeometry`) — the finding's own acceptance
   tests (`DiskMap_Ntfs_Allocated_Free_Regions`, `DiskMap_Bad_Sectors_From_ErrorMap`) are
   absent, matching the PARTIALLY-FIXED verdict.

## Not executable here (unchanged)

- og-run comparisons (BitLocker unlock, og carve runs, og .rsf production): needs the Windows
  lab; procedures are written into the relevant findings (S-014, S-007, S-013 bench steps).
- New Ghidra decompiles: corpus decompiled trees + objdump/strings were used instead (all
  session-2 findings cite them).
