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
#include <functional>
#include <map>
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
u64 get64(const u8* p) {
    return u64(u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24)) |
           (u64(u32(p[4]) | (u32(p[5]) << 8) | (u32(p[6]) << 16) | (u32(p[7]) << 24)) << 32);
}

/* ---- blkptr construction (OpenZFS spa.h packing) ----
 * M-019: every fixture bp now carries a REAL fletcher4 checksum (type
 * 7) stamped over the raw on-disk bytes - real pools are
 * checksum-carrying, and the engine's verify gate (the og's
 * ZfsSearchDLL :18076 `(zc==7)||(zc==0)` + :18117 32-byte memcmp)
 * rejects bps whose stored checksum does not match their data. The
 * fixture's fletcher4 below is the STRAIGHTFORWARD textbook loop - an
 * encoding path INDEPENDENT of the engine's interleaved port (two
 * encodings of ground truth must agree; the paired tests fail if
 * either side drifts). */
void fletcher4_ref(const u8* p, size_t len, u64 out[4]) {
    u64 a = 0, b = 0, c = 0, d = 0;
    for (size_t i = 0; i + 4 <= len; i += 4) {
        u32 w;
        memcpy(&w, p + i, 4);
        a += w;
        b += a;
        c += b;
        d += c;
    }
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d;
}
void stamp_fletcher4(u8* bp, const u8* raw_data, size_t psize) {
    u64 ck[4];
    fletcher4_ref(raw_data, psize, ck);
    put64(bp + 96, ck[0]);
    put64(bp + 96 + 8, ck[1]);
    put64(bp + 96 + 16, ck[2]);
    put64(bp + 96 + 24, ck[3]);
    /* checksum type 7 (fletcher4) in prop bits 40-47 */
    const u64 prop = (get64(bp + 48) & ~(0xFFull << 40)) | (7ull << 40);
    put64(bp + 48, prop);
}
void make_bp(u8* bp, u64 offset, u64 psize, u64 lsize, u32 compress,
             const std::vector<u8>* img = nullptr) {
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
    if (img)
        stamp_fletcher4(bp, img->data() + offset, size_t(psize));
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

/* ---- M-038: SA registry fixtures ----
 * Real pools carry the SA registry chain (master node "SA_ATTRS" ->
 * {REGISTRY, LAYOUTS}); layout entries are u16 attr-number arrays whose
 * value can exceed 8 bytes (layout 1 = 14 slots = 28 bytes -> fat ZAP
 * with intlen-2 value arrays). */
constexpr u64 B_SA_MASTER = 0xA8000;    /* object 23: REGISTRY/LAYOUTS map */
constexpr u64 B_SA_REG = 0xA8200;       /* object 24: 2K fzap (hdr + leaf) */
constexpr u64 B_SA_LAYOUTS = 0xA9200;   /* object 25: header + leaf @+512 */
constexpr u64 B_REGFILE_DATA = 0xA9800; /* object 22's data block */

std::vector<u8> make_fzap_leaf_raw(
    const std::vector<std::pair<std::string, std::vector<u8>>>& entries, u32 intlen,
    u64 block_size = FZ_BLOCK) {
    /* like make_fzap_leaf but with arbitrary-length intlen-byte value
     * arrays (the SA layouts object shape) and arbitrary block size (the
     * registry's 14 entries need a 2K leaf: 42 chunks > a 512B leaf's
     * 18) */
    std::vector<u8> leaf(size_t(block_size), 0);
    put64(leaf.data() + 0, (1ull << 63) + 0); /* ZBT_LEAF */
    put64(leaf.data() + 16, 0);               /* prefix */
    put32(leaf.data() + 24, 0x2AB1EAF);       /* ZAP_LEAF_MAGIC */
    put16(leaf.data() + 30, u16(entries.size()));
    u32 bshift = 0;
    while ((1ull << bshift) < block_size)
        ++bshift;
    const u32 hash_entries = 1u << (bshift > 5 ? bshift - 5 : 1);
    const u32 chunks_at = 48 + hash_entries * 2;
    const u32 nchunks = u32((block_size - chunks_at) / 24);
    u32 next_chunk = 0;
    for (const auto& e : entries) {
        if (next_chunk + 2 > nchunks)
            break;
        const u32 entry_chunk = next_chunk++;
        /* value array chunks (21 data bytes each, chained) */
        std::vector<u8> val = e.second;
        u32 first_value_chunk = 0xFFFFFFFF;
        u32 consumed = 0;
        u32 prev_chunk = 0xFFFFFFFF;
        while (consumed < val.size()) {
            if (next_chunk >= nchunks)
                break;
            const u32 ci = next_chunk++;
            if (first_value_chunk == 0xFFFFFFFF)
                first_value_chunk = ci;
            u8* vc = leaf.data() + chunks_at + u64(ci) * 24;
            vc[0] = 251; /* ZAP_CHUNK_ARRAY */
            const u32 take = u32(val.size()) - consumed < 21 ? u32(val.size()) - consumed : 21;
            memcpy(vc + 1, val.data() + consumed, take);
            put16(vc + 22, 0xFFFF);
            if (prev_chunk != 0xFFFFFFFF)
                put16(leaf.data() + chunks_at + u64(prev_chunk) * 24 + 22, u16(ci));
            prev_chunk = ci;
            consumed += take;
        }
        /* name array chunks */
        std::vector<u8> name(e.first.begin(), e.first.end());
        name.push_back(0);
        u32 first_name_chunk = next_chunk;
        consumed = 0;
        prev_chunk = 0xFFFFFFFF;
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
        ec[1] = u8(intlen);
        put16(ec + 4, u16(first_name_chunk));
        put16(ec + 6, u16(name.size()));
        put16(ec + 8, u16(first_value_chunk));
        put16(ec + 10, u16((val.size() + intlen - 1) / intlen)); /* value_numints */
        put32(ec + 12, 0);                                       /* cd */
        put64(ec + 16, 0);                                       /* hash */
    }
    return leaf;
}

/* layout-2 bonus: slots [ZPL_SIZE, ZPL_MODE, ZPL_PARENT, ZPL_CRTIME] in
 * registry attr numbers 6/5/7/3 - an order deliberately DIFFERENT from
 * layout 1, proving registry-driven offset resolution */
std::vector<u8> make_sa_layout2_bonus(u64 size, u64 mode, u64 parent, u64 crtime) {
    /* slot lengths come from the registry: ZPL_SIZE/MODE/PARENT are 8
     * bytes; ZPL_CRTIME is a 16-byte sec+usec PAIR (like every ZPL time
     * attribute) - the layout reserves each slot's full registered
     * length */
    std::vector<u8> b(8 + 8 + 8 + 8 + 16, 0);
    put32(b.data(), 0x2F505A); /* SA_MAGIC */
    put16(b.data() + 4, u16(2) | (u16(1) << 10)); /* layout 2, 8-byte hdr */
    put64(b.data() + 8, size);    /* slot 0: ZPL_SIZE (attr 6, 8 bytes) */
    put64(b.data() + 16, mode);   /* slot 1: ZPL_MODE (attr 5, 8 bytes) */
    put64(b.data() + 24, parent); /* slot 2: ZPL_PARENT (attr 7, 8 bytes) */
    put64(b.data() + 32, crtime); /* slot 3: ZPL_CRTIME (attr 3, 16
                                   * bytes: seconds + usec) */
    return b;
}

std::vector<u8> make_fzap_header(u64 leaf_block, u64 block_size = FZ_BLOCK) {
    std::vector<u8> hdr(size_t(block_size), 0);
    put64(hdr.data() + 0, (1ull << 63) + 1); /* ZBT_HEADER */
    put64(hdr.data() + 8, 0x2F52AB2ABull);   /* ZAP_MAGIC */
    /* embedded pointer table (zt_blk == 0): second half of the block */
    put64(hdr.data() + 16, 0); /* zt_blk = 0 -> embedded */
    put64(hdr.data() + 24, 0); /* zt_numblks */
    put64(hdr.data() + 32, 4); /* zt_shift (the reader uses blocksize/16
                                * for the embedded table) */
    put64(hdr.data() + 56, 2); /* freeblk */
    put64(hdr.data() + 64, 1); /* num_leafs */
    put64(hdr.data() + 72, 0); /* num_entries */
    /* embedded ptrtbl at block/2, entry 0 -> leaf block number */
    put64(hdr.data() + block_size / 2, leaf_block);
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

std::vector<u8> build_pool(const char* vdev_type = "disk") {
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
            make_bp(dn + 64, B_OBJDIR_ZAP, 512, 512, 0, &d);
            memcpy(blk.data() + 512, dn, 512);
        }
        /* object 2 at slot 2: dsl_dir dnode (dsl_dir_phys per OpenZFS
         * dsl_dir.h: head_dataset_obj u64@8, parent_obj@16, origin_obj@24,
         * child_dir_zapobj u64@32 — A-008 kernel layout; origin left 0) */
        {
            u8 dn[512];
            make_dnode(dn, 12 /* DSL_DIR */, 12, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            put64(bonus + 8, 4);  /* head_dataset_obj */
            put64(bonus + 24, 0); /* origin_obj (NOT the child zap) */
            put64(bonus + 32, 3); /* child_dir_zapobj */
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
            make_bp(dn + 64, B_MOS_DNODES + 4096, 512, 512, 0, &d);
        }
        /* object 4 at slot 4: head dataset dnode (bonus bp@128 -> ZPL) */
        {
            u8* dn = d.data() + B_MOS_DNODES + 4 * 512;
            make_dnode(dn, 16 /* DSL_DATASET */, 16, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            u8 bp[128];
            make_bp(bp, B_ZPL, 4096, 4096, 0); /* ZPL objset block - forward ref: stamped after the ZPL block lands */
            memcpy(bonus + 128, bp, 128);
        }
        /* object 5 at slot 5: nested dsl dir (no children, head 6) */
        {
            u8* dn = d.data() + B_MOS_DNODES + 5 * 512;
            make_dnode(dn, 12, 12, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            put64(bonus + 8, 6);  /* head_dataset_obj */
            put64(bonus + 24, 0); /* origin_obj */
            put64(bonus + 32, 0); /* child_dir_zapobj */
        }
        /* object 6 at slot 6: nested head dataset (bp -> the same ZPL
         * objset block; A-008: resolvable so the child-dataset walk is
         * observable end-to-end) */
        {
            u8* dn = d.data() + B_MOS_DNODES + 6 * 512;
            make_dnode(dn, 16, 16, 1, 1, 1, 256);
            u8* bonus = dn + 64 + 128;
            u8 bp[128];
            make_bp(bp, B_ZPL, 4096, 4096, 0); /* same ZPL content as tank */
            memcpy(bonus + 128, bp, 128);
        }
    }

    /* ---- MOS objset_phys at B_MOS: meta dnode -> the array block ---- */
    {
        std::vector<u8> os(2048, 0);
        u8* meta = os.data();
        make_dnode(meta, 10 /* DNODE */, 0, 8 /* 4096B */, 1, 1, 0);
        make_bp(meta + 64, B_MOS_DNODES, 4096, 4096, 0); /* checksum stamped in the ordered pass below */
        /* objset fields at 512+ (sync funcs etc.) left zero */
        memcpy(d.data() + B_MOS, os.data(), os.size());
    }

    /* ---- ZPL master ZAP: ROOT -> 2 ---- */
    {
        auto z = make_microzap({{"ROOT", 2}, {"SA_ATTRS", 23}}); /* M-038 */
        memcpy(d.data() + B_MASTER_ZAP, z.data(), z.size());
    }

    /* ---- ZPL root dir ZAP (micro): entries ---- */
    {
        auto z = make_microzap({{"hello.txt", 10},
                                {"bigdir", 11},
                                {"zlefile", 12},
                                {"embedded.txt", 13},
                                {"legacy.txt", 14},
                                {"registryfile", 22}}); /* M-038 */
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

    /* ---- M-038: SA registry chain ----
     * object 66 (SA master): REGISTRY -> 67, LAYOUTS -> 68
     * object 67 (registry): ZPL_* name -> {attr number | length<<16}
     *   (the zfs_sa.c registration order: atime mtime ctime crtime gen
     *   mode size parent links xattr rdev flags uid gid = numbers 0-13)
     * object 68 (layouts): "1" -> the 14 default slots; "2" ->
     *   [size(6), mode(5), parent(7), crtime(3)] - a different order */
    {
        auto m = make_microzap({{"REGISTRY", 24}, {"LAYOUTS", 25}});
        memcpy(d.data() + B_SA_MASTER, m.data(), m.size());
        std::vector<std::pair<std::string, u64>> regs;
        const char* names[14] = {"ZPL_ATIME", "ZPL_MTIME",   "ZPL_CTIME",  "ZPL_CRTIME",
                                 "ZPL_GEN",   "ZPL_MODE",    "ZPL_SIZE",   "ZPL_PARENT",
                                 "ZPL_LINKS", "ZPL_XATTR",   "ZPL_RDEV",   "ZPL_FLAGS",
                                 "ZPL_UID",   "ZPL_GID"};
        for (int i = 0; i < 14; ++i) {
            const u64 len = i < 4 ? 16 : 8; /* times are sec+usec pairs */
            regs.push_back({names[i], u64(i) | (len << 16)});
        }
        /* the 14-entry registry needs a 2K fzap (a 512B block holds at
         * most 7 microzap entries / ~6 fzap entries) */
        std::vector<std::pair<std::string, std::vector<u8>>> reg_raw;
        for (const auto& e : regs) {
            std::vector<u8> v(8, 0);
            put64(v.data(), e.second);
            reg_raw.push_back({e.first, v});
        }
        auto rh = make_fzap_header(1, 2048);
        memcpy(d.data() + B_SA_REG, rh.data(), rh.size());
        auto rl = make_fzap_leaf_raw(reg_raw, 8, 2048);
        memcpy(d.data() + B_SA_REG + 2048, rl.data(), rl.size());
        auto lh = make_fzap_header(1);
        memcpy(d.data() + B_SA_LAYOUTS, lh.data(), lh.size());
        std::vector<u8> lay1(28, 0);
        for (int i = 0; i < 14; ++i) {
            lay1[size_t(i) * 2] = u8(i); /* u16 LE slot i */
        }
        std::vector<u8> lay2 = {6, 0, 5, 0, 7, 0, 3, 0}; /* u16 LE slots */
        auto lf = make_fzap_leaf_raw({{"1", lay1}, {"2", lay2}}, 2);
        memcpy(d.data() + B_SA_LAYOUTS + 512, lf.data(), lf.size());
        /* object 22's data block (a registry-layout-2 file) */
        memcpy(d.data() + B_REGFILE_DATA, "Registry layout-2 file", 22);
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
            make_bp(dn + 64, B_MASTER_ZAP, 512, 512, 0, &d);
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
            make_bp(dn + 64, B_ROOTDIR_ZAP, 512, 512, 0, &d);
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
            make_bp(dn + 64, B_FILE_DATA, 512, 512, 0, &d);
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
            make_bp(dn + 64, B_BIGDIR_ZAP, 512, 512, 0, &d);         /* block 0 */
            make_bp(dn + 64 + 128, B_BIGDIR_ZAP + 512, 512, 512, 0, &d); /* block 1 */
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
            make_bp(dn + 64, B_ZLE_DATA, 512, 4096, 14 /* ZLE */, &d);
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
            make_bp(dn + 64, B_FILE_DATA, 512, 512, 0, &d);
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
            make_bp(dn + 64, B_FILE_DATA, 512, 512, 0, &d);
            put16(dn + 10, u16(bonus.size()));
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* object 22 (M-038): a file whose SA bonus uses registered
         * layout 2 - [size, mode, parent, crtime], decoded only through
         * the registry (no implicit default exists for it) */
        {
            u8* dn = blk.data() + 22 * 512;
            auto bonus = make_sa_layout2_bonus(4096, 0100644, 2, 1700000002);
            make_dnode(dn, 19 /* PLAIN_FILE */, 44 /* SA */, 1, 1, 1, u32(bonus.size()));
            make_bp(dn + 64, B_REGFILE_DATA, 512, 512, 0, &d);
            memcpy(dn + 64 + 128, bonus.data(), bonus.size());
        }
        /* objects 23-25 (M-038): the SA registry chain (the 16K dnode
         * array holds 32 slots; 23-25 are free) */
        {
            u8* dn = blk.data() + 23 * 512;
            make_dnode(dn, 45 /* SA_MASTER_NODE */, 0, 1, 1, 1, 0);
            make_bp(dn + 64, B_SA_MASTER, 512, 512, 0, &d);
        }
        {
            u8* dn = blk.data() + 24 * 512;
            make_dnode(dn, 46 /* SA_ATTR_REGISTRATION */, 0, 4 /* 2K blocks */, 2, 1, 0);
            make_bp(dn + 64, B_SA_REG, 2048, 2048, 0, &d);             /* block 0 */
            make_bp(dn + 64 + 128, B_SA_REG + 2048, 2048, 2048, 0, &d); /* block 1 */
        }
        {
            u8* dn = blk.data() + 25 * 512;
            make_dnode(dn, 47 /* SA_ATTR_LAYOUTS */, 0, 1, 2, 1, 0); /* 2 bps: hdr + leaf */
            make_bp(dn + 64, B_SA_LAYOUTS, 512, 512, 0, &d);         /* block 0 */
            make_bp(dn + 64 + 128, B_SA_LAYOUTS + 512, 512, 512, 0, &d); /* block 1 */
        }
        memcpy(d.data() + B_ZPL_DNODES, blk.data(), blk.size());
    }

    /* ---- ZPL objset_phys at B_ZPL: meta dnode -> array block ---- */
    {
        std::vector<u8> os(2048, 0);
        u8* meta = os.data();
        make_dnode(meta, 10, 0, 32 /* 16K blocks */, 1, 1, 0);
        make_bp(meta + 64, B_ZPL_DNODES, 16384, 16384, 0, &d);
        memcpy(d.data() + B_ZPL, os.data(), os.size());
    }

    /* ---- M-019: ordered checksum stamping ----
     * The objset blocks reference each other's arrays, so their bps are
     * stamped in dependency order now that every block has landed:
     *   1. MOS object 4's head-dataset bp (in the MOS dnode array;
     *      dnode at B_MOS_DNODES+4*512, bonus at +64+128, bp at
     *      bonus+128) over the final ZPL objset block at B_ZPL (its
     *      meta bp was already stamped when that block was built).
     *   2. The MOS objset's meta dnode bp (at B_MOS+64) over the final
     *      B_MOS_DNODES content - AFTER (1), since (1) mutates that
     *      block. The uberblock rootbp (stamped in the label loop
     *      below) reads B_MOS after this, closing the chain. */
    {
        stamp_fletcher4(d.data() + B_MOS_DNODES + 4 * 512 + 64 + 128 + 128, d.data() + B_ZPL,
                        4096);
        /* A-008: object 6 (the nested "sub" head dataset) also points at
         * the ZPL objset block; stamped before the enclosing array's own
         * bp so the chain closes */
        stamp_fletcher4(d.data() + B_MOS_DNODES + 6 * 512 + 64 + 128 + 128, d.data() + B_ZPL,
                        4096);
        stamp_fletcher4(d.data() + B_MOS + 64, d.data() + B_MOS_DNODES, 4096);
    }

    /* ---- labels (all 4 at OpenZFS positions); config nvlist is XDR
     * (vdev_label.c packs NV_ENCODE_XDR) ---- */
    std::vector<u8> nv;
    {
        NvBuf vt; /* vdev_tree nested nvlist */
        vt.str_pair("type", vdev_type);
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
        make_bp(bp, B_MOS, 2048, 2048, 0, &d);
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
    /* A-008: the child dataset (dsl_dir 5, head 6) enumerated through the
     * kernel-offset child zap — same ZPL content under "sub" */
    EXPECT_TRUE(paths.count("sub"));
    EXPECT_TRUE(paths.count("sub/hello.txt"));

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

/* A-008: the child_dir_zapobj offset. OpenZFS dsl_dir.h puts
 * dd_child_dir_zapobj at bonus+32 (dd_origin_obj at +24); the engine
 * read +24 — on kernel-layout pools a normal dsl_dir's origin is 0, so
 * NO child dataset ever enumerated (every zfs create under the pool
 * silently missing from results); on clones the non-zero origin was
 * followed as if it were the child zap. The fixture stamps the kernel
 * layout only — the pre-fix engine fails both tests below. */
TEST(Zfs, ChildDirZapReadsKernelOffset32) {
    auto img = build_pool();
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
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
    EXPECT_TRUE(paths.count("sub")) << "child dataset missing - child zap read from wrong slot";
    EXPECT_TRUE(paths.count("sub/hello.txt"));
}

TEST(Zfs, OriginObjNotTreatedAsChildZap) {
    /* the root dsl_dir carries a NON-ZERO origin (clone shape): origin@24
     * points at the objdir zap (object 1, entries {"root_dataset": 2}).
     * The child set must come from +32 ONLY — the pre-fix engine treated
     * the origin as the child zap and enumerated "root_dataset" as a
     * dataset (recursing into the root dir itself). */
    auto img = build_pool();
    put64(img.data() + B_MOS_DNODES + 2 * 512 + 64 + 128 + 24, 1); /* origin_obj -> objdir zap */
    /* re-stamp the enclosing-block chain (MOS meta bp, then uberblocks) */
    stamp_fletcher4(img.data() + B_MOS + 64, img.data() + B_MOS_DNODES, 4096);
    for (int li = 0; li < 4; ++li) {
        const u64 base = li < 2 ? u64(li) * (256ull << 10) : IMG - (4 - li) * (256ull << 10);
        stamp_fletcher4(img.data() + base + (128ull << 10) + 40, img.data() + B_MOS, 2048);
    }
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
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
    EXPECT_TRUE(paths.count("sub")) << "kernel-offset child zap lost";
    EXPECT_TRUE(paths.count("sub/hello.txt"));
    EXPECT_FALSE(paths.count("root_dataset"))
        << "origin_obj followed as a child zap - ghost dataset from the wrong slot";
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
    make_bp(bp, B_MOS, 2048, 2048, 0, &img);
    memcpy(ub + 40, bp, 128);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(u64(TXG + 5), st.txg);
}

/* M-019: re-stamp the ZPL dnode array's enclosing-block checksum after
 * any test mutates a bp INSIDE the array (a real pool writes the whole
 * array block + its checksum together). The checksum chain is
 *   uberblock rootbp -> B_MOS -> (MOS meta bp) -> B_MOS_DNODES
 *     -> (obj-4 bp) -> B_ZPL -> (ZPL meta bp) -> B_ZPL_DNODES
 * so every re-stamp mutates its parent block and the whole chain up to
 * the four label uberblocks must be re-stamped in order. */
void restamp_zpl_array(std::vector<u8>& img) {
    stamp_fletcher4(img.data() + B_ZPL + 64, img.data() + B_ZPL_DNODES, 16384);
    stamp_fletcher4(img.data() + B_MOS_DNODES + 4 * 512 + 64 + 128 + 128, img.data() + B_ZPL,
                    4096);
    /* A-008: object 6 (the "sub" head dataset) shares the ZPL block */
    stamp_fletcher4(img.data() + B_MOS_DNODES + 6 * 512 + 64 + 128 + 128, img.data() + B_ZPL,
                    4096);
    stamp_fletcher4(img.data() + B_MOS + 64, img.data() + B_MOS_DNODES, 4096);
    for (int li = 0; li < 4; ++li) {
        const u64 base = li < 2 ? u64(li) * (256ull << 10) : IMG - (4 - li) * (256ull << 10);
        stamp_fletcher4(img.data() + base + (128ull << 10) + 40, img.data() + B_MOS, 2048);
    }
}

/* ---- S-034: the second ring state (txg-history fixture) ----------------
 * Builds a CoW-correct NEWER state (new_txg) whose tree no longer contains
 * hello.txt: the root-dir ZAP is rebuilt without its entry and object 10
 * is freed in the copied dnode array (the real deletion shape - the
 * parent ZAP entry removal plus the dnode free). Everything the state
 * did not change stays shared with the old state's blocks (real ZFS CoW
 * shares unchanged blocks). A new MOS chain lands at the B_*2 offsets
 * and a second uberblock is stamped into label 0's ring slot 1. */
constexpr u64 B_ROOTDIR_ZAP2 = 0xB0000;
constexpr u64 B_ZPL_DNODES2 = 0xB2000;  /* 16K */
constexpr u64 B_ZPL2 = 0xB6000;         /* 4K */
constexpr u64 B_MOS_DNODES2 = 0xBA000;  /* 4K */
constexpr u64 B_MOS2 = 0xBC000;         /* 2K */

void stamp_second_state(std::vector<u8>& img, u64 new_txg, bool without_hello) {
    /* 1. the new root-dir ZAP (without hello.txt when requested) */
    std::vector<std::pair<std::string, u64>> entries = {
        {"bigdir", 11}, {"zlefile", 12}, {"embedded.txt", 13}, {"legacy.txt", 14},
        {"registryfile", 22}};
    if (!without_hello)
        entries.insert(entries.begin(), {"hello.txt", 10});
    auto z = make_microzap(entries);
    memcpy(img.data() + B_ROOTDIR_ZAP2, z.data(), z.size());

    /* 2. the copied ZPL dnode array: object 2's bp -> the new ZAP;
     * object 10 freed (dnode zeroed) when the state dropped hello.txt */
    memcpy(img.data() + B_ZPL_DNODES2, img.data() + B_ZPL_DNODES, 16384);
    make_bp(img.data() + B_ZPL_DNODES2 + 2 * 512 + 64, B_ROOTDIR_ZAP2, 512, 512, 0, &img);
    if (without_hello)
        memset(img.data() + B_ZPL_DNODES2 + 10 * 512, 0, 512);

    /* 3. the copied ZPL objset: meta bp -> the new dnode array */
    memcpy(img.data() + B_ZPL2, img.data() + B_ZPL, 4096);
    make_bp(img.data() + B_ZPL2 + 64, B_ZPL_DNODES2, 16384, 16384, 0, &img);

    /* 4. the copied MOS dnode array: head datasets 4 and 6 -> the new
     * ZPL objset (both datasets moved forward, real CoW) */
    memcpy(img.data() + B_MOS_DNODES2, img.data() + B_MOS_DNODES, 4096);
    make_bp(img.data() + B_MOS_DNODES2 + 4 * 512 + 64 + 128 + 128, B_ZPL2, 4096, 4096, 0, &img);
    make_bp(img.data() + B_MOS_DNODES2 + 6 * 512 + 64 + 128 + 128, B_ZPL2, 4096, 4096, 0, &img);

    /* 5. the copied MOS objset: meta bp -> the new MOS dnode array */
    memcpy(img.data() + B_MOS2, img.data() + B_MOS, 2048);
    make_bp(img.data() + B_MOS2 + 64, B_MOS_DNODES2, 4096, 4096, 0, &img);

    /* 6. the second uberblock: label 0 ring slot 1, new txg, rootbp ->
     * the new MOS (checksum stamped over its final bytes) */
    u8* ub = img.data() + (128ull << 10) + 1024;
    put64(ub + 0, 0x00BAB10C);
    put64(ub + 8, 5000);
    put64(ub + 16, new_txg);
    put64(ub + 32, 1700000001);
    make_bp(ub + 40, B_MOS2, 2048, 2048, 0, &img);
}

/* S-034: collect (path -> count) over a root - duplicate detection needs
 * counts, not set membership */
void collect_paths_counted(tree::FileNode* n, const std::string& p,
                           std::map<std::string, int>& out) {
    for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        const std::string nm = c->name_utf8();
        const std::string full = p.empty() ? nm : p + "/" + nm;
        ++out[full];
        collect_paths_counted(c, full, out);
    }
}

TEST(Zfs, ChecksumFailDropsAndRetriesNextDva) {
    /* M-019: the og verifies fletcher4 over the raw on-disk bytes and
     * retries the NEXT DVA copy on mismatch before dropping the block
     * (ZfsSearchDLL :18076 gate `(zc==7)||(zc==0)` + :18129
     * FUN_180014360(..., index+1)). The old engine trusted every read:
     * a torn first copy entered results as if valid - here DVA0 serves
     * a well-formed DECOY microzap, so the tree shows decoy.txt and
     * hides every real entry (the exact silent-garbage class). */
    auto img = build_pool();
    /* a well-formed decoy microzap in the free gap after the child zap
     * (B_MOS_DNODES + 4096 is the child zap; +512 past it is unused) */
    const u64 B_DECOY = B_MOS_DNODES + 4096 + 512;
    auto z = make_microzap({{"decoy.txt", 10}});
    memcpy(img.data() + B_DECOY, z.data(), z.size());
    /* rewrite the root-dir dnode's bp: DVA0 -> decoy, DVA1 -> the real
     * zap; stored checksum = fletcher4 of the REAL copy */
    u8* bp = img.data() + B_ZPL_DNODES + 2 * 512 + 64;
    memset(bp, 0, 128);
    put64(bp + 0, (511ull + 1) >> 9); /* DVA0: vdev 0, asize 512 */
    put64(bp + 8, B_DECOY >> 9);
    put64(bp + 16, (511ull + 1) >> 9); /* DVA1: vdev 0, asize 512 */
    put64(bp + 24, B_ROOTDIR_ZAP >> 9);
    const u64 prop = ((512 >> 9) - 1) | (((512 >> 9) - 1) << 16);
    put64(bp + 48, prop);
    put64(bp + 72, TXG);
    stamp_fletcher4(bp, img.data() + B_ROOTDIR_ZAP, 512);
    restamp_zpl_array(img); /* the bp lives inside the checksummed array */
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    std::set<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : p + "/" + nm;
            paths.insert(full);
            walk(c, full);
        }
    };
    walk(ra, "");

    /* the REAL zap content wins through DVA1; the decoy is dropped by
     * the checksum gate and counted */
    EXPECT_TRUE(paths.count("tank/hello.txt")) << "real entry missing - decoy won";
    EXPECT_FALSE(paths.count("tank/decoy.txt")) << "torn first copy entered results";
    EXPECT_GE(st.checksum_drops, u64(1));
}

TEST(Zfs, StripedPoolForeignVdevFallbackRead) {
    /* M-019: the og NEVER refuses a DVA on vdev id - the registry
     * lookup falls back to the FIRST entry (the scanned disk,
     * FUN_1800168b0 :19544-19556) and the fletcher4 gate decides
     * validity. A stripe member's block whose copy also exists on the
     * scanned disk therefore RECOVERS through the fallback read; the
     * old engine refused every foreign-vdev DVA outright (silent
     * unsupported_vdevs counter, no read attempt). */
    auto img = build_pool();
    /* the "sibling member's" copy of alpha.bin's data block, on the
     * scanned disk (what a mirror-in-stripe / copies=2 layout produces) */
    const u64 B_STRIPE_COPY = 0xA4000;
    memcpy(img.data() + B_STRIPE_COPY, "Hello, ZFS world!", 18);
    /* rewrite alpha.bin's (object 20) bp: DVA0 names vdev 1 */
    u8* bp = img.data() + B_ZPL_DNODES + 20 * 512 + 64;
    memset(bp, 0, 128);
    put64(bp + 0, (1ull << 32) | (512ull >> 9)); /* vdev 1, asize 512 */
    put64(bp + 8, B_STRIPE_COPY >> 9);
    const u64 prop = ((512 >> 9) - 1) | (((512 >> 9) - 1) << 16);
    put64(bp + 48, prop);
    put64(bp + 72, TXG);
    stamp_fletcher4(bp, img.data() + B_STRIPE_COPY, 512);
    restamp_zpl_array(img); /* the bp lives inside the checksummed array */
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    std::set<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : p + "/" + nm;
            paths.insert(full);
            walk(c, full);
        }
    };
    walk(ra, "");

    /* the foreign-vdev block is PLANNED at the fallback position (the
     * og's registry fallback reads the scanned disk); before the fix
     * the L0 walk skipped every foreign-vdev DVA, leaving alpha.bin
     * extentless (silent refusal) */
    EXPECT_TRUE(paths.count("tank/bigdir/alpha.bin")) << "foreign-vdev file dropped";
    EXPECT_TRUE(paths.count("tank/bigdir/beta.bin"));
    bool alpha_extent = false;
    for (tree::FileNode* c0 = ra->first_child(); c0; c0 = c0->next_sibling())
        for (tree::FileNode* f = c0->first_child(); f; f = f->next_sibling()) {
            if (!f->props())
                continue;
            if (f->name_utf8() == "bigdir") {
                for (tree::FileNode* g = f->first_child(); g; g = g->next_sibling()) {
                    if (g->props() && g->name_utf8() == "alpha.bin") {
                        ASSERT_EQ(1u, g->props()->extents.size())
                            << "foreign-vdev extent skipped";
                        EXPECT_EQ(B_STRIPE_COPY, g->props()->extents[0].offset);
                        alpha_extent = true;
                    }
                }
            }
        }
    EXPECT_TRUE(alpha_extent);
    EXPECT_GE(st.foreign_vdev_reads, u64(1));
    EXPECT_EQ(u64(0), st.checksum_drops); /* the fallback copy is valid */
}

TEST(Zfs, MirrorSecondMemberScan) {
    /* M-019 pin: a mirrored pool's DVAs name the top-level MIRROR vdev
     * (id 0), so a scan of either member reads correct content through
     * vdev 0 - the "accidental MATCH at id 0" is the designed og
     * behavior, pinned here so it stays intentional. The mirror
     * topology still reports through unsupported_vdevs (label-level
     * report, not a refusal). */
    auto img = build_pool("mirror");
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    std::set<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : p + "/" + nm;
            paths.insert(full);
            walk(c, full);
        }
    };
    walk(ra, "");

    EXPECT_TRUE(paths.count("tank/hello.txt"));
    EXPECT_TRUE(paths.count("tank/bigdir/alpha.bin"));
    EXPECT_GE(st.unsupported_vdevs, u64(1)); /* reported, not refused */
    EXPECT_EQ(u64(0), st.checksum_drops);    /* every read verified clean */
    EXPECT_EQ(u64(0), st.foreign_vdev_reads);
}

