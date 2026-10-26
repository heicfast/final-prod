/*
 * cr_sha256.h - SHA-256 (FIPS 180-4). Pure, cross-platform.
 * Full 32-byte digest; also the 128-bit truncated id used for node dedup.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SHA256_H_
#define CR_SHA256_H_

#include "../cr_types.h"
#include <string>
#include <cstring>

namespace cr {
namespace crypto {

class Sha256 {
public:
    Sha256() { reset(); }

    void reset();
    void update(const void* data, size_t len);
    /* finalize into out[32]; the object is reusable after reset() */
    void final(u8 out[32]);

    /* one-shot convenience */
    static void digest(const void* data, size_t len, u8 out[32]) {
        Sha256 h;
        h.update(data, len);
        h.final(out);
    }
    /* 128-bit truncated id (node dedup key) */
    static void id128(const void* data, size_t len, u8 out[16]) {
        u8 full[32];
        digest(data, len, full);
        std::memcpy(out, full, 16);
    }

private:
    void process_block(const u8* p);
    void update_raw(const u8* p, size_t len); /* padding bytes: no bit count */
    u32 state_[8];
    u64 total_bits_ = 0;
    u8 buf_[64];
    size_t buf_len_ = 0;
};

/* Hex helper (CLI/logging). */
std::string to_hex(const u8* data, size_t len);

} // namespace crypto
} // namespace cr

#endif // CR_SHA256_H_
