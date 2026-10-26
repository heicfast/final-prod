/* fuzz_image.cpp - libFuzzer harness for the S-030 disk-image parsers:
 * VMDK (monolithicSparse header + embedded descriptor, standalone text
 * descriptors with FLAT/SPARSE/ZERO extents, grain mapping, parent chains)
 * and VHD (fixed/dynamic/differencing footers, dynamic header, BAT +
 * sector bitmaps), plus VHDX (header pair, region table, metadata, BAT,
 * log-overlay walk) and the flat fallback + LBA0 classifier. All input
 * flows through a memory ImageFileIo; sibling extent files resolve to
 * fuzz-owned copies of the same bytes (the multi-file path exercises the
 * IO seam), parent chains resolve to the same image (depth-capped).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_image.h"
#include <cstring>
#include <memory>
#include <set>
#include <vector>

namespace {

using namespace cr;
using cr::storage::ImageFileIo;

/* every "file" the parsers ask for is a copy of the fuzz input: extent
 * data files and parent-chain images resolve deterministically without
 * inventing content, and the same-bytes recursion exercises the depth
 * cap on circular parent references. */
class FuzzIo final : public ImageFileIo {
public:
    explicit FuzzIo(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {}
    u32 open(const std::u16string&, u32*) override {
        next_ += 1;
        open_.insert(next_);
        return next_;
    }
    u32 read(u64 file, u64 at, void* buf, u32 len) override {
        if (!open_.count(file))
            return ERR_BAD_PARAMETER;
        if (at > bytes_.size() || u64(len) > bytes_.size() - at)
            return ERR_TRUNCATED;
        if (len)
            memcpy(buf, bytes_.data() + size_t(at), len);
        return OK;
    }
    u64 size(u64 file) const override { return open_.count(file) ? bytes_.size() : 0; }
    void close(u64 file) override { open_.erase(file); }

private:
    std::vector<uint8_t> bytes_;
    std::set<u64> open_;
    u64 next_ = 0;
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 16)
        return 0;
    FuzzIo io(std::vector<uint8_t>(data, data + size));
    u32 err = 0;
    std::unique_ptr<IStorageDevice> dev = storage::open_image(io, u"fuzz.img", &err);
    /* dispatch through every extension family on the same bytes */
    if (!dev) {
        dev = storage::open_image(io, u"fuzz.vmdk", &err);
        if (!dev)
            dev = storage::open_image(io, u"fuzz.vhd", &err);
        if (!dev)
            dev = storage::open_image(io, u"fuzz.vhdx", &err);
    }
    if (dev) {
        /* the device must be readable at a spread of shapes without
         * crashing: whole, head/tail, unaligned, block-boundary probes */
        uint8_t buf[8192];
        const uint64_t total = dev->size();
        (void)dev->read_at(0, buf, sizeof(buf));
        if (total > sizeof(buf))
            (void)dev->read_at(total - sizeof(buf), buf, sizeof(buf));
        (void)dev->read_at(total / 3, buf, 517);
        (void)dev->read_at(total / 2, buf, 4096);
        if (total > 8) /* bounds refusal must hold under fuzzed geometry */
            (void)dev->read_at(total - 4, buf, 8);
    }
    return 0;
}
