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
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/compress/cr_decomp.h"
#include "cr/storage/cr_device.h"
#include "cr/session/cr_records.h"
#include "cr/tree/cr_diskmap.h"
#include "cr/storage/cr_freemap.h"
#include <array>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::ntfs;

namespace {

/* N-039 (diff-to-fix) comment correction: 0x100 is winioctl.h
 * USN_REASON_HARD_LINK_CHANGE (FILE_CREATE is 0x00010000). The old
 * "FILE_CREATE" label re-derived the dead probe's inverted premise -
 * keep this a neutral fixture bit name. The engine filters on
 * USN_REASON_FILE_DELETE alone (AUDIT-NTFS §4 row 5). */
constexpr u32 kFixtureReasonBit = 0x00000100; /* hard-link-change bit:
                                                 non-delete fixture material */

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
    u64 rs_ = RECORD_SIZE; /* record size (S-012 stride pin: parametrizable) */

    explicit RecBuilder(u64 rs = RECORD_SIZE) : buf(size_t(rs), 0), rs_(rs) {}

    void header(u64 record_no, u16 flags) {
        const u64 nsec = rs_ / SEC; /* USA: one slot per sector + the USN */
        auto* rh = reinterpret_cast<RecordHeader*>(buf.data());
        rh->magic = RECORD_MAGIC_FILE;
        rh->update_sequence_offset = 0x30;
        rh->update_sequence_size = u16(nsec + 1);
        rh->sequence = 1;
        rh->link_count = 1;
        /* the USA grows with the sector count (nsec+1 u16 slots); the first
         * attribute must start past it, 8-aligned (0x38 for 1 KiB records,
         * 0x40 for 2 KiB) */
        const u32 aoff = u32((0x30 + (nsec + 1) * 2 + 7) / 8 * 8);
        rh->attribute_offset = u16(aoff);
        rh->flags = flags;
        rh->bytes_in_use = u32(rs_);
        rh->bytes_allocated = u32(rs_);
        rh->base_record = 0;
        rh->record_number = u32(record_no);
        attr_pos = aoff;
        /* USA: slot0 = USN 0x0BB8; slots 1..n = originals */
        for (u64 i = 0; i <= nsec; ++i) {
            const u16 v = i == 0 ? 0x0BB8 : u16(0xAAAA + i);
            buf[size_t(0x30 + i * 2)] = u8(v);
            buf[size_t(0x30 + i * 2 + 1)] = u8(v >> 8);
        }
        /* tails hold the USN (torn state) */
        for (u64 i = 0; i < nsec; ++i) {
            u8* t = buf.data() + i * SEC + SEC - 2;
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

    /* S-001 test surface: $DATA with one fully SPARSE run (all-hole file) */
    void attr_data_runlist_sparse(u64 clusters, u64 real_size) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, 0, true});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_DATA);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 1;
        u16_from_bytes_le_wr(buf, pos + 10, 0x40);
        u16_from_bytes_le_wr(buf, pos + 16, 0);
        u64_from_bytes_le_wr(buf, pos + 24, clusters - 1);
        u16_from_bytes_le_wr(buf, pos + 32, 0x40);
        u32_from_bytes_le_wr(buf, pos + 36, 0);
        u64_from_bytes_le_wr(buf, pos + 40, clusters * CLUSTER);
        u64_from_bytes_le_wr(buf, pos + 48, real_size);
        u64_from_bytes_le_wr(buf, pos + 56, real_size);
        std::memcpy(buf.data() + pos + 0x40, rle.data(), rlen);
        attr_pos += alen;
    }

    /* S-001 test surface: $INDEX_ALLOCATION ($I30) runlist for a directory */
    void attr_index_runlist(u64 lcn, u64 clusters) {
        u32 pos = attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
        std::vector<u8> rle = rl.encode();
        u32 rlen = u32(rle.size());
        u32 alen = (0x40 + rlen + 7) / 8 * 8;
        u32_from_bytes_le_wr(buf, pos + 0, ATTR_TYPE_INDEX_ALLOCATION);
        u32_from_bytes_le_wr(buf, pos + 4, alen);
        buf[pos + 8] = 1;
        u16_from_bytes_le_wr(buf, pos + 10, 0x40);
        u16_from_bytes_le_wr(buf, pos + 16, 0);
        u64_from_bytes_le_wr(buf, pos + 24, clusters - 1);
        u16_from_bytes_le_wr(buf, pos + 32, 0x40);
        u32_from_bytes_le_wr(buf, pos + 36, 0);
        u64_from_bytes_le_wr(buf, pos + 40, clusters * CLUSTER);
        u64_from_bytes_le_wr(buf, pos + 48, clusters * CLUSTER);
        u64_from_bytes_le_wr(buf, pos + 56, clusters * CLUSTER);
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
        /* re-assert the USA tails: long resident payloads can cross sector
         * boundaries and clobber them (the on-disk state always carries the
         * USN at every sector tail) */
        const u64 nsec = rs_ / SEC;
        for (u64 i = 0; i < nsec; ++i) {
            u8* t = buf.data() + i * SEC + SEC - 2;
            t[0] = 0xB8;
            t[1] = 0x0B;
        }
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
        std::memcpy(img.sec(MFT_LCN + 36), r.buf.data(), RECORD_SIZE); /* rec 18 */
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
    /* record 8: dirdel (DELETED DIRECTORY, $I30 at LCN 220..223 - the
     * same fully-reused cluster range as overwrit.dat: folders must NEVER
     * take the destroy path (og gate `(flags & 0x10) == 0`). Record 8 is
     * an unused slot inside the primary $DATA run.) */
    {
        RecBuilder r;
        r.header(8, RECORD_IS_DIRECTORY); /* dir, NOT in use */
        r.attr_si(0x30);
        r.attr_file_name(16, u"dirdel", FILE_NAME_WIN32_AND_DOS, 0x30, 4096, 4096);
        r.attr_index_runlist(220, 4);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 16), r.buf.data(), RECORD_SIZE);
    }
    /* record 9: holes.dat (DELETED, ALL-SPARSE runlist - a hole-only file;
     * the og's viability iterator skips sparse entries so it can never
     * grade Overwritten; a $Bitmap-inflated ratio must not destroy it) */
    {
        RecBuilder r;
        r.header(9, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"holes.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_data_runlist_sparse(4, 4096);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 18), r.buf.data(), RECORD_SIZE);
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
     * canonical (first-emitted) link, not the last. Two modes (M-007):
     * parity default = ONE link emitted (the first-seen namespace winner
     * "hl@root.txt"), the ADS attaches to it; the opt-in fan-out = both
     * links emitted, ADS still under the FIRST (parent-5) link. */
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

    /* ---- parity mode (the default): one link, ADS under it ---- */
    {
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"a");
        auto* rd = tree.add_root(u"d");
        auto* rl = tree.add_root(u"l");
        WalkerFlags wf;
        WalkStats stats;
        ASSERT_EQ(
            walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
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
        EXPECT_EQ(at_folder, nullptr) << "parity mode: one node per record (M-007)";
        ASSERT_NE(ads, nullptr);
        EXPECT_EQ(ads->name_utf8(), "hl@root.txt:stream");
        bool under_root_link = false;
        for (tree::FileNode* c = at_root->first_child(); c; c = c->next_sibling())
            if (c == ads)
                under_root_link = true;
        EXPECT_TRUE(under_root_link);
    }

    /* ---- opt-in fan-out: both links, ADS under the FIRST ---- */
    {
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"a");
        auto* rd = tree.add_root(u"d");
        auto* rl = tree.add_root(u"l");
        WalkerFlags wf;
        wf.single_link_per_record = false; /* the opt-in superset */
        WalkStats stats;
        ASSERT_EQ(
            walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
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
        off += usn(off, u"keptfile.txt", kFixtureReasonBit);
        off += usn(off, u"gonefile.txt", kFixtureReasonBit | USN_REASON_FILE_DELETE);
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
            j32(u32(off + 40), kFixtureReasonBit | USN_REASON_FILE_DELETE);
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
                               nullptr, tree, root_lost, stats);
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
    WalkStats stats;
    u32 rc = walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, root_active,
                      root_deleted, root_lost, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    /* meta records skipped by '$' name; SYSTEM-attributed: none in fixture */
    EXPECT_EQ(stats.meta_skipped, 7u);          /* 0,1,2,3,4,6,7 ($ named) */
    EXPECT_EQ(stats.destroyed_overwritten, 0u); /* S-012: pass 1 never destroys -
                                                    the rule lives in the dump pass */
    EXPECT_EQ(stats.damaged, 0u);
    EXPECT_EQ(stats.orphans, 0u);
    /* M-007: parity mode default - no fan-out, no counter (the og's scan
     * has no hardlink counter; the fan-out is the opt-in superset) */
    EXPECT_EQ(stats.hardlinks, 0u);
    EXPECT_EQ(stats.ads, 1u);
    EXPECT_GE(stats.emitted, 10u); /* overwrit.dat now a member (S-012) */

    /* locate nodes by name */
    tree::FileNode *rootdot = nullptr, *folder1 = nullptr, *readme = nullptr, *deleted = nullptr,
                   *partial = nullptr, *hard16 = nullptr, *hard5 = nullptr, *dirdel = nullptr,
                   *holes = nullptr, *adsnode = nullptr, *adschild = nullptr, *over = nullptr;
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
        else if (nm == "dirdel")
            dirdel = n;
        else if (nm == "holes.dat")
            holes = n;
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
        else if (nm == "overwrit.dat")
            over = n;
    }
    ASSERT_NE(rootdot, nullptr);
    ASSERT_NE(folder1, nullptr);
    ASSERT_NE(readme, nullptr);
    ASSERT_NE(deleted, nullptr);
    /* S-012: the og's pass 1 emits EVERY record - fully-overwritten deleted
     * files included, ungraded and undestroyed (the destroy rule lives in
     * the GUID-dump pass 2; destroy-in-walk is the opt-in honest mode) */
    ASSERT_NE(over, nullptr);
    EXPECT_EQ(over->deleted_state(), tree::DeletedState::PartiallyOverwritten);
    ASSERT_NE(dirdel, nullptr);
    ASSERT_NE(holes, nullptr);
    ASSERT_NE(partial, nullptr);
    ASSERT_NE(hard16, nullptr);
    /* M-007 parity default: the second link is dropped (see below) */
    EXPECT_EQ(hard5, nullptr) << "the og emits one node per record (M-007)";
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
    /* M-007: parity mode (default) emits ONE node per record - the
     * namespace-best name (first-seen wins the WIN32/WIN32 tie here) under
     * ITS parent; the second link is NOT emitted (the og drops it -
     * FUN_1800448b0 fetches two, emits one). */
    EXPECT_EQ(hard5, nullptr) << "parity mode drops the second link";
    EXPECT_EQ(adschild->parent(), adsnode);

    /* the emitted link carries the shared record identity + extents */
    EXPECT_EQ(hard16->props()->extents.size(), 1u);
    EXPECT_EQ(hard16->props()->extents[0].offset, 240 * CLUSTER);
    EXPECT_STREQ(hard16->name_utf8().c_str(), "hard.txt");

    /* grading */
    EXPECT_EQ(readme->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(readme->deleted_state(), tree::DeletedState::Active);
    EXPECT_EQ(deleted->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(deleted->deleted_state(), tree::DeletedState::Deleted);
    /* S-012: membership restored, grading kept (the in-walk Confidence is
     * the documented D15 superset - the og grades downstream) */
    EXPECT_EQ(over->confidence(), tree::Confidence::Overwritten);
    /* folders are NEVER destroyed (og `(flags & 0x10) == 0` gate) even when
     * the i30 probe finds every cluster re-used - they keep the graded node
     * (the folder bitmap grading itself is pre-existing core behavior) */
    EXPECT_EQ(dirdel->confidence(), tree::Confidence::Overwritten);
    EXPECT_NE(dirdel->deleted_state(), tree::DeletedState::Active);
    EXPECT_EQ(holes->confidence(), tree::Confidence::Excellent); /* sparse runs never
        count as re-used (og iterator skips -1 entries); a hole-only file
        cannot be Overwritten */
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

/* ------------------------------------------------------------------ */
/* M-007: hard-link emission parity (one node per record; opt-in fan-out) */
/* ------------------------------------------------------------------ */

namespace {

/* record 26: a 3-link file - POSIX under root(5), Win32 under folder1(16),
 * DOS under dirdel(8). Namespace ranking: POSIX wins. */
void add_three_link_record(Image& img) {
    RecBuilder r;
    r.header(26, RECORD_IN_USE);
    r.attr_si(0x20);
    r.attr_file_name(5, u"tri.posix", FILE_NAME_POSIX, 0x20, 512, 512);
    r.attr_file_name(16, u"tri.txt", FILE_NAME_WIN32, 0x20, 512, 512);
    r.attr_file_name(8, u"TRI.TXT", FILE_NAME_DOS, 0x20, 512, 512);
    r.attr_data_runlist(211, 1, 512);
    r.finish();
    std::memcpy(img.sec(MFT_LCN + 52), r.buf.data(), RECORD_SIZE);
}

std::vector<tree::FileNode*> nodes_named(tree::ResultTree& t, const char* want) {
    std::vector<tree::FileNode*> out;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == want)
                out.push_back(c);
            collect(c);
        }
    };
    for (auto* r : t.roots())
        collect(r);
    return out;
}

} // namespace

