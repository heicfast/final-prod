/*
 * cr_rescue_win.h - the Win32 platform driver for ddrescue-style
 * two-pass imaging (F-33/F-34, Wave 6).
 *
 * Disposition policy (deliberately DIFFERENT from the flat producer's
 * temp+rename, and why): the rescue flow writes the destination DIRECTLY
 * at the final path - a run must be resumable across process restarts, and
 * a temp+rename commit would orphan the mapfile's claims every run. The
 * mapfile (<out>.mapfile) is the commit marker:
 *   - a map whose blocks are ALL '+' (Finished) certifies a complete image;
 *   - cancel/failure leaves the destination + mapfile on disk, resumable
 *     by re-running the same command (auto-resume);
 *   - resume REFUSES when the map does not match the source (wrong map),
 *     or when the map claims Finished bytes the destination does not hold
 *     (missing/short dest) - never a silent hole.
 *
 * No SHA-256 sidecar for rescue images by design (see cr_rescue.h): an
 * image with NonTried holes is not a byte-image of the source; the
 * mapfile is the truth artifact.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RESCUE_WIN_H_
#define CR_RESCUE_WIN_H_

#include "cr/storage/cr_device.h"
#include "cr/storage/cr_rescue.h"
#include <string>

namespace cr {
namespace win {

/* Two-pass rescue image of `src` at out_path_utf16 (mapfile at
 * "<out>.mapfile"). Auto-resume: an existing valid mapfile continues the
 * run (the refusals above are returned as ERR_BAD_PARAMETER with the
 * reason logged). cfg tunes the passes (chunk/skip/retries/reverse/
 * checkpoint cadence - the UI presets map onto it). The reverse mode
 * marks the destination sparse up front (FSCTL_SET_SPARSE, best-effort -
 * a FAT/exFAT destination silently declines). stats (may be null) always
 * carries the map-derived truth. Privilege is the CALLER's precondition
 * (same posture as every producer here). */
u32 create_rescue_image(IStorageDevice& src, const std::u16string& out_path_utf16,
                        storage::RescueImagerConfig cfg, CancelToken* cancel,
                        storage::RescueProgressFn progress, void* progress_user,
                        storage::RescueStats* stats);

} // namespace win
} // namespace cr

#endif // CR_RESCUE_WIN_H_
