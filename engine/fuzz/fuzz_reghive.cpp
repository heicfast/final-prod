/* fuzz_reghive.cpp - libFuzzer harness for the registry hive parser +
 * FVEAutoUnlock harvest (diff-to-fix S-014).
 *
 * The input is treated as a raw regf hive: parse, walk the og's
 * \ControlSet001\Control\FVEAutoUnlock family, harvest, and select against
 * a GUID derived from the input tail. Every path must return without
 * reading out of bounds, hanging or crashing - ASan/UBSan enforce that.
 * A valid seed corpus lives in fuzz/corpus/reghive (the test suite's
 * fixture builder shape).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/reghive/cr_reghive.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    using cr::u32;
    using cr::u64;
    if (size < 0x1000)
        return 0;
    cr::reghive::RegistryHive hive;
    if (!hive.open(data, size))
        return 0; /* rejected hives are the common case - fine */

    /* walk the og's hardcoded family (case variants exercise the fold) */
    const u64 fve = hive.find_key("\\ControlSet001\\Control\\FVEAutoUnlock");
    if (fve == 0)
        return 0;

    /* enumerate + one-level child lookup + value reads on every subkey */
    std::vector<std::string> guids;
    if (!hive.enum_subkeys(fve, guids))
        return 0;
    std::vector<cr::reghive::AutoUnlockKey> keys;
    for (const std::string& g : guids) {
        const u64 sub = hive.find_subkey(fve, g.c_str());
        if (sub == 0)
            continue;
        u32 type = 0;
        std::vector<uint8_t> bytes;
        (void)hive.read_value(sub, "Data", type, bytes);
        (void)hive.read_value(sub, "NoData", type, bytes);
    }

    /* full harvest with the corpus caps, then selection against a GUID
     * string built from the input tail (bounded to 50 chars) */
    keys.clear();
    (void)cr::reghive::harvest_fve_auto_unlock(hive, keys);
    char guid[64] = {};
    if (size > 0x1010) {
        const size_t n = std::min<size_t>(size - 0x1010, 50);
        std::memcpy(guid, data + 0x1010, n);
        guid[n] = '\0';
    }
    (void)cr::reghive::select_auto_unlock_attempts(keys, guid);
    (void)cr::reghive::select_auto_unlock_attempts(keys, nullptr);
    (void)cr::reghive::guid_equals_ci(keys.empty() ? std::string() : keys[0].volume_guid, guid);
    return 0;
}
