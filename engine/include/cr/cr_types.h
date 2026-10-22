/*
 * cr_types.h - shared scalar types for the core_recovery stack.
 *
 * Everything here compiles identically on MSVC (C++20) and gcc/clang (C++20,
 * Linux CI). No OS headers may be included from this file or anything it
 * includes - the pure core must build on both sides (dual-verification, the
 * same methodology as rda-driver's shared headers).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TYPES_H_
#define CR_TYPES_H_

#include <cstdint>
#include <cstddef>

namespace cr {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

constexpr u32 u32_from_bytes_le(const u8 *p) {
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) |
           (static_cast<u32>(p[2]) << 16) | (static_cast<u32>(p[3]) << 24);
}
constexpr u16 u16_from_bytes_le(const u8 *p) {
    return static_cast<u16>(static_cast<u16>(p[0]) | (static_cast<u16>(p[1]) << 8));
}
constexpr u64 u64_from_bytes_le(const u8 *p) {
    return static_cast<u64>(p[0]) | (static_cast<u64>(p[1]) << 8) |
           (static_cast<u64>(p[2]) << 16) | (static_cast<u64>(p[3]) << 24) |
           (static_cast<u64>(p[4]) << 32) | (static_cast<u64>(p[5]) << 40) |
           (static_cast<u64>(p[6]) << 48) | (static_cast<u64>(p[7]) << 56);
}

/* Round up v to a multiple of m (m > 0). */
constexpr u64 round_up(u64 v, u64 m) { return (v + m - 1) / m * m; }
constexpr u64 round_down(u64 v, u64 m) { return v / m * m; }

} // namespace cr

#endif // CR_TYPES_H_