TEST(Zfs, SaRegistryDrivenLayout2) {
    /* M-038: objects carrying a REGISTERED non-1 layout must decode
     * through the dataset's SA registry (the og resolves attributes BY
     * NAME - ZPL_PARENT :24324, ZPL_CRTIME :24807 - and decodes every
     * registered layout through the LAYOUTS map :17636). The fixture's
     * layout 2 slots are [ZPL_SIZE, ZPL_MODE, ZPL_PARENT, ZPL_CRTIME] -
     * a deliberately different order than layout 1 - so this test
     * proves the offsets come from the registry, not the default table.
     * Before the fix the object was dropped outright (`layout != 1 ->
     * nullptr`) and the file never appeared. */
    auto img = build_pool();
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    const tree::FileProperties* reg = nullptr;
    const tree::FileProperties* hello = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props()) {
                if (c->name_utf8() == "registryfile")
                    reg = c->props();
                if (c->name_utf8() == "hello.txt")
                    hello = c->props();
            }
            find(c);
        }
    };
    find(ra);

    /* the layout-2 object surfaced with registry-decoded attrs */
    ASSERT_NE(reg, nullptr) << "registered layout-2 object dropped";
    EXPECT_EQ(u64(4096), reg->logical_size);   /* slot 0 = ZPL_SIZE */
    EXPECT_EQ(0u, reg->attributes & 0x10);     /* slot 1 = ZPL_MODE: file */
    EXPECT_EQ(u64(1700000002ull + 11644473600ull) * 10000000ull,
              reg->times.created); /* slot 3 = ZPL_CRTIME */
    /* layout-1 objects decode through the registry path too (the
     * fixture's LAYOUTS object carries the full 14-slot layout 1) */
    ASSERT_NE(hello, nullptr);
    EXPECT_EQ(u64(18), hello->logical_size);
}

