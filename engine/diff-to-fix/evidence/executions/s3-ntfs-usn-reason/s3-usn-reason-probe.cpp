/* s3-usn-reason-probe.cpp — executed-evidence probe for the USN harvest
 * reason-constant audit (agent S3-NTFS, read-only session).
 *
 * Builds a minimal synthetic NTFS image (mirrors tests/unit/
 * test_ntfs_volume.cpp's fixture builders byte-for-byte where shared) with
 * an $Extend/$UsnJrnl:$J stream carrying THREE USN_RECORD_V2 entries:
 *
 *   realdelete.txt  reason = 0x80000200  (USN_REASON_FILE_DELETE|USN_REASON_CLOSE
 *                                         — what Windows actually writes when a
 *                                         file is deleted; winioctl.h values)
 *   fixturesync.txt reason = 0x00020100  (USN_REASON_FILE_CREATE | the core
 *                                         header's USN_REASON_FILE_DELETE
 *                                         =0x00020000 constant — the value
 *                                         the green unit test feeds)
 *   createonly.txt  reason = 0x00000100  (USN_REASON_FILE_CREATE only)
 *
 * Then runs walk_mft (HEAD 34b3441) and prints stats + the harvested lost
 * names. If the engine filtered on the REAL FILE_DELETE bit (0x200, the og
 * NTFSSearchDLL FUN_1800196d0:20098 `reason & 0x200` gate), realdelete.txt
 * would be harvested and fixturesync.txt would not. Watch what happens.
 *
 * Build (out-of-tree, no repo files modified):
 *   g++ -std=c++20 -O1 -I<repo>/include s3-usn-reason-probe.cpp \
 *       <repo>/src/core/ntfs/cr_bootstrap.cpp <repo>/src/core/ntfs/cr_attributes.cpp \
 *       <repo>/src/core/ntfs/cr_fixup.cpp <repo>/src/core/ntfs/cr_runlist.cpp \
 *       <repo>/src/core/ntfs/cr_bitmap.cpp <repo>/src/core/ntfs/cr_mftwalk.cpp \
 *       <repo>/src/core/tree/cr_node.cpp <repo>/src/core/cr_parallel.cpp \
 *       -lpthread -o s3-usn-reason-probe
 */

#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/storage/cr_device.h"
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::ntfs;

namespace {

constexpr u32 SEC = 512;
constexpr u32 CLUSTER = SEC; /* spc = 1 */
constexpr u64 NSECTORS = 256;
constexpr u64 RECORD_SIZE = 1024;
constexpr u64 MFT_LCN = 4;
constexpr u64 MFT_CLUSTERS = 68; /* records 0..33 (mirror test fixture) */
constexpr u64 J_LCN = 160;       /* $J stream data */

class Image {
public:
    Image() : data_(NSECTORS * SEC, 0) {
        geom_.logical_sector = SEC;
        geom_.physical_sector = SEC;
        geom_.sector_count = NSECTORS;
        geom_.total_bytes = data_.size();
    }
    u8* sec(u64 lba) { return data_.data() + lba * SEC; }
    std::vector<u8>& raw() { return data_; }
    Geometry& geom() { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8>& img, Geometry& g) : img_(img), geom_(g) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > img_.size() || len > img_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, img_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8>& img_;
    Geometry& geom_;
};

struct RecBuilder {
    std::vector<u8> buf;
    u32 attr_pos = 0;

