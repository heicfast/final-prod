/* cr_efifix.cpp - the UEFI NVRAM Boot#### load-option updater (M-040
 * stage b). Re-implemented from understanding of EFIBoot.mo
 * (re-workspace/reversed/ghidra-mo/EFIBoot_decompiled.c, corpus
 * 03ad46c); every gate cited below was re-verified first-hand.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/efifix/cr_efifix.h"
#include "cr/cr_error.h"

#include <cstdio>
#include <cstring>

namespace cr {
namespace efifix {

namespace {

/* ---- byte readers (the shared cr_bits fold keeps W4 quiet; local
 * use is self-contained here so the module stays header-only for the
 * consumers) ---- */
u8 rd8(const u8* p) { return *p; }
void wr8(u8* p, u8 v) { *p = v; }
u16 rd16(const u8* p) { return u16(p[0]) | (u16(p[1]) << 8); }
u32 rd32(const u8* p) { return u32(rd16(p)) | (u32(rd16(p + 2)) << 16); }
u64 rd64(const u8* p) { return u64(rd32(p)) | (u64(rd32(p + 4)) << 32); }
void wr32(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}
void wr64(u8* p, u64 v) {
    wr32(p, u32(v));
    wr32(p + 4, u32(v >> 32));
}

/* EFI_DEVICE_PATH node header: type u8@0, subtype u8@1, length u16@2
 * (includes the 4-byte header). The node types the walk cares about
 * (UEFI spec 10.3): */
constexpr u8 kPathMedia = 0x04;
constexpr u8 kMediaHardDrive = 0x01;
constexpr u8 kPathEndEntire = 0x7F;
constexpr u8 kEndEntireSubtype = 0xFF;
constexpr u32 kNodeHeaderSize = 4;
constexpr u32 kHardDriveNodeSize = 0x2A; /* 4 hdr + 4 num + 8 start + 8 size
                                          * + 16 sig + format + sigtype */

/* MEDIA_HARDDRIVE_DP field offsets inside the node (UEFI spec 10.3.3.6;
 * the og's reads at the same offsets, EFIBoot :3386-3420). */
constexpr u32 kHdPartitionNumber = 0x04;
constexpr u32 kHdStartLba = 0x08;
constexpr u32 kHdSizeLbas = 0x10;
constexpr u32 kHdSignature = 0x18;
constexpr u32 kHdFormat = 0x28;     /* 1 = MBR, 2 = GPT */
constexpr u32 kHdSignatureType = 0x29; /* 0 = none, 1 = 32-bit MBR sig, 2 = GUID */

constexpr u32 kSigTypeMbr = 1;
constexpr u32 kSigTypeGpt = 2;

/* The device-path start inside an EFI_LOAD_OPTION: u32 attributes @0,
 * u16 FilePathListLength @4, the UTF-16 NUL-terminated description @6,
 * then the file-path list (EFIBoot :3378-3382: buf + 6, skip the
 * string, then the nodes). */
constexpr u32 kLoadOptionDescriptionOffset = 6;

/* Locate the device-path list start inside a load option, clamped to
 * the buffer end. Returns size_t(-1) when malformed (no terminator or
 * no room for a node header after it) - the og walks unbounded; the
 * core refuses to overread (the registered hardening). */
size_t device_path_start(const u8* option, u32 len) {
    if (len < kLoadOptionDescriptionOffset + 2)
        return size_t(-1);
    size_t i = kLoadOptionDescriptionOffset;
    /* the description is UTF-16: NUL = two zero bytes */
    for (;;) {
        if (i + 2 > len)
            return size_t(-1); /* unterminated: malformed */
        if (option[i] == 0 && option[i + 1] == 0) {
            i += 2;
            break;
        }
        i += 2;
    }
    if (i + kNodeHeaderSize > len)
        return size_t(-1); /* no room for any node */
    return i;
}

} // namespace

bool boot_entry_matches(const PartitionRef& ref, const u8* load_option, u32 len) {
    if (load_option == nullptr || len == 0)
        return false;
    const size_t path_off = device_path_start(load_option, len);
    if (path_off == size_t(-1))
        return false;

    /* The og's sticky flags (FUN_1800046f0): bVar3 accumulates "some
     * HARDDRIVE node carried the signature", bVar4 "some node carried
     * the start+size"; the MBR-mode verdict is the AND of the two at
     * walk end. GPT mode breaks immediately on the GUID match. */
    bool sig_seen = false;
    bool geometry_seen = false;

    size_t off = path_off;
    for (;;) {
        if (off + kNodeHeaderSize > len)
            break; /* malformed tail: stop (the og would overread) */
        const u8 type = rd8(load_option + off);
        const u8 subtype = rd8(load_option + off + 1);
        const u16 node_len = rd16(load_option + off + 2);
        if (node_len < kNodeHeaderSize || off + node_len > len)
            break; /* hostile length: stop (the registered hardening) */
        if (type == kPathEndEntire && subtype == kEndEntireSubtype)
            break;
        if (type == kPathMedia && subtype == kMediaHardDrive &&
            node_len >= kHardDriveNodeSize) {
            const u8* node = load_option + off;
            const u32 format = rd8(node + kHdFormat);
            const u32 sig_type = rd8(node + kHdSignatureType);
            if (!ref.gpt && format == kSigTypeMbr && sig_type == kSigTypeMbr) {
                /* MBR: u32 disk signature AND start AND size (the og's
                 * int compare at :3392 + the two longlong compares at
                 * :3396-3400) */
                if (rd32(node + kHdSignature) == rd32(ref.signature))
                    sig_seen = true;
                if (rd64(node + kHdStartLba) == ref.start_lba &&
                    rd64(node + kHdSizeLbas) == ref.size_lbas)
                    geometry_seen = true;
            } else if (ref.gpt && format == kSigTypeGpt && sig_type == kSigTypeGpt) {
                /* GPT: the partition GUID only (the og's two u64
                 * compares at :3421-3424) */
                if (std::memcmp(node + kHdSignature, ref.signature, 16) == 0)
                    return true;
            }
        }
        off += node_len;
    }
    /* the MBR-mode verdict (FUN_1800046f0's tail: mode==0 && bVar2 && bVar4) */
    return !ref.gpt && sig_seen && geometry_seen;
}

