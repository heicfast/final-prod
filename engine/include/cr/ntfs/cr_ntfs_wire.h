/*
 * cr_ntfs_wire.h - NTFS on-disk structures (pure).
 *
 * Constants cross-checked against the RE corpus (ntfs deep-dive, internals,
 * algorithms-v2) - see docs/ground-truth-contracts.md section 3. Where the
 * public NTFS spec and the corpus agree, both are cited; the values below are
 * the format's own, there is no vendor twist.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_NTFS_WIRE_H_
#define CR_NTFS_WIRE_H_

#include "../cr_types.h"
#include <cstring>

namespace cr {
namespace ntfs {

#pragma pack(push, 1)

/* ---- Boot sector (DBR) -------------------------------------------------- */

constexpr u16 NTFS_SIGNATURE = 0xAA55; /* +0x1FE */
constexpr char OEM_ID[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};

struct BootSector {              /* spec-exact, offsets in comments */
    u8 jump[3];                  /* +0x00 */
    char oem[8];                 /* +0x03 "NTFS    " */
    u16 bytes_per_sector;        /* +0x0B */
    u8 sectors_per_cluster;      /* +0x0D */
    u8 unused0[7];               /* +0x0E */
    u8 media_descriptor;         /* +0x15 (0xF8 typical) */
    u16 unused2;                 /* +0x16 */
    u16 sectors_per_track;       /* +0x18 */
    u16 heads;                   /* +0x1A */
    u32 hidden_sectors;          /* +0x1C */
    u32 unused3;                 /* +0x20 */
    u32 unused4;                 /* +0x24 must be 0x00800080 */
    u64 total_sectors;           /* +0x28 */
    u64 mft_lcn;                 /* +0x30 */
    u64 mftmirr_lcn;             /* +0x38 */
    i8 clusters_per_record;      /* +0x40 >0: clusters; <0: 2^(-v) bytes */
    u8 unused5[3];               /* +0x41 */
    i8 clusters_per_index_block; /* +0x44 */
    u8 unused6[3];               /* +0x45 */
    u64 volume_serial;           /* +0x48 */
    u32 checksum;                /* +0x50 must be 0 */
    u8 boot_code[426];           /* +0x54 */
    u8 signature[2];             /* +0x1FE 55 AA */
};
static_assert(sizeof(BootSector) == 512);

/* ---- MFT record ---------------------------------------------------------- */

constexpr u32 RECORD_MAGIC_FILE = 0x454C4946; /* 'FILE' LE - corpus line 21152 */

struct RecordHeader {
    u32 magic;                  /* 'FILE' */
    u16 update_sequence_offset; /* to USA array (corpus: must be nonzero mod 256) */
    u16 update_sequence_size;   /* USA entries + 1, in u16 units */
    u64 log_sequence;
    u16 sequence;
    u16 link_count;
    u16 attribute_offset; /* to first attribute, 8-aligned */
    u16 flags;            /* 0x01 in-use, 0x02 directory (corpus bit mapping) */
    u32 bytes_in_use;
    u32 bytes_allocated;
    u64 base_record; /* 0 = base record */
    u16 next_attr_id;
    u16 unused;
    u32 record_number; /* only on XP+ */
};
static_assert(sizeof(RecordHeader) == 48);

/* flags */
constexpr u16 RECORD_IN_USE = 0x0001; /* corpus: walker deleted branch */
constexpr u16 RECORD_IS_DIRECTORY = 0x0002;

/* ---- Attribute ----------------------------------------------------------- */

/* Attribute-type registry below this line is WIRE-FORMAT DOCUMENTATION
 * only - the parser dispatches on the types it consumes (0x10/0x20/0x30/
 * 0x80/0x90/0xA0); the rest document the on-disk namespace (AUDIT-NTFS §4
 * row 19: kept deliberately, not consumed). */
constexpr u32 ATTR_TYPE_STANDARD_INFORMATION = 0x10;
constexpr u32 ATTR_TYPE_ATTRIBUTE_LIST = 0x20; /* corpus: fragmented-MFT gate */
constexpr u32 ATTR_TYPE_FILE_NAME = 0x30;
constexpr u32 ATTR_TYPE_VOLUME_VERSION_OLD = 0x40; /* $OBJECT_ID is 0x40 modern */
constexpr u32 ATTR_TYPE_OBJECT_ID = 0x40;
constexpr u32 ATTR_TYPE_SECURITY_DESCRIPTOR = 0x50;
constexpr u32 ATTR_TYPE_VOLUME_NAME = 0x60;
constexpr u32 ATTR_TYPE_VOLUME_INFORMATION = 0x70;
constexpr u32 ATTR_TYPE_DATA = 0x80; /* corpus 54487 */
constexpr u32 ATTR_TYPE_INDEX_ROOT = 0x90;
constexpr u32 ATTR_TYPE_INDEX_ALLOCATION = 0xA0;
constexpr u32 ATTR_TYPE_BITMAP = 0xB0;
constexpr u32 ATTR_TYPE_REPARSE_POINT = 0xC0;
constexpr u32 ATTR_TYPE_EA_INFORMATION = 0xD0;
constexpr u32 ATTR_TYPE_EA = 0xE0;
constexpr u32 ATTR_TYPE_LOGGED_UTILITY_STREAM = 0x100;
constexpr u32 ATTR_TYPE_TERMINATOR = 0xFFFFFFFF;

