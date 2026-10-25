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
#include "cr/compress/cr_decomp.h"
#include "cr/storage/cr_device.h"
#include <array>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::ntfs;

namespace {

/* winioctl.h USN_REASON_FILE_CREATE - fixture-only value (the engine filters
 * on USN_REASON_FILE_DELETE alone; header no longer carries it, AUDIT-NTFS §4
 * row 5). */
constexpr u32 USN_REASON_FILE_CREATE = 0x00000100;

constexpr u32 SEC = 512;
constexpr u32 CLUSTER = SEC; /* spc = 1 */
constexpr u64 NSECTORS = 256;
constexpr u64 RECORD_SIZE = 1024;
constexpr u64 MFT_LCN = 4;
constexpr u64 MFT_CLUSTERS = 68; /* records 0..33 (30..33: recycle fixture) */
constexpr u64 MIRR_LCN = 68;
constexpr u64 BITMAP_LCN = 80;
constexpr u64 BITMAP_CLUSTERS = 4;
constexpr u64 TOTAL_CLUSTERS = NSECTORS; /* spc = 1: cluster count == sector count */

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

    void attr_data_runlist_compressed(u64 lcn, u64 clusters, u64 real_size, u16 cunit) {
        /* GAP-P1-1: nonresident $DATA with ATTR_FLAG_COMPRESSED (0x0001)
         * and compression_unit at +0x22 - the on-disk stream is the
         * compression-unit envelope, NOT raw file bytes */
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 1; /* nonresident */
        u16_from_bytes_le_wr(buf, pos + 10, 0x40);
        u16_from_bytes_le_wr(buf, pos + 12, 0x0001);       /* ATTR_FLAG_COMPRESSED */
        u16_from_bytes_le_wr(buf, pos + 16, 0);            /* start_vcn */
        u64_from_bytes_le_wr(buf, pos + 24, clusters - 1); /* last_vcn */
        u16_from_bytes_le_wr(buf, pos + 32, 0x40);         /* runlist_offset */
        u16_from_bytes_le_wr(buf, pos + 34, cunit);        /* compression_unit @+0x22 */
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

    /* resident unnamed $DATA carrying a v2 $I (Recycle Bin metadata) payload:
     * version 2, size, deletion time, pathlen, path (the ORIGINAL location) */
    void attr_data_recycle_info(u64 file_size, u64 del_time, const char16_t* orig_path) {
        std::u16string op = orig_path;
        u32 plen = u32(op.size());
        u32 vlen = 544;
        u32 voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 0; /* resident */
        buf[pos + 9] = 0; /* unnamed */
        u16_from_bytes_le_wr(buf, pos + 10, u16(voff));
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        u32_from_bytes_le_wr(buf, pos + voff + 0, 2); /* version 2 */
        u64_from_bytes_le_wr(buf, pos + voff + 8, file_size);
        u64_from_bytes_le_wr(buf, pos + voff + 16, del_time);
        u32_from_bytes_le_wr(buf, pos + voff + 24, plen);
        for (u32 i = 0; i < plen; ++i)
            u16_from_bytes_le_wr(buf, pos + voff + 28 + i * 2, op[i]);
        attr_pos += alen;
    }

    /* Windows 10+ VARIABLE-length $I: total = 28 + 2*pathlen (the @24 field
     * is the deletion-sequence counter, NOT the length; path NUL-padded) */
    void attr_data_recycle_info_v2_2(u64 file_size, u64 del_time, u32 seq,
                                     const char16_t* orig_path) {
        std::u16string op = orig_path;
        u32 plen = u32(op.size());
        u32 vlen = 28 + plen * 2;
        u32 voff = 0x18;
        u32 alen = (voff + vlen + 7) / 8 * 8;
        u32 pos = attr_pos;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 0;
        buf[pos + 9] = 0;
        u16_from_bytes_le_wr(buf, pos + 10, u16(voff));
        u16_from_bytes_le_wr(buf, pos + 16, u16(vlen & 0xFFFF));
        u16_from_bytes_le_wr(buf, pos + 20, u16(voff));
        u32_from_bytes_le_wr(buf, pos + voff + 0, 2); /* version 2 */
        u64_from_bytes_le_wr(buf, pos + voff + 8, file_size);
        u64_from_bytes_le_wr(buf, pos + voff + 16, del_time);
        u32_from_bytes_le_wr(buf, pos + voff + 24, seq); /* Win10 sequence field */
        for (u32 i = 0; i < plen; ++i)
            u16_from_bytes_le_wr(buf, pos + voff + 28 + i * 2, op[i]);
        /* NUL padding to vlen */
        for (u32 i = plen; i < plen + 4 && 28 + (i + 1) * 2 <= vlen; ++i)
            u16_from_bytes_le_wr(buf, pos + voff + 28 + i * 2, 0);
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
    /* record 28: $R7Z1Q4T.png (parent 31, in-use, nonresident) - REAL
     * Windows convention: the $R KEEPS the original extension while the
     * $I does not, and the $R record precedes its $I in MFT order (the
     * $R is the recycled file's own record; the $I is newly allocated).
     * Pre-fix: substr(2) keys "<rand>.png" vs "<rand>" never matched AND
     * the single-pass map lookup ran before the $I was parsed -> raw
     * $R-named rows (GAP-P1-3, bench: ours 0/26 vs og 26/26). */
    {
        RecBuilder r;
        r.header(28, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(31, u"$R7Z1Q4T.png", FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_data_runlist(254, 1, 4096); /* in-device (256-sector fixture) */
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 56), r.buf.data(), RECORD_SIZE);
    }
    /* record 29: $I7Z1Q4T (v1 $I, NO extension) */
    {
        RecBuilder r;
        r.header(29, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(31, u"$I7Z1Q4T", FILE_NAME_WIN32_AND_DOS, 0x20, 544, 544);
        r.attr_data_recycle_info(4096, 136000000000000000ULL, u"seed\\pics\\z.png");
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 58), r.buf.data(), RECORD_SIZE);
    }
    /* record 30: $RECYCLE.BIN dir (parent 5) - NOT a metafile: surfaces */
    {
        RecBuilder r;
        r.header(30, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(5, u"$RECYCLE.BIN", FILE_NAME_WIN32, 0x10, 0, 0);
        r.attr_index_root();
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 60), r.buf.data(), RECORD_SIZE);
    }
    /* record 31: SID dir (parent 30) */
    {
        RecBuilder r;
        r.header(31, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(30, u"S-1-5-21-999", FILE_NAME_WIN32, 0x10, 0, 0);
        r.attr_index_root();
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 62), r.buf.data(), RECORD_SIZE);
    }
    /* record 32: $I2X9K4L.jpg (parent 31) - resident unnamed $DATA
     * carrying the v2 $I payload (see RecBuilder::attr_data_recycle_info) */
    {
        RecBuilder r;
        r.header(32, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(31, u"$I2X9K4L.jpg", FILE_NAME_WIN32_AND_DOS, 0x20, 544, 544);
        r.attr_data_recycle_info(17847, 134000000000000000ULL, u"seed\\small\\s0099.jpg");
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 64), r.buf.data(), RECORD_SIZE);
    }
    /* record 33: $R2X9K4L.jpg (parent 31, in-use, nonresident $DATA) */
    {
        RecBuilder r;
        r.header(33, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(31, u"$R2X9K4L.jpg", FILE_NAME_WIN32_AND_DOS, 0x20, 17847, 17847);
        r.attr_data_runlist(240, 4, 17847);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 66), r.buf.data(), RECORD_SIZE);
    }
    /* record 24: $I3P7Q2N.bin (Win10 variable form) - payload 28+2*17 */
    {
        RecBuilder r;
        r.header(24, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(31, u"$I3P7Q2N.bin", FILE_NAME_WIN32_AND_DOS, 0x20, 62, 62);
        r.attr_data_recycle_info_v2_2(4096, 135000000000000000ULL, 37,
                                      u"seed\\doomed\\n0\\w00.bin");
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 48), r.buf.data(), RECORD_SIZE);
    }
    /* record 25: $R3P7Q2N.bin (in-use; maps via the variable $I) */
    {
        RecBuilder r;
        r.header(25, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(31, u"$R3P7Q2N.bin", FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_data_runlist(250, 1, 4096);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 50), r.buf.data(), RECORD_SIZE);
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
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, TOTAL_CLUSTERS, nullptr, nullptr,
                               nullptr, tree,                               root_lost, stats);
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
    EXPECT_EQ(vol->max_record_no(), 34u);

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

