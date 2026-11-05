/* cr_mountpoints2.cpp - the Explorer MountPoints2\CPC\Volume history walk,
 * offline-hive form (M-118, diff-to-fix).
 *
 * Corpus ground truth (verified first-hand this session):
 *   - engine-side walk: PartitionRecoveryD_decompiled.c:8930-8965
 *     (FUN_18000f840): RegOpenKeyA(HKCU,
 *     "Software\\...\\MountPoints2\\CPC\\Volume"), RegEnumKeyA loop with
 *     the 1000-subkey cap, each subkey decoded by FUN_18000f5b0
 *     (:8841-8929): RegQueryValueExA(subkey, "Data"), REG_BINARY (type
 *     3), size > 0xe7, data[0xc4] == '#' && data[0xe6] == '#', the
 *     0x20-char hex field at +0xc6 parsed "%I64X", byte_start % bps == 0,
 *     start_lba = byte_start / bps;
 *   - DRWUI-side mirror: FUN_1405895d0 (:922335+): the same key and
 *     "Data" decode (100-subkey cap), then FUN_140589e80 reads 0x800
 *     sectors at each remembered start and runs the parser chain
 *     (FUN_14058a070) - the probe gate the core's register pass applies
 *     at discovery time (lost::discover's register_starts leg).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/reghive/cr_mountpoints2.h"
#include "cr/cr_error.h"
#include "cr/lost/cr_lostpart.h"

namespace cr {
namespace reghive {

u32 mountpoints2_placements(const RegistryHive& user_hive, u32 bytes_per_sector,
                            std::vector<u64>& out_starts) {
    out_starts.clear();
    if (bytes_per_sector == 0)
        return ERR_BAD_PARAMETER;
    /* the hive's own validity: a hive that never opened has no root cell;
     * find_key returns 0 and the walk yields zero placements - the og's
     * RegOpenKeyA failure path (no key, no candidates, not an error) */
    const u64 volume_key = user_hive.find_key(kMountPoints2VolumeKey);
    if (volume_key == 0)
        return OK; /* absent key: the profile never recorded a mount */
    std::vector<std::string> subkeys;
    if (!user_hive.enum_subkeys(volume_key, subkeys))
        return OK; /* key with no subkey list: zero placements */
    /* the engine-side cap (FUN_18000f840's 1000-iteration bound) */
    if (subkeys.size() > kMountPoints2MaxSubkeys)
        subkeys.resize(kMountPoints2MaxSubkeys);
    for (const auto& name : subkeys) {
        const u64 sub = user_hive.find_subkey(volume_key, name.c_str());
        if (sub == 0)
            continue; /* the og's RegOpenKeyA miss on its own enumerated
                       * name: skip, keep walking */
        u32 type = 0;
        std::vector<u8> data;
        if (!user_hive.read_value(sub, "Data", type, data))
            continue; /* no "Data" value: skip this subkey */
        if (type != 3)
            continue; /* the og's lpType == 3 (REG_BINARY) gate */
        u64 start_lba = 0;
        if (lost::decode_mountpoints2_placement(data.data(), data.size(), bytes_per_sector,
                                                start_lba))
            out_starts.push_back(start_lba);
        /* gate failures skip the subkey individually - FUN_18000f5b0's
         * zero-return path in the og's loop */
    }
    return OK;
}

} // namespace reghive
} // namespace cr
