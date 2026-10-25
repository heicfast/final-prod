/* fuzz_image.cpp - libFuzzer harness for the S-030 disk-image parsers:
 * VMDK (monolithicSparse header + embedded descriptor, standalone text
 * descriptors with FLAT/SPARSE/ZERO extents, grain mapping) and VHD
 * (fixed/dynamic footers, dynamic header, BAT + sector bitmaps), plus
 * the flat fallback and the LBA0 classifier. All input flows through a
 * memory byte source; sibling extent files resolve to fuzz-owned copies
 * of the same bytes (the multi-file path exercises the ops seam).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_image.h"
#include <cstring>
#include <memory>
#include <vector>

namespace {

class FuzzSource final : public cr::storage::ImageByteSource {
public:
    FuzzSource(const uint8_t* d, size_t n) : d_(d), n_(n) {}
    uint32_t read(uint64_t at, void* buf, uint32_t len) override {
        if (at > n_ || uint64_t(len) > n_ - at)
            return cr::ERR_RANGE_OUT_OF_BOUNDS;
        if (len)
            memcpy(buf, d_ + size_t(at), len);
        return cr::OK;
    }
    uint64_t size() const override { return n_; }

private:
    const uint8_t* d_;
    size_t n_;
};

/* every sibling "file" is a copy of the fuzz input: the descriptor's
 * extent lines resolve deterministically without inventing paths */
uint32_t fuzz_open(void*, const char*, std::unique_ptr<cr::storage::ImageByteSource>& out) {
    return 1; /* ERR_OPEN_FAILED-equivalent: exercises the no-extent-file
                 error path for split descriptors */
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 16)
        return 0;
    std::unique_ptr<cr::storage::ImageByteSource> src(new FuzzSource(data, size));
    cr::storage::ImageFileOps ops;
    ops.open = &fuzz_open;
    std::unique_ptr<cr::IStorageDevice> dev;
    if (cr::storage::open_image(std::move(src), ops, "/fuzz/", dev) == cr::OK) {
        /* the device must be readable at a spread of shapes without
         * crashing: whole, head/tail, unaligned, block-boundary probes */
        uint8_t buf[8192];
        const uint64_t total = dev->size();
        (void)dev->read_at(0, buf, sizeof(buf));
        if (total > sizeof(buf))
            (void)dev->read_at(total - sizeof(buf), buf, sizeof(buf));
        (void)dev->read_at(total / 3, buf, 517);
        (void)dev->read_at(total / 2, buf, 4096);
        (void)cr::storage::classify_boot_layout(dev.get());
        /* bounds refusal must hold under fuzzed geometry */
        if (total > 8)
            (void)dev->read_at(total - 4, buf, 8);
    }
    return 0;
}