TEST(Zfs, SaRegistryAbsentFallsBackToLayout1) {
    /* M-038 compat pin: pools (and fixtures) without the SA registry
     * chain decode layout-1 objects through the implicit default table
     * - the pre-M-038 behavior, required for registry-less pools. */
    auto img = build_pool();
    {
        auto z = make_microzap({{"ROOT", 2}}); /* SA_ATTRS stripped */
        memcpy(img.data() + B_MASTER_ZAP, z.data(), z.size());
        /* re-stamp the master dnode's bp over the rewritten block, then
         * the enclosing ZPL array chain */
        stamp_fletcher4(img.data() + B_ZPL_DNODES + 1 * 512 + 64, img.data() + B_MASTER_ZAP,
                        512);
        restamp_zpl_array(img);
    }
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    std::set<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : p + "/" + nm;
            paths.insert(full);
            walk(c, full);
        }
    };
    walk(ra, "");
    EXPECT_TRUE(paths.count("tank/hello.txt"));
    EXPECT_TRUE(paths.count("tank/bigdir/alpha.bin"));
    /* the layout-2 object surfaces with zero attrs when the registry is
     * absent (og parity: the uniform map lookup misses -> zeroed attrs,
     * the object still surfaces) */
    EXPECT_TRUE(paths.count("tank/registryfile"));
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

