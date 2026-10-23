/* cr_exfat_quick.cpp - exFAT scan engine (R5.2).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/exfat/cr_exfat_engine.h"
#include <cstring>
#include <set>

namespace cr {
namespace exfat {

namespace {

constexpr u32 EXFAT_EOC = 0xFFFFFFFF;
constexpr u32 EXFAT_EOC_MIN = 0xFFFFFFF8; /* spec EOC family (EXFAT-3) */
constexpr u32 EXFAT_BAD = 0xFFFFFFF7;
constexpr u32 EXFAT_FREE = 0;

struct ExfatFat {
    std::vector<u8> data;
    u32 byte_len = 0;
    u32 entry(u32 c) const {
        if (u64(c) * 4 + 4 > byte_len)
            return EXFAT_FREE;
        return u32_from_bytes_le(data.data() + c * 4);
    }
    u32 next(u32 c) const {
        u32 v = entry(c);
        if (v == EXFAT_FREE || v == EXFAT_BAD)
            return 0;
        /* EXFAT-3 (AUDIT-FAT): the spec EOC family is 0xFFFFFFF8..0xFFFFFFFF
         * (and og treats the range as chain end); only exact 0xFFFFFFFF
         * previously terminated, so 0xFFFFFFF8..E were followed as cluster
         * numbers -> bounds failure -> broken chains on spec-legal volumes. */
        if (v >= EXFAT_EOC_MIN)
            return EXFAT_EOC;
        return v;
    }
};

struct ChainResult {
    std::vector<u32> clusters;
    bool cycle = false;
    bool broken = false;
    bool assumed_contiguous = false; /* EXFAT-2: FREE-entry continuation */
};

ChainResult follow_exfat_chain(const ExfatFat& fat, u32 start, u32 max_clusters,
                               u32 cluster_count = 0) {
    ChainResult r;
    std::vector<u64> visited;
    u32 c = start;
    u32 guard = 0;
    while (c >= 2) {
        if (++guard > max_clusters) {
            r.cycle = true;
            break;
        }
        for (u64 v : visited)
            if (v == c) {
                r.cycle = true;
                return r;
            }
        visited.push_back(c);
        r.clusters.push_back(c);
        if (r.clusters.size() >= max_clusters)
            break;
        u32 n = fat.next(c);
        if (n == 0) {
            /* EXFAT-2 (AUDIT-FAT): og FUN_180021c60:28158-28163 - a FREE
             * (zero) FAT entry mid-chain means "no chain info here"; the og
             * continues with cluster+1 under an "assumed" flag (the exFAT
             * salvage heuristic for FAT-zeroed volumes - deletion zeroes
             * the chain). The old hard-break truncated extents and
             * downgraded confidence on damaged-but-recoverable volumes.
             * Bounded by the data-length expectation (max_clusters) and the
             * heap size. */
            if (cluster_count != 0 && c + 1 >= cluster_count + 2) {
                r.broken = true;
                break;
            }
            r.assumed_contiguous = true;
            n = c + 1;
        }
        if (n >= EXFAT_EOC_MIN)
            break;
        c = n;
    }
    return r;
}

/* contiguous chain for NoFatChain streams (corpus: no-FAT behavior) */
ChainResult contiguous_chain(u32 first, u32 count) {
    ChainResult r;
    if (count == 0)
        return r;
    r.clusters.reserve(count);
    for (u32 i = 0; i < count; ++i)
        r.clusters.push_back(first + i);
    return r;
}

u64 cluster_offset(const ExfatParams& p, u32 c) {
    return (u64(c) - 2) * p.cluster_bytes + u64(p.cluster_heap_offset) * p.bytes_per_sector;
}

struct Bitmap {
    std::vector<u8> data;
    u32 cluster_count = 0;
    bool test(u32 c) const {
        if (c / 8 >= data.size())
            return false;
        return (data[c / 8] & (1u << (c % 8))) != 0;
    }
};

/* exFAT time: packed u32 (date<<16 | time, FAT field layout) + tz byte
 * (quarter-hours | 0x80, 0x00 = unused). -> 100ns since 1601. */
