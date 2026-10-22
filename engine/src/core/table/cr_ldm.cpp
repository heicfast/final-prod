/* cr_ldm.cpp - LDM database recognition (report-only).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/table/cr_ldm.h"
#include <cstring>

namespace cr {
namespace table {

namespace {

/* The LDM private region: 1 MiB aligned copies at the end of the disk and
 * the classic 1 MiB/last-MiB positions. TOCBLOCK at +0x200 within a copy,
 * VMDB follows the config bitmap (we scan a window for the magics). */
constexpr u64 LDM_SCAN_BYTES = 1ull << 20;
constexpr u32 TOCBLOCK_SIG = 0x544F4342; /* "TOCB" (LE read of "TOCBLOCK") */
constexpr u32 VMDB_SIG = 0x564D4442;     /* "VMDB" */

u16 rd16be(const u8* p) {
    return u16((u32(p[0]) << 8) | p[1]);
}
u32 rd32be(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}
u64 rd64be(const u8* p) {
    return (u64(rd32be(p)) << 32) | rd32be(p + 4);
}

} // namespace

u32 parse_ldm_database(IStorageDevice* device, LdmInfo& out) {
    if (device == nullptr)
        return ERR_BAD_PARAMETER;
    const u64 size = device->size();
    if (size < LDM_SCAN_BYTES * 2)
        return ERR_BAD_PARAMETER;

    /* the private region: last MiB of the disk (primary) and the MiB
     * before it (secondary copy) */
    const u64 bases[2] = {size - LDM_SCAN_BYTES, size - 2 * LDM_SCAN_BYTES};
    for (u64 base : bases) {
        std::vector<u8> buf(size_t(LDM_SCAN_BYTES / 4)); /* scan 256K */
        if (device->read_at(base, buf.data(), u32(buf.size())) != OK)
            continue;
        /* TOCBLOCK sits at a small offset within the region (0x0/0x200) */
        for (u64 off = 0; off + 0x80 <= buf.size(); off += 0x100) {
            if (std::memcmp(buf.data() + off, "TOCBLOCK", 8) != 0)
                continue;
            out.tocblock_ok = true;
            out.db_offset = base + off;
            /* first bitmap name MUST be "config" (+0x24), second "log"
             * (+0x46) - the decompile-verified slots */
            if (off + 0x46 + 8 <= buf.size()) {
                if (std::memcmp(buf.data() + off + 0x24, "config", 6) == 0)
                    out.disk_group_name.assign("config present");
            }
            /* find VMDB within the following window (the database header
             * follows the TOCBLOCK bitmap area) */
            for (u64 vo = off + 0x80; vo + 0x90 < buf.size(); vo += 8) {
                if (std::memcmp(buf.data() + vo, "VMDB", 4) != 0)
                    continue;
                const u8* v = buf.data() + vo;
                /* BE fields: version u16@+0x12 (4) u16@+0x14 (10) */
                out.vmdb_version_major = rd16be(v + 0x12);
                out.vmdb_version_minor = rd16be(v + 0x14);
                out.disk_group_id = rd64be(v + 0x75);
                out.vblk_size = rd32be(v + 0x10);
                out.vblk_count = rd32be(v + 0x08);
                out.vblk_offset = rd32be(v + 0x0C);
                out.status =
                    rd16be(v + 0x10 + 0); /* corpus: status BE u16 (0=healthy sentinel differs) */
                out.found = out.vmdb_version_major == 4; /* corpus: must be 4.10 */
                return out.found ? OK : ERR_BAD_MAGIC;
            }
            out.found = true; /* TOCBLOCK without VMDB: damaged but LDM */
            return ERR_BAD_CHECKSUM;
        }
    }
    return ERR_BAD_MAGIC;
}

} // namespace table
} // namespace cr
