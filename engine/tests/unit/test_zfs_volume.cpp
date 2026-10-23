/* test_zfs_volume.cpp - synthetic ZFS pool end-to-end (single vdev).
 *
 * Session-5 rewrite: every structure built to the OpenZFS on-disk layout
 * (see AUDIT.md) - labels at {0, 256K, end-512K, end-256K}, config nvlist
 * at label+16K, uberblock ring at label+128K (128 x 1KiB), dnode fields
 * at {datablkszsec@8, bonuslen@10, maxblkid@16, blkptr@64}, microzap
 * entries {64B, value@0, name@14}, fzap header + embedded ptrtbl + leaf
 * chunks, SA layout-1 bonuses, legacy znode_phys bonuses, (v+1)<<9 bp
 * sizes, compressed (zle) extents and embedded block pointers.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/zfs/cr_zfs_engine.h"
#include "cr/compress/cr_decomp.h"
#include <gtest/gtest.h>
#include <cstring>
#include <set>

namespace cr {
namespace zfs {
namespace {

constexpr u64 IMG = 16ull << 20;

/* block placements (absolute device offsets; vdev offset 0). All data
 * lives past the two head labels (0..512K) and before the two tail labels
 * (last 512K) - labels own their footprints. */
constexpr u64 B_MOS = 0x80000;        /* MOS objset_phys */
constexpr u64 B_MOS_DNODES = 0x82000; /* MOS dnode array block */
constexpr u64 B_OBJDIR_ZAP = 0x84000;
constexpr u64 B_ZPL = 0x86000; /* ZPL objset_phys */
constexpr u64 B_ZPL_DNODES = 0x88000;
constexpr u64 B_MASTER_ZAP = 0x8E000;
constexpr u64 B_ROOTDIR_ZAP = 0x90000;
constexpr u64 B_BIGDIR_ZAP = 0x92000; /* fat zap (embedded ptrtbl) */
constexpr u64 B_FILE_DATA = 0xA0000;
constexpr u64 B_ZLE_DATA = 0xA2000;

constexpr u64 TXG = 77;

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

/* ---- blkptr construction (OpenZFS spa.h packing) ---- */
void make_bp(u8* bp, u64 offset, u64 psize, u64 lsize, u32 compress) {
    memset(bp, 0, 128);
    /* DVA0: w0 = vdev(24b)<<32 | asize>>9 ; w1 = offset>>9
     * (spa.h DVA_SET_VDEV writes bits 32..55 of word 0) */
    const u64 asize = (psize + 511) & ~511ull;
    put64(bp + 0, (0ull << 32) | (asize >> 9));
    put64(bp + 8, offset >> 9);
    /* prop: lsize sectors-1 | psize sectors-1<<16 | comp<<32 */
    const u64 prop = (((lsize >> 9) - 1) & 0xFFFFull) | ((((psize >> 9) - 1) & 0xFFFFull) << 16) |
                     (u64(compress) << 32);
    put64(bp + 48, prop);
    put64(bp + 72, TXG); /* birth */
}

/* ---- dnode construction (OpenZFS dnode.h) ---- */
void make_dnode(u8* dn, u32 type, u32 bonustype, u16 datablkszsec, u8 nblkptr, u8 nlevels,
                u32 bonuslen) {
    memset(dn, 0, 512);
    dn[0] = u8(type);
    dn[2] = nlevels;
    dn[3] = nblkptr;
    dn[4] = u8(bonustype);
    put16(dn + 8, datablkszsec);
    put16(dn + 10, u16(bonuslen));
}

/* ---- SA layout-1 bonus (sa_impl.h; attr order = zfs_attr_table
 * registration order from module/zfs/zfs_sa.c: atime mtime ctime
 * crtime gen mode size parent links xattr rdev flags uid gid) ---- */
