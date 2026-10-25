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
#include "../storage/cr_freemap.h"
#include "../tree/cr_node.h"
#include "../tree/cr_grading.h"
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
u32 deserialize_tree(const u8* data, size_t len, tree::ResultTree& tree,
                      u32* reparented_out = nullptr);

/* ---- read error map (REC_ERROR_MAP payload) ---- */
u32 serialize_error_map(const ReadErrorMap& map, std::vector<u8>& out);
u32 deserialize_error_map(const u8* data, size_t len, ReadErrorMap& map);

/* ---- S-009 engine free map (REC_ALLOC_MAP payload) ----
 * Wire layout: u32 cluster_bytes, u32 reserved(0), u64 base_byte,
 * u64 cluster_count, u32 bits_len, bits bytes. The stored map is the
 * DOWNSAMPLED cell-granular form (viz::downsample_free_map) - bounded
 * by the overlay budget, not the raw cluster bitmap. */
u32 serialize_free_map(const storage::FreeMap& fm, std::vector<u8>& out);
u32 deserialize_free_map(const u8* data, size_t len, storage::FreeMap& fm);

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
    /* identity card (SESS-2 / GAP-W3-9a, v2 tail - v1 sessions deserialize
     * with zeros and skip verification): device model/serial (enumeration
     * data, informational - a volume legitimately moved between disks keeps
     * its boot hash) + CRC32 of the first 512 bytes at the slice start (the
     * volume identity proper; the resume path compares it and warns on
     * mismatch). */
    char model[48] = {0};
    char serial[32] = {0};
    u32 boot_hash = 0;
    /* S-030 (v3 tail - v1/v2 sessions deserialize with zeros): the image
     * FILE path when the scan target was a disk image; empty otherwise. */
    char image[256] = {0};
};
u32 serialize_volume_info(const VolumeInfo& vi, std::vector<u8>& out);
u32 deserialize_volume_info(const u8* data, size_t len, VolumeInfo& vi);

/* ---- session state (REC_SESSION_STATE payload) ----
 * Wire layout v2 (appended fields; the deserializer accepts both the
 * 24-byte v1 layout and the 40-byte v2 layout - old sessions load with
 * zeroed counters, new sessions restore the full found-set): v1 =
 * mask/files/bytes/ranges; v2 appends deleted_found, carved_found so a
 * reloaded session reports truthful totals without re-running stages. */
struct SessionState {
    u32 stage_done_mask = 0; /* bit0 quick, bit1 deep, bit2 carve */
    u64 files_found = 0;
    u64 bytes_scanned = 0;
    u32 error_map_ranges = 0;
    u64 deleted_found = 0; /* v2 (appended): restored on load */
    u64 carved_found = 0;  /* v2 (appended): restored on load */
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

/* ---- S-023 assistant statistics (REC_ASSISTANT_STATS payload) ----
 * Fixed-layout wire form (v1): 8 u64 counters + 8 u64 category counts +
 * matched + total + graded + small_result flag. */
u32 serialize_assistant_stats(const tree::grading::AssistantStats& st, std::vector<u8>& out);
u32 deserialize_assistant_stats(const u8* data, size_t len, tree::grading::AssistantStats& st);
u32 deserialize_partition_context(const u8* data, size_t len, PartitionContext& pc);

/* ---- restored session summary (G-14/G-23: what load_session gives back
 * beyond the tree itself). Mirrors VolumeInfo on the wire; fs_family is
 * the orchestrator FsFamily the scan ran with (the wire value's true
 * meaning - see cr_scan.cpp save path). */
struct VolumeSessionInfo {
    u32 fs_family = 0;       /* orchestrator::FsFamily as stored (D-1 truth) */
    u32 stage_done_mask = 0; /* actual completion mask (quick|deep|carve) */
    u32 configured_mask = 0; /* stages the original run was configured for */
    u64 slice_offset = 0;
    u64 slice_bytes = 0;
    u32 logical_sector = 0;
    u32 physical_sector = 0;
    char label[32] = {0}; /* utf8, NUL padded */
    u64 scanned_bytes = 0;
    u64 elapsed_ms = 0;
    /* identity card (SESS-2 v2 tail; zeros on v1 sessions = unverifiable) */
    char model[48] = {0};
    char serial[32] = {0};
    u32 boot_hash = 0;
    /* S-030 (v3 tail): the image file path when the scan target was a
     * disk image; empty for disk/volume targets and old sessions. */
    char image[256] = {0};
};

} // namespace session
} // namespace cr

#endif // CR_RECORDS_H_
