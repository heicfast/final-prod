/*
 * cr_aes.cpp - AES (FIPS-197) + XTS (IEEE 1619-2007 / NIST SP 800-38E).
 *
 * Verified against: FIPS-197 Appendix C.1/C.3 known-answer vectors and
 * cross-implementation vectors produced independently by pycryptodome and
 * OpenSSL EVP 3.5 (the same EVP interface the reference's BitLockerLib
 * statically links per the corpus artifact) - see
 * tests/unit/test_bitlocker_xts.cpp for the byte-exact pins.
 *
 * Data-unit discipline (SP 800-38E / IEEE 1619-2007): XTS data units are
 * at least one block and a whole number of 16-byte blocks. Partial data
 * units (the later CTS extension) are intentionally NOT implemented -
 * the reference decrypts whole sectors only (BitLocker volume-content
 * units are 512/4096-byte sectors), and whole-unit operation keeps the
 * encrypt/decrypt pairing trivially exact.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/crypto/cr_aes.h"
#include <cstring>

namespace cr {
namespace crypto {

namespace {

/* FIPS-197 sbox / inverse sbox (public standard constants). */
constexpr u8 SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};
constexpr u8 INV_SBOX[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d,
};

/* xtime: multiply by 2 in GF(2^8) (AES field poly 0x11b). */
inline u8 xtime(u8 x) { return u8((x << 1) ^ ((x >> 7) * 0x1b)); }

/* GF(2^128) alpha multiply for XTS tweaks (IEEE 1619 little-endian
 * convention: whole-tweak left shift by one bit toward higher bytes,
 * reduction constant 0x87 fed back into byte 0). */
void gf128_mul_alpha(u8 t[16]) {
    u8 carry = 0;
    for (int i = 0; i < 16; i++) {
        const u8 next_carry = u8(t[i] >> 7);
        t[i] = u8((t[i] << 1) | carry);
        carry = next_carry;
    }
    if (carry)
        t[0] ^= 0x87;
}

inline u32 sub_word(u32 w) {
    return u32(SBOX[w & 0xff]) | (u32(SBOX[(w >> 8) & 0xff]) << 8) |
           (u32(SBOX[(w >> 16) & 0xff]) << 16) | (u32(SBOX[(w >> 24) & 0xff]) << 24);
}
inline u32 inv_sub_word(u32 w) {
    return u32(INV_SBOX[w & 0xff]) | (u32(INV_SBOX[(w >> 8) & 0xff]) << 8) |
           (u32(INV_SBOX[(w >> 16) & 0xff]) << 16) | (u32(INV_SBOX[(w >> 24) & 0xff]) << 24);
}
inline u32 rot_word(u32 w) { return (w >> 8) | (w << 24); }

/* Round constants (FIPS-197 section 5.2). */
constexpr u32 RCON[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10,
                         0x20, 0x40, 0x80, 0x1b, 0x36};

/* ShiftRows on the 4-word state. Word c = column c; byte lane j of a
 * word = row j (load: in[4c+j] -> lane j). Row r rotates LEFT by r:
 * new col c row r = old col (c + r) mod 4 row r. */
void shift_rows(u32 s[4]) {
    /* row 1 (lane 0x0000ff00): rotate left 1 */
    const u32 o0 = s[0], o1 = s[1], o2 = s[2], o3 = s[3];
    constexpr u32 L1 = 0x0000ff00, NL1 = 0xffff00ff;
    s[0] = (o0 & NL1) | (o1 & L1);
    s[1] = (o1 & NL1) | (o2 & L1);
    s[2] = (o2 & NL1) | (o3 & L1);
    s[3] = (o3 & NL1) | (o0 & L1);
    /* row 2 (lane 0x00ff0000): rotate left 2 */
    constexpr u32 L2 = 0x00ff0000, NL2 = 0xff00ffff;
    const u32 p0 = s[0], p1 = s[1], p2 = s[2], p3 = s[3];
    s[0] = (p0 & NL2) | (p2 & L2);
    s[1] = (p1 & NL2) | (p3 & L2);
    s[2] = (p2 & NL2) | (p0 & L2);
    s[3] = (p3 & NL2) | (p1 & L2);
    /* row 3 (lane 0xff000000): rotate left 3 (= right 1) */
    constexpr u32 L3 = 0xff000000, NL3 = 0x00ffffff;
    const u32 q0 = s[0], q1 = s[1], q2 = s[2], q3 = s[3];
    s[0] = (q0 & NL3) | (q3 & L3);
    s[1] = (q1 & NL3) | (q0 & L3);
    s[2] = (q2 & NL3) | (q1 & L3);
    s[3] = (q3 & NL3) | (q2 & L3);
}

