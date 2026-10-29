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
#include "../table/cr_ldm.h" /* S-052: the LDM layouts ride the session */
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
/* N-060: save-path progress + cancel hook (the og's per-child save
 * progress window with per-child IsCancelled teardown - rsf-format
 * --scan-session.md flow section). Called per node during the tree
 * emission; done/total are NODE counts; returning false cancels the
 * serialization (ERR_CANCELLED). NULL = silent save (the pre-N-060
 * behavior). */
using SerializeProgress = bool (*)(u64 done, u64 total, void* user);
/* W1b (R-54): load-path progress + cancel hook - the N-060 mirror. The
 * og's RESTORE reports progress inside the load window (40+50*done/
 * (total+done) - rsf-format--scan-session.md flow section) and checks
 * IsCancelled after EVERY child, tearing down on cancel. Called per
 * attached node during the tree deserialization; done/total are NODE
 * counts; returning false aborts with ERR_CANCELLED - the caller
 * discards the partial tree (a torn load never half-claims state,
 * the load-side twin of the save-side "a torn save never destroys
 * the previous good file"). NULL = silent load (the pre-W1b
 * behavior). */
using DeserializeProgress = bool (*)(u64 done, u64 total, void* user);
u32 serialize_tree(const tree::ResultTree& tree, std::vector<u8>& out,
                   SerializeProgress progress = nullptr, void* progress_user = nullptr);
u32 deserialize_tree(const u8* data, size_t len, tree::ResultTree& tree,
                     u32* reparented_out = nullptr, DeserializeProgress progress = nullptr,
                     void* progress_user = nullptr);

/* ---- S-052: LDM dynamic-volume layouts (REC_LDM_LAYOUTS payload) ----
 * Wire: u32 count + per layout {u8 kind, u64 vol_obj_id, u64
 * total_sectors, u64 chunk_sectors, u32 columns, u64 missing_members,
 * u8 complete, u16 name_len + bytes, u32 copies + per copy u32 members
 * + per member {u64 part_obj_id, u64 disk_obj_id, 16B guid, u64
 * disk_lba, u64 part_start, u64 sectors, u64 vol_offset, u32 index,
 * u8 disk_present}}. The members' disk_lba values are the RE-ANCHORED
 * ones only when the scan's resolver held the member; the recovery-side
 * re-arm re-anchors again through the member's own PRIVHEAD. */
u32 serialize_ldm_layouts(const std::vector<table::LdmVolumeLayout>& layouts, std::vector<u8>& out);
u32 deserialize_ldm_layouts(const u8* data, size_t len, std::vector<table::LdmVolumeLayout>& out);

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
    /* N-066 (v4 tail - v1/v2/v3 sessions deserialize with zeros and skip):
     * the og record-3 identity-card axes + the boot snapshot (rsf-format
     * :228-242 — firmware revision +0x206, WWN/serial-2 +0x30A, interface
     * description +0x414, byte[512] MBR/BOOT snapshot +0x518). The CRC32
     * boot_hash stays the FAST verify path; the snapshot serves mismatch
     * diagnostics (show WHAT changed) and offline partition-context
     * re-materialization if tooling ever needs it. */
    char firmware[32] = {0};
    char wwn[32] = {0};
    char interface_desc[32] = {0};
    u8 boot_snapshot[512] = {0};
    /* W1a (v5 tail - v1..v4 sessions deserialize with 0 = "not
     * recorded"): the save timestamp, unix seconds UTC - our mirror of
     * the og's record-2 SYSTEMTIME scanTime (rsf-format--scan-session.md
     * section 5.2 +0x6C; the og's session list shows the EMBEDDED time,
     * which survives copies, unlike the mtime). */
    u64 save_unix = 0;
    /* N-090 (v6 tail - v1..v5 sessions deserialize with zeros = the
     * legacy 31-char card is the whole truth): the COMPLETE volume
     * label at the og's record-2 volumeName[100] width (strncpy(...,99)
     * - rsf-format section 5.2 +0x04). label[32] stays the legacy
     * prefix view; old readers stop before this tail (the v2..v5
     * len-gate pattern). */
    char label_full[100] = {0};
};
u32 serialize_volume_info(const VolumeInfo& vi, std::vector<u8>& out);
u32 deserialize_volume_info(const u8* data, size_t len, VolumeInfo& vi);

/* ---- session state (REC_SESSION_STATE payload) ----
 * Wire layout v3 (appended fields; the deserializer accepts the 24-byte
 * v1, 40-byte v2 and 48-byte v3 layouts - old sessions load with zeroed
 * counters, new sessions restore the full found-set): v1 =
 * mask/files/bytes/ranges; v2 appends deleted_found, carved_found; v3
 * appends bytes_found (N-167 - the og's "Files Found: N (X GB)"
 * headline aggregate) so a reloaded session reports the found-size
 * truth without re-walking the tree. */
struct SessionState {
    u32 stage_done_mask = 0; /* bit0 quick, bit1 deep, bit2 carve */
    u64 files_found = 0;
    u64 bytes_scanned = 0;
    u32 error_map_ranges = 0;
    u64 deleted_found = 0; /* v2 (appended): restored on load */
    u64 carved_found = 0;  /* v2 (appended): restored on load */
    u64 bytes_found = 0;   /* v3 (appended, N-167): restored on load */
};
u32 serialize_session_state(const SessionState& st, std::vector<u8>& out);
u32 deserialize_session_state(const u8* data, size_t len, SessionState& st);

