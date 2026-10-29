/* cr_usersigns.cpp - the user-extensible signature database (M-048).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_usersigns.h"
#include "cr/carve/cr_signatures.h" /* S-056: carve_token_listed - the same token matcher the raw-sign layer uses */
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
    /* M-129: skip an unknown member's VALUE - the og's member loop acts
     * only on _stricmp(name,"signs") and silently falls through every
     * other member (DRWUI:1005818), so a future-versioned or hand-edited
     * EUUserData with extra members loads there and must load here.
     * Walks strings (escape-aware, no storage cap), balanced
     * objects/arrays, and bare literals; unterminated structures still
     * fail (a torn file is not og-loadable either - the og's JSON layer
     * would fail it). */
    bool skip_value() {
        skip_ws();
        if (p >= end)
            return fail();
        if (*p == '"') {
            ++p;
            while (p < end && *p != '"') {
                if (*p == '\\' && p + 1 < end)
                    ++p; /* the escaped char itself */
                ++p;
            }
            if (p >= end)
                return fail(); /* unterminated string */
            ++p;
            return true;
        }
        if (*p == '{' || *p == '[') {
            const char open = *p;
            const char close = (open == '{') ? '}' : ']';
            int depth = 0;
            while (p < end) {
                const char c = *p;
                if (c == '"') { /* strings inside the structure */
                    ++p;
                    while (p < end && *p != '"') {
                        if (*p == '\\' && p + 1 < end)
                            ++p;
                        ++p;
                    }
                    if (p >= end)
                        return fail();
                } else if (c == open) {
                    ++depth;
                } else if (c == close) {
                    --depth;
                    if (depth == 0) {
                        ++p;
                        return true;
                    }
                }
                ++p;
            }
            return fail(); /* unterminated structure */
        }
        /* bare literal: number / true / false / null */
        const char* start = p;
        while (p < end && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\t' &&
               *p != '\r' && *p != '\n')
            ++p;
        return p != start ? true : fail();
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

/* "offset,HEX" -> UserSign fields. M-129 tri-state: SIGN_OK / SIGN_SKIP
 * (malformed-but-tolerated: the og's split-count and hex tolerances - a
 * sign that doesn't split into exactly 2 comma tokens is SKIPPED at
 * DRWUI:1005986, odd-length/non-hex loads but never matches there -
 * skipping is equivalent-in-effect, registered D-126) / SIGN_REJECT
 * (over-cap: the S-039 caps doctrine, deliberate strictness - a
 * past-bound sign still fails the whole file, HostileBlobStillAtomic). */
enum class SignParse { Ok, Skip, Reject };

SignParse parse_sign(const char* s, UserSign& us) {
    const char* comma = std::strchr(s, ',');
    if (comma == nullptr || comma == s)
        return SignParse::Skip; /* not exactly 2 tokens / empty offset */
    u64 off = 0;
    if (comma - s > 4)
        return SignParse::Reject; /* offset past 9999: over the cap, not malformed */
    for (const char* q = s; q < comma; ++q) {
        if (!std::isdigit(unsigned(*q)))
            return SignParse::Skip; /* the og's atoi would decode garbage; we skip */
        off = off * 10 + u64(*q - '0');
    }
    if (off > kMaxOffset)
        return SignParse::Reject; /* over-cap: the S-039 doctrine */
    const char* hex = comma + 1;
    const size_t n = std::strlen(hex);
    if (n == 0 || (n & 1) != 0)
        return SignParse::Skip; /* odd/empty hex: dead entry in the og, skipped here */
    if (n > kMaxSignBytes * 2)
        return SignParse::Reject; /* over-cap: the S-039 doctrine */
    for (size_t i = 0; i < n; i += 2) {
        u8 hi, lo;
        if (!hex_nibble(hex[i], hi) || !hex_nibble(hex[i + 1], lo))
            return SignParse::Skip; /* non-hex chars: dead entry in the og */
        us.bytes[i / 2] = u8((hi << 4) | lo);
    }
    us.offset = u32(off);
    us.len = u16(n / 2);
    return SignParse::Ok;
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
            /* M-129: the og's member dispatch is _stricmp (DRWUI:1005818)
             * - "SIGNS"/"Signs" act like "signs", and every other
             * member is silently ignored (its value skipped). */
            auto key_ieq = [&](const char* known) {
                for (const char *a = key, *b = known; *a || *b; ++a, ++b)
                    if (std::tolower(unsigned(*a)) != std::tolower(unsigned(*b)))
                        return false;
                return true;
            };
            if (key_ieq("signs")) {
                if (have_signs || !r.eat('['))
                    return ERR_BAD_PARAMETER;
                have_signs = true;
                bool f3 = true;
                bool saw_any = false;
                while (!r.peek(']')) {
                    if (!f3 && !r.eat(','))
                        return ERR_BAD_PARAMETER;
                    f3 = false;
                    saw_any = true;
                    if (out.signs.size() >= kMaxEntries)
                        return ERR_BAD_PARAMETER; /* the S-039 cap */
                    if (sign_count >= kMaxSignsPerEntry)
                        return ERR_BAD_PARAMETER; /* the S-039 cap */
                    char sig[8 + 1 + kMaxSignBytes * 2 + 8] =
                        {}; /* "offset," + 96 hex digits max */
                    if (!r.read_string(sig, sizeof(sig) - 1))
                        return ERR_BAD_PARAMETER;
                    UserSign us;
                    std::strcpy(us.ext, ext);
                    std::strcpy(us.mime, mime); /* filled below when mime follows */
                    /* M-129: the og's tolerance - a malformed sign string
                     * (wrong token count, odd hex, non-hex chars) is
                     * SKIPPED and parsing continues (DRWUI:1005986's
                     * goto-next); only the S-039 over-cap shapes still
                     * reject the whole file. */
                    const SignParse sp = parse_sign(sig, us);
                    if (sp == SignParse::Reject)
                        return ERR_BAD_PARAMETER;
                    if (sp == SignParse::Ok) {
                        out.signs.push_back(us);
                        ++sign_count;
                    }
                }
                if (!r.eat(']'))
                    return ERR_BAD_PARAMETER;
                if (!saw_any)
                    return ERR_BAD_PARAMETER; /* a syntactically-empty array
                                                  is dead config (the og's
                                                  split loop runs zero times
                                                  - we refuse the shape); an
                                                  all-skipped array (every
                                                  sign malformed) is the og's
                                                  dead-entry tolerance and
                                                  contributes nothing */
            } else if (key_ieq("mime")) {
                if (mime[0] != 0 || !r.read_string(mime, kMaxMimeLen))
                    return ERR_BAD_PARAMETER;
            } else {
                /* M-129: unknown member - the og's member loop falls
                 * through silently; skip its value and continue (a
                 * future-versioned file loads, the member ignored) */
                if (!r.skip_value())
                    return r.err ? r.err : ERR_BAD_PARAMETER;
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
    /* M-129: the return code is the STRUCTURAL verdict - a sound file
     * whose every sign was malformed-but-tolerated loads (the og's
     * dead-entry semantics); out.valid stays the install gate (an empty
     * set cannot be installed, the consumers print their own truth). */
    out.valid = !out.signs.empty();
    return OK;
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

u32 clear() {
    std::lock_guard<std::mutex> lock(g_mu);
    g_active = UserSignSet{}; /* valid=false, no signs */
    return OK;
}

const UserSign* match_at(const u8* p, u64 avail, const char* disabled_exts) {
    std::lock_guard<std::mutex> lock(g_mu);
    for (const auto& s : g_active.signs) {
        if (u64(s.offset) + s.len > avail)
            continue;
        /* S-056: the format toggle gates this layer like every other
         * carve source - the og's FUN_180006b30 removal-loop semantics
         * (M-024): a disabled analyser's signs never fire; signs of
         * OTHER extensions at the same position still can (the removal
         * happens in the registry, not at the probe). */
        if (disabled_exts != nullptr && *disabled_exts != 0 &&
            carve_token_listed(disabled_exts, s.ext, nullptr))
            continue;
        if (std::memcmp(p + s.offset, s.bytes, s.len) == 0)
            return &s;
    }
    return nullptr;
}

} // namespace usersigns
} // namespace carve
} // namespace cr
