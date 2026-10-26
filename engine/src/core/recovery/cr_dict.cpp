/* cr_dict.cpp - the password-dictionary unlock aid (N-064, diff-to-fix).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/recovery/cr_dict.h"
#include "cr/cr_error.h"
#include "cr/cr_log.h"

namespace cr {
namespace dict {

bool parse(const char* bytes, size_t len, std::vector<std::string>& out) {
    out.clear();
    if (bytes == nullptr && len != 0)
        return false;
    std::vector<std::string> parsed;
    parsed.reserve(kDictEntryCount);
    const char* p = bytes;
    const char* end = bytes + len;
    while (p < end) {
        /* one line: up to LF (CR stripped) */
        const char* line_begin = p;
        while (p < end && *p != '\n')
            ++p;
        const char* line_end = p;
        if (line_end > line_begin && line_end[-1] == '\r')
            --line_end;
        if (p < end)
            ++p; /* consume the LF */
        /* empty line: skip (the og's file has none; tolerate) */
        if (line_begin == line_end)
            continue;
        /* comma terminator: strip one trailing comma (the og terminates
         * every line but the last; accepting both keeps the grammar
         * total for hand-edited dictionaries) */
        if (line_end[-1] == ',')
            --line_end;
        const size_t n = size_t(line_end - line_begin);
        if (n == 0 || n > kMaxPasswordLen)
            return false; /* empty-after-comma or oversized candidate */
        for (const char* q = line_begin; q < line_end; ++q) {
            const unsigned char c = static_cast<unsigned char>(*q);
            if (c < 0x20 || c > 0x7E)
                return false; /* non-printable-ASCII: reject the blob */
        }
        if (parsed.size() >= kMaxCandidates)
            return false; /* hostile-blob bound */
        parsed.emplace_back(line_begin, n);
    }
    out = std::move(parsed);
    return true;
}

AttemptResult try_passwords(u32 (*attempt)(const char* password, void* user), void* user,
                            const std::vector<std::string>* user_candidates) {
    AttemptResult res;
    if (attempt == nullptr)
        return res;
    const size_t user_count =
        user_candidates != nullptr ? user_candidates->size() : size_t(0);
    const u32 total = u32(kDictEntryCount) + u32(user_count);
    u32 index = 0;
    /* shipped set first (the og's order), then the user candidates */
    for (u32 pass = 0; pass < 2; ++pass) {
        const bool user_pass = pass == 1;
        const size_t count = user_pass ? user_count : size_t(kDictEntryCount);
        for (size_t i = 0; i < count; ++i) {
            const char* pw = user_pass ? (*user_candidates)[i].c_str()
                                       : kDictEntries[i];
            ++index;
            log::write(log::Level::Info, "dict unlock attempt %u/%u", index, total);
            const u32 rc = attempt(pw, user);
            res.rc = rc;
            if (rc == OK) {
                res.attempted = index;
                res.success_index = index;
                res.from_user_set = user_pass;
                log::write(log::Level::Info,
                           "dict unlock: candidate %u succeeded (%s set)", index,
                           user_pass ? "user" : "shipped");
                return res;
            }
        }
    }
    res.attempted = index;
    return res;
}

} // namespace dict
} // namespace cr
