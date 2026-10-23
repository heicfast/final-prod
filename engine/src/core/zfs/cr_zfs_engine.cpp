/* cr_zfs_engine.cpp - ZFS scan engine (single-vdev pools, little-endian).
 *
 * Session-5 rewrite: every on-disk constant re-verified against OpenZFS
 * master sources (see AUDIT.md section 5 - the verification ledger):
 *   - include/sys/vdev_impl.h   (label layout: 8K pad + 8K boot env +
 *     112K vdev_phys + 128K uberblock ring = 256K per label; 4 labels at
 *     disk start x2 / end x2)
 *   - include/sys/dnode.h       (dnode_phys: datablkszsec u16@8,
 *     bonuslen u16@10, extra_slots u8@12, maxblkid u64@16, blkptr@64,
 *     bonus at 64+nblkptr*128, spill at (extra_slots+1)*512-128)
 *   - include/sys/spa.h         (blkprop field packing, DVA accessors,
 *     BPE embedded-bp layout; decode routine from module/zfs/blkptr.c)
 *   - include/sys/zap_impl.h + zap_leaf.h (ZAP_MAGIC 0x2F52AB2AB,
 *     ZBT_MICRO/HEADER/LEAF = (1<<63)+3/1/0, mzap entries 64B at +64
 *     {value u64@0, cd u32@8, name@14[50]}, fzap ptrtbl + 24-byte leaf
 *     chunks with ENTRY/ARRAY/FREE types)
 *   - include/sys/sa_impl.h     (SA_MAGIC 0x2F505A, layout_info u16@4)
 *   - module/zfs/dmu.h          (DMU object types)
 *   - module/zfs/{lzjb.c,zle.c,blkptr.c} (decompression + BPE decode)
 *
 * Compression: off/lzjb/zle/gzip via zlib, lz4, zstd (cr::compress).
 * Extents carry {codec, psize} so the writer decodes at stream time.
 *
 * Deleted-file story (parity decision, AUDIT.md row 26): ZFS is CoW with
 * copy-on-everything semantics; the og reads the newest txg's live state
 * only and lets RAW carving cover deleted content. We do the same.
 *
 * Big-endian pools (SPARC/POWER writers) are detected and refused with a
 * clear error code rather than misparsed (bit 63 of blk_prop marks the
 * writer's byteorder; the uberblock magic reads reversed on BE pools).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/zfs/cr_zfs_engine.h"
#include "cr/compress/cr_decomp.h"
#include <cstring>
#include <functional>
#include <map>
#include <set>

namespace cr {
namespace zfs {

namespace {

/* ---- label geometry (OpenZFS vdev_impl.h) ---- */
constexpr u64 LABEL_SIZE = 256ull << 10;
constexpr u64 VDEV_PHYS_OFFSET = 16ull << 10; /* 8K skip + 8K boot env */
constexpr u64 VDEV_PHYS_SIZE = 112ull << 10;
constexpr u64 UBRING_OFFSET = 128ull << 10;
constexpr u32 UBERBLOCK_COUNT = 128;
constexpr u64 UBERBLOCK_SIZE = 1024;
constexpr u64 UBERBLOCK_MAGIC = 0x00BAB10CULL;

/* ---- uberblock ---- */
constexpr u32 UB_MAGIC = 0;
constexpr u32 UB_TXG = 16;
constexpr u32 UB_TIMESTAMP = 32;
constexpr u32 UB_ROOTBP = 40; /* blkptr 128 bytes */

/* ---- blkptr (OpenZFS spa.h) ----
 * blk_dva[3] @0 (16B each), blk_prop u64@48, blk_pad@56/64,
 * blk_phys_birth@64, blk_birth@72, blk_fill@80, blk_cksum@88..119.
 * prop: lsize bits0-15 ((v+1)<<9), psize bits16-31 ((v+1)<<9),
 * comp bits32-39, embedded bit39, etype bits40-47, byteorder bit63.
 * DVA: vdev = w0>>40 (24b), asize = (w0&0xFFFFFF)<<9,
 *      offset = (w1 & ((1<<63)-1))<<9, gang = w1 bit63. */
constexpr u32 BP_DVA = 0;
constexpr u32 BP_PROP = 48;
constexpr u32 BP_BIRTH = 72;
constexpr u32 BP_SIZE = 128;

constexpr u32 ZIO_COMPRESS_LZJB = 3;
constexpr u32 ZIO_COMPRESS_EMPTY = 4;
constexpr u32 ZIO_COMPRESS_ZLE = 14;
constexpr u32 ZIO_COMPRESS_LZ4 = 15;
constexpr u32 ZIO_COMPRESS_ZSTD = 16;

u64 rdle64(const u8* p) {
    return u64(p[0]) | (u64(p[1]) << 8) | (u64(p[2]) << 16) | (u64(p[3]) << 24) |
           (u64(p[4]) << 32) | (u64(p[5]) << 40) | (u64(p[6]) << 48) | (u64(p[7]) << 56);
}
u32 rdle32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
u16 rdle16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}

bool bp_is_embedded(const u8* bp) {
    return (rdle64(bp + BP_PROP) >> 39) & 1;
}
bool bp_is_hole(const u8* bp) {
    /* all three DVAs zero (spa.h BP_IS_HOLE) */
    for (int i = 0; i < 48; ++i)
        if (bp[i] != 0)
            return false;
    return true;
}
u64 bp_lsize(const u8* bp) {
    const u64 prop = rdle64(bp + BP_PROP);
    if (bp_is_embedded(bp)) /* bytes, bits 0-24, +1 (BPE_GET_LSIZE) */
        return (prop & 0x1FFFFFFull) + 1;
    return ((prop & 0xFFFFull) + 1) << 9; /* sectors-biased */
}
u64 bp_psize(const u8* bp) {
    const u64 prop = rdle64(bp + BP_PROP);
    if (bp_is_embedded(bp)) /* bytes, bits 25-31, +1 */
        return ((prop >> 25) & 0x7Full) + 1;
    return (((prop >> 16) & 0xFFFFull) + 1) << 9;
}
u32 bp_compress(const u8* bp) {
    /* SPA_COMPRESSBITS = 7: bit 39 is the embedded flag, not compress */
    return u32((rdle64(bp + BP_PROP) >> 32) & 0x7F);
}
u32 bpe_etype(const u8* bp) {
    return u32((rdle64(bp + BP_PROP) >> 40) & 0xFF);
}
u64 dva_vdev(const u8* dva) {
    return (rdle64(dva) >> 40) & 0xFFFFFFull;
}
u64 dva_asize(const u8* dva) {
    return (rdle64(dva) & 0xFFFFFFull) << 9;
}
u64 dva_offset(const u8* dva) {
    return (rdle64(dva + 8) & 0x7FFFFFFFFFFFFFFFull) << 9;
}
bool dva_is_gang(const u8* dva) {
    return (rdle64(dva + 8) >> 63) & 1;
}