struct SaAttrs {
    u64 atime = 0, mtime = 0, ctime = 0, crtime = 0;
    u64 mode = 0, uid = 0, gid = 0, parent = 0, links = 0, size = 0, flags = 0;
};
std::vector<u8> make_sa_bonus(const SaAttrs& a) {
    std::vector<u8> b(8 + 144, 0);
    put32(b.data(), 0x2F505A);                    /* SA_MAGIC */
    put16(b.data() + 4, u16(1) | (u16(1) << 10)); /* layout 1, hdrsize 8 */
    put64(b.data() + 8 + 0, a.atime);             /* atime (16 bytes: sec+usec) */
    put64(b.data() + 8 + 16, a.mtime);
    put64(b.data() + 8 + 32, a.ctime);
    put64(b.data() + 8 + 48, a.crtime);
    put64(b.data() + 8 + 64, 1); /* gen */
    put64(b.data() + 8 + 72, a.mode);
    put64(b.data() + 8 + 80, a.size);
    put64(b.data() + 8 + 88, a.parent);
    put64(b.data() + 8 + 96, a.links);
    put64(b.data() + 8 + 104, 0); /* xattr */
    put64(b.data() + 8 + 112, 0); /* rdev */
    put64(b.data() + 8 + 120, a.flags);
    put64(b.data() + 8 + 128, a.uid);
    put64(b.data() + 8 + 136, a.gid);
    return b;
}

/* ---- legacy znode_phys bonus (include/sys/zfs_sa.h) ---- */
std::vector<u8> make_znode_bonus(const SaAttrs& a) {
    std::vector<u8> b(128, 0);
    put64(b.data() + 0x00, a.atime);
    put64(b.data() + 0x10, a.mtime);
    put64(b.data() + 0x20, a.ctime);
    put64(b.data() + 0x30, a.crtime);
    put64(b.data() + 0x48, a.mode);
    put64(b.data() + 0x50, a.size);
    put64(b.data() + 0x58, a.parent);
    put64(b.data() + 0x60, a.links);
    return b;
}

/* ---- microzap block (zap_impl.h) ---- */
std::vector<u8> make_microzap(const std::vector<std::pair<std::string, u64>>& entries) {
    std::vector<u8> blk(512, 0);
    put64(blk.data() + 0, (1ull << 63) + 3); /* ZBT_MICRO */
    put64(blk.data() + 8, 12345);            /* salt */
    u32 at = 64;
    for (const auto& e : entries) {
        if (at + 64 > blk.size())
            break;
        u8* slot = blk.data() + at;
        put64(slot + 0, e.second); /* mze_value */
        put32(slot + 8, 0);        /* mze_cd */
        memcpy(slot + 14, e.first.c_str(), e.first.size() < 50 ? e.first.size() : 49);
        at += 64;
    }
    return blk;
}

/* ---- fat zap: header block + leaf blocks (zap_leaf.h) ---- */
constexpr u64 FZ_BLOCK = 512;

std::vector<u8> make_fzap_leaf(const std::vector<std::pair<std::string, u64>>& entries) {
    /* 512B leaf: header (48 B) + hash table (512>>5 = 16 x u16) + 15 chunks
     * of 24 B. Name arrays use 21-byte payload chunks. */
    std::vector<u8> leaf(FZ_BLOCK, 0);
    put64(leaf.data() + 0, (1ull << 63) + 0); /* ZBT_LEAF */
    put64(leaf.data() + 16, 0);               /* prefix */
    put32(leaf.data() + 24, 0x2AB1EAF);       /* ZAP_LEAF_MAGIC */
    put16(leaf.data() + 28, 0);               /* nfree (unused by reader) */
    put16(leaf.data() + 30, u16(entries.size()));
    const u32 chunks_at = 48 + 16 * 2; /* 80 */
    const u32 nchunks = u32((FZ_BLOCK - chunks_at) / 24);
    u32 next_chunk = 0;
    for (const auto& e : entries) {
        if (next_chunk + 2 > nchunks)
            break;
        const u32 entry_chunk = next_chunk++;
        /* value array chunk (8-byte u64 value) */
        const u32 value_chunk = next_chunk++;
        u8* vc = leaf.data() + chunks_at + u64(value_chunk) * 24;
        vc[0] = 251; /* ZAP_CHUNK_ARRAY */
        put64(vc + 1, e.second);
        put16(vc + 22, 0xFFFF); /* la_next: chain end */
        /* name array chunks (21 bytes payload each) */
        std::vector<u8> name(e.first.begin(), e.first.end());
        name.push_back(0); /* include NUL like ZFS does */
        u32 first_name_chunk = next_chunk;
        u32 consumed = 0;
        u32 prev_chunk = 0xFFFFFFFF;
        while (consumed < name.size()) {
            if (next_chunk >= nchunks)
                break;
            const u32 ci = next_chunk++;
            u8* nc = leaf.data() + chunks_at + u64(ci) * 24;
            nc[0] = 251;
            const u32 take = u32(name.size()) - consumed < 21 ? u32(name.size()) - consumed : 21;
            memcpy(nc + 1, name.data() + consumed, take);
            put16(nc + 22, 0xFFFF);
            if (prev_chunk != 0xFFFFFFFF)
                put16(leaf.data() + chunks_at + u64(prev_chunk) * 24 + 22, u16(ci));
            prev_chunk = ci;
            consumed += take;
        }
        u8* ec = leaf.data() + chunks_at + u64(entry_chunk) * 24;
        ec[0] = 252; /* ZAP_CHUNK_ENTRY */
        ec[1] = 8;   /* value_intlen */
        put16(ec + 4, u16(first_name_chunk));
        put16(ec + 6, u16(name.size()));
        put16(ec + 8, u16(value_chunk));
        put16(ec + 10, 1); /* value_numints */
        put32(ec + 12, 0); /* cd */
        put64(ec + 16, 0); /* hash */
    }
    return leaf;
}

