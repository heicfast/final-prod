/*
 * cr_text.h - shared Unicode text codecs (pure, no locale).
 *
 * S-005 (diff-to-fix): ext4/Btrfs/ZFS store filenames as UTF-8 bytes; the
 * reference passes them through byte-identically (EXT4SearchDLL imports no
 * conversion API; .rsf name fields are UTF-8 - rsf-format--scan-session.md
 * :326), so the core must DECODE them to its internal UTF-16 rather than
 * byte-lift (the byte-lift turned every non-ASCII name into mojibake, e.g.
 * "é" -> U+00C3 U+00A9, re-encoded as double-UTF-8 by the report layer).
 *
 * Policy (shared with cr_ui's u8_to_u16 and the report escaper): never
 * locale-dependent; invalid input never throws away the rest of the string -
 * each ILL-FORMED MAXIMAL SUBPART becomes ONE U+FFFD (RFC 3629 §3.1's
 * maximal-subpart formulation; MultiByteToWideChar(CP_UTF8, flags=0)
 * semantics - the og's restore path, DRWUI :264664-694/:357529). A
 * truncated or interrupted multi-byte sequence consumes its lead byte plus
 * every continuation byte that extended it; a fully-read but invalid
 * sequence (overlong / surrogate half / beyond U+10FFFF) consumes all of
 * it; bytes that can never begin a well-formed sequence (C0/C1 per RFC
 * 3629, lone continuations 80..BF, F8..FF) are single-byte subparts.
 * Overlong forms, UTF-16 surrogate halves and code points above U+10FFFF are
 * invalid per RFC 3629 and decode as U+FFFD.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TEXT_H_
#define CR_TEXT_H_

#include "cr_types.h"
#include <string>
#include <string_view>

namespace cr {
namespace text {

/* Append one Unicode codepoint to a UTF-16 string (surrogate pair >= U+10000).
 * Values above U+10FFFF clamp to U+FFFD so names can never carry invalid
 * UTF-16 code units. */
inline void append_codepoint(std::u16string& out, u32 cp) {
    if (cp > 0x10FFFFu)
        cp = 0xFFFDu;
    if (cp >= 0x10000u) {
        cp -= 0x10000u;
        out.push_back(u16(0xD800u + (cp >> 10)));
        out.push_back(u16(0xDC00u + (cp & 0x3FFu)));
    } else {
        out.push_back(u16(cp));
    }
}

/* Strict UTF-8 -> UTF-16 decode. Accepts exactly the RFC 3629 well-formed
 * set; every ill-formed maximal subpart yields ONE U+FFFD (N-029a:
 * maximal-subpart resync, matching the og's restore path - Windows
 * MultiByteToWideChar(CP_UTF8, flags=0) - instead of one FFFD per bad
 * byte; a torn byte in the middle of a name still must not destroy the
 * suffix). */
inline std::u16string utf8_to_u16(std::string_view in) {
    std::u16string out;
    out.reserve(in.size());
    const u8* p = reinterpret_cast<const u8*>(in.data());
    const u8* end = p + in.size();
    while (p < end) {
        u32 cp = 0;
        u32 extra = 0;
        if (*p < 0x80u) {
            cp = *p;
        } else if (*p >= 0xC2u && *p <= 0xDFu) {
            /* C0/C1 excluded at classification (RFC 3629: no well-formed
             * sequence begins with them) - they fall to the single-byte
             * subpart branch below */
            cp = *p & 0x1Fu;
            extra = 1;
        } else if ((*p & 0xF0u) == 0xE0u) {
            cp = *p & 0x0Fu;
            extra = 2;
        } else if ((*p & 0xF8u) == 0xF0u) {
            cp = *p & 0x07u;
            extra = 3;
        } else {
            /* C0/C1, lone continuations 80..BF, F8..FF: the maximal
             * subpart is the offending byte alone */
            append_codepoint(out, 0xFFFDu);
            ++p;
            continue;
        }
        u32 good = 0; /* continuation bytes that extended the subpart */
        for (u32 i = 1; i <= extra; ++i) {
            if (p + i >= end || (p[i] & 0xC0u) != 0x80u)
                break;
            cp = (cp << 6) | (p[i] & 0x3Fu);
            ++good;
        }
        bool ok = good == extra;
        /* RFC 3629: reject overlong forms and surrogate halves */
        if (ok &&
            ((extra == 1 && cp < 0x80u) || (extra == 2 && cp < 0x800u) ||
             (extra == 3 && cp < 0x10000u) || (cp >= 0xD800u && cp <= 0xDFFFu) || cp > 0x10FFFFu))
            ok = false;
        if (ok) {
            append_codepoint(out, cp);
            p += 1 + extra;
        } else {
            /* maximal-subpart resync (N-029a): ONE U+FFFD per maximal
             * subpart. A truncated/interrupted sequence consumes the lead
             * + the continuations that extended it; a fully-read but
             * invalid sequence (overlong/surrogate/range) consumes all */
            append_codepoint(out, 0xFFFDu);
            p += 1 + (good == extra ? extra : good);
        }
    }
    return out;
}

