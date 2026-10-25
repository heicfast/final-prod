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

namespace {
const Signature kTable[] = {
    {FMT_JPG, "jpg", {0xFF, 0xD8, 0xFF}, 3, 0, 512, 96ull << 20, 900, validate_jpeg, "JPEG"},
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
     {'F', 'U', 'J', 'I', 'F', 'I', 'L', 'M', 'C', 'C', 'D', '-', 'R', 'A', 'W', ' '},
     16,
     0,
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

} // namespace carve
} // namespace cr