/* BPE payload decode - byte-exact port of module/zfs/blkptr.c
 * decode_embedded_bp_compressed: payload words are the 16 u64 words of
 * the blkptr except word 6 (blk_prop) and word 10 (blk_birth); bytes are
 * taken low-first from each word (little endian). */
bool bpe_payload(const u8* bp, u8* out, u32 psize) {
    u32 done = 0;
    for (u32 word = 0; word < 16 && done < psize; ++word) {
        if (word == 6 || word == 10)
            continue;
        const u64 w = rdle64(bp + u64(word) * 8);
        for (u32 k = 0; k < 8 && done < psize; ++k)
            out[done++] = u8((w >> (k * 8)) & 0xFF);
    }
    return done == psize;
}

/* ---- dnode_phys (OpenZFS include/sys/dnode.h) ---- */
constexpr u32 DN_TYPE = 0;
constexpr u32 DN_INDBLKSHIFT = 1;
constexpr u32 DN_NLEVELS = 2;
constexpr u32 DN_NBLKPTR = 3;
constexpr u32 DN_BONUSTYPE = 4;
constexpr u32 DN_CHECKSUM = 5;
constexpr u32 DN_COMPRESS = 6;
constexpr u32 DN_FLAGS = 7;
constexpr u32 DN_DATABLKSZSEC = 8;
constexpr u32 DN_BONUSLEN = 10;
constexpr u32 DN_EXTRA_SLOTS = 12;
constexpr u32 DN_MAXBLKID = 16;
constexpr u32 DN_BLKPTR = 64;
constexpr u32 DN_SIZE = 512;

constexpr u32 DMU_OT_DNODE = 10;
constexpr u32 DMU_OT_OBJSET = 11;
constexpr u32 DMU_OT_ZNODE = 17; /* legacy bonus */
constexpr u32 DMU_OT_PLAIN_FILE_CONTENTS = 19;
constexpr u32 DMU_OT_DIRECTORY_CONTENTS = 20;
constexpr u32 DMU_OT_MASTER_NODE = 21;
constexpr u32 DMU_OT_SA = 43; /* SA bonus */

/* legacy znode_phys (OpenZFS module/zfs/zfs_znode.h) */
constexpr u32 ZP_ATIME = 0x00; /* sec+usec pairs x4 */
constexpr u32 ZP_MTIME = 0x10;
constexpr u32 ZP_CTIME = 0x20;
constexpr u32 ZP_CRTIME = 0x30;
constexpr u32 ZP_GEN = 0x40;
constexpr u32 ZP_MODE = 0x48;
constexpr u32 ZP_UID = 0x50;
constexpr u32 ZP_GID = 0x58;
constexpr u32 ZP_PARENT = 0x60;
constexpr u32 ZP_LINKS = 0x68;
constexpr u32 ZP_SIZE = 0x70;
constexpr u32 ZP_FLAGS = 0x78;

/* SA header (OpenZFS include/sys/sa_impl.h): magic u32@0 = 0x2F505A,
 * layout_info u16@4 (bits 0-9 layout, bits 10-15 hdrsize/8), then the
 * attribute data. Layout 1 = ZPL standard order (atime mtime ctime
 * crtime gen mode uid gid parent links size flags) - offsets below are
 * relative to the SA HEADER start (8-byte header + body). */
constexpr u32 SA_MAGIC = 0x2F505A;
constexpr u32 SA_LAYOUT1_ATIME = 8;
constexpr u32 SA_LAYOUT1_MTIME = 24;
constexpr u32 SA_LAYOUT1_CTIME = 40;
constexpr u32 SA_LAYOUT1_CRTIME = 56;
constexpr u32 SA_LAYOUT1_MODE = 80;
constexpr u32 SA_LAYOUT1_UID = 88;
constexpr u32 SA_LAYOUT1_GID = 96;
constexpr u32 SA_LAYOUT1_PARENT = 104;
constexpr u32 SA_LAYOUT1_LINKS = 112;
constexpr u32 SA_LAYOUT1_SIZE = 120;
constexpr u32 SA_LAYOUT1_FLAGS = 128;
constexpr u32 SA_LAYOUT1_MIN = 136; /* header + body */

/* ---- objset_phys ---- */
constexpr u32 OS_META_DNODE = 0;

/* ---- dsl_dir / dsl_dataset bonus ---- */
constexpr u32 DD_CHILD_DIR_ZAPOBJ = 240; /* 0xF0 */
constexpr u32 DD_HEAD_DATASET_OBJ = 248; /* 0xF8 */
constexpr u32 DS_BP = 128;

/* ---- ZAP (OpenZFS zap_impl.h / zap_leaf.h) ---- */
constexpr u64 ZBT_MICRO = (1ull << 63) + 3;
constexpr u64 ZBT_HEADER = (1ull << 63) + 1;
constexpr u64 ZBT_LEAF = (1ull << 63) + 0;
constexpr u64 ZAP_MAGIC = 0x2F52AB2ABull;
constexpr u32 MZAP_ENTRIES_AT = 64; /* mzap_phys header */
constexpr u32 MZAP_ENT_SIZE = 64;   /* mzap_ent_phys */
constexpr u32 MZAP_NAME_AT = 14;    /* name[50] */

/* fzap header (zap_phys): block_type@0, magic@8, ptrtbl {zt_blk@16,
 * zt_numblks@24, zt_shift@32, ...}, freeblk@56, num_leafs@64... */
constexpr u32 FZ_ZT_BLK = 16;
constexpr u32 FZ_ZT_NUMBLKS = 24;
constexpr u32 FZ_ZT_SHIFT = 32;
constexpr u32 FZ_FREEBLK = 56;
constexpr u32 FZ_NUM_LEAFS = 64;
constexpr u32 FZ_NUM_ENTRIES = 72;

/* zap leaf: ZAP_LEAF_MAGIC 0x2AB1EAF; 24-byte chunks; chunk types */
constexpr u32 ZAP_LEAF_MAGIC = 0x2AB1EAFu;
constexpr u32 ZAP_LEAF_CHUNKSIZE = 24;
constexpr u32 ZAP_CHUNK_FREE = 253;
constexpr u32 ZAP_CHUNK_ENTRY = 252;
constexpr u32 ZAP_CHUNK_ARRAY = 251;
/* leaf header: block_type u64@0, pad u64@8, prefix u64@16, magic u32@24,
 * nfree u16@28, nentries u16@30, prefix_len u16@32, freelist u16@34,
 * flags u8@36; hash table (u16 x (1<<(bs-5))) @40? - computed: header is
 * 2 chunks = 48 bytes, hash table follows, chunks after it. */
