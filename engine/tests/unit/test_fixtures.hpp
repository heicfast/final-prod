/* test_fixtures.hpp - shared test fixtures (LDM dynamic-disk builder
 * + minimal FAT16 volume), used by test_ldm.cpp and test_orchestrator.cpp.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TEST_FIXTURES_HPP_
#define CR_TEST_FIXTURES_HPP_

#include "cr/table/cr_ldm.h"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>

namespace cr {
namespace testfixture {

using namespace cr::table;

/* ---------------------------------------------------------------- */
/* BE writers                                                        */
/* ---------------------------------------------------------------- */

inline void p16be(u8* p, u16 v) {
    p[0] = u8(v >> 8);
    p[1] = u8(v);
}
inline void p32be(u8* p, u32 v) {
    p[0] = u8(v >> 24);
    p[1] = u8(v >> 16);
    p[2] = u8(v >> 8);
    p[3] = u8(v);
}
inline void p64be(u8* p, u64 v) {
    p32be(p, u32(v >> 32));
    p32be(p + 4, u32(v));
}

/* variable-width int: 1 length byte + n BE payload bytes; returns
 * bytes written */
inline u32 pvnum(u8* p, u64 v) {
    if (v == 0) {
        p[0] = 0;
        return 1;
    }
    u32 w = 0;
    u64 t = v;
    while (t) {
        w++;
        t >>= 8;
    }
    p[0] = u8(w);
    for (u32 i = 0; i < w; i++)
        p[1 + i] = u8(v >> (8 * (w - 1 - i)));
    return w + 1;
}
inline u32 pvstr(u8* p, const char* s) {
    const u32 n = u32(strlen(s));
    p[0] = u8(n);
    memcpy(p + 1, s, n);
    return n + 1;
}

/* ---------------------------------------------------------------- */
/* Fixture builder                                                   */
/* ---------------------------------------------------------------- */

constexpr u32 VBLK_SIZE = 128;
constexpr u64 CHUNK = 128; /* 64 KiB stripe units, in sectors */

struct PartSpec {
    u64 obj_id;
    const char* name;
    u64 start;      /* sectors, disk-relative */
    u64 size;       /* sectors */
    u64 vol_offset; /* position in volume/column */
    u64 parent_id;  /* component */
    u32 index;      /* column index */
    u64 disk_id;    /* owning disk object id */
    bool with_index = true;
};

struct CompSpec {
    u64 obj_id;
    const char* name;
    u8 type; /* 1 stripe, 2 concat, 3 raid5 */
    u64 n_parts;
    u64 parent_id; /* volume */
    u64 chunk = 0;
    u64 n_columns = 0;
};

struct VolSpec {
    u64 obj_id;
    const char* name;
    u8 int_volume_type; /* libldm _VOLUME_TYPE_GEN=3, RAID5=4 */
    u64 size;           /* sectors */
    u64 n_comps;
    bool as_composite = false; /* split into 2 fragments */
};

class LdmFixture {
public:
    /* total_secs: whole-disk size in 512-byte sectors; the last 2048
     * sectors are the database region */
    explicit LdmFixture(u64 total_secs) : d_(size_t(total_secs) * 512, 0) {
        total_secs_ = total_secs;
        cfg_start_ = total_secs - 2048;
        /* MBR: single type-0x42 entry spanning the disk */
        u8* mbr = d_.data();
        mbr[0x1BE + 4] = 0x42;
        p32be(mbr + 0x1BE + 8, 0);
        p32be(mbr + 0x1BE + 12, u32(total_secs_ - 1));
        mbr[510] = 0x55;
        mbr[511] = 0xAA;
        /* TOCBLOCK copies at config+1 and config+2 */
        for (u64 o : {u64(1), u64(2)}) {
            u8* toc = d_.data() + (cfg_start_ + o) * 512;
            memcpy(toc, "TOCBLOCK", 8);
            memcpy(toc + 0x24, "config", 6);
            p64be(toc + 0x2E, 17); /* bitmap1 start -> VMDB @ config+17 */
            p64be(toc + 0x36, 1600);
            memcpy(toc + 0x46, "log", 3);
            p64be(toc + 0x50, 1617);
            p64be(toc + 0x58, 400);
        }
        /* VMDB at config+17 */
        u8* vm = d_.data() + (cfg_start_ + 17) * 512;
        memcpy(vm, "VMDB", 4);
        p32be(vm + 0x04, 0); /* vblk_last: set at finish() */
        p32be(vm + 0x08, VBLK_SIZE);
        p32be(vm + 0x0C, 512); /* vblk_offset */
        p16be(vm + 0x10, 1);   /* committed */
        p16be(vm + 0x12, 4);
        p16be(vm + 0x14, 10);
        memcpy(vm + 0x16, "TestPCDg0", 9);
        memcpy(vm + 0x35, "A1B2C3D4-E5F6-4789-9ABC-D0E1F2A3B4C5", 36);
        p64be(vm + 0x75, 42); /* committed sequence */
        vmdb_ = vm;
        /* PRIVHEAD: primary at LBA 6 + 2 backups */
        write_privhead(6);
        vblk_base_ = (cfg_start_ + 17) * 512 + 512;
        vblk_capacity_ = (2048 - 17) * 512 - 512;
    }

