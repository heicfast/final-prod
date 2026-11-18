/*
 * cr_image_create.h - S-040: the flat-raw disk-image producer (win layer).
 *
 * The og's home page ships a "Create Image" flow (UIUX/07:36-37; the
 * imaging capability lives in ToolsModule.dll, mom-module-system :87) and
 * the product then recommends scanning the produced image as the damage
 * posture for failing disks (UIUX/11:17). This is the producer half the
 * core lacked (S-040); the reader half is S-030 (storage::open_image).
 * Flat raw only - VHD/VHDX writers are out of scope (registered
 * divergence D-147: the og's own reader accepts flat, and the corpus does
 * not decode the og's writer format beyond the libvmdk link).
 *
 * Disposition policy (the truth contract, S-040's acceptance):
 *   - the stream is written to `<final>.crimg-tmp` (CREATE_ALWAYS);
 *   - on completion: flush + rename to `<final>` (atomic commit - the
 *     final path appears ONLY for a complete image) + the SHA-256 sidecar
 *     `<final>.sha256` (the standard `<hex>  <basename>` form);
 *   - on cancel or failure: the handle closes and the partial file is
 *     RENAMED to `<final>.partial` - kept on disk, explicitly named,
 *     reported through the return code and the stats. Never deleted,
 *     never silently passed off as complete. (If the rename itself
 *     fails, the partial stays under its .crimg-tmp name - still
 *     truthful; the original error code is returned.)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_IMAGE_CREATE_H_
#define CR_IMAGE_CREATE_H_

#include "cr/storage/cr_device.h"
#include "cr/storage/cr_image.h"
#include <string>

namespace cr {
namespace win {

/* Produce a flat raw image of `src` (any device: the direct-transport disk
 * view, an image reader, a test device) at out_path_utf16. stats (may be
 * null) carries the committed prefix. Privilege is NOT checked here:
 * the caller opens the device (see create_disk_image below for the
 * class-(a) convenience form). */
u32 create_image_file(IStorageDevice& src, const std::u16string& out_path_utf16,
                      CancelToken* cancel, storage::ImageCreateProgressFn progress,
                      void* progress_user, storage::ImageCreateStats* stats);

/* The class-(a) convenience form: opens physical disk N through the
 * direct read view (win::open_disk) and produces its image. The
 * privilege check is the CALLER's precondition (the CLI's
 * require_privilege() gate and the ABI's caller both refuse first - same
 * posture as scan/recover/fix/apply/bench). */
u32 create_disk_image(u32 disk, const std::u16string& out_path_utf16, CancelToken* cancel,
                      storage::ImageCreateProgressFn progress, void* progress_user,
                      storage::ImageCreateStats* stats);

/* D-IMG-3 (the installer-integration session, INTEGRATION-MATRIX M02): the
 * RANGED producer. create_image_file_range copies the byte range
 * [first_byte, first_byte + length) of src (first_byte 0 + length ==
 * src.size() is byte-identical to create_image_file over the same device -
 * create_image_file delegates here); the disposition policy is identical
 * (temp / atomic commit + sidecar / the truthful .partial). A range that
 * exceeds the device refuses with ERR_BAD_PARAMETER (never a silent
 * truncation). */
u32 create_image_file_range(IStorageDevice& src, u64 first_byte, u64 length,
                            const std::u16string& out_path_utf16, CancelToken* cancel,
                            storage::ImageCreateProgressFn progress, void* progress_user,
                            storage::ImageCreateStats* stats);

/* The SECTOR-denominated disk form (the ABI's twin, D-IMG-3): produce the
 * flat raw image of physical disk N's SECTOR RANGE [first_sector,
 * first_sector + sector_count). sector_count 0 = from first_sector to the
 * END of the disk; 0/0 = the whole disk = byte-identical to
 * create_disk_image. The sector validation happens HERE (the geometry
 * exists after the open): first_sector beyond the disk, or a range that
 * runs past the end, refuses with ERR_BAD_PARAMETER - the og's "Invalid
 * sector" posture at the engine layer (DiskBackupSaveDlg
 * fromLabel/toLabel, strings:11640-11760). */
u32 create_disk_image_ex(u32 disk, const std::u16string& out_path_utf16, u64 first_sector,
                         u64 sector_count, CancelToken* cancel,
                         storage::ImageCreateProgressFn progress, void* progress_user,
                         storage::ImageCreateStats* stats);

} // namespace win
} // namespace cr

#endif // CR_IMAGE_CREATE_H_