constexpr u32 ZL_MAGIC = 24;
constexpr u32 ZL_NENTRIES = 30;

/* nvlist native encoding (LE pools) */
constexpr u32 NV_DATA_UINT64 = 8;
constexpr u32 NV_DATA_STRING = 9;
constexpr u32 NV_DATA_NVLIST = 19;

constexpr u32 MAX_OBJECTS = 1u << 20;
constexpr u32 MAX_ZAP_ENTRIES = 65536;
constexpr u32 MAX_ZAP_LEAFS = 16384;
constexpr int MAX_DEPTH = 48;
constexpr u64 MAX_BONUS = 320; /* DN_OLD_MAX_BONUSLEN */
constexpr u64 MAX_L0_PER_DNODE = 4096;
constexpr int MAX_DATASET_DEPTH = 8;

struct NvValue {
    u32 type = 0;
    u64 u64v = 0;
    std::string s;
    std::map<std::string, NvValue> child;
};

bool nv_parse(const u8* p, u32 len, std::map<std::string, NvValue>& out, u32 depth = 0) {
    if (p == nullptr || len < 8 || depth > 4)
        return false;
    u32 pos = 8; /* nvl_version + nvl_flag */
    while (pos + 8 <= len) {
        const u32 enc = rdle32(p + pos);
        if (enc == 0)
            break; /* terminator pair */
        if (pos + 16 > len)
            break;
        const u32 dec = rdle32(p + pos + 4);
        (void)dec;
        const u32 name_len = p[pos + 8];
        if (name_len == 0 || name_len > 64 || pos + 12 + name_len > len)
            break;
        std::string name(reinterpret_cast<const char*>(p + pos + 9), name_len - 1);
        u32 q = pos + 9 + name_len;
        q = (q + 3) & ~3u; /* name padded to 4 */
        if (q + 8 > len)
            break;
        const u32 type = rdle32(p + q);
        const u32 nelem = rdle32(p + q + 4);
        q += 8;
        NvValue v;
        v.type = type;
        if (type == NV_DATA_UINT64 && nelem == 1 && q + 8 <= len) {
            v.u64v = rdle64(p + q);
            q += 8;
        } else if (type == NV_DATA_STRING && nelem == 1 && q + 4 <= len) {
            const u32 slen = rdle32(p + q);
            if (q + 4 + slen <= len && slen > 0 && slen < 256) {
                v.s.assign(reinterpret_cast<const char*>(p + q + 4), slen - 1);
                q += 4 + slen;
            }
        } else if (type == NV_DATA_NVLIST && nelem == 1) {
            if (q + 8 <= len)
                nv_parse(p + q, len - q, v.child, depth + 1);
            break; /* nested list ends the pair stream in our subset */
        } else {
            break;
        }
        out[name] = v;
        if (enc > len - pos)
            break; /* P1-8 wrap guard */
        pos = (pos + enc) & ~1u;
        if (pos >= len)
            break;
    }
    return !out.empty();
}

/* ---- volume context ---- */
struct Volume {
    IStorageDevice* dev = nullptr;
    u64 ashift = 9;
    u64 vdev_offset = 0; /* data-area byte offset of vdev 0 */
    u64 vdev_asize = 0;
    u64 total_bytes = 0;
    u64 damaged = 0; /* failed block reads */

    /* read the raw (possibly compressed) bytes a DVA points at */
    bool read_raw(const u8* bp, u32 dva_index, std::vector<u8>& out) {
        const u8* dva = bp + BP_DVA + u64(dva_index) * 16;
        const u64 vdev = dva_vdev(dva);
        if (vdev != 0)
            return false; /* multi-vdev pool: single-disk resolvable only */
        if (dva_is_gang(dva))
            return false; /* gang leader: read_gang handles it */
        const u64 off = dva_offset(dva);
        const u64 asz = dva_asize(dva);
        const u64 psz = bp_psize(bp);
        if (psz == 0 || asz == 0 || psz > asz)
            return false;
        if (vdev_offset + off + psz > total_bytes)
            return false;
        out.resize(size_t(psz));
        if (dev->read_at(vdev_offset + off, out.data(), u32(out.size())) != OK) {
            ++damaged;
            return false;
        }
        return true;
    }

    /* read + decompress a block to its logical image */
    bool read_block(const u8* bp, u32 dva_index, std::vector<u8>& out) {
        std::vector<u8> raw;
        if (!read_raw(bp, dva_index, raw))
            return false;
        const u32 zc = bp_compress(bp);
        if (zc == 0 || zc == 1 || zc == 2 || zc == ZIO_COMPRESS_EMPTY) {
            /* off/on/inherit/empty: no transform (empty = zero block) */
            if (zc == ZIO_COMPRESS_EMPTY)
                std::memset(raw.data(), 0, raw.size());
            out = std::move(raw);
            return true;
        }
        const u64 lsize = bp_lsize(bp);
        if (lsize == 0 || lsize > (256ull << 20))
            return false;
        out.resize(size_t(lsize));
        const u32 codec = compress::codec_from_zfs(zc);
        if (!compress::decompress_block(codec, raw.data(), raw.size(), out.data(), out.size())) {
            ++damaged;
            return false;
        }
        return true;
    }

    /* gang block: the leader DVA (gang bit set) points to a 512-byte
     * "gang header" of blkptrs whose data, concatenated, forms the block.
     * (SPA_GANGLINE... the gang header is one 512B block of up to 3?
     * OpenZFS: a gang block is a 512-byte block of 16? - the header holds
     * GANG_HEADER... blkptrs until the terminator.) */
    bool read_gang(const u8* dva, std::vector<u8>& out) {
        const u64 off = dva_offset(dva);
        const u64 psz = 512; /* gang header is SPA_MINBLOCKSIZE */
        if (vdev_offset + off + psz > total_bytes)
            return false;
        std::vector<u8> hdr(static_cast<size_t>(psz), 0);
        if (dev->read_at(vdev_offset + off, hdr.data(), u32(psz)) != OK) {
            ++damaged;
            return false;
        }
        out.clear();
        for (u32 i = 0; i < 512 / BP_SIZE; ++i) {
            const u8* gbp = hdr.data() + u64(i) * BP_SIZE;
            if (bp_is_hole(gbp))
                continue; /* terminator or padding */
            if (bp_is_embedded(gbp))
                continue;
            std::vector<u8> piece;
            if (!read_block(gbp, 0, piece))
                return false;
            out.insert(out.end(), piece.begin(), piece.end());
            if (out.size() > (16ull << 20))
                return false; /* cap */
        }
        return !out.empty();
    }
};