    void set_disk(u64 obj_id, const char* guid_text) {
        disk_id_ = obj_id;
        disk_guid_ = guid_text;
    }

    void write_privhead(u64 lba) {
        u8* p = d_.data() + lba * 512;
        memcpy(p, "PRIVHEAD", 8);
        p16be(p + 0x0C, 2);
        p16be(p + 0x0E, 11);
        memcpy(p + 0x30, disk_guid_, strlen(disk_guid_));
        memcpy(p + 0xB0, "11111111-2222-3333-4444-555555555555", 36);
        memcpy(p + 0xF0, "TestPCDg0", 9);
        p64be(p + 0x11B, 0);          /* logical disk start */
        p64be(p + 0x123, cfg_start_); /* logical disk size */
        p64be(p + 0x12B, cfg_start_); /* config start */
        p64be(p + 0x133, 2048);       /* config size */
    }

    /* ---- VBLK emitters (kernel/libldm-true geometry) ---- */

    /* Records are built in a staging buffer where offset 0 is the
     * secondary-header view (status@0, flags@2, type@3, length@4, data@8);
     * finish_vblk copies it into a 128-byte slot: header16 + payload. */
    u8* begin_vblk() {
        memset(stage_, 0, sizeof(stage_));
        return stage_ + 8; /* cursor for variable fields */
    }

    void finish_vblk(u8* cur, u8 type, u8 flags, u32 seq) {
        const u32 data_len = u32(cur - (stage_ + 8));
        ASSERT_LE(data_len + 8, VBLK_SIZE - 16); /* must fit one slot */
        p16be(stage_ + 0x00, 0);                 /* active */
        stage_[0x02] = flags;
        stage_[0x03] = type;
        p32be(stage_ + 0x04, data_len);
        emit_slot(stage_, data_len + 8, seq, 0, 1);
    }

    /* Composite: split the record view across n fragments. Fragment 0
     * carries the head (up to a full 112-byte payload); fragments 1+
     * continue with the remainder. The parser reassembles the view as the
     * concatenation of all fragments' payload regions. */
    void finish_vblk_composite(u8* cur, u8 type, u8 flags, u32 seq, u32 group, u32 nfrags) {
        const u32 view_len = u32(cur - stage_); /* incl. 8-byte header */
        p16be(stage_ + 0x00, 0);
        stage_[0x02] = flags;
        stage_[0x03] = type;
        p32be(stage_ + 0x04, view_len - 8);
        const u32 payload_max = VBLK_SIZE - 16;
        ASSERT_GT(view_len, payload_max); /* must actually span */
        const u32 first = payload_max;
        const u32 rest = view_len - first;
        const u32 each = (rest + nfrags - 2) / (nfrags - 1);
        ASSERT_LE(each, payload_max);
        /* fragment 0: payload = view[0 .. first) */
        emit_slot(stage_, first, seq, group, u16(nfrags), 0);
        /* fragments 1+: payload = view[first + roff .. ) */
        u32 roff = 0;
        for (u32 f = 1; f < nfrags; f++) {
            const u32 n = each < rest - roff ? each : rest - roff;
            emit_slot(stage_ + first + roff, n, seq + f, group, u16(nfrags), u16(f));
            roff += n;
        }
        ASSERT_EQ(rest, roff);
    }

