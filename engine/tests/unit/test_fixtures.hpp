/* test_fixtures.hpp - shared test fixtures (LDM dynamic-disk builder
 * + minimal FAT16 volume), used by test_ldm.cpp and test_orchestrator.cpp.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TEST_FIXTURES_HPP_
#define CR_TEST_FIXTURES_HPP_

#include "cr/table/cr_ldm.h"
#include "cr/exfat/cr_exfat_engine.h"
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

/* ------------------------------------------------------------------ */
/* Shared exFAT volume builder (extracted from test_exfat_volume.cpp  */
/* for S-051's mixed-family whole-disk fixtures; the original TU      */
/* consumes the same code via using-declarations - one builder, no    */
/* drift).                                                            */
/* ------------------------------------------------------------------ */

namespace cr {
namespace testfixture {

inline void fx_put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
inline void fx_put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}
inline void fx_put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8(x >> (8 * i));
}

inline constexpr u32 EXFAT_TEST_TS = (0x5A46u << 16) | 0x6000u; /* 2025-02-06 12:00:
    every field in range - M-014's timestamp gates reject zero-day/garbage
    stamps, so planted sets carry a realistic stamp like real media */

struct SetBuilder {
    std::vector<u8> entries;
    size_t name_entry_first_ = 0; /* S-013: first name-entry offset (multi-chunk flip) */

    void file_entry(u16 attribute, u32 ts, u8 tz = 0) {
        u8* e = add();
        e[0] = 0x85;
        e[1] = 2;                            /* secondary count: stream + 1 name */
        fx_put16(entries, cur_off() + 2, 0); /* checksum filled later */
        fx_put16(entries, cur_off() + 4, attribute);
        fx_put32v(entries, cur_off() + 8, ts);  /* create */
        fx_put32v(entries, cur_off() + 12, ts); /* modify */
        fx_put32v(entries, cur_off() + 16, ts); /* access */
        e[20] = 0;
        e[21] = 0; /* 10ms */
        /* tz bytes at +0x16/+0x17/+0x18 (create/modify/access). S-011
         * fixtures set them to prove the engine does NOT apply them (og
         * parity: the bytes are parsed but ignored). */
        e[22] = tz;
        e[23] = tz;
        e[24] = tz;
    }
    void file_entry_deleted(u16 attribute, u32 ts) {
        file_entry(attribute, ts);
        entries[cur_off()] = 0x05;
    }
    void stream_entry(u8 flags, u8 name_len, u32 first_cluster, u64 data_len) {
        u8* e = add();
        e[0] = 0xC0;
        e[1] = flags;
        e[3] = name_len;
        fx_put64(entries, cur_off() + 8, data_len); /* valid data len */
        fx_put32v(entries, cur_off() + 20, first_cluster);
        fx_put64(entries, cur_off() + 24, data_len);
    }
    void stream_entry_deleted(u8 flags, u8 name_len, u32 first_cluster, u64 data_len) {
        stream_entry(flags, name_len, first_cluster, data_len);
        entries[cur_off()] = 0x40;
    }
    void name_entry(const char16_t* nm) {
        std::u16string n = nm;
        /* S-013 fixture: names longer than 15 chars span ceil(len/15)
         * name entries (spec 7.6.3 chunked UTF-16), 15 chars each */
        const size_t chunks = (n.size() + 14) / 15;
        name_entry_first_ = entries.size();
        for (size_t c = 0; c < chunks; ++c) {
            u8* e = add();
            e[0] = 0xC1;
            for (size_t i = 0; i < 15; ++i) {
                const size_t idx = c * 15 + i;
                fx_put16(entries, cur_off() + 2 + i * 2, idx < n.size() ? u16(n[idx]) : 0);
            }
        }
    }
    void name_entry_deleted(const char16_t* nm) {
        name_entry(nm);
        /* flip EVERY name entry of this name (multi-chunk sets) */
        for (size_t o = name_entry_first_; o < entries.size(); o += 32)
            entries[o] = 0x41;
    }
    /* fill SetCheckSum over the built set (excluding the +2 field) */
    void finish_set(std::vector<u8>& root, size_t set_at) {
        /* normalize the secondary-count byte to the real entry count
         * (S-013: multi-name sets change the count; single-name callers
         * are unaffected: 2 == 3 entries - 1) */
        entries[1] = u8(entries.size() / 32 - 1);
        /* compute over the set bytes, skipping the +2 checksum field */
        u16 sum = 0;
        for (size_t b = 0; b < entries.size() * 32; ++b) {
            if (b >= 2 && b < 4)
                continue;
            sum = static_cast<u16>(((sum & 1) ? 0x8000u : 0u) + (sum >> 1) +
                                   *(root.data() + set_at + b));
        }
        fx_put16(root, set_at + 2, sum);
    }

    size_t set_off = 0; /* offset within entries where the SET starts */

private:
    u8* add() {
        entries.resize(entries.size() + 32, 0);
        return entries.data() + entries.size() - 32;
    }
    size_t cur_off() const { return entries.size() - 32; }
};

