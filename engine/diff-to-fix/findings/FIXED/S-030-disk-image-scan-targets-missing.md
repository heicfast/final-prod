# SEVERE S-030 — Disk-image scan targets: the og scans .vmdk/.vhd/.vhdx/.edisk/.img/.dmg as first-class devices; the core accepts only --disk/--volume

**Severity:** SEVERE
**Module:** imaging (device layer) — new module row
**Parameters affected:** P1 (detection — image inputs invisible), P8 (multi-volume/boundaries — image-bounded devices), P42 (public surface — target selection), P47 (throughput — no libvmdk extent translation), P54 (module boundaries — image backends)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — extension-dispatch factory `FUN_1405b3410` (:948237-948409: `"edisk"`→FUN_1400dafc0 :948270-948278, `"vhdx"`→CVHDXDisk :948293, `"vmdk"`→VmdkDevice :948311, `"vhd"`→CVHDDisk :948347; `IImageFile` :948440; **flat-file fallback for unrecognized extensions (.img/.dmg/raw)** CreateFileW + GetFileSizeEx :948389-948393), construction `FUN_140178250` (:275744-275889: PathFileExistsW :275793, **reads LBA0 (0x1000 bytes) and classifies MBR/GPT** :275877-275880), UI wiring ("Import a disk image" :1212897/:1213510; **filter `"*.vmdk *.vhd *.vhdx *.edisk *.dmg *.img"`** :1213924; telemetry `Select_diskImage` :1213030; compressed variant `CImageFileDeviceWithDecompress` :993808; embedded `disk_vmdk::Vmdk` parser :978187+ with "Unsupported or corrupt VMDK (magic mismatch)" :978668 and sparse-extent regexes `.*[sS](\d{3,})\.vmdk` :970533); `re-workspace/reversed/ghidra-mo/Device_decompiled.c` — engine-side VMDK block device `FUN_1800545c0` (:49106-49124: `libvmdk_handle_read_buffer_at_offset(h, buf, sector*0x200, count*0x200)` — 512-byte sectors; success = exact byte count) and parent-chain open `FUN_180054120` (:48965-49100: open_extent_data_files :49017-49023, recursive parent :49062, set_parent_handle :49072); `Device.mo` is the sole libvmdk importer (Device_imports.txt); `og-codebase/PartitionRecovery/libvmdk.dll` (50 exports incl. check_file_signature family, per-binary/libvmdk.md:26-48)
**Core references:** audited `7cf6754` — `src/app/rrecov.cpp:495-506` (CLI accepts ONLY `--disk N` → open_disk_view :241 and `--volume X` → open_volume_view :270; unknown options rejected :562-565); `src/win/cr_transport.cpp` (open_physical_drive `\\.\PhysicalDrive%u` :782-786, open_harddisk_volume :788-792 — device namespace only); `include/cr/storage/cr_device.h:89-98` (IStorageDevice with **no image/file-backed subclass**); zero vmdk/vhd/vhdx/edisk hits across src/include/tests (grep-verified); tests use in-memory MemDevice only
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og treats disk-image FILES as first-class scan targets: DRWUI's "Import a disk image" flow dispatches on extension to per-format backends (edisk, vhdx, vmdk, vhd) with a flat-file fallback for raw/.img/.dmg, wraps the file as an `IStorageDevice`, reads LBA0 and classifies MBR/GPT, and hands it to the engines; Device.mo additionally exposes a libvmdk-backed VMDK block device (sparse extents + parent chains) for the engine layer. The core cannot scan any image file: the CLI exposes only `--disk`/`--volume`, the transport opens only PhysicalDrive/HarddiskVolume device namespaces, and IStorageDevice has no file-backed implementation. A whole input class of the reference product is missing.

## What the reference does

Two cooperating layers. (1) **DRWUI `CImageFileDevice`**: the import
handler builds a device record from the image path, reads the first 0x1000
bytes (LBA0 region) and classifies the partition layout (MBR vs GPT,
:275877-275880), then presents the image in the device list exactly like a
physical disk — the scan flow downstream is identical. The extension factory
(:948237-948409) picks the backend: edisk (EaseUS Todo-Backup image),
vhdx, vmdk, vhd; anything else (.img, .dmg, raw) falls back to a flat file
device (CreateFileW GENERIC_READ + size). A compressed-image variant exists
(:993808) and DRWUI additionally embeds its own `disk_vmdk::Vmdk` parser
with sparse-extent descriptor support and a clear error string for corrupt
magic (:978668). (2) **Device.mo's VMDK block device**: the MOM device layer
uses libvmdk (`read_buffer_at_offset` at 512-byte sector granularity,
:49106-49124) including extent-data-file opening and recursive parent-chain
resolution (:48965-49100) — so engine reads against a VMDK-backed volume are
transparently translated.