    void emit_slot(const u8* view, u32 view_len, u32 seq, u32 group, u16 nrec, u16 rec = 0) {
        ASSERT_LT(vblk_used_ + VBLK_SIZE, vblk_capacity_);
        u8* slot = d_.data() + vblk_base_ + vblk_used_;
        memset(slot, 0, VBLK_SIZE);
        memcpy(slot, "VBLK", 4);
        p32be(slot + 0x04, seq);
        p32be(slot + 0x08, group);
        p16be(slot + 0x0C, rec);
        p16be(slot + 0x0E, nrec);
        memcpy(slot + 16, view, view_len);
        vblk_used_ += VBLK_SIZE;
        vblk_count_++;
    }

    void add_dgr4(u64 obj_id, const char* name) {
        u8* c = begin_vblk();
        c += pvnum(c, obj_id);
        c += pvstr(c, name);
        memcpy(c, "\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xF0\x01", 16);
        memcpy(c + 16, "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xF2", 16);
        c += 32;
        c += 4; /* zeros */
        c += 8; /* commit TID */
        finish_vblk(c, 0x45, 0x00, next_seq_++);
    }

    void add_dsk4(u64 obj_id, const char* name, const char* guid_text) {
        /* the DSK4 binary GUID must equal the owning disk's PRIVHEAD text
         * GUID (real-disk invariant; the orchestrator resolver relies on
         * member.guid == privhead.disk_guid_bin for the scanned disk) */
        LdmGuid g = {};
        if (!ldm_parse_guid_text(guid_text, g))
            return;
        u8* c = begin_vblk();
        c += pvnum(c, obj_id);
        c += pvstr(c, name);
        memcpy(c, g.b, 16);
        memcpy(c + 16, "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xF2", 16);
        c += 32;
        c += pvstr(c, ""); /* last device name (empty, kernel-exact) */
        c += 4;            /* flags */
        c += 8;            /* commit TID */
        finish_vblk(c, 0x44, 0x00, next_seq_++);
    }

    void add_volume(const VolSpec& v) {
        u8* c = begin_vblk();
        c += pvnum(c, v.obj_id);
        c += pvstr(c, v.name);
        c += pvstr(c, v.int_volume_type == 4 ? "raid5" : "gen");
        c += pvstr(c, "");  /* disable drive letter */
        memset(c, 'S', 14); /* volume state slot */
        c += 14;
        *c++ = v.int_volume_type;
        *c++ = 0;    /* unknown */
        *c++ = 1;    /* volume number */
        c += 3;      /* zeros */
        *c++ = 0x10; /* volume flags */
        c += pvnum(c, v.n_comps);
        c += 8; /* commit TID */
        c += 8; /* unknown TID */
        c += pvnum(c, v.size);
        c += 4;      /* zeros */
        *c++ = 0x07; /* partition type: NTFS */
        memcpy(c, "\xAA\xBB\xCC\xDD\xEE\xFF\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99", 16);
        c += 16;
        if (v.as_composite)
            finish_vblk_composite(c, 0x51, 0x00, next_seq_++, 0x9000 + u32(v.obj_id), 2);
        else
            finish_vblk(c, 0x51, 0x00, next_seq_++);
    }

    void add_component(const CompSpec& s) {
        u8* c = begin_vblk();
        c += pvnum(c, s.obj_id);
        c += pvstr(c, s.name);
        c += pvstr(c, "ACTIVE");
        *c++ = s.type;
        c += 4; /* component flags */
        c += pvnum(c, s.n_parts);
        c += 8; /* commit TID */
        c += 8; /* zeros */
        c += pvnum(c, s.parent_id);
        c += 1; /* pad byte after volume id (kernel 0x2D->0x2E jump) */
        const u8 flags = (s.type == LDM_COMP_STRIPE || s.type == LDM_COMP_RAID) ? 0x10 : 0x00;
        if (flags) {
            c += pvnum(c, s.chunk);
            c += pvnum(c, s.n_columns);
        }
        finish_vblk(c, 0x32, flags, next_seq_++);
    }

