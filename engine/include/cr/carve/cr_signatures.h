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
    /* N-052 (diff-to-fix): the og's HEIC analyser distinguishes the heic
     * brand (label L"HEIC") from HEIF-family siblings, rewriting their
     * display label to L"HEIF" (analyser-granular-batch1 :67-80). The
     * FMT_HEIC row shrinks to heic-proper; this row carries the siblings.
     * Id APPENDED (serialized carve_format values stay stable). */
    FMT_HEIF,
    /* N-053 (diff-to-fix): the og's registry binds the IIU\0 key to the
     * TIFF chain (BruteForce::TiffAnalyzer, TIFF/EP variant - census :53);
     * the core's IIU row was bound to "Panasonic RAW" instead. This row
     * takes the og binding; FMT_RW2 becomes the fallback after TIFF/EP
     * rejects. Id APPENDED. */
    FMT_TIFF_EP,
    /* N-069 (diff-to-fix): the og registers case-insensitive formats with
     * DUPLICATE keys, not case-folding - "MSCF"/"mscf" are two separate
     * registry rows pointing at FileCABAnalyser (registry census :94;
     * "deliberate registry bloat for correctness", :127). This row is the
     * lowercase CAB key; it shares validate_cab with FMT_CAB. Id APPENDED
     * (serialized carve_format values stay stable). */
    FMT_CAB_LC,
    /* S-037 (diff-to-fix): the og's registry binds FreeHand 11 (key 0x11C
     * -> BruteForce::CFH11Analyser, id 0x1d; decompile:1952-1961) and the
     * three Corel PhotoPaint generations CPT7/CPT8/CPT9 (keys
     * 0x37545043/0x38545043/0x39545043 -> RawSpeed::CPTAnalyser + the
     * FileCPTAnalyser fallback, census :93/:128) - both were absent from
     * every core format list. All APPENDED (serialized carve_format
     * values stay stable). */
    FMT_FH11,
    FMT_CPT7,
    FMT_CPT8,
    FMT_CPT9,
    /* S-007 niche batch 1 (SCAN-2f): the og-only keyed formats absent from
     * every core list - each key re-read first-hand at its RAWSearchDLL
     * registration site (retdec decompile, the static initializer at
     * :5450-6600 + the u16 layer at :6205-6590) before this row landed.
     * All APPENDED (serialized carve_format values stay stable).
     * Multi-key formats keep the og's duplicate-key policy (one row per
     * key, shared validator - MRW/HLP/WMF mirror the og's separate
     * registrations; census :127 "deliberate registry bloat"). */
    FMT_MID, /* "MThd" 0x6468544d -> MIDAnalyser+FileMID, idx 38 */
    FMT_OGG, /* "OggS" 0x5367674f -> OGGAnalyser+CFileOgg, idx 82 */
    FMT_RM,  /* ".RMF" 0x464d522e -> RM_RMVB chain, idx 94 */
    FMT_MXF, /* 06 0E 2B 34 0x342b0e06 -> MXFAnalyser, idx 120 */
    FMT_DMG, /* "BZh1" 0x31685a42 -> DMG chain, idx 113 (level-1 bzip2) */
    FMT_X3F, /* "FOVb" 0x62564f46 -> X3F chain, idx 108 */
    FMT_CRW, /* II\x1A\x00 0x001a4949 -> Brance::CRWAnalyser/FileCRW, id 0x12 (M-081/SCAN-4: Canon
                CIFF, NOT MRW) */
    FMT_CRW_MM,  /* MM\x1A\x00 0x001a4d4d -> the CRW chain's BE sibling key (M-081/SCAN-4) */
    FMT_MRW,     /* \x00MRM 0x004d524d -> FileMRWAnalyser, idx 102, id 0x66 (the ONLY true MRW key -
                    M-081/SCAN-4) */
    FMT_WPG,     /* "wpg\0" 0x00677077 -> FileInfo {min 20, max 0x5F5E100}, stamp 69 */
    FMT_WPD,     /* \xFFWPC 0x435057ff -> FileWPDAnalyser, idx 68 */
    FMT_PSP,     /* "Pain" 0x6e696150 -> PSPAnalyser chain, idx 55 */
    FMT_SIT,     /* "Stuf" 0x66757453 -> SIT chain, idx 93 */
    FMT_GZ,      /* u16 0x8B1F (1F 8B) -> GZAnalyzer+FileGZ, idx 39 */
    FMT_APE,     /* "MAC " 0x2043414d -> CAPEAnalyser, idx 81 */
    FMT_AU,      /* ".snd" 0x646e732e -> FileAUAnalyser, idx 9 */
    FMT_AIFF,    /* "FORM" 0x4d524f46 -> CFileAIFAnalyzer, idx 80 */
    FMT_LOWDOC,  /* DB A5 2D 00 0x002da5db -> FileLowDocAnalyser, id 0x17 (SCAN-4: old-Word .doc
                    family, NOT HLP) */
    FMT_LOWDOC2, /* 94 A6 2E 00 0x002ea694 -> FileLowDocAnalyser, id 0x17 (second key) */
    FMT_HLP, /* 3F 5F 03 00 0x00035f3f -> FileHLPAnalyser, idx 32, id 0x20 (the ONLY true HLP key -
                E49) */
    FMT_ICO, /* 00 00 01 00 0x00010000 -> FileICOAnalyser, idx 34 */
    FMT_CUR, /* 00 00 02 00 0x00020000 -> FileICOAnalyser (cursor dir) */
    FMT_WMF_PL, /* D7 CD C6 9A 0x9ac6cdd7 -> placeable-WMF node, idx 70 */
    FMT_WMF,    /* 01 00 09 00 0x00090001 -> standard-WMF node, idx 70 */
    FMT_TTF,    /* 00 01 00 00 (u16-key 0x0100 chain with MDB) -> FileTTF */
    FMT_MDB,    /* 00 01 00 00 (u16-key 0x0100) + "Standard Jet DB" ->
                 * CMDBAnalyser, type-id 39, bounds {0x1000, 0xBEBC200} */
    FMT_TAR,    /* "ustar" @+257 list-layer probe (strncmp, TARAnalyser) */
    FMT_EPS,    /* u16 "%!" 0x2125 -> CEPS-family PostScript chain */
    FMT_AI,     /* C5 D0 D3 C6 0xc6d3d0c5 -> CEPSAnalyser (Illustrator), idx 27 */
    FMT_DXF,    /* "  0\r" 0x0d302020 -> CDXF chain (g146+g144), idx 25 */
    FMT_3GP,    /* ftyp brand 3gp (the og's File3GP legacy row) */
    /* ---- S-041 niche batch 2, school 1 (SCAN-4): og grammars verified
     * first-hand (the session-5 decode table in SCAN-ENGINE-DELTA). ---- */
    FMT_CHM,      /* "ITSF" 0x46535449 -> FileCHMAnalyser, id 0x0e */
    FMT_ONE,      /* E4 52 5C 7B 0x7b5c52e4 -> OneAnalyser/FileONE, id 0x31 */
    FMT_MNY,      /* u16 "MS" 0x534d -> FileMNYAnalyser, id 0x28 */
    FMT_EMF,      /* list-layer " EMF" @+0x28 -> FileEMFAnalyser, id 0x1a */
    FMT_SWF,      /* list-layer "FWS" -> FileSWFAnalyser, id 0x3e (ZWS NOT accepted) */
    FMT_SWF_CWS,  /* list-layer "CWS" -> FileSWFAnalyser, id 0x3e (2nd key) */
    FMT_3DS,      /* u16 "MM" 0x4d4d -> New3DS/File3DS, id 0x01 ('=='@+0x10 or NCSD@+0x100) */
    FMT_3DS_NCSD, /* "NCSD" @+0x100 -> the same id-0x01 chain's LIST-layer registration
                   * (:3223-3243, first-hand) - the CCI form whose NCSD header sits at
                   * +0x100; the late magic re-expresses that probe */
    /* M-084 (diff-to-fix): the og second Jet-family signature - the
     * "Standard ACE DB" branch of FUN_180015260 (:16515-16517, type 0x4C,
     * the Access 2007+ .accdb generation). Id APPENDED (serialized
     * carve_format values stay stable). */
    FMT_ACCDB,
    /* S-047 (diff-to-fix): the og's two list-layer TS sync scanners
     * (BruteForce::CMTSAnalyser id 0x76 / CM2TSAnalyser id 0x74), fully
     * decoded at :3143-3149/:3173-3179 + FUN_18002e430 :35578-35677 +
     * FUN_180032460 :38457-38536. Candidate generation rides the 1-byte
     * 0x47 magic: TS anchors the sync at +0 (188-byte packets), M2TS at
     * +4 (192-byte BDAV packets, the 4-byte TP_extra_header in front) -
     * the og's own auto-detect order. APPENDED (serialized
     * carve_format values stay stable). */
    FMT_TS,   /* 0x47 sync + follow-byte gate, 188-byte packet walk (CMTS) */
    FMT_M2TS, /* 0x47 sync at +4 + follow-byte gate, 192-byte packet walk (CM2TS;
               * the og stamps the M2TS extension for this family) */
    /* M-092 (diff-to-fix): the og's HTML chain - four u16 keys (<h/<H/<!
     * + the UTF-16LE BOM 0xfeff, census erratum E59) -> HTMLAnalyser id
     * 0x21; the case-insensitive <HTML / <!DOCTYPE HTML / wide probes;
     * the </html> end-tag walk (1 MiB cap, sector round-up). One row per
     * key, shared validator (the og's duplicate-key policy). */
    FMT_HTML,       /* u16 "<h" 0x683c */
    FMT_HTML_H,     /* u16 "<H" 0x483c */
    FMT_HTML_BANG,  /* u16 "<!" 0x213c */
    FMT_HTML_UTF16, /* u16 FF FE 0xfeff - the BOM chain */
    FMT_COUNT
};

