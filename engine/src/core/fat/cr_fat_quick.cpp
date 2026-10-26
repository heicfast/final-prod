/* cr_fat_quick.cpp - FAT scan engine: dentry walk, LFN glue, deleted chains.
 *
 * R5.1: deleted entries kept + chain reconstruction (complete mode);
 * fragmented-file confidence via chain-length-vs-expected grading.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/fat/cr_fat_engine.h"
#include "cr/cr_recycle.h" /* M-052: the shared $I/$R recycle model */

#include <cstring>
#include <map>
#include <set>

namespace cr {
namespace fat {

namespace {

struct DirBuffer {
    std::vector<u8> data;
    bool valid = false;
};

/* Read a directory's content by following its cluster chain. Cycle guard
 * inside follow_chain (fix F7). Whole-run batched reads per contiguous run.
 * The chain walker polls the cancel token at every cluster step (F-A1-11). */
DirBuffer read_dir_chain(IStorageDevice* dev, const FatParams& p, const FatTable& t,
                         u32 first_cluster, u32 max_bytes, const CancelToken* cancel) {
    DirBuffer d;
    ChainResult chain = follow_chain(t, first_cluster, 0x10000, cancel);
    if (chain.cancelled)
        return d; /* unwinding: no half-directory walks */
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
    /* M-052: the recycle-bin state. A bin ROOT ($RECYCLE.BIN / Recycled
     * / Recycler, case-blind - the corpus's _stricmp family) marks its
     * whole subtree: $I<token> streams are parsed for the original
     * name/size/time (never surfaced), $R<token> files surface under
     * their ORIGINAL name marked Deleted (in-use inside the bin, but
     * deleted from the user's location - the NTFS engine's model,
     * now shared through cr/recycle). */
    bool in_recycle = false;
    std::map<std::u16string, recycle::RecycleInfo> recycle_info;
    struct PendingR {
        tree::FileNode* node;
        std::u16string token;
    };
    std::vector<PendingR> pending_recycle_r;
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
    DirBuffer d = read_dir_chain(ctx.dev, ctx.p, *ctx.table, first_cluster, 4u << 20, ctx.cancel);
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
        /* per-dentry cancel poll (F-A1-11, og FUN_18000fb80 :11530 - the
         * og checks the token at every 32-byte entry, not per directory) */
        if (ctx.cancel && ctx.cancel->checkpoint())
            return;
        /* live progress (G-04): one dentry examined per iteration - honest
         * cumulative counts; the orchestrator clamps permille monotonic. */
        ++ctx.dentries_done;
        if (ctx.progress && (ctx.dentries_done & 63) == 0)
            (void)ctx.progress(ctx.dentries_done, ctx.dentries_done + (count - i - 1),
                               ctx.progress_user);
        const u8* raw = buf.data.data() + i * 32;
        const DirEntry* e = reinterpret_cast<const DirEntry*>(raw);
        const u8 first = e->name[0];
        if (first == DENTRY_END)
            break; /* end of directory (corpus DENTRY_END) */
        if (e->attributes == ATTR_LFN) {
            const LfnEntry* l = reinterpret_cast<const LfnEntry*>(raw);
            /* FAT-10 (AUDIT-FAT): og requires an LFN slot's first_cluster
             * (+0x1A) to be ZERO before treating an 0x0F entry as an LFN
             * slot (FUN_1800365a0:44287) - junk 0x0F-attr entries with
             * stray +0x1A bytes are otherwise absorbed into the name. */
            if (l->first_cluster != 0) {
                pending_lfn.clear();
                continue;
            }
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
        u8 chk = lfn_checksum(e->name); /* checksum over ORIGINAL 11 bytes */
        if (deleted) {
            /* GAP-S6-1 (bench 40079065019, probe-verified): the raw checksum
             * over 0xE5-led bytes can NEVER match the LFN slots' stored value
             * (computed at write time over the ORIGINAL first byte) - every
             * deleted-LFN file fell back to the '_'-mangled 8.3 name. The
             * stored checksum brute-forces the destroyed byte
             * deterministically (bijection over 256 candidates); success
             * restores BOTH the LFN validation target and the 8.3 fallback's
             * first character. */
            u8 recovered = 0, recovered_chk = 0;
            if (!pending_lfn.empty() &&
                lfn_deleted_checksum_recover(pending_lfn, e->name, recovered, recovered_chk)) {
                chk = recovered_chk;
                short_name[0] = recovered;
            } else {
                short_name[0] = '_'; /* undelete convention (no oracle) */
            }
        }
        if (short_name[0] == 0x05)
            short_name[0] = 0xE5; /* KANJI escape */
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

        /* N-010c (diff-to-fix): the og's dentry validator
         * (FUN_1800365a0 :44298-44315) rejects a dentry whose create
         * (+0x10) / write (+0x18) / access (+0x12) date field is nonzero
         * with month > 12 or day == 0 - corrupt-date dentries are
         * destroyed, not emitted. Whole-zero fields PASS (they flow to
         * the N-021 zero-FileTime conversion, exactly like the og).
         * Config-gated in the og (singleton +0x40); the flag mirrors
         * that gate, default OFF. */
        if (ctx.flags.strict_date_sanity) {
            const u16 dates[3] = {e->creation_date, e->write_date, e->access_date};
            bool bad_date = false;
            for (u16 d : dates) {
                if (d == 0)
                    continue; /* whole-zero field passes the og's gate */
                const u32 month = (d >> 5) & 0x0F;
                const u32 day = d & 0x1F;
                if (month > 12 || day == 0) {
                    bad_date = true;
                    break;
                }
            }
            if (bad_date) {
                ++ctx.stats->date_rejected;
                continue;
            }
        }

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

    /* chain resolution (F-A1-11: the walkers poll the token per step) */
    ChainResult chain;
    bool inferred_chain = false;
    if (!deleted) {
        chain = follow_chain(*ctx.table, first_cluster, 0x10000, ctx.cancel);
    } else if (ctx.flags.reconstruct_deleted_chains && first_cluster >= 2) {
        chain = reconstruct_deleted_chain(*ctx.table, first_cluster, is_dir ? 0 : expect_clusters,
                                          ctx.cancel);
        /* GAP-S6-2 (bench 40079065019: 46/10143 hash-true): an UNLINKED
         * deleted chain (FAT zeroed at delete time) reconstructs as a
         * single-cluster run while the dentry carries the full size - the
         * writer then emits plausible-size, wrong-byte files. The
         * contiguous-candidate heuristic completes the run when the linked
         * walk came up short AND the first cluster is FREE (the freed-chain
         * signature): [start, start+expect) validated against the FAT
         * free-state. Inferred chains are flagged and never graded
         * Excellent (the honesty cap below enforces PartialRisk). */
        if (!is_dir && !chain.cancelled && chain.length() < expect_clusters &&
            ctx.table->raw(first_cluster) == 0) {
            ChainResult contig = reconstruct_contiguous_chain(*ctx.table, first_cluster,
                                                              expect_clusters, ctx.cancel);
            if (contig.length() > chain.length()) {
                chain = std::move(contig);
                inferred_chain = true;
            }
        }
    }
    if (chain.cancelled)
        return; /* unwinding mid-chain: no partial emission */

    /* M-052: inside a recycle-bin subtree, $I<token> streams carry the
     * ORIGINAL name/size/time of the deletion - parse and remember,
     * never surface (bin metadata, not recoverable content). The $I is
     * small (v1: 28+2*pathlen; v2: 544) - read straight off the chain. */
    if (ctx.in_recycle && !is_dir) {
        const std::u16string tok = recycle::token(name);
        if (!tok.empty() && name.size() >= 3 && name[1] == u'I') {
            std::vector<u8> ibuf;
            ibuf.resize(size_t(std::min<u64>(4096, e.file_size ? e.file_size : 544)));
            u64 filled = 0;
            for (u32 c : chain.clusters) {
                if (filled >= ibuf.size())
                    break;
                const u32 take = u32(std::min<u64>(ctx.p.cluster_bytes, ibuf.size() - filled));
                if (ctx.dev->read_at(FatTable::cluster_offset(ctx.p, c), ibuf.data() + filled,
                                     take) != OK)
                    break;
                filled += take;
            }
            recycle::RecycleInfo ri;
            if (filled >= 30 && recycle::parse_stream(ibuf.data(), u32(filled), ri)) {
                ctx.recycle_info[tok] = std::move(ri);
                ++ctx.stats->recycle_info;
            }
            return; /* bin metadata: not a result */
        }
    }

    tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
    if (props == nullptr)
        return;
    props->name = name;
    props->fs_kind = u32(ctx.p.flavor); /* 0=FAT12 1=FAT16 2=FAT32 (corpus codes) */
    props->logical_size = is_dir ? u64(chain.length()) * ctx.p.cluster_bytes : e.file_size;
    /* N-021 og parity: an invalid/zero FAT date (DosDateTimeToFileTime
     * failure class - month 0/13, day 0/32) yields the ZERO FileTime
     * (1601-01-01 UTC), not 1970-01-01. The og zeroes the props slot on
     * conversion failure (FATSearchDLL_decompiled.c:32135-32138:
     * "if (BVar5 == 0) *slot = 0"); fat_date_to_unix returns 0 exactly for
     * that class (a valid date is >= 1980-01-01 = unix 315532800), so the
     * 0 here maps 1:1 onto the og's failure branch. The exFAT sibling has
     * carried the same early-exit since S-011 (exfat_time_to_unix100). */
    const auto fat_to_filetime = [](u16 date, u16 time, u8 tenths) -> u64 {
        const u32 secs = fat_date_to_unix(date, time, tenths);
        return secs ? u64(secs) * 10000000ULL + 116444736000000000ULL : 0;
    };
    props->times.created = fat_to_filetime(e.creation_date, e.creation_time, e.creation_tenths);
    props->times.modified = fat_to_filetime(e.write_date, e.write_time, 0);
    props->times.changed = props->times.modified;
    props->times.accessed = fat_to_filetime(e.access_date, 0, 0);
    props->attributes = e.attributes;
    if (deleted)
        props->flags &= ~u16(0x10);
    else
        props->flags |= u16(0x10); /* OUR internal in-use convention (NTFS-5:
                                      the og's 0x10 is the DIRECTORY bit -
                                      do not read og semantics into this word) */

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
        /* GAP-S6-2: an inferred (contiguous-candidate) run is NEVER
         * Excellent - it is a reconstruction, not FAT-verified truth */
        if (inferred_chain && g == tree::Confidence::Excellent)
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

    /* M-052: a $R<token> inside the bin surfaces under its ORIGINAL
     * name/size/time when the $I was already seen (the pair is usually
     * adjacent; the post-walk fixup catches the higher-order case) and
     * is marked Deleted - in-use inside the bin, deleted from the
     * user's location (the NTFS engine's model). */
    if (ctx.in_recycle && !is_dir) {
        const std::u16string rtok = recycle::token(name);
        if (!rtok.empty() && name.size() >= 3 && name[1] == u'R') {
            auto it = ctx.recycle_info.find(rtok);
            if (it != ctx.recycle_info.end()) {
                props->name = it->second.orig_name;
                if (it->second.size != 0)
                    props->logical_size = it->second.size;
                props->times.modified = it->second.time;
                n->set_deleted_state(tree::DeletedState::Deleted);
                ++ctx.stats->recycle_remapped;
            } else {
                ctx.pending_recycle_r.push_back({n, rtok});
            }
        }
    }

    /* recurse into subdirectories (both live and deleted trees); a bin
     * ROOT marks its subtree as recycle-bin (M-052 - the corpus's
     * $RECYCLE.BIN / Recycled / Recycler roots, case-blind) */
    if (is_dir && first_cluster >= 2) {
        const bool was_in_recycle = ctx.in_recycle;
        if (!was_in_recycle && recycle::is_root_name(name))
            ctx.in_recycle = true;
        recurse_dir(ctx, first_cluster, n, deleted);
        ctx.in_recycle = was_in_recycle;
    }
}

} // namespace

u32 scan_fat(IStorageDevice* slice, const FatScanFlags& flags, CancelToken* cancel,
             WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             FatScanStats& stats, storage::FreeMap* free_out) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    u8 boot[512];
    if (slice->read_at(0, boot, sizeof(boot)) != OK)
        return ERR_READ_FAILED;
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    FatParams p;
    u8 gating_sector[512]; /* N-010b: gate the sector that won */
    std::memcpy(gating_sector, boot, sizeof(gating_sector));
    u32 rc = validate_boot(boot, slice->size(), p);
    if (rc != OK) {
        /* backup boot sector fallback (R5.1): FAT32 reserves a full copy of
         * the boot sector at sector 6 (spec default). FAT-8 (AUDIT-FAT): the
         * fallback is now RESTRICTED to a candidate that validates as FAT32
         * - FAT12/16 have no backup boot sector, so a sector-6 hit there was
         * FAT-area bytes that merely passed validation. Sector math uses the
         * device geometry: on 4Kn volumes the backup boot sits at 6 * 4096,
         * not the hardcoded 3072 (locked fix F1 'no fixed 512 anywhere'). */
        const u32 bs = slice->geometry().logical_sector ? slice->geometry().logical_sector : 512;
        u8 b[512];
        if (slice->read_at(u64(6) * bs, b, sizeof(b)) == OK) {
            FatParams bp;
            if (validate_boot(b, slice->size(), bp) == OK && bp.flavor == Flavor::Fat32) {
                p = bp;
                rc = OK;
                std::memcpy(gating_sector, b, sizeof(gating_sector));
            }
        }
        if (rc != OK)
            return rc;
    }
    /* N-010b (diff-to-fix): the og's engine-open gates (FATSearchDLL
     * FUN_18002f970 - FS-name string, numFATs {1,2}, bps == device
     * sector, FAT16 root floor + divisibility). The og's quick engine
     * declines (flavor 0xE) when these fail; the volume then routes to
     * other engines / raw carving on both sides. FAT12 is exempt (the
     * og validator has no FAT12 branch; the core keeps FAT12 recovery
     * as the documented I-002-family superset). */
    rc = boot_passes_open_gates(
        gating_sector,
        u16(slice->geometry().logical_sector ? slice->geometry().logical_sector : 512), p);
    if (rc != OK)
        return rc;

