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
#include <cstdio>
#include <cstdlib>
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

/* ---- attribute type ids (thesis table 4.1 + og key+0 spaces) ---- */
constexpr u16 ATTR_DATA = 0x80; /* thesis/core grammar: type u16 @key+8 */
/* og grammar (ReFSSearchDLL FUN_18000ba90 :8348-8427, type u16 @key+0):
 *   0x178 old-layout data-run rows (gen < 8); >= 0xb1 with 0x28-byte keys
 *   (gen >= 8, modern); 0x108/0xf8 property-flag rows; 100 ("$I…"
 *   recycle) and subtype-1 specials -> resident materialization
 *   (FUN_18000ea30 / FUN_18000e490). */
constexpr u16 OG_ROW_DATARUN_OLD = 0x178;
constexpr u16 OG_ROW_FLAG_A = 0x108;
constexpr u16 OG_ROW_FLAG_B = 0xf8;
constexpr u16 OG_ROW_RECYCLE_I = 100;

/* ---- data-run flags (thesis table 4.38 + og FUN_18000c790 :8880-8895) ---- */
constexpr u16 RUN_HAS_DATA = 0x0010;  /* core/thesis grammar */
constexpr u16 RUN_HAS_BLOCK = 0x0040; /* og: entry carries a block reference */
constexpr u16 RUN_PHYSICAL = 0x0020;  /* og: physical LCN, no container translate */
constexpr u16 RUN_REF_SIZE_OK = 0x18; /* og: ref-size u16@+0xa must equal 0x18 */

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
    /* M-026: EVERY first-LCN read during the live walk (unbounded, unlike
     * the bounded cache) - the stale-page sweep's live-reachability set */
    std::set<u64> live_pages;

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
        live_pages.insert(first); /* M-026 */
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
    /* S-020 resident materialization: the page the entry came from (all
     * entries of one page share these) - the og computes content positions
     * through a record-context offset chain (FUN_18000e490 :10365-75
     * value{+8,+0xc} + node{+0x5c,+0x60} + caller); the pointer form is the
     * exact equivalent with the buffers already in hand. */
    const u8* page_buf = nullptr;
    u64 page_byte_base = 0;
    u32 page_len = 0;
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
    const u64 page_base = first * e.boot.cluster_bytes;
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
        ent.page_buf = page.data();
        ent.page_byte_base = page_base;
        ent.page_len = u32(page.size());
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

/* $DATA value = embedded data-run table root node (thesis 4.38 rows);
 * superseded by attr_walk_runs (S-020) which also follows paged tables.
 * The thesis key grammar (type u16 @key+8, ATTR_DATA 0x80) is consumed
 * inline in emit_file's attribute loop. */

/* ---- S-020 / M-022: the og's attribute-value walk ----------------------
 *
 * The og iterates EVERY attribute row of a file's embedded table
 * (FUN_18000ba90 :8348-8427), dispatching on the type u16 at key+0:
 *   0x178 -> old-layout data-run rows (gen < 8); gen >= 8 modern rows are
 *            key-size 0x28 with type >= 0xb1 (outer gate) / >= 0x29 (the
 *            value handler FUN_18000c560 :8776-8800);
 *   0x108 / 0xf8 -> property-flag markers (bits 1 / 3);
 *   key-size 0x10 + type 100 + "$I" name -> the recycle-$I resident
 *            materializer FUN_18000ea30;
 *   key-size 0x10 + object subtype 1 -> the resident materializer
 *            FUN_18000e490 (inline file content).
 * Inside a data-run value table, entries are either run entries
 * (FUN_18000c790: flags & 0x40 required, ref-size u16@+0xa == 0x18, block
 * u64@+0, cluster-offset u32@+0xc, cluster-count u32@+0x14; flag 0x20 =
 * physical, else container-translated; block 0 -> skipped) or indirect node
 * records whose pages are chased and merged (FUN_18000cf30) - inner-node
 * run tables are FOLLOWED, never rejected. After the loop the runlist is
 * granularity-validated (FUN_18000e020 :10201-81: pow2-floor(avg sectors
 * per run cluster) must equal sectors-per-cluster; mismatch discards the
 * whole runlist - a garbage-run false-positive guard).
 *
 * The core's thesis grammar (type u16@key+8 == 0x80) stays alongside the og
 * grammar - both are accepted until a real-volume dump (OQ-2 in the M-022
 * finding) settles which generations carry which key layout; the existing
 * synthetic fixtures pin the 0x80 form. */

