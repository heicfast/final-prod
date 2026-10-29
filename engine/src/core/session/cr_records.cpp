/* cr_records.cpp - session record payload codecs.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/session/cr_records.h"
#include <cstring>
#include <functional>

namespace cr {
namespace session {

namespace {
void put32(std::vector<u8>& o, u32 v) {
    o.push_back(u8(v));
    o.push_back(u8(v >> 8));
    o.push_back(u8(v >> 16));
    o.push_back(u8(v >> 24));
}
void put64(std::vector<u8>& o, u64 v) {
    for (int i = 0; i < 8; ++i)
        o.push_back(u8(v >> (8 * i)));
}
void put16(std::vector<u8>& o, u16 v) {
    o.push_back(u8(v));
    o.push_back(u8(v >> 8));
}
struct Cursor {
    const u8* p;
    size_t left;
    bool fail = false;
    bool take(size_t n) {
        if (n > left) {
            fail = true;
            return false;
        }
        p += n;
        left -= n;
        return true;
    }
    u32 g32() {
        if (left < 4) {
            fail = true;
            return 0;
        }
        u32 v = u32_from_bytes_le(p);
        take(4);
        return v;
    }
    u16 g16() {
        if (left < 2) {
            fail = true;
            return 0;
        }
        u16 v = u16_from_bytes_le(p);
        take(2);
        return v;
    }
    u64 g64() {
        if (left < 8) {
            fail = true;
            return 0;
        }
        u64 v = u64_from_bytes_le(p);
        take(8);
        return v;
    }
    u8 g8() {
        if (left < 1) {
            fail = true;
            return 0;
        }
        u8 v = *p;
        take(1);
        return v;
    }
    bool bytes(u8* dst, size_t n) {
        if (n > left) {
            fail = true;
            return false;
        }
        std::memcpy(dst, p, n);
        take(n);
        return true;
    }
};
} // namespace

u32 serialize_tree(const tree::ResultTree& tree, std::vector<u8>& out, SerializeProgress progress,
                   void* progress_user) {
    out.clear();
    /* depth-first walk; ids in emission order; parents always earlier.
     * The ROOTS THEMSELVES are emitted (marker=1 full nodes, parent
     * 0xFFFFFFFF) so a loaded session reconstructs the Found/Deleted/
     * Lost/Carved structure. Previously roots were skipped and their
     * children collapsed under a synthetic "Restored" root on load,
     * destroying the role structure resume() recovers roots by (G-07).
     * Legacy files (no root records) still load exactly as before. */
    std::vector<const tree::FileNode*> order;
    std::vector<u32> parent_ids;
    std::function<void(const tree::FileNode*, u32)> walk = [&](const tree::FileNode* n, u32 pid) {
        const u32 id = u32(order.size());
        order.push_back(n);
        parent_ids.push_back(pid);
        for (const tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
            walk(c, id);
    };
    for (const tree::FileNode* r : tree.roots())
        walk(r, 0xFFFFFFFFu);

    /* N-060: per-node progress + cancel (the og checks per child during
     * save teardown - rsf-format--scan-session.md flow section; the
     * pre-order walk already knows the total). The terminal tick is
     * (total, total) so a completed save is always observable; a false
     * return aborts with ERR_CANCELLED and the caller discards the
     * partial output (the writer never sees it). */
    const u64 total_nodes = u64(order.size());
    if (progress && !progress(0, total_nodes, progress_user))
        return ERR_CANCELLED;
    /* first pass: measure node_count; header written now, patched after */
    put32(out, u32(order.size()));
    for (size_t i = 0; i < order.size(); ++i) {
        if (progress && !progress(u64(i), total_nodes, progress_user))
            return ERR_CANCELLED;
        const tree::FileNode* n = order[i];
        const tree::FileProperties* pr = n->props();
        /* v2 node flags (stored in the v1 reserved slot; old readers skip
         * the extra fields, old files read back with flags==0):
         *   bit0: extents carry {u32 codec, u64 psize} each
         *   bit1: resident_data blob follows the extents
         *   bit2: short_name follows the name (N-011a, versioned
         *         optional: u16 length + UTF-16 units)
         *   bit3: the S-023 grade byte follows the short name (2-bit
         *         grade | 0x04 overlap flag | 0xF8 reserved-zero)
         *   bit4: M-051 RAW provenance follows the hash flag
         *   bit5: N-016b name_raw follows the short name (u16 byte
         *         length + raw bytes - the og's .rsf keeps original
         *         on-disk name bytes verbatim)
         *   bit6: N-059 per-FS secondary payload follows (u32 id_a,
         *         u32 id_b, u64 generation, u16 zfs_flags, u16 hfs_flags,
         *         u32 hfs_type - the og's Btrfs +0x3B/+0x3F/+0x81, ZFS
         *         +0x5A, HFS+ part2 +0xE/+0x74)
         *   bit7: OQ-SA-1 FAT-family origin cluster follows (u32 - the
         *         og's CFAT +0x3e / CExFat +0x50 first-cluster anchor,
         *         stamped from the dentry independent of chain
         *         resolution; passive datum)
         *   bit11 (2048): S-059 extents carry {u64 logical, u8 hole}
         *         each (sparse/compressed holes keep their logical
         *         position; old files read back logical==0/hole==false
         *         and take the legacy cumulative walk)
         *   bit12 (4096): N-131 the NTFS parent-reference qword (the
         *         $FILE_NAME parent FILE reference masked to the record
         *         number - the og's CNTFSFileProperties +0x40)
         *   bit13 (8192): N-131 the FAT parent-directory cluster (the
         *         containing directory's own origin cluster - the og's
         *         CFATFileProperties +0x42; exFAT has no parent anchor:
         *         the og never stamps its +0x54 for children) */
        u16 nflags = 0;
        if (pr) {
            for (const auto& e : pr->extents)
                if (e.codec != 0 || e.psize != 0) {
                    nflags |= 1;
                    break;
                }
            if (!pr->resident_data.empty())
                nflags |= 2;
            if (!pr->short_name.empty())
                nflags |= 4;
            if (n->grade() != tree::Grade::Average || n->overlap())
                nflags |= 8;
            if (n->origin() == tree::Origin::Carved &&
                (pr->carve_checksum != 0xFFFFFFFFu || !pr->carve_note.empty() ||
                 pr->carve_anchor_b != 0 || pr->carve_flag))
                nflags |= 16; /* M-051: RAW provenance follows */
            if (pr && !pr->name_raw.empty())
                nflags |= 32; /* N-016b: original name bytes follow */
            if (pr && (pr->btrfs_id_a != 0 || pr->btrfs_id_b != 0 || pr->btrfs_generation != 0 ||
                       pr->zfs_secondary_flags != 0 || pr->hfs_secondary_flags != 0 ||
                       pr->hfs_type_id != 0))
                nflags |= 64; /* N-059: per-FS secondary payload follows */
            if (pr && pr->fat_origin_cluster != 0)
                nflags |= 128; /* OQ-SA-1: the FAT-family origin cluster */
            if (pr && pr->ntfs_origin_lcn != 0)
                nflags |= 256; /* OQ-SA-1: the NTFS first-$DATA-run LCN anchor */
            if (pr && pr->ntfs_total_clusters != 0)
                nflags |= 512; /* N-100: the NTFS total-$DATA-clusters twin (+0x60) */
            if (pr && pr->ntfs_parent_ref != 0)
                nflags |= 4096; /* N-131: the NTFS parent-reference anchor (+0x40) */
            if (pr && pr->fat_parent_cluster != 0)
                nflags |= 8192; /* N-131: the FAT parent-directory anchor (+0x42) */
            if (pr && pr->version_group != 0)
                nflags |= 16384; /* M-101: the multi-version family id follows */
            if (pr)
                for (const auto& e : pr->extents)
                    if (e.hole || e.logical != 0) {
                        nflags |= 2048; /* S-059: positioned/hole extents */
                        break;
                    }
        }
        /* S-052: the ROOT's extent-basis id (roots only, nonzero only) -
         * the LDM dynamic-volume basis that survives the file boundary */
        if (n->is_full() && n->basis() != 0)
            nflags |= 1024;
        put32(out, parent_ids[i]);
        put16(out, nflags);
        /* S-048: marker 2 = a child FOLDER (a FullNode with a role under a
         * parent - the carve tree's volume groups + per-ext skeleton
         * folders). Roots stay marker 1 (old readers re-attach them as
         * roots exactly as before); old files never contain marker 2. */
        out.push_back(u8(n->is_full() ? (parent_ids[i] == 0xFFFFFFFFu ? 1 : 2) : 0));
        out.push_back(u8(n->origin()));
        out.push_back(u8(n->deleted_state()));
        out.push_back(u8(n->confidence()));
        if (nflags & 8) /* S-023: grade | overlap */
            out.push_back(u8(n->grade()) | (n->overlap() ? 0x04 : 0));
        put64(out, pr ? pr->record_ref : 0);
        put32(out, pr ? pr->fs_kind : 0);
        put64(out, pr ? pr->logical_size : 0);
        put64(out, pr ? pr->allocated_size : 0);
        put64(out, pr ? pr->times.created : 0);
        put64(out, pr ? pr->times.modified : 0);
        put64(out, pr ? pr->times.changed : 0);
        put64(out, pr ? pr->times.accessed : 0);
        put32(out, pr ? pr->attributes : 0);
        put16(out, pr ? pr->flags : 0);
        const std::u16string& nm = pr ? pr->name : std::u16string();
        put16(out, u16(nm.size()));
        for (u16 ch : nm)
            put16(out, ch);
        if (nflags & 4) { /* N-011a: the 8.3 short name */
            const std::u16string& sn = pr->short_name;
            put16(out, u16(sn.size()));
            for (u16 ch : sn)
                put16(out, ch);
        }
        if (nflags & 32) { /* N-016b: the original on-disk name bytes */
            put16(out, u16(pr->name_raw.size()));
            out.insert(out.end(), pr->name_raw.begin(), pr->name_raw.end());
        }
        const u32 ec = pr ? u32(pr->extents.size()) : 0;
        put32(out, ec);
        for (const auto& e : (pr ? pr->extents : std::vector<tree::FileProperties::Extent>{})) {
            put64(out, e.offset);
            put64(out, e.length);
            if (nflags & 1) {
                put32(out, e.codec);
                put64(out, e.psize);
            }
            if (nflags & 2048) { /* S-059: the logical position + hole mark */
                put64(out, e.logical);
                out.push_back(e.hole ? 1 : 0);
            }
        }
        if (nflags & 2) {
            put32(out, u32(pr->resident_data.size()));
            out.insert(out.end(), pr->resident_data.begin(), pr->resident_data.end());
        }
        put32(out, pr ? pr->carve_format : 0);
        put16(out, u16(pr ? pr->carve_confidence : 0));
        out.push_back(pr && pr->hash_valid ? 1 : 0);
        if (pr && pr->hash_valid)
            out.insert(out.end(), pr->sha128, pr->sha128 + 16);
        /* M-051 (nflags bit4): the RAW provenance block — the og's
         * CRAWFileProperties fields (ifileproperties-part3 :77-90):
         * checksum u32 (0xFFFFFFFF = invalid), note string, tail anchor B
         * u64, flag u8. */
        if (nflags & 16) {
            put32(out, pr->carve_checksum);
            const std::u16string& note = pr->carve_note;
            put16(out, u16(note.size()));
            for (u16 ch : note)
                put16(out, ch);
            put64(out, pr->carve_anchor_b);
            out.push_back(pr->carve_flag ? 1 : 0);
        }
        /* N-059 (nflags bit6): the per-FS secondary payload — the og's
         * Btrfs secondary ids + generation/transid qword, the ZFS
         * secondary flags, the HFS+ mode/flags + type id (the field map
         * in cr_node.h). Fixed 24-byte block, append-only. */
        if (nflags & 64) {
            put32(out, pr->btrfs_id_a);
            put32(out, pr->btrfs_id_b);
            put64(out, pr->btrfs_generation);
            put16(out, pr->zfs_secondary_flags);
            put16(out, pr->hfs_secondary_flags);
            put32(out, pr->hfs_type_id);
        }
        /* OQ-SA-1 (nflags bit7): the FAT-family origin cluster — the og's
         * CFAT +0x3e / CExFat +0x50 first-cluster anchor (part1 :125-127;
         * the SetFirstCluster vt+0x28 contract, stamped from the dentry
         * regardless of chain resolution — FUN_180012e60's unconditional
         * closing stamp :13737-13738). 4-byte block; bit7 fires only when
         * the carrier is nonzero, so pre-OQ-SA-1 sessions stay shape-
         * identical. */
        if (nflags & 128)
            put32(out, pr->fat_origin_cluster);
        /* OQ-SA-1 NTFS half (nflags bit8): the first-$DATA-run LCN anchor —
         * the og's CNTFSFileProperties +0x64 (the vtable+0x30 location-
         * anchor family; producer chain FUN_1800336f0 :41028 ->
         * FUN_1800069e0 :5126 -> FUN_180003020 :1987 -> serializer :2362).
         * 4-byte append-only block; the bit fires only when the carrier is
         * nonzero, so pre-fix sessions stay shape-identical. The sparse-
         * first-run 0xFFFFFFFF value (the og's stored-start read) is
         * nonzero and therefore persists. */
        if (nflags & 256)
            put32(out, pr->ntfs_origin_lcn);
        /* N-100: the +0x60 twin (total $DATA clusters, sparse-anywhere => 0
         * per FUN_1800336f0 :41096-98) — the same 4-byte append-only block
         * shape as the anchor, firing only when nonzero (pre-fix sessions
         * stay shape-identical; the og's rsf record-4 persists the pair in
         * the same order, rsf-format :398). */
        if (nflags & 512)
            put32(out, pr->ntfs_total_clusters);
        /* N-131 (nflags bit12): the NTFS parent-reference qword — the
         * og's CNTFSFileProperties +0x40 (part1 :84; the $FILE_NAME
         * parent reference masked to the 48-bit record number, stamped
         * per-link by the walk). 8-byte append-only block; the bit fires
         * only when the carrier is nonzero, so pre-N-131 sessions stay
         * shape-identical. */
        if (nflags & 4096)
            put64(out, pr->ntfs_parent_ref);
        /* N-131 (nflags bit13): the FAT parent-directory cluster — the
         * og's CFATFileProperties +0x42 (part1 :129; the containing
         * directory node's own +0x3e origin cluster, stamped at every
         * dentry emission). 4-byte append-only block; the bit fires only
         * when the carrier is nonzero (root-level files carry 0 by
         * construction — the og's volume-root node anchors at 0). */
        if (nflags & 8192)
            put32(out, pr->fat_parent_cluster);
        /* M-101 (nflags bit14): the multi-version family id — the og's
         * Versions node axis (UIUX/09:29; the DRWUI coaching string
         * "another version of the file in the same directory"). The
         * family's minimum MFT record number, stamped by the post-scan
         * version pass. 8-byte append-only block; the bit fires only
         * when the carrier is nonzero, so pre-M-101 sessions stay
         * shape-identical. The runtime tag_group is deliberately NOT
         * persisted (the og's session grammar carries no tag field —
         * the branch is runtime grouping per the finding's fix plan). */
        if (nflags & 16384)
            put64(out, pr->version_group);
        if (n->is_full()) {
            const tree::FullNode* f = static_cast<const tree::FullNode*>(n);
            put16(out, u16(f->role.size()));
            for (u16 ch : f->role)
                put16(out, ch);
            put64(out, f->stats().file_count);
            put64(out, f->stats().dir_count);
            put64(out, f->stats().total_logical);
            put64(out, f->stats().deleted_count);
            if (nflags & 1024) /* S-052: the basis id rides the root record */
                put32(out, n->basis());
        }
    }
    if (progress && !progress(total_nodes, total_nodes, progress_user))
        return ERR_CANCELLED; /* terminal tick is cancellable too */
    return OK;
}

