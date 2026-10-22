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
    /* depth-first walk; ids in emission order; parents always earlier */
    std::vector<const tree::FileNode*> order;
    std::vector<u32> parent_ids;
    std::function<void(const tree::FileNode*, u32)> walk = [&](const tree::FileNode* n, u32 pid) {
        for (const tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            const u32 id = u32(order.size());
            order.push_back(c);
            parent_ids.push_back(pid);
            walk(c, id);
        }
    };
    for (const tree::FileNode* r : tree.roots())
        walk(r, 0xFFFFFFFFu);

    /* first pass: measure node_count; header written now, patched after */
    put32(out, u32(order.size()));
    for (size_t i = 0; i < order.size(); ++i) {
        const tree::FileNode* n = order[i];
        const tree::FileProperties* pr = n->props();
        put32(out, parent_ids[i]);
        put16(out, 0); /* reserved v1 alignment */
        out.push_back(u8(n->is_full() ? 1 : 0));
        out.push_back(u8(n->origin()));
        out.push_back(u8(n->deleted_state()));
        out.push_back(u8(n->confidence()));
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
        const u32 ec = pr ? u32(pr->extents.size()) : 0;
        put32(out, ec);
        for (const auto& e : (pr ? pr->extents : std::vector<tree::FileProperties::Extent>{})) {
            put64(out, e.offset);
            put64(out, e.length);
        }
        put32(out, pr ? pr->carve_format : 0);
        put16(out, pr ? pr->carve_confidence : 0);
        out.push_back(pr && pr->hash_valid ? 1 : 0);
        if (pr && pr->hash_valid)
            out.insert(out.end(), pr->sha128, pr->sha128 + 16);
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

u32 deserialize_tree(const u8* data, size_t len, tree::ResultTree& tree) {
    Cursor c{data, len};
    const u32 count = c.g32();
    if (c.fail)
        return 0x50040001;
    std::vector<tree::FileNode*> by_id(count, nullptr);
    std::vector<tree::FileNode*> created;
    tree::FullNode* default_root = nullptr; /* lazy synthetic root */
    for (u32 i = 0; i < count; ++i) {
        const u32 parent_id = c.g32();
        const u16 reserved = c.g16();
        (void)reserved;
        const u8 marker = c.g8();
        const u8 origin = c.g8();
        const u8 deleted = c.g8();
        const u8 confidence = c.g8();
        auto* props = new (std::nothrow) tree::FileProperties();
        if (!props)
            return 0x50030009;
        props->record_ref = c.g64();
        props->fs_kind = c.g32();
        props->logical_size = c.g64();
        props->allocated_size = c.g64();
        props->times.created = c.g64();
        props->times.modified = c.g64();
        props->times.changed = c.g64();
        props->times.accessed = c.g64();
        props->attributes = c.g32();
        props->flags = c.g16();
        const u16 name_len = c.g16();
        props->name.resize(name_len);
        for (u16 k = 0; k < name_len; ++k)
            props->name[k] = c.g16();
        const u32 ec = c.g32();
        props->extents.resize(ec);
        for (u32 k = 0; k < ec; ++k) {
            props->extents[k].offset = c.g64();
            props->extents[k].length = c.g64();
        }
        props->carve_format = c.g32();
        props->carve_confidence = c.g16();
        const u8 hv = c.g8();
        if (hv) {
            if (!c.bytes(props->sha128, 16))
                break;
            props->hash_valid = true;
        }
        tree::FileNode* n = nullptr;
        if (marker == 1) {
            /* full node: role + stats; re-attach as a root-level virtual */
            const u16 role_len = c.g16();
            std::u16string role(role_len, 0);
            for (u16 k = 0; k < role_len; ++k)
                role[k] = c.g16();
            (void)c.g64();
            (void)c.g64();
            (void)c.g64();
            (void)c.g64(); /* stats */
            tree::FullNode* full = tree.add_root(role);
            if (full) {
                full->set_props(props);
                n = full;
            }
        } else {
            tree::FileNode* parent = (parent_id == 0xFFFFFFFFu) ? nullptr : by_id[parent_id];
            if (parent == nullptr) {
                /* top-level file node: attach under a synthetic root */
                if (!default_root)
                    default_root = tree.add_root(u"Restored");
                parent = default_root;
            }
            if (parent)
                n = tree.add_file(parent, props);
        }
        props->release();
        if (!n) {
            break;
        }
        n->set_origin(tree::Origin(origin));
        n->set_deleted_state(tree::DeletedState(deleted));
        n->set_confidence(tree::Confidence(confidence));
        by_id[i] = n;
        created.push_back(n);
        if (c.fail)
            break;
    }
    tree.refresh_stats();
    return c.fail && count > 0 && created.size() < count ? u32(0x50040001) : u32(OK);
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
    return c.fail ? u32(0x50040001) : u32(OK);
}

u32 serialize_session_state(const SessionState& st, std::vector<u8>& out) {
    out.clear();
    put32(out, st.stage_done_mask);
    put64(out, st.files_found);
    put64(out, st.bytes_scanned);
    put32(out, st.error_map_ranges);
    return OK;
}

u32 deserialize_session_state(const u8* data, size_t len, SessionState& st) {
    Cursor c{data, len};
    st.stage_done_mask = c.g32();
    st.files_found = c.g64();
    st.bytes_scanned = c.g64();
    st.error_map_ranges = c.g32();
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

} // namespace session
} // namespace cr
