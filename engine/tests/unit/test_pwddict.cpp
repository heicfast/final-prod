/* test_pwddict.cpp - the password-dictionary unlock aid (N-064).
 *
 * Proves:
 *   1. The og's Dict file FORMAT decodes byte-faithfully (comma-
 *      terminated CRLF lines, unterminated final entry, space-bearing
 *      entries) - first-hand decoded from the corpus file this session
 *      (og-codebase/Dict: 2,742 B, 228 entries, longest 20 chars).
 *   2. The shipped seed list is sane (ASCII, cap-safe, deduplicated).
 *   3. The consent-gated sequencer: order-preserving dedup, first-
 *      success-wins stop, attempt counting, ERR_FVE_LOCKED on exhaust.
 *   4. The attempt log lines carry progress + length ONLY - no
 *      candidate text ever reaches the sink (the og's EPMLOG run-log
 *      is world-readable in its own flows).
 *   5. The consent architecture: the sequencer is inert until invoked
 *      - there is no scan-path hookup (grep-pinned in the audit trail;
 *      the scan pipeline holds no reference to try_passwords).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/pwddict/cr_pwddict.h"
#include "cr/cr_error.h"
#include "cr/cr_log.h"
#include <cstring>
#include <string>
#include <vector>

using namespace cr;
using pwddict::kMaxEntries;
using pwddict::kMaxEntryChars;

namespace {

/* A recorder "unlock": rejects everything, records what it was fed. */
struct RejectAll {
    std::vector<std::string> seen;
    static u32 thunk(const std::string& password, void* user) {
        static_cast<RejectAll*>(user)->seen.push_back(password);
        return ERR_FVE_LOCKED;
    }
};

/* Succeeds on the N-th distinct candidate (1-based). */
struct SucceedAt {
    size_t n;
    std::vector<std::string> seen;
    static u32 thunk(const std::string& password, void* user) {
        SucceedAt* self = static_cast<SucceedAt*>(user);
        self->seen.push_back(password);
        if (self->seen.size() >= self->n)
            return OK;
        return ERR_FVE_LOCKED;
    }
};

/* N-048 seam capture */
struct LogCapture {
    std::vector<std::string> lines;
    static void sink(log::Level, const char* line, void* user) {
        static_cast<LogCapture*>(user)->lines.push_back(line);
    }
};

class PwddictLogScope {
public:
    PwddictLogScope(LogCapture& cap) {
        log::set_sink(&LogCapture::sink, &cap);
    }
    ~PwddictLogScope() { log::set_sink(nullptr, nullptr); }
};

} // namespace

/* ------------------------------------------------------------------ */
/* 1. the og's Dict format                                             */
/* ------------------------------------------------------------------ */

TEST(Pwddict, ParsesOgeDictFormatByteFaithfully) {
    /* the og's exact shape, first-hand from the corpus file: every line
     * "entry," + CRLF, the FINAL entry with neither comma nor newline */
    const std::string og_shaped =
        "12345678,\r\n"
        "11111111,\r\n"
        "password,\r\n"
        "charlie brown,\r\n" /* spaces are legal entries (corpus :63 lines) */
        "simmons";           /* the unterminated final entry */
    std::vector<std::string> out;
    const size_t n = pwddict::parse(og_shaped.data(), og_shaped.size(), out);
    ASSERT_EQ(n, 5u);
    ASSERT_EQ(out.size(), 5u);
    EXPECT_EQ(out[0], "12345678");
    EXPECT_EQ(out[1], "11111111");
    EXPECT_EQ(out[2], "password");
    EXPECT_EQ(out[3], "charlie brown");
    EXPECT_EQ(out[4], "simmons");
}

TEST(Pwddict, ParseAcceptsLfAndOptionalComma) {
    /* hand-edited dictionaries: LF endings, dropped commas, blank lines */
    const std::string loose = "alpha\nbeta,\n\n\ngamma\n";
    std::vector<std::string> out;
    ASSERT_EQ(pwddict::parse(loose.data(), loose.size(), out), 3u);
    EXPECT_EQ(out[0], "alpha");
    EXPECT_EQ(out[1], "beta");
    EXPECT_EQ(out[2], "gamma");
}

TEST(Pwddict, ParseEnforcesCapsAndRefusesOversize) {
    /* entry-count cap: exactly kMaxEntries accepted, one more skipped */
    std::string many;
    for (size_t i = 0; i < kMaxEntries + 2; ++i)
        many += "pw" + std::to_string(i) + ",\r\n";
    std::vector<std::string> out;
    const size_t n = pwddict::parse(many.data(), many.size(), out);
    EXPECT_EQ(n, kMaxEntries);

    /* per-entry cap: over-cap entries skipped, in-cap kept */
    const std::string long_line = std::string(kMaxEntryChars, 'x');
    const std::string over_line = std::string(kMaxEntryChars + 1, 'y');
    const std::string mixed = long_line + ",\r\n" + over_line + ",\r\nok,\r\n";
    std::vector<std::string> out2;
    ASSERT_EQ(pwddict::parse(mixed.data(), mixed.size(), out2), 2u);
    EXPECT_EQ(out2[0], long_line);
    EXPECT_EQ(out2[1], "ok");

    /* total-size cap: over-cap input refused whole (0, out untouched) */
    std::vector<std::string> out3;
    out3.push_back("preexisting");
    const std::string huge(pwddict::kMaxFileBytes + 1, 'z');
    EXPECT_EQ(pwddict::parse(huge.data(), huge.size(), out3), 0u);
    ASSERT_EQ(out3.size(), 1u); /* untouched */

    /* null/empty refused */
    EXPECT_EQ(pwddict::parse(nullptr, 10, out3), 0u);
    EXPECT_EQ(pwddict::parse("x", 0, out3), 0u);
}

