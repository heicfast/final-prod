/* cr_preview_text_exts.cpp - the TxtViewFiles text-preview eligibility
 * gate (M-108, diff-to-fix).
 *
 * Corpus: the og gates text preview by EXTENSION MEMBERSHIP in
 * config/FilterFolder.ini [Filter] TxtViewFiles - decompile-verified
 * first-hand (DRWUI_decompiled.c:535858 and :535937, the two consumers):
 * the ini value loads once (thread-safe static; the ini-absent default
 * is ".txt;"), the candidate's cached extension comes from the node
 * (FUN_140089050), and the membership test is the toupper-folded
 * delimited substring search FUN_140310aa0 (case-INSENSITIVE - the
 * toupper fold is in the loop, read verbatim). Content is never sniffed
 * for eligibility: a whitelisted extension renders as text whatever its
 * bytes, a non-whitelisted extension never does, and an extensionless
 * name fails the consumers' empty-needle early return (not viewable).
 *
 * The pool is generated (scripts/gen_preview_text_exts.py) - the og's
 * value verbatim, 97 tokens / 92 unique case-folded. Matching here
 * splits on the ';' delimiters and folds ASCII case - the same accept
 * set as the og's ";ext;" substring probe for every well-formed entry
 * (all pool entries are dot-prefixed and non-empty; duplicates are
 * harmless).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/preview/cr_preview.h"

#include <cstring>
#include <string_view>

namespace cr {
namespace preview {
namespace textexts {

namespace {

/* ASCII tolower fold (the pool is pure ASCII - verified at generation;
 * the og's FUN_140310aa0 folds with toupper, same equivalence class) */
inline char fold(char c) {
    return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c;
}

} // namespace

/* Is this dot-prefixed extension (e.g. ".txt") a text-preview format?
 * ASCII case-insensitive membership in the generated TxtViewFiles pool.
 * An empty or non-ASCII extension is never eligible (the og's empty
 * needle early-returns; the pool carries ASCII extensions only). */
bool eligible(std::string_view dot_ext) {
    if (dot_ext.empty() || dot_ext[0] != '.')
        return false;
    /* reject non-ASCII: the pool cannot match (the og's byte-wise
     * toupper fold is ASCII-only for the < 0x80 entries in the pool) */
    for (char c : dot_ext)
        if (u8(c) >= 0x80u)
            return false;

    const std::string_view pool(reinterpret_cast<const char*>(data::kTxtViewPool),
                                data::kTxtViewPoolLen);
    size_t pos = 0;
    while (pos < pool.size()) {
        const size_t semi = pool.find(';', pos);
        const size_t end = (semi == std::string_view::npos) ? pool.size() : semi;
        if (end > pos) {
            const std::string_view entry = pool.substr(pos, end - pos);
            if (entry.size() == dot_ext.size()) {
                bool eq = true;
                for (size_t i = 0; i < entry.size(); ++i)
                    if (fold(entry[i]) != fold(dot_ext[i])) {
                        eq = false;
                        break;
                    }
                if (eq)
                    return true;
            }
        }
        if (semi == std::string_view::npos)
            break;
        pos = semi + 1;
    }
    return false;
}

} // namespace textexts
} // namespace preview
} // namespace cr
