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
