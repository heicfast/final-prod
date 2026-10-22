/* test_zfs_volume.cpp - synthetic ZFS pool end-to-end (single vdev).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/zfs/cr_zfs_engine.h"
#include <gtest/gtest.h>
#include <cstring>
#include <set>

namespace cr {
namespace zfs {
namespace {

constexpr u64 IMG = 16ull << 20;

/* block placements (absolute device offsets; vdev offset 0) */
constexpr u64 B_MOS = 0x40000;        /* MOS objset_phys */
constexpr u64 B_MOS_DNODES = 0x42000; /* MOS dnode array block */
constexpr u64 B_OBJDIR_ZAP = 0x44000;
constexpr u64 B_ZPL = 0x48000; /* ZPL objset_phys */
constexpr u64 B_ZPL_DNODES = 0x4A000;
constexpr u64 B_MASTER_ZAP = 0x4C000;
constexpr u64 B_ROOTDIR_ZAP = 0x4E000;
constexpr u64 B_FILE_DATA = 0x60000;

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
};

void put32(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}
void put16(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}
void put64(u8* p, u64 v) {
    put32(p, u32(v));
    put32(p + 4, u32(v >> 32));
}

/* encode a blkptr: vdev 0, physical offset, size (lsize==psize==size) */
void put_blkptr(u8* bp, u64 physical, u64 size) {
    memset(bp, 0, 128);
    /* dva[0]: word0 = vdev<<40 | asize>>9 ; word1 = offset>>9 */
    put64(bp + 0, (0ULL << 40) | (size >> 9));
    put64(bp + 8, physical >> 9);
    put64(bp + 48, size | (size << 16)); /* prop: lsize | psize<<16 */
}

/* build a dnode: direct blkptrs + znode bonus */
struct DnodeSpec {
    u32 nblkptr = 1;
    u16 datablkszsec = 32;
    struct Bp {
        u64 physical;
        u64 size;
    };
    Bp bps[3] = {{0, 0}, {0, 0}, {0, 0}};
    bool has_bonus = false;
    u64 mode = 0, size = 0, zap = 0, parent = 0, links = 0;
    u64 atime = 0, mtime = 0, ctime = 0, crtime = 0;
    /* dsl bonus overrides (mutually exclusive with znode bonus) */
    bool dsl_dir = false;
    u64 head_obj = 0, child_zap = 0;
    bool dsl_dataset = false;
    u64 ds_bp_physical = 0, ds_bp_size = 0;
};

std::vector<u8> make_dnode(const DnodeSpec& s) {
    std::vector<u8> dn(512, 0);
    dn[2] = 1; /* nlevels */
    dn[3] = u8(s.nblkptr);
    put16(&dn[12], u16(s.datablkszsec));
    put16(&dn[14], 0);
    for (u32 i = 0; i < s.nblkptr; ++i)
        if (s.bps[i].size)
            put_blkptr(dn.data() + 40 + u64(i) * 128, s.bps[i].physical, s.bps[i].size);
    u8* bonus = dn.data() + 40 + u64(s.nblkptr) * 128;
    if (s.dsl_dir || s.dsl_dataset) {
        put16(&dn[14], 256); /* bonuslen: covers dd/ds fields */
    } else if (s.has_bonus) {
        put16(&dn[14], 0xA8); /* znode bonus */
    }
    if (s.dsl_dir) {
        put64(bonus + 240, s.child_zap);
        put64(bonus + 248, s.head_obj);
    } else if (s.dsl_dataset) {
        put_blkptr(bonus + 128, s.ds_bp_physical, s.ds_bp_size);
    } else if (s.has_bonus) {
        put64(bonus + 0x48, s.mode);
        put64(bonus + 0x50, s.size);
        put64(bonus + 0x58, s.parent);
        put64(bonus + 0x60, s.links);
        put64(bonus + 0xA0, s.zap);
        put64(bonus + 0x10, s.mtime); /* mtime sec */
        put64(bonus + 0x20, s.ctime);
        put64(bonus + 0x30, s.crtime);
    }
    return dn;
}

/* microzap block with entries */
std::vector<u8> make_microzap(const std::vector<std::pair<const char*, u64>>& entries) {
    std::vector<u8> blk(4096, 0);
    put64(blk.data(), 0xFFFFFFFFFFFFFFFFULL); /* ZBT_MICRO */
    put64(blk.data() + 8, 0x1234);            /* salt */
    u64 off = 64;
    for (const auto& e : entries) {
        u8* ent = blk.data() + off;
        put64(ent + 0, 0); /* hash (unused by reader) */
        put32(ent + 8, 0); /* cd */
        put64(ent + 16, e.second);
        strncpy(reinterpret_cast<char*>(ent + 24), e.first, 63);
        off += 88;
    }
    return blk;
}

