/*
<<<<<<< HEAD
 * cr_recycle.h - the shared Windows recycle-bin model (M-052).
 *
 * Extracted from the NTFS engine's in-repo reference implementation
 * (cr_mftwalk.cpp: $I/$R pairing) so the FAT and exFAT engines carry
 * the same capability the corpus documents for all three
 * (fat-exfat-engine-internals.md :21, :37-40, :71, :101: the roots
 * $RECYCLE.BIN / Recycled / Recycler matched byte-exact, case-blind;
 * the $I record carries the original path/size/time; the $R carries
 * the content; pairing is by the random token after the marker).
=======
 * cr_recycle.h - shared Windows recycle-bin model ($I/$R pairing).
 *
 * M-052 (diff-to-fix): the og's FAT and exFAT engines both recognize the
 * Windows recycle-bin roots and surface recycle-bin deletions through the
 * $I/$R info/restore pairing - the same capability the core implemented
 * for NTFS only (cr_mftwalk.cpp). This header is the shared model the
 * three engines consume; the NTFS walk keeps its own thin wrappers so its
 * landed behavior (and tests) are unchanged except where the M-052
 * verification found a scope defect (see N-079, findings/FIXED/).
 *
 * Corpus ground truth (verified first-hand, session DFX35):
 *   - Root recognition is _stricmp over FOUR names, both engines:
 *     FATSearchDLL_decompiled.c :1668-1677 ("Recycler", "Recycled",
 *     "$Recycle.Bin", "$RECYCLE.BI0"), exFATSearchDLL_decompiled.c
 *     :40583-40592 (same four), NTFSSearchDLL_decompiled.c :37387-37410
 *     (same four). "$RECYCLE.BI0" is the FAT-short-name mangled form the
 *     og matches deliberately. Other "$RECYCLE.BIN" _stricmp sites
 *     (FAT :11799/:15974/:16014/:16053/:30388, exFAT :4066/:6720/:6830/
 *     :7152/:13121/:13243/:42135) are the same comparison case-insensitive.
 *   - The pairing: "$I<token>" is the Vista+ metadata record (original
 *     path + size + FILETIME), "$R<token>" is the content; the og pairs
 *     by token "by index across bin subfolders"
 *     (fat-exfat-engine-internals.md :37-40, IRecycleBinNode2UI).
 *   - The og scopes the handling to the recognized bin roots (the
 *     ancestor-chain walk over FUN_18002e5b0, NTFSSearchDLL :43538-
 *     :43550; the S- SID-subfolder context :53985-53996).
>>>>>>> 880e2a1b1429859df8d575aa53e835d1d65fe01f
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RECYCLE_H_
#define CR_RECYCLE_H_

#include "cr_types.h"
#include <string>

namespace cr {
namespace recycle {

<<<<<<< HEAD
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
=======
/* One $I metadata record's user-relevant fields. */
struct Info {
    u64 size = 0;               /* original file size (u64 @+8) */
    u64 time = 0;               /* deletion FILETIME, 100-ns since 1601 (u64 @+16) */
    std::u16string orig_name;   /* basename of the recorded original path */
};

/* Parse a $I record image (Vista..Win8 fixed v2 = 544 bytes; Windows 10+
 * variable form = 28 + 2*pathlen). Shared shape with the NTFS landing's
 * parse_recycle_info (cr_mftwalk.cpp), which carried the runner-benchmark
 * evidence for the format-dependent pathlen determination:
 *   - version u32 @0 (1 or 2; anything else is not a $I record)
 *   - size u64 @8, deletion time u64 @16
 *   - v2 fixed form (dlen == 544): pathlen = u32 @0x18
 *   - Win10 variable form: @0x18 was repurposed as a deletion-sequence
 *     counter; the path is the whole tail, NUL-padded
 *   - try @0x18 when it validates against dlen, else derive from the
 *     tail length; the string is cut at the first NUL either way
 * Returns false when the image is not a defensible $I record (then the
 * caller treats the file as ordinary content - never bin metadata). */
inline bool parse_record(const u8* d, u32 dlen, Info& out) {
    if (d == nullptr || dlen < 30)
        return false;
    const u32 ver = u32_from_bytes_le(d);
    if (ver != 1 && ver != 2)
        return false;
    const u64 size = u64_from_bytes_le(d + 8);
    const u64 t = u64_from_bytes_le(d + 16);
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

/* Pairing token: the random chars after the $I/$R marker, up to the
 * first '.' (the shell's token never contains one; the $R keeps the
 * ORIGINAL EXTENSION while its $I does not - $R2X9K4L.jpg pairs with
 * $I2X9K4L - so the extension is NOT part of the token). Empty when the
 * name is not a $I/$R-form bin name. */
inline std::u16string token_of(const std::u16string& n) {
    if (n.size() < 3 || n[0] != u'$' || (n[1] != u'I' && n[1] != u'R'))
        return {};
    std::u16string tok;
    for (size_t i = 2; i < n.size(); ++i) {
        if (n[i] == u'.')
            break;
        tok.push_back(n[i]);
    }
    return tok;
}

inline bool ascii_ieq(char32_t a, char32_t b) {
    if (a >= u'A' && a <= u'Z')
        a += 32;
    if (b >= u'A' && b <= u'Z')
        b += 32;
    return a == b;
}

/* Bin-root recognition: the og's four-name _stricmp set (case-insensitive,
 * ASCII). Verified byte-exact in all three SearchDLLs (citations in the
 * header comment). Used both for the walk-time subtree marking (FAT/exFAT)
 * and the post-walk ancestry check (all engines). */
inline bool is_bin_root(const std::u16string& n) {
    static const std::u16string kRoots[] = {u"Recycler", u"Recycled", u"$Recycle.Bin",
                                            u"$RECYCLE.BI0"};
    for (const std::u16string& r : kRoots) {
        if (n.size() != r.size())
            continue;
        bool same = true;
        for (size_t i = 0; i < n.size(); ++i)
            if (!ascii_ieq(n[i], r[i])) {
                same = false;
                break;
            }
        if (same)
            return true;
    }
    return false;
}
>>>>>>> 880e2a1b1429859df8d575aa53e835d1d65fe01f

} // namespace recycle
} // namespace cr

#endif // CR_RECYCLE_H_
