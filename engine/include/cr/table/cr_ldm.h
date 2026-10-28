/*
 * cr_ldm.h - LDM (Logical Disk Manager / Windows dynamic disks) full decode.
 *
 * Session-5 scope: FULL LDM - database validation (PRIVHEAD / TOCBLOCK /
 * VMDB), the complete VBLK object graph (disk groups, disks, volumes,
 * components, partitions, incl. composite multi-fragment records), and
 * dynamic-volume reconstruction (simple / spanned / striped / mirrored /
 * RAID-5) as read-only virtual devices wired into the scan orchestrator.
 *
 * Ground truth (every constant below is traceable, see AUDIT.md row 30):
 *   - Linux kernel block/partitions/ldm.{c,h} (deployed MBR-dynamic-disk
 *     parser; saved verbatim at scripts/ldm_reference.{c,h}): record bases
 *     0x18/0x1D/0x2D/0x2E + fixed gaps, per-type length checks
 *     (VBLK_SIZE_*), VMDB geometry (vblk array at vblk_offset, bounded by
 *     vblk_size*vblk_last), TOCBLOCK copies at config+1/2/2045/2046,
 *     PRIVHEAD copies at LBA 6 / config+1856 / config+2047.
 *   - flatcap LDM technical documentation (linux-ntfs project): field
 *     names/order for every VBLK type, "Stripe Size LBA" (= sectors),
 *     variable-width integer/string encodings, update-status semantics.
 *   - libldm (mdbooth/ldmtool, the production userspace implementation):
 *     GPT dynamic-disk discovery (PRIVHEAD in the LAST LBA of the LDM
 *     metadata partition, GUID 5808C8AA-...), VMDB field map (committed_
 *     seq@0x75, disk-group name/guid), composite-record reassembly as
 *     payload concatenation, volume kind rules, RAID-5 layout "raid5_ls"
 *     (= md left-symmetric) and chunk size in 512-byte sectors.
 *   - corpus LdmManager.mo (91,475-line decompile): the volume-storage
 *     class family is CONFIRMED PRESENT (SimpleVolumeStorage,
 *     SpanVolumeStorage, StripeVolumeStorage, MirrorVolumeStorage,
 *     Raid5VolumeStorage incl. ComputeDataWithOtherDisk - a crash-site
 *     string). LDM-1 note (OQ-T7 RESOLVED, N-148 re-pinned): the RAID-5
 *     parity rotation was decoded - left-symmetric, hardwired, IDENTICAL
 *     to our raid5_ls layout (the evidence record:
 *     diff-to-fix/evidence/decompiled-quotes/ldm-raid5-and-scoring.md
 *     :8-34; the comparison at cr_ldm.cpp:1346-59). The layout is both
 *     the libldm verdict AND an og-parity MATCH.
 *
 * Divergences (documented, deliberate):
 *   - The kernel's cmp3 "chunksize" reads the first vnum after the
 *     post-volume-id pad and its vol5 "volume_state" memcpy overreads 2
 *     bytes into volume_type - both are dead code in the kernel; we read
 *     the structured fields the way libldm does (which feeds real dm
 *     tables, so it is the field-validated reading).
 *   - Damaged individual VBLKs are counted and skipped instead of aborting
 *     the whole database walk (kernel/libldm abort; a recovery scanner
 *     must surface every salvageable object - failures are reported in
 *     LdmStats, never silent).
 *   - LBA->byte translation uses the device's logical sector size
 *     (kernel-consistent; identical to the flatcap "512-byte logical
 *     blocks" rule on every real 512-byte dynamic disk).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_LDM_H_
#define CR_LDM_H_

#include "../storage/cr_device.h"
#include <functional>
#include <string>
#include <vector>

namespace cr {
namespace table {

/* ------------------------------------------------------------------ */
/* GUID (16 raw bytes, mixed-endian on GPT wire, opaque here)          */
/* ------------------------------------------------------------------ */

struct LdmGuid {
    u8 b[16] = {};
    bool operator==(const LdmGuid& o) const {
        for (int i = 0; i < 16; i++)
            if (b[i] != o.b[i])
                return false;
        return true;
    }
    bool operator!=(const LdmGuid& o) const { return !(*this == o); }
    bool operator<(const LdmGuid& o) const {
        for (int i = 0; i < 16; i++)
            if (b[i] != o.b[i])
                return b[i] < o.b[i];
        return false;
    }
    bool is_zero() const {
        for (int i = 0; i < 16; i++)
            if (b[i] != 0)
                return false;
        return true;
    }
    void to_string(char out[37]) const; /* dashed lowercase, NUL-terminated */
};

