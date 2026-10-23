/* cr_fat_quick.cpp - FAT scan engine: dentry walk, LFN glue, deleted chains.
 *
 * R5.1: deleted entries kept + chain reconstruction (complete mode);
 * fragmented-file confidence via chain-length-vs-expected grading.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/fat/cr_fat_engine.h"

#include <cstring>
#include <set>

namespace cr {
namespace fat {

namespace {

struct DirBuffer {
    std::vector<u8> data;
    bool valid = false;
};

/* Read a directory's content by following its cluster chain. Cycle guard
 * inside follow_chain (fix F7). Whole-run batched reads per contiguous run. */
DirBuffer read_dir_chain(IStorageDevice* dev, const FatParams& p, const FatTable& t,
                         u32 first_cluster, u32 max_bytes) {
    DirBuffer d;
    ChainResult chain = follow_chain(t, first_cluster, 0x10000);
    if (chain.clusters.empty())
        return d;
    u64 total = u64(chain.length()) * p.cluster_bytes;
    if (total > max_bytes)
        total = max_bytes;
    d.data.resize(size_t(total));
    u64 filled = 0;
    /* contiguous runs */
    size_t i = 0;
    while (i < chain.clusters.size() && filled < total) {
        size_t j = i;
        while (j + 1 < chain.clusters.size() && chain.clusters[j + 1] == chain.clusters[j] + 1)
            ++j;
        u64 run_off = FatTable::cluster_offset(p, chain.clusters[i]);
        u32 run_len = u32((j - i + 1) * p.cluster_bytes);
        if (filled + run_len > total)
            run_len = u32(total - filled);
        if (dev->read_at(run_off, d.data.data() + filled, run_len) != OK) {
            /* partial: zero the remainder (Skip policy in transport layer) */
            std::memset(d.data.data() + filled, 0, run_len);
        }
        filled += run_len;
        i = j + 1;
    }
    d.valid = filled > 0;
    return d;
}

struct EmitCtx {
    IStorageDevice* dev;
    FatParams p;
    FatTable* table;
    FatScanFlags flags;
    tree::ResultTree* tree;
    tree::FullNode* root_active;
    tree::FullNode* root_deleted;
    tree::FullNode* root_lost;
    FatScanStats* stats;
    CancelToken* cancel = nullptr;               /* pause-aware walk (G-07) */
    bool (*progress)(u64, u64, void*) = nullptr; /* live progress (G-04) */
    void* progress_user = nullptr;
    u64 dentries_done = 0;      /* cumulative dentries examined (progress unit) */
    std::set<u64> visited_dirs; /* cycle guard for dir recursion */
    int depth = 0;
    constexpr static int MAX_DEPTH = 64;
};

tree::Confidence grade_deleted_chain(const ChainResult& chain, u32 expect_clusters) {
    if (chain.clusters.empty())
        return tree::Confidence::Overwritten;
    if (chain.cycle || chain.broken)
        return tree::Confidence::PartialRisk;
    if (expect_clusters == 0 || chain.length() >= expect_clusters)
        return tree::Confidence::Excellent;
    if (chain.length() * 2 >= expect_clusters)
        return tree::Confidence::PartialRisk;
    return tree::Confidence::Overwritten;
}

void emit_entry(EmitCtx& ctx, const DirEntry& e, const std::u16string& name, bool deleted,
                tree::FileNode* parent_host, tree::FullNode* deleted_root);

void walk_dir_entries(EmitCtx& ctx, const DirBuffer& buf, tree::FileNode* host,
                      tree::FullNode* deleted_host);

void recurse_dir(EmitCtx& ctx, u32 first_cluster, tree::FileNode* host_node, bool deleted) {
    if (ctx.depth >= EmitCtx::MAX_DEPTH)
        return;
    /* visited-dir cycle guard: use first cluster as identity */
    u64 key = first_cluster;
    if (ctx.visited_dirs.count(key)) {
        ++ctx.stats->cycles_cut;
        return;
    }
    ctx.visited_dirs.insert(key);
    ++ctx.depth;
    DirBuffer d = read_dir_chain(ctx.dev, ctx.p, *ctx.table, first_cluster, 4u << 20);
    if (d.valid) {
        /* '.' self-entry check (corpus): confirms real directory content */
        walk_dir_entries(ctx, d, host_node, deleted ? ctx.root_lost : nullptr);
    }
    --ctx.depth;
}

