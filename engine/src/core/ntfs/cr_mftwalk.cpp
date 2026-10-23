/* cr_mftwalk.cpp - the MFT record walker (R2.3, R2.4).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_mftwalk.h"
#include "cr/cr_parallel.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include <atomic>
#include <cstring>
#include <map>
#include <vector>

namespace cr {
namespace ntfs {

namespace {

constexpr u32 FRAG_DEPTH_LIMIT = 64;         /* attrlist chase depth (bootstrap parity) */
constexpr u64 USN_HARVEST_CAP = 64ull << 20; /* journal bytes mined (audit plan) */

struct EmittedNode {
    tree::FileNode* node = nullptr;
    u64 parent_record = 0;
    bool parent_resolved = false;
};

/* Journal evidence for one deleted name (USN_RECORD_V2 fields). */
struct UsnEvidence {
    std::u16string name;
    u64 parent_record = 0;
    u64 timestamp = 0;
};

int namespace_rank(u8 ns) {
    switch (ns) {
    case FILE_NAME_POSIX:
        return 4;
    case FILE_NAME_WIN32_AND_DOS:
        return 3;
    case FILE_NAME_WIN32:
        return 2;
    case FILE_NAME_DOS:
        return 1;
    default:
        return 0;
    }
}

/* Build props for one record from its attribute views. */
u32 build_props(NtfsVolume* vol, const std::vector<AttrView>& attrs, u64 record_no,
                u16 record_flags, tree::FileProperties*& props_out,
                std::vector<AttrListEntry>& attrlist_out, std::vector<FileNameInfo>& names_out,
                RunList& unnamed_data_out, bool& has_unnamed_data, RunList& i30_out, bool& has_i30,
                tree::FileTimes& si_times, bool& has_si, u64& data_real_size) {
    props_out = nullptr;
    attrlist_out.clear();
    names_out.clear();
    unnamed_data_out = RunList();
    has_unnamed_data = false;
    i30_out = RunList();
    has_i30 = false;
    has_si = false;
    data_real_size = 0;

    for (const AttrView& a : attrs) {
        switch (a.type) {
        case ATTR_TYPE_ATTRIBUTE_LIST: {
            /* entries recorded for the engine's second read (F6 chase) */
            (void)parse_attr_list_entries(a, attrlist_out);
            break;
        }
        case ATTR_TYPE_FILE_NAME: {
            FileNameInfo fi;
            if (parse_file_name(a, fi) == OK)
                names_out.push_back(fi);
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
                    const u8* d = nullptr;
                    u32 dlen = 0;
                    if (resident_span(a, d, dlen) == OK) {
                        has_unnamed_data = true; /* resident */
                        if (a.value_length != 0)
                            data_real_size = a.value_length;
                    }
                } else {
                    RunList rl;
                    if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                        has_unnamed_data = true;
                        /* merge multi-fragment $DATA (same attr across records) */
                        for (const DataRun& r : rl.runs())
                            unnamed_data_out.runs().push_back(r);
                    }
                    /* authoritative file size in bytes (tail @+0x30):
                     * $FILE_NAME sizes are 0 on real volumes for
                     * freshly-written files (runner benchmark evidence) */
                    if (a.actual != 0)
                        data_real_size = a.actual;
                }
            }
            break;
        }
        case ATTR_TYPE_INDEX_ALLOCATION: {
            if (a.name_length ==
                0) { /* $I30 by default name index 0x00? name is "$I30" - accept any */
                RunList rl;
                if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                    has_i30 = true;
                    for (const DataRun& r : rl.runs())
                        i30_out.runs().push_back(r);
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

u32 walk_mft(NtfsVolume* vol, VolumeBitmap* bitmap, const WalkerFlags& flags, CancelToken* cancel,
             WalkProgress progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             WalkStats& stats) {
    if (vol == nullptr || root_active == nullptr || root_deleted == nullptr || root_lost == nullptr)
        return ERR_BAD_PARAMETER;

    const u32 sector_bytes = vol->params().sector_bytes;
    const u64 max_record = vol->max_record_no();
    std::vector<EmittedNode> emitted;
    emitted.reserve(4096);

    /* $Extend/$UsnJrnl:$J capture (harvest input) */
    RunList usn_runs;
    bool have_usn = false;

    /* temp record buffer with fixup scratch */
    std::vector<u8> rec;
    for (u64 record_no = 0; record_no < max_record; ++record_no) {
        if (cancel && cancel->cancelled())
            return ERR_CANCELLED;
        if ((record_no & 0x3F) == 0 && progress && !progress(record_no, max_record, progress_user))
            return ERR_CANCELLED;
        ++stats.examined;

        u32 rc = vol->read_record(record_no, rec);
        if (rc == ERR_RANGE_OUT_OF_BOUNDS)
            continue;
        if (rc != OK) {
            ++stats.damaged;
            continue;
        }
        RecordHeader rh;
        std::memcpy(&rh, rec.data(), sizeof(rh));
        if (rh.magic != RECORD_MAGIC_FILE)
            continue; /* unused slot - not damage */
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
        u64 data_real_size = 0; /* primary unnamed $DATA: authoritative size */
        tree::FileTimes si_times{};
        tree::FileProperties* scratch = nullptr;
        if (build_props(vol, attrs, record_no, rh.flags, scratch, attrlist, names, data_runs,
                        has_data, i30_runs, has_i30, si_times, has_si, data_real_size) != OK) {
            ++stats.damaged;
            continue;
        }

        /* $ATTRIBUTE_LIST chase (F6): read child records, merge their
         * fragments into the views. Child records may carry their own
         * $ATTRIBUTE_LIST (nested lists) - followed breadth-first to
         * FRAG_DEPTH_LIMIT with cycle guards, like the bootstrap's
         * fragmented-MFT path (audit row 6a). */
        if (!attrlist.empty()) {
            std::vector<AttrView> merged = attrs;
            std::vector<u64> visited;
            std::vector<AttrListEntry> frontier = attrlist;
            for (int level = 0; level < int(FRAG_DEPTH_LIMIT) && !frontier.empty(); ++level) {
                std::vector<AttrListEntry> next;
                for (const AttrListEntry& e : frontier) {
                    if (e.type != ATTR_TYPE_DATA && e.type != ATTR_TYPE_INDEX_ALLOCATION &&
                        e.type != ATTR_TYPE_ATTRIBUTE_LIST)
                        continue;
                    u64 child = mft_reference_record(e.record_ref);
                    if (child == record_no || child >= max_record)
                        continue;
                    bool cyc = false;
                    for (u64 v : visited)
                        if (v == child) {
                            cyc = true;
                            break;
                        }
                    if (cyc) {
                        ++stats.damaged;
                        continue;
                    }
                    visited.push_back(child);
                    std::vector<u8> crec;
                    if (vol->read_record(child, crec) != OK)
                        continue;
                    RecordHeader crh;
                    std::memcpy(&crh, crec.data(), sizeof(crh));
                    if (crh.magic != RECORD_MAGIC_FILE)
                        continue;
                    if (fixup_apply(crec.data(), u32(crec.size()), crh.update_sequence_offset,
                                    crh.update_sequence_size, sector_bytes) != OK)
                        continue;
                    std::vector<AttrView> cattrs;
                    if (enum_attributes(crec.data(), u32(crec.size()), cattrs) != OK)
                        continue;
                    for (const AttrView& ca : cattrs) {
                        if (ca.type != ATTR_TYPE_DATA && ca.type != ATTR_TYPE_INDEX_ALLOCATION &&
                            ca.type != ATTR_TYPE_ATTRIBUTE_LIST)
                            continue;
                        merged.push_back(ca);
                        /* fold runlists directly */
                        if (ca.type == ATTR_TYPE_DATA && ca.nonresident && ca.name_length == 0) {
                            RunList rl;
                            if (parse_data_runs(ca, vol->params().total_clusters, rl) == OK) {
                                for (const DataRun& r : rl.runs())
                                    data_runs.runs().push_back(r);
                                has_data = true;
                            }
                            /* the $DATA tail's file-size-in-bytes (@+0x30) is
                             * the authoritative size: $FILE_NAME's size field
                             * is 0 on real volumes for freshly-written files
                             * (runner benchmark evidence: every logical_size
                             * came out 0 while exFAT/FAT dentry sizes were
                             * correct) */
                            if (ca.actual != 0)
                                data_real_size = ca.actual;
                        }
                        if (ca.type == ATTR_TYPE_DATA && !ca.nonresident && ca.name_length == 0) {
                            /* resident primary $DATA (small files): the
                             * content length is the size; no runlist */
                            has_data = true;
                            if (ca.value_length != 0)
                                data_real_size = ca.value_length;
                        }
                        if (ca.type == ATTR_TYPE_INDEX_ALLOCATION && ca.nonresident) {
                            RunList rl;
                            if (parse_data_runs(ca, vol->params().total_clusters, rl) == OK) {
                                for (const DataRun& r : rl.runs())
                                    i30_runs.runs().push_back(r);
                                has_i30 = true;
                            }
                        }
                        if (ca.type == ATTR_TYPE_ATTRIBUTE_LIST) {
                            /* nested list: queue its entries for the next
                             * level (resident content only - the entry
                             * bytes live inside the child record) */
                            if (!ca.nonresident) {
                                std::vector<AttrListEntry> nested;
                                if (parse_attr_list_entries(ca, nested) == OK)
                                    next.insert(next.end(), nested.begin(), nested.end());
                            }
                        }
                    }
                }
                frontier = std::move(next);
            }
            attrs = std::move(merged);
        }

        if (names.empty()) { /* no usable $FILE_NAME - the corpus 'name == NULL' skip */
            ++stats.damaged;
            continue;
        }

        /* pick the best-namespace name; collect distinct parents (hard links) */
        const FileNameInfo* best = &names[0];
        for (const FileNameInfo& n : names)
            if (namespace_rank(n.namespace_type) > namespace_rank(best->namespace_type))
                best = &n;

        /* corpus filters: '$'-prefixed metafiles; SYSTEM files (flags & 4) */
        if (!best->name.empty() && best->name[0] == u'$') {
            /* $UsnJrnl: capture the $J stream for the post-walk harvest
             * (journal-evidenced deleted names the MFT pass missed) */
            if (best->name == u"$UsnJrnl") {
                for (const AttrView& a : attrs) {
                    if (a.type != ATTR_TYPE_DATA || !a.nonresident)
                        continue;
                    std::u16string an = attr_name(a);
                    if (an != u"$J")
                        continue;
                    RunList rl;
                    if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                        usn_runs = rl;
                        have_usn = true;
                    }
                }
            }
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
            const RunList& probe = is_dir ? (has_i30 ? i30_runs : RunList()) : data_runs;
            const u64 probe_size =
                is_dir ? (has_i30 ? i30_runs.total_clusters() * vol->params().cluster_bytes : 0)
                       : (data_real_size ? data_real_size : best->real_size);
            if (!is_dir && !has_data) {
                conf = tree::Confidence::MetadataOnly;
            } else if (probe_size >= flags.viability_min_size && probe.total_clusters() > 0) {
                if (bitmap && bitmap->valid()) {
                    const double ratio = bitmap->reused_ratio(probe);
                    if (ratio >= 1.0)
                        conf = tree::Confidence::Overwritten;
                    else if (ratio > 0.0)
                        conf = tree::Confidence::PartialRisk;
                    else
                        conf = tree::Confidence::Excellent;
                } else {
                    conf = tree::Confidence::PartialRisk; /* no bitmap: honest unknown-ish */
                }
            } else if (probe_size < flags.viability_min_size) {
                conf = tree::Confidence::Excellent; /* corpus fast-path: <1 KiB, likely resident */
                if (!is_dir && !has_data)
                    conf = tree::Confidence::MetadataOnly;
            } else {
                conf = tree::Confidence::PartialRisk;
            }
        } else {
            conf = tree::Confidence::Excellent; /* live file: extents owned by it */
        }

        /* distinct parents -> hard links (shared props object) */
        std::vector<u64> parents;
        for (const FileNameInfo& n : names) {
            u64 p = n.parent_record;
            bool dup = false;
            for (u64 q : parents)
                if (q == p) {
                    dup = true;
                    break;
                }
            if (!dup)
                parents.push_back(p);
        }
        if (parents.size() > 1)
            stats.hardlinks += parents.size() - 1;

        tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
        if (props == nullptr)
            return ERR_NO_MEMORY;
        props->record_ref = (record_no) | (u64(rh.sequence) << 48);
        props->fs_kind = 3; /* NTFS (corpus FS type codes) */
        props->name = best->name;
        /* $DATA size first (authoritative); $FILE_NAME size only as the
         * metadata-only fallback (0 on real volumes for fresh files) */
        props->logical_size = data_real_size ? data_real_size : best->real_size;
        props->allocated_size = best->allocated_size;
        props->times = has_si ? si_times : best->times;
        props->attributes = u16(best->dos_flags & 0xFF) | (is_dir ? 0x10 : 0);
        props->flags = u16(in_use ? PFLAG_IN_USE : 0) | u16(is_dir ? PFLAG_IS_DIR : 0);

        /* extents in slice-relative bytes; the tree consumer resolves to
         * absolute via the slice base (orchestrator does this at merge). */
        if (has_data && !data_runs.empty()) {
            std::vector<RunList::Extent> ex;
            data_runs.extents(ex);
            for (const auto& e : ex) {
                props->extents.push_back({e.lcn * vol->params().cluster_bytes,
                                          e.clusters * vol->params().cluster_bytes});
            }
        }

        /* emit one node per parent, each with ITS OWN link name (per-link
         * $FILE_NAME); hard links duplicate props (rare) - dedup via
         * record_ref downstream (writer + merge). */
        tree::FullNode* default_root = in_use ? root_active : root_deleted;
        tree::FileNode* first_link_node = nullptr; /* canonical link: ADS host */
        for (u64 p : parents) {
            /* this link's name: highest-namespace FILE_NAME with this parent */
            const FileNameInfo* link_name = &names[0];
            for (const FileNameInfo& n : names)
                if (n.parent_record == p &&
                    namespace_rank(n.namespace_type) >= namespace_rank(link_name->namespace_type))
                    link_name = &n;
            if (link_name->parent_record != p && !names.empty() && parents.size() == 1)
                link_name = best; /* single link: keep best */
            tree::FileProperties* link_props = props;
            if (parents.size() > 1 || link_name != best) {
                link_props = props->clone();
                if (link_props == nullptr) {
                    props->release();
                    return ERR_NO_MEMORY;
                }
                link_props->name = link_name->name;
                link_props->logical_size = data_real_size ? data_real_size : link_name->real_size;
                link_props->allocated_size = link_name->allocated_size;
                link_props->times = link_name->times;
            }
            tree::FileNode* n = tree.add_file(default_root, link_props);
            if (n == nullptr) {
                link_props->release();
                props->release();
                return ERR_NO_MEMORY;
            }
            if (link_props != props)
                link_props->release(); /* node holds it */
            if (!in_use) {
                n->set_deleted_state(tree::DeletedState::Deleted);
                if (conf == tree::Confidence::Overwritten)
                    n->set_deleted_state(tree::DeletedState::PartiallyOverwritten);
            }
            n->set_confidence(conf);
            n->set_origin(tree::Origin::Structured);
            if (first_link_node == nullptr)
                first_link_node = n; /* audit row 6c: ADS attaches to the
                                        canonical (first) link, not the last */
            emitted.push_back({n, p, false});
            ++stats.emitted;
        }
        /* ADS: named $DATA -> child nodes (R2.3), attached under the
         * canonical (first) link node of this record */
        if (flags.emit_ads && !is_dir && first_link_node != nullptr) {
            for (const AttrView& a : attrs) {
                if (a.type != ATTR_TYPE_DATA || a.name_length == 0)
                    continue;
                std::u16string sname = attr_name(a);
                if (sname.empty() || sname[0] == u'$')
                    continue;
                tree::FileProperties* sp = new (std::nothrow) tree::FileProperties();
                if (sp == nullptr)
                    continue;
                sp->name = sname;
                sp->fs_kind = 3;
                if (!a.nonresident) {
                    const u8* d = nullptr;
                    u32 dlen = 0;
                    if (resident_span(a, d, dlen) == OK && dlen > 0) {
                        /* resident stream payload: kept with the props so
                         * the writer can emit the actual content (audit row
                         * 6b - size-only left the stream unrecoverable) */
                        sp->logical_size = dlen;
                        sp->resident_data.assign(d, d + dlen);
                    }
                } else {
                    RunList rl;
                    if (parse_data_runs(a, vol->params().total_clusters, rl) == OK) {
                        sp->logical_size = a.actual;
                        std::vector<RunList::Extent> ex;
                        rl.extents(ex);
                        for (const auto& e : ex)
                            sp->extents.push_back({e.lcn * vol->params().cluster_bytes,
                                                   e.clusters * vol->params().cluster_bytes});
                    }
                }
                sp->times = props->times;
                sp->attributes = props->attributes;
                sp->flags = props->flags;
                sp->name = props->name + u":" + sname;
                tree::FileNode* adn = tree.add_file(first_link_node, sp);
                if (adn) {
                    adn->set_origin(tree::Origin::Structured);
                    adn->set_confidence(conf);
                    ++stats.ads;
                }
                sp->release();
            }
        }
        /* each add_file call holds one ref; drop the builder's ref for clones */
        props->release(); /* nodes hold the refs */
        if (in_use)
            ++stats.active;
        else
            ++stats.deleted;
    }

    /* ---- phase B: resolve parents (orphans -> root_lost) ---- */
    std::map<u64, tree::FileNode*> by_record;
    for (const EmittedNode& e : emitted)
        if (e.node->props() && e.node->props()->record_ref != 0) {
            u64 rn = mft_reference_record(e.node->props()->record_ref);
            by_record[rn] = e.node;
        }
    for (const EmittedNode& e : emitted) {
        u64 own = e.node->props() ? mft_reference_record(e.node->props()->record_ref) : 0;
        if (e.parent_record == own && own != 0) {
            /* NTFS root ".": its parent reference is itself - it becomes the
             * visible volume root under the active tree */
            tree.reparent(e.node, root_active);
            continue;
        }
        tree::FileNode* parent_node = nullptr;
        auto it = by_record.find(e.parent_record);
        if (it != by_record.end() && it->second != e.node)
            parent_node = it->second;
        if (parent_node != nullptr) {
            tree.reparent(e.node, parent_node);
        } else {
            ++stats.orphans;
            /* deleted/unresolved -> lost root for coherent display; active
             * orphans (rare: parent record itself damaged) go there too */
            tree.reparent(e.node, root_lost);
        }
    }

    /* ---- phase C: $UsnJrnl harvest (audit row 7) ----
     *
     * Journal-evidenced names the MFT pass missed (record reused or wiped):
     * parse USN_RECORD_V2 entries from $Extend/$UsnJrnl:$J (bounded), keep
     * entries carrying FILE_DELETE, and emit the ones whose (parent, name)
     * the structured walk did not produce. */
    if (flags.harvest_usnjrnl && have_usn && !usn_runs.empty()) {
        std::vector<u8> jbuf;
        u64 jtotal = 0;
        for (const DataRun& dr : usn_runs.runs()) {
            if (dr.sparse || dr.length == 0)
                continue;
            jtotal += dr.length * vol->params().cluster_bytes;
        }
        if (jtotal > USN_HARVEST_CAP)
            jtotal = USN_HARVEST_CAP;
        jbuf.resize(size_t(jtotal));
        u64 got = 0;
        for (const DataRun& dr : usn_runs.runs()) {
            if (got >= jtotal)
                break;
            if (dr.sparse || dr.length == 0)
                continue;
            u64 take = dr.length * vol->params().cluster_bytes;
            if (take > jtotal - got)
                take = jtotal - got;
            if (vol->read_at(dr.lcn * vol->params().cluster_bytes, jbuf.data() + got, u32(take)) !=
                OK)
                break;
            got += take;
        }
        /* structured names for dedup: (parent record, name) */
        std::vector<std::pair<u64, std::u16string>> known;
        known.reserve(emitted.size());
        for (const EmittedNode& e : emitted) {
            if (e.node->props() != nullptr)
                known.emplace_back(e.parent_record, e.node->props()->name);
        }
        /* walk the journal: USN_RECORD_V2 (winioctl.h layout) */
        std::vector<UsnEvidence> evidence;
        size_t jp = 0;
        while (jp + 64 <= jbuf.size()) {
            const u32 rlen = u32(jbuf[jp]) | (u32(jbuf[jp + 1]) << 8) | (u32(jbuf[jp + 2]) << 16) |
                             (u32(jbuf[jp + 3]) << 24);
            if (rlen < 64 || jp + rlen > jbuf.size())
                break; /* zero tail / damaged journal: stop */
            const u16 major = u16(jbuf[jp + 4]) | (u16(jbuf[jp + 5]) << 8);
            if (major != 2) {
                jp += rlen; /* V3 or unknown: self-describing length */
                continue;
            }
            const u64 parent = mft_reference_record(u64_from_bytes_le(jbuf.data() + jp + 16));
            const u64 ts = u64_from_bytes_le(jbuf.data() + jp + 32);
            const u32 reason = u32(jbuf[jp + 40]) | (u32(jbuf[jp + 41]) << 8) |
                               (u32(jbuf[jp + 42]) << 16) | (u32(jbuf[jp + 43]) << 24);
            const u32 namelen = u32(jbuf[jp + 56]) | (u32(jbuf[jp + 57]) << 8) |
                                (u32(jbuf[jp + 58]) << 16) | (u32(jbuf[jp + 59]) << 24);
            const u32 nameoff = u32(jbuf[jp + 60]) | (u32(jbuf[jp + 61]) << 8) |
                                (u32(jbuf[jp + 62]) << 16) | (u32(jbuf[jp + 63]) << 24);
            ++stats.usn_records;
            if ((reason & USN_REASON_FILE_DELETE) != 0 && namelen > 0 && namelen <= 512 &&
                nameoff >= 64 && size_t(nameoff) + namelen <= rlen && (namelen & 1) == 0) {
                std::u16string nm(namelen / 2, u'\0');
                for (u32 i = 0; i < namelen / 2; i++)
                    nm[i] = u16(jbuf[jp + nameoff + i * 2]) |
                            (u16(jbuf[jp + nameoff + i * 2 + 1]) << 8);
                if (!nm.empty() && nm[0] != u'$')
                    evidence.push_back({nm, parent, ts});
            }
            jp += rlen;
        }
        /* dedup against structured results, then emit under root_lost */
        for (const UsnEvidence& ev : evidence) {
            bool dup = false;
            for (const auto& k : known)
                if (k.first == ev.parent_record && k.second == ev.name) {
                    dup = true;
                    break;
                }
            if (dup)
                continue;
            for (const UsnEvidence& prior : evidence) {
                if (&prior == &ev)
                    break;
                if (prior.parent_record == ev.parent_record && prior.name == ev.name) {
                    dup = true; /* same (parent, name) twice in the journal */
                    break;
                }
            }
            if (dup)
                continue;
            tree::FileProperties* jp2 = new (std::nothrow) tree::FileProperties();
            if (jp2 == nullptr)
                break;
            jp2->name = ev.name;
            jp2->fs_kind = 3;
            jp2->logical_size = 0; /* journal carries no size */
            jp2->flags = 0;
            jp2->times.modified = ev.timestamp;
            tree::FileNode* jn = tree.add_file(root_lost, jp2);
            if (jn) {
                jn->set_origin(tree::Origin::Lost);
                jn->set_deleted_state(tree::DeletedState::Deleted);
                jn->set_confidence(tree::Confidence::MetadataOnly);
                ++stats.usn_names;
            }
            jp2->release();
        }
    }

    if (progress)
        (void)progress(max_record, max_record, progress_user);
    return OK;
}

namespace {

/* Block-buffered remnant scanning: the original per-sector 4-byte probes
 * were IO-bound to one syscall per 512 bytes; this reads 4 MiB blocks and
 * scans in memory (records crossing block edges are re-read via a
 * record_size overlap). Semantics preserved exactly: 'FILE' prefilter at
 * 512-byte strides, fixup validation, first valid $FILE_NAME, $-skip. */
constexpr u64 REMNANT_BLOCK = 4ull << 20;

struct RemnantCtx {
    IStorageDevice* slice;
    u32 sector_bytes;
    u64 record_size;
    tree::ResultTree* tree;
    tree::FullNode* root_lost;
    std::atomic<u64> emitted{0};
    std::atomic<bool> aborted{false};
};

void scan_remnant_buffer(RemnantCtx& ctx, const u8* buf, u64 buf_len) {
    if (buf_len < ctx.record_size)
        return;
    const u64 scan_end = buf_len - ctx.record_size;
    for (u64 off = 0; off <= scan_end; off += 512) {
        if (ctx.aborted.load(std::memory_order_relaxed))
            return;
        const u8* rec = buf + off;
        if (std::memcmp(rec, "FILE", 4) != 0)
            continue;
        bool emitted_any = false;
        std::vector<u8> fix(size_t(ctx.record_size));
        std::memcpy(fix.data(), rec, size_t(ctx.record_size));
        RecordHeader rh;
        std::memcpy(&rh, fix.data(), sizeof(rh));
        /* every parse failure below falls through to the shifted-header
         * salvage (og behavior: the probe runs on the failed record) */
        if (rh.magic == RECORD_MAGIC_FILE &&
            fixup_apply(fix.data(), u32(fix.size()), rh.update_sequence_offset,
                        rh.update_sequence_size, ctx.sector_bytes) == OK) {
            std::vector<AttrView> attrs;
            if (enum_attributes(fix.data(), u32(fix.size()), attrs) == OK) {
                for (const AttrView& a : attrs) {
                    if (a.type != ATTR_TYPE_FILE_NAME)
                        continue;
                    FileNameInfo fi;
                    if (parse_file_name(a, fi) != OK)
                        continue;
                    if (fi.name.empty() || fi.name[0] == u'$')
                        continue;
                    tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
                    if (props == nullptr) {
                        ctx.aborted.store(true, std::memory_order_relaxed);
                        return;
                    }
                    props->name = fi.name;
                    props->fs_kind = 3;
                    props->logical_size = fi.real_size;
                    props->times = fi.times;
                    props->attributes = u16(fi.dos_flags & 0xFF);
                    props->flags = 0; /* detached: no in-use knowledge */
                    tree::FileNode* n = ctx.tree->add_file(ctx.root_lost, props);
                    if (n) {
                        n->set_origin(tree::Origin::Lost);
                        n->set_deleted_state(tree::DeletedState::Damaged);
                        n->set_confidence(tree::Confidence::MetadataOnly);
                        ctx.emitted.fetch_add(1, std::memory_order_relaxed);
                    }
                    props->release();
                    emitted_any = true;
                    break; /* first valid $FILE_NAME is enough for a remnant */
                }
            }
        }
        if (emitted_any)
            continue;
        /* og shifted-header salvage (NTFSSearchDLL FUN_18002a780): when a
         * candidate's header parses as 'FILE' but yields no record, probe
         * for a healthy "FILE"+0x30 ('FILE0': magic + the typical
         * usa_offset low byte) at +0x58, else +0x60, and parse the record
         * from there into a zeroed record_size buffer (tail bytes beyond
         * the shifted window are lost - a partial record with a valid
         * $FILE_NAME still earns a Lost node). */
        u32 delta = 0;
        if (0x58 + 0x70 <= ctx.record_size && std::memcmp(rec + 0x58, "FILE", 4) == 0 &&
            rec[0x58 + 4] == '0')
            delta = 0x58;
        else if (0x60 + 0x70 <= ctx.record_size && std::memcmp(rec + 0x60, "FILE", 4) == 0 &&
                 rec[0x60 + 4] == '0')
            delta = 0x60;
        if (delta == 0)
            continue;
        /* full record when the block buffer reaches past the shift (the
         * og copied only record_size - delta bytes - a buffer artifact;
         * the full read recovers the record's tail); at the buffer edge
         * we fall back to the og's truncated form (fixup may refuse) */
        const u64 from_delta = buf_len - (off + delta);
        const u64 avail = from_delta >= ctx.record_size ? ctx.record_size : ctx.record_size - delta;
        if (avail < 0x70)
            continue;
        std::vector<u8> shifted(size_t(ctx.record_size), 0);
        std::memcpy(shifted.data(), rec + delta, size_t(avail));
        RecordHeader srh;
        std::memcpy(&srh, shifted.data(), sizeof(srh));
        if (srh.magic != RECORD_MAGIC_FILE)
            continue;
        if (fixup_apply(shifted.data(), u32(shifted.size()), srh.update_sequence_offset,
                        srh.update_sequence_size, ctx.sector_bytes) != OK)
            continue;
        std::vector<AttrView> sattrs;
        if (enum_attributes(shifted.data(), u32(shifted.size()), sattrs) != OK)
            continue;
        for (const AttrView& a : sattrs) {
            if (a.type != ATTR_TYPE_FILE_NAME)
                continue;
            FileNameInfo fi;
            if (parse_file_name(a, fi) != OK)
                continue;
            if (fi.name.empty() || fi.name[0] == u'$')
                continue;
            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (props == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return;
            }
            props->name = fi.name;
            props->fs_kind = 3;
            props->logical_size = fi.real_size;
            props->times = fi.times;
            props->attributes = u16(fi.dos_flags & 0xFF);
            props->flags = 0;
            tree::FileNode* n = ctx.tree->add_file(ctx.root_lost, props);
            if (n) {
                n->set_origin(tree::Origin::Lost);
                n->set_deleted_state(tree::DeletedState::Damaged);
                n->set_confidence(tree::Confidence::MetadataOnly);
                ctx.emitted.fetch_add(1, std::memory_order_relaxed);
            }
            props->release();
            break;
        }
    }
}

u32 remnant_range_scan(RemnantCtx& ctx, u64 start, u64 end) {
    std::vector<u8> buf;
    u64 base = start;
    while (base < end) {
        if (ctx.aborted.load(std::memory_order_relaxed))
            return ERR_CANCELLED;
        const u64 remain = end - base;
        const u64 want =
            remain < REMNANT_BLOCK + ctx.record_size ? remain : REMNANT_BLOCK + ctx.record_size;
        if (want < ctx.record_size)
            break;
        buf.resize(size_t(want));
        if (ctx.slice->read_at(base, buf.data(), u32(want)) != OK) {
            /* unreadable block: skip forward (bad-sector policy is the
             * transport's job; here we just continue the sweep) */
            base += REMNANT_BLOCK;
            continue;
        }
        scan_remnant_buffer(ctx, buf.data(), want);
        base += REMNANT_BLOCK;
    }
    return OK;
}

} // namespace

u32 scan_mft_remnants(IStorageDevice* slice, u32 sector_bytes, u32 cluster_bytes, u64 record_size,
                      CancelToken* cancel, WalkProgress progress, void* progress_user,
                      tree::ResultTree& tree, tree::FullNode* root_lost, WalkStats& stats,
                      u32 threads) {
    if (slice == nullptr || root_lost == nullptr || record_size < 0x400)
        return ERR_BAD_PARAMETER;
    (void)cluster_bytes;

    const u64 total = slice->size();
    RemnantCtx ctx;
    ctx.slice = slice;
    ctx.sector_bytes = sector_bytes;
    ctx.record_size = record_size;
    ctx.tree = &tree;
    ctx.root_lost = root_lost;

    u32 rc = OK;
    if (threads > 1 && total > REMNANT_BLOCK * 2) {
        /* fan out chunk ranges over the pool; the tree serializes emission */
        ThreadPool pool(threads);
        PoolCancelToken pool_cancel(&pool);
        const u64 chunk = ((total / threads) + 511) & ~u64(511);
        const u64 nchunks = (total + chunk - 1) / chunk;
        std::atomic<u64> done_chunks{0};
        rc = parallel_for(pool, nchunks, [&](u64 ci) -> u32 {
            if (cancel && cancel->cancelled())
                return ERR_CANCELLED;
            const u64 cstart = ci * chunk;
            const u64 cend = (ci + 1) * chunk < total ? (ci + 1) * chunk : total;
            const u32 prc = remnant_range_scan(ctx, cstart, cend);
            const u64 d = done_chunks.fetch_add(1, std::memory_order_relaxed) + 1;
            if (progress && !progress(d * chunk > total ? total : d * chunk, total, progress_user))
                return ERR_CANCELLED;
            return prc;
        });
        if (cancel && cancel->cancelled() && rc == OK)
            rc = ERR_CANCELLED;
    } else {
        rc = remnant_range_scan(ctx, 0, total);
        if (rc == OK && cancel && cancel->cancelled())
            rc = ERR_CANCELLED;
    }
    stats.emitted += ctx.emitted.load();
    if (progress)
        (void)progress(total, total, progress_user);
    return rc;
}

} // namespace ntfs
} // namespace cr