/* Parse "{...}" / dashed ASCII GUID text (PRIVHEAD stores GUIDs as text). */
bool ldm_parse_guid_text(const char* text, LdmGuid& out);

/* ------------------------------------------------------------------ */
/* Database structures                                                 */
/* ------------------------------------------------------------------ */

struct LdmPrivhead {
    u16 ver_major = 0, ver_minor = 0; /* 2.11 (Win2K/XP) or 2.12 (Vista) */
    u64 logical_disk_start = 0;       /* sectors: public (data) region */
    u64 logical_disk_size = 0;
    u64 config_start = 0;        /* sectors: 1 MiB database region */
    u64 config_size = 0;         /* normally 2048 sectors */
    std::string disk_guid;       /* ASCII, slot @0x30 (64 bytes) */
    std::string host_guid;       /* ASCII, slot @0x70 */
    std::string disk_group_guid; /* ASCII, slot @0xB0 */
    std::string disk_group_name; /* ASCII, slot @0xF0 (32 bytes) */
    LdmGuid disk_guid_bin = {};
    bool is_vista = false;
};

struct LdmTocblock {
    std::string bitmap1_name; /* "config" */
    u64 bitmap1_start = 0;    /* sectors, relative to config_start */
    u64 bitmap1_size = 0;
    std::string bitmap2_name; /* "log" */
    u64 bitmap2_start = 0;
    u64 bitmap2_size = 0;
};

struct LdmVmdb {
    u16 ver_major = 0, ver_minor = 0; /* 4.10 */
    u16 update_status = 0;            /* 0x01 = committed / consistent */
    u32 vblk_size = 0;                /* bytes per VBLK record (128 typical) */
    bool vblk_size_defaulted = false; /* zero vblk_size defaulted to 0x80 (og) */
    u32 vblk_offset = 0;              /* byte offset of the VBLK array (512 typical) */
    u32 vblk_last = 0;                /* last committed sequence number (array bound) */
    u64 committed_seq = 0;            /* BE u64 @0x75 */
    u64 pending_seq = 0;
    u32 n_vols = 0, n_comps = 0, n_parts = 0, n_disks = 0;
    u32 n_pending_vols = 0, n_pending_comps = 0, n_pending_parts = 0, n_pending_disks = 0;
    std::string disk_group_name; /* @0x16, 31 bytes */
    std::string disk_group_guid; /* @0x35, 64 bytes */
};

/* ------------------------------------------------------------------ */
/* VBLK objects                                                        */
/* ------------------------------------------------------------------ */

/* Component partition layout (flatcap "Partition Layout"). */
enum LdmComponentType : u8 {
    LDM_COMP_STRIPE = 0x01,
    LDM_COMP_CONCAT = 0x02, /* simple + spanned volumes */
    LDM_COMP_RAID = 0x03    /* RAID-5 (volume read policy 4 / type "raid5") */
};

struct LdmDiskGroupObj {
    u64 obj_id = 0;
    std::string name;
    std::string guid_str;  /* v3: ASCII GUID string */
    LdmGuid guid_bin = {}; /* v4: raw 16 bytes */
};

struct LdmDiskObj {
    u64 obj_id = 0;
    std::string name;  /* "Disk1" ... */
    LdmGuid guid = {}; /* v3: parsed from ASCII; v4: raw binary */
    bool guid_valid = false;
    std::string alt_name; /* "Last Device Name" */
};

struct LdmComponentObj {
    u64 obj_id = 0;
    u64 parent_id = 0; /* owning volume */
    std::string name;
    std::string state;  /* "ACTIVE" */
    u8 type = 0;        /* LdmComponentType */
    u64 n_parts = 0;    /* declared children count */
    u64 chunk_size = 0; /* sectors; striped/raid5 only */
    u64 n_columns = 0;  /* striped/raid5 only */
};

struct LdmPartitionObj {
    u64 obj_id = 0;
    u64 parent_id = 0;  /* owning component */
    u64 disk_id = 0;    /* owning disk object */
    std::string name;   /* "Disk1-01" ... */
    u64 start = 0;      /* sectors, disk-relative (add logical_disk_start) */
    u64 size = 0;       /* sectors */
    u64 vol_offset = 0; /* "Partition Offset In Column" - position in volume/column */
    u32 index = 0;      /* column index (flag 0x08) */
    bool has_index = false;
};