/* WIRE-FORMAT DOCUMENTATION (not consumed): nonresident attribute flags. */
constexpr u16 ATTR_FLAG_COMPRESSED = 0x0001;
constexpr u16 ATTR_FLAG_ENCRYPTED = 0x4000;
constexpr u16 ATTR_FLAG_SPARSE = 0x8000;

struct AttributeHeader {
    u32 type;
    u32 length;      /* includes header; 8-aligned */
    u8 nonresident;  /* 0 resident */
    u8 name_length;  /* in u16 chars */
    u16 name_offset; /* to name, after header */
    u16 flags;
    u16 attribute_id;
    /* resident continues below; nonresident layout in NonresidentHeader */
};

struct ResidentAttributeHeader {
    AttributeHeader base;
    u32 value_length; /* corpus: resident $DATA cap logic 0xFFF */
    u16 value_offset;
    u8 indexed_flag;
    u8 unused;
};
static_assert(sizeof(ResidentAttributeHeader) == 24);

struct NonresidentTail {  /* 48 bytes following the 16-byte common header */
    u64 start_vcn;        /* +0x10 absolute */
    u64 last_vcn;         /* +0x18 */
    u16 runlist_offset;   /* +0x20 from attribute start */
    u16 compression_unit; /* +0x22 */
    u32 unused;           /* +0x24 */
    u64 allocated;        /* +0x28 */
    /* NTFS-7 (AUDIT-NTFS): spec field order is FILE (real) SIZE @+0x30
     * then INITIALIZED SIZE @+0x38 (ntfs-3g layout.h; ntfs3
     * data_size@0x30 / valid_size@0x38). The previous labels were swapped,
     * so `na.actual` read the INITIALIZED size - benign while
     * allocated == initialized == real, but a sparse nonresident
     * $ATTRIBUTE_LIST under-read its content length. */
    u64 actual;      /* +0x30 file (real) size */
    u64 initialized; /* +0x38 valid-data (initialized) size */
};
static_assert(sizeof(NonresidentTail) == 48);
/* compressed-size u64 at +0x40 exists only in compressed records - parsed
 * on demand, not part of the fixed tail. */

/* ---- $FILE_NAME ---------------------------------------------------------- */

constexpr u8 FILE_NAME_POSIX = 0x00;
constexpr u8 FILE_NAME_WIN32 = 0x01;
constexpr u8 FILE_NAME_DOS = 0x02;
constexpr u8 FILE_NAME_WIN32_AND_DOS = 0x03;

struct FileNameAttribute { /* resident value body, after 0x66-byte header part */
    u64 parent_record;     /* MFT reference of parent (record# | seq<<48) */
    u64 creation_time;     /* 100ns since 1601-01-01 (R2.2: exact resolution) */
    u64 modification_time;
    u64 mft_change_time;
    u64 access_time;
    u64 allocated_size;
    u64 real_size;
    u32 flags;
    u32 reparse;
    u8 name_length; /* in u16 chars */
    u8 namespace_type;
    /* u16 name[name_length] follows */
};
static_assert(sizeof(FileNameAttribute) == 66);

/* $STANDARD_INFORMATION resident value body (0x48 bytes, XP+ v3). */
struct StandardInformation {
    u64 creation_time;     /* +0x00 100ns since 1601 */
    u64 modification_time; /* +0x08 */
    u64 mft_change_time;   /* +0x10 */
    u64 access_time;       /* +0x18 */
    u32 dos_flags;         /* +0x20 */
    u32 max_version;       /* +0x24 */
    u32 version;           /* +0x28 */
    u32 class_id;          /* +0x2C */
    u32 owner_id;          /* +0x30 */
    u32 security_id;       /* +0x34 */
    u64 quota_charged;     /* +0x38 */
    u64 usn;               /* +0x40 */
};
static_assert(sizeof(StandardInformation) == 72);

/* $INDEX_ROOT / $INDEX_ALLOCATION walking: the walk model is
 * attribute-based (children enumerated via $ATTRIBUTE_LIST record refs,
 * cr_mftwalk); no INDX-block walker exists and none is planned. The
 * speculative INDX wire family (magic, IndexHeader, IndexEntryHeader,
 * IndxBlockHeader) was deleted (plan ruling D-N, AUDIT-NTFS §4 row 9:
 * 0 refs; og INDX walking itself is UNVERIFIED-7 in the corpus). */

/* ---- $MFT record 16 = $Bitmap etc: cluster bitmap attr ------------------ */

#pragma pack(pop)

/* ---- helpers (pure) ------------------------------------------------------ */

inline u64 mft_reference_record(u64 ref) {
    return ref & 0x0000FFFFFFFFFFFFULL;
}

/* Record size from DBR: clusters_per_record sign convention. */
inline u64 record_size_from_dbr(const BootSector& bs, u32 bytes_per_sector) {
    if (bs.clusters_per_record > 0)
        return static_cast<u64>(bs.clusters_per_record) * bs.sectors_per_cluster * bytes_per_sector;
    if (bs.clusters_per_record < 0) {
        if (-bs.clusters_per_record > 20) /* P1-4: shift-UB guard, 1 MiB cap */
            return 0;
        return 1ULL << (-bs.clusters_per_record);
    }
    return 0;
}

inline bool boot_signature_ok(const u8* sector) {
    return sector[510] == 0x55 && sector[511] == 0xAA;
}
inline bool oem_is_ntfs(const BootSector& bs) {
    char want[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
    return std::memcmp(bs.oem, want, 8) == 0;
}

} // namespace ntfs
} // namespace cr

#endif // CR_NTFS_WIRE_H_
