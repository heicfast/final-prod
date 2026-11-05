/* test_bitlocker_xts.cpp - the in-stack BitLocker decrypt layer (M-033).
 *
 * Proves, in order:
 *   1. AES against FIPS-197 Appendix C known-answer vectors.
 *   2. XTS against cross-implementation vectors generated independently by
 *      pycryptodome AND OpenSSL EVP 3.5 (the same EVP interface the og's
 *      BitLockerLib statically links - corpus
 *      artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md),
 *      byte-exact.
 *   3. DecryptingDevice: plaintext reads through the wrapper, byte-exact
 *      unaligned/cross-sector reads, transport-error propagation.
 *   4. The M-033 acceptance: an encrypted volume is opaque to the engine
 *      without the key and scans correctly through the in-stack decrypt
 *      device with it (the og's "engines are BitLocker-agnostic, decrypt
 *      happens below them" - corpus architecture doc §8).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/crypto/cr_aes.h"
#include "cr/storage/cr_decrypt_device.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/fat/cr_fat_wire.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::fat;
using crypto::Aes;
using crypto::AesXts;

namespace {

/* ---------------------------------------------------------------- */
/* helpers                                                           */
/* ---------------------------------------------------------------- */

std::vector<u8> hex(const char* s) {
    std::vector<u8> v;
    while (*s && s[1]) {
        unsigned x = 0;
        for (int i = 0; i < 2; i++) {
            char c = s[i];
            x <<= 4;
            if (c >= '0' && c <= '9')
                x |= unsigned(c - '0');
            else if (c >= 'a' && c <= 'f')
                x |= unsigned(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                x |= unsigned(c - 'A' + 10);
        }
        v.push_back(u8(x));
        s += 2;
    }
    return v;
}

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (fail_next_) {
            fail_next_ = false;
            return u32(ERR_OPEN_FAILED); /* any transport error code */
        }
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    std::vector<u8>& raw() { return data_; }
    void fail_next_read() { fail_next_ = true; }

private:
    std::vector<u8> data_;
    Geometry geom_;
    bool fail_next_ = false;
};

/* ---------------------------------------------------------------- */
/* minimal FAT16 volume (compact M-033-specific fixture; the         */
/* full-featured builder stays in test_fat_volume.cpp)                */
/* ---------------------------------------------------------------- */

constexpr u16 kFat16TotalSecs = 512; /* 256 KiB */

void put16v(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}