TEST(NtfsVolume, HardLinkSingleLinkMatchesOg) {
    /* M-007 parity mode (the default): the og emits ONE node per record -
     * the namespace-best $FILE_NAME under ITS parent. The 3-link record
     * yields the POSIX node under root only; the Win32/DOS links are
     * dropped exactly as the og drops them (FUN_1800448b0 fetches two,
     * emits one). The hardlinks counter stays 0 (no fan-out happened). */
    Image img;
    build_image(img);
    add_three_link_record(img);
    MemDevice dev(img.raw(), img.geom());

    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);

    tree::ResultTree tree;
    auto* root_active = tree.add_root(u"Found (NTFS)");
    auto* root_deleted = tree.add_root(u"Deleted");
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkerFlags wf; /* defaults: single_link_per_record = true */
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, root_active,
                       root_deleted, root_lost, stats),
              u32(OK));

    /* ONE tri.* node: the POSIX name (rank 4 beats WIN32 2 and DOS 1) */
    const auto posix = nodes_named(tree, "tri.posix");
    ASSERT_EQ(posix.size(), 1u) << "parity mode must emit the POSIX link once";
    EXPECT_EQ(posix[0]->parent()->name_utf8(), ".") << "under the namespace-best parent (root)";
    EXPECT_EQ(nodes_named(tree, "tri.txt").size(), 0u) << "the Win32 link is dropped (og parity)";
    EXPECT_EQ(nodes_named(tree, "TRI.TXT").size(), 0u) << "the DOS link is dropped (og parity)";
    EXPECT_EQ(stats.hardlinks, 0u) << "no fan-out: the counter never increments in parity mode";
    /* the existing 2-link record 21 also collapses to one node */
    EXPECT_EQ(nodes_named(tree, "hard@root.txt").size(), 0u);
    EXPECT_EQ(nodes_named(tree, "hard.txt").size(), 1u);
}

TEST(NtfsVolume, HardLinkFanOutOptIn) {
    /* M-007 opt-in superset: single_link_per_record=false emits one node
     * per distinct parent, each with ITS OWN link name - the pre-M-007
     * behavior, now explicit. Regression guard for the flag's off path. */
    Image img;
    build_image(img);
    add_three_link_record(img);
    MemDevice dev(img.raw(), img.geom());

    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);

    tree::ResultTree tree;
    auto* root_active = tree.add_root(u"Found (NTFS)");
    auto* root_deleted = tree.add_root(u"Deleted");
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    wf.single_link_per_record = false; /* the opt-in fan-out */
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, root_active,
                       root_deleted, root_lost, stats),
              u32(OK));

    EXPECT_EQ(nodes_named(tree, "tri.posix").size(), 1u);
    EXPECT_EQ(nodes_named(tree, "tri.txt").size(), 1u);
    EXPECT_EQ(nodes_named(tree, "TRI.TXT").size(), 1u);
    EXPECT_EQ(nodes_named(tree, "hard.txt").size(), 1u);
    EXPECT_EQ(nodes_named(tree, "hard@root.txt").size(), 1u);
    EXPECT_EQ(stats.hardlinks, 3u) << "record 26 adds 2 (3 links); record 21 adds 1 (2 links)";
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
                               nullptr, tree, root_lost, stats);
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

/* ---- S-001: the og destroy rule --------------------------------------- */

TEST(NtfsVolume, OverwrittenHonestModeKeepsAndGrades) {
    /* WalkerFlags::destroy_fully_overwritten=false keeps the previous
     * keep-and-grade visibility (Confidence::Overwritten +
     * DeletedState::PartiallyOverwritten) for honest-recovery scans - the
     * reviewed-diversion mode of the S-001 fix. */
    Image img;
    build_image(img);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found (NTFS)");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost Files");
    WalkerFlags wf;
    wf.destroy_fully_overwritten = false;
    WalkStats stats;
    ASSERT_EQ(
        OK, walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_EQ(0u, stats.destroyed_overwritten);

    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    tree::FileNode* over = nullptr;
    for (tree::FileNode* n : all)
        if (n->name_utf8() == "overwrit.dat")
            over = n;
    ASSERT_NE(over, nullptr) << "honest mode must keep the graded node";
    EXPECT_EQ(over->confidence(), tree::Confidence::Overwritten);
    EXPECT_EQ(over->deleted_state(), tree::DeletedState::PartiallyOverwritten);
}

/* ---- M-001: record-size degradation ladder (og never rejects) ---------- */

namespace {

/* patch the DBR clusters-per-record byte (signed i8 at +0x40) */
void patch_dbr_cpr(Image& img, i8 cpr) {
    img.sec(0)[0x40] = u8(cpr);
}

} // namespace

TEST(NtfsVolume, BootNoPow2RecordSizeAcceptedAtDecodedValue) {
    /* N-011b (diff-to-fix; supersedes the M-001-era clamp semantics at
     * VOLUME OPEN): the og's LoadNtfsVolume (:19292-19344) has NO pow2 /
     * divisibility check and NO 0x400 clamp - cpr = 3 decodes to
     * 3 x spc x bps = 1536 bytes and the volume OPENS at that stride
     * (the M-001 0x200-fallback/0x400-clamp ladder belongs to the WALK
     * function FUN_18001acc0, not to open). The old core rejected the
     * non-pow2 record size outright. */
    Image img;
    build_image(img);
    patch_dbr_cpr(img, 3);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    EXPECT_EQ(vol->params().record_size, 1536u); /* decoded value, unclamped */
}

TEST(NtfsVolume, BootRecordSizeZeroRejected) {
    /* N-011b: cpr == 0 or a 0-result decodes to a zero record size and
     * the og REJECTS the volume ("It is not a NTFS volume!",
     * LoadNtfsVolume :19322/:19331). The M-001-era fallback-to-0x400 at
     * OPEN was mis-scoped (its evidence was the walk function). */
    Image img;
    build_image(img);
    patch_dbr_cpr(img, 0);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    EXPECT_EQ(nullptr, vol) << "cpr == 0 must reject the volume (og parity)";
}

TEST(NtfsVolume, BootNegativeExponentOverflowRejected) {
    /* N-011b: cpr = -30 decodes past the P1-4 shift-UB guard (exponent
     * cap 20 -> 1 MiB, registered core hardening - the og would compute a
     * 1-GiB record size); the 0-result rejects per the og's 0-result
     * rule. Sub-sector 0-results (e.g. cpr = -1 at bps 512: 2 bytes)
     * reject the same way. */
    Image img;
    build_image(img);
    patch_dbr_cpr(img, -30);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    EXPECT_EQ(nullptr, vol) << "0-result record size must reject";
    patch_dbr_cpr(img, -1); /* 2 bytes < 1 sector */
    MemDevice dev2(img.raw(), img.geom());
    vol = ntfs::NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev2)), &err);
    EXPECT_EQ(nullptr, vol) << "sub-sector record size must reject";
}

TEST(NtfsVolume, BootUsableLargeRecordSizeKept) {
    /* cpr = -12 -> 4096: pow2 and sector-divisible - the og keeps the raw
     * value (buf = max(0x400, rs)); the ladder must not squash it */
    Image img;
    build_image(img);
    patch_dbr_cpr(img, -12);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    EXPECT_EQ(vol->params().record_size, 4096u);
}