    void add_partition(const PartSpec& s) {
        u8* c = begin_vblk();
        c += pvnum(c, s.obj_id);
        c += pvstr(c, s.name);
        c += 4; /* partition flags */
        c += 8; /* commit TID */
        p64be(c, s.start);
        c += 8;
        p64be(c, s.vol_offset);
        c += 8;
        c += pvnum(c, s.size);
        c += pvnum(c, s.parent_id);
        c += pvnum(c, s.disk_id);
        u8 flags = 0;
        if (s.with_index) {
            c += pvnum(c, s.index);
            flags = 0x08;
        }
        finish_vblk(c, 0x33, flags, next_seq_++);
    }

    void finish() {
        /* vblk_last = number of committed records (kernel bound:
         * vblk_size * vblk_last bytes of array) */
        p32be(vmdb_ + 0x04, vblk_count_);
        /* PRIVHEAD backups at config+1856 and config+2047 */
        write_privhead(cfg_start_ + 1856);
        write_privhead(cfg_start_ + 2047);
    }

    std::vector<u8> take() { return std::move(d_); }

private:
    std::vector<u8> d_;
    u64 total_secs_ = 0;
    u64 cfg_start_ = 0;
    u8* vmdb_ = nullptr;
    u64 vblk_base_ = 0;
    u64 vblk_capacity_ = 0;
    u64 vblk_used_ = 0;
    u32 vblk_count_ = 0;
    u32 next_seq_ = 4; /* flatcap: sequence numbers start at 4 */
    u64 disk_id_ = 1000;
    const char* disk_guid_ = "01234567-89AB-CDEF-0123-456789ABCDEF";
    u8 stage_[VBLK_SIZE * 4] = {};
};

/* ---------------------------------------------------------------- */
/* Fixture: one disk, simple + striped volumes                        */
/* ---------------------------------------------------------------- */

constexpr u64 SIMPLE_START = 2048;                        /* sectors */
constexpr u64 SIMPLE_SIZE = 4096;                         /* 2 MiB */
constexpr u64 STRIPE_A_START = 8192;                      /* member column 0 */
constexpr u64 STRIPE_MEMBER_SIZE = 8192;                  /* 4 MiB per member */
constexpr u64 STRIPE_B_START = 8192 + STRIPE_MEMBER_SIZE; /* after A: no overlap */

struct StripeDisk {
    LdmFixture fx;
    StripeDisk() : fx(65536) { /* 32 MiB disk */
        fx.set_disk(1000, "01234567-89AB-CDEF-0123-456789ABCDEF");
        fx.add_dgr4(1, "TestPCDg0");
        fx.add_dsk4(1000, "Disk1", "01234567-89AB-CDEF-0123-456789ABCDEF");
        /* simple volume: emitted as a real 2-fragment composite (the
         * long name pushes the record past one 112-byte payload) */
        fx.add_volume({2000, "Volume1-CompositeSpanTest-0123456789", 3, SIMPLE_SIZE, 1, true});
        fx.add_component({3000, "Volume1-01", LDM_COMP_CONCAT, 1, 2000, 0, 0});
        fx.add_partition({4000, "Disk1-01", SIMPLE_START, SIMPLE_SIZE, 0, 3000, 0, 1000});
        /* striped volume: Stripe1 (2 columns, 64 KiB chunks) */
        fx.add_volume({2001, "Stripe1", 3, STRIPE_MEMBER_SIZE * 2 /* full capacity */, 1});
        fx.add_component({3001, "Stripe1-01", LDM_COMP_STRIPE, 2, 2001, CHUNK, 2});
        fx.add_partition({4001, "Disk1-02", STRIPE_A_START, STRIPE_MEMBER_SIZE, 0, 3001, 0, 1000});
        fx.add_partition({4002, "Disk1-03", STRIPE_B_START, STRIPE_MEMBER_SIZE, 0, 3001, 1, 1000});
        fx.finish();
    }
};

} // namespace testfixture
} // namespace cr