constexpr u8 LDM_VOLTYPE_GEN = 3;   /* libldm _VOLUME_TYPE_GEN */
constexpr u8 LDM_VOLTYPE_RAID5 = 4; /* libldm _VOLUME_TYPE_RAID5 */

struct LdmVolumeObj {
    u64 obj_id = 0;
    std::string name;        /* "Volume1", "Stripe1", "Raid1" ... */
    std::string volume_type; /* "gen" or "raid5" */
    std::string state;       /* 14-byte slot, NUL padded */
    u8 int_volume_type = 0;  /* libldm _int_type: _VOLUME_TYPE_GEN=3
                              * (all kinds but RAID-5) or _VOLUME_TYPE_RAID5=4;
                              * the byte after the 14-byte volume state */
    u8 flags = 0;
    u8 part_type = 0; /* 0x07 NTFS, 0x0B FAT32, 0x06 RAW/FAT16 */
    u64 size = 0;     /* sectors (Size LBA) */
    u64 n_comps = 0;  /* declared component count */
    LdmGuid guid = {};
    std::string id1, id2; /* optional (flags 0x08 / 0x20) */
    u64 size2 = 0;        /* optional column size (flag 0x80) */
    std::string hint;     /* optional mount hint (flag 0x02) */
};

/* ------------------------------------------------------------------ */
/* Resolved volume layouts                                             */
/* ------------------------------------------------------------------ */

enum class LdmVolumeKind : u8 {
    Simple = 0,
    Spanned,
    Striped,
    Mirrored,
    Raid5,
    Broken /* graph incomplete / inconsistent */
};

const char* ldm_volume_kind_name(LdmVolumeKind k);

/* One partition participating in a volume, resolved to absolute LBAs. */
struct LdmMember {
    u64 part_obj_id = 0;
    u64 disk_obj_id = 0;
    LdmGuid disk_guid = {};
    u64 disk_lba = 0;          /* absolute start LBA on its disk (incl. lds) */
    u64 part_start = 0;        /* raw partition start (PRT3), before the lds */
    u64 sectors = 0;           /* partition size, sectors */
    u64 vol_offset = 0;        /* spanned ordering */
    u32 index = 0;             /* column order */
    bool disk_present = false; /* resolver knew this disk's GUID */
};

/* A resolved volume: one entry per mirror copy; exactly one copy for the
 * non-mirrored kinds. Members are ordered per kind (spanned: vol_offset;
 * striped/raid5: column index; simple: single). */
struct LdmVolumeLayout {
    u64 vol_obj_id = 0;
    std::string name;
    LdmVolumeKind kind = LdmVolumeKind::Broken;
    u64 total_sectors = 0; /* min(volume Size LBA, computed capacity) */
    u64 chunk_sectors = 0; /* striped / raid5 */
    u32 columns = 0;       /* striped / raid5 */
    u64 missing_members = 0;
    bool complete = false; /* graph + geometry fully consistent */
    std::vector<std::vector<LdmMember>> copies;
};

struct LdmStats {
    u32 vblks_total = 0;  /* records visited (incl. unused slots) */
    u32 vblks_parsed = 0; /* objects decoded */
    u32 vblks_bad = 0;    /* damaged records skipped (counted, not silent) */
    u32 composites = 0;   /* multi-fragment records reassembled */
    u32 composites_incomplete = 0;
    u32 privheads_matched = 0;
    u32 tocblocks_matched = 0;
    bool degraded = false; /* VMDB status != 1, vblk_size defaulted, or the
                            * kernel TOC-bound check failed - database read
                            * anyway (og recovery behavior), surfaced, not
                            * silent */
};

/* ------------------------------------------------------------------ */
/* Parse result                                                        */
/* ------------------------------------------------------------------ */

struct LdmInfo {
    bool found = false;
    bool is_gpt = false;   /* GPT-labelled dynamic disk (Vista+) */
    u64 metadata_base = 0; /* byte offset the database was found at (GPT:
                              metadata partition start; MBR: 0) */
    LdmPrivhead ph;
    LdmTocblock toc;
    LdmVmdb vm;
    std::vector<LdmDiskGroupObj> groups;
    std::vector<LdmDiskObj> disks;
    std::vector<LdmVolumeObj> volumes;
    std::vector<LdmComponentObj> components;
    std::vector<LdmPartitionObj> partitions;
    std::vector<LdmVolumeLayout> layouts; /* resolved, one per volume */
    LdmStats stats;
};