u32 deserialize_tree(const u8* data, size_t len, tree::ResultTree& tree, u32* reparented_out,
                     DeserializeProgress progress, void* progress_user) {
    Cursor c{data, len};
    const u32 count = c.g32();
    if (c.fail)
        return 0x50040001;
    /* M-150 (diff-to-fix): the count pre-allocation bomb gate. Each node
     * consumes at least 10 wire bytes by the grammar (parent id 4 +
     * nflags 2 + marker 1 + origin 1 + deleted 1 + confidence 1; the
     * grade byte is optional), so a count beyond (len-4)/10 cannot fit
     * the record - a torn write's garbage count (the M-131 class's
     * read-side complement) or any hostile .crsf. The old code
     * value-initialized by_id(count) straight from the u32: up to
     * 34.4 GB of touched pages at the u32 max - minutes of pagefile
     * thrash on Windows before bad_alloc, an uncatchable OOM-kill on
     * Linux overcommit. The og's record-4 reader allocates node-by-node
     * through per-FS factories (rsf-record4--tree-node-grammar.md:16,26)
     * - its allocation is bounded by the bytes actually present; this
     * gate restores that bound at the count field. */
    constexpr size_t MIN_NODE_BYTES = 10;
    if (u64(count) > (u64(len) - 4) / MIN_NODE_BYTES)
        return 0x50040001; /* corrupt count: the container's resync layer recovers */
    /* N-077 (session 22 baseline repair): the caller-held FileProperties
     * reference leaked on every early break between the per-record
     * allocation and the successful attach (the N-058 bounds-gates and
     * the truncated-tail paths) - LeakSanitizer red on
     * Session.TruncatedTailFailsAfterFullCount and
     * Session.HostileNameAndRoleLengthBombs since b7d4e22. The guard
     * releases the caller's ref on ANY exit from the iteration unless
     * the normal path has already released it (owned=false after the
     * successful handoff). */
    struct PropsRef {
        tree::FileProperties* p;
        bool owned = true;
        explicit PropsRef(tree::FileProperties* pp) : p(pp) {}
        ~PropsRef() {
            if (owned)
                p->release();
        }
        PropsRef(const PropsRef&) = delete;
        PropsRef& operator=(const PropsRef&) = delete;
    };
    std::vector<tree::FileNode*> by_id(count, nullptr);
    std::vector<tree::FileNode*> created;
    tree::FullNode* default_root = nullptr; /* lazy synthetic root */
    /* N-058 (diff-to-fix, landed with M-050 per its fix plan): forward
     * parent references are DEFERRED and resolved after the loop, not
     * silently re-parented to the synthetic root; nodes whose parent
     * never resolves (out of range / that record failed) stay under the
     * synthetic root and are counted in *reparented_out. The count is
     * the surface the finding demanded ("surface a reparented count in
     * the load result"). */
    struct Deferred {
        tree::FileNode* node;
        u32 wanted_parent;
    };
    std::vector<Deferred> deferred;
    u32 reparented = 0;
    for (u32 i = 0; i < count; ++i) {
        const u32 parent_id = c.g32();
        const u16 nflags = c.g16(); /* v1 reserved / v2 node flags */
        const u8 marker = c.g8();
        /* N-058: the og validates marker in {0,1} per-node (rsf-record4);
         * anything else is corruption - fail the record and let the
         * container's CRC/resync layer recover, never silently normalize
         * to a light node. */
        if (marker != 0 && marker != 1 && marker != 2) { /* S-048: +2 (child folder) */
            c.fail = true;
            break;
        }
        const u8 origin = c.g8();
        const u8 deleted = c.g8();
        const u8 confidence = c.g8();
        const u8 grade_byte = (nflags & 8) ? c.g8() : 0; /* S-023 */
        auto* props = new (std::nothrow) tree::FileProperties();
        if (!props)
            return 0x50030009;
        PropsRef props_ref(
            props); /* N-077: releases on every early break
                     * (three convergent landings - session-22's PropsRef [canonical,
                     * first-landed], session-19 DFX19's PropsGuard, DFX17's
                     * PropsGuard - all functionally identical; the PropsRef stands) */

        /* (Convergent repair note: this session's DFX19 leg found the
         * same leak independently during the s19 ASan/LSan suite sweep
         * and filed a PropsGuard fix; the parallel lane's N-077 PropsRef
         * landing is canonical - kept, the duplicate guard removed at
         * the rebase union to avoid a double release on the break
         * paths.) */
        props->record_ref = c.g64();
        props->fs_kind = c.g32();
        if (props->fs_kind == 12)
            props->fs_kind = u32(FsKindCode::Zfs); /* legacy content-map ZFS remap (row 45) */
        props->logical_size = c.g64();
        props->allocated_size = c.g64();
        props->times.created = c.g64();
        props->times.modified = c.g64();
        props->times.changed = c.g64();
        props->times.accessed = c.g64();
        props->attributes = c.g32();
        props->flags = c.g16();
        const u16 name_len = c.g16();
        /* N-058: bounds-check BEFORE allocating (a hostile u16 could
         * claim 128 KiB per node); the og validates name length against
         * the record bounds per-node. */
        if (c.fail || c.left < size_t(name_len) * 2u) {
            c.fail = true;
            break;
        }
        props->name.resize(name_len);
        for (u16 k = 0; k < name_len; ++k)
            props->name[k] = c.g16();
        if (nflags & 4) { /* N-011a: versioned optional short name */
            const u16 sn_len = c.g16();
            props->short_name.resize(sn_len);
            for (u16 k = 0; k < sn_len; ++k)
                props->short_name[k] = c.g16();
        }
        if (nflags & 32) { /* N-016b: original on-disk name bytes */
            const u16 nr = c.g16();
            /* N-058/N-077 class: bounds-check BEFORE the resize (the og
             * validates per-node); the PropsRef guard covers the exit */
            if (c.fail || c.left < size_t(nr)) {
                c.fail = true;
                break;
            }
            props->name_raw.resize(nr);
            if (nr && !c.bytes(reinterpret_cast<u8*>(props->name_raw.data()), nr))
                break;
        }
        const u32 ec = c.g32();
        props->extents.resize(ec);
        for (u32 k = 0; k < ec; ++k) {
            props->extents[k].offset = c.g64();
            props->extents[k].length = c.g64();
            if (nflags & 1) {
                props->extents[k].codec = c.g32();
                props->extents[k].psize = c.g64();
            }
            if (nflags & 2048) { /* S-059: logical position + hole mark */
                props->extents[k].logical = c.g64();
                props->extents[k].hole = c.g8() != 0;
            }
        }
        if (nflags & 2) {
            const u32 rlen = c.g32();
            props->resident_data.resize(rlen);
            if (rlen && !c.bytes(props->resident_data.data(), rlen))
                break;
        }
        props->carve_format = c.g32();
        props->carve_confidence = c.g16();
        const u8 hv = c.g8();
        if (hv) {
            if (!c.bytes(props->sha128, 16))
                break;
            props->hash_valid = true;
        }
        if (nflags & 16) { /* M-051: restore the RAW provenance */
            props->carve_checksum = c.g32();
            const u16 nl = c.g16();
            /* N-078 (NEW-FINDING, filed+fixed this session; the parallel
             * session-22 lane's N-077 covers the leak half): this break
             * was the ONLY corrupt-record exit that left c.fail unset -
             * a truncated carve_note record loaded as OK (silent partial
             * load, violating N-058's "a failed stream is an error"
             * rule). Every other length-bomb site sets c.fail. */
            if (c.left < size_t(nl) * 2u) {
                c.fail = true;
                break;
            }
            props->carve_note.resize(nl);
            for (u16 k = 0; k < nl; ++k)
                props->carve_note[k] = c.g16();
            props->carve_anchor_b = c.g64();
            props->carve_flag = c.g8() != 0;
        }
        if (nflags & 64) { /* N-059: the per-FS secondary payload */
            /* fixed 24-byte block; a truncated tail is the N-058
             * corrupt-record error (c.fail set by the exhausted reads) */
            props->btrfs_id_a = c.g32();
            props->btrfs_id_b = c.g32();
            props->btrfs_generation = c.g64();
            props->zfs_secondary_flags = c.g16();
            props->hfs_secondary_flags = c.g16();
            props->hfs_type_id = c.g32();
            if (c.fail)
                break;
        }
        if (nflags & 128) { /* OQ-SA-1: the FAT-family origin cluster */
            props->fat_origin_cluster = c.g32();
            if (c.fail)
                break;
        }
        if (nflags & 256) { /* OQ-SA-1: the NTFS origin LCN anchor */
            props->ntfs_origin_lcn = c.g32();
            if (c.fail)
                break;
        }
        if (nflags & 512) { /* N-100: the NTFS total-clusters twin (+0x60) */
            props->ntfs_total_clusters = c.g32();
            if (c.fail)
                break;
        }
        if (nflags & 4096) { /* N-131: the NTFS parent-reference anchor */
            props->ntfs_parent_ref = c.g64();
            if (c.fail)
                break;
        }
        if (nflags & 8192) { /* N-131: the FAT parent-directory anchor */
            props->fat_parent_cluster = c.g32();
            if (c.fail)
                break;
        }
        if (nflags & 16384) { /* M-101: the multi-version family id */
            props->version_group = c.g64();
            if (c.fail)
                break;
        }
        tree::FileNode* n = nullptr;
        if (marker == 2) {
            /* S-048: a child FOLDER - a FullNode with a role under its
             * parent (parents always precede children in the writer's
             * depth-first walk). A marker-2 record without a resolvable
             * parent is structural corruption: c.fail (the N-058 class -
             * never a silent re-parent, never a root promotion). */
            const u16 role_len = c.g16();
            if (c.fail || c.left < size_t(role_len) * 2u) {
                c.fail = true;
                break;
            }
            std::u16string role(role_len, 0);
            for (u16 k = 0; k < role_len; ++k)
                role[k] = c.g16();
            tree::FullNode::Stats st{};
            st.file_count = c.g64();
            st.dir_count = c.g64();
            st.total_logical = c.g64();
            st.deleted_count = c.g64();
            tree::FileNode* parent = nullptr;
            if (parent_id != 0xFFFFFFFFu && parent_id < count)
                parent = by_id[parent_id];
            if (parent == nullptr) {
                c.fail = true; /* a folder without its parent: corrupt */
                break;
            }
            tree::FullNode* folder = tree.add_folder(parent, role);
            if (folder) {
                folder->set_stats(st);
                folder->set_props(props);
                if (folder->props() == nullptr) {
                    c.fail = true;
                    break;
                }
            }
            n = folder;
            if (n == nullptr)
                break; /* allocation failure: the N-058 error path */
        } else if (marker == 1) {
            /* full node: role + stats; re-attach as a root-level virtual */
            const u16 role_len = c.g16();
            /* N-058: same class as the name-length bomb (the og bounds-
             * checks per-node); checked BEFORE the allocation. */
            if (c.fail || c.left < size_t(role_len) * 2u) {
                c.fail = true;
                break;
            }
            std::u16string role(role_len, 0);
            for (u16 k = 0; k < role_len; ++k)
                role[k] = c.g16();
            /* M-047: the persisted recursive accumulator is STORED, not
             * discarded (the og's reader "stores the 40-byte image
             * directly at +0x98", rsf-format :318-320) - reloaded roots
             * carry their pre-save totals by construction. */
            tree::FullNode::Stats st{};
            st.file_count = c.g64();
            st.dir_count = c.g64();
            st.total_logical = c.g64();
            st.deleted_count = c.g64();
            const u32 root_basis = (nflags & 1024) ? c.g32() : 0u; /* S-052 */
            tree::FullNode* full = tree.add_root(role);
            if (full) {
                full->set_stats(st);
                full->set_props(props);
                if (root_basis != 0)
                    tree.set_root_basis(full, root_basis);
                n = full;
            }
        } else {
            tree::FileNode* parent = nullptr;
            if (parent_id != 0xFFFFFFFFu && parent_id < count)
                parent = by_id[parent_id];
            if (parent == nullptr && parent_id != 0xFFFFFFFFu) {
                /* N-058: a forward reference (pid > i) or a reference to
                 * a record that failed - DEFER it; resolve after the
                 * loop. Never silently re-parent at emit time. */
                if (!default_root)
                    default_root = tree.add_root(u"Restored");
                if (default_root) {
                    n = tree.add_file(default_root, props);
                    if (n)
                        deferred.push_back({n, parent_id});
                }
            } else if (parent == nullptr) {
                /* parent 0xFFFFFFFF on a LIGHT node: legacy top-level
                 * file (the writer never emits this today; old files
                 * did) - the designed synthetic-root path */
                if (!default_root)
                    default_root = tree.add_root(u"Restored");
                if (default_root)
                    n = tree.add_file(default_root, props);
            } else {
                n = tree.add_file(parent, props);
            }
        }
        props->release();
        props_ref.owned = false; /* the tree owns its reference now */

        if (!n) {
            break;
        }
        n->set_origin(tree::Origin(origin));
        n->set_deleted_state(tree::DeletedState(deleted));
        n->set_confidence(tree::Confidence(confidence));
        if (nflags & 8) { /* S-023: restore grade + overlap (direct field
                           * restore - round-trip, not a rule application) */
            n->set_grade_raw(tree::Grade(grade_byte & 0x03));
            if (grade_byte & 0x04)
                n->set_overlap();
        }
        by_id[i] = n;
        created.push_back(n);
        if (c.fail)
            break;
        /* W1b (R-54): the per-node load hook - the N-060 save mirror.
         * The og's restore checks IsCancelled after EVERY child and
         * tears down (rsf-format--scan-session.md flow section); false
         * here aborts with ERR_CANCELLED - the tree is PARTIAL and the
         * caller discards it (a torn load never half-claims state). */
        if (progress && !progress(u64(i) + 1, u64(count), progress_user))
            return ERR_CANCELLED;
    }
    /* N-058: resolve the deferred forward references now that every
     * record had its chance to load; the ones that never resolve stay
     * under the synthetic root and are counted (never silent). */
    for (const Deferred& d : deferred) {
        if (d.wanted_parent < count && by_id[d.wanted_parent] != nullptr &&
            by_id[d.wanted_parent] != d.node)
            tree.reparent(d.node, by_id[d.wanted_parent]);
        else
            ++reparented;
    }
    if (reparented_out != nullptr)
        *reparented_out = reparented;
    /* M-047: intact loads TRUST the persisted root stats (the og's model
     * - the image is the pre-save recursive truth). Only a PARTIAL load
     * (some nodes missing) recomputes: the stale image would overcount a
     * tree that lost nodes to corruption. The recompute is recursive -
     * the same accumulator semantics the writer used.
     * N-058: a stream that FAILED is an error even when every expected
     * node was created (a degraded final node must not load as OK). */
    if (c.fail && count > 0) {
        tree.refresh_stats();
        return u32(0x50040001);
    }
    return u32(OK);
}

