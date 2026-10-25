/*
 * cr_image.h - disk-image scan targets (S-030).
 *
 * The og treats disk-image FILES as first-class scan targets: DRWUI's
 * "Import a disk image" flow dispatches on extension to per-format backends
 * (edisk/vhdx/vmdk/vhd) with a flat-file fallback for unrecognized
 * extensions (.img/.dmg/raw) - corpus
 * re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c :948237-948409
 * ("edisk"->FUN_1400dafc0 :948270, "vhdx"->CVHDXDisk :948293,
 * "vmdk"->VmdkDevice :948311, "vhd"->CVHDDisk :948347; flat fallback
 * CreateFileW + GetFileSizeEx :948389-948393), reads LBA0 (0x1000 bytes)
 * and classifies MBR/GPT (:275877-275880), and Device.mo exposes a
 * libvmdk-backed VMDK block device with extent files and parent chains
 * (Device_decompiled.c :49106-49124, :48965-49100).
 *
 * This module is the clean-room equivalent, in the pure core:
 *   - ImageFileIo is the injected raw-file IO (Win32 implementation in the
 *     platform layer; in-memory implementation in tests) - the core stays
 *     free of OS headers.
 *   - open_image() dispatches exactly like the og: extension first, then
 *     content verification. A .vmdk/.vhd/.vhdx that fails its magic check
 *     is an error (the og's embedded parser: "Unsupported or corrupt VMDK
 *     (magic mismatch)", DRWUI:978668), NOT a silent flat fallback.
 *   - .img/.dmg/raw/unknown extensions take the flat-file fallback exactly
 *     like the og (the og does NOT parse Apple UDIF for .dmg - it opens
 *     the file raw; parity means we do the same).
 *   - .edisk (EaseUS Todo-Backup proprietary format, undecoded in the
 *     corpus) is a registered deliberate exclusion: refused with
 *     ERR_UNSUPPORTED (see docs/PARITY-DELTAS.md).
 *
 * Formats implemented (read-only, evidence integrity preserved):
 *   - Flat raw (any size, 512-byte logical sectors)
 *   - VMDK: monolithicSparse + descriptor-with-extent-files (FLAT and
 *     SPARSE extents, version 1/2/3 sparse headers, grain directory /
 *     grain table translation at 512-byte sectors like libvmdk), plus
 *     differencing chains via parentFileNameHint (depth-capped).
 *     streamOptimized (compressed) extents are refused (ERR_UNSUPPORTED).
 *   - VHD: fixed + dynamic + differencing (footer/dynamic-header/BAT,
 *     512-byte sector-bitmap per block, parent chains via W2KU/W2RU
 *     locators, depth-capped).
 *   - VHDX: dynamic (header pair, region table, metadata, BAT with the
 *     chunk-ratio sector-bitmap interleave) with a READ-ONLY in-memory
 *     log replay overlay (a recovery tool never rewrites its input;
 *     qemu itself refuses read-only opens with a valid log sequence -
 *     we instead serve the logged bytes from the log region).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_IMAGE_H_
#define CR_IMAGE_H_

#include "cr_device.h"
#include <memory>
#include <string>

namespace cr {
namespace storage {

/* Raw-file IO seam (the platform layer owns real files; tests own memory).
 * Handles are nonzero ids valid until close(); open returns 0 on failure. */
class ImageFileIo {
public:
    virtual ~ImageFileIo() = default;
    /* open a file by UTF-16 path; 0 = failure (err carries the code) */
    virtual u32 open(const std::u16string& path_utf16, u32* err) = 0;
    /* exact-size read; OK fills len bytes, else an error code */
    virtual u32 read(u64 file, u64 offset, void* buffer, u32 len) = 0;
    virtual u64 size(u64 file) const = 0;
    virtual void close(u64 file) = 0;
};

/* Detected image container (classification mirrors the og's extension
 * dispatch; INFO only - never changes read semantics). */
enum class ImageFormat : u32 {
    Flat = 0, /* .img/.dmg/.raw/unknown - the og's flat fallback */
    Vmdk = 1,
    Vhd = 2,
    Vhdx = 3,
};

/* LBA0 layout classification - the og reads the first 0x1000 bytes and
 * classifies MBR/GPT at device construction (DRWUI:275877-275880). */
enum class Lba0Layout : u32 {
    Unknown = 0,
    Mbr = 1, /* 0x55AA at 510, no GPT signature */
    Gpt = 2, /* "EFI PART" at 512 (protective MBR optional) */
};

/* S-030 image error codes (0x500A family - see cr_error.h). */

/* Open an image file as a storage device. The returned device is
 * self-contained: it holds every file handle it needs (extent files,
 * parent-chain images) until destruction. Errors: ERR_BAD_MAGIC (extension
 * says vmdk/vhd/vhdx but content does not verify), ERR_BAD_CHECKSUM
 * (footer/header checksum mismatch), ERR_TRUNCATED (structural metadata
 * beyond EOF), ERR_UNSUPPORTED (.edisk, compressed VMDK extents, VHDX
 * differencing), ERR_DEPTH_LIMIT (parent chain too deep), ERR_OPEN_FAILED /
 * ERR_READ_FAILED (IO), ERR_BAD_PARAMETER (structural nonsense). */
std::unique_ptr<IStorageDevice> open_image(ImageFileIo& io, const std::u16string& path_utf16,
                                           u32* err);

/* The extension/format classification the og's factory performs
 * (DRWUI:948237-948409). Pure function over the path. */
ImageFormat classify_extension(const std::u16string& path_utf16);

/* LBA0 classification (see Lba0Layout). len must be >= 512; shorter
 * buffers classify as Unknown. */
Lba0Layout classify_lba0(const u8* lba0, size_t len);

} // namespace storage
} // namespace cr

#endif // CR_IMAGE_H_