u64 exfat_time_to_unix100(u32 ts, u8 tz) {
    const u16 date = u16(ts >> 16);
    const u16 time = u16(ts & 0xFFFF);
    if (date == 0)
        return 0;
    const u32 year = 1980u + ((date >> 9) & 0x7F);
    const u32 month = (date >> 5) & 0x0F;
    const u32 day = date & 0x1F;
    const u32 hour = (time >> 11) & 0x1F;
    const u32 minute = (time >> 5) & 0x3F;
    const u32 second = (time & 0x1F) * 2;
    if (month < 1 || month > 12 || day < 1 || day > 31)
        return 0;
    const i64 y = i64(year) - (month <= 2 ? 1 : 0);
    const i64 era = (y >= 0 ? y : y - 399) / 400;
    const u64 yoe = u64(y - era * 400);
    const u64 doy = (153u * ((month > 2 ? month - 3 : month + 9)) + 2) / 5 + day - 1;
    const u64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const i64 days = era * 146097 + i64(doe) - 719468;
    if (days < 0)
        return 0;
    /* timezone: 7-bit two's complement quarters, bit7 = used; 0x00 = unused */
    i64 tz_secs = 0;
    if (tz != 0x00) {
        i32 q = i32(tz & 0x7F);
        if (q >= 64)
            q -= 128;
        tz_secs = i64(q) * 900;
    }
    const u64 secs = u64(days) * 86400 + u64(hour) * 3600 + u64(minute) * 60 + second;
    return secs * 10000000ULL + 116444736000000000ULL - u64(tz_secs) * 10000000ULL;
}

struct ScanCtx {
    IStorageDevice* dev;
    CancelToken* cancel = nullptr;               /* P2-8 */
    bool (*progress)(u64, u64, void*) = nullptr; /* live progress (G-04) */
    void* progress_user = nullptr;
    u64 dentries_done = 0; /* cumulative dentries examined (progress unit) */
    ExfatParams p;
    ExfatFat fat;
    Bitmap bitmap;
    bool have_fat = false;
    bool have_bitmap = false;
    ExfatScanFlags flags;
    tree::ResultTree* tree;
    tree::FullNode* root_active;
    tree::FullNode* root_deleted;
    tree::FullNode* root_lost;
    ExfatScanStats* stats;
    std::set<u64> visited_dirs;
};

/* read a directory chain's content */
std::vector<u8> read_dir_chain(ScanCtx& ctx, u32 first, u32 max_bytes, bool* ok) {
    *ok = false;
    ChainResult chain;
    if (ctx.have_fat)
        chain = follow_exfat_chain(ctx.fat, first, 0x10000, ctx.p.cluster_count);
    if (chain.clusters.empty() && first >= 2) {
        /* no FAT or FAT lost: try contiguous single cluster */
        chain = contiguous_chain(first, 1);
        chain.broken = true;
    }
    u64 total = u64(chain.clusters.size()) * ctx.p.cluster_bytes;
    if (total > max_bytes)
        total = max_bytes;
    if (total == 0)
        return {};
    std::vector<u8> buf(size_t(total), 0);
    u64 filled = 0;
    size_t i = 0;
    while (i < chain.clusters.size() && filled < total) {
        size_t j = i;
        while (j + 1 < chain.clusters.size() && chain.clusters[j + 1] == chain.clusters[j] + 1)
            ++j;
        u64 off = cluster_offset(ctx.p, chain.clusters[i]);
        u32 run = u32((j - i + 1) * ctx.p.cluster_bytes);
        if (filled + run > total)
            run = u32(total - filled);
        if (ctx.dev->read_at(off, buf.data() + filled, run) != OK)
            std::memset(buf.data() + filled, 0, run);
        filled += run;
        i = j + 1;
    }
    *ok = filled > 0;
    return buf;
}

void walk_entries(ScanCtx& ctx, const std::vector<u8>& buf, bool deleted_context,
                  tree::FileNode* parent_host, u32 depth);

constexpr u32 EXFAT_MAX_DEPTH = 64; /* P0-6: stack-exhaustion guard */

void recurse_dir(ScanCtx& ctx, u32 first, bool deleted, tree::FileNode* host_node, u32 depth = 0) {
    if (depth > EXFAT_MAX_DEPTH) {
        ++ctx.stats->chains_broken;
        return;
    }
    u64 key = first;
    if (ctx.visited_dirs.count(key)) {
        ++ctx.stats->chains_broken;
        return;
    }
    ctx.visited_dirs.insert(key);
    bool ok = false;
    std::vector<u8> buf = read_dir_chain(ctx, first, 4u << 20, &ok);
    if (ok)
        walk_entries(ctx, buf, deleted, host_node, depth);
}

