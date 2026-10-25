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
