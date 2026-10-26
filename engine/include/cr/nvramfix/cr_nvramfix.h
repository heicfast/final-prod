/*
 * cr_nvramfix.h - the UEFI NVRAM Boot#### entry updater (M-040 stage b).
 *
 * Corpus lineage (EFIBoot.mo — re-workspace/reversed/ghidra-mo/
 * EFIBoot_decompiled.c, every mechanism below re-verified first-hand):
 * after a partition layout change the og walks the firmware's BootOrder
 * variable, and for every Boot#### load option whose device path
 * references the target partition it rewrites the HARDDRIVE device-path
 * node's PartitionNumber and writes the variable back through
 * SetFirmwareEnvironmentVariable (EFI global GUID
 * {8BE4DF61-93CA-11d2-AA0D-00E098032B8C}).
 *
 * The matcher (FUN_1800046f0): the load option is {attributes u32,
 * FilePathListLength u16, description UTF-16, FilePathList...}; the
 * device path is walked node by node {type u8, subtype u8, length u16}
 * until the END_ENTIRE node (type 0x7f, subtype 0xff). A
 * MEDIA_HARDDRIVE_DP node (type 0x04, subtype 0x01) is 0x2A bytes:
 * PartitionNumber u32 @+4, PartitionStart u64 @+8, PartitionSize u64
 * @+0x10, Signature[16] @+0x18, MBRType u8 @+0x28, SignatureType u8
 * @+0x29.
 *   - MBR mode: MBRType == 1 (PC-AT) AND SignatureType == 1 (MBR
 *     signature); the u32 at +0x18 == the disk's MBR signature sets the
 *     signature flag; PartitionStart == the partition's old start LBA
 *     AND PartitionSize == its size set the geometry flag. The flags are
 *     collected ACROSS THE WHOLE WALK (the og does not require both on
 *     the same node) and the option matches when both hold.
 *   - GPT mode: MBRType == 2 AND SignatureType == 2; the 16-byte GUID at
 *     +0x18 equals the partition GUID -> immediate match.
 *
 * The patch (FUN_180004840, the state-4 update): in every MATCHED load
 * option, EVERY HARDDRIVE node with SignatureType != 0 has its
 * PartitionNumber (u32 @+4) rewritten to the new partition number, and
 * the whole load option is written back. Create/delete of entries and
 * BootOrder rewrites are NOT part of the update path (they live in the
 * og's create state machine, FUN_1800012c0 — deliberately out of scope
 * here per the finding's fix plan: "keep create/delete out of v1").
 *
 * The walk aborts at the first failing variable read, mirroring the og
 * (FUN_1800046f0 returns 2 on a zero-byte read and FUN_180004840 breaks
 * the whole loop; a BootOrder entry whose Boot#### variable is gone
 * stops the entire pass — the reference's own fragility, preserved and
 * documented, not silently "improved").
 *
 * The firmware-variable API is INJECTED (IFirmwareVarStore below): the
 * og resolves Get/SetFirmwareEnvironmentVariableW dynamically at
 * runtime (:3281-3290 — the exports do not exist on legacy-BIOS
 * systems); the win layer owns that resolution plus the
 * SeSystemEnvironmentPrivilege enable (:3248-3270). The pure module
 * sees only the store. NOTE the og additionally PRE-PROBES the API
 * (a dummy get whose GetLastError()==1 means ERROR_INVALID_FUNCTION);
 * this interface folds that distinction into the get() contract —
 * ERR_OPEN_FAILED = the benign variable-absent case, any other error =
 * the API is unusable.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_NVramFIX_H_
#define CR_NVramFIX_H_

#include "../cr_types.h"
#include <cstddef>

namespace cr {
namespace nvramfix {

/* The EFI global variable GUID string (every Boot####/BootOrder access
 * carries it; EFIBoot_decompiled.c :3417 et al.). */
extern const wchar_t kEfiGlobalGuid[];

/* MEDIA_HARDDRIVE_DP node layout (UEFI spec 10.1.1; the og's field
 * reads at FUN_1800046f0 map onto it exactly — see the header comment). */
constexpr size_t kHdNodeHeader = 4;       /* type u8, subtype u8, length u16 */
constexpr size_t kHdPartitionNumber = 4;  /* u32 — THE PATCHED FIELD */
constexpr size_t kHdPartitionStart = 8;   /* u64, LBA */
constexpr size_t kHdPartitionSize = 0x10; /* u64, LBA count */
constexpr size_t kHdSignature = 0x18;     /* 16 bytes: MBR sig u32 (+12 pad) or GPT GUID */
constexpr size_t kHdMbrType = 0x28;       /* u8: 1 = PC-AT, 2 = GPT */
constexpr size_t kHdSignatureType = 0x29; /* u8: 1 = MBR signature, 2 = GUID */
constexpr size_t kHdNodeSize = 0x2A;      /* 42 bytes */

/* The og's load-option read buffer cap (FUN_1800046f0's 0x100000). */
constexpr u32 kMaxLoadOptionBytes = 1u << 20;

/* The injected firmware-variable store. The win layer implements this
 * over dynamically resolved Get/SetFirmwareEnvironmentVariableW
 * (EFIBoot :3281-3290) with SeSystemEnvironmentPrivilege enabled.
 *
 * get() contract:
 *   OK            — variable read, *out_len bytes copied (out_len <= buf_len)
 *   ERR_OPEN_FAILED — the variable is NOT PRESENT (the benign case)
 *   other         — the API is unusable (privilege/firmware); aborts the pass
 */
class IFirmwareVarStore {
public:
    virtual ~IFirmwareVarStore() = default;
    virtual u32 get(const wchar_t* name, void* buf, u32 buf_len, u32* out_len) = 0;
    virtual u32 set(const wchar_t* name, const void* buf, u32 len) = 0;
};

/* The target partition identity + the patch value (the og's context
 * fields: mode @+0x30, signature/GUID @+0x34/+0x3c, new partition
 * number @+0x44, old start LBA @+0x48, size @+0x50). */
struct Target {
    bool gpt = false;             /* false = MBR mode, true = GPT mode */
    u32 mbr_signature = 0;        /* MBR mode: the disk signature */
    u64 gpt_guid[2] = {0, 0};     /* GPT mode: the partition GUID */
    u64 old_partition_start = 0;  /* MBR mode: the partition's OLD start LBA */
    u64 partition_size = 0;       /* MBR mode: its size in LBAs */
    u32 new_partition_number = 0; /* THE PATCH VALUE (HARDDRIVE node +4) */
};

struct UpdateStats {
    size_t inspected = 0; /* Boot#### ids seen in BootOrder */
    size_t matched = 0;   /* load options matching the target */
    size_t patched = 0;   /* variables written back */
};

/* Does one load option's device path match the target? (FUN_1800046f0
 * exactly: the flag semantics of the header comment. Exposed for
 * testing; update_boot_entries is the production entry.) */
bool load_option_matches(const u8* option, size_t size, const Target& t);

/* The state-4 update pass (FUN_180004840): walk BootOrder, patch every
 * matched Boot####'s HARDDRIVE PartitionNumbers, write back through the
 * store. An absent BootOrder is a clean no-op (OK, zero stats — the og's
 * zero-byte BootOrder case); any other variable-read failure aborts the
 * pass and returns that code with the stats accumulated so far (the
 * og's break-the-whole-loop posture). */
u32 update_boot_entries(IFirmwareVarStore& store, const Target& t,
                        UpdateStats* stats = nullptr);

} // namespace nvramfix
} // namespace cr

#endif // CR_NVramFIX_H_
