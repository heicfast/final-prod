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
CarveResult validate_xmlsvg(IStorageDevice* s, u64 hit, u64 cap); /* M-093: the shared chain */

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
    {FMT_ZIP, "zip", {'P', 'K', 0x03, 0x04}, 4, 0, 128, 2ull << 30, 700, validate_zip, "ZIP"},
    {FMT_DOCX,
     "docx",
     {'P', 'K', 0x03, 0x04},
     4,
     0,
     128,
     512ull << 20,
     800,
     validate_docx,
     "Office Open XML"},
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
     "ZIP (central directory)"},
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
    {FMT_DWG, "dwg", {'A', 'C', '1', '0'}, 4, 0, 64, 64ull << 20, 620, validate_dwg, "AutoCAD DWG"},
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
    {FMT_MID, "mid", {'M', 'T', 'h', 'd'}, 4, 0, 14, 64ull << 20, 800, validate_mid, "MIDI"},
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
    {FMT_X3F, "x3f", {'F', 'O', 'V', 'b'}, 4, 0, 64, 64ull << 20, 620, validate_x3f, "Sigma X3F"},
    /* M-081 (SCAN-4): the II/MM 1A 00 keys are CANON CRW (CIFF) -
     * Brance::CRWAnalyser id 0x12 (registration :1746-1761; grammar
     * FUN_180052000 :62333-62353: 00 00 HEAP + {CCDR,TIFP,ARCH} +
     * version byte 2). The batch-1 landing mislabeled them "Minolta
     * MRW" with an invented PRD gate that rejected every real CRW
     * file. MRW is the \x00MRM key only (id 0x66, og size formula
     * FUN_180012100 :14233-14258). */
    {FMT_CRW,
     "crw",
     {'I', 'I', 0x1A, 0x00},
     4,
     0,
     512,
     64ull << 20,
     650,
     validate_crw,
     "Canon CRW (CIFF)"},
    {FMT_CRW_MM,
     "crw",
     {'M', 'M', 0x1A, 0x00},
     4,
     0,
     512,
     64ull << 20,
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
     64ull << 20,
     600,
     validate_wpd,
     "WordPerfect Document"},
    {FMT_PSP,
     "psp",
     {'P', 'a', 'i', 'n'},
     4,
     0,
     0x20,
     128ull << 20,
     650,
     validate_psp,
     "Paint Shop Pro"},
    {FMT_SIT,
     "sit",
     {'S', 't', 'u', 'f'},
     4,
     0,
     16,
     512ull << 20,
     620,
     validate_sit,
     "StuffIt Archive"},
    {FMT_GZ, "gz", {0x1F, 0x8B}, 2, 0, 18, 512ull << 20, 600, validate_gz, "gzip"},
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
    {FMT_EPS, "eps", {'%', '!'}, 2, 0, 4, 64ull << 20, 620, validate_eps, "PostScript"},
    {FMT_AI,
     "ai",
     {0xC5, 0xD0, 0xD3, 0xC6},
     4,
     0,
     8,
     16ull << 20,
     620,
     validate_ai,
     "Adobe Illustrator"},
    {FMT_DXF,
     "dxf",
     {' ', ' ', '0', '\r'},
     4,
     0,
     32,
     64ull << 20,
     620,
     validate_dxf,
     "AutoCAD DXF"},
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
     * (min extent 0x5000, cap = the 200 MB controller global x 10).
     * suppress_overlap: every packet of a TS stream re-matches the
     * 1-byte key - the repeating-sync family (the MPEG-audio
     * precedent); mid-stream syncs inside an emitted stream are the
     * SAME stream. */
    {FMT_TS,
     "ts",
     {0x47},
     1,
     0,
     0x5000,
     2000000000ull, /* the og's controller global (200 MB) x 10 */
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
     2000000000ull,
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
    /* M-093: the XML/SVG chain - four u32 keys, one shared validator;
     * the probe stamps FMT_XML (the "csl" generic marker) or FMT_SVG
     * (svg content) via CarveResult::format_override (the og's id is
     * content-decided, not key-decided). SVGRawFileInfo bounds
     * {0, 0x100000}. */
    {FMT_XML, "xml", {'<', '?', 'x', 'm'}, 4, 0, 0, 0x100000ull, 620, validate_xmlsvg,
     "XML document"},
    {FMT_XML_BOM, "xml", {0xEF, 0xBB, 0xBF, 0x3C}, 4, 0, 0, 0x100000ull, 620, validate_xmlsvg,
     "XML document"},
    {FMT_SVG_CD01, "svg", {0xCD, 0x01, 0x00, 0x00}, 4, 0, 0, 0x100000ull, 620, validate_xmlsvg,
     "SVG image"},
    {FMT_SVG, "svg", {'<', 's', 'v', 'g'}, 4, 0, 0, 0x100000ull, 620, validate_xmlsvg,
     "SVG image"},
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

} // namespace carve
} // namespace cr
