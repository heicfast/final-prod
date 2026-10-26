/* fuzz_exfat.cpp - per-parser libFuzzer harness: validate_boot_region + scan_exfat (entry sets,
 * checksums).
 *
 * Audit row 47 / mandate C: one binary per parser family for focused
 * campaigns with corpus isolation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "fuzz_parser_common.h"
#include "cr/exfat/cr_exfat_engine.h"
#include <memory>

using namespace cr;
using namespace cr::exfat;
using namespace crfuzz;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 512)
        return 0;
    MemDevice dev(data, size);
    ScanSetup s;
    ExfatScanFlags fl;
    ExfatScanStats st;
    (void)scan_exfat(&dev, fl, nullptr, nullptr, nullptr, s.tree, s.ra, s.rd, s.rl, st);
    return 0;
}
