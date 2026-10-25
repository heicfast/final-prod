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
#include <string_view>

namespace {

using cr::text::utf8_to_u16;

std::u16string u(std::initializer_list<char16_t> cps) {
    return std::u16string(cps.begin(), cps.end());
}

} // namespace

TEST(Text, AsciiPassesThrough) {
    EXPECT_EQ(utf8_to_u16(std::string_view("hello.txt", 9)), u({u'h', u'e', u'l', u'l',
                                                                u'o', u'.', u't', u'x', u't'}));
    EXPECT_TRUE(utf8_to_u16(std::string_view("", 0)).empty());
}

TEST(Text, LatinAccentDecodes) {
    /* "café" - the é is U+00E9, UTF-8 C3 A9 */
    EXPECT_EQ(utf8_to_u16("caf\xc3\xa9"), u({u'c', u'a', u'f', u'\u00E9'}));
}

TEST(Text, CjkDecodesToCodePoints) {
    /* "照片" - U+7167 U+7247, UTF-8 E7 85 A7 E7 89 87 */
    EXPECT_EQ(utf8_to_u16("\xe7\x85\xa7\xe7\x89\x87"),
              u({char16_t(0x7167), char16_t(0x7247)}));
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
    /* the resync rule is per lead byte (cr_ui + report-escaper policy): a
     * rejected sequence's continuations are re-scanned and each surfaces
     * as its own replacement - one FFFD per bad byte */
    EXPECT_EQ(utf8_to_u16("\xc0\x80"), u({u'\uFFFD', u'\uFFFD'})); /* NUL overlong */
    EXPECT_EQ(utf8_to_u16("\xc1\xbf"), u({u'\uFFFD', u'\uFFFD'})); /* 2-byte overlong */
    EXPECT_EQ(utf8_to_u16("\xe0\x80\x80"), u({u'\uFFFD', u'\uFFFD', u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("\xf0\x80\x80\x80"),
              u({u'\uFFFD', u'\uFFFD', u'\uFFFD', u'\uFFFD'}));
    /* the overlong VALUE never decodes: no U+0000 from C0 80 */
    EXPECT_EQ(utf8_to_u16("\xc0\x80").find(u'\0'), std::u16string::npos);
}

TEST(Text, SurrogateHalvesRejected) {
    /* U+D800 encoded as UTF-8 (ED A0 80) must not enter UTF-16 raw;
     * per-byte resync: the rejected lead + each continuation = FFFD */
    EXPECT_EQ(utf8_to_u16("\xed\xa0\x80"), u({u'\uFFFD', u'\uFFFD', u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("\xed\xbf\xbf"), u({u'\uFFFD', u'\uFFFD', u'\uFFFD'}));
    /* the last legal code point below the surrogate gap still decodes */
    EXPECT_EQ(utf8_to_u16("\xed\x9f\xbf"), u({char16_t(0xD7FF)})); /* U+D7FF */
}

TEST(Text, BeyondUnicodeRejected) {
    /* U+110000 (F4 90 80 80) and above - reject + per-byte resync */
    EXPECT_EQ(utf8_to_u16("\xf4\x90\x80\x80"),
              u({u'\uFFFD', u'\uFFFD', u'\uFFFD', u'\uFFFD'}));
    /* the RFC-legal maximum U+10FFFF decodes fine */
    EXPECT_EQ(utf8_to_u16("\xf4\x8f\xbf\xbf"), u({char16_t(0xDBFF), char16_t(0xDFFF)}));
}

TEST(Text, InvalidLeadBytesResyncOneByte) {
    /* 0xFF / 0xFE / lone continuation bytes: one U+FFFD each, the rest
     * of the name survives - a torn byte must not destroy the suffix */
    EXPECT_EQ(utf8_to_u16("\xff\xfe(bad"),
              u({u'\uFFFD', u'\uFFFD', u'(', u'b', u'a', u'd'}));
    EXPECT_EQ(utf8_to_u16("ok\x80ok"), u({u'o', u'k', u'\uFFFD', u'o', u'k'}));
}

TEST(Text, TruncatedTailBecomesReplacement) {
    /* valid lead with missing continuations at end-of-buffer */
    EXPECT_EQ(utf8_to_u16("caf\xc3"), u({u'c', u'a', u'f', u'\uFFFD'}));
    /* a partial sequence resyncs one byte past the bad lead, so the
     * already-consumed continuation bytes each surface as their own
     * replacement (cr_ui policy; matches the report escaper's resync) */
    EXPECT_EQ(utf8_to_u16("\xe7\x85"), u({u'\uFFFD', u'\uFFFD'}));
    EXPECT_EQ(utf8_to_u16("\xf0\x9f"), u({u'\uFFFD', u'\uFFFD'}));
    /* interrupted mid-sequence: the continuation is not consumed by the
     * next decode step (resync lands on the continuation byte itself) */
    EXPECT_EQ(utf8_to_u16("\xe7\x41"), u({u'\uFFFD', u'A'}));
}

TEST(Text, EmbeddedNulDecodes) {
    /* name buffers are length-prefixed on disk: embedded NULs must decode
     * (they are legal in POSIX filenames) */
    EXPECT_EQ(utf8_to_u16(std::string_view("a\0b", 3)), u({u'a', u'\0', u'b'}));
}

TEST(Text, RoundTripThroughNodeUtf8) {
    /* the pipeline contract: decode(UTF-8) -> u16 -> re-encode must yield
     * the original bytes for every well-formed input */
    const std::string cases[] = {
        "plain", "café", "\xe7\x85\xa7\xe7\x89\x87.jpg", "\xf0\x9f\x98\x80.png",
        "\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80"};
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
