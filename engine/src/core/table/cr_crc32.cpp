/* cr_crc32.cpp - CRC32 (IEEE 802.3 reflected).
 *
 * Shared by GPT validation and the session container record checks.
 * Table-driven, right-shifted, init 0xFFFFFFFF, final complement - the same
 * polynomial used by zlib/Linux crc32 (GPT requirement).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/table/cr_table_wire.h"

namespace cr {
namespace table {

namespace {

struct Crc32Table {
    u32 t[256];
    Crc32Table() {
        for (u32 i = 0; i < 256; ++i) {
            u32 c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            t[i] = c;
        }
    }
};
const Crc32Table g_table;

} // namespace

u32 crc32(const void *data, size_t len) {
    const u8 *p = static_cast<const u8 *>(data);
    u32 c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        c = g_table.t[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

} // namespace table
} // namespace cr