#include <cstring>
#include <zlib.h>
/* ---- GAP-P1-2: lzjb source bound + codec round-trips (session-5 cont.) ----
 * The pre-fix lzjb port ignored its psize parameter entirely - a corrupt
 * block over-read the source heap (ASan-visible). Every codec now: (a)
 * round-trips a real compressed payload, (b) REJECTS truncated sources
 * without reading past the buffer. */

static std::vector<cr::u8> zlib_deflate_raw(const cr::u8* data, size_t len, int level) {
    uLongf cl = compressBound(static_cast<uLong>(len)) + 64;
    std::vector<cr::u8> out(cl);
    z_stream zs{};
    deflateInit2(&zs, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    zs.next_in = const_cast<Bytef*>(data);
    zs.avail_in = uInt(len);
    zs.next_out = out.data();
    zs.avail_out = uInt(out.size()); /* uInt: zlib's field width (MSVC C4244 guard) */
    deflate(&zs, Z_FINISH);
    out.resize(zs.total_out);
    deflateEnd(&zs);
    return out;
}

static std::vector<cr::u8> lzjb_compress_reference(const std::vector<cr::u8>& in) {
    /* minimal reference LZJB encoder: emit copymap control bytes + matches
     * with (offset,len); enough to drive the decoder - a valid lzjb stream
     * can also be all-literals: [map=0x00, 8 literals]* */
    std::vector<cr::u8> out;
    std::vector<cr::u8> body;
    for (size_t i = 0; i < in.size();) {
        size_t map_pos = body.size();
        body.push_back(0);
        cr::u8 map = 0;
        for (int bit = 0; bit < 8 && i < in.size(); ++bit) {
            /* long-range match? search back for a run >= 3 */
            size_t best_len = 0, best_off = 0;
            const size_t max_back = i < 1023 ? i : 1023;
            for (size_t back = 1; back <= max_back; ++back) {
                size_t l = 0;
                const size_t maxl = in.size() - i;
                while (l < maxl && l < (63 + 3) && in[i - back + l] == in[i + l])
                    ++l;
                if (l >= 3 && l > best_len) {
                    best_len = l;
                    best_off = back;
                }
            }
            if (best_len >= 3 && best_off <= 0x3FF) {
                map |= cr::u8(1u << bit);
                const cr::u32 v = (cr::u32(best_off - 1) << 6) | cr::u32(best_len - 3);
                body.push_back(cr::u8(v >> 8));
                body.push_back(cr::u8(v));
                i += best_len;
            } else {
                body.push_back(in[i]);
                ++i;
            }
        }
        body[map_pos] = map;
    }
    out = body;
    return out;
}

TEST(Zfs, CodecRoundTrips) {
    /* gzip: real deflate round-trip */
    {
        std::vector<cr::u8> plain(5000, 0);
        for (size_t i = 0; i < plain.size(); ++i)
            plain[i] = cr::u8((i * 7 + (i >> 5)) & 0xFF);
        const std::vector<cr::u8> comp = zlib_deflate_raw(plain.data(), plain.size(), 6);
        std::vector<cr::u8> back(plain.size(), 0);
        ASSERT_TRUE(cr::compress::decompress_block(cr::compress::kGzip, comp.data(), comp.size(),
                                                   back.data(), back.size()));
        EXPECT_EQ(0, std::memcmp(plain.data(), back.data(), plain.size()));
    }
    /* lzjb: valid stream round-trip through the reference encoder */
    {
        std::vector<cr::u8> plain(4096, 0);
        for (size_t i = 0; i < plain.size(); ++i)
            plain[i] = cr::u8((i * 13) ^ (i >> 3));
        const std::vector<cr::u8> comp = lzjb_compress_reference(plain);
        ASSERT_FALSE(comp.empty());
        std::vector<cr::u8> back(plain.size(), 0);
        ASSERT_TRUE(cr::compress::decompress_block(cr::compress::kLzjb, comp.data(), comp.size(),
                                                   back.data(), back.size()))
            << "reference lzjb stream must decode";
        EXPECT_EQ(0, std::memcmp(plain.data(), back.data(), plain.size()));
    }
    /* zle already round-trips via ZleRoundTrip above */
}

TEST(Zfs, TruncatedSourcesRejectedNotOverread) {
    /* GAP-P1-2: a truncated lzjb payload must return false (never read
     * past the source). The pre-fix decoder looped on d alone and read
     * the heap past `psize`. */
    std::vector<cr::u8> plain(4096, 0);
    for (size_t i = 0; i < plain.size(); ++i)
        plain[i] = cr::u8((i * 13) ^ (i >> 3));
    const std::vector<cr::u8> full = lzjb_compress_reference(plain);
    /* every truncation of a REAL stream must fail cleanly */
    int accepted = 0;
    for (size_t cut = 0; cut < full.size(); ++cut) {
        std::vector<cr::u8> src(full.begin(), full.begin() + cut);
        std::vector<cr::u8> back(plain.size(), 0xAA);
        /* heap-padded source: ASan trips on ANY over-read (GAP-P1-2
         * reproduction shape) */
        if (!cr::compress::decompress_block(cr::compress::kLzjb, src.data(), src.size(),
                                            back.data(), back.size()))
            ++accepted; /* rejected = correct */
    }
    /* the full stream alone decodes; the empty one fails trivially -
     * the vast majority of truncations must be rejected */
    EXPECT_GT(accepted, int(full.size()) - 4);
}

namespace cr {
namespace zfs {

/* ---- S-005: on-disk UTF-8 names decode, never byte-lift ---------------- */

TEST(Zfs, NonAsciiNamesDecodedNotByteLifted) {
    /* ZFS zap/dnode names and the pool name are raw UTF-8 (nvlist strings
     * and zap name arrays); the og passes them through unconverted. Decode
     * to true code points - the pool display name included. */
    auto img = build_pool();
    {
        /* re-write the ZPL root dir microzap: a CJK entry, an accented
         * entry, and an invalid-sequence entry, all pointing at existing
         * objects (10 = hello.txt's dnode, 11 = bigdir) */
        auto z = make_microzap({{"hello.txt", 10},
                                {"\xe7\x85\xa7\xe7\x89\x87.jpg", 10},
                                {"caf\xc3\xa9.txt", 10},
                                {"\xff(bad", 11},
                                {"bigdir", 11},
                                {"zlefile", 12},
                                {"embedded.txt", 13},
                                {"legacy.txt", 14}});
        memcpy(img.data() + B_ROOTDIR_ZAP, z.data(), z.size());
        /* M-019: the rewritten zap block needs its bp checksum re-stamped
         * (a real pool always writes a matching fletcher4) */
        stamp_fletcher4(img.data() + B_ZPL_DNODES + 2 * 512 + 64, img.data() + B_ROOTDIR_ZAP,
                        512);
        restamp_zpl_array(img); /* the bp lives inside the checksummed array */
        /* pool display name: patch the 4-byte nvlist string "tank" in all
         * four vdev labels to the 4-byte UTF-8 name "\xe6\x96\x87x" ("文x")
         * - same length, so the packed nvlist stays structurally valid */
        const unsigned char from[4] = {'t', 'a', 'n', 'k'};
        const unsigned char to[4] = {0xe6, 0x96, 0x87, 'x'};
        size_t patched = 0;
        for (size_t i = 0; i + 4 <= img.size(); ++i) {
            if (std::memcmp(img.data() + i, from, 4) == 0) {
                std::memcpy(img.data() + i, to, 4);
                ++patched;
            }
        }
        ASSERT_GE(patched, 4u) << "pool name not found in labels";
    }
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));

    std::map<std::string, std::u16string> names;
    std::function<void(tree::FileNode*, const std::string&)> collect = [&](tree::FileNode* n,
                                                                           const std::string& p) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props())
                continue;
            const std::string nm = c->name_utf8();
            const std::string full = p.empty() ? nm : p + "/" + nm;
            names[full] = c->props()->name;
            collect(c, full);
        }
    };
    collect(ra, "");

    /* CJK file name: true code points under the CJK pool root */
    ASSERT_TRUE(names.count("\xe6\x96\x87x/\xe7\x85\xa7\xe7\x89\x87.jpg") != 0)
        << "CJK path absent";
    EXPECT_EQ(names["\xe6\x96\x87x/\xe7\x85\xa7\xe7\x89\x87.jpg"],
              std::u16string(u"\u7167\u7247.jpg"));
    /* accented name */
    ASSERT_TRUE(names.count("\xe6\x96\x87x/caf\xc3\xa9.txt") != 0);
    EXPECT_EQ(names["\xe6\x96\x87x/caf\xc3\xa9.txt"], std::u16string(u"caf\u00e9.txt"));
    /* invalid lead byte -> U+FFFD, valid tail survives */
    ASSERT_TRUE(names.count("\xe6\x96\x87x/\xef\xbf\xbd(bad") != 0);
    EXPECT_EQ(names["\xe6\x96\x87x/\xef\xbf\xbd(bad"], std::u16string(u"\ufffd(bad"));
    /* the pool display root itself decoded the patched name */
    ASSERT_TRUE(names.count("\xe6\x96\x87x") != 0);
    EXPECT_EQ(names["\xe6\x96\x87x"], std::u16string(u"\u6587x"));
}