std::vector<u8> build_small_fat16() {
    /* layout: reserved 2 | FAT 2x12 | root 32 sectors -> data at sec 58.
     * S-064 re-shape: spf 2 made this a string/bracket-DISAGREEING volume
     * (FAT16 string, 474 clusters) that slid through as FAT12 pre-fix;
     * the og's string-first flavor + the child-table Fat16 window
     * [12,256] (FUN_180036e90, the M-011 decode) decline spf=2 outright.
     * spf 12 is the window floor - a clean og-faithful small FAT16. */
    std::vector<u8> img(u64(kFat16TotalSecs) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSWIN4.1", 8);
    put16v(img, 0x0B, 512);
    b[0x0D] = 1;            /* sectors/cluster */
    put16v(img, 0x0E, 2);   /* reserved */
    b[0x10] = 2;            /* FAT count */
    put16v(img, 0x11, 512); /* root entries */
    put16v(img, 0x13, kFat16TotalSecs);
    b[0x15] = 0xF8;        /* media */
    put16v(img, 0x16, 12); /* sectors/FAT - the Fat16 window floor */
    put16v(img, 0x18, 63);
    put16v(img, 0x1A, 255);
    b[0x26] = 0x29;
    put32v(img, 0x27, 0x1234);
    std::memcpy(b + 0x2B, "XTSVOLUME ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const size_t FAT1 = 2 * 512, FAT2 = 14 * 512;
    const size_t ROOT = 26 * 512; /* 32 sectors */
    const size_t DATA = 58 * 512;

    /* canonical media words (M-015) */
    put16v(img, FAT1 + 0, 0xFFF8);
    put16v(img, FAT1 + 2, 0xFFFF);
    put16v(img, FAT2 + 0, 0xFFF8);
    put16v(img, FAT2 + 2, 0xFFFF);
    /* cluster 2 -> EOC: one file, one cluster */
    put16v(img, FAT1 + 4, 0xFFFF);
    put16v(img, FAT2 + 4, 0xFFFF);

    /* root: READme.TXT @ cluster 2, 256 bytes */
    auto* e = reinterpret_cast<DirEntry*>(img.data() + ROOT);
    std::memcpy(e->name, "READMe  TXT", 11);
    e->attributes = 0x20;
    e->creation_time = 0x6000;
    e->creation_date = 0x5A46;
    e->access_date = 0x5A46;
    e->write_time = 0x6000;
    e->write_date = 0x5A46;
    e->first_cluster_low = 2;
    e->file_size = 256;

    std::memcpy(img.data() + DATA, "XTS-DECRYPTED-PAYLOAD-M033", 26);
    return img;
}

} // namespace

/* ---------------------------------------------------------------- */
/* AES known-answer (FIPS-197 Appendix C)                            */
/* ---------------------------------------------------------------- */

TEST(AesXts, Aes128Fips197KnownAnswer) {
    Aes a;
    ASSERT_TRUE(a.init(hex("000102030405060708090a0b0c0d0e0f").data(), 128));
    const auto pt = hex("00112233445566778899aabbccddeeff");
    u8 out[16];
    a.encrypt_block(pt.data(), out);
    const auto expect = hex("69c4e0d86a7b0430d8cdb78070b4c55a");
    ASSERT_EQ(0, std::memcmp(out, expect.data(), 16));
    u8 back[16];
    a.decrypt_block(out, back);
    ASSERT_EQ(0, std::memcmp(back, pt.data(), 16));
}

TEST(AesXts, Aes256Fips197KnownAnswer) {
    Aes a;
    ASSERT_TRUE(a.init(hex("000102030405060708090a0b0c0d0e0f1011121314151617"
                           "18191a1b1c1d1e1f")
                           .data(),
                       256));
    const auto pt = hex("00112233445566778899aabbccddeeff");
    u8 out[16];
    a.encrypt_block(pt.data(), out);
    const auto expect = hex("8ea2b7ca516745bfeafc49904b496089");
    ASSERT_EQ(0, std::memcmp(out, expect.data(), 16));
}

/* ---------------------------------------------------------------- */
/* XTS cross-implementation vectors (pycryptodome == OpenSSL EVP,    */
/* byte-exact, both generated the same values independently)         */
/* ---------------------------------------------------------------- */

TEST(AesXts, Xts128CrossImplementationVector) {
    AesXts x;
    const auto key = hex("000102030405060708090a0b0c0d0e0f1011121314151617"
                         "18191a1b1c1d1e1f");
    ASSERT_TRUE(x.init(key.data(), key.size()));
    const auto dun = hex("0102030405060708090a0b0c0d0e0f10");
    auto data = hex("01060b10151a1f24292e33383d42474c51565b60656a6f74797e83888d92979c");
    x.encrypt(dun.data(), data.data(), data.size());
    const auto expect = hex("b65cab47531cacce8542a3c193c085a222a27e35b8f7cd9468fc4e593a338d04");
    ASSERT_EQ(0, std::memcmp(data.data(), expect.data(), 32));
    /* exact round trip */
    x.decrypt(dun.data(), data.data(), data.size());
    const auto orig = hex("01060b10151a1f24292e33383d42474c51565b60656a6f74797e83888d92979c");
    ASSERT_EQ(0, std::memcmp(data.data(), orig.data(), 32));
}

TEST(AesXts, Xts256CrossImplementationVector) {
    AesXts x;
    const auto key = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                         "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    ASSERT_TRUE(x.init(key.data(), key.size()));
    const auto dun = hex("11223344556677889900aabbccddeeff");
    auto data = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                    "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    x.encrypt(dun.data(), data.data(), 64);
    const auto expect = hex("cd80d5d4e3397b42de4abc44463bf49f5ac61df1c7d8df9ee6c84353d8770cf3"
                            "a0e1fe62ef685683ba78a08989b2c9c889dda5c00cf52e21050ffbb304481c6d");
    ASSERT_EQ(0, std::memcmp(data.data(), expect.data(), 64));
    x.decrypt(dun.data(), data.data(), 64);
    const auto orig = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                          "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    ASSERT_EQ(0, std::memcmp(data.data(), orig.data(), 64));
}

TEST(AesXts, Xts256WholeSectorVector) {
    /* a 512-byte data unit exactly as BitLocker consumes them - generated
     * by OpenSSL EVP aes_256_xts with the above key/tweak. */
    AesXts x;
    const auto key = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                         "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    ASSERT_TRUE(x.init(key.data(), key.size()));
    const auto dun = hex("11223344556677889900aabbccddeeff");
    std::vector<u8> data(512);
    for (size_t i = 0; i < 512; i++)
        data[i] = u8((i * 7 + 3) & 0xff);
    x.encrypt(dun.data(), data.data(), 512);
    const auto expect = hex("b6428fc77bb36c61118d2aab799b520e1e4f3cb66da271648277beae03a521d6"
                            "c17ab99455b04d632efcb956a4f227599a580c2ddd8ad2c49deb47bb492d22f7"
                            "28c50c3e4bb2a7606dd3702c2a998fb9c3035a9a9b55ff473f1c6fdf86dcbc47"
                            "f2e878bef81c6855e1baae8b1cb1ce3be632ff600be0cb0c03bd148854754cbf"
                            "a05572be2cd04d619c79fc76e41223411e0182ac823060db828771c789ad6a3c"
                            "538146b5cda5e983d7e0fefb5fe6a7015e3ac714c79f017ba3f0478f285c3b14"
                            "60ef58a3ca6a63d0b18f2a2d1ccfe23c5719488a4fa901a12b70950afda5ea90"
                            "934ea2600232a3bef7c6307659c12aaf30d8d90782b7d6f26fa403b4bbfe8d6f"
                            "546136b759615ffc8a8508b44ac13438c2770c716241777749891f5ea12241ba6"
                            "5e839743f0bdcd4a417bf6c22e5418ea98970c0757e8413106918d1732b1777a"
                            "c8377a5c56da3b1cda5ba6dd7ec4c9edcaac290b576780cb17a8101f5f99ac31"
                            "b86411f2ddfee6e5528f282de2ea27e61944c871eb08c9e190f65426de2a2f62"
                            "f0f4602b93dd17f7d40dc0840f2fcbca6e676252781d6754c8d0b5261a3eeac9"
                            "7745cad0325f815d0856b999d3966b13cfb9743438d92d95bfb1ec3de9a72885"
                            "0faad460430109a51a6b58b77a8ff3deb614898fb41cbe7d60d77742e83c18113"
                            "e965e74451e967d96fa708cd764ecf2a0794f2b8d9e348778ffd01ea70b164");
    ASSERT_EQ(512u, expect.size());
    ASSERT_EQ(0, std::memcmp(data.data(), expect.data(), 512));
    x.decrypt(dun.data(), data.data(), 512);
    for (size_t i = 0; i < 512; i++)
        ASSERT_EQ(u8((i * 7 + 3) & 0xff), data[i]) << "byte " << i;
}

TEST(AesXts, WeakKeyHalvesRejected) {
    AesXts x;
    std::vector<u8> weak(32, 0);
    ASSERT_FALSE(x.init(weak.data(), weak.size()));
    std::vector<u8> repeated(64);
    for (int i = 0; i < 32; i++)
        repeated[i] = repeated[i + 32] = u8(i * 3 + 1);
    ASSERT_FALSE(x.init(repeated.data(), repeated.size()));
    /* wrong sizes rejected */
    ASSERT_FALSE(x.init(weak.data(), 16));
    ASSERT_FALSE(x.init(weak.data(), 48));
}

TEST(AesXts, RoundTripVariedLengths) {
    /* whole data units of many sizes, both key classes */
    const auto key128 = hex("000102030405060708090a0b0c0d0e0f1011121314151617"
                            "18191a1b1c1d1e1f");
    const auto key256 = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                            "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    for (const auto* key : {&key128, &key256}) {
        AesXts x;
        ASSERT_TRUE(x.init(key->data(), key->size()));
        for (size_t len : {16u, 32u, 48u, 512u, 4096u, 16u * 33}) {
            std::vector<u8> orig(len);
            for (size_t i = 0; i < len; i++)
                orig[i] = u8((i * 31 + 7) & 0xff);
            std::vector<u8> enc = orig, dec = orig;
            u8 dun[16];
            crypto::sector_dun(len / 16, dun);
            x.encrypt(dun, enc.data(), len);
            ASSERT_NE(0, std::memcmp(enc.data(), orig.data(), len));
            x.decrypt(dun, enc.data(), len);
            ASSERT_EQ(0, std::memcmp(enc.data(), dec.data(), len)) << "len " << len;
        }
    }
}