/* ---- S-052: the LDM layouts record codec ---- */

u32 serialize_ldm_layouts(const std::vector<table::LdmVolumeLayout>& layouts,
                          std::vector<u8>& out) {
    out.clear();
    put32(out, u32(layouts.size()));
    for (const auto& lay : layouts) {
        out.push_back(u8(lay.kind));
        put64(out, lay.vol_obj_id);
        put64(out, lay.total_sectors);
        put64(out, lay.chunk_sectors);
        put32(out, lay.columns);
        put64(out, lay.missing_members);
        out.push_back(lay.complete ? 1 : 0);
        put16(out, u16(lay.name.size()));
        for (char ch : lay.name)
            out.push_back(u8(ch));
        put32(out, u32(lay.copies.size()));
        for (const auto& copy : lay.copies) {
            put32(out, u32(copy.size()));
            for (const auto& m : copy) {
                put64(out, m.part_obj_id);
                put64(out, m.disk_obj_id);
                out.insert(out.end(), m.disk_guid.b, m.disk_guid.b + 16);
                put64(out, m.disk_lba);
                put64(out, m.part_start);
                put64(out, m.sectors);
                put64(out, m.vol_offset);
                put32(out, m.index);
                out.push_back(m.disk_present ? 1 : 0);
            }
        }
    }
    return OK;
}

