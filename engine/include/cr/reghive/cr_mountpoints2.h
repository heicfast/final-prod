/*
 * cr_mountpoints2.h - the Explorer MountPoints2\CPC\Volume history walk,
 * offline-hive form (M-118, diff-to-fix).
 *
 * Purpose: the og's third lost-partition candidate source - the Windows
 * Explorer volume-mount history. The registry remembers where volumes were
 * mounted even after the partition is deleted and its table entry is gone;
 * the og walks it engine-side (PartitionRecoveryD_decompiled.c:8930-8965,
 * RegOpenKeyA HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\
 * MountPoints2\CPC\Volume", up to 1000 subkeys, each subkey's "Data"
 * REG_BINARY decoded by FUN_18000f5b0) and DRWUI-side (FUN_1405895d0,
 * up to 100 subkeys, the same "Data" decode + a 0x800-sector probe read
 * at the remembered start through the parser chain FUN_14058a070).
 *
 * This module reproduces the walk over an OFFLINE user hive (NTUSER.DAT)
 * through the read-only regf parser - the pure, cross-platform form the
 * hosts and the test suite can drive without a live HKCU. The live-HKCU
 * form (the exact RegOpenKeyExA/RegEnumKeyA/RegQueryValueExA walk) lives
 * in the win layer (cr/win/cr_mountpoints.h) and feeds the SAME blob
 * decoder (lost::decode_mountpoints2_placement).
 *
 * The walk's exact semantics (verified first-hand against both corpus
 * decompiles this session):
 *   - key path: Software\Microsoft\Windows\CurrentVersion\Explorer\
 *     MountPoints2\CPC\Volume (case-insensitive, registry semantics);
 *   - subkeys: volume GUIDs, enumerated in order, the engine-side cap
 *     1000 (FUN_18000f840's loop bound; the DRWUI mirror caps at 100);
 *   - per subkey: the "Data" value, REG_BINARY (type 3);
 *   - decode: lost::decode_mountpoints2_placement (size > 0xe7, the
 *     data[0xc4]/data[0xe6] '#' delimiters, the 0x20-char hex byte
 *     offset at +0xc6, bps alignment, /bps conversion);
 *   - output: the placement's start LBA per the caller's bps.
 *
 * Safety: strictly read-only; absent key/subkeys/values are tolerated
 * (zero placements - a user profile that never mounted a volume is not an
 * error); subkeys whose "Data" fails the gates are skipped individually,
 * exactly like the og's per-subkey loop.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_REGHIVE_MOUNTPOINTS2_H_
#define CR_REGHIVE_MOUNTPOINTS2_H_

#include "cr_reghive.h"
#include <cstddef>
#include <vector>

namespace cr {
namespace reghive {

/* The og's key path under the user hive (HKCU offline = NTUSER.DAT). */
constexpr const char* kMountPoints2VolumeKey =
    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2\\CPC\\Volume";

/* The engine-side subkey cap (FUN_18000f840's 1000-iteration loop,
 * PartitionRecoveryD_decompiled.c:8939). */
constexpr u32 kMountPoints2MaxSubkeys = 1000;

/* Walk the volume-mount history in an OPEN user hive and emit the
 * remembered placements as sector LBAs (byte_start / bytes_per_sector,
 * the og's conversion), in subkey enumeration order. Returns OK on a
 * completed walk (including the absent-key case: zero placements), or
 * ERR_BAD_PARAMETER on a null/invalid hive argument. Placements that
 * fail the per-subkey gates are skipped, not fatal - the og's loop
 * shape. bytes_per_sector 0 is refused (the og divides by the device's
 * bps; the core refuses rather than trapping). */
u32 mountpoints2_placements(const RegistryHive& user_hive, u32 bytes_per_sector,
                            std::vector<u64>& out_starts);

} // namespace reghive
} // namespace cr

#endif // CR_REGHIVE_MOUNTPOINTS2_H_
