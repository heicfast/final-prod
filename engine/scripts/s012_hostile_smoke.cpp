/* s012_hostile_smoke.cpp - ASan/UBSan hostile-input smoke for the S-012
 * GUID-dump pass + size-0 rescue (local stand-in for the CI libFuzzer lane,
 * which needs clang; g++-only containers run this instead). Written during
 * the DF4-S012 parallel verification; adapted to the landed stat surface.
 *
 * Builds the S-012 GUID-dump fixture, applies N random mutations (byte
 * pokes in the dump region, boot-geometry corruption, truncation), and
 * drives NtfsVolume::open + walk_mft (default flags: pass 2 active) under
 * -fsanitize=address,undefined. Any sanitizer report or non-OK walk rc
 * on the UNMUTATED fixture is a failure; mutated runs must not crash.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/storage/cr_device.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <vector>

using namespace cr;
using namespace cr::ntfs;

namespace {

constexpr u32 SEC = 512;
constexpr u32 CLUSTER = SEC;
constexpr u64 NSECTORS = 256;
constexpr u64 RECORD_SIZE = 1024;
constexpr u32 GD_MFT_LCN = 4;
constexpr u64 GD_DUMP_LCN = 40;
constexpr u64 GD_ANCHOR_LCN = 100;

struct RecBuilder {
    std::vector<u8> buf;
    u32 attr_pos = 0;

    RecBuilder() { buf.assign(RECORD_SIZE, 0); }

    void header(u64 record_no, u16 flags) {
        auto* rh = reinterpret_cast<RecordHeader*>(buf.data());
        rh->magic = RECORD_MAGIC_FILE;
        rh->update_sequence_offset = 0x30;
        rh->update_sequence_size = 3;
        rh->sequence = 1;
        rh->link_count = 1;
        rh->attribute_offset = 0x38;
        rh->flags = flags;
        rh->bytes_in_use = u32(RECORD_SIZE);
        rh->bytes_allocated = u32(RECORD_SIZE);
        rh->base_record = 0;
        rh->record_number = u32(record_no);
        attr_pos = 0x38;
        u16 slots[3] = {0x0BB8, 0xAAAA, 0xBBBB};
        for (int i = 0; i < 3; ++i) {
            buf[0x30 + size_t(i) * 2] = u8(slots[i]);
            buf[0x30 + size_t(i) * 2 + 1] = u8(slots[i] >> 8);
        }
        for (int i = 0; i < 2; ++i) {
            u8* t = buf.data() + u64(i) * SEC + SEC - 2;
            t[0] = 0xB8;
            t[1] = 0x0B;
        }
    }

    static void w32(std::vector<u8>& v, size_t at, u32 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
        v[at + 2] = u8(x >> 16);
        v[at + 3] = u8(x >> 24);
    }
    static void w16(std::vector<u8>& v, size_t at, u16 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
    }
    static void w64(std::vector<u8>& v, size_t at, u64 x) {
        for (int i = 0; i < 8; ++i)
            v[at + size_t(i)] = u8(x >> (8 * i));
    }

    void attr_si(u32 dos_flags) {
        u32 pos = attr_pos;
        u32 vlen = 72, voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        w32(buf, pos + 0, ATTR_TYPE_STANDARD_INFORMATION);
        w32(buf, pos + 4, alen);
        w16(buf, pos + 10, u16(voff));
        w16(buf, pos + 16, u16(vlen & 0xFFFF));
        w16(buf, pos + 20, u16(voff));
        w32(buf, pos + voff + 32, dos_flags);
        attr_pos += alen;
    }

    void attr_file_name(u64 parent_record, const char16_t* name, u8 ns, u32 dos_flags,
                        u64 real_size, u64 alloc_size) {
        std::u16string n = name;
        u32 vlen = 66 + u32(n.size()) * 2;
        u32 voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        w32(buf, pos + 0, ATTR_TYPE_FILE_NAME);
        w32(buf, pos + 4, alen);
        w16(buf, pos + 10, u16(voff));
        w16(buf, pos + 16, u16(vlen & 0xFFFF));
        w16(buf, pos + 20, u16(voff));
        u8* d = buf.data() + pos + voff;
        w64(buf, pos + voff + 0, parent_record | (1ULL << 48));
        w64(buf, pos + voff + 40, alloc_size);
        w64(buf, pos + voff + 48, real_size);
        w32(buf, pos + voff + 56, dos_flags);
        d[64] = u8(n.size());
        d[65] = ns;
        for (size_t i = 0; i < n.size(); ++i)
            w16(buf, pos + voff + 66 + u32(i) * 2, n[i]);
        attr_pos += alen;
    }

    void attr_data_runlist(u64 lcn, u64 clusters, u64 real_size) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        w32(buf, pos + 0, ATTR_TYPE_DATA);
        w32(buf, pos + 4, alen);
        buf[pos + 8] = 1;
        w16(buf, pos + 10, 0x40);
        w16(buf, pos + 16, 0);
        w64(buf, pos + 24, clusters - 1);
        w16(buf, pos + 32, 0x40);
        w64(buf, pos + 40, clusters * CLUSTER);
        w64(buf, pos + 48, real_size);
        w64(buf, pos + 56, real_size);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    void attr_index_runlist(u64 lcn, u64 clusters) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        w32(buf, pos + 0, ATTR_TYPE_INDEX_ALLOCATION);
        w32(buf, pos + 4, alen);
        buf[pos + 8] = 1;
        w16(buf, pos + 10, 0x40);
        w16(buf, pos + 16, 0);
        w64(buf, pos + 24, clusters - 1);
        w16(buf, pos + 32, 0x40);
        w64(buf, pos + 40, clusters * CLUSTER);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    void finish() {
        u32 end = attr_pos;
        w32(buf, end, 0xFFFFFFFF);
        attr_pos = (end + 8 + 7) / 8 * 8;
    }
};

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8>& img) : img_(img) {
        geom_.logical_sector = SEC;
        geom_.physical_sector = SEC;
        geom_.sector_count = img_.size() / SEC;
        geom_.total_bytes = img_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > img_.size() || len > img_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, img_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8>& img_;
    Geometry geom_;
};

void build_fixture(std::vector<u8>& img) {
    img.assign(NSECTORS * SEC, 0);
    {
        u8* bs = img.data();
        BootSector* boot = reinterpret_cast<BootSector*>(bs);
        const char oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
        std::memcpy(boot->oem, oem, 8);
        boot->bytes_per_sector = SEC;
        boot->sectors_per_cluster = 1;
        boot->media_descriptor = 0xF8;
        boot->total_sectors = NSECTORS;
        boot->mft_lcn = GD_MFT_LCN;
        boot->mftmirr_lcn = 20;
        boot->clusters_per_record = 2;
        boot->clusters_per_index_block = 2;
        bs[510] = 0x55;
        bs[511] = 0xAA;
    }
    auto put = [&](u64 sec, const RecBuilder& r) {
        std::memcpy(img.data() + sec * SEC, r.buf.data(), RECORD_SIZE);
    };
    {
        RecBuilder r;
        r.header(0, RECORD_IN_USE);
        r.attr_data_runlist(GD_MFT_LCN, 16, 16 * CLUSTER);
        r.attr_file_name(5, u"$MFT", FILE_NAME_WIN32, 0x06, 0, 0);
        r.finish();
        put(GD_MFT_LCN, r);
    }
    {
        RecBuilder r;
        r.header(5, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(5, u".", FILE_NAME_POSIX, 0x00, 0, 0);
        r.finish();
        put(GD_MFT_LCN + 10, r);
    }
    {
        RecBuilder r;
        r.header(6, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"anchor.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 1024, 1024);
        r.attr_data_runlist(GD_ANCHOR_LCN, 2, 1024);
        r.finish();
        put(GD_MFT_LCN + 12, r);
    }
    {
        RecBuilder r;
        r.header(7, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5,
                         u"{447c1b0e-0a76-4e5f-8f0e-2d5a9d63a1b2}"
                         u"{3808876b-c176-4e48-b7ae-04046e6cc752}",
                         FILE_NAME_WIN32_AND_DOS, 0x20, 8192, 8192);
        r.attr_data_runlist(GD_DUMP_LCN, 16, 8192);
        r.finish();
        put(GD_MFT_LCN + 14, r);
    }
    {
        RecBuilder r;
        r.header(90, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"ghost.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(150, 4, 2048);
        r.finish();
        put(GD_DUMP_LCN, r);
    }
    {
        RecBuilder r;
        r.header(91, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"claimed.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(GD_ANCHOR_LCN, 2, 2048);
        r.finish();
        put(GD_DUMP_LCN + 2, r);
    }
    {
        RecBuilder r;
        r.header(92, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"interior.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(99, 4, 2048);
        r.finish();
        put(GD_DUMP_LCN + 4, r);
    }
    {
        RecBuilder r;
        r.header(93, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"tiny.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 512, 512);
        r.attr_data_runlist(160, 1, 512);
        r.finish();
        put(GD_DUMP_LCN + 6, r);
    }
    {
        RecBuilder r;
        r.header(94, RECORD_IS_DIRECTORY);
        r.attr_si(0x30);
        r.attr_file_name(5, u"dumpdir", FILE_NAME_WIN32_AND_DOS, 0x30, 0, 0);
        r.attr_index_runlist(GD_ANCHOR_LCN, 2);
        r.finish();
        put(GD_DUMP_LCN + 8, r);
    }
}

} // namespace

int main() {
    std::mt19937_64 rng(0xC0FFEE);
    int failures = 0;

    /* baseline: unmutated fixture must walk OK and exercise pass 2 */
    {
        std::vector<u8> img;
        build_fixture(img);
        MemDevice dev(img);
        u32 err = 0;
        auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(img)), &err);
        if (!vol) {
            std::printf("BASELINE: open failed err=%u\n", err);
            return 1;
        }
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"F");
        auto* rd = tree.add_root(u"D");
        auto* rl = tree.add_root(u"L");
        WalkerFlags wf;
        WalkStats st;
        u32 rc = walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        if (rc != OK || st.guid_dump_records == 0) {
            std::printf("BASELINE: rc=%u dump_records=%llu\n", rc,
                        (unsigned long long)st.guid_dump_records);
            ++failures;
        }
    }

    /* hostile: 400 mutated iterations (dump-region pokes, boot geometry,
     * truncation, random stabs) - no crash, no sanitizer report */
    for (int iter = 0; iter < 400; ++iter) {
        std::vector<u8> img;
        build_fixture(img);
        const int mode = int(rng() % 4);
        if (mode == 0) { /* poke the dump region */
            for (int p = 0; p < 4; ++p) {
                size_t at = size_t(GD_DUMP_LCN * SEC) + size_t(rng() % (16 * SEC));
                img[at] = u8(rng());
            }
        } else if (mode == 1) { /* corrupt boot geometry */
            const size_t fields[] = {0x0B, 0x0D, 0x40, 0x30, 0x28};
            img[fields[rng() % 5]] = u8(rng());
        } else if (mode == 2) { /* truncate the image */
            img.resize(size_t(SEC) * size_t(32 + rng() % (NSECTORS - 32)));
        } else { /* random stabs anywhere */
            for (int p = 0; p < 8; ++p)
                img[size_t(rng() % img.size())] = u8(rng());
        }
        MemDevice dev(img);
        u32 err = 0;
        auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(img)), &err);
        if (!vol)
            continue;
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"F");
        auto* rd = tree.add_root(u"D");
        auto* rl = tree.add_root(u"L");
        WalkerFlags wf;
        WalkStats st;
        (void)walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    }
    std::printf(failures == 0 ? "S012 SMOKE: PASS (400 hostile iterations + baseline)\n"
                              : "S012 SMOKE: FAIL (%d)\n",
                failures);
    return failures == 0 ? 0 : 1;
}
