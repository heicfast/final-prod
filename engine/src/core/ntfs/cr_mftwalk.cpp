/* cr_mftwalk.cpp - the MFT record walker (R2.3, R2.4).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_mftwalk.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include <cstring>
#include <map>
#include <vector>

namespace cr {
namespace ntfs {

namespace {

struct EmittedNode {
    tree::FileNode *node = nullptr;
    u64 parent_record = 0;
    bool parent_resolved = false;
};

int namespace_rank(u8 ns) {
    switch (ns) {
    case FILE_NAME_POSIX: return 4;
    case FILE_NAME_WIN32_AND_DOS: return 3;
    case FILE_NAME_WIN32: return 2;
    case FILE_NAME_DOS: return 1;
    default: return 0;
    }
}

/* Build props for one record from its attribute views. */
u32 build_props(NtfsVolume *vol, const std::vector<AttrView> &attrs, u64 record_no,
                u16 record_flags,
                tree::FileProperties *&props_out, std::vector<AttrListEntry> &attrlist_out,
                std::vector<FileNameInfo> &names_out, RunList &unnamed_data_out,
                bool &has_unnamed_data, RunList &i30_out, bool &has_i30,
                tree::FileTimes &si_times, bool &has_si) {
    props_out = nullptr;
    attrlist_out.clear();
    names_out.clear();
    unnamed_data_out = RunList();
    has_unnamed_data = false;
    i30_out = RunList();
    has_i30 = false;
    has_si = false;

    for (const AttrView &a : attrs) {
        switch (a.type) {
        case ATTR_TYPE_ATTRIBUTE_LIST: {
            /* entries recorded for the engine's second read (F6 chase) */
            (void)parse_attr_list_entries(a, attrlist_out);
            break;
        }
        case ATTR_TYPE_FILE_NAME: {
            FileNameInfo fi;
            if (parse_file_name(a, fi) == OK) names_out.push_back(fi);
            break;
        }
        case ATTR_TYPE_STANDARD_INFORMATION: {
            StandardInfo si;
            if (parse_standard_information(a, si) == OK) {
                si_times = si.times;
                has_si = true;
            }
            break;
        }
        case ATTR_TYPE_DATA: {
            if (a.name_length == 0) {
                if (!a.nonresident) {
                    const u8 *d = nullptr;
                    u32 dlen = 0;
                    if (resident_span(a, d, dlen) == OK) has_unnamed_data = true; /* resident */
                } else {
                    RunList rl;
                    if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                        has_unnamed_data = true;
                        /* merge multi-fragment $DATA (same attr across records) */
                        for (const DataRun &r : rl.runs()) unnamed_data_out.runs().push_back(r);
                    }
                }
            }
            break;
        }
        case ATTR_TYPE_INDEX_ALLOCATION: {
            if (a.name_length == 0) { /* $I30 by default name index 0x00? name is "$I30" - accept any */
                RunList rl;
                if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                    has_i30 = true;
                    for (const DataRun &r : rl.runs()) i30_out.runs().push_back(r);
                }
            }
            break;
        }
        default:
            break;
        }
    }
    (void)record_flags;
    (void)record_no;
    return OK;
}

} // namespace