constexpr u32 REGISTRY_VERSION = 1;

/* S-032: one inner archive entry (ZIP LFH/CD record or RAR4 FILE block).
 * The og's analysers return the complete inner file inventory - names,
 * sizes, offsets - which the results tree presents as the archive's
 * contents (rawsearchdll--zip-analyser-carving.md :29-38,
 * analyser-granular-batch1 :36-41). */
struct CarveEntry {
    std::string name;    /* inner file name (archive-native bytes, UTF-8 pass-through) */
    u64 size = 0;        /* uncompressed size */
    u64 data_offset = 0; /* slice-relative data start (0 = not localized) */
    u64 data_len = 0;    /* compressed data length (0 = not localized) */
};

/* Validator outcome. */
struct CarveResult {
    bool ok = false;
    u64 end = 0;                     /* absolute end offset (slice-relative) */
    u16 confidence = 0;              /* 0..1000 */
    const char* note = "";           /* evidence note for the node registry */
    std::vector<CarveEntry> entries; /* S-032: inner-entry inventory */
    /* N-063: the analyser's OWN name for the carved file (the og's
     * per-analyser SetName - content/metadata-derived, e.g. the MP4/MOV
     * "Lost Name File (<time>)_EaseUS" template, RAWSearchDLL
     * :33312/:33408/:33447/:33531). Empty = no og naming rule for this
     * format -> the emit site falls back to the namer callback / the
     * documented File%06u deliberate default. */
    std::u16string embedded_name;
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

/* M-024 (diff-to-fix): the runtime per-format enable/disable surface.
 * The og rebuilds its working registry per search from a host-supplied
 * IAnalyserFilter driven by RawConfig.ini [Filter] (34 named toggles in the
 * shipped file - CORRECTIONS #5; STL and VPJ ship DISABLED; the removal loop is RAWSearchDLL
 * FUN_180006b30 :4386-4434 - an analyser whose filter callback rejects
 * it is destroyed out of the u32 map / u16 multimap / list). The core's
 * form: CarveFlags::disabled_formats carries a comma/space/semicolon-
 * separated list of EXTENSIONS or display NAMES (case-insensitive; the
 * og's [Filter] keys are name-form: HEIC, WEBM, 7Z...); both carve
 * layers skip listed formats. Enumeration: registry()'s table order IS
 * FormatId order (one entry per FormatId) - hosts enumerate formats +
 * names + enabled state in one stable order (N-033's surface). */
bool carve_token_listed(const char* disabled_list, const char* ext, const char* name);
bool carve_format_listed(const char* disabled_list, const Signature& sig);

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