TEST(NtfsVolume, RecycleBinMapping) {
    Image img;
    build_image(img);
    MemDevice dev(img.raw(), img.geom());

    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);

    tree::ResultTree tree;
    auto* root_active = tree.add_root(u"Found (NTFS)");
    auto* root_deleted = tree.add_root(u"Deleted");
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    WalkStats stats;
    u32 rc = walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, root_active,
                      root_deleted, root_lost, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_GE(stats.recycle_info, 1u) << "the $I record must parse";

    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);

    tree::FileNode *remapped = nullptr, *bin_dir = nullptr, *sid_dir = nullptr;
    int r_named = 0, i_named = 0;
    for (tree::FileNode* n : all) {
        const std::string nm = n->name_utf8();
        if (nm == "$R2X9K4L.jpg")
            ++r_named; /* unmapped bin content row: must not exist */
        if (nm == "$I2X9K4L.jpg")
            ++i_named; /* bin metadata row: must not exist */
        if (nm == "s0099.jpg")
            remapped = n;
        else if (nm == "$RECYCLE.BIN")
            bin_dir = n;
        else if (nm == "S-1-5-21-999")
            sid_dir = n;
    }
    /* the bin structure surfaces (no more blanket $-skip) */
    ASSERT_NE(bin_dir, nullptr);
    ASSERT_NE(sid_dir, nullptr);
    /* $R content remapped to its ORIGINAL name with the $I size+time */
    ASSERT_NE(remapped, nullptr) << "the $R record must emit as s0099.jpg";
    EXPECT_EQ(u64(17847), remapped->props()->logical_size);
    EXPECT_EQ(u64(134000000000000000ULL), remapped->props()->times.modified);
    EXPECT_EQ(tree::DeletedState::Deleted, remapped->deleted_state());
    /* no $I/$R-named file rows anywhere */
    EXPECT_EQ(0, i_named);
    EXPECT_EQ(0, r_named);

    /* the Win10 variable-form $I pair (records 24/25): mapped too */
    tree::FileNode* remapped2 = nullptr;
    int r2_named = 0;
    for (tree::FileNode* n : all) {
        const std::string nm = n->name_utf8();
        if (nm == "w00.bin")
            remapped2 = n;
        if (nm == "$R3P7Q2N.bin")
            ++r2_named;
    }
    EXPECT_EQ(stats.recycle_info, 3u) << "all three $I records must parse";
    ASSERT_NE(remapped2, nullptr) << "variable-form $I must map its $R";
    EXPECT_EQ(u64(4096), remapped2->props()->logical_size);
    EXPECT_EQ(u64(135000000000000000ULL), remapped2->props()->times.modified);
    EXPECT_EQ(tree::DeletedState::Deleted, remapped2->deleted_state());
    EXPECT_EQ(0, r2_named);

    /* GAP-P1-3 pair: real Windows naming ($R keeps the extension, $I
     * does not) + $R at a LOWER record number than its $I (post-walk
     * fixup). Pre-fix both failed -> the row stayed raw-named. */
    tree::FileNode* remapped3 = nullptr;
    int r3_named = 0;
    for (tree::FileNode* n : all) {
        const std::string nm = n->name_utf8();
        if (nm == "z.png")
            remapped3 = n;
        if (nm == "$R7Z1Q4T.png")
            ++r3_named;
        if (nm == "$I7Z1Q4T")
            ++i_named;
    }
    ASSERT_NE(remapped3, nullptr) << "the real-convention pair must map";
    EXPECT_EQ(u64(4096), remapped3->props()->logical_size);
    EXPECT_EQ(u64(136000000000000000ULL), remapped3->props()->times.modified);
    EXPECT_EQ(tree::DeletedState::Deleted, remapped3->deleted_state());
    EXPECT_EQ(0, r3_named) << "no raw $R-named row for the mapped pair";
    EXPECT_EQ(0, i_named) << "$I metadata never surfaces";
    EXPECT_EQ(stats.recycle_remapped, 3u) << "both in-flight and post-walk remaps";
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
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, TOTAL_CLUSTERS, nullptr, nullptr,
                               nullptr, tree,                               root_lost, stats);
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