/* N-067 helper: the fixture's 64-entry upcase table (a-z folded) and its
 * exact checksum32 stamp - the dentry and the stream stay consistent by
 * construction. Mirrors the bytes build_exfat_image plants at cluster 60. */
inline u32 exfat_test_upcase_table_checksum() {
    std::vector<u8> tbl(128, 0);
    for (u32 i = 0; i < 64; ++i) {
        const u32 ch = (i >= 'a' && i <= 'z') ? (i - 0x20) : i;
        tbl[i * 2] = u8(ch);
        tbl[i * 2 + 1] = u8(ch >> 8);
    }
    return exfat::checksum32(tbl.data(), u32(tbl.size()), 0);
}

inline std::vector<u8> build_exfat_image(u8 tz = 0) {
    /* 2048 sectors (1MB) - spec minimum volume size; layout keeps the
     * small fixture geometry inside (heap 25..124, rest unused) */
    std::vector<u8> img(u64(2048) * 512, 0);

    /* ---- boot sector (main at 0, backup at 12) ---- */
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    /* MustBeZero 11..63 stays zero */
    fx_put64(img, 64, 0);    /* partition offset */
    fx_put64(img, 72, 2048); /* volume length (spec min 1MB) */
    fx_put32v(img, 80, 24);  /* fat offset */
    fx_put32v(img, 84, 1);   /* fat length */
    fx_put32v(img, 88, 25);  /* cluster heap offset */
    fx_put32v(img, 92, 100); /* cluster count */
    fx_put32v(img, 96, 2);   /* root first cluster */
    fx_put32v(img, 100, 0x12345678);
    fx_put16(img, 104, 0x0100); /* fs revision 1.00 */
    fx_put16(img, 106, 0);      /* volume flags */
    b[108] = 9;                 /* 512B sectors */
    b[109] = 0;                 /* 1 sector per cluster */
    b[110] = 1;                 /* 1 FAT */
    b[111] = 0x80;
    b[112] = 0xFF; /* percent in use: n/a */
    b[510] = 0x55;
    b[511] = 0xAA;
    std::memcpy(img.data() + 12 * 512, b, 512); /* backup copy */

    /* extended boot sectors 1..8 / 13..20: 0xAA550000 at +508 */
    for (u32 s = 1; s <= 8; ++s) {
        img[s * 512 + 508] = 0x00;
        img[s * 512 + 509] = 0x00;
        img[s * 512 + 510] = 0x55;
        img[s * 512 + 511] = 0xAA;
        img[(12 + s) * 512 + 508] = 0x00;
        img[(12 + s) * 512 + 509] = 0x00;
        img[(12 + s) * 512 + 510] = 0x55;
        img[(12 + s) * 512 + 511] = 0xAA;
    }
    /* boot-region checksum: over sectors 0..10, bytes 106/107/112 of
     * sector 0 skipped entirely (spec 3.1.14.1); FILL-REPEATED at 11/23 */
    u32 chk = 0;
    for (u32 s = 0; s <= 10; ++s) {
        for (u32 i = 0; i < 512; ++i) {
            if (s == 0 && ((i >= 106 && i < 108) || i == 112))
                continue;
            chk = ((chk & 1) ? 0x80000000u : 0u) + (chk >> 1) + img[s * 512 + i];
        }
    }
    for (u32 k = 0; k < 128; ++k) {
        fx_put32v(img, 11 * 512 + k * 4, chk);
        fx_put32v(img, 23 * 512 + k * 4, chk);
    }

    /* ---- FAT (sector 24) ---- */
    const size_t FAT = 24 * 512;
    fx_put32v(img, FAT + 2 * 4, 3);          /* root chain: 2 -> 3 */
    fx_put32v(img, FAT + 3 * 4, 0xFFFFFFFF); /* root EOC */
    fx_put32v(img, FAT + 20 * 4, 21);
    fx_put32v(img, FAT + 21 * 4, 22);
    fx_put32v(img, FAT + 22 * 4, 0xFFFFFFFF); /* doc.txt 20-22 */
    fx_put32v(img, FAT + 50 * 4, 0xFFFFFFFF); /* bitmap */
    fx_put32v(img, FAT + 60 * 4, 0xFFFFFFFF); /* upcase table */
    fx_put32v(img, FAT + 70 * 4, 0xFFFFFFFF); /* subdir */

    /* ---- allocation bitmap at cluster 50 (sector 75) ----
     * M-054: the on-disk bitmap's bit i describes cluster i+2 (MS exFAT
     * spec section 7.1) - setbit(c) writes the bit FOR cluster c. */
    {
        const size_t BM = 25 * 512 + (50 - 2) * 512;
        auto setbit = [&](u32 c) {
            const u32 bit = c - 2; /* cluster c -> bit c-2 (spec) */
            img[BM + bit / 8] |= u8(1u << (bit % 8));
        };
        setbit(2);
        setbit(3); /* root (2-cluster chain) */
        setbit(10);
        setbit(11); /* photo.jpg */
        setbit(20);
        setbit(21);
        setbit(22); /* doc.txt */
        setbit(50); /* bitmap itself */
        setbit(60); /* upcase table */
        setbit(70); /* subdir */
        setbit(80); /* inner.txt */
        setbit(40); /* over.dat's cluster: REUSED */
        /* 30 (gone.dat) NOT set */
    }

    /* ---- root dir at cluster 2 (sector 25) ---- */
    const size_t ROOT = 25 * 512;
    size_t at = 0;
    const u32 TS = (0x5A46u << 16) | 0x6000u; /* 2025-02-06 12:00 local */

    /* 0x81 bitmap dentry */
    {
        u8* e = img.data() + ROOT;
        std::memset(e, 0, 32);
        e[0] = 0x81;
        e[1] = 0;
        fx_put32v(img, ROOT + 20, 50);
        fx_put64(img, ROOT + 24, 13); /* (100+7)/8 */
        at += 32;
    }
    /* 0x82 upcase: a REAL 64-entry table at cluster 60 with its exact
     * checksum32 stamp (N-067: the volume-integrity pass validates this
     * stream; corrupt-checksum variants mutate the dentry/bytes). */
    {
        u8* e = img.data() + ROOT + at;
        std::memset(e, 0, 32);
        e[0] = 0x82;
        fx_put32v(img, ROOT + at + 4, exfat_test_upcase_table_checksum());
        fx_put32v(img, ROOT + at + 20, 60);
        fx_put64(img, ROOT + at + 24, 128);
        /* the table bytes at cluster 60 = sector 83: 64 u16 upcase
         * entries, ASCII a-z folded (identical bytes to the checksum
         * helper above) */
        for (u32 i = 0; i < 64; ++i) {
            const u32 ch = (i >= 'a' && i <= 'z') ? (i - 0x20) : i;
            fx_put16(img, 83 * 512 + i * 2, u16(ch));
        }
        at += 32;
    }
    /* 0x83 volume label - M-079 spec-repair: the label chars live at
     * +4 (exFAT spec 7.2: EntryType@0, CharacterCount@1, Reserved@2-3,
     * VolumeLabel@4-33); the old +2 write overlapped the reserved field
     * (a spec-invalid entry - invisible to the skip-only tests that
     * consumed it before, wrong for anything that reads the label). */
    {
        u8* e = img.data() + ROOT + at;
        std::memset(e, 0, 32);
        e[0] = 0x83;
        e[1] = 7;
        const char16_t* lbl = u"TESTVOL";
        for (int i = 0; i < 7; ++i)
            fx_put16(img, ROOT + at + 4 + i * 2, u16(lbl[i]));
        at += 32;
    }
    /* photo.jpg: live NoFatChain (clusters 10-11, 1024B). Carries the
     * caller's tz bytes (S-011: non-zero values prove the engine leaves
     * them unapplied). */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x20, TS, tz);
        sb.stream_entry(0x03 /*AllocPossible|NoFatChain*/, 9, 10, 1024);
        sb.name_entry(u"photo.jpg");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* doc.txt: live FAT chain 20-22 (1536B) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x01 /*AllocPossible, FAT chain*/, 7, 20, 1536);
        sb.name_entry(u"doc.txt");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* gone.dat: DELETED NoFatChain (cluster 30, 512B; bitmap free).
     * M-013: the checksum IS stamped (finish_set) - a real deleted set
     * keeps its original SetChecksum field (deletion only clears the
     * InUse bits; the og's dual-form gate expects the restored form to
     * match it). The old "no checksum validation" comment encoded
     * N-003's refuted premise. */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry_deleted(0x20, TS);
        sb.stream_entry_deleted(0x03, 8, 30, 512);
        sb.name_entry_deleted(u"gone.dat");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* over.dat: DELETED NoFatChain (cluster 40; bitmap SET -> Overwritten) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry_deleted(0x20, TS);
        sb.stream_entry_deleted(0x03, 8, 40, 512);
        sb.name_entry_deleted(u"over.dat");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* subdir: live dir (FAT chain, cluster 70, 512B) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x10 /*Directory*/, TS);
        sb.stream_entry(0x01, 6, 70, 512);
        sb.name_entry(u"subdir");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* end of directory: a zero entry follows (image is zero-filled) */

    /* ---- subdir content at cluster 70 (sector 93) ---- */
    {
        const size_t SUB = 25 * 512 + (70 - 2) * 512;
        SetBuilder sb;
        size_t set_at = SUB;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x03, 9, 80, 256);
        sb.name_entry(u"inner.txt");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
    }
    /* content markers */
    std::memcpy(img.data() + 25 * 512 + (10 - 2) * 512, "PHOTODATA", 9);
    std::memcpy(img.data() + 25 * 512 + (80 - 2) * 512, "INNERDATA", 9);
    return img;
}

} // namespace testfixture
} // namespace cr

#endif // CR_TEST_FIXTURES_HPP_