TEST(NtfsVolume, RemnantReachableUnderTornDbr) {
    /* M-001's availability claim end-to-end: a torn-cpr DBR (the remnant
     * scan's own trigger scenario) must open the volume AND the remnant
     * sweep must run - the pre-fix ERR_BAD_PARAMETER made both unreachable.
     * Plant a shifted-header remnant the way ShiftedHeaderSalvage does. */
    Image img;
    build_image(img);
    patch_dbr_cpr(img, 3); /* torn clusters_per_record */
    {
        RecBuilder r;
        r.header(999, 0);
        r.attr_si(0x20);
        r.attr_file_name(16, u"tornrem.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 100, 100);
        r.finish();
        std::vector<u8> window(0x58 + RECORD_SIZE, 0);
        std::memcpy(window.data(), "FILE", 4);
        window[0x14] = 0x00;
        window[0x15] = 0xFF; /* broken head: attribute_offset out of range */
        std::memcpy(window.data() + 0x58, r.buf.data(), RECORD_SIZE);
        std::memcpy(img.sec(150), window.data(), window.size()); /* free space
            (the fixture image is 256 sectors; 150 sits between the bitmap
            region and the data zone at LCN 200) */
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "torn-DBR volume refused: " << err_text(err);

    tree::ResultTree tree;
    auto* root_lost = tree.add_root(u"Lost Files");
    WalkStats stats;
    u32 rc = scan_mft_remnants(&dev, vol->params().sector_bytes, vol->params().cluster_bytes,
                               vol->params().record_size, vol->params().total_clusters, nullptr,
                               nullptr, nullptr, tree, root_lost, stats);
    ASSERT_EQ(rc, OK);
    tree::FileNode* found = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "tornrem.txt")
            found = c;
    ASSERT_NE(found, nullptr) << "remnant sweep never ran / remnant lost";
}

/* ---- N-001: mirror service derives from the runlist span ---------------- */

namespace {

void put64le_at(u8* p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = u8(v >> (8 * i));
}

/* patch record 1 ($MFTMirr)'s $DATA runlist in the MFT to a new single run */
void patch_mirror_runlist(Image& img, u64 lcn, u64 clusters) {
    u8* rec = img.sec(MFT_LCN + 2); /* record 1 */
    /* $MFTMirr's attrs: $FILE_NAME ("$MFTMirr", 8 chars) then $DATA - the
     * fixture builder computes the same layout every time; re-encode the
     * runlist at the $DATA attribute's runlist offset (attr+0x40) */
    u32 name_alen = (0x18 + 66 + 2 * 8 + 7) / 8 * 8; /* "$MFTMirr" = 8 chars */
    u8* attr = rec + 0x38 + name_alen;               /* $FILE_NAME precedes $DATA */
    RunList rl;
    rl.runs().push_back(DataRun{0, clusters, 0, lcn, false});
    std::vector<u8> rle = rl.encode();
    /* alen is (0x40 + 4 + 7)/8*8 = 72 -> the runlist area is 8 bytes; NEVER
     * wipe past it (the attribute terminator sits at +0x48) */
    std::memset(attr + 0x40, 0, 8);
    std::memcpy(attr + 0x40, rle.data(), rle.size());
    put64le_at(attr + 0x18, clusters - 1);       /* last_vcn */
    put64le_at(attr + 0x28, clusters * CLUSTER); /* allocated */
}

/* patch record 0 ($MFT)'s $DATA runlist to [data 4, sparse 60]: records at
 * VCN >= 4 (record_no >= 2) fall to the mirror path */
void patch_mft_runlist_with_hole(Image& img) {
    u8* rec = img.sec(MFT_LCN); /* record 0 */
    /* $MFT's attrs per the fixture: $FILE_NAME "$MFT" (4 chars) precedes
     * the $DATA runlist attribute */
    u32 name_alen = (0x18 + 66 + 2 * 4 + 7) / 8 * 8; /* "$MFT" = 4 chars */
    u8* attr = rec + 0x38 + name_alen;
    RunList rl;
    rl.runs().push_back(DataRun{0, 4, 0, 4, false});
    rl.runs().push_back(DataRun{4, 60, 0, 0, true});
    std::vector<u8> rle = rl.encode();
    ASSERT_LE(rle.size(), 8u); /* the runlist area (alen 72 - 0x40) */
    std::memset(attr + 0x40, 0, 8);
    std::memcpy(attr + 0x40, rle.data(), rle.size());
    put64le_at(attr + 0x18, 63);           /* last_vcn */
    put64le_at(attr + 0x28, 64 * CLUSTER); /* allocated */
}

} // namespace

TEST(NtfsVolume, MirrorCoversRunlistSize) {
    /* N-001: an 8-record mirror runlist must serve record 2 through the
     * fallback when the primary $MFT has a hole there; the pre-fix
     * hardcoded `record_no < 4` refused mirrors > 4 records (og
     * ReadFileRecord gates on the runlist-derived this+0x70, :17460). */
    Image img;
    build_image(img);
    patch_mirror_runlist(img, MIRR_LCN, 16); /* 16 clusters = 8 records @1024 */
    /* the mirror loader reads the MIRROR-REGION copy of record 1 (mirror
     * vcn 2 = sector 70), not the MFT original - write the patched record
     * there (the fixture's mirror slots overlap the MFT tail; slot 1 held
     * MFT record 32, which this test never reads). The fixture also leaves
     * record 2's mirror slot as a build-order zero placeholder - fill it
     * with the real record so the served copy carries a 'FILE' header. */
    std::memcpy(img.sec(MIRR_LCN + 2), img.sec(MFT_LCN + 2), RECORD_SIZE);
    std::memcpy(img.sec(MIRR_LCN + 4), img.sec(MFT_LCN + 4), RECORD_SIZE);
    /* record 5's mirror slot (5*1024 bytes in = sector 78, inside the
     * 16-cluster mirror span but past the fixture's 4-record region):
     * THE discriminating record - the pre-fix hardcoded `record_no < 4`
     * gate refused it even though the runlist covers 8 records */
    std::memcpy(img.sec(MIRR_LCN + 10), img.sec(MFT_LCN + 4), RECORD_SIZE);
    patch_mft_runlist_with_hole(img);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);

    std::vector<u8> rec;
    ASSERT_EQ(OK, vol->read_record(2, rec)) << "record 2 not served from the span-sized mirror";
    ASSERT_GE(rec.size(), 4u);
    EXPECT_EQ(0, std::memcmp(rec.data(), "FILE", 4)) << "mirror copy mismatch";
    /* THE N-001 assertion: record 5 is past the old hardcoded 4-record
     * window but inside the runlist-derived span - must be served (the
     * pre-fix code returned ERR_RANGE_OUT_OF_BOUNDS here) */
    ASSERT_EQ(OK, vol->read_record(5, rec)) << "runlist span ignored: record 5 refused";
    EXPECT_EQ(0, std::memcmp(rec.data(), "FILE", 4)) << "record 5 mirror copy mismatch";
    /* the capacity gate still bounds service: record 9 is past the 8-record
     * span (and the primary hole) - refused, not fabricated */
    EXPECT_EQ(ERR_RANGE_OUT_OF_BOUNDS, vol->read_record(9, rec));
}

TEST(NtfsVolume, MirrorResidentDataRejected) {
    /* og parity of the abort path (InitMFTMirror "$MFTMirror::$DATA is
     * resident!" :17378-17382): a resident $DATA on the mirror record
     * means the mirror is INVALID - never decode resident content as a
     * runlist. The volume still opens (mirror is best-effort). */
    Image img;
    build_image(img);
    {
        u8* rec = img.sec(MFT_LCN + 2); /* record 1 = $MFTMirr */
        u32 name_alen = (0x18 + 66 + 2 * 8 + 7) / 8 * 8;
        u8* attr = rec + 0x38 + name_alen; /* the $DATA attribute */
        attr[8] = 0;                       /* nonresident flag -> resident */
    }
    patch_mft_runlist_with_hole(img); /* primary hole forces the fallback */
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "volume must open regardless of mirror state";
    std::vector<u8> rec;
    EXPECT_EQ(ERR_RANGE_OUT_OF_BOUNDS, vol->read_record(2, rec))
        << "resident mirror $DATA must never serve a fabricated mapping";
}