TEST(NtfsVolume, ViabilityProbeUsesDataSize) {
    /* PARITY-AUDIT F-A1-2: a deleted file whose $FILE_NAME real_size is 0
     * (the fresh-file real-volume case behind benchmark Gen-2) but whose
     * $DATA carries the authoritative 8 KiB size with one of two clusters
     * reused in $Bitmap. The viability probe must use the $DATA size and
     * grade PartialRisk via the bitmap — not take the <1 KiB fast-path to
     * Excellent (which skipped the $Bitmap check entirely). */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(26, 0); /* not in use = deleted */
        r.attr_si(0x20);
        r.attr_file_name(16, u"zfn.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 0, 0);
        r.attr_data_runlist(84, 2, 8192);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 52), r.buf.data(), RECORD_SIZE);
    }
    /* $Bitmap: cluster 84 reused (SET), 85 still free (fixture LCNs 84+
     * are outside every other record's runs; volume is 256 clusters) */
    {
        u8* bm = img.sec(BITMAP_LCN);
        bm[84 / 8] |= u8(1u << (84 % 8));
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    tree::ResultTree tree;
    auto* root_active = tree.add_root(u"Found");
    auto* root_deleted = tree.add_root(u"Deleted");
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    wf.skip_system_files = true;
    WalkStats stats;
    u32 rc = walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, root_active,
                      root_deleted, root_lost, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    tree::FileNode* zfn = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "zfn.dat")
                zfn = c;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(zfn, nullptr) << "deleted file with zero $FILE_NAME size missing";
    EXPECT_EQ(zfn->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(zfn->props()->logical_size, 8192u);                /* $DATA authoritative */
    EXPECT_EQ(zfn->confidence(), tree::Confidence::PartialRisk); /* 1/2 clusters reused */
}