void inv_shift_rows(u32 s[4]) {
    /* row 1 (lane 0x0000ff00): rotate right 1 */
    const u32 o0 = s[0], o1 = s[1], o2 = s[2], o3 = s[3];
    constexpr u32 L1 = 0x0000ff00, NL1 = 0xffff00ff;
    s[0] = (o0 & NL1) | (o3 & L1);
    s[1] = (o1 & NL1) | (o0 & L1);
    s[2] = (o2 & NL1) | (o1 & L1);
    s[3] = (o3 & NL1) | (o2 & L1);
    /* row 2 (lane 0x00ff0000): rotate right 2 (= left 2) */
    constexpr u32 L2 = 0x00ff0000, NL2 = 0xff00ffff;
    const u32 p0 = s[0], p1 = s[1], p2 = s[2], p3 = s[3];
    s[0] = (p0 & NL2) | (p2 & L2);
    s[1] = (p1 & NL2) | (p3 & L2);
    s[2] = (p2 & NL2) | (p0 & L2);
    s[3] = (p3 & NL2) | (p1 & L2);
    /* row 3 (lane 0xff000000): rotate right 3 (= left 1) */
    constexpr u32 L3 = 0xff000000, NL3 = 0x00ffffff;
    const u32 q0 = s[0], q1 = s[1], q2 = s[2], q3 = s[3];
    s[0] = (q0 & NL3) | (q1 & L3);
    s[1] = (q1 & NL3) | (q2 & L3);
    s[2] = (q2 & NL3) | (q3 & L3);
    s[3] = (q3 & NL3) | (q0 & L3);
}

/* MixColumns: each column multiplied by the circulant [02 03 01 01]. */
void mix_columns(u32 s[4]) {
    for (int c = 0; c < 4; c++) {
        const u8 a0 = u8(s[c]), a1 = u8(s[c] >> 8), a2 = u8(s[c] >> 16), a3 = u8(s[c] >> 24);
        const u8 b0 = u8(xtime(a0) ^ xtime(a1) ^ a1 ^ a2 ^ a3);
        const u8 b1 = u8(a0 ^ xtime(a1) ^ xtime(a2) ^ a2 ^ a3);
        const u8 b2 = u8(a0 ^ a1 ^ xtime(a2) ^ xtime(a3) ^ a3);
        const u8 b3 = u8(xtime(a0) ^ a0 ^ a1 ^ a2 ^ xtime(a3));
        s[c] = u32(b0) | (u32(b1) << 8) | (u32(b2) << 16) | (u32(b3) << 24);
    }
}

/* GF(2^8) AES-field multipliers for InvMixColumns coefficients:
 * 2 = xtime, 4 = xtime^2, 8 = xtime^3; then
 * 0e = 8^4^2, 0b = 8^2^1, 0d = 8^4^1, 09 = 8^1. */
