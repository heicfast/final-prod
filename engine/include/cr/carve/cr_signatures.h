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
#include <string>
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
    /* S-007 (diff-to-fix): the og's hardcoded common-format carve set -
     * the dual-registry keys the core lacks entirely (GIF8, BM, the
     * MPEG-audio sync family, AAC ADTS, {\\rt, OLE D0CF11E0, MPEG PS/ES,
     * AC10). All APPENDED (serialized carve_format values stable). */
    FMT_GIF,
    FMT_BMP,
    FMT_RTF,
    FMT_OLE,
    FMT_MPEGPS,
    FMT_MPEGES,
    FMT_DWG,
    FMT_AAC,
    FMT_AAC2, /* FF F9: the second ADTS sync (MPEG-2 branch) */
    /* headerless MPEG audio: the og's u16 sync family
     * (RAWSearchDLL :2565-2721) - one entry per key, shared validator */
    FMT_MPGA_E2,
    FMT_MPGA_E3,
    FMT_MPGA_E4,
    FMT_MPGA_E5,
    FMT_MPGA_E6,
    FMT_MPGA_E7,
    FMT_MPGA_F2,
    FMT_MPGA_F3,
    FMT_MPGA_F4,
    FMT_MPGA_F5,
    FMT_MPGA_F6,
    FMT_MPGA_F7,
    FMT_MPGA_FA,
    FMT_MPGA_FB,
    FMT_MPGA_FC,
    FMT_MPGA_FD,
    FMT_MPGA_FE,
    FMT_MPGA_FF,
    FMT_COUNT
};

constexpr u32 REGISTRY_VERSION = 1;

/* S-032: one inner archive entry (ZIP LFH/CD record or RAR4 FILE block).
 * The og's analysers return the complete inner file inventory - names,
 * sizes, offsets - which the results tree presents as the archive's
 * contents (rawsearchdll--zip-analyser-carving.md :29-38,
 * analyser-granular-batch1 :36-41). */
struct CarveEntry {
    std::string name; /* inner file name (archive-native bytes, UTF-8 pass-through) */
    u64 size = 0;     /* uncompressed size */
    u64 data_offset = 0; /* slice-relative data start (0 = not localized) */
    u64 data_len = 0;    /* compressed data length (0 = not localized) */
};

/* Validator outcome. */
struct CarveResult {
    bool ok = false;
    u64 end = 0;           /* absolute end offset (slice-relative) */
    u16 confidence = 0;    /* 0..1000 */
    const char* note = ""; /* evidence note for the node registry */
    std::vector<CarveEntry> entries; /* S-032: inner-entry inventory */
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
    /* S-007: true for the repeating-sync audio family - every frame of a
     * headerless MPEG/AAC stream re-matches the 2-byte magic, so a later
     * candidate starting strictly INSIDE an already-emitted stream of the
     * same format is the SAME stream (the og's sector-aligned probe never
     * sees mid-stream syncs; one walker run = one file). Other formats
     * keep the exact-start dedup only (nested cross-format files stay
     * carvable). */
    bool suppress_overlap = false;
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
