/* S3-CARVE probe 2: (a) grammar-valid JPEG control proves the harness works;
 * (b) real MP4 vs degenerate ftyp; (c) ISO/BMFF "size-0 box + brand qtw /
 * heic/mif1" variants to exhaust the ftyp-family match space. */
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
        geom_.logical_sector = 512; geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off) return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len); return OK;
    }
    const Geometry& geometry() const override { return geom_; }
private:
    std::vector<u8> data_; Geometry geom_;
};

void putbe16(std::vector<u8>& v, size_t at, u16 x){ v[at]=u8(x>>8); v[at+1]=u8(x); }
void putbe32(std::vector<u8>& v, size_t at, u32 x){ v[at]=u8(x>>24); v[at+1]=u8(x>>16); v[at+2]=u8(x>>8); v[at+3]=u8(x); }

size_t plant_jpeg(std::vector<u8>& img, size_t at) {
    img[at]=0xFF; img[at+1]=0xD8; img[at+2]=0xFF; img[at+3]=0xE0;
    putbe16(img, at+4, 16); std::memcpy(img.data()+at+6, "JFIF", 4);
    img[at+10]=1; img[at+11]=1; img[at+12]=0; img[at+13]=0; img[at+14]=1; img[at+15]=1; img[at+16]=0; img[at+17]=0;
    size_t p = at + 20;
    img[p]=0xFF; img[p+1]=0xFE; putbe16(img, p+2, 460);
    for (int i=0;i<456;++i) img[p+4+i]=u8('A'+(i%26));
    p += 2+460;
    img[p]=0xFF; img[p+1]=0xC0; putbe16(img, p+2, 17); img[p+4]=8;
    putbe16(img, p+5, 64); putbe16(img, p+7, 48); img[p+9]=1; img[p+10]=1; img[p+11]=0x22; img[p+12]=0;
    p += 2+17;
    img[p]=0xFF; img[p+1]=0xDA; putbe16(img, p+2, 12);
    for (int i=0;i<10;++i) img[p+4+i]=0;
    p += 2+12;
    img[p]=0x00; img[p+1]=0x11; img[p+2]=0x22; img[p+3]=0x33;
    img[p+4]=0xFF; img[p+5]=0xD9;
    return (p+6)-at;
}

int main() {
    std::vector<u8> img(6ull<<20, 0);
    /* control: valid JPEG at 512 -> grammar must carve it */
    plant_jpeg(img, 512);
    /* real MP4 at 1 MiB: size=0x18 ftyp isom + free + moov */
    unsigned char mp4[] = {0,0,0,0x18,'f','t','y','p','i','s','o','m', 0,0,2,0,'i','s','o','m','i','s','o','2',
                           0,0,0,8,'f','r','e','e', 0,0,0,0x20,'m','o','o','v'};
    std::memcpy(img.data()+(1ull<<20), mp4, sizeof(mp4));
    /* real MOV at 2 MiB: size=0x14 ftyp qt   */
    unsigned char mov[] = {0,0,0,0x14,'f','t','y','p','q','t',' ',' ', 0,0,0,8,'f','r','e','e'};
    std::memcpy(img.data()+(2ull<<20), mov, sizeof(mov));
    /* real HEIC at 3 MiB: size=0x18 ftyp heic */
    unsigned char heic[] = {0,0,0,0x18,'f','t','y','p','h','e','i','c', 0,0,0,0,'m','i','f','1','h','e','i','c'};
    std::memcpy(img.data()+(3ull<<20), heic, sizeof(heic));
    /* real M4A at 4 MiB: size=0x20 ftyp M4A  */
    unsigned char m4a[] = {0,0,0,0x20,'f','t','y','p','M','4','A',' ', 0,0,0,0,'M','4','A',' ','m','p','4','2', 0,0,0,8,'f','r','e','e'};
    std::memcpy(img.data()+(4ull<<20), m4a, sizeof(m4a));
    /* degenerate size-0 ftyp heic at 5 MiB */
    unsigned char z[] = {0,0,0,0,'f','t','y','p','h','e','i','c'};
    std::memcpy(img.data()+(5ull<<20), z, sizeof(z));

    MemDevice dev(img);
    tree::ResultTree tree; auto* root = tree.add_root(u"Lost Files");
    carve::CarveFlags flags; flags.threads = 1; flags.hash_small_files = false;
    carve::CarveStats stats;
    u32 rc = carve::carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    printf("rc=%u hits=%llu validated=%llu rejected=%llu\n", rc,
        (unsigned long long)stats.hits, (unsigned long long)stats.validated, (unsigned long long)stats.rejected);
    size_t n=0;
    for (auto* node=root->first_child(); node; node=node->next_sibling()) {
        const tree::FileProperties* p = node->props();
        std::string name; for (auto ch: p->name) name += char(ch);
        printf("node[%zu] name=%s fmt=0x%x conf=%u size=%llu ext=%llu\n", n++,
            name.c_str(), p->carve_format, p->carve_confidence,
            (unsigned long long)p->logical_size,
            (unsigned long long)(p->extents.empty()?0:p->extents[0].offset));
    }
    printf("total=%zu\n", n);
    return 0;
}