u32 deserialize_ldm_layouts(const u8* data, size_t len, std::vector<table::LdmVolumeLayout>& out) {
    out.clear();
    Cursor c{data, len};
    const u32 count = c.g32();
    if (c.fail)
        return ERR_SESSION_FORMAT;
    if (count > 4096u) /* the same bomb class as the tree's node count */
        return ERR_SESSION_FORMAT;
    out.reserve(count);
    for (u32 i = 0; i < count; ++i) {
        table::LdmVolumeLayout lay;
        lay.kind = table::LdmVolumeKind(c.g8());
        lay.vol_obj_id = c.g64();
        lay.total_sectors = c.g64();
        lay.chunk_sectors = c.g64();
        lay.columns = c.g32();
        lay.missing_members = c.g64();
        lay.complete = c.g8() != 0;
        const u16 name_len = c.g16();
        if (c.fail || c.left < size_t(name_len)) {
            c.fail = true;
            break;
        }
        lay.name.assign(reinterpret_cast<const char*>(c.p), size_t(name_len));
        c.p += name_len;
        c.left -= name_len;
        const u32 copies = c.g32();
        if (c.fail || copies > 64u) {
            c.fail = true;
            break;
        }
        lay.copies.resize(copies);
        for (u32 k = 0; k < copies; ++k) {
            const u32 members = c.g32();
            if (c.fail || members > 4096u) {
                c.fail = true;
                break;
            }
            lay.copies[k].resize(members);
            for (u32 j = 0; j < members; ++j) {
                table::LdmMember& m = lay.copies[k][j];
                m.part_obj_id = c.g64();
                m.disk_obj_id = c.g64();
                if (c.left < 16) {
                    c.fail = true;
                    break;
                }
                std::memcpy(m.disk_guid.b, c.p, 16);
                c.p += 16;
                c.left -= 16;
                m.disk_lba = c.g64();
                m.part_start = c.g64();
                m.sectors = c.g64();
                m.vol_offset = c.g64();
                m.index = c.g32();
                m.disk_present = c.g8() != 0;
            }
            if (c.fail)
                break;
        }
        if (c.fail)
            break;
        out.push_back(std::move(lay));
    }
    return c.fail ? ERR_SESSION_FORMAT : OK;
}

