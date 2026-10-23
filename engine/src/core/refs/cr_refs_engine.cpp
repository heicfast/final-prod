/* cr_refs_engine.cpp - full ReFS scan engine (MSB-chain reconstruction).
 *
 * Bootstrap chain: VBR -> superblock (cluster 30 + backups) -> checkpoint
 * (newer virtual clock wins) -> ObjectID table (table map) -> directory
 * tables (root = fixed ID 0x600, fallback: not-a-child in the parent-child
 * table) -> embedded file tables -> $DATA data-run tables -> extents.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/refs/cr_refs_engine.h"
#include "cr/cr_bits.h" /* shared byte readers (W4 dedup, DC-12) */
#include "cr/cr_error.h"
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <vector>

namespace cr {
namespace refs {

namespace {

/* ---- page signatures ---- */
constexpr u32 SIG_SUPB = 0x42505553; /* "SUPB" */
constexpr u32 SIG_CHKP = 0x504B4843; /* "CHKP" */
constexpr u32 SIG_MSB = 0x2B42534D;  /* "MSB+" */

/* ---- directory-table row key markers (thesis tables 4.25-4.28) ---- */
constexpr u32 ROW_DIR_DESCRIPTOR = 0x00000010;
constexpr u32 ROW_FILE = 0x00010030;
constexpr u32 ROW_DIR_LINK = 0x00020030;
constexpr u32 ROW_ID2 = 0x80000020;

/* ---- attribute type ids (thesis table 4.1) ---- */
constexpr u16 ATTR_DATA = 0x80;

/* ---- data-run flags (thesis table 4.38) ---- */
constexpr u16 RUN_HAS_DATA = 0x0010;

/* ---- index-entry flags ---- */
constexpr u16 ENTRY_DELETED = 0x4;

/* ---- index-header flags ---- */
constexpr u8 NODE_INNER = 0x1;
constexpr u8 NODE_ROOT = 0x2;

constexpr u32 MAX_NODES_PER_TABLE = 4096;
constexpr u32 MAX_TABLE_DEPTH = 32;
constexpr u64 ROOT_DIR_TABLE = 0x600; /* fixed root directory id */
constexpr u64 FSMETADATA_TABLE = 0x520;

/* Session state threaded through the walk. */
struct Engine {
    IStorageDevice* slice = nullptr;
    RefsBoot boot;
    u32 page_clusters = 4; /* clusters per page (4 at 4 KiB, 1 at 64 KiB) */
    u32 page_bytes = 0;
    u32 volume_sig = 0;
    RefsScanStats* stats = nullptr;
    CancelToken* cancel = nullptr;

    /* page cache keyed by first LCN (bounded) */
    std::map<u64, std::vector<u8>> cache;

    /* cluster translation: container-tagged VCNs carry container+1 in the
     * top nibble; without a container table the tag is masked (typical
     * single-container volume). */
    std::map<u64, u64> container_lcn; /* container index -> start LCN */
    u64 translate_cluster(u64 vcn) const {
        const u64 tag = vcn >> 60;
        u64 cl = vcn & 0x0FFFFFFFFFFFFFFFull;
        if (tag >= 1) {
            auto it = container_lcn.find(tag - 1);
            if (it != container_lcn.end())
                cl += it->second;
        }
        return cl;
    }

    bool read_cluster(u64 cluster, void* buf) const {
        const u64 off = cluster * boot.cluster_bytes;
        if (cluster >= boot.volume_clusters)
            return false;
        return slice->read_at(off, buf, u32(boot.cluster_bytes)) == OK;
    }

    /* Assemble the page addressed by an LCN quadruple. Returns false when
     * any cluster read fails or (for signature-bearing pages) the volume
     * signature does not match. */
    bool read_page(const u64 lcns[4], std::vector<u8>& out) {
        const u64 first = translate_cluster(lcns[0]);
        if (first == 0 || first >= boot.volume_clusters)
            return false;
        auto it = cache.find(first);
        if (it != cache.end()) {
            out = it->second;
            return true;
        }
        std::vector<u8> pb(page_bytes);
        for (u32 i = 0; i < page_clusters; ++i) {
            u64 c = i == 0 ? first : translate_cluster(lcns[i]);
            if (c == 0)
                c = first + i; /* zeroed tail entries mean contiguous */
            if (!read_cluster(c, pb.data() + u64(i) * boot.cluster_bytes))
                return false;
        }
        if (pb.size() >= 0x50) {
            const u32 psig = le32(pb.data());
            if (psig == SIG_SUPB || psig == SIG_CHKP || psig == SIG_MSB) {
                if (le32(pb.data() + 0x0C) != volume_sig) {
                    if (stats)
                        ++stats->damaged_nodes;
                    return false;
                }
            }
        } else {
            return false;
        }
        if (cache.size() > 256)
            cache.clear();
        cache.emplace(first, pb);
        out = std::move(pb);
        return true;
    }

