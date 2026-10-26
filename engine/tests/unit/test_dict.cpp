/* test_dict.cpp - the password-dictionary unlock aid (N-064, diff-to-fix).
 *
 * Proves, in order:
 *   1. The shipped table's shape and grammar (228 entries, corpus order,
 *      ASCII, comma-free, bounded lengths - the generated data integrity).
 *   2. The Dict-file parse grammar (comma-terminated CRLF lines, the
 *      unterminated final line, LF tolerance, empty-line skip, the
 *      non-ASCII and oversized rejections).
 *   3. The consent-gated attempt loop: shipped set first in order, user
 *      candidates after, stop at the first success, honest stats, every
 *      attempt reported through the engine log seam (index only - the
 *      password never appears in a log line).
 *
 * The win-layer halves (the PowerShell/manage-bde command shape, the
 * wide-candidate seam) live in test_bitlocker_win.cpp (Windows lane).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/recovery/cr_dict.h"
#include "cr/cr_error.h"
#include "cr/cr_log.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::dict;

namespace {

/* ---------------- log-seam recorder (the N-048/N-061 seam) ---------------- */

struct LogRecorder {
    std::vector<std::string> lines;
    static void sink(log::Level level, const char* line, void* user) {
        auto* self = static_cast<LogRecorder*>(user);
        self->lines.emplace_back(line);
        (void)level;
    }
    size_t count_prefix(const char* prefix) const {
        size_t n = 0;
        for (const std::string& l : lines)
            if (l.rfind(prefix, 0) == 0)
                ++n;
        return n;
    }
};

/* ---------------- attempt recorder ---------------- */

struct AttemptRecorder {
    std::vector<std::string> seen;
    u32 fail_until = 0; /* attempts with index <= fail_until return LOCKED */
    u32 ok_at = 0;       /* 1-based candidate position that unlocks */
    u32 calls = 0;

    static u32 attempt(const char* password, void* user) {
        auto* self = static_cast<AttemptRecorder*>(user);
        ++self->calls;
        self->seen.emplace_back(password);
        if (self->ok_at != 0 && self->calls >= self->ok_at)
            return OK;
        return ERR_FVE_LOCKED;
    }
};

} // namespace

/* ---------------------------------------------------------------- */
/* 1. the shipped table                                              */
/* ---------------------------------------------------------------- */

TEST(Dict, ShippedTableMatchesCorpusShape) {
    /* The generated table: 228 entries (config-data-reference.md :20),
     * corpus order preserved (numeric-first; "simmons" last), ASCII only,
     * no commas, bounded lengths (corpus observed 2..20). */
    ASSERT_EQ(u32(228), kDictEntryCount);
    EXPECT_STREQ("12345678", kDictEntries[0]);
    EXPECT_STREQ("11111111", kDictEntries[1]);
    EXPECT_STREQ("00000000", kDictEntries[2]);
    EXPECT_STREQ("simmons", kDictEntries[kDictEntryCount - 1]);
    for (u32 i = 0; i < kDictEntryCount; ++i) {
        const char* e = kDictEntries[i];
        ASSERT_NE(nullptr, e);
        const size_t n = std::strlen(e);
        EXPECT_GT(n, size_t(0)) << "empty entry at " << i;
        EXPECT_LE(n, kMaxPasswordLen) << "oversized entry at " << i;
        for (size_t k = 0; k < n; ++k) {
            const unsigned char c = static_cast<unsigned char>(e[k]);
            EXPECT_GE(c, 0x20u) << "control byte in entry " << i;
            EXPECT_LE(c, 0x7Eu) << "non-ASCII byte in entry " << i;
            EXPECT_NE(',', c) << "comma inside entry " << i;
        }
    }
}