TEST(NtfsVolume, UserFileNamedDollarNotHidden) {
    /* I-001 (diff-to-fix, INFORMATIONAL): the og's blanket name[0]=='$'
     * skip (FUN_18001acc0 :21186) hides user files named '$...'; our
     * known-20 metafile filter surfaces them (a reviewed recovery-positive
     * superset). Pin it so nobody "fixes" parity by restoring the blanket
     * skip. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(23, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(16, u"$notes.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 8, 8);
        r.attr_data_runlist(240, 1, 8);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 46), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    auto* rd = tree.add_root(u"D");
    auto* rl = tree.add_root(u"L");
    WalkerFlags wf;
    WalkStats st;
    ASSERT_EQ(OK,
              walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, st));
    bool found = false;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "$notes.txt")
                found = true;
            find(c);
        }
    };
    for (auto* r : tree.roots())
        find(r);
    EXPECT_TRUE(found) << "user file named '$...' must not be hidden by the "
                          "known-metafile filter";
}

/* ---- S-012: two-pass NTFS - GUID-dump walk + size-0 rescue --------------- */

namespace {

/* Dump fixture (512B sectors/clusters, 1024B records, 256 clusters):
 *   LCN 4..31    $MFT (28 clusters -> 14 records: 0..13)
 *   LCN 40..63   $MFTMirr (24 clusters -> 12-record mirror capacity)
 *   LCN 76..79   $Bitmap
 *   record 10    the GUID-named MFT dump file (live, $DATA LCN 100..103)
 *   LCN 100..107 the dump content = four stale MFT records:
 *                  zombie1.dat  (deleted, runs 150..151 UNCLAIMED -> emitted)
 *                  zombie2.dat  (deleted, runs 100..101 claimed by the dump
 *                                file itself -> destroyed, all-run-starts)
 *                  tiny.dat     (deleted, size 500 <= 0x3ff -> destroyed)
 *                  zombiedir    (deleted FOLDER, $I30 100..101 claimed ->
 *                                survives: folders never destroyed)
 *   record 11    ghost0.dat (deleted, size 0, no $DATA in the primary);
 *                its MIRROR copy at sector 62 keeps a resident $DATA -> the
 *                walk-time mirror rescue recovers content+size
 *   record 12    twindrive.dat (deleted, size 0, no mirror coverage) ->
 *                post-walk same-name merge adopts record 13's extents+size
 *   record 13    twindrive.dat (live, $DATA LCN 190, 600 bytes) */
constexpr u64 D_MFT_LCN = 4;
constexpr u64 D_MFT_CLUSTERS = 28; /* records 0..13 */
constexpr u64 D_MIRR_LCN = 40;
constexpr u64 D_MIRR_CLUSTERS = 24;
constexpr u64 D_BITMAP_LCN = 76;
constexpr u64 D_DUMP_LCN = 100;

void build_dump_image(Image& img) {
    /* boot sector */
    {
        RecBuilder b;
        u8* bs = img.sec(0);
        BootSector* boot = reinterpret_cast<BootSector*>(bs);
        const char oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
        std::memcpy(boot->oem, oem, 8);
        boot->bytes_per_sector = SEC;
        boot->sectors_per_cluster = 1;
        boot->media_descriptor = 0xF8;
        boot->total_sectors = NSECTORS;
        boot->mft_lcn = D_MFT_LCN;
        boot->mftmirr_lcn = D_MIRR_LCN;
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
        r.attr_file_name(5, u"$MFT", FILE_NAME_WIN32, 0x06, D_MFT_CLUSTERS * CLUSTER,
                         D_MFT_CLUSTERS * CLUSTER);
        r.attr_data_runlist(D_MFT_LCN, D_MFT_CLUSTERS, D_MFT_CLUSTERS * CLUSTER);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN), r.buf.data(), RECORD_SIZE);
    }
    /* record 1: $MFTMirr (24 clusters -> 12-record mirror capacity) */
    {
        RecBuilder r;
        r.header(1, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFTMirr", FILE_NAME_WIN32, 0x06, D_MIRR_CLUSTERS * CLUSTER,
                         D_MIRR_CLUSTERS * CLUSTER);
        r.attr_data_runlist(D_MIRR_LCN, D_MIRR_CLUSTERS, D_MIRR_CLUSTERS * CLUSTER);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + 2), r.buf.data(), RECORD_SIZE);
        /* the mirror's own record-1 copy (bootstrap reads it for the mapping) */
        std::memcpy(img.sec(D_MIRR_LCN + 2), r.buf.data(), RECORD_SIZE);
        std::memcpy(img.sec(D_MIRR_LCN), img.sec(D_MFT_LCN), RECORD_SIZE); /* rec0 copy */
    }
    /* records 2,3,4,6,7: meta */
    for (u64 rn : {2u, 3u, 4u, 6u, 7u}) {
        RecBuilder r;
        r.header(rn, RECORD_IN_USE);
        const char16_t* nm = rn == 6 ? u"$Bitmap" : (rn == 2 ? u"$LogFile" : u"$Volume");
        r.attr_file_name(5, nm, FILE_NAME_WIN32, 0x06, 4 * CLUSTER, 4 * CLUSTER);
        if (rn == 6)
            r.attr_data_runlist(D_BITMAP_LCN, 4, 4 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + rn * 2), r.buf.data(), RECORD_SIZE);
    }
    /* record 5: root "." */
    {
        RecBuilder r;
        r.header(5, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(5, u".", FILE_NAME_POSIX, 0x00, 0, 0);
        r.attr_index_root();
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + 10), r.buf.data(), RECORD_SIZE);
    }
    /* record 10: the GUID-named MFT dump file (live) */
    {
        RecBuilder r;
        r.header(10, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5,
                         u"{11111111-2222-3333-4444-555555555555}"
                         u"{3808876b-c176-4e48-b7ae-04046e6cc752}",
                         FILE_NAME_WIN32, 0x20, 4096, 4096);
        r.attr_data_runlist(D_DUMP_LCN, 8, 4096);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + 20), r.buf.data(), RECORD_SIZE);
    }
    /* the dump content: four stale records at LCN 100..103 */
    {
        RecBuilder r;
        r.header(999, 0); /* record# irrelevant inside dumps (pseudo# used) */
        r.attr_si(0x20);
        r.attr_file_name(5, u"zombie1.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(150, 2, 2048); /* unclaimed run starts */
        r.finish();
        std::memcpy(img.sec(D_DUMP_LCN), r.buf.data(), RECORD_SIZE);
    }
    {
        RecBuilder r;
        r.header(998, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"zombie2.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        r.attr_data_runlist(D_DUMP_LCN, 2, 2048); /* starts INSIDE the dump file's
                                                     claimed extent */
        r.finish();
        std::memcpy(img.sec(D_DUMP_LCN + 2), r.buf.data(), RECORD_SIZE);
    }
    {
        RecBuilder r;
        r.header(997, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"tiny.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 500, 500);
        r.attr_data_runlist(160, 1, 500); /* size <= 0x3ff: destroyed */
        r.finish();
        std::memcpy(img.sec(D_DUMP_LCN + 4), r.buf.data(), RECORD_SIZE);
    }
    {
        RecBuilder r;
        r.header(996, RECORD_IS_DIRECTORY); /* deleted folder */
        r.attr_si(0x30);
        r.attr_file_name(5, u"zombiedir", FILE_NAME_WIN32_AND_DOS, 0x30, 2048, 2048);
        r.attr_index_runlist(D_DUMP_LCN, 1); /* claimed $I30 - folders survive */
        r.finish();
        std::memcpy(img.sec(D_DUMP_LCN + 6), r.buf.data(), RECORD_SIZE);
    }
    /* record 11: ghost0.dat - deleted, size 0, NO $DATA in the primary */
    {
        RecBuilder r;
        r.header(11, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"ghost0.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 0, 0);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + 22), r.buf.data(), RECORD_SIZE);
        /* the MIRROR copy of record 11 (sector 62) keeps the resident $DATA */
        RecBuilder m;
        m.header(11, 0);
        m.attr_si(0x20);
        m.attr_file_name(5, u"ghost0.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 20, 20);
        m.attr_data_resident(u"", 5); /* 20 bytes of 0xC0FFEE00.. payload */
        m.finish();
        std::memcpy(img.sec(D_MIRR_LCN + 22), m.buf.data(), RECORD_SIZE);
    }
    /* record 12: twindrive.dat - deleted, size 0, beyond mirror capacity */
    {
        RecBuilder r;
        r.header(12, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"twindrive.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 0, 0);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + 24), r.buf.data(), RECORD_SIZE);
    }
    /* record 13: twindrive.dat - live content the merge adopts */
    {
        RecBuilder r;
        r.header(13, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"twindrive.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 600, 600);
        r.attr_data_runlist(190, 1, 600);
        r.finish();
        std::memcpy(img.sec(D_MFT_LCN + 26), r.buf.data(), RECORD_SIZE);
    }
    /* $Bitmap: MFT 4..27, mirror 40..63, bitmap 76..79, dump 100..103, 190 */
    {
        std::vector<u8> bm(4 * CLUSTER, 0);
        auto setbit = [&bm](u64 c) { bm[c / 8] |= u8(1u << (c % 8)); };
        for (u64 c = D_MFT_LCN; c < D_MFT_LCN + D_MFT_CLUSTERS; ++c)
            setbit(c);
        for (u64 c = D_MIRR_LCN; c < D_MIRR_LCN + D_MIRR_CLUSTERS; ++c)
            setbit(c);
        for (u64 c = D_BITMAP_LCN; c < D_BITMAP_LCN + 4; ++c)
            setbit(c);
        for (u64 c = D_DUMP_LCN; c < D_DUMP_LCN + 8; ++c)
            setbit(c);
        setbit(190);
        std::memcpy(img.sec(D_BITMAP_LCN), bm.data(), bm.size());
    }
}

struct DumpWalkResult {
    std::unique_ptr<NtfsVolume> vol;
    std::unique_ptr<VolumeBitmap> bm;
    tree::ResultTree tree;
    tree::FullNode *ra = nullptr, *rd = nullptr, *rl = nullptr;
    WalkStats stats;
    std::vector<tree::FileNode*> all;
};

void walk_dump_image(Image& img, DumpWalkResult& w) {
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    w.vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(w.vol, nullptr) << "open err " << err_text(err);
    ASSERT_EQ(VolumeBitmap::load(w.vol.get(), w.bm), OK);
    w.ra = w.tree.add_root(u"Found");
    w.rd = w.tree.add_root(u"Deleted");
    w.rl = w.tree.add_root(u"Lost");
    WalkerFlags wf;
    ASSERT_EQ(OK, walk_mft(w.vol.get(), w.bm.get(), wf, nullptr, nullptr, nullptr, w.tree, w.ra,
                           w.rd, w.rl, w.stats));
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            w.all.push_back(c);
            collect(c);
        }
    };
    for (auto* r : w.tree.roots())
        collect(r);
}

tree::FileNode* find_node(const std::vector<tree::FileNode*>& all, const char* name) {
    for (tree::FileNode* n : all)
        if (n->name_utf8() == name)
            return n;
    return nullptr;
}

} // namespace

TEST(NtfsVolume, MainWalkKeepsOverwrittenDeleted) {
    /* S-012 step 1: og pass 1 emits EVERY live-MFT record - a fully-
     * overwritten deleted file stays in the result set (graded, never
     * destroyed in the walk). (EndToEndWalk pins the same rule on the base
     * fixture; this is the named pin for the S-012 default flip.) */
    Image img;
    build_image(img);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    auto* rd = tree.add_root(u"D");
    auto* rl = tree.add_root(u"L");
    WalkerFlags wf; /* defaults: S-012 og-parity profile */
    WalkStats stats;
    ASSERT_EQ(
        OK, walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    EXPECT_EQ(0u, stats.destroyed_overwritten);
    tree::FileNode* over = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "overwrit.dat")
                over = c;
            find(c);
        }
    };
    for (auto* r : tree.roots())
        find(r);
    ASSERT_NE(over, nullptr) << "fully-overwritten deleted file must stay in the "
                                "result set (og pass-1 membership)";
    EXPECT_EQ(over->confidence(), tree::Confidence::Overwritten);
    EXPECT_EQ(over->deleted_state(), tree::DeletedState::PartiallyOverwritten);
}

TEST(NtfsVolume, GuidDumpPassRecoversReusedRecord) {
    /* S-012 step 2: a stale MFT record inside a `{GUID}{3808876b-...}`-
     * named dump file is recovered - the record the live MFT lost. */
    Image img;
    build_dump_image(img);
    DumpWalkResult w;
    ASSERT_NO_FATAL_FAILURE(walk_dump_image(img, w));
    EXPECT_EQ(1u, w.stats.guid_dump_files);
    EXPECT_GE(w.stats.guid_dump_records, 2u); /* zombie1.dat + zombiedir */
    tree::FileNode* z1 = find_node(w.all, "zombie1.dat");
    ASSERT_NE(z1, nullptr) << "dump-recovered record must be emitted";
    EXPECT_EQ(z1->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(z1->props()->logical_size, 2048u);
    ASSERT_EQ(z1->props()->extents.size(), 1u);
    EXPECT_EQ(z1->props()->extents[0].offset, 150 * CLUSTER);
    EXPECT_EQ(z1->props()->extents[0].length, 1024u);
}

TEST(NtfsVolume, GuidDumpDestroyRules) {
    /* S-012: the og's dump-pass destroy rules - all run STARTs claimed by
     * pass-1 extents -> destroyed; size <= 0x3ff -> destroyed; folders
     * never destroyed; unclaimed starts survive. */
    Image img;
    build_dump_image(img);
    DumpWalkResult w;
    ASSERT_NO_FATAL_FAILURE(walk_dump_image(img, w));
    EXPECT_EQ(2u, w.stats.guid_dump_destroyed); /* zombie2.dat + tiny.dat */
    EXPECT_EQ(nullptr, find_node(w.all, "zombie2.dat"))
        << "all-run-starts-claimed dump record must be destroyed";
    EXPECT_EQ(nullptr, find_node(w.all, "tiny.dat"))
        << "size <= 0x3ff dump record must be destroyed";
    EXPECT_NE(nullptr, find_node(w.all, "zombiedir"))
        << "folders are never destroyed by the dump pass";
    EXPECT_NE(nullptr, find_node(w.all, "zombie1.dat"));
}

TEST(NtfsVolume, SizeZeroRescueFromMftCopy) {
    /* S-012 step 3a: a deleted size-0 record whose $MFTMirr copy keeps the
     * resident $DATA gets content + size recovered at walk time. */
    Image img;
    build_dump_image(img);
    DumpWalkResult w;
    ASSERT_NO_FATAL_FAILURE(walk_dump_image(img, w));
    EXPECT_GE(w.stats.size_zero_rescued, 1u);
    tree::FileNode* g = find_node(w.all, "ghost0.dat");
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->props()->logical_size, 20u) << "mirror-resident content recovered";
    ASSERT_GE(g->props()->resident_data.size(), 20u);
    EXPECT_EQ(g->props()->resident_data[0], 0x00u);
    EXPECT_EQ(g->props()->resident_data[1], 0xEEu);
    EXPECT_EQ(g->props()->resident_data[2], 0xFFu);
    EXPECT_EQ(g->props()->resident_data[3], 0xC0u);
}

TEST(NtfsVolume, SizeZeroRescueSameNameMerge) {
    /* S-012 step 3b: a size-0 record beyond mirror coverage merges the
     * same-name record's runlist + size (post-walk relaxed pass). */
    Image img;
    build_dump_image(img);
    DumpWalkResult w;
    ASSERT_NO_FATAL_FAILURE(walk_dump_image(img, w));
    EXPECT_GE(w.stats.size_zero_rescued, 2u); /* mirror + merge */
    /* two records share the name; the merged one carries the adopted size */
    tree::FileNode* merged = nullptr;
    for (tree::FileNode* n : w.all) {
        if (n->name_utf8() == "twindrive.dat" && n->deleted_state() == tree::DeletedState::Deleted)
            merged = n;
    }
    ASSERT_NE(merged, nullptr);
    EXPECT_EQ(merged->props()->logical_size, 600u) << "size adopted from the same-name record";
    ASSERT_EQ(merged->props()->extents.size(), 1u);
    EXPECT_EQ(merged->props()->extents[0].offset, 190 * CLUSTER);
}

TEST(NtfsVolume, GuidDumpStrideIsRecordAligned) {
    /* S-012 stride correction: the og's dump walk advances local_c0 by the
     * RECORD SIZE on every path (uVar8 at :21151 and LAB_18001b0cb :21293;
     * uVar16 = max(0x400, rs) only after props-fail/skip/destroy - equal to
     * rs through the boot clamp). It NEVER strides by one sector: a stale
     * record at a 512-shifted (non-record-aligned) dump offset is never
     * examined. The pre-correction code advanced holes by 512 and found
     * such records - a superset the og does not produce. */
    std::vector<u8> rec;
    {
        Image base;
        build_dump_image(base);
        rec.assign(base.sec(D_DUMP_LCN), base.sec(D_DUMP_LCN) + RECORD_SIZE); /* zombie1.dat */
    }
    for (int shifted = 0; shifted <= 1; ++shifted) {
        Image img;
        build_dump_image(img);
        /* wipe the whole dump, then place ONE stale record at either offset
         * 0 (record-aligned) or offset 512 (sector-shifted) */
        std::memset(img.sec(D_DUMP_LCN), 0, 8 * CLUSTER);
        std::memcpy(img.sec(D_DUMP_LCN + (shifted ? 1 : 0)), rec.data(), RECORD_SIZE);
        DumpWalkResult w;
        ASSERT_NO_FATAL_FAILURE(walk_dump_image(img, w)) << "shifted=" << shifted;
        tree::FileNode* z1 = find_node(w.all, "zombie1.dat");
        if (shifted) {
            EXPECT_EQ(nullptr, z1)
                << "a 512-shifted dump record is never examined (og record-size stride)";
            EXPECT_EQ(0u, w.stats.guid_dump_records);
        } else {
            ASSERT_NE(z1, nullptr) << "control: the record-aligned dump record is found";
            EXPECT_GE(w.stats.guid_dump_records, 1u);
        }
    }
}

TEST(NtfsVolume, GuidDumpLargeRecordParsedFully) {
    /* S-012 stride correction: on a 2 KiB-record volume (cpr=4) the dump
     * pass must parse the FULL record - the pre-correction code bounded
     * fixup/enum at max(sector, 0x400) = 1024 and lost every record whose
     * attribute layout crossed that boundary. */
    Image img;
    {
        RecBuilder b;
        u8* bs = img.sec(0);
        BootSector* boot = reinterpret_cast<BootSector*>(bs);
        const char oem[8] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' '};
        std::memcpy(boot->oem, oem, 8);
        boot->bytes_per_sector = SEC;
        boot->sectors_per_cluster = 1;
        boot->media_descriptor = 0xF8;
        boot->total_sectors = NSECTORS;
        boot->mft_lcn = 4;
        boot->mftmirr_lcn = 60;
        boot->clusters_per_record = 4; /* 2048-byte records */
        boot->clusters_per_index_block = 2;
        boot->checksum = 0;
        bs[510] = 0x55;
        bs[511] = 0xAA;
    }
    constexpr u64 RS2K = 2048;
    /* record 0: $MFT - 48 clusters = 12 records of 2 KiB */
    {
        RecBuilder r(RS2K);
        r.header(0, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFT", FILE_NAME_WIN32, 0x06, 48 * CLUSTER, 48 * CLUSTER);
        r.attr_data_runlist(4, 48, 48 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(4), r.buf.data(), RS2K);
    }
    /* record 1: $MFTMirr */
    {
        RecBuilder r(RS2K);
        r.header(1, RECORD_IN_USE);
        r.attr_file_name(5, u"$MFTMirr", FILE_NAME_WIN32, 0x06, 8 * CLUSTER, 8 * CLUSTER);
        r.attr_data_runlist(60, 8, 8 * CLUSTER);
        r.finish();
        std::memcpy(img.sec(4 + 4), r.buf.data(), RS2K);
        std::memcpy(img.sec(60), img.sec(4), RS2K);         /* rec0 copy */
        std::memcpy(img.sec(60 + 4), img.sec(4 + 4), RS2K); /* rec1 copy */
    }
    /* record 5: root "." */
    {
        RecBuilder r(RS2K);
        r.header(5, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0);
        r.attr_file_name(5, u".", FILE_NAME_POSIX, 0x00, 0, 0);
        r.attr_index_root();
        r.finish();
        std::memcpy(img.sec(4 + 4 * 5), r.buf.data(), RS2K);
    }
    /* record 8: the live GUID-named dump file, $DATA = LCN 200, 8 clusters
     * (4096 bytes = two 2 KiB record slots) */
    {
        RecBuilder r(RS2K);
        r.header(8, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5,
                         u"{8c8d1e50-9f2e-4a1b-8e6f-3c2d5a4b7e90}"
                         u"{3808876b-c176-4e48-b7ae-04046e6cc752}",
                         FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_data_runlist(200, 8, 4096);
        r.finish();
        std::memcpy(img.sec(4 + 4 * 8), r.buf.data(), RS2K);
    }
    /* the dump content: one 2 KiB stale record at offset 0 with a resident
     * $DATA large enough that the attribute walk crosses the 1024 boundary
     * (terminator lands past 1 KiB - the pre-fix parse bound) */
    {
        RecBuilder r(RS2K);
        r.header(17, 0);
        r.attr_si(0x20);
        r.attr_file_name(5, u"bigrec.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 1100, 1100);
        r.attr_data_resident(u"", 275); /* 1100 bytes of resident payload */
        r.finish();
        ASSERT_GT(r.attr_pos, 1024u) << "fixture: attributes must cross the 1 KiB boundary";
        std::memcpy(img.sec(200), r.buf.data(), RS2K);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    EXPECT_EQ(2048u, vol->params().record_size);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    auto* rd = tree.add_root(u"D");
    auto* rl = tree.add_root(u"L");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(OK,
              walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    tree::FileNode* big = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "bigrec.dat")
                big = c;
            find(c);
        }
    };
    for (auto* r : tree.roots())
        find(r);
    ASSERT_NE(big, nullptr) << "a 2 KiB-record dump record must parse at the full record size";
    EXPECT_EQ(big->props()->logical_size, 1100u);
    EXPECT_FALSE(big->props()->resident_data.empty());
}

TEST(NtfsVolume, MainWalkKeepsSystemFlaggedUserFile) {
    /* N-040: the og's pass-1 attribute gate is (og-15 metafile name) AND
     * ($SI SYSTEM+HIDDEN) - FUN_180034b20 -> FUN_180040f20/FUN_180040ea0.
     * There is NO $FILE_NAME-flags system filter: a user file carrying
     * $FILE_NAME SYSTEM|HIDDEN flags (pagefile.sys-class) is KEPT. The old
     * `dos_flags & 4` gate dropped exactly those files. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(10, RECORD_IN_USE);
        r.attr_si(0x20); /* $SI: ARCHIVE only - not the og's SYSTEM+HIDDEN gate */
        r.attr_file_name(5, u"pagefile.sys", FILE_NAME_WIN32_AND_DOS, 0x06, 1024, 1024);
        r.attr_data_runlist(150, 2, 1024);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 20), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"F");
    auto* rd = tree.add_root(u"D");
    auto* rl = tree.add_root(u"L");
    WalkerFlags wf;
    WalkStats stats;
    ASSERT_EQ(
        OK, walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats));
    tree::FileNode* pf = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "pagefile.sys")
                pf = c;
            find(c);
        }
    };
    for (auto* r : tree.roots())
        find(r);
    ASSERT_NE(pf, nullptr) << "$FN-SYSTEM user files surface (og has no such filter)";
    EXPECT_EQ(pf->deleted_state(), tree::DeletedState::Active);
}

