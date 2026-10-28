# modules/imaging.md — disk-image scan targets (S-030; new module row, session 8)

The og treats image FILES as first-class scan targets (DRWUI "Import a
disk image": extension dispatch `FUN_1405b3410` → edisk/vhdx/vmdk/vhd
backends + the flat CreateFileW/GetFileSizeEx fallback for raw/.img/.dmg;
LBA0 read + MBR/GPT classification `FUN_140178250`; Device.mo's
libvmdk-backed VMDK block device at 512-byte sector granularity
`FUN_1800545c0`/`FUN_180054120`). This module row tracks the core's
equivalent surface.

## Session-8 verdict (fix session — S-030 closed)

**S-030 FIXED** (this session's S-030 commit; finding → findings/FIXED/
with the full Resolution).

| Parameter | Verdict | Evidence |
|---|---|---|
| P1 (detection — image inputs visible) | **MATCH** | `include/cr/storage/cr_image.h` + `src/core/storage/cr_image.cpp`: `open_image` content-sniffs (VMDK magic / text descriptor; VHD by the TRAILING "conectix" footer — fixed disks carry raw data at offset 0) and dispatches flat / VMDK / VHD; under-a-sector non-descriptors refused; VHDX/edisk → ERR_UNSUPPORTED (never silent) |
| P8 (multi-volume/boundaries — image-bounded devices) | **MATCH** | flat device: sector-granular bound over [offset, bytes); VMDK: extent-concatenated logical disk (FLAT/SPARSE/ZERO); VHD dynamic: BAT + per-block sector bitmaps; unallocated grains/blocks read zeros (the sparse model) |
| P42 (public surface — target selection) | **MATCH** | CLI `scan --image FILE` / `recover --image FILE` (`rrecov.cpp`), the og's open-time print (size + layout), the F12 dest-overlap warning guarded for image sources |
| P47 (throughput — extent translation) | **MATCH (v1 shape)** | grain-segment batching (one GDE/GTE lookup per grain run), state-run splitting in the VHD reader; no libvmdk dependency (re-implemented from format understanding) |
| P54 (module boundaries — image backends) | **MATCH** | the format logic is OS-header-free pure core over injected `ImageByteSource`/`ImageFileOps` seams; the CLI supplies the FILE source; tests/fuzz supply memory sources; the UI shim's flat-only `cr_ui_open_image` migration onto this module is the recorded mechanical follow-up (S-008 writer coupling — not risked mid-flight) |
| session identity (v3 tail) | **MATCH** | `ScanConfig.volume_image` → `VolumeInfo.image` (80+84+256 layout, v1/v2 back-compat) → `VolumeSessionInfo.image`; `Image.SessionRoundTripCarriesTheImagePath` + the updated `Crsf.VolumeInfoIdentityCardRoundTrip` |

Deliberate exclusions + tracked remainders: **PARITY-DELTAS D-64** (edisk /
dmg excluded — proprietary / Apple containers; VHDX, VMDK parent chains +
compressed grains, VHD differencing = capability remainders, honest
ERR_UNSUPPORTED).

Fuzz: `fuzz/fuzz_image.cpp` (memory byte source; head/tail/unaligned/
block-boundary read shapes + the bounds-refusal contract) — wired into
the fuzz suite's Makefile (target + run + corpus), executed on the CI
fuzz lane.


## Session-10 convergence update (the VHDX + differencing families + the compatibility proof layer)

The parallel S-030 landing above (flat/VMDK/VHD) was converged with a
second lane's superset implementation; the merged surface is now:

| Parameter | Verdict | Evidence |
|---|---|---|
| VHDX dynamic | **MATCH** (header pair CRC-32C + highest-sequence selection, region table, metadata incl. block size / virtual size / logical sector, chunk-ratio BAT interleave; states 0/1/2/3/5 read zeros, 6 = data, 7 without parent = refused) | `Image.VhdxDynamicReads` |
| VHDX log replay | **MATCH-with-hardening D-64d** (read-only in-memory overlay serving journaled bytes — qemu vhdx-log.c walk: guid match, seq+1 chains, CRC-32C over header+descriptors+data, flushed-offset truncation rule) | `Image.VhdxLogOverlayServesUnflushedWrites` |
| VMDK parent chains | **MATCH** (parentFileNameHint, relative resolution, depth cap 32; unallocated grains read the parent at the same virtual offset) | `Image.VmdkDifferencingChainReadsParent` |
| VHD differencing | **MATCH** (BAT + 512-byte sector bitmaps, bit-clear = parent, W2ku/W2ru UTF-16BE locators) | `Image.VhdDifferencingReadsParentOnClearBitmap` |
| .dmg | **MATCH** (the og's factory has NO dmg case — .dmg rides the flat fallback as raw bytes; ours does the same, no UDIF parsing) | `Image.ExtensionClassificationMatchesOgDispatch` |
| refusals | **MATCH** (corrupt .vmdk/.vhd/.vhdx magic → error, never silent flat fallback — DRWUI:978668; streamOptimized/VMFS/VHDX-diff/edisk → ERR_UNSUPPORTED) | `Image.BadMagicVmdkRefusedNotFlatFallback`, `Image.VmdkStreamOptimizedRefused`, `Image.VhdxDifferencingRefusedPrecisely`, `Image.EdiskRefusedAsRegisteredExclusion` |
| session identity v3 | **MATCH** (image_path[512] tail — widened from the first landing's 256; v1/v2 back-compat pinned) | `Crsf.VolumeInfoIdentityCardRoundTrip`, `Image.SessionRoundTripPersistsImagePath` |
| real-world compat | **NEW PROOF LAYER** (qemu-img 10.0.13 fixtures committed under tests/data/images with generation provenance; caught two spec-memory transpositions — the VHDX log-descriptor field order and the metadata-region GUID data3 — that in-memory builders could not) | `Image.QemuVmdkCompat`, `Image.QemuVhdCompat`, `Image.QemuVhdxCompat` |

The v1 seam (`ImageByteSource`/`ImageFileOps`) was superseded by the
converged `ImageFileIo` single seam (the core opens paths itself through
the injected IO; Win32 implementation `src/win/cr_image_win.cpp`). The
`cr_ui_open_image` flat-only UI-shim migration follow-up recorded above
stays open.


## Session-8 section (2026-10-28, audited `35adcb9`)

**S-040** (SEVERE — the producer half absent: no `cr_ui_image_create`, no
writer, no `image` verb; the og's home-page "Create Image" flow and the
corpus's image-scan-first damage posture (UIUX/11:17) are unreachable; the
reader half (S-030) is complete). Fix = a flat-image writer over the dest-I/O
abstractions, driver class (a), SHA256 sidecar, cancel support.
