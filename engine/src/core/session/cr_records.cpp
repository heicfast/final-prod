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

u32 serialize_tree(const tree::ResultTree& tree, std::vector<u8>& out) {
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

    /* first pass: measure node_count; header written now, patched after */
    put32(out, u32(order.size()));
    for (size_t i = 0; i < order.size(); ++i) {
        const tree::FileNode* n = order[i];
        const tree::FileProperties* pr = n->props();
        /* v2 node flags (stored in the v1 reserved slot; old readers skip
         * the extra fields, old files read back with flags==0):
         *   bit0: extents carry {u32 codec, u64 psize} each
         *   bit1: resident_data blob follows the extents
         *   bit2: short_name follows the name (N-011a, versioned
         *         optional: u16 length + UTF-16 units)
         *   bit3: the S-023 grade byte follows the short name (2-bit
         *         grade | 0x04 overlap flag | 0xF8 reserved-zero) */
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
        }
        put32(out, parent_ids[i]);
        put16(out, nflags);
        out.push_back(u8(n->is_full() ? 1 : 0));
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
        const u32 ec = pr ? u32(pr->extents.size()) : 0;
        put32(out, ec);
        for (const auto& e : (pr ? pr->extents : std::vector<tree::FileProperties::Extent>{})) {
            put64(out, e.offset);
            put64(out, e.length);
            if (nflags & 1) {
                put32(out, e.codec);
                put64(out, e.psize);
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
        if (n->is_full()) {
            const tree::FullNode* f = static_cast<const tree::FullNode*>(n);
            put16(out, u16(f->role.size()));
            for (u16 ch : f->role)
                put16(out, ch);
            put64(out, f->stats().file_count);
            put64(out, f->stats().dir_count);
            put64(out, f->stats().total_logical);
            put64(out, f->stats().deleted_count);
        }
    }
    return OK;
}

u32 deserialize_tree(const u8* data, size_t len, tree::ResultTree& tree, u32* reparented_out) {
    Cursor c{data, len};
    const u32 count = c.g32();
    if (c.fail)
        return 0x50040001;
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
        if (marker != 0 && marker != 1) {
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
        PropsRef props_ref(props); /* N-077: releases on every early break */

        /* (Convergent repair note: this session's DFX16 leg found the
         * same leak and filed the identical unique_ptr fix; the parallel
         * lane's N-077 PropsRef landing is canonical - kept, duplicate
         * removed at the rebase union.) */        /* Leak repair (found by the s19 ASan/LSan suite sweep): the
         * N-058/M-051 break paths between this allocation and the
         * attach+release below leaked the properties object (two leaks
         * in the hostile-record tests). The guard releases on ANY exit
         * from this iteration until ownership transfers. */
        struct PropsGuard {
            tree::FileProperties* p;
            ~PropsGuard() {
                if (p)
                    p->release();
            }
        } props_guard{props};
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
        const u32 ec = c.g32();
        props->extents.resize(ec);
        for (u32 k = 0; k < ec; ++k) {
            props->extents[k].offset = c.g64();
            props->extents[k].length = c.g64();
            if (nflags & 1) {
                props->extents[k].codec = c.g32();
                props->extents[k].psize = c.g64();
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
            if (c.left < size_t(nl) * 2u)
                break;
            props->carve_note.resize(nl);
            for (u16 k = 0; k < nl; ++k)
                props->carve_note[k] = c.g16();
            props->carve_anchor_b = c.g64();
            props->carve_flag = c.g8() != 0;
        }
        tree::FileNode* n = nullptr;
        if (marker == 1) {
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
            tree::FullNode* full = tree.add_root(role);
            if (full) {
                full->set_stats(st);
                full->set_props(props);
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
        props_guard.p = nullptr; /* ownership transferred (or the node
                                  * attach failed: released below) */
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
    return c.fail ? u32(0x50040001) : u32(OK);
}

u32 serialize_partition_context(const PartitionContext& pc, std::vector<u8>& out) {
    out.clear();
    put64(out, pc.start_lba);
    put64(out, pc.sector_count);
    put32(out, pc.fs_kind);
    put32(out, pc.score);
    out.push_back(pc.kind);
    return OK;
}

u32 deserialize_partition_context(const u8* data, size_t len, PartitionContext& pc) {
    Cursor c{data, len};
    pc.start_lba = c.g64();
    pc.sector_count = c.g64();
    pc.fs_kind = c.g32();
    pc.score = c.g32();
    pc.kind = c.g8();
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
