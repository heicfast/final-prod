/* cr_registry.cpp - the signature table + load-time validation (F8 gate).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_signatures.h"
#include "cr/storage/cr_device.h"

namespace cr {
namespace carve {

/* defined in cr_analysers.cpp */
CarveResult validate_jpeg(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_png(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_tiff(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_cr2(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_webp(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_heic(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_heif(IStorageDevice* s, u64 hit, u64 cap); /* N-052 */
CarveResult validate_pdf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_zip(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_docx(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_7z(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_sqlite(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mp4(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mov(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_avi(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mkv(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_rar4(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_pst(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_rar5(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_dng(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_orf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_raf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_rw2(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_psd(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mp3(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_wav(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_m4a(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_asf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_flv(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_cab(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_iso(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_zip_cd(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mov_naked(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_gif(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_bmp(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mp3_raw(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_aac_adts(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_rtf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_ole(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mpegps(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mpeges(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_dwg(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_fh11(IStorageDevice* s, u64 hit, u64 cap); /* S-037 */
CarveResult validate_cpt(IStorageDevice* s, u64 hit, u64 cap);  /* S-037 */
/* S-007 niche batch 1 (SCAN-2f) */
CarveResult validate_mid(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_ogg(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_rm(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mxf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_dmg(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_x3f(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mrw(IStorageDevice* s, u64 hit,
                         u64 cap); /* M-081/SCAN-4: 1 row (\x00MRM only) */
CarveResult validate_crw(IStorageDevice* s, u64 hit,
                         u64 cap); /* M-081/SCAN-4: 2 rows (II/MM 1A 00) */
CarveResult validate_wpg(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_wpd(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_psp(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_sit(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_gz(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_ape(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_au(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_aiff(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_hlp(IStorageDevice* s, u64 hit,
                         u64 cap); /* SCAN-4: 1 row (3F 5F 03 00 only) */
CarveResult validate_lowdoc(IStorageDevice* s, u64 hit,
                            u64 cap); /* SCAN-4: 2 rows (DBA5/94A6 = FileLowDoc id 0x17) */
CarveResult validate_ico(IStorageDevice* s, u64 hit, u64 cap); /* ico+cur */
CarveResult validate_wmf_placeable(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_wmf_std(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_ttf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mdb(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_accdb(IStorageDevice* s, u64 hit, u64 cap); /* M-084: the ACE branch */
CarveResult validate_tar(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_eps(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_ai(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_dxf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_3gp(IStorageDevice* s, u64 hit, u64 cap);
/* S-041 niche batch 2, school 1 (SCAN-4) */
CarveResult validate_chm(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_one(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_mny(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_emf(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_swf(IStorageDevice* s, u64 hit, u64 cap); /* swf + swf_cws */
CarveResult validate_3ds(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_3ds_ncsd(IStorageDevice* s, u64 hit, u64 cap); /* the list-layer twin */
CarveResult validate_ts(IStorageDevice* s, u64 hit, u64 cap);       /* S-047: CMTS, 188-byte */
CarveResult validate_m2ts(IStorageDevice* s, u64 hit, u64 cap);     /* S-047: CM2TS, 192-byte */
CarveResult validate_html(IStorageDevice* s, u64 hit, u64 cap);     /* M-092: the four-key chain */
CarveResult validate_xmlsvg(IStorageDevice* s, u64 hit, u64 cap);   /* M-093: the shared chain */
CarveResult validate_mobi(IStorageDevice* s, u64 hit, u64 cap);     /* M-096: the salvage gate */
CarveResult validate_jp2(IStorageDevice* s, u64 hit, u64 cap);      /* N-111: the JP2 family */
CarveResult validate_lzh(IStorageDevice* s, u64 hit, u64 cap);      /* N-112: the LZH family */
CarveResult validate_vcf(IStorageDevice* s, u64 hit, u64 cap);      /* N-113: the text school */
CarveResult validate_gpx(IStorageDevice* s, u64 hit, u64 cap);      /* N-113 */
CarveResult validate_osp(IStorageDevice* s, u64 hit, u64 cap);      /* N-113 */
CarveResult validate_pos(IStorageDevice* s, u64 hit, u64 cap);      /* N-113 */
CarveResult validate_chfp(IStorageDevice* s, u64 hit, u64 cap);
CarveResult validate_office_zip(IStorageDevice* s, u64 hit, u64 cap); /* N-115: the typed rows */
CarveResult validate_dbx(IStorageDevice* s, u64 hit, u64 cap);        /* M-117/M-121 */
CarveResult validate_hik(IStorageDevice* s, u64 hit, u64 cap);        /* M-098 */
CarveResult validate_aep(IStorageDevice* s, u64 hit, u64 cap);        /* M-099 */

namespace {
const Signature kTable[] = {
    {FMT_JPG,
     "jpg",
     {0xFF, 0xD8, 0xFF},
     3, /* N-054: the og keys JPEG on FOUR exact dwords (FF D8 FF
         * E0/E1/DB/EE - census :54); the core matches the 3-byte prefix -
         * a registered deliberate detection superset (I-009 family,
         * D-64): FF D8 FF xx with xx outside the og's four is carved here
         * and not dispatched by the og. validate_jpeg's grammar gates the
         * false-positive surface; do not tighten for parity. */
     0,
     512,
     96ull << 20,
     900,
     validate_jpeg,
     "JPEG"},
    {FMT_PNG,
     "png",
     {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A},
     8,
     0,
     64,
     64ull << 20,
     950,
     validate_png,
     "PNG"},
    {FMT_TIFF, "tif", {'I', 'I', 0x2A, 0x00}, 4, 0, 512, 256ull << 20, 800, validate_tiff, "TIFF"},
    {FMT_CR2,
     "cr2",
     {'I', 'I', 0x2A, 0x00},
     4,
     0,
     512,
     256ull << 20,
     850,
     validate_cr2,
     "Canon RAW 2"},
    {FMT_WEBP, "webp", {'R', 'I', 'F', 'F'}, 4, 0, 64, 128ull << 20, 800, validate_webp, "WebP"},
    {FMT_HEIC, "heic", {'f', 't', 'y', 'p'}, 4, 4, 128, 512ull << 20, 750, validate_heic, "HEIC"},
    {FMT_PDF, "pdf", {'%', 'P', 'D', 'F', '-'}, 5, 0, 128, 512ull << 20, 900, validate_pdf, "PDF"},
    {FMT_ZIP,
     "zip",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     2ull << 30,
     700,
     validate_zip,
     "ZIP",
     /* N-153: an inner LFH strictly inside an emitted archive is the
      * SAME archive (the og's sector-bitmap extent consumption; the
      * S-032 inventory presents the entries as children instead) */
     true},
    {FMT_DOCX,
     "docx",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     800,
     validate_docx,
     "Office Open XML",
     /* N-153: an inner LFH position strictly inside an emitted archive
      * is the SAME archive (the og's sector-bitmap consumption; its
      * S-032 inventory presents the entries as children instead) */
     true},
    {FMT_7Z,
     "7z",
     {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C},
     6,
     0,
     64,
     8ull << 30,
     850,
     validate_7z,
     "7-Zip"},
    {FMT_SQLITE,
     "sqlite",
     {'S', 'Q', 'L', 'i', 't', 'e', ' ', 'f', 'o', 'r', 'm', 'a'},
     12,
     0,
     4096,
     2ull << 30,
     850,
     validate_sqlite,
     "SQLite"},
    /* S-017: the BMFF magic is the "ftyp" BOX TYPE at candidate+4 - the
     * box SIZE at candidate+0 is data (0x14/0x18/0x20 for real files), so
     * baking literal zero size-bytes into the magic made all four
     * signatures unreachable (only a degenerate size-0 box ever matched,
     * and even that mis-shifted the candidate by 4 so every brand read
     * landed on the "ftyp" bytes and rejected). Keyed on "ftyp" with
     * header_offset 4, the candidate lands on the size field and the
     * validators read the brand at candidate+8 - where real files carry
     * it. */
    {FMT_MP4, "mp4", {'f', 't', 'y', 'p'}, 4, 4, 128, 4ull << 30, 750, validate_mp4, "MP4"},
    {FMT_MOV, "mov", {'f', 't', 'y', 'p'}, 4, 4, 128, 4ull << 30, 700, validate_mov, "QuickTime"},
    {FMT_AVI, "avi", {'R', 'I', 'F', 'F'}, 4, 0, 64, 4ull << 30, 700, validate_avi, "AVI"},
    {FMT_MKV,
     "mkv",
     {0x1A, 0x45, 0xDF, 0xA3},
     4,
     0,
     128,
     16ull << 30,
     700,
     validate_mkv,
     "Matroska"},
    {FMT_RAR4,
     "rar",
     {'R', 'a', 'r', '!', 0x1A, 0x07, 0x00},
     7,
     0,
     128,
     4ull << 30,
     800,
     validate_rar4,
     "RAR4"},
    {FMT_PST,
     "pst",
     {'!', 'B', 'D', 'N'},
     4,
     0,
     64ull << 10,
     48ull << 30,
     850,
     validate_pst,
     "Outlook PST"},
    {FMT_RAR5,
     "rar5",
     {'R', 'a', 'r', '!', 0x1A, 0x07, 0x01, 0x00},
     8,
     0,
     32, /* sig+main+endarc = 22-byte valid minimum */
     4ull << 30,
     850,
     validate_rar5,
     "RAR5"},
    {FMT_DNG,
     "dng",
     {'I', 'I', 0x2A, 0x00},
     4,
     0,
     1024,
     4ull << 30,
     880,
     validate_dng,
     "Digital Negative"},
    {FMT_ORF,
     "orf",
     {'I', 'I', 'R', 'O'},
     4,
     0,
     1024,
     1ull << 30,
     800,
     validate_orf,
     "Olympus RAW"},
    {FMT_RAF,
     "raf",
     {'F', 'U', 'J', 'I'},
     4,
     0, /* N-054: the og keys RAF on the 4-byte "FUJI" u32 (census :55,
         * 0x494a5546); the 16-byte FUJIFILMCCD-RAW string is now the
         * PRIMARY acceptance path inside validate_raf (not the sole one)
         * - variant headers whose bytes 4..15 differ still carve. */
     1024,
     2ull << 30,
     850,
     validate_raf,
     "Fuji RAW"},
    {FMT_RW2,
     "rw2",
     {'I', 'I', 'U', 0x00},
     4,
     0,
     1024,
     1ull << 30,
     800,
     validate_rw2,
     "Panasonic RAW"},
    {FMT_PSD, "psd", {'8', 'B', 'P', 'S'}, 4, 0, 64, 4ull << 30, 870, validate_psd, "Photoshop"},
    {FMT_MP3, "mp3", {'I', 'D', '3'}, 3, 0, 512, 512ull << 20, 820, validate_mp3, "MP3 audio"},
    {FMT_WAV, "wav", {'R', 'I', 'F', 'F'}, 4, 0, 36, 4ull << 30, 900, validate_wav, "WAV audio"},
    {FMT_M4A, "m4a", {'f', 't', 'y', 'p'}, 4, 4, 128, 4ull << 30, 800, validate_m4a, "M4A audio"},
    {FMT_ASF,
     "wmv",
     {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE,
      0x6C},
     16,
     0,
     64,
     4ull << 30,
     850,
     validate_asf,
     "ASF/WMV"},
    {FMT_FLV, "flv", {'F', 'L', 'V'}, 3, 0, 64, 4ull << 30, 820, validate_flv, "Flash video"},
    {FMT_CAB, "cab", {'M', 'S', 'C', 'F'}, 4, 0, 36, 2ull << 30, 870, validate_cab, "Cabinet"},
    {FMT_ISO,
     "iso",
     {'C', 'D', '0', '0', '1'},
     5,
     0x8001,
     33792, /* PVD sector + margin (F8: min covers the late magic) */
     32ull << 30,
     900,
     validate_iso,
     "ISO9660"},
    /* CARVE-3 (AUDIT-PIPELINE, F-A3-4): the og registry seeds the TIFF
     * family BE variant (2a004d4d = LE-read of "MM\\0\\x2A") and the ORF
     * IIRS (53524949) / MMOR (524f4d4d) forms. tiff_common already walks
     * BE IFDs - the branches were dead only because no magic reached
     * them. */
    {FMT_TIFF_BE,
     "tif",
     {'M', 'M', 0x00, 0x2A},
     4,
     0,
     512,
     256ull << 20,
     800,
     validate_tiff,
     "TIFF (big-endian)"},
    {FMT_ORF_IIRS,
     "orf",
     {'I', 'I', 'R', 'S'},
     4,
     0,
     1024,
     1ull << 30,
     800,
     validate_orf,
     "Olympus RAW (IIRS)"},
    {FMT_ORF_MMOR,
     "orf",
     {'M', 'M', 'O', 'R'},
     4,
     0,
     1024,
     1ull << 30,
     800,
     validate_orf,
     "Olympus RAW (MMOR)"},
    /* CARVE-2: the og ZIP analyser's PREFERRED anchor - a surviving
     * central directory (PK\x01\x02) carves even when the archive head
     * is destroyed. */
    {FMT_ZIP_CD,
     "zip",
     {'P', 'K', 0x01, 0x02},
     4,
     0,
     46,
     2ull << 30,
     600,
     validate_zip_cd,
     "ZIP (central directory)",
     /* N-153: a CD record inside an emitted archive's extent is already
      * carved as part of it; the headless-salvage path (a CD with the
      * head destroyed) is never inside an emitted extent - unaffected */
     true},
    /* S-017: the og's list-layer MovAnalyzer (RAWSearchDLL FUN_180004fb0
     * :2919-4208; the u16-key 0x0000 registration at :2748 is dead under
     * the dispatch's zero-header guard :9954 - the LIVE path is the list
     * layer) also salvages MOV/MP4 whose ftyp box was destroyed. We mirror
     * it conservatively: anchor on a naked top-level "moov" box (metadata
     * anchor - mdat/wide/free are too generic to key on), walk the box
     * grammar, accept with raw-sign-class confidence. The og's exact
     * accept criteria are not decoded (pending-continuation model
     * :10114 - tracked as a follow-up in S-017's Open questions). */
    {FMT_MOV_NAKED,
     "mov",
     {'m', 'o', 'o', 'v'},
     4,
     4,
     64,
     4ull << 30,
     620,
     validate_mov_naked,
     "QuickTime (naked moov)"},
    /* S-007 (diff-to-fix): the og's hardcoded common formats - corpus
     * keys cited per row. Validators in cr_analysers.cpp. */
    {FMT_GIF, "gif", {'G', 'I', 'F', '8'}, 4, 0, 64, 64ull << 20, 840, validate_gif, "GIF image"},
    {FMT_BMP, "bmp", {'B', 'M'}, 2, 0, 64, 256ull << 20, 780, validate_bmp, "BMP image"},
    {FMT_RTF,
     "rtf",
     {'{', 0x5C, 'r', 't'},
     4,
     0,
     16,
     16ull << 20,
     800,
     validate_rtf,
     "RTF document"},
    {FMT_OLE,
     "doc",
     {0xD0, 0xCF, 0x11, 0xE0},
     4,
     0,
     512,
     2ull << 30,
     850,
     validate_ole,
     "OLE compound (doc/xls/ppt)"},
    {FMT_MPEGPS,
     "mpg",
     {0x00, 0x00, 0x01, 0xBA},
     4,
     0,
     32,
     2ull << 30,
     650,
     validate_mpegps,
     "MPEG video (program stream)"},
    {FMT_MPEGES,
     "m1v",
     {0x00, 0x00, 0x01, 0xB3},
     4,
     0,
     32,
     2ull << 30,
     600,
     validate_mpeges,
     "MPEG video (elementary)"},
    /* M-107 member 3 (DWG): the row max carries the og walk's own cap
     * constant - the child walk's cap check at raw-asm 0x18002f5ee
     * (`imul $0x64,0x70(%rax)`): singleton+0x70 (0xbebc200 = 200 MB)
     * x 100 = 0x4A817C800 (20 GB). The 64 MiB stand-in value is gone. */
    /* the S86 convergence rider: the row floor
     * re-pinned to the og's registered 0x15 = 21 (the CDWGRawInfo factory FUN_18002ff20's qword
     * write at +0x110 - puVar1[0x22] = 0x15, the PNG/ONE ctor pattern
     * [0x22]=min/[0x23]=max; the SAME write seeds the gen-1 walk cursor).
     * The max stays the first landing's 0x4A817C800 (100 x the engine
     * default 200 MB, the LAB_18002f5e9 sanity). */
    {FMT_DWG,
     "dwg",
     {'A', 'C', '1', '0'},
     4,
     0,
     0x15,
     0x4A817C800ull,
     620,
     validate_dwg,
     "AutoCAD DWG"},
    {FMT_AAC,
     "aac",
     {0xFF, 0xF1},
     2,
     0,
     512,
     256ull << 20,
     700,
     validate_aac_adts,
     "AAC audio (ADTS)",
     true},
    {FMT_AAC2,
     "aac",
     {0xFF, 0xF9},
     2,
     0,
     512,
     256ull << 20,
     700,
     validate_aac_adts,
     "AAC audio (ADTS/MPEG-2)",
     true},
    /* the headerless MPEG-audio family: the og registers one u16 key per
     * sync (:2565-2721); each shares the 10-frame BruteForce walker */
    {FMT_MPGA_E2,
     "mp3",
     {0xFF, 0xE2},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync E2)",
     true},
    {FMT_MPGA_E3,
     "mp3",
     {0xFF, 0xE3},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync E3)",
     true},
    {FMT_MPGA_E4,
     "mp3",
     {0xFF, 0xE4},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync E4)",
     true},
    {FMT_MPGA_E5,
     "mp3",
     {0xFF, 0xE5},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync E5)",
     true},
    {FMT_MPGA_E6,
     "mp3",
     {0xFF, 0xE6},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync E6)",
     true},
    {FMT_MPGA_E7,
     "mp3",
     {0xFF, 0xE7},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync E7)",
     true},
    {FMT_MPGA_F2,
     "mp3",
     {0xFF, 0xF2},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync F2)",
     true},
    {FMT_MPGA_F3,
     "mp3",
     {0xFF, 0xF3},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync F3)",
     true},
    {FMT_MPGA_F4,
     "mp3",
     {0xFF, 0xF4},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync F4)",
     true},
    {FMT_MPGA_F5,
     "mp3",
     {0xFF, 0xF5},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync F5)",
     true},
    {FMT_MPGA_F6,
     "mp3",
     {0xFF, 0xF6},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync F6)",
     true},
    {FMT_MPGA_F7,
     "mp3",
     {0xFF, 0xF7},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync F7)",
     true},
    {FMT_MPGA_FA,
     "mp3",
     {0xFF, 0xFA},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync FA)",
     true},
    {FMT_MPGA_FB,
     "mp3",
     {0xFF, 0xFB},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync FB)",
     true},
    {FMT_MPGA_FC,
     "mp3",
     {0xFF, 0xFC},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync FC)",
     true},
    {FMT_MPGA_FD,
     "mp3",
     {0xFF, 0xFD},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync FD)",
     true},
    {FMT_MPGA_FE,
     "mp3",
     {0xFF, 0xFE},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync FE)",
     true},
    {FMT_MPGA_FF,
     "mp3",
     {0xFF, 0xFF},
     2,
     0,
     512,
     512ull << 20,
     700,
     validate_mp3_raw,
     "MPEG audio (sync FF)",
     true},
    /* N-052: HEIF-family siblings (heix/mif1/msf1) - the og rewrites their
     * display label to L"HEIF" (analyser-granular-batch1 :67-80). Same
     * magic/offset/limits as FMT_HEIC (which now takes heic-proper only). */
    {FMT_HEIF, "heif", {'f', 't', 'y', 'p'}, 4, 4, 128, 512ull << 20, 750, validate_heif, "HEIF"},
    /* N-053: the og binds the IIU\0 key to the TIFF chain (census :53,
     * BruteForce::TiffAnalyzer - the TIFF/EP variant). FMT_RW2 stays as
     * the post-reject fallback (its validator gained the IIU-salvage
     * path); at an IIU position the trial order tries this row first. */
    {FMT_TIFF_EP,
     "tif",
     {'I', 'I', 'U', 0x00},
     4,
     0,
     1024,
     1ull << 30,
     850,
     validate_tiff,
     "TIFF/EP"},
    /* N-069: the og's duplicate-key row for case-insensitive formats -
     * lowercase `mscf` (census :94, policy :127). Shares validate_cab
     * with FMT_CAB; a distinct format id keeps the case-insensitive
     * pair enumerable like the og's registry. */
    {FMT_CAB_LC, "cab", {'m', 's', 'c', 'f'}, 4, 0, 36, 2ull << 30, 870, validate_cab, "Cabinet"},
    /* S-037: the og's FH11 row - key 0x11C (bytes 1C 01 00 00) ->
     * BruteForce::CFH11Analyser (decompile:1952-1961, id 0x1d;
     * the og id is display-side only). The validator mirrors the og's
     * FUN_180029f30 constant predicate exactly; min_size 0x24 covers
     * the full predicate span (+0x22..+0x23 "11" marker). */
    {FMT_FH11,
     "fh11",
     {0x1C, 0x01, 0x00, 0x00},
     4,
     0,
     0x24,
     64ull << 20,
     820,
     validate_fh11,
     "FreeHand 11"},
    /* S-037: the og's three PhotoPaint generation rows - keys CPT7/
     * CPT8/CPT9 (census :93) all -> RawSpeed::CPTAnalyser + the
     * FileCPTAnalyser fallback (id 0x11, display-side only). Duplicate-key
     * rows per the og's registry policy (same validator, distinct ids).
     * The validator gates on "FILE" @ +4 and 0x13C @ +0x34, then the
     * FUN_1800436e0 pointer-chase. */
    {FMT_CPT7,
     "cpt",
     {'C', 'P', 'T', '7'},
     4,
     0,
     0x38,
     1ull << 30,
     800,
     validate_cpt,
     "Corel PhotoPaint"},
    {FMT_CPT8,
     "cpt",
     {'C', 'P', 'T', '8'},
     4,
     0,
     0x38,
     1ull << 30,
     800,
     validate_cpt,
     "Corel PhotoPaint"},
    {FMT_CPT9,
     "cpt",
     {'C', 'P', 'T', '9'},
     4,
     0,
     0x38,
     1ull << 30,
     800,
     validate_cpt,
     "Corel PhotoPaint"},
    /* ---- S-007 niche batch 1 (SCAN-2f): every key re-read first-hand
     * at its RAWSearchDLL registration site (retdec :5450-6600, u16
     * layer :6205-6590). Duplicate-key formats keep the og's one-row-
     * per-key policy (census :127). ---- */
    /* N-093: the og's registered caps (MIDRawFileInfo :16326-16327: min 14,
     * max 10,000,000) - the 64 MiB superset is retired; the tighter cap
     * bounds false-positive extent and walk time on keyed garbage. */
    {FMT_MID, "mid", {'M', 'T', 'h', 'd'}, 4, 0, 14, 10000000, 800, validate_mid, "MIDI"},
    {FMT_OGG, "ogg", {'O', 'g', 'g', 'S'}, 4, 0, 27, 512ull << 20, 800, validate_ogg, "Ogg"},
    {FMT_RM, "rm", {'.', 'R', 'M', 'F'}, 4, 0, 0x20, 512ull << 20, 700, validate_rm, "RealMedia"},
    {FMT_MXF, "mxf", {0x06, 0x0E, 0x2B, 0x34}, 4, 0, 32, 1ull << 30, 700, validate_mxf, "MXF"},
    /* DMG: the og keys the LEVEL-1 bzip2 stream byte-exactly ("BZh1");
     * M-090: the DMGRawFileInfo ctor's OWN bound is max 0xC800000
     * (:49436-49440, first-hand) - not the 0xBEBC200 global singleton
     * (:9049) the batch-1 landing carried. The validator walks the koly
     * UDIF trailer (FUN_180053de0 :63782-63868). */
    {FMT_DMG,
     "dmg",
     {'B', 'Z', 'h', '1'},
     4,
     0,
     32,
     0xC800000ull,
     700,
     validate_dmg,
     "Apple DMG (bzip2)"},
    /* M-156: the row bound carries the og walker's OWN cap constant - the
     * modern X3F walker's [obj+0x118] = 0x6400000 (100 MiB), raw-asm at
     * the factory FUN_1800400a0 0x1800400ee (evidence:
     * diff-to-fix/evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md).
     * The 64 MiB DWG-pattern stand-in VALUE is gone from this row. */
    {FMT_X3F, "x3f", {'F', 'O', 'V', 'b'}, 4, 0, 64, 0x6400000ull, 620, validate_x3f, "Sigma X3F"},
    /* M-081 (SCAN-4): the II/MM 1A 00 keys are CANON CRW (CIFF) -
     * Brance::CRWAnalyser id 0x12 (registration :1746-1761; grammar
     * FUN_180052000 :62333-62353: 00 00 HEAP + {CCDR,TIFP,ARCH} +
     * version byte 2). The batch-1 landing mislabeled them "Minolta
     * MRW" with an invented PRD gate that rejected every real CRW
     * file. MRW is the \x00MRM key only (id 0x66, og size formula
     * FUN_180012100 :14233-14258). */
    /* M-107/S83 (the CRW member): the row bound carries the og walker's
     * OWN cap constant - the CRW child walker's r15 = 0x3200000 (50 MiB),
     * raw-asm at 0x18005219d (evidence:
     * diff-to-fix/evidence/s83-lane-sheets/s83-l2-crw-ciff-walk-decode.md).
     * The 64 MiB DWG-pattern stand-in value is gone from these rows. */
    {FMT_CRW,
     "crw",
     {'I', 'I', 0x1A, 0x00},
     4,
     0,
     512,
     0x3200000ull,
     650,
     validate_crw,
     "Canon CRW (CIFF)"},
    {FMT_CRW_MM,
     "crw",
     {'M', 'M', 0x1A, 0x00},
     4,
     0,
     512,
     0x3200000ull,
     650,
     validate_crw,
     "Canon CRW (CIFF, BE)"},
    {FMT_MRW,
     "mrw",
     {0x00, 'M', 'R', 'M'},
     4,
     0,
     512,
     64ull << 20,
     600,
     validate_mrw,
     "Minolta MRW"},
    /* WPG: the og's verified FileInfo bounds {20, 0x5F5E100} (ctor
     * :20542-20548) - the row carries them exactly */
    {FMT_WPG,
     "wpg",
     {'w', 'p', 'g', 0x00},
     4,
     0,
     20,
     0x5F5E100ull,
     620,
     validate_wpg,
     "WordPerfect Graphics"},
    {FMT_WPD,
     "wpd",
     {0xFF, 'W', 'P', 'C'},
     4,
     0,
     16,
     /* the S90 row-max rider over the S85 WPD landing: the og's analyser
      * object carries NO FileInfo row (0x10 bytes: vptr + id) and takes
      * the header's u32@+0x14 verbatim with no cap (FUN_180041f50's
      * 0x18004200c-0x180042015) - a 64 MiB row truncates every
      * over-declaring document at exactly the stand-in value the M-107
      * falsification names. The field's own 4 GiB bound (the
      * MP4/AVI/PSD header-declared house pattern) is the honest no-row
      * encoding: the declared size always fits, the device end clamps */
     4ull << 30,
     600,
     validate_wpd,
     "WordPerfect Document"},
    /* N-093: the og's registered caps (PSPRawFileInfo :17373-17378: min 36,
     * max 20,000,000) - the S-007 bounded-extent note now cites the true
     * values instead of implying the og bounds were unknown. */
    {FMT_PSP, "psp", {'P', 'a', 'i', 'n'}, 4, 0, 36, 20000000, 650, validate_psp, "Paint Shop Pro"},
    /* N-093: the og's registered caps (SITRawFileInfo :16886-16892: min
     * 20,480, max 200,000,000) - a real StuffIt archive is never smaller
     * than the 20 KiB floor; the 16-byte superset is retired. */
    {FMT_SIT,
     "sit",
     {'S', 't', 'u', 'f'},
     4,
     0,
     20480,
     2000000000ull, /* M-107 member 9 (SIT): the og walk's OWN cap - the
                       child's 200,000,000 max scaled by ten (the asm's
                       lea (%rax,%rax,4)+add at 0x180015e77-7b, the
                       force-stamp boundary); the 64 MiB stand-in gone.
                       The min keeps the ctor's 20480 floor (N-093). */
     620,
     validate_sit,
     "StuffIt Archive"},
    /* N-093: the og's registered caps (GZRawFileInfo :15824-15829: min 20,
     * max 400,000,000) - min 18 -> 20 and the 512 MiB superset retired. */
    {FMT_GZ, "gz", {0x1F, 0x8B}, 2, 0, 20, 400000000, 600, validate_gz, "gzip"},
    {FMT_APE,
     "ape",
     {'M', 'A', 'C', ' '},
     4,
     0,
     32,
     256ull << 20,
     620,
     validate_ape,
     "Monkey's Audio"},
    {FMT_AU, "au", {'.', 's', 'n', 'd'}, 4, 0, 24, 256ull << 20, 800, validate_au, "Sun Audio"},
    {FMT_AIFF, "aiff", {'F', 'O', 'R', 'M'}, 4, 0, 12, 2ull << 30, 750, validate_aiff, "AIFF/IFF"},
    /* SCAN-4 (the M-081 twin): DB A5 2D 00 / 94 A6 2E 00 =
     * FileLowDocAnalyser id 0x17 (the old-Word .doc family;
     * formula-only validator FUN_18001b650 :21167-21188). HLP is
     * 3F 5F 03 00 only (id 0x20, og size gate FUN_180041c50
     * :49883-49895). The batch-1 landing keyed the LowDoc keys as
     * "Windows Help". */
    {FMT_LOWDOC,
     "doc",
     {0xDB, 0xA5, 0x2D, 0x00},
     4,
     0,
     32,
     64ull << 20,
     600,
     validate_lowdoc,
     "Word Document (legacy .doc)"},
    {FMT_LOWDOC2,
     "doc",
     {0x94, 0xA6, 0x2E, 0x00},
     4,
     0,
     32,
     64ull << 20,
     600,
     validate_lowdoc,
     "Word Document (legacy .doc, 2nd key)"},
    {FMT_HLP,
     "hlp",
     {0x3F, 0x5F, 0x03, 0x00},
     4,
     0,
     32,
     64ull << 20,
     600,
     validate_hlp,
     "Windows Help"},
    {FMT_ICO, "ico", {0x00, 0x00, 0x01, 0x00}, 4, 0, 22, 64ull << 20, 700, validate_ico, "Icon"},
    {FMT_CUR, "cur", {0x00, 0x00, 0x02, 0x00}, 4, 0, 22, 64ull << 20, 700, validate_ico, "Cursor"},
    {FMT_WMF_PL,
     "wmf",
     {0xD7, 0xCD, 0xC6, 0x9A},
     4,
     0,
     40,
     64ull << 20,
     750,
     validate_wmf_placeable,
     "Windows Metafile (placeable)"},
    {FMT_WMF,
     "wmf",
     {0x01, 0x00, 0x09, 0x00},
     4,
     0,
     18,
     64ull << 20,
     700,
     validate_wmf_std,
     "Windows Metafile"},
    {FMT_TTF,
     "ttf",
     {0x00, 0x01, 0x00, 0x00},
     4,
     0,
     28,
     128ull << 20,
     700,
     validate_ttf,
     "TrueType Font"},
    /* MDB: the og's verified bounds {0x1000, 0xBEBC200} (ctor :21316) */
    {FMT_MDB,
     "mdb",
     {0x00, 0x01, 0x00, 0x00},
     4,
     0,
     0x1000,
     0xBEBC200ull,
     650,
     validate_mdb,
     "Access Database"},
    /* TAR: the list-layer probe - magic at +257, the F8 late-magic rule
     * needs min_size > 257+32 (1024 = the two-block floor) */
    {FMT_TAR,
     "tar",
     {'u', 's', 't', 'a', 'r'},
     5,
     257,
     1024,
     8ull << 30,
     750,
     validate_tar,
     "TAR Archive"},
    /* M-107 members 7/8/6: the row maxes carry the og walks' own cap
     * constants - EPS: the CZeroEnd child's singleton+0x70 x 100 (the
     * ctor FUN_180032c50's [obj+0x110]); AI: the tier-1 formula's cap
     * (the same singleton x 100); DXF: the RAW singleton field
     * 0xbebc200 = 200 MB (FUN_1800379c0's cap check, no multiplier).
     * The 64 MiB stand-in values are gone from these rows. */
    {FMT_EPS, "eps", {'%', '!'}, 2, 0, 11, 0x4A817C800ull, 620, validate_eps, "PostScript"},
    {FMT_AI,
     "ai",
     {0xC5, 0xD0, 0xD3, 0xC6},
     4,
     0,
     0x1c,
     0x4A817C800ull,
     620,
     validate_ai,
     "Adobe Illustrator"},
    {FMT_DXF,
     "dxf",
     {' ', ' ', '0', '\r'},
     4,
     0,
     32,
     0xbebc200ull,
     620,
     validate_dxf,
     "AutoCAD DXF",
     /* N-152: a comment-led DXF re-matches "  0\r" strictly inside the
      * emitted comment-led stream (the SAME document - the u16 "99" key
      * claims the position first, the section header sits inside); the
      * N-151/N-153 repeating-key family rule. */
     true},
    /* N-152: the og's second DXF entry key - u16 "99" 0x3939 ->
     * BruteForce::CDXFAnalyser (RAWSearchDLL :2722-2733, the 0x18-byte
     * node stamped idx 0x19 inserted under the u16 magic map). The
     * validator's comment branch (the probe FUN_1800376f0 :42240-42290)
     * accepts the leading "999\n"/"999\r\n" comment line and scans
     * forward for the section pattern (LF form after "999\n", CRLF form
     * after "999\r\n", NUL-terminated scan). */
    {FMT_DXF_C999,
     "dxf",
     {'9', '9'},
     2,
     0,
     32,
     64ull << 20,
     620,
     validate_dxf,
     "AutoCAD DXF (999 comment)",
     /* the same-family rule from the sibling row: a canonical DXF's
      * interior "99" bytes (coordinates, comments) never re-carve inside
      * an emitted stream */
     true},
    {FMT_3GP, "3gp", {'f', 't', 'y', 'p'}, 4, 4, 128, 4ull << 30, 780, validate_3gp, "3GP Video"},
    /* ---- S-041 niche batch 2, school 1 (SCAN-4): every grammar
     * re-verified first-hand at the analyser body (the session-5 log in
     * SCAN-ENGINE-DELTA carries the decode table). The ONE row carries
     * the og ctor's OWN bounds {20, 500,000,000} (FUN_180016080
     * :17191-17209); the 3DS row carries S-041's census cap. ---- */
    {FMT_CHM,
     "chm",
     {'I', 'T', 'S', 'F'},
     4,
     0,
     32,
     64ull << 20,
     750,
     validate_chm,
     "Compiled HTML Help"},
    {FMT_ONE,
     "one",
     {0xE4, 0x52, 0x5C, 0x7B},
     4,
     0,
     20,
     500000000ull,
     620,
     validate_one,
     "OneNote Document"},
    {FMT_MNY, "mny", {'M', 'S'}, 2, 0, 32, 64ull << 20, 650, validate_mny, "Money Database"},
    /* EMF: the og probes " EMF"@+0x28 (list-layer) - the late magic
     * re-expresses that probe; F8's late-magic rule sets min 0x60. */
    {FMT_EMF,
     "emf",
     {' ', 'E', 'M', 'F'},
     4,
     0x28,
     0x60,
     64ull << 20,
     700,
     validate_emf,
     "Enhanced Metafile"},
    {FMT_SWF, "swf", {'F', 'W', 'S'}, 3, 0, 16, 64ull << 20, 700, validate_swf, "Shockwave Flash"},
    {FMT_SWF_CWS,
     "swf",
     {'C', 'W', 'S'},
     3,
     0,
     16,
     64ull << 20,
     700,
     validate_swf,
     "Shockwave Flash (compressed)"},
    {FMT_3DS, "3ds", {'M', 'M'}, 2, 0, 0x12, 0xBEBC200ull, 620, validate_3ds, "3DS Image"},
    /* the same id-0x01 chain's list-layer registration (:3223-3243) - the
     * NCSD-at-+0x100 CCI form; the late magic re-expresses the probe,
     * F8's late-magic rule sets min 0x140. */
    {FMT_3DS_NCSD,
     "3ds",
     {'N', 'C', 'S', 'D'},
     4,
     0x100,
     0x140,
     0xBEBC200ull,
     620,
     validate_3ds_ncsd,
     "3DS Image (NCSD)"},
    /* M-084: the og second Jet-family signature - "Standard ACE DB"
     * (FUN_180015260 :16515-16517, type 0x4C) on the same u16-key chain;
     * trial order keeps the og Jet-first branch order. */
    {FMT_ACCDB,
     "accdb",
     {0x00, 0x01, 0x00, 0x00},
     4,
     0,
     0x1000,
     0xBEBC200ull,
     650,
     validate_accdb,
     "Access Database (ACE)"},
    /* S-047: the og's TS sync scanners. The 1-byte 0x47 magic + the
     * follow-byte set {0x40,0x41,'P',0x10} + the strided packet walk
     * (min extent 0x5000). S-054: the cap is the og's 200 MB controller
     * global EXACTLY - the walk clamps the computed size to
     * 200000000 (RAWSearchDLL_decompiled.c:35507-35509) and the
     * size-unknown fallback assigns the same 200000000 literal
     * (:35513); the global itself is initialized at :9049
     * (+0x70 = 200000000). The former "x 10" provenance was refuted:
     * 2000000000/0x77359400 appears NOWHERE in the decompile (grep
     * verified twice by the audit lanes, re-verified first-hand at the
     * fix). suppress_overlap: every packet of a TS stream re-matches
     * the 1-byte key - the repeating-sync family (the MPEG-audio
     * precedent); mid-stream syncs inside an emitted stream are the
     * SAME stream. */
    {FMT_TS,
     "ts",
     {0x47},
     1,
     0,
     0x5000,
     200000000ull, /* the og's TS walk clamps to the 200 MB controller global */
     600,
     validate_ts,
     "MPEG Transport Stream",
     true},
    {FMT_M2TS,
     "m2ts",
     {0x47},
     1,
     4, /* the sync sits 4 bytes into the file (TP_extra_header) */
     0x5000,
     200000000ull,
     600,
     validate_m2ts,
     "AVCHD M2TS",
     true},
    /* M-092: HTML - the og's four u16 keys (one row per key, the shared
     * validator; the probe itself is case-insensitive so <h/<H/<! all
     * route the same grammar). FileInfo bounds {20, 0x100000}. */
    {FMT_HTML, "html", {'<', 'h'}, 2, 0, 20, 0x100000ull, 620, validate_html, "HTML document"},
    {FMT_HTML_H, "html", {'<', 'H'}, 2, 0, 20, 0x100000ull, 620, validate_html, "HTML document"},
    {FMT_HTML_BANG, "html", {'<', '!'}, 2, 0, 20, 0x100000ull, 620, validate_html, "HTML document"},
    {FMT_HTML_UTF16,
     "html",
     {0xFF, 0xFE},
     2,
     0,
     20,
     0x100000ull,
     620,
     validate_html,
     "HTML document"},
    /* M-093: the XML/SVG chain - four u32 keys, one shared validator.
     * N-151 (session-40 lane B): suppress_overlap on all four rows - an
     * XML-declared SVG re-matches "<svg" strictly inside the emitted
     * declaration stream (the SAME document; the og's sector-aligned
     * probe + extent consumption never re-fires inside a walked file).
     * The pre-fix double-carve (the declaration candidate AND the
     * interior direct-<svg candidate both emitted) reproduced on
     * pristine main: two nodes at 4096/4118 for one document.
     * the probe stamps FMT_XML (the "csl" generic marker) or FMT_SVG
     * (svg content) via CarveResult::format_override (the og's id is
     * content-decided, not key-decided). SVGRawFileInfo bounds
     * {0, 0x100000}. */
    {FMT_XML,
     "xml",
     {'<', '?', 'x', 'm'},
     4,
     0,
     0,
     0x100000ull,
     620,
     validate_xmlsvg,
     "XML document",
     true},
    {FMT_XML_BOM,
     "xml",
     {0xEF, 0xBB, 0xBF, 0x3C},
     4,
     0,
     0,
     0x100000ull,
     620,
     validate_xmlsvg,
     "XML document",
     true},
    {FMT_SVG_CD01,
     "svg",
     {0xCD, 0x01, 0x00, 0x00},
     4,
     0,
     0,
     0x100000ull,
     620,
     validate_xmlsvg,
     "SVG image",
     true},
    {FMT_SVG,
     "svg",
     {'<', 's', 'v', 'g'},
     4,
     0,
     0,
     0x100000ull,
     620,
     validate_xmlsvg,
     "SVG image",
     true},
    /* M-096: MOBI - the BOOKMOBI pair at +0x3C is the whole gate (the
     * full-salvage posture: the 0x3C bytes before it are never read);
     * the 512-step end-tag walk, cap 500 MB. Shipped RawConfig MOBI=1 -
     * default ON; hosts disable via the M-024 list ("mobi"). */
    {FMT_MOBI,
     "mobi",
     {'B', 'O', 'O', 'K', 'M', 'O', 'B', 'I'},
     8,
     0x3C,
     0x60, /* the F8 late-magic floor (header_offset+32) + the probe's own 0x44 read */
     0x1F400000ull,
     620,
     validate_mobi,
     "Kindle e-book"},
    /* N-111: JP2 - the og's JP2Analyser (id 0x80, the RawConfig "JP2"
     * token, list-layer :3544-3559). FMT_JP2 carries THE og scan key:
     * the "jP  " signature-box type at +4 (the box length at +0 is
     * UNCHECKED - the salvage posture, the probe never reads it). The
     * validator gates the payload/ftyp boxes and maps the brand
     * exactly (jp2 /jpx /jpm ; unknown brands reject - the og's
     * exact-match map walk :21886-21936), then walks the box structure
     * for the extent (decoded :21941-22115: BE u32 lengths; len==1 ->
     * the u64 extended form; the jp2c-consecutive rule - the first
     * non-jp2c box after a codestream box ends the file; len==0 -> the
     * FF D9 EOC scan with the 16-zero back-off, guard 0x1F3FFFFF).
     * FMT_JPX/FMT_JPM carry the BRAND-position keys (the map's own
     * discriminative keys at +0x14) so a jpx/jpm file fires its own row
     * at the same start as this row's jP key (equal score - this row
     * wins by trial order, the override stamps the brand id). */
    {FMT_JP2,
     "jp2",
     {0x6a, 0x50, 0x20, 0x20}, /* "jP  " */
     4,
     4,
     0x18,          /* the probe's own brand read (+0x14..0x18) */
     0x1F400000ull, /* the EOC-scan guard 0x1F3FFFFF rounded - 500 MB */
     750,
     validate_jp2,
     "JPEG 2000 image"},
    {FMT_JPX,
     "jpx",
     {0x6a, 0x70, 0x78, 0x20},
     4,
     0x14,
     0x40,
     0x1F400000ull,
     750,
     validate_jp2,
     "JPEG 2000 extended"},
    {FMT_JPM,
     "jpm",
     {0x6a, 0x70, 0x6d, 0x20},
     4,
     0x14,
     0x40,
     0x1F400000ull,
     750,
     validate_jp2,
     "JPEG 2000 compound"},
    /* N-112: LZH - the og's Brance::LZHAnalyser (id 0x24, list-layer,
     * RawConfig token-gated). The scan key is the probe's own gate: the
     * level-1 method field at +2 (5 bytes: "-lh?-"). The shared
     * validator runs the decoded entry-chain walk with the og's exact
     * per-method strides (-lh0/-lhd: hdr+comp+2 with the ext-flag
     * suppression; -lh5: hdr only; -lh7: hdr+comp) and the -2/+2 CRC
     * step-back chain-break (size = pos+1, the end-marker byte); cap
     * 200 MiB (the og ctor bound 0xC800000). One row per method key
     * (the M-092 duplicate-key policy); all rows share ext "lzh" - the
     * og stamps one id (0x24) whichever key fired. */
    {FMT_LZH,
     "lzh",
     {'-', 'l', 'h', '0', '-'},
     5,
     2,
     0x0B, /* the first entry's 11-byte read (hdr u8@0, method@2..6, comp u32@7) */
     0xC800000ull,
     700,
     validate_lzh,
     "LZH archive"},
    {FMT_LZH5,
     "lzh",
     {'-', 'l', 'h', '5', '-'},
     5,
     2,
     0x0B,
     0xC800000ull,
     700,
     validate_lzh,
     "LZH archive"},
    {FMT_LZHD,
     "lzh",
     {'-', 'l', 'h', 'd', '-'},
     5,
     2,
     0x0B,
     0xC800000ull,
     700,
     validate_lzh,
     "LZH archive"},
    {FMT_LZH7,
     "lzh",
     {'-', 'l', 'h', '7', '-'},
     5,
     2,
     0x0B,
     0xC800000ull,
     700,
     validate_lzh,
     "LZH archive"},
    /* N-113: the text school - the og's RawConfig-token-gated list probes.
     * The magics carry the probes' own canonical gates; the validators run
     * the decoded walks (see cr_signatures.h's N-113 block for the full
     * per-grammar decode). GPX/CHFP share the "<?xm" key with the M-093 XML
     * rows (their grammars reject each other's content; N-070 ranks the
     * accepting candidate). The case-insensitive og gates (VCF/POS/CHFP's
     * memicmp forms) are expressed by the canonical-case magic - the
     * exotic-case subset beyond the magic is a registered detection subset
     * (the M-092 HTML class). */
    {FMT_VCF,
     "vcf",
     {'B', 'E', 'G', 'I', 'N', ':', 'V', 'C', 'A', 'R', 'D'},
     11,
     0,
     0x0B,
     0x1400000ull, /* the walk's 20 MiB cap, byte-exact */
     620,
     validate_vcf,
     "vCard contacts",
     /* N-151-class (this lane's verification of the parallel N-113
      * landing): the multi-card form re-matches BEGIN:VCARD strictly
      * inside the emitted file - an inner card is the SAME file (the
      * og's sector-aligned probe + extent consumption never re-fires
      * inside a walked file; the repeating-sync family rule). The
      * parallel landing's own test checked only the outer extent; the
      * count pin (TextSchoolVcfInnerCardIsSameFile) closes this. */
     true},
    {FMT_GPX,
     "gpx",
     {'<', '?', 'x', 'm'},
     4,
     0,
     0x20,
     0x6400000ull,
     620,
     validate_gpx,
     "GPX track",
     /* N-151-class: a BOM'd file re-matches the declaration strictly
      * inside the emitted stream - the same file */
     true},
    {FMT_GPX_BOM,
     "gpx",
     {0xEF, 0xBB, 0xBF, '<'},
     4,
     0,
     0x20,
     0x6400000ull,
     620,
     validate_gpx,
     "GPX track",
     true},
    {FMT_OSP,
     "osp",
     {0x7b, 0x0d, 0x0a, 0x20, 0x22, 0x64, 0x69, 0x22, 0x3a, 0x20}, /* '{\r\n "di": ' */
     10,
     0,
     0x0A,
     0x200000ull, /* 2 MiB - the reject-on-missing family cap */
     620,
     validate_osp,
     "OpenShot project"},
    {FMT_POS,
     "pos",
     {0x7b, 0x22, 'd', 'i', 'a', 'g', 'r', 'a', 'm', 0x22, ':'}, /* '{"diagram":' */
     11,
     0,
     0x0B,
     0x6400000ull,
     620,
     validate_pos,
     "ProcessOn schema"},
    {FMT_CHFP,
     "hfp",
     {'<', '?', 'x', 'm'},
     4,
     0,
     0x40,
     0x6400000ull,
     620,
     validate_chfp,
     "HFP project"},
    /* N-115: the OfficeTagFromXML typing set - override-target rows
     * sharing the LFH key. The typing walk (validate_office_zip) stamps
     * the id by per-entry content; the FMT_DOCX row above wins the
     * registry-order tie at a shared hit, so these rows resolve the
     * effective ext/name for the override. The og's ids for the
     * non-OOXML families are undecoded (the family granularity is the
     * tag strings' own). */
    {FMT_XLSX,
     "xlsx",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     800,
     validate_office_zip,
     "Excel Workbook",
     true},
    {FMT_PPTX,
     "pptx",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     800,
     validate_office_zip,
     "PowerPoint Presentation",
     true},
    {FMT_ODT,
     "odt",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "OpenDocument Text",
     true},
    {FMT_ODS,
     "ods",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "OpenDocument Spreadsheet",
     true},
    {FMT_ODP,
     "odp",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "OpenDocument Presentation",
     true},
    {FMT_EPUB,
     "epub",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "EPUB e-book",
     true},
    {FMT_IWORK,
     "iwa",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "Apple iWork",
     true},
    {FMT_HWP,
     "hwp",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "Hangul Document",
     true},
    {FMT_CDR,
     "cdr",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "CorelDRAW Drawing",
     true},
    /* M-121: Outlook Express Dbx (RawSpeed::DbxAnalyser id 0x15) -
     * three keys, one shared validator (the og's duplicate-key
     * policy, the CAB/CPT precedent). RENUMBERED from the S41 lane's
     * M-117 filing per SA-9, then to M-121 (the parallel lostpart
     * finding consumed the M-118 mid-flight).
     * Bounds: the plain forms' size field at +0x7C sets the honest
     * floor 0x80 (the parallel lane's uniform 16 never rejects a
     * short-header candidate the read bound itself refuses); the
     * marked form's whole gate is the 16-byte pattern (floor 0x10);
     * max = the og engine default 0xBEBC200 (the user-info
     * singleton's 200,000,000 reset, the MDB family precedent - the
     * parallel lane's 0xFFFFFFFF cap surface would let the marked
     * walk run 4 GiB per candidate, far past every og bound).
     * Confidence: the GUID/JMF6 forms are exact 16-byte identity
     * matches (800); the marked form keys the pattern alone (700). */
    {FMT_DBX_JMF6,
     "dbx",
     {'J', 'M', 'F', '6'},
     4,
     0,
     0x80,
     0xBEBC200ull,
     800,
     validate_dbx,
     "Outlook Express Dbx"},
    {FMT_DBX,
     "dbx",
     {0xCF, 0xAD, 0x12, 0xFE},
     4,
     0,
     0x80,
     0xBEBC200ull,
     800,
     validate_dbx,
     "Outlook Express Dbx"},
    {FMT_DBX_MARKED,
     "dbx",
     {0xF7, 0x89, 0x34, 0x2C},
     4,
     0,
     0x10,
     0xBEBC200ull,
     700,
     validate_dbx,
     "Outlook Express Dbx"},
    /* M-098: HIK (Hikvision surveillance) - the og's CHIKAnalyser
     * (id 0x92, list-layer, RawConfig HIK=1 shipped). Keyed on the
     * MPEG-PS pack magic; the trial order tries this row BEFORE
     * FMT_MPEGPS so the private-stream discriminator claims the vendor
     * layout first and the generic stream row keeps everything else.
     * The probe's own floor: pack (14) + the BD PES head (6); the cap
     * is the og's walk cap 0x40000000 (1 GiB). Base confidence 700:
     * the N-070 accumulated-score contest (base + validator) must
     * rank the SPECIFIC private-stream discriminator (700+650=1350)
     * over the generic MPEG-PS stream row (650+650=1300) - the
     * finding's own precedence rule (HIK before MPEGPS) encoded in
     * the score model, not just the trial order. */
    {FMT_HIK,
     "hik",
     {0x00, 0x00, 0x01, 0xBA},
     4,
     0,
     20,
     0x40000000ull,
     700,
     validate_hik,
     "Hikvision recording"},
    /* M-099: AEP (After Effects project) - the og's AEPAnalyser
     * (id 0x9c, list-layer, RawConfig AEP=1 shipped). BARE RIFX key,
     * the og's own weak grammar ported AS-IS (no "Egg!" body gate -
     * any big-endian RIFF passes; the weak probe is the parity
     * target). The row floor: the RIFX head (8) the size field read
     * needs; the cap is the og's walk cap 0x1e00000 (30 MiB). */
    {FMT_AEP,
     "aep",
     {'R', 'I', 'F', 'X'},
     4,
     0,
     8,
     0x1e00000ull,
     620,
     validate_aep,
     "After Effects project"},
    /* N-158: the og's five-branch typing tail (RAWSearchDLL :53025-
     * 53100, disassembly-verified) - override-target rows sharing the
     * LFH key. The typing walk (validate_office_zip) stamps the ids on
     * entry-name/content matches; the FMT_DOCX row wins the registry-
     * order tie at a shared hit, so these rows resolve the effective
     * ext/name for the override. Bounds/floor mirror the office
     * family (the typing rides the same walk). Unlike the N-115 nine
     * these rows are NOT in carve_format_always_on: the og ships
     * [Filter] keys for exactly these five ids (XMIND/MMAP/EDDX/
     * WFP/DRP=1, RawConfig.ini :12/:15/:17/:18/:22), so the core's
     * M-024 disabled_formats surface gates them row-level (the
     * conservative landing; PARITY-DELTAS D-134). The display labels
     * are the og's own (the L"XMIND"/L"MMAP"/L"EDDX" inline label
     * stamps + the .rdata-resolved UTF-16 "WFP"/"DRP"). */
    {FMT_XMIND,
     "xmind",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "XMIND",
     true},
    {FMT_MMAP,
     "mmap",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "MMAP",
     true},
    {FMT_EDDX,
     "eddx",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "EDDX",
     true},
    {FMT_WFP,
     "wfp",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "WFP",
     true},
    {FMT_DRP,
     "drp",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     620,
     validate_office_zip,
     "DRP",
     true},
};
} // namespace

const Signature* registry() {
    return kTable;
}
size_t registry_size() {
    return sizeof(kTable) / sizeof(kTable[0]);
}

u32 validate_registry(std::vector<const Signature*>* bad) {
    u32 rejects = 0;
    std::vector<const Signature*> local;
    std::vector<const Signature*>& out = bad ? *bad : local;
    if (registry_size() != FMT_COUNT) {
        ++rejects; /* size mismatch: table corrupt */
    }
    for (size_t i = 0; i < registry_size(); ++i) {
        const Signature& s = kTable[i];
        bool ok = true;
        if (s.validator == nullptr)
            ok = false; /* F8: no header-only */
        if (s.header_len == 0 || s.header_len > 16)
            ok = false;
        /* late magics are legal when min_size covers them (ISO9660: the
         * volume descriptor signature sits at 0x8001) */
        if (s.header_offset > (1u << 20))
            ok = false;
        if (s.header_offset > 8 && s.min_size <= u64(s.header_offset) + 32)
            ok = false;
        if (s.max_size <= s.min_size)
            ok = false;
        if (s.ext == nullptr || s.name == nullptr)
            ok = false;
        if (s.base_confidence > 1000)
            ok = false;
        if (i != size_t(s.id))
            ok = false; /* order must match ids */
        if (!ok) {
            ++rejects;
            out.push_back(&s);
        }
    }
    return rejects;
}

u32 self_probe_registry(std::vector<const Signature*>* bad) {
    /* S-017 F8 self-probe: prove every signature's magic is REACHABLE
     * through the exact window-match primitive the scan uses. A synthetic
     * buffer per signature: magic at header_offset, candidate lands at
     * buffer start. This is the gate that would have caught the pre-S-017
     * BMFF entries (magic included four literal zero size-bytes - a real
     * file's 0x14/0x18/0x20 size field can never match them, but the
     * synthetic buffer faithfully carried the same zeros, so the probe
     * would have shown a hit ONLY for a degenerate size-0 box, which is
     * not what the validator accepts: the probe also asserts the
     * candidate shift puts the magic exactly at header_offset of the
     * candidate). */
    std::vector<const Signature*> local;
    std::vector<const Signature*>& out = bad ? *bad : local;
    u32 failures = 0;
    for (size_t i = 0; i < registry_size(); ++i) {
        const Signature& s = kTable[i];
        bool reached = true;
        /* structural rule (S-017): a magic whose first four bytes are all
         * zero asserts a 32-bit DATA field is a constant - the pre-S-017
         * BMFF entries baked the box SIZE into the magic this way. No real
         * format's constant magic starts with four zero bytes (a size-0
         * prefix is the degenerate to-EOF box, not a real file); reject
         * the class at load time. */
        if (s.header_len >= 4 && s.header[0] == 0 && s.header[1] == 0 && s.header[2] == 0 &&
            s.header[3] == 0)
            reached = false;
        const u64 buf_len = u64(s.header_offset) + s.header_len + 64;
        std::vector<u8> buf(size_t(buf_len), 0);
        std::memcpy(buf.data() + s.header_offset, s.header, s.header_len);
        bool synth_reached = false;
        for (u64 off = 0; off + s.header_len <= buf_len; ++off) {
            u64 cand_rel = 0;
            if (match_signature(s, buf.data(), buf_len, off, cand_rel)) {
                /* the candidate must start exactly where the synthetic
                 * file starts (buffer offset 0) - a mis-shifted magic
                 * (the pre-S-017 candidate-4 bug class) fails here too */
                if (cand_rel == 0)
                    synth_reached = true;
            }
        }
        if (!reached || !synth_reached) {
            ++failures;
            out.push_back(&s);
        }
    }
    return failures;
}

/* ---- M-024 (diff-to-fix): the per-format enable/disable surface ---- */

namespace {
bool ci_token_eq(const char* tok, size_t len, const char* want) {
    if (want == nullptr)
        return false;
    /* EXACT length: a token must equal the full ext/name - "jpe" is a
     * prefix of "JPEG" and must NOT match (the first form bounded the
     * length scan by the token and accepted prefixes). */
    size_t wl = 0;
    while (want[wl] != 0)
        ++wl;
    if (wl != len)
        return false;
    for (size_t i = 0; i < len; ++i) {
        char a = tok[i], b = want[i];
        if (a >= 'A' && a <= 'Z')
            a = char(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = char(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}
} // namespace

bool carve_token_listed(const char* disabled_list, const char* ext, const char* name) {
    if (disabled_list == nullptr || *disabled_list == 0)
        return false;
    const char* p = disabled_list;
    while (*p != 0) {
        while (*p == ',' || *p == ';' || *p == ' ' || *p == '\t')
            ++p;
        const char* tok = p;
        while (*p != 0 && *p != ',' && *p != ';' && *p != ' ' && *p != '\t')
            ++p;
        const size_t len = size_t(p - tok);
        if (len != 0) {
            if (ci_token_eq(tok, len, ext))
                return true;
            if (ci_token_eq(tok, len, name))
                return true;
        }
    }
    return false;
}

bool carve_format_listed(const char* disabled_list, const Signature& sig) {
    return carve_token_listed(disabled_list, sig.ext, sig.name);
}

/* N-149 (diff-to-fix): the og's per-search filter rebuild NEVER removes the
 * OLE/RIFF/ZIP analysers - the removal loop FUN_180006b30's keep-condition
 * carries the exempt mask 0x1000801 over id-0x30 (RAWSearchDLL :4412-4418,
 * re-read first-hand: set bits 0/11/24 -> ids 0x30 FileOLEAnalyser /
 * 0x3b FileRIFFAnalyser / 0x48 FileZIPAnalyser; the ids pinned at the
 * registry builders :2337 / :2304-2310 / :2243, all three re-read this
 * session). The core's row set splits the og's three analysers into
 * per-form rows (AVI/WAV/WEBP under the RIFF key; the office/ODF-typed
 * family and the CD-salvage anchor under the ZIP key) - every row of
 * those families is always-on through the disable surface. The raw-sign
 * LIST layer is NOT exempt: its formats sit outside the og's mask range
 * (the og's shipped [Filter] targets exactly that layer - STL/VPJ ship
 * disabled - and no raw-sign row exists for zip/avi/wav/webp/doc). */
bool carve_format_always_on(FormatId id) {
    switch (id) {
    case FMT_OLE: /* the og's 0x30 */
    case FMT_AVI:
    case FMT_WAV:
    case FMT_WEBP: /* the og's 0x3b RIFF family */
    case FMT_ZIP:
    case FMT_ZIP_CD:
    case FMT_DOCX:
    case FMT_XLSX:
    case FMT_PPTX:
    case FMT_ODT:
    case FMT_ODS:
    case FMT_ODP:
    case FMT_EPUB:
    case FMT_IWORK:
    case FMT_HWP:
    case FMT_CDR: /* the og's 0x48 ZIP family (the CD row is the
                     analyser's preferred anchor; the typed rows are its
                     in-analyser typing pass - the N-115 decode) */
        return true;
    default:
        return false;
    }
}

} // namespace carve
} // namespace cr