struct AttrWalkCtx {
    Engine& e;
    std::vector<tree::FileProperties::Extent>& extents; /* byte-space, merged */
    u64 cluster_bytes = 0;
    u32 spc = 0; /* sectors per cluster */
    u64 volume_clusters = 0;
    u64 volume_bytes = 0;
    RefsScanStats* stats = nullptr;
    u64 run_clusters = 0; /* total clusters across merged runs */
    u32 hops = 0;         /* indirect-chase bound */
};

/* append a cluster-space run, coalescing when adjacent (og c790/e020) */
static void attr_push_run(AttrWalkCtx& c, u64 start, u32 len) {
    if (len == 0 || len >= 0x10000000u)
        return;
    if (!c.extents.empty()) {
        auto& last = c.extents.back();
        const u64 last_start = last.offset / c.cluster_bytes;
        const u64 last_len = last.length / c.cluster_bytes;
        if (last_start + last_len == start && last.length % c.cluster_bytes == 0) {
            last.length += u64(len) * c.cluster_bytes;
            c.run_clusters += len;
            return;
        }
    }
    const u64 off = start * c.cluster_bytes;
    const u64 l = u64(len) * c.cluster_bytes;
    /* P1-9 overflow-safe bound (emit-time check repeated for the merge) */
    if (start >= c.volume_clusters || l > c.volume_bytes || off > c.volume_bytes - l)
        return;
    c.extents.push_back({off, l});
    c.run_clusters += len;
}

/* walk one attribute value as a run table: INNER nodes chase their child
 * page references (og FUN_18000cf30 - paged run tables are followed, never
 * rejected); LEAF entries are run rows under the og recognizer (flags &
 * 0x40 + ref-size 0x18) or the thesis grammar (flags & 0x10). Depth- and
 * hop-bounded recursion. */
static void attr_walk_runs(AttrWalkCtx& c, const u8* node, u32 node_len, u32 depth) {
    if (depth > MAX_TABLE_DEPTH || c.hops > 64)
        return;
    const NodeView nv = parse_node(node, node_len);
    if (!nv.ok)
        return;
    if ((nv.flags & NODE_INNER) != 0) {
        /* inner node: entries carry LCN-quad page references */
        for (u32 i = 0; i < nv.ki_count; ++i) {
            EntryView ent = entry_at(nv, i);
            if (!ent.ok || ent.val_len < 0x20)
                continue;
            u64 cl[4];
            for (int k = 0; k < 4; ++k)
                cl[k] = le64(ent.val + k * 8);
            if (cl[0] == 0)
                continue;
            ++c.hops;
            std::vector<u8> page;
            if (c.e.read_page(cl, page) && page.size() > 0x50) {
                if (c.stats)
                    ++c.stats->paged_chases;
                attr_walk_runs(c, page.data() + 0x50, u32(page.size() - 0x50), depth + 1);
            }
        }
        return;
    }
    for (u32 i = 0; i < nv.ki_count; ++i) {
        EntryView ent = entry_at(nv, i);
        if (!ent.ok || ent.val_len < 0x18)
            continue;
        /* run entry: og recognizer (0x40 + refsize) or thesis grammar (0x10) */
        const u16 fl = le16(ent.val + 8);
        const bool og_run = (fl & RUN_HAS_BLOCK) != 0 && le16(ent.val + 0xa) == RUN_REF_SIZE_OK;
        const bool thesis_run = (fl & RUN_HAS_DATA) != 0;
        if (!og_run && !thesis_run)
            continue;
        u64 start = le64(ent.val + 0x00);
        const u32 off = le32(ent.val + 0xc);
        const u32 len = le32(ent.val + 0x14);
        if (og_run && (fl & RUN_PHYSICAL) == 0)
            start = c.e.translate_cluster(start); /* container-relative */
        if (start == 0)
            continue; /* og: block 0 -> no reference */
        attr_push_run(c, start + off, len);
    }
}

/* resident/inline content materialization (og FUN_18000e490 / FUN_18000ea30):
 * locate the inline bytes by the value's offset pair + the entry's own page
 * position, enforce the og gates (value length == type, cluster <= 64 KiB,
 * size <= 64 KiB, run < 4 clusters) and emit ONE synthetic extent covering
 * the content (the og builds a CRefsResidentDataRunList - same placement,
 * deferred read). Returns true when a resident extent was materialized. */
