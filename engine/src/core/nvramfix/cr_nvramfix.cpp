/* cr_nvramfix.cpp - the UEFI NVRAM Boot#### entry updater (M-040 stage b).
 *
 * Pure logic over an injected firmware-variable store; every mechanism
 * is the corpus EFIBoot.mo decode documented in cr_nvramfix.h
 * (FUN_1800046f0 the matcher, FUN_180004840 the state-4 update pass).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/nvramfix/cr_nvramfix.h"
#include "cr/cr_error.h"
#include <cstdio>
#include <cwchar>
#include <vector>

namespace cr {
namespace nvramfix {

const wchar_t kEfiGlobalGuid[] = L"{8BE4DF61-93CA-11d2-AA0D-00E098032B8C}";

namespace {

/* little-endian readers (the firmware variables are LE on every UEFI
 * implementation the product runs on — the og reads them with plain
 * x86 loads) */
u16 rd16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
u64 rd64(const u8* p) {
    return u64(rd32(p)) | (u64(rd32(p + 4)) << 32);
}
void wr32(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

/* The device-path cursor: EFIBoot walks {type, subtype, length u16}
 * nodes; END_ENTIRE (0x7f/0xff) terminates. The cursor also guards
 * against zero/short lengths (a malformed node would otherwise loop
 * forever — the og trusts the firmware; we stop the walk, the match
 * simply sees fewer nodes). */
template <typename Byte /* u8 (patch walk) or const u8 (match walk) */>
struct NodeWalkT {
    Byte* p = nullptr;
    size_t left = 0;
    bool ended = false;

    explicit NodeWalkT(Byte* path, size_t size) : p(path), left(size) {}

    /* Advance to the next node; sets ended at END_ENTIRE or exhaustion. */
    void next() {
        if (left < 4) {
            ended = true;
            return;
        }
        const u16 len = rd16(p + 2);
        if (len < 4 || size_t(len) > left) {
            ended = true; /* malformed length: stop (see the comment above) */
            return;
        }
        if (p[0] == 0x7f && p[1] == 0xff) {
            ended = true;
            return;
        }
        p += len;
        left -= len;
    }
};

using NodeWalk = NodeWalkT<const u8>;
using NodeWalkMut = NodeWalkT<u8>;

} // namespace

bool load_option_matches(const u8* option, size_t size, const Target& t) {
    /* EFI_LOAD_OPTION: attributes u32 @0, FilePathListLength u16 @4,
     * description UTF-16 @6, FilePathList @ 6 + (description + NUL) * 2.
     * The og skips the description by scanning for the UTF-16 NUL
     * (FUN_1800046f0 :3429-3435) — FilePathListLength is not consulted
     * on this path (mirrored). */
    if (option == nullptr || size < 6)
        return false;
    size_t at = 6;
    /* skip the UTF-16 description incl. its NUL */
    while (at + 1 < size && (option[at] != 0 || option[at + 1] != 0))
        at += 2;
    at += 2; /* the NUL */
    if (at > size)
        return false;

    /* the flag semantics: MBR mode collects signature-match and
     * geometry-match flags ACROSS THE WHOLE WALK (the og does not
     * require them on the same node); GPT mode matches on the first
     * GUID-equal node. */
    bool sig_match = false;
    bool geom_match = false;
    NodeWalk w(option + at, size - at);
    while (!w.ended) {
        if (w.left >= kHdNodeSize && w.p[0] == 0x04 && w.p[1] == 0x01) {
            if (!t.gpt) {
                /* MBR mode: MBRType 1 + SignatureType 1; the u32 at
                 * +0x18 == the disk signature sets the signature flag;
                 * PartitionStart == old LBA AND PartitionSize == size
                 * set the geometry flag. */
                if (w.p[kHdSignatureType] == 1 && w.p[kHdMbrType] == 1) {
                    if (rd32(w.p + kHdSignature) == t.mbr_signature)
                        sig_match = true;
                    if (rd64(w.p + kHdPartitionStart) == t.old_partition_start &&
                        rd64(w.p + kHdPartitionSize) == t.partition_size)
                        geom_match = true;
                }
            } else {
                /* GPT mode: MBRType 2 + SignatureType 2 + the 16-byte
                 * GUID at +0x18 == the partition GUID -> immediate. */
                if (w.p[kHdSignatureType] == 2 && w.p[kHdMbrType] == 2 &&
                    rd64(w.p + kHdSignature) == t.gpt_guid[0] &&
                    rd64(w.p + kHdSignature + 8) == t.gpt_guid[1])
                    return true;
            }
        }
        w.next();
    }
    return !t.gpt && sig_match && geom_match;
}

u32 update_boot_entries(IFirmwareVarStore& store, const Target& t, UpdateStats* stats) {
    UpdateStats local;
    /* stats report on EVERY exit (the abort paths carry the partial
     * progress — the og's loop breaks mid-walk the same way). */
    struct StatsGuard {
        UpdateStats* out;
        UpdateStats& local;
        ~StatsGuard() {
            if (out)
                *out = local;
        }
    } guard{stats, local};

    /* BootOrder: the u16 id list. Absent (ERR_OPEN_FAILED) = nothing to
     * do — the og's zero-byte BootOrder no-op. Any other error is an
     * unusable API: propagate. */
    std::vector<u8> order(kMaxLoadOptionBytes);
    u32 order_len = 0;
    const u32 rc = store.get(L"BootOrder", order.data(), u32(order.size()), &order_len);
    if (rc == ERR_OPEN_FAILED)
        return OK; /* no BootOrder: zero entries, clean no-op */
    if (rc != OK)
        return rc;
    order.resize(order_len);
    if (order.size() % 2 != 0)
        order.pop_back(); /* odd tail: the og iterates (len>>1) u16s */
    const size_t count = order.size() / 2;

    std::vector<u8> option(kMaxLoadOptionBytes);
    for (size_t i = 0; i < count; ++i) {
        const u16 id = rd16(order.data() + i * 2);
        wchar_t name[16];
        /* Boot%04X — the og's exact format (FUN_180004380) */
        std::swprintf(name, 16, L"Boot%04X", unsigned(id));
        ++local.inspected;

        u32 opt_len = 0;
        const u32 grc = store.get(name, option.data(), u32(option.size()), &opt_len);
        if (grc != OK)
            return grc; /* the og's whole-loop break (see the header) */
        if (opt_len < 6)
            continue; /* nothing to walk */

        if (!load_option_matches(option.data(), opt_len, t))
            continue;
        ++local.matched;

        /* the patch: EVERY HARDDRIVE node with SignatureType != 0 gets
         * the new PartitionNumber (FUN_180004840 :3533-3544). The walk
         * mirrors the matcher's description skip. */
        size_t at = 6;
        while (at + 1 < opt_len && (option[at] != 0 || option[at + 1] != 0))
            at += 2;
        at += 2;
        if (at > opt_len)
            continue;
        bool patched = false;
        NodeWalkMut w(option.data() + at, opt_len - at);
        while (!w.ended) {
            if (w.left >= kHdNodeSize && w.p[0] == 0x04 && w.p[1] == 0x01 &&
                w.p[kHdSignatureType] != 0) {
                wr32(w.p + kHdPartitionNumber, t.new_partition_number);
                patched = true;
            }
            w.next();
        }
        if (!patched)
            continue; /* matched but no patchable node: nothing written */

        const u32 src = store.set(name, option.data(), opt_len);
        if (src != OK)
            return src;
        ++local.patched;
    }

    return OK;
}

} // namespace nvramfix
} // namespace cr