/* ------------------------------------------------------------------ */
/* 2. the shipped seed list (D-78: authored, not the og's asset)       */
/* ------------------------------------------------------------------ */

TEST(Pwddict, SeedDictionaryIsSane) {
    const std::vector<std::string>& seed = pwddict::seed_dictionary();
    EXPECT_FALSE(seed.empty());
    std::vector<std::string> seen;
    for (const std::string& s : seed) {
        ASSERT_FALSE(s.empty());
        ASSERT_LE(s.size(), kMaxEntryChars);
        ASSERT_EQ(std::memchr(s.data(), ',', s.size()), nullptr);
        ASSERT_EQ(std::memchr(s.data(), '\n', s.size()), nullptr);
        ASSERT_EQ(std::memchr(s.data(), '\r', s.size()), nullptr);
        for (unsigned char c : s)
            ASSERT_LE(c, 0x7Eu) << "seed entries are ASCII";
        for (const std::string& p : seen)
            ASSERT_NE(p, s) << "seed entries are deduplicated";
        seen.push_back(s);
    }
    /* the seed round-trips through the sequencer directly (format-safe) */
    RejectAll rej;
    size_t made = 999;
    EXPECT_EQ(pwddict::try_passwords(seed, &RejectAll::thunk, &rej, &made),
              ERR_FVE_LOCKED);
    EXPECT_EQ(made, seed.size());
    EXPECT_EQ(rej.seen.size(), seed.size());
}

/* ------------------------------------------------------------------ */
/* 3. the consent-gated sequencer                                      */
/* ------------------------------------------------------------------ */

TEST(Pwddict, TryPasswordsStopsAtFirstSuccess) {
    SucceedAt win{3};
    const std::vector<std::string> cands = {"one", "two", "three", "four"};
    size_t made = 0;
    EXPECT_EQ(pwddict::try_passwords(cands, &SucceedAt::thunk, &win, &made), OK);
    EXPECT_EQ(made, 3u);
    ASSERT_EQ(win.seen.size(), 3u);
    EXPECT_EQ(win.seen[0], "one");
    EXPECT_EQ(win.seen[1], "two");
    EXPECT_EQ(win.seen[2], "three"); /* the winner; "four" never attempted */
}

TEST(Pwddict, TryPasswordsDedupsPreservingOrder) {
    RejectAll rej;
    const std::vector<std::string> cands = {"a", "b", "a", "c", "b", "a", ""};
    size_t made = 0;
    EXPECT_EQ(pwddict::try_passwords(cands, &RejectAll::thunk, &rej, &made),
              ERR_FVE_LOCKED);
    /* the empty candidate is never a real attempt */
    ASSERT_EQ(rej.seen.size(), 3u);
    EXPECT_EQ(rej.seen[0], "a");
    EXPECT_EQ(rej.seen[1], "b");
    EXPECT_EQ(rej.seen[2], "c");
    EXPECT_EQ(made, 3u);
}

TEST(Pwddict, TryPasswordsExhaustsToLockedAndGuardsArgs) {
    RejectAll rej;
    /* empty candidate list: no attempts, straight refusal */
    size_t made = 99;
    EXPECT_EQ(pwddict::try_passwords({}, &RejectAll::thunk, &rej, &made),
              ERR_FVE_LOCKED);
    EXPECT_EQ(made, 0u);
    EXPECT_TRUE(rej.seen.empty());

    /* only-empty/only-dup input behaves the same */
    EXPECT_EQ(pwddict::try_passwords({"", ""}, &RejectAll::thunk, &rej, &made),
              ERR_FVE_LOCKED);
    EXPECT_TRUE(rej.seen.empty());

    /* null attempt callback is a configuration error, not a crash */
    EXPECT_EQ(pwddict::try_passwords({"x"}, nullptr, nullptr, nullptr),
              ERR_FVE_LOCKED);
}

/* ------------------------------------------------------------------ */
/* 4. the attempt log lines never carry password text                  */
/* ------------------------------------------------------------------ */

TEST(Pwddict, AttemptLogCarriesProgressAndLengthNeverSecrets) {
    LogCapture cap;
    PwddictLogScope scope(cap);
    SucceedAt win{2};
    const std::vector<std::string> cands = {"secret-one", "secret-two"};
    EXPECT_EQ(pwddict::try_passwords(cands, &SucceedAt::thunk, &win, nullptr),
              OK);
    ASSERT_FALSE(cap.lines.empty());
    bool saw_attempt = false, saw_success = false;
    for (const std::string& line : cap.lines) {
        EXPECT_EQ(line.find("secret-one"), std::string::npos);
        EXPECT_EQ(line.find("secret-two"), std::string::npos);
        EXPECT_EQ(line.find("secret"), std::string::npos);
        if (line.find("attempt 1/2") != std::string::npos)
            saw_attempt = true;
        if (line.find("succeeded") != std::string::npos)
            saw_success = true;
    }
    EXPECT_TRUE(saw_attempt);
    EXPECT_TRUE(saw_success);
}

TEST(Pwddict, ExhaustLogsWarnOnce) {
    LogCapture cap;
    PwddictLogScope scope(cap);
    RejectAll rej;
    const std::vector<std::string> cands = {"nope", "nada"};
    EXPECT_EQ(pwddict::try_passwords(cands, &RejectAll::thunk, &rej, nullptr),
              ERR_FVE_LOCKED);
    int warns = 0, attempts = 0;
    for (const std::string& line : cap.lines) {
        if (line.find("exhausted") != std::string::npos)
            ++warns;
        if (line.find("attempt 1/2") != std::string::npos)
            ++attempts;
    }
    EXPECT_EQ(warns, 1);
    EXPECT_EQ(attempts, 1);
}
