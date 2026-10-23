/* End-to-end NTFS volume test: synthetic image -> open -> walk -> grade.
 *
 * Fixture layout (512-byte sectors/clusters, 1024-byte records, 256 clusters):
 *   LBA 0        boot sector (mft_lcn 4, mirr_lcn 68, cpr 2 -> 1024B records)
 *   LCN 4..67    $MFT (64 clusters -> 32 records: 0..31)
 *   LCN 68..75   $MFTMirr (copies of records 0..3)
 *   LCN 80..83   $Bitmap stream (4 clusters)
 *   records: 0..7 meta ($ named), 5 = root ".", 16 folder1, 17 readme,
 *            18 deleted.dat (clusters 210..213 free), 19 overwrit.dat
 *            (220..223 in-use), 20 partial.dat (230,231 in-use; 232,233 free),
 *            21 hard-link record (two $FILE_NAME: parents 16 and 5),
 *            22 ads.txt with named resident $DATA "sec"
 */
#include <gtest/gtest.h>
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/storage/cr_device.h"
#include <array>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::ntfs;

namespace {

constexpr u32 SEC = 512;
constexpr u32 CLUSTER = SEC; /* spc = 1 */
constexpr u64 NSECTORS = 256;
constexpr u64 RECORD_SIZE = 1024;
constexpr u64 MFT_LCN = 4;
constexpr u64 MFT_CLUSTERS = 64; /* records 0..31 */
constexpr u64 MIRR_LCN = 68;
constexpr u64 BITMAP_LCN = 80;
constexpr u64 BITMAP_CLUSTERS = 4;

class Image {
public:
    Image() : data_(NSECTORS * SEC, 0) {
        geom_.logical_sector = SEC;
        geom_.physical_sector = SEC;
        geom_.sector_count = NSECTORS;
        geom_.total_bytes = data_.size();
    }
    u8* sec(u64 lba) { return data_.data() + lba * SEC; }
    const u8* sec(u64 lba) const { return data_.data() + lba * SEC; }
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

/* ---- record builder ---- */
struct RecBuilder {
    std::vector<u8> buf;
    u32 attr_pos = 0;

    RecBuilder() { buf.assign(RECORD_SIZE, 0); }

    void header(u64 record_no, u16 flags) {
        auto* rh = reinterpret_cast<RecordHeader*>(buf.data());
        rh->magic = RECORD_MAGIC_FILE;
        rh->update_sequence_offset = 0x30;
        rh->update_sequence_size = 3; /* 2 sectors + 1 */
        rh->sequence = 1;
        rh->link_count = 1;
        rh->attribute_offset = 0x38;
        rh->flags = flags;
        rh->bytes_in_use = u32(RECORD_SIZE);
        rh->bytes_allocated = u32(RECORD_SIZE);
        rh->base_record = 0;
        rh->record_number = u32(record_no);
        attr_pos = 0x38;
        /* USA: slot0 = USN 0x0BB8; slots 1,2 = originals */
        u16 slots[3] = {0x0BB8, 0xAAAA, 0xBBBB};
        for (int i = 0; i < 3; ++i) {
            buf[0x30 + i * 2] = u8(slots[i]);
            buf[0x30 + i * 2 + 1] = u8(slots[i] >> 8);
        }
        /* tails hold the USN (torn state) */
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
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_STANDARD_INFORMATION);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 0;
        buf[pos + 9] = 0;
        u16_from_bytes_le_wr(buf, pos + 10, u16(voff));
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        u64_from_bytes_le_wr(buf, pos + voff + 0, 132000000000000000ULL);  /* created */
        u64_from_bytes_le_wr(buf, pos + voff + 8, 133000000000000000ULL);  /* modified */
        u64_from_bytes_le_wr(buf, pos + voff + 16, 134000000000000000ULL); /* changed */
        u64_from_bytes_le_wr(buf, pos + voff + 24, 135000000000000000ULL); /* accessed */
        u32_from_bytes_le_wr(buf, pos + voff + 32, dos_flags);
        attr_pos += alen;
    }

    void attr_file_name(u64 parent_record, const char16_t* name, u8 ns, u32 dos_flags,
                        u64 real_size, u64 alloc_size) {
        std::u16string n = name;
        u32 vlen = 66 + u32(n.size()) * 2;
        u32 voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_FILE_NAME);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 0;
        buf[pos + 9] = 0;
        u16_from_bytes_le_wr(buf, pos + 10, u16(voff));
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        u8* d = buf.data() + pos + voff;
        u64_from_bytes_le_wr(buf, pos + voff + 0, parent_record | (1ULL << 48));
        u64_from_bytes_le_wr(buf, pos + voff + 8, 132000000000000000ULL);
        u64_from_bytes_le_wr(buf, pos + voff + 16, 133000000000000000ULL);
        u64_from_bytes_le_wr(buf, pos + voff + 24, 134000000000000000ULL);
        u64_from_bytes_le_wr(buf, pos + voff + 32, 135000000000000000ULL);
        u64_from_bytes_le_wr(buf, pos + voff + 40, alloc_size);
        u64_from_bytes_le_wr(buf, pos + voff + 48, real_size);
        u32_from_bytes_le_wr(buf, pos + voff + 56, dos_flags);
        u32_from_bytes_le_wr(buf, pos + voff + 60, 0);
        d[64] = u8(n.size());
        d[65] = ns;
        for (size_t i = 0; i < n.size(); ++i)
            u16_from_bytes_le_wr(buf, pos + voff + 66 + u32(i) * 2, n[i]);
        attr_pos += alen;
    }

