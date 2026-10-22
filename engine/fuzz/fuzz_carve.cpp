/* fuzz_carve.cpp - libFuzzer harness for the carving pipeline.
 *
 * The whole input becomes a "disk" carved end-to-end: magic scanning,
 * every grammar validator, dedup tree interaction and hashing run on
 * arbitrary bytes - the pipeline must terminate on any input.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_carve.h"
#include "cr/tree/cr_node.h"
#include <cstring>
#include <vector>

namespace {

class MemDevice final : public cr::IStorageDevice {
public:
    MemDevice(const uint8_t* d, size_t n) : d_(d), n_(n) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = n;
        g_.sector_count = n / 512;
    }
    cr::u32 read_at(cr::u64 off, void* buf, cr::u32 len) override {
        if (off > n_ || len > n_ - off)
            return cr::ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_ + off, len);
        return cr::OK;
    }
    const cr::Geometry& geometry() const override { return g_; }

private:
    const uint8_t* d_;
    size_t n_;
    cr::Geometry g_{};
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 16)
        return 0;
    MemDevice dev(data, size);
    cr::tree::ResultTree tree;
    auto* root = tree.add_root(u"carved");
    cr::carve::CarveFlags cf;
    cf.threads = 1;
    cr::carve::CarveStats st;
    (void)cr::carve::carve_slice(&dev, cf, nullptr, nullptr, nullptr, tree, root, nullptr,
                                 cr::carve::default_namer, st);
    tree.reset();
    return 0;
}