u32 serialize_error_map(const ReadErrorMap& map, std::vector<u8>& out) {
    out.clear();
    map.serialize(out);
    return OK;
}

u32 deserialize_error_map(const u8* data, size_t len, ReadErrorMap& map) {
    u32 consumed = 0;
    map = ReadErrorMap::deserialize(data, len, &consumed);
    return consumed <= len ? u32(OK) : u32(0x50040001);
}

u32 serialize_free_map(const storage::FreeMap& fm, std::vector<u8>& out) {
    out.clear();
    if (!fm.valid())
        return ERR_BAD_PARAMETER;
    put32(out, fm.cluster_bytes);
    put32(out, 0); /* reserved */
    put64(out, fm.base_byte);
    put64(out, fm.cluster_count);
    put32(out, u32(fm.bits.size()));
    out.insert(out.end(), fm.bits.begin(), fm.bits.end());
    return OK;
}

u32 deserialize_free_map(const u8* data, size_t len, storage::FreeMap& fm) {
    fm.clear();
    if (data == nullptr || len < 28)
        return ERR_SESSION_FORMAT;
    Cursor c(data, len);
    const u32 cluster_bytes = c.g32();
    (void)c.g32(); /* reserved */
    const u64 base_byte = c.g64();
    const u64 cluster_count = c.g64();
    const u32 bits_len = c.g32();
    if (c.fail || cluster_bytes == 0 || cluster_count == 0)
        return ERR_SESSION_FORMAT;
    if (u64(bits_len) != (cluster_count + 7) / 8)
        return ERR_SESSION_FORMAT; /* the bits must cover the declared clusters */
    if (size_t(bits_len) > c.left)
        return ERR_SESSION_FORMAT;
    fm.cluster_bytes = cluster_bytes;
    fm.base_byte = base_byte;
    fm.cluster_count = cluster_count;
    fm.bits.assign(c.p, c.p + bits_len);
    return OK;
}