/* M-010 (diff-to-fix): LOSSLESS variant for the raw-byte engines (ext4,
 * Btrfs). The og's EXT4/Btrfs DLLs import no byte<->wide conversion at
 * all (EXT4SearchDLL_imports.txt / BtrfsSearchDLL_imports.txt - re-verified
 * M-010 session: zero hits), so whatever bytes the directory entry carries
 * reach the og's session verbatim. A name that is not valid UTF-8 must
 * therefore stay RECONSTRUCTIBLE in the core too: every byte that cannot
 * start (or continue) a valid sequence maps to U+E000 + byte (the
 * private-use plane; MUTF-8-style lossless mapping). The mapping is a
 * bijection - original_byte = unit - 0xE000 - so the on-disk name bytes
 * round-trip: no information is destroyed.
 *
 * The split surface IS the og parity: ZFS's og engine converts every name
 * with MultiByteToWideChar(CP_UTF8, flags=0) and bakes U+FFFD
 * (ZfsSearchDLL FUN_18001c630 :24995+), so ZFS keeps utf8_to_u16
 * (replacement) - the engine asymmetry is deliberate and pinned by test.
 *
 * Do NOT "clean" private-use units downstream (session/report/CLI must
 * pass them through; the round-trip pin test guards this). */
inline std::u16string utf8_to_u16_lossless(std::string_view in) {
    std::u16string out;
    out.reserve(in.size());
    const u8* p = reinterpret_cast<const u8*>(in.data());
    const u8* end = p + in.size();
    while (p < end) {
        u32 cp = 0;
        u32 extra = 0;
        if (*p < 0x80u) {
            cp = *p;
        } else if ((*p & 0xE0u) == 0xC0u) {
            cp = *p & 0x1Fu;
            extra = 1;
        } else if ((*p & 0xF0u) == 0xE0u) {
            cp = *p & 0x0Fu;
            extra = 2;
        } else if ((*p & 0xF8u) == 0xF0u) {
            cp = *p & 0x07u;
            extra = 3;
        } else {
            cp = 0xE000u + *p; /* lone continuation / 0xF8+: raw byte */
        }
        bool ok = true;
        for (u32 i = 1; ok && i <= extra; ++i) {
            if (p + i >= end || (p[i] & 0xC0u) != 0x80u)
                ok = false;
            else
                cp = (cp << 6) | (p[i] & 0x3Fu);
        }
        /* RFC 3629: reject overlong forms and surrogate halves */
        if (ok &&
            ((extra == 1 && cp < 0x80u) || (extra == 2 && cp < 0x800u) ||
             (extra == 3 && cp < 0x10000u) || (cp >= 0xD800u && cp <= 0xDFFFu) || cp > 0x10FFFFu))
            ok = false;
        if (!ok && extra == 0 && cp >= 0xE000u && cp <= 0xE0FFu) {
            /* a raw-byte mapping: emit as-is (the branch above set it) */
        } else if (!ok) {
            /* a torn multi-byte sequence: keep the LEAD byte losslessly
             * and resync; the continuation bytes map individually below
             * (each fails its own lead test -> U+E000 + byte) */
            cp = 0xE000u + *p;
            extra = 0;
        }
        append_codepoint(out, cp);
        p += 1 + extra;
    }
    return out;
}

/* M-010: recover the original raw bytes from a lossless-mapped name
 * (inverse of utf8_to_u16_lossless). Valid codepoints re-encode as
 * UTF-8; private-use units in [U+E000, U+E0FF] return their byte.
 * Returns false if the string contains any other private-use unit
 * (not produced by the lossless decoder). */
