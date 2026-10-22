/*
 * cr_records.h - typed session record payloads (tree, error map, volume).
 *
 * Tree grammar: depth-first serialization with explicit parent node ids;
 * light/full marker byte per node (corpus pattern section 9); nodes carry
 * the full props field map (corpus serializer table).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RECORDS_H_
#define CR_RECORDS_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <vector>

namespace cr {
namespace session {

/* ---- result tree (REC_RESULT_TREE payload) ---- */

/* Serializes the whole forest (all roots) depth-first. Every node:
 *   u32 parent_id (0xFFFFFFFF = attached to a root)
 *   u8  marker (0 = light, 1 = full)
 *   u8  origin, u8 deleted, u8 confidence
 *   u64 record_ref, u32 fs_kind
 *   u64 logical_size, u64 allocated_size
 *   u64 created, modified, changed, accessed (100ns since 1601)
 *   u32 attributes, u16 flags
 *   u16 name_len + name bytes (UTF-16LE)
 *   u32 extent_count + {u64 offset, u64 length}*
 *   u32 carve_format, u16 carve_confidence
 *   u8 hash_valid + 16B sha128 (when valid)
 *   if full: u16 role_len + role bytes + stats (u64 x4)
 * Container: u32 node_count header + nodes. */
u32 serialize_tree(const tree::ResultTree& tree, std::vector<u8>& out);
u32 deserialize_tree(const u8* data, size_t len, tree::ResultTree& tree);

/* ---- read error map (REC_ERROR_MAP payload) ---- */
u32 serialize_error_map(const ReadErrorMap& map, std::vector<u8>& out);
u32 deserialize_error_map(const u8* data, size_t len, ReadErrorMap& map);

/* ---- volume info (REC_VOLUME_INFO payload) ---- */
struct VolumeInfo {
    u32 fs_kind = 0;      /* NTFS 3 / FAT 0-2 / exFAT 7 / carved 0xFF */
    u64 slice_offset = 0; /* absolute device offset */
    u64 slice_bytes = 0;
    u32 logical_sector = 0;
    u32 physical_sector = 0;
    char label[32] = {0}; /* utf8, NUL padded */
    u64 scanned_bytes = 0;
    u64 elapsed_ms = 0;
    u32 flags = 0; /* bit0 quick, bit1 deep, bit2 carved */
};
u32 serialize_volume_info(const VolumeInfo& vi, std::vector<u8>& out);
u32 deserialize_volume_info(const u8* data, size_t len, VolumeInfo& vi);

/* ---- session state (REC_SESSION_STATE payload) ---- */
struct SessionState {
    u32 stage_done_mask = 0; /* bit0 quick, bit1 deep, bit2 carve */
    u64 files_found = 0;
    u64 bytes_scanned = 0;
    u32 error_map_ranges = 0;
};
u32 serialize_session_state(const SessionState& st, std::vector<u8>& out);
u32 deserialize_session_state(const u8* data, size_t len, SessionState& st);

/* ---- partition context (REC_PARTITION_CONTEXT payload) ---- */
struct PartitionContext {
    u64 start_lba = 0;
    u64 sector_count = 0;
    u32 fs_kind = 0;
    u32 score = 0; /* lost-partition score when applicable */
    u8 kind = 0;   /* 0 live, 1 lost candidate */
};
u32 serialize_partition_context(const PartitionContext& pc, std::vector<u8>& out);
u32 deserialize_partition_context(const u8* data, size_t len, PartitionContext& pc);

} // namespace session
} // namespace cr

#endif // CR_RECORDS_H_