    RecBuilder() { buf.assign(RECORD_SIZE, 0); }

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
    void u32wr(size_t at, u32 x) { w32(buf, at, x); }
    void u16wr(size_t at, u16 x) { w16(buf, at, x); }
    void u64wr(size_t at, u64 x) { w64(buf, at, x); }

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
            buf[0x30 + i * 2] = u8(slots[i]);
            buf[0x30 + i * 2 + 1] = u8(slots[i] >> 8);
        }
        for (int i = 0; i < 2; ++i) {
            u8* t = buf.data() + u64(i) * SEC + SEC - 2;
            t[0] = 0xB8;
            t[1] = 0x0B;
        }
    }

    void attr_si(u32 dos_flags) {
        u32 pos = attr_pos;
        u32 vlen = 72, voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32wr(pos + 0, ATTR_TYPE_STANDARD_INFORMATION);
        u32wr(pos + 4, alen);
        buf[pos + 8] = 0;
        buf[pos + 9] = 0;
        u16wr(pos + 10, u16(voff));
        u16wr(pos + 16, u16(vlen & 0xFFFF));
        u16wr(pos + 20, u16(voff));
        w64(buf, pos + voff + 0, 132000000000000000ULL);
        w64(buf, pos + voff + 8, 133000000000000000ULL);
        w64(buf, pos + voff + 16, 134000000000000000ULL);
        w64(buf, pos + voff + 24, 135000000000000000ULL);
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
        u32wr(pos + 0, ATTR_TYPE_FILE_NAME);
        u32wr(pos + 4, alen);
        buf[pos + 8] = 0;
        buf[pos + 9] = 0;
        u16wr(pos + 10, u16(voff));
        u16wr(pos + 16, u16(vlen & 0xFFFF));
        u16wr(pos + 20, u16(voff));
        u8* d = buf.data() + pos + voff;
        w64(buf, pos + voff + 0, parent_record | (1ULL << 48));
        w64(buf, pos + voff + 8, 132000000000000000ULL);
        w64(buf, pos + voff + 16, 133000000000000000ULL);
        w64(buf, pos + voff + 24, 134000000000000000ULL);
        w64(buf, pos + voff + 32, 135000000000000000ULL);
        w64(buf, pos + voff + 40, alloc_size);
        w64(buf, pos + voff + 48, real_size);
        w32(buf, pos + voff + 56, dos_flags);
        w32(buf, pos + voff + 60, 0);
        d[64] = u8(n.size());
        d[65] = ns;
        for (size_t i = 0; i < n.size(); ++i)
            w16(buf, pos + voff + 66 + u32(i) * 2, u16(n[i]));
        attr_pos += alen;
    }

    void attr_data_runlist(u64 lcn, u64 clusters, u64 real_size) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32wr(pos + 0, ATTR_TYPE_DATA);
        u32wr(pos + 4, alen);
        buf[pos + 8] = 1;
        u16wr(pos + 10, 0x40);
        w64(buf, pos + 16, 0);
        w64(buf, pos + 24, clusters - 1);
        u16wr(pos + 32, 0x40);
        w32(buf, pos + 36, 0);
        w64(buf, pos + 40, clusters * CLUSTER);
        w64(buf, pos + 48, real_size);
        w64(buf, pos + 56, real_size);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    void finish() {
        u32 pos = attr_pos;
        u32wr(pos + 0, ATTR_TYPE_TERMINATOR);
        u32wr(pos + 4, 8);
        attr_pos += 8;
        auto* rh = reinterpret_cast<RecordHeader*>(buf.data());
        rh->bytes_in_use = attr_pos;
    }
};

