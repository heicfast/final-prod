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
     * checkpoint variant embeds "CHKPSUPBMSB+ReFSFSRS".
     *
     * N-007 (the og's discovery width, ReFSSearchDLL :11689-11720,
     * re-verified first-hand): the og selects the variant string
     * ("SUPBMSB+ReFSFSRS" / "CHKPSUPBMSB+ReFSFSRS" / "MSB+ReFSFSRS")
     * and compares the FIRST DWORD ONLY (`*param_2 == *(int*)pcVar4`) -
     * the page's remaining bytes are validated downstream by the
     * ref-parser (FUN_18000ff60), not by the discovery gate. A page
     * whose first 4 bytes match but whose later signature bytes are
     * corrupt is therefore DISCOVERED by the og; rejecting it outright
     * made this probe blind on volumes where every page carries
     * late-byte corruption. Discovery here = first-DWORD; the full-width
     * memcmp survives as a GRADING signal (signature_partial), never a
     * rejection. The three first-DWORD keys are distinct ("SUPB",
     * "CHKP", "MSB+"), so no variant masking exists at 4 bytes. */
    constexpr u32 SIG_SUPB = 0x42505553; /* "SUPB" */
    constexpr u32 SIG_CHKP = 0x504B4843; /* "CHKP" */
    constexpr u32 SIG_MSB = 0x2B42534D;  /* "MSB+" */
    for (u32 off = 0; off + 32 <= probe; off += 512) {
        const u8* p = buf.data() + off;
        const u32 psig = u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
        if (psig != SIG_SUPB && psig != SIG_CHKP && psig != SIG_MSB)
            continue;
        /* og discovery: first-DWORD match opens the page */
        out.found = true;
        out.msb_offset = off;
        if (psig == SIG_CHKP)
            out.checkpointed = true;
        /* grading signal: the full-width signature (the pre-N-007 gate) */
        const char* full;
        size_t full_len;
        if (psig == SIG_CHKP) {
            full = "CHKPSUPBMSB+ReFSFSRS";
            full_len = 20;
        } else if (psig == SIG_SUPB) {
            full = "SUPBMSB+ReFSFSRS";
            full_len = 16;
        } else {
            full = "MSB+ReFSFSRS";
            full_len = 12;
        }
        if (std::memcmp(p, full, full_len) != 0)
            out.signature_partial = true;
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