u32 serialize_volume_info(const VolumeInfo& vi, std::vector<u8>& out) {
    out.clear();
    put32(out, vi.fs_kind);
    put64(out, vi.slice_offset);
    put64(out, vi.slice_bytes);
    put32(out, vi.logical_sector);
    put32(out, vi.physical_sector);
    out.insert(out.end(), vi.label, vi.label + 32);
    put64(out, vi.scanned_bytes);
    put64(out, vi.elapsed_ms);
    put32(out, vi.flags);
    /* identity card (SESS-2 v2 tail) */
    out.insert(out.end(), vi.model, vi.model + 48);
    out.insert(out.end(), vi.serial, vi.serial + 32);
    put32(out, vi.boot_hash);
    /* S-030 v3 tail: the image file path (empty for disk/volume targets;
     * old readers stop before it - the record stays loadable) */
    out.insert(out.end(), vi.image, vi.image + 256);
    /* N-066 v4 tail: the og record-3 axes + the 512-byte boot snapshot
     * (empty strings / zeroed snapshot when the source had none). */
    out.insert(out.end(), vi.firmware, vi.firmware + 32);
    out.insert(out.end(), vi.wwn, vi.wwn + 32);
    out.insert(out.end(), vi.interface_desc, vi.interface_desc + 32);
    out.insert(out.end(), vi.boot_snapshot, vi.boot_snapshot + 512);
    /* W1a v5 tail: the embedded save time (the og's record-2 SYSTEMTIME,
     * rsf-format section 5.2 +0x6C); old readers stop before it and old
     * files simply lack it (the deserializer len-gates the read). */
    put64(out, vi.save_unix);
    /* N-090 v6 tail: the complete label at the og's volumeName[100]
     * width - label[32] above keeps the legacy prefix view; old readers
     * stop before this tail (the same len-gate pattern as v2..v5). */
    out.insert(out.end(), vi.label_full, vi.label_full + 100);
    return OK;
}

