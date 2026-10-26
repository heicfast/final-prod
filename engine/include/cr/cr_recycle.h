/*
 * cr_recycle.h - the shared Windows recycle-bin model (M-052).
 *
 * Extracted from the NTFS engine's in-repo reference implementation
 * (cr_mftwalk.cpp: $I/$R pairing) so the FAT and exFAT engines carry
 * the same capability the corpus documents for all three
 * (fat-exfat-engine-internals.md :21, :37-40, :71, :101: the roots
 * $RECYCLE.BIN / Recycled / Recycler matched byte-exact, case-blind;
 * the $I record carries the original path/size/time; the $R carries
 * the content; pairing is by the random token after the marker).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RECYCLE_H_
#define CR_RECYCLE_H_

#include "cr_types.h"
#include <string>

namespace cr {
namespace recycle {

/* The original-file metadata parsed from a $I<token> stream. */
struct RecycleInfo {
    u64 size = 0;             /* original file size (recorded at deletion) */
    u64 time = 0;             /* deletion FILETIME (100ns since 1601) */
    std::u16string orig_name; /* basename of the recorded original path */
};

/* A recycle-bin ROOT directory name: $RECYCLE.BIN (Vista+), Recycled
 * (Win9x/ME), Recycler (NT4..2000) - case-insensitive, exact match
 * (the corpus's byte-exact _stricmp family). Subdirectories of a root
 * (the per-SID folders) are bins by inheritance, not by name. */
bool is_root_name(const std::u16string& dir_name);

/* The pairing token of a $I/$R bin file: the chars after the marker up
 * to the first '.' (the $R keeps the original extension, the $I does
 * not - $R2X9K4L.jpg pairs with $I2X9K4L; the token never contains a
 * dot). Empty when the name is not a $I/$R marker shape. */
std::u16string token(const std::u16string& file_name);

/* Parse a $I stream (v1: 28 + 2*pathlen bytes; v2: 544 fixed).
 * Shared field map: version u32@0, size u64@8, time u64@16, then the
 * original path - v2 as the u32@0x18 length when it validates against
 * the buffer, else the whole NUL-terminated tail (the Windows 10+
 * variable form repurposed @0x18 as a deletion sequence counter).
 * Returns false when the bytes are not a parseable $I record. */
bool parse_stream(const u8* d, u32 dlen, RecycleInfo& out);

} // namespace recycle
} // namespace cr

#endif // CR_RECYCLE_H_