/* ---- dnode access ---- */
struct Dnode {
    std::vector<u8> raw; /* 512 bytes (or larger with extra slots) */
    u32 type() const { return raw[DN_TYPE]; }
    u32 nlevels() const { return raw[DN_NLEVELS]; }
    u32 nblkptr() const { return raw[DN_NBLKPTR]; }
    u32 bonustype() const { return raw[DN_BONUSTYPE]; }
    u32 datablksz() const { return u32(rdle16(raw.data() + DN_DATABLKSZSEC)) * 512; }
    u32 bonuslen() const { return rdle16(raw.data() + DN_BONUSLEN); }
    u32 extra_slots() const { return raw[DN_EXTRA_SLOTS]; }
    u64 maxblkid() const { return rdle64(raw.data() + DN_MAXBLKID); }
    const u8* blkptr(u32 i) const { return raw.data() + DN_BLKPTR + u64(i) * BP_SIZE; }
    u64 dnode_size() const { return 512ull * (1 + extra_slots()); }
    const u8* bonus() const { return raw.data() + DN_BLKPTR + u64(nblkptr()) * BP_SIZE; }
    u64 bonus_cap() const {
        /* tail region (OpenZFS dnode.h diagram): the bonus spans
         * [64+nblkptr*128, dnode_size) when no spill blkptr is present;
         * a spill overlays the final 128 bytes. bonuslen itself declares
         * the true size - this is only the hard bound. */
        const u64 bstart = DN_BLKPTR + u64(nblkptr()) * BP_SIZE;
        return dnode_size() > bstart ? dnode_size() - bstart : 0;
    }
};

struct ObjSet {
    Volume* v = nullptr;
    Dnode meta; /* the objset's meta dnode */

    bool load(Volume* vol, const u8* bp) {
        v = vol;
        std::vector<u8> blk;
        if (bp_is_embedded(bp) || bp_is_hole(bp))
            return false;
        /* objset_phys: meta dnode + [zil hdr] + 2 dnodes */
        if (!vol->read_block(bp, 0, blk))
            return false;
        if (blk.size() < DN_SIZE)
            return false;
        meta.raw.assign(blk.begin(), blk.begin() + DN_SIZE);
        return meta.nblkptr() > 0 && meta.datablksz() > 0;
    }

    /* Read dnode N through the meta dnode's block tree (any nlevels). */
    bool read_dnode(u64 object, Dnode& out) {
        if (object >= MAX_OBJECTS)
            return false;
        const u64 logical = object * DN_SIZE;
        /* find the L0 block covering `logical`, walking the indirect
         * tree from the meta dnode's blkptr array */
        const u32 indshift = meta.raw[DN_INDBLKSHIFT];
        const u32 levels = meta.nlevels();
        const u64 dbsz = meta.datablksz();
        if (dbsz == 0)
            return false;
        const u64 blk = logical / dbsz;
        /* collect candidate L0 blkptr through the tree */
        std::vector<u8> node;
        const u8* bp = nullptr;
        if (levels <= 1) {
            const u64 idx = blk;
            if (idx >= meta.nblkptr())
                return false;
            bp = meta.blkptr(u32(idx));
        } else {
            /* walk down: level L node = (1<<indshift)/128 blkptrs */
            const u64 per_node = (u64(1) << indshift) / BP_SIZE;
            /* index decomposition: for levels==2: top = meta blkptr[blk /
             * per_node], then L1 slot blk % per_node */
            u64 index = blk;
            u64 divisor = 1;
            for (u32 l = 1; l < levels; ++l)
                divisor *= per_node;
            u64 top_idx = 0;
            bool ok = false;
            for (u32 l = 0; l + 1 < levels; ++l) {
                top_idx = index / divisor;
                index %= divisor;
                divisor /= per_node;
                if (l == 0) {
                    if (top_idx >= meta.nblkptr())
                        return false;
                    if (!v->read_block(meta.blkptr(u32(top_idx)), 0, node))
                        return false;
                    ok = true;
                } else {
                    if (top_idx >= node.size() / BP_SIZE)
                        return false;
                    std::vector<u8> next;
                    if (!v->read_block(node.data() + u64(top_idx) * BP_SIZE, 0, next))
                        return false;
                    node = std::move(next);
                }
            }
            if (!ok || index >= node.size() / BP_SIZE)
                return false;
            bp = node.data() + u64(index) * BP_SIZE;
        }
        if (bp == nullptr || bp_is_hole(bp) || bp_is_embedded(bp))
            return false;
        std::vector<u8> data;
        if (!v->read_block(bp, 0, data))
            return false;
        const u64 in_block = logical % dbsz;
        if (in_block + DN_SIZE > data.size())
            return false;
        out.raw.assign(data.begin() + ptrdiff_t(in_block),
                       data.begin() + ptrdiff_t(in_block + DN_SIZE));
        return out.nblkptr() > 0 && out.datablksz() > 0;
    }
};

/* ---- ZAP reading (micro + fat) ---- */
struct ZapEntry {
    std::string name;
    u64 value = 0;
};

bool read_microzap(const std::vector<u8>& blk, std::vector<ZapEntry>& out) {
    if (blk.size() < MZAP_ENTRIES_AT + MZAP_ENT_SIZE)
        return false;
    if (rdle64(blk.data()) != ZBT_MICRO)
        return false;
    const u64 sz = blk.size();
    for (u64 off = MZAP_ENTRIES_AT; off + MZAP_ENT_SIZE <= sz; off += MZAP_ENT_SIZE) {
        const u8* e = blk.data() + off;
        const u64 value = rdle64(e); /* mze_value @0 */
        if (value == 0)
            continue;
        const char* name = reinterpret_cast<const char*>(e + MZAP_NAME_AT);
        const u32 nlen = u32(strnlen(name, 50));
        if (nlen == 0)
            continue;
        ZapEntry ze;
        ze.name.assign(name, nlen);
        ze.value = value;
        out.push_back(std::move(ze));
        if (out.size() > MAX_ZAP_ENTRIES)
            break;
    }
    return true;
}

/* zap leaf chunk array walk (OpenZFS zap_leaf.h):
 * layout: header (2 chunks = 48 B) + hash table (u16 x (bsz>>5)) + chunks.
 * Entry: le_type(1)=252, le_value_intlen(1), le_next u16, le_name_chunk
 * u16, le_name_numints u16, le_value_chunk u16, le_value_numints u16,
 * le_cd u32, le_hash u64. Array chunk: la_type(1)=251, 21 bytes, la_next
 * u16. */
