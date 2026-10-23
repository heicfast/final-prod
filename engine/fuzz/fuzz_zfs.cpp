/* fuzz_zfs.cpp - per-parser libFuzzer harness: scan_zfs (labels, uberblocks, ZAP, dnodes, compressed extents).
 *
 * Audit row 47 / mandate C: one binary per parser family for focused
 * campaigns with corpus isolation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "fuzz_parser_common.h"
#include "cr/zfs/cr_zfs_engine.h"
#include <memory>

using namespace cr;
using namespace cr::zfs;
using namespace crfuzz;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 512)
        return 0;
    MemDevice dev(data, size);
    ScanSetup s;
    ZfsScanStats st;
    (void)scan_zfs(&dev, nullptr, nullptr, nullptr, s.tree, s.ra, s.rd, s.rl, st);
    return 0;
}