inline bool lossless_u16_to_utf8(const std::u16string& in, std::string& out) {
    out.clear();
    out.reserve(in.size());
    for (u16 u : in) {
        if (u >= 0xE000u && u <= 0xE0FFu) {
            out.push_back(char(u8(u - 0xE000u)));
            continue;
        }
        if (u > 0xE0FFu && u <= 0xF8FFu)
            return false; /* private-use units the decoder never emits */
        u32 cp = u;
        if (cp < 0x80u) {
            out.push_back(char(u8(cp)));
        } else if (cp < 0x800u) {
            out.push_back(char(u8(0xC0u | (cp >> 6))));
            out.push_back(char(u8(0x80u | (cp & 0x3Fu))));
        } else if (cp < 0x10000u) {
            out.push_back(char(u8(0xE0u | (cp >> 12))));
            out.push_back(char(u8(0x80u | ((cp >> 6) & 0x3Fu))));
            out.push_back(char(u8(0x80u | (cp & 0x3Fu))));
        } else {
            out.push_back(char(u8(0xF0u | (cp >> 18))));
            out.push_back(char(u8(0x80u | ((cp >> 12) & 0x3Fu))));
            out.push_back(char(u8(0x80u | ((cp >> 6) & 0x3Fu))));
            out.push_back(char(u8(0x80u | (cp & 0x3Fu))));
        }
    }
    return true;
}

/* N-016b (N-029 residual, s17): does this decoded name carry lossless
 * raw-byte mappings (the U+E000..U+E0FF band)? Such names MUST carry
 * their original on-disk bytes in props->name_raw - the band is exactly
 * where "mapped invalid byte" and "legitimately-encoded private-use
 * codepoint" become ambiguous, so the raw bytes (not the inverse map)
 * are the only faithful reconstruction. */
inline bool name_has_raw_band(const std::u16string& name) {
    for (u16 u : name)
        if (u >= 0xE000u && u <= 0xE0FFu)
            return true;
    return false;
}

/* N-009c (diff-to-fix): wide host argument -> UTF-8, locale-independent.
 * The CLI --filter argument arrives from wmain as wide characters (UTF-16
 * units on Windows hosts, plain codepoints on 32-bit wchar_t hosts) and
 * must match names compared as UTF-8 (the query layer's name_substring
 * contract: "UTF-8, ASCII-case-insensitive"). The previous app-side
 * narrow() replaced every non-ASCII unit with '?', so any non-ASCII
 * filter never matched anything. Lone surrogate halves encode as U+FFFD
 * (the shared text policy); well-formed pairs encode their codepoint;
 * values above U+10FFFF clamp to U+FFFD so the output is always
 * well-formed UTF-8. */
inline std::string wide_to_utf8(const std::wstring& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        u32 cp = u32(u64(in[i])); /* zero-extend any wchar_t width */
        if (cp >= 0xD800u && cp <= 0xDBFFu && i + 1 < in.size() && u32(in[i + 1]) >= 0xDC00u &&
            u32(in[i + 1]) <= 0xDFFFu) {
            cp = 0x10000u + ((cp - 0xD800u) << 10) + (u32(in[i + 1]) - 0xDC00u);
            ++i;
        } else if (cp >= 0xD800u && cp <= 0xDFFFu) {
            cp = 0xFFFDu; /* lone half - one U+FFFD per the shared policy */
        }
        if (cp > 0x10FFFFu)
            cp = 0xFFFDu;
        if (cp < 0x80u) {
            out.push_back(char(u8(cp)));
        } else if (cp < 0x800u) {
            out.push_back(char(u8(0xC0u | (cp >> 6))));
            out.push_back(char(u8(0x80u | (cp & 0x3Fu))));
        } else if (cp < 0x10000u) {
            out.push_back(char(u8(0xE0u | (cp >> 12))));
            out.push_back(char(u8(0x80u | ((cp >> 6) & 0x3Fu))));
            out.push_back(char(u8(0x80u | (cp & 0x3Fu))));
        } else {
            out.push_back(char(u8(0xF0u | (cp >> 18))));
            out.push_back(char(u8(0x80u | ((cp >> 12) & 0x3Fu))));
            out.push_back(char(u8(0x80u | ((cp >> 6) & 0x3Fu))));
            out.push_back(char(u8(0x80u | (cp & 0x3Fu))));
        }
    }
    return out;
}

} // namespace text
} // namespace cr

#endif // CR_TEXT_H_