    bool cancelled() const { return cancel && cancel->cancelled(); }
    /* pause-aware gate for walk loop heads (never used in progress
     * callbacks - those must not block; RE G-07) */
    bool cancel_gate() const { return cancel && cancel->checkpoint(); }
};

/* ---- node parsing (pages after the 0x50 header, or embedded roots) ---- */

struct EntryView {
    const u8* key = nullptr;
    u16 key_len = 0;
    const u8* val = nullptr;
    u16 val_len = 0;
    u16 flags = 0;
    bool ok = false;
};

struct NodeView {
    const u8* buf = nullptr;
    size_t len = 0;
    const u8* ih = nullptr; /* index header base */
    u32 da_start = 0, da_end = 0;
    u8 height = 0, flags = 0;
    u32 ki_start = 0, ki_count = 0;
    const u8* var = nullptr; /* variable index-root component (file info) */
    u32 var_len = 0;
    bool ok = false;
};

NodeView parse_node(const u8* buf, size_t len) {
    NodeView nv;
    nv.buf = buf;
    nv.len = len;
    if (buf == nullptr || len < 0x28)
        return nv;
    const u32 ir_size = le32(buf);
    if (ir_size < 8 || u64(ir_size) + 0x28 > len)
        return nv;
    const u32 fixed = le16(buf + 4);
    if (fixed != 0x28 && fixed != 0x8)
        return nv;
    if (ir_size > fixed) {
        nv.var = buf + fixed;
        nv.var_len = ir_size - fixed;
    }
    nv.ih = buf + ir_size;
    if (size_t(nv.ih - buf) + 0x28 > len)
        return nv;
    nv.da_start = le32(nv.ih + 0x00);
    nv.da_end = le32(nv.ih + 0x04);
    nv.height = nv.ih[0x0C];
    nv.flags = nv.ih[0x0D];
    nv.ki_start = le32(nv.ih + 0x10);
    nv.ki_count = le32(nv.ih + 0x14);
    if (nv.da_end > len || nv.ki_count > 0x10000)
        return nv;
    if (u64(nv.ki_start) + u64(nv.ki_count) * 4 > len - size_t(nv.ih - buf))
        return nv;
    nv.ok = true;
    return nv;
}

/* Entry i in key-index order: u32 at ih+ki_start+i*4 (high 16 bits are
 * 0xFFFF garbage), entry offsets relative to the index header. */
EntryView entry_at(const NodeView& nv, u32 i) {
    EntryView e;
    if (!nv.ok || i >= nv.ki_count)
        return e;
    const u32 off = le32(nv.ih + nv.ki_start + u64(i) * 4) & 0xFFFF;
    const u8* ent = nv.ih + off;
    if (size_t(ent - nv.buf) + 14 > nv.len)
        return e;
    const u32 ent_len = le32(ent);
    if (ent_len < 14 || size_t(ent - nv.buf) + ent_len > nv.len)
        return e;
    e.flags = le16(ent + 8);
    const u16 ko = le16(ent + 4), kl = le16(ent + 6);
    const u16 vo = le16(ent + 10), vl = le16(ent + 12);
    if (u64(ko) + kl > ent_len || u64(vo) + vl > ent_len)
        return e;
    e.key = ent + ko;
    e.key_len = kl;
    e.val = ent + vo;
    e.val_len = vl;
    e.ok = true;
    return e;
}

/* Walk one table (root page LCN quad); cb per LEAF entry. Inner nodes
 * recurse through the child page references carried in entry values. */
bool walk_table(Engine& e, const u64 root_lcns[4], u32 depth,
                const std::function<bool(const EntryView&)>& cb, std::set<u64>& visited,
                u32& node_count) {
    if (depth > MAX_TABLE_DEPTH || node_count > MAX_NODES_PER_TABLE || e.cancel_gate())
        return false;
    const u64 first = e.translate_cluster(root_lcns[0]);
    if (first == 0 || !visited.insert(first).second)
        return false; /* cycle / null */
    ++node_count;
    std::vector<u8> page;
    if (!e.read_page(root_lcns, page))
        return false;
    const NodeView nv = parse_node(page.data() + 0x50, page.size() - 0x50);
    if (!nv.ok) {
        if (e.stats)
            ++e.stats->damaged_nodes;
        return false;
    }
    for (u32 i = 0; i < nv.ki_count; ++i) {
        EntryView ent = entry_at(nv, i);
        if (!ent.ok)
            continue;
        if (nv.flags & NODE_INNER) {
            if (ent.val_len >= 0x20) {
                u64 cl[4];
                for (int k = 0; k < 4; ++k)
                    cl[k] = le64(ent.val + k * 8);
                if (cl[0] != 0)
                    walk_table(e, cl, depth + 1, cb, visited, node_count);
            }
        } else {
            if (!cb(ent))
                return false;
        }
    }
    return true;
}

/* File/dir general info from the variable index-root component (thesis
 * tables 4.29/4.30): MACE times @+0x00..0x20, flags @+0x20, size @+0x30,
 * allocated size @+0x38. */
struct GeneralInfo {
    u64 create = 0, modify = 0, meta_modify = 0, access = 0;
    u32 flags = 0;
    u64 size = 0, alloc_size = 0;
    bool ok = false;
};
GeneralInfo parse_general_info(const u8* var, u32 len) {
    GeneralInfo gi;
    if (var == nullptr || len < 0x40)
        return gi;
    gi.create = le64(var + 0x00);
    gi.modify = le64(var + 0x08);
    gi.meta_modify = le64(var + 0x10);
    gi.access = le64(var + 0x18);
    gi.flags = le32(var + 0x20);
    gi.size = le64(var + 0x30);
    gi.alloc_size = le64(var + 0x38);
    gi.ok = true;
    return gi;
}

u16 attr_type_of(const EntryView& e) {
    return e.key_len >= 10 ? le16(e.key + 8) : 0;
}

/* $DATA value = embedded data-run table root node (thesis 4.38 rows). */
struct DataRun {
    u64 lcn = 0;
    u32 clusters = 0;
    bool has_data = false;
};

bool parse_data_runs(const u8* node, u32 node_len, std::vector<DataRun>& out) {
    const NodeView nv = parse_node(node, node_len);
    if (!nv.ok || (nv.flags & NODE_INNER))
        return false;
    for (u32 i = 0; i < nv.ki_count; ++i) {
        EntryView ent = entry_at(nv, i);
        if (!ent.ok || ent.val_len < 0x18)
            continue;
        DataRun r;
        r.lcn = le64(ent.val + 0x00);
        const u16 fl = le16(ent.val + 8);
        r.clusters = le32(ent.val + 0x14);
        r.has_data = (fl & RUN_HAS_DATA) != 0;
        if (r.clusters != 0 && r.clusters < 0x10000000u)
            out.push_back(r);
    }
    return true;
}

/* Medium-allocator bitmap (checkpoint pointer[1]) for deleted-extent
 * grading; rows {firstCluster u64, numClusters u64, free u16, flags u16
 * @+0x12 (2=sparse), bitmap @+0x18}. Bit polarity calibrated against the
 * superblock cluster (30, always allocated). */
struct Allocator {
    struct Row {
        u64 first = 0, count = 0;
        bool sparse = false;
        std::vector<u8> bitmap;
    };
    std::vector<Row> rows;
    bool allocated_bit_set = true;
    bool valid = false;