void walk_entries(ScanCtx& ctx, const std::vector<u8>& buf, bool deleted_context,
                  tree::FileNode* parent_host, u32 depth) {
    if (buf.empty() || (ctx.cancel && ctx.cancel->checkpoint())) /* P2-8 */
        return;
    (void)deleted_context; /* host selection now driven by parent_host */
    const size_t count = buf.size() / 32;
    size_t i = 0;
    while (i < count) {
        /* live progress (G-04): one dentry examined per iteration - honest
         * cumulative counts; the orchestrator clamps permille monotonic. */
        ++ctx.dentries_done;
        if (ctx.progress && (ctx.dentries_done & 63) == 0)
            (void)ctx.progress(ctx.dentries_done, ctx.dentries_done + (count - i),
                               ctx.progress_user);
        const u8* f = buf.data() + i * 32;
        const u8 raw_type = f[0];
        if (raw_type == TYPE_NONE)
            break; /* end of directory */
        const u8 t = entry_type(raw_type);
        const bool in_use = entry_in_use(raw_type);

        if (t == TYPE_BITMAP && in_use) {
            /* allocation bitmap dentry (live only; deleted bitmap has 0x80
             * cleared - useless for allocation tests) */
            if (!ctx.have_bitmap && buf.size() >= (i + 1) * 32 + sizeof(BitmapEntry)) {
                BitmapEntry be;
                std::memcpy(&be, f, sizeof(be));
                if (be.first_cluster >= 2) {
                    ChainResult bc = ctx.have_fat ? follow_exfat_chain(ctx.fat, be.first_cluster,
                                                                       0x10000, ctx.p.cluster_count)
                                                  : contiguous_chain(be.first_cluster, 1);
                    if (bc.clusters.empty())
                        bc = contiguous_chain(be.first_cluster, 1);
                    u64 total = u64(bc.clusters.size()) * ctx.p.cluster_bytes;
                    if (total > (8u << 20))
                        total = 8u << 20;
                    ctx.bitmap.data.resize(size_t(total), 0);
                    u64 filled = 0;
                    size_t k = 0;
                    while (k < bc.clusters.size() && filled < total) {
                        u64 off = cluster_offset(ctx.p, bc.clusters[k]);
                        u32 run = ctx.p.cluster_bytes;
                        if (filled + run > total)
                            run = u32(total - filled);
                        ctx.dev->read_at(off, ctx.bitmap.data.data() + filled, run);
                        filled += run;
                        ++k;
                    }
                    ctx.bitmap.cluster_count = ctx.p.cluster_count;
                    ctx.have_bitmap = filled > 0;
                }
            }
            ++i;
            continue;
        }
        if (t == TYPE_UPCASE || t == TYPE_VOLUME_LABEL) {
            ++i;
            continue; /* not results */
        }
        if (t == TYPE_FILE) {
            /* entry set: file + stream + names (attribute_count streams) */
            const FileEntry* fe = reinterpret_cast<const FileEntry*>(f);
            const u8 stream_count = fe->secondary_count;
            if (i + 1 + stream_count > count)
                break; /* torn set */
            const u8* stream = buf.data() + (i + 1) * 32;
            if (entry_type(stream[0]) != TYPE_STREAM) {
                ++i;
                continue;
            }
            const StreamEntry* se = reinterpret_cast<const StreamEntry*>(stream);
            /* og stream-dentry gates (FUN_180003090, audit row 11):
             * reserved fields must be zero on a live stream entry -
             * damaged-set counting parity. A non-zero reserved field
             * downgrades the set (treated as damaged, not skipped). */
            const bool stream_reserved_bad =
                in_use && (se->reserved1 != 0 || se->reserved2 != 0 || se->reserved3 != 0);
            if (stream_reserved_bad)
                ++ctx.stats->sets_damaged;
            const u8 name_len = se->name_length;
            u32 name_entries = (name_len + 14) / 15;
            if (u32(name_entries) > stream_count)
                name_entries = stream_count;

            /* entry-set checksum (live sets only - deleted sets have
             * checksum over live values anyway) */
            bool set_ok = true;
            if (ctx.flags.validate_entry_sets && in_use) {
                u16 chk = entry_set_checksum(buf.data() + i * 32, 1 + 1 + name_entries);
                if (chk != fe->set_checksum) {
                    set_ok = false;
                    ++ctx.stats->sets_damaged;
                }
            }

            /* assemble name from fragments */
            std::u16string name;
            name.reserve(name_len);
            for (u32 ne = 0; ne < name_entries && name.size() < name_len; ++ne) {
                const u8* nf = buf.data() + (i + 2 + ne) * 32;
                if (entry_type(nf[0]) != TYPE_NAME)
                    break;
                const NameEntry* nm = reinterpret_cast<const NameEntry*>(nf);
                for (int c = 0; c < 15 && name.size() < name_len; ++c)
                    name.push_back(nm->name[c]);
            }
            if (name.empty()) {
                i += 1 + 1 + name_entries;
                continue;
            }

            const bool deleted = !in_use;
            const bool is_dir = (fe->file_attribute & 0x10) != 0;
            const u64 data_len = se->data_length;
            const u32 first_cluster = se->first_cluster;
            const bool no_fat = (se->flags & 0x02) != 0;

            /* EXFAT-1 (AUDIT-FAT): og rejects first_cluster == 1 and
             * first_cluster >= 0xFFFFFFF7 (FUN_180003090:1857-1859, inlined
             * 24404-24412, backward-walk form 2109) - 1 is reserved and
             * 0xFFFFFFF7+ are BAD/EOC-family sentinels, never positions.
             * (The v2 artifact's "must be 1 or >= 0xFFFFFF7" polarity was
             * inverted; the decompile is unambiguous across three sites.)
             * Damaged set: counted, entry kept as metadata-only. */
            const bool fc_invalid = first_cluster == 1 || first_cluster >= 0xFFFFFFF7u;
            if (fc_invalid)
                ++ctx.stats->sets_damaged;

            /* chain resolution */
            ChainResult chain;
            if (!fc_invalid && (is_dir || first_cluster >= 2)) {
                u32 expect = u32((data_len + ctx.p.cluster_bytes - 1) / ctx.p.cluster_bytes);
                if (no_fat && !is_dir && expect > 0) {
                    chain = contiguous_chain(first_cluster, expect);
                    ++ctx.stats->no_fat_chains;
                } else if (no_fat && is_dir) {
                    /* EXFAT-6 (AUDIT-FAT): NoFatChain directories read
                     * ceil(data_length / cluster_bytes) clusters, not 1
                     * (spec-legal multi-cluster dirs truncated before). */
                    u32 dir_clusters =
                        u32((data_len + ctx.p.cluster_bytes - 1) / ctx.p.cluster_bytes);
                    chain = contiguous_chain(first_cluster, dir_clusters ? dir_clusters : 1);
                    ++ctx.stats->no_fat_chains;
                } else if (ctx.have_fat) {
                    chain = follow_exfat_chain(ctx.fat, first_cluster,
                                               is_dir ? 0x10000 : (expect ? expect : 1),
                                               ctx.p.cluster_count);
                    if (chain.assumed_contiguous)
                        ++ctx.stats->assumed_contiguous;
                } else {
                    chain = contiguous_chain(first_cluster, 1);
                }
            }

            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (!props) {
                i += 1 + 1 + name_entries;
                continue;
            }
            props->name = name;
            props->fs_kind = 7; /* exFAT (corpus FS codes) */
            props->logical_size =
                is_dir ? u64(chain.clusters.size()) * ctx.p.cluster_bytes : data_len;
            props->times.created = exfat_time_to_unix100(fe->create_timestamp, fe->create_tz);
            props->times.modified = exfat_time_to_unix100(fe->modify_timestamp, fe->modify_tz);
            props->times.changed = props->times.modified;
            props->times.accessed = exfat_time_to_unix100(fe->access_timestamp, fe->access_tz);
            props->attributes = fe->file_attribute;
            props->flags = in_use ? u16(0x10) : u16(0);

            /* extents (contiguous runs, slice-relative) */
            size_t k = 0;
            while (k < chain.clusters.size()) {
                size_t j = k;
                while (j + 1 < chain.clusters.size() &&
                       chain.clusters[j + 1] == chain.clusters[j] + 1)
                    ++j;
                u64 off = cluster_offset(ctx.p, chain.clusters[k]);
                u64 len = u64(j - k + 1) * ctx.p.cluster_bytes;
                if (is_dir && props->extents.size() >= 64)
                    break;
                props->extents.push_back({off, len});
                k = j + 1;
            }

            /* grading */
            tree::Confidence conf;
            if (!deleted) {
                conf = set_ok ? tree::Confidence::Excellent : tree::Confidence::PartialRisk;
            } else if (ctx.have_bitmap && !chain.clusters.empty()) {
                u64 total = 0, reused = 0;
                for (u32 c : chain.clusters) {
                    ++total;
                    if (ctx.bitmap.test(c))
                        ++reused;
                }
                const double ratio = double(reused) / double(total);
                if (ratio >= 1.0)
                    conf = tree::Confidence::Overwritten;
                else if (ratio > 0.0)
                    conf = tree::Confidence::PartialRisk;
                else
                    conf = tree::Confidence::Excellent;
            } else if (chain.clusters.empty()) {
                conf = tree::Confidence::Overwritten;
            } else {
                conf = tree::Confidence::PartialRisk;
            }
            /* An assumed-contiguous chain (EXFAT-2) is never graded above
             * PartialRisk: the FAT did not confirm the layout. */
            if (chain.assumed_contiguous && conf == tree::Confidence::Excellent)
                conf = tree::Confidence::PartialRisk;

            /* children attach under their directory node; top-level under
             * the roots. deleted sets land in the deleted/lost roots. */
            tree::FileNode* host =
                parent_host ? parent_host
                            : (deleted ? (ctx.root_lost ? ctx.root_lost : ctx.root_deleted)
                                       : static_cast<tree::FileNode*>(ctx.root_active));
            if (deleted && !parent_host && ctx.root_deleted)
                host = ctx.root_deleted;
            tree::FileNode* n = ctx.tree->add_file(host, props);
            props->release();
            if (n) {
                n->set_origin(tree::Origin::Structured);
                if (deleted) {
                    ++ctx.stats->deleted_entries;
                    n->set_deleted_state(set_ok ? tree::DeletedState::Deleted
                                                : tree::DeletedState::Damaged);
                }
                n->set_confidence(conf);
            }
            if (is_dir)
                ++ctx.stats->dirs;
            else
                ++ctx.stats->files;

            /* recursion: children of this directory attach under its node */
            if (is_dir && first_cluster >= 2)
                recurse_dir(ctx, first_cluster, deleted, n, depth + 1);

            i += 1 + 1 + u32(name_entries);
            continue;
        }
        ++i; /* unknown dentry type: skip */
    }
    if (ctx.progress && count)
        (void)ctx.progress(ctx.dentries_done, ctx.dentries_done, ctx.progress_user);
}

} // namespace