    FatTable table;
    table.set_mirror_bad_salvage(flags.mirror_bad_salvage); /* N-022 */
    rc = table.load(slice, p);
    if (rc != OK)
        return rc;
    /* M-012: mirror-only scans surface in the stats (og child1==NULL) */
    if (table.mirror_only())
        ++stats.mirror_only_scan;

    /* S-009 (free overlay): the FAT IS the allocation bitmap - entry 0
     * = free cluster (the IBuildVolumeBitmap truth the og's engines
     * produce). Cluster 0's byte = the data-area base minus the two
     * virtual reserved clusters, so the map is linear in the cluster
     * number exactly like the og's cluster->sector math.
     * Hardening (S-009 follow-up): clusters 0/1 are the FAT's VIRTUAL
     * reserved clusters - raw() answers 0 for them (out of table), and
     * their byte span [base, data_base) is the root-dir/FAT region, NOT
     * free space. They are never marked free (the first fill loop
     * started at c=0 and painted the last two pre-data sectors free on
     * every FAT volume - a false claim the engine tests now pin). */
    if (free_out != nullptr) {
        free_out->clear();
        const u64 data_base = u64(p.first_data_sector) * p.bytes_per_sector;
        const u64 two_cb = u64(2) * p.cluster_bytes;
        free_out->base_byte = data_base >= two_cb ? data_base - two_cb : 0;
        free_out->cluster_bytes = p.cluster_bytes;
        free_out->cluster_count = table.max_cluster() + 1;
        free_out->bits.assign(size_t((free_out->cluster_count + 7) / 8), 0);
        for (u64 c = 2; c < free_out->cluster_count; ++c) {
            if (table.raw(u32(c)) == 0)
                free_out->bits[size_t(c >> 3)] |= u8(1u << (c & 7));
        }
    }

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

    /* M-052: post-walk recycle fixup - $R files emitted before their $I
     * was parsed (the $I can sit at a later dentry or a sibling
     * subfolder of the bin). Nodes are arena-stable; mutating through
     * the node is safe after the walk. */
    for (const auto& pr : ctx.pending_recycle_r) {
        auto it = ctx.recycle_info.find(pr.token);
        if (it == ctx.recycle_info.end())
            continue;
        tree::FileProperties* rp = pr.node->props();
        if (rp == nullptr)
            continue;
        rp->name = it->second.orig_name;
        if (it->second.size != 0)
            rp->logical_size = it->second.size;
        rp->times.modified = it->second.time;
        pr.node->set_deleted_state(tree::DeletedState::Deleted);
        ++stats.recycle_remapped;
    }

    if (progress)
        (void)progress(1, 1, progress_user);
    return OK;
}

} // namespace fat
} // namespace cr
