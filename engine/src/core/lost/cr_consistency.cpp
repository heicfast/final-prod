/* cr_consistency.cpp - the M-035 content-consistency scorer implementation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/lost/cr_consistency.h"
#include "cr/carve/cr_signatures.h"
#include <cstring>

namespace cr {
namespace lost {

namespace {

/* Uppercase ASCII projection of a UTF-16 name (the og operates on ASCII
 * names; non-ASCII code units can never match an ASCII family extension
 * and are collapsed to a separator so adjacent segments cannot fuse into
 * a false substring). */
std::string ascii_upper(const std::u16string& name) {
    std::string out;
    out.reserve(name.size());
    for (char16_t c : name) {
        if (c >= u'a' && c <= u'z')
            out.push_back(char(c - u'a' + u'A'));
        else if (c < 0x80)
            out.push_back(char(c));
        else
            out.push_back(' ');
    }
    return out;
}

bool ext_equals_ci(const std::string& a, const char* b) {
    size_t n = std::strlen(b);
    if (a.size() != n)
        return false;
    for (size_t i = 0; i < n; ++i) {
        char ca = a[i];
        if (ca >= 'a' && ca <= 'z')
            ca = char(ca - 'a' + 'A');
        if (ca != b[i])
            return false;
    }
    return true;
}

/* The checkable vocabulary: the carve registry's extension families. Built
 * once (const registry, stable for the process lifetime). */
const std::vector<std::string>& checkable_exts() {
    static const std::vector<std::string> exts = [] {
        std::vector<std::string> v;
        const carve::Signature* reg = carve::registry();
        const size_t n = carve::registry_size();
        v.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            std::string e = reg[i].ext;
            for (char& c : e)
                if (c >= 'a' && c <= 'z')
                    c = char(c - 'a' + 'A');
            v.push_back(std::move(e));
        }
        return v;
    }();
    return exts;
}

constexpr u32 kProbeBytes = 512; /* every registry magic fits (max
                                    header_offset + header_len <= 512) */

} // namespace

std::string consistency_extension(const std::u16string& name) {
    /* last '.' in the ASCII projection (the og uppercases the name first,
     * then takes the extension after the last dot - FUN_180025d60). */
    const std::string up = ascii_upper(name);
    const size_t dot = up.rfind('.');
    if (dot == std::string::npos)
        return {};
    return up.substr(dot + 1);
}

bool consistency_extension_checkable(const std::string& ext_upper) {
    if (ext_upper.empty())
        return false;
    /* the og's two filter-set specials (a .TIFF/.JPEG name is checkable
     * even though the vocabulary spells the families TIF/JPG) */
    if (ext_upper == "TIFF" || ext_upper == "JPEG")
        return true;
    for (const std::string& e : checkable_exts())
        if (e == ext_upper)
            return true;
    return false;
}

bool consistency_name_matches_family(const std::u16string& name, const std::string& ext_upper,
                                     const char* sig_ext_upper) {
    const std::string up = ascii_upper(name);
    /* (a) SUBSTRING: the family extension appears inside the name
     * (FUN_180024fe0 :30843-30854). */
    if (std::strstr(up.c_str(), sig_ext_upper) != nullptr)
        return true;
    /* (b) the MOV/MP4/JPG family specials (:30858-30920): a name whose
     * own extension is JPG or JPEG matches those three families. */
    if (ext_equals_ci(ext_upper, "JPG") || ext_equals_ci(ext_upper, "JPEG")) {
        if (ext_equals_ci(std::string(sig_ext_upper), "MOV") ||
            ext_equals_ci(std::string(sig_ext_upper), "MP4") ||
            ext_equals_ci(std::string(sig_ext_upper), "JPG"))
            return true;
    }
    return false;
}

ConsistencyVerdict score_content_consistency(IStorageDevice* slice, const ConsistencyFile* files,
                                             size_t count) {
    ConsistencyVerdict v;
    if (slice == nullptr || files == nullptr || count == 0)
        return v;
    const u64 vol_bytes = slice->size();
    std::vector<u8> win(kProbeBytes);
    std::string ext_upper;

    for (size_t i = 0; i < count; ++i) {
        const ConsistencyFile& f = files[i];
        if (f.is_directory)
            continue; /* the og's 0x10-attribute skip (iVar14) */
        ++v.considered;
        if (f.name == nullptr || !f.has_extent)
            continue; /* considered, but neither filterable nor unfilterable
                       * (the og's start_cluster<2 entries) */
        ext_upper = consistency_extension(*f.name);
        if (!consistency_extension_checkable(ext_upper))
            continue; /* unfilterable (the og's FUN_180025d60 == false) */
        ++v.filterable;

        /* the og's out-of-bounds invalidation (:31077-31083): cluster math
         * pointing past the volume end means the candidate's geometry is
         * structurally impossible - invalidate immediately. */
        if (f.data_start >= vol_bytes) {
            v.out_of_bounds = true;
            v.rejected = true;
            return v;
        }
        /* probe the data start: one bounded read; failures = no signature
         * (the og's sig map has no entry at unreadable offsets). */
        const u32 want =
            u32(kProbeBytes < vol_bytes - f.data_start ? kProbeBytes : vol_bytes - f.data_start);
        if (want == 0 || slice->read_at(f.data_start, win.data(), want) != OK)
            continue;
        /* classify: the FIRST registry signature whose magic matches at the
         * data start (registry order = the carve dispatch precedence - the
         * same order the carve layer itself classifies hits in). */
        const carve::Signature* hit = nullptr;
        const size_t n = carve::registry_size();
        const carve::Signature* reg = carve::registry();
        for (size_t s = 0; s < n; ++s) {
            u64 cand_rel = 0;
            /* the magic sits at header_offset inside the file: the window
             * offset of the first magic byte IS sig.header_offset */
            if (carve::match_signature(reg[s], win.data(), want, reg[s].header_offset, cand_rel)) {
                hit = &reg[s];
                break;
            }
        }
        if (hit == nullptr)
            continue; /* no signature at the data start (the og's -1) */
        std::string sig_ext = hit->ext;
        for (char& c : sig_ext)
            if (c >= 'a' && c <= 'z')
                c = char(c - 'a' + 'A');
        if (consistency_name_matches_family(*f.name, ext_upper, sig_ext.c_str())) {
            ++v.matched;
            v.consistent = true;
        }
    }
    if (v.filterable != 0)
        v.ratio = double(v.matched) / double(v.filterable);
    /* the og reject rule (:31152-31155): considered/2 <= filterable &&
     * ratio < 0.3 (the decompile's `<= 0.3 && != 0.3` is strict <) &&
     * filterable != 0. */
    if (v.filterable != 0 && (v.considered >> 1) <= v.filterable && v.ratio < 0.3)
        v.rejected = true;
    return v;
}

} // namespace lost
} // namespace cr
