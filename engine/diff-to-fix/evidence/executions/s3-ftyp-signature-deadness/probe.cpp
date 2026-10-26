/* S3-CARVE executed probe (evidence for S-012): does the core's ftyp-family
 * signature match a real MP4? Compiles the REAL cr_registry/cr_analysers/
 * cr_carve/cr_rawsigns from core_recovery @ 34b3441. */
#include "cr/carve/cr_carve.h"
#include "cr/carve/cr_signatures.h"
#include "cr/storage/cr_device.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace cr;

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

int main() {
    /* REAL-WORLD MP4 head: size=0x18 BE, "ftyp", "isom", minor 0x200,
     * compat isom/iso2, then free + moov boxes (grammar walkable). */
    std::vector<u8> img(4ull * 1024 * 1024, 0);
    const unsigned char real_mp4[] = {
        0x00,0x00,0x00,0x18, 'f','t','y','p', 'i','s','o','m',
        0x00,0x00,0x02,0x00, 'i','s','o','m', 'i','s','o','2',
        0x00,0x00,0x00,0x08, 'f','r','e','e',
        0x00,0x00,0x00,0x20, 'm','o','o','v',
    };
    std::memcpy(img.data() + 512, real_mp4, sizeof(real_mp4));

    /* degenerate size-0 ftyp (to-EOF box) at 2 MiB */
    const unsigned char zero_ftyp[] = {0,0,0,0, 'f','t','y','p', 'm','p','4','2'};
    std::memcpy(img.data() + (2ull << 20), zero_ftyp, sizeof(zero_ftyp));

    /* control: a real JPEG so we can see the machinery works at all */
    const unsigned char jpg[] = {0xFF,0xD8,0xFF,0xE0, 0,0x10,'J','F','I','F',0,
        0xFF,0xC0,0,0x11,8, 0,8, 0,6, 1,2,0,2,2,2,2,  1,0,2,0, 3, 4, 2, 4,
        0xFF,0xDA,0,8,1,1,0,0,0x3F,0, 0x12,0x34, 0xFF,0xD9};
    std::memcpy(img.data() + (3ull << 20), jpg, sizeof(jpg));

    MemDevice dev(img);
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    carve::CarveFlags flags;
    flags.threads = 1;
    carve::CarveStats stats;
    u32 rc = carve::carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree,
                                root, nullptr, nullptr, stats);
    printf("rc=%u hits=%llu validated=%llu rejected=%llu sign_hits=%llu\n", rc,
           (unsigned long long)stats.hits, (unsigned long long)stats.validated,
           (unsigned long long)stats.rejected, (unsigned long long)stats.sign_hits);
    size_t n = 0;
    for (auto* node = root->first_child(); node; node = node->next_sibling()) {
        const tree::FileProperties* p = node->props();
        std::string name;
        for (auto ch : p->name) name += char(ch);
        printf("node[%zu] name=%s fmt=0x%x conf=%u size=%llu ext0=%llu\n", n++,
               name.c_str(), p->carve_format, p->carve_confidence,
               (unsigned long long)p->logical_size,
               (unsigned long long)(p->extents.empty() ? 0 : p->extents[0].offset));
    }
    printf("total_nodes=%zu\n", n);
    return 0;
}