std::vector<u8> make_fzap_header(u64 leaf_block) {
    std::vector<u8> hdr(FZ_BLOCK, 0);
    put64(hdr.data() + 0, (1ull << 63) + 1); /* ZBT_HEADER */
    put64(hdr.data() + 8, 0x2F52AB2ABull);   /* ZAP_MAGIC */
    /* embedded pointer table (zt_blk == 0): second half of the block */
    put64(hdr.data() + 16, 0); /* zt_blk = 0 -> embedded */
    put64(hdr.data() + 24, 0); /* zt_numblks */
    put64(hdr.data() + 32, 4); /* zt_shift: 16 entries */
    put64(hdr.data() + 56, 2); /* freeblk */
    put64(hdr.data() + 64, 1); /* num_leafs */
    put64(hdr.data() + 72, 0); /* num_entries */
    /* embedded ptrtbl at block/2, entry 0 -> leaf block number */
    put64(hdr.data() + FZ_BLOCK / 2, leaf_block);
    return hdr;
}

/* ---- XDR nvlist (vdev labels are packed NV_ENCODE_XDR - vdev_label.c;
 * big-endian pair stream per module/nvpair/nvpair.c nvs_xdr_*:
 *   buffer: [encoding=1][endian][rsvd x2] {version BE32}{nvflag BE32}
 *   pair: [enc_len BE32][dec_len BE32] [name_len BE32 (excl NUL)]
 *         [name][pad4] [type BE32][nelem BE32] [value]
 *   XDR string value: [len BE32 excl NUL][bytes][pad4]
 *   embedded nvlist: {version BE32}{nvflag BE32} pairs... [0][0] ) ---- */