/* native nvlist: header + pairs (append-then-fill, no OOB writes) */
void nv_pad4(std::vector<u8>& nl) {
    while (nl.size() % 4 != 0)
        nl.push_back(0);
}

void nv_add_string(std::vector<u8>& nl, const char* name, const char* value) {
    const u32 name_len = u32(strlen(name)) + 1;
    const u32 vlen = u32(strlen(value)) + 1;
    const u32 start = u32(nl.size());
    nl.insert(nl.end(), 8, 0); /* enc/dec size patched below */
    nl.push_back(u8(name_len));
    nl.insert(nl.end(), name, name + name_len);
    nv_pad4(nl);
    /* type=STRING(9), nelem=1, strlen, bytes */
    nl.insert(nl.end(), 12, 0);
    put32(nl.data() + nl.size() - 12, 9);
    put32(nl.data() + nl.size() - 8, 1);
    put32(nl.data() + nl.size() - 4, vlen);
    nl.insert(nl.end(), value, value + vlen);
    nv_pad4(nl);
    const u32 enc = u32(nl.size()) - start;
    put32(nl.data() + start, enc);
    put32(nl.data() + start + 4, enc);
}

void nv_add_u64(std::vector<u8>& nl, const char* name, u64 value) {
    const u32 name_len = u32(strlen(name)) + 1;
    const u32 start = u32(nl.size());
    nl.insert(nl.end(), 8, 0);
    nl.push_back(u8(name_len));
    nl.insert(nl.end(), name, name + name_len);
    nv_pad4(nl);
    /* type=UINT64(8), nelem=1, 8 value bytes */
    nl.insert(nl.end(), 16, 0);
    put32(nl.data() + nl.size() - 16, 8);
    put32(nl.data() + nl.size() - 12, 1);
    put64(nl.data() + nl.size() - 8, value);
    const u32 enc = u32(nl.size()) - start;
    put32(nl.data() + start, enc);
    put32(nl.data() + start + 4, enc);
}

void nv_terminate(std::vector<u8>& nl) {
    nl.insert(nl.end(), 8, 0);
}