TEST(Dict, ShippedTableKeepsCorpusDuplicates) {
    /* The og's file DOES carry duplicate lines (byte-verified this
     * session: password/clinton/administrator/admin/super/user x2, cola x3)
     * and the og's flow attempts them in file order - a byte-faithful
     * conversion keeps them; deduping would be an invented improvement.
     * This pin guards the conversion against accidental dedup. */
    std::map<std::string, u32> counts;
    for (u32 i = 0; i < kDictEntryCount; ++i)
        ++counts[kDictEntries[i]];
    EXPECT_EQ(u32(2), counts.at("password"));
    EXPECT_EQ(u32(2), counts.at("clinton"));
    EXPECT_EQ(u32(2), counts.at("administrator"));
    EXPECT_EQ(u32(2), counts.at("admin"));
    EXPECT_EQ(u32(2), counts.at("super"));
    EXPECT_EQ(u32(2), counts.at("user"));
    EXPECT_EQ(u32(3), counts.at("cola"));
    /* ...and exactly these seven entries repeat (228 lines, 220 unique:
     * six +1 lines and cola's +2 = 8 surplus lines over the unique set) */
    u32 repeats = 0;
    for (const auto& kv : counts)
        if (kv.second > 1)
            ++repeats;
    EXPECT_EQ(u32(7), repeats);
    EXPECT_EQ(size_t(220), counts.size());
}

/* ---------------------------------------------------------------- */
/* 2. the Dict-file grammar                                          */
/* ---------------------------------------------------------------- */

TEST(Dict, ParseDictionaryGrammar) {
    /* The og's file shape: comma-terminated CRLF lines; the FINAL line
     * carries neither comma nor terminator. */
    const std::string og_shape = "12345678,\r\npassword,\r\nqwerty";
    std::vector<std::string> out;
    ASSERT_TRUE(parse(og_shape.data(), og_shape.size(), out));
    ASSERT_EQ(size_t(3), out.size());
    EXPECT_EQ("12345678", out[0]);
    EXPECT_EQ("password", out[1]);
    EXPECT_EQ("qwerty", out[2]);
}

TEST(Dict, ParseToleratesLfAndEmptyLines) {
    const std::string lf = "aaa,\nbbb,\n\nccc,\nddd";
    std::vector<std::string> out;
    ASSERT_TRUE(parse(lf.data(), lf.size(), out));
    ASSERT_EQ(size_t(4), out.size());
    EXPECT_EQ("aaa", out[0]);
    EXPECT_EQ("bbb", out[1]);
    EXPECT_EQ("ccc", out[2]);
    EXPECT_EQ("ddd", out[3]);
}

TEST(Dict, ParseRoundTripsTheCorpusGrammar) {
    /* A rebuilt corpus-shaped blob parses back to the shipped table. */
    std::string blob;
    for (u32 i = 0; i < kDictEntryCount; ++i) {
        blob += kDictEntries[i];
        if (i + 1 < kDictEntryCount)
            blob += ",\r\n";
    }
    std::vector<std::string> out;
    ASSERT_TRUE(parse(blob.data(), blob.size(), out));
    ASSERT_EQ(size_t(kDictEntryCount), out.size());
    for (u32 i = 0; i < kDictEntryCount; ++i)
        EXPECT_EQ(std::string(kDictEntries[i]), out[i]) << "entry " << i;
}

TEST(Dict, ParseRejectsBadGrammar) {
    std::vector<std::string> out;
    /* non-ASCII byte: the manage-bde command line carries ASCII only */
    const std::string non_ascii = "abc,\r\n\xd1\x88";
    EXPECT_FALSE(parse(non_ascii.data(), non_ascii.size(), out));
    EXPECT_TRUE(out.empty());
    /* control byte inside a candidate */
    const std::string ctl = "ab\x01c,";
    EXPECT_FALSE(parse(ctl.data(), ctl.size(), out));
    /* oversized candidate (cap 32) */
    const std::string big = std::string(33, 'x') + ",";
    EXPECT_FALSE(parse(big.data(), big.size(), out));
    /* comma-only line = empty candidate after strip */
    const std::string empt = ",\r\n";
    EXPECT_FALSE(parse(empt.data(), empt.size(), out));
}