/* ---- S-034: the ring txg-history walk -----------------------------------
 * The og's quick searcher iterates EVERY uberblock slot in the ring and
 * walks each txg state's tree (ZfsSearchDLL FUN_180006b90 -> addBlock
 * FUN_180003150 -> the unbounded from_txg=0 descent). Files deleted
 * within the ring window are og-visible and were core-invisible. */

TEST(Zfs, RingTxgHistorySurfacesInWindowDeletions) {
    /* state txg 77 contains hello.txt; state txg 78 (ring slot 1) does
     * not. hello.txt must surface from the 77-state walk as Deleted. */
    auto img = build_pool();
    stamp_second_state(img, TXG + 1, /*without_hello=*/true);
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st));
    EXPECT_EQ(u64(2), st.ring_states);
    EXPECT_EQ(u64(TXG + 1), st.txg);

    std::map<std::string, int> live, dead;
    collect_paths_counted(ra, "", live);
    collect_paths_counted(rd, "", dead);

    /* live: the 78-state's content, hello.txt absent in BOTH datasets
     * (they share the moved-forward ZPL chain) */
    EXPECT_EQ(0, live["tank/hello.txt"]) << "hello.txt must not be live";
    EXPECT_EQ(0, live["sub/hello.txt"]) << "sub shares the new ZPL: same deletion";
    EXPECT_EQ(1, live["tank/bigdir"]);
    EXPECT_EQ(1, live["tank/zlefile"]);
    EXPECT_EQ(1, live["tank/legacy.txt"]);

    /* deleted: exactly the in-window deletions. The live datasets keep
     * SHADOW containers under the deleted root (pruned when empty):
     * tank holds the deleted hello.txt; sub holds its own copy. Live
     * children never re-emit. */
    ASSERT_EQ(1, dead["tank/hello.txt"]) << "in-window deletion not surfaced";
    ASSERT_EQ(1, dead["sub/hello.txt"]) << "sub's in-window deletion not surfaced";
    EXPECT_EQ(0, dead["tank/bigdir"]) << "live dir must not re-emit as deleted";
    EXPECT_EQ(0, dead["tank/bigdir/alpha.bin"]) << "live child re-emitted";
    EXPECT_GE(st.deleted_objects, u64(2));
}

