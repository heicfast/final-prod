/*
 * cr_mountpoints.h - the Explorer MountPoints2\CPC\Volume history walk,
 * live-HKCU form (M-118, diff-to-fix). Win layer: live registry access is
 * an OS-integration concern, like the BitLocker FVEAutoUnlock live path.
 *
 * The og's engine-side walk (PartitionRecoveryD_decompiled.c:8930-8965,
 * FUN_18000f840): RegOpenKeyA(HKCU,
 * "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2\\
 * CPC\\Volume"), RegEnumKeyA over up to 1000 subkeys, per subkey
 * RegQueryValueExA(subkey, "Data") decoded by FUN_18000f5b0 (type 3,
 * size > 0xe7, the '#' delimiters at +0xc4/+0xe6, the 0x20-char hex
 * byte offset at +0xc6, bps alignment, /bps conversion). This module
 * reproduces that walk against the LIVE HKCU and feeds the same pure
 * decoder the offline-hive form uses (lost::decode_mountpoints2_placement)
 * - one decode, two sources, like the og's own DRWUI/engine pair.
 *
 * Read-only: KEY_READ (the og's engine side opens with RegOpenKeyA's
 * default; the DRWUI mirror's 0xf003f mask is its own over-privilege the
 * core does not adopt - the walk never writes).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_WIN_MOUNTPOINTS_H_
#define CR_WIN_MOUNTPOINTS_H_

#include "../cr_types.h"
#include <windows.h>
#include <vector>

namespace cr {
namespace win {

/* Walk the live HKCU volume-mount history and emit the remembered
 * placements as sector LBAs (byte_start / bytes_per_sector). Returns OK
 * (with zero placements) when the key is absent - a Windows profile that
 * never mounted a volume is not an error; the og's RegOpenKeyA failure
 * path walks nothing. Subkeys whose "Data" fails the gates are skipped
 * individually. bytes_per_sector 0 is refused. */
u32 mountpoints2_volume_history(u32 bytes_per_sector, std::vector<u64>& out_starts);

} // namespace win
} // namespace cr

#endif // CR_WIN_MOUNTPOINTS_H_