u32 scan_exfat(IStorageDevice* slice, const ExfatScanFlags& flags, CancelToken* cancel,
               bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
               tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
               ExfatScanStats& stats) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    ExfatParams p;
    u32 rc = validate_boot_region(slice, p);
    if (rc != OK)
        return rc;
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    ScanCtx ctx;
    ctx.cancel = cancel; /* P2-8: cancellable walk */
    ctx.progress = progress;
    ctx.progress_user = progress_user;
    ctx.dev = slice;
    ctx.p = p;
    ctx.flags = flags;
    ctx.tree = &tree;
    ctx.root_active = root_active;
    ctx.root_deleted = root_deleted;
    ctx.root_lost = root_lost;
    ctx.stats = &stats;

    /* FAT load (if the volume has one) */
    if (p.fat_length != 0) {
        u64 fat_bytes = u64(p.fat_length) * p.bytes_per_sector;
        if (fat_bytes <= (32u << 20) &&
            u64(p.fat_offset) * p.bytes_per_sector + fat_bytes <= slice->size()) {
            ctx.fat.data.resize(size_t(fat_bytes));
            if (slice->read_at(u64(p.fat_offset) * p.bytes_per_sector, ctx.fat.data.data(),
                               u32(fat_bytes)) == OK) {
                ctx.fat.byte_len = u32(fat_bytes);
                ctx.have_fat = true;
            }
        }
    }

    /* root directory (chain from FAT; single-cluster fallback) */
    if (p.first_cluster_of_root >= 2)
        recurse_dir(ctx, p.first_cluster_of_root, false, nullptr, 0);
    if (ctx.cancel && ctx.cancel->checkpoint())
        return ERR_CANCELLED;

    if (progress)
        (void)progress(1, 1, progress_user);
    return OK;
}

} // namespace exfat
} // namespace cr