struct NvBuf {
    std::vector<u8> data;
    static void b32(std::vector<u8>& v, u32 x) {
        v.push_back(u8(x >> 24));
        v.push_back(u8(x >> 16));
        v.push_back(u8(x >> 8));
        v.push_back(u8(x));
    }
    static void b64(std::vector<u8>& v, u64 x) {
        b32(v, u32(x >> 32));
        b32(v, u32(x));
    }
    static u32 pad4(u32 n) { return (n + 3) & ~3u; }
    void u64_pair(const char* name, u64 v) {
        const u32 nlen = u32(strlen(name));
        std::vector<u8> pair;
        b32(pair, 0); /* enc_len placeholder */
        b32(pair, 0); /* dec_len placeholder */
        b32(pair, nlen);
        pair.insert(pair.end(), name, name + nlen);
        pair.resize(pad4(u32(pair.size())), 0);
        b32(pair, 8); /* DATA_TYPE_UINT64 */
        b32(pair, 1); /* nelem */
        b64(pair, v);
        const u32 total = u32(pair.size());
        pair[0] = u8(total >> 24);
        pair[1] = u8(total >> 16);
        pair[2] = u8(total >> 8);
        pair[3] = u8(total);
        pair[4] = 0;
        pair[5] = 0;
        pair[6] = 0;
        pair[7] = 8; /* dec_len: plausible in-memory size */
        data.insert(data.end(), pair.begin(), pair.end());
    }
    void str_pair(const char* name, const std::string& v) {
        const u32 nlen = u32(strlen(name));
        std::vector<u8> pair;
        b32(pair, 0);
        b32(pair, 0);
        b32(pair, nlen);
        pair.insert(pair.end(), name, name + nlen);
        pair.resize(pad4(u32(pair.size())), 0);
        b32(pair, 9); /* DATA_TYPE_STRING */
        b32(pair, 1);
        b32(pair, u32(v.size())); /* excl NUL */
        pair.insert(pair.end(), v.begin(), v.end());
        pair.resize(pad4(u32(pair.size())), 0);
        const u32 total = u32(pair.size());
        pair[0] = u8(total >> 24);
        pair[1] = u8(total >> 16);
        pair[2] = u8(total >> 8);
        pair[3] = u8(total);
        data.insert(data.end(), pair.begin(), pair.end());
    }
    void nvlist_pair(const char* name, const std::vector<u8>& child) {
        const u32 nlen = u32(strlen(name));
        std::vector<u8> pair;
        b32(pair, 0);
        b32(pair, 0);
        b32(pair, nlen);
        pair.insert(pair.end(), name, name + nlen);
        pair.resize(pad4(u32(pair.size())), 0);
        b32(pair, 19); /* DATA_TYPE_NVLIST */
        b32(pair, 1);
        pair.insert(pair.end(), child.begin(), child.end());
        const u32 total = u32(pair.size());
        pair[0] = u8(total >> 24);
        pair[1] = u8(total >> 16);
        pair[2] = u8(total >> 8);
        pair[3] = u8(total);
        data.insert(data.end(), pair.begin(), pair.end());
    }
    /* stream bytes WITHOUT the top nvs_header (embedded/recursive form) */
    std::vector<u8> stream() const {
        std::vector<u8> out;
        b32(out, 0); /* nvl_version */
        b32(out, 1); /* nvl_nvflag */
        out.insert(out.end(), data.begin(), data.end());
        b32(out, 0); /* terminator enc_len */
        b32(out, 0); /* terminator dec_len */
        return out;
    }
    /* full packed buffer with the XDR nvs_header */
    std::vector<u8> pack() const {
        std::vector<u8> out;
        out.push_back(1); /* NV_ENCODE_XDR */
        out.push_back(1); /* host endian marker (LE host) */
        out.push_back(0);
        out.push_back(0);
        std::vector<u8> body = stream();
        out.insert(out.end(), body.begin(), body.end());
        return out;
    }
};