void build_image(Image& img) {
    {
        u8* bs = img.sec(0);
        BootSector* boot = reinterpret_cast<BootSector*>(bs);
        const char oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
        std::memcpy(boot->oem, oem, 8);
        boot->bytes_per_sector = SEC;
        boot->sectors_per_cluster = 1;
        boot->media_descriptor = 0xF8;
        boot->total_sectors = NSECTORS;
        boot->mft_lcn = MFT_LCN;
        boot->mftmirr_lcn = 68;
        boot->clusters_per_record = 2;
        boot->clusters_per_index_block = 2;
        boot->checksum = 0;
        bs[510] = 0x55;
        bs[511] = 0xAA;
    }
    /* record 0: $MFT */
    {
        RecBuilder r;
        r.header(0, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFT", FILE_NAME_WIN32, 0x06, MFT_CLUSTERS * CLUSTER,
                         MFT_CLUSTERS * CLUSTER);
        r.attr_data_runlist(MFT_LCN, MFT_CLUSTERS, MFT_CLUSTERS * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 0), r.buf.data(), RECORD_SIZE);
    }
    /* record 1: $MFTMirr */
    {
        RecBuilder r;
        r.header(1, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFTMirr", FILE_NAME_WIN32, 0x06, 8 * CLUSTER, 8 * CLUSTER);
        r.attr_data_runlist(68, 8, 8 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 2), r.buf.data(), RECORD_SIZE);
    }
    /* records 2,3,4,6,7: minimal meta */
    for (u64 rn : {2u, 3u, 4u, 6u, 7u}) {
        RecBuilder r;
        r.header(rn, RECORD_IN_USE);
        const char16_t* nm = rn == 6 ? u"$Bitmap" : (rn == 2 ? u"$LogFile" : u"$Volume");
        r.attr_file_name(5, nm, FILE_NAME_WIN32, 0x06, 4 * CLUSTER, 4 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + rn * 2), r.buf.data(), RECORD_SIZE);
    }
    /* record 5: root "." */
    {
        RecBuilder r;
        r.header(5, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(5, u".", FILE_NAME_POSIX, 0x00, 0, 0);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 10), r.buf.data(), RECORD_SIZE);
    }
    /* record 11: $Extend (dir) so $UsnJrnl's parent exists-ish */
    {
        RecBuilder r;
        r.header(11, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0x30);
        r.attr_file_name(5, u"$Extend", FILE_NAME_WIN32_AND_DOS, 0x30, 0, 0);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 22), r.buf.data(), RECORD_SIZE);
    }
    /* record 23: $UsnJrnl with named nonresident $DATA "$J" at LCN 160 x2 */
    {
        RecBuilder r;
        r.header(23, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(11, u"$UsnJrnl", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        {
            u32 pos = r.attr_pos;
            RunList rl;
            rl.runs().push_back(DataRun{0, 2, 0, J_LCN, false});
            std::vector<u8> rle = rl.encode();
            u32 rlen = u32(rle.size());
            const u32 name_off = 0x40;
            const u32 runs_off = name_off + 2 * 2;
            u32 alen = (runs_off + rlen + 7) / 8 * 8;
            r.u32wr(pos + 0, ATTR_TYPE_DATA);
            r.u32wr(pos + 4, alen);
            r.buf[pos + 8] = 1;
            r.buf[pos + 9] = 2;
            r.u16wr(pos + 10, u16(name_off));
            r.u16wr(pos + 16, 0);
            r.u64wr(pos + 24, 1);
            r.u16wr(pos + 32, u16(runs_off));
            r.u64wr(pos + 40, 2 * CLUSTER);
            r.u64wr(pos + 48, 2 * CLUSTER);
            r.u64wr(pos + 56, 2 * CLUSTER);
            r.u16wr(pos + name_off, u'$');
            r.u16wr(pos + name_off + 2, u'J');
            std::memcpy(r.buf.data() + pos + runs_off, rle.data(), rlen);
            r.attr_pos += alen;
        }
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 46), r.buf.data(), RECORD_SIZE);
    }
}

/* USN_RECORD_V2 writer (layout per winioctl.h; identical field placement to
 * the core's parser table at cr_mftwalk.cpp:1015-1023). */
u32 usn_record(std::vector<u8>& imgv, u32 at, u64 file_ref, u64 parent_ref, u64 ts, u32 reason,
               const char16_t* name) {
    const std::u16string n = name;
    const u32 namelen = u32(n.size() * 2);
    const u32 rlen = 64 + namelen;
    auto j32 = [&](u32 a, u32 x) {
        imgv[a] = u8(x);
        imgv[a + 1] = u8(x >> 8);
        imgv[a + 2] = u8(x >> 16);
        imgv[a + 3] = u8(x >> 24);
    };
    auto j16 = [&](u32 a, u16 x) {
        imgv[a] = u8(x);
        imgv[a + 1] = u8(x >> 8);
    };
    auto j64 = [&](u32 a, u64 x) {
        for (int i = 0; i < 8; i++)
            imgv[a + size_t(i)] = u8(x >> (8 * i));
    };
    j32(at + 0, rlen);
    j16(at + 4, 2); /* major 2 */
    j16(at + 6, 0); /* minor 0 */
    j64(at + 8, file_ref);
    j64(at + 16, parent_ref);
    j64(at + 24, 1); /* USN */
    j64(at + 32, ts);
    j32(at + 40, reason);
    j32(at + 44, 0);  /* source info */
    j32(at + 48, 0);  /* security id */
    j32(at + 52, 0x20); /* file attributes (ARCHIVE) */
    j32(at + 56, namelen);
    j32(at + 60, 64); /* name offset */
    for (size_t i = 0; i < n.size(); ++i)
        j16(at + 64 + u32(i) * 2, u16(n[i]));
    return rlen;
}

} // namespace