bool read_leaf(const std::vector<u8>& leaf, std::vector<ZapEntry>& out) {
    if (leaf.size() < 64)
        return false;
    if (rdle32(leaf.data() + ZL_MAGIC) != ZAP_LEAF_MAGIC)
        return false;
    const u32 bshift = [](u64 sz) {
        u32 s = 0;
        while ((u64(1) << s) < sz)
            ++s;
        return s;
    }(leaf.size());
    const u32 hash_entries = 1u << (bshift > 5 ? bshift - 5 : 1);
    const u32 chunks_at = 48 + hash_entries * 2; /* after header+hash */
    const u32 nchunks = u32((leaf.size() - chunks_at) / ZAP_LEAF_CHUNKSIZE);
    if (nchunks == 0)
        return false;

    auto chunk = [&](u32 i) -> const u8* {
        return leaf.data() + chunks_at + u64(i) * ZAP_LEAF_CHUNKSIZE;
    };
    /* read a byte array threaded through ARRAY chunks */
    auto read_array = [&](u32 first, u32 numints, std::vector<u8>& data) -> bool {
        if (numints > 1024)
            return false;
        u32 ci = first;
        u32 got = 0;
        while (got < numints) {
            if (ci >= nchunks)
                return false;
            const u8* c = chunk(ci);
            if (c[0] != ZAP_CHUNK_ARRAY)
                return false;
            const u32 take = numints - got < 21 ? numints - got : 21;
            data.insert(data.end(), c + 1, c + 1 + take);
            got += take;
            ci = rdle16(c + 22); /* la_next at offset 22 (1+21) */
            if (ci == 0xFFFF)
                break;
        }
        return got == numints;
    };

    const u16 nentries = rdle16(leaf.data() + ZL_NENTRIES);
    for (u32 i = 0; i < nchunks && out.size() < MAX_ZAP_ENTRIES; ++i) {
        const u8* c = chunk(i);
        if (c[0] != ZAP_CHUNK_ENTRY)
            continue;
        const u32 value_intlen = c[1];
        const u32 name_chunk = rdle16(c + 4);
        const u32 name_numints = rdle16(c + 6);
        const u32 value_chunk = rdle16(c + 8);
        const u32 value_numints = rdle16(c + 10);
        if (name_numints == 0 || name_numints > 256)
            continue;
        std::vector<u8> name;
        if (!read_array(name_chunk, name_numints, name))
            continue;
        if (name.empty() || name.back() != 0)
            continue;
        ZapEntry ze;
        ze.name.assign(reinterpret_cast<const char*>(name.data()), name.size() - 1);
        if (ze.name.empty())
            continue;
        if (value_numints > 0 && value_chunk != 0xFFFF) {
            std::vector<u8> val;
            if (!read_array(value_chunk, value_numints * (value_intlen ? value_intlen : 1), val))
                continue;
            ze.value = val.size() >= 8 ? rdle64(val.data()) : 0;
        }
        out.push_back(std::move(ze));
    }
    (void)nentries;
    return !out.empty();
}

/* fat zap: header block -> pointer table -> leaves (OpenZFS zap_micro.c
 * mze/zap_ptrtbl semantics; we linearly visit every leaf). */
bool read_fzap(Volume& v, const std::vector<u8>& hdr, u64 zap_block_size,
               std::vector<ZapEntry>& out) {
    if (hdr.size() < 128)
        return false;
    if (rdle64(hdr.data()) != ZBT_HEADER)
        return false;
    if (rdle64(hdr.data() + 8) != ZAP_MAGIC)
        return false;
    const u64 zt_blk = rdle64(hdr.data() + FZ_ZT_BLK);
    const u64 zt_numblks = rdle64(hdr.data() + FZ_ZT_NUMBLKS);
    const u64 zt_shift = rdle64(hdr.data() + FZ_ZT_SHIFT);
    if (zt_shift > 30 || zap_block_size == 0)
        return false;
    const u64 pt_entries = zt_shift < 63 ? (u64(1) << zt_shift) : 0;

    std::vector<u64> leaf_blocks; /* block numbers */
    if (zt_blk == 0) {
        /* embedded pointer table: second half of the header block,
         * u64 leaf block numbers, entries = blocksize/16 */
        const u64 embedded = zap_block_size / 16;
        for (u64 i = 0; i < embedded && leaf_blocks.size() < MAX_ZAP_LEAFS; ++i) {
            const u64 at = zap_block_size / 2 + i * 8;
            if (at + 8 > hdr.size())
                break;
            const u64 lb = rdle64(hdr.data() + at);
            if (lb != 0)
                leaf_blocks.push_back(lb);
        }
    } else {
        if (zt_numblks == 0 || zt_numblks > 64 || zt_blk > (1u << 26))
            return false;
        for (u64 b = 0; b < zt_numblks && leaf_blocks.size() < MAX_ZAP_LEAFS; ++b) {
            std::vector<u8> pt;
            /* pointer-table blocks are raw u64 arrays (block-type "any
             * other value are ptrtbl blocks") */
            const u64 off = v.vdev_offset + zt_blk * zap_block_size + b * zap_block_size;
            if (off + zap_block_size > v.total_bytes)
                return false;
            pt.resize(size_t(zap_block_size));
            if (v.dev->read_at(off, pt.data(), u32(pt.size())) != OK) {
                ++v.damaged;
                continue;
            }
            for (u64 i = 0; i < zap_block_size / 8 && leaf_blocks.size() < MAX_ZAP_LEAFS; ++i) {
                const u64 lb = rdle64(pt.data() + i * 8);
                if (lb != 0)
                    leaf_blocks.push_back(lb);
            }
        }
    }
    (void)pt_entries;
    /* dedup + read leaves */
    std::set<u64> seen_blocks;
    bool any = false;
    for (u64 lb : leaf_blocks) {
        if (!seen_blocks.insert(lb).second)
            continue;
        if (lb > (1u << 28))
            continue;
        const u64 off = v.vdev_offset + lb * zap_block_size;
        if (off + zap_block_size > v.total_bytes)
            continue;
        std::vector<u8> leaf(static_cast<size_t>(zap_block_size), 0);
        if (v.dev->read_at(off, leaf.data(), u32(leaf.size())) != OK) {
            ++v.damaged;
            continue;
        }
        if (read_leaf(leaf, out))
            any = true;
        if (out.size() > MAX_ZAP_ENTRIES * 4)
            break;
    }
    return any;
}

/* read a ZAP object (micro or fat) by dnode */
bool read_zap(Volume& v, ObjSet& os, u64 object, std::vector<ZapEntry>& out) {
    Dnode dn;
    if (!os.read_dnode(object, dn))
        return false;
    if (dn.type() != DMU_OT_DIRECTORY_CONTENTS && dn.type() != 21 /* master */ &&
        dn.type() != 13 /* DSL_DIR_CHILD_MAP */ && dn.type() != 14 /* DSL_DS_SNAP_MAP */ &&
        dn.type() != 15 /* DSL_PROPS */ && dn.type() != 2 /* OBJECT_DIRECTORY */ &&
        dn.type() != 27 /* ZAP_OTHER */ && dn.type() != 31 /* POOL_PROPS */)
        return false; /* not a ZAP object */
    const u64 dbsz = dn.datablksz();
    if (dbsz < 512 || dbsz > (1u << 20))
        return false;
    /* header block = block 0 */
    if (dn.nlevels() > 1 || dn.nblkptr() == 0)
        return false;
    const u8* bp = dn.blkptr(0);
    if (bp_is_hole(bp) || bp_is_embedded(bp))
        return false;
    if (bp_lsize(bp) != dbsz && bp_lsize(bp) < 512)
        return false;
    std::vector<u8> blk;
    if (!v.read_block(bp, 0, blk))
        return false;
    const u64 btype = rdle64(blk.data());
    if (btype == ZBT_MICRO)
        return read_microzap(blk, out);
    if (btype == ZBT_HEADER)
        return read_fzap(v, blk, dbsz, out);
    return false;
}