/* ---- partition context (REC_PARTITION_CONTEXT payload) ----
 * v1 (25 bytes): start/count/fs/score/kind. v2 (41 bytes, M-042): the
 * og's per-record geometry payload appended — bytes_per_sector,
 * sectors_per_cluster, hidden_sectors (the CSearchedPartitionInfo
 * bps/SPC/hidden fields, PartitionRecoveryD lostpart.dat +0x38/+0x444)
 * + a flags word (bit 0 = size_from_device_end, GAP-S6-6's tail-fallback
 * marker). v1 sessions load with the extras zeroed (have-geometry =
 * flags bit 1 unset); the resume re-validation treats zero-geometry
 * records as unprobeable-on-geometry and falls back to the
 * classification identity only. */
struct PartitionContext {
    u64 start_lba = 0;
    u64 sector_count = 0;
    u32 fs_kind = 0;
    u32 score = 0; /* lost-partition score when applicable */
    u8 kind = 0;   /* 0 live, 1 lost candidate */
    /* v2 (M-042): */
    u32 bytes_per_sector = 0;
    u32 sectors_per_cluster = 0;
    u32 hidden_sectors = 0;
    u32 flags = 0; /* bit 0 = size_from_device_end; bit 1 = geometry valid */
    /* v3 (M-137): the og's found-partition row context (UIUX/08 §2) +
     * the naming template's label source. disk_layout is the scan
     * target's table::TableLayout (0 none, 1 MBR, 2 EBR, 3 GPT,
     * 4 GPT-damaged); disk_flags bit 0 = dynamic (LDM database found),
     * bit 1 = the target's table parsed. volume_label is the FAT12/16
     * boot-sector label (NUL-padded, empty for the unnamed path).
     * OPTIONAL per the skip-unknown rule: v2 payloads deserialize with
     * zeros and the older fields are unaffected (append-only wire). */
    u32 disk_layout = 0;
    u32 disk_flags = 0;
    char volume_label[12] = {0};
};
u32 serialize_partition_context(const PartitionContext& pc, std::vector<u8>& out);

/* ---- whole-disk layout snapshot (REC_DISK_LAYOUT payload, M-042) ----
 * The og's partinfo.dat semantics: the live partition table's entries
 * captured at save time; on the next search the freshly enumerated
 * layout is compared against it to flag "disk changed" (PRD:20998-21039,
 * FUN_1800229c0). Surfaced as a summary flag, never enforced (a changed
 * layout does not block the resume — the per-record re-validation is
 * the drop path; this is the honest user-visible signal). Entries:
 * u32 count + per-entry {u64 start_lba, u64 sector_count, u32 mbr_type}.
 * OPTIONAL with the skip-unknown-types rule (sessions saved before
 * M-042 lack it; the flag just stays false). */
struct DiskLayoutEntry {
    u64 start_lba = 0;
    u64 sector_count = 0;
    u32 mbr_type = 0;
};
u32 serialize_disk_layout(const std::vector<DiskLayoutEntry>& entries, std::vector<u8>& out);
u32 deserialize_disk_layout(const u8* data, size_t len, std::vector<DiskLayoutEntry>& out);

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
    char label[32] = {0}; /* utf8, NUL padded (the legacy prefix view) */
    /* N-090: the complete label (the v6 record tail; == label on
     * v1..v5 sessions - the uniform degrade). */
    char label_full[100] = {0};
    u64 scanned_bytes = 0;
    u64 elapsed_ms = 0;
    /* identity card (SESS-2 v2 tail; zeros on v1 sessions = unverifiable) */
    char model[48] = {0};
    char serial[32] = {0};
    u32 boot_hash = 0;
    /* S-030 (v3 tail): the image file path when the scan target was a
     * disk image; empty for disk/volume targets and old sessions. */
    char image[256] = {0};
    /* W1a: the embedded save time (v5+ sessions; 0 = pre-v5). */
    u64 save_unix = 0;
    /* S-052: the LDM dynamic-volume layouts the scan reconstructed,
     * persisted as REC_LDM_LAYOUTS and re-loaded here so the session's
     * recovery/preview consumers can re-arm the virtual devices (the
     * tree's LDM roots carry the matching basis ids). Empty for
     * non-dynamic scans and pre-S-052 sessions. */
    std::vector<table::LdmVolumeLayout> ldm_layouts;
};

/* ---- W1a: the light per-file summary (R-56; the og's record-2 read) ----
 * What a session LIST decodes per file: the two light records only
 * (SESSION_STATE + VOLUME_INFO). The heavy payloads are skipped BY
 * TYPE before any payload I/O (ScanController::read_summary +
 * Reader::peek_record). configured_mask follows load_session_summary's
 * legacy rule (all stages configured when no VolumeInfo rides the
 * file). */
struct SessionSummary {
    u64 session_id = 0;
    SessionState state;       /* counts + stage_done_mask (zeros if absent) */
    VolumeInfo volume;        /* label + identity + save_unix (v5 tail) */
    u32 configured_mask = 0;  /* derived: volume.flags, or all stages */
    u32 record_count = 0;     /* the container's indexed record count */
    bool have_state = false;  /* honest emptiness: no SESSION_STATE seen */
    bool have_volume = false; /* legacy sessions carry no VolumeInfo */
};

} // namespace session
} // namespace cr

#endif // CR_RECORDS_H_