    bool raw_bit(u64 c, bool& covered) const {
        covered = false;
        for (const auto& r : rows) {
            /* P1-9: overflow-safe range test */
            if (c < r.first || c - r.first >= r.count)
                continue;
            covered = true;
            if (r.sparse)
                return false;
            const u64 bit = c - r.first;
            if (r.bitmap.empty() || bit / 8 >= r.bitmap.size())
                return false;
            return (r.bitmap[size_t(bit / 8)] >> (bit % 8)) & 1;
        }
        return false;
    }

    bool cluster_allocated(u64 c) const {
        bool covered = false;
        const bool bit = raw_bit(c, covered);
        if (!covered)
            return false;
        return allocated_bit_set ? bit : !bit;
    }
};

} // namespace

bool validate_boot(const u8 sector[512], RefsBoot& out) {
    out = RefsBoot{};
    if (sector == nullptr)
        return false;
    /* og gate (FUN_180012390:13529-13531 - REFS-3 pin correction: the
     * previously cited FUN_180013420 does not exist): "ReFS" @+3,
     * bytesPerSector u32 @0x20, sectorsPerCluster u32 @0x24, major u8
     * @0x28, minor u8 @0x29. The og accepts major==3 && minor>3 (ReFS
     * 3.4+; re-validated on the checkpoint body, FUN_180012650:13744-50)
     * plus a separate major==1 path. This engine validates the full shape
     * and accepts 1.x-3.x - wider recovery scope, same structure walk. */
    if (std::memcmp(sector + 3, "ReFS", 4) != 0)
        return false;
    if (le32(sector + 0x10) != 0x53525346) /* "FSRS" */
        return false;
    const u32 bps = le32(sector + 0x20);
    if (bps != 512 && bps != 4096)
        return false;
    const u32 spc = le32(sector + 0x24);
    if (spc == 0 || (spc & (spc - 1)) != 0 || spc > 128)
        return false;
    const u64 sectors = le64(sector + 0x18);
    if (sectors == 0)
        return false;
    out.valid = true;
    out.bytes_per_sector = bps;
    out.sectors_per_cluster = spc;
    out.cluster_bytes = u64(bps) * spc;
    out.volume_clusters = sectors / spc;
    out.volume_bytes = sectors * bps;
    out.major = sector[0x28];
    out.minor = sector[0x29];
    out.serial = le64(sector + 0x38);
    return out.major >= 1 && out.major <= 3;
}

u32 scan_refs(IStorageDevice* slice, const RefsScanFlags& flags, CancelToken* cancel,
              WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
              tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
              RefsScanStats& stats) {
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;
    (void)root_lost;

    /* ---- 1. boot ---- */
    u8 sec[512];
    if (slice->read_at(0, sec, 512) != OK)
        return ERR_READ_FAILED;
    RefsBoot boot;
    if (!validate_boot(sec, boot))
        return ERR_BAD_MAGIC;

    Engine eng;
    eng.slice = slice;
    eng.boot = boot;
    eng.stats = &stats;
    eng.cancel = cancel;
    /* page geometry: 16 KiB pages from 4 clusters at 4 KiB cluster size;
     * one cluster at >= 16 KiB (thesis 4.3.2). */
    eng.page_clusters = boot.cluster_bytes < 16384 ? u32(16384 / boot.cluster_bytes) : 1;
    if (eng.page_clusters == 0)
        eng.page_clusters = 1;
    if (eng.page_clusters > 4)
        eng.page_clusters = 4;
    eng.page_bytes = eng.page_clusters * u32(boot.cluster_bytes);

    /* ---- 2. superblock: cluster 30, backups in the 2nd/3rd-last clusters ---- */
    auto load_superblock = [&](u64 cluster, std::vector<u8>& out) -> bool {
        if (cluster == 0 || cluster >= boot.volume_clusters)
            return false;
        out.resize(eng.page_bytes);
        for (u32 i = 0; i < eng.page_clusters; ++i)
            if (!eng.read_cluster(cluster + i, out.data() + u64(i) * boot.cluster_bytes))
                return false;
        return out.size() >= 0x80 && le32(out.data()) == SIG_SUPB;
    };
    std::vector<u8> sup;
    bool have_sup = load_superblock(30, sup);
    if (!have_sup) {
        const u64 n = boot.volume_clusters;
        if (n > 4)
            have_sup = load_superblock(n - 3, sup);
        if (!have_sup && n > 4)
            have_sup = load_superblock(n - 2, sup);
    }
    if (!have_sup)
        return ERR_BAD_MAGIC;

    /* volume signature from the GUID (4 x u32 XOR, thesis fig 4.27) */
    const u32 vol_sig = le32(sup.data() + 0x50) ^ le32(sup.data() + 0x54) ^
                        le32(sup.data() + 0x58) ^ le32(sup.data() + 0x5C);
    eng.volume_sig = vol_sig;

    /* ---- 3. checkpoint refs: u32 @+0x70 -> two u64 cluster numbers ---- */
    const u32 chkp_off = le32(sup.data() + 0x70);
    if (u64(chkp_off) + 16 > sup.size())
        return ERR_BAD_MAGIC;
    const u64 chkp_clusters[2] = {le64(sup.data() + chkp_off), le64(sup.data() + chkp_off + 8)};

    auto load_checkpoint = [&](u64 cluster, std::vector<u8>& out) -> bool {
        if (cluster == 0 || cluster >= boot.volume_clusters)
            return false;
        out.resize(eng.page_bytes);
        for (u32 i = 0; i < eng.page_clusters; ++i)
            if (!eng.read_cluster(cluster + i, out.data() + u64(i) * boot.cluster_bytes))
                return false;
        return out.size() >= 0xC8 && le32(out.data()) == SIG_CHKP &&
               le32(out.data() + 0x0C) == vol_sig;
    };
    std::vector<u8> chkp, chkp2;
    const bool ok1 = load_checkpoint(chkp_clusters[0], chkp);
    const bool ok2 = load_checkpoint(chkp_clusters[1], chkp2);
    if (ok1 && ok2)
        chkp = le64(chkp.data() + 0x60) >= le64(chkp2.data() + 0x60) ? chkp : chkp2;
    else if (ok2)
        chkp = chkp2;
    else if (!ok1)
        return ERR_BAD_MAGIC;

    /* ---- 4. checkpoint system-table descriptor list ----
     * REFS-2 (AUDIT-LINUX-FS, HIGH; og FUN_180012650, decompiled
     * :13744-13790, re-verified this session): the checkpoint body starts
     * at the 0x50 page header; the og re-validates the version on the body
     * (major u16@body+4 == 3 && minor u16@body+6 > 3 = ReFS 3.4+). The
     * descriptor COUNT sits at body+0x40 (page+0x90) in BOTH layouts; the
     * u32 offset LIST base is version-dependent: body+0x44 (page+0x94) for
     * minor<14, body+0x58 (page+0xA8) for minor>=14 (ReFS >= 3.7). Each
     * list entry is a page-relative offset to a 0x30-byte REFERENCE
     * structure {LCN quad@+0x00, flags byte@+0x22, next u64@+0x28} - and
     * next@+0x28 chains to the NEWER version of the same table
     * (RefsMetadataBlockRefrenceImpl -> RefsBaseDataImplManager chains,
     * FUN_18000fc80/FUN_18000ff60:11715/11860; REFS-1). The old code
     * hardcoded 0x90/0x94 (wrong list on 3.7+) and never followed next
     * (post-checkpoint updates missed). */
    {
        const u32 body_major = le16(chkp.data() + 0x54);
        const u32 body_minor = le16(chkp.data() + 0x56);
        if (body_major != 0 || body_minor != 0) { /* body carries a version */
            if (body_major == 3 && body_minor <= 3)
                return ERR_BAD_MAGIC; /* og rejects pre-3.4 checkpoints */
            if (body_major != 0 && body_major != boot.major)
                return ERR_BAD_MAGIC; /* inconsistent with the VBR */
        }
    }
    const u32 ptr_count = le32(chkp.data() + 0x90); /* body+0x40, both layouts */
    if (ptr_count == 0 || ptr_count > 16)
        return ERR_BAD_MAGIC;
    const u32 ptr_lst_off = boot.minor >= 0x0E ? 0xA8 : 0x94; /* body+0x58 / body+0x44 */
    u32 ptrs[16] = {};
    for (u32 i = 0; i < ptr_count && i < 16; ++i)
        ptrs[i] = le32(chkp.data() + ptr_lst_off + i * 4);

    auto table_root = [&](u32 ptr_index, u64 out_lcns[4]) -> bool {
        if (ptr_index >= ptr_count)
            return false;
        u32 off = ptrs[ptr_index];
        /* REFS-1: follow the reference's next-chain (next@ref+0x28, a
         * page-relative offset to the newer reference) to the chain tail -
         * the checkpoint pins a possibly-older version and post-checkpoint
         * updates extend the chain. */
        for (u32 hop = 0; hop < 16; ++hop) {
            if (u64(off) + 0x30 > chkp.size())
                return false;
            const u64 next = le64(chkp.data() + off + 0x28);
            if (next == 0 || next + 0x30 > chkp.size())
                break; /* chain tail (or corrupt link: keep this version) */
            const u32 noff = u32(next);
            if (noff <= off)
                break; /* not forward-progressing: corrupt loop */
            off = noff;
        }
        if (u64(off) + 0x20 > chkp.size())
            return false;
        for (int k = 0; k < 4; ++k)
            out_lcns[k] = le64(chkp.data() + off + k * 8);
        return out_lcns[0] != 0;
    };

    /* ---- 5. ObjectID table: tableID -> root LCN quad (thesis 4.12) ---- */
    struct TableInfo {
        u64 lcns[4] = {0, 0, 0, 0};
        bool visited = false;
    };
    std::map<u64, TableInfo> tables;
    {
        u64 root[4];
        if (!table_root(0, root))
            return ERR_BAD_MAGIC;
        std::set<u64> visited;
        u32 nodes = 0;
        walk_table(
            eng, root, 0,
            [&](const EntryView& ev) {
                if (ev.key_len < 8 || ev.val_len < 0x38)
                    return true;
                TableInfo ti;
                for (int k = 0; k < 4; ++k)
                    ti.lcns[k] = le64(ev.val + 0x30 + k * 8);
                if (ti.lcns[0] != 0) {
                    tables[le64(ev.key)] = ti;
                    ++stats.tables;
                }
                return !eng.cancelled();
            },
            visited, nodes);
    }
    if (tables.empty())
        return ERR_BAD_MAGIC;

    /* ---- 6. parent-child table: child set (root fallback) ---- */
    std::set<u64> child_tables;
    {
        u64 root[4];
        if (table_root(4, root)) {
            std::set<u64> visited;
            u32 nodes = 0;
            walk_table(
                eng, root, 0,
                [&](const EntryView& ev) {
                    if (ev.key_len >= 16)
                        child_tables.insert(le64(ev.key + 8));
                    return !eng.cancelled();
                },
                visited, nodes);
        }
    }

    /* ---- 7. medium allocator (deleted-extent grading) ---- */
    Allocator alloc;
    if (flags.allocator_grading) {
        u64 root[4];
        if (table_root(1, root)) {
            std::set<u64> visited;
            u32 nodes = 0;
            walk_table(
                eng, root, 0,
                [&](const EntryView& ev) {
                    if (ev.val_len < 0x18)
                        return true;
                    Allocator::Row r;
                    r.first = le64(ev.val + 0x00);
                    r.count = le64(ev.val + 0x08);
                    r.sparse = (le16(ev.val + 0x12) & 0x2) != 0;
                    if (!r.sparse && r.count <= 0x10000000ull) {
                        const u32 bytes = u32((r.count + 7) / 8);
                        if (u64(0x18) + bytes <= ev.val_len)
                            r.bitmap.assign(ev.val + 0x18, ev.val + 0x18 + bytes);
                    }
                    if (r.count != 0 && r.count <= 0x10000000ull) {
                        alloc.rows.push_back(std::move(r));
                        ++stats.allocator_rows;
                    }
                    return !eng.cancelled();
                },
                visited, nodes);
            if (!alloc.rows.empty()) {
                alloc.valid = true;
                /* calibrate bit polarity: the superblock cluster (30) is
                 * always allocated; if its raw bit is 0 the polarity is
                 * inverted on this volume. */
                bool covered = false;
                const bool bit30 = alloc.raw_bit(30, covered);
                alloc.allocated_bit_set = covered ? bit30 : true;
            }
        }
    }

    /* ---- 8. hierarchy walk from the root directory (0x600) ---- */
    struct Walker {
        Engine& e;
        tree::ResultTree& tree;
        tree::FullNode* del_root;
        const RefsScanFlags& flags;
        const Allocator& alloc;
        RefsScanStats& stats;
        std::map<u64, TableInfo>& tables;
        std::set<u64>& child_tables;

        std::u16string name_from_key(const u8* key, u16 key_len) const {
            std::u16string out;
            if (key_len < 8)
                return out;
            for (u32 i = 4; i + 1 < key_len; i += 2)
                out.push_back(u16(key[i]) | (u16(key[i + 1]) << 8));
            return out;
        }

        /* File row: key = {u32 0x00010030, UTF-16 name}; value = embedded
         * file-table root node (general info + attribute rows). */
        void emit_file(tree::FileNode* parent, const EntryView& ev, u64 dir_id, bool deleted) {
            const std::u16string name16 = name_from_key(ev.key, ev.key_len);
            if (name16.empty() || ev.val_len < 0x28)
                return;
            const NodeView nv = parse_node(ev.val, ev.val_len);
            if (!nv.ok)
                return;
            const GeneralInfo gi = parse_general_info(nv.var, nv.var_len);

            tree::FileProperties* props = new tree::FileProperties();
            props->fs_kind = REFS_FS_KIND;
            props->record_ref = dir_id; /* metadata address: directory id */
            props->name = name16;
            props->logical_size = gi.size;
            props->allocated_size = gi.alloc_size;
            if (gi.ok) {
                props->times.created = gi.create;
                props->times.modified = gi.modify;
                props->times.changed = gi.meta_modify;
                props->times.accessed = gi.access;
            }
            props->attributes = (gi.flags & 0x10000000) != 0 ? 0x10 : 0;

            /* attribute rows: $DATA (0x80) -> embedded data-run table */
            for (u32 i = 0; i < nv.ki_count; ++i) {
                const EntryView ae = entry_at(nv, i);
                if (!ae.ok || attr_type_of(ae) != ATTR_DATA || ae.val_len < 0x28)
                    continue;
                std::vector<DataRun> runs;
                if (!parse_data_runs(ae.val, ae.val_len, runs))
                    break;
                for (const auto& r : runs) {
                    if (!r.has_data)
                        continue; /* sparse hole */
                    const u64 lcn = e.translate_cluster(r.lcn);
                    const u64 off = lcn * e.boot.cluster_bytes;
                    const u64 len = u64(r.clusters) * e.boot.cluster_bytes;
                    /* P1-9: overflow-safe form (off/len can wrap) */
                    if (lcn < e.boot.volume_clusters && len <= e.boot.volume_bytes &&
                        off <= e.boot.volume_bytes - len) {
                        props->extents.push_back({off, len});
                        ++stats.data_runs;
                    }
                }
                break; /* default (unnamed) data stream */
            }

            tree::FileNode* node = tree.add_file(parent, props);
            props->release(); /* tree holds the reference */
            if (node == nullptr)
                return; /* arena OOM (P2-1) */
            if (deleted) {
                node->set_deleted_state(tree::DeletedState::Deleted);
                bool reused_any = false;
                if (alloc.valid) {
                    for (const auto& ex : props->extents) {
                        if (alloc.cluster_allocated(ex.offset / e.boot.cluster_bytes)) {
                            reused_any = true;
                            break;
                        }
                    }
                }
                node->set_confidence(reused_any ? tree::Confidence::Overwritten
                                                : tree::Confidence::PartialRisk);
                ++stats.deleted_rows;
            } else {
                ++stats.files;
            }
        }

        /* Walk one directory table emitting rows under parent. */
        void walk_dir(u64 table_id, tree::FileNode* parent, u32 depth) {
            if (depth > MAX_TABLE_DEPTH || e.cancel_gate())
                return;
            auto it = tables.find(table_id);
            if (it == tables.end())
                return;
            TableInfo& ti = it->second;
            if (ti.visited)
                return; /* cycle guard */
            ti.visited = true;

            std::set<u64> visited;
            u32 nodes = 0;
            walk_table(
                e, ti.lcns, 0,
                [&](const EntryView& e2) {
                    if (e2.key_len < 4)
                        return true;
                    const u32 marker = le32(e2.key);
                    if (marker == ROW_FILE) {
                        const bool del = (e2.flags & ENTRY_DELETED) != 0;
                        if (!del)
                            emit_file(parent, e2, table_id, false);
                        else if (flags.harvest_deleted)
                            emit_file(del_root, e2, table_id, true);
                    } else if (marker == ROW_DIR_LINK && e2.val_len >= 0x48) {
                        const u64 child_tid = le64(e2.val + 0x08);
                        const std::u16string name16 = name_from_key(e2.key, e2.key_len);
                        if (name16.empty())
                            return true;
                        if (!(e2.flags & ENTRY_DELETED) && tables.count(child_tid)) {
                            tree::FileProperties* dp = new tree::FileProperties();
                            dp->fs_kind = REFS_FS_KIND;
                            dp->name = name16;
                            dp->attributes = 0x10; /* DIRECTORY */
                            dp->record_ref = child_tid;
                            /* directory-link metadata (thesis 4.28):
                             * times value+0x10..0x30, sizes, flags */
                            dp->times.created = le64(e2.val + 0x10);
                            dp->times.modified = le64(e2.val + 0x18);
                            dp->times.changed = le64(e2.val + 0x20);
                            dp->times.accessed = le64(e2.val + 0x28);
                            dp->logical_size = le64(e2.val + 0x38);
                            dp->allocated_size = le64(e2.val + 0x30);
                            tree::FileNode* dn = tree.add_file(parent, dp);
                            dp->release();
                            ++stats.dirs;
                            walk_dir(child_tid, dn, depth + 1);
                        } else if ((e2.flags & ENTRY_DELETED) && flags.harvest_deleted) {
                            tree::FileProperties* dp = new tree::FileProperties();
                            dp->fs_kind = REFS_FS_KIND;
                            dp->name = name16;
                            dp->attributes = 0x10;
                            dp->record_ref = child_tid;
                            dp->times.created = le64(e2.val + 0x10);
                            dp->times.modified = le64(e2.val + 0x18);
                            tree::FileNode* dn = tree.add_file(del_root, dp);
                            dp->release();
                            dn->set_deleted_state(tree::DeletedState::Deleted);
                            dn->set_confidence(tree::Confidence::MetadataOnly);
                            ++stats.deleted_rows;
                        }
                    } else if (marker == ROW_ID2 && flags.harvest_deleted) {
                        /* ID2 (thesis 4.26): value = {u64 type,
                         * u16 nameOff, u16 nameLen, name} for type 1
                         * (name evidence of deleted/moved files) */
                        if ((e2.flags & ENTRY_DELETED) != 0 && e2.val_len >= 0x0C) {
                            const u64 type = le64(e2.val);
                            if (type == 1) {
                                const u16 name_off = le16(e2.val + 0x08);
                                const u16 name_len = le16(e2.val + 0x0A);
                                if (name_off >= 0x0C && u64(name_off) + name_len <= e2.val_len &&
                                    name_len > 0) {
                                    std::u16string name16;
                                    const u8* nm = e2.val + name_off;
                                    for (u32 i = 0; i + 1 < name_len; i += 2)
                                        name16.push_back(u16(nm[i]) | (u16(nm[i + 1]) << 8));
                                    if (!name16.empty()) {
                                        tree::FileProperties* p2 = new tree::FileProperties();
                                        p2->fs_kind = REFS_FS_KIND;
                                        p2->name = name16;
                                        tree::FileNode* n2 = tree.add_file(del_root, p2);
                                        p2->release();
                                        n2->set_deleted_state(tree::DeletedState::Deleted);
                                        n2->set_confidence(tree::Confidence::MetadataOnly);
                                        ++stats.deleted_rows;
                                    }
                                }
                            }
                        }
                    } else if (marker == ROW_DIR_DESCRIPTOR) {
                        /* directory's own metadata lives in the
                         * embedded node; nothing to emit (the
                         * directory link already carried it) */
                    }
                    return !e.cancelled();
                },
                visited, nodes);
        }
    };

    Walker w{eng, tree, root_deleted, flags, alloc, stats, tables, child_tables};
    u64 root_id = ROOT_DIR_TABLE;
    if (tables.find(root_id) == tables.end()) {
        /* fallback: a directory table that is nobody's child */
        for (const auto& kv : tables) {
            if (kv.first == FSMETADATA_TABLE || child_tables.count(kv.first))
                continue;
            root_id = kv.first;
            break;
        }
    }
    if (tables.find(root_id) == tables.end())
        return ERR_BAD_MAGIC;
    w.walk_dir(root_id, root_active, 0);

    /* ---- 9. orphan tables -> lost tree ---- */
    if (root_lost != nullptr) {
        for (const auto& kv : tables) {
            if (kv.second.visited || kv.first == FSMETADATA_TABLE || kv.first == root_id ||
                child_tables.count(kv.first))
                continue;
            std::set<u64> visited;
            u32 nodes = 0;
            walk_table(
                eng, kv.second.lcns, 0,
                [&](const EntryView& e2) {
                    if (e2.key_len >= 4 && le32(e2.key) == ROW_FILE)
                        w.emit_file(root_lost, e2, kv.first, false);
                    return !eng.cancelled();
                },
                visited, nodes);
            ++stats.orphan_tables;
        }
    }

    if (progress)
        (void)progress(1, 1, progress_user);
    return OK;
}

} // namespace refs
} // namespace cr