    void attr_data_runlist(u64 lcn, u64 clusters, u64 real_size) {
        u32 pos = attr_pos;
        /* runlist bytes: 1 run */
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 1; /* nonresident */
        u16_from_bytes_le_wr(buf, pos + 10, 0x40);
        u16_from_bytes_le_wr(buf, pos + 16, 0);            /* start_vcn (nonresident tail at +16) */
        u64_from_bytes_le_wr(buf, pos + 24, clusters - 1); /* last_vcn */
        u16_from_bytes_le_wr(buf, pos + 32, 0x40);         /* runlist_offset */
        u32_from_bytes_le_wr(buf, pos + 36, 0);
        u64_from_bytes_le_wr(buf, pos + 40, clusters * CLUSTER);
        u64_from_bytes_le_wr(buf, pos + 48, real_size);
        u64_from_bytes_le_wr(buf, pos + 56, real_size);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    void attr_data_resident(const char16_t* stream_name, u32 payload_words) {
        std::u16string n = stream_name;
        u32 vlen = payload_words * 4;
        u32 voff = 0x18 + u32(n.size()) * 2;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 0; /* resident */
        buf[pos + 9] = u8(n.size());
        u16_from_bytes_le_wr(buf, pos + 10, 0x18);
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        for (size_t i = 0; i < n.size(); ++i)
            u16_from_bytes_le_wr(buf, pos + 0x18 + u32(i) * 2, n[i]);
        for (u32 i = 0; i < vlen / 4; ++i)
            u32_from_bytes_le_wr(buf, pos + voff + i * 4, 0xC0FFEE00 + i);
        attr_pos += alen;
    }

    void attr_index_root() {
        u32 vlen = 0x20 + 16; /* header + one end-entry */
        u32 voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_INDEX_ROOT);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        u16_from_bytes_le_wr(buf, pos + 10, 0x18);
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        u8* d = buf.data() + pos + voff;
        u32_from_bytes_le_wr(buf, pos + voff + 0x00, 0x30); /* $I30 */
        u32_from_bytes_le_wr(buf, pos + voff + 0x04, 0);    /* collation */
        u32_from_bytes_le_wr(buf, pos + voff + 0x08, RECORD_SIZE);
        d[0x0C] = 2;
        d[0x0D] = 0;                                        /* clusters per block */
        u32_from_bytes_le_wr(buf, pos + voff + 0x10, 0x20); /* entries_offset */
        u32_from_bytes_le_wr(buf, pos + voff + 0x14, 16);
        u32_from_bytes_le_wr(buf, pos + voff + 0x18, 16);
        u32_from_bytes_le_wr(buf, pos + voff + 0x1C, 0); /* flags: small */
        /* end entry: parent back-ref, flag 2 */
        u64_from_bytes_le_wr(buf, pos + voff + 0x20, 5 | (1ULL << 48));
        u16_from_bytes_le_wr(buf, pos + voff + 0x28, 16);
        u16_from_bytes_le_wr(buf, pos + voff + 0x2A, 0);
        u32_from_bytes_le_wr(buf, pos + voff + 0x2C, 2);
        attr_pos += alen;
    }

    /* instance wrappers used by inline attribute construction in tests */
    void u32wr(size_t at, u32 x) { u32_from_bytes_le_wr(buf, at, x); }
    void u16wr(size_t at, u16 x) { u16_from_bytes_le_wr(buf, at, x); }
    void u64wr(size_t at, u64 x) { u64_from_bytes_le_wr(buf, at, x); }

