/* test_text.cpp - shared UTF-8/UTF-16 codec tests (S-005 proof set).
 *
 * The decoder is the single point every byte-name filesystem (ext4, Btrfs,
 * ZFS) funnels its on-disk names through; these vectors pin RFC 3629
 * well-formedness, the U+FFFD policy and the resync rule.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_text.h"
#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include <string_view>

namespace {

using cr::text::utf8_to_u16;
using cr::text::utf8_to_u16_lossless;
using cr::text::lossless_u16_to_utf8;

std::u16string u(std::initializer_list<char16_t> cps) {
    return std::u16string(cps.begin(), cps.end());
}

} // namespace

TEST(Text, AsciiPassesThrough) {
    EXPECT_EQ(utf8_to_u16(std::string_view("hello.txt", 9)),
              u({u'h', u'e', u'l', u'l', u'o', u'.', u't', u'x', u't'}));
    EXPECT_TRUE(utf8_to_u16(std::string_view("", 0)).empty());
}

TEST(Text, LatinAccentDecodes) {
    /* "café" - the é is U+00E9, UTF-8 C3 A9 */
    EXPECT_EQ(utf8_to_u16("caf\xc3\xa9"), u({u'c', u'a', u'f', u'\u00E9'}));
}

TEST(Text, CjkDecodesToCodePoints) {
    /* "照片" - U+7167 U+7247, UTF-8 E7 85 A7 E7 89 87 */
    EXPECT_EQ(utf8_to_u16("\xe7\x85\xa7\xe7\x89\x87"), u({char16_t(0x7167), char16_t(0x7247)}));
    /* the byte-lift used to produce U+00E7 U+0085 U+00A7 U+00E7 U+0089 U+0087 */
    EXPECT_EQ(utf8_to_u16("\xe7\x85\xa7").size(), 1u);
}

TEST(Text, AstralPlaneBecomesSurrogatePair) {
    /* U+1F600 (emoji) - UTF-8 F0 9F 98 80 -> D83D DE00 */
    EXPECT_EQ(utf8_to_u16("\xf0\x9f\x98\x80"), u({char16_t(0xD83D), char16_t(0xDE00)}));
    /* boundary: U+10000 = F0 90 80 80 -> D800 DC00 */
    EXPECT_EQ(utf8_to_u16("\xf0\x90\x80\x80"), u({char16_t(0xD800), char16_t(0xDC00)}));
}

TEST(Text, OverlongFormsRejectedAsReplacement) {
    /* N-029a maximal-subpart policy (RFC 3629 §3.1; the og's restore path -
     * MultiByteToWideChar(CP_UTF8, flags=0)): a fully-read but invalid
     * sequence is ONE U+FFFD consuming all of its bytes. C0/C1 leads are
     * RFC-excluded at classification (no well-formed sequence begins
     * with them), so their continuations surface as their own subparts. */
    EXPECT_EQ(utf8_to_u16("\xc0\x80"), u({u'\uFFFD', u'\uFFFD'})); /* NUL overlong */
    EXPECT_EQ(utf8_to_u16("\xc1\xbf"), u({u'\uFFFD', u'\uFFFD'})); /* 2-byte overlong */
    EXPECT_EQ(utf8_to_u16("\xe0\x80\x80"), u({u'\uFFFD'})); /* 3-byte overlong: one subpart */
    EXPECT_EQ(utf8_to_u16("\xf0\x80\x80\x80"), u({u'\uFFFD'})); /* 4-byte overlong: one subpart */
    /* the overlong VALUE never decodes: no U+0000 from C0 80 */
    EXPECT_EQ(utf8_to_u16("\xc0\x80").find(u'\0'), std::u16string::npos);
}