TEST(AesXts, DistinctTweaksGiveDistinctCiphertexts) {
    /* the sector-tweak binding: same plaintext, different sector index
     * -> different ciphertext (XTS data-unit separation - what makes
     * sector-granular decryption correct). */
    AesXts x;
    const auto key256 = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                            "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    ASSERT_TRUE(x.init(key256.data(), key256.size()));
    std::vector<u8> a(512, 0xAB), b(512, 0xAB);
    u8 d0[16], d1[16];
    crypto::sector_dun(0, d0);
    crypto::sector_dun(1, d1);
    x.encrypt(d0, a.data(), 512);
    x.encrypt(d1, b.data(), 512);
    ASSERT_NE(0, std::memcmp(a.data(), b.data(), 512));
}

/* ---------------------------------------------------------------- */
/* DecryptingDevice                                                  */
/* ---------------------------------------------------------------- */

TEST(DecryptingDevice, ReadsPlaintextThroughWrapper) {
    /* the M-033 finding's own acceptance: synthetic XTS volume, known
     * key; assert plaintext reads through the in-stack wrapper. */
    const auto key = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                         "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    std::vector<u8> plain(4 * 512);
    for (size_t i = 0; i < plain.size(); i++)
        plain[i] = u8((i * 11 + 5) & 0xff);

    AesXts x;
    ASSERT_TRUE(x.init(key.data(), key.size()));
    std::vector<u8> disk = plain;
    for (u64 s = 0; s < 4; s++) {
        u8 dun[16];
        crypto::sector_dun(s, dun);
        x.encrypt(dun, disk.data() + s * 512, 512);
    }
    MemDevice mem(disk);
    auto dev = DecryptingDevice::create(&mem, key.data(), key.size());
    ASSERT_NE(nullptr, dev);

    /* whole-volume read back */
    std::vector<u8> out(plain.size(), 0);
    ASSERT_EQ(OK, dev->read_at(0, out.data(), u32(out.size())));
    ASSERT_EQ(0, std::memcmp(out.data(), plain.data(), plain.size()));

    /* spot reads */
    u8 buf[16];
    ASSERT_EQ(OK, dev->read_at(1024 + 37, buf, 16));
    ASSERT_EQ(0, std::memcmp(buf, plain.data() + 1024 + 37, 16));
}