static bool attr_materialize_resident(AttrWalkCtx& c, const EntryView& ae, u16 og_type,
                                      u64 logical_size, u64 msb_base) {
    if (ae.val_len != og_type)
        return false; /* og gate: value length == the type field */
    if (ae.val_len < 0x10 || c.cluster_bytes == 0 || c.cluster_bytes > 0x10000)
        return false;
    if (logical_size == 0 || logical_size > 0x10000)
        return false; /* resident gate: <= 64 KiB */
    if (ae.val_len < 0x10 || ae.page_buf == nullptr)
        return false;
    /* content position: value offset pair + the entry's position in its page
     * (the og's record-context chain, pointer form) */
    const u32 o1 = le32(ae.val + 8);
    const u32 o2 = le32(ae.val + 0xc);
    const i64 ent_pos = ae.val - ae.page_buf;
    if (ent_pos < 0 || u64(ent_pos) + o1 + o2 >= ae.page_len)
        return false;
    const u64 content_abs = ae.page_byte_base + u64(ent_pos) + o1 + o2;
    u64 block = content_abs / c.cluster_bytes;
    const u64 rem = content_abs % c.cluster_bytes;
    if (block < 4)
        block += msb_base; /* og: reserved low clusters live in the MSB base */
    block = c.e.translate_cluster(block);
    u32 run_clusters = u32((logical_size + c.cluster_bytes - 1) / c.cluster_bytes);
    if (rem != 0)
        ++run_clusters; /* the og's +1 slack when content straddles */
    if (run_clusters >= 4)
        return false; /* og gate: resident runs stay under 4 clusters */
    const u64 off = block * c.cluster_bytes + rem;
    const u64 l = u64(run_clusters) * c.cluster_bytes;
    if (off > c.volume_bytes || l > c.volume_bytes - off)
        return false;
    c.extents.push_back({off, l});
    if (c.stats)
        ++c.stats->resident_files;
    return true;
}

/* og granularity validation (FUN_18000e020 :10201-81): the pow2-floored
 * average sector-granularity of the merged runs must equal the volume's
 * sectors-per-cluster; mismatch discards the whole runlist (the og reads
 * the props' allocated size at +0 - the alloc-vs-run-count consistency
 * invariant; logical size only when no allocation is recorded). */
static bool attr_runs_granularity_ok(const AttrWalkCtx& c, u64 allocated_size, u64 logical_size) {
    if (c.run_clusters == 0 || c.spc == 0)
        return true; /* no non-resident runs: nothing to validate */
    u64 bytes = allocated_size != 0 ? allocated_size : logical_size;
    if (bytes == 0)
        return true; /* sizes unknown: nothing to validate against */
    const u64 bps = c.e.boot.bytes_per_sector ? c.e.boot.bytes_per_sector : 512;
    const u64 sectors = bytes / bps + (bytes % bps != 0 ? 1 : 0);
    const u64 avg = sectors / c.run_clusters;
    u64 pow = 0;
    for (u64 bit = 1; bit <= avg; bit <<= 1)
        pow = bit; /* largest power of two <= avg (0 when avg == 0) */
    return pow == c.spc;
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
    /* N-013b (MINOR): the og's bps gate is the full powers-of-two ladder
     * {0x100,0x200,0x400,0x800,0x1000} = {256,512,1024,2048,4096}
     * (FUN_180012390:13544-13546, re-verified first-hand this session:
     * `uVar3 == 0x100 || uVar3 == 0x200 || uVar3 == 0x400 || uVar3 == 0x800
     * || uVar3 == 0x1000`). The old core accepted only {512,4096} - a
     * 256/1024/2048-bps volume the og opens was refused outright. */
    if (bps != 256 && bps != 512 && bps != 1024 && bps != 2048 && bps != 4096)
        return false;
    const u32 spc = le32(sector + 0x24);
    if (spc == 0)
        return false;
    /* N-013b: the og gates the CLUSTER product, not spc itself
     * (:13547-13549: `uVar6 = spc * bps; uVar6 == 0x1000 ||
     * uVar6 == 0x10000`) - ReFS formats only ever carry 4-KiB or 64-KiB
     * clusters (the og accepts no other shape). The old core accepted any
     * pow2 spc <= 128 (any pow2 cluster up to 512 KiB) - an unregistered
     * superset over shapes the og refuses; parity closes on the product
     * gate (a subsumed spc sanity: only integral spc can hit the set). */
    const u64 cluster = u64(bps) * spc;
    if (cluster != 4096 && cluster != 65536)
        return false;
    const u64 sectors = le64(sector + 0x18);
    if (sectors == 0)
        return false;
    out.valid = true;
    out.bytes_per_sector = bps;
    out.sectors_per_cluster = spc;
    out.cluster_bytes = cluster;
    out.volume_clusters = sectors / spc;
    out.volume_bytes = sectors * bps;
    out.major = sector[0x28];
    out.minor = sector[0x29];
    out.serial = le64(sector + 0x38);
    out.container_bytes = le64(sector + 0x40); /* M-026: MSB/OID area bound */
    return out.major >= 1 && out.major <= 3;
}

