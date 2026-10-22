/* cr_sha256.cpp - FIPS 180-4 SHA-256.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/crypto/cr_sha256.h"

namespace cr {
namespace crypto {

namespace {
constexpr u32 K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
inline u32 ror(u32 x, int n) {
    return (x >> n) | (x << (32 - n));
}
} // namespace

void Sha256::reset() {
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
    total_bits_ = 0;
    buf_len_ = 0;
}

void Sha256::process_block(const u8* p) {
    u32 w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (u32(p[i * 4]) << 24) | (u32(p[i * 4 + 1]) << 16) | (u32(p[i * 4 + 2]) << 8) |
               u32(p[i * 4 + 3]);
    for (int i = 16; i < 64; ++i) {
        u32 s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        u32 s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    u32 a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    u32 e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (int i = 0; i < 64; ++i) {
        u32 S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
        u32 ch = (e & f) ^ (~e & g);
        u32 t1 = h + S1 + ch + K[i] + w[i];
        u32 S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
        u32 maj = (a & b) ^ (a & c) ^ (b & c);
        u32 t2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void Sha256::update(const void* data, size_t len) {
    const u8* p = static_cast<const u8*>(data);
    total_bits_ += u64(len) * 8;
    while (len > 0) {
        size_t take = 64 - buf_len_;
        if (take > len)
            take = len;
        std::memcpy(buf_ + buf_len_, p, take);
        buf_len_ += take;
        p += take;
        len -= take;
        if (buf_len_ == 64) {
            process_block(buf_);
            buf_len_ = 0;
        }
    }
}

void Sha256::final(u8 out[32]) {
    u8 pad[72];
    size_t pad_len = 0;
    /* length in bits BEFORE padding (already accumulated) */
    u64 bits = total_bits_;
    size_t zero_pad = (buf_len_ < 56) ? 56 - buf_len_ : 120 - buf_len_;
    pad[pad_len++] = 0x80;
    --zero_pad;
    for (size_t i = 0; i < zero_pad; ++i)
        pad[pad_len++] = 0;
    for (int i = 7; i >= 0; --i)
        pad[pad_len++] = u8(bits >> (8 * i));
    update_raw(pad, pad_len);
    for (int i = 0; i < 8; ++i) {
        out[i * 4] = u8(state_[i] >> 24);
        out[i * 4 + 1] = u8(state_[i] >> 16);
        out[i * 4 + 2] = u8(state_[i] >> 8);
        out[i * 4 + 3] = u8(state_[i]);
    }
}

void Sha256::update_raw(const u8* p, size_t len) {
    /* padding bytes must not count toward total_bits_ again */
    while (len > 0) {
        size_t take = 64 - buf_len_;
        if (take > len)
            take = len;
        std::memcpy(buf_ + buf_len_, p, take);
        buf_len_ += take;
        p += take;
        len -= take;
        if (buf_len_ == 64) {
            process_block(buf_);
            buf_len_ = 0;
        }
    }
}

std::string to_hex(const u8* data, size_t len) {
    static const char hex[] = "0123456789abcdef";
    std::string s;
    s.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(hex[data[i] >> 4]);
        s.push_back(hex[data[i] & 0xF]);
    }
    return s;
}

} // namespace crypto
} // namespace cr