    /* $ATTRIBUTE_LIST entry: (type, lowest_vcn, record_ref), 26 bytes each */
    void attr_attrlist_resident(const std::vector<std::array<u64, 3>>& entries) {
        std::vector<u8> content;
        for (const auto& e : entries) {
            std::array<u8, 26> rec = {};
            u32_from_bytes_le_wr_content(rec, 0, u32(e[0])); /* type */
            rec[4] = 26;                                     /* reclen */
            rec[5] = 0;
            rec[6] = 0;                                  /* namelen */
            rec[7] = 1;                                  /* nonresident */
            u64_from_bytes_le_wr_content(rec, 8, e[1]);  /* lowest vcn */
            u64_from_bytes_le_wr_content(rec, 16, e[2]); /* record ref */
            rec[24] = 0;                                 /* attr id */
            rec[25] = 0;
            content.insert(content.end(), rec.begin(), rec.end());
        }
        u32 vlen = u32(content.size());
        u32 voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_ATTRIBUTE_LIST);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 0;
        buf[pos + 9] = 0;
        u16_from_bytes_le_wr(buf, pos + 10, u16(voff));
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        std::memcpy(buf.data() + pos + voff, content.data(), vlen);
        attr_pos += alen;
    }

    /* nonresident $ATTRIBUTE_LIST whose content lives at lcn */
    void attr_attrlist_nonresident(u64 lcn, u64 clusters, u64 actual) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_ATTRIBUTE_LIST);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 1; /* nonresident */
        u16_from_bytes_le_wr(buf, pos + 10, 0x40);
        u16_from_bytes_le_wr(buf, pos + 16, 0); /* start_vcn */
        u64_from_bytes_le_wr(buf, pos + 24, clusters - 1);
        u16_from_bytes_le_wr(buf, pos + 32, 0x40); /* runlist_offset */
        u64_from_bytes_le_wr(buf, pos + 40, clusters * CLUSTER);
        u64_from_bytes_le_wr(buf, pos + 48, actual);
        u64_from_bytes_le_wr(buf, pos + 56, actual);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    /* $DATA fragment with an explicit start_vcn (attrlist chase pieces) */
    void attr_data_runlist_vcn(u64 lcn, u64 clusters, u64 start_vcn, u64 real_size) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{start_vcn, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 1; /* nonresident */
        u16_from_bytes_le_wr(buf, pos + 10, 0x40);
        u16_from_bytes_le_wr(buf, pos + 16, u16(start_vcn & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 18, u16(start_vcn >> 16));
        u64_from_bytes_le_wr(buf, pos + 24, start_vcn + clusters - 1);
        u16_from_bytes_le_wr(buf, pos + 32, 0x40);
        u64_from_bytes_le_wr(buf, pos + 40, clusters * CLUSTER);
        u64_from_bytes_le_wr(buf, pos + 48, real_size);
        u64_from_bytes_le_wr(buf, pos + 56, real_size);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    /* raw attrlist entry bytes for a nonresident list's content */
    static std::vector<u8> attrlist_bytes(const std::vector<std::array<u64, 3>>& entries) {
        std::vector<u8> content;
        for (const auto& e : entries) {
            std::array<u8, 26> rec = {};
            u32_from_bytes_le_wr_content(rec, 0, u32(e[0]));
            rec[4] = 26;
            rec[5] = 0;
            rec[6] = 0;
            rec[7] = 1;
            u64_from_bytes_le_wr_content(rec, 8, e[1]);
            u64_from_bytes_le_wr_content(rec, 16, e[2]);
            content.insert(content.end(), rec.begin(), rec.end());
        }
        return content;
    }

    void finish() {
        /* terminator */
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_TERMINATOR);
        u32_from_bytes_le_wr(buf, pos + 4, 8);
        attr_pos += 8;
        auto* rh = reinterpret_cast<RecordHeader*>(buf.data());
        rh->bytes_in_use = attr_pos;
    }

    static void u32_from_bytes_le_wr_content(std::array<u8, 26>& v, size_t at, u32 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
        v[at + 2] = u8(x >> 16);
        v[at + 3] = u8(x >> 24);
    }
    static void u64_from_bytes_le_wr_content(std::array<u8, 26>& v, size_t at, u64 x) {
        for (int i = 0; i < 8; i++)
            v[at + size_t(i)] = u8(x >> (8 * i));
    }

    static void u32_from_bytes_le_wr(std::vector<u8>& v, size_t at, u32 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
        v[at + 2] = u8(x >> 16);
        v[at + 3] = u8(x >> 24);
    }
    static void u16_from_bytes_le_wr(std::vector<u8>& v, size_t at, u16 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
    }
    static void u64_from_bytes_le_wr(std::vector<u8>& v, size_t at, u64 x) {
        for (int i = 0; i < 8; ++i)
            v[at + i] = u8(x >> (8 * i));
    }
};

