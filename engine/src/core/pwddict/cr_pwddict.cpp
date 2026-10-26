/* cr_pwddict.cpp - the password-dictionary unlock aid (N-064).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/pwddict/cr_pwddict.h"
#include "cr/cr_error.h"
#include "cr/cr_log.h"
#include <unordered_set>

namespace cr {
namespace pwddict {

size_t parse(const char* data, size_t size, std::vector<std::string>& out) {
    if (data == nullptr || size == 0 || size > kMaxFileBytes)
        return 0;
    const size_t before = out.size();
    size_t line_start = 0;
    for (size_t i = 0; i <= size; ++i) {
        /* the final segment (i == size) is the og's unterminated last
         * entry; every other segment ends at LF */
        if (i != size && data[i] != '\n')
            continue;
        size_t line_end = i;
        /* strip one CR of a CRLF pair */
        if (line_end > line_start && data[line_end - 1] == '\r')
            --line_end;
        /* strip the og's per-line comma terminator (optional: the file's
         * final entry carries none; hand-edited files may drop it) */
        if (line_end > line_start && data[line_end - 1] == ',')
            --line_end;
        if (line_end > line_start && line_end - line_start <= kMaxEntryChars &&
            out.size() - before < kMaxEntries) {
            out.emplace_back(data + line_start, line_end - line_start);
        }
        /* empty lines / over-cap entries: skipped, not fatal (the og's
         * file has none; a hostile one cannot poison the loop) */
        line_start = i + 1;
    }
    return out.size() - before;
}

const std::vector<std::string>& seed_dictionary() {
    /* D-78: authored for the core - universal common-password patterns
     * that appear in every public leaked-credential corpus. Deliberately
     * NOT the og's 228-entry asset (never copy proprietary data files);
     * hosts wanting that exact list load it with parse() - the format is
     * byte-compatible. */
    static const std::vector<std::string> seed = {
        "123456",     "1234567",      "12345678",  "123456789", "1234567890",
        "password",   "password1",    "password123", "qwerty",   "qwertyuiop",
        "111111",     "000000",       "666666",    "888888",    "abc123",
        "abcd1234",   "iloveyou",     "letmein",   "admin",     "admin123",
    };
    return seed;
}

u32 try_passwords(const std::vector<std::string>& candidates, AttemptFn attempt,
                  void* user, size_t* attempts_made) {
    if (attempt == nullptr)
        return ERR_FVE_LOCKED;
    if (attempts_made != nullptr)
        *attempts_made = 0;

    /* order-preserving dedup: a repeated candidate is a guaranteed
     * re-reject and every attempt is a real unlock round trip */
    std::vector<const std::string*> queue;
    queue.reserve(candidates.size());
    std::unordered_set<std::string> seen;
    seen.reserve(candidates.size() * 2 + 1);
    for (const std::string& c : candidates) {
        if (c.empty())
            continue;
        if (seen.insert(c).second)
            queue.push_back(&c);
    }
    if (queue.empty())
        return ERR_FVE_LOCKED;

    size_t done = 0;
    for (const std::string* c : queue) {
        ++done;
        /* NEVER log the password text - progress + length only (the
         * run-log is world-readable in the og's own EPMLOG flows) */
        log::write(log::Level::Info,
                   "BitLocker password attempt %zu/%zu (candidate length %zu)",
                   done, queue.size(), c->size());
        const u32 rc = attempt(*c, user);
        if (attempts_made != nullptr)
            *attempts_made = done;
        if (rc == OK) {
            log::write(log::Level::Info,
                       "BitLocker password attempt %zu/%zu succeeded", done,
                       queue.size());
            return OK;
        }
    }
    log::write(log::Level::Warn,
               "BitLocker password unlock exhausted: %zu candidate(s) rejected",
               queue.size());
    return ERR_FVE_LOCKED;
}

} // namespace pwddict
} // namespace cr
