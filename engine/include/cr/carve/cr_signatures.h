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
#include <cstring>
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
    /* CARVE-3 (AUDIT-PIPELINE, F-A3-4): the og registry keys the TIFF
     * family BE variants and the ORF IIRS/MMOR forms (raw-dual-registry
     * seeds 2a004d4d / 53524949 / 524f4d4d) - our table was II-only and
     * the BE branch of tiff_common was unreachable. Ids are APPENDED so
     * serialized carve_format values stay stable. */
    FMT_TIFF_BE,
    FMT_ORF_IIRS,
    FMT_ORF_MMOR,
    /* CARVE-2 (AUDIT-PIPELINE, F-A3-3): the og ZIP analyser anchors on
     * LFH AND central-directory entries (PK\x01\x02 - preferred) AND the
     * EOCD; indexless archives (destroyed CD) still carve. */
    FMT_ZIP_CD,
    /* S-017: the og's list-layer MovAnalyzer also salvages MOV/MP4 whose
     * ftyp box was destroyed (naked moov). Id APPENDED to keep serialized
     * carve_format values stable. */
    FMT_MOV_NAKED,
    FMT_COUNT
};

constexpr u32 REGISTRY_VERSION = 1;

/* Validator outcome. */
struct CarveResult {
    bool ok = false;
    u64 end = 0;           /* absolute end offset (slice-relative) */
    u16 confidence = 0;    /* 0..1000 */
    const char* note = ""; /* evidence note for the node registry */
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

/* S-017 F8 self-probe: for every signature, place its magic at
 * header_offset inside a synthetic buffer and run the SAME window-match
 * logic scan_one_window uses (match_signature below). Unreachable
 * signatures - a magic no real file can carry, e.g. the pre-S-017 BMFF
 * entries that baked literal zero size-bytes into the magic - fail here
 * at load time instead of shipping as dead code. Returns 0 = OK, else the
 * count of signatures whose magic could not be reached (listed in `bad`). */
u32 self_probe_registry(std::vector<const Signature*>* bad);

/* S-017: the single window-matching primitive shared by the carve scan
 * and the F8 self-probe (one source of truth - a magic change cannot
 * desynchronize the two). `off` is the window offset of the first magic
 * byte; on match, `cand_rel` receives the candidate's window-relative
 * start (off - header_offset). Window-head clip included. */
inline bool match_signature(const Signature& sig, const u8* win, u64 win_len, u64 off,
                            u64& cand_rel) {
    if (off >= win_len || win_len - off < sig.header_len)
        return false;
    if (sig.header[0] != win[off])
        return false;
    if (sig.header_len > 1 && std::memcmp(win + off, sig.header, sig.header_len) != 0)
        return false;
    if (off < sig.header_offset)
        return false; /* window head clip */
    cand_rel = off - sig.header_offset;
    return true;
}

} // namespace carve
} // namespace cr

#endif // CR_SIGNATURES_H_
