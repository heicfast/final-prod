/* cr_usersigns.cpp - the user-extensible signature database (M-048).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_usersigns.h"
#include "cr/cr_error.h"
#include <cctype>
#include <cstring>
#include <mutex>

namespace cr {
namespace carve {
namespace usersigns {

namespace {

/* ---- a token reader for the og's JSON subset ------------------------
 * EUUserData is a flat object: string keys -> objects of
 * { "signs": [ "offset,HEX", ... ], "mime": "..." }. The reader handles
 * whitespace, strings with the two escapes the og's file uses (\\ and \"),
 * arrays, objects, and the punctuation - enough for this shape without a
 * general JSON dependency. Anything else is ERR_BAD_PARAMETER (honest
 * rejection, never a guess). */
struct Reader {
    const char* p;
    const char* end;
    u32 err = 0;

    void skip_ws() {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
            ++p;
    }
    bool peek(char c) {
        skip_ws();
        return p < end && *p == c;
    }
    bool eat(char c) {
        if (!peek(c))
            return false;
        ++p;
        return true;
    }
    /* read a JSON string into out (cap: cap chars + NUL; longer = format error) */
    bool read_string(char* out, size_t cap) {
        skip_ws();
        if (p >= end || *p != '"')
            return fail();
        ++p;
        size_t n = 0;
        while (p < end && *p != '"') {
            char c = *p++;
            if (c == '\\') {
                if (p >= end)
                    return fail();
                c = *p++;
                if (c != '\\' && c != '"')
                    return fail(); /* only the og's two escapes */
            }
            if (n + 1 > cap)
                return fail();
            out[n++] = c;
        }
        if (p >= end)
            return fail();
        ++p; /* closing quote */
        out[n] = 0;
        return true;
    }
    bool fail() {
        if (err == 0)
            err = ERR_BAD_PARAMETER;
        return false;
    }
};

bool hex_nibble(char c, u8& v) {
    if (c >= '0' && c <= '9') {
        v = u8(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        v = u8(c - 'a' + 10);
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        v = u8(c - 'A' + 10);
        return true;
    }
    return false;
}

/* "offset,HEX" -> UserSign fields; validates every bound */
bool parse_sign(const char* s, UserSign& us) {
    const char* comma = std::strchr(s, ',');
    if (comma == nullptr || comma == s)
        return false;
    u64 off = 0;
    if (comma - s > 4)
        return false; /* offset <= 1024 is four digits (S-039: the og's max is 526) */
    for (const char* q = s; q < comma; ++q) {
        if (!std::isdigit(unsigned(*q)))
            return false;
        off = off * 10 + u64(*q - '0');
    }
    if (off > kMaxOffset)
        return false;
    const char* hex = comma + 1;
    const size_t n = std::strlen(hex);
    if (n < 2 || n > kMaxSignBytes * 2 || (n & 1) != 0)
        return false;
    for (size_t i = 0; i < n; i += 2) {
        u8 hi, lo;
        if (!hex_nibble(hex[i], hi) || !hex_nibble(hex[i + 1], lo))
            return false;
        us.bytes[i / 2] = u8((hi << 4) | lo);
    }
    us.offset = u32(off);
    us.len = u16(n / 2);
    return true;
}

bool valid_ext(const char* e) {
    const size_t n = std::strlen(e);
    if (n == 0 || n > kMaxExtLen)
        return false;
    for (const char* q = e; *q; ++q) {
        char c = *q;
        if (!std::isalnum(unsigned(c)))
            return false;
    }
    return true;
}

} // namespace

