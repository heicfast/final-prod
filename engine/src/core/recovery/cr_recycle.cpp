/* cr_recycle.cpp - the shared Windows recycle-bin model (M-052).
 *
 * The $I/$R pairing extracted from the NTFS engine's reference
 * implementation into the engine-neutral form: every consumer hands
 * over the file's NAME and its CONTENT STREAM bytes.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_recycle.h"

#include <cstring>

namespace cr {
namespace recycle {

namespace {

/* ASCII-case-insensitive u16 compare (the corpus's _stricmp family;
 * the bin roots are ASCII by construction). */
bool ieq_ascii(const std::u16string& a, const char16_t* b) {
    size_t i = 0;
    for (; b[i] != 0; ++i) {
        if (i >= a.size())
            return false;
        char16_t x = a[i];
        char16_t y = b[i];
        if (x >= u'a' && x <= u'z')
            x = char16_t(x - u'a' + u'A');
        if (y >= u'a' && y <= u'z')
            y = char16_t(y - u'a' + u'A');
        if (x != y)
            return false;
    }
    return i == a.size();
}

} // namespace

bool is_root_name(const std::u16string& dir_name) {
    return ieq_ascii(dir_name, u"$RECYCLE.BIN") || ieq_ascii(dir_name, u"Recycled") ||
           ieq_ascii(dir_name, u"Recycler");
}

std::u16string token(const std::u16string& file_name) {
    if (file_name.size() < 3 || file_name[0] != u'$' ||
        (file_name[1] != u'I' && file_name[1] != u'R'))
        return {};
    std::u16string tok;
    for (size_t i = 2; i < file_name.size(); ++i) {
        if (file_name[i] == u'.')
            break;
        tok.push_back(file_name[i]);
    }
    return tok;
}

bool parse_stream(const u8* d, u32 dlen, RecycleInfo& out) {
    if (d == nullptr || dlen < 30)
        return false;
    const u32 ver = u32_from_bytes_le(d);
    if (ver != 1 && ver != 2)
        return false;
    const u64 size = u64_from_bytes_le(d + 8);
    const u64 t = u64_from_bytes_le(d + 16);
    /* Path length determination is FORMAT-DEPENDENT (runner benchmark
     * evidence from the NTFS side - every $I parse was rejected on the
     * real volume until the variable form was honored):
     * - Vista..Win8 fixed form (dlen==544): pathlen is the u32@0x18.
     * - Windows 10+ variable form (dlen == 28 + 2*pathlen): @0x18 was
     *   repurposed as a deletion-sequence counter; the path is the
     *   whole tail, NUL-padded. Try the field when it validates
     *   against dlen, else derive from the tail length; cut at the
     *   first NUL either way. */
    const u32 field24 = u32_from_bytes_le(d + 24);
    const u32 derived = (dlen >= 28 && ((dlen - 28) & 1) == 0) ? u32((dlen - 28) / 2) : 0;
    u32 plen = 0;
    if (field24 >= 1 && field24 <= 1024 && 28 + u64(field24) * 2 <= dlen)
        plen = field24;
    else if (derived >= 1)
        plen = derived;
    if (plen == 0 || plen > 1024)
        return false;
    std::u16string path;
    path.reserve(plen);
    for (u32 i = 0; i < plen; ++i) {
        const u16 ch = u16_from_bytes_le(d + 28 + i * 2);
        if (ch == 0)
            break; /* NUL-terminated / NUL-padded tail */
        path.push_back(ch);
    }
    if (path.empty())
        return false;
    const size_t slash = path.rfind(u'\\');
    const std::u16string base = slash == std::u16string::npos ? path : path.substr(slash + 1);
    if (base.empty())
        return false;
    out.size = size;
    out.time = t;
    out.orig_name = base;
    return true;
}

} // namespace recycle
} // namespace cr