inline u8 gf2(u8 a) { return xtime(a); }
inline u8 gf4(u8 a) { return xtime(xtime(a)); }
inline u8 gf8(u8 a) { return xtime(xtime(xtime(a))); }
inline u8 gf_mul_09(u8 a) { return u8(gf8(a) ^ a); }
inline u8 gf_mul_0b(u8 a) { return u8(gf8(a) ^ gf2(a) ^ a); }
inline u8 gf_mul_0d(u8 a) { return u8(gf8(a) ^ gf4(a) ^ a); }
inline u8 gf_mul_0e(u8 a) { return u8(gf8(a) ^ gf4(a) ^ gf2(a)); }

void inv_mix_columns(u32 s[4]) {
    /* multiply each column by the circulant [0e 0b 0d 09] (FIPS-197 §5.3.3) */
    for (int c = 0; c < 4; c++) {
        const u8 a0 = u8(s[c]), a1 = u8(s[c] >> 8), a2 = u8(s[c] >> 16), a3 = u8(s[c] >> 24);
        const u8 b0 = u8(gf_mul_0e(a0) ^ gf_mul_0b(a1) ^ gf_mul_0d(a2) ^ gf_mul_09(a3));
        const u8 b1 = u8(gf_mul_09(a0) ^ gf_mul_0e(a1) ^ gf_mul_0b(a2) ^ gf_mul_0d(a3));
        const u8 b2 = u8(gf_mul_0d(a0) ^ gf_mul_09(a1) ^ gf_mul_0e(a2) ^ gf_mul_0b(a3));
        const u8 b3 = u8(gf_mul_0b(a0) ^ gf_mul_0d(a1) ^ gf_mul_09(a2) ^ gf_mul_0e(a3));
        s[c] = u32(b0) | (u32(b1) << 8) | (u32(b2) << 16) | (u32(b3) << 24);
    }
}

} // namespace

Aes::Aes() {
    std::memset(ek_, 0, sizeof(ek_));
    std::memset(dk_, 0, sizeof(dk_));
}

bool Aes::init(const u8* key, u32 key_bits) {
    if (!key || (key_bits != 128 && key_bits != 256))
        return false;
    expand(key, key_bits);
    return true;
}

void Aes::expand(const u8* key, u32 key_bits) {
    const int nk = int(key_bits) / 32;
    rounds_ = nk + 6;
    const int total_words = 4 * (rounds_ + 1);

    for (int i = 0; i < nk; i++)
        ek_[i] = u32(key[4 * i]) | (u32(key[4 * i + 1]) << 8) | (u32(key[4 * i + 2]) << 16) |
                 (u32(key[4 * i + 3]) << 24);
    for (int i = nk; i < total_words; i++) {
        u32 t = ek_[i - 1];
        if (i % nk == 0)
            t = sub_word(rot_word(t)) ^ RCON[i / nk];
        else if (nk > 6 && (i % nk) == 4)
            t = sub_word(t);
        ek_[i] = ek_[i - nk] ^ t;
    }

    /* decrypt schedule: InvMixColumns over the inner round keys */
    for (int i = 0; i < total_words; i++)
        dk_[i] = ek_[i];
    for (int r = 1; r < rounds_; r++) {
        u32 s[4] = {dk_[4 * r], dk_[4 * r + 1], dk_[4 * r + 2], dk_[4 * r + 3]};
        inv_mix_columns(s);
        dk_[4 * r] = s[0];
        dk_[4 * r + 1] = s[1];
        dk_[4 * r + 2] = s[2];
        dk_[4 * r + 3] = s[3];
    }
}

void Aes::encrypt_block(const u8 in[16], u8 out[16]) const {
    u32 s[4];
    for (int c = 0; c < 4; c++)
        s[c] = u32(in[4 * c]) | (u32(in[4 * c + 1]) << 8) | (u32(in[4 * c + 2]) << 16) |
               (u32(in[4 * c + 3]) << 24);

    for (int c = 0; c < 4; c++)
        s[c] ^= ek_[c];

    for (int r = 1; r <= rounds_; r++) {
        for (int c = 0; c < 4; c++)
            s[c] = sub_word(s[c]);
        shift_rows(s);
        if (r != rounds_)
            mix_columns(s);
        for (int c = 0; c < 4; c++)
            s[c] ^= ek_[4 * r + c];
    }

    for (int c = 0; c < 4; c++) {
        out[4 * c] = u8(s[c]);
        out[4 * c + 1] = u8(s[c] >> 8);
        out[4 * c + 2] = u8(s[c] >> 16);
        out[4 * c + 3] = u8(s[c] >> 24);
    }
}