void build_image(Image& img) {
    /* boot sector */
    {
        RecBuilder b; /* reuse writer helpers on the raw sector */
        u8* bs = img.sec(0);
        BootSector* boot = reinterpret_cast<BootSector*>(bs);
        const char oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
        std::memcpy(boot->oem, oem, 8);
        boot->bytes_per_sector = SEC;
        boot->sectors_per_cluster = 1;
        boot->media_descriptor = 0xF8;
        boot->total_sectors = NSECTORS;
        boot->mft_lcn = MFT_LCN;
        boot->mftmirr_lcn = MIRR_LCN;
        boot->clusters_per_record = 2; /* 2 clusters = 1024B */
        boot->clusters_per_index_block = 2;
        boot->checksum = 0;
        bs[510] = 0x55;
        bs[511] = 0xAA;
    }

    /* record 0: $MFT with $DATA runlist LCN 4 len 64 */
    {
        RecBuilder r;
        r.header(0, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFT", FILE_NAME_WIN32, 0x06, MFT_CLUSTERS * CLUSTER,
                         MFT_CLUSTERS * CLUSTER);
        r.attr_data_runlist(MFT_LCN, MFT_CLUSTERS, MFT_CLUSTERS * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 0), r.buf.data(), RECORD_SIZE);
    }
    /* record 1: $MFTMirr with $DATA runlist LCN 68 len 8 */
    {
        RecBuilder r;
        r.header(1, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFTMirr", FILE_NAME_WIN32, 0x06, 8 * CLUSTER, 8 * CLUSTER);
        r.attr_data_runlist(MIRR_LCN, 8, 8 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 2), r.buf.data(), RECORD_SIZE);
        /* mirror copies of records 0..3 at MIRR region */
        std::memcpy(img.sec(MIRR_LCN), img.sec(MFT_LCN), RECORD_SIZE);         /* rec0 copy */
        std::memcpy(img.sec(MIRR_LCN + 2), r.buf.data(), RECORD_SIZE);         /* rec1 copy */
        std::memcpy(img.sec(MIRR_LCN + 4), img.sec(MFT_LCN + 4), RECORD_SIZE); /* rec2 */
        std::memcpy(img.sec(MIRR_LCN + 6), img.sec(MFT_LCN + 6), RECORD_SIZE); /* rec3 */
    }
    /* records 2,3,4,6,7: meta ($ named) - minimal in-use records */
    for (u64 rn : {2u, 3u, 4u, 6u, 7u}) {
        RecBuilder r;
        r.header(rn, RECORD_IN_USE);
        const char16_t* nm = rn == 6 ? u"$Bitmap" : (rn == 2 ? u"$LogFile" : u"$Volume");
        r.attr_file_name(5, nm, FILE_NAME_WIN32, 0x06, 4 * CLUSTER, 4 * CLUSTER);
        if (rn == 6)
            r.attr_data_runlist(BITMAP_LCN, BITMAP_CLUSTERS, BITMAP_CLUSTERS * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + rn * 2), r.buf.data(), RECORD_SIZE);
    }
    /* record 5: root "." (parent = itself) with resident $I30 */
    {
        RecBuilder r;
        r.header(5, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(5, u".", FILE_NAME_POSIX, 0x00, 0, 0);
        r.attr_index_root();
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 10), r.buf.data(), RECORD_SIZE);
    }
    /* record 16: folder1 (parent 5) */
    {
        RecBuilder r;
        r.header(16, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0x20);
        r.attr_file_name(5, u"folder1", FILE_NAME_WIN32, 0x20, 0, 0);
        r.attr_index_root();
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 32), r.buf.data(), RECORD_SIZE);
    }
    /* record 17: readme.txt (parent 16, in-use, data LCN 200 len 2) */
    {
        RecBuilder r;
        r.header(17, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(16, u"readme.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 1024, 1024);
        r.attr_data_runlist(200, 2, 1024);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 34), r.buf.data(), RECORD_SIZE);
    }
    /* record 18: deleted.dat (parent 16, DELETED, LCN 210 len 4) */
    {
        RecBuilder r;
        r.header(18, 0); /* not in use */
        r.attr_si(0x20);
        r.attr_file_name(16, u"deleted.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(210, 4, 2048);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 36), r.buf.data(), RECORD_SIZE);
    }
    /* record 19: overwrit.dat (DELETED, LCN 220..223 -> bitmap SET) */
    {
        RecBuilder r;
        r.header(19, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"overwrit.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(220, 4, 2048);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 38), r.buf.data(), RECORD_SIZE);
    }
    /* record 20: partial.dat (DELETED, LCN 230..233, 230,231 in-use) */
    {
        RecBuilder r;
        r.header(20, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"partial.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(230, 4, 2048);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 40), r.buf.data(), RECORD_SIZE);
    }
    /* record 21: hard link (parents 16 AND 5) */
    {
        RecBuilder r;
        r.header(21, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(16, u"hard.txt", FILE_NAME_WIN32, 0x20, 512, 512);
        r.attr_file_name(5, u"hard@root.txt", FILE_NAME_WIN32, 0x20, 512, 512);
        r.attr_data_runlist(240, 1, 512);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 42), r.buf.data(), RECORD_SIZE);
    }
    /* record 22: ads.txt (parent 16, named resident $DATA "sec") */
    {
        RecBuilder r;
        r.header(22, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(16, u"ads.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 256, 256);
        r.attr_data_runlist(250, 1, 256);
        r.attr_data_resident(u"sec", 4); /* 16 bytes payload */
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 44), r.buf.data(), RECORD_SIZE);
    }

    /* $Bitmap stream: bits set = in-use.
     * meta: 4..83 (MFT+mirr+bitmap), live: 200,201,240,250;
     * deleted content NOT set: 210..213, 220..223(overwrit->grade), 232,233;
     * partial: 230,231 SET. */
    {
        std::vector<u8> bm(BITMAP_CLUSTERS * CLUSTER, 0);
        auto setbit = [&bm](u64 c) {
            if (c / 8 < bm.size())
                bm[c / 8] |= u8(1u << (c % 8));
        };
        for (u64 c = 4; c <= 83; ++c)
            setbit(c);
        setbit(200);
        setbit(201);
        setbit(240);
        setbit(250);
        setbit(220);
        setbit(221);
        setbit(222);
        setbit(223); /* overwrit.dat */
        setbit(230);
        setbit(231);
        std::memcpy(img.sec(BITMAP_LCN), bm.data(), bm.size());
    }
}

/* ------------------------------------------------------------------ */
/* session-5 audit row 3/6/7: attrlist depth, resident ADS, ADS attach,
 * UsnJrnl harvest, FILE0 salvage                                       */
/* ------------------------------------------------------------------ */

TEST(NtfsVolume, NonresidentAttrlistOnMft) {
    /* $MFT's own $ATTRIBUTE_LIST stored nonresident: runlist decode +
     * bounded stream read, then the fragmented-MFT reassembly. Primary
     * $DATA covers records 0..15 (LCN 4, 32 clusters); record 15 (inside
     * the primary, hence readable) carries the vcn 32..39 fragment at
     * LCN 200; record 18 lives in the extension. */
    Image img;
    build_image(img); /* boot sector + standard records */
    {
        RecBuilder r;
        r.header(0, RECORD_IN_USE);
        r.attr_data_runlist(MFT_LCN, 32, 32 * CLUSTER);
        r.attr_attrlist_nonresident(100, 1, 26);
        r.finish();
        std::memcpy(img.sec(MFT_LCN), r.buf.data(), RECORD_SIZE);
    }
    /* attrlist content at LCN 100: $DATA vcn32 -> record 15 */
    {
        auto bytes = RecBuilder::attrlist_bytes({{ATTR_TYPE_DATA, 32, 15 | (1ULL << 48)}});
        std::memcpy(img.sec(100), bytes.data(), bytes.size());
    }
    /* record 15: the extension carrier (must sit inside the primary) */
    {
        RecBuilder r;
        r.header(15, RECORD_IN_USE);
        r.attr_data_runlist_vcn(200, 8, 32, 40 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 30), r.buf.data(), RECORD_SIZE);
    }
    /* record 18 inside the extension: LCN 200 + (18-16)*2 sectors */
    {
        RecBuilder r;
        r.header(18, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"extfile.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 64, 64);
        r.finish();
        std::memcpy(img.sec(200 + 4), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    /* the record gate now covers the extension (audit row 3) */
    EXPECT_EQ(vol->max_record_no(), 20u);
    std::vector<u8> rec;
    ASSERT_EQ(vol->read_record(18, rec), OK);
    EXPECT_EQ(std::memcmp(rec.data(), "FILE", 4), 0);
}

TEST(NtfsVolume, NestedAttrlistChase) {
    /* file record with $ATTRIBUTE_LIST -> child with ITS OWN attrlist ->
     * grandchild carrying the final $DATA fragment: the chase must follow
     * both levels and merge every run */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(23, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"bigfrag.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_attrlist_resident({{ATTR_TYPE_DATA, 0, 24 | (1ULL << 48)}});
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 46), r.buf.data(), RECORD_SIZE);
    }
    {
        /* child: first fragment + a nested attrlist -> record 25 */
        RecBuilder r;
        r.header(24, RECORD_IN_USE);
        r.attr_data_runlist_vcn(120, 4, 0, 4096);
        r.attr_attrlist_resident({{ATTR_TYPE_DATA, 4, 25 | (1ULL << 48)}});
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 48), r.buf.data(), RECORD_SIZE);
    }
    {
        /* grandchild: second fragment */
        RecBuilder r;
        r.header(25, RECORD_IN_USE);
        r.attr_data_runlist_vcn(130, 4, 4, 4096);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 50), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << err_text(err);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    tree::FileNode* big = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "bigfrag.txt")
                big = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(big, nullptr);
    ASSERT_NE(big->props(), nullptr);
    /* both fragments merged: 8 clusters across two extents */
    u64 total_bytes = 0;
    for (const auto& e : big->props()->extents)
        total_bytes += e.length;
    EXPECT_EQ(total_bytes, u64(8 * CLUSTER));
    EXPECT_EQ(big->props()->extents.size(), size_t(2));
}