TEST(NtfsVolume, ViabilityBoundaryExactFastPath) {
    /* NTFS-1 (AUDIT-NTFS): the og gate is STRICT - `0x3ff < size` (Dec
     * 21208). A deleted file of EXACTLY 1023 bytes takes the fast-path
     * with NO bitmap check, even when its cluster is fully reused; the old
     * `>=` probed it and graded Overwritten. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(26, 0); /* deleted */
        r.attr_si(0x20);
        r.attr_file_name(16, u"edge.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 0, 1023);
        r.attr_data_runlist(86, 1, 1023); /* exactly 0x3FF bytes, 1 cluster */
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 52), r.buf.data(), RECORD_SIZE);
    }
    { /* cluster 86 fully reused in $Bitmap */
        u8* bm = img.sec(BITMAP_LCN);
        bm[86 / 8] |= u8(1u << (86 % 8));
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    tree::FileNode* edge = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "edge.dat")
                edge = c;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(edge, nullptr);
    /* fast-path: no bitmap probe at exactly the boundary (og parity) */
    EXPECT_EQ(edge->confidence(), tree::Confidence::Excellent);
}

TEST(NtfsVolume, ExtensionRecordsNotCountedDamaged) {
    /* NTFS-8 (AUDIT-NTFS): extension records (base_record != 0) carry no
     * $FILE_NAME; og skips them silently (FUN_180041970, Dec 21155-21157).
     * Ours counted each one damaged via names.empty(). */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(27, 0); /* not in use; will become an extension record */
        r.attr_si(0x20);
        /* NO $FILE_NAME - an extension record's shape */
        r.attr_data_runlist(90, 1, 512);
        r.finish();
        auto* rh = reinterpret_cast<RecordHeader*>(r.buf.data());
        rh->base_record = 26; /* points at record 26 as the base */
        /* 27*2 = 54 (the pre-fix write at +53 straddled records 26/27, so
         * the patched record's header never landed at a record boundary
         * and the test passed without exercising the base-record skip) */
        std::memcpy(img.sec(MFT_LCN + 54), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    EXPECT_EQ(stats.damaged, 0u); /* the extension record is skipped, not damaged */
}

/* ---- GAP-P1-1: compressed $DATA recovers real content ----
 * The pre-fix engine surfaced the compression-unit envelope bytes as raw
 * extents: NTFS-compressed files recovered as silently-corrupt LZNT1
 * bytes. The walker now expands the envelope into codec-tagged extents
 * (kLznt1 + psize), and the writer/preview decode path turns them back
 * into the original bytes. Fixture: a 512-byte block compressed to a
 * 5-byte LZNT1 chunk ('A', tuple(back=1,len=510), 'B'). */
TEST(NtfsVolume, CompressedDataRecoversRealContent) {
    Image img;
    build_image(img);

    /* stored stream at cluster 252: u16 hdr 0x3004 (compressed, 5 bytes
     * payload) + LZNT1 tagged groups: tag 0x06 -> literal 'A', tuple
     * 0x01FB (back 1, len 510), literal 'B' -> 512 bytes decoded */
    {
        u8* blk = img.sec(252);
        blk[0] = 0x04;
        blk[1] = 0x30; /* hdr = 0x3004: compressed, payload 5 */
        blk[2] = 0x02; /* tag: b0=0 literal, b1=1 tuple, b2=0 literal */
        blk[3] = 'A';
        blk[4] = 0xFB; /* tuple LE 0x01FB: len = 0x1FB + 3 = 510, back = 0 + 1 */
        blk[5] = 0x01;
        blk[6] = 'B';
        /* cluster 253: all zeros (hole-unit fixture for record 27) */
    }

    /* record 26: squeezed.dat - compressed $DATA, cunit 0 (one 512 B block) */
    {
        RecBuilder r;
        r.header(26, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"squeezed.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 512, 512);
        r.attr_data_runlist_compressed(252, 1, 512, 0);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 52), r.buf.data(), RECORD_SIZE);
    }
    /* record 27: holed.dat - compressed $DATA whose stored unit is an
     * all-zero header (0x0000): the unit is a HOLE - no extents, the
     * content is sparse zeros (never raw envelope bytes) */
    {
        RecBuilder r;
        r.header(27, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"holed.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 512, 512);
        r.attr_data_runlist_compressed(253, 1, 512, 0);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 54), r.buf.data(), RECORD_SIZE);
    }

    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);

    tree::ResultTree tree;
    auto* root_active = tree.add_root(u"Found (NTFS)");
    auto* root_deleted = tree.add_root(u"Deleted");
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    WalkStats stats;
    u32 rc = walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, root_active,
                      root_deleted, root_lost, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    tree::FileNode *sq = nullptr, *ho = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            const std::string nm = c->name_utf8();
            if (nm == "squeezed.dat")
                sq = c;
            if (nm == "holed.dat")
                ho = c;
            find(c);
        }
    };
    for (auto* r : tree.roots())
        find(r);

    /* squeezed.dat: ONE codec-tagged extent - never a raw runlist extent */
    ASSERT_NE(sq, nullptr) << "compressed file must surface";
    ASSERT_EQ(size_t(1), sq->props()->extents.size())
        << "one block = one extent (raw runlist extents were the bug)";
    const auto& ext = sq->props()->extents[0];
    EXPECT_EQ(u64(252) * CLUSTER + 2, ext.offset); /* after the u16 header */
    EXPECT_EQ(u64(512), ext.length);
    EXPECT_EQ(u32(compress::kLznt1), ext.codec);
    EXPECT_EQ(u64(5), ext.psize);
    EXPECT_EQ(u64(1), stats.compressed_blocks);

    /* end-to-end content proof: the writer's exact decode path */
    std::vector<u8> raw(5, 0);
    ASSERT_EQ(OK, vol->read_at(ext.offset, raw.data(), 5));
    std::vector<u8> out(512, 0);
    ASSERT_TRUE(
        compress::decompress_block(ext.codec, raw.data(), raw.size(), out.data(), out.size()));
    EXPECT_EQ('A', out[0]);
    EXPECT_EQ('A', out[510]);
    EXPECT_EQ('B', out[511]);
    for (size_t i = 0; i < 511; ++i)
        EXPECT_EQ('A', out[i]) << "at " << i;

    /* holed.dat: the all-zero stored header = sparse unit: no extents */
    ASSERT_NE(ho, nullptr) << "hole-compressed file must surface";
    EXPECT_TRUE(ho->props()->extents.empty()) << "a hole unit emits no extents (content is zeros)";

    EXPECT_EQ(u64(0), stats.compressed_failed);
}

