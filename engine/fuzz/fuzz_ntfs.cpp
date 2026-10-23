/* fuzz_ntfs.cpp - per-parser libFuzzer harness: NtfsVolume::open + record reads (fragmented MFT, attrlist, mirror fallback).
 *
 * Audit row 47 / mandate C: one binary per parser family for focused
 * campaigns with corpus isolation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "fuzz_parser_common.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include <memory>

using namespace cr;
using namespace cr::ntfs;
using namespace crfuzz;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 512)
        return 0;
    MemDevice dev(data, size);
    ScanSetup s;
    /* full NTFS treatment: open (boot validation, $MFT stream incl. the
     * nonresident-attrlist fragmented path, mirror), bitmap, walk (nested
     * attrlist chase, ADS, hard links, UsnJrnl harvest) */
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(SliceShim::shim(&dev)), &err);
    if (vol) {
        std::unique_ptr<VolumeBitmap> bm;
        (void)VolumeBitmap::load(vol.get(), bm);
        WalkerFlags wf;
        WalkStats ws;
        (void)walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, s.tree, s.ra, s.rd,
                       s.rl, ws);
        for (u64 r = 0; r < 8 && r < vol->max_record_no(); ++r) {
            std::vector<u8> rec;
            (void)vol->read_record(r, rec);
        }
    }
    return 0;
}
