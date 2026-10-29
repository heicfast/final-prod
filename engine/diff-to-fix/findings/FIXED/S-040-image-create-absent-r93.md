# SEVERE S-040 — Disk-image creation wholly absent (the og's "Create Image" home-page flow; the safe-scan posture has no producer)

**Severity:** SEVERE
**Module:** imaging / storage / ABI / CLI
**Parameters affected:** P42 (public surface shape), C24 (CLI exit codes), F59 (driver-absent behavior — image-create is a driver-class-(a) op per the plan of record), B8 (multi-volume and cross-partition boundaries — whole-disk imaging)
**Corpus references:** `re-workspace/artifacts/UIUX/07-home-phase-deep-dive.md:36-37` ("Sub-flows on this page: Disk-image creation: 'Back' / 'Create Image' / 'Select a drive or a partition to create a backup disk image' (→ `DiskBackupDialog` chain)"); `re-workspace/artifacts/UIUX/06-dialog-telemetry-inventory.md:27` ("`DiskBackupDialog` + `DiskBackupSaveDlg` + `DiskBackProgressPage` | 12–14 strings | Disk-image creation flow"); `re-workspace/artifacts/UIUX/11-recovery-phase-funnel.md:17` ("We recommend using the image file of this disk for scanning…"); `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:87` (ToolsModule.dll — "toolbox: WinPE media building, burning, imaging (links PE_Tools, Burn.dll, libvmdk)")
**Core references:** audited `35adcb9` (product base; trail commits are docs-only). Absence verified: `rg -c "image_create|create_image|cr_ui_image"` over `include/` + `src/` → 0 code hits (only `docs/PLAN-API-EXPANSION.md` / `diff-to-fix/API-DELTA.md` board text). `src/core/storage/cr_image.cpp:1-2` = the reader only ("disk-image scan targets (S-030): flat / VMDK / VHD / VHDX"); `src/win/cr_image_win.cpp:37` opens images `GENERIC_READ`. CLI dispatch `src/app/rrecov.cpp:3027-3048` — 11 verbs, no `image` verb. `include/cr/cr_ui.h` — 81 exports, none creates an image.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og product's default landing page ships a "Create Image" flow (disk → image file) that produces the exact image files its own scan path consumes — the corpus explicitly recommends image-scan as the safe-scan posture for damaged disks. The core implements the *consumer* half only (flat/VMDK/VHD/VHDX readers, `--image` scan targets) and has no *producer* half: no `cr_ui_image_create` export, no win-layer writer, no CLI verb. This is a user-facing feature the reference has that the core wholly lacks, and it breaks the recommended safe-recovery workflow at its first step.

## What the reference does