u32 deserialize_volume_info(const u8* data, size_t len, VolumeInfo& vi) {
    Cursor c{data, len};
    vi.fs_kind = c.g32();
    vi.slice_offset = c.g64();
    vi.slice_bytes = c.g64();
    vi.logical_sector = c.g32();
    vi.physical_sector = c.g32();
    if (!c.bytes(reinterpret_cast<u8*>(vi.label), 32))
        return 0x50040001;
    vi.scanned_bytes = c.g64();
    vi.elapsed_ms = c.g64();
    vi.flags = c.g32();
    /* v2 identity tail: absent on v1 records - zeros mean "unverifiable",
     * never "verified" (the resume side skips comparison on zero hash).
     * v1 prefix is 80 bytes (4+8+8+4+4+32+8+8+4); the tail adds 48+32+4.
     * S-030 v3 tail adds the 256-byte image path (zeros when absent). */
    if (!c.fail && len >= 80 + 84) {
        if (!c.bytes(reinterpret_cast<u8*>(vi.model), 48))
            return 0x50040001;
        if (!c.bytes(reinterpret_cast<u8*>(vi.serial), 32))
            return 0x50040001;
        vi.boot_hash = c.g32();
        if (len >= 80 + 84 + 256) {
            if (!c.bytes(reinterpret_cast<u8*>(vi.image), 256))
                return 0x50040001;
            vi.image[255] = 0; /* never trust a torn NUL */
        }
        /* N-066 v4 tail: absent on v1/v2/v3 records - zeros mean the
         * axes were never captured (the same skip-on-zero contract as
         * the v2 identity tail; the boot snapshot stays all-zero and
         * the mismatch diagnostics degrade to the hash compare). */
        if (len >= 80 + 84 + 256 + 32 + 32 + 32 + 512) {
            if (!c.bytes(reinterpret_cast<u8*>(vi.firmware), 32))
                return 0x50040001;
            if (!c.bytes(reinterpret_cast<u8*>(vi.wwn), 32))
                return 0x50040001;
            if (!c.bytes(reinterpret_cast<u8*>(vi.interface_desc), 32))
                return 0x50040001;
            if (!c.bytes(vi.boot_snapshot, 512))
                return 0x50040001;
            vi.firmware[31] = 0;
            vi.wwn[31] = 0;
            vi.interface_desc[31] = 0;
            /* W1a v5 tail: absent on v1..v4 records - 0 means "not
             * recorded" (the list surfaces it honestly; the file's mtime
             * is the caller's fallback display, never a silent guess). */
            if (len >= 80 + 84 + 256 + 32 + 32 + 32 + 512 + 8)
                vi.save_unix = c.g64();
            /* N-090 v6 tail: the complete label (the og's
             * volumeName[100]); absent on v1..v5 records - zeros mean
             * "the legacy 31-char card is the whole truth" (the
             * surface joins to label[] alone, exactly pre-v6). */
            if (len >= 80 + 84 + 256 + 32 + 32 + 32 + 512 + 8 + 100) {
                if (!c.bytes(reinterpret_cast<u8*>(vi.label_full), 100))
                    return 0x50040001;
                vi.label_full[99] = 0; /* never trust a torn NUL */
            }
        }
    }
    return c.fail ? u32(0x50040001) : u32(OK);
}

u32 serialize_session_state(const SessionState& st, std::vector<u8>& out) {
    out.clear();
    put32(out, st.stage_done_mask);
    put64(out, st.files_found);
    put64(out, st.bytes_scanned);
    put32(out, st.error_map_ranges);
    /* v2 appended counters (G-07/G-14): a reloaded session reports the
     * full found-set; v1 readers stop before these and v1 files simply
     * lack them (deserializer len-gates the reads) */
    put64(out, st.deleted_found);
    put64(out, st.carved_found);
    /* v3 (N-167): the found-size aggregate - the og's "Files Found: N
     * (X GB)" headline truth, restored on load */
    put64(out, st.bytes_found);
    return OK;
}

u32 deserialize_session_state(const u8* data, size_t len, SessionState& st) {
    Cursor c{data, len};
    st.stage_done_mask = c.g32();
    st.files_found = c.g64();
    st.bytes_scanned = c.g64();
    st.error_map_ranges = c.g32();
    /* v2 appended counters - present only in 40-byte records; the 24-byte
     * v1 layout (old sessions) leaves them zeroed */
    if (!c.fail && len >= 32)
        st.deleted_found = c.g64();
    if (!c.fail && len >= 40)
        st.carved_found = c.g64();
    /* v3 (N-167): the found-size aggregate - present only in 48-byte
     * records; v1/v2 sessions leave it zeroed (the tree re-stamps it on
     * the first count refresh after resume) */
    if (!c.fail && len >= 48)
        st.bytes_found = c.g64();
    return c.fail ? u32(0x50040001) : u32(OK);
}

u32 serialize_partition_context(const PartitionContext& pc, std::vector<u8>& out) {
    out.clear();
    put64(out, pc.start_lba);
    put64(out, pc.sector_count);
    put32(out, pc.fs_kind);
    put32(out, pc.score);
    out.push_back(pc.kind);
    /* v2 tail (M-042): the og's per-record geometry payload + flags. The
     * record length itself carries the version (25 = v1, 41 = v2) — the
     * same length-detection the SessionState v2 tail uses. */
    put32(out, pc.bytes_per_sector);
    put32(out, pc.sectors_per_cluster);
    put32(out, pc.hidden_sectors);
    put32(out, pc.flags);
    /* v3 tail (M-137): the found-partition row context + the label
     * (61 bytes = v3; 41 = v2; 25 = v1 — the same length detection). */
    put32(out, pc.disk_layout);
    put32(out, pc.disk_flags);
    for (int k = 0; k < 12; ++k)
        out.push_back(u8(pc.volume_label[k]));
    /* v4 tail (N-203): the data-area extent (77 bytes = v4; the og's
     * CSearchedPartitionInfo replays with the full extent identity —
     * EndSec anchored at the data-area boundary). */
    put64(out, pc.data_start_lba);
    put64(out, pc.data_sector_count);
    return OK;
}

