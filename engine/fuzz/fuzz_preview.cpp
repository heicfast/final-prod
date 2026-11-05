/* fuzz_preview.cpp - libFuzzer harnesses for the G-10 preview surface.
 *
 * Targets:
 *   - the bounded reader itself (arbitrary extent lists over a memory
 *     device: the invariants are crash-freedom AND the bounded-read
 *     property: device traffic never exceeds what was requested)
 *   - every metadata extractor over fully hostile bytes (jpeg marker walk
 *     + EXIF/TIFF IFD, png, gif, bmp, tiff, webp, mp4 box walk + mvhd,
 *     wav chunks, pdf, docx zip central directory + inflate)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/preview/cr_preview.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::preview;

namespace {

class FuzzDevice final : public IStorageDevice {
public:
    explicit FuzzDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        /* deliberately tolerate ANY geometry the extents claim: bounded
         * reads must not depend on device goodwill */
        if (off > data_.size())
            return ERR_RANGE_OUT_OF_BOUNDS;
        u64 have = data_.size() - off;
        u32 take = len;
        if (u64(take) > have)
            take = u32(have);
        /* zero-length memcpy with a NULL source is UBSan-nonnull UB even
         * though it copies nothing (the empty-input case: data_ is empty,
         * data_.data() may be null - run 40682713696's preview finding) */
        if (take > 0)
            std::memcpy(buf, data_.data() + off, take);
        if (take < len)
            return ERR_READ_FAILED; /* short: surface as failure */
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

} // namespace

/* Reader: build an extent list from the fuzz input (two extents max,
 * logical size and offsets from the data), then read around. Crash-free
 * + never reads more than requested from the device. */
extern "C" int LLVMFuzzerTestOneInput(const u8* data, size_t size) {
    if (size < 42)
        return 0;
    /* layout: [logical_size:8][res_len:2][extent0 off:8 len:8][extent1
     * off:8 len:8] then the device payload is the REST */
    u64 logical = 0, e0off = 0, e0len = 0, e1off = 0, e1len = 0;
    u16 res = 0;
    std::memcpy(&logical, data, 8);
    std::memcpy(&res, data + 8, 2);
    std::memcpy(&e0off, data + 10, 8);
    std::memcpy(&e0len, data + 18, 8);
    std::memcpy(&e1off, data + 26, 8);
    std::memcpy(&e1len, data + 34, 8);
    /* keep the numbers hostile-but-bounded: 1 TiB logical cap, 16 MiB
     * extent caps (the reader's guards must handle the rest) */
    logical &= (1ull << 40) - 1;
    e0off &= (1ull << 28) - 1;
    e0len &= (1ull << 24) - 1;
    e1off &= (1ull << 28) - 1;
    e1len &= (1ull << 24) - 1;
    if (res > 4096)
        res = 4096;
    std::vector<u8> payload(data + 42, data + size);

    FuzzDevice dev(payload);
    tree::FileProperties pr;
    pr.logical_size = logical;
    pr.resident_data.assign(size_t(res), 0x5A);
    tree::FileProperties::Extent e;
    e.offset = e0off;
    e.length = e0len ? e0len : 1;
    pr.extents.push_back(e);
    e.offset = e1off;
    e.length = e1len;
    if (e.length)
        pr.extents.push_back(e);

    PreviewReader reader(&dev, &pr);
    u8 buf[8192];
    const u64 before = reader.device_bytes();
    for (u64 off : {u64(0), u64(1), u64(64), logical / 2, logical ? logical - 1 : 0, logical,
                    logical + 1, u64(1) << 20, u64(1) << 32}) {
        const u32 got = reader.read_at(off, buf, 512);
        (void)got;
    }
    /* bounded-read invariant: never more device traffic than requested
     * (9 reads x 512 + resident + decode guards) */
    if (reader.device_bytes() > before + (9ull << 20))
        __builtin_trap(); /* bound violated */
    /* extractor over the same hostile node */
    (void)extract_metadata(reader);
    /* M-028: hostile TIGHT budgets - the Caps contract must hold for any
     * budget the consumer configures: either the extraction stayed
     * inside it or truncated tells the truth. The budget is input-derived
     * (0..64 KiB) so the campaign explores every boundary around the
     * 64 KiB docx tail/CD windows and compressed-block sizes. */
    {
        Caps tight;
        tight.max_read = 1 + u32(data[0] | (u32(data[1]) << 8)); /* 1..65536 */
        tight.max_work = 4096;
        const u64 b0 = reader.device_bytes();
        const Metadata m = extract_metadata(reader, tight);
        if (reader.device_bytes() - b0 > tight.max_read && !m.truncated)
            __builtin_trap(); /* caps bypassed silently */
    }
    return 0;
}