TEST(Text, SurrogateHalvesRejected) {
    /* U+D800 encoded as UTF-8 (ED A0 80) must not enter UTF-16 raw;
     * N-029a maximal-subpart: the fully-read invalid sequence is ONE
     * U+FFFD consuming all three bytes (the og's MB2WC semantics) */
    EXPECT_EQ(utf8_to_u16("\xed\xa0\x80"), u({u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("\xed\xbf\xbf"), u({u'\uFFFD'}));
    /* the last legal code point below the surrogate gap still decodes */
    EXPECT_EQ(utf8_to_u16("\xed\x9f\xbf"), u({char16_t(0xD7FF)})); /* U+D7FF */
}

TEST(Text, BeyondUnicodeRejected) {
    /* U+110000 (F4 90 80 80) and above - ONE U+FFFD for the maximal
     * subpart (N-029a; the whole well-formed-but-invalid sequence
     * consumes) */
    EXPECT_EQ(utf8_to_u16("\xf4\x90\x80\x80"), u({u'\uFFFD'}));
    /* the RFC-legal maximum U+10FFFF decodes fine */
    EXPECT_EQ(utf8_to_u16("\xf4\x8f\xbf\xbf"), u({char16_t(0xDBFF), char16_t(0xDFFF)}));
}

TEST(Text, InvalidLeadBytesResyncOneByte) {
    /* 0xFF / 0xFE / lone continuation bytes: one U+FFFD each, the rest
     * of the name survives - a torn byte must not destroy the suffix */
    EXPECT_EQ(utf8_to_u16("\xff\xfe(bad"), u({u'\uFFFD', u'\uFFFD', u'(', u'b', u'a', u'd'}));
    EXPECT_EQ(utf8_to_u16("ok\x80ok"), u({u'o', u'k', u'\uFFFD', u'o', u'k'}));
}

TEST(Text, TruncatedTailBecomesReplacement) {
    /* valid lead with missing continuations at end-of-buffer */
    EXPECT_EQ(utf8_to_u16("caf\xc3"), u({u'c', u'a', u'f', u'\uFFFD'}));
    /* N-029a maximal-subpart: a truncated sequence consumes its lead +
     * the continuations that extended it - ONE U+FFFD (the og's
     * MB2WC(CP_UTF8, flags=0) count; the pre-fix core emitted one
     * FFFD per byte) */
    EXPECT_EQ(utf8_to_u16("\xe7\x85"), u({u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("\xf0\x9f"), u({u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("\xf0\x9f\x98"), u({u'\uFFFD'}));
    /* interrupted mid-sequence: the maximal subpart is the lead byte
     * alone; the ASCII suffix decodes */
    EXPECT_EQ(utf8_to_u16("\xe7\x41"), u({u'\uFFFD', u'A'}));
}

TEST(Text, MaximalSubpartReplacement) {
    /* N-029a (the finding's named vectors): the og's restore path
     * converts names via MultiByteToWideChar(CP_UTF8, flags=0), which
     * follows the maximal-subpart policy - ONE U+FFFD per maximal
     * subpart, resuming at the offending byte. The pre-fix core emitted
     * one FFFD per bad byte ("FFFD FFFD x" for \xE6\x96x). */
    /* \xE6\x96x: E6 starts a 3-byte sequence, 96 extends it, 'x'
     * breaks it - the maximal subpart is E6 96 -> ONE FFFD, then 'x' */
    EXPECT_EQ(utf8_to_u16("\xe6\x96x"), u({u'\uFFFD', u'x'}));
    /* \xF0\x80\x80\x80: fully-read overlong sequence - ONE FFFD
     * consuming all four bytes, the 'y' suffix survives */
    EXPECT_EQ(utf8_to_u16("\xf0\x80\x80\x80y"), u({u'\uFFFD', u'y'}));
    /* truncated-tail classes at true end-of-buffer */
    EXPECT_EQ(utf8_to_u16("a\xe6\x96"), u({u'a', u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("a\xf0\x9f\x98"), u({u'a', u'\uFFFD'}));
    /* a continuation-only run: each lone continuation is its own
     * single-byte subpart (they can never begin a sequence) */
    EXPECT_EQ(utf8_to_u16("\x80\x80\x80"), u({u'\uFFFD', u'\uFFFD', u'\uFFFD'}));
    /* mixed: maximal-subpart FFFD + a valid CJK sequence after it */
    EXPECT_EQ(utf8_to_u16("\xe6\x96\xe7\x85\xa7"),
              u({u'\uFFFD', char16_t(0x7167)}));
}

TEST(Text, EmbeddedNulDecodes) {
    /* name buffers are length-prefixed on disk: embedded NULs must decode
     * (they are legal in POSIX filenames) */
    EXPECT_EQ(utf8_to_u16(std::string_view("a\0b", 3)), u({u'a', u'\0', u'b'}));
}

TEST(Text, RoundTripThroughNodeUtf8) {
    /* the pipeline contract: decode(UTF-8) -> u16 -> re-encode must yield
     * the original bytes for every well-formed input */
    const std::string cases[] = {"plain", "café", "\xe7\x85\xa7\xe7\x89\x87.jpg",
                                 "\xf0\x9f\x98\x80.png", "\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80"};
    for (const std::string& c : cases) {
        const std::u16string wide = utf8_to_u16(c);
        std::string back;
        for (size_t i = 0; i < wide.size(); ++i) {
            unsigned long cp = wide[i];
            if (wide[i] >= 0xD800 && wide[i] <= 0xDBFF && i + 1 < wide.size() &&
                wide[i + 1] >= 0xDC00 && wide[i + 1] <= 0xDFFF) {
                cp = 0x10000 + ((static_cast<unsigned long>(wide[i]) - 0xD800u) << 10) +
                     (static_cast<unsigned long>(wide[i + 1]) - 0xDC00u);
                ++i;
            }
            if (cp < 0x80)
                back.push_back(char(cp));
            else if (cp < 0x800) {
                back.push_back(char(0xC0 | (cp >> 6)));
                back.push_back(char(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                back.push_back(char(0xE0 | (cp >> 12)));
                back.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
                back.push_back(char(0x80 | (cp & 0x3F)));
            } else {
                back.push_back(char(0xF0 | (cp >> 18)));
                back.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
                back.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
                back.push_back(char(0x80 | (cp & 0x3F)));
            }
        }
        EXPECT_EQ(c, back) << "round-trip broke for " << c;
    }
}

/* ---- M-010: the lossless invalid-byte mapping (ext4/Btrfs) ------------- */

TEST(Text, InvalidByteLosslessMapping) {
    /* a Latin-1 byte that cannot start a valid sequence maps to
     * U+E000 + byte - bijective, reconstructible, never U+FFFD */
    EXPECT_EQ(u"\uE0E9", utf8_to_u16_lossless("\xE9"));
    /* valid sequences untouched */
    EXPECT_EQ(u"r\u00E9sum\u00E9.txt",
              utf8_to_u16_lossless("r\xC3\xA9sum\xC3\xA9.txt"));
    /* a torn 3-byte lead: the lead byte maps losslessly, each following
     * continuation byte fails its own lead test and maps individually */
    const std::u16string torn = utf8_to_u16_lossless("\xE2\x28\xA1");
    EXPECT_EQ(3u, torn.size());
    EXPECT_EQ(0xE0E2u, torn[0]);
    EXPECT_EQ(0x28u, torn[1]); /* '(' is valid ASCII */
    EXPECT_EQ(0xE0A1u, torn[2]);
    /* lone continuation bytes */
    EXPECT_EQ(0xE080u, utf8_to_u16_lossless("\x80")[0]);
    /* 0xF8+ lead bytes */
    EXPECT_EQ(0xE0FEu, utf8_to_u16_lossless("\xFE")[0]);
}

TEST(Text, LosslessRoundTripRecoversRawBytes) {
    /* the inverse: original name bytes come back byte-for-byte.
     * MSVC /W4 C4310 note: char is signed there, so the >0x7F test
     * bytes are spelled as their negative two's-complement values
     * (0xE9 == -0x17, 0xC3 == -0x3D, 0xA9 == -0x57) - same bytes,
     * constant fits, no truncation warning. */
    const char raw[] = {'r', char(-0x17), 's', 'u', 'm', char(-0x17), '.', 't', 'x', 't'};
    const std::u16string mapped =
        utf8_to_u16_lossless(std::string_view(raw, sizeof(raw)));
    std::string back;
    ASSERT_TRUE(lossless_u16_to_utf8(mapped, back));
    EXPECT_EQ(0, memcmp(raw, back.data(), sizeof(raw)));
    /* mixed: valid UTF-8 + raw bytes round-trip together */
    const char mixed[] = {'a', char(-0x3D), char(-0x57), 'b', char(-0x17), 'c'};
    const std::u16string m2 =
        utf8_to_u16_lossless(std::string_view(mixed, sizeof(mixed)));
    std::string b2;
    ASSERT_TRUE(lossless_u16_to_utf8(m2, b2));
    EXPECT_EQ(0, memcmp(mixed, b2.data(), sizeof(mixed)));
    /* a foreign private-use unit (never emitted by the decoder) is
     * refused by the inverse */
    const std::u16string foreign = u"\uF8FF";
    std::string b3;
    EXPECT_FALSE(lossless_u16_to_utf8(foreign, b3));
}

TEST(Text, ZfsDecoderStaysReplacement) {
    /* M-010 pin: the ZFS-side decoder keeps U+FFFD (the og ZFS engine
     * converts via MultiByteToWideChar(CP_UTF8, flags=0) - FUN_18001c630
     * :24995+). The engine asymmetry is deliberate og parity; guards
     * against someone "unifying" the decoders. */
    EXPECT_EQ(u"\uFFFD", utf8_to_u16("\xE9"));
    EXPECT_EQ(u"r\uFFFDsum\uFFFD.txt", utf8_to_u16("r\xE9sum\xE9.txt"));
}

/* ---- N-009c: wide host argument -> UTF-8 (the CLI --filter seam) ------- */

TEST(Text, WideToUtf8EncodesNonAscii) {
    /* "日本" - U+65E5 U+672C. On a 32-bit wchar_t host each unit is a
     * codepoint; on Windows (16-bit) the same two units arrive from
     * wmain. Either way the UTF-8 bytes must be E6 97 A5 E6 9C AC -
     * the exact byte sequence the engine-side decoders produce/consume
     * (the query layer compares filters against name_utf8() bytes). */
    const std::wstring wide = {wchar_t(0x65E5), wchar_t(0x672C)};
    EXPECT_EQ(std::string("\xE6\x97\xA5\xE6\x9C\xAC"), cr::text::wide_to_utf8(wide));
    /* ASCII passes through untouched */
    EXPECT_EQ(std::string("report.docx"), cr::text::wide_to_utf8(L"report.docx"));
    /* accented Latin: U+00E9 -> C3 A9 */
    const std::wstring wide_e9 = {wchar_t(0x00E9)};
    EXPECT_EQ(std::string("\xC3\xA9"), cr::text::wide_to_utf8(wide_e9));
    /* the old narrow() polarity: non-ASCII units must NEVER become '?'
     * (0x3F) - that mangling is the defect N-009c fixes */
    EXPECT_EQ(std::string::npos, cr::text::wide_to_utf8(wide).find('?'));
}

TEST(Text, WideToUtf8SurrogatePairAndLoneHalf) {
    /* U+1F600 as an explicit surrogate pair (D83D DE00) - the exact
     * units a Windows wmain argv carries for an emoji filter */
    const std::wstring pair = {wchar_t(0xD83D), wchar_t(0xDE00)};
    EXPECT_EQ(std::string("\xF0\x9F\x98\x80"), cr::text::wide_to_utf8(pair));
    /* lone halves each collapse to one U+FFFD (EF BF BD) - the shared
     * text policy, consistent with utf8_to_u16's maximal-subpart rule */
    EXPECT_EQ(std::string("\xEF\xBF\xBD"), cr::text::wide_to_utf8(std::wstring{wchar_t(0xD83D)}));
    EXPECT_EQ(std::string("\xEF\xBF\xBD\xEF\xBF\xBD"),
              cr::text::wide_to_utf8(std::wstring({wchar_t(0xD83D), wchar_t(0xD83D)})));
    /* lead followed by a non-trailing unit: no pair, both halves FFFD'd */
    EXPECT_EQ(std::string("a\xEF\xBF\xBD" "b"),
              cr::text::wide_to_utf8(std::wstring({wchar_t(L'a'), wchar_t(0xD83D), wchar_t(L'b')})));
}

TEST(Text, CliFilterUtf8MatchesDecodedNames) {
    /* the N-009c CLI seam end-to-end at the byte level: a node name
     * "写真_日本.docx" reaches the tree as UTF-16 (engines decode via
     * utf8_to_u16); the CLI filter for "日本" arrives wide. The filter
     * must match the name's UTF-8 form (name_utf8()) - before the fix,
     * narrow() turned the filter into "??" and never matched. */
    const std::string name_on_disk = "\xE5\x86\x99\xE7\x9C\x9F_\xE6\x97\xA5\xE6\x9C\xAC.docx";
    const std::u16string name_u16 = cr::text::utf8_to_u16(name_on_disk);
    const std::wstring filter_wide = {wchar_t(0x65E5), wchar_t(0x672C)};

    /* name_utf8() = the u16 name re-encoded to UTF-8; the round trip
     * through utf8_to_u16 must be stable for well-formed input */
    const std::string name_u8 = cr::text::wide_to_utf8(
        std::wstring(name_u16.begin(), name_u16.end()));
    EXPECT_EQ(name_on_disk, name_u8);

    /* the filter's UTF-8 is a byte substring of the name's UTF-8: the
     * exact comparison the CLI performs (ASCII-case-insensitive; the
     * non-ASCII bytes fold as themselves) */
    const std::string filter_u8 = cr::text::wide_to_utf8(filter_wide);
    EXPECT_NE(std::string::npos, name_u8.find(filter_u8));
    /* and the mangled-'?' form the old narrow() produced does NOT match:
     * pinning the polarity so the defect cannot return silently */
    EXPECT_EQ(std::string::npos, name_u8.find("??"));
}