## What our core does

The CLI target grammar is exactly two options: `--disk N` and `--volume X`
(rrecov.cpp:495-506); everything else is rejected (:562-565). The transport
layer opens only `\\.\PhysicalDrive%u` and harddisk-volume device paths
(cr_transport.cpp:782-792). `IStorageDevice` (cr_device.h:89-98) is the
storage seam — `cr_cached`/`cr_slice`/`cr_devmap` are I/O helpers over it,
not file-backed devices — and no image format code exists anywhere in the
tree. Tests exercise in-memory devices (MemDevice) only; there is no
image-file scanning test to fail.

## Gap

The og scans disk-image files as devices (6 formats: vmdk/vhd/vhdx/edisk/
img/dmg, with sparse/extent/parent-chain VMDK support and compressed
variants); the core has no image input path at all. Falsifiable:
`rrecov scan --image <file>.img` is not a valid invocation (unknown option),
and `rg -i "vmdk|vhdx|edisk" src include` returns zero hits.

## Impact

Users cannot scan forensic images, VM disks, or backups with the core — a
first-class input mode of the reference product. This also blocks parity
verification workflows (fixture images are how this audit suite should
eventually run og-vs-core comparisons on identical inputs). Recovery
correctness on physical disks is unaffected; the gap is input-surface, but
it is a product-level capability the reference has and users observe.

## Reproduction / verification

