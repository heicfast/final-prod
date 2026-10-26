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
 * Deleted-file story (S-034, DFX30): ZFS is CoW with copy-on-everything
 * semantics. The PREMISE this engine was built on — "the og reads the
 * newest txg's live state only" — is REFUTED by first-hand decode of the
 * og's quick-searcher run loop (ZfsSearchDLL FUN_180006b90 :5685-5841
 * iterates EVERY uberblock slot in the label-0 ring and walks each txg's
 * tree through a per-txg VdevLoc::UberData registry, FUN_180003150/
 * FUN_180017310, emitting ZplNodes per state). This engine currently
 * walks the newest txg only — a REGISTERED GAP (diff-to-fix findings/
 * SEVERE/S-034) until the fix lands; deleted content meanwhile surfaces
 * via RAW carving only (a strict subset of the og's ring-window recall).
 *
 * Big-endian pools (SPARC/POWER writers) are detected and refused with a
 * clear error code rather than misparsed (bit 63 of blk_prop marks the
 * writer's byteorder; the uberblock magic reads reversed on BE pools).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/zfs/cr_zfs_engine.h"
#include "cr/cr_bits.h" /* shared byte readers (W4 dedup, DC-12) */
#include "cr/cr_text.h" /* S-005: on-disk UTF-8 names decode, not byte-lift */
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
constexpr u32 UB_MAGIC = 0;
constexpr u32 UB_TXG = 16;
constexpr u32 UB_ROOTBP = 40;        /* blkptr 128 bytes */
constexpr u64 UBERBLOCK_SIZE = 1024; /* smallest slot; larger slots only
                                      * use the first 1KiB for these fields */
constexpr u64 UBERBLOCK_MAGIC = 0x00BAB10CULL;

/* ---- blkptr (OpenZFS spa.h) ----
 * blk_dva[3] @0 (16B each), blk_prop u64@48, blk_pad u64@56/@64,
 * blk_phys_birth@72, blk_birth@80, blk_fill@88, blk_cksum@96..127.
 * prop: lsize bits0-24 ((v+1)<<9), psize bits25-31 ((v+1)<<9),
 * comp bits32-38 (7), embedded bit39; NON-embedded: checksum bits40-47
 * (the og's verify gate reads exactly this field, ZfsSearchDLL
 * :18010 uVar22 = prop>>0x28 & 0xff); embedded: etype bits40-47.
 * byteorder bit63. DVA (spa.h DVA_GET_*): vdev = (w0>>32) & 0xFFFFFF
 * (SPA_VDEVBITS=24), asize = (w0 & 0xFFFFFF)<<9,
 * offset = (w1 & 2^63-1)<<9, gang = w1 bit63. */
constexpr u32 BP_DVA = 0;
constexpr u32 BP_PROP = 48;
constexpr u32 BP_CKSUM = 96; /* blk_cksum (zio_cksum_t, 32 bytes) */
constexpr u32 BP_BIRTH = 72;
constexpr u32 BP_SIZE = 128;

constexpr u32 ZIO_COMPRESS_LZJB = 3;
constexpr u32 ZIO_COMPRESS_EMPTY = 4;
constexpr u32 ZIO_COMPRESS_ZLE = 14;
constexpr u32 ZIO_COMPRESS_LZ4 = 15;
constexpr u32 ZIO_COMPRESS_ZSTD = 16;

bool bp_is_embedded(const u8* bp) {
    return (le64(bp + BP_PROP) >> 39) & 1;
}
bool bp_is_hole(const u8* bp) {
    /* all three DVAs zero (spa.h BP_IS_HOLE) */
    for (int i = 0; i < 48; ++i)
        if (bp[i] != 0)
            return false;
    return true;
}
u64 bp_lsize(const u8* bp) {
    const u64 prop = le64(bp + BP_PROP);
    if (bp_is_embedded(bp)) /* bytes, bits 0-24, +1 (BPE_GET_LSIZE) */
        return (prop & 0x1FFFFFFull) + 1;
    return ((prop & 0xFFFFull) + 1) << 9; /* sectors-biased */
}
u64 bp_psize(const u8* bp) {
    const u64 prop = le64(bp + BP_PROP);
    if (bp_is_embedded(bp)) /* bytes, bits 25-31, +1 */
        return ((prop >> 25) & 0x7Full) + 1;
    return (((prop >> 16) & 0xFFFFull) + 1) << 9;
}
u32 bp_compress(const u8* bp) {
    /* SPA_COMPRESSBITS = 7: bit 39 is the embedded flag, not compress */
    return u32((le64(bp + BP_PROP) >> 32) & 0x7F);
}
u32 bp_checksum(const u8* bp) {
    /* checksum type: prop bits 40-47 (spa.h BP_GET_CHECKSUM; the og reads
     * exactly this field for its verify gate, ZfsSearchDLL :18010) */
    return u32((le64(bp + BP_PROP) >> 40) & 0xFF);
}
u32 bpe_etype(const u8* bp) {
    return u32((le64(bp + BP_PROP) >> 40) & 0xFF);
}
u64 dva_vdev(const u8* dva) {
    return (le64(dva) >> 32) & 0xFFFFFFull;
}
u64 dva_asize(const u8* dva) {
    return (le64(dva) & 0xFFFFFFull) << 9;
}
u64 dva_offset(const u8* dva) {
    return (le64(dva + 8) & 0x7FFFFFFFFFFFFFFFull) << 9;
}
bool dva_is_gang(const u8* dva) {
    return (le64(dva + 8) >> 63) & 1;
}

/* M-019: fletcher4 over LE u32 words - byte-exact port of the og's
 * interleaved accumulator form (ZfsSearchDLL :18093-18111: two words per
 * iteration, D split across two partial sums, single-word tail; final
 * D = tail-C + D1 + D2). Mathematically the standard four-accumulator
 * fletcher4: A += w; B += A; C += B; D += C (mod 2^64). The og computes
 * it over the RAW (on-disk, pre-decompression) bytes and compares the
 * 32-byte result against blk_cksum at bp+0x60 (:18117 memcmp 0x20). */
void fletcher4(const u8* p, size_t len, u64 out[4]) {
    u64 a = 0, b = 0, c = 0, d1 = 0, d2 = 0, d_tail = 0;
    const size_t words = len >> 2;
    size_t i = 0;
    auto rdw = [&](size_t k) {
        u32 w;
        std::memcpy(&w, p + k * 4, 4); /* LE host + LE pool = LE words */
        return w;
    };
    if (words >= 2) {
        for (; i + 1 < words; i += 2) {
            const u32 w0 = rdw(i), w1 = rdw(i + 1);
            b = a + b + w0;
            c = c + b;
            d1 = d1 + c;
            a = a + w1 + w0;
            b = b + a;
            c = c + b;
            d2 = d2 + c;
        }
    }
    if (i < words) {
        a = a + rdw(i);
        b = b + a;
        c = c + b;
        d_tail = c;
    }
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d_tail + d1 + d2;
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
        const u64 w = le64(bp + u64(word) * 8);
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
constexpr u32 DN_COMPRESS = 6;
constexpr u32 DN_FLAGS = 7;
constexpr u32 DN_DATABLKSZSEC = 8;
constexpr u32 DN_BONUSLEN = 10;
constexpr u32 DN_EXTRA_SLOTS = 12;
constexpr u32 DN_MAXBLKID = 16;
constexpr u32 DN_BLKPTR = 64;
constexpr u32 DN_SIZE = 512;

constexpr u32 DMU_OT_OBJECT_DIRECTORY = 1;
constexpr u32 DMU_OT_DNODE = 10;
constexpr u32 DMU_OT_OBJSET = 11;
constexpr u32 DMU_OT_DSL_DIR_CHILD_MAP = 13;
constexpr u32 DMU_OT_DSL_DS_SNAP_MAP = 14;
constexpr u32 DMU_OT_DSL_PROPS = 15;
constexpr u32 DMU_OT_ZNODE = 17; /* legacy bonus (zfs_sa.h) */
constexpr u32 DMU_OT_PLAIN_FILE_CONTENTS = 19;
constexpr u32 DMU_OT_DIRECTORY_CONTENTS = 20;
constexpr u32 DMU_OT_MASTER_NODE = 21;
constexpr u32 DMU_OT_ERROR_LOG = 28;
constexpr u32 DMU_OT_POOL_PROPS = 31;
constexpr u32 DMU_OT_DDT_ZAP = 42;
constexpr u32 DMU_OT_DDT_STATS = 43;
constexpr u32 DMU_OT_SA = 44; /* SA bonus (dmu.h enum; 43 is DDT_STATS) */
constexpr u32 DMU_OT_SA_MASTER_NODE = 45;
constexpr u32 DMU_OT_SA_ATTR_REGISTRATION = 46;
constexpr u32 DMU_OT_SA_ATTR_LAYOUTS = 47;

bool zap_typed(u32 t) {
    /* every DMU_OT whose content is a ZAP (dmu.h "/ * ZAP * /" rows) */
    switch (t) {
    case DMU_OT_OBJECT_DIRECTORY:
    case DMU_OT_DSL_DIR_CHILD_MAP:
    case DMU_OT_DSL_DS_SNAP_MAP:
    case DMU_OT_DSL_PROPS:
    case DMU_OT_DIRECTORY_CONTENTS:
    case DMU_OT_MASTER_NODE:
    case DMU_OT_ERROR_LOG:
    case DMU_OT_POOL_PROPS:
    case DMU_OT_DDT_ZAP:
    case DMU_OT_DDT_STATS:
    case DMU_OT_SA_MASTER_NODE:
    case DMU_OT_SA_ATTR_REGISTRATION:
    case DMU_OT_SA_ATTR_LAYOUTS:
    case 27: /* ZAP_OTHER */
        return true;
    default:
        return false;
    }
}

/* legacy znode_phys (OpenZFS include/sys/zfs_sa.h - field order verified
 * against master: zp_atime@0 .. zp_zap@144, ACL at 176) */
constexpr u32 ZP_ATIME = 0x00; /* sec+usec pairs x4 */
constexpr u32 ZP_MTIME = 0x10;
constexpr u32 ZP_CTIME = 0x20;
constexpr u32 ZP_CRTIME = 0x30;
constexpr u32 ZP_MODE = 0x48;
constexpr u32 ZP_SIZE = 0x50;
constexpr u32 ZP_PARENT = 0x58;
constexpr u32 ZP_LINKS = 0x60;
constexpr u32 ZP_MIN = 0x68; /* through links */

/* SA header (OpenZFS include/sys/sa_impl.h): magic u32@0 = 0x2F505A,
 * layout_info u16@4 (bits 0-9 layout, bits 10-15 hdrsize/8), then the
 * attribute data. Layout 1 attr order = the zfs_attr_table registration
 * order (module/zfs/zfs_sa.c): atime mtime ctime crtime gen mode size
 * parent links xattr rdev flags uid gid - offsets below are relative to
 * the SA HEADER start (8-byte header + body). */
constexpr u32 SA_MAGIC = 0x2F505A;
constexpr u32 SA_LAYOUT1_ATIME = 8;
constexpr u32 SA_LAYOUT1_MTIME = 24;
constexpr u32 SA_LAYOUT1_CTIME = 40;
constexpr u32 SA_LAYOUT1_CRTIME = 56;
constexpr u32 SA_LAYOUT1_GEN = 72;
constexpr u32 SA_LAYOUT1_MODE = 80;
constexpr u32 SA_LAYOUT1_SIZE = 88;
constexpr u32 SA_LAYOUT1_PARENT = 96;
constexpr u32 SA_LAYOUT1_LINKS = 104;
constexpr u32 SA_LAYOUT1_XATTR = 112;
constexpr u32 SA_LAYOUT1_RDEV = 120;
constexpr u32 SA_LAYOUT1_FLAGS = 128;
constexpr u32 SA_LAYOUT1_UID = 136;
constexpr u32 SA_LAYOUT1_GID = 144;
constexpr u32 SA_LAYOUT1_MIN = 112; /* header + body through links */

/* ---- objset_phys ---- */
constexpr u32 OS_META_DNODE = 0;

/* ---------------------------------------------------------------- */
/* M-038: SA attribute registry (per dataset).                       */
/* The og resolves system attributes BY NAME through the on-disk     */
/* registry (ZfsSearchDLL: "ZPL_PARENT" :24324, "ZPL_ATIME" :24441,  */
/* "ZPL_MTIME" :24563, "ZPL_CTIME" :24685, "ZPL_CRTIME" :24807,      */
/* "ZPL_SIZE" :26449), and decodes every layout through the layouts  */
/* map built from the LAYOUTS object (:23251; the object decode at   */
/* :17636 does a uniform map lookup and ZEROES the attrs on a miss   */
/* - the object still surfaces). Structure (the og's three-object    */
/* chain, :22934-23140): master node ZAP "SA_ATTRS" -> object; that  */
/* object's ZAP holds "REGISTRY" -> the attribute registry and       */
/* "LAYOUTS" -> the layouts object. Registry entries: attribute      */
/* name -> { bits 0-15 attr number, bits 16-23 length in bytes }     */
/* (the og's decode: (short)value / (byte)(value>>0x10) :23170-80).  */
/* Layout entries: decimal-string layout number -> u16 attr-number   */
/* array (OpenZFS sa.c sa_load_layout; og :23190-23250 stride-2 BE   */
/* walk - LE pool values read LE here).                              */
/* ---------------------------------------------------------------- */
struct SaRegistry {
    bool valid = false;
    std::map<u16, u8> len_by_num;               /* attr number -> length */
    std::map<u16, std::string> name_by_num;     /* attr number -> name */
    std::map<u64, std::vector<u16>> layouts;    /* layout number -> ordered slots */

    bool has_layout(u64 n) const { return layouts.count(n) != 0; }
};

/* ---- dsl_dir / dsl_dataset bonus (OpenZFS include/sys/dsl_dir.h:
 * dsl_dir_phys_s {creation_time@0, head_dataset_obj@8, origin_obj@16,
 * child_dir_zapobj@24, ...}; dsl_dataset.h: ds_bp@128) ---- */
constexpr u32 DD_HEAD_DATASET_OBJ = 8;
constexpr u32 DD_CHILD_DIR_ZAPOBJ = 24;
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

/* nvpair on-disk encodings (OpenZFS module/nvpair/nvpair.c + spl-xdr.c).
 * Packed buffer = {nvs_header_t: encoding u8, endian u8, rsvd x2} then
 * the stream. Vdev-label config nvlists are ALWAYS XDR
 * (vdev_label.c: nvlist_pack(..., NV_ENCODE_XDR, ...)) - big-endian.
 * XDR pair: {enc_len BE32, dec_len BE32, name_len BE32 (excl NUL,
 * spl-xdr xdr_enc_string), name bytes pad4, type BE32, nelem BE32, value};
 * dec_len==0 terminates. XDR strings: {BE32 len excl NUL, bytes, pad4}.
 * Native pair (LE pools / in-core images): {size i32, name_sz i16 (incl
 * NUL), value_elem i32, type i32, name[name_sz], pad8, value}; size==0
 * terminates. */
constexpr u32 NV_DATA_BOOLEAN = 1;
constexpr u32 NV_DATA_UINT64 = 8;
constexpr u32 NV_DATA_STRING = 9;
constexpr u32 NV_DATA_NVLIST = 19;
constexpr u32 NV_DATA_BOOLEAN_VALUE = 21;

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

/* Parse the pair stream AFTER the {version, nvflag} words (nvpair.c
 * nvs_operation: nvs_nvlist consumes version+nvflag, then pairs, then
 * the terminator). `xdr` is inherited from the parent buffer - embedded
 * nvlists use the same encoding (nvs_embedded -> same nvs ops). */
bool nv_parse_stream(const u8* p, u32 len, bool xdr, std::map<std::string, NvValue>& out,
                     u32 depth) {
    if (p == nullptr || len < 8 || depth > 6)
        return false;
    auto rd32 = [&](const u8* q) -> u32 { return xdr ? be32(q) : le32(q); };
    auto rd64 = [&](const u8* q) -> u64 { return xdr ? be64(q) : le64(q); };
    const auto align = [&](u32 v, u32 a) -> u32 { return (v + a - 1) & ~(a - 1); };
    u32 pos = 8; /* skip {version, nvflag} */
    u32 pairs = 0;
    while (pos + 8 <= len && pairs < 512) {
        ++pairs;
        u32 enc = rd32(p + pos);
        if (enc == 0)
            break; /* terminator pair (XDR: dec_len==0 next; both zero) */
        if (enc < 8 || pos + enc > len)
            break;
        u32 q = pos + 8; /* {enc_len, dec_len} consumed */
        u32 name_len = 0;
        std::string name;
        if (xdr) {
            if (q + 4 > len)
                break;
            name_len = rd32(p + q); /* xdr_string: length EXCLUDES the NUL */
            q += 4;
            if (name_len == 0 || name_len > 128 || q + name_len > len)
                break;
            name.assign(reinterpret_cast<const char*>(p + q), name_len);
            q = align(q + name_len, 4);
        } else {
            /* native: {size i32@0, name_sz i16@4, value_elem i32@8, type
             * i32@12, name[name_sz incl NUL]@16, value 8-aligned} */
            if (q + 2 > len)
                break;
            const u32 nsz = le16(p + pos + 4);
            if (nsz < 2 || nsz > 128 || pos + 16 + nsz > len)
                break;
            name.assign(reinterpret_cast<const char*>(p + pos + 16), nsz - 1);
            const u32 vty = le32(p + pos + 12);
            u32 vq = align(pos + 16 + nsz, 8);
            NvValue v;
            v.type = vty;
            const u32 vsz = le32(p + pos + 8);
            if (vty == NV_DATA_UINT64 && vsz == 1 && vq + 8 <= len) {
                v.u64v = le64(p + vq);
            } else if (vty == NV_DATA_STRING && vsz == 1 && vq + 4 <= len) {
                const u32 slen = le32(p + vq);
                if (slen > 0 && slen < 256 && vq + 4 + slen <= len)
                    v.s.assign(reinterpret_cast<const char*>(p + vq + 4), slen - 1);
            } else if (vty == NV_DATA_NVLIST && vsz == 1) {
                nv_parse_stream(p + vq, (pos + enc) - vq, false, v.child, depth + 1);
            }
            out[name] = v;
            pos += enc;
            continue;
        }
        if (q + 8 > len)
            break;
        const u32 type = rd32(p + q);
        const u32 nelem = rd32(p + q + 4);
        q += 8;
        NvValue v;
        v.type = type;
        if (type == NV_DATA_UINT64 && nelem == 1 && q + 8 <= len) {
            v.u64v = rd64(p + q);
        } else if (type == NV_DATA_STRING && nelem == 1 && q + 4 <= len) {
            const u32 slen = rd32(p + q);
            if (slen < 256 && q + 4 + slen <= len)
                v.s.assign(reinterpret_cast<const char*>(p + q + 4), slen);
        } else if (type == NV_DATA_BOOLEAN_VALUE && nelem == 1 && q + 4 <= len) {
            v.u64v = rd32(p + q);
        } else if (type == NV_DATA_NVLIST && nelem >= 1 && q + 8 <= len) {
            /* embedded nvlist: bounded by this pair; the outer loop
             * continues at pos+enc (pairs after vdev_tree are parsed) */
            nv_parse_stream(p + q, (pos + enc) - q, xdr, v.child, depth + 1);
        }
        out[name] = v;
        pos += enc; /* enc_len covers the whole pair incl. value */
    }
    return !out.empty();
}

/* Top level: packed buffer = {nvs_header_t: encoding u8, endian u8, rsvd x2}
 * (nvpair.c:2389-2395), then the stream. */
bool nv_parse(const u8* p, u32 len, std::map<std::string, NvValue>& out, u32 depth = 0) {
    if (p == nullptr || len < 12 || depth > 6)
        return false;
    const u32 enc_method = p[0];
    if (enc_method > 1)
        return false;
    return nv_parse_stream(p + 4, len - 4, enc_method == 1, out, depth);
}

/* ---- volume context ---- */
struct Volume {
    IStorageDevice* dev = nullptr;
    u64 ashift = 9;
    u64 vdev_offset = 0; /* data-area byte offset of vdev 0 */
    u64 vdev_asize = 0;
    u64 total_bytes = 0;
    u64 damaged = 0; /* failed block reads */
    u64 foreign_vdev_reads = 0; /* M-019: DVAs naming another vdev id,
                                 * served through the og's first-entry
                                 * fallback (the scanned disk) */
    u64 checksum_drops = 0; /* M-019: blocks dropped after fletcher4
                             * mismatch on every DVA copy */
    u64 microzap_entries = 0;
    u64 macrozap_entries = 0;

    /* read the raw (possibly compressed) bytes a DVA points at.
     * M-019: the og NEVER refuses a DVA on vdev id - it resolves the
     * vdev registry and, on no match, falls back to the FIRST registry
     * entry, which is the scanned disk (FUN_1800168b0 :19544-19556:
     * loop ends -> take *(param_1+0x18), the registry head). With a
     * single scanned stream every DVA therefore reads from this device;
     * the fletcher4 gate in read_block rejects the garbage a fallback
     * read can produce (mirror members read correct content; stripe
     * members get checksum-detected failures -> next DVA -> drop). */
    bool read_raw(const u8* bp, u32 dva_index, std::vector<u8>& out) {
        const u8* dva = bp + BP_DVA + u64(dva_index) * 16;
        if (dva_vdev(dva) != 0)
            ++foreign_vdev_reads; /* the og's first-entry fallback path */
        if (dva_is_gang(dva))
            return false; /* gang leader: read_gang handles it */
        const u64 off = dva_offset(dva);
        const u64 asz = dva_asize(dva);
        const u64 psz = bp_psize(bp);
        if (psz == 0 || asz == 0 || psz > asz)
            return false; /* zero/short DVA slot: natural retry bound */
        if (vdev_offset + off + psz > total_bytes)
            return false; /* bounds guard (safety hardening; the og's
                           * device layer fails the same read) */
        out.resize(size_t(psz));
        if (dev->read_at(vdev_offset + off, out.data(), u32(out.size())) != OK) {
            ++damaged;
            return false;
        }
        return true;
    }

    /* M-019: the og retries the NEXT DVA copy when a read or the
     * checksum gate fails (ZfsSearchDLL :18124-131: index < 2 ->
     * FUN_180014360(..., index + 1); after the last copy the block is
     * dropped). A failed read reaches the same retry in the og: the
     * empty buffer fails the checksum memcmp unless the stored checksum
     * is all-zero (an unobservable-on-real-pools quirk noted here, not
     * replicated). */
    bool read_block(const u8* bp, u32 dva_index, std::vector<u8>& out) {
        /* gang leaders assemble through the 512B header */
        if (dva_is_gang(bp + BP_DVA + u64(dva_index) * 16))
            return read_gang(bp + BP_DVA + u64(dva_index) * 16, out);
        std::vector<u8> raw;
        if (!read_raw(bp, dva_index, raw))
            return dva_index < 2 ? read_block(bp, dva_index + 1, out) : false;
        /* fletcher4 gate: checksum types {7, 0} are verified against
         * blk_cksum over the RAW bytes (the og's exact gate,
         * :18076 `(uVar22 == 7) || (uVar22 == 0)`; other types pass
         * unverified - og parity). */
        const u32 zc = bp_checksum(bp);
        if (zc == 7 || zc == 0) {
            u64 ck[4];
            fletcher4(raw.data(), raw.size(), ck);
            if (std::memcmp(bp + BP_CKSUM, ck, 32) != 0) {
                ++checksum_drops;
                return dva_index < 2 ? read_block(bp, dva_index + 1, out) : false;
            }
        }
        const u32 zio_c = bp_compress(bp);
        if (zio_c == 0 || zio_c == 1 || zio_c == 2 || zio_c == ZIO_COMPRESS_EMPTY) {
            /* off/on/inherit/empty: no transform (empty = zero block) */
            if (zio_c == ZIO_COMPRESS_EMPTY)
                std::memset(raw.data(), 0, raw.size());
            out = std::move(raw);
            return true;
        }
        const u64 lsize = bp_lsize(bp);
        if (lsize == 0 || lsize > (256ull << 20))
            return false;
        out.resize(size_t(lsize));
        const u32 codec = compress::codec_from_zfs(zio_c);
        if (!compress::decompress_block(codec, raw.data(), raw.size(), out.data(), out.size())) {
            ++damaged;
            return false;
        }
        return true;
    }

    /* gang block: the leader DVA (gang bit set) points to a 512-byte
     * "gang header" whose data, concatenated, forms the block. The header
     * is SPA_MINBLOCKSIZE (512) bytes holding (512 - sizeof(zio_eck_t)) /
     * sizeof(blkptr_t) = 480/128 = 3 blkptrs (zio.h zio_gbh_phys_t - the
     * 32-byte self-checksum occupies the tail; there is NO 4th slot). */
    bool read_gang(const u8* dva, std::vector<u8>& out, u32 depth = 0) {
        if (depth > 4)
            return false; /* nested-gang cap */
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
        for (u32 i = 0; i < 3; ++i) {
            const u8* gbp = hdr.data() + u64(i) * BP_SIZE;
            if (bp_is_hole(gbp))
                continue; /* terminator or padding */
            if (bp_is_embedded(gbp))
                continue;
            const u8* gdva = gbp + BP_DVA;
            if (dva_is_gang(gdva)) {
                std::vector<u8> nested;
                if (!read_gang(gdva, nested, depth + 1))
                    return false;
                out.insert(out.end(), nested.begin(), nested.end());
                continue;
            }
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
    u32 datablksz() const { return u32(le16(raw.data() + DN_DATABLKSZSEC)) * 512; }
    u32 bonuslen() const { return le16(raw.data() + DN_BONUSLEN); }
    u32 extra_slots() const { return raw[DN_EXTRA_SLOTS]; }
    u64 maxblkid() const { return le64(raw.data() + DN_MAXBLKID); }
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
        const u64 dbsz = meta.datablksz();
        if (dbsz == 0)
            return false;
        std::vector<u8> data;
        if (!read_object_block(meta, logical / dbsz, data))
            return false;
        const u64 in_block = logical % dbsz;
        if (in_block + DN_SIZE > data.size())
            return false;
        out.raw.assign(data.begin() + ptrdiff_t(in_block),
                       data.begin() + ptrdiff_t(in_block + DN_SIZE));
        return out.nblkptr() > 0 && out.datablksz() > 0;
    }

    /* Read logical block `blk` of a dnode through its indirect tree. */
    bool read_object_block(const Dnode& dn, u64 blk, std::vector<u8>& data) {
        const u32 levels = dn.nlevels();
        const u64 indshift = dn.raw[DN_INDBLKSHIFT];
        const u64 per_node = (u64(1) << indshift) / BP_SIZE;
        const u8* bp = nullptr;
        if (levels <= 1) {
            if (blk >= dn.nblkptr())
                return false;
            bp = dn.blkptr(u32(blk));
        } else {
            u64 index = blk;
            u64 divisor = 1;
            for (u32 l = 1; l < levels; ++l)
                divisor *= per_node;
            std::vector<u8> node;
            bool have = false;
            for (u32 l = 0; l + 1 < levels; ++l) {
                const u64 top_idx = index / divisor;
                index %= divisor;
                divisor /= per_node;
                if (l == 0) {
                    if (top_idx >= dn.nblkptr())
                        return false;
                    if (!v->read_block(dn.blkptr(u32(top_idx)), 0, node))
                        return false;
                    have = true;
                } else {
                    if (top_idx >= node.size() / BP_SIZE)
                        return false;
                    std::vector<u8> next;
                    if (!v->read_block(node.data() + u64(top_idx) * BP_SIZE, 0, next))
                        return false;
                    node = std::move(next);
                }
            }
            if (!have || index >= node.size() / BP_SIZE)
                return false;
            bp = node.data() + u64(index) * BP_SIZE;
        }
        if (bp == nullptr || bp_is_hole(bp) || bp_is_embedded(bp))
            return false;
        return v->read_block(bp, 0, data);
    }
};

/* ---- ZAP reading (micro + fat) ---- */
struct ZapEntry {
    std::string name;
    u64 value = 0;
    std::vector<u8> raw_value; /* M-038: full value bytes (fzap arrays can
                                * exceed 8 bytes - e.g. SA layout slot
                                * lists; microzap = the 8-byte LE image) */
};

bool read_microzap(const std::vector<u8>& blk, std::vector<ZapEntry>& out) {
    if (blk.size() < MZAP_ENTRIES_AT + MZAP_ENT_SIZE)
        return false;
    if (le64(blk.data()) != ZBT_MICRO)
        return false;
    const u64 sz = blk.size();
    for (u64 off = MZAP_ENTRIES_AT; off + MZAP_ENT_SIZE <= sz; off += MZAP_ENT_SIZE) {
        const u8* e = blk.data() + off;
        const u64 value = le64(e); /* mze_value @0 */
        if (value == 0)
            continue;
        const char* name = reinterpret_cast<const char*>(e + MZAP_NAME_AT);
        const u32 nlen = u32(strnlen(name, 50));
        if (nlen == 0)
            continue;
        ZapEntry ze;
        ze.name.assign(name, nlen);
        ze.value = value;
        ze.raw_value.assign(e, e + 8); /* M-038: 8-byte LE image */
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
    if (le32(leaf.data() + ZL_MAGIC) != ZAP_LEAF_MAGIC)
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
            ci = le16(c + 22); /* la_next at offset 22 (1+21) */
            if (ci == 0xFFFF)
                break;
        }
        return got == numints;
    };

    const u16 nentries = le16(leaf.data() + ZL_NENTRIES);
    for (u32 i = 0; i < nchunks && out.size() < MAX_ZAP_ENTRIES; ++i) {
        const u8* c = chunk(i);
        if (c[0] != ZAP_CHUNK_ENTRY)
            continue;
        const u32 value_intlen = c[1];
        const u32 name_chunk = le16(c + 4);
        const u32 name_numints = le16(c + 6);
        const u32 value_chunk = le16(c + 8);
        const u32 value_numints = le16(c + 10);
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
            ze.raw_value = std::move(val);
            ze.value = ze.raw_value.size() >= 8 ? le64(ze.raw_value.data()) : 0;
        }
        out.push_back(std::move(ze));
    }
    (void)nentries;
    return !out.empty();
}

/* fat zap: header block -> pointer table -> leaves (OpenZFS zap_micro.c
 * zap_ptrtbl semantics: ptrtbl entries are OBJECT-RELATIVE block numbers;
 * ptrtbl blocks (zt_blk>0) and leaf blocks are object blocks - read through
 * the dnode's block tree so compression and indirect levels are honored;
 * we linearly visit every leaf). */
bool read_fzap(Volume& v, ObjSet& os, const Dnode& dn, const std::vector<u8>& hdr,
               u64 zap_block_size, std::vector<ZapEntry>& out) {
    if (hdr.size() < 128)
        return false;
    if (le64(hdr.data()) != ZBT_HEADER)
        return false;
    if (le64(hdr.data() + 8) != ZAP_MAGIC)
        return false;
    const u64 zt_blk = le64(hdr.data() + FZ_ZT_BLK);
    const u64 zt_numblks = le64(hdr.data() + FZ_ZT_NUMBLKS);
    const u64 zt_shift = le64(hdr.data() + FZ_ZT_SHIFT);
    if (zt_shift > 30 || zap_block_size == 0)
        return false;

    std::vector<u64> leaf_blocks; /* object-relative block numbers */
    if (zt_blk == 0) {
        /* embedded pointer table: second half of the header block,
         * u64 leaf block numbers, entries = blocksize/16 */
        const u64 embedded = zap_block_size / 16;
        for (u64 i = 0; i < embedded && leaf_blocks.size() < MAX_ZAP_LEAFS; ++i) {
            const u64 at = zap_block_size / 2 + i * 8;
            if (at + 8 > hdr.size())
                break;
            const u64 lb = le64(hdr.data() + at);
            if (lb != 0)
                leaf_blocks.push_back(lb);
        }
    } else {
        if (zt_numblks == 0 || zt_numblks > 64 || zt_blk > (1u << 26))
            return false;
        for (u64 b = 0; b < zt_numblks && leaf_blocks.size() < MAX_ZAP_LEAFS; ++b) {
            std::vector<u8> pt;
            if (!os.read_object_block(dn, zt_blk + b, pt)) {
                ++v.damaged;
                continue;
            }
            if (pt.size() < zap_block_size)
                continue;
            for (u64 i = 0; i < zap_block_size / 8 && leaf_blocks.size() < MAX_ZAP_LEAFS; ++i) {
                const u64 lb = le64(pt.data() + i * 8);
                if (lb != 0)
                    leaf_blocks.push_back(lb);
            }
        }
    }
    /* dedup + read leaves through the object tree */
    std::set<u64> seen_blocks;
    u64 got = 0;
    for (u64 lb : leaf_blocks) {
        if (!seen_blocks.insert(lb).second)
            continue;
        std::vector<u8> leaf;
        if (!os.read_object_block(dn, lb, leaf)) {
            ++v.damaged;
            continue;
        }
        if (leaf.size() < zap_block_size)
            continue;
        leaf.resize(size_t(zap_block_size));
        const size_t before = out.size();
        if (read_leaf(leaf, out))
            got += out.size() - before;
        if (out.size() > MAX_ZAP_ENTRIES * 4)
            break;
    }
    v.macrozap_entries += got;
    return got > 0;
}

/* read a ZAP object (micro or fat) by dnode */
bool read_zap(Volume& v, ObjSet& os, u64 object, std::vector<ZapEntry>& out) {
    Dnode dn;
    if (!os.read_dnode(object, dn))
        return false;
    if (!zap_typed(dn.type()))
        return false; /* not a ZAP object */
    const u64 dbsz = dn.datablksz();
    if (dbsz < 512 || dbsz > (1u << 20))
        return false;
    if (dn.nblkptr() == 0)
        return false;
    /* header block = block 0, through the block tree (handles any
     * nlevels; big child ZAPs have indirect blocks) */
    std::vector<u8> blk;
    if (!os.read_object_block(dn, 0, blk))
        return false;
    const u64 btype = le64(blk.data());
    if (btype == ZBT_MICRO) {
        const size_t before = out.size();
        const bool ok = read_microzap(blk, out);
        v.microzap_entries += out.size() - before;
        return ok;
    }
    if (btype == ZBT_HEADER)
        return read_fzap(v, os, dn, blk, dbsz, out);
    return false;
}

bool load_sa_registry(Volume& v, ObjSet& zpl, const std::vector<ZapEntry>& master,
                      SaRegistry& reg) {
    u64 sa_obj = 0;
    for (const auto& e : master)
        if (e.name == "SA_ATTRS")
            sa_obj = e.value;
    if (sa_obj == 0)
        return false;
    std::vector<ZapEntry> sa_entries;
    if (!read_zap(v, zpl, sa_obj, sa_entries))
        return false;
    u64 reg_obj = 0, lay_obj = 0;
    for (const auto& e : sa_entries) {
        if (e.name == "REGISTRY")
            reg_obj = e.value;
        else if (e.name == "LAYOUTS")
            lay_obj = e.value;
    }
    if (reg_obj != 0) {
        std::vector<ZapEntry> rs;
        if (read_zap(v, zpl, reg_obj, rs)) {
            for (const auto& e : rs) {
                const u16 num = u16(e.value & 0xFFFF);
                const u8 len = u8((e.value >> 16) & 0xFF);
                if (len == 0)
                    continue; /* variable-length attrs cannot sit in a
                               * fixed layout slot (ZPL registers none);
                               * attr number 0 IS valid (ZPL_ATIME) */
                reg.len_by_num[num] = len;
                reg.name_by_num[num] = e.name;
            }
        }
    }
    if (lay_obj != 0) {
        std::vector<ZapEntry> ls;
        if (read_zap(v, zpl, lay_obj, ls)) {
            for (auto& e : ls) {
                char* end = nullptr;
                const unsigned long long ln = strtoull(e.name.c_str(), &end, 10);
                if (end == e.name.c_str() || *end != '\0' || ln == 0)
                    continue; /* layout entries are named by number */
                std::vector<u16> slots;
                for (size_t i = 0; i + 2 <= e.raw_value.size(); i += 2)
                    slots.push_back(u16(e.raw_value[i] | (u16(e.raw_value[i + 1]) << 8)));
                if (!slots.empty() && slots.size() <= 64)
                    reg.layouts[u64(ln)] = std::move(slots);
            }
        }
    }
    reg.valid = !reg.len_by_num.empty() || !reg.layouts.empty();
    return reg.valid;
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

/* Walk a gang header at the leader DVA and emit each member as its own
 * extent in order (member bps carry their own lsize/psize/codec; nested
 * gangs recurse, depth-capped). Returns false when the header is
 * unreadable (the whole L0 block is lost). */
bool plan_gang_pieces(Volume& v, const u8* dva, FileExtentPlan& plan, u32 depth = 0) {
    if (depth > 4)
        return false;
    const u64 off = dva_offset(dva);
    if (v.vdev_offset + off + 512 > v.total_bytes)
        return false;
    std::vector<u8> hdr(512, 0);
    if (v.dev->read_at(v.vdev_offset + off, hdr.data(), 512) != OK)
        return false;
    bool any = false;
    for (u32 i = 0; i < 3; ++i) {
        const u8* gbp = hdr.data() + u64(i) * BP_SIZE;
        if (bp_is_hole(gbp) || bp_is_embedded(gbp))
            continue;
        const u8* gdva = gbp + BP_DVA;
        if (dva_is_gang(gdva)) {
            if (plan_gang_pieces(v, gdva, plan, depth + 1))
                any = true;
            continue;
        }
        if (dva_vdev(gdva) != 0)
            continue; /* other vdev: single-disk scan skips */
        const u64 lsize = bp_lsize(gbp);
        const u64 psz = bp_psize(gbp);
        const u32 zc = bp_compress(gbp);
        if (lsize == 0 || psz == 0)
            continue;
        if (v.vdev_offset + dva_offset(gdva) +
                (zc == 0 || zc == 1 || zc == 2 || zc == ZIO_COMPRESS_EMPTY ? lsize : psz) >
            v.total_bytes)
            continue;
        FileExtentPlan::Item it;
        it.offset = v.vdev_offset + dva_offset(gdva);
        it.length = lsize;
        it.psize = (zc == 0 || zc == 1 || zc == 2 || zc == ZIO_COMPRESS_EMPTY) ? lsize : psz;
        it.codec = compress::codec_from_zfs(zc);
        plan.items.push_back(it);
        plan.logical += lsize;
        any = true;
    }
    return any;
}

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
            /* M-019: the og resolves every DVA through the registry with
             * the first-entry fallback (FUN_1800168b0) - a foreign vdev
             * id reads from the scanned disk. The extent is therefore
             * planned at the fallback position and counted; content
             * integrity is enforced at stream time in the extent-lazy
             * model (the registered architectural divergence), while
             * every block the scan READS goes through the fletcher4
             * gate in read_block. */
            if (dva_vdev(dva) != 0)
                ++v.foreign_vdev_reads; /* og first-entry fallback path */
            FileExtentPlan::Item it;
            it.offset = v.vdev_offset + dva_offset(dva);
            it.length = lsize;
            it.psize = (zc == 0 || zc == 1 || zc == 2 || zc == ZIO_COMPRESS_EMPTY) ? lsize : psz;
            it.codec = compress::codec_from_zfs(zc);
            plan.logical += lsize;
            if (dva_is_gang(dva)) {
                /* gang block: content is the concatenation of the header's
                 * member blocks - emitted as their own ordered extents so
                 * the writer streams the exact bytes (no leader garbage).
                 * An unreadable header loses only this L0 block. */
                if (!plan_gang_pieces(v, dva, plan))
                    ++v.damaged;
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

u32 scan_zfs(IStorageDevice* slice, CancelToken* cancel, WalkProgressLike progress,
             void* progress_user, tree::ResultTree& tree, tree::FullNode* root_active,
             tree::FullNode* root_deleted, tree::FullNode* root_lost, ZfsScanStats& stats) {
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
        /* uberblock ring (vdev_impl.h VDEV_UBERBLOCK_SHIFT = MAX(ashift,10)):
         * slot size = 1 << shift, slot count = 128K >> shift. Probe shifts
         * 10..13 and keep the geometry with the most valid magics - pools on
         * 4Kn disks (ashift 12) have 32 x 4KiB slots, not 128 x 1KiB. */
        for (u32 shift = 10; shift <= 13; ++shift) {
            const u64 slot_size = u64(1) << shift;
            const u32 slot_count = u32((LABEL_SIZE - UBRING_OFFSET) >> shift);
            u32 hits = 0;
            std::vector<u8> ub(static_cast<size_t>(slot_size), 0);
            for (u32 ui = 0; ui < slot_count; ++ui) {
                const u64 at = label_base[li] + UBRING_OFFSET + u64(ui) * slot_size;
                if (at + slot_size > total)
                    break;
                if (slice->read_at(at, ub.data(), u32(ub.size())) != OK)
                    break;
                const u64 magic = le64(ub.data());
                if (magic != UBERBLOCK_MAGIC) {
                    if (magic == 0x0CB1BA00ULL || magic == 0x0CBA0000ULL)
                        return ERR_UNSUPPORTED; /* big-endian pool: refused,
                                                 * not misparsed */
                    continue;
                }
                ++hits;
                const u64 txg = le64(ub.data() + UB_TXG);
                if (txg > best_txg) {
                    best_txg = txg;
                    best_ub.assign(ub.begin(), ub.begin() + UBERBLOCK_SIZE);
                }
            }
            if (hits > 0)
                break; /* found the pool's true uberblock geometry */
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
            if (depth > MAX_DATASET_DEPTH || (cancel && cancel->checkpoint()))
                return;
            Dnode dn;
            if (!mos.read_dnode(dsl_dir_obj, dn))
                return;
            const u32 blen = dn.bonuslen();
            if (blen < 256 || blen > MAX_BONUS || u64(blen) > dn.bonus_cap())
                return;
            const u8* bonus = dn.bonus();
            const u64 head_obj = le64(bonus + DD_HEAD_DATASET_OBJ);
            const u64 child_zap = le64(bonus + DD_CHILD_DIR_ZAPOBJ);
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
                        /* S-005: zap child names are UTF-8 - decode */
                        std::u16string sub = prefix;
                        sub += text::utf8_to_u16(c.name);
                        walk_dir(c.value, sub, depth + 1);
                    }
                }
            }
        };
    walk_dir(dataset_dir_obj, u"", 0);
    if (progress)
        (void)progress(2, 4, progress_user);

    /* 4. per-dataset ZPL walk: master node object 1 -> ROOT -> dirs. */
    std::u16string pool_display_ = text::utf8_to_u16(
        cfg.valid && !cfg.pool_name.empty() ? cfg.pool_name : std::string("pool"));
    for (const auto& ds : datasets) {
        if (cancel && cancel->checkpoint())
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

        /* M-038: per-dataset SA registry (master node "SA_ATTRS" ->
         * {REGISTRY, LAYOUTS}); absent registry = the implicit default
         * layout only (registry-less pools and the legacy fixtures) */
        SaRegistry sa_reg;
        load_sa_registry(v, zpl, master, sa_reg);

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
            if (dn.bonustype() == DMU_OT_SA && blen >= 8) {
                /* M-038: registry-driven SA decode (the og resolves
                 * attributes BY NAME through the dataset's SA registry
                 * and decodes every registered layout; the uniform map
                 * lookup at :17636 ZEROES attrs on a miss - the object
                 * still surfaces). Layout 1 falls back to the implicit
                 * default table (the zfs_attr_table order) when the
                 * registry/layouts object is absent - the pre-M-038
                 * behavior, kept for registry-less pools. */
                const u32 magic = le32(b);
                const u32 layout_info = le16(b + 4);
                const u32 layout = layout_info & 0x3FF;
                const u32 hdr_bytes = ((layout_info >> 10) & 0x3F) << 3;
                if (magic != SA_MAGIC)
                    return nullptr; /* not an SA bonus */
                bool decoded = false;
                if (sa_reg.valid && sa_reg.has_layout(layout)) {
                    /* registry-driven: offsets by cumulative registered
                     * lengths over the layout's slot order */
                    u32 off = hdr_bytes;
                    for (u16 slot : sa_reg.layouts[layout]) {
                        auto lit = sa_reg.len_by_num.find(slot);
                        if (lit == sa_reg.len_by_num.end())
                            break; /* unregistered attr number: keep the
                                    * attrs decoded so far (the og zero-
                                    * fills the missing tail) */
                        const u8 alen = lit->second;
                        if (u64(off) + alen > blen)
                            break; /* bonus shorter than the layout: keep
                                    * the in-bounds prefix */
                        auto nit = sa_reg.name_by_num.find(slot);
                        if (nit != sa_reg.name_by_num.end()) {
                            const u8* p = b + off;
                            u64 v64 = alen >= 8 ? le64(p)
                                                : (alen == 4 ? u64(le32(p))
                                                             : (alen == 2 ? u64(le16(p))
                                                                          : u64(*p)));
                            const std::string& nm = nit->second;
                            if (nm == "ZPL_ATIME")
                                oi.atime[0] = v64;
                            else if (nm == "ZPL_MTIME")
                                oi.mtime[0] = v64;
                            else if (nm == "ZPL_CTIME")
                                oi.ctime[0] = v64;
                            else if (nm == "ZPL_CRTIME")
                                oi.crtime[0] = v64;
                            else if (nm == "ZPL_MODE")
                                oi.mode = v64;
                            else if (nm == "ZPL_SIZE")
                                oi.size = v64;
                            else if (nm == "ZPL_PARENT")
                                oi.parent = v64;
                            else if (nm == "ZPL_LINKS")
                                oi.links = v64;
                            /* gen/xattr/rdev/flags/uid/gid: carried in
                             * the registry, not consumed by the walk */
                        }
                        off += alen;
                    }
                    decoded = true;
                } else if (layout == 1 && blen >= SA_LAYOUT1_MIN) {
                    /* implicit default layout 1 (zfs_attr_table order):
                     * the fallback for registry-less pools - real pools
                     * carry layout 1 in the LAYOUTS object and decode
                     * through the registry path above */
                    oi.atime[0] = le64(b + SA_LAYOUT1_ATIME);
                    oi.mtime[0] = le64(b + SA_LAYOUT1_MTIME);
                    oi.ctime[0] = le64(b + SA_LAYOUT1_CTIME);
                    oi.crtime[0] = le64(b + SA_LAYOUT1_CRTIME);
                    oi.mode = le64(b + SA_LAYOUT1_MODE);
                    oi.parent = le64(b + SA_LAYOUT1_PARENT);
                    oi.links = le64(b + SA_LAYOUT1_LINKS);
                    oi.size = le64(b + SA_LAYOUT1_SIZE);
                    decoded = true;
                }
                (void)decoded; /* an unregistered non-default layout
                 * surfaces the object with ZERO attrs - og parity (the
                 * uniform map lookup at :17636 zero-fills); the pre-M-038
                 * core DROPPED the object here, losing the file entirely */
            } else if (dn.bonustype() == DMU_OT_ZNODE && blen >= ZP_MIN) {
                /* legacy znode_phys (zfs_sa.h) */
                oi.atime[0] = le64(b + ZP_ATIME);
                oi.mtime[0] = le64(b + ZP_MTIME);
                oi.ctime[0] = le64(b + ZP_CTIME);
                oi.crtime[0] = le64(b + ZP_CRTIME);
                oi.mode = le64(b + ZP_MODE);
                oi.parent = le64(b + ZP_PARENT);
                oi.links = le64(b + ZP_LINKS);
                oi.size = le64(b + ZP_SIZE);
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
                if (cancel && cancel->checkpoint())
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
                p->name = text::utf8_to_u16(e.name); /* S-005: UTF-8 decode */
                p->fs_kind = u32(FsKindCode::Zfs);   /* corpus factory code */
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
                        u64 compressed = 0;
                        for (auto& it : plan.items) {
                            if (it.codec != 0)
                                ++compressed;
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
                        stats.compressed_extents += compressed;
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
    stats.microzap_entries = v.microzap_entries;
    stats.macrozap_entries = v.macrozap_entries;
    stats.foreign_vdev_reads = v.foreign_vdev_reads; /* M-019 */
    stats.checksum_drops = v.checksum_drops;         /* M-019 */
    if (progress)
        (void)progress(4, 4, progress_user);
    return OK;
}

} // namespace zfs
} // namespace cr