/* ---------------------------------------------------------------- */
/* 3. the consent-gated attempt loop                                 */
/* ---------------------------------------------------------------- */

TEST(Dict, TryStopsAtFirstSuccessInShippedOrder) {
    AttemptRecorder rec;
    rec.ok_at = 5; /* the 5th shipped candidate unlocks */
    const AttemptResult res = try_passwords(&AttemptRecorder::attempt, &rec);
    EXPECT_EQ(u32(5), res.success_index);
    EXPECT_EQ(u32(5), res.attempted);
    EXPECT_FALSE(res.from_user_set);
    EXPECT_EQ(u32(5), rec.calls); /* no attempts past the winner */
    ASSERT_EQ(size_t(5), rec.seen.size());
    for (u32 i = 0; i < 5; ++i)
        EXPECT_EQ(std::string(kDictEntries[i]), rec.seen[i]) << "order at " << i;
}

TEST(Dict, UserCandidatesFollowShippedDictionary) {
    AttemptRecorder rec;
    std::vector<std::string> user{"hunter2", "correct horse"};
    /* winner = the SECOND user candidate (shipped 228 + 2) */
    rec.ok_at = kDictEntryCount + 2;
    const AttemptResult res = try_passwords(&AttemptRecorder::attempt, &rec, &user);
    EXPECT_NE(u32(0), res.success_index);
    EXPECT_TRUE(res.from_user_set);
    EXPECT_EQ(kDictEntryCount + 2, res.success_index);
    EXPECT_EQ(kDictEntryCount + 2, rec.calls);
    ASSERT_EQ(size_t(kDictEntryCount + 2), rec.seen.size());
    EXPECT_EQ("hunter2", rec.seen[kDictEntryCount]);
    EXPECT_EQ("correct horse", rec.seen[kDictEntryCount + 1]);
}

TEST(Dict, AllFailReportsEveryAttempt) {
    AttemptRecorder rec; /* ok_at = 0: everything fails */
    std::vector<std::string> user{"one", "two"};
    const AttemptResult res = try_passwords(&AttemptRecorder::attempt, &rec, &user);
    EXPECT_EQ(u32(0), res.success_index);
    EXPECT_EQ(kDictEntryCount + 2, res.attempted);
    EXPECT_EQ(u32(ERR_FVE_LOCKED), res.rc);
    EXPECT_EQ(kDictEntryCount + 2, rec.calls);
}

TEST(Dict, NullAttemptFunctionIsANoOp) {
    const AttemptResult res = try_passwords(nullptr, nullptr);
    EXPECT_EQ(u32(0), res.attempted);
    EXPECT_EQ(u32(0), res.success_index);
}

TEST(Dict, AttemptsLoggedThroughSeamWithoutPasswordMaterial) {
    LogRecorder log;
    log::set_sink(&LogRecorder::sink, &log);
    AttemptRecorder rec;
    rec.ok_at = 3;
    const AttemptResult res = try_passwords(&AttemptRecorder::attempt, &rec);
    log::set_sink(nullptr, nullptr);
    EXPECT_NE(u32(0), res.success_index);
    /* every attempt has its index line; the winner has its success line */
    EXPECT_EQ(size_t(3), log.count_prefix("dict unlock attempt "));
    EXPECT_EQ(size_t(1), log.count_prefix("dict unlock: candidate 3 succeeded (shipped set)"));
    /* password material NEVER appears in any line */
    for (const std::string& l : log.lines) {
        EXPECT_EQ(std::string::npos, l.find(rec.seen[2]))
            << "password leaked into log line: " << l;
        EXPECT_EQ(std::string::npos, l.find("12345678"))
            << "shipped password leaked into log line: " << l;
    }
}