TEST(DecryptingDevice, UnalignedAndCrossSectorReads) {
    const auto key = hex("0d4277ace1164b80b5ea1f5489bef3285d92c7fc31669bd0053a6fa4d90e4378"
                         "ade2174c81b6eb20558abff4295e93c8fd32679cd1063b70a5da0f4479aee318");
    std::vector<u8> plain(6 * 512);
    for (size_t i = 0; i < plain.size(); i++)
        plain[i] = u8((i * 73 + 11) & 0xff);
    AesXts x;
    ASSERT_TRUE(x.init(key.data(), key.size()));
    std::vector<u8> disk = plain;
    for (u64 s = 0; s < 6; s++) {
        u8 dun[16];
        crypto::sector_dun(s, dun);
        x.encrypt(dun, disk.data() + s * 512, 512);
    }
    MemDevice mem(disk);
    auto dev = DecryptingDevice::create(&mem, key.data(), key.size());
    ASSERT_NE(nullptr, dev);

    /* byte-exact reads at every offset/length class: intra-sector,
     * sector-crossing, multi-sector, tail-clipped */
    struct Case {
        u64 off;
        u32 len;
    };
    for (const Case& c : std::vector<Case>{{0, 1},
                                           {1, 1},
                                           {511, 1},
                                           {511, 2},
                                           {100, 1000},
                                           {512, 512},
                                           {1023, 514},
                                           {0, 6 * 512},
                                           {3000, 72}}) {
        std::vector<u8> out(c.len, 0xEE);
        ASSERT_EQ(OK, dev->read_at(c.off, out.data(), c.len))
            << "off=" << c.off << " len=" << c.len;
        ASSERT_EQ(0, std::memcmp(out.data(), plain.data() + c.off, c.len))
            << "off=" << c.off << " len=" << c.len;
    }
    /* out-of-bounds refused */
    u8 b;
    EXPECT_EQ(u32(ERR_RANGE_OUT_OF_BOUNDS), dev->read_at(6 * 512, &b, 1));
    EXPECT_EQ(u32(ERR_RANGE_OUT_OF_BOUNDS), dev->read_at(6 * 512 - 1, &b, 2));
}

TEST(DecryptingDevice, InnerErrorsPropagateVerbatim) {
    /* transport failures pass through - decryption never masks them
     * (the error-map honesty contract). */
    const auto key = hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
                         "202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
    MemDevice mem(std::vector<u8>(2048, 0));
    auto dev = DecryptingDevice::create(&mem, key.data(), key.size());
    ASSERT_NE(nullptr, dev);
    u8 buf[512];
    mem.fail_next_read();
    EXPECT_EQ(u32(ERR_OPEN_FAILED), dev->read_at(0, buf, 512));
}

