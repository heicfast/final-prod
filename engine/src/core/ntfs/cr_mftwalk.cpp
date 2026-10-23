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
#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace cr {
namespace ntfs {

namespace {

constexpr u32 FRAG_DEPTH_LIMIT = 64;         /* attrlist chase depth (bootstrap parity) */
constexpr u64 USN_HARVEST_CAP = 64ull << 20; /* journal bytes mined (audit plan) */
constexpr u32 USN_RESYNC_LIMIT = 1u << 20;   /* 8-byte re-sync probes before
                                              * giving up on a damaged $J */

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
    u64 file_ref = 0; /* USN_RECORD FileReferenceNumber (u48 record) */
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
                tree::FileTimes& si_times, bool& has_si, std::vector<u8>& unnamed_resident_out,
                u64& data_real_size) {
    props_out = nullptr;
    attrlist_out.clear();
    names_out.clear();
    unnamed_data_out = RunList();
    has_unnamed_data = false;
    i30_out = RunList();
    has_i30 = false;
    has_si = false;
    unnamed_resident_out.clear();
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
                        /* capture the content (bounded by the record itself):
                         * tiny resident-only files become recoverable */
                        unnamed_resident_out.assign(d, d + dlen);
                        /* the resident length IS the file size */
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
                    /* the $DATA tail's file-size-in-bytes (@+0x30) is the
                     * authoritative size: $FILE_NAME's size field is 0 on
                     * real volumes for freshly-written files */
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

/* ---- $Recycle.Bin mapping (og parity: recycle-bin files surface under
 * their pre-deletion name; $I metadata parsed, $R content remapped) ---- */
struct RecycleInfo {
    u64 size = 0;
    u64 time = 0;
    std::u16string orig_name; /* basename of the $I-recorded original path */
};

/* $I layout (verified against the shell's writer behavior): shared
 * offsets for v1 (28 + 2*pathlen bytes) and v2 (544 fixed):
 * version u32@0, size u64@8, time u64@16, pathlen u32@24, path@28 */
static u32 parse_recycle_info(const std::vector<AttrView>& attrs, RecycleInfo& out) {
    for (const AttrView& a : attrs) {
        if (a.type != ATTR_TYPE_DATA || a.nonresident || a.name_length != 0)
            continue;
        const u8* d = nullptr;
        u32 dlen = 0;
        if (resident_span(a, d, dlen) != OK)
            continue;
        if (dlen < 30)
            continue;
        const u32 ver = u32_from_bytes_le(d);
        if (ver != 1 && ver != 2)
            continue;
        const u64 size = u64_from_bytes_le(d + 8);
        const u64 t = u64_from_bytes_le(d + 16);
        /* Path length determination is FORMAT-DEPENDENT (runner benchmark
         * evidence - every $I parse was rejected on the real volume):
         * - Vista..Win8 fixed form (dlen==544): pathlen is the u32@0x18
         *   (the field the old code trusted on ALL forms).
         * - Windows 10+ variable form (dlen == 28 + 2*pathlen): @0x18 was
         *   REPURPOSED as a deletion-sequence counter (37, 38, 42... -
         *   Champlain Forensics, Win10 $I research); the path is the whole
         *   tail, NUL-padded, terminated by three zero bytes.
         * Try the @0x18 value first when it validates against dlen, else
         * derive from the tail length; the string is cut at the first NUL
         * either way. */
        const u32 field24 = u32_from_bytes_le(d + 24);
        const u32 derived = (dlen >= 28 && ((dlen - 28) & 1) == 0) ? u32((dlen - 28) / 2) : 0;
        u32 plen = 0;
        if (field24 >= 1 && field24 <= 1024 && 28 + u64(field24) * 2 <= dlen)
            plen = field24;
        else if (derived >= 1)
            plen = derived;
        if (plen == 0 || plen > 1024)
            continue;
        std::u16string path;
        path.reserve(plen);
        for (u32 i = 0; i < plen; ++i) {
            const u16 ch = u16_from_bytes_le(d + 28 + i * 2);
            if (ch == 0)
                break; /* NUL-terminated / NUL-padded tail */
            path.push_back(ch);
        }
        if (path.empty())
            continue;
        const size_t slash = path.rfind(u'\\');
        const std::u16string base = slash == std::u16string::npos ? path : path.substr(slash + 1);
        if (base.empty())
            continue;
        out.size = size;
        out.time = t;
        out.orig_name = base;
        return OK;
    }
    return ERR_UNSUPPORTED;
}

/* volume metafiles only - NOT every '$' name (the old blanket skip
 * swallowed $Recycle.Bin/$Extend subtree content: recycle deletions
 * vanished from the results; user files named '$...' also vanished) */
static bool is_metafile_name(const std::u16string& n) {
    static const std::u16string kMeta[] = {
        u"$MFT",        u"$MFTMirr", u"$LogFile", u"$Volume", u"$AttrDef",  u"$Bitmap", u"$Boot",
        u"$BadClus",    u"$Secure",  u"$UpCase",  u"$Extend", u"$Quota",    u"$ObjId",  u"$Reparse",
        u"$RmMetadata", u"$UsnJrnl", u"$SDH",     u"$SII",    u"$Transact", u"$Config"};
    for (const std::u16string& m : kMeta)
        if (n == m)
            return true;
    return false;
}

constexpr u64 REMNANT_BLOCK = 4ull << 20;

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
    /* $Recycle.Bin $I metadata seen so far (random suffix -> info) */
    std::map<std::u16string, RecycleInfo> recycle_info;

    /* temp record buffer with fixup scratch */
    std::vector<u8> rec;
    for (u64 record_no = 0; record_no < max_record; ++record_no) {
        if (cancel && cancel->checkpoint())
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
        /* NTFS-8 (AUDIT-NTFS): extension records (base_record != 0) carry
         * no $FILE_NAME of their own - the og skips them silently via the
         * base-record gate (FUN_180041970, Dec 21155-21157) while our
         * names.empty() path counted each one damaged, inflating the
         * damaged stat on every fragmented ($ATTRIBUTE_LIST) volume.
         * The attrlist chase reads children explicitly; the main walk
         * never needs them. */
        if (rh.base_record != 0)
            continue;
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
        u64 data_real_size = 0;       /* primary unnamed $DATA: authoritative size */
        bool recycle_pending = false; /* this record is a $Recycle.Bin $R */
        tree::FileTimes si_times{};
        tree::FileProperties* scratch = nullptr;
        std::vector<u8> unnamed_resident;
        if (build_props(vol, attrs, record_no, rh.flags, scratch, attrlist, names, data_runs,
                        has_data, i30_runs, has_i30, si_times, has_si, unnamed_resident,
                        data_real_size) != OK) {
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
            /* child record buffers: ownership pool so AttrViews created in
             * the chase stay valid for the whole record's processing */
            std::vector<std::vector<u8>> child_records;
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
                    /* the views point into crec's buffer - it must outlive
                     * `merged` (named child attrs are dereferenced by
                     * attr_name/resident_span long after this iteration) */
                    child_records.push_back(std::move(crec));
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
                            /* $DATA authoritative size from the child too */
                            if (ca.actual != 0)
                                data_real_size = ca.actual;
                        }
                        if (ca.type == ATTR_TYPE_DATA && !ca.nonresident && ca.name_length == 0) {
                            /* resident primary $DATA fragment in a child
                             * record: content + size */
                            const u8* d = nullptr;
                            u32 dlen = 0;
                            if (resident_span(ca, d, dlen) == OK) {
                                has_data = true;
                                if (unnamed_resident.empty())
                                    unnamed_resident.assign(d, d + dlen);
                                if (ca.value_length != 0)
                                    data_real_size = ca.value_length;
                            }
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

        /* corpus filters: KNOWN metafiles only (the old blanket '$'-skip
         * also swallowed every $Recycle.Bin/$Extend SUBTREE file - recycle
         * deletions vanished from the results entirely); SYSTEM files */
        if (is_metafile_name(best->name)) {
            /* $UsnJrnl: capture the $J stream for the post-walk harvest
             * (journal-evidenced deleted names the MFT pass missed) */
            if (best->name == u"$UsnJrnl") {
                /* $J may be fragmented across extension records: every $J
                 * fragment's runlist contributes; runs carry absolute VCNs
                 * (each fragment decodes from its own start_vcn), so the
                 * merged stream is the VCN-sorted concatenation - not the
                 * last fragment (last-fragment-wins harvested a random
                 * slice of real journals) */
                std::vector<DataRun> jruns;
                for (const AttrView& a : attrs) {
                    if (a.type != ATTR_TYPE_DATA || !a.nonresident)
                        continue;
                    std::u16string an = attr_name(a);
                    if (an != u"$J")
                        continue;
                    RunList rl;
                    if (parse_data_runs(a, vol->params().total_clusters, rl) == OK)
                        for (const DataRun& r : rl.runs())
                            jruns.push_back(r);
                }
                if (!jruns.empty()) {
                    std::sort(jruns.begin(), jruns.end(),
                              [](const DataRun& x, const DataRun& y) { return x.vcn < y.vcn; });
                    usn_runs.runs() = std::move(jruns);
                    have_usn = true;
                }
            }
            ++stats.meta_skipped;
            continue;
        }
        /* $Recycle.Bin: $I<rand> carries the ORIGINAL path/size/time of the
         * recycle-deleted file (resident $DATA, v1: 28+2*pathlen bytes,
         * v2: 544 fixed); $R<rand> is the content. Map $R records back to
         * their ORIGINAL name + $I-recorded size and mark them Deleted
         * (they were deleted from the user's location; the record stays
         * in-use in the bin). $I metadata files never surface. */
        if (best->name.size() >= 3 && best->name[0] == u'$' &&
            (best->name[1] == u'I' || best->name[1] == u'R')) {
            const std::u16string rnd = best->name.substr(2);
            if (best->name[1] == u'I') {
                RecycleInfo ri;
                if (parse_recycle_info(attrs, ri) == OK) {
                    recycle_info[rnd] = ri;
                    ++stats.recycle_info;
                }
                continue; /* bin metadata, not recoverable content */
            }
            recycle_pending = true; /* $R: mapped below if its $I was seen */
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
            /* probe size must use the $DATA-authoritative size: $FILE_NAME's
             * real_size is 0 on real volumes for freshly-written files
             * (PARITY-AUDIT F-A1-2 — the Gen-2 emission lesson, applied to
             * the probe; emission prefers data_real_size at :531 already) */
            const u64 authoritative_size = data_real_size ? data_real_size : best->real_size;
            const u64 probe_size =
                is_dir ? (has_i30 ? i30_runs.total_clusters() * vol->params().cluster_bytes : 0)
                       : (has_data ? authoritative_size : 0);
            if (!is_dir && !has_data) {
                conf = tree::Confidence::MetadataOnly;
            } else if (probe_size > flags.viability_min_size && probe.total_clusters() > 0) {
                /* NTFS-1 (AUDIT-NTFS): og viability gate is STRICT -
                 * `0x3ff < size` (Dec 21208) - files of exactly 0x3FF
                 * (1023) bytes take the fast-path with NO bitmap check.
                 * The old `>=` probed them (unregistered divergence). */
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
            } else if (probe_size <= flags.viability_min_size && (is_dir || has_data)) {
                conf = tree::Confidence::Excellent; /* corpus fast-path: <1 KiB, likely resident
                                                     * (== boundary included: og's gate is
                                                     * `0x3ff < size`, NTFS-1) */
            } else {
                conf = tree::Confidence::PartialRisk; /* >= gate but no clusters to probe */
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
        props->logical_size = data_real_size ? data_real_size : best->real_size;
        props->allocated_size = best->allocated_size;
        props->times = has_si ? si_times : best->times;
        props->attributes = u16(best->dos_flags & 0xFF) | (is_dir ? 0x10 : 0);
        /* $R<rand> recycle remap: the ORIGINAL name/size/time from the $I
         * sibling (the $R record itself stays in-use inside the bin - it
         * was deleted from the user's location, so the node is Deleted) */
        bool recycle_remapped = false;
        if (recycle_pending) {
            recycle_pending = false;
            auto it = recycle_info.find(best->name.substr(2));
            if (it != recycle_info.end()) {
                props->name = it->second.orig_name;
                if (it->second.size != 0)
                    props->logical_size = it->second.size;
                props->times.modified = it->second.time;
                recycle_remapped = true;
            }
        }
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
        } else if (has_data && !unnamed_resident.empty() && !is_dir) {
            /* resident-only file: the content travels in the props (the
             * writer emits resident_data before any extents) */
            props->resident_data = std::move(unnamed_resident);
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
            } else if (recycle_remapped) {
                /* recycle-bin content: the record is in-use inside the bin,
                 * but the file was deleted from the user's location */
                n->set_deleted_state(tree::DeletedState::Deleted);
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
     * parse USN_RECORD_V2/V3 entries from $Extend/$UsnJrnl:$J (bounded),
     * keep entries carrying FILE_DELETE, and emit the ones whose record or
     * (parent, name) the structured walk did not produce. $J is sparse:
     * the stream is materialized in VCN order with sparse runs as zeros
     * (record alignment depends on it); a damaged record re-syncs to the
     * next 8-byte boundary instead of terminating the harvest.
     *
     * Layouts (winioctl.h): V2 refs are 64-bit; V3 uses 128-bit
     * FileReferenceNumber/ParentFileReferenceNumber, shifting every later
     * field by +0x10. DELETE is the only deletion signal - a bare
     * FILE_CREATE belongs to live files and must never surface deleted. */
    if (flags.harvest_usnjrnl && have_usn && !usn_runs.empty()) {
        std::vector<u8> jbuf;
        u64 jtotal = 0;
        for (const DataRun& dr : usn_runs.runs())
            jtotal += dr.length * vol->params().cluster_bytes;
        if (jtotal > USN_HARVEST_CAP)
            jtotal = USN_HARVEST_CAP;
        jbuf.assign(size_t(jtotal), 0);
        u64 got = 0;
        for (const DataRun& dr : usn_runs.runs()) {
            if (got >= jtotal)
                break;
            u64 run_bytes = dr.length * vol->params().cluster_bytes;
            if (run_bytes > jtotal - got)
                run_bytes = jtotal - got;
            if (dr.sparse || dr.length == 0 || dr.lcn == 0) {
                got += run_bytes; /* sparse/unmapped: zeros (pre-assigned) */
                continue;
            }
            if (vol->read_at(dr.lcn * vol->params().cluster_bytes, jbuf.data() + got,
                             u32(run_bytes)) != OK) {
                /* damaged run: leave zeros, continue the stream */
                ++stats.usn_damaged_runs;
                got += run_bytes;
                continue;
            }
            got += run_bytes;
        }
        /* structured record set + names for dedup */
        std::set<u64> known_records;
        std::vector<std::pair<u64, std::u16string>> known;
        known.reserve(emitted.size());
        for (const EmittedNode& e : emitted) {
            if (e.node->props() != nullptr) {
                known.emplace_back(e.parent_record, e.node->props()->name);
                known_records.insert(e.node->props()->record_ref);
            }
        }
        /* walk the journal in 8-byte alignment with resync */
        std::vector<UsnEvidence> evidence;
        size_t jp = 0;
        u32 resyncs = 0;
        while (jp + 64 <= jbuf.size()) {
            const u32 rlen = u32(jbuf[jp]) | (u32(jbuf[jp + 1]) << 8) | (u32(jbuf[jp + 2]) << 16) |
                             (u32(jbuf[jp + 3]) << 24);
            const u16 major = u16(jbuf[jp + 4]) | (u16(jbuf[jp + 5]) << 8);
            const u16 minor = u16(jbuf[jp + 6]) | (u16(jbuf[jp + 7]) << 8);
            if (rlen < 64 || jp + rlen > jbuf.size() || (major != 2 && major != 3) ||
                (major == 2 && minor != 0)) {
                /* zero/short/damaged window: re-sync to the next 8-byte
                 * boundary and probe again (bounded) */
                jp = (jp + 7) & ~size_t(7);
                if (++resyncs > USN_RESYNC_LIMIT)
                    break;
                jp += 8;
                continue;
            }
            const u32 ref_off = 8;                       /* V2 */
            const u32 parent_off = major == 2 ? 16 : 24; /* V3: 128-bit refs */
            const u32 ts_off = major == 2 ? 32 : 48;
            const u32 reason_off = major == 2 ? 40 : 56;
            const u32 namelen_off = major == 2 ? 56 : 72;
            const u32 nameoff_off = major == 2 ? 60 : 76;
            const u32 name_off = major == 2 ? 64 : 80;
            const u64 file_ref =
                mft_reference_record(u64_from_bytes_le(jbuf.data() + jp + ref_off));
            const u64 parent =
                mft_reference_record(u64_from_bytes_le(jbuf.data() + jp + parent_off));
            const u64 ts = u64_from_bytes_le(jbuf.data() + jp + ts_off);
            const u32 reason = u32_from_bytes_le(jbuf.data() + jp + reason_off);
            const u32 namelen = u32_from_bytes_le(jbuf.data() + jp + namelen_off);
            const u32 nameoff = u32_from_bytes_le(jbuf.data() + jp + nameoff_off);
            ++stats.usn_records;
            if ((reason & USN_REASON_FILE_DELETE) != 0 && namelen > 0 && namelen <= 512 &&
                nameoff >= name_off && nameoff + namelen <= rlen && (namelen & 1) == 0) {
                std::u16string nm(namelen / 2, u'\0');
                for (u32 i = 0; i < namelen / 2; i++)
                    nm[i] = u16(jbuf[jp + nameoff + i * 2]) |
                            (u16(jbuf[jp + nameoff + i * 2 + 1]) << 8);
                if (!nm.empty() && nm[0] != u'$')
                    evidence.push_back({nm, parent, ts, file_ref});
            }
            jp += rlen;
        }
        /* dedup against structured results, then emit under root_lost */
        for (const UsnEvidence& ev : evidence) {
            bool dup = known_records.count(ev.file_ref) != 0;
            if (!dup) {
                for (const auto& k : known)
                    if (k.first == ev.parent_record && k.second == ev.name) {
                        dup = true;
                        break;
                    }
            }
            if (dup)
                continue;
            for (const UsnEvidence& prior : evidence) {
                if (&prior == &ev)
                    break;
                if (prior.parent_record == ev.parent_record && prior.name == ev.name &&
                    prior.file_ref == ev.file_ref) {
                    dup = true; /* same (record, parent, name) twice */
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
            jp2->record_ref = ev.file_ref;
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

struct RemnantCtx {
    IStorageDevice* slice;
    u32 sector_bytes;
    u64 record_size;
    tree::ResultTree* tree;
    tree::FullNode* root_lost;
    std::atomic<u64> emitted{0};
    std::atomic<u64> dup_skipped{0};
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
                    /* prefer a $DATA size from the same record (real
                     * volumes leave $FILE_NAME sizes 0 for fresh files);
                     * detached records have no other source */
                    u64 rem_size = fi.real_size;
                    for (const AttrView& da : attrs) {
                        if (da.type != ATTR_TYPE_DATA || da.name_length != 0)
                            continue;
                        if (!da.nonresident && da.value_length != 0) {
                            rem_size = da.value_length;
                            break;
                        }
                        if (da.nonresident && da.actual != 0) {
                            rem_size = da.actual;
                            break;
                        }
                    }
                    /* dedup vs structured results: if the parent dir node
                     * (by record ref) already carries this name, the MFT
                     * pass emitted this record - the remnant scan must not
                     * add a size-0 duplicate */
                    tree::FileNode* parent_node = ctx.tree->find_by_record(fi.parent_record);
                    if (parent_node != nullptr) {
                        for (tree::FileNode* c = parent_node->first_child(); c != nullptr;
                             c = c->next_sibling()) {
                            if (c->props() != nullptr && c->props()->name == fi.name) {
                                ctx.dup_skipped.fetch_add(1, std::memory_order_relaxed);
                                props->release();
                                props = nullptr;
                                break;
                            }
                        }
                    }
                    if (props == nullptr)
                        continue;
                    props->name = fi.name;
                    props->fs_kind = 3;
                    props->logical_size = rem_size;
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
        /* The og salvages WITHOUT applying the USA fixup (FUN_18002a780:
         * header sanity + parse only) - truncated-edge images carry zero
         * sector tails that a strict fixup_validate would reject. Try the
         * fixup; on failure parse anyway (enum_attributes bounds-checks
         * every field, and the remnant path below grades by content). */
        (void)fixup_apply(shifted.data(), u32(shifted.size()), srh.update_sequence_offset,
                          srh.update_sequence_size, ctx.sector_bytes);
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
            if (cancel && cancel->checkpoint())
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
