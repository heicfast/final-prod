/* fuzz_thumbnail.cpp - libFuzzer harness for the cr_thumbnail engine.
 *
 * Targets every parser the thumbnail surface owns over fully hostile
 * bytes: the stb callback adapter (jpeg/png/bmp/gif/psd/pnm/hdr decode
 * paths), the native TIFF baseline decoder (IFD walk, strip fetch,
 * PackBits / Deflate uncompression, palette mapping, 16-bit narrowing),
 * the EXIF APP1 walker (marker walk + IFD0->IFD1 + embedded JPEG), the
 * native dims-first header parsers, and the box downscaler. Invariants:
 * crash-freedom under ASan/UBSan, and on Ok the output geometry contract
 * (both axes within max_edge, rgba exactly width*height*4).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/preview/cr_thumbnail.h"
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
        if (off > data_.size())
            return ERR_RANGE_OUT_OF_BOUNDS;
        u64 have = data_.size() - off;
        u32 take = len;
        if (u64(take) > have)
            take = u32(have);
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

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > 4u << 20)
        return 0; /* the caps surface never accepts more anyway */

    /* run 1: resident content, the corpus-anchored default caps */
    {
        FuzzDevice dev(std::vector<u8>(512, 0));
        tree::FileProperties props;
        props.name = u"f";
        props.logical_size = size;
        props.resident_data.assign(data, data + size);
        PreviewReader r(&dev, &props);
        Thumbnail t;
        const ThumbStatus st = make_thumbnail(r, ThumbnailCaps{}, t);
        if (st == ThumbStatus::Ok) {
            if (t.width == 0 || t.height == 0)
                __builtin_trap();
            if (t.width > 256 || t.height > 256)
                __builtin_trap(); /* both axes bounded, never upscaled */
            if (t.rgba.size() != size_t(t.width) * t.height * 4)
                __builtin_trap();
        }
    }

    /* run 2: extent-backed + tight budget + small edge - exercises the
     * budget/work guards and the (failing) device interplay */
    {
        std::vector<u8> disk(4096 + size, 0);
        if (size)
            std::memcpy(disk.data() + 4096, data, size);
        FuzzDevice dev(std::move(disk));
        tree::FileProperties props;
        props.name = u"f";
        props.logical_size = size;
        tree::FileProperties::Extent e{4096, size, 0, 0};
        if (size)
            props.extents.push_back(e);
        PreviewReader r(&dev, &props);
        ThumbnailCaps c;
        c.max_edge = 7;
        c.max_read = 4096;
        c.max_work = 512;
        c.allow_embedded = (size % 2) == 0; /* both policy branches */
        Thumbnail t;
        const ThumbStatus st = make_thumbnail(r, c, t);
        if (st == ThumbStatus::Ok) {
            if (t.width == 0 || t.height == 0)
                __builtin_trap();
            if (t.width > 7 || t.height > 7)
                __builtin_trap();
            if (t.rgba.size() != size_t(t.width) * t.height * 4)
                __builtin_trap();
        }
    }
    return 0;
}