u32 walk_mft(NtfsVolume *vol, VolumeBitmap *bitmap, const WalkerFlags &flags,
             CancelToken *cancel, WalkProgress progress, void *progress_user,
             tree::ResultTree &tree, tree::FullNode *root_active,
             tree::FullNode *root_deleted, tree::FullNode *root_lost, WalkStats &stats) {
    if (vol == nullptr || root_active == nullptr || root_deleted == nullptr ||
        root_lost == nullptr)
        return ERR_BAD_PARAMETER;

    const u32 sector_bytes = vol->params().sector_bytes;
    const u64 max_record = vol->max_record_no();
    std::vector<EmittedNode> emitted;
    emitted.reserve(4096);

    /* temp record buffer with fixup scratch */
    std::vector<u8> rec;
    for (u64 record_no = 0; record_no < max_record; ++record_no) {
        if (cancel && cancel->cancelled()) return ERR_CANCELLED;
        if ((record_no & 0x3F) == 0 && progress &&
            !progress(record_no, max_record, progress_user))
            return ERR_CANCELLED;
        ++stats.examined;

        u32 rc = vol->read_record(record_no, rec);
        if (rc == ERR_RANGE_OUT_OF_BOUNDS) continue;
        if (rc != OK) {
            ++stats.damaged;
            continue;
        }
        RecordHeader rh;
        std::memcpy(&rh, rec.data(), sizeof(rh));
        if (rh.magic != RECORD_MAGIC_FILE) continue; /* unused slot - not damage */
        /* F4: fixup mandatory before any parse */
        if (fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                        rh.update_sequence_size, sector_bytes) != OK) {
            ++stats.damaged;
            continue;
        }

        std::vector<AttrView> attrs;
        if (enum_attributes(rec.data(), u32(rec.size()), attrs) != OK) {
            ++stats.damaged;
            continue;
        }

        std::vector<AttrListEntry> attrlist;
        std::vector<FileNameInfo> names;
        RunList data_runs, i30_runs;
        bool has_data = false, has_i30 = false, has_si = false;
        tree::FileTimes si_times{};
        tree::FileProperties *scratch = nullptr;
        if (build_props(vol, attrs, record_no, rh.flags, scratch, attrlist,
                        names, data_runs, has_data, i30_runs, has_i30, si_times, has_si) != OK) {
            ++stats.damaged;
            continue;
        }

        /* $ATTRIBUTE_LIST chase (F6): read child records, merge their
         * fragments into the views (single level - deeper lists are corrupt). */
        if (!attrlist.empty()) {
            std::vector<AttrView> merged = attrs;
            std::vector<u64> visited;
            for (const AttrListEntry &e : attrlist) {
                if (e.type != ATTR_TYPE_DATA && e.type != ATTR_TYPE_INDEX_ALLOCATION) continue;
                u64 child = mft_reference_record(e.record_ref);
                if (child == record_no || child >= max_record) continue;
                bool cyc = false;
                for (u64 v : visited)
                    if (v == child) { cyc = true; break; }
                if (cyc) { ++stats.damaged; continue; }
                visited.push_back(child);
                std::vector<u8> crec;
                if (vol->read_record(child, crec) != OK) continue;
                RecordHeader crh;
                std::memcpy(&crh, crec.data(), sizeof(crh));
                if (crh.magic != RECORD_MAGIC_FILE) continue;
                if (fixup_apply(crec.data(), u32(crec.size()), crh.update_sequence_offset,
                                crh.update_sequence_size, sector_bytes) != OK)
                    continue;
                std::vector<AttrView> cattrs;
                if (enum_attributes(crec.data(), u32(crec.size()), cattrs) != OK) continue;
                for (const AttrView &ca : cattrs) {
                    if (ca.type != ATTR_TYPE_DATA && ca.type != ATTR_TYPE_INDEX_ALLOCATION)
                        continue;
                    merged.push_back(ca);
                    /* fold runlists directly */
                    if (ca.type == ATTR_TYPE_DATA && ca.nonresident && ca.name_length == 0) {
                        RunList rl;
                        if (parse_data_runs(ca, vol->params().total_clusters, rl) == OK) {
                            for (const DataRun &r : rl.runs()) data_runs.runs().push_back(r);
                            has_data = true;
                        }
                    }
                    if (ca.type == ATTR_TYPE_INDEX_ALLOCATION && ca.nonresident) {
                        RunList rl;
                        if (parse_data_runs(ca, vol->params().total_clusters, rl) == OK) {
                            for (const DataRun &r : rl.runs()) i30_runs.runs().push_back(r);
                            has_i30 = true;
                        }
                    }
                }
            }
            attrs = std::move(merged);
        }

        if (names.empty()) { /* no usable $FILE_NAME - the corpus 'name == NULL' skip */
            ++stats.damaged;
            continue;
        }

        /* pick the best-namespace name; collect distinct parents (hard links) */
        const FileNameInfo *best = &names[0];
        for (const FileNameInfo &n : names)
            if (namespace_rank(n.namespace_type) > namespace_rank(best->namespace_type))
                best = &n;

        /* corpus filters: '$'-prefixed metafiles; SYSTEM files (flags & 4) */
        if (!best->name.empty() && best->name[0] == u'$') {
            ++stats.meta_skipped;
            continue;
        }
        if (flags.skip_system_files && (best->dos_flags & 0x04)) {
            ++stats.system_skipped;
            continue;
        }

        const bool in_use = (rh.flags & RECORD_IN_USE) != 0;
        const bool is_dir = (rh.flags & RECORD_IS_DIRECTORY) != 0;

        /* viability grading (fix F5 + corpus gate) */
        tree::Confidence conf = tree::Confidence::Unknown;
        if (!in_use) {
            const RunList &probe = is_dir ? (has_i30 ? i30_runs : RunList()) : data_runs;
            const u64 probe_size = is_dir ? (has_i30 ? i30_runs.total_clusters() *
                                                          vol->params().cluster_bytes
                                                    : 0)
                                          : (has_data ? best->real_size : 0);
            if (!is_dir && !has_data) {
                conf = tree::Confidence::MetadataOnly;
            } else if (probe_size >= flags.viability_min_size && probe.total_clusters() > 0) {
                if (bitmap && bitmap->valid()) {
                    const double ratio = bitmap->reused_ratio(probe);
                    if (ratio >= 1.0) conf = tree::Confidence::Overwritten;
                    else if (ratio > 0.0) conf = tree::Confidence::PartialRisk;
                    else conf = tree::Confidence::Excellent;
                } else {
                    conf = tree::Confidence::PartialRisk; /* no bitmap: honest unknown-ish */
                }
            } else if (probe_size < flags.viability_min_size) {
                conf = tree::Confidence::Excellent; /* corpus fast-path: <1 KiB, likely resident */
                if (!is_dir && !has_data) conf = tree::Confidence::MetadataOnly;
            } else {
                conf = tree::Confidence::PartialRisk;
            }
        } else {
            conf = tree::Confidence::Excellent; /* live file: extents owned by it */
        }

        /* distinct parents -> hard links (shared props object) */
        std::vector<u64> parents;
        for (const FileNameInfo &n : names) {
            u64 p = n.parent_record;
            bool dup = false;
            for (u64 q : parents)
                if (q == p) { dup = true; break; }
            if (!dup) parents.push_back(p);
        }
        if (parents.size() > 1) stats.hardlinks += parents.size() - 1;

        tree::FileProperties *props = new (std::nothrow) tree::FileProperties();
        if (props == nullptr) return ERR_NO_MEMORY;
        props->record_ref = (record_no) | (u64(rh.sequence) << 48);
        props->fs_kind = 3; /* NTFS (corpus FS type codes) */
        props->name = best->name;
        props->logical_size = best->real_size;
        props->allocated_size = best->allocated_size;
        props->times = has_si ? si_times : best->times;
        props->attributes = u16(best->dos_flags & 0xFF) | (is_dir ? 0x10 : 0);
        props->flags = u16(in_use ? PFLAG_IN_USE : 0) | u16(is_dir ? PFLAG_IS_DIR : 0);

        /* extents in slice-relative bytes; the tree consumer resolves to
         * absolute via the slice base (orchestrator does this at merge). */
        if (has_data && !data_runs.empty()) {
            std::vector<RunList::Extent> ex;
            data_runs.extents(ex);
            for (const auto &e : ex) {
                props->extents.push_back(
                    {e.lcn * vol->params().cluster_bytes, e.clusters * vol->params().cluster_bytes});
            }
        }

        /* emit one node per parent, each with ITS OWN link name (per-link
         * $FILE_NAME); hard links duplicate props (rare) - dedup via
         * record_ref downstream (writer + merge). */
        tree::FullNode *default_root = in_use ? root_active : root_deleted;
        for (u64 p : parents) {
            /* this link's name: highest-namespace FILE_NAME with this parent */
            const FileNameInfo *link_name = &names[0];
            for (const FileNameInfo &n : names)
                if (n.parent_record == p &&
                    namespace_rank(n.namespace_type) >= namespace_rank(link_name->namespace_type))
                    link_name = &n;
            if (link_name->parent_record != p && !names.empty() && parents.size() == 1)
                link_name = best; /* single link: keep best */
            tree::FileProperties *link_props = props;
            if (parents.size() > 1 || link_name != best) {
                link_props = props->clone();
                if (link_props == nullptr) {
                    props->release();
                    return ERR_NO_MEMORY;
                }
                link_props->name = link_name->name;
                link_props->logical_size = link_name->real_size;
                link_props->allocated_size = link_name->allocated_size;
                link_props->times = link_name->times;
            }
            tree::FileNode *n = tree.add_file(default_root, link_props);
            if (n == nullptr) {
                link_props->release();
                props->release();
                return ERR_NO_MEMORY;
            }
            if (link_props != props) link_props->release(); /* node holds it */
            if (!in_use) {
                n->set_deleted_state(tree::DeletedState::Deleted);
                if (conf == tree::Confidence::Overwritten)
                    n->set_deleted_state(tree::DeletedState::PartiallyOverwritten);
            }
            n->set_confidence(conf);
            n->set_origin(tree::Origin::Structured);
            emitted.push_back({n, p, false});
            ++stats.emitted;
        }
        /* each add_file call holds one ref; drop the builder's ref for clones */
        props->release(); /* nodes hold the refs */
        if (in_use) ++stats.active;
        else ++stats.deleted;

        /* ADS: named $DATA -> child nodes (R2.3) */
        if (flags.emit_ads && !is_dir) {
            for (const AttrView &a : attrs) {
                if (a.type != ATTR_TYPE_DATA || a.name_length == 0) continue;
                std::u16string sname = attr_name(a);
                if (sname.empty() || sname[0] == u'$') continue;
                tree::FileProperties *sp = new (std::nothrow) tree::FileProperties();
                if (sp == nullptr) continue;
                sp->name = sname;
                sp->fs_kind = 3;
                if (!a.nonresident) {
                    const u8 *d = nullptr;
                    u32 dlen = 0;
                    if (resident_span(a, d, dlen) == OK && dlen > 0) {
                        /* resident stream payload: record an extent pointing
                         * into the record buffer is invalid post-scan; store
                         * size only - content recovered via record re-read */
                        sp->logical_size = dlen;
                    }
                } else {
                    RunList rl;
                    if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                        sp->logical_size = a.actual;
                        std::vector<RunList::Extent> ex;
                        rl.extents(ex);
                        for (const auto &e : ex)
                            sp->extents.push_back({e.lcn * vol->params().cluster_bytes,
                                                   e.clusters * vol->params().cluster_bytes});
                    }
                }
                sp->times = props->times;
                sp->attributes = props->attributes;
                sp->flags = props->flags;
                /* ADS nodes attach directly under the LAST created node of
                 * this record (the primary hard link) */
                if (!emitted.empty()) {
                    tree::FileNode *host = emitted.back().node;
                    sp->name = props->name + u":" + sname;
                    tree::FileNode *adn = tree.add_file(host, sp);
                    if (adn) {
                        adn->set_origin(tree::Origin::Structured);
                        adn->set_confidence(conf);
                        ++stats.ads;
                    }
                    sp->release();
                } else {
                    sp->release();
                }
            }
        }
    }

    /* ---- phase B: resolve parents (orphans -> root_lost) ---- */
    std::map<u64, tree::FileNode *> by_record;
    for (const EmittedNode &e : emitted)
        if (e.node->props() && e.node->props()->record_ref != 0) {
            u64 rn = mft_reference_record(e.node->props()->record_ref);
            by_record[rn] = e.node;
        }
    for (const EmittedNode &e : emitted) {
        u64 own = e.node->props() ? mft_reference_record(e.node->props()->record_ref) : 0;
        if (e.parent_record == own && own != 0) {
            /* NTFS root ".": its parent reference is itself - it becomes the
             * visible volume root under the active tree */
            tree.reparent(e.node, root_active);
            continue;
        }
        tree::FileNode *parent_node = nullptr;
        auto it = by_record.find(e.parent_record);
        if (it != by_record.end() && it->second != e.node) parent_node = it->second;
        if (parent_node != nullptr) {
            tree.reparent(e.node, parent_node);
        } else {
            ++stats.orphans;
            /* deleted/unresolved -> lost root for coherent display; active
             * orphans (rare: parent record itself damaged) go there too */
            tree.reparent(e.node, root_lost);
        }
    }

    if (progress) (void)progress(max_record, max_record, progress_user);
    return OK;
}