u32 scan_refs(IStorageDevice* slice, const RefsScanFlags& flags, CancelToken* cancel,
              WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
              tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
              RefsScanStats& stats) {
    if (slice == nullptr || root_active == nullptr)
        return ERR_BAD_PARAMETER;
    (void)root_lost;

    /* GAP-S6-3 diagnostics: CR_REFS_DEBUG=1 dumps every detection gate's
     * verdict + the first 64 bytes of each probed page to stderr, so a
     * failing REAL volume is pinpointed from the bench stage logs (the
     * synthetic fixtures can only prove what they model - run 40079065019
     * proved the gap the hard way). Zero cost when unset. */
    const bool dbg = [] {
#ifdef _WIN32
        char v[8] = {};
        size_t n = 0;
        return getenv_s(&n, v, sizeof(v), "CR_REFS_DEBUG") == 0 && n > 0 && v[0] == '1';
#else
        const char* v = getenv("CR_REFS_DEBUG");
        return v != nullptr && v[0] == '1';
#endif
    }();
    auto dump = [&](const char* what, const u8* p, size_t n) {
        if (!dbg)
            return;
        std::fprintf(stderr, "[refs-dbg] %s:", what);
        for (size_t i = 0; i < n; ++i)
            std::fprintf(stderr, "%s%02x", (i & 15) == 0 ? "\n  " : " ", unsigned(p[i]));
        std::fprintf(stderr, "\n");
    };

    /* ---- 1. boot ---- */
    /* N-013d (MINOR): the og opens the volume with a 4-KiB read
     * (FUN at :2425-2444: alloc(0x1000) + read + FUN_180012390(...,
     * 0x1000); its gate needs >= 0x200). The old core read 512 B -
     * harmless on every real volume but not the og's shape; aligned
     * here, with the sub-4-KiB slice narrow-read kept for degenerate
     * synthetic images only. */
    u8 sec[4096];
    u32 open_len = 4096;
    if (slice->read_at(0, sec, open_len) != OK) {
        open_len = 512;
        if (slice->read_at(0, sec, open_len) != OK)
            return ERR_READ_FAILED;
    }
    dump("DBR sector 0", sec, 64);
    RefsBoot boot;
    if (!validate_boot(sec, boot)) {
        if (dbg)
            std::fprintf(stderr,
                         "[refs-dbg] boot REJECTED: oem=%.4s fsrs=%08x bps=%u spc=%u major=%u "
                         "minor=%u sectors=%llu\n",
                         sec + 3, le32(sec + 0x10), le32(sec + 0x20), le32(sec + 0x24),
                         unsigned(sec[0x28]), unsigned(sec[0x29]),
                         static_cast<unsigned long long>(le64(sec + 0x18)));
        return ERR_BAD_MAGIC;
    }

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
    if (dbg)
        std::fprintf(stderr,
                     "[refs-dbg] boot ok: bps=%u spc=%u cluster=%u clusters=%llu major=%u "
                     "minor=%u page=%u x %u clusters\n",
                     boot.bytes_per_sector, boot.sectors_per_cluster, unsigned(boot.cluster_bytes),
                     static_cast<unsigned long long>(boot.volume_clusters), boot.major, boot.minor,
                     eng.page_bytes, eng.page_clusters);

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
    if (!have_sup) {
        if (dbg)
            std::fprintf(stderr,
                         "[refs-dbg] superblock NOT FOUND (probed clusters 30, %llu, %llu)\n",
                         static_cast<unsigned long long>(boot.volume_clusters - 3),
                         static_cast<unsigned long long>(boot.volume_clusters - 2));
        return ERR_BAD_MAGIC;
    }
    dump("superblock page", sup.data(), 64);

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
    if (dbg)
        std::fprintf(stderr,
                     "[refs-dbg] checkpoint probes: cluster %llu -> %s, cluster %llu -> %s "
                     "(vol_sig=%08x)\n",
                     static_cast<unsigned long long>(chkp_clusters[0]),
                     ok1 ? "CHKP+sig-ok" : "FAIL",
                     static_cast<unsigned long long>(chkp_clusters[1]),
                     ok2 ? "CHKP+sig-ok" : "FAIL", vol_sig);
    if (ok1)
        dump("checkpoint page", chkp.data(), 64);
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
    /* GAP-S6-3 (bench 40079065019: real Server-2022 ReFS 3.7 refused with
     * ERR_BAD_MAGIC in ~2 ms): the layout gate must read the CHECKPOINT
     * BODY minor - og FUN_180012650: `lVar7 = 0x58; if (*(ushort
     * *)(param_2 + uVar14 + 6) < 0xe) lVar7 = 0x44;` where param_2+uVar14
     * IS the body - not the VBR minor (7 on ReFS 3.7). The VBR and the
     * body layout version are independent: a 3.7 volume carries VBR
     * minor=7 with a body layout minor >= 14. The old code keyed on
     * boot.minor and read the WRONG descriptor list on every such volume
     * -> table_root(0) garbage -> BAD_MAGIC. */
    const u32 ptr_lst_off =
        le16(chkp.data() + 0x56) >= 0x0E ? 0xA8 : 0x94; /* body+0x58 / body+0x44 */
    if (dbg)
        std::fprintf(stderr,
                     "[refs-dbg] checkpoint body: major=%u minor=%u ptr_count=%u list@0x%x "
                     "(gated on the BODY minor, og FUN_180012650)\n",
                     le16(chkp.data() + 0x54), le16(chkp.data() + 0x56), ptr_count, ptr_lst_off);
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
        /* M-031 (diff-to-fix): per-file progress — the og's quick-searcher
         * registers a per-file progress binder bool(u64, RefsFileProperties*&)
         * (ReFSSearchDLL :617-619, RTTI-verified); the engine reports
         * progress per file with the file in hand. Our progress ABI is
         * (done, total): every kProgressEvery emitted files we report
         * (done, done + kProgressEvery) — a growing-estimate total, exactly
         * the FAT engine's dentry pattern; the orchestrator clamps the
         * permille monotonic. The terminal tick reports (emitted, emitted)
         * = 1000 permille. */
        WalkProgressLike progress = nullptr;
        void* progress_user = nullptr;
        u64 emitted = 0;
        /* M-027: object-identity keep-one. The og keys its map on the
         * record's node id (props+0x3a, FUN_18000d970 :9905-34); the core's
         * synthetic row model has no object-id field, so identity is the
         * CONTENT-PLACEMENT signature {first extent offset, total extent
         * bytes, logical size} - two rows of one hard-linked object share
         * placement exactly; different objects never share live placement.
         * Documented simplification (the finding's own proposed rule:
         * first-wins with live-over-deleted preference). */
        struct PlacementSig {
            u64 first_offset = 0;
            u64 total_bytes = 0;
            u64 logical = 0;
            u64 created = 0, modified = 0; /* same-object rows share times */
            bool operator<(const PlacementSig& o) const {
                if (first_offset != o.first_offset)
                    return first_offset < o.first_offset;
                if (total_bytes != o.total_bytes)
                    return total_bytes < o.total_bytes;
                if (logical != o.logical)
                    return logical < o.logical;
                if (created != o.created)
                    return created < o.created;
                return modified < o.modified;
            }
        };
        std::map<PlacementSig, tree::FileNode*> identity;
        /* M-027: recycle-shaped rows stage here and flush AFTER the main
         * results (og routing :9817-61: "$R*" / "S-*" / "System Volume
         * Information" -> the dedicated collection, flushed at
         * FUN_18000ef20:10881-911; NOT through the identity map) */
        struct StagedRow {
            tree::FileNode* parent;
            tree::FileProperties* props; /* caller-held ref */
        };
        std::vector<StagedRow> recycle_pending;
        /* M-027 step 3: "$I" resident evidence {token -> info} for the $R
         * remap (the $I layout mirrors the shell family's: version u32@0,
         * size u64@8, time u64@16, pathlen u32@24, path UTF-16@28) */
        struct DollarI {
            u64 size = 0;
            u64 time = 0;
            std::u16string orig_name;
        };
        std::map<std::u16string, DollarI> dollar_i;

        static bool is_recycle_name(const std::u16string& n) {
            if (n.size() >= 2 && n[0] == u'$' && n[1] == u'R')
                return true;
            if (n.size() >= 2 && n[0] == u'S' && n[1] == u'-')
                return true;
            return n == u"System Volume Information";
        }
        static std::u16string recycle_token(const std::u16string& n) {
            /* "$R<token>[.ext]" / "$I<token>[.ext]": the pairing token is
             * the chars after the marker, cut at the first '.' (the shell
             * convention the NTFS engine already models) */
            if (n.size() < 3 || (n[0] != u'$' && n[1] != u'R' && n[1] != u'I'))
                return {};
            std::u16string tok;
            for (size_t i = 2; i < n.size(); ++i) {
                if (n[i] == u'.')
                    break;
                tok.push_back(n[i]);
            }
            return tok;
        }

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
        void emit_file(tree::FileNode* parent, const EntryView& ev, u64 dir_id, bool deleted,
                       bool from_sweep = false) {
            /* M-031: the og's per-file progress (binder at :617-619); our
             * ABI carries (done, total-in-estimate) — throttled to every
             * 16th emit so a big table does not flood the bridge. */
            ++emitted;
            if (progress && (emitted & 0xF) == 0)
                (void)progress(emitted, emitted + 0x10, progress_user);
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

            /* S-020/M-022: the og attribute walk - EVERY row is dispatched
             * (no first-match break), under both grammars:
             *   thesis/core: type u16 @key+8 == 0x80, value >= 0x28 (the
             *                historical fixture grammar, kept as a superset
             *                until the real-volume dump settles the layouts)
             *   og: type u16 @key+0 - 0x178 old data rows / >=0xb1 modern
             *       (gen >= 8) / 0x108+0xf8 property flags / "$I" recycle /
             *       subtype-1 resident materialization
             * gen tag (og FUN_180004980 :2466-77): major 3 -> minor < 5: 3,
             * 5..10: 7, 11..12: 8, 13+: 9; modern grammar iff gen >= 8. */
            u32 gen = 0;
            if (e.boot.major == 3)
                gen =
                    e.boot.minor < 5 ? 3 : (e.boot.minor < 0xb ? 7 : (e.boot.minor < 0xd ? 8 : 9));
            AttrWalkCtx ctx{e,
                            props->extents,
                            e.boot.cluster_bytes,
                            e.boot.sectors_per_cluster,
                            e.boot.volume_clusters,
                            e.boot.volume_bytes,
                            &stats,
                            0,
                            0};
            const u64 msb_base = 0; /* single-container volumes: pages are
                                     * volume-absolute (no MSB rebase; the
                                     * og's +0x70 chain is context-form) */
            u32 og_prop_flags = 0;
            bool have_grammar_row = false;
            PlacementSig pending_sig{};
            bool pending_has_sig = false;
            for (u32 i = 0; i < nv.ki_count; ++i) {
                EntryView ae = entry_at(nv, i);
                if (!ae.ok)
                    continue;
                /* the embedded table lives inside the file row's value, in
                 * the same physical page: propagate the page context so the
                 * resident materialization can compute absolute positions */
                ae.page_buf = ev.page_buf;
                ae.page_byte_base = ev.page_byte_base;
                ae.page_len = ev.page_len;
                /* (A) thesis grammar: $DATA (0x80) at key+8 */
                if (ae.key_len >= 10 && le16(ae.key + 8) == ATTR_DATA && ae.val_len >= 0x28) {
                    attr_walk_runs(ctx, ae.val, ae.val_len, 0);
                    have_grammar_row = true;
                    continue; /* ALL such rows contribute (og: no break) */
                }
                /* (B) og grammar: type u16 at key+0 */
                if (ae.key_len < 2)
                    continue;
                const u16 og_type = le16(ae.key);
                if (og_type == OG_ROW_DATARUN_OLD) {
                    og_prop_flags |= 2;
                    attr_walk_runs(ctx, ae.val, ae.val_len, 0);
                    have_grammar_row = true;
                } else if (og_type == OG_ROW_FLAG_A) {
                    og_prop_flags |= 1;
                } else if (og_type == OG_ROW_FLAG_B) {
                    og_prop_flags |= 3;
                } else if (gen >= 8 && ae.key_len == 0x28 && og_type >= 0xb1) {
                    attr_walk_runs(ctx, ae.val, ae.val_len, 0);
                    have_grammar_row = true;
                } else if (ae.key_len == 0x10) {
                    /* recycle $I (type 100 + "$I" name) or the subtype-1
                     * resident content descriptor (FUN_18000ea30/e490) */
                    const bool is_dollar_i = og_type == OG_ROW_RECYCLE_I && name16.size() > 2 &&
                                             name16[0] == u'$' && name16[1] == u'I';
                    const bool is_resident_desc = le16(ae.key + 2) == 1;
                    if ((is_dollar_i || is_resident_desc) &&
                        attr_materialize_resident(ctx, ae, og_type, gi.size, msb_base)) {
                        have_grammar_row = true;
                        if (is_dollar_i)
                            ++ctx.stats->dollar_i_rows;
                    }
                }
            }
            (void)have_grammar_row;
            /* runlist granularity validation (og e020): discard garbage
             * runlists; the resident extent (if any) survives only when no
             * non-resident runs exist (og: resident first, runlist after) */
            if (!attr_runs_granularity_ok(ctx, gi.alloc_size, gi.size)) {
                props->extents.clear();
                ++ctx.stats->granularity_discards;
            }
            /* keep exactly one resident extent when runs supersede it (the
             * og's non-resident runlist overwrites the resident slot) */
            if (ctx.run_clusters > 0) {
                /* drop a resident extent pushed earlier in the loop */
                for (size_t k = 0; k < props->extents.size();) {
                    if (props->extents[k].length < 4 * e.boot.cluster_bytes &&
                        props->extents[k].length % e.boot.cluster_bytes == 0 &&
                        props->extents[k].offset % e.boot.cluster_bytes != 0) {
                        props->extents.erase(props->extents.begin() + long(k));
                        if (ctx.stats->resident_files > 0)
                            --ctx.stats->resident_files;
                    } else {
                        ++k;
                    }
                }
            }
            stats.data_runs += props->extents.size();
            (void)og_prop_flags; /* property flags tracked for og-parity
                                  * surfaces (grading consumers); not yet
                                  * consumed by the core's grading model */

            /* M-027: "$I" rows - read the resident evidence for the later
             * $R remap (the content bytes live at the materialized extent;
             * the walker has the slice) */
            if (name16.size() >= 3 && name16[0] == u'$' && name16[1] == u'I' &&
                props->extents.size() == 1 && props->extents[0].length <= 0x10000) {
                std::vector<u8> ibuf(size_t(props->extents[0].length), 0);
                if (e.slice->read_at(props->extents[0].offset, ibuf.data(), u32(ibuf.size())) ==
                        OK &&
                    ibuf.size() >= 28) {
                    const u32 ver = le32(ibuf.data());
                    if (ver == 1 || ver == 2) {
                        DollarI di;
                        di.size = le64(ibuf.data() + 8);
                        di.time = le64(ibuf.data() + 16);
                        const u32 plen = le32(ibuf.data() + 24);
                        if (plen >= 1 && plen <= 1024 && 28 + u64(plen) * 2 <= ibuf.size()) {
                            for (u32 i = 0; i < plen; ++i) {
                                const u16 ch =
                                    u16(ibuf[28 + i * 2]) | (u16(ibuf[28 + i * 2 + 1]) << 8);
                                if (ch == 0)
                                    break;
                                di.orig_name.push_back(ch);
                            }
                            const size_t slash = di.orig_name.rfind(u'\\');
                            if (slash != std::u16string::npos)
                                di.orig_name = di.orig_name.substr(slash + 1);
                            if (!di.orig_name.empty())
                                dollar_i[recycle_token(name16)] = di;
                        }
                    }
                }
            }

            /* M-027: recycle-shaped rows stage for the post-main flush (og
             * :9817-61 routing: NOT through the identity map) */
            if (is_recycle_name(name16)) {
                ++stats.recycle_routed;
                recycle_pending.push_back({parent, props}); /* keep the ref */
                if (deleted)
                    ++stats.deleted_rows;
                else
                    ++stats.files;
                return;
            }

            /* M-027: object-identity keep-one (the og's identity map +
             * FUN_18000ac40 compare; our placement-signature keying) */
            if (!props->extents.empty()) {
                PlacementSig sig;
                sig.first_offset = props->extents[0].offset;
                for (const auto& ex : props->extents)
                    sig.total_bytes += ex.length;
                sig.logical = props->logical_size;
                sig.created = props->times.created;
                sig.modified = props->times.modified;
                auto it = identity.find(sig);
                if (it != identity.end() && it->second != nullptr) {
                    /* live-over-deleted promotion: a live row for an object
                     * whose earlier row was deleted reclaims the node (the
                     * og's keep-one resolves to the live copy) */
                    if (!deleted && it->second->deleted_state() != tree::DeletedState::Active) {
                        tree.reparent(it->second, parent);
                        it->second->set_deleted_state(tree::DeletedState::Active);
                        it->second->set_confidence(tree::Confidence::Excellent);
                    } else {
                        ++stats.identity_dedups;
                        props->release();
                        return; /* the loser is destroyed (og :10021-27) */
                    }
                }
                pending_sig = sig;
                pending_has_sig = true;
                (void)from_sweep; /* sweep rows participate in the same map */
            }

            tree::FileNode* node = tree.add_file(parent, props);
            props->release(); /* tree holds the reference */
            if (node == nullptr)
                return; /* arena OOM (P2-1) */
            if (pending_has_sig)
                identity[pending_sig] = node; /* first row of this object */
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

        /* M-027: recycle flush - the staged "$R…" / "S-…" / SVI rows emit
         * AFTER the main results (og ordering), then the $I evidence
         * remaps its $R siblings (the core-family recycle convention the
         * NTFS engine already models; the og's SearchDLL leaves the
         * rename to its UI layer - the evidence READ is the parity part).
         * Nodes are arena-stable: mutating props through a node's props()
         * is safe post-walk (the tree's hash indexes key on
         * record_ref/extent, never name). */
        void flush_recycle() {
            std::vector<tree::FileNode*> added;
            for (const StagedRow& st : recycle_pending) {
                tree::FileNode* n = tree.add_file(st.parent, st.props);
                st.props->release(); /* node holds the ref */
                if (n != nullptr)
                    added.push_back(n);
            }
            recycle_pending.clear();
            for (tree::FileNode* n : added) {
                tree::FileProperties* p = n->props();
                if (p == nullptr)
                    continue;
                const std::u16string& nm = p->name;
                if (nm.size() < 3 || nm[0] != u'$' || nm[1] != u'R')
                    continue;
                auto it = dollar_i.find(recycle_token(nm));
                if (it == dollar_i.end())
                    continue;
                /* recycle content: deleted from the user's location */
                p->name = it->second.orig_name;
                if (it->second.size != 0)
                    p->logical_size = it->second.size;
                p->times.modified = it->second.time;
                n->set_deleted_state(tree::DeletedState::Deleted);
                ++stats.dollar_i_pairs;
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
    w.progress = progress;
    w.progress_user = progress_user;
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

    /* ---- 8b. M-026: the stale-MSB-page sweep (og FUN_180005330, called
     * unconditionally after the live walks by FUN_180003f40:2015) ----
     *
     * ReFS is copy-on-write: deleted objects' metadata persists as stale
     * MSB page versions the live chain no longer references. The og scans
     * the object-ID area in 0x80-cluster windows for "MSB+" pages that are
     * NOT in the live page cache (:2904 sig, :2910-31 cache-skip), parses
     * their records, force-marks every record deleted (record+0x59 := 1,
     * :2939) and runs the SAME live dispatch (:2942-65) - stale pages yield
     * deleted-file results with full names/timestamps/runs. The area bound:
     * the og reads the device's OID allocated ranges (container size /
     * cluster, FUN_180012390:13560 -> ctx+0x148); our engine bounds the
     * sweep by min(volume, DBR container/cluster) - the same region on
     * standard layouts. Live pages are excluded by the unbounded
     * live_pages set (the bounded cache would evict on large volumes).
     * Bounded simplification: only ROW_FILE rows are dispatched - stale
     * dir-links reference tables outside the live table map (the og's
     * handler resolves them through its own machinery); the orphan-table
     * walk covers unreachable live tables. */
    if (flags.stale_sweep) {
        const u64 area_clusters = eng.boot.container_bytes != 0
                                      ? eng.boot.container_bytes / eng.boot.cluster_bytes
                                      : eng.boot.volume_clusters;
        u64 bound = eng.boot.volume_clusters;
        if (area_clusters != 0 && area_clusters < bound)
            bound = area_clusters;
        const u64 window = 0x80; /* og chunking */
        std::vector<u8> page(eng.page_bytes, 0);
        for (u64 base = 0; base < bound; base += window) {
            if (eng.cancel_gate())
                break; /* S32 parity: cancel between windows, partials kept */
            for (u64 c = base; c < base + window && c < bound; c += eng.page_clusters) {
                const u64 off = c * eng.boot.cluster_bytes;
                if (off + eng.page_bytes > eng.boot.volume_bytes)
                    break;
                if (eng.slice->read_at(off, page.data(), eng.page_bytes) != OK)
                    break;
                if (le32(page.data()) != SIG_MSB)
                    continue;
                if (le32(page.data() + 0x0C) != eng.volume_sig)
                    continue; /* signature-validated pages only */
                if (eng.live_pages.count(c) != 0)
                    continue; /* reachable from the live chain: not stale */
                const NodeView nv = parse_node(page.data() + 0x50, page.size() - 0x50);
                if (!nv.ok)
                    continue;
                ++stats.stale_pages;
                for (u32 i = 0; i < nv.ki_count; ++i) {
                    EntryView ent = entry_at(nv, i);
                    if (!ent.ok || ent.key_len < 4 || le32(ent.key) != ROW_FILE)
                        continue;
                    ent.page_buf = page.data();
                    ent.page_byte_base = off;
                    ent.page_len = u32(page.size());
                    /* force-deleted (og :2939) - and the identity map lets
                     * live rows win over stale copies of the same object */
                    w.emit_file(root_deleted, ent, 0, true, /*from_sweep=*/true);
                }
            }
        }
    }

    /* ---- 8c. M-027: the recycle flush + $I remap (og FUN_18000ef20:
     * the dedicated collection emits AFTER the main map :10881-911) ---- */
    w.flush_recycle();

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
        (void)progress(w.emitted, w.emitted, progress_user); /* terminal: 1000 permille */
    return OK;
}

} // namespace refs
} // namespace cr
