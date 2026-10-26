/*
 * cr_aes.h - AES block cipher (FIPS-197) + XTS mode (IEEE 1619-2007),
 * the volume-content cipher of the reference's in-stack BitLocker layer
 * (corpus: architecture/05-scan-engine-architecture.md §8 "BitLocker: FVE
 * keys (DRWService pipe) -> XTS-AES decrypt (BitLockerLib/OpenSSL)";
 * artifacts/PartitionRecovery/bitlockerlib--bitlocker-implementation.md:
 * "OpenSSL EVP aes_{128,256}_xts - the actual volume-content cipher").
 *
 * Clean-room implementation from the public standards (FIPS-197 / IEEE
 * 1619-2007). No third-party crypto dependency: the core stays
 * self-contained (the tree links zlib/lz4/zstd only), Win7-inclusive, and
 * the whole layer is pure cross-platform so the Linux suite proves it.
 *
 * Key-size discipline: XTS uses 128- or 256-bit cipher keys; the XTS key
 * material is key||tweak_key (32 or 64 bytes) - exactly the FVEK formats
 * BitLocker produces (AES-128-XTS / AES-256-XTS full-volume-encryption
 * keys, the reference BitLockerManager::GetFullVolumeEncryptionKey output
 * shape: at least 0x40 bytes, key first - corpus decompile :27075-27107).
 * All-zero/repeated key halves are REJECTED (identical data/tweak keys
 * are the XTS weak-key class OpenSSL also refuses; a real FVEK never has
 * them, and accepting them would silently run a degenerate cipher).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_AES_H_
#define CR_AES_H_

#include "../cr_types.h"
#include <cstddef>

namespace cr {
namespace crypto {

/* AES-128 / AES-256 block cipher (16-byte blocks only). */
class Aes {
public:
    Aes();
    /* key_bits: 128 or 256. Returns false on any other size. */
    bool init(const u8* key, u32 key_bits);
    void encrypt_block(const u8 in[16], u8 out[16]) const;
    void decrypt_block(const u8 in[16], u8 out[16]) const;
    bool initialized() const { return rounds_ != 0; }
    u32 key_bits() const { return rounds_ == 10 ? 128 : 256; }

private:
    void expand(const u8* key, u32 key_bits);
    u32 ek_[60]; /* encrypt round keys (rounds+1) x 4 words */
    u32 dk_[60]; /* decrypt round keys (equivalent-inverse order) */
    int rounds_ = 0;
};

/* XTS-AES (IEEE 1619-2007), sector/data-unit oriented.
 *
 *   C_i = E_k1(P_i ^ T_i) ^ T_i,  T_{i+1} = T_i * alpha (GF(2^128))
 *   T_0 = E_k2(DUN)                (DUN = 16-byte data-unit number;
 *                                  BitLocker: LE sector index, zero-padded)
 *   final partial block: ciphertext stealing, standard formulation.
 *
 * decrypt() is the exact inverse (incl. the CTS tail). Both directions
 * operate in place or with disjoint buffers. */
class AesXts {
public:
    AesXts();
    /* xts_key: 32 bytes (AES-128-XTS: key||tweak) or 64 (AES-256-XTS).
     * Weak/repeated key halves are rejected (see file header). */
    bool init(const u8* xts_key, size_t len);
    /* dun: 16-byte data-unit number. len: any length >= 1. */
    void encrypt(const u8 dun[16], u8* data, size_t len) const;
    void decrypt(const u8 dun[16], u8* data, size_t len) const;
    bool valid() const { return k1_.initialized(); }
    u32 key_bits() const { return k1_.key_bits(); }

private:
    Aes k1_, k2_;
};

/* Convenience: 16-byte little-endian data-unit number for a sector index
 * (BitLocker's per-sector tweak convention). */
inline void sector_dun(u64 sector_index, u8 dun[16]) {
    for (int i = 0; i < 8; i++)
        dun[i] = u8(sector_index >> (8 * i));
    for (int i = 8; i < 16; i++)
        dun[i] = 0;
}

} // namespace crypto
} // namespace cr

#endif // CR_AES_H_
