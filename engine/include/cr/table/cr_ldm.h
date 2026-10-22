/*
 * cr_ldm.h - LDM (Logical Disk Manager / dynamic disk) database recognition.
 *
 * Corpus: LdmManager.mo (VMDB parser FUN_180072790, TOCBLOCK parser
 * FUN_180072ad0, decompile-verified): VMDB magic 0x564D4442 with version
 * 4.10, big-endian fields, PRIVHEAD chain validation ("PRIVHEAD size
 * doesn't match real disk size"), TOCBLOCK with config/log bitmap slots.
 *
 * Recognition only (R3.3): dynamic disks are reported with their disk
 * group identity and database status; spanned/striped/mirrored volume
 * reconstruction is explicitly out of scope (single-disk product).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_LDM_H_
#define CR_LDM_H_

#include "../storage/cr_device.h"
#include <string>
#include <vector>

namespace cr {
namespace table {

struct LdmInfo {
    bool found = false;
    u64 db_offset = 0; /* byte offset of the TOCBLOCK/VMDB hit */
    u16 vmdb_version_major = 0;
    u16 vmdb_version_minor = 0;
    u16 status = 0;        /* 1 = healthy (corpus semantics) */
    u64 disk_group_id = 0; /* BE u64 @VMDB+0x75 */
    u32 vblk_size = 0;
    u32 vblk_count = 0;
    u32 vblk_offset = 0;
    bool tocblock_ok = false;    /* config + log bitmap slots present */
    std::string disk_group_name; /* from TOCBLOCK config bitmap name */
};

/* Scan the last MiB (plus the two classic private-region offsets) for the
 * LDM database: PRIVHEAD / TOCBLOCK / VMDB signatures. Report-only. */
u32 parse_ldm_database(IStorageDevice* device, LdmInfo& out);

} // namespace table
} // namespace cr

#endif // CR_LDM_H_