TEST(Zfs, RingTxgHistoryDeletedNodeCarriesPropsAndMarks) {
    auto img = build_pool();
    stamp_second_state(img, TXG + 1, /*without_hello=*/true);
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st));

    /* locate tank/hello.txt under the deleted root */
    tree::FileNode* tank = nullptr;
    for (tree::FileNode* c = rd->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "tank")
            tank = c;
    ASSERT_NE(nullptr, tank);
    tree::FileNode* hello = nullptr;
    for (tree::FileNode* c = tank->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "hello.txt")
            hello = c;
    ASSERT_NE(nullptr, hello);
    EXPECT_EQ(tree::DeletedState::Deleted, hello->deleted_state());
    EXPECT_EQ(tree::Origin::Structured, hello->origin());
    ASSERT_NE(nullptr, hello->props());
    EXPECT_EQ(u64(18), hello->props()->logical_size);
    ASSERT_EQ(1u, hello->props()->extents.size());
    EXPECT_EQ(B_FILE_DATA, hello->props()->extents[0].offset);
    EXPECT_EQ(u32(FsKindCode::Zfs), hello->props()->fs_kind);
}

TEST(Zfs, RingHistoryDedupNewestStateWins) {
    /* both ring states contain identical content (the newer uberblock
     * points at the same MOS): every object emits exactly once in the
     * live tree, the deleted view stays empty (shadows pruned). */
    auto img = build_pool();
    stamp_second_state(img, TXG + 1, /*without_hello=*/false);
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st));
    EXPECT_EQ(u64(2), st.ring_states);

    std::map<std::string, int> live, dead;
    collect_paths_counted(ra, "", live);
    collect_paths_counted(rd, "", dead);
    EXPECT_EQ(1, live["tank/hello.txt"]) << "object present in both states emitted twice";
    EXPECT_EQ(1, live["tank/bigdir/alpha.bin"]);
    EXPECT_EQ(1, live["sub/hello.txt"]);
    EXPECT_TRUE(dead.empty()) << "no deletions: the deleted root must stay empty";
    EXPECT_EQ(u64(0), st.deleted_objects);
}

