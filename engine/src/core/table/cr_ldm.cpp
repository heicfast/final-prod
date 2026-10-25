/* cr_ldm.cpp - LDM dynamic-disk database: full decode + volume devices.
 *
 * Every on-disk constant is verified against the Linux kernel parser
 * (scripts/ldm_reference.{c,h}), the flatcap LDM technical documentation
 * and libldm (mdbooth/ldmtool); see the header's ground-truth ledger.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_wire.h"
#include <algorithm>
#include <cstring>

namespace cr {
namespace table {

namespace {

/* ---------------------------------------------------------------- */
/* Big-endian readers (LDM is BE throughout)                          */
/* ---------------------------------------------------------------- */

u16 rd16be(const u8* p) {
    return u16((u32(p[0]) << 8) | p[1]);
}
u32 rd32be(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | p[3];
}
u64 rd64be(const u8* p) {
    return (u64(rd32be(p)) << 32) | rd32be(p + 4);
}

/* ---------------------------------------------------------------- */
/* Variable-width fields (flatcap "P" = 1-byte length prefix)         */
/* ---------------------------------------------------------------- */

/* vnum: 1 width byte w (0..8), then w big-endian payload bytes. */
bool get_vnum(const u8*& p, const u8* end, u64& out) {
    if (p >= end)
        return false;
    const u32 w = p[0];
    ++p;
    if (w > 8 || p + w > end)
        return false;
    u64 v = 0;
    for (u32 i = 0; i < w; i++)
        v = (v << 8) | p[i];
    p += w;
    out = v;
    return true;
}

/* vstr: 1 width byte w, then w raw bytes (not NUL terminated on disk). */
bool get_vstr(const u8*& p, const u8* end, std::string& out, u32 max_len = 64) {
    if (p >= end)
        return false;
    const u32 w = p[0];
    ++p;
    if (p + w > end)
        return false;
    out.assign(reinterpret_cast<const char*>(p), w > max_len ? max_len : w);
    p += w;
    return true;
}

bool skip_bytes(const u8*& p, const u8* end, u64 n) {
    if (u64(end - p) < n)
        return false;
    p += n;
    return true;
}

/* ---------------------------------------------------------------- */
/* Fixed slot strings (PRIVHEAD GUID slots: fixed width, NUL padded)  */
/* ---------------------------------------------------------------- */

std::string slot_string(const u8* p, u32 slot) {
    u32 n = 0;
    while (n < slot && p[n] != 0)
        ++n;
    return std::string(reinterpret_cast<const char*>(p), n);
}

/* ---------------------------------------------------------------- */
/* Sector-aligned read helper (whole database region reads)           */
/* ---------------------------------------------------------------- */

bool read_exact(IStorageDevice* dev, u64 byte_off, u8* buf, u64 len) {
    u64 done = 0;
    while (done < len) {
        const u32 chunk = len - done > 0x100000 ? 0x100000 : u32(len - done);
        if (dev->read_at(byte_off + done, buf + done, chunk) != OK)
            return false;
        done += chunk;
    }
    return true;
}

/* ---------------------------------------------------------------- */
/* PRIVHEAD (kernel ldm_parse_privhead, libldm struct _privhead)     */
/* ---------------------------------------------------------------- */

constexpr u64 PRIVHEAD_MAGIC = 0x5052495648454144ull; /* "PRIVHEAD" BE64 */

bool parse_privhead(const u8* data, LdmPrivhead& ph) {
    if (rd64be(data) != PRIVHEAD_MAGIC)
        return false;
    ph.ver_major = rd16be(data + 0x0C);
    ph.ver_minor = rd16be(data + 0x0E);
    ph.disk_guid = slot_string(data + 0x30, 64);
    ph.host_guid = slot_string(data + 0x70, 64);
    ph.disk_group_guid = slot_string(data + 0xB0, 64);
    ph.disk_group_name = slot_string(data + 0xF0, 32);
    /* 0x110: u16 unknown1 + 9 pad bytes, then the geometry @0x11B..0x13A */
    ph.logical_disk_start = rd64be(data + 0x11B);
    ph.logical_disk_size = rd64be(data + 0x123);
    ph.config_start = rd64be(data + 0x12B);
    ph.config_size = rd64be(data + 0x133);
    ph.is_vista = (ph.ver_major == 2 && ph.ver_minor == 12);
    if (ph.ver_major != 2 || (ph.ver_minor != 11 && ph.ver_minor != 12))
        return false;
    if (!ldm_parse_guid_text(ph.disk_guid.c_str(), ph.disk_guid_bin))
        return false; /* kernel: invalid GUID = corrupt */
    if (ph.logical_disk_size == 0)
        return false;
    return true;
}

bool privheads_equal(const LdmPrivhead& a, const LdmPrivhead& b) {
    return a.ver_major == b.ver_major && a.ver_minor == b.ver_minor &&
           a.logical_disk_start == b.logical_disk_start &&
           a.logical_disk_size == b.logical_disk_size && a.config_start == b.config_start &&
           a.config_size == b.config_size && a.disk_guid_bin == b.disk_guid_bin;
}

/* ---------------------------------------------------------------- */
/* TOCBLOCK (kernel ldm_parse_tocblock)                              */
/* ---------------------------------------------------------------- */

constexpr u64 TOCBLOCK_MAGIC = 0x544F43424C4F434Bull; /* "TOCBLOCK" BE64 */

bool parse_tocblock(const u8* data, LdmTocblock& toc) {
    if (rd64be(data) != TOCBLOCK_MAGIC)
        return false;
    /* bitmap layout (libldm _tocblock_bitmap, packed): name[8], flags1
     * u16, start u64, size u64, flags2 u64 - matches kernel 0x24/0x2E/
     * 0x36 and 0x46/0x50/0x58 exactly. */
    toc.bitmap1_name = slot_string(data + 0x24, 8);
    toc.bitmap1_start = rd64be(data + 0x2E);
    toc.bitmap1_size = rd64be(data + 0x36);
    toc.bitmap2_name = slot_string(data + 0x46, 8);
    toc.bitmap2_start = rd64be(data + 0x50);
    toc.bitmap2_size = rd64be(data + 0x58);
    if (toc.bitmap1_name != "config")
        return false;
    if (toc.bitmap2_name != "log")
        return false;
    return true;
}

bool tocblocks_equal(const LdmTocblock& a, const LdmTocblock& b) {
    return a.bitmap1_start == b.bitmap1_start && a.bitmap1_size == b.bitmap1_size &&
           a.bitmap2_start == b.bitmap2_start && a.bitmap2_size == b.bitmap2_size &&
           a.bitmap1_name == b.bitmap1_name && a.bitmap2_name == b.bitmap2_name;
}

/* ---------------------------------------------------------------- */
/* VMDB (kernel ldm_parse_vmdb + libldm struct _vmdb)                */
/* ---------------------------------------------------------------- */

