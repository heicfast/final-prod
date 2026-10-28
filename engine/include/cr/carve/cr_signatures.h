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
    /* M-093 (diff-to-fix): the og's XML/SVG chain - four u32 keys
     * (<?xm / UTF-8 BOM+'<' / the numeric dword 0x1CD / <svg), one
     * shared validator, the probe stamping id 0x13 (generic XML, the
     * "csl" marker) or 0x5b (SVG) by CONTENT (carried by
     * CarveResult::format_override). Ids APPENDED (serialized
     * carve_format values stay stable). */
    FMT_XML,      /* u32 "<?xm" - the declaration path */
    FMT_XML_BOM,  /* u32 EF BB BF 3C - the UTF-8 BOM path */
    FMT_SVG_CD01, /* u32 CD 01 00 00 (the decoded numeric key 0x1cd) */
    FMT_SVG,      /* u32 "<svg" - the direct SVG path + the probe's override target */
    /* M-096 (diff-to-fix): the og's MOBI analyser (id 0x85, the RawConfig
     * "MOBI" toggle - shipped =1/enabled) - the full-salvage PalmDB gate
     * (the BOOKMOBI pair at +0x3C, NOTHING before it validated) + the
     * 512-step end-tag walk (0A 0D 8E E9), cap 500 MB. The magic carries
     * the probe's own acceptance (the pair at +0x3C). */
    FMT_MOBI, /* "BOOKMOBI" @+0x3C - the weak gate, ported exactly */
    /* N-111 (diff-to-fix): the og's JP2 family (JP2Analyser id 0x80,
     * list-layer, the RawConfig "JP2" token :3544-3559). The probe gates
     * the "jP  " signature-box type @+4, the 0x0D0A870A payload @+8 and
     * the ftyp box @+0x10, then maps the major brand @+0x14 through an
     * exact-match std::map walk (:21886-21936): "jp2 " -> 0x80,
     * "jpx " -> 0x81, "jpm " -> 0x82 - every other brand REJECTS. The
     * signature-box length @+0 is never read (the salvage posture). The
     * size machinery decoded at :21941-22115 (the finding had recorded
     * it as the follow-up; this session read it first-hand): the box
     * walk advances by BE u32 box lengths (len==1 -> the u64 extended
     * form at +8; len==0 -> the FF D9 EOC byte scan with a 16-zero
     * back-off, guard 0x1F3FFFFF; high-bit length -> size 0); a "jp2c"
     * box latches the codestream flag and the FIRST NON-jp2c box after
     * it ends the extent (the jp2c-consecutive rule - the og's end
     * detector). FMT_JP2 carries the og's own scan key (the jP box);
     * FMT_JPX/FMT_JPM carry the brand-position keys (the map's
     * discriminative keys) so each family member resolves its own
     * extension. Ids APPENDED (serialized carve_format values stay
     * stable). */
    FMT_JP2, /* brand "jp2 " (the og id 0x80) - the jP-box scan key */
    FMT_JPX, /* brand "jpx " (the og id 0x81) - the brand-position key */
    FMT_JPM, /* brand "jpm " (the og id 0x82) - the brand-position key */
    /* N-112 (diff-to-fix): the og's LZH (LHARC/LHA) archive family
     * (Brance::LZHAnalyser + FileLZHAnalyser, id 0x24, LIST-layer
     * registration only - no u32 key; erratum E53: the census's E4 52 5C
     * 7B key binds OneNote). The probe (FUN_18003db20 :46937-46973) gates
     * the level-1 method field: *(u32*)(buf+2) in {"-lh0" 0x30686c2d,
     * "-lh5" 0x35686c2d, "-lhd" 0x64686c2d, "-lh7" 0x37686c2d} AND
     * buf[6]=='-'. The size walk (FUN_18003dd70 :47045+, decoded
     * first-hand + the .data method constants dumped from the binary at
     * RVA 0x9388c: -lh0/-lhd pair, -lh5, -lh7) chains entries with
     * PER-METHOD strides: -lh0/-lhd advance hdr_u8@0 + comp_u32@7 + 2
     * (the CRC; the +2 suppressed once the ext flag latched), -lh5
     * advances hdr_u8@0 only and latches the ext flag, -lh7 advances
     * hdr_u8@0 + comp_u32@7 and latches it; a non-matching position
     * triggers the -2/+2 CRC step-back dance and ends the chain with
     * size = pos + 1 (the single end-marker byte); 0x401 iterations per
     * window; the FileInfo ctor bound 0xC800000 (200 MiB). One row per
     * method key (the M-092 duplicate-key policy), all ext "lzh" - the
     * og stamps one id (0x24) regardless of key. Ids APPENDED. */
    FMT_LZH,  /* the "-lh0-" key (stored) */
    FMT_LZH5, /* the "-lh5-" key (the dominant compressed method) */
    FMT_LZHD, /* the "-lhd-" key (directory entries) */
    FMT_LZH7, /* the "-lh7-" key (newer compressed method) */
    /* N-113 (diff-to-fix): the og's small text-format list probes, the
     * five-grammar school (every probe + walk re-verified first-hand this
     * session; the POS open question settled by reading FUN_18001a470):
     *   VCF  (id 0x88, :3390-3453): _memicmp(buf,"BEGIN:VCARD",11)==0
     *        (case-insensitive - the registry magic carries the canonical
     *        uppercase form, the RFC-universal writer shape); the walk
     *        scans 0x400 chunks for the exact-case "END:VCARD\r\n" (the
     *        u64/u16/byte compare chain), extent += find+11, then the
     *        MULTI-CARD continuation (the next 11 bytes memicmp
     *        "BEGIN:VCARD" again -> keep walking); cap 0x1400000 (20 MiB).
     *   GPX  (id 0x97, :3947-4074): memcmp(buf,"<?xml version=\"",15) at
     *        0 OR +3 (the UTF-8 BOM form), then a u32 "<gpx" scan in
     *        [+0x16, +0x216) (the root element window); the walk scans
     *        0x200 chunks for "</gpx>", extent = find+6; cap 0x6400000
     *        (100 MiB - the id!=0xa0 branch of the shared walk).
     *   OSP  (id 0x8f, :3695-3758): the 10-byte prefix u64+u16 compare
     *        ('{\r\n "di": '); the walk scans 0x200 chunks for
     *        "\"libopenshot\":" within 2 MiB; extent = find+14+0xF; the
     *        needle NOT FOUND -> size 0 -> REJECT (the family's only
     *        reject-on-missing - the og sets the size field to 0).
     *   POS  (id 0x8b, :3631-3694): _memicmp(buf,"{\"diagram\":",11)==0;
     *        the walk needle (the finding's open question, settled from
     *        FUN_18001a470): "ProcessOn Schema File" (21 bytes); 0x200
     *        chunks, extent = find+21+0x13; cap 0x6400000.
     *   CHFP (id 0x90, :3759-3822): the XML declaration (case-SENSITIVE
     *        memcmp) AND _memicmp(buf+0x28,"<BiffProject",12)==0; the
     *        walk scans 0x200 chunks for "</BiffProject>", extent =
     *        find+14; cap 0x6400000.
     * The 0x200-chunk finds are PER-CHUNK (a needle straddling a 512-byte
     * boundary is missed by the og's own chunked std::string find -
     * ported exactly); VCF's 0x401-position loop over-reads 10 bytes past
     * the 0x400 advance (the u64 compare chain - also ported). The og's
     * 64 KiB sliding-window trailing-NUL trim (the >16-zero tail check) is
     * adapted as the family's bounded fallback at the slice cap (the
     * M-092/M-093 convention for this school's walks). Ids APPENDED. */
    FMT_VCF,     /* "BEGIN:VCARD" - the vCard key */
    FMT_GPX,     /* "<?xm" + the <gpx window - the GPX key */
    FMT_GPX_BOM, /* EF BB BF 3C - the BOM-form GPX key */
    FMT_OSP,     /* '{\r\n "di": ' - the OpenShot key */
    FMT_POS,     /* '{"diagram":' - the ProcessOn key */
    FMT_CHFP,    /* "<?xm" + <BiffProject@0x28 - the CHFP key */
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
    /* S-049: the analyser's CONTENT-DERIVED naming prefix (the og's
     * per-analyser prefix string that reaches DRWUI's FUN_140c44770
     * "%s_%06d" naming - DRWUI_decompiled.c:1878330-81, both caller
     * sites :1877600/:1877724). The og's engine-side analysers supply
     * a prefix extracted from the file's own content - real-session
     * evidence (evidence/executions/s10b-real-rsf-session-mining.md):
     * JPEG SOF dims (JPG_192x144) / EXIF Model (GT-S7562) / EXIF
     * Software (Adobe Photoshop 7.0), MP4/MOV mvhd duration (MP4_08m07s),
     * MP3 ID3v2 title, PDF Title/Creator-Producer + CreationDate,
     * DOCX docProps created, ZIP first central-directory entry name.
     * Empty = no decoded content prefix -> the emit site names via the
     * og default: the format's own UPPERCASE extension as the prefix.
     *
     * History: this carrier was N-063's `embedded_name` (the RAWSearchDLL
     * "Lost Name File (<time>)_EaseUS" template, :33312/:33408). The
     * three REAL og sessions prove the template NEVER fires for carved
     * MP4/MOV in the shipped product (57/57 real MP4/MOV carve rows are
     * MP4_%06d.MP4 / MP4_<duration>_%06d.MP4 / MOV_*): the DRWUI tree
     * naming layer names every unnamed carve record through the
     * %s_%06d contract and the engine template does not survive it.
     * The carrier's semantics moved from full-name to PREFIX with the
     * S-049 landing (the N-063 landing superseded on the carve-tree
     * application by executed evidence). */
    std::u16string name_prefix;
    /* M-093: the og stamps the FORMAT ID at the probe, not at the
     * dispatch key - the SVG chain yields 0x5b for svg-content and 0x13
     * for the generic-XML marker regardless of which of its four keys
     * fired. When a validator sets this (default 0xFFFFFFFF = none),
     * the emit site uses it for carve_format and resolves the effective
     * Signature (ext/name/base-confidence) from the registry row of the
     * overridden id. */
    u32 format_override = 0xFFFFFFFFu;
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