std::vector<u8> build_pool() {
    std::vector<u8> d(size_t(IMG), 0);

    /* ---- the objdir ZAP: "root_dataset" -> dsl_dir object 2 ----
     * MOS object 1 is the object directory; its dnode type is
     * DMU_OT_OBJECT_DIRECTORY = 1 (dmu.h) */
    {
        auto z = make_microzap({{"root_dataset", 2}});
        memcpy(d.data() + B_OBJDIR_ZAP, z.data(), z.size());
    }

    /* ---- MOS dnode array block: objects 1 (objdir zap) and 2 (dsl dir) ---- */
    {
        std::vector<u8> blk(4096, 0);
        /* object 1 at slot 1: objdir ZAP dnode (type 1: OBJECT_DIRECTORY) */
        {
            u8 dn[512];
            make_dnode(dn, 1 /* OBJECT_DIRECTORY */, 0, 1 /* 512B */, 1, 1, 0);
            make_bp(dn + 64, B_OBJDIR_ZAP, 512, 512, 0);
            memcpy(blk.data() + 512, dn, 512);
        }
        /* object 2 at slot 2: dsl_dir dnode (dsl_dir_phys: head_dataset_obj
         * u64@8, child_dir_zapobj u64@24 - dsl_dir.h) */
        {
            u8 dn[512];
            make_dnode(dn, 12 /* DSL_DIR */, 12, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            put64(bonus + 8, 4);  /* head_dataset_obj */
            put64(bonus + 24, 3); /* child_dir_zapobj */
            memcpy(blk.data() + 2 * 512, dn, 512);
        }
        /* object 3 at slot 3: child-dir ZAP ("sub" -> dsl dir 5); its
         * blkptr is patched after the array lands in the image */
        {
            u8 dn[512];
            make_dnode(dn, 13 /* DSL_DIR_CHILD_MAP */, 0, 1, 1, 1, 0);
            memcpy(blk.data() + 3 * 512, dn, 512);
        }
        memcpy(d.data() + B_MOS_DNODES, blk.data(), blk.size());
        /* child zap block right after */
        auto cz = make_microzap({{"sub", 5}});
        memcpy(d.data() + B_MOS_DNODES + 4096, cz.data(), cz.size());
        /* fix object 3's blkptr to the child zap */
        {
            u8* dn = d.data() + B_MOS_DNODES + 3 * 512;
            make_bp(dn + 64, B_MOS_DNODES + 4096, 512, 512, 0);
        }
        /* object 4 at slot 4: head dataset dnode (bonus bp@128 -> ZPL) */
        {
            u8* dn = d.data() + B_MOS_DNODES + 4 * 512;
            make_dnode(dn, 16 /* DSL_DATASET */, 16, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            u8 bp[128];
            make_bp(bp, B_ZPL, 4096, 4096, 0); /* ZPL objset block */
            memcpy(bonus + 128, bp, 128);
        }
        /* object 5 at slot 5: nested dsl dir (no children, head 6) */
        {
            u8* dn = d.data() + B_MOS_DNODES + 5 * 512;
            make_dnode(dn, 12, 12, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            put64(bonus + 8, 6);  /* head_dataset_obj */
            put64(bonus + 24, 0); /* child_dir_zapobj */
        }
        /* object 6 at slot 6: nested head dataset (bp -> bad, skip) */
        {
            u8* dn = d.data() + B_MOS_DNODES + 6 * 512;
            make_dnode(dn, 16, 16, 1, 1, 1, 256);
            /* bp zeros: load fails -> dataset skipped cleanly */
        }
    }

    /* ---- MOS objset_phys at B_MOS: meta dnode -> the array block ---- */
    {
        std::vector<u8> os(2048, 0);
        u8* meta = os.data();
        make_dnode(meta, 10 /* DNODE */, 0, 8 /* 4096B */, 1, 1, 0);
        make_bp(meta + 64, B_MOS_DNODES, 4096, 4096, 0);
        /* objset fields at 512+ (sync funcs etc.) left zero */
        memcpy(d.data() + B_MOS, os.data(), os.size());
    }

    /* ---- ZPL master ZAP: ROOT -> 2 ---- */
    {
        auto z = make_microzap({{"ROOT", 2}});
        memcpy(d.data() + B_MASTER_ZAP, z.data(), z.size());
    }

    /* ---- ZPL root dir ZAP (micro): entries ---- */
    {
        auto z = make_microzap({{"hello.txt", 10},
                                {"bigdir", 11},
                                {"zlefile", 12},
                                {"embedded.txt", 13},
                                {"legacy.txt", 14}});
        memcpy(d.data() + B_ROOTDIR_ZAP, z.data(), z.size());
    }

    /* ---- fat-zap directory (B_BIGDIR_ZAP header + leaf at +512).
     * The bigdir dnode has TWO blkptrs: block 0 = fzap header, block 1 =
     * leaf (single-level dnodes index L0 blocks directly; the embedded
     * ptrtbl stores the OBJECT-RELATIVE leaf block number = 1). ---- */
    {
        auto leaf = make_fzap_leaf({{"alpha.bin", 20}, {"beta.bin", 21}});
        memcpy(d.data() + B_BIGDIR_ZAP + 512, leaf.data(), leaf.size());
        auto hdr = make_fzap_header(1); /* leaf block number (object-relative) */
        memcpy(d.data() + B_BIGDIR_ZAP, hdr.data(), hdr.size());
    }

    /* ---- file data blocks ---- */
    memcpy(d.data() + B_FILE_DATA, "Hello, ZFS world!", 18);
    /* zle-compressed block: 4 zero sectors compress to run bytes */
    {
        std::vector<u8> zle_out;
        /* 4096 zero bytes -> runs of 256 zeros: 0xFF byte = 247+9 zeros?
         * zle: run byte b -> (1+b) - 9 zeros when 1+b > 9. b=255 -> 256-9
         * = 247 zeros. 4096 = 247*16 + 144 -> 16 x 255 then 152 (153-9) */
        for (int i = 0; i < 16; ++i)
            zle_out.push_back(255);
        zle_out.push_back(152); /* 153 - 9 = 144 zeros; total 4096 */
        /* trailing pad to 512 so psize is sector-aligned */
        zle_out.resize(512, 0);
        memcpy(d.data() + B_ZLE_DATA, zle_out.data(), zle_out.size());
    }

    /* ---- ZPL dnode array (16K block): objects 1..2, 10..14, 20..21 ---- */
    {
        std::vector<u8> blk(16384, 0);
        /* object 1: master node ZAP (ROOT -> 2) */
        {
            u8* dn = blk.data() + 1 * 512;
            make_dnode(dn, 21 /* MASTER_NODE */, 0, 1, 1, 1, 0);
            make_bp(dn + 64, B_MASTER_ZAP, 512, 512, 0);
        }
        /* object 2: root directory (SA bonus; bonustype SA=44) */
        {
            u8* dn = blk.data() + 2 * 512;
            SaAttrs a;
            a.mode = 0040755; /* octal: S_IFDIR|rwxr-xr-x */
            a.parent = 2;
            a.links = 6;
            a.size = 512;
            auto bonus = make_sa_bonus(a);
            make_dnode(dn, 20, 44 /* SA */, 1, 1, 1, 0);
            /* root dir's blkptr 0 points at the root-dir ZAP... but the
             * ZAP is the object's data - ZFS stores it via the dnode's
             * blkptrs (object 2's own data). read_zap reads blkptr 0. */
            make_bp(dn + 64, B_ROOTDIR_ZAP, 512, 512, 0);
            const u32 blen = u32(bonus.size());
            put16(dn + 10, u16(blen));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* object 10: hello.txt - SA bonus + one L0 blkptr */
        {
            u8* dn = blk.data() + 10 * 512;
            SaAttrs a;
            a.mode = 0100644; /* octal: S_IFREG|rw-r--r-- */
            a.parent = 2;
            a.links = 1;
            a.size = 18;
            a.mtime = a.ctime = a.crtime = 1700000000;
            auto bonus = make_sa_bonus(a);
            make_dnode(dn, 19 /* PLAIN_FILE_CONTENTS */, 44, 1, 1, 1, 0);
            make_bp(dn + 64, B_FILE_DATA, 512, 512, 0);
            const u32 blen = u32(bonus.size());
            put16(dn + 10, u16(blen));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* object 11: bigdir - fat-zap directory (SA bonus, type dir) */
        {
            u8* dn = blk.data() + 11 * 512;
            SaAttrs a;
            a.mode = 0040755; /* octal: S_IFDIR|rwxr-xr-x */
            a.parent = 2;
            a.links = 3;
            a.size = 512;
            auto bonus = make_sa_bonus(a);
            make_dnode(dn, 20, 44, 1, 2, 1, 0);                      /* TWO blkptrs: hdr + leaf */
            make_bp(dn + 64, B_BIGDIR_ZAP, 512, 512, 0);             /* block 0 */
            make_bp(dn + 64 + 128, B_BIGDIR_ZAP + 512, 512, 512, 0); /* block 1 */
            const u32 blen = u32(bonus.size());
            put16(dn + 10, u16(blen));
            memcpy(dn + 64 + 2 * 128, bonus.data(), bonus.size()); /* bonus after 2 bps */
        }
        /* object 12: zlefile - compressed extent (codec zle) */
        {
            u8* dn = blk.data() + 12 * 512;
            SaAttrs a;
            a.mode = 0100644; /* octal: S_IFREG|rw-r--r-- */
            a.parent = 2;
            a.links = 1;
            a.size = 4096;
            auto bonus = make_sa_bonus(a);
            make_dnode(dn, 19, 44, 8, 1, 1, 0);
            make_bp(dn + 64, B_ZLE_DATA, 512, 4096, 14 /* ZLE */);
            put16(dn + 10, u16(bonus.size()));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* object 13: embedded.txt - embedded block pointer (comp off,
         * psize == lsize <= 112) */
        {
            u8* dn = blk.data() + 13 * 512;
            SaAttrs a;
            a.mode = 0100644; /* octal: S_IFREG|rw-r--r-- */
            a.parent = 2;
            a.links = 1;
            a.size = 26;
            auto bonus = make_sa_bonus(a);
            make_dnode(dn, 19, 44, 1, 1, 1, 0);
            const char* payload = "embedded payload bytes";
            const u32 psz = 22;
            /* embedded bp: prop = (lsize-1) | ((psize-1)<<25) | etype<<40,
             * E bit 39 set; payload in words except 6 and 10 */
            u8 bp[128] = {0};
            const u64 prop = (26 - 1) | (u64(psz - 1) << 25) | (u64(0) << 40) | (1ull << 39);
            put64(bp + 48, prop);
            u32 done = 0;
            for (u32 word = 0; word < 16 && done < psz; ++word) {
                if (word == 6 || word == 10)
                    continue;
                u64 w = 0;
                for (u32 k = 0; k < 8 && done + k < psz; ++k)
                    w |= u64(u8(payload[done + k])) << (k * 8);
                put64(bp + u64(word) * 8, w);
                done += 8;
            }
            memcpy(dn + 64, bp, 128);
            put16(dn + 10, u16(bonus.size()));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* object 14: legacy.txt - legacy znode_phys bonus */
        {
            u8* dn = blk.data() + 14 * 512;
            SaAttrs a;
            a.mode = 0100644; /* octal: S_IFREG|rw-r--r-- */
            a.parent = 2;
            a.links = 1;
            a.size = 18;
            a.mtime = 1699999999;
            auto bonus = make_znode_bonus(a);
            make_dnode(dn, 19, 17 /* ZNODE */, 1, 1, 1, 0);
            make_bp(dn + 64, B_FILE_DATA, 512, 512, 0);
            put16(dn + 10, u16(bonus.size()));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* objects 20/21: files inside bigdir (SA bonuses, extents) */
        for (u32 obj = 20; obj <= 21; ++obj) {
            u8* dn = blk.data() + obj * 512;
            SaAttrs a;
            a.mode = 0100644; /* octal: S_IFREG|rw-r--r-- */
            a.parent = 11;
            a.links = 1;
            a.size = 18;
            auto bonus = make_sa_bonus(a);
            make_dnode(dn, 19, 44, 1, 1, 1, 0);
            make_bp(dn + 64, B_FILE_DATA, 512, 512, 0);
            put16(dn + 10, u16(bonus.size()));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        memcpy(d.data() + B_ZPL_DNODES, blk.data(), blk.size());
    }

    /* ---- ZPL objset_phys at B_ZPL: meta dnode -> array block ---- */
    {
        std::vector<u8> os(2048, 0);
        u8* meta = os.data();
        make_dnode(meta, 10, 0, 32 /* 16K blocks */, 1, 1, 0);
        make_bp(meta + 64, B_ZPL_DNODES, 16384, 16384, 0);
        memcpy(d.data() + B_ZPL, os.data(), os.size());
    }

    /* ---- labels (all 4 at OpenZFS positions); config nvlist is XDR
     * (vdev_label.c packs NV_ENCODE_XDR) ---- */
    std::vector<u8> nv;
    {
        NvBuf vt; /* vdev_tree nested nvlist */
        vt.str_pair("type", "disk");
        vt.u64_pair("ashift", 9);
        vt.u64_pair("asize", IMG);
        vt.u64_pair("offset", 0);
        vt.u64_pair("guid", 0x1234);
        NvBuf top;
        top.str_pair("name", "tank");
        top.u64_pair("version", 5000); /* pair AFTER vdev_tree must parse */
        top.nvlist_pair("vdev_tree", vt.stream());
        nv = top.pack();
    }

    for (int li = 0; li < 4; ++li) {
        const u64 base = li < 2 ? u64(li) * (256ull << 10) : IMG - (4 - li) * (256ull << 10);
        u8* label = d.data() + base;
        /* config nvlist at label+16K (vdev_phys start) */
        memcpy(label + (16ull << 10), nv.data(), nv.size());
        /* uberblock ring at label+128K: slot 0 = ours (txg 77) */
        u8* ub = label + (128ull << 10);
        put64(ub + 0, 0x00BAB10C); /* magic */
        put64(ub + 8, 5000);       /* version */
        put64(ub + 16, TXG);       /* txg */
        put64(ub + 32, 1700000000);
        u8 bp[128];
        make_bp(bp, B_MOS, 2048, 2048, 0);
        memcpy(ub + 40, bp, 128); /* rootbp */
    }
    return d;
}

TEST(Zfs, EndToEndWalk) {
    auto img = build_pool();
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    u32 rc = scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(u64(TXG), st.txg);

    /* collect paths */
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

    /* dataset root + entries incl. the fat-zap directory */
    EXPECT_TRUE(paths.count("tank"));
    EXPECT_TRUE(paths.count("tank/hello.txt"));
    EXPECT_TRUE(paths.count("tank/bigdir"));
    EXPECT_TRUE(paths.count("tank/bigdir/alpha.bin"));
    EXPECT_TRUE(paths.count("tank/bigdir/beta.bin"));
    EXPECT_TRUE(paths.count("tank/zlefile"));
    EXPECT_TRUE(paths.count("tank/embedded.txt"));
    EXPECT_TRUE(paths.count("tank/legacy.txt"));

    /* verify props: hello.txt size + extent (raw) */
    for (tree::FileNode* c = ra->first_child(); c; c = c->next_sibling())
        for (tree::FileNode* f = c->first_child(); f; f = f->next_sibling()) {
            if (!f->props())
                continue;
            if (f->name_utf8() == "hello.txt") {
                EXPECT_EQ(u64(18), f->props()->logical_size);
                ASSERT_EQ(1u, f->props()->extents.size());
                EXPECT_EQ(B_FILE_DATA, f->props()->extents[0].offset);
                EXPECT_EQ(0u, f->props()->extents[0].codec);
            }
            if (f->name_utf8() == "zlefile") {
                /* compressed extent carries codec + psize */
                ASSERT_EQ(1u, f->props()->extents.size());
                EXPECT_EQ(u32(compress::kZle), f->props()->extents[0].codec);
                EXPECT_EQ(512u, f->props()->extents[0].psize);
                EXPECT_EQ(4096u, f->props()->extents[0].length);
            }
            if (f->name_utf8() == "embedded.txt") {
                /* embedded payload becomes resident content */
                EXPECT_TRUE(f->props()->extents.empty());
                ASSERT_FALSE(f->props()->resident_data.empty());
                EXPECT_EQ(0,
                          memcmp("embedded payload bytes", f->props()->resident_data.data(), 22));
            }
            if (f->name_utf8() == "legacy.txt") {
                /* legacy znode_phys bonus parsed */
                EXPECT_EQ(u64(18), f->props()->logical_size);
                EXPECT_EQ(u64(1699999999ull + 11644473600ull) * 10000000ull,
                          f->props()->times.modified);
            }
        }
}

TEST(Zfs, LabelPositionsRecoveredFromTail) {
    /* smash the two head labels; the pool must still be found through the
     * tail labels (L2@end-512K, L3=end-256K) - the old quarter-point
     * formula never probed them. */
    auto img = build_pool();
    memset(img.data(), 0xFF, 2 * (256ull << 10));
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(u64(TXG), st.txg);
}

TEST(Zfs, NewestTxgAcrossLabelsWins) {
    auto img = build_pool();
    /* write a newer uberblock into label 1 (base 256K) slot 1 */
    u8* ub = img.data() + (256ull << 10) + (128ull << 10) + 1024;
    put64(ub + 0, 0x00BAB10C);
    put64(ub + 16, TXG + 5);
    u8 bp[128];
    make_bp(bp, B_MOS, 2048, 2048, 0);
    memcpy(ub + 40, bp, 128);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(u64(TXG + 5), st.txg);
}

TEST(Zfs, BadMagicRejected) {
    std::vector<u8> d(1ull << 20, 0);
    MemDevice dev(d);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC,
              scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
}

TEST(Zfs, ZleRoundTrip) {
    /* the fixture's zle block must decode to 4096 zeros via the shared
     * decompressor (writer-time decode path) */
    std::vector<u8> enc;
    for (int i = 0; i < 16; ++i)
        enc.push_back(255);
    enc.push_back(152);
    enc.resize(512, 0);
    std::vector<u8> out(4096, 0xAB);
    ASSERT_TRUE(compress::decompress_block(compress::kZle, enc.data(), 512, out.data(), 4096));
    for (u8 b : out)
        EXPECT_EQ(0u, b);
}

TEST(Zfs, CodecMapping) {
    EXPECT_EQ(u32(compress::kRaw), compress::codec_from_zfs(0));
    EXPECT_EQ(u32(compress::kRaw), compress::codec_from_zfs(2));
    EXPECT_EQ(u32(compress::kLzjb), compress::codec_from_zfs(3));
    EXPECT_EQ(u32(compress::kRaw), compress::codec_from_zfs(4)); /* empty */
    EXPECT_EQ(u32(compress::kGzip), compress::codec_from_zfs(5));
    EXPECT_EQ(u32(compress::kGzip), compress::codec_from_zfs(13));
    EXPECT_EQ(u32(compress::kZle), compress::codec_from_zfs(14));
    EXPECT_EQ(u32(compress::kLz4), compress::codec_from_zfs(15));
    EXPECT_EQ(u32(compress::kZstd), compress::codec_from_zfs(16));
}

} // namespace
} // namespace zfs
} // namespace cr
