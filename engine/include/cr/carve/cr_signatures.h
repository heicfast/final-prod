/*
 * cr_signatures.h - versioned signature registry + validator contract (R4).
 *
 * Fix F8: every entry MUST carry a grammar validator - header-only entries
 * are rejected at registry load. The table is versioned; validation runs at
 * engine init and fails closed.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SIGNATURES_H_
#define CR_SIGNATURES_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include <vector>

namespace cr {
namespace carve {

enum FormatId : u32 {
    FMT_JPG = 0,
    FMT_PNG,
    FMT_TIFF,
    FMT_CR2,
    FMT_WEBP,
    FMT_HEIC,
    FMT_PDF,
    FMT_ZIP,
    FMT_DOCX, /* ZIP + [Content_Types].xml as first local entry */
    FMT_7Z,
    FMT_SQLITE,
    FMT_MP4,
    FMT_MOV,
    FMT_AVI,
    FMT_MKV,
    FMT_RAR4,
    FMT_PST,
    FMT_RAR5,
    FMT_DNG,
    FMT_ORF,
    FMT_RAF,
    FMT_RW2,
    FMT_PSD,
    FMT_MP3,
    FMT_WAV,
    FMT_M4A,
    FMT_ASF,
    FMT_FLV,
    FMT_CAB,
    FMT_ISO,
    FMT_COUNT
};

constexpr u32 REGISTRY_VERSION = 1;

/* Validator outcome. */
struct CarveResult {
    bool ok = false;
    u64 end = 0;             /* absolute end offset (slice-relative) */
    u16 confidence = 0;      /* 0..1000 */
    bool fragmented = false; /* grammar detected index/table gaps (R4.5) */
    const char* note = "";   /* evidence note for the node registry */
};

/*
 * Validator: sees the WHOLE slice (bounded reads) + the candidate hit offset.
 * Contract: MUST read through the slice (no unbounded allocations), MUST
 * cap by entry max_size, MUST return ok=false on any grammar break.
 */
using Validator = CarveResult (*)(IStorageDevice* slice, u64 hit, u64 max_size);

struct Signature {
    FormatId id;
    const char* ext; /* "jpg" */
    u8 header[16];   /* magic bytes (16 covers GUID-length signatures) */
    u32 header_len;
    u32 header_offset; /* magic position within the file */
    u64 min_size;
    u64 max_size;
    u16 base_confidence; /* 0..1000 before validator delta */
    Validator validator;
    const char* name; /* human label */
};

/* The compile-time table (one entry per FormatId, order = FormatId). */
const Signature* registry();
size_t registry_size();

/* F8 gate: validate every entry; returns 0 = OK, else the count of rejects
 * (with a diagnostic list in `bad`). */
u32 validate_registry(std::vector<const Signature*>* bad);

} // namespace carve
} // namespace cr

#endif // CR_SIGNATURES_H_