/*
 * Parse the LDM database off a whole-disk device.
 *
 * Handles both MBR dynamic disks (type 0x42 span; PRIVHEAD at LBA 6 with
 * backups, database at config_start) and GPT dynamic disks (LDM metadata
 * partition; PRIVHEAD in its last LBA). Returns:
 *   OK                 database parsed (fields always fully populated)
 *   ERR_BAD_MAGIC      not a dynamic disk (or no valid PRIVHEAD)
 *   ERR_BAD_CHECKSUM   PRIVHEAD/TOCBLOCK copies inconsistent or damaged
 *   ERR_UNSUPPORTED    structure recognized but version-gated out
 *   ERR_READ_FAILED    IO errors on the metadata region
 *   ERR_TRUNCATED      database claims geometry beyond the device
 */
u32 parse_ldm_database(IStorageDevice* device, LdmInfo& out);

/* ------------------------------------------------------------------ */
/* Read-only virtual volume device                                     */
/* ------------------------------------------------------------------ */

/* Resolves a member disk's GUID to its device, or nullptr when that disk
 * is not part of this scan (e.g. the other half of a spanned volume). */
using LdmDiskResolver = std::function<IStorageDevice*(const LdmGuid&)>;

/*
 * LdmVolumeDevice - presents one dynamic volume as an IStorageDevice.
 *
 * Read semantics (all failures surface, nothing is silently zeroed):
 *   simple/spanned  a member region that cannot be read fails the request
 *   striped         same, per stripe chunk
 *   mirrored        copies are tried in order; all failing fails the
 *   raid5           a missing/unreadable DATA column is reconstructed by
 *                   XOR of the surviving columns (left-symmetric layout,
 *                   parity rotates P(row) = N-1-(row mod N)); a missing
 *                   parity column needs no reconstruction; two or more
 *                   missing columns make the intersecting rows
 *                   unrecoverable (ERR_READ_FAILED)
 *
 * Thread safety: reads carry no shared mutable state (reconstruction
 * buffers are per-call); the underlying devices must be safe for
 * concurrent reads (the standard IStorageDevice contract).
 */
class LdmVolumeDevice final : public IStorageDevice {
public:
    /*
     * The layout is COPIED (the device re-anchors each member's LBAs
     * through its own PRIVHEAD when the resolver provides the member's
     * device, so it must own the adjusted geometry); the resolver is
     * borrowed (must outlive the device). The volume's logical geometry
     * is 512-byte blocks per the LDM spec.
     */
    LdmVolumeDevice(const LdmVolumeLayout& layout, LdmDiskResolver resolver);

    u32 read_at(u64 offset, void* buffer, u32 length) override;
    const Geometry& geometry() const override { return geom_; }

    /* S-052: the (re-anchored) layout this device serves - the session
     * persistence and the recovery-side overlap mapping read it. */
    const LdmVolumeLayout& layout() const { return layout_; }

    /* S-052: the physical spans a volume-relative byte range touches -
     * the recovery writer's destination-overlap check input (R7.2/F12
     * must compare PHYSICAL placements, not layout-relative offsets).
     * Per kind:
     *   simple/spanned  the exact member runs (translate_spanned)
     *   striped         the data chunks' member runs (exact - a striped
     *                   read touches only its own column)
     *   mirrored        EVERY copy's runs (conservative: copies are
     *                   tried in order, any may be read)
     *   raid5           EVERY column of every covered row (conservative:
     *                   reconstruction XORs the surviving columns)
     * A member whose device does not resolve contributes no span (the
     * read of that member fails; its bytes are not touched). */
    struct PhysicalSpan {
        IStorageDevice* dev; /* the member disk's device (non-null) */
        u64 offset;          /* absolute byte offset on that device */
        u64 length;          /* bytes */
    };
    std::vector<PhysicalSpan> physical_spans(u64 offset, u64 length) const;

private:
    u32 read_spanned_copy(const std::vector<LdmMember>& members, u64 sec, u32 nsec, u8* dst);
    LdmVolumeLayout layout_;
    LdmDiskResolver resolver_;
    Geometry geom_{};
};

} // namespace table
} // namespace cr

#endif // CR_LDM_H_