u32 update_boot_entries(const FirmwareVars& fw, const PartitionRef& old_ref,
                        const PartitionRef& new_ref, ConsentFn consent, void* consent_user,
                        NvramStats* stats) {
    NvramStats local;
    if (stats == nullptr)
        stats = &local;

    if (!fw.complete())
        return ERR_BAD_PARAMETER;
    if (!fw.available(fw.user))
        return OK; /* the og's readiness gate: silent no-op (FUN_180004b40's
                    * zero-GUID probe + GetLastError() != 1) */

    /* BootOrder: the u16 Boot#### id array (the og reads it first,
     * copies it, and walks the copy). */
    std::vector<u8> order(kMaxLoadOptionBytes, 0);
    const u32 order_bytes = fw.get(fw.user, u"BootOrder", order.data(), u32(order.size()));
    if (order_bytes == 0 || (order_bytes & 1) != 0)
        return OK; /* absent or malformed: nothing to walk (the og's
                    * uVar10 == 0 no-op) */
    const u32 ids = order_bytes / 2;

    /* The consent gate: default-deny (nullptr callback) - nothing is
     * read for patching, nothing written. Checked BEFORE the first
     * matching write would happen, exactly like bcdfix's execute_plan. */
    const bool allowed = consent != nullptr ? consent(consent_user) : false;

    std::vector<u8> option(kMaxLoadOptionBytes, 0);
    for (u32 i = 0; i < ids; ++i) {
        const u16 id = rd16(order.data() + i * 2);
        ++stats->entries_scanned;

        /* the og's L"Boot%04X" (FUN_180004380) - uppercase hex, 4 wide.
         * Zero-initialized: the trailing bytes past the copied chars must
         * read as the terminator (an uninit tail made the lookup key
         * nondeterministic - caught by the -O0 sanitizer lane). */
        char16_t name[16] = {0};
        char narrow[16];
        std::snprintf(narrow, sizeof(narrow), "Boot%04X", unsigned(id));
        for (u32 c = 0; narrow[c] != 0 && c < 15; ++c)
            name[c] = char16_t(narrow[c]);

        const u32 opt_len = fw.get(fw.user, name, option.data(), u32(option.size()));
        if (opt_len == 0)
            break; /* the matcher's get failure stops the sweep (the og's
                    * first break site) */
        if (!boot_entry_matches(old_ref, option.data(), opt_len))
            continue;
        ++stats->entries_matched;

        if (!allowed)
            return ERR_REPAIR_REFUSED; /* refused before any write:
                                        * the P62 default-deny */

        /* Re-read confirmed the match (the og re-reads the variable
         * after the matcher, :3636-3642 - same buffer, same content in
         * the injected-resolver model). Patch every HARDDRIVE node. */
        const size_t path_off = device_path_start(option.data(), opt_len);
        if (path_off == size_t(-1))
            continue; /* malformed tail on a matched entry: skip (the
                       * registered hardening; the og overreads) */
        bool patched = false;
        size_t off = path_off;
        for (;;) {
            if (off + kNodeHeaderSize > opt_len)
                break;
            const u8 type = rd8(option.data() + off);
            const u8 subtype = rd8(option.data() + off + 1);
            const u16 node_len = rd16(option.data() + off + 2);
            if (node_len < kNodeHeaderSize || off + node_len > opt_len)
                break;
            if (type == kPathEndEntire && subtype == kEndEntireSubtype)
                break;
            if (type == kPathMedia && subtype == kMediaHardDrive &&
                node_len >= kHardDriveNodeSize) {
                u8* node = option.data() + off;
                /* the patch (FUN_180004b40 :3666-3687): partition
                 * number, start, size, signature - the MBR form writes
                 * the u32 signature only (the node's remaining 12
                 * signature bytes keep their zeros); the GPT form
                 * writes all 16 GUID bytes. */
                wr32(node + kHdPartitionNumber, new_ref.partition_number);
                wr64(node + kHdStartLba, new_ref.start_lba);
                wr64(node + kHdSizeLbas, new_ref.size_lbas);
                if (rd8(node + kHdSignatureType) == kSigTypeGpt) {
                    std::memcpy(node + kHdSignature, new_ref.signature, 16);
                    wr8(node + kHdFormat, kSigTypeGpt);
                    wr8(node + kHdSignatureType, kSigTypeGpt);
                } else {
                    wr32(node + kHdSignature, rd32(new_ref.signature));
                    wr8(node + kHdFormat, kSigTypeMbr);
                    wr8(node + kHdSignatureType, kSigTypeMbr);
                }
                patched = true;
            }
            off += node_len;
        }
        if (!patched)
            continue;
        if (!fw.set(fw.user, name, option.data(), opt_len))
            break; /* the write failure stops the sweep (the og's third
                     * break site); entries already patched stay patched */
        ++stats->entries_patched;
    }
    return OK;
}

} // namespace efifix
} // namespace cr