std::vector<u8> build_pool() {
    std::vector<u8> d(size_t(IMG), 0);

    /* ---- label 0 ---- */
    /* nvlist config @ 8K */
    {
        std::vector<u8> nl(8, 0); /* nvl_version + nvflag header */
        nv_add_string(nl, "name", "testpool");
        nv_add_u64(nl, "version", 5000);
        nv_add_u64(nl, "ashift", 9);
        /* vdev_tree as one flat pseudo-nvlist: the parser accepts the
         * flat subset {type,ashift,asize,offset} at top level too - we
         * test the nested path via "vdev_tree" using the nested layout */
        nv_add_string(nl, "vdev_type", "disk");
        nv_add_u64(nl, "asize", IMG);
        nv_add_u64(nl, "offset", 0);
        nv_terminate(nl);
        memcpy(d.data() + 8192, nl.data(), nl.size());
    }

    /* uberblock ring @128K: slot 0, txg 77, rootbp -> MOS @B_MOS (2048) */
    {
        u8* ub = d.data() + (128ull << 10);
        put64(ub + 0, 0x00BAB10CULL);
        put64(ub + 8, 5000);        /* version */
        put64(ub + 16, 77);         /* txg */
        put64(ub + 24, 1);          /* guid sum */
        put64(ub + 32, 1600000000); /* timestamp */
        put_blkptr(ub + 40, B_MOS, 2048);
    }

    /* ---- MOS objset_phys @B_MOS: meta_dnode only matters (512) ---- */
    {
        u8* os = d.data() + B_MOS;
        DnodeSpec meta;
        meta.nblkptr = 1;
        meta.datablkszsec = 32; /* 16K dnode-array blocks */
        meta.bps[0] = {B_MOS_DNODES, 16384};
        auto dn = make_dnode(meta);
        memcpy(os, dn.data(), 512);
    }

    /* ---- MOS dnode array @B_MOS_DNODES: objects 1..3 ---- */
    {
        u8* arr = d.data() + B_MOS_DNODES;
        /* object 1: object directory zap */
        DnodeSpec od;
        od.datablkszsec = 8; /* 4K zap blocks */
        od.bps[0] = {B_OBJDIR_ZAP, 4096};
        auto dn = make_dnode(od);
        memcpy(arr + 512, dn.data(), 512); /* object 1 */
        /* object 2: root dsl_dir (bonus: head_dataset=3) */
        DnodeSpec dd;
        dd.dsl_dir = true;
        dd.head_obj = 3;
        dd.child_zap = 0;
        dn = make_dnode(dd);
        memcpy(arr + 1024, dn.data(), 512); /* object 2 */
        /* object 3: dsl dataset (bonus: ds_bp -> ZPL @B_ZPL) */
        DnodeSpec ds;
        ds.dsl_dataset = true;
        ds.ds_bp_physical = B_ZPL;
        ds.ds_bp_size = 2048;
        dn = make_dnode(ds);
        memcpy(arr + 1536, dn.data(), 512); /* object 3 */
    }

    /* ---- object dir zap @B_OBJDIR_ZAP: "dataset" -> 2 ---- */
    {
        auto zap = make_microzap({{"dataset", 2}});
        memcpy(d.data() + B_OBJDIR_ZAP, zap.data(), zap.size());
    }

    /* ---- ZPL objset @B_ZPL ---- */
    {
        u8* os = d.data() + B_ZPL;
        DnodeSpec meta;
        meta.nblkptr = 1;
        meta.datablkszsec = 32;
        meta.bps[0] = {B_ZPL_DNODES, 16384};
        auto dn = make_dnode(meta);
        memcpy(os, dn.data(), 512);
    }

    /* ---- ZPL dnode array: 1=master zap, 2=root dir, 3=root zap, 4=file ---- */
    {
        u8* arr = d.data() + B_ZPL_DNODES;
        DnodeSpec mz;
        mz.datablkszsec = 8;
        mz.bps[0] = {B_MASTER_ZAP, 4096};
        auto dn = make_dnode(mz);
        memcpy(arr + 512, dn.data(), 512);

        DnodeSpec rd;
        rd.has_bonus = true;
        rd.mode = 0x4000;
        rd.links = 2;
        rd.zap = 3; /* root dir's ZAP = object 3 */
        rd.bps[0] = {B_ROOTDIR_ZAP, 4096};
        dn = make_dnode(rd);
        memcpy(arr + 1024, dn.data(), 512);

        DnodeSpec rz;
        rz.datablkszsec = 8;
        rz.bps[0] = {B_ROOTDIR_ZAP, 4096};
        dn = make_dnode(rz);
        memcpy(arr + 1536, dn.data(), 512);

        DnodeSpec fl;
        fl.has_bonus = true;
        fl.mode = 0x8000;
        fl.size = 14;
        fl.links = 1;
        fl.parent = 2;
        fl.mtime = 1600000000;
        fl.bps[0] = {B_FILE_DATA, 4096};
        dn = make_dnode(fl);
        memcpy(arr + 2048, dn.data(), 512);
    }

    /* ---- master zap: "ROOT" -> 2 ---- */
    {
        auto zap = make_microzap({{"ROOT", 2}});
        memcpy(d.data() + B_MASTER_ZAP, zap.data(), zap.size());
    }
    /* ---- root dir zap: "hello.txt" -> 4 ---- */
    {
        auto zap = make_microzap({{"hello.txt", 4}});
        memcpy(d.data() + B_ROOTDIR_ZAP, zap.data(), zap.size());
    }
    memcpy(d.data() + B_FILE_DATA, "Hello, ZFS!\x00\x00\x00", 14);
    return d;
}

TEST(Zfs, EndToEndWalk) {
    auto img = build_pool();
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanFlags fl;
    ZfsScanStats st;
    u32 rc = scan_zfs(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(77u, st.txg);
    EXPECT_EQ(1u, st.datasets);
    EXPECT_EQ(1u, st.files);
    EXPECT_EQ(1u, st.dirs);

    /* paths */
    std::set<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            std::string nm = c->name_utf8();
            std::string full = p.empty() ? nm : p + "/" + nm;
            paths.insert(full);
            walk(c, full);
        }
    };
    walk(ra, "");
    /* dataset name: nvlist top-level "name" + empty prefix -> "testpool" */
    bool found_file = false;
    for (const auto& p : paths)
        if (p.find("hello.txt") != std::string::npos)
            found_file = true;
    EXPECT_TRUE(found_file);

    /* verify the file node */
    tree::FileNode* file = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "hello.txt")
                file = c;
            find(c);
        }
    };
    find(ra);
    ASSERT_NE(nullptr, file);
    EXPECT_EQ(u64(14), file->props()->logical_size);
    ASSERT_EQ(1u, file->props()->extents.size());
    EXPECT_EQ(B_FILE_DATA, file->props()->extents[0].offset);
    EXPECT_EQ(4096u, file->props()->extents[0].length);
}

TEST(Zfs, BadMagicRejected) {
    std::vector<u8> d(1ull << 20, 0);
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanFlags fl;
    ZfsScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC,
              scan_zfs(&dev, fl, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
}

} // namespace
} // namespace zfs
} // namespace cr