The og home page carries a disk-image creation sub-flow: "Select a drive or a partition to create a backup disk image" behind a `DiskBackupDialog` → `DiskBackupSaveDlg` → `DiskBackProgressPage` chain (UIUX/07:36-37, UIUX/06:27 — 12–14 strings of the flow telemetry'd). The imaging capability itself lives in ToolsModule.dll (the 2.5 MB MOM toolbox that also links PE_Tools, Burn.dll and libvmdk — mom-module-system--architecture.md:87). The product then recommends scanning the produced image instead of the live disk on the recovery funnel (UIUX/11:17: "We recommend using the image file of this disk for scanning…"). The exact on-disk format the create flow writes is not decoded in the corpus beyond the libvmdk link and the reader-side dispatch (`DRWUI_decompiled.c:948237-948409`, cited by `src/core/storage/cr_image.cpp:8-9` for flat/VMDK/VHD/VHDX reading) — the reader accepting flat images is the corpus-grounded minimum.

## What our core does

The core reads images only. `storage::open_image` (cr_image.cpp) dispatches flat / VMDK / VHD / VHDX to read-capable block-device adapters; `WinImageFileIo::open` (cr_image_win.cpp:37) opens `GENERIC_READ`. The scan CLI accepts `--image` targets (rrecov.cpp:2992) — the read path is complete and tested (S-030 lineage). No code path creates an image: zero grep hits for `image_create`/`create_image`/`cr_ui_image` across `include/` and `src/`; the 81-export ABI has no producer; the 11-verb CLI has no `image` verb (rrecov.cpp:3027-3048). The gap is tracked on the plan board only (`docs/PLAN-API-EXPANSION.md` Wave 4, row R-93 "image create … `cr_ui_image_create` ABSENT; `storage::open_image` is the reader only" — verified still true at this tree).

## Gap

A user cannot produce a disk image from the core: `cr_ui_image_create` is absent from the ABI, no win-layer writer exists, and the CLI has no verb. The og's home-page "Create Image" flow and the corpus-recommended image-scan posture are unreachable — the core can consume images but not create them.

## Impact

User-observable missing feature (the og's default landing page carries it). Operationally it disables the product's own recommended safe-scan workflow for failing disks: scan-the-image-first is the corpus's stated damage posture, and the core's answer to "then make me an image" is nothing. Hosts embedding the ABI cannot offer the flow either — there is no surface to bind. Recovery correctness of the scan path itself is unaffected (the consumer half is complete).

## Reproduction / verification

- Corpus-derived (cited above; UIUX/07:36-37, UIUX/06:27, mom-module-system:87, UIUX/11:17).
- Core absence verified first-hand: `rg -c "image_create|create_image|cr_ui_image" include/ src/` → 0 code hits; `cr_ui.h` export census (81) contains no image-create; `rrecov.cpp:3027-3048` verb table has no `image`.
- Cross-verified: `diff-to-fix/API-DELTA.md` §5.2 row S3-W4b ("R-93 PARTIAL … `cr_ui_image_create` ABSENT") — verified still accurate at `35adcb9`.

## Fix plan (for the engineering team)

1. Architectural: a win-layer image writer over the existing dest-I/O/safewriter abstractions — stream the device through 64 KiB envelope reads (the transport contract) to a flat `.img`; raw-flat only, with VHD/VHDX writers explicitly out of scope (documented divergence: the og's own reader accepts flat, and the corpus does not decode the og's writer format beyond the libvmdk link).
2. Files: new `src/win/cr_image_create.cpp` (+ `include/cr/win/` header if needed); `include/cr/cr_ui.h` ABI append; `src/ui/cr_ui.cpp` export; `src/app/rrecov.cpp` verb.
3. Algorithm: sector-by-sector (or 64 KiB-block) copy with progress callback, cancel support (PausableToken, the scan precedent), and a SHA256 sidecar per the plan-of-record's shape; driver class (a) precondition — check driver status first (DRIVER-REQUIRED table row).
4. ABI: `cr_ui_image_create(uint32_t disk, const char* path_utf8, cr_ui_progress_fn progress, void* user)` — append-only (ABI-CONTRACT §9, no version bump).
5. CLI: `rrecov image --create --disk N --out file.img` — new verb ⇒ the three-site lock (docs/API.md §1 + CORE-API-CURRENT §c + rrecov.cpp header comment) AND the usage `-h` block (see N-094 — the lock must cover the fourth site); all count sites 11→12 verbs, 81→82 exports.
6. Contract/doc updates: ABI-CONTRACT §7 row, DRIVER-REQUIRED class-(a) row, CORE-API-CURRENT, API.md, CI-SCENARIOS entry.
7. Acceptance criteria: created image hashes byte-identical to the source device on the Windows lane; cancel mid-image leaves a partial file that is reported, never silently passed off as complete; zero-sanitizer clean on truncated/partial fixtures.

## Tests to add

- `ImageCreate.HashMatchesSource` (Windows lane: full-disk image SHA == device read-back SHA).
- `ImageCreate.CancelMidStream` (partial file + truthful rc; no silent success).
- `ImageCreate.NoDriverRefuses` (driver-class-(a) gate).
- CLI round-trip: `image --create` then `scan --image` on the produced file finds the planted fixture.
- Unit: 4Kn and 512e geometry through the copy loop (envelope alignment).

## Related findings

- S-030 (FIXED — the image reader/scan-target surface this producer would feed).
- API-DELTA §5.2 row S3-W4b (this finding files it as a finding, per the session-8 mandate).
- N-094 (the CLI lock must grow the `-h` site when the new verb lands).

## Open questions

The og's exact written format (flat vs VMDK-first) is not decoded in the corpus (ToolsModule internals not decompiled to that depth). The fix plan scopes to flat (the reader's own accepted floor). If a future corpus decode shows the og's default writer emits VMDK, file the follow-up then.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

rrecov.cpp's verb table drifted to :3127-3148 (was :3027-3048); the CR_UI_API export census is now 86 (was 81 — the thumbnail ABI additions); the absence itself unchanged (zero image_create symbols; cr_image_win.cpp:37 still reader-only GENERIC_READ)

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-040 landing; SHA recorded in worklog.md)
**Verified by:** the portable copy loop pinned by 7 new tests over a surface that did not exist before the landing (a pre-fix tree fails at compile: the symbols are absent) — ImageCreate.HashMatchesSource / ShortTailNotPadded / FourKnGeometryStreamsUnpadded / CancelMidStreamKeepsTruthfulPartialStats / ReadFailureReportsCommittedPrefix / ProgressCallbackPerChunk / EmptyDeviceIsEmptyStream (cr_tests green, 7/7 this session); the win-layer disposition contract by cr_image_create_tests (ImageCreateFile.CommitIsAtomicWithSidecar / CancelKeepsExplicitPartialNeverComplete / ReCreateReplacesOldImageAndPartial / EmptyPathRefused — MSVC lane; the parse gate green locally); the ABI by the C-consumer rows (Linux: ERR_UNSUPPORTED + the BAD_PARAMETER argument gates; cr_ui_abi_check green); the CLI four-site lock by the cr_ui verb-lock pin (dispatch + -h usage + header table + exit codes, green); the docs by the doc_truth_gate (87 exports / 13 verbs, D-143 unique, gate green). Full local suite after the landing: 1470/1470 ctest green (cr_tests 1388: 1346 pass / 42 win-skip / 0 fail + cr_ui_tests + abi_check + win_syntax_check).
**Resolution notes:** The producer half landed exactly per the fix plan: (1) portable `storage::create_image_stream` in the core (cr_image.cpp — 64 KiB envelope reads in the transport contract, SHA-256 over the emitted stream, cancel at chunk heads, cumulative progress; the stats always carry the committed prefix so a partial is distinguishable from complete); (2) win-layer `win::create_image_file` (atomic `<out>.crimg-tmp` -> rename commit + the `<out>.sha256` sidecar in the standard `"<hex>  <basename>"` form; cancel/failure keeps the explicit `<out>.partial` — reported, never silently complete) + `win::create_disk_image` over `win::open_disk` (class (a)); (3) ABI append `cr_ui_image_create(disk, utf8_path, progress, user)` (87 exports, append-only per ABI-CONTRACT 9, new contract section 7j; ERR_DRIVER_* gate before the open, the same posture as cr_ui_open_disk; honest ERR_UNSUPPORTED on non-Windows); (4) CLI verb 13 `image --create --disk N --out file.img` (four-site lock + exit codes 3/2 with the kept-.partial reporting; refuses a container-extension --out honestly - D-143); (5) the flat-only scope registered as D-143 (VHD/VHDX writers out of scope - the og's own reader accepts flat and the corpus does not decode the og's writer format beyond the libvmdk link; the finding's own open question stays recorded for a future corpus decode); (6) contract docs swept: DRIVER-REQUIRED (a) row current, R-93 PARTIAL->PRESENT (both rows), CORE-API-CURRENT/API/API-SURFACE-AUDIT counts, ABI-CONTRACT 7j, CAPABILITY untouched (the create op was not a capability-matrix row), and the Windows-lane probe added to Run-Benchmark.ps1 phase 1 (create -> sidecar digest == Get-FileHash(image) -> size == disk size -> scan --image exit 0). The digest equality is the byte-identity proof: the digest was computed over the bytes READ FROM the device, so sidecar == file-hash proves the committed file equals the device stream.