TEST(DecryptingDevice, BadKeyMaterialCreateFails) {
    MemDevice mem(std::vector<u8>(2048, 0));
    EXPECT_EQ(nullptr, DecryptingDevice::create(&mem, nullptr, 64));
    EXPECT_EQ(nullptr, DecryptingDevice::create(&mem, hex("aa").data(), 16));
    EXPECT_EQ(nullptr, DecryptingDevice::create(nullptr, hex("aa").data(), 64));
    std::vector<u8> weak(32, 0);
    EXPECT_EQ(nullptr, DecryptingDevice::create(&mem, weak.data(), weak.size()));
    EXPECT_EQ(nullptr, DecryptingDevice::create(&mem, weak.data(), 0));
}

/* ---------------------------------------------------------------- */
/* The M-033 acceptance: engine-through-decrypt integration          */
/* ---------------------------------------------------------------- */

TEST(BitLockerStack, EncryptedVolumeScansOnlyThroughDecryptDevice) {
    /* the og's contract (corpus architecture §8 + the BitLockerLib
     * artifact's recovery flow): with the key, engines scan the
     * plaintext view BECAUSE the decrypt happens below them - the
     * engine itself is untouched. Without the key the volume bytes
     * are opaque ciphertext: the engine cannot even see a boot sector.
     * This is the harness-portable form of the finding's
     * "LockedVolumeScanWithKey" acceptance (the locked-volume refusal
     * itself stays pinned on the Windows lane). */
    const auto key = hex("0724415e7b98b5d2ef0c294663809dbad7f4112e4b6885a2bfdcf91633506d8a"
                         "a7c4e1fe1b3855728facc9e603203d5a7794b1ceeb0825425f7c99b6d3f00d2a");
    ASSERT_EQ(64u, key.size());
    const auto plain_img = build_small_fat16();

    /* encrypt the whole volume, sector by sector, with the BitLocker
     * tweak convention (LE sector index) */
    AesXts x;
    ASSERT_TRUE(x.init(key.data(), key.size()));
    std::vector<u8> disk = plain_img;
    const u64 secs = disk.size() / 512;
    for (u64 s = 0; s < secs; s++) {
        u8 dun[16];
        crypto::sector_dun(s, dun);
        x.encrypt(dun, disk.data() + s * 512, 512);
    }

    /* without the key: raw ciphertext is not a FAT volume */
    {
        MemDevice raw(disk);
        fat::FatScanFlags flags;
        fat::FatScanStats stats;
        tree::ResultTree tree;
        tree::FullNode ra, rd, rl;
        const u32 rc =
            fat::scan_fat(&raw, flags, nullptr, nullptr, nullptr, tree, &ra, &rd, &rl, stats);
        EXPECT_NE(OK, rc); /* the volume is opaque: detection fails */
    }

    /* with the key: the same engine, unmodified, scans through the
     * in-stack decrypt device and finds the file */
    {
        MemDevice raw(disk);
        auto dev = DecryptingDevice::create(&raw, key.data(), key.size());
        ASSERT_NE(nullptr, dev);
        fat::FatScanFlags flags;
        fat::FatScanStats stats;
        tree::ResultTree tree;
        tree::FullNode ra, rd, rl;
        const u32 rc =
            fat::scan_fat(dev.get(), flags, nullptr, nullptr, nullptr, tree, &ra, &rd, &rl, stats);
        ASSERT_EQ(OK, rc);
        /* exactly one live file, 256 bytes, name recovered. N-175: the
         * fixture dentry stores mixed-case "READMe" bytes with NTRes=0,
         * and the og's 8.3 fold renders uppercase (FATSearchDLL
         * :44191-44216) - the expectation re-pinned from the pre-N-175
         * raw-bytes form "READMe.TXT" to the folded "README.TXT". */
        size_t files = 0;
        for (auto* n = ra.first_child(); n; n = n->next_sibling())
            files++;
        ASSERT_EQ(1u, files);
        const auto* node = ra.first_child();
        ASSERT_NE(nullptr, node->props());
        EXPECT_EQ(256u, node->props()->logical_size);
        const std::u16string expect_name = u"README.TXT";
        EXPECT_EQ(expect_name, node->props()->name);
        EXPECT_EQ(1u, stats.files);
    }
}