u32 scan_mft_remnants(IStorageDevice *slice, u32 sector_bytes, u32 cluster_bytes,
                      u64 record_size, CancelToken *cancel, WalkProgress progress,
                      void *progress_user, tree::ResultTree &tree,
                      tree::FullNode *root_lost, WalkStats &stats) {
    if (slice == nullptr || root_lost == nullptr || record_size < 0x400) return ERR_BAD_PARAMETER;
    const u64 total = slice->size();
    std::vector<u8> rec;
    u64 off = 0;
    u64 found = 0;
    while (off + record_size <= total) {
        if (cancel && cancel->cancelled()) return ERR_CANCELLED;
        if ((off & 0xFFFFF) == 0 && progress && !progress(off, total, progress_user))
            return ERR_CANCELLED;
        /* cheap pre-filter: 'FILE' magic only first */
        u8 magic[4];
        if (slice->read_at(off, magic, 4) != OK) break;
        if (u32_from_bytes_le(magic) != RECORD_MAGIC_FILE) {
            off += 512; /* stride by sector - remnant alignment is arbitrary */
            continue;
        }
        rec.resize(size_t(record_size));
        if (slice->read_at(off, rec.data(), u32(rec.size())) != OK) break;
        RecordHeader rh;
        std::memcpy(&rh, rec.data(), sizeof(rh));
        if (rh.magic == RECORD_MAGIC_FILE &&
            fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                        rh.update_sequence_size, sector_bytes) == OK) {
            /* parse names; emit metadata-only nodes (R2.5) */
            std::vector<AttrView> attrs;
            if (enum_attributes(rec.data(), u32(rec.size()), attrs) == OK) {
                for (const AttrView &a : attrs) {
                    if (a.type != ATTR_TYPE_FILE_NAME) continue;
                    FileNameInfo fi;
                    if (parse_file_name(a, fi) != OK) continue;
                    if (fi.name.empty() || fi.name[0] == u'$') continue;
                    tree::FileProperties *props = new (std::nothrow) tree::FileProperties();
                    if (props == nullptr) return ERR_NO_MEMORY;
                    props->name = fi.name;
                    props->fs_kind = 3;
                    props->logical_size = fi.real_size;
                    props->times = fi.times;
                    props->attributes = u16(fi.dos_flags & 0xFF);
                    props->flags = 0; /* detached: no in-use knowledge */
                    tree::FileNode *n = tree.add_file(root_lost, props);
                    if (n) {
                        n->set_origin(tree::Origin::Lost);
                        n->set_deleted_state(tree::DeletedState::Damaged);
                        n->set_confidence(tree::Confidence::MetadataOnly);
                        ++stats.emitted;
                        ++found;
                    }
                    props->release();
                    break; /* first valid $FILE_NAME is enough for a remnant */
                }
            }
        }
        off += 512;
    }
    (void)cluster_bytes;
    if (progress) (void)progress(total, total, progress_user);
    return OK;
}

} // namespace ntfs
} // namespace cr