int main() {
    std::printf("USN reason-constant probe (core HEAD 34b3441)\n");
    std::printf("winioctl.h: USN_REASON_FILE_CREATE=0x00000100 "
                "USN_REASON_FILE_DELETE=0x00000200 USN_REASON_CLOSE=0x80000000\n");
    std::printf("core header: cr_mftwalk.h USN_REASON_FILE_DELETE = 0x00020000\n");
    std::printf("og decompile: FUN_1800196d0 harvest gate `reason & 0x200` "
                "(NTFSSearchDLL_decompiled.c:20098; helper +0x80 harvest-all flag "
                "ctor-defaults to 0 at :19371)\n\n");

    Image img;
    build_image(img);
    {
        std::vector<u8>& imgv = img.raw();
        const u32 jbase = u32(J_LCN * SEC);
        u32 off = jbase;
        /* 1. what Windows actually writes on deletion: FILE_DELETE|CLOSE */
        off += usn_record(imgv, off, 9001, 5 | (1ULL << 48), 132000000000000001ULL,
                          0x80000200u, u"realdelete.txt");
        /* 2. what the core's green unit test feeds: FILE_CREATE|0x20000 */
        off += usn_record(imgv, off, 9002, 5 | (1ULL << 48), 132000000000000002ULL,
                          0x00020100u, u"fixturesync.txt");
        /* 3. plain create (must never be harvested) */
        off += usn_record(imgv, off, 9003, 5 | (1ULL << 48), 132000000000000003ULL,
                          0x00000100u, u"createonly.txt");
    }

    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    if (vol == nullptr) {
        std::printf("OPEN FAILED err=%u\n", err);
        return 1;
    }
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    WalkerFlags wf; /* defaults: harvest_usnjrnl = true */
    WalkStats stats;
    u32 rc = walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    std::printf("walk_mft rc=%u\n", rc);
    std::printf("stats.usn_records = %llu  (3 records were written into $J)\n",
                (unsigned long long)stats.usn_records);
    std::printf("stats.usn_names   = %llu\n", (unsigned long long)stats.usn_names);
    std::printf("root_lost children:\n");
    for (tree::FileNode* c = rl->first_child(); c != nullptr; c = c->next_sibling())
        std::printf("  - %s\n", c->name_utf8().c_str());
    std::printf("\n");
    bool real_ok = false, fixture_ok = false, create_ok = false;
    for (tree::FileNode* c = rl->first_child(); c != nullptr; c = c->next_sibling()) {
        if (c->name_utf8() == "realdelete.txt")
            real_ok = true;
        if (c->name_utf8() == "fixturesync.txt")
            fixture_ok = true;
        if (c->name_utf8() == "createonly.txt")
            create_ok = true;
    }
    std::printf("  realdelete.txt (REAL deletion record):     %s\n",
                real_ok ? "harvested" : "NOT HARVESTED");
    std::printf("  fixturesync.txt (test-fixture 0x20100 rec): %s\n",
                fixture_ok ? "harvested" : "not harvested");
    std::printf("  createonly.txt (FILE_CREATE only):          %s\n",
                create_ok ? "harvested (BAD)" : "not harvested (correct)");
    std::printf("\nog NTFSSearchDLL with the same $J would harvest realdelete.txt "
                "(gate `reason & 0x200`) and reject fixturesync.txt.\n");
    return 0;
}