TEST(NtfsVolume, Lznt1MsXcaDocumentedVector) {
    /* MS-XCA 3.3 example: the 142-byte ANSI string compressed to one
     * 59-byte chunk (header 0xB038 = compressed, 57-byte payload). This
     * pins the decoder against the specification's own vector. */
    static const u8 payload[57] = {
        0x88, 0x46, 0x23, 0x20, 0x00, 0x20, 0x47, 0x20, 0x41, 0x00, 0x10, 0xa2, 0x47, 0x01, 0xa0,
        0x45, 0x20, 0x44, 0x00, 0x08, 0x45, 0x01, 0x50, 0x79, 0x00, 0xc0, 0x45, 0x20, 0x05, 0x24,
        0x13, 0x88, 0x05, 0xb4, 0x02, 0x4a, 0x44, 0xef, 0x03, 0x58, 0x02, 0x8c, 0x09, 0x16, 0x01,
        0x48, 0x45, 0x00, 0xbe, 0x00, 0x9e, 0x00, 0x04, 0x01, 0x18, 0x90, 0x00,
    };
    static const char expected[142] =
        "F# F# G A A G F# E D D E F# F# E E F# F# G A A G F# E D D E F# E D "
        "D E E F# D E F# G F# D E F# G F# E D E A F# F# G A A G F# E D D "
        "E F# E D D";
    static_assert(sizeof(expected) == 142, "MS-XCA vector is 142 bytes incl NUL");
    std::vector<u8> out(142, 0);
    ASSERT_TRUE(compress::decompress_block(compress::kLznt1, payload, sizeof(payload), out.data(),
                                           out.size()));
    EXPECT_EQ(0, std::memcmp(out.data(), expected, sizeof(expected)));
}