/* ---------------------------------------------------------------- */
/* S-051/M-104 (diff-to-fix): the whole-disk partition-list target   */
/* fixtures - a two-volume MBR disk, a GPT twin, and the labeled     */
/* FAT16 volume they plant (the executed evidence's own shape:        */
/* diff-to-fix/evidence/executions/s10-wholedisk-fixture-builder.py,  */
/* two type-0x0E FAT16 volumes with VOLONE/VOLTWO labels, one live +  */
/* one deleted file each).                                           */
/* ---------------------------------------------------------------- */
namespace cr {
namespace testfixture {

inline void put16le(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}
inline void put32le(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

/* A minimal labeled FAT16 volume: boot sector (label at +0x2B), one FAT,
 * one root-dir block, DOC.TXT (live) + DELFILE.TXT (deleted). Mirrors the
 * evidence builder's shape but stays self-contained for the unit suite.
 * Returns the volume image; vol_secs_out (optional) receives its sector
 * count. The label is space-padded to 11 like the on-disk field. */
inline std::vector<u8> build_labeled_fat16(const char* label, u64* vol_secs_out = nullptr) {
    constexpr u32 BPS = 512, SPC = 1, RESERVED = 1, SPF = 66, ROOT_ENTRIES = 512;
    const u32 root_secs = (ROOT_ENTRIES * 32 + BPS - 1) / BPS;
    const u32 total = RESERVED + SPF + root_secs + 16384 * SPC;
    std::vector<u8> vol(u64(total) * BPS, 0);
    u8* b = vol.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16le(b + 0x0B, BPS);
    b[0x0D] = SPC;
    put16le(b + 0x0E, RESERVED);
    b[0x10] = 1; /* one FAT */
    put16le(b + 0x11, ROOT_ENTRIES);
    put16le(b + 0x16, SPF);
    put16le(b + 0x18, 63);
    put16le(b + 0x1A, 255);
    put16le(b + 0x20, u16(total)); /* total16 (fits: ~16482) */
    b[0x15] = 0xF8;
    /* the 11-byte label, space padded */
    char lbl[11] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    for (int i = 0; i < 11 && label[i]; ++i)
        lbl[i] = label[i];
    std::memcpy(b + 0x2B, lbl, 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const u32 fat_off = RESERVED * BPS;
    const u32 root_off = (RESERVED + SPF) * BPS;
    const u32 data_off = root_off + root_secs * BPS;
    put16le(vol.data() + fat_off + 0, 0xFFF8);
    put16le(vol.data() + fat_off + 2, 0xFFFF);
    put16le(vol.data() + fat_off + 4, 0xFFFF); /* cluster 2: DOC.TXT */
    put16le(vol.data() + fat_off + 6, 0xFFFF); /* cluster 3: DELFILE.TXT */

    auto dentry = [&](size_t slot, const char* n8, const char* e3, u32 cl, u32 sz, bool del) {
        u8* e = vol.data() + root_off + slot * 32;
        std::memset(e, 0, 32);
        for (int i = 0; i < 8; ++i)
            e[i] = u8(n8[i]);
        for (int i = 0; i < 3; ++i)
            e[8 + i] = u8(e3[i]);
        if (del)
            e[0] = 0xE5;
        e[11] = 0x20;
        put16le(e + 0x1A, u16(cl));
        put16le(e + 0x1C, u16(sz));
    };
    dentry(0, "DOC     ", "TXT", 2, 11, false);
    dentry(1, "DELFILE ", "TXT", 3, 9, true);
    std::memcpy(vol.data() + data_off, "HELLO CORE", 10);
    std::memcpy(vol.data() + data_off + BPS, "DELETED!!", 9);
    if (vol_secs_out)
        *vol_secs_out = total;
    return vol;
}

/* The S-051 evidence shape: an MBR disk with TWO type-0x0E FAT16 volumes
 * (labels l1/l2), each with one live + one deleted file. Both partitions'
 * start LBAs land in start1_out/start2_out when provided. */
inline std::vector<u8> build_two_volume_mbr_disk(const char* l1, const char* l2,
                                                 u64* start1_out = nullptr,
                                                 u64* start2_out = nullptr) {
    u64 s1_secs = 0, s2_secs = 0;
    auto v1 = build_labeled_fat16(l1, &s1_secs);
    auto v2 = build_labeled_fat16(l2, &s2_secs);
    const u64 p1_start = 2048, p2_start = p1_start + s1_secs;
    const u64 disk_secs = p2_start + s2_secs + 2048;
    std::vector<u8> img(disk_secs * 512, 0);
    std::memcpy(img.data() + p1_start * 512, v1.data(), v1.size());
    std::memcpy(img.data() + p2_start * 512, v2.data(), v2.size());
    u8* mbr = img.data();
    auto pentry = [&](u64 start, u64 secs, int idx) {
        u8* e = mbr + 446 + idx * 16;
        e[0] = 0x00;
        e[1] = 0x20;
        e[2] = 0x21;
        e[3] = 0x00;
        e[4] = 0x0E; /* FAT16 LBA */
        e[5] = 0x20;
        e[6] = 0x21;
        e[7] = 0x00;
        put32le(e + 8, u32(start));
        put32le(e + 12, u32(secs));
    };
    pentry(p1_start, s1_secs, 0);
    pentry(p2_start, s2_secs, 1);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    if (start1_out)
        *start1_out = p1_start;
    if (start2_out)
        *start2_out = p2_start;
    return img;
}

/* The GPT twin: same two FAT16 volumes, a valid GPT header + entry array
 * (both CRCs computed), a protective MBR. start1/start2 in the outputs. */
inline std::vector<u8> build_two_volume_gpt_disk(const char* l1, const char* l2,
                                                 u64* start1_out = nullptr,
                                                 u64* start2_out = nullptr) {
    u64 s1_secs = 0, s2_secs = 0;
    auto v1 = build_labeled_fat16(l1, &s1_secs);
    auto v2 = build_labeled_fat16(l2, &s2_secs);
    const u64 p1_start = 2048, p2_start = p1_start + s1_secs;
    const u64 disk_secs = p2_start + s2_secs + 4096;
    std::vector<u8> img(disk_secs * 512, 0);
    std::memcpy(img.data() + p1_start * 512, v1.data(), v1.size());
    std::memcpy(img.data() + p2_start * 512, v2.data(), v2.size());
    /* protective MBR */
    u8* mbr = img.data();
    u8* e = mbr + 446;
    e[0] = 0x00;
    e[1] = 0x00;
    e[2] = 0x02;
    e[3] = 0x00;
    e[4] = 0xEE;
    e[5] = 0xFF;
    e[6] = 0xFF;
    e[7] = 0xFF;
    put32le(e + 8, 1);
    put32le(e + 12, u32(disk_secs - 1));
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    /* partition entries at LBA 2 (2 x 128B = fits one sector) */
    constexpr u8 GPT_TYPE_BASIC_DATA[16] = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
                                            0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};
    u8* ents = img.data() + 2 * 512;
    auto gpt_entry = [&](int idx, u64 first, u64 last) {
        u8* p = ents + idx * 128;
        std::memcpy(p, GPT_TYPE_BASIC_DATA, 16);
        /* unique GUID: the same bytes with the last byte bumped */
        std::memcpy(p + 16, GPT_TYPE_BASIC_DATA, 16);
        p[31] = u8(0xC7 + idx);
        put32le(p + 32, u32(first));
        put32le(p + 36, 0);
        put32le(p + 40, u32(last));
        put32le(p + 44, 0);
        p[48] = 0; /* attrs */
    };
    gpt_entry(0, p1_start, p1_start + s1_secs - 1);
    gpt_entry(1, p2_start, p2_start + s2_secs - 1);
    const u32 ents_crc = table::crc32(ents, 256);
    /* GPT header at LBA 1 */
    u8* h = img.data() + 512;
    std::memcpy(h, "EFI PART", 8);
    put32le(h + 8, 0x00010000);
    put32le(h + 12, 92);
    put32le(h + 16, 0); /* header CRC, patched after */
    put32le(h + 24, 1); /* current LBA */
    put32le(h + 32, u32(disk_secs - 1));
    put32le(h + 40, 2);                   /* first usable */
    put32le(h + 48, u32(disk_secs - 34)); /* last usable (low dword; disk is small) */
    put32le(h + 72, 2);                   /* entries start LBA */
    put32le(h + 80, 2);                   /* num entries */
    put32le(h + 84, 128);                 /* entry size */
    put32le(h + 88, ents_crc);
    const u32 hdr_crc = table::crc32(h, 92);
    put32le(h + 16, hdr_crc);
    if (start1_out)
        *start1_out = p1_start;
    if (start2_out)
        *start2_out = p2_start;
    return img;
}

} // namespace testfixture
} // namespace cr

#endif // CR_TEST_FIXTURES_HPP_