/* ---- S-022: directory-entry second source (INDX + $INDEX_ROOT) ----------- */

namespace {

/* one INDEX_ENTRY with an embedded $FILE_NAME key (test-side writer; the
 * layout is the og-decoded one - see cr_ntfs_wire.h S-022 comment) */
struct IndxTestEntry {
    u64 ref = 0;
    u64 parent = 0;
    u64 alloc_sz = 0;
    u64 real_sz = 0;
    u32 dos_flags = 0x20; /* ARCHIVE by default (og gate needs it) */
    const char16_t* name = u"";
    bool last = false;
};

size_t write_indx_entry(std::vector<u8>& v, size_t at, const IndxTestEntry& e) {
    const std::u16string n = e.name;
    const u32 elen = 0x52u + u32(n.size()) * 2u;
    const u32 padded = (elen + 7u) & ~7u;
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x00, e.ref);
    RecBuilder::u16_from_bytes_le_wr(v, at + 0x08, u16(padded));
    RecBuilder::u16_from_bytes_le_wr(v, at + 0x0A, u16(0x42 + n.size() * 2)); /* key len */
    RecBuilder::u16_from_bytes_le_wr(v, at + 0x0C, u16(e.last ? 2 : 0));
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x10, e.parent);
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x18, 132000000000000000ULL); /* create */
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x20, 133000000000000000ULL); /* modify */
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x28, 134000000000000000ULL); /* change */
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x30, 135000000000000000ULL); /* access */
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x38, e.alloc_sz);
    RecBuilder::u64_from_bytes_le_wr(v, at + 0x40, e.real_sz);
    RecBuilder::u32_from_bytes_le_wr(v, at + 0x48, e.dos_flags);
    RecBuilder::u32_from_bytes_le_wr(v, at + 0x4C, 0); /* reparse */
    v[at + 0x50] = u8(n.size());
    v[at + 0x51] = 0; /* namespace: win32 */
    for (size_t i = 0; i < n.size(); ++i)
        RecBuilder::u16_from_bytes_le_wr(v, at + 0x52 + i * 2, n[i]);
    return at + padded;
}

/* a 4 KiB INDX block (8 sectors, usa_count 9) with the given in-use chain */
std::vector<u8> build_indx_block(const std::vector<IndxTestEntry>& entries, u32 in_use_override = 0,
                                 std::vector<IndxTestEntry>* slack = nullptr) {
    std::vector<u8> blk(4096, 0);
    std::memcpy(blk.data(), "INDX", 4);
    RecBuilder::u16_from_bytes_le_wr(blk, 0x04, 0x28);        /* usa offset */
    RecBuilder::u16_from_bytes_le_wr(blk, 0x06, 9);           /* usa count: 8 sectors + 1 */
    RecBuilder::u32_from_bytes_le_wr(blk, 0x18, 0x28);        /* entries offset (rel. +0x18) */
    RecBuilder::u32_from_bytes_le_wr(blk, 0x20, 4096 - 0x18); /* allocated (rel. +0x18) */
    RecBuilder::u32_from_bytes_le_wr(blk, 0x24, 1);           /* leaf */
    size_t at = 0x40;
    for (const IndxTestEntry& e : entries)
        at = write_indx_entry(blk, at, e);
    const u32 in_use = in_use_override ? in_use_override : u32(at - 0x18);
    RecBuilder::u32_from_bytes_le_wr(blk, 0x1C, in_use);
    /* slack entries live after the in-use chain (never harvested) */
    if (slack != nullptr)
        for (const IndxTestEntry& e : *slack)
            at = write_indx_entry(blk, at, e);
    /* USA: slot0 = USN, slots 1..8 = sector-tail originals */
    for (int i = 0; i < 9; ++i)
        RecBuilder::u16_from_bytes_le_wr(blk, 0x28 + u32(i) * 2, 0xAAAA);
    RecBuilder::u16_from_bytes_le_wr(blk, 0x28, 0x0BB8);
    for (int s = 0; s < 8; ++s) {
        blk[u64(s) * 512 + 510] = 0xB8;
        blk[u64(s) * 512 + 511] = 0x0B;
    }
    return blk;
}