/* ---- GAP-S6-4: remnant records reconstruct their $DATA content ----
 *
 * Bench quick_format (runs 40073093997 + 40079065019): the remnant scanner
 * found the old MFT records (10093 rows) but downgraded every one to
 * MetadataOnly - recovery produced 10013 ERR_TRUNCATED and 0/10228
 * hash-true while the og rebuilt full files from the same remnants (51.5%
 * smoke). The records survive a quick format with their $DATA runlists
 * intact; the remnant path must parse them. */

TEST(NtfsVolume, RemnantScanReconstructsDataRunlist) {
    /* a detached record with a non-resident $DATA runlist: the emitted node
     * must carry slice-relative extents and PartialRisk (reconstructed
     * runlist, allocation state unverifiable), not MetadataOnly. */
    Image img;
    {
        RecBuilder r;
        r.header(999, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"survivor.bin", FILE_NAME_WIN32_AND_DOS, 0x20, 8192, 8192);
        r.attr_data_runlist(200, 8, 8192); /* 8 clusters at LCN 200 */
        r.finish();
        std::memcpy(img.sec(10), r.buf.data(), RECORD_SIZE);
        std::memcpy(img.sec(200), "CONTENT-MARKER", 14); /* the data is on disk */
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkStats stats;
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, TOTAL_CLUSTERS, nullptr, nullptr,
                               nullptr, tree, root_lost, stats);
    ASSERT_EQ(rc, OK);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "survivor.bin")
            found = c;
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->origin(), tree::Origin::Lost);
    /* the reconstructed extents: LCN 200 x 8 clusters, slice-relative */
    ASSERT_EQ(found->props()->extents.size(), 1u);
    EXPECT_EQ(found->props()->extents[0].offset, u64(200) * CLUSTER);
    EXPECT_EQ(found->props()->extents[0].length, u64(8) * CLUSTER);
    EXPECT_EQ(found->props()->logical_size, 8192u);
    EXPECT_EQ(found->confidence(), tree::Confidence::PartialRisk);
}

TEST(NtfsVolume, RemnantScanReconstructsResidentData) {
    /* a detached record with a resident $DATA stream: the content travels
     * in the props (the writer emits resident_data before extents). */
    Image img;
    {
        RecBuilder r;
        r.header(998, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"tiny.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 8, 8);
        r.attr_data_resident(u"", 2); /* unnamed resident $DATA */
        r.finish();
        std::memcpy(img.sec(10), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkStats stats;
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, TOTAL_CLUSTERS, nullptr, nullptr,
                               nullptr, tree, root_lost, stats);
    ASSERT_EQ(rc, OK);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "tiny.txt")
            found = c;
    ASSERT_NE(found, nullptr);
    EXPECT_FALSE(found->props()->resident_data.empty())
        << "resident remnant content must travel in the props";
    EXPECT_EQ(found->confidence(), tree::Confidence::PartialRisk);
}

TEST(NtfsVolume, RemnantScanCompressedStreamStaysMetadataOnly) {
    /* GAP-S6-4 honesty boundary: a compressed remnant stream gets NO
     * extents - surfacing raw LZNT1 bytes as content is the
     * silent-corruption class GAP-P1-1 removed from the main walk. The
     * node stays MetadataOnly (registered bounded gap). */
    Image img;
    {
        RecBuilder r;
        r.header(997, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"packed.bin", FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_data_runlist_compressed(150, 4, 4096, 4);
        r.finish();
        std::memcpy(img.sec(10), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkStats stats;
    u32 rc = scan_mft_remnants(&dev, SEC, CLUSTER, RECORD_SIZE, TOTAL_CLUSTERS, nullptr, nullptr,
                               nullptr, tree, root_lost, stats);
    ASSERT_EQ(rc, OK);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "packed.bin")
            found = c;
    ASSERT_NE(found, nullptr);
    EXPECT_TRUE(found->props()->extents.empty())
        << "compressed remnant streams must NOT carry raw compressed extents";
    EXPECT_EQ(found->confidence(), tree::Confidence::MetadataOnly);
}