TEST(NtfsVolume, ResidentAdsCarriesContent) {
    /* the record-22 "sec" stream: size-only storage left the payload
     * unrecoverable; props must now carry the resident bytes */
    Image img;
    build_image(img);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << err_text(err);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    tree::FileNode* adsnode = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "ads.txt:sec")
                adsnode = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(adsnode, nullptr);
    ASSERT_NE(adsnode->props(), nullptr);
    EXPECT_EQ(adsnode->props()->logical_size, 16u);
    ASSERT_EQ(adsnode->props()->resident_data.size(), size_t(16));
    for (u32 i = 0; i < 4; i++) {
        const u32 expect = 0xC0FFEE00 + i;
        const u32 got = u32(adsnode->props()->resident_data[i * 4]) |
                        (u32(adsnode->props()->resident_data[i * 4 + 1]) << 8) |
                        (u32(adsnode->props()->resident_data[i * 4 + 2]) << 16) |
                        (u32(adsnode->props()->resident_data[i * 4 + 3]) << 24);
        EXPECT_EQ(got, expect) << "word " << i;
    }
}

TEST(NtfsVolume, AdsAttachesToFirstLink) {
    /* hard-linked file with an ADS: the stream must attach to the
     * canonical (first-emitted) link, not the last */
    Image img;
    build_image(img); /* boot sector + records 5/16 for the link targets */
    {
        RecBuilder r;
        r.header(17, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"hl@root.txt", FILE_NAME_WIN32, 0x20, 512, 512);
        r.attr_file_name(16, u"hl.txt", FILE_NAME_WIN32, 0x20, 512, 512);
        r.attr_data_runlist(240, 1, 512);
        r.attr_data_resident(u"stream", 2);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 34), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << err_text(err);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    tree::FileNode *at_root = nullptr, *at_folder = nullptr, *ads = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "hl@root.txt")
                at_root = c;
            if (c->name_utf8() == "hl.txt")
                at_folder = c;
            if (c->name_utf8() == "hl@root.txt:stream" || c->name_utf8() == "hl.txt:stream")
                ads = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(at_root, nullptr);
    ASSERT_NE(at_folder, nullptr);
    ASSERT_NE(ads, nullptr);
    /* the ADS node's parent IS the first link node (parent 5's link) */
    EXPECT_EQ(ads->name_utf8(), "hl@root.txt:stream");
    bool under_root_link = false;
    for (tree::FileNode* c = at_root->first_child(); c; c = c->next_sibling())
        if (c == ads)
            under_root_link = true;
    EXPECT_TRUE(under_root_link);
    bool under_folder_link = false;
    for (tree::FileNode* c = at_folder->first_child(); c; c = c->next_sibling())
        if (c == ads)
            under_folder_link = true;
    EXPECT_FALSE(under_folder_link);
}

