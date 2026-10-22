/* SHA-256 known vectors (FIPS 180-4). */
#include <gtest/gtest.h>
#include "cr/crypto/cr_sha256.h"
#include <cstring>

using namespace cr;
using namespace cr::crypto;

TEST(Sha256, NistVectors) {
    struct { const char *in; const char *hex; } v[] = {
        {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
         "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
        {"The quick brown fox jumps over the lazy dog",
         "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"},
    };
    for (const auto &c : v) {
        u8 out[32];
        Sha256::digest(c.in, std::strlen(c.in), out);
        EXPECT_EQ(to_hex(out, 32), c.hex) << "input: " << c.in;
    }
}

TEST(Sha256, StreamedMatchesOneShot) {
    /* 1,000,000 x 'a' is the classic; use 100,000 for speed */
    std::vector<u8> data(100000, 'a');
    u8 streamed[32], oneshot[32];
    {
        Sha256 h;
        const size_t CH = 997; /* awkward chunking on purpose */
        for (size_t off = 0; off < data.size(); off += CH)
            h.update(data.data() + off, u32(std::min(CH, data.size() - off)));
        h.final(streamed);
    }
    Sha256::digest(data.data(), data.size(), oneshot);
    EXPECT_EQ(std::memcmp(streamed, oneshot, 32), 0);
}

TEST(Sha256, Id128Truncation) {
    u8 id[16];
    Sha256::id128("abc", 3, id);
    EXPECT_EQ(to_hex(id, 16), "ba7816bf8f01cfea414140de5dae2223");
}