/* resident $INDEX_ROOT attribute carrying the given entries */
void attr_index_root_entries(RecBuilder& r, const std::vector<IndxTestEntry>& es) {
    std::vector<u8> val(0x400, 0);
    RecBuilder::u32_from_bytes_le_wr(val, 0x00, 0x30);         /* $I30 */
    RecBuilder::u32_from_bytes_le_wr(val, 0x04, 1);            /* collation */
    RecBuilder::u32_from_bytes_le_wr(val, 0x08, 512);          /* index block size */
    val[0x0C] = 1;                                             /* clusters per index block */
    RecBuilder::u32_from_bytes_le_wr(val, 0x10, 0x10);         /* entries offset (rel. to +0x10) */
    RecBuilder::u32_from_bytes_le_wr(val, 0x18, 0x400 - 0x10); /* allocated */
    RecBuilder::u32_from_bytes_le_wr(val, 0x1C, 0);            /* flags: not large-index */
    size_t at = 0x20;
    for (const IndxTestEntry& e : es)
        at = write_indx_entry(val, at, e);
    RecBuilder::u32_from_bytes_le_wr(val, 0x14, u32(at - 0x10)); /* in-use (rel. to +0x10) */
    /* write as a resident attribute */
    const u32 vlen = u32(at);
    const u32 voff = 0x18;
    const u32 alen = (voff + vlen + 7) / 8 * 8;
    const u32 pos = r.attr_pos;
    RecBuilder::u32_from_bytes_le_wr(r.buf, pos + 0, ATTR_TYPE_INDEX_ROOT);
    RecBuilder::u32_from_bytes_le_wr(r.buf, pos + 4, alen);
    r.buf[pos + 8] = 0; /* resident */
    r.buf[pos + 9] = 0; /* unnamed */
    RecBuilder::u16_from_bytes_le_wr(r.buf, pos + 10, u16(voff));
    RecBuilder::u16_from_bytes_le_wr(r.buf, pos + 16, u16(vlen & 0xFFFF));
    RecBuilder::u16_from_bytes_le_wr(r.buf, pos + 20, u16(voff));
    std::memcpy(r.buf.data() + pos + voff, val.data(), vlen);
    /* the bulk copy may span the USA sector tails - re-stamp them (the
     * field-wise writers leave them intact; this one must restore them) */
    for (u32 t = 0; t * SEC + SEC <= RECORD_SIZE; ++t) {
        r.buf[u64(t) * SEC + SEC - 2] = 0xB8;
        r.buf[u64(t) * SEC + SEC - 1] = 0x0B;
    }
    r.attr_pos += alen;
}

u32 scan_remnants_simple(IStorageDevice* dev, tree::ResultTree& tree, WalkStats& stats) {
    auto* root_lost = tree.add_root(u"Lost Files");
    return scan_mft_remnants(dev, SEC, CLUSTER, RECORD_SIZE, TOTAL_CLUSTERS, nullptr, nullptr,
                             nullptr, tree, root_lost, stats);
}

} // namespace

TEST(NtfsVolume, OrphanIndxBlockHarvestsEntries) {
    /* S-022 feeder B: a directory's INDX block survives in raw space (its
     * MFT record is gone) - the in-use entries surface as Lost nodes with
     * name/size/times from the embedded $FILE_NAME keys. */
    Image img;
    {
        const std::vector<IndxTestEntry> entries = {
            {(1ull << 48) | 9001, (1ull << 48) | 55, 4096, 3000, 0x20, u"indx-a.dat", false},
            {(1ull << 48) | 9002, (1ull << 48) | 55, 128, 100, 0x30, u"indx-dir", false},
            {(1ull << 48) | 9003, (1ull << 48) | 55, 512, 512, 0x20, u"indx-b.dat", true},
        };
        auto blk = build_indx_block(entries);
        std::memcpy(img.sec(20), blk.data(), blk.size());
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    WalkStats stats;
    ASSERT_EQ(OK, scan_remnants_simple(&dev, tree, stats));
    auto* root_lost = tree.roots()[0];
    tree::FileNode *a = nullptr, *d = nullptr, *b = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling()) {
        if (c->name_utf8() == "indx-a.dat")
            a = c;
        else if (c->name_utf8() == "indx-dir")
            d = c;
        else if (c->name_utf8() == "indx-b.dat")
            b = c;
    }
    ASSERT_NE(a, nullptr);
    ASSERT_NE(d, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->props()->logical_size, 3000u);
    EXPECT_EQ(a->props()->times.created, 132000000000000000ULL);
    EXPECT_EQ(a->confidence(), tree::Confidence::MetadataOnly);
    EXPECT_EQ(a->origin(), tree::Origin::Lost);
    EXPECT_EQ(d->props()->attributes & 0x10, 0x10u); /* folder bit from flags */
    EXPECT_EQ(b->props()->logical_size, 512u);
    EXPECT_GE(stats.emitted, 3u);
}

TEST(NtfsVolume, IndxHiddenSystemEntryRejected) {
    /* S-022: the og's entry gate rejects HIDDEN|SYSTEM both-set entries
     * (FUN_18000e970 :11032) - metafile-class content never surfaces. */
    Image img;
    {
        const std::vector<IndxTestEntry> entries = {
            {(1ull << 48) | 9101, (1ull << 48) | 55, 4096, 3000, 0x26, u"secret.dat", false},
            {(1ull << 48) | 9102, (1ull << 48) | 55, 512, 512, 0x20, u"plain.dat", true},
        };
        auto blk = build_indx_block(entries);
        std::memcpy(img.sec(20), blk.data(), blk.size());
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    WalkStats stats;
    ASSERT_EQ(OK, scan_remnants_simple(&dev, tree, stats));
    auto* root_lost = tree.roots()[0];
    tree::FileNode* secret = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "secret.dat")
            secret = c;
    EXPECT_EQ(nullptr, secret) << "HIDDEN|SYSTEM entries must be rejected";
}

TEST(NtfsVolume, IndxSlackNeverHarvested) {
    /* S-022 parity edge: entries beyond the in-use end (deleted-entry
     * remnants in INDX slack) are NEVER harvested - the og's walk is
     * bounded by the in-use size and stops at the last-entry flag. */
    Image img;
    {
        const std::vector<IndxTestEntry> entries = {
            {(1ull << 48) | 9201, (1ull << 48) | 55, 512, 512, 0x20, u"live-entry.dat", true},
        };
        std::vector<IndxTestEntry> slack = {
            {(1ull << 48) | 9202, (1ull << 48) | 55, 8000, 8000, 0x20, u"slack-ghost.dat", false},
        };
        auto blk = build_indx_block(entries, 0, &slack);
        std::memcpy(img.sec(20), blk.data(), blk.size());
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    WalkStats stats;
    ASSERT_EQ(OK, scan_remnants_simple(&dev, tree, stats));
    auto* root_lost = tree.roots()[0];
    tree::FileNode *live = nullptr, *ghost = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling()) {
        if (c->name_utf8() == "live-entry.dat")
            live = c;
        else if (c->name_utf8() == "slack-ghost.dat")
            ghost = c;
    }
    ASSERT_NE(live, nullptr);
    EXPECT_EQ(nullptr, ghost) << "INDX slack entries must never be harvested";
}

TEST(NtfsVolume, RemnantDirRecordHarvestsIndexRootChildren) {
    /* S-022 feeder A: a recovered directory record found in raw space with
     * a resident $INDEX_ROOT (large-index==0, in-use <= 0x320) - its child
     * entries surface as Lost/MetadataOnly nodes. */
    Image img;
    {
        RecBuilder r;
        r.header(40, RECORD_IN_USE | RECORD_IS_DIRECTORY);
        r.attr_si(0x30);
        r.attr_file_name(5, u"orphandir", FILE_NAME_WIN32, 0x30, 0, 0);
        attr_index_root_entries(r, {
                                       {(1ull << 48) | 9301, (1ull << 48) | 40, 2048, 1500, 0x20,
                                        u"rootchild-a.txt", false},
                                       {(1ull << 48) | 9302, (1ull << 48) | 40, 256, 200, 0x20,
                                        u"rootchild-b.txt", true},
                                   });
        r.finish();
        std::memcpy(img.sec(40), r.buf.data(), RECORD_SIZE);
    }
    MemDevice dev(img.raw(), img.geom());
    tree::ResultTree tree;
    WalkStats stats;
    ASSERT_EQ(OK, scan_remnants_simple(&dev, tree, stats));
    auto* root_lost = tree.roots()[0];
    tree::FileNode *dir = nullptr, *ca = nullptr, *cb = nullptr;
    for (tree::FileNode* c = root_lost->first_child(); c; c = c->next_sibling()) {
        if (c->name_utf8() == "orphandir")
            dir = c;
        else if (c->name_utf8() == "rootchild-a.txt")
            ca = c;
        else if (c->name_utf8() == "rootchild-b.txt")
            cb = c;
    }
    EXPECT_NE(dir, nullptr) << "the directory record itself still emits";
    ASSERT_NE(ca, nullptr) << "$INDEX_ROOT children must be harvested";
    ASSERT_NE(cb, nullptr);
    EXPECT_EQ(ca->props()->logical_size, 1500u);
    EXPECT_EQ(ca->confidence(), tree::Confidence::MetadataOnly);
    EXPECT_EQ(cb->props()->logical_size, 200u);
}
TEST(NtfsVolume, AdsDroppedInParityMode) {
    /* N-011 (diff-to-fix, FIXED-ALREADY verification pin): the og's
     * FUN_1800069e0 :5159-5160 skips named $DATA (ADS) entirely; the
     * core's superset (emit_ads default ON) mirrors M-007's opt-in
     * parity shape - with the flag OFF the record's ADS must produce
     * ZERO child nodes while the primary stream is unaffected. */
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
    wf.emit_ads = false; /* og parity mode */
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    EXPECT_EQ(stats.ads, 0u) << "parity mode must drop every ADS node";
    int colon_names = 0;
    std::function<void(tree::FileNode*)> scan = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8().find(':') != std::string::npos)
                ++colon_names;
            scan(c);
        }
    };
    scan(ra);
    scan(rd);
    scan(rl);
    EXPECT_EQ(colon_names, 0) << "no ADS-shaped names may survive parity mode";
    /* the host file itself is unaffected */
    bool host_found = false;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "ads.txt")
                host_found = true;
            find(c);
        }
    };
    find(ra);
    find(rd);
    EXPECT_TRUE(host_found) << "the primary stream's node must survive";
}

TEST(NtfsVolume, AdsEmittedByDefaultSuperset) {
    /* N-011 companion pin: default flags keep the recovery superset
     * (one child node per named $DATA, name = host:stream). */
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
    WalkerFlags wf; /* defaults: emit_ads = true */
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    EXPECT_GE(stats.ads, 1u) << "default superset keeps ADS emission";
}

/* ---- N-012 / N-011a / N-039 (diff-to-fix minors session) ---------------- */

TEST(NtfsVolume, AttributesFromSIWhenAvailable) {
    /* N-012: the og serializes attributes from $STANDARD_INFORMATION's
     * dos_flags (FUN_180044f60 on the $SI attribute, NTFSSearchDLL
     * :5351-5368), NOT from $FILE_NAME. $SI says hidden (0x02), the
     * $FILE_NAME says nothing -> the node must carry 0x02. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(18, RECORD_IN_USE);
        r.attr_si(0x02); /* $SI: hidden */
        r.attr_file_name(5, u"sihidden.txt", FILE_NAME_WIN32_AND_DOS, 0x00, 512, 512);
        r.attr_data_runlist(240, 1, 512);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 36), r.buf.data(), RECORD_SIZE); /* rec 18 */
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
    tree::FileNode* n = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "sihidden.txt")
                n = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->attributes & 0x02, 2u) << "$SI hidden bit must win over $FILE_NAME";
}

TEST(NtfsVolume, AttributesFallBackToFileNameWhenSIAbsent) {
    /* N-012 companion: no $SI -> $FILE_NAME flags serve (mirrors the
     * time-precedence rule one line above in the walk). */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(19, RECORD_IN_USE);
        /* deliberately NO attr_si */
        r.attr_file_name(5, u"nosi.txt", FILE_NAME_WIN32_AND_DOS, 0x02, 512, 512);
        r.attr_data_runlist(241, 1, 512);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 38), r.buf.data(), RECORD_SIZE); /* rec 19 */
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
    tree::FileNode* n = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "nosi.txt")
                n = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->attributes & 0x02, 2u) << "$FILE_NAME fallback must serve";
}

