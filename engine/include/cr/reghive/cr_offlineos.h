/*
 * cr_offlineos.h - the offline Windows OS profile (M-082): the og
 * DeviceManager's CPartitionBootInfo hive walk, ported as a pure C++ API
 * over the reghive regf parser.
 *
 * Purpose: identify WHICH partition is the (offline) system partition and
 * WHAT Windows it hosts - the input to boot repair, offline-OS-targeted
 * recovery, and PE-context flows. The og's DeviceManager.mo (live-loaded by
 * name - diff-to-fix evidence s25-oq-s5-1-checkversion-dead-shipped.md:13)
 * mounts a candidate partition's
 * "<root>\Windows\System32\config\{SOFTWARE,SYSTEM}" through its in-house
 * HMG hive classes (the mount tags "SOFTWARE_EASEUS_HMG" /
 * "SYSTEM_EASEUS_HMG", DeviceManager_decompiled.c :62021/:62025) and fills
 * a 0x418-byte profile record. This module reproduces that walk read-only:
 *   - the 8 identity fields of
 *     \Microsoft\Windows NT\CurrentVersion (SOFTWARE hive; each capped at
 *     0x80 bytes, present-if-found, never fatal - the og's
 *     lstrcpynA-if-found shape :62034-62119),
 *   - SystemRoot lowercased and accepted ONLY in drive-letter form
 *     ("x:" with x in a-z) - the og's gate at :62113-62116,
 *   - the system partition's location from SYSTEM's
 *     \MountedDevices\DosDevices\<sysroot-drive>: - a 12-byte REG_BINARY
 *     value: u32 disk signature + u64 partition start offset (:62184-62195,
 *     stored at record +0x408/+0x410),
 *   - the real ControlSet via \Select\Current with the \ControlSet001
 *     fallback (:62213-62245 - the og DOES consult Select here, unlike the
 *     FVE harvest's hardcoded 001; corpus cr_reghive.h's own note),
 *   - the ProductOptions\ProductType classification:
 *     "WinNT" -> 1, "LanmanNT" -> 2, "ServerNT" -> 3 (lstrcmpiA,
 *     case-insensitive; :62275-62322),
 *   - the fixed 0x0100 stamp at record +0x402 (the og's own record marker,
 *     :62323 - constant, kept in the record shape for fidelity).
 *
 * Layering: core-only C++ (no OS API, no ABI export yet - the ABI lands
 * when a host-facing flow consumes it, per the M-082 fix plan; boot-fix
 * auto-locate is the recorded follow-up consumer).
 *
 * Safety: strictly read-only; every field absent-key-tolerant; value reads
 * capped by RegistryHive's own kFveMaxHiveValueBytes.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_OFFLINEOS_H_
#define CR_OFFLINEOS_H_

#include "cr_reghive.h"
#include <cstddef>
#include <string>

namespace cr {
namespace reghive {
namespace offlineos {

/* The og's 0x418-byte profile record, field-mapped. Empty/zero fields mean
 * "absent in the hive" (the og skips missing keys - a profile with only
 * some fields is a valid partial, e.g. a SOFTWARE-only pass). */
struct OsProfile {
    /* \Microsoft\Windows NT\CurrentVersion (SOFTWARE) - 0x80 each,
     * record offsets +0x000..+0x37F, NUL-terminated copies. */
    char build_lab[0x80] = {0};       /* +0x000 BuildLab */
    char build_lab_ex[0x80] = {0};    /* +0x080 BuildLabEx */
    char csd_version[0x80] = {0};     /* +0x100 CSDVersion */
    char current_build[0x80] = {0};   /* +0x180 CurrentBuildNumber */
    char current_version[0x80] = {0}; /* +0x200 CurrentVersion */
    char product_name[0x80] = {0};    /* +0x280 ProductName */
    char product_id[0x80] = {0};      /* +0x300 ProductId */
    char system_root[0x80] = {0};     /* +0x380 SystemRoot (lowercased,
                                       * drive-letter form only) */

    /* +0x400: the ProductType classification byte (0 = unknown/not read). */
    u8 product_type = 0;
    static constexpr u8 kProductWinNt = 1;    /* workstation */
    static constexpr u8 kProductLanmanNt = 2; /* domain controller */
    static constexpr u8 kProductServerNt = 3; /* server */

    /* +0x402: the og's fixed 0x0100 record stamp (constant; exposed for
     * record-shape fidelity). */
    static constexpr u16 kRecordStamp = 0x0100;

    /* +0x408/+0x410: the system partition's location, valid only when
     * has_mounted_device (the og requires the SystemRoot drive letter
     * before it consults \MountedDevices). */
    bool has_mounted_device = false;
    u32 disk_signature = 0;   /* MBR disk signature */
    u64 partition_offset = 0; /* partition start byte offset */
};

/* The resolved ControlSet ("\ControlSet001" style, leading backslash).
 * Empty when the SYSTEM hive is unusable. Exposed for tests + the FVE
 * path's future Select-awareness. */
std::string resolve_control_set(const RegistryHive& system);

/* Profile the offline OS. `software` must be an open SOFTWARE hive;
 * `system` may be null (SOFTWARE-only pass: the identity fields fill, the
 * MountedDevices/ProductType halves stay empty). Returns false only when
 * `software` is null or its CurrentVersion key family cannot be walked at
 * all (a hive without the key still yields a valid empty-ish profile -
 * the og's skip-if-absent shape). */
bool profile_os(const RegistryHive* software, const RegistryHive* system, OsProfile& out);

} // namespace offlineos
} // namespace reghive
} // namespace cr

#endif // CR_OFFLINEOS_H_