TEST(NtfsVolume, UsnJrnlHarvest) {
    /* $UsnJrnl:$J with two V2 records: gonefile.txt (created+deleted -
    journal-only, its MFT record was reused) and keptfile.txt (created
    only). Only the deleted, structured-unknown name is harvested. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(23, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(11, u"$UsnJrnl", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        /* named nonresident $DATA "$J" -> LCN 160, 2 clusters */
        {
            u32 pos = r.attr_pos;
            RunList rl;
            rl.runs().push_back(DataRun{0, 2, 0, 160, false});
            std::vector<u8> rle = rl.encode();
            u32 rlen = u32(rle.size());
            const u32 name_off = 0x40;             /* name right after the tail */
            const u32 runs_off = name_off + 2 * 2; /* "$J" then the runlist */
            u32 alen = (runs_off + rlen + 7) / 8 * 8;
            r.u32wr(pos + 0, ATTR_TYPE_DATA);
            r.u32wr(pos + 4, alen);
            r.buf[pos + 8] = 1; /* nonresident */
            r.buf[pos + 9] = 2; /* name length "$J" */
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
    /* journal content: two USN_RECORD_V2 entries */
    {
        std::vector<u8>& imgv = img.raw();
        auto j32 = [&](u32 at, u32 x) {
            imgv[at] = u8(x);
            imgv[at + 1] = u8(x >> 8);
            imgv[at + 2] = u8(x >> 16);
            imgv[at + 3] = u8(x >> 24);
        };
        auto j16 = [&](u32 at, u16 x) {
            imgv[at] = u8(x);
            imgv[at + 1] = u8(x >> 8);
        };
        auto j64 = [&](u32 at, u64 x) {
            for (int i = 0; i < 8; i++)
                imgv[at + size_t(i)] = u8(x >> (8 * i));
        };
        const u64 jbase = 160 * SEC;
        auto usn = [&](u64 at, const char16_t* name, u32 reason) {
            const std::u16string n = name;
            const u32 namelen = u32(n.size() * 2);
            const u32 rlen = 64 + namelen;
            j32(u32(at + 0), rlen);
            j16(u32(at + 4), 2);                      /* major */
            j16(u32(at + 6), 0);                      /* minor */
            j64(u32(at + 8), 1234);                   /* file ref */
            j64(u32(at + 16), 5 | (1ULL << 48));      /* parent: root */
            j64(u32(at + 24), 1);                     /* usn */
            j64(u32(at + 32), 132000000000000000ULL); /* timestamp */
            j32(u32(at + 40), reason);
            j32(u32(at + 44), 0);    /* source */
            j32(u32(at + 48), 0);    /* security */
            j32(u32(at + 52), 0x20); /* attributes */
            j32(u32(at + 56), namelen);
            j32(u32(at + 60), 64); /* name offset */
            for (size_t i = 0; i < n.size(); i++)
                j16(u32(at + 64 + u32(i) * 2), u16(n[i]));
            return rlen;
        };
        u64 off = jbase;
        off += usn(off, u"keptfile.txt", USN_REASON_FILE_CREATE);
        off += usn(off, u"gonefile.txt", USN_REASON_FILE_CREATE | USN_REASON_FILE_DELETE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << err_text(err);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    EXPECT_EQ(stats.usn_records, 2u);
    EXPECT_EQ(stats.usn_names, 1u);
    tree::FileNode* gone = nullptr;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "gonefile.txt")
            gone = c;
    ASSERT_NE(gone, nullptr);
    EXPECT_EQ(gone->origin(), tree::Origin::Lost);
    EXPECT_EQ(gone->deleted_state(), tree::DeletedState::Deleted);
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        EXPECT_NE(c->name_utf8(), "keptfile.txt");
}

TEST(NtfsVolume, UsnJrnlSparseV3Resync) {
    /* $J materialization must preserve VCN order across a SPARSE window
     * (zeros, not spliced-out), re-sync past a damaged record on 8-byte
     * boundaries instead of stopping, and parse USN_RECORD_V3 (128-bit
     * refs: parent@0x18, reason@0x38, namelen@0x48, name@0x50) alongside
     * V2. Layout: [data LCN 160, 1 cluster: V2 record + damaged stub]
     * [sparse 1 cluster: zeros] [data LCN 161, 1 cluster: V3 record]. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(23, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(11, u"$UsnJrnl", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        {
            u32 pos = r.attr_pos;
            RunList rl;
            /* run 1: data at LCN 160; run 2: sparse 1; run 3: data at 161 */
            rl.runs().push_back(DataRun{0, 1, 0, 160, false});
            rl.runs().push_back(DataRun{1, 1, 0, 0, true});
            rl.runs().push_back(DataRun{2, 1, 0, 161, false});
            std::vector<u8> rle = rl.encode();
            u32 rlen = u32(rle.size());
            const u32 name_off = 0x40;
            const u32 runs_off = name_off + 2 * 2;
            u32 alen = (runs_off + rlen + 7) / 8 * 8;
            r.u32wr(pos + 0, ATTR_TYPE_DATA);
            r.u32wr(pos + 4, alen);
            r.buf[pos + 8] = 1; /* nonresident */
            r.buf[pos + 9] = 2; /* name length "$J" */
            r.u16wr(pos + 10, u16(name_off));
            r.u16wr(pos + 16, 0);
            r.u64wr(pos + 24, 0);
            r.u16wr(pos + 32, u16(runs_off));
            r.u64wr(pos + 40, 3 * CLUSTER);
            r.u64wr(pos + 48, 3 * CLUSTER);
            r.u64wr(pos + 56, 3 * CLUSTER);
            r.u16wr(pos + name_off, u'$');
            r.u16wr(pos + name_off + 2, u'J');
            std::memcpy(r.buf.data() + pos + runs_off, rle.data(), rlen);
            r.attr_pos += alen;
        }
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 46), r.buf.data(), RECORD_SIZE);
    }
    {
        std::vector<u8>& imgv = img.raw();
        auto j32 = [&](u32 at, u32 x) {
            imgv[at] = u8(x);
            imgv[at + 1] = u8(x >> 8);
            imgv[at + 2] = u8(x >> 16);
            imgv[at + 3] = u8(x >> 24);
        };
        auto j16 = [&](u32 at, u16 x) {
            imgv[at] = u8(x);
            imgv[at + 1] = u8(x >> 8);
        };
        auto j64 = [&](u32 at, u64 x) {
            for (int i = 0; i < 8; i++)
                imgv[at + size_t(i)] = u8(x >> (8 * i));
        };
        /* V2 record in cluster 160: deleted name */
        const u64 jbase = 160 * SEC;
        u64 off = jbase;
        {
            const std::u16string n = u"gonefile.txt";
            const u32 namelen = u32(n.size() * 2);
            const u32 rlen = 64 + namelen;
            j32(u32(off + 0), rlen);
            j16(u32(off + 4), 2);
            j16(u32(off + 6), 0);
            j64(u32(off + 8), 1234);
            j64(u32(off + 16), 5 | (1ULL << 48));
            j64(u32(off + 24), 1);
            j64(u32(off + 32), 132000000000000000ULL);
            j32(u32(off + 40), USN_REASON_FILE_CREATE | USN_REASON_FILE_DELETE);
            j32(u32(off + 44), 0);
            j32(u32(off + 48), 0);
            j32(u32(off + 52), 0x20);
            j32(u32(off + 56), namelen);
            j32(u32(off + 60), 64);
            for (size_t i = 0; i < n.size(); i++)
                j16(u32(off + 64 + u32(i) * 2), u16(n[i]));
            off += rlen;
        }
        /* damaged stub: implausible length right after (forces a re-sync) */
        j32(u32(off + 0), 4);
        off += 16; /* land 8-aligned for the next record */
        /* V3 record in cluster 161 (after the sparse window): deleted name.
         * V3 layout (winioctl.h): 128-bit refs at +8/+0x18 shift the rest. */
        const u64 j3 = 161 * SEC;
        {
            const std::u16string n = u"v3file.txt";
            const u32 namelen = u32(n.size() * 2);
            const u32 rlen = 80 + namelen;
            j32(u32(j3 + 0), rlen);
            j16(u32(j3 + 4), 3); /* major = 3 */
            j16(u32(j3 + 6), 0);
            j64(u32(j3 + 8), 4321);              /* FileReferenceNumber[0] */
            j64(u32(j3 + 16), 0);                /* FileReferenceNumber[1] */
            j64(u32(j3 + 24), 5 | (1ULL << 48)); /* ParentFileReferenceNumber */
            j64(u32(j3 + 32), 0);
            j64(u32(j3 + 40), 2);                      /* Usn */
            j64(u32(j3 + 48), 132000000000000001ULL);  /* TimeStamp */
            j32(u32(j3 + 56), USN_REASON_FILE_DELETE); /* Reason */
            j32(u32(j3 + 60), 0);
            j32(u32(j3 + 64), 0);
            j32(u32(j3 + 68), 0x20);
            j32(u32(j3 + 72), namelen);
            j32(u32(j3 + 76), 80); /* name offset */
            for (size_t i = 0; i < n.size(); i++)
                j16(u32(j3 + 80 + u32(i) * 2), u16(n[i]));
        }
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << err_text(err);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    /* both the V2 record before the sparse window and the V3 record after
     * it were parsed - the harvest re-synced through the damaged stub and
     * the zero window without losing stream alignment */
    EXPECT_EQ(stats.usn_records, 2u);
    EXPECT_EQ(stats.usn_names, 2u);
    std::set<std::string> harvested;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        if (c->props())
            harvested.insert(c->name_utf8());
    EXPECT_TRUE(harvested.count("gonefile.txt"));
    EXPECT_TRUE(harvested.count("v3file.txt"));
}

