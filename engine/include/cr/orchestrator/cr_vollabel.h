/* cr_vollabel.h - engine-side on-disk volume-label decode (N-133).
 *
 * The og's engines decode the scanned volume's on-disk label themselves -
 * NTFS from $Volume's $VOLUME_NAME attribute (type 0x60, the
 * instruction-decisive decode: NTFSSearchDLL_decompiled.c:647-676 +
 * FUN_18003f3a0 at :50963-50980), ext4 from s_volume_name
 * (ext4-hfs-engine-algorithms-v2.md SS2 step 5) - and surface it to the
 * volume node/UI; this works identically for image scans (the decodes read
 * the volume's own metadata, no OS mount required). The probe below gives
 * the core the same capability at the orchestrator's volume seam: one call
 * per volume slice, the family detected from the boot sector, the label
 * decoded per family.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_VOLLABEL_H
#define CR_VOLLABEL_H

#include <cr/cr_types.h>
#include <cr/storage/cr_device.h>

namespace cr {
namespace orchestrator {

/* Decode the on-disk volume label of the slice's filesystem into out
 * (NUL-terminated, max 32 chars + NUL). Returns OK with out[0] == 0 when
 * the family is recognized but the label is empty/absent (a label-less
 * volume - the caller keeps its fallback), OK with a label when decoded,
 * ERR_UNSUPPORTED when the boot sector matches no known family (the
 * caller keeps its fallback), or a read error. Never aborts a scan:
 * label decode is informational (the og's own posture - a failed $Volume
 * read leaves the volume unlabeled, the scan proceeds). */
u32 probe_slice_volume_label(IStorageDevice* slice, char out[33]);

} // namespace orchestrator
} // namespace cr

#endif // CR_VOLLABEL_H
