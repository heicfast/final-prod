/* cr_carve.cpp - carving pipeline (R4).
 *
 * Scan: offset-outer with a first-byte dispatch; per-hit formats are tried
 * in specificity order (DOCX before ZIP, CR2 before TIFF) and the first
 * grammar-validated format wins the candidate.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_carve.h"
#include "cr/crypto/cr_sha256.h"
#include <cstring>
#include <set>

namespace cr {
namespace carve {

namespace {
constexpr u64 WINDOW = 1ull << 20; /* 1 MiB */
constexpr u64 OVERLAP = 4096;      /* magic search continuity */
constexpr u64 MAX_FILE_READ_FOR_HASH = 8ull << 20;

/* trial order: most specific grammar first when magics coincide */
constexpr u32 kTrialOrder[] = {FMT_DOCX,   FMT_ZIP, FMT_CR2, FMT_TIFF, FMT_MOV, FMT_MP4,
                               FMT_HEIC,   FMT_JPG, FMT_PNG, FMT_WEBP, FMT_PDF, FMT_7Z,
                               FMT_SQLITE, FMT_AVI, FMT_MKV, FMT_RAR4};
constexpr u32 kTrialCount = sizeof(kTrialOrder) / sizeof(kTrialOrder[0]);

/* per-format: candidate file start for a magic found at window offset */
inline u64 file_start(const Signature& sig, u64 magic_pos) {
    return magic_pos - sig.header_offset;
}
} // namespace

std::u16string default_namer(const Signature& sig, u64 size, u32 index) {
    (void)size;
    char buf[32];
    /* File000001.jpg - zero-padded for stable ordering (corpus policy) */
    int n = snprintf(buf, sizeof(buf), "File%06u.%s", index, sig.ext);
    if (n <= 0)
        buf[0] = 0;
    std::u16string out;
    for (const char* p = buf; *p; ++p)
        out.push_back(u16(u8(*p)));
    return out;
}

u32 carve_slice(IStorageDevice* slice, const CarveFlags& flags, CancelToken* cancel,
                bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
                tree::FullNode* carve_root, const tree::ResultTree* dedup_tree,
                const NameCallback& namer, CarveStats& stats) {
    if (slice == nullptr || carve_root == nullptr)
        return ERR_BAD_PARAMETER;
    if (validate_registry(nullptr) != 0)
        return ERR_BAD_PARAMETER; /* F8 gate */

    const Signature* reg = registry();
    const u64 total = slice->size();
    u32 file_index = 0;
    std::vector<u8> win(size_t(WINDOW + OVERLAP));
    std::set<u64> emitted_at; /* file starts already emitted (window overlap) */

    for (u64 base = 0; base < total; base += WINDOW) {
        if (cancel && cancel->cancelled())
            return ERR_CANCELLED;
        if (progress && !progress(base, total, progress_user))
            return ERR_CANCELLED;
        u64 avail = total - base;
        u64 want = avail < WINDOW + OVERLAP ? avail : WINDOW + OVERLAP;
        if (want < 16)
            break;
        if (slice->read_at(base, win.data(), u32(want)) != OK) {
            stats.bytes_scanned += want;
            continue;
        }
        stats.bytes_scanned += want;

        for (u64 off = 0; off + 12 <= want; ++off) {
            const u8 c = win[off];
            /* first-byte dispatch: only formats whose magic starts with c */
            for (u32 ti = 0; ti < kTrialCount; ++ti) {
                const Signature& sig = reg[kTrialOrder[ti]];
                if (sig.header[0] != c)
                    continue;
                if (off + sig.header_len > want)
                    continue;
                if (std::memcmp(win.data() + off, sig.header, sig.header_len) != 0)
                    continue;
                /* magic found at window offset off: candidate file start */
                if (off < sig.header_offset)
                    continue; /* window head clip */
                const u64 cand_abs = base + off - sig.header_offset;
                if (cand_abs + sig.min_size > total)
                    continue;
                ++stats.hits;

                /* try THIS format's grammar; first validated format at this
                 * magic position wins (specificity order) */
                CarveResult cr = sig.validator(slice, cand_abs, sig.max_size);
                if (!cr.ok) {
                    ++stats.rejected;
                    continue;
                }
                u64 file_len = cr.end - cand_abs;
                if (file_len < sig.min_size || file_len > sig.max_size) {
                    ++stats.rejected;
                    continue;
                }
                if (emitted_at.count(cand_abs)) {
                    ++stats.duplicates;
                    break;
                }

                /* F9 dedup vs structured results + prior carved nodes */
                u64 len_class = 0;
                if (file_len >= (1 << 20)) {
                    while ((1ULL << len_class) < file_len)
                        ++len_class;
                    len_class += 64;
                } else {
                    len_class = file_len / 65536;
                }
                bool dup = false;
                if (dedup_tree && dedup_tree->find_by_extent(cand_abs, len_class))
                    dup = true;
                if (!dup && tree.find_by_extent(cand_abs, len_class))
                    dup = true;
                if (dup) {
                    ++stats.duplicates;
                    break;
                }

                tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
                if (props == nullptr)
                    return ERR_NO_MEMORY;
                props->name = namer ? namer(sig, file_len, file_index)
                                    : default_namer(sig, file_len, file_index);
                props->fs_kind = 0xFF; /* carved */
                props->logical_size = file_len;
                props->carve_format = sig.id;
                props->carve_confidence = u16((sig.base_confidence + cr.confidence) / 2);
                if (props->carve_confidence > 1000)
                    props->carve_confidence = 1000;
                props->extents.push_back({cand_abs, file_len});

                if (flags.hash_small_files && file_len <= flags.hash_threshold &&
                    file_len <= MAX_FILE_READ_FOR_HASH) {
                    std::vector<u8> content((size_t(file_len)));
                    if (slice->read_at(cand_abs, content.data(), u32(file_len)) == OK) {
                        u8 full[32];
                        crypto::Sha256::digest(content.data(), size_t(file_len), full);
                        std::memcpy(props->sha128, full, 16);
                        props->hash_valid = true;
                        ++stats.hashed;
                    }
                }
                tree::FileNode* n = tree.add_file(carve_root, props);
                props->release();
                if (n == nullptr)
                    return ERR_NO_MEMORY;
                n->set_origin(tree::Origin::Carved);
                n->set_confidence(cr.confidence >= 850 ? tree::Confidence::Excellent
                                                       : tree::Confidence::PartialRisk);
                ++stats.validated;
                ++file_index;
                emitted_at.insert(cand_abs);
                break; /* this magic position resolved */
            }
        }
    }
    if (progress)
        (void)progress(total, total, progress_user);
    return OK;
}

} // namespace carve
} // namespace cr