void walk_dir_entries(EmitCtx& ctx, const DirBuffer& buf, tree::FileNode* host,
                      tree::FullNode* deleted_host) {
    if (!buf.valid || buf.data.empty())
        return;
    /* pause-aware gate + cancel poll per directory (G-07) */
    if (ctx.cancel && ctx.cancel->checkpoint())
        return;
    const size_t count = buf.data.size() / 32;
    std::vector<const LfnEntry*> pending_lfn;
    for (size_t i = 0; i < count; ++i) {
        /* live progress (G-04): one dentry examined per iteration - honest
         * cumulative counts; the orchestrator clamps permille monotonic. */
        ++ctx.dentries_done;
        if (ctx.progress && (ctx.dentries_done & 63) == 0)
            (void)ctx.progress(ctx.dentries_done, ctx.dentries_done + (count - i - 1),
                               ctx.progress_user);
        const u8* raw = buf.data.data() + i * 32;
        const DirEntry* e = reinterpret_cast<const DirEntry*>(raw);
        const u8 first = e->name[0];
        if (first == 0x00)
            break; /* end of directory (corpus DENTRY_END) */
        if (e->attributes == ATTR_LFN) {
            const LfnEntry* l = reinterpret_cast<const LfnEntry*>(raw);
            pending_lfn.push_back(l);
            continue;
        }
        if (e->attributes & ATTR_VOLUME_ID) {
            pending_lfn.clear();
            continue; /* volume label - not a result */
        }
        /* '.' / '..' anchors: skip as results (identity check only) */
        if ((e->name[0] == '.' && (e->name[1] == ' ' || e->name[1] == '.')) &&
            (e->attributes & ATTR_DIRECTORY)) {
            pending_lfn.clear();
            continue;
        }
        const bool deleted = (first == DENTRY_DELETED);
        /* name resolution: LFN (checksum-validated) else 8.3 */
        std::u16string name;
        u8 short_name[11];
        std::memcpy(short_name, e->name, 11);
        if (deleted)
            short_name[0] = '_'; /* undelete convention */
        if (short_name[0] == 0x05)
            short_name[0] = 0xE5;       /* KANJI escape */
        u8 chk = lfn_checksum(e->name); /* checksum over ORIGINAL 11 bytes */
        bool lfn_ok = false;
        if (!pending_lfn.empty()) {
            std::u16string lfn;
            if (lfn_assemble(pending_lfn, chk, lfn)) {
                name = lfn;
                lfn_ok = true;
                if (deleted)
                    ++ctx.stats->lfn_recovered;
            }
        }
        if (!lfn_ok)
            name = short_name_to_u16(short_name);
        pending_lfn.clear();
        if (name.empty())
            continue;

        emit_entry(ctx, *e, name, deleted, host, deleted_host);
    }
    if (ctx.progress && count)
        (void)ctx.progress(ctx.dentries_done, ctx.dentries_done, ctx.progress_user);
}

void emit_entry(EmitCtx& ctx, const DirEntry& e, const std::u16string& name, bool deleted,
                tree::FileNode* parent_host, tree::FullNode* deleted_root) {
    const u32 first_cluster = (u32(e.first_cluster_high) << 16) | e.first_cluster_low;
    const bool is_dir = (e.attributes & ATTR_DIRECTORY) != 0;
    const u32 expect_clusters = u32((e.file_size + ctx.p.cluster_bytes - 1) / ctx.p.cluster_bytes);

    /* chain resolution */
    ChainResult chain;
    if (!deleted) {
        chain = follow_chain(*ctx.table, first_cluster, 0x10000);
    } else if (ctx.flags.reconstruct_deleted_chains && first_cluster >= 2) {
        chain = reconstruct_deleted_chain(*ctx.table, first_cluster, is_dir ? 0 : expect_clusters);
    }

    tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
    if (props == nullptr)
        return;
    props->name = name;
    props->fs_kind = u32(ctx.p.flavor); /* 0=FAT12 1=FAT16 2=FAT32 (corpus codes) */
    props->logical_size = is_dir ? u64(chain.length()) * ctx.p.cluster_bytes : e.file_size;
    props->times.created =
        u64(fat_date_to_unix(e.creation_date, e.creation_time, e.creation_tenths)) * 10000000ULL +
        116444736000000000ULL; /* unix->100ns1601 */
    props->times.modified =
        u64(fat_date_to_unix(e.write_date, e.write_time, 0)) * 10000000ULL + 116444736000000000ULL;
    props->times.changed = props->times.modified;
    props->times.accessed =
        u64(fat_date_to_unix(e.access_date, 0, 0)) * 10000000ULL + 116444736000000000ULL;
    props->attributes = e.attributes;
    if (deleted)
        props->flags &= ~u16(0x10);
    else
        props->flags |= u16(0x10); /* corpus PFLAG_IN_USE bit4 */

    /* extents from contiguous chain runs (slice-relative) */
    size_t i = 0;
    while (i < chain.clusters.size()) {
        size_t j = i;
        while (j + 1 < chain.clusters.size() && chain.clusters[j + 1] == chain.clusters[j] + 1)
            ++j;
        u64 off = FatTable::cluster_offset(ctx.p, chain.clusters[i]);
        u64 len = u64(j - i + 1) * ctx.p.cluster_bytes;
        if (is_dir) {
            /* directory content extents recorded for completeness; size cap */
            if (props->extents.size() < 64)
                props->extents.push_back({off, len});
        } else {
            props->extents.push_back({off, len});
        }
        i = j + 1;
    }

    tree::FullNode* default_root =
        deleted ? (deleted_root ? deleted_root : ctx.root_deleted) : ctx.root_active;
    tree::FileNode* n = ctx.tree->add_file(parent_host ? parent_host : default_root, props);
    props->release();
    if (n == nullptr)
        return;
    n->set_origin(tree::Origin::Structured);
    if (deleted) {
        ++ctx.stats->deleted_entries;
        n->set_deleted_state(tree::DeletedState::Deleted);
        tree::Confidence g =
            is_dir ? tree::Confidence::PartialRisk : grade_deleted_chain(chain, expect_clusters);
        /* honesty cap: FAT unlinked the first cluster (raw == FREE) - the
         * chain was zeroed at delete time; content may be reused */
        if (!is_dir && first_cluster >= 2 && ctx.table->raw(first_cluster) == 0 &&
            g == tree::Confidence::Excellent)
            g = tree::Confidence::PartialRisk;
        n->set_confidence(g);
    } else {
        n->set_confidence(tree::Confidence::Excellent);
    }
    if (is_dir)
        ++ctx.stats->dirs;
    else
        ++ctx.stats->files;

    if (chain.broken)
        ++ctx.stats->chains_broken;

    /* recurse into subdirectories (both live and deleted trees) */
    if (is_dir && first_cluster >= 2) {
        recurse_dir(ctx, first_cluster, n, deleted);
    }
}

} // namespace

