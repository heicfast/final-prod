/*
 * cr_image.h - disk-image scan targets (S-030): file-backed storage
 * devices with per-format parsing (flat / VMDK sparse / VHD dynamic).
 *
 * Corpus lineage: the og treats image FILES as first-class scan targets.
 * DRWUI's import flow dispatches on extension to per-format backends
 * (edisk, vhdx, vmdk, vhd) with a flat-file fallback for raw/.img
 * (FUN_1405b3410 :948237-948409; the flat fallback CreateFileW +
 * GetFileSizeEx :948389-948393), reads LBA0 and classifies MBR/GPT
 * (FUN_140178250 :275877-275880). Device.mo's VMDK block device reads
 * through libvmdk at 512-byte sector granularity incl. extent files and
 * parent chains (FUN_1800545c0 :49106-49124, FUN_180054120
 * :48965-49100). This module re-implements that surface from format
 * understanding - no libvmdk, no decompiled code.
 *
 * Layering: the core stays OS-header-free - every byte flows through the
 * injected ImageByteSource/ImageFileOps seams (the CLI and the UI shim
 * provide FILE-backed sources; tests and the fuzz harness provide
 * memory sources). VHDX and the proprietary edisk/Apple dmg containers
 * are out of scope v1 (tracked in the finding's Resolution +
 * PARITY-DELTAS for the deliberate exclusions).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_IMAGE_H_
#define CR_IMAGE_H_

#include "cr_device.h"
#include <memory>

namespace cr {
namespace storage {

/* ---- byte sources (host-provided) -------------------------------------- */

/* A seekable read-only byte source. Contract: read fills exactly len
 * bytes or returns an error (ERR_READ_FAILED / ERR_RANGE_OUT_OF_BOUNDS);
 * engines never see the source, only the device over it. Hosts provide
 * FILE*-backed sources (UTF-8 paths); tests/fuzz provide memory. */
class ImageByteSource {
public:
    virtual ~ImageByteSource() = default;
    virtual u32 read(u64 at, void* buf, u32 len) = 0;
    virtual u64 size() const = 0;
};

/* Opener for SIBLING files a multi-file image references (VMDK extent
 * files, resolved relative to the descriptor's directory). The platform
 * owner (CLI / UI / test) supplies this; open_image never invents paths. */
struct ImageFileOps {
    u32 (*open)(void* user, const char* utf8_path, std::unique_ptr<ImageByteSource>& out);
    void* user = nullptr;
    bool valid() const { return open != nullptr; }
};

/* ---- format dispatch ---------------------------------------------------- */

enum class ImageFormat : u8 { Flat = 0, Vmdk, Vhd, Vhdx, Edisk, Unknown };

/* Content sniff over the first bytes of the image (extension-independent
 * truth; the og's factory dispatches on extension but every backend
 * validates content first - "Unsupported or corrupt VMDK (magic
 * mismatch)", DRWUI :978668). */
ImageFormat sniff_image_format(const u8* first_bytes, size_t n);

/* Build the scan device over an image source.
 *
 *  - Flat: the whole source (sector-granular bound, like every device
 *    here - sub-sector tail bytes are not addressable).
 *  - Vmdk: a monolithicSparse image (embedded descriptor, the source is
 *    the extent) or a standalone text descriptor whose extent lines
 *    (FLAT / SPARSE / ZERO) are opened via `ops` relative to
 *    `utf8_base_dir`. Sparse extents read through the grain-directory /
 *    grain-table mapping; unallocated grains read as zeros.
 *  - Vhd: fixed (data minus the trailing footer) or dynamic (BAT +
 *    per-block sector bitmaps); unallocated blocks read as zeros.
 *
 * Errors: ERR_BAD_MAGIC (a recognizable container with a bad header),
 * ERR_UNSUPPORTED (VHDX/edisk/dmg containers, VMDK parent chains /
 * compressed grains / VMFS extent types, VHD differencing disks),
 * ERR_OPEN_FAILED / ERR_READ_FAILED (io), ERR_BAD_PARAMETER.
 *
 * v1 scope honesty (tracked in the S-030 Resolution): no VMDK parent
 * chains (differencing), no compressed grains, no VHDX. */
u32 open_image(std::unique_ptr<ImageByteSource> src, const ImageFileOps& ops,
               const char* utf8_base_dir, std::unique_ptr<IStorageDevice>& out);

/* ---- the og's LBA0 classification ---------------------------------------
 * FUN_140178250 reads the first 0x1000 bytes and classifies the layout
 * (MBR vs GPT); one sector of the MBR plus the protective-entry check
 * carries the same truth. */
enum class BootLayout : u8 { Unknown = 0, Mbr, Gpt };
BootLayout classify_boot_layout(IStorageDevice* dev);

} // namespace storage
} // namespace cr

#endif /* CR_IMAGE_H_ */