bool parse_vmdb(const u8* data, LdmVmdb& vm) {
    if (std::memcmp(data, "VMDB", 4) != 0)
        return false;
    vm.vblk_last = rd32be(data + 0x04);
    vm.vblk_size = rd32be(data + 0x08);
    vm.vblk_offset = rd32be(data + 0x0C);
    vm.update_status = rd16be(data + 0x10);
    vm.ver_major = rd16be(data + 0x12);
    vm.ver_minor = rd16be(data + 0x14);
    vm.disk_group_name = slot_string(data + 0x16, 31);
    vm.disk_group_guid = slot_string(data + 0x35, 64);
    vm.committed_seq = rd64be(data + 0x75);
    vm.pending_seq = rd64be(data + 0x7D);
    vm.n_vols = rd32be(data + 0x85);
    vm.n_comps = rd32be(data + 0x89);
    vm.n_parts = rd32be(data + 0x8D);
    vm.n_disks = rd32be(data + 0x91);
    vm.n_pending_vols = rd32be(data + 0xA1);
    vm.n_pending_comps = rd32be(data + 0xA5);
    vm.n_pending_parts = rd32be(data + 0xA9);
    vm.n_pending_disks = rd32be(data + 0xAD);
    if (vm.ver_major != 4 || vm.ver_minor != 10)
        return false;
    if (vm.vblk_size == 0) {
        /* og LdmManager defaults a zero vblk_size to 0x80 instead of
         * failing (OG:59525-59528); surfaced as degraded by the caller */
        vm.vblk_size = 0x80;
        vm.vblk_size_defaulted = true;
    }
    return true;
}

} // namespace

/* ---------------------------------------------------------------- */
/* GUID text parsing                                                 */
/* ---------------------------------------------------------------- */

