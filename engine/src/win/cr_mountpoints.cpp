/* cr_mountpoints.cpp - the Explorer MountPoints2\CPC\Volume history walk,
 * live-HKCU form (M-118, diff-to-fix).
 *
 * The og's exact registry sequence (PartitionRecoveryD_decompiled.c
 * :8930-8965, re-verified first-hand this session):
 *   RegOpenKeyA(HKCU,
 *       "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\
 *        MountPoints2\\CPC\\Volume", &hkey)
 *   for (i = 0; i < 1000; i++) {
 *       if (RegEnumKeyA(hkey, i, name, 0x104)) break;   // 260-char cap
 *       start = FUN_18000f5b0(ctx, bps, hkey, name);    // the decode
 *       if (start != 0) insert-as-candidate(start);
 *   }
 *   RegCloseKey(hkey);
 *
 * FUN_18000f5b0's per-subkey shape (:8841-8929): RegOpenKeyA(parent,
 * name) -> RegQueryValueExA(sub, "Data") -> the gate set -> the hex
 * parse -> the bps alignment + conversion. The gates and parse live in
 * lost::decode_mountpoints2_placement (one decode, both sources).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_mountpoints.h"
#include "cr/cr_error.h"
#include "cr/lost/cr_lostpart.h"

#include <cstring>

namespace cr {
namespace win {

namespace {

/* The og's key path and loop constants (FUN_18000f840). */
constexpr const char* kVolumeKeyPath =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2\\CPC\\Volume";
constexpr DWORD kMaxSubkeys = 1000;
constexpr DWORD kNameCap = 0x104; /* the og's RegEnumKeyA name buffer */

} // namespace

u32 mountpoints2_volume_history(u32 bytes_per_sector, std::vector<u64>& out_starts) {
    out_starts.clear();
    if (bytes_per_sector == 0)
        return ERR_BAD_PARAMETER;

    HKEY volume_key = nullptr;
    /* KEY_READ: the walk is read-only (the og's engine side opens
     * RegOpenKeyA's default access; the DRWUI mirror's 0xf003f mask is
     * its own over-privilege, not adopted) */
    if (RegOpenKeyExA(HKEY_CURRENT_USER, kVolumeKeyPath, 0, KEY_READ, &volume_key) != ERROR_SUCCESS)
        return OK; /* absent key: the profile never recorded a mount -
                    * the og's failed-open path walks nothing */

    for (DWORD i = 0; i < kMaxSubkeys; ++i) {
        char name[kNameCap];
        std::memset(name, 0, sizeof(name)); /* the layer's memset convention (cr_health.cpp) */
        if (RegEnumKeyA(volume_key, i, name, kNameCap) != ERROR_SUCCESS)
            break; /* enumeration exhausted (or error): the og's break */

        HKEY sub = nullptr;
        if (RegOpenKeyA(volume_key, name, &sub) != ERROR_SUCCESS)
            continue; /* the og's per-subkey RegOpenKeyA miss: skip */

        /* RegQueryValueExA(sub, "Data"): the shell-cache blob. The og
         * allocates 0x8000 and lets the call size it; the core queries
         * the size first, then reads - the same gate outcome without the
         * fixed 32 KiB assumption (a larger real blob still passes the
         * > 0xe7 gate, exactly like the og's truncated read would not). */
        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExA(sub, "Data", nullptr, &type, nullptr, &size) == ERROR_SUCCESS &&
            type == REG_BINARY && size > 0) {
            std::vector<u8> data(size);
            if (RegQueryValueExA(sub, "Data", nullptr, nullptr, data.data(), &size) ==
                ERROR_SUCCESS) {
                u64 start_lba = 0;
                if (lost::decode_mountpoints2_placement(data.data(), size, bytes_per_sector,
                                                        start_lba))
                    out_starts.push_back(start_lba);
            }
        }
        RegCloseKey(sub);
    }
    RegCloseKey(volume_key);
    return OK;
}

} // namespace win
} // namespace cr