u32 deserialize_partition_context(const u8* data, size_t len, PartitionContext& pc) {
    Cursor c{data, len};
    pc.start_lba = c.g64();
    pc.sector_count = c.g64();
    pc.fs_kind = c.g32();
    pc.score = c.g32();
    pc.kind = c.g8();
    /* v1 sessions (25 bytes) keep the v2 extras zeroed; v2 (41) carries
     * the geometry payload + flags (M-042). */
    if (!c.fail && len >= 41) {
        pc.bytes_per_sector = c.g32();
        pc.sectors_per_cluster = c.g32();
        pc.hidden_sectors = c.g32();
        pc.flags = c.g32();
    }
    /* v3 (61 bytes, M-137): the presentation context + the boot label;
     * v1/v2 payloads keep the v3 extras zeroed (the append-only wire). */
    if (!c.fail && len >= 61) {
        pc.disk_layout = c.g32();
        pc.disk_flags = c.g32();
        for (int k = 0; k < 12; ++k)
            pc.volume_label[k] = char(c.g8());
        pc.volume_label[11] = 0; /* NUL-floor the wire slot */
    }
    /* v4 (77 bytes, N-203): the data-area extent; v1..v3 payloads keep
     * the v4 extras zeroed — a zero extent routes the resume into the
     * revalidate scratch copy-back (the first-resume repair). */
    if (!c.fail && len >= 77) {
        pc.data_start_lba = c.g64();
        pc.data_sector_count = c.g64();
    }
    return c.fail ? u32(0x50040001) : u32(OK);
}

/* ---- M-042: whole-disk layout snapshot (REC_DISK_LAYOUT) ---- */

u32 serialize_disk_layout(const std::vector<DiskLayoutEntry>& entries, std::vector<u8>& out) {
    out.clear();
    if (entries.size() > 0xFFFF)
        return ERR_BAD_PARAMETER; /* a partition table never has 65k entries;
                                     a hostile record never round-trips */
    put32(out, u32(entries.size()));
    for (const auto& e : entries) {
        put64(out, e.start_lba);
        put64(out, e.sector_count);
        put32(out, e.mbr_type);
    }
    return OK;
}

u32 deserialize_disk_layout(const u8* data, size_t len, std::vector<DiskLayoutEntry>& out) {
    out.clear();
    Cursor c{data, len};
    const u32 count = c.g32();
    if (c.fail || count > 0xFFFF)
        return u32(0x50040001);
    out.reserve(count);
    for (u32 i = 0; i < count; i++) {
        DiskLayoutEntry e;
        e.start_lba = c.g64();
        e.sector_count = c.g64();
        e.mbr_type = c.g32();
        out.push_back(e);
    }
    return c.fail ? u32(0x50040001) : u32(OK);
}

/* ---- S-023 assistant statistics (REC_ASSISTANT_STATS) ---- */

u32 serialize_assistant_stats(const tree::grading::AssistantStats& st, std::vector<u8>& out) {
    out.clear();
    put64(out, st.total_file_count);
    put64(out, st.raw_count);
    put64(out, st.lost_partition_exist_file_count);
    put64(out, st.lost_partition_delete_file_count);
    put64(out, st.quick_scan_exist_file_count);
    put64(out, st.quick_scan_delete_file_count);
    put64(out, st.deep_scan_file_count);
    put64(out, st.quick_scan_delete_extend);
    for (size_t i = 0; i < 8; ++i)
        put64(out, st.category_counts[i]);
    put64(out, st.matched);
    put64(out, st.total);
    put64(out, st.graded_count);
    put32(out, st.small_result ? 1u : 0u);
    return OK;
}

u32 serialize_carve_tally(const CarveTallyRecord& t, std::vector<u8>& out) {
    out.clear();
    const u32 n = t.slots < 512 ? t.slots : 512;
    put32(out, n);
    put64(out, t.overflow);
    for (u32 i = 0; i < n; ++i)
        put64(out, t.counts[i]);
    return OK;
}

u32 serialize_scan_filter(const std::vector<std::string>& prefixes, std::vector<u8>& out) {
    /* M-073: u32 count, then per prefix u32 byte length + the raw UTF-8
     * (the config's own text; normalization lives in the prune). A
     * prefix longer than u32 bytes is refused, not truncated (a filter
     * that outgrows 4 GiB is a config error, not a payload). */
    out.clear();
    if (prefixes.size() > 0xFFFFFFFFu)
        return u32(0x50040004); /* ERR_BAD_PARAMETER family (session codes) */
    put32(out, u32(prefixes.size()));
    for (const auto& p : prefixes) {
        if (p.size() > 0xFFFFFFFFu)
            return u32(0x50040004);
        put32(out, u32(p.size()));
        out.insert(out.end(), p.begin(), p.end());
    }
    return OK;
}

u32 deserialize_scan_filter(const u8* data, size_t len, std::vector<std::string>& prefixes) {
    prefixes.clear();
    Cursor c{data, len};
    const u32 n = c.g32();
    if (c.fail)
        return u32(0x50040001);
    for (u32 i = 0; i < n && !c.fail; ++i) {
        const u32 plen = c.g32();
        if (c.fail || !c.take(plen)) {
            c.fail = true;
            break;
        }
        prefixes.emplace_back(reinterpret_cast<const char*>(c.p) - plen, plen);
    }
    if (c.fail) {
        /* honest degrade: a torn record yields no filter (the session
         * loads; the host config governs) */
        prefixes.clear();
        return u32(0x50040001);
    }
    return u32(OK);
}

u32 deserialize_carve_tally(const u8* data, size_t len, CarveTallyRecord& t) {
    Cursor c{data, len};
    t.slots = 0;
    t.overflow = 0;
    for (u32 i = 0; i < 512; ++i)
        t.counts[i] = 0;
    const u32 n = c.g32();
    t.overflow = c.g64();
    if (c.fail)
        return u32(0x50040001);
    /* the count may exceed this build's 512-row view (a newer writer):
     * read every row so the cursor stays truthful, keep the in-capacity
     * prefix, report the record's own bound */
    u32 i = 0;
    for (; i < n && !c.fail; ++i) {
        const u64 v = c.g64();
        if (i < 512)
            t.counts[i] = v;
    }
    t.slots = n;
    return c.fail ? u32(0x50040001) : u32(OK);
}

u32 deserialize_assistant_stats(const u8* data, size_t len, tree::grading::AssistantStats& st) {
    Cursor c{data, len};
    st.total_file_count = c.g64();
    st.raw_count = c.g64();
    st.lost_partition_exist_file_count = c.g64();
    st.lost_partition_delete_file_count = c.g64();
    st.quick_scan_exist_file_count = c.g64();
    st.quick_scan_delete_file_count = c.g64();
    st.deep_scan_file_count = c.g64();
    st.quick_scan_delete_extend = c.g64();
    for (size_t i = 0; i < 8 && !c.fail; ++i)
        st.category_counts[i] = c.g64();
    st.matched = c.g64();
    st.total = c.g64();
    st.graded_count = c.g64();
    st.small_result = c.g32() != 0;
    return c.fail ? u32(0x50040001) : u32(OK);
}
} // namespace session
} // namespace cr