u32 parse(const char* bytes, size_t len, UserSignSet& out) {
    out.signs.clear();
    out.valid = false;
    if (bytes == nullptr || len == 0)
        return ERR_BAD_PARAMETER;
    Reader r{bytes, bytes + len, 0};
    if (!r.eat('{'))
        return ERR_BAD_PARAMETER;
    bool first = true;
    while (!r.peek('}')) {
        if (!first && !r.eat(','))
            return ERR_BAD_PARAMETER;
        first = false;
        char ext[kMaxExtLen + 8] = {};
        if (!r.read_string(ext, sizeof(ext) - 1))
            return r.err ? r.err : ERR_BAD_PARAMETER;
        if (!r.eat(':'))
            return ERR_BAD_PARAMETER;
        for (char* q = ext; *q; ++q)
            *q = char(std::tolower(unsigned(*q)));
        if (!valid_ext(ext))
            return ERR_BAD_PARAMETER;
        if (!r.eat('{'))
            return ERR_BAD_PARAMETER;
        char mime[kMaxMimeLen + 8] = {};
        u32 sign_count = 0;
        bool have_signs = false;
        bool f2 = true;
        while (!r.peek('}')) {
            if (!f2 && !r.eat(','))
                return ERR_BAD_PARAMETER;
            f2 = false;
            char key[16] = {};
            if (!r.read_string(key, sizeof(key) - 1))
                return ERR_BAD_PARAMETER;
            if (!r.eat(':'))
                return ERR_BAD_PARAMETER;
            if (std::strcmp(key, "signs") == 0) {
                if (have_signs || !r.eat('['))
                    return ERR_BAD_PARAMETER;
                have_signs = true;
                bool f3 = true;
                while (!r.peek(']')) {
                    if (!f3 && !r.eat(','))
                        return ERR_BAD_PARAMETER;
                    f3 = false;
                    if (out.signs.size() >= kMaxEntries)
                        return ERR_BAD_PARAMETER;
                    if (sign_count >= kMaxSignsPerEntry)
                        return ERR_BAD_PARAMETER;
                    char sig[8 + 1 + kMaxSignBytes * 2 + 8] = {}; /* "offset," + 96 hex digits max */
                    if (!r.read_string(sig, sizeof(sig) - 1))
                        return ERR_BAD_PARAMETER;
                    UserSign us;
                    std::strcpy(us.ext, ext);
                    std::strcpy(us.mime, mime); /* filled below when mime follows */
                    if (!parse_sign(sig, us))
                        return ERR_BAD_PARAMETER;
                    out.signs.push_back(us);
                    ++sign_count;
                }
                if (!r.eat(']'))
                    return ERR_BAD_PARAMETER;
                if (sign_count == 0)
                    return ERR_BAD_PARAMETER; /* an entry with no signs is dead config */
            } else if (std::strcmp(key, "mime") == 0) {
                if (mime[0] != 0 || !r.read_string(mime, kMaxMimeLen))
                    return ERR_BAD_PARAMETER;
            } else {
                return ERR_BAD_PARAMETER; /* unknown key: strict, the og's shape only */
            }
        }
        if (!r.eat('}') || !have_signs)
            return ERR_BAD_PARAMETER;
        /* the og writes mime after signs; apply it to this ext's signs */
        for (auto& s : out.signs)
            if (std::strcmp(s.ext, ext) == 0 && s.mime[0] == 0 && mime[0] != 0)
                std::strcpy(s.mime, mime);
    }
    if (!r.eat('}'))
        return ERR_BAD_PARAMETER;
    r.skip_ws();
    if (r.p != r.end)
        return ERR_BAD_PARAMETER; /* trailing garbage: honest rejection */
    out.valid = !out.signs.empty();
    return out.valid ? OK : ERR_BAD_PARAMETER;
}

/* ---- the process-global registry ---- */

namespace {
std::mutex g_mu;
UserSignSet g_active;
} // namespace

u32 install_set(UserSignSet&& set) {
    if (!set.valid)
        return ERR_BAD_PARAMETER;
    std::lock_guard<std::mutex> lock(g_mu);
    g_active = std::move(set);
    return OK;
}

u32 active_count() {
    std::lock_guard<std::mutex> lock(g_mu);
    return u32(g_active.signs.size());
}

u32 snapshot(UserSignSet& out) {
    std::lock_guard<std::mutex> lock(g_mu);
    out = g_active;
    return OK;
}

const UserSign* match_at(const u8* p, u64 avail) {
    std::lock_guard<std::mutex> lock(g_mu);
    for (const auto& s : g_active.signs) {
        if (u64(s.offset) + s.len > avail)
            continue;
        if (std::memcmp(p + s.offset, s.bytes, s.len) == 0)
            return &s;
    }
    return nullptr;
}

} // namespace usersigns
} // namespace carve
} // namespace cr
