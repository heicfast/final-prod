/* fuzz_ext4.cpp - per-parser libFuzzer harness: scan_ext4 (SB, GDT, inodes, journal salvage).
 *
 * Audit row 47 / mandate C: one binary per parser family for focused
 * campaigns with corpus isolation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "fuzz_parser_common.h"
#include "cr/ext4/cr_ext4_engine.h"
#include <memory>

using namespace cr;
using namespace cr::ext4;
using namespace crfuzz;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 512)
        return 0;
    MemDevice dev(data, size);
    ScanSetup s;
    Ext4ScanFlags fl;
    Ext4ScanStats st;
    (void)scan_ext4(&dev, fl, nullptr, nullptr, nullptr, s.tree, s.ra, s.rd, s.rl, st);
    return 0;
}
