/* cr_registry.cpp - the signature table + load-time validation (F8 gate).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_signatures.h"
#include "cr/storage/cr_device.h"

namespace cr {
namespace carve {

/* defined in cr_analysers.cpp */
CarveResult validate_jpeg(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_png(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_tiff(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_cr2(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_webp(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_heic(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_pdf(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_zip(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_docx(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_7z(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_sqlite(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_mp4(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_mov(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_avi(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_mkv(IStorageDevice *s, u64 hit, u64 cap);
CarveResult validate_rar4(IStorageDevice *s, u64 hit, u64 cap);

namespace {
const Signature kTable[] = {
    {FMT_JPG, "jpg", {0xFF, 0xD8, 0xFF}, 3, 0, 512, 96ull << 20, 900, validate_jpeg, "JPEG"},
    {FMT_PNG, "png", {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}, 8, 0, 64, 64ull << 20,
     950, validate_png, "PNG"},
    {FMT_TIFF, "tif", {'I', 'I', 0x2A, 0x00}, 4, 0, 512, 256ull << 20, 800, validate_tiff,
     "TIFF"},
    {FMT_CR2, "cr2", {'I', 'I', 0x2A, 0x00}, 4, 0, 512, 256ull << 20, 850, validate_cr2,
     "Canon RAW 2"},
    {FMT_WEBP, "webp", {'R', 'I', 'F', 'F'}, 4, 0, 64, 128ull << 20, 800, validate_webp,
     "WebP"},
    {FMT_HEIC, "heic", {0, 0, 0, 0, 'f', 't', 'y', 'p'}, 8, 4, 128, 512ull << 20, 750,
     validate_heic, "HEIC"},
    {FMT_PDF, "pdf", {'%', 'P', 'D', 'F', '-'}, 5, 0, 128, 512ull << 20, 900, validate_pdf,
     "PDF"},
    {FMT_ZIP, "zip", {'P', 'K', 0x03, 0x04}, 4, 0, 128, 2ull << 30, 700, validate_zip, "ZIP"},
    {FMT_DOCX, "docx", {'P', 'K', 0x03, 0x04}, 4, 0, 128, 512ull << 20, 800, validate_docx,
     "Office Open XML"},
    {FMT_7Z, "7z", {'7', 'z', 0xBC, 0xAF, 0x27, 0x1C}, 6, 0, 64, 8ull << 30, 850,
     validate_7z, "7-Zip"},
    {FMT_SQLITE, "sqlite", {'S', 'Q', 'L', 'i', 't', 'e', ' ', 'f', 'o', 'r', 'm', 'a'}, 12,
     0, 4096, 2ull << 30, 850, validate_sqlite, "SQLite"},
    {FMT_MP4, "mp4", {0, 0, 0, 0, 'f', 't', 'y', 'p'}, 8, 4, 128, 4ull << 30, 750,
     validate_mp4, "MP4"},
    {FMT_MOV, "mov", {0, 0, 0, 0, 'f', 't', 'y', 'p'}, 8, 4, 128, 4ull << 30, 700,
     validate_mov, "QuickTime"},
    {FMT_AVI, "avi", {'R', 'I', 'F', 'F'}, 4, 0, 64, 4ull << 30, 700, validate_avi, "AVI"},
    {FMT_MKV, "mkv", {0x1A, 0x45, 0xDF, 0xA3}, 4, 0, 128, 16ull << 30, 700, validate_mkv,
     "Matroska"},
    {FMT_RAR4, "rar", {'R', 'a', 'r', '!', 0x1A, 0x07, 0x00}, 7, 0, 128, 4ull << 30, 800,
     validate_rar4, "RAR4"},
};
} // namespace

const Signature *registry() { return kTable; }
size_t registry_size() { return sizeof(kTable) / sizeof(kTable[0]); }

u32 validate_registry(std::vector<const Signature *> *bad) {
    u32 rejects = 0;
    std::vector<const Signature *> local;
    std::vector<const Signature *> &out = bad ? *bad : local;
    if (registry_size() != FMT_COUNT) {
        ++rejects; /* size mismatch: table corrupt */
    }
    for (size_t i = 0; i < registry_size(); ++i) {
        const Signature &s = kTable[i];
        bool ok = true;
        if (s.validator == nullptr) ok = false;             /* F8: no header-only */
        if (s.header_len == 0 || s.header_len > 12) ok = false;
        if (s.header_offset > 8) ok = false;
        if (s.min_size == 0 || s.max_size <= s.min_size) ok = false;
        if (s.ext == nullptr || s.name == nullptr) ok = false;
        if (s.base_confidence > 1000) ok = false;
        if (i != size_t(s.id)) ok = false; /* order must match ids */
        if (!ok) {
            ++rejects;
            out.push_back(&s);
        }
    }
    return rejects;
}

} // namespace carve
} // namespace cr
