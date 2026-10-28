/* cr_mftwalk.cpp - the MFT record walker (R2.3, R2.4).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_mftwalk.h"
#include "cr/compress/cr_decomp.h"
#include "cr/cr_parallel.h"
#include "cr/cr_recycle.h" /* M-052/N-079: shared $I/$R pairing model */
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
                tree::FileTimes& si_times, bool& has_si, u32& si_flags_out,
                std::vector<u8>& unnamed_resident_out, u64& data_real_size,
                bool& data_compressed_out, u16& data_cunit_out) {
    props_out = nullptr;
    attrlist_out.clear();
    names_out.clear();
    unnamed_data_out = RunList();
    has_unnamed_data = false;
    i30_out = RunList();
    has_i30 = false;
    has_si = false;
    si_flags_out = 0;
    unnamed_resident_out.clear();
    data_real_size = 0;
    data_compressed_out = false;
    data_cunit_out = 0;

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
                si_flags_out = si.dos_flags; /* N-012: the og serializes
                                              * attributes from $SI
                                              * (FUN_180044f60 @ :5351-68) */
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
                    /* GAP-P1-1: compressed $DATA - capture the envelope
                     * facts (compression_unit @+0x22 of the nonresident
                     * header); encryption is out of scope (F13) and means
                     * no content recovery at all */
                    if ((a.flags & ATTR_FLAG_COMPRESSED) != 0 &&
                        (a.flags & ATTR_FLAG_ENCRYPTED) == 0 && a.at != nullptr &&
                        size_t(a.at + 0x22 + 2) <= size_t(a.at + a.buf_size)) {
                        data_compressed_out = true;
                        data_cunit_out = u16(a.at[0x22]) | (u16(a.at[0x23]) << 8);
                    }
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
 * their pre-deletion name; $I metadata parsed, $R content remapped) ----
 * M-052: the record/token/bin-root primitives moved to the shared
 * cr/cr_recycle.h (the FAT and exFAT engines consume the same model);
 * the NTFS wrappers below keep this walk's landed shape. */
struct RecycleInfo {
    u64 size = 0;
    u64 time = 0;
    std::u16string orig_name; /* basename of the $I-recorded original path */
};

/* $I layout (verified against the shell's writer behavior): shared
 * offsets for v1 (28 + 2*pathlen bytes) and v2 (544 fixed):
 * version u32@0, size u64@8, time u64@16, pathlen u32@24, path@28.
 * The parse itself is recycle::parse_record (cr_recycle.h). */
static u32 parse_recycle_info(const std::vector<AttrView>& attrs, RecycleInfo& out) {
    for (const AttrView& a : attrs) {
        if (a.type != ATTR_TYPE_DATA || a.nonresident || a.name_length != 0)
            continue;
        const u8* d = nullptr;
        u32 dlen = 0;
        if (resident_span(a, d, dlen) != OK)
            continue;
        recycle::Info ri;
        if (!recycle::parse_record(d, dlen, ri))
            continue;
        out.size = ri.size;
        out.time = ri.time;
        out.orig_name = std::move(ri.orig_name);
        return OK;
    }
    return ERR_UNSUPPORTED;
}

/* $Recycle.Bin pairing token: the random chars after the $I/$R marker.
 * REAL Windows convention (benchmark-volume evidence, GAP-P1-3): the
 * $R keeps the ORIGINAL EXTENSION while its $I does not
 * ($R2X9K4L.jpg pairs with $I2X9K4L) - so substr(2) keys never
 * matched and 0/26 recycle deletions surfaced raw-named. The extension
 * is NOT part of the pairing token: strip at the first '.' (the shell
 * token never contains one). Shared spelling: recycle::token_of. */