TEST(Zfs, TornNewestUberblockFallsBackToOlderState) {
    /* the newest uberblock's rootbp is garbage (a torn write): the og's
     * addBlock no-ops on the unopenable state and the next-newest state
     * provides the live tree - the scan must not fail. */
    auto img = build_pool();
    stamp_second_state(img, TXG + 1, /*without_hello=*/true);
    /* corrupt the txg-78 uberblock's rootbp: DVA offset beyond the device
     * (the MOS load fails, the state is skipped) */
    u8* ub = img.data() + (128ull << 10) + 1024;
    put64(ub + 48, IMG * 2); /* rootbp DVA0 word 1: offset>>9, far out of range */
    MemDevice dev(img);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, st))
        << "a torn newest uberblock must not abort the scan";
    EXPECT_EQ(u64(TXG + 1), st.txg) << "stats report the newest FOUND txg";
    EXPECT_EQ(u64(2), st.ring_states);

    std::map<std::string, int> live, dead;
    collect_paths_counted(ra, "", live);
    collect_paths_counted(rd, "", dead);
    /* the 77-state is now the live source: hello.txt is LIVE in both
     * datasets (the newest WALKABLE state still has them); nothing older
     * remains for the deleted view (77 is the oldest state) */
    EXPECT_EQ(1, live["tank/hello.txt"]);
    EXPECT_EQ(1, live["sub/hello.txt"]);
    EXPECT_TRUE(dead.empty());
}