TEST(NtfsVolume, AttributesPreserveOfflineBit) {
    /* N-012: the og preserves the 0x400 OFFLINE bit (HSM-migrated files);
     * the old & 0xFF mask dropped it. */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(20, RECORD_IN_USE);
        r.attr_si(0x400 | 0x20); /* offline + archive */
        r.attr_file_name(5, u"offline.dat", FILE_NAME_WIN32_AND_DOS, 0x20, 512, 512);
        r.attr_data_runlist(242, 1, 512);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 40), r.buf.data(), RECORD_SIZE); /* rec 20 */
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
    tree::FileNode* n = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "offline.dat")
                n = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->attributes & 0x400, 0x400u) << "OFFLINE bit must survive the mask";
}

TEST(NtfsVolume, ShortNameFieldRoundtrips) {
    /* N-011a: a distinct DOS-namespace $FILE_NAME becomes the props'
     * short_name (the og's GetShortName* vtable field), and the field
     * survives a session serialize/deserialize roundtrip (versioned
     * optional nflags bit 2). */
    Image img;
    build_image(img);
    {
        RecBuilder r;
        r.header(21, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(5, u"longfilename.txt", FILE_NAME_WIN32, 0x20, 512, 512);
        r.attr_file_name(5, u"LONGFI~1.TXT", FILE_NAME_DOS, 0x20, 512, 512);
        r.attr_data_runlist(243, 1, 512);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 42), r.buf.data(), RECORD_SIZE); /* rec 21 */
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
    tree::FileNode* n = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "longfilename.txt")
                n = c;
            find(c);
        }
    };
    find(ra);
    find(rd);
    ASSERT_NE(n, nullptr);
    ASSERT_NE(n->props(), nullptr);
    EXPECT_EQ(n->name_utf8(), "longfilename.txt");
    ASSERT_EQ(n->props()->short_name, u"LONGFI~1.TXT")
        << "the DOS-namespace name must land in short_name";
    /* session roundtrip: the field survives serialize + deserialize */
    std::vector<u8> blob;
    ASSERT_EQ(cr::session::serialize_tree(tree, blob), OK);
    tree::ResultTree tree2;
    ASSERT_EQ(cr::session::deserialize_tree(blob.data(), blob.size(), tree2), OK);
    tree::FileNode* n2 = nullptr;
    std::function<void(tree::FileNode*)> find2 = [&](tree::FileNode* p) {
        for (tree::FileNode* c = p->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->name_utf8() == "longfilename.txt")
                n2 = c;
            find2(c);
        }
    };
    for (auto* root : tree2.roots())
        find2(root);
    ASSERT_NE(n2, nullptr) << "node lost in the session roundtrip";
    ASSERT_NE(n2->props(), nullptr);
    EXPECT_EQ(n2->props()->short_name, u"LONGFI~1.TXT")
        << "short_name must survive the session roundtrip (nflags bit 2)";
}

/* Plant the $UsnJrnl:$J record (MFT slot 23) mapping LCN 160, 2 clusters -
   the same shape the UsnJrnlHarvest fixture uses. */
void plant_usnjrnl_record(Image& img) {
    RecBuilder r;
    r.header(23, RECORD_IN_USE);
    r.attr_si(0x20);
    r.attr_file_name(11, u"$UsnJrnl", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
    {
        u32 pos = r.attr_pos;
        RunList rl;
        rl.runs().push_back(DataRun{0, 2, 0, 160, false});
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

TEST(NtfsVolume, UsnHarvestTrueDeleteReason0x80020000) {
    /* N-039 pin: a true Windows delete record (CLOSE|FILE_DELETE =
     * 0x80020000) IS harvested - the og's gate is the mis-decoded 0x200
     * (COMPRESSION_CHANGE); do NOT copy it. */
    Image img;
    build_image(img);
    plant_usnjrnl_record(img);
    {
        std::vector<u8>& imgv = img.raw();
        const u64 jbase = 160 * SEC;
        auto put_rec = [&](u64 at, const char16_t* name, u32 reason) {
            const std::u16string n = name;
            const u32 namelen = u32(n.size() * 2);
            const u32 rlen = 64 + namelen;
            auto j32 = [&](u32 a, u32 x) {
                for (int i = 0; i < 4; i++)
                    imgv[size_t(a) + i] = u8(x >> (8 * i));
            };
            auto j16 = [&](u32 a, u16 x) {
                imgv[size_t(a)] = u8(x);
                imgv[size_t(a) + 1] = u8(x >> 8);
            };
            auto j64 = [&](u32 a, u64 x) {
                for (int i = 0; i < 8; i++)
                    imgv[size_t(a) + i] = u8(x >> (8 * i));
            };
            j32(u32(at + 0), rlen);
            j16(u32(at + 4), 2);
            j16(u32(at + 6), 0);
            j64(u32(at + 8), 4321);
            j64(u32(at + 16), 5 | (1ULL << 48));
            j64(u32(at + 24), 2);
            j64(u32(at + 32), 132000000000000000ULL);
            j32(u32(at + 40), reason);
            j32(u32(at + 44), 0);
            j32(u32(at + 48), 0);
            j32(u32(at + 52), 0x20);
            j32(u32(at + 56), namelen);
            j32(u32(at + 60), 64);
            for (size_t i = 0; i < n.size(); i++)
                j16(u32(at + 64 + u32(i) * 2), u16(n[i]));
            return rlen;
        };
        put_rec(jbase, u"realdelete.txt", 0x80020000u); /* CLOSE|FILE_DELETE */
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
    tree::FileNode* got = nullptr;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        if (c->name_utf8() == "realdelete.txt")
            got = c;
    EXPECT_NE(got, nullptr) << "0x80020000 (true delete) must harvest";
}

TEST(NtfsVolume, UsnHarvestRejectsCompressionChangeOnly) {
    /* N-039 pin: 0x80000200 (CLOSE|COMPRESSION_CHANGE - NOT a deletion)
     * must NOT harvest. The og's 0x200 gate fires here - an og defect. */
    Image img;
    build_image(img);
    plant_usnjrnl_record(img);
    {
        std::vector<u8>& imgv = img.raw();
        const u64 jbase = 160 * SEC;
        auto put_rec = [&](u64 at, const char16_t* name, u32 reason) {
            const std::u16string n = name;
            const u32 namelen = u32(n.size() * 2);
            const u32 rlen = 64 + namelen;
            auto j32 = [&](u32 a, u32 x) {
                for (int i = 0; i < 4; i++)
                    imgv[size_t(a) + i] = u8(x >> (8 * i));
            };
            auto j16 = [&](u32 a, u16 x) {
                imgv[size_t(a)] = u8(x);
                imgv[size_t(a) + 1] = u8(x >> 8);
            };
            auto j64 = [&](u32 a, u64 x) {
                for (int i = 0; i < 8; i++)
                    imgv[size_t(a) + i] = u8(x >> (8 * i));
            };
            j32(u32(at + 0), rlen);
            j16(u32(at + 4), 2);
            j16(u32(at + 6), 0);
            j64(u32(at + 8), 4322);
            j64(u32(at + 16), 5 | (1ULL << 48));
            j64(u32(at + 24), 2);
            j64(u32(at + 32), 132000000000000000ULL);
            j32(u32(at + 40), reason);
            j32(u32(at + 44), 0);
            j32(u32(at + 48), 0);
            j32(u32(at + 52), 0x20);
            j32(u32(at + 56), namelen);
            j32(u32(at + 60), 64);
            for (size_t i = 0; i < n.size(); i++)
                j16(u32(at + 64 + u32(i) * 2), u16(n[i]));
            return rlen;
        };
        put_rec(jbase, u"compresschg.txt", 0x80000200u); /* CLOSE|COMPRESSION_CHANGE */
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
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling())
        EXPECT_NE(c->name_utf8(), "compresschg.txt")
            << "compression-change-only records must not harvest";
}

TEST(NtfsVolume, UsnHarvestCapMinesTail) {
    /* N-039 fix-plan item 3: $J is circular - the newest records live at
     * the TAIL. A stream larger than the 64 MiB mining cap must mine the
     * tail window (the old head-mining read exactly the stale half).
     * Layout (r3 sized to EXACTLY the cap so the mined window aligns to
     * r3's start; the benign records tile the window so the resync limit
     * is never tripped):
     *   r1: mapped 1 KiB @160 - the OLD delete (skipped head window)
     *   r2: sparse 2 MiB hole (skipped)
     *   r3: mapped 64 MiB - benign non-delete records tiled from the
     *       start + the NEW delete (tailgone.txt) at the very end. */
    Image img;
    build_image(img);
    const u64 kHoleClusters = 4096;                  /* 2 MiB sparse */
    const u64 kR3Clusters = (64ull << 20) / CLUSTER; /* exactly the cap */
    const u64 kTailLcn = 160 + 2 + kHoleClusters;    /* r3 mapped here */
    const u64 kTotalSectors = (kTailLcn + kR3Clusters) * (CLUSTER / SEC);
    img.raw().resize(size_t(kTotalSectors * SEC), 0);
    img.geom().sector_count = kTotalSectors;
    img.geom().total_bytes = img.raw().size();
    /* grow the DBR's total_sectors too - the $J runlist is bounds-checked
     * against the declared volume size (parse_data_runs) */
    for (int i = 0; i < 8; i++)
        img.sec(0)[0x28 + size_t(i)] = u8(kTotalSectors >> (8 * i));
    /* $UsnJrnl:$J with the three runs */
    {
        RecBuilder r;
        r.header(23, RECORD_IN_USE);
        r.attr_si(0x20);
        r.attr_file_name(11, u"$UsnJrnl", FILE_NAME_WIN32_AND_DOS, 0x20, 2048, 2048);
        {
            u32 pos = r.attr_pos;
            RunList rl;
            rl.runs().push_back(DataRun{0, 2, 0, 160, false});
            rl.runs().push_back(DataRun{2, kHoleClusters, 0, 0, true});
            rl.runs().push_back(DataRun{2 + kHoleClusters, kR3Clusters, 0, kTailLcn, false});
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
            r.u64wr(pos + 40, (2 + kHoleClusters + kR3Clusters) * CLUSTER);
            r.u64wr(pos + 48, (2 + kHoleClusters + kR3Clusters) * CLUSTER);
            r.u64wr(pos + 56, (2 + kHoleClusters + kR3Clusters) * CLUSTER);
            r.u16wr(pos + name_off, u'$');
            r.u16wr(pos + name_off + 2, u'J');
            std::memcpy(r.buf.data() + pos + runs_off, rle.data(), rlen);
            r.attr_pos += alen;
        }
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 46), r.buf.data(), RECORD_SIZE);
    }
    /* journal content */
    {
        std::vector<u8>& imgv = img.raw();
        auto put_rec = [&](u64 at, const char16_t* name, u32 reason) {
            const std::u16string n = name;
            const u32 namelen = u32(n.size() * 2);
            const u32 rlen = 64 + namelen;
            auto j32 = [&](u32 a, u32 x) {
                for (int i = 0; i < 4; i++)
                    imgv[size_t(a) + i] = u8(x >> (8 * i));
            };
            auto j16 = [&](u32 a, u16 x) {
                imgv[size_t(a)] = u8(x);
                imgv[size_t(a) + 1] = u8(x >> 8);
            };
            auto j64 = [&](u32 a, u64 x) {
                for (int i = 0; i < 8; i++)
                    imgv[size_t(a) + i] = u8(x >> (8 * i));
            };
            j32(u32(at + 0), rlen);
            j16(u32(at + 4), 2);
            j16(u32(at + 6), 0);
            j64(u32(at + 8), 4242);
            j64(u32(at + 16), 5 | (1ULL << 48));
            j64(u32(at + 24), 1);
            j64(u32(at + 32), 132000000000000000ULL);
            j32(u32(at + 40), reason);
            j32(u32(at + 44), 0);
            j32(u32(at + 48), 0);
            j32(u32(at + 52), 0x20);
            j32(u32(at + 56), namelen);
            j32(u32(at + 60), 64);
            for (size_t i = 0; i < n.size(); i++)
                j16(u32(at + 64 + u32(i) * 2), u16(n[i]));
            return rlen;
        };
        /* r1: the OLD delete - inside the skipped head window */
        put_rec(160 * SEC, u"gonefile.txt", kFixtureReasonBit | USN_REASON_FILE_DELETE);
        /* r3: benign non-delete records tiled from r3's start (the mined
         * window is exactly r3, so the tiling is phase-aligned), then the
         * NEW delete at the very end of the stream. */
        const u64 r3_bytes = kR3Clusters * CLUSTER;
        const u64 r3_base = kTailLcn * CLUSTER;
        const u64 benign_rlen = 88; /* 64 + 12 chars x 2 */
        u64 off = r3_base;
        while (off + benign_rlen <= r3_base + r3_bytes - 1024) {
            put_rec(off, u"padrec000000", kFixtureReasonBit);
            off += benign_rlen;
        }
        put_rec(r3_base + r3_bytes - 1024, u"tailgone.txt",
                kFixtureReasonBit | USN_REASON_FILE_DELETE);
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
    tree::FileNode* tail_gone = nullptr;
    tree::FileNode* head_gone = nullptr;
    for (tree::FileNode* c = rl->first_child(); c; c = c->next_sibling()) {
        if (c->name_utf8() == "tailgone.txt")
            tail_gone = c;
        if (c->name_utf8() == "gonefile.txt")
            head_gone = c;
    }
    EXPECT_NE(tail_gone, nullptr) << "the TAIL delete must be mined (circular journal)";
    EXPECT_EQ(head_gone, nullptr) << "the skipped head window must not be mined";
}


/* ---- S-009 hardening (session 9's port): the REAL $Bitmap drives the
 * free overlay through the exact producer contract the dispatch uses
 * (invert: bit set = cluster free). The fixture's planted bitmap truth:
 * used 4..83 (meta) + 200/201/240/250 + 220..223/230..231; free 210 and
 * the rest. */
TEST(NtfsVolume, DiskMapFreeCellsFromRealBitmap) {
    Image img;
    build_image(img);
    MemDevice dev(img.raw(), img.geom());

    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << "open err " << err_text(err);
    std::unique_ptr<VolumeBitmap> bm;
    ASSERT_EQ(VolumeBitmap::load(vol.get(), bm), OK);
    ASSERT_NE(bm, nullptr);
    ASSERT_TRUE(bm->valid());
    ASSERT_TRUE(bm->test(200));
    ASSERT_FALSE(bm->test(210));

    /* the dispatch's producer contract (cr_scan.cpp FsFamily::Ntfs):
     * copy $Bitmap's bytes and invert - free = !in-use, out-of-range
     * never consulted (bounded by bit_count) */
    storage::FreeMap fm;
    fm.base_byte = 0;
    fm.cluster_bytes = vol->params().cluster_bytes;
    fm.cluster_count = bm->bit_count();
    fm.bits.assign(size_t((fm.cluster_count + 7) / 8), 0);
    for (u64 c = 0; c < fm.cluster_count; ++c)
        if (!bm->test(c))
            fm.bits[size_t(c >> 3)] |= u8(1u << (c & 7));
    ASSERT_TRUE(fm.valid());

    /* a tree whose ACTIVE file lives on the bitmap-used cluster 200 */
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Found (NTFS)");
    auto* props = new tree::FileProperties();
    props->name = u"onused.dat";
    props->logical_size = CLUSTER;
    props->extents.push_back({200 * CLUSTER, CLUSTER});
    tree::FileNode* n = tree.add_file(root, props);
    props->release();
    ASSERT_NE(n, nullptr);
    n->set_origin(tree::Origin::Structured);

    viz::DiskMapInputs in;
    in.slice_start = 0;
    in.slice_bytes = fm.cluster_count * CLUSTER;
    in.cluster_bytes = u32(CLUSTER);
    in.free_map = &fm;
    viz::DiskMapGeometry g;
    std::vector<u8> alloc;
    std::vector<std::pair<u64, u64>> bad;
    ASSERT_EQ(
        viz::build_disk_map(in, tree, g, &alloc, nullptr, nullptr, nullptr, bad, 0, nullptr), OK);
    ASSERT_EQ(g.cell_bytes, u64(CLUSTER)) << "cluster hint renders cluster-sized cells";
    ASSERT_GT(alloc.size(), 210u);

    EXPECT_EQ(alloc[200], viz::kCellAllocUsed) << "active file on a used cluster";
    EXPECT_EQ(alloc[210], viz::kCellAllocFree) << "bitmap-free, unclaimed cluster";
    EXPECT_EQ(alloc[size_t(BITMAP_LCN)], viz::kCellAllocUnknown)
        << "metadata-only used cluster stays Unknown (no file extent claims it)";

    u64 free_cells = 0;
    for (u8 v : alloc)
        if (v == viz::kCellAllocFree)
            ++free_cells;
    EXPECT_GT(free_cells, alloc.size() / 2);
}

/* ------------------------------------------------------------------ */
/* M-008: the attrlist chase depth model (default multi-hop; depth-1    */
/* opt-in parity; the deleted-gate premise corrected against S-012)     */
/* ------------------------------------------------------------------ */

namespace {

/* a deleted 3-record chain: attrlist -> child (fragment 1 + nested
 * attrlist) -> grandchild (fragment 2). Header flag 0 = not in use. */
void add_deleted_chain(Image& img) {
    {
        RecBuilder r;
        r.header(27, 0); /* NOT in use: the deleted base record */
        r.attr_si(0x20);
        r.attr_file_name(5, u"gonefrag.txt", FILE_NAME_WIN32_AND_DOS, 0x20, 4096, 4096);
        r.attr_attrlist_resident({{ATTR_TYPE_DATA, 0, 12 | (1ULL << 48)}});
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 54), r.buf.data(), RECORD_SIZE);
    }
    {
        RecBuilder r;
        r.header(12, 0); /* deleted child: fragment 1 + nested list -> 13 */
        r.attr_data_runlist_vcn(140, 4, 0, 4096);
        r.attr_attrlist_resident({{ATTR_TYPE_DATA, 4, 13 | (1ULL << 48)}});
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 24), r.buf.data(), RECORD_SIZE);
    }
    {
        RecBuilder r;
        r.header(13, 0); /* deleted grandchild: fragment 2 */
        r.attr_data_runlist_vcn(150, 4, 4, 4096);
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 26), r.buf.data(), RECORD_SIZE);
    }
}