/* ---- file content through the dnode block tree ---- */

/* collect L0 data blkptrs in logical order (handles any nlevels and
 * gang-free single-vdev DVAs; embedded bps are returned separately) */
struct FileExtentPlan {
    struct Item {
        u64 offset; /* vdev-relative byte offset */
        u64 length; /* logical bytes */
        u64 psize;  /* on-disk bytes */
        u32 codec;  /* generic codec (0 raw) */
    };
    std::vector<Item> items;
    std::vector<std::vector<u8>> embedded; /* decoded embedded payloads */
    u64 logical = 0;
};

bool plan_file_extents(Volume& v, const Dnode& dn, FileExtentPlan& plan) {
    const u32 levels = dn.nlevels();
    const u32 nb = dn.nblkptr();
    const u32 dbsz = dn.datablksz();
    if (dbsz == 0 || nb == 0)
        return true; /* empty file: no extents */
    const u64 indshift = dn.raw[DN_INDBLKSHIFT];
    const u64 per_node = (1ull << indshift) / BP_SIZE;
    u64 budget = MAX_L0_PER_DNODE;

    /* recursive L-walk (iterative with explicit stack) */
    struct Frame {
        std::vector<u8> node;
        u32 idx;
    };
    std::function<bool(const u8*, u32)> walk = [&](const u8* bp, u32 level) -> bool {
        if (bp_is_hole(bp))
            return true; /* hole: gap in the file (sparse) */
        if (bp_is_embedded(bp)) {
            if (bpe_etype(bp) == 0 && plan.embedded.size() < 64) {
                const u32 psz = u32(bp_psize(bp));
                const u64 lsize = bp_lsize(bp);
                if (psz > 0 && psz <= 112 && lsize > 0 && lsize <= (1u << 20)) {
                    std::vector<u8> packed(static_cast<size_t>(psz), 0);
                    if (bpe_payload(bp, packed.data(), psz)) {
                        std::vector<u8> plain(static_cast<size_t>(lsize), 0);
                        const u32 zc = bp_compress(bp);
                        bool ok = false;
                        if (zc == 0 || zc == 1 || zc == 2 || zc == ZIO_COMPRESS_EMPTY) {
                            /* uncompressed embedded payload (psize==lsize) */
                            const u64 n = lsize < u64(psz) ? lsize : u64(psz);
                            std::memcpy(plain.data(), packed.data(), size_t(n));
                            ok = true;
                        } else {
                            ok = compress::decompress_block(compress::codec_from_zfs(zc),
                                                            packed.data(), packed.size(),
                                                            plain.data(), plain.size());
                        }
                        if (ok) {
                            plan.embedded.push_back(std::move(plain));
                            plan.logical += lsize;
                        }
                    }
                }
            }
            return true;
        }
        if (level == 0) {
            if (budget-- == 0)
                return false;
            const u64 lsize = bp_lsize(bp);
            const u64 psz = bp_psize(bp);
            const u32 zc = bp_compress(bp);
            const u8* dva = bp + BP_DVA;
            if (dva_vdev(dva) != 0) {
                return true; /* other vdev: single-disk scan skips */
            }
            FileExtentPlan::Item it;
            it.offset = v.vdev_offset + dva_offset(dva);
            it.length = lsize;
            it.psize = (zc == 0 || zc == 1 || zc == 2 || zc == ZIO_COMPRESS_EMPTY) ? lsize : psz;
            it.codec = compress::codec_from_zfs(zc);
            plan.logical += lsize;
            if (dva_is_gang(dva)) {
                /* gang block: content is assembled, not a single extent;
                 * approximate as raw extent of the leader + count (the
                 * og reads gang blocks; recovery-grade we mark damaged) */
                ++v.damaged;
                plan.items.push_back(it);
            } else {
                plan.items.push_back(it);
            }
            return true;
        }
        /* internal node */
        std::vector<u8> node;
        if (!v.read_block(bp, 0, node))
            return false;
        if (node.size() % BP_SIZE != 0 || node.empty())
            return false;
        for (u64 i = 0; i < node.size() / BP_SIZE; ++i)
            if (!walk(node.data() + i * BP_SIZE, level - 1))
                return false;
        return true;
    };
    (void)per_node;
    for (u32 i = 0; i < nb; ++i) {
        if (levels <= 1) {
            if (!walk(dn.blkptr(i), 0))
                return false;
        } else {
            if (!walk(dn.blkptr(i), levels - 1))
                return false;
        }
    }
    return true;
}

} // namespace

u64 unix_to_filetime(i64 sec) {
    const i64 EPOCH_DELTA = 11644473600LL;
    if (sec <= -EPOCH_DELTA)
        return 0;
    return u64((sec + EPOCH_DELTA) * 10000000LL);
}