static std::u16string recycle_token(const std::u16string& n) {
    return recycle::token_of(n);
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

/* ---- S-012 pass 2 helpers ----
 *
 * Og dump-file name shape (collection site :18584-18625): a 0x4C-char name,
 * '{' at [0], '}' at [37], and the fixed 38-char magic GUID at [38..75] -
 * i.e. "{<dump-id-guid>}{3808876b-c176-4e48-b7ae-04046e6cc752}" (the
 * memcmp at :18607-09 compares the last 0x26 bytes against the template
 * with its NUL, and the separate pcVar13[-1]=='}' check pins [37]). */
static bool is_guid_dump_name(const std::u16string& n) {
    static const std::u16string kMagic = u"{3808876b-c176-4e48-b7ae-04046e6cc752}";
    return n.size() == 0x4C && n[0] == u'{' && n[37] == u'}' && n[38] == u'{' && n[75] == u'}' &&
           n.compare(38, 38, kMagic) == 0;
}

/* Og pass-2 metafile gate (FUN_180034b20 -> FUN_180040ea0/FUN_180040f20):
 * FIRST $FILE_NAME's name matched against the classic 15-entry table AND
 * the $STANDARD_INFORMATION dos_flags carrying BOTH hidden (0x02) and
 * system (0x04). (Pass 1 keeps the core's wider 20-entry table - I-001.) */
static bool og_pass2_metafile_gate(const std::u16string& name, u64 si_flags) {
    static const std::u16string kOg15[] = {u"$MFT",     u"$MFTMirr", u"$LogFile",   u"$Volume",
                                           u"$AttrDef", u"$Bitmap",  u"$Boot",      u"$BadClus",
                                           u"$Secure",  u"$UpCase",  u"$Extend",    u"$Quota",
                                           u"$ObjId",   u"$Reparse", u"$RmMetadata"};
    for (const std::u16string& m : kOg15)
        if (name == m)
            return (si_flags & 0x06) == 0x06;
    return false;
}

/* NTFS compression-block envelope expansion (GAP-P1-1).
 *
 * A compressed nonresident $DATA stores per-COMPRESSION-UNIT streams:
 * the unit = 2^cunit clusters, each covered by per-cluster blocks
 * [u16 header][payload] (ntfs-3g ntfs_decompress shape; MS-XCA 3.3):
 *   header == 0    -> hole: the remaining blocks of the unit are sparse
 *                     (no extent; the writer zero-fills)
 *   bit 15 set     -> raw block: payload = one full cluster, codec 0
 *   else           -> LZNT1 chunk: payload = (hdr & 0xFFF) + 1 bytes,
 *                     decoding to min(cluster, logical-remaining) bytes
 *   bits 12..14 carry the signature (3); we accept any (recovery-grade)
 * The per-unit stored stream occupies the FIRST clusters of the unit's
 * VCN range, so buffer position maps back to disk run-by-run. On any
 * envelope inconsistency (unit read failure, header truncation, hostile
 * cunit/cluster geometry) the file gets NO extents: all-zero content is
 * the honest fallback - surfacing the raw compressed bytes would be the
 * silent corruption this fix exists to remove. */
static void expand_compressed_data(NtfsVolume* vol, const RunList& rl, u16 cunit, u64 real_size,
                                   std::vector<tree::FileProperties::Extent>& out, u64& blocks_out,
                                   bool& failed_out) {
    const u64 cluster = vol->params().cluster_bytes;
    failed_out = false;
    blocks_out = 0;
    if (cluster == 0 || cluster > 4096 || cunit > 15 || real_size == 0) {
        failed_out = true; /* NTFS never compresses with cluster > 4 KiB;
                            * cunit is 4 on real volumes (64 KiB units) */
        return;
    }
    const u64 unit_blocks = u64(1) << cunit;
    const u64 unit_bytes = unit_blocks * cluster;

    /* segment map: buffer offset -> disk offset (filled per run) */
    struct Seg {
        size_t buf_at;
        u64 disk_at;
        size_t len;
    };
    std::vector<u8> buf;
    std::vector<Seg> segs;
    const auto disk_at = [&](size_t p) -> u64 {
        for (const Seg& s : segs)
            if (p >= s.buf_at && p < s.buf_at + s.len)
                return s.disk_at + (p - s.buf_at);
        return 0;
    };

    const std::vector<DataRun>& runs = rl.runs();

    for (u64 unit_start = 0; unit_start < real_size && !failed_out; unit_start += unit_bytes) {
        /* the unit's stored stream lives in the clusters backing its
         * VCN range; gather [unit_vcn, unit_vcn + unit_blocks) */
        const u64 unit_vcn = unit_start / cluster;
        buf.clear();
        segs.clear();
        u64 filled = 0;
        for (const DataRun& r : runs) {
            if (r.sparse || r.lcn == 0)
                continue;
            const u64 r_start = r.vcn;
            const u64 r_end = r.vcn + r.length;
            if (r_end <= unit_vcn || r_start >= unit_vcn + unit_blocks)
                continue;
            const u64 lo = r_start > unit_vcn ? r_start : unit_vcn;
            const u64 hi = r_end < unit_vcn + unit_blocks ? r_end : unit_vcn + unit_blocks;
            if (hi <= lo)
                continue;
            const size_t at = size_t((lo - unit_vcn) * cluster);
            const size_t len = size_t((hi - lo) * cluster);
            if (buf.size() < at + len)
                buf.resize(at + len, 0);
            if (vol->read_at(r.lcn * cluster + (lo - r_start) * cluster, buf.data() + at,
                             u32(len)) != OK) {
                failed_out = true; /* unreadable envelope: no extents */
                return;
            }
            segs.push_back({at, r.lcn * cluster + (lo - r_start) * cluster, len});
            filled = at + len > filled ? at + len : filled;
        }
        buf.resize(filled);

        /* parse the unit's blocks */
        size_t pos = 0;
        for (u64 b = 0; b < unit_blocks; ++b) {
            const u64 logical = unit_start + b * cluster;
            if (logical >= real_size)
                break;
            const u64 block_len = (real_size - logical) < cluster ? (real_size - logical) : cluster;
            if (pos + 2 > buf.size()) {
                /* no stored header for this block: sparse tail of the unit
                 * (stored stream ends mid-unit) - remaining blocks are
                 * holes, which emit no extents */
                break;
            }
            const u16 hdr = u16(buf[pos]) | (u16(buf[pos + 1]) << 8);
            if (hdr == 0) {
                break; /* hole: the rest of the unit is sparse */
            }
            if (hdr & 0x8000) {
                if (pos + 2 + size_t(cluster) > buf.size()) {
                    failed_out = true;
                    return;
                }
                out.push_back({disk_at(pos + 2), block_len, 0, 0});
                pos += 2 + size_t(cluster);
            } else {
                const size_t plen = size_t(hdr & 0xFFF) + 1;
                if (plen > 4096 || pos + 2 + plen > buf.size()) {
                    failed_out = true;
                    return;
                }
                out.push_back({disk_at(pos + 2), block_len, compress::kLznt1, plen});
                pos += 2 + plen;
            }
            ++blocks_out;
        }
    }
}

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

    /* ---- S-012 pass-2 state (built during the walk) ----
     *
     * Claim map: cluster bitmap over every record's extents as the walk
     * emits it (files claim $DATA runs, dirs claim $I30 runs - the og's
     * FUN_180017190 insertions at :18525, live AND deleted records alike:
     * the skip-deleted config bit engine+0x258 defaults off). The dump pass
     * probes each run's START cluster against this map - NOT against the
     * volume $Bitmap (the og's viability input is pass-1 claim state). */
    std::vector<u8> claim_map;
    const u64 total_clusters = vol->params().total_clusters;
    if (flags.guid_dump_pass && total_clusters > 0)
        claim_map.assign(size_t((total_clusters + 7) / 8), 0);
    auto claim_runlist = [&claim_map](const RunList& rl) {
        if (claim_map.empty())
            return;
        for (const DataRun& dr : rl.runs()) {
            if (dr.sparse || dr.length == 0)
                continue;
            for (u64 c = dr.lcn; c < dr.lcn + dr.length && (c >> 3) < claim_map.size(); ++c)
                claim_map[size_t(c >> 3)] |= u8(1u << (c & 7));
        }
    };
    auto claim_test = [&claim_map](u64 cluster) -> bool {
        if (claim_map.empty() || (cluster >> 3) >= claim_map.size())
            return false;
        return (claim_map[size_t(cluster >> 3)] & (1u << (cluster & 7))) != 0;
    };
    /* live `{GUID}{3808876b-c176-4e48-b7ae-04046e6cc752}`-named dump files
     * (og collection :18584-18625: live only, name length 0x4C, '{' head,
     * '}' at [37], the 38-char magic GUID at [38..75]). */
    std::vector<RunList> guid_dumps;
    /* S-012 size-0 rescue: records that took the rescue path but found no
     * mirror content - the post-walk same-name merge retries them. */
    struct PendingRescue {
        u64 record_no;
        std::u16string name;
    };
    std::vector<PendingRescue> pending_rescue;

    /* $Extend/$UsnJrnl:$J capture (harvest input) */
    RunList usn_runs;
    bool have_usn = false;
    /* $Recycle.Bin $I metadata seen so far (token -> info); $R records
     * whose $I sat at a HIGHER record number wait for the post-walk
     * fixup below (GAP-P1-3: record order is MFT allocation order, not
     * bin-pair order) */
    std::map<std::u16string, RecycleInfo> recycle_info;
    struct PendingRecycleR {
        tree::FileNode* node; /* arena-stable; props mutated post-walk */
        std::u16string token;
    };
    std::vector<PendingRecycleR> pending_recycle_r;

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
        bool data_compressed = false; /* GAP-P1-1: compressed $DATA envelope */
        u16 data_cunit = 0;
        u32 si_flags = 0;             /* N-012: $STANDARD_INFORMATION dos_flags */
        bool recycle_pending = false; /* this record is a $Recycle.Bin $R */
        std::u16string recycle_rnd;   /* its pairing token (extension-stripped) */
        tree::FileTimes si_times{};
        tree::FileProperties* scratch = nullptr;
        std::vector<u8> unnamed_resident;
        if (build_props(vol, attrs, record_no, rh.flags, scratch, attrlist, names, data_runs,
                        has_data, i30_runs, has_i30, si_times, has_si, si_flags, unnamed_resident,
                        data_real_size, data_compressed, data_cunit) != OK) {
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
            /* M-008: the og's chase is DEPTH-1 (FUN_180041df0 iterates the
             * child list, no recursion). single_hop_attrlist=true caps the
             * BFS at one level for benchmark parity; the default keeps the
             * multi-hop superset (the finding's recommendation). */
            const int depth_limit = flags.single_hop_attrlist ? 1 : int(FRAG_DEPTH_LIMIT);
            for (int level = 0; level < depth_limit && !frontier.empty(); ++level) {
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
         * in-use in the bin). $I metadata files never surface.
         * N-079 (M-052 verification, new finding): the og scopes the
         * bin handling to the RECOGNIZED bin roots (the ancestor-chain
         * walk over the four-name _stricmp recognizer,
         * NTFSSearchDLL :43538-43550/:37387-37410). The old landing
         * skipped EVERY $I-named record and remapped every $R-named
         * one regardless of location: user files like "$Image.txt"
         * vanished (skip with no parse gate) and "$R_notes.txt" could
         * mis-pair. Now: the $I skip is PARSE-GATED (a record whose
         * $DATA does not parse as a $I record is ordinary content and
         * is emitted), and the $R remap defers to the post-walk fixup
         * which checks the node's bin ancestry - out-of-bin $R-named
         * files keep their raw name. */
        if (best->name.size() >= 3 && best->name[0] == u'$' &&
            (best->name[1] == u'I' || best->name[1] == u'R')) {
            const std::u16string tok = recycle_token(best->name);
            if (best->name[1] == u'I') {
                if (!tok.empty()) {
                    RecycleInfo ri;
                    if (parse_recycle_info(attrs, ri) == OK) {
                        recycle_info[tok] = ri;
                        ++stats.recycle_info;
                        continue; /* parsed bin metadata, not content */
                    }
                    /* unparsed $I-named file: ordinary content - falls
                     * through to normal emission */
                }
            } else if (!tok.empty()) {
                recycle_pending = true; /* $R: bin-scoped remap post-walk */
                recycle_rnd = tok;
            }
        }
        /* N-040 (diff-to-fix, verified first-hand): the og's pass-1
         * attribute gate is FUN_180034b20 -> FUN_180040ea0 =
         * (name in the og-15 metafile table via FUN_180040f20) AND
         * ($STANDARD_INFORMATION attributes carry BOTH SYSTEM 0x4 AND
         * HIDDEN 0x2). There is NO $FILE_NAME-flags system filter anywhere
         * in the og's pass 1 - the old `dos_flags & 4` gate (header gloss
         * "corpus: dos_flags & 4 -> skip") skipped a different set:
         * $FN-SYSTEM user files (pagefile.sys, hiberfil.sys, bootmgr,
         * desktop.ini-class) that the og KEEPS, while missing the og's
         * SI-condition entirely. The og-15 name table is subsumed by the
         * known-name list above (I-001 documented superset), and real
         * metafiles always carry $SI SYSTEM|HIDDEN - so the name list
         * alone reproduces the og's gate on every real volume, with the
         * clean-SI/torn-SI edge staying inside I-001's registered
         * divergence. The gate is removed; `skip_system_files` and the
         * `system_skipped` counter went with it (dead surface). */

        const bool in_use = (rh.flags & RECORD_IN_USE) != 0;
        const bool is_dir = (rh.flags & RECORD_IS_DIRECTORY) != 0;

        /* ---- S-012 pass-2 state accumulation (og :18525/:18584-18625) ----
         *
         * Every record reaching emission claims its extents into the
         * pass-1 claim map (live AND deleted; the og's skip-deleted config
         * bit defaults off). Live files whose name is the two-GUID dump
         * pattern are collected for the post-walk dump pass. */
        if (flags.guid_dump_pass) {
            claim_runlist(is_dir ? i30_runs : data_runs);
            if (in_use && is_guid_dump_name(best->name)) {
                guid_dumps.push_back(is_dir ? i30_runs : data_runs);
                ++stats.guid_dump_files;
            }
        }

        /* ---- S-012 size-0 rescue gate (og :18493-18499) ----
         *
         * Deleted non-directory size-0 records take the rescue detour:
         * harvest resident $DATA from the $MFTMirr copy (fixup + record#
         * validated, og FUN_18001e120/FUN_18001dd80). Compressed
         * (0x800-flagged) records skip the rescue chain entirely (og
         * FUN_18001d690 entry gate). Records the mirror cannot serve stay
         * pending for the post-walk same-name merge. */
        if (flags.size_zero_rescue && !in_use && !is_dir && data_runs.empty() &&
            (data_real_size ? data_real_size : best->real_size) == 0 &&
            (best->dos_flags & 0x800) == 0) {
            std::vector<u8> mrec;
            if (vol->read_record_mirror(record_no, mrec) == OK && mrec.size() >= 0x38) {
                RecordHeader mrh;
                std::memcpy(&mrh, mrec.data(), sizeof(mrh));
                if (mrh.magic == RECORD_MAGIC_FILE && mrh.base_record == 0 &&
                    u64(mrh.record_number) == record_no &&
                    fixup_apply(mrec.data(), u32(mrec.size()), mrh.update_sequence_offset,
                                mrh.update_sequence_size, sector_bytes) == OK) {
                    std::vector<AttrView> mattrs;
                    if (enum_attributes(mrec.data(), u32(mrec.size()), mattrs) == OK) {
                        for (const AttrView& a : mattrs) {
                            if (a.type != ATTR_TYPE_DATA || a.name_length != 0 || a.nonresident)
                                continue;
                            const u8* d = nullptr;
                            u32 dlen = 0;
                            if (resident_span(a, d, dlen) == OK && dlen > 0) {
                                /* mirror keeps the older bytes with the
                                 * resident stream intact - adopt them
                                 * (has_data: the emission path's resident
                                 * branch must run even when the primary
                                 * record carried no $DATA at all) */
                                unnamed_resident.assign(d, d + dlen);
                                data_real_size = dlen;
                                has_data = true;
                                ++stats.size_zero_rescued;
                                break;
                            }
                        }
                    }
                }
            }
            if (data_real_size == 0)
                pending_rescue.push_back({record_no, best->name});
        }

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

        /* S-001 (diff-to-fix): the og destroy rule. After the viability
         * grading, a DELETED non-directory file whose every probe cluster
         * reads re-allocated is released WITHOUT reaching the result list
         * (FUN_18001acc0 :21249-21262: `0 < total && intact == total ->
         * goto LAB_18001b06f` calls props Release and skips the
         * list-insert at LAB_18001afe6). Folders never take the viability
         * walk og-side (`(flags & 0x10) == 0` gate) and are never
         * destroyed; live files own their clusters. Recovering such a
         * file yields zeros - the og never offers it, so neither do we
         * (default; the flag keeps the old keep-and-grade visibility for
         * honest-recovery scans). */
        if (flags.destroy_fully_overwritten && !in_use && !is_dir &&
            conf == tree::Confidence::Overwritten) {
            ++stats.destroyed_overwritten;
            continue; /* the whole record: no node, no ADS, no props */
        }

        /* distinct parents -> hard links (shared props object). M-007:
         * the counter (and the fan-out below) is the opt-in superset mode
         * only - the og emits ONE node per record (namespace-best name under
         * its parent; FUN_180006db0 :5273 + FUN_1800448b0 :54124-54162). */
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
        if (!flags.single_link_per_record && parents.size() > 1)
            stats.hardlinks += parents.size() - 1;

        tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
        if (props == nullptr)
            return ERR_NO_MEMORY;
        props->record_ref = (record_no) | (u64(rh.sequence) << 48);
        props->fs_kind = u32(FsKindCode::Ntfs); /* corpus factory code */
        props->name = best->name;
        /* N-011a: capture a distinct DOS-namespace $FILE_NAME as the 8.3
         * short name (the og's props short-name field). Only when a
         * SEPARATE DOS name exists - a single WIN32_AND_DOS name is the
         * display name already and carries no extra information. */
        for (const FileNameInfo& n : names) {
            if (n.namespace_type == FILE_NAME_DOS && n.name != best->name) {
                props->short_name = n.name;
                break;
            }
        }
        props->logical_size = data_real_size ? data_real_size : best->real_size;
        props->allocated_size = best->allocated_size;
        props->times = has_si ? si_times : best->times;
        /* N-012 (diff-to-fix): the og serializes attributes from
         * $STANDARD_INFORMATION's dos_flags (FUN_180044f60 on the $SI
         * attribute, NTFSSearchDLL :5351-5368: read-only/hidden/system/
         * archive bits gated per-bit + the 0x400 OFFLINE bit), NOT from
         * $FILE_NAME; the old core used the namespace-best $FILE_NAME's
         * flags masked to 0xFF (dropping OFFLINE and every high bit).
         * $SI absent -> $FILE_NAME fallback (mirrors the time-precedence
         * rule one line above). Mask widened to 0x7FF to preserve the
         * OFFLINE bit (HSM-migrated files) and the 0x100-0x200 family. */
        props->attributes = (has_si ? u16(si_flags & 0x7FF) : u16(best->dos_flags & 0x7FF)) |
                            (is_dir ? u16(0x10) : 0);
        /* $R<rand> recycle remap: DEFERRED ENTIRELY to the post-walk
         * fixup (N-079) - the bin-ancestry check needs the finished
         * tree, and the $I may sit at a higher record number. The node
         * is queued below; the fixup remaps only in-bin nodes. */
        bool recycle_missed = false;
        if (recycle_pending) {
            recycle_pending = false;
            recycle_missed = true; /* every $R goes through the fixup */
        }
        props->flags = u16(in_use ? PFLAG_IN_USE : 0) | u16(is_dir ? PFLAG_IS_DIR : 0);

        /* extents in slice-relative bytes; the tree consumer resolves to
         * absolute via the slice base (orchestrator does this at merge). */
        if (has_data && !data_runs.empty()) {
            /* OQ-SA-1 NTFS half: the +0x64 location anchor — the low u32 of
             * the FIRST $DATA run's start LCN (the og's FUN_1800069e0
             * :5126-5127 <- FUN_1800336f0 :41028 chain; sparse first run ->
             * 0xFFFFFFFF, the og's stored-start read; stamped ahead of the
             * compression branch — the og stamps from the runlist object
             * whenever the datastream attaches, compressed or not). */
            const DataRun& r0 = data_runs.runs().front();
            props->ntfs_origin_lcn = r0.sparse ? 0xFFFFFFFFu : u32(r0.lcn);
            if (data_compressed && !is_dir) {
                /* GAP-P1-1: expand the compression-unit envelope into
                 * codec-tagged per-block extents; raw runlist extents
                 * would surface LZNT1 bytes as file content (silent
                 * corruption). Failure = no extents (zero-filled),
                 * never wrong bytes. */
                bool failed = false;
                u64 blocks = 0;
                expand_compressed_data(vol, data_runs, data_cunit,
                                       data_real_size ? data_real_size : props->logical_size,
                                       props->extents, blocks, failed);
                stats.compressed_blocks += blocks;
                if (failed)
                    ++stats.compressed_failed;
            } else {
                std::vector<RunList::Extent> ex;
                data_runs.extents(ex);
                for (const auto& e : ex) {
                    props->extents.push_back({e.lcn * vol->params().cluster_bytes,
                                              e.clusters * vol->params().cluster_bytes});
                }
            }
        } else if (has_data && !unnamed_resident.empty() && !is_dir) {
            /* resident-only file: the content travels in the props (the
             * writer emits resident_data before any extents) */
            props->resident_data = std::move(unnamed_resident);
        }

        /* emit: M-007 parity mode (default) = ONE node per record - the
         * namespace-best $FILE_NAME from the FULL list (rank across all
         * entries, first-seen wins ties - deterministic), under ITS OWN
         * parent. Opt-out (single_link_per_record=false) = the per-parent
         * fan-out: one node per distinct parent, each with ITS OWN link
         * name (per-link $FILE_NAME); hard links duplicate props (rare) -
         * dedup via record_ref downstream (writer + merge). */
        tree::FullNode* default_root = in_use ? root_active : root_deleted;
        tree::FileNode* first_link_node = nullptr; /* canonical link: ADS host */
        std::vector<u64> emit_parents;
        if (flags.single_link_per_record) {
            emit_parents.push_back(best->parent_record);
        } else {
            emit_parents = parents;
        }
        for (u64 p : emit_parents) {
            /* this link's name: parity mode = the namespace-best name
             * (best, period); fan-out mode = the highest-namespace
             * $FILE_NAME WITH THIS PARENT (per-link names - initialized
             * from this parent's own entries, never the global first
             * name: a lower-ranked parent's links must still find THEIR
             * name, not be blocked by a higher-ranked sibling's name). */
            const FileNameInfo* link_name;
            if (flags.single_link_per_record) {
                link_name = best;
            } else {
                link_name = nullptr;
                for (const FileNameInfo& n : names)
                    if (n.parent_record == p &&
                        (link_name == nullptr || namespace_rank(n.namespace_type) >=
                                                     namespace_rank(link_name->namespace_type)))
                        link_name = &n;
                if (link_name == nullptr)
                    link_name = best; /* parents derive from names; unreachable */
            }
            tree::FileProperties* link_props = props;
            if ((!flags.single_link_per_record && parents.size() > 1) || link_name != best) {
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
            /* N-079: the in-use $R Deleted-state forcing moved to the
             * post-walk fixup (it now rides the bin-ancestry check) */
            n->set_confidence(conf);
            n->set_origin(tree::Origin::Structured);
            if (first_link_node == nullptr)
                first_link_node = n; /* audit row 6c: ADS attaches to the
                                        canonical (first) link, not the last */
            emitted.push_back({n, p, false});
            ++stats.emitted;
        }
        /* GAP-P1-3: an un-mappable $R just emitted under its raw name -
         * defer to the post-walk fixup (the $I can still appear at a
         * higher record number). Nodes are arena-stable and the props
         * object is node-owned after add_file; mutating through the
         * node is safe (no name-keyed index exists: by_record and
         * by_extent only). */
        if (recycle_missed && first_link_node != nullptr) {
            pending_recycle_r.push_back({first_link_node, recycle_rnd});
            recycle_missed = false;
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
                sp->fs_kind = u32(FsKindCode::Ntfs);
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

    /* ---- phase B: $Recycle.Bin $R fixup (GAP-P1-3 + M-052/N-079) ----
     *
     * $R files were emitted under their raw bin name during the walk;
     * every $I has been parsed by now, so the remap can complete. The
     * remap is BIN-SCOPED (N-079): the node's ancestor chain must
     * contain one of the og's four recognized bin roots
     * (recycle::is_bin_root; NTFSSearchDLL :37387-37410 + the
     * ancestor walk :43538-43550) - out-of-bin $R-named user files
     * keep their raw name. Mutating props through the node is safe
     * post-emission: nodes are arena-stable, and the tree's hash
     * indexes key on record_ref/extent (never name). Orphan $R records
     * (damaged/missing $I) keep their raw name - same as the og. */
    for (const auto& pr : pending_recycle_r) {
        auto it = recycle_info.find(pr.token);
        if (it == recycle_info.end())
            continue;
        bool in_bin = false;
        for (tree::FileNode* a = pr.node->parent(); a != nullptr; a = a->parent()) {
            const tree::FileProperties* ap = a->props();
            if (ap != nullptr && recycle::is_bin_root(ap->name)) {
                in_bin = true;
                break;
            }
        }
        if (!in_bin)
            continue; /* out-of-bin $R-named file: ordinary content */
        tree::FileProperties* p = pr.node->props();
        if (p == nullptr)
            continue;
        p->name = it->second.orig_name;
        if (it->second.size != 0)
            p->logical_size = it->second.size;
        p->times.modified = it->second.time;
        pr.node->set_deleted_state(tree::DeletedState::Deleted);
        ++stats.recycle_remapped;
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
        u64 jstream = 0;
        for (const DataRun& dr : usn_runs.runs())
            jstream += dr.length * vol->params().cluster_bytes;
        /* N-039 (diff-to-fix): $J is circular - the newest records (the
         * recovery-relevant deletions) live at the TAIL. When the stream
         * exceeds the mining cap, skip the head and mine the tail window
         * (the old head-mining lost exactly the records a wrapped journal
         * kept current). */
        u64 jskip = 0;
        u64 jtotal = jstream;
        if (jtotal > USN_HARVEST_CAP) {
            jskip = jtotal - USN_HARVEST_CAP;
            jtotal = USN_HARVEST_CAP;
        }
        jbuf.assign(size_t(jtotal), 0);
        u64 got = 0;
        u64 stream_pos = 0;
        for (const DataRun& dr : usn_runs.runs()) {
            if (got >= jtotal)
                break;
            const u64 run_start = stream_pos;
            u64 run_bytes = dr.length * vol->params().cluster_bytes;
            stream_pos += run_bytes;
            if (stream_pos <= jskip)
                continue; /* entirely inside the skipped head */
            const u64 take_from = run_start > jskip ? run_start : jskip;
            u64 take = run_bytes - (take_from - run_start);
            if (take > jtotal - got)
                take = jtotal - got;
            if (dr.sparse || dr.length == 0 || dr.lcn == 0) {
                got += take; /* sparse/unmapped: zeros (pre-assigned) */
                continue;
            }
            if (vol->read_at(dr.lcn * vol->params().cluster_bytes + (take_from - run_start),
                             jbuf.data() + got, u32(take)) != OK) {
                /* damaged run: leave zeros, continue the stream */
                ++stats.usn_damaged_runs;
                got += take;
                continue;
            }
            got += take;
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
            jp2->fs_kind = u32(FsKindCode::Ntfs);
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

    /* ---- phase D: S-012 pass 2 - the GUID-dump walk ----
     *
     * Og FUN_18001a980/FUN_18001acc0: each collected dump file's $DATA is
     * read in <= 0x8000-sector chunks and parsed as MFT-record buffers.
     * STRIDE (corpus-exact, re-verified line-by-line this session):
     * FUN_18001acc0 advances local_c0 by uVar19, which carries THREE
     * distinct values depending on the path taken:
     *   - uVar8  (the raw record size, vt[0x28] with the 0x200 fallback)
     *     after a HOLE (magic/fixup/metafile/base-record failure,
     *     LAB_18001b0cb :21293) AND after a successful emit (assigned at
     *     :21151, never overwritten on the emit path);
     *   - uVar16 (max(0x400, record size)) after a props-build failure
     *     (LAB_18001b0b8 :21169), the (flags&4)/no-name/'$' skip, and the
     *     destroy paths (LAB_18001b06f :21259).
     * The two values coincide whenever the record size is >= 0x400 (the
     * core's boot clamp guarantees exactly that), so a single record-size
     * stride is corpus-exact here; the og's sub-0x400 divergence (uVar16
     * 0x400 vs uVar8 512) is unreachable through the clamped params and
     * stays registered with the M-001 ladder. The pre-correction code
     * advanced holes by one SECTOR (512) - examining positions the og
     * never visits - and used max(sector, 0x400) as the parse bound, which
     * truncates 2 KiB/4 KiB MFT records.
     * Gates: 'FILE' magic, fixup over the record size, og-15-table +
     * hidden+system metafile gate, base-record != 0 -> hole. Survivors of
     * the parse then face the destroy rules:
     *   - (dos_flags & 4) || no name || name[0] == '$'  -> destroyed
     *   - folders bypass everything else (never destroyed)
     *   - size <= 0x3ff                                   -> destroyed
     *   - else per-RUN viability: every non-sparse run's START cluster
     *     probed against the pass-1 claim map; all claimed -> destroyed
     * Dedup: by the pseudo record number - the og's exact arithmetic
     * (lVar18 = local_c0/record_size + chunk byte base, :21150-51; the
     * record number is position-derived and synthetic, exactly like the
     * og's). */
    if (flags.guid_dump_pass && !guid_dumps.empty()) {
        const u64 rs = vol->params().record_size;
        const u64 stride = rs; /* == max(0x400, rs) through the boot clamp */
        const u64 chunk_bytes = u64(0x8000) * sector_bytes;
        std::set<u64> seen;
        std::vector<u8> buf;
        std::vector<u8> drec(stride); /* fixup scratch */
        for (const RunList& rl : guid_dumps) {
            for (const DataRun& dr : rl.runs()) {
                if (dr.sparse || dr.length == 0)
                    continue;
                const u64 run_base = dr.lcn * vol->params().cluster_bytes;
                const u64 run_total = dr.length * vol->params().cluster_bytes;
                u64 off = 0;
                while (off < run_total) {
                    if (cancel && cancel->checkpoint())
                        return ERR_CANCELLED;
                    const u64 take = run_total - off < chunk_bytes ? run_total - off : chunk_bytes;
                    /* + stride slack: the og's 32 MiB buffer gives the last
                     * partial record zero slack to fail against, same shape */
                    buf.assign(size_t(take) + stride, 0);
                    if (vol->read_at(run_base + off, buf.data(), u32(take)) != OK)
                        break;                             /* read failure ends this run's dump */
                    const u64 chunk_base = run_base + off; /* og param_4 (bytes) */
                    u64 p = 0;
                    while (p + 4 <= take) {
                        if (std::memcmp(buf.data() + p, "FILE", 4) != 0) {
                            p += rs; /* hole (og uVar8, LAB_18001b0cb) */
                            continue;
                        }
                        /* candidate: copy the record, validate */
                        const u64 avail = buf.size() - p;
                        if (avail < stride) {
                            p += rs; /* hole */
                            continue;
                        }
                        std::memcpy(drec.data(), buf.data() + p, stride);
                        RecordHeader rh;
                        std::memcpy(&rh, drec.data(), sizeof(rh));
                        if (rh.magic != RECORD_MAGIC_FILE ||
                            fixup_apply(drec.data(), stride, rh.update_sequence_offset,
                                        rh.update_sequence_size, sector_bytes) != OK) {
                            p += rs; /* hole (failed fixup; og uVar8) */
                            continue;
                        }
                        /* og-15 metafile gate on the FIRST $FILE_NAME + $SI */
                        {
                            std::vector<AttrView> gattrs;
                            std::u16string gname;
                            u64 gsi_flags = 0;
                            bool have_name = false, have_si = false;
                            if (enum_attributes(drec.data(), u32(stride), gattrs) == OK) {
                                for (const AttrView& a : gattrs) {
                                    if (!have_name && a.type == ATTR_TYPE_FILE_NAME) {
                                        FileNameInfo fi{};
                                        if (parse_file_name(a, fi) == OK && !fi.name.empty()) {
                                            gname = fi.name;
                                            have_name = true;
                                        }
                                    } else if (!have_si &&
                                               a.type == ATTR_TYPE_STANDARD_INFORMATION) {
                                        StandardInfo gsi;
                                        if (parse_standard_information(a, gsi) == OK) {
                                            gsi_flags = gsi.dos_flags;
                                            have_si = true;
                                        }
                                    }
                                    if (have_name && have_si)
                                        break;
                                }
                            }
                            if (have_name && have_si && og_pass2_metafile_gate(gname, gsi_flags)) {
                                p += rs; /* hole (metafile; og uVar8) */
                                continue;
                            }
                        }
                        if (rh.base_record != 0) {
                            p += rs; /* extension record: hole (og uVar8) */
                            continue;
                        }
                        /* pseudo record# = og lVar18 = in-chunk-offset/
                         * record_size + chunk byte base (:21150-51) */
                        const u64 pseudo = chunk_base + p / rs;
                        if (!seen.insert(pseudo).second) {
                            p += stride; /* already recovered from a dump */
                            continue;
                        }
                        /* full props build from the fixed-up record */
                        std::vector<AttrView> attrs;
                        std::vector<AttrListEntry> attrlist;
                        std::vector<FileNameInfo> names;
                        RunList data_runs2, i30_runs2;
                        bool has_data2 = false, has_i30_2 = false, has_si2 = false;
                        u64 data_real2 = 0;
                        bool data_comp2 = false;
                        u16 data_cunit2 = 0;
                        u32 gsi_flags2 = 0; /* N-012: the dumped record's $SI flags */
                        tree::FileTimes si_times2{};
                        tree::FileProperties* scratch2 = nullptr;
                        std::vector<u8> resident2;
                        if (enum_attributes(drec.data(), u32(stride), attrs) != OK ||
                            build_props(vol, attrs, pseudo, rh.flags, scratch2, attrlist, names,
                                        data_runs2, has_data2, i30_runs2, has_i30_2, si_times2,
                                        has_si2, gsi_flags2, resident2, data_real2, data_comp2,
                                        data_cunit2) != OK ||
                            names.empty()) {
                            p += stride;
                            continue;
                        }
                        const FileNameInfo* best2 = &names[0];
                        for (const FileNameInfo& n : names)
                            if (namespace_rank(n.namespace_type) >=
                                namespace_rank(best2->namespace_type))
                                best2 = &n;
                        const bool is_dir2 = (rh.flags & RECORD_IS_DIRECTORY) != 0;
                        const u64 size2 = data_real2 ? data_real2 : best2->real_size;
                        /* destroy: system-flagged, unnamed, or $-prefixed */
                        if ((best2->dos_flags & 0x04) != 0 || best2->name.empty() ||
                            best2->name[0] == u'$') {
                            ++stats.guid_dump_destroyed;
                            p += stride;
                            continue;
                        }
                        bool destroyed = false;
                        tree::Confidence conf2 = tree::Confidence::PartialRisk;
                        if (!is_dir2) {
                            if (size2 <= 0x3ff) {
                                /* og dump-pass rule: tiny records are noise */
                                destroyed = true;
                            } else if (has_data2 && !data_runs2.empty()) {
                                u64 total = 0, intact = 0;
                                for (const DataRun& dr2 : data_runs2.runs()) {
                                    if (dr2.sparse || dr2.length == 0)
                                        continue;
                                    ++total;
                                    if (claim_test(dr2.lcn))
                                        ++intact;
                                }
                                if (total > 0 && intact == total)
                                    destroyed = true; /* every run START claimed */
                            }
                            /* no runlist object (resident/empty): survives
                             * (og puVar12 == 0 -> straight to the emit) */
                        }
                        if (destroyed) {
                            ++stats.guid_dump_destroyed;
                            p += stride;
                            continue;
                        }
                        /* emit the dump-recovered node */
                        tree::FileProperties* dp = new (std::nothrow) tree::FileProperties();
                        if (dp == nullptr)
                            return ERR_NO_MEMORY;
                        dp->record_ref = pseudo | (u64(rh.sequence) << 48);
                        dp->fs_kind = u32(FsKindCode::Ntfs);
                        dp->name = best2->name;
                        dp->logical_size = size2;
                        dp->allocated_size = best2->allocated_size;
                        dp->times = has_si2 ? si_times2 : best2->times;
                        /* N-012: same $SI-precedence rule as the main walk
                         * (the dumped record's own $SI flags when parsed) */
                        dp->attributes =
                            (has_si2 ? u16(gsi_flags2 & 0x7FF) : u16(best2->dos_flags & 0x7FF)) |
                            (is_dir2 ? u16(0x10) : 0);
                        dp->flags = u16(is_dir2 ? PFLAG_IS_DIR : 0);
                        if (has_data2 && !data_runs2.empty()) {
                            /* OQ-SA-1 NTFS half: same anchor stamp on the
                             * dump-recovered record (the og's GUID-dump twin
                             * FUN_18001c660 :22470-22471 sites the same
                             * encoder outputs). */
                            const DataRun& dr0 = data_runs2.runs().front();
                            dp->ntfs_origin_lcn = dr0.sparse ? 0xFFFFFFFFu : u32(dr0.lcn);
                            std::vector<RunList::Extent> ex;
                            data_runs2.extents(ex);
                            for (const auto& e : ex)
                                dp->extents.push_back({e.lcn * vol->params().cluster_bytes,
                                                       e.clusters * vol->params().cluster_bytes});
                        } else if (!resident2.empty() && !is_dir2) {
                            dp->resident_data = std::move(resident2);
                        }
                        /* parent: resolve against pass-1 records; unresolved
                         * -> lost root (dump records are deleted evidence) */
                        tree::FileNode* host = root_lost;
                        auto pit = by_record.find(best2->parent_record);
                        if (pit != by_record.end() && pit->second != nullptr)
                            host = pit->second;
                        tree::FileNode* dn = tree.add_file(host, dp);
                        /* drop the caller-held ref - the node holds its own
                         * (add_file contract; S-012 leg leak fix: this site
                         * never released, 460+ bytes per scan failing the
                         * ASan lane - reported by the api-session leg in
                         * 591f40c with the exact trace) */
                        dp->release();
                        if (dn == nullptr)
                            return ERR_NO_MEMORY;
                        dn->set_deleted_state(tree::DeletedState::Deleted);
                        dn->set_confidence(is_dir2 ? tree::Confidence::Unknown : conf2);
                        dn->set_origin(tree::Origin::Structured);
                        emitted.push_back({dn, best2->parent_record, true});
                        ++stats.guid_dump_records;
                        ++stats.emitted;
                        p += rs; /* og uVar8: the emit path keeps the record
                                  * stride assigned at :21151 */
                    }
                    off += take;
                }
            }
        }
    }

    /* ---- phase E: S-012 size-0 rescue, post-walk same-name merge ----
     *
     * Og outer-driver relaxed pass (FUN_18001d690 with param_4=1, called
     * after the dump walk over the complete record map): for each still-
     * size-0 rescued record, find a same-name record at a HIGHER record
     * number with non-zero size and adopt its runlist + size (compressed
     * candidates excluded; scan bounded at 5000 - the og's counter gate). */
    if (flags.size_zero_rescue && !pending_rescue.empty()) {
        for (const PendingRescue& pr : pending_rescue) {
            auto nit = by_record.find(pr.record_no);
            if (nit == by_record.end() || nit->second == nullptr)
                continue;
            tree::FileProperties* props = nit->second->props();
            if (props == nullptr || props->logical_size != 0)
                continue; /* mirror rescue already served it */
            /* candidates: emitted nodes from our record# onward */
            u64 examined = 0;
            for (const EmittedNode& e : emitted) {
                if (e.node == nullptr || e.node->props() == nullptr)
                    continue;
                const u64 ern = mft_reference_record(e.node->props()->record_ref);
                if (ern < pr.record_no)
                    continue;
                if (++examined > 5000)
                    break; /* og bound */
                tree::FileProperties* cp = e.node->props();
                if (cp == props || cp->name != pr.name || cp->logical_size == 0)
                    continue;
                if (cp->extents.empty() && cp->resident_data.empty())
                    continue; /* candidate needs a runlist/content to adopt */
                if (!cp->extents.empty()) {
                    props->extents = cp->extents;
                } else if (!cp->resident_data.empty()) {
                    props->resident_data = cp->resident_data;
                } else {
                    continue; /* nothing to adopt */
                }
                props->logical_size = cp->logical_size;
                ++stats.size_zero_rescued;
                break;
            }
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
    u32 cluster_bytes;
    u64 total_clusters;
    u64 record_size;
    tree::ResultTree* tree;
    tree::FullNode* root_lost;
    std::atomic<u64> emitted{0};
    std::atomic<u64> dup_skipped{0};
    std::atomic<bool> aborted{false};
};

/* GAP-S6-4: extract the remnant record's unnamed $DATA into recoverable
 * content - resident streams travel in the props, non-resident runlists
 * become slice-relative extents exactly like the main walk (LCN x
 * cluster_bytes). COMPRESSED streams get NO extents: surfacing raw LZNT1 /
 * XPRESS bytes as file content is the silent-corruption class GAP-P1-1
 * removed from the main walk; remnant reconstruction of compressed units
 * is a registered bounded gap (CORE-GAP-LIST GAP-S6-4 note). Returns true
 * when the node carries recoverable content (extents or resident data). */
static bool remnant_extract_data(const std::vector<AttrView>& attrs, const RemnantCtx& ctx,
                                 tree::FileProperties* props, u64& rem_size) {
    bool have_content = false;
    for (const AttrView& da : attrs) {
        if (da.type != ATTR_TYPE_DATA || da.name_length != 0)
            continue;
        if ((da.flags & ATTR_FLAG_COMPRESSED) != 0) {
            /* honest refusal: compressed remnant content stays
             * MetadataOnly - never raw compressed bytes as content */
            return false;
        }
        if (!da.nonresident && da.value_length != 0) {
            const u8* d = nullptr;
            u32 dlen = 0;
            if (resident_span(da, d, dlen) == OK && dlen > 0) {
                rem_size = dlen;
                props->resident_data.assign(d, d + dlen);
                have_content = true; /* resident content IS recoverable */
            }
            break;
        }
        if (da.nonresident) {
            if (da.actual != 0)
                rem_size = da.actual;
            RunList rl;
            if (parse_data_runs(da, ctx.total_clusters, rl) == OK && rl.total_clusters() > 0) {
                std::vector<RunList::Extent> ex;
                rl.extents(ex);
                for (const auto& e : ex)
                    props->extents.push_back(
                        {e.lcn * ctx.cluster_bytes, e.clusters * ctx.cluster_bytes});
                have_content = !props->extents.empty();
            }
            break;
        }
    }
    return have_content;
}

/* ---- S-022: directory-entry second source (INDX blocks + $INDEX_ROOT) ----
 *
 * The og's "orphaned-record sweep" harvests the $FILE_NAME content embedded
 * in INDX-block / $INDEX_ROOT entries as a second source for files whose
 * MFT records no longer exist (first-hand decode:
 * evidence/decompiled-quotes/s3-ntfs-indx-second-source.md). Gates below
 * mirror the og exactly: entry >= 0x54, flags require DIRECTORY (0x10000000)
 * or ARCHIVE (0x20), HIDDEN|SYSTEM both-set rejected, name-length bound,
 * real size 48-bit, FOLDER attribute from flags bit 0x1C. In-use entries
 * only - slack is NEVER harvested (the og's walk is bounded by the in-use
 * size and stops at the last-entry flag). */

struct IndxEntryInfo {
    u64 file_ref = 0; /* seq<<48 | record# */
    u64 parent_ref = 0;
    u64 real_size = 0;
    tree::FileTimes times{};
    u32 dos_flags = 0;
    bool is_folder = false;
    std::u16string name;
};

/* og FUN_18000e970 (:11025-49): per-entry gate + decode. Returns false when
 * the entry fails a gate (caller skips it). */
static bool indx_entry_decode(const u8* entry, u32 entry_len, IndxEntryInfo& out) {
    if (entry_len < 0x54)
        return false;
    const u32 flags = u32(entry[0x48]) | (u32(entry[0x49]) << 8) | (u32(entry[0x4A]) << 16) |
                      (u32(entry[0x4B]) << 24);
    if ((flags & 0x10000020u) == 0)
        return false; /* require DIRECTORY or ARCHIVE */
    if ((flags & 0x06u) == 0x06u)
        return false; /* HIDDEN|SYSTEM both set: rejected */
    const u32 name_len = entry[0x50];
    if (u64(entry_len) < u64(name_len) * 2 + 0x52)
        return false;
    std::memcpy(&out.file_ref, entry + 0x00, 8);
    std::memcpy(&out.parent_ref, entry + 0x10, 8);
    u64 alloc_sz = 0, real_sz = 0;
    std::memcpy(&alloc_sz, entry + 0x38, 8);
    std::memcpy(&real_sz, entry + 0x40, 8);
    if (real_sz > alloc_sz)
        return false; /* FUN_180045b50 sanity */
    out.real_size = real_sz & 0xFFFFFFFFFFFFULL;
    out.dos_flags = flags;
    out.is_folder = (flags & 0x10000000u) != 0;
    u64 t;
    std::memcpy(&t, entry + 0x18, 8);
    out.times.created = t;
    std::memcpy(&t, entry + 0x20, 8);
    out.times.modified = t;
    std::memcpy(&t, entry + 0x30, 8);
    out.times.accessed = t;
    out.times.changed = 0; /* change time lives at +0x28; the og reads
                              create/modify/access only (:11049 evidence) */
    out.name.resize(name_len);
    for (u32 i = 0; i < name_len; ++i)
        out.name[i] = u16(entry[0x52 + i * 2]) | (u16(entry[0x53 + i * 2]) << 8);
    return !out.name.empty();
}

/* Emit one harvested entry as a Lost node (the og's +0x888 collection is
 * consumed into nodes by the post-walk merger; directories are emitted
 * directly with the "$..."-name filter - FUN_1800445e0). Dedup: against
 * structured results by (parent node, name) and by full file ref. */
static void indx_emit_entry(RemnantCtx& ctx, const IndxEntryInfo& ei, std::set<u64>& emitted_refs,
                            std::atomic<u64>& counter) {
    if (ei.is_folder && !ei.name.empty() && ei.name[0] == u'$')
        return; /* directory $-name filter */
    if (emitted_refs.count(ei.file_ref) != 0)
        return;
    /* structured dedup: the file ref may already exist as a node (live MFT
     * pass or an earlier remnant record) */
    if (ctx.tree->find_by_record(ei.file_ref) != nullptr) {
        emitted_refs.insert(ei.file_ref);
        return;
    }
    /* name-level dedup under the parent directory (same shape the remnant
     * record path uses) */
    const u64 parent_record = ei.parent_ref & 0xFFFFFFFFFFFFULL;
    tree::FileNode* parent_node = ctx.tree->find_by_record(ei.parent_ref);
    if (parent_node != nullptr) {
        for (tree::FileNode* c = parent_node->first_child(); c != nullptr; c = c->next_sibling()) {
            if (c->props() != nullptr && c->props()->name == ei.name) {
                emitted_refs.insert(ei.file_ref);
                return;
            }
        }
    }
    tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
    if (props == nullptr)
        return;
    props->record_ref = ei.file_ref;
    props->fs_kind = u32(FsKindCode::Ntfs);
    props->name = ei.name;
    props->logical_size = ei.real_size;
    props->times = ei.times;
    props->attributes = u16(ei.dos_flags & 0xFF) | (ei.is_folder ? 0x10 : 0);
    props->flags = ei.is_folder ? u16(PFLAG_IS_DIR) : u16(0);
    tree::FileNode* n = ctx.tree->add_file(ctx.root_lost, props);
    props->release();
    if (n) {
        n->set_origin(tree::Origin::Lost);
        n->set_deleted_state(tree::DeletedState::Damaged);
        n->set_confidence(tree::Confidence::MetadataOnly);
        emitted_refs.insert(ei.file_ref);
        counter.fetch_add(1, std::memory_order_relaxed);
        (void)parent_record;
    }
}

/* og FUN_180045510 (:54704-30): INDX-buffer validator. */
static bool indx_block_validate(const u8* blk, u32 size) {
    if (size < 0x28 || std::memcmp(blk, "INDX", 4) != 0)
        return false;
    const u16 usa_off = u16(blk[4]) | (u16(blk[5]) << 8);
    const u16 usa_count = u16(blk[6]) | (u16(blk[7]) << 8);
    const u32 entries_off =
        u32(blk[0x18]) | (u32(blk[0x19]) << 8) | (u32(blk[0x1A]) << 16) | (u32(blk[0x1B]) << 24);
    const u32 in_use =
        u32(blk[0x1C]) | (u32(blk[0x1D]) << 8) | (u32(blk[0x1E]) << 16) | (u32(blk[0x1F]) << 24);
    const u32 alloc =
        u32(blk[0x20]) | (u32(blk[0x21]) << 8) | (u32(blk[0x22]) << 16) | (u32(blk[0x23]) << 24);
    if (size < alloc)
        return false;
    if (size / 512 < u32(usa_count) - 1)
        return false;
    if (alloc < in_use)
        return false;
    if (usa_count >= 0x11)
        return false;
    if (((alloc + entries_off) / 0x200u & 0xFFFFu) != ((u32(usa_count) - 1) & 0xFFFFu))
        return false;
    return usa_off >= 0x28 && usa_off < 0x101;
}

/* og FUN_18000b4c0/FUN_18000e780: validate + fixup an INDX block found in
 * raw space, then walk the IN-USE entries (bounded, never slack; cap 0x1f5
 * entries per block) and emit each passing entry. */
static void harvest_indx_block(RemnantCtx& ctx, const u8* blk, u32 avail,
                               std::set<u64>& emitted_refs) {
    if (!indx_block_validate(blk, avail))
        return;
    const u16 usa_off = u16(blk[4]) | (u16(blk[5]) << 8);
    const u16 usa_count = u16(blk[6]) | (u16(blk[7]) << 8);
    const u32 block_bytes = u32(usa_count - 1) * 512;
    if (u64(block_bytes) > u64(avail))
        return; /* torn block at the buffer edge - the chunk-boundary/
                   read-window loss class the og's carry handles for its own
                   windows; rejected here */
    std::vector<u8> fix(blk, blk + block_bytes);
    /* USA fixup: same multi-sector scheme as MFT records */
    if (fixup_apply(fix.data(), block_bytes, usa_off, usa_count, ctx.sector_bytes) != OK)
        return;
    const u32 entries_off =
        u32(fix[0x18]) | (u32(fix[0x19]) << 8) | (u32(fix[0x1A]) << 16) | (u32(fix[0x1B]) << 24);
    const u32 in_use =
        u32(fix[0x1C]) | (u32(fix[0x1D]) << 8) | (u32(fix[0x1E]) << 16) | (u32(fix[0x1F]) << 24);
    const u64 entries_abs = u64(0x18) + entries_off;
    u64 bound = entries_abs + in_use;
    if (bound > block_bytes)
        bound = block_bytes; /* in-use never exceeds the block (validated) */
    /* chain validation pass (FUN_180045990): entry_len != 0, parent refs
     * consistent with the first entry, real <= alloc per entry, stop at the
     * last-entry flag - a chain violation voids the whole block */
    u64 first_parent = 0;
    bool have_first = false;
    {
        u64 p = entries_abs;
        u32 walked = 0;
        while (p + 0x0E <= bound && walked < 0x1F5) {
            const u16 elen = u16(fix[p + 8]) | (u16(fix[p + 9]) << 8);
            if (elen == 0 || p + elen > bound)
                break;
            const u16 eflags = u16(fix[p + 0x0C]) | (u16(fix[p + 0x0D]) << 8);
            u64 parent = 0;
            std::memcpy(&parent, fix.data() + p + 0x10, 8);
            if (!have_first) {
                first_parent = parent;
                have_first = true;
            } else if (parent != first_parent) {
                return; /* inconsistent chain: not an index block */
            }
            if ((eflags & 2) != 0)
                break; /* last entry */
            p += elen;
            ++walked;
        }
    }
    /* harvest pass (in-use entries only) */
    u64 p = entries_abs;
    u32 walked = 0;
    while (p + 0x0E <= bound && walked < 0x1F5) {
        const u16 elen = u16(fix[p + 8]) | (u16(fix[p + 9]) << 8);
        if (elen == 0 || p + elen > bound)
            break;
        IndxEntryInfo ei;
        if (indx_entry_decode(fix.data() + p, elen, ei))
            indx_emit_entry(ctx, ei, emitted_refs, ctx.emitted);
        const u16 eflags = u16(fix[p + 0x0C]) | (u16(fix[p + 0x0D]) << 8);
        if ((eflags & 2) != 0)
            break; /* last entry: never walk into slack */
        p += elen;
        ++walked;
    }
}

/* og FUN_18000f760 (:11609-83): $INDEX_ROOT entry harvest for remnant
 * directory records - resident entries only (large-index flag 0, in-use
 * size <= 0x320), same entry builder/gates as INDX blocks. */
static void harvest_index_root(RemnantCtx& ctx, const std::vector<AttrView>& attrs,
                               std::set<u64>& emitted_refs) {
    for (const AttrView& a : attrs) {
        if (a.type != ATTR_TYPE_INDEX_ROOT || a.nonresident)
            continue;
        const u8* v = nullptr;
        u32 vlen = 0;
        if (resident_span(a, v, vlen) != OK || vlen < 0x20)
            continue;
        /* INDEX_HEADER at value+0x10 */
        const u32 entries_off =
            u32(v[0x10]) | (u32(v[0x11]) << 8) | (u32(v[0x12]) << 16) | (u32(v[0x13]) << 24);
        const u32 in_use =
            u32(v[0x14]) | (u32(v[0x15]) << 8) | (u32(v[0x16]) << 16) | (u32(v[0x17]) << 24);
        const u32 ih_flags =
            u32(v[0x1C]) | (u32(v[0x1D]) << 8) | (u32(v[0x1E]) << 16) | (u32(v[0x1F]) << 24);
        if ((ih_flags & 0x01) != 0)
            continue; /* large-index: entries live in INDX blocks, not here */
        if (in_use == 0 || in_use > 0x320)
            continue; /* og gate: resident entries <= 0x320 bytes */
        const u64 entries_abs = u64(0x10) + entries_off;
        u64 bound = entries_abs + in_use;
        if (bound > vlen)
            bound = vlen;
        u64 p = entries_abs;
        u32 walked = 0;
        while (p + 0x0E <= bound && walked < 0x1F5) {
            const u16 elen = u16(v[p + 8]) | (u16(v[p + 9]) << 8);
            if (elen == 0 || p + elen > bound)
                break;
            IndxEntryInfo ei;
            if (indx_entry_decode(v + p, elen, ei))
                indx_emit_entry(ctx, ei, emitted_refs, ctx.emitted);
            const u16 eflags = u16(v[p + 0x0C]) | (u16(v[p + 0x0D]) << 8);
            if ((eflags & 2) != 0)
                break;
            p += elen;
            ++walked;
        }
        break; /* first $INDEX_ROOT only (the og's 0x90 lookup) */
    }
}

void scan_remnant_buffer(RemnantCtx& ctx, const u8* buf, u64 buf_len) {
    if (buf_len < ctx.record_size)
        return;
    const u64 scan_end = buf_len - ctx.record_size;
    /* S-022: file-ref dedup across the harvested entries of this buffer
     * (cross-buffer dedup rides on find_by_record + the writer's
     * record_ref merge, the same shape the remnant record path uses) */
    std::set<u64> indx_refs;
    for (u64 off = 0; off <= scan_end; off += 512) {
        if (ctx.aborted.load(std::memory_order_relaxed))
            return;
        const u8* rec = buf + off;
        if (std::memcmp(rec, "FILE", 4) != 0) {
            /* S-022 feeder B: orphaned INDX block probe (the og's raw
             * scanner dispatches 'INDX' clusters to FUN_18000b4c0; the
             * claim-map side of that gate is the structured-dedup here -
             * live directories' blocks yield entries that find_by_record
             * resolves against the MFT-walk nodes) */
            if (std::memcmp(rec, "INDX", 4) == 0)
                harvest_indx_block(ctx, rec, u32(buf_len - off), indx_refs);
            continue;
        }
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
                    /* GAP-S6-4: parse the record's $DATA runlist instead of
                     * downgrading every remnant to MetadataOnly - the og
                     * rebuilds full files from post-format remnants (bench
                     * quick_format: ours 0/10228 vs og 51.5%; 10013 of them
                     * were ERR_TRUNCATED with the content still on disk) */
                    const bool have_content = remnant_extract_data(attrs, ctx, props, rem_size);
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
                    props->fs_kind = u32(FsKindCode::Ntfs);
                    props->logical_size = rem_size;
                    props->times = fi.times;
                    props->attributes = u16(fi.dos_flags & 0xFF);
                    props->flags = 0; /* detached: no in-use knowledge */
                    tree::FileNode* n = ctx.tree->add_file(ctx.root_lost, props);
                    if (n) {
                        n->set_origin(tree::Origin::Lost);
                        n->set_deleted_state(tree::DeletedState::Damaged);
                        /* GAP-S6-4: content-bearing remnants are PartialRisk
                         * (reconstructed runlist, allocation state
                         * unverifiable post-format); empty ones stay
                         * MetadataOnly */
                        n->set_confidence(have_content ? tree::Confidence::PartialRisk
                                                       : tree::Confidence::MetadataOnly);
                        ctx.emitted.fetch_add(1, std::memory_order_relaxed);
                    }
                    props->release();
                    emitted_any = true;
                    break; /* first valid $FILE_NAME is enough for a remnant */
                }
                /* S-022 feeder A: directory records found in raw space
                 * harvest their resident $INDEX_ROOT entries (og
                 * FUN_18000f760, fired from the remnant/run path :9951-52:
                 * large-index==0, in-use <= 0x320, same entry gates) */
                if ((rh.flags & RECORD_IS_DIRECTORY) != 0)
                    harvest_index_root(ctx, attrs, indx_refs);
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
            /* GAP-S6-4 applies to the shifted-header salvage too: attempt
             * the $DATA extraction on the salvaged record (bounds-checked;
             * a runlist torn by the truncation fails cleanly -> the node
             * stays MetadataOnly, exactly the pre-existing shape) */
            u64 rem_size = fi.real_size;
            const bool have_content = remnant_extract_data(sattrs, ctx, props, rem_size);
            props->name = fi.name;
            props->fs_kind = u32(FsKindCode::Ntfs);
            props->logical_size = rem_size;
            props->times = fi.times;
            props->attributes = u16(fi.dos_flags & 0xFF);
            props->flags = 0;
            tree::FileNode* n = ctx.tree->add_file(ctx.root_lost, props);
            if (n) {
                n->set_origin(tree::Origin::Lost);
                n->set_deleted_state(tree::DeletedState::Damaged);
                n->set_confidence(have_content ? tree::Confidence::PartialRisk
                                               : tree::Confidence::MetadataOnly);
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
    /* INDX blocks reach 8 KiB (usa_count < 0x11 -> <= 16 sectors); the
     * inter-block overlap must cover them or blocks straddling a 4 MiB
     * window edge are torn (the og carries an 8-sector window for the
     * same reason - FUN_18000b4c0's split-block reassembly). */
    const u64 overlap = ctx.record_size > 0x2000 ? ctx.record_size : 0x2000;
    while (base < end) {
        if (ctx.aborted.load(std::memory_order_relaxed))
            return ERR_CANCELLED;
        const u64 remain = end - base;
        const u64 want = remain < REMNANT_BLOCK + overlap ? remain : REMNANT_BLOCK + overlap;
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
                      u64 total_clusters, CancelToken* cancel, WalkProgress progress,
                      void* progress_user, tree::ResultTree& tree, tree::FullNode* root_lost,
                      WalkStats& stats, u32 threads) {
    if (slice == nullptr || root_lost == nullptr || record_size < 0x400)
        return ERR_BAD_PARAMETER;

    const u64 total = slice->size();
    RemnantCtx ctx;
    ctx.slice = slice;
    ctx.sector_bytes = sector_bytes;
    ctx.cluster_bytes = cluster_bytes ? cluster_bytes : 4096;
    ctx.total_clusters = total_clusters ? total_clusters : total / ctx.cluster_bytes;
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