tree::FileNode* node_named(tree::ResultTree& t, const char* want) {
    tree::FileNode* out = nullptr;
    std::function<void(tree::FileNode*)> find = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == want)
                out = c;
            find(c);
        }
    };
    for (auto* r : t.roots())
        find(r);
    return out;
}

u64 extent_bytes(const tree::FileNode* n) {
    u64 total = 0;
    if (n && n->props())
        for (const auto& e : n->props()->extents)
            total += e.length;
    return total;
}

} // namespace

TEST(NtfsVolume, MultiHopAttrlistOgParityStopsAtDepth1) {
    /* M-008 parity mode: single_hop_attrlist=true caps the chase at the
     * first level, exactly as the og's FUN_180041df0 does (no recursion
     * into child records' own lists). bigfrag.txt keeps record 24's
     * fragment and loses record 25's grandchild fragment. */
    Image img;
    build_image(img);
    /* the NestedAttrlistChase fixture chain: 23 -> 24 -> 25 */
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
        RecBuilder r;
        r.header(24, RECORD_IN_USE);
        r.attr_data_runlist_vcn(120, 4, 0, 4096);
        r.attr_attrlist_resident({{ATTR_TYPE_DATA, 4, 25 | (1ULL << 48)}});
        r.finish();
        std::memcpy(img.sec(MFT_LCN + 48), r.buf.data(), RECORD_SIZE);
    }
    {
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
    wf.single_hop_attrlist = true; /* depth-1 parity */
    WalkStats stats;
    ASSERT_EQ(walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
              OK);
    tree::FileNode* big = node_named(tree, "bigfrag.txt");
    ASSERT_NE(big, nullptr);
    /* record 24's fragment only; record 25's fragment never merged */
    ASSERT_NE(big->props(), nullptr);
    EXPECT_EQ(big->props()->extents.size(), size_t(1));
    EXPECT_EQ(extent_bytes(big), u64(4 * CLUSTER));
    EXPECT_EQ(big->props()->extents[0].offset, u64(120 * CLUSTER));
}

TEST(NtfsVolume, DeletedAttrlistChaseFollowsChildrenAtDepth1) {
    /* M-008 premise CORRECTION, pinned: the og's pass 1 (FUN_180017390 -
     * the per-record processor that emits unconditionally, S-012's decode)
     * builds the child-record list for EVERY record, deleted included -
     * the old "deleted records are never chased" claim came from reading
     * FUN_18001acc0 (the DUMP walk) as a deleted-phase walk. Both modes
     * chase deleted records; only the DEPTH differs. */
    Image img;
    build_image(img);
    add_deleted_chain(img);
    MemDevice dev(img.raw(), img.geom());
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(new MemDevice(dev)), &err);
    ASSERT_NE(vol, nullptr) << err_text(err);

    /* default (multi-hop): full content - both fragments */
    {
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"a");
        auto* rd = tree.add_root(u"d");
        auto* rl = tree.add_root(u"l");
        WalkerFlags wf;
        WalkStats stats;
        ASSERT_EQ(
            walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
            OK);
        tree::FileNode* gone = node_named(tree, "gonefrag.txt");
        ASSERT_NE(gone, nullptr);
        EXPECT_EQ(gone->deleted_state(), tree::DeletedState::Deleted);
        EXPECT_EQ(extent_bytes(gone), u64(8 * CLUSTER)) << "the default chase recovers the chain";
        EXPECT_EQ(gone->props()->extents.size(), size_t(2));
    }
    /* parity (depth-1): the deleted record still chases its CHILDREN (the
     * corrected og model) - fragment 1 only, the grandchild lost */
    {
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"a");
        auto* rd = tree.add_root(u"d");
        auto* rl = tree.add_root(u"l");
        WalkerFlags wf;
        wf.single_hop_attrlist = true;
        WalkStats stats;
        ASSERT_EQ(
            walk_mft(vol.get(), nullptr, wf, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats),
            OK);
        tree::FileNode* gone = node_named(tree, "gonefrag.txt");
        ASSERT_NE(gone, nullptr);
        EXPECT_EQ(gone->deleted_state(), tree::DeletedState::Deleted);
        EXPECT_EQ(extent_bytes(gone), u64(4 * CLUSTER))
            << "depth-1 parity: children chased, grandchildren lost";
        EXPECT_EQ(gone->props()->extents.size(), size_t(1));
        EXPECT_EQ(gone->props()->extents[0].offset, u64(140 * CLUSTER));
    }
}
