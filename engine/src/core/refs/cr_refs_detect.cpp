/* cr_refs_detect.cpp - lightweight ReFS triage probe (boot-sector + MSB
 * signature scan for partition classification). The FULL engine - MSB-chain
 * reconstruction, hierarchy walk, deleted harvest - lives in
 * cr_refs_engine.cpp (scan_refs); ReFS is first-class there.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/refs/cr_refs_detect.h"
#include <cstdio>
#include <cstring>

namespace cr {
namespace refs {

namespace {
constexpr u32 PROBE_BYTES = 1u << 20;
/* The MSB (metadata session block) chain carries "MSB+ReFSFSRS" at its
 * page start; the identification record near the volume start carries the
 * ASCII version ("MS" version pairs). */
} // namespace

u32 detect_refs(IStorageDevice* slice, RefsInfo& out) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    const u64 size = slice->size();
    const u32 probe = size < PROBE_BYTES ? u32(size) : PROBE_BYTES;
    if (probe < 4096)
        return ERR_BAD_PARAMETER;
    std::vector<u8> buf(probe);
    if (slice->read_at(0, buf.data(), probe) != OK)
        return ERR_READ_FAILED;

    /* "MSB+ReFSFSRS" appears at the start of metadata session pages; the
     * checkpoint variant embeds "CHKPSUPBMSB+ReFSFSRS". Both start with
     * the 16-byte ReFSFSRS page identity at offset 8.
     *
     * N-007 (diff-to-fix): the og's dispatch matches the FIRST DWORD
     * only (ReFSSearchDLL :11689-11720: `*param_2 == *(int*)pcVar4`
     * against "SUPB"/"CHKP"/"MSB+") - a page with a corrupt signature
     * tail is still opened and left to the ref-parser's validation. The
     * core now matches that discovery rule; the full-width memcmp stays
     * as the grading signal (signature_partial = first-4 hit, full
     * compare failed). */
    auto page_match = [&](const char* sig, u32 sig_len, const u8* p) -> int {
        /* returns: 0 = no match, 1 = full match, 2 = first-dword only */
        if (std::memcmp(p, sig, 4) != 0)
            return 0;
        if (std::memcmp(p, sig, sig_len) == 0)
            return 1;
        return 2;
    };
    for (u32 off = 0; off + 32 <= probe; off += 512) {
        const u8* p = buf.data() + off;
        if (const int m = page_match("SUPBMSB+ReFSFSRS", 16, p)) {
            out.found = true;
            out.msb_offset = off;
            out.signature_partial = (m == 2);
            continue;
        }
        if (const int m = page_match("CHKPSUPBMSB+ReFSFSRS", 20, p)) {
            out.found = true;
            out.checkpointed = true;
            out.msb_offset = off;
            out.signature_partial = (m == 2);
            continue;
        }
        if (const int m = page_match("MSB+ReFSFSRS", 12, p)) {
            out.found = true;
            out.msb_offset = off;
            out.signature_partial = (m == 2);
        }
    }
    if (!out.found) {
        /* ReFS v3+ identification: the FS name "ReFS" at volume offset
         * 0x200 of the identification page. The version comes from the
         * boot sector's real major/minor bytes (+0x28/+0x29, the same
         * fields the engine reads - audit row 28: the old fallback
         * fabricated the string "ReFS" instead). */
        if (probe > 0x204 && std::memcmp(buf.data() + 0x200, "ReFS", 4) == 0) {
            out.found = true;
            if (probe > 0x2A) {
                const int major = buf[0x28];
                const int minor = buf[0x29];
                snprintf(out.version, sizeof(out.version), "ReFS v%d.%d", major, minor);
            } else {
                std::memcpy(out.version, "ReFS", 4);
            }
        } else {
            return ERR_BAD_MAGIC;
        }
    }
    /* sequence: little-endian u32 at MSB+0x0C (page header field) when an
     * MSB page was found */
    if (out.msb_offset + 16 <= probe) {
        const u8* q = buf.data() + out.msb_offset + 12;
        out.sequence = u32(q[0]) | (u32(q[1]) << 8) | (u32(q[2]) << 16) | (u32(q[3]) << 24);
    }
    return OK;
}

} // namespace refs
} // namespace cr