u32 scan_fat(IStorageDevice* slice, const FatScanFlags& flags, CancelToken* cancel,
             WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             FatScanStats& stats) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    u8 boot[512];
    if (slice->read_at(0, boot, sizeof(boot)) != OK)
        return ERR_READ_FAILED;
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    FatParams p;
    u32 rc = validate_boot(boot, slice->size(), p);
    if (rc != OK) {
        /* backup boot sector fallback (R5.1): sector 6 on FAT32 volumes is
         * a full copy of the boot sector - validate it independently.
         * Sector math uses the device geometry: on 4Kn volumes the backup
         * boot sits at 6 * 4096, not the hardcoded 3072 (audit row 9 -
         * violates the locked fix F1 'no fixed 512 anywhere'). */
        const u32 bs = slice->geometry().logical_sector ? slice->geometry().logical_sector : 512;
        u8 b[512];
        if (slice->read_at(u64(6) * bs, b, sizeof(b)) == OK) {
            FatParams bp;
            if (validate_boot(b, slice->size(), bp) == OK) {
                p = bp;
                rc = OK;
            }
        }
        if (rc != OK)
            return rc;
    }

    FatTable table;
    rc = table.load(slice, p);
    if (rc != OK)
        return rc;

    EmitCtx ctx;
    ctx.dev = slice;
    ctx.p = p;
    ctx.table = &table;
    ctx.flags = flags;
    ctx.tree = &tree;
    ctx.root_active = root_active;
    ctx.root_deleted = root_deleted;
    ctx.root_lost = root_lost;
    ctx.stats = &stats;
    ctx.cancel = cancel;
    ctx.progress = progress;
    ctx.progress_user = progress_user;

    /* root directory: FAT12/16 fixed area; FAT32 chain from root_cluster */
    if (p.flavor != Flavor::Fat32) {
        DirBuffer root;
        u64 len = u64(p.root_entries) * 32;
        root.data.resize(size_t(len));
        if (slice->read_at(p.root_dir_offset, root.data.data(), u32(len)) != OK)
            return ERR_READ_FAILED;
        root.valid = true;
        walk_dir_entries(ctx, root, root_active, root_deleted);
        if (ctx.cancel && ctx.cancel->checkpoint())
            return ERR_CANCELLED;
    } else {
        if (p.root_cluster < 2 || p.root_cluster > table.max_cluster())
            return ERR_BAD_PARAMETER;
        recurse_dir(ctx, p.root_cluster, nullptr, false);
        /* recurse_dir inserted the root cluster into visited_dirs; children
         * of the volume root attach under root_active directly */
        if (ctx.cancel && ctx.cancel->checkpoint())
            return ERR_CANCELLED;
    }
    if (progress)
        (void)progress(1, 1, progress_user);
    return OK;
}

} // namespace fat
} // namespace cr