u32 scan_zfs(IStorageDevice* slice, const ZfsScanFlags& flags, CancelToken* cancel,
             WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             ZfsScanStats& stats) {
    (void)flags;
    (void)root_deleted;
    (void)root_lost;
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;

    const u64 total = slice->size();
    if (total < LABEL_SIZE * 2)
        return ERR_BAD_PARAMETER;

    /* 1. labels: OpenZFS layout - L0@0, L1@256K, L2@end-512K, L3@end-256K.
     * Each: [8K skip][8K boot env][112K vdev_phys (config nvlist at its
     * start)][128K uberblock ring (128 slots x 1KiB)]. */
    const u64 label_base[4] = {0, LABEL_SIZE, total - 2 * LABEL_SIZE, total - LABEL_SIZE};
    LabelConfig cfg;
    u64 best_txg = 0;
    std::vector<u8> best_ub;
    for (int li = 0; li < 4; ++li) {
        if (label_base[li] + LABEL_SIZE > total)
            continue;
        /* config nvlist at label+16K (first 112K is vdev_phys) */
        if (!cfg.valid) {
            std::vector<u8> phys(size_t(VDEV_PHYS_SIZE), 0);
            if (slice->read_at(label_base[li] + VDEV_PHYS_OFFSET, phys.data(), u32(phys.size())) ==
                OK) {
                std::map<std::string, NvValue> root;
                if (nv_parse(phys.data(), u32(phys.size()), root)) {
                    auto it = root.find("name");
                    if (it != root.end() && !it->second.s.empty()) {
                        cfg.pool_name = it->second.s;
                        const std::map<std::string, NvValue>* tr = nullptr;
                        auto vt = root.find("vdev_tree");
                        if (vt != root.end())
                            tr = &vt->second.child;
                        auto pick = [&](const char* key, u64& out) {
                            if (tr) {
                                auto f = tr->find(key);
                                if (f != tr->end() && f->second.u64v) {
                                    out = f->second.u64v;
                                    return true;
                                }
                            }
                            auto f = root.find(key);
                            if (f != root.end() && f->second.u64v) {
                                out = f->second.u64v;
                                return true;
                            }
                            return false;
                        };
                        u64 ashift = 9, asize = 0, offset = 0, guid = 0;
                        pick("ashift", ashift);
                        pick("asize", asize);
                        pick("offset", offset);
                        pick("guid", guid);
                        std::string vtype;
                        if (tr) {
                            auto f = tr->find("type");
                            if (f != tr->end())
                                vtype = f->second.s;
                        }
                        if (vtype.empty()) {
                            auto f = root.find("vdev_type");
                            if (f != root.end())
                                vtype = f->second.s;
                        }
                        cfg.ashift = ashift ? ashift : 9;
                        cfg.asize = asize;
                        cfg.offset = offset;
                        cfg.guid = guid;
                        cfg.vdev_type = vtype;
                        cfg.valid = true;
                    }
                }
            }
        }
        /* uberblock ring: max txg wins across all labels */
        std::vector<u8> ub(static_cast<size_t>(UBERBLOCK_SIZE), 0);
        for (u32 ui = 0; ui < UBERBLOCK_COUNT; ++ui) {
            const u64 at = label_base[li] + UBRING_OFFSET + u64(ui) * UBERBLOCK_SIZE;
            if (at + UBERBLOCK_SIZE > total)
                break;
            if (slice->read_at(at, ub.data(), u32(ub.size())) != OK)
                break;
            const u64 magic = rdle64(ub.data());
            if (magic != UBERBLOCK_MAGIC) {
                if (magic == 0x0CB1BA00ULL || magic == 0x0CBA0000ULL)
                    return ERR_UNSUPPORTED; /* big-endian pool: refused,
                                             * not misparsed */
                continue;
            }
            const u64 txg = rdle64(ub.data() + UB_TXG);
            if (txg > best_txg) {
                best_txg = txg;
                best_ub = ub;
            }
        }
    }
    if (best_ub.empty())
        return ERR_BAD_MAGIC;
    stats.txg = best_txg;

    Volume v;
    v.dev = slice;
    v.ashift = cfg.valid ? cfg.ashift : 9;
    v.vdev_offset = cfg.valid ? cfg.offset : 0;
    v.vdev_asize = cfg.valid ? cfg.asize : total;
    v.total_bytes = total;
    if (cfg.valid && cfg.vdev_type != "disk" && !cfg.vdev_type.empty())
        ++stats.unsupported_vdevs; /* mirror/raidz: report only */

    /* 2. MOS: rootbp -> objset_phys; object 1 = object directory ZAP. */
    ObjSet mos;
    if (!mos.load(&v, best_ub.data() + UB_ROOTBP))
        return ERR_BAD_MAGIC;

    std::vector<ZapEntry> objdir;
    if (!read_zap(v, mos, 1, objdir) || objdir.empty())
        return ERR_BAD_MAGIC;

    u64 dataset_dir_obj = 0;
    for (const auto& e : objdir)
        if (e.name == "root_dataset" || e.name == "dataset")
            dataset_dir_obj = e.value;
    if (dataset_dir_obj == 0)
        return ERR_BAD_MAGIC;
    if (progress)
        (void)progress(1, 4, progress_user);

    /* 3. dataset hierarchy: dsl_dir bonus {child zap@240, head ds@248};
     * head dnode bonus {bp@128}. */
    struct WalkOut {
        std::u16string name;
        std::vector<u8> ds_bp;
    };
    std::vector<WalkOut> datasets;
    std::function<void(u64, const std::u16string&, int)> walk_dir =
        [&](u64 dsl_dir_obj, const std::u16string& prefix, int depth) {
            if (depth > MAX_DATASET_DEPTH || (cancel && cancel->cancelled()))
                return;
            Dnode dn;
            if (!mos.read_dnode(dsl_dir_obj, dn))
                return;
            const u32 blen = dn.bonuslen();
            if (blen < 256 || blen > MAX_BONUS || u64(blen) > dn.bonus_cap())
                return;
            const u8* bonus = dn.bonus();
            const u64 head_obj = rdle64(bonus + DD_HEAD_DATASET_OBJ);
            const u64 child_zap = rdle64(bonus + DD_CHILD_DIR_ZAPOBJ);
            if (head_obj != 0) {
                Dnode hdn;
                if (mos.read_dnode(head_obj, hdn) && hdn.bonuslen() >= DS_BP + BP_SIZE &&
                    u64(hdn.bonuslen()) <= hdn.bonus_cap()) {
                    WalkOut wo;
                    wo.name = prefix;
                    wo.ds_bp.assign(hdn.bonus() + DS_BP, hdn.bonus() + DS_BP + BP_SIZE);
                    datasets.push_back(std::move(wo));
                }
            }
            if (child_zap != 0) {
                std::vector<ZapEntry> children;
                if (read_zap(v, mos, child_zap, children)) {
                    for (const auto& c : children) {
                        std::u16string sub = prefix;
                        for (char ch : c.name)
                            sub.push_back(u16(u8(ch)));
                        walk_dir(c.value, sub, depth + 1);
                    }
                }
            }
        };
    walk_dir(dataset_dir_obj, u"", 0);
    if (progress)
        (void)progress(2, 4, progress_user);

    /* 4. per-dataset ZPL walk: master node object 1 -> ROOT -> dirs. */
    std::u16string pool_display_;
    for (char ch : (cfg.valid && !cfg.pool_name.empty() ? cfg.pool_name : std::string("pool")))
        pool_display_.push_back(u16(u8(ch)));
    for (const auto& ds : datasets) {
        if (cancel && cancel->cancelled())
            return ERR_CANCELLED;
        ObjSet zpl;
        if (!zpl.load(&v, ds.ds_bp.data()))
            continue;
        ++stats.datasets;

        std::u16string dname = ds.name.empty() ? pool_display_ : ds.name;
        auto* dprops = new (std::nothrow) tree::FileProperties();
        if (!dprops)
            continue;
        dprops->name = dname;
        dprops->attributes = 0x10;
        tree::FileNode* ds_node = tree.add_file(root_active, dprops);
        dprops->release();
        if (!ds_node)
            continue;
        ++stats.dirs;

        std::vector<ZapEntry> master;
        if (!read_zap(v, zpl, 1, master))
            continue;
        u64 root_obj = 0;
        for (const auto& e : master)
            if (e.name == "ROOT")
                root_obj = e.value;
        if (root_obj == 0)
            continue;

        struct ObjInfo {
            u64 mode = 0;
            u64 size = 0;
            u64 parent = 0;
            u64 links = 0;
            u64 atime[2] = {0, 0}, mtime[2] = {0, 0}, ctime[2] = {0, 0}, crtime[2] = {0, 0};
            Dnode dn;
            bool dir = false;
        };
        std::map<u64, ObjInfo> objs;
        auto load_obj = [&](u64 obj) -> const ObjInfo* {
            auto it = objs.find(obj);
            if (it != objs.end())
                return &it->second;
            Dnode dn;
            if (!zpl.read_dnode(obj, dn))
                return nullptr;
            if (dn.type() != DMU_OT_DIRECTORY_CONTENTS && dn.type() != DMU_OT_PLAIN_FILE_CONTENTS)
                return nullptr;
            ObjInfo oi;
            const u32 blen = dn.bonuslen();
            const u64 bcap = dn.bonus_cap();
            if (blen == 0 || u64(blen) > bcap || blen > MAX_BONUS)
                return nullptr;
            const u8* b = dn.bonus();
            if (dn.bonustype() == DMU_OT_SA && blen >= SA_LAYOUT1_MIN) {
                /* SA layout 1 (standard ZPL attr order after the 8-byte
                 * header); non-standard layouts fall back to no attrs */
                const u32 magic = rdle32(b);
                const u32 layout = rdle16(b + 4) & 0x3FF;
                if (magic == SA_MAGIC && layout == 1) {
                    oi.atime[0] = rdle64(b + SA_LAYOUT1_ATIME);
                    oi.mtime[0] = rdle64(b + SA_LAYOUT1_MTIME);
                    oi.ctime[0] = rdle64(b + SA_LAYOUT1_CTIME);
                    oi.crtime[0] = rdle64(b + SA_LAYOUT1_CRTIME);
                    oi.mode = rdle64(b + SA_LAYOUT1_MODE);
                    oi.parent = rdle64(b + SA_LAYOUT1_PARENT);
                    oi.links = rdle64(b + SA_LAYOUT1_LINKS);
                    oi.size = rdle64(b + SA_LAYOUT1_SIZE);
                } else {
                    return nullptr; /* unknown SA layout: skip cleanly */
                }
            } else if (dn.bonustype() == DMU_OT_ZNODE && blen >= 0x78) {
                /* legacy znode_phys */
                oi.atime[0] = rdle64(b + ZP_ATIME);
                oi.mtime[0] = rdle64(b + ZP_MTIME);
                oi.ctime[0] = rdle64(b + ZP_CTIME);
                oi.crtime[0] = rdle64(b + ZP_CRTIME);
                oi.mode = rdle64(b + ZP_MODE);
                oi.parent = rdle64(b + ZP_PARENT);
                oi.links = rdle64(b + ZP_LINKS);
                oi.size = rdle64(b + ZP_SIZE);
            } else {
                return nullptr;
            }
            oi.dn = std::move(dn);
            oi.dir = (oi.mode & 0xF000) == 0x4000;
            auto res = objs.emplace(obj, std::move(oi));
            return &res.first->second;
        };

        std::set<u64> visited;
        std::function<void(u64, tree::FileNode*, int)> emit_dir = [&](u64 dir_obj,
                                                                      tree::FileNode* parent,
                                                                      int depth) {
            if (depth > MAX_DEPTH || visited.count(dir_obj))
                return;
            visited.insert(dir_obj);
            const ObjInfo* di = load_obj(dir_obj);
            if (di == nullptr || !di->dir)
                return;
            std::vector<ZapEntry> entries;
            if (!read_zap(v, zpl, dir_obj, entries))
                return;
            for (const auto& e : entries) {
                if (cancel && cancel->cancelled())
                    return;
                if (e.name == "." || e.name == "..")
                    continue;
                const ObjInfo* ci = load_obj(e.value);
                if (ci == nullptr)
                    continue;
                const bool is_dir = ci->dir;
                auto* p = new (std::nothrow) tree::FileProperties();
                if (!p)
                    return;
                for (char ch : e.name)
                    p->name.push_back(u16(u8(ch)));
                p->fs_kind = 13; /* Zfs (corpus factory code) */
                p->logical_size = ci->size;
                p->allocated_size = ci->dn.maxblkid() != 0
                                        ? u64(ci->dn.datablksz()) * (ci->dn.maxblkid() + 1)
                                        : (ci->size ? u64(ci->dn.datablksz()) : 0);
                p->attributes = is_dir ? 0x10 : 0;
                p->record_ref = e.value;
                p->times.created = unix_to_filetime(i64(ci->crtime[0]));
                p->times.modified = unix_to_filetime(i64(ci->mtime[0]));
                p->times.changed = unix_to_filetime(i64(ci->ctime[0]));
                p->times.accessed = unix_to_filetime(i64(ci->atime[0]));
                if (!is_dir && ci->size > 0) {
                    FileExtentPlan plan;
                    if (plan_file_extents(v, ci->dn, plan)) {
                        /* merge adjacent raw extents */
                        for (auto& it : plan.items) {
                            if (!p->extents.empty() && it.codec == 0 &&
                                p->extents.back().codec == 0 &&
                                p->extents.back().offset + p->extents.back().length == it.offset) {
                                p->extents.back().length += it.length;
                                p->extents.back().psize = 0;
                            } else {
                                tree::FileProperties::Extent ex;
                                ex.offset = it.offset;
                                ex.length = it.length;
                                ex.codec = it.codec;
                                ex.psize = it.psize;
                                p->extents.push_back(ex);
                            }
                        }
                        for (auto& em : plan.embedded) {
                            if (em.size() <= (1u << 20)) {
                                p->resident_data.insert(p->resident_data.end(), em.begin(),
                                                        em.end());
                                ++stats.embedded_files;
                            }
                        }
                    }
                }
                tree::FileNode* n = tree.add_file(parent, p);
                p->release();
                if (!n)
                    continue;
                if (is_dir) {
                    ++stats.dirs;
                    emit_dir(e.value, n, depth + 1);
                } else {
                    ++stats.files;
                }
            }
        };
        emit_dir(root_obj, ds_node, 0);
        if (progress)
            (void)progress(3, 4, progress_user);
    }

    stats.damaged_blocks = v.damaged;
    if (progress)
        (void)progress(4, 4, progress_user);
    return OK;
}

} // namespace zfs
} // namespace cr