TEST(Zfs, RingStatesCountedAcrossLabels) {
    /* the stock fixture stamps the SAME txg in all four labels: the
     * distinct-txg dedup must collapse them to one state (the og's
     * consecutive-slot memo collapses its label-0 repeats). */
    auto img = build_pool();
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    ZfsScanStats st;
    ASSERT_EQ(OK, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(u64(1), st.ring_states);
}

/* ------------------------------------------------------------------ */
/* N-072: the delay-execution label retry (live handles only)          */
/* ------------------------------------------------------------------ */

/* A device whose first K read calls fail (the torn-label window), then
 * reads succeed - the hot-unplug/replug shape the og's polling import
 * heals. is_static_media stays FALSE (a live handle). */
class FlakyStartDevice : public IStorageDevice {
public:
    FlakyStartDevice(std::vector<u8> d, u32 fail_first)
        : d_(std::move(d)), fail_first_(fail_first) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (reads_++ < fail_first_)
            return ERR_READ_FAILED;
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
    u32 fail_first_ = 0;
    u32 reads_ = 0;
};

TEST(Zfs, LabelRetryBackoff) {
    /* the finding's named scenario: first probe fails, second succeeds
     * via the fault-injecting device - the pool is found on retry and
     * the retry is counted (fail-before by construction: the pre-N-072
     * engine returned ERR_BAD_MAGIC after ONE sweep - no retry existed). */
    auto img = build_pool();
    /* fail the first 64 reads: the first label sweep dies on every label
     * (each label's reads break on the first failure); the 250 ms backoff
     * passes, the second sweep reads the intact pool */
    FlakyStartDevice dev(img, 64);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    u32 rc = scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_GE(st.label_retries, 1u);
    EXPECT_NE(st.txg, 0u);
}

TEST(Zfs, LabelRetryGivesUpBounded) {
    /* every read fails forever: the retry loop is BOUNDED (3 retries) and
     * the engine reports ERR_BAD_MAGIC (worst case adds 250+500+1000 ms
     * of cancel-aware backoff, then stops - never an infinite poll) */
    auto img = build_pool();
    FlakyStartDevice dev(img, 0xFFFFFFFFu);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(st.label_retries, 3u);
}

TEST(Zfs, StaticMediaNeverRetries) {
    /* an image file (static media) with a torn label: NO retry - a torn
     * image cannot heal (the finding's rule; is_static_media gates the
     * loop). A failing read device marked static fails after ONE sweep. */
    auto img = build_pool();
    class StaticFlakyDevice final : public FlakyStartDevice {
    public:
        StaticFlakyDevice(std::vector<u8> d, u32 fail_first)
            : FlakyStartDevice(std::move(d), fail_first) {}
        bool is_static_media() const override { return true; }
    };
    StaticFlakyDevice dev(img, 0xFFFFFFFFu);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ZfsScanStats st;
    EXPECT_EQ(ERR_BAD_MAGIC, scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, st));
    EXPECT_EQ(st.label_retries, 0u) << "static media never enters the retry loop";
}

} // namespace zfs
} // namespace cr