void Aes::decrypt_block(const u8 in[16], u8 out[16]) const {
    u32 s[4];
    for (int c = 0; c < 4; c++)
        s[c] = u32(in[4 * c]) | (u32(in[4 * c + 1]) << 8) | (u32(in[4 * c + 2]) << 16) |
               (u32(in[4 * c + 3]) << 24);

    for (int c = 0; c < 4; c++)
        s[c] ^= dk_[4 * rounds_ + c];

    /* Equivalent inverse cipher (FIPS-197 §5.3.5): with the
     * InvMixColumns-transformed inner round keys in dk_, the per-round
     * order is InvShiftRows -> InvSubBytes -> InvMixColumns ->
     * AddRoundKey; the final round (r == 0) skips InvMixColumns. */
    for (int r = rounds_ - 1; r >= 0; r--) {
        inv_shift_rows(s);
        for (int c = 0; c < 4; c++)
            s[c] = inv_sub_word(s[c]);
        if (r != 0)
            inv_mix_columns(s);
        for (int c = 0; c < 4; c++)
            s[c] ^= dk_[4 * r + c];
    }

    for (int c = 0; c < 4; c++) {
        out[4 * c] = u8(s[c]);
        out[4 * c + 1] = u8(s[c] >> 8);
        out[4 * c + 2] = u8(s[c] >> 16);
        out[4 * c + 3] = u8(s[c] >> 24);
    }
}

AesXts::AesXts() = default;

bool AesXts::init(const u8* xts_key, size_t len) {
    if (!xts_key)
        return false;
    u32 bits = 0;
    if (len == 32)
        bits = 128;
    else if (len == 64)
        bits = 256;
    else
        return false;
    /* weak-key refusal: identical data/tweak halves are the XTS degenerate
     * class (OpenSSL 3.x rejects them too); a real FVEK never has them. */
    if (std::memcmp(xts_key, xts_key + len / 2, len / 2) == 0)
        return false;
    if (!k1_.init(xts_key, bits))
        return false;
    return k2_.init(xts_key + len / 2, bits);
}

namespace {
/* Shared whole-unit driver. len must be a positive multiple of 16
 * (SP 800-38E data-unit rule; BitLocker sectors always comply). */
inline bool xts_unit(const Aes& k1, const Aes& k2, const u8* dun, u8* data, size_t len,
                     bool enc) {
    if (!dun || !data || len == 0 || (len % 16) != 0)
        return false;
    u8 t[16];
    k2.encrypt_block(dun, t);
    const size_t blocks = len / 16;
    for (size_t i = 0; i < blocks; i++) {
        u8* blk = data + i * 16;
        u8 buf[16];
        for (int j = 0; j < 16; j++)
            buf[j] = u8(blk[j] ^ t[j]);
        if (enc) {
            k1.encrypt_block(buf, blk);
        } else {
            k1.decrypt_block(buf, blk);
        }
        for (int j = 0; j < 16; j++)
            blk[j] ^= t[j];
        gf128_mul_alpha(t);
    }
    return true;
}
} // namespace

void AesXts::encrypt(const u8 dun[16], u8* data, size_t len) const {
    if (!valid())
        return;
    (void)xts_unit(k1_, k2_, dun, data, len, true);
}

void AesXts::decrypt(const u8 dun[16], u8* data, size_t len) const {
    if (!valid())
        return;
    (void)xts_unit(k1_, k2_, dun, data, len, false);
}

} // namespace crypto
} // namespace cr