namespace {

int hex_val(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

u8 hex_byte(const char* p) {
    return u8((hex_val(p[0]) << 4) | hex_val(p[1]));
}

} // namespace

bool ldm_parse_guid_text(const char* text, LdmGuid& out) {
    if (text == nullptr)
        return false;
    /* accept {G}, G with dashes; 32 hex digits total */
    char t[40];
    u32 n = 0;
    for (const char* p = text; *p && n < 39; ++p) {
        if (*p == '{' || *p == '}')
            continue;
        if (*p == '-')
            continue;
        if (hex_val(*p) < 0)
            return false;
        t[n++] = *p;
    }
    t[n] = 0;
    if (n != 32)
        return false;
    /* Microsoft GUID wire order: first 3 fields LE, rest raw */
    out.b[0] = hex_byte(t + 6);
    out.b[1] = hex_byte(t + 4);
    out.b[2] = hex_byte(t + 2);
    out.b[3] = hex_byte(t + 0);
    out.b[4] = hex_byte(t + 10);
    out.b[5] = hex_byte(t + 8);
    out.b[6] = hex_byte(t + 14);
    out.b[7] = hex_byte(t + 12);
    for (u32 i = 0; i < 8; i++)
        out.b[8 + i] = hex_byte(t + 16 + 2 * i);
    return true;
}

void LdmGuid::to_string(char out[37]) const {
    static const char* hx = "0123456789abcdef";
    /* first 3 fields are stored little-endian (Microsoft wire order) */
    static const u8 order[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};
    u32 p = 0;
    for (u32 i = 0; i < 16; i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            out[p++] = '-';
        const u8 v = b[order[i]];
        out[p++] = hx[v >> 4];
        out[p++] = hx[v & 0xF];
    }
    out[36] = 0;
}

const char* ldm_volume_kind_name(LdmVolumeKind k) {
    switch (k) {
    case LdmVolumeKind::Simple:
        return "simple";
    case LdmVolumeKind::Spanned:
        return "spanned";
    case LdmVolumeKind::Striped:
        return "striped";
    case LdmVolumeKind::Mirrored:
        return "mirrored";
    case LdmVolumeKind::Raid5:
        return "raid5";
    default:
        return "broken";
    }
}

/* ================= Part 2 continues: VBLK parsing ================= */

namespace {

/* ---------------------------------------------------------------- */
/* VBLK records                                                       */
/*                                                                    */
/* Record model (uniform for standalone + composite): a byte range    */
/* beginning with the 8-byte "secondary header": status u16 @0x00,    */
/* flags u8 @0x02, type u8 @0x03, length BE32 @0x04, variable data   */
/* from @0x08. For a standalone record this is on-disk offset 0x10;   */
/* for a composite it is the concatenation of the fragments' payload  */
/* regions (on-disk [16 .. vblk_size) each, entry-indexed) - exactly  */
/* the kernel's assembly, expressed in libldm's coordinates.          */
/* ---------------------------------------------------------------- */

constexpr u32 VBLK_TYPE_COMPONENT_V3 = 0x32;
constexpr u32 VBLK_TYPE_PARTITION_V3 = 0x33;
constexpr u32 VBLK_TYPE_DISK_V3 = 0x34;
constexpr u32 VBLK_TYPE_DISK_V4 = 0x44;
constexpr u32 VBLK_TYPE_DISKGRP_V3 = 0x35;
constexpr u32 VBLK_TYPE_DISKGRP_V4 = 0x45;
constexpr u32 VBLK_TYPE_VOLUME_V5 = 0x51;

/* flags (flatcap, kernel) */
constexpr u32 VBLK_FLAG_COMP_STRIPE = 0x10; /* cmp3: chunk+columns present */
constexpr u32 VBLK_FLAG_PART_INDEX = 0x08;  /* prt3: column index present */
constexpr u32 VBLK_FLAG_DGR_IDS = 0x08;     /* dgr3/4: copy counts present */
constexpr u32 VBLK_FLAG_VOLU_ID1 = 0x08;
constexpr u32 VBLK_FLAG_VOLU_ID2 = 0x20;
constexpr u32 VBLK_FLAG_VOLU_SIZE = 0x80;
constexpr u32 VBLK_FLAG_VOLU_DRIVE = 0x02;

/* Fragment header of an on-disk VBLK slot (first 16 bytes): magic
 * "VBLK" @0x00, sequence u32 @0x04, group u32 @0x08 (composite key,
 * libldm "record_id"), entry u16 @0x0C, entries_total u16 @0x0E. The
 * 8-byte secondary header (status u16, flags u8, type u8, length BE32)
 * follows at 0x10 for standalone records and at assembled+0 for
 * composites (fragment 0's payload IS the assembled record's head). */
struct VblkFragHead {
    u32 seq = 0;
    u32 group = 0;
    u16 rec = 0;
    u16 nrec = 0;
};

bool parse_vblk_frag_head(const u8* p, size_t len, VblkFragHead& h) {
    if (len < 16)
        return false;
    if (std::memcmp(p, "VBLK", 4) != 0)
        return false;
    h.seq = rd32be(p + 0x04);
    h.group = rd32be(p + 0x08);
    h.rec = rd16be(p + 0x0C);
    h.nrec = rd16be(p + 0x0E);
    return true;
}

/* Per-type payload parsers. p advances past the 8-byte secondary header
 * (i.e. points at the first variable field); end bounds the record. */

bool parse_component(const u8*& p, const u8* end, u8 flags, LdmComponentObj& c) {
    if (!get_vnum(p, end, c.obj_id))
        return false;
    if (!get_vstr(p, end, c.name, 64))
        return false;
    if (!get_vstr(p, end, c.state, 16))
        return false;
    if (p >= end)
        return false;
    c.type = *p++;
    if (c.type != LDM_COMP_STRIPE && c.type != LDM_COMP_CONCAT && c.type != LDM_COMP_RAID)
        return false;
    if (!skip_bytes(p, end, 4)) /* component flags */
        return false;
    if (!get_vnum(p, end, c.n_parts))
        return false;
    if (!skip_bytes(p, end, 8)) /* commit TID */
        return false;
    if (!skip_bytes(p, end, 8)) /* zeros */
        return false;
    if (!get_vnum(p, end, c.parent_id))
        return false;
    if (!skip_bytes(p, end, 1)) /* pad byte (kernel 0x2D->0x2E base jump;
                                 * libldm "Zeroes" after parent id) */
        return false;
    if (flags & VBLK_FLAG_COMP_STRIPE) {
        if (!get_vnum(p, end, c.chunk_size))
            return false;
        if (!get_vnum(p, end, c.n_columns))
            return false;
    }
    return p <= end;
}

bool parse_partition(const u8*& p, const u8* end, u8 flags, LdmPartitionObj& part) {
    if (!get_vnum(p, end, part.obj_id))
        return false;
    if (!get_vstr(p, end, part.name, 64))
        return false;
    if (!skip_bytes(p, end, 4)) /* partition flags */
        return false;
    if (!skip_bytes(p, end, 8)) /* commit TID */
        return false;
    if (p + 16 > end)
        return false;
    part.start = rd64be(p);
    p += 8;
    part.vol_offset = rd64be(p);
    p += 8;
    if (!get_vnum(p, end, part.size))
        return false;
    if (!get_vnum(p, end, part.parent_id))
        return false;
    if (!get_vnum(p, end, part.disk_id))
        return false;
    if (flags & VBLK_FLAG_PART_INDEX) {
        /* Column Index - VarUInt32 (flatcap; libldm reads a vnum. The
         * kernel's raw-byte read at +1 is a dead-code quirk that happens
         * to alias the value for 1-byte vnums.) */
        u64 idx = 0;
        if (!get_vnum(p, end, idx))
            return false;
        part.index = u32(idx);
        part.has_index = true;
    }
    return p <= end;
}

bool parse_disk_v3(const u8*& p, const u8* end, LdmDiskObj& d) {
    if (!get_vnum(p, end, d.obj_id))
        return false;
    if (!get_vstr(p, end, d.name, 64))
        return false;
    std::string guid_str;
    if (!get_vstr(p, end, guid_str, 64))
        return false;
    if (!get_vstr(p, end, d.alt_name, 128))
        return false;
    if (!skip_bytes(p, end, 4)) /* disk flags */
        return false;
    if (!skip_bytes(p, end, 8)) /* commit TID */
        return false;
    d.guid_valid = ldm_parse_guid_text(guid_str.c_str(), d.guid);
    return d.guid_valid;
}

bool parse_disk_v4(const u8*& p, const u8* end, LdmDiskObj& d) {
    if (!get_vnum(p, end, d.obj_id))
        return false;
    if (!get_vstr(p, end, d.name, 64))
        return false;
    if (p + 32 > end)
        return false;
    std::memcpy(d.guid.b, p, 16); /* raw binary GUID + disk-set GUID */
    p += 32;
    d.guid_valid = !d.guid.is_zero();
    if (!get_vstr(p, end, d.alt_name, 128))
        return false;
    if (!skip_bytes(p, end, 4))
        return false;
    if (!skip_bytes(p, end, 8))
        return false;
    return true;
}

bool parse_disk_group_v3(const u8*& p, const u8* end, u8 flags, LdmDiskGroupObj& g) {
    if (!get_vnum(p, end, g.obj_id))
        return false;
    if (!get_vstr(p, end, g.name, 32))
        return false;
    if (!get_vstr(p, end, g.guid_str, 64))
        return false;
    if (!skip_bytes(p, end, 4)) /* zeros */
        return false;
    if (!skip_bytes(p, end, 8)) /* commit TID */
        return false;
    if (flags & VBLK_FLAG_DGR_IDS) {
        u64 n = 0;
        if (!get_vnum(p, end, n)) /* config copies */
            return false;
        if (!get_vnum(p, end, n)) /* log copies */
            return false;
    }
    (void)ldm_parse_guid_text(g.guid_str.c_str(), g.guid_bin);
    return true;
}

bool parse_disk_group_v4(const u8*& p, const u8* end, u8 flags, LdmDiskGroupObj& g) {
    if (!get_vnum(p, end, g.obj_id))
        return false;
    if (!get_vstr(p, end, g.name, 32))
        return false;
    if (p + 32 > end)
        return false;
    std::memcpy(g.guid_bin.b, p, 16); /* raw GUID + last-disk-set GUID */
    p += 32;
    if (!skip_bytes(p, end, 4))
        return false;
    if (!skip_bytes(p, end, 8))
        return false;
    if (flags & VBLK_FLAG_DGR_IDS) {
        u64 n = 0;
        if (!get_vnum(p, end, n))
            return false;
        if (!get_vnum(p, end, n))
            return false;
    }
    return true;
}

bool parse_volume_v5(const u8*& p, const u8* end, u8 flags, LdmVolumeObj& v) {
    if (!get_vnum(p, end, v.obj_id))
        return false;
    if (!get_vstr(p, end, v.name, 64))
        return false;
    if (!get_vstr(p, end, v.volume_type, 16))
        return false;
    {
        std::string ddl; /* '8000000000000000' or zero - flatcap "Disable
                          * Driver Letter Assignment" */
        if (!get_vstr(p, end, ddl, 32))
            return false;
    }
    if (end - p < 14)
        return false;
    v.state.assign(reinterpret_cast<const char*>(p), 14); /* NUL padded */
    p += 14;
    if (p >= end)
        return false;
    v.int_volume_type = *p++;   /* libldm _int_type: 3 = gen, 4 = RAID-5 */
    if (!skip_bytes(p, end, 1)) /* unknown */
        return false;
    if (!skip_bytes(p, end, 1)) /* volume number (fixed 1 byte, libldm) */
        return false;
    if (!skip_bytes(p, end, 3)) /* zeros */
        return false;
    if (p >= end)
        return false;
    v.flags = *p++;
    if (!get_vnum(p, end, v.n_comps))
        return false;
    if (!skip_bytes(p, end, 8)) /* commit TID */
        return false;
    if (!skip_bytes(p, end, 8)) /* unknown TID */
        return false;
    if (!get_vnum(p, end, v.size))
        return false;
    if (!skip_bytes(p, end, 4)) /* zeros */
        return false;
    if (p >= end)
        return false;
    v.part_type = *p++;
    if (p + 16 > end)
        return false;
    std::memcpy(v.guid.b, p, 16);
    p += 16;
    if (flags & VBLK_FLAG_VOLU_ID1) {
        if (!get_vstr(p, end, v.id1, 64))
            return false;
    }
    if (flags & VBLK_FLAG_VOLU_ID2) {
        if (!get_vstr(p, end, v.id2, 64))
            return false;
    }
    if (flags & VBLK_FLAG_VOLU_SIZE) {
        if (!get_vnum(p, end, v.size2))
            return false;
    }
    if (flags & VBLK_FLAG_VOLU_DRIVE) {
        if (!get_vstr(p, end, v.hint, 8))
            return false;
    }
    return p <= end;
}

} // namespace

namespace {

/* ---------------------------------------------------------------- */
/* Record dispatch (one assembled/standalone view)                    */
/* ---------------------------------------------------------------- */

/* view: [0x00 status u16][0x02 flags][0x03 type][0x04 length BE32]
 *       [0x08 variable data ...]. Returns false on damaged records. */
bool parse_vblk_view(const u8* view, size_t len, LdmInfo& out) {
    if (len < 8)
        return false;
    const u8 flags = view[0x02];
    const u8 type = view[0x03];
    const u32 declared = rd32be(view + 0x04);
    const u8* p = view + 8;
    const u8* end = view + len;
    bool ok = false;
    size_t consumed = 0;

    switch (type) {
    case VBLK_TYPE_COMPONENT_V3: {
        LdmComponentObj c;
        ok = parse_component(p, end, flags, c);
        consumed = size_t(p - (view + 8));
        /* kernel: equality (VBLK_SIZE_CMP3 = 22 fixed bytes) */
        ok = ok && consumed == declared;
        if (ok)
            out.components.push_back(std::move(c));
        break;
    }
    case VBLK_TYPE_PARTITION_V3: {
        LdmPartitionObj part;
        ok = parse_partition(p, end, flags, part);
        consumed = size_t(p - (view + 8));
        /* kernel: floor (VBLK_SIZE_PRT3 = 28) - optional trailing fields
         * (unknown TID / hidden count) may follow the column index */
        ok = ok && consumed <= declared;
        if (ok)
            out.partitions.push_back(std::move(part));
        break;
    }
    case VBLK_TYPE_DISK_V3: {
        LdmDiskObj d;
        ok = parse_disk_v3(p, end, d);
        consumed = size_t(p - (view + 8));
        ok = ok && consumed == declared; /* kernel: equality (12) */
        if (ok)
            out.disks.push_back(std::move(d));
        break;
    }
    case VBLK_TYPE_DISK_V4: {
        LdmDiskObj d;
        ok = parse_disk_v4(p, end, d);
        consumed = size_t(p - (view + 8));
        /* kernel demands equality against a formula that only holds for
         * empty device names (w_alt=0); real Vista+ disks carry names, so
         * accept the floor (documented divergence - AUDIT.md row 30) */
        ok = ok && consumed <= declared;
        if (ok)
            out.disks.push_back(std::move(d));
        break;
    }
    case VBLK_TYPE_DISKGRP_V3: {
        LdmDiskGroupObj g;
        ok = parse_disk_group_v3(p, end, flags, g);
        consumed = size_t(p - (view + 8));
        ok = ok && consumed == declared; /* kernel: equality (12) */
        if (ok)
            out.groups.push_back(std::move(g));
        break;
    }
    case VBLK_TYPE_DISKGRP_V4: {
        LdmDiskGroupObj g;
        ok = parse_disk_group_v4(p, end, flags, g);
        consumed = size_t(p - (view + 8));
        ok = ok && consumed == declared; /* kernel: equality (44) */
        if (ok)
            out.groups.push_back(std::move(g));
        break;
    }
    case VBLK_TYPE_VOLUME_V5: {
        LdmVolumeObj v;
        ok = parse_volume_v5(p, end, flags, v);
        consumed = size_t(p - (view + 8));
        ok = ok && consumed <= declared; /* kernel: floor (58) */
        if (ok)
            out.volumes.push_back(std::move(v));
        break;
    }
    default:
        return false; /* unknown type - counted by the caller */
    }
    return ok;
}

/* ---------------------------------------------------------------- */
/* Composite (multi-fragment) records                                 */
/* ---------------------------------------------------------------- */

struct FragGroup {
    u16 total = 0;
    u16 found = 0;
    u32 mask = 0;        /* which entry slots arrived */
    bool broken = false; /* duplicate fragment: kernel marks the group
                          * permanently incomplete (map &= 0x7F) */
    std::vector<u8> data;
};

/* Returns true when the group completed and parsed. */
bool frag_commit(FragGroup& g, LdmInfo& out) {
    if (g.broken || g.found != g.total ||
        g.mask != (g.total >= 32 ? 0xFFFFFFFFu : ((1u << g.total) - 1))) {
        out.stats.composites_incomplete++;
        return false;
    }
    out.stats.composites++;
    return parse_vblk_view(g.data.data(), g.data.size(), out);
}

/* ---------------------------------------------------------------- */
/* The VBLK array walk                                                */
/*                                                                    */
/* Geometry (kernel ldm_get_vblks + libldm _parse_vblks): the array   */
/* starts at VMDB + vblk_offset bytes and runs for at most            */
/* vblk_size * vblk_last bytes (kernel bound), capped by the config   */
/* region; records are vblk_size each. A slot whose magic is not      */
/* "VBLK" ends the walk (libldm). nrec==0 slots are unused.           */
/* ---------------------------------------------------------------- */

u32 walk_vblks(const u8* vmdb_base, const LdmVmdb& vm, u64 region_bytes, LdmInfo& out) {
    const u64 start = vm.vblk_offset;
    const u64 bound = u64(vm.vblk_size) * u64(vm.vblk_last);
    if (bound == 0)
        return OK; /* empty database (fresh disk): nothing to walk */
    if (start >= region_bytes)
        return ERR_TRUNCATED;
    const u64 avail = region_bytes - start;
    const u64 walk_bytes = bound < avail ? bound : avail;

    std::vector<std::pair<u32, FragGroup>> frags;
    const u32 vsize = vm.vblk_size;
    for (u64 off = 0; off + vsize <= walk_bytes; off += vsize) {
        const u8* slot = vmdb_base + start + off;
        VblkFragHead h;
        if (!parse_vblk_frag_head(slot, vsize, h)) {
            /* Non-VBLK slot: the committed array ends here (libldm stop
             * rule). Damaged tail - counted, not silent. */
            out.stats.vblks_bad++;
            break;
        }
        out.stats.vblks_total++;
        if (h.nrec == 0)
            continue; /* unused slot */
        if (h.nrec > 4 || h.rec >= h.nrec) {
            /* kernel ldm.c:1232: fragments come in groups of at most 4 */
            out.stats.vblks_bad++;
            continue;
        }
        if (h.nrec == 1) {
            if (!parse_vblk_view(slot + 16, vsize - 16, out))
                out.stats.vblks_bad++;
            else
                out.stats.vblks_parsed++;
            continue;
        }
        /* fragment of a composite */
        FragGroup* g = nullptr;
        for (auto& kv : frags) {
            if (kv.first == h.group) {
                g = &kv.second;
                break;
            }
        }
        if (g == nullptr) {
            FragGroup ng;
            ng.total = h.nrec;
            ng.data.resize(size_t(h.nrec) * (vsize - 16), 0);
            frags.emplace_back(h.group, std::move(ng));
            g = &frags.back().second;
        } else if (g->total != h.nrec) {
            out.stats.vblks_bad++; /* inconsistent group declaration */
            continue;
        }
        if (g->mask & (1u << h.rec)) {
            /* duplicate fragment: kernel ldm.c:1264-1268 marks the group
             * permanently broken (map &= 0x7F) - it must never commit */
            g->broken = true;
            out.stats.vblks_bad++;
            continue;
        }
        g->mask |= 1u << h.rec;
        g->found++;
        std::memcpy(g->data.data() + size_t(h.rec) * (vsize - 16), slot + 16, vsize - 16);
    }
    for (auto& kv : frags) {
        if (frag_commit(kv.second, out))
            out.stats.vblks_parsed++;
        else
            out.stats.vblks_bad++;
    }
    return OK;
}

} // namespace

/* ================= Part 4 continues: graph + entry ================= */

/* ---------------------------------------------------------------- */
/* Object graph -> volume layouts                                    */
/* ---------------------------------------------------------------- */

namespace {

LdmMember build_member(const LdmPartitionObj& part, const LdmDiskObj* disk,
                       u64 logical_disk_start) {
    LdmMember m;
    m.part_obj_id = part.obj_id;
    m.disk_obj_id = part.disk_id;
    if (disk != nullptr) {
        m.disk_guid = disk->guid;
        m.disk_present = true;
    }
    m.part_start = part.start;
    /* lds of the PARSED disk - correct only for members on that disk;
     * LdmVolumeDevice re-anchors remote members through their own
     * PRIVHEAD when a resolver provides their device */
    m.disk_lba = logical_disk_start + part.start;
    m.sectors = part.size;
    m.vol_offset = part.vol_offset;
    m.index = part.has_index ? part.index : 0;
    return m;
}

u64 spanned_capacity(const std::vector<LdmMember>& members) {
    u64 cap = 0;
    for (const auto& m : members) {
        const u64 end = m.vol_offset + m.sectors;
        if (end > cap)
            cap = end;
    }
    return cap;
}

u64 stripe_capacity(const std::vector<LdmMember>& members, u64 chunk, u32 columns, bool raid5) {
    if (chunk == 0 || columns == 0 || members.size() != columns)
        return 0;
    u64 min_member = ~0ull;
    for (const auto& m : members)
        min_member = min_member < m.sectors ? min_member : m.sectors;
    const u64 rows = min_member / chunk;
    return rows * chunk * (raid5 ? u64(columns - 1) : u64(columns));
}

bool resolve_one_volume(const LdmInfo& db, const LdmVolumeObj& v, LdmVolumeLayout& lay) {
    lay.vol_obj_id = v.obj_id;
    lay.name = v.name;

    /* components owned by this volume, deterministic order */
    std::vector<const LdmComponentObj*> comps;
    for (const auto& c : db.components)
        if (c.parent_id == v.obj_id)
            comps.push_back(&c);
    std::sort(comps.begin(), comps.end(), [](const LdmComponentObj* a, const LdmComponentObj* b) {
        return a->obj_id < b->obj_id;
    });
    if (comps.empty())
        return false;

    const auto parts_of = [&](u64 comp_id) {
        std::vector<const LdmPartitionObj*> ps;
        for (const auto& p : db.partitions)
            if (p.parent_id == comp_id)
                ps.push_back(&p);
        return ps;
    };

    if (comps.size() > 1) {
        /* mirror: every copy is a concat component (libldm rule) */
        if (v.int_volume_type != LDM_VOLTYPE_GEN) /* spanned components need a gen volume */
            return false;
        for (const auto* c : comps) {
            if (c->type != LDM_COMP_CONCAT)
                return false;
            auto ps = parts_of(c->obj_id);
            if (ps.empty())
                return false;
            std::vector<LdmMember> members;
            for (const auto* p : ps) {
                const LdmDiskObj* disk = nullptr;
                for (const auto& d : db.disks)
                    if (d.obj_id == p->disk_id) {
                        disk = &d;
                        break;
                    }
                members.push_back(build_member(*p, disk, db.ph.logical_disk_start));
                if (disk == nullptr)
                    lay.missing_members++;
            }
            std::sort(members.begin(), members.end(), [](const LdmMember& a, const LdmMember& b) {
                return a.vol_offset < b.vol_offset;
            });
            lay.copies.push_back(std::move(members));
        }
        lay.kind = LdmVolumeKind::Mirrored;
        u64 cap = ~0ull;
        for (const auto& copy : lay.copies)
            cap = std::min(cap, spanned_capacity(copy));
        lay.total_sectors = v.size != 0 ? std::min(v.size, cap) : cap;
        lay.complete = lay.missing_members == 0 && lay.total_sectors > 0;
        return lay.total_sectors > 0;
    }

    const LdmComponentObj& c = *comps[0];
    auto ps = parts_of(c.obj_id);
    if (ps.empty())
        return false;

    std::vector<LdmMember> members;
    for (const auto* p : ps) {
        const LdmDiskObj* disk = nullptr;
        for (const auto& d : db.disks)
            if (d.obj_id == p->disk_id) {
                disk = &d;
                break;
            }
        members.push_back(build_member(*p, disk, db.ph.logical_disk_start));
        if (disk == nullptr)
            lay.missing_members++;
    }

    switch (c.type) {
    case LDM_COMP_CONCAT: {
        lay.kind = ps.size() == 1 ? LdmVolumeKind::Simple : LdmVolumeKind::Spanned;
        std::sort(members.begin(), members.end(), [](const LdmMember& a, const LdmMember& b) {
            return a.vol_offset < b.vol_offset;
        });
        lay.copies.push_back(std::move(members));
        const u64 cap = spanned_capacity(lay.copies[0]);
        lay.total_sectors = v.size != 0 ? std::min(v.size, cap) : cap;
        break;
    }
    case LDM_COMP_STRIPE: {
        if (v.int_volume_type != LDM_VOLTYPE_GEN) /* libldm: striped requires a "gen" volume */
            return false;
        lay.kind = LdmVolumeKind::Striped;
        lay.chunk_sectors = c.chunk_size;
        lay.columns = c.n_columns != 0 ? u32(c.n_columns) : u32(ps.size());
        std::stable_sort(members.begin(), members.end(),
                         [](const LdmMember& a, const LdmMember& b) { return a.index < b.index; });
        lay.copies.push_back(std::move(members));
        lay.total_sectors = stripe_capacity(lay.copies[0], lay.chunk_sectors, lay.columns, false);
        if (v.size != 0 && lay.total_sectors != 0)
            lay.total_sectors = std::min(v.size, lay.total_sectors);
        break;
    }
    case LDM_COMP_RAID: {
        if (v.int_volume_type != LDM_VOLTYPE_RAID5) /* libldm: RAID component requires raid5 */
            return false;
        lay.kind = LdmVolumeKind::Raid5;
        lay.chunk_sectors = c.chunk_size;
        lay.columns = c.n_columns != 0 ? u32(c.n_columns) : u32(ps.size());
        if (lay.columns < 3)
            return false; /* RAID-5 needs >= 3 columns */
        std::stable_sort(members.begin(), members.end(),
                         [](const LdmMember& a, const LdmMember& b) { return a.index < b.index; });
        lay.copies.push_back(std::move(members));
        lay.total_sectors = stripe_capacity(lay.copies[0], lay.chunk_sectors, lay.columns, true);
        if (v.size != 0 && lay.total_sectors != 0)
            lay.total_sectors = std::min(v.size, lay.total_sectors);
        break;
    }
    default:
        return false;
    }
    lay.complete = lay.missing_members == 0 && lay.total_sectors > 0;
    return lay.total_sectors > 0;
}

void resolve_layouts(LdmInfo& db) {
    db.layouts.reserve(db.volumes.size());
    for (const auto& v : db.volumes) {
        LdmVolumeLayout lay;
        if (!resolve_one_volume(db, v, lay)) {
            /* keep the volume visible for reporting with whatever could
             * be resolved (AUDIT: no silent drops) */
            lay.vol_obj_id = v.obj_id;
            lay.name = v.name;
            lay.kind = LdmVolumeKind::Broken;
            lay.complete = false;
        }
        db.layouts.push_back(std::move(lay));
    }
}

} // namespace

/* ================= Part 5 continues: entry point =================== */

/* ---------------------------------------------------------------- */
/* Database discovery + entry point                                   */
/* ---------------------------------------------------------------- */

namespace {

/* Locate the LDM metadata anchor. MBR dynamic disk: PRIVHEAD at disk
 * block 6 (kernel OFF_PRIV1). GPT dynamic disk: PRIVHEAD in the LAST
 * block of the LDM metadata partition (libldm _read_privhead_gpt).
 * read_at is byte-granular, so the 512-byte MBR probe works for every
 * logical sector size. */
u32 find_privhead(IStorageDevice* dev, u64& ph_byte_off, bool& is_gpt) {
    u8 sec[512];
    if (dev->read_at(0, sec, 512) != OK)
        return ERR_READ_FAILED;
    is_gpt = false;

    if (sec[510] != 0x55 || sec[511] != 0xAA)
        return ERR_BAD_MAGIC;

    /* MBR: any type-0x42 (dynamic) entry? */
    for (u32 i = 0; i < 4; i++) {
        const u8* e = sec + 0x1BE + 16 * i;
        if (e[4] == MBR_TYPE_LDM) {
            ph_byte_off = u64(6) * 512;
            return OK;
        }
    }
    /* GPT protective MBR (0xEE) -> parse the GPT */
    bool protective = false;
    for (u32 i = 0; i < 4; i++) {
        const u8* e = sec + 0x1BE + 16 * i;
        if (e[4] == 0xEE)
            protective = true;
    }
    if (!protective)
        return ERR_BAD_MAGIC;

    /* M-046 (diff-to-fix): the GPT path's LBA->byte conversions use the
     * DEVICE geometry, exactly like the main parser (cr_table_parse.cpp
     * read_sectors: off = lba * g.logical_sector). The hardcoded 512 put
     * the header read mid-sector-0 on 4Kn disks and made LDM-over-GPT
     * undiscoverable. (The MBR path's 6*512 above is the kernel's fixed
     * OFF_PRIV1 BYTE offset - correct for every sector size.) */
    const u64 ls = dev->geometry().logical_sector != 0 ? u64(dev->geometry().logical_sector)
                                                       : u64(512);
    GptHeader gh{};
    if (dev->read_at(u64(1) * ls, &gh, sizeof(gh)) != OK)
        return ERR_READ_FAILED;
    if (std::memcmp(gh.signature, GPT_SIGNATURE, 8) != 0)
        return ERR_BAD_MAGIC;
    if (gh.entry_size != 128 || gh.entry_count == 0 || gh.entry_count > 1024)
        return ERR_UNSUPPORTED;
    std::vector<u8> entries(size_t(gh.entry_count) * 128);
    if (dev->read_at(gh.entries_start_lba * ls, entries.data(), u32(entries.size())) != OK)
        return ERR_READ_FAILED;
    for (u32 i = 0; i < gh.entry_count; i++) {
        const GptEntry* e = reinterpret_cast<const GptEntry*>(entries.data() + size_t(i) * 128);
        if (!gpt_entry_used(*e))
            continue;
        if (gpt_guid_equal(e->type_guid, GPT_TYPE_LDM_METADATA)) {
            ph_byte_off = e->last_lba * ls; /* last block of the partition */
            is_gpt = true;
            return OK;
        }
    }
    return ERR_BAD_MAGIC;
}

} // namespace

u32 parse_ldm_database(IStorageDevice* device, LdmInfo& out) {
    if (device == nullptr)
        return ERR_BAD_PARAMETER;
    const Geometry& g = device->geometry();
    if (!g.valid() || g.logical_sector == 0)
        return ERR_BAD_PARAMETER;

    u64 ph_off = 0;
    bool is_gpt = false;
    u32 rc = find_privhead(device, ph_off, is_gpt);
    if (rc != OK)
        return rc;
    out.is_gpt = is_gpt;
    out.metadata_base = 0; /* LDM LBAs are absolute disk blocks */

    /* primary PRIVHEAD + the two backups (kernel: config+1856, +2047).
     * Recovery stance: a damaged primary is replaced by the first valid
     * backup (the kernel aborts; we must not). */
    std::vector<u8> phbuf(512);
    if (!read_exact(device, ph_off, phbuf.data(), 512))
        return ERR_READ_FAILED;
    LdmPrivhead ph;
    bool have_primary = parse_privhead(phbuf.data(), ph);
    if (!have_primary) {
        /* try the classic end-of-disk positions before giving up: the
         * database is the last 1 MiB on MBR dynamic disks. M-046: LDM's
         * own coordinates are 512-byte LBAs (kernel sector semantics) -
         * the end-of-disk bound is the DEVICE size in 512-LBA units, not
         * g.sector_count (which counts 4096-byte sectors on 4Kn). */
        const u64 last512 = device->size() / 512;
        const u64 cands[3] = {last512 - 2048 + 1856, last512 - 2048 + 2047, last512 - 1};
        for (u64 c : cands) {
            if (c == 0 || c >= last512)
                continue;
            if (!read_exact(device, c * 512, phbuf.data(), 512))
                continue;
            if (parse_privhead(phbuf.data(), ph)) {
                have_primary = true;
                break;
            }
        }
        if (!have_primary)
            return ERR_BAD_MAGIC;
    }
    out.stats.privheads_matched = 1;

    /* cross-check the configured backups when the primary came from LBA 6.
     * M-046: the bound is byte-based (LDM 512-LBA units, not device
     * sectors) - on 4Kn disks g.sector_count is 8x smaller than the
     * 512-LBA space and the old gate skipped valid backups. */
    {
        const u64 dev_bytes = device->size();
        const u64 backs[2] = {ph.config_start + 1856, ph.config_start + 2047};
        for (u64 b : backs) {
            if (b == 0 || (b + 1) * 512 > dev_bytes)
                continue;
            if (!read_exact(device, b * 512, phbuf.data(), 512))
                continue;
            LdmPrivhead tmp;
            if (parse_privhead(phbuf.data(), tmp)) {
                if (privheads_equal(ph, tmp))
                    out.stats.privheads_matched++;
            }
        }
    }

    /* geometry gates (kernel ldm_validate_privheads). M-046: LDM
     * coordinates are 512-byte LBAs (kernel sector semantics); the
     * capacity bound is the device size expressed in the SAME units -
     * g.sector_count counts device sectors (4096-byte on 4Kn) and made
     * every 4Kn dynamic disk spuriously TRUNCATED/clamped. */
    const u64 dev_512 = device->size() / 512;
    if (ph.logical_disk_start + ph.logical_disk_size > ph.config_start)
        return ERR_BAD_CHECKSUM; /* "disk and database overlap" */
    if (ph.config_start + ph.config_size > dev_512) {
        /* recovery divergence: clamp instead of aborting - the walk is
         * bounded by what actually exists on the device */
        if (ph.config_start >= dev_512)
            return ERR_TRUNCATED;
    }

    /* read the config region (clamped) */
    u64 cfg_secs = ph.config_size;
    if (ph.config_start + cfg_secs > dev_512)
        cfg_secs = dev_512 - ph.config_start;
    if (cfg_secs == 0)
        return ERR_TRUNCATED;
    if (cfg_secs > 8192)
        cfg_secs = 8192; /* 4 MiB hard cap (spec: 1 MiB) */
    std::vector<u8> cfg(size_t(cfg_secs * 512));
    if (!read_exact(device, ph.config_start * 512, cfg.data(), cfg.size()))
        return ERR_READ_FAILED;

    /* TOCBLOCK: kernel copies at config+1/2/2045/2046 */
    LdmTocblock toc;
    bool have_toc = false;
    const u64 toc_offs[4] = {1, 2, 2045, 2046};
    for (u64 o : toc_offs) {
        if (o >= cfg_secs)
            continue;
        const u8* p = cfg.data() + o * 512;
        LdmTocblock t;
        if (parse_tocblock(p, t)) {
            if (!have_toc) {
                toc = t;
                have_toc = true;
            } else if (!tocblocks_equal(toc, t)) {
                out.stats.tocblocks_matched++; /* counted; first copy wins */
            } else {
                out.stats.tocblocks_matched++;
            }
        }
    }
    if (!have_toc)
        return ERR_BAD_CHECKSUM;
    if (toc.bitmap1_start + toc.bitmap1_size > ph.config_size ||
        toc.bitmap2_start + toc.bitmap2_size > ph.config_size)
        return ERR_BAD_CHECKSUM; /* kernel: bitmaps out of range */

    /* VMDB: prefer the TOCBLOCK-declared position (libldm), fall back to
     * the kernel's fixed config+17 */
    u64 vmdb_sec = toc.bitmap1_start;
    if (vmdb_sec == 0 || vmdb_sec >= cfg_secs ||
        std::memcmp(cfg.data() + vmdb_sec * 512, "VMDB", 4) != 0)
        vmdb_sec = 17;
    if (vmdb_sec >= cfg_secs || std::memcmp(cfg.data() + vmdb_sec * 512, "VMDB", 4) != 0)
        return ERR_BAD_MAGIC;
    LdmVmdb vm;
    if (!parse_vmdb(cfg.data() + vmdb_sec * 512, vm))
        return ERR_UNSUPPORTED; /* version gate failed */
    /* kernel: uncommitted transactions abort; recovery stance: expose the
     * status and parse the committed array anyway (divergence documented
     * in the header) */
    if (vm.vblk_size < 24 || vm.vblk_size > 8192)
        return ERR_UNSUPPORTED;
    if (vm.update_status != 1 || vm.vblk_size_defaulted)
        out.stats.degraded = true;
    /* kernel ldm.c:461-465: vblk_size * vblk_last must fit the config
     * bitmap - on violation the kernel aborts; recovery: clamp to the
     * region (walk_vblks already bounds by region_bytes) and record
     * degraded, never silently mis-walk */
    if (vm.vblk_size != 0 && toc.bitmap1_size != 0 &&
        u64(vm.vblk_size) * u64(vm.vblk_last) > (toc.bitmap1_size << 9))
        out.stats.degraded = true;

    out.found = true;
    out.ph = ph;
    out.toc = toc;
    out.vm = vm;

    /* walk the VBLK array */
    const u8* vmdb_base = cfg.data() + vmdb_sec * 512;
    const u64 region_bytes = cfg.size() - vmdb_sec * 512;
    rc = walk_vblks(vmdb_base, vm, region_bytes, out);
    if (rc != OK)
        return rc;

    resolve_layouts(out);
    return OK;
}

/* ================= Part 6 continues: volume device ================ */

/* ---------------------------------------------------------------- */
/* LdmVolumeDevice - read-only virtual dynamic volume                 */
/* ---------------------------------------------------------------- */

namespace {

/* Find the member containing volume LBA s (spanned semantics; members
 * sorted by vol_offset). Returns nullptr for holes. */
const LdmMember* spanned_find(const std::vector<LdmMember>& ms, u64 s) {
    size_t lo = 0, hi = ms.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        if (ms[mid].vol_offset <= s)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return nullptr;
    const LdmMember& m = ms[lo - 1];
    if (s < m.vol_offset + m.sectors)
        return &m;
    return nullptr; /* hole (or past the end) */
}

/* Translate one contiguous run [s, run_end) within a linear member set.
 * run_end is clamped to `limit`. */
u32 translate_spanned(const std::vector<LdmMember>& ms, u64 s, u64 limit, u64& run_end,
                      u64& member_lba, const LdmMember*& m) {
    m = spanned_find(ms, s);
    if (m == nullptr)
        return ERR_READ_FAILED; /* hole in the span */
    run_end = m->vol_offset + m->sectors;
    if (run_end > limit)
        run_end = limit;
    member_lba = m->disk_lba + (s - m->vol_offset);
    return OK;
}

void xor_into(u8* dst, const u8* src, u64 n) {
    for (u64 i = 0; i < n; i++)
        dst[i] ^= src[i];
}

} // namespace

/* Probe a member disk's own logical_disk_start from its PRIVHEAD copies
 * (primary at LBA 6; then the end-of-disk convention positions used by
 * find_privhead's damaged-primary fallback). Returns false when no PRIVHEAD
 * is reachable - the member then keeps the parsed disk's lds and is marked
 * un-anchored so callers can refuse wrong-LBA reads instead of guessing. */
namespace {
bool probe_disk_lds(IStorageDevice* dev, u64& lds_out) {
    if (dev == nullptr)
        return false;
    u8 buf[512];
    const u64 total = dev->size();
    const u64 last_sec = total / 512;
    u64 candidates[3] = {6, 0, 0};
    if (last_sec > 2048) {
        candidates[1] = last_sec - 2048 + 1856;
        candidates[2] = last_sec - 2048 + 2047;
    }
    for (u64 sec : candidates) {
        if (sec == 0)
            continue;
        if (dev->read_at(sec * 512, buf, sizeof(buf)) != OK)
            continue;
        if (std::memcmp(buf, "PRIVHEAD", 8) != 0)
            continue;
        const u64 lds = rd64be(buf + 0x11B);
        if (lds != 0) {
            lds_out = lds;
            return true;
        }
    }
    return false;
}
} // namespace

LdmVolumeDevice::LdmVolumeDevice(const LdmVolumeLayout& layout, LdmDiskResolver resolver)
    : layout_(layout), resolver_(std::move(resolver)) {
    geom_.logical_sector = 512; /* LDM LBAs are 512-byte logical blocks */
    geom_.physical_sector = 512;
    geom_.sector_count = layout.total_sectors;
    geom_.total_bytes = layout.total_sectors * 512;
    /* Re-anchor every member that the resolver can provide a device for:
     * each member disk has its OWN logical_disk_start (63 vs 2048 vs GPT
     * layouts differ across disks) - using the parsed disk's lds for a
     * remote member would read the wrong LBAs. The member's PRIVHEAD is
     * the authoritative source. */
    if (resolver_) {
        for (auto& copy : layout_.copies) {
            for (auto& m : copy) {
                IStorageDevice* d = resolver_(m.disk_guid);
                if (d == nullptr)
                    continue;
                if (d->geometry().physical_sector != 0 && geom_.physical_sector == 512)
                    geom_.physical_sector = d->geometry().physical_sector;
                u64 lds = 0;
                if (probe_disk_lds(d, lds))
                    m.disk_lba = lds + m.part_start; /* re-anchored */
            }
        }
    }
}

/* Spanned/simple/mirror-copy read of [sec, sec+nsec) into dst. */
u32 LdmVolumeDevice::read_spanned_copy(const std::vector<LdmMember>& members, u64 sec, u32 nsec,
                                       u8* dst) {
    u64 s = sec;
    const u64 limit = sec + nsec;
    while (s < limit) {
        u64 run_end = 0, member_lba = 0;
        const LdmMember* m = nullptr;
        if (translate_spanned(members, s, limit, run_end, member_lba, m) != OK)
            return ERR_READ_FAILED;
        IStorageDevice* dev = resolver_ ? resolver_(m->disk_guid) : nullptr;
        if (dev == nullptr)
            return ERR_READ_FAILED;
        const u32 bytes = u32((run_end - s) * 512);
        if (dev->read_at(member_lba * 512, dst + (s - sec) * 512, bytes) != OK)
            return ERR_READ_FAILED;
        s = run_end;
    }
    return OK;
}

u32 LdmVolumeDevice::read_at(u64 offset, void* buffer, u32 length) {
    if (offset > geom_.total_bytes || u64(length) > geom_.total_bytes - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;
    if (length == 0)
        return OK;
    u8* dst = static_cast<u8*>(buffer);
    const u64 sec0 = offset / 512;
    const u64 secA = (offset + length) / 512; /* aligned body end (floor) */
    const bool head_partial = (offset % 512) != 0;
    const bool tail_partial = (offset + length) > secA * 512;

    if (layout_.kind == LdmVolumeKind::Broken || layout_.copies.empty())
        return ERR_UNSUPPORTED;

    /* One translated, possibly-reconstructed read of the sector range
     * [s, s + nsec) into out. */
    const auto read_run = [&](u64 s, u32 nsec, u8* out) -> u32 {
        if (layout_.kind == LdmVolumeKind::Simple || layout_.kind == LdmVolumeKind::Spanned)
            return read_spanned_copy(layout_.copies[0], s, nsec, out);

        if (layout_.kind == LdmVolumeKind::Mirrored) {
            u32 last = ERR_READ_FAILED;
            for (const auto& copy : layout_.copies) {
                last = read_spanned_copy(copy, s, nsec, out);
                if (last == OK)
                    return OK;
            }
            return last;
        }

        /* striped / raid5: one run inside one chunk of one column */
        const std::vector<LdmMember>& members = layout_.copies[0];
        const u64 chunk = layout_.chunk_sectors;
        const u32 cols = layout_.columns;
        if (chunk == 0 || cols == 0 || members.size() != cols)
            return ERR_UNSUPPORTED;
        const u64 ci = s / chunk;
        const u64 in = s % chunk;
        u64 row, col;
        if (layout_.kind == LdmVolumeKind::Striped) {
            col = ci % cols;
            row = ci / cols;
        } else {              /* raid5 left-symmetric (libldm "raid5_ls"): parity
                               * P(row) = N-1-(row mod N); data chunk i of the row
                               * lands on column (P+1+i) mod N */
            const u64 k = ci; /* global data-chunk index */
            row = k / (cols - 1);
            const u64 i = k % (cols - 1);
            const u64 parity = (cols - 1) - (row % cols);
            col = (parity + 1 + i) % cols;
        }
        const LdmMember& m = members[size_t(col)];
        IStorageDevice* dev = resolver_ ? resolver_(m.disk_guid) : nullptr;
        const u32 bytes = nsec * 512;
        if (dev != nullptr) {
            if (dev->read_at((m.disk_lba + row * chunk + in) * 512, out, bytes) == OK)
                return OK;
            /* unreadable member: RAID-5 reconstructs, striped fails */
        }
        if (layout_.kind != LdmVolumeKind::Raid5)
            return ERR_READ_FAILED;

        /* RAID-5 reconstruction: XOR every other column of this row
         * (parity included). A second missing/unreadable column makes
         * the range unrecoverable - surfaced, never zeroed. */
        std::vector<u8> acc(bytes, 0);
        std::vector<u8> tmp(bytes);
        for (u32 c2 = 0; c2 < cols; c2++) {
            if (c2 == col)
                continue;
            const LdmMember& m2 = members[c2];
            IStorageDevice* dev2 = resolver_ ? resolver_(m2.disk_guid) : nullptr;
            if (dev2 == nullptr)
                return ERR_READ_FAILED;
            if (dev2->read_at((m2.disk_lba + row * chunk + in) * 512, tmp.data(), bytes) != OK)
                return ERR_READ_FAILED;
            xor_into(acc.data(), tmp.data(), bytes);
        }
        std::memcpy(out, acc.data(), bytes);
        return OK;
    };

    /* head: one staged (possibly partial) sector */
    if (head_partial) {
        u8 stage[512];
        if (u32 rc = read_run(sec0, 1, stage))
            return rc;
        const u64 skip = offset - sec0 * 512;
        const u64 avail = 512 - skip;
        const u64 n = avail < u64(length) ? avail : u64(length);
        std::memcpy(dst, stage + skip, size_t(n));
        if (offset + length <= (sec0 + 1) * 512)
            return OK; /* request ended inside the head sector */
    }

    /* aligned body: [first, secA) */
    const u64 first = head_partial ? sec0 + 1 : sec0;
    for (u64 s = first; s < secA;) {
        /* compute the maximal contiguous run for this kind */
        u64 run_end = secA;
        if (layout_.kind == LdmVolumeKind::Simple || layout_.kind == LdmVolumeKind::Spanned ||
            layout_.kind == LdmVolumeKind::Mirrored) {
            u64 re = 0, ml = 0;
            const LdmMember* m = nullptr;
            const std::vector<LdmMember>* ms = &layout_.copies[0];
            if (translate_spanned(*ms, s, secA, re, ml, m) != OK) {
                /* holes in mirrors: try per-copy translation to advance */
                if (layout_.kind == LdmVolumeKind::Mirrored) {
                    bool any = false;
                    for (const auto& copy : layout_.copies) {
                        if (translate_spanned(copy, s, secA, re, ml, m) == OK) {
                            any = true;
                            break;
                        }
                    }
                    if (!any)
                        return ERR_READ_FAILED;
                } else {
                    return ERR_READ_FAILED;
                }
            }
            if (layout_.kind == LdmVolumeKind::Mirrored) {
                for (const auto& copy : layout_.copies) {
                    u64 re2 = 0, ml2 = 0;
                    const LdmMember* m2 = nullptr;
                    if (translate_spanned(copy, s, secA, re2, ml2, m2) == OK && re2 < run_end)
                        run_end = re2;
                }
            } else {
                run_end = re;
            }
        } else {
            /* striped / raid5: runs end at chunk boundaries */
            const u64 chunk = layout_.chunk_sectors;
            const u64 in = s % chunk;
            const u64 chunk_end = s - in + chunk;
            if (chunk_end < run_end)
                run_end = chunk_end;
        }
        if (run_end <= s)
            return ERR_INTERNAL;
        const u32 nsec = u32(run_end - s);
        if (u32 rc = read_run(s, nsec, dst + (s * 512 - offset)))
            return rc;
        s = run_end;
    }

    /* tail: one staged (possibly partial) sector */
    if (tail_partial) {
        u8 stage[512];
        if (u32 rc = read_run(secA, 1, stage))
            return rc;
        const u64 have = offset + length - secA * 512; /* 1..511 */
        std::memcpy(dst + (secA * 512 - offset), stage, size_t(have));
    }
    return OK;
}

} // namespace table
} // namespace cr
