/*
 * cr_bits.h - shared byte-order readers for the engine layer.
 *
 * W4 dedup (AUDIT-LINUX-FS DC-12): seven engines carried private
 * near-identical copies of these fold functions (ext4 rd16 and rd32,
 * hfsplus be16-be64, btrfs le16-le64, zfs rdle and be families, refs
 * le16-le64). One spelling now: the le family delegates to the cr_types
 * core, the be family lives here.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BITS_H_
#define CR_BITS_H_

#include "cr_types.h"

namespace cr {

/* little-endian readers (canonical spellings of the cr_types core) */
inline u16 le16(const u8* p) {
    return u16_from_bytes_le(p);
}
inline u32 le32(const u8* p) {
    return u32_from_bytes_le(p);
}
inline u64 le64(const u8* p) {
    return u64_from_bytes_le(p);
}

/* big-endian readers */
inline u16 be16(const u8* p) {
    return static_cast<u16>((static_cast<u16>(p[0]) << 8) | static_cast<u16>(p[1]));
}
inline u32 be32(const u8* p) {
    return (static_cast<u32>(p[0]) << 24) | (static_cast<u32>(p[1]) << 16) |
           (static_cast<u32>(p[2]) << 8) | static_cast<u32>(p[3]);
}
inline u64 be64(const u8* p) {
    return (static_cast<u64>(be32(p)) << 32) | static_cast<u64>(be32(p + 4));
}

} // namespace cr

#endif // CR_BITS_H_