Corpus-derived (decompile citations above, agent-B decode + lead
spot-verification of the core CLI and transport). Execution: attempted —
`rrecov --help` equivalent shows only --disk/--volume (source-verified
rrecov.cpp:495-506; no execution environment for the Windows CLI in this
session). Evidence:
`diff-to-fix/evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §A.

## Fix plan (for the engineering team)

1. Add a file-backed device: `include/cr/storage/cr_image.h` +
   `src/core/storage/cr_image.cpp` — `open_image(path) ->
   unique_ptr<IStorageDevice>`; flat-file first (CreateFileW/fopen +
   size), sector-512 reads, same read_at contract as DirectDevice.
2. CLI: `--image <path>` (rrecov.cpp target grammar) + session-restore
   support (persist the image path in the session header — extend the
   identity card, cr_records.cpp:307-310 family).
3. VMDK v1: host-bus-adapter sparse extent reader (the descriptor grammar
   is small: extent tables + flat extents; parent chains can be v2). Match
   the og's 512-byte sector translation (Device.mo:49117-49119). Test with
   `qemu-img create -f vmdk` fixtures (subprocess-independent: ship
   pre-made tiny .vmdk fixtures in tests/data).
4. vhd/vhdx: footer/header parse (dynamic + differencing); edisk/.dmg:
   register as deliberate exclusions in PARITY-DELTAS (proprietary/Apple
   formats) unless product requires them.
5. Tests: `ImageFlatFileScansMbr`, `ImageVmdkSparseExtentReads`,
   `ImageVhdDynamicDisk`, `ImageLba0LayoutClassification` (MBR and GPT
   fixtures), `ImageSessionRoundTrip`, `ImageRejectsTruncatedFile`.

## Tests to add

The six named tests above; all fail-before by absence of the code path.

## Related findings

M-030 (transport envelope — the image device must route through
aligned_read_at like every other direct read path), N-049 (session
persistence of richer target identity), I-020 (CTbFile corpus-doc
correction — the image role was misattributed).

## Open questions

The og's edisk backend (FUN_1400dafc0) and `CImageFileDeviceWithDecompress`
internals are undecoded (corpus next artifacts: DRWUI:948270-948278,
:993808) — needed only if product wants edisk/compressed-image parity.

<<<<<<< HEAD
---

## Resolution (first landing — flat/VMDK/VHD)

**Status:** FIXED
**Fixed in commit:** (this session's S-030 commit — see `git log` for "fix: S-030")
**Verified by:** 8 new tests in `tests/unit/test_image.cpp` (fail-before by construction: no image-format code existed — grep `rg -i "vmdk|vhdx|edisk" src include` returned zero hits at filing): `Image.FlatFileScansMbr` (flat device + LBA0 MBR classification + a full FAT scan finds the fixture's files), `Image.Lba0LayoutClassification` (protective-GPT, torn-protective-with-intact-EFI-PART, unrecognizable), `Image.VmdkMonolithicSparseReadsAndScans` (a hand-built monolithicSparse image holding the FAT16 fixture with only nonzero grains allocated — < half the flat size; byte-identical full readback + unaligned/tail reads + an engine scan finds the same files), `Image.VmdkDescriptorWithFlatAndZeroExtents` (a split image: FLAT + ZERO + FLAT-with-offset extents across sibling files through the ops seam; the ZERO hole reads zeros), `Image.VhdDynamicDiskReadsAndScans` (dynamic VHD with per-sector bitmaps; byte-identical readback incl. an unaligned cross-block read; engine scan green), `Image.VhdFixedDiskIsFlatMinusFooter`, `Image.RejectsBrokenInputs` (under-a-sector input, VHDX → ERR_UNSUPPORTED, garbage VMDK magic → ERR_BAD_MAGIC, descriptorless → ERR_BAD_MAGIC, out-of-bounds device reads), `Image.SessionRoundTripCarriesTheImagePath` (the VolumeInfo v3 tail round-trips through save/load). The updated `Crsf.VolumeInfoIdentityCardRoundTrip` pins the v3 layout (80+84+256) with v2/v1 backward-compat assertions. Suite **525/525 + cr_ui_tests 21/21 + cr_ui_abi_check** green on Release AND ASan (merged tree). Fuzz: `fuzz/fuzz_image.cpp` added to the suite (Makefile target + run/corpus rules) — the S-030 parsers are structure-aware-fuzzed on the CI fuzz lane like every other parser (local pre-verification not possible: no clang/libFuzzer in this environment — recorded, not claimed; the M-006 precedent).
**Resolution notes:** The core module `include/cr/storage/cr_image.h` + `src/core/storage/cr_image.cpp` implements the og's import surface re-implemented from format understanding (no libvmdk, no decompiled code): content sniffing (VMDK magic / text descriptor head, "conectix" TRAILING footer for VHD — the tests caught the head-only sniff missing fixed VHDs), flat fallback with the sector-granular bound, VMDK monolithicSparse (embedded descriptor; the source is the single extent) and standalone descriptors (FLAT/SPARSE/ZERO extent lines opened through the injected `ImageFileOps` seam, relative to the descriptor's directory), grain-directory/grain-table mapping with grain-segment batching (one GDE/GTE lookup per grain run; unallocated grains read zeros), VHD fixed (data minus footer) and dynamic (BAT + per-block sector bitmaps, state-run splitting), and the og's LBA0 MBR/GPT classification (FUN_140178250's model: 55AA + partition entries vs the 0xEE protective entry / EFI PART header). Layering honesty: the core stays OS-header-free (its own build rule) — every byte flows through the host-injected `ImageByteSource`; the CLI provides the FILE source (`rrecov.cpp` `CliImageSource`, `_wfopen_s`), tests/fuzz provide memory sources. The CLI grammar: `scan --image FILE` and `recover --image FILE` (the F12 dest-overlap warning is disk-scoped, guarded for image sources), with the og's print shape (size + layout) at open; the identity card carries the image path (ScanConfig.volume_image → VolumeInfo v3 tail → VolumeSessionInfo.image; v1/v2 sessions load with an empty path, append-only, old readers unaffected). Deliberate exclusions + remainders registered as **PARITY-DELTAS D-64** (edisk/dmg exclusions; VHDX + parent-chains/compressed-grains/differencing as tracked capability remainders — ERR_UNSUPPORTED, never silent). Scope note recorded for the next session: the UI shim's `cr_ui_open_image` keeps its flat-only reader (the S-008 writer coupling deserves its own careful migration onto the core module — the seam is the byte source, mechanical follow-up, tracked here rather than risked mid-flight).
=======
## Resolution (session-10 convergence upgrade — two parallel lanes, union landed)

**Status:** FIXED (core scope; two registered remainders — D-64 converged entry)
**Fixed in commits:** b30354d (first landing: flat/VMDK/VHD + CLI --image) + this push (convergence: VHDX dynamic + log overlay, VMDK parent chains, VHD differencing, .dmg flat-parity correction, image_path widened to 512, qemu compat fixtures; supersedes the first landing's ImageByteSource/ImageFileOps seam with the single ImageFileIo seam)
**Verified by:** 20 tests in tests/unit/test_image.cpp, all fail-before by
absence (no image code path existed — grep-verified at audit time):
`Image.FlatFileScansMbr`, `Image.FlatRejectsEmptyAndMissing`,
`Image.ImageRejectsTruncatedFile`, `Image.EdiskRefusedAsRegisteredExclusion`,
`Image.BadMagicVmdkRefusedNotFlatFallback`, `Image.VmdkSparseExtentReads`,
`Image.VmdkDifferencingChainReadsParent`, `Image.VmdkStreamOptimizedRefused`,
`Image.VhdDynamicDisk`, `Image.VhdDifferencingReadsParentOnClearBitmap`,
`Image.VhdBadChecksumRefused`, `Image.VhdxDynamicReads`,
`Image.VhdxLogOverlayServesUnflushedWrites`, `Image.VhdxDifferencingRefusedPrecisely`,
`Image.Lba0LayoutClassification`, `Image.ExtensionClassificationMatchesOgDispatch`,
`Image.SessionRoundTripPersistsImagePath`, `Image.QemuVmdkCompat`,
`Image.QemuVhdCompat`, `Image.QemuVhdxCompat`. Full suite 555/555 Linux
Release (was 533/533 at session start). The three `Qemu*Compat` tests run
against real qemu-img 10.0.13 images committed under tests/data/images
(generation commands recorded in tests/data/images/README.md) — the
compatibility proof that the parsers read what real tools write, not just
self-built structures. That proof layer earned its keep during development:
it caught a descriptor-field transposition and a metadata-region GUID
transposition that the in-memory builders (written from the same
misreading) could never catch.

**Resolution notes:** implemented exactly the finding's fix plan, verified
against the corpus dispatch semantics (extension factory
DRWUI:948237-948409, flat fallback :948389-948393, LBA0 classify
:275877-275880, corrupt-VMDK refusal :978668) and against the public
on-disk formats, empirically pinned structure-by-structure on
qemu-img-10.0.13-written files (VMDK packed sparse header — capacity at
0x0C, not the padded 0x10 a from-spec-memory implementation would write;
VHD one's-complement checksum rule — 32-bit ~S comparison, verified
against qemu's own footer bytes; VHDX log descriptor layout —
trailing@0x04/leading@0x08/file_offset@0x10/sequence@0x18 and the
metadata-region GUID data3=4B9A, both pinned byte-exact on the qemu file).
Scope landed: pure-core `cr::storage::open_image` (include/cr/storage/
cr_image.h + src/core/storage/cr_image.cpp) with the injected
`ImageFileIo` seam (Win32 impl src/win/cr_image_win.cpp — CreateFileW +
GetFileSizeEx, the og's own primitives); flat raw (.img/.dmg/.raw/unknown —
the og does NOT parse Apple UDIF for .dmg, it opens the file raw, and
parity means we do the same); VMDK monolithicSparse + descriptor extents
(FLAT/SPARSE/ZERO), grain-directory/grain-table translation at 512-byte
sectors (the libvmdk contract, Device.mo:49106-49124), differencing chains
via parentFileNameHint with depth cap; VHD fixed + dynamic + differencing
(footer/dynamic-header checksums, BAT, 512-byte sector bitmaps, W2ku/W2ru
parent locators); VHDX dynamic (header pair with CRC-32C + highest-sequence
selection, region table pair, metadata, chunk-ratio BAT interleave) with a
READ-ONLY in-memory log replay overlay (qemu vhdx-log.c walk semantics —
see D-64d: a recovery tool never rewrites its input; qemu itself refuses
read-only opens with a valid log — we instead serve the journaled bytes).
CLI: `scan --image` and `recover --image` (usage text updated; LBA0
classification printed at open like the og's construction; repair on an
image target stays analysis-only — the writer hook is not wired, the
orchestrator's documented "repair unavailable" path). Session: identity
card v3 tail `image_path[512]` (VolumeInfo + VolumeSessionInfo +
ScanConfig.volume_image; v1/v2 records deserialize with an empty tail —
pinned in Crsf.VolumeInfoIdentityCardRoundTrip); the resume path warns on
image/target mismatch like the model/serial identity. Registered
non-adoptions (PARITY-DELTAS D-64): .edisk proprietary (refused,
ERR_UNSUPPORTED), compressed VMDK streamOptimized/VMFS families (refused),
VHDX differencing (refused — parent-locator metadata + SB blocks are the
next concrete step, tracked here). M-046's 4Kn note: image devices are
file-reads (no direct-I/O alignment constraints; M-009's 64 KiB envelope
does not apply to file-backed reads) — VHDX logical-sector-size metadata
(512/4096) drives geometry. Win7-inclusive gate: the Win32 IO uses
CreateFileW/GetFileSizeEx/ReadFile only — no new import families.
>>>>>>> 0b24f69 (fix(S-030): disk-image scan targets — flat/VMDK/VHD/VHDX as first-class devices)