TEST(NtfsVolume, ShiftedHeaderSalvage) {
    /* candidate record whose header parses as 'FILE' but fails deeper
     * (attribute_offset beyond the record), with a healthy record
     * starting at +0x58: "FILE" + usa_offset low byte 0x30 ('0') */
    Image img;
    {
        RecBuilder r; /* the healthy record that lands at +0x58 */
        r.header(999, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"shifted.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 100, 100);
        r.finish();
        std::vector<u8> window(0x58 + RECORD_SIZE, 0);
        /* broken head: 'FILE' magic but attribute_offset = 0xFF00 */
        std::memcpy(window.data(), "FILE", 4);
        window[0x14] = 0x00;
        window[0x15] = 0xFF;
        std::memcpy(window.data() + 0x58, r.buf.data(), RECORD_SIZE);
        std::memcpy(img.sec(10), window.data(), window.size());
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkStats stats;
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, nullptr, nullptr, nullptr, tree,
                               root_lost, stats);
    ASSERT_EQ(rc, OK);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "shifted.txt")
            found = c;
    ASSERT_NE(found, nullptr) << "shifted record not salvaged";
    EXPECT_EQ(found->origin(), tree::Origin::Lost);
    EXPECT_EQ(found->confidence(), tree::Confidence::MetadataOnly);
}

} // namespace

TEST(NtfsVolume, EndToEndWalk) {
    Image img;
    build_image(img);
    MemDevice dev(img.raw(), img.geom());

    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    EXPECT_EQ(vol->params().record_size, RECORD_SIZE);
    EXPECT_EQ(vol->params().cluster_bytes, CLUSTER);
    EXPECT_EQ(vol->max_record_no(), 32u);

    /* bitmap */
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    ASSERT_NE(bm, nullptr);
    EXPECT_TRUE(bm->test(200));
    EXPECT_FALSE(bm->test(210));

    /* walk */
    tree::ResultTree tree;
    auto* root_active = tree.add_root(u"Found (NTFS)");
    auto* root_deleted = tree.add_root(u"Deleted");
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    wf.skip_system_files = true;
    WalkStats stats;
    u32 rc = walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, root_active,
                      root_deleted, root_lost, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    /* meta records skipped by '$' name; SYSTEM-attributed: none in fixture */
    EXPECT_EQ(stats.meta_skipped, 7u); /* 0,1,2,3,4,6,7 ($ named) */
    EXPECT_EQ(stats.damaged, 0u);
    EXPECT_EQ(stats.orphans, 0u);
    EXPECT_EQ(stats.hardlinks, 1u);
    EXPECT_EQ(stats.ads, 1u);
    EXPECT_GE(stats.emitted, 9u);

    /* locate nodes by name */
    tree::FileNode *rootdot = nullptr, *folder1 = nullptr, *readme = nullptr, *deleted = nullptr,
                   *overwrit = nullptr, *partial = nullptr, *hard16 = nullptr, *hard5 = nullptr,
                   *adsnode = nullptr, *adschild = nullptr;
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    for (auto* n : all) {
        std::string nm = n->name_utf8();
        if (nm == ".")
            rootdot = n;
        else if (nm == "folder1")
            folder1 = n;
        else if (nm == "readme.txt")
            readme = n;
        else if (nm == "deleted.dat")
            deleted = n;
        else if (nm == "overwrit.dat")
            overwrit = n;
        else if (nm == "partial.dat")
            partial = n;
        else if (nm == "hard.txt")
            hard16 = n;
        else if (nm == "hard@root.txt")
            hard5 = n;
        else if (nm == "ads.txt")
            adsnode = n;
        else if (nm == "ads.txt:sec")
            adschild = n;
    }
    ASSERT_NE(rootdot, nullptr);
    ASSERT_NE(folder1, nullptr);
    ASSERT_NE(readme, nullptr);
    ASSERT_NE(deleted, nullptr);
    ASSERT_NE(overwrit, nullptr);
    ASSERT_NE(partial, nullptr);
    ASSERT_NE(hard16, nullptr);
    ASSERT_NE(hard5, nullptr);
    ASSERT_NE(adsnode, nullptr);
    ASSERT_NE(adschild, nullptr);

    /* structure: rootdot under active root; folder1 under rootdot; files
     * under folder1; partial under rootdot (parent 5) */
    EXPECT_EQ(rootdot->parent(), root_active);
    EXPECT_EQ(folder1->parent(), rootdot);
    EXPECT_EQ(readme->parent(), folder1);
    EXPECT_EQ(deleted->parent(), folder1);
    EXPECT_EQ(partial->parent(), rootdot);
    EXPECT_EQ(hard16->parent(), folder1);
    EXPECT_EQ(hard5->parent(), rootdot);
    EXPECT_EQ(adschild->parent(), adsnode);

    /* hard links: per-link props (names preserved), shared identity */
    EXPECT_EQ(hard16->props()->record_ref, hard5->props()->record_ref);
    EXPECT_EQ(hard16->props()->extents.size(), 1u);
    EXPECT_EQ(hard16->props()->extents[0].offset, 240 * CLUSTER);
    EXPECT_EQ(hard5->props()->extents[0].offset, 240 * CLUSTER);
    EXPECT_STREQ(hard16->name_utf8().c_str(), "hard.txt");
    EXPECT_STREQ(hard5->name_utf8().c_str(), "hard@root.txt");

    /* grading */
    EXPECT_EQ(readme->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(readme->deleted_state(), tree::DeletedState::Active);
    EXPECT_EQ(deleted->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(deleted->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(overwrit->confidence(), tree::Confidence::Overwritten);
    EXPECT_EQ(overwrit->deleted_state(), tree::DeletedState::PartiallyOverwritten);
    EXPECT_EQ(partial->confidence(), tree::Confidence::PartialRisk);
    EXPECT_EQ(partial->deleted_state(), tree::DeletedState::Deleted);

    /* ADS child */
    EXPECT_EQ(adschild->props()->logical_size, 16u);
    EXPECT_EQ(adschild->confidence(), tree::Confidence::Excellent);

    /* times preserved at 100ns (from $SI preferred) */
    EXPECT_EQ(readme->props()->times.created, 132000000000000000ULL);

    tree.refresh_stats();
    EXPECT_GE(root_active->stats().file_count, 0u);
}

TEST(NtfsVolume, RemnantScanFindsDetachedRecord) {
    /* a bare slice with one valid 'FILE' record floating at LBA 10 */
    Image img;
    {
        RecBuilder r;
        r.header(999, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"floating.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 100, 100);
        r.finish();
        std::memcpy(img.sec(10), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkStats stats;
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, nullptr, nullptr, nullptr, tree,
                               root_lost, stats);
    ASSERT_EQ(rc, OK);
    /* find the floating node */
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "floating.txt")
            found = c;
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->origin(), tree::Origin::Lost);
    EXPECT_EQ(found->confidence(), tree::Confidence::MetadataOnly);
}
