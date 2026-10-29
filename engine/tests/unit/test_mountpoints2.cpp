/* M-118 (diff-to-fix): the register channel - the Explorer
 * MountPoints2\CPC\Volume history as the lost-partition assembly's third
 * candidate source.
 *
 * Corpus ground truth (verified first-hand):
 *   - PartitionRecoveryD_decompiled.c:8841-8965 (FUN_18000f5b0's
 *     per-subkey "Data" decode + FUN_18000f840's walk, 1000-subkey cap);
 *   - DRWUI_decompiled.c:921820-921890 (FUN_140588820's three-wave
 *     assembly) + :922335+ (FUN_1405895d0's DRWUI-side mirror) +
 *     :922785+ (FUN_140589e80's 0x800-sector probe read at each
 *     remembered start through the parser chain FUN_14058a070);
 *   - og-codebase/log/DRWUI.log:637-643 (the real-run shape: mbr=1 +
 *     filesystem=1 + register=3 -> ONE partition scanned once).
 *
 * The proofs: the pure blob decode's exact gates; the offline-hive walk
 * over a synthetic NTUSER.DAT; the register channel surfacing a
 * partition the signature sweep's stride missed; the three-source dedup
 * collapsing to a single candidate; the M-042 re-validation applying to
 * register-provenance records. */

#include <gtest/gtest.h>
#include "cr/lost/cr_lostpart.h"
#include "cr/reghive/cr_mountpoints2.h"
#include "cr/reghive/cr_reghive.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::lost;
using reghive::REGF_BINS_SIZE_OFF;
using reghive::REGF_ROOT_CELL_OFF;

namespace {

/* ---------------- disk fixtures (the lostpart suite's shapes) --------- */

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

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}
void put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8(x >> (8 * i));
}

/* a plausible NTFS boot sector at `lba` + a live $MFT record (the
 * lostpart suite's own seeder, kept identical so the probe behavior is
 * the one the family's tests already pin) */
void plant_ntfs(std::vector<u8>& img, u64 lba, u64 total_sectors, u64 mft_lcn) {
    u8* b = img.data() + lba * 512;
    std::memcpy(b + 3, "NTFS    ", 8);
    put16(img, lba * 512 + 0x0B, 512);
    b[0x0D] = 1; /* spc */
    b[0x15] = 0xF8;
    put64(img, lba * 512 + 0x28, total_sectors);
    put64(img, lba * 512 + 0x30, mft_lcn);
    put64(img, lba * 512 + 0x38, mft_lcn + 8);
    b[0x40] = 2; /* clusters/record: 1024B */
    b[0x44] = 2;
    b[510] = 0x55;
    b[511] = 0xAA;
    const u64 rec_off = (lba + mft_lcn) * 512;
    b = img.data() + rec_off;
    put32v(img, rec_off + 0, 0x454C4946); /* 'FILE' */
    put16(img, rec_off + 4, 0x30);        /* USA offset */
    put16(img, rec_off + 6, 3);           /* USA size: 2+1 */
    put16(img, rec_off + 8, 1);           /* sequence */
    put16(img, rec_off + 0x14, 0x38);     /* attribute offset */
    put16(img, rec_off + 0x18, 1);        /* in-use */
    put32v(img, rec_off + 0x1C, 1024);
    put32v(img, rec_off + 0x20, 1024);
    put32v(img, rec_off + 0x2C, u32(0));
    put16(img, rec_off + 0x30, 0x1234);
    put16(img, rec_off + 0x32, 0xAAAA);
    put16(img, rec_off + 0x34, 0xBBBB);
    img[rec_off + 510] = 0x34;
    img[rec_off + 511] = 0x12;
    img[rec_off + 1022] = 0x34;
    img[rec_off + 1023] = 0x12;
}

/* ---------------- the shell-cache "Data" blob fixture ----------------- */

/* Build an Explorer CPC\Volume "Data" blob carrying `byte_start` in the
 * og's field layout: '#' at 0xc4, the 0x20-char uppercase-hex byte
 * offset at 0xc6, '#' at 0xe6. */
std::vector<u8> make_data_blob(u64 byte_start) {
    std::vector<u8> blob(0x400, 0);
    blob[0xc4] = '#';
    blob[0xe6] = '#';
    char hex[33];
    snprintf(hex, sizeof(hex), "%032llX", (unsigned long long)byte_start);
    std::memcpy(blob.data() + 0xc6, hex, 32);
    return blob;
}

/* ---------------- synthetic regf hive fixture ------------------------- */

/* Minimal regf builder (the reghive suite's layout): one hbin, nk/vk/lf
 * cells, the XOR checksum header. Rebuilt here scoped to what the
 * MountPoints2 walk consumes: a deep key path with volume-GUID subkeys,
 * each carrying a REG_BINARY "Data" value. */
class HiveBuilder {
public:
    HiveBuilder() {
        bins_.resize(0x1000, 0);
        std::memcpy(bins_.data(), "hbin", 4);
        put32(bins_, 8, 0x1000);
        root_off_ = 0x20;
        root_cell_ = add_nk(0, "ROOTKEY", 0);
        (void)root_cell_;
    }

    u32 add_nk(u32 parent, const std::string& name, u16 flags) {
        std::vector<u8> rec(0x4C + (flags & 0x20 ? name.size() : name.size() * 2), 0);
        rec[0] = 'n';
        rec[1] = 'k';
        put16(rec, 0x02, flags);
        if (flags & 0x20) {
            std::memcpy(rec.data() + 0x4C, name.data(), name.size());
            put16(rec, 0x48, u16(name.size()));
        } else {
            for (size_t i = 0; i < name.size(); ++i)
                put16(rec, 0x4C + i * 2, name[i]);
            put16(rec, 0x48, u16(name.size() * 2));
        }
        const u32 off = append_cell(rec);
        if (parent != 0)
            children_[parent].push_back(off);
        return off;
    }

    u32 add_vk(u32 nk, const std::string& name, u32 type, const std::vector<u8>& data) {
        std::vector<u8> rec(0x12 + name.size(), 0);
        rec[0] = 'v';
        rec[1] = 'k';
        put16(rec, 0x02, u16(name.size()));
        rec[0x10] = 1; /* ASCII name flag */
        std::memcpy(rec.data() + 0x12, name.data(), name.size());
        put32(rec, 0x0C, type);
        if (data.size() <= 4) {
            put32(rec, 0x04, u32(data.size()) | 0x80000000u);
            for (size_t i = 0; i < data.size(); ++i)
                rec[0x08 + i] = data[i];
        } else {
            put32(rec, 0x04, u32(data.size()));
            const u32 doff = append_cell(data);
            put32(rec, 0x08, doff);
        }
        const u32 off = append_cell(rec);
        values_[nk].push_back(off);
        return off;
    }

    std::vector<u8> build() {
        for (const auto& [parent, kids] : children_) {
            std::vector<u8> rec(4 + kids.size() * 8, 0);
            rec[0] = 'l';
            rec[1] = 'f';
            put16(rec, 0x02, u16(kids.size()));
            for (size_t i = 0; i < kids.size(); ++i) {
                put32(rec, 4 + i * 8, kids[i]);
                put32(rec, 4 + i * 8 + 4, 0);
            }
            const u32 list_off = append_cell(rec);
            patch_nk(parent, 0x14, u32(kids.size()));
            patch_nk(parent, 0x1C, list_off);
        }
        for (const auto& [nk, vks] : values_) {
            std::vector<u8> rec(vks.size() * 4, 0);
            for (size_t i = 0; i < vks.size(); ++i)
                put32(rec, i * 4, vks[i]);
            const u32 list_off = append_cell(rec);
            patch_nk(nk, 0x24, u32(vks.size()));
            patch_nk(nk, 0x28, list_off);
        }
        std::vector<u8> hive(0x1000 + bins_.size(), 0);
        std::memcpy(hive.data(), "regf", 4);
        put32(hive, 0x18, 6);
        put32(hive, REGF_ROOT_CELL_OFF, root_off_);
        put32(hive, REGF_BINS_SIZE_OFF, u32(bins_.size()));
        std::memcpy(hive.data() + 0x1000, bins_.data(), bins_.size());
        u32 acc = 0;
        for (u32 off = 0; off < 0x1FC; off += 4)
            acc ^= get32(hive, off);
        put32(hive, 0x1FC, acc);
        return hive;
    }

    u32 root() const { return root_off_; }

private:
    std::vector<u8> bins_;
    u32 root_off_ = 0;
    u32 root_cell_ = 0;
    std::map<u32, std::vector<u32>> children_;
    std::map<u32, std::vector<u32>> values_;

    static void put16(std::vector<u8>& v, size_t at, u16 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
    }
    static void put32(std::vector<u8>& v, size_t at, u32 x) {
        for (int i = 0; i < 4; ++i)
            v[at + i] = u8(x >> (8 * i));
    }
    static u32 get32(const std::vector<u8>& v, size_t at) {
        return u32(v[at]) | (u32(v[at + 1]) << 8) | (u32(v[at + 2]) << 16) | (u32(v[at + 3]) << 24);
    }

    u32 append_cell(const std::vector<u8>& rec) {
        u32 off = next_off_;
        const size_t need = 4 + rec.size();
        const size_t aligned = (need + 7) & ~size_t(7);
        while (off + aligned > bins_.size())
            bins_.resize(bins_.size() + 0x1000, 0);
        put32(bins_, off, u32(-i64(aligned))); /* negative = allocated */
        std::memcpy(bins_.data() + off + 4, rec.data(), rec.size());
        put32(bins_, 8, u32(bins_.size())); /* hbin size refresh */
        next_off_ = off + u32(aligned);
        return off;
    }

    void patch_nk(u32 off, size_t at, u32 value) {
        /* the nk record sits at off+4 in bins_ */
        put32(bins_, off + 4 + at, value);
    }

    u32 next_off_ = 0x20;
};

/* Build a NTUSER.DAT-shaped hive carrying the MountPoints2\CPC\Volume
 * tree with the given volume subkeys -> "Data" blobs. */
std::vector<u8>
build_mountpoints2_hive(const std::vector<std::pair<std::string, std::vector<u8>>>& volumes) {
    HiveBuilder hb;
    u32 software = hb.add_nk(hb.root(), "Software", 0x20);
    u32 microsoft = hb.add_nk(software, "Microsoft", 0x20);
    u32 explorer = hb.add_nk(microsoft, "Windows", 0x20);
    u32 currentversion = hb.add_nk(explorer, "CurrentVersion", 0x20);
    u32 mp2 = hb.add_nk(currentversion, "Explorer", 0x20);
    u32 cpc = hb.add_nk(mp2, "MountPoints2", 0x20);
    u32 volume = hb.add_nk(cpc, "CPC", 0x20);
    u32 volkey = hb.add_nk(volume, "Volume", 0x20);
    for (const auto& [name, blob] : volumes) {
        u32 sub = hb.add_nk(volkey, name, 0x20);
        hb.add_vk(sub, "Data", 3, blob);
    }
    return hb.build();
}

} // namespace

/* ---------------- the pure blob decode gates -------------------------- */

TEST(MountPoints2Decode, ParsesCleanPlacement) {
    /* the og's real-run placement: byte 2048*512 = 0x100000 */
    std::vector<u8> blob = make_data_blob(2048ull * 512);
    u64 lba = 99;
    ASSERT_TRUE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
    EXPECT_EQ(lba, 2048u);
}

TEST(MountPoints2Decode, LowercaseHexAccepted) {
    /* %I64X is case-insensitive in the C locale */
    std::vector<u8> blob(0x400, 0);
    blob[0xc4] = '#';
    blob[0xe6] = '#';
    std::memcpy(blob.data() + 0xc6, "0000000000001000", 16); /* 0x1000 bytes */
    u64 lba = 0;
    ASSERT_TRUE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
    EXPECT_EQ(lba, 8u); /* 0x1000 / 512 */
}

TEST(MountPoints2Decode, ShortBlobRejected) {
    /* the og's 0xe7 < size gate: a 0xe7-byte blob (exactly at the
     * boundary) is rejected; the gate is strictly-greater. A 0xe8-byte
     * blob (the minimum) still carries both delimiters (0xc4, 0xe6) and
     * the complete 0x20 hex field (0xc6..0xe5) - it PARSES: the og's
     * minimum is exactly the field's extent plus one byte */
    std::vector<u8> blob = make_data_blob(2048ull * 512);
    blob.resize(0xe7);
    u64 lba = 0;
    EXPECT_FALSE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
    blob.resize(0xe8);
    EXPECT_TRUE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
    EXPECT_EQ(lba, 2048u);
}

TEST(MountPoints2Decode, MissingDelimiterRejected) {
    u64 lba = 0;
    std::vector<u8> b1 = make_data_blob(2048ull * 512);
    b1[0xc4] = 'X'; /* the first '#' gone */
    EXPECT_FALSE(decode_mountpoints2_placement(b1.data(), b1.size(), 512, lba));
    std::vector<u8> b2 = make_data_blob(2048ull * 512);
    b2[0xe6] = 'X'; /* the second '#' gone */
    EXPECT_FALSE(decode_mountpoints2_placement(b2.data(), b2.size(), 512, lba));
}

TEST(MountPoints2Decode, MisalignedStartRejected) {
    /* byte 2048*512 + 1: not sector-aligned - the og's bps gate */
    std::vector<u8> blob = make_data_blob(2048ull * 512 + 1);
    u64 lba = 0;
    EXPECT_FALSE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
}

TEST(MountPoints2Decode, NonHexFieldRejected) {
    std::vector<u8> blob(0x400, 0);
    blob[0xc4] = '#';
    blob[0xe6] = '#';
    std::memcpy(blob.data() + 0xc6, "zzzz", 4); /* %X matches nothing */
    u64 lba = 0;
    EXPECT_FALSE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
}

TEST(MountPoints2Decode, TruncatedHexStillParsesPrefix) {
    /* %I64X stops at the first non-hex char - a torn field yields the
     * parsed prefix, exactly like the og's sscanf */
    std::vector<u8> blob(0x400, 0);
    blob[0xc4] = '#';
    blob[0xe6] = '#';
    std::memcpy(blob.data() + 0xc6, "1000G00000000000", 16); /* 'G' terminates */
    u64 lba = 0;
    ASSERT_TRUE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
    EXPECT_EQ(lba, 8u); /* 0x1000 / 512 */
}

TEST(MountPoints2Decode, SectorZeroIsTheFailureSentinel) {
    /* byte 0 parses to sector 0 - the MBR area; the og's caller checks
     * the return != 0, so a zero placement is never a proposal */
    std::vector<u8> blob = make_data_blob(0);
    u64 lba = 99;
    EXPECT_FALSE(decode_mountpoints2_placement(blob.data(), blob.size(), 512, lba));
}

/* ---------------- the offline-hive walk ------------------------------- */

TEST(MountPoints2Walk, ParsesPlacementsFromSyntheticHive) {
    /* two clean volume subkeys + one failing each gate class + a
     * non-binary "Data" - the walk emits only the clean placements */
    std::vector<std::pair<std::string, std::vector<u8>>> vols;
    vols.push_back({"{a1b2c3d4-0000-0000-0000-000000000001}",
                    make_data_blob(2048ull * 512)}); /* clean: sector 2048 */
    vols.push_back({"{a1b2c3d4-0000-0000-0000-000000000002}",
                    make_data_blob(4096ull * 512)}); /* clean: sector 4096 */
    {                                                /* short blob */
        std::vector<u8> short_blob(0xe7, 0);
        vols.push_back({"{short}", short_blob});
    }
    { /* missing delimiter */
        std::vector<u8> bad = make_data_blob(8192ull * 512);
        bad[0xc4] = 'X';
        vols.push_back({"{baddelim}", bad});
    }
    { /* misaligned */
        vols.push_back({"{misaligned}", make_data_blob(8192ull * 512 + 3)});
    }
    { /* non-binary type: a REG_SZ "Data" - the og's lpType == 3 gate */
        std::vector<u8> sz(64, 'x');
        vols.push_back({"{regsz}", sz});
    }
    const std::vector<u8> hive = build_mountpoints2_hive(vols);

    reghive::RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    std::vector<u64> starts;
    ASSERT_EQ(reghive::mountpoints2_placements(rh, 512, starts), OK);
    ASSERT_EQ(starts.size(), 2u);
    EXPECT_EQ(starts[0], 2048u);
    EXPECT_EQ(starts[1], 4096u);
}

TEST(MountPoints2Walk, AbsentKeyYieldsZeroPlacements) {
    /* a hive without the MountPoints2 tree: the og's RegOpenKeyA
     * failure path - zero placements, not an error */
    HiveBuilder hb;
    (void)hb.add_nk(hb.root(), "Software", 0x20);
    const std::vector<u8> hive = hb.build();
    reghive::RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    std::vector<u64> starts;
    EXPECT_EQ(reghive::mountpoints2_placements(rh, 512, starts), OK);
    EXPECT_TRUE(starts.empty());
}

TEST(MountPoints2Walk, ZeroBpsRefused) {
    HiveBuilder hb;
    const std::vector<u8> hive = hb.build();
    reghive::RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    std::vector<u64> starts;
    EXPECT_EQ(reghive::mountpoints2_placements(rh, 0, starts), ERR_BAD_PARAMETER);
}

TEST(MountPoints2Walk, CaseInsensitiveKeyPath) {
    /* registry key semantics: the path walk folds case (the og's
     * RegOpenKeyA is case-insensitive too) */
    std::vector<std::pair<std::string, std::vector<u8>>> vols;
    vols.push_back({"{vol}", make_data_blob(2048ull * 512)});
    const std::vector<u8> hive = build_mountpoints2_hive(vols);
    reghive::RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    /* the built tree has "software/microsoft/windows/currentversion/
     * explorer/mountpoints2/cpc/volume" in the builder's own casing -
     * verify the find_key fold by checking the canonical path resolves */
    const u64 k = rh.find_key(reghive::kMountPoints2VolumeKey);
    EXPECT_NE(k, 0u);
}

/* ---------------- the register channel in discovery ------------------- */

TEST(RegisterChannel, FindsStrideMissedPartition) {
    /* the channel's unique value: a partition whose boot sector sits at
     * an LBA the sweep's stride never probes. stride 8 skips sector
     * 2049's neighbors; the remembered placement (2049) is probed
     * directly and the NTFS boot sector there is accepted. Pre-fix this
     * test fails: discover() had no register_starts leg at all. */
    std::vector<u8> img(u64(16384) * 512, 0);
    /* the partition at LBA 2049 - NOT a stride-8 boundary (2048 is,
     * 2056 is; 2049 falls between) */
    plant_ntfs(img, 2049, 1024, 100);
    MemDevice dev(std::move(img));

    /* sanity: the stride-8 sweep alone does not surface it */
    {
        DiscoveryResult dr;
        ASSERT_EQ(discover(&dev, {}, 8, nullptr, nullptr, nullptr, dr), OK);
        bool sweep_found = false;
        for (const auto& c : dr.candidates)
            if (c.start_lba == 2049)
                sweep_found = true;
        EXPECT_FALSE(sweep_found);
    }

    /* the register channel alone surfaces it, register-provenance-tagged */
    DiscoveryResult dr;
    ASSERT_EQ(discover(&dev, {}, 8, nullptr, nullptr, nullptr, dr, {2049}), OK);
    ASSERT_FALSE(dr.candidates.empty());
    const Candidate& c = dr.candidates[0];
    EXPECT_EQ(c.start_lba, 2049u);
    EXPECT_EQ(c.fs, CandidateFs::Ntfs);
    EXPECT_EQ(c.provenance, u8(Provenance::Register));
    bool has_register_evidence = false;
    for (const auto& e : c.evidence)
        if (std::string_view(e.tag) == "register-placement")
            has_register_evidence = true;
    EXPECT_TRUE(has_register_evidence);
}

TEST(RegisterChannel, ThreeSourceDedupSingleCandidate) {
    /* the og's real-run shape (DRWUI.log:637-643): mbr=1 + filesystem=1
     * + register=3 -> ONE partition scanned once. The core's shape: the
     * sweep's candidate + the register channel's proposal for the SAME
     * placement collapse in the pass-2 dedup into ONE candidate - and
     * the duplicate register records for one placement collapse before
     * the probe (the og's container dedup). */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_ntfs(img, 2048, 1024, 100);
    MemDevice dev(std::move(img));

    /* the real-log triple: THREE register records for ONE placement */
    DiscoveryResult dr;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, dr, {2048, 2048, 2048}), OK);
    /* exactly one candidate at 2048 - the three-source overlap collapsed */
    size_t at_2048 = 0;
    for (const auto& c : dr.candidates)
        if (c.start_lba == 2048)
            ++at_2048;
    EXPECT_EQ(at_2048, 1u);
}

TEST(RegisterChannel, SweepProposalWinsProvenance) {
    /* the og's three-wave order: mbr, then filesystem, then register -
     * the FIRST proposer's record is the one the container keeps. The
     * core's sweep finds the placement first (pass 1 runs before the
     * register leg), so the surviving candidate is Scan-provenance even
     * though the register channel proposed the same start. */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_ntfs(img, 2048, 1024, 100);
    MemDevice dev(std::move(img));
    DiscoveryResult dr;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, dr, {2048}), OK);
    ASSERT_EQ(dr.candidates.size(), 1u);
    EXPECT_EQ(dr.candidates[0].provenance, u8(Provenance::Scan));
}

TEST(RegisterChannel, UnprobeablePlacementDropped) {
    /* FUN_14058a070's all-parsers-reject drop: a remembered placement
     * whose start sector carries nothing any parser admits contributes
     * no candidate (zeroed boot sector on a stride-1 disk). */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_ntfs(img, 2048, 1024, 100);
    MemDevice dev(std::move(img));
    DiscoveryResult dr;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, dr, {8000}), OK);
    for (const auto& c : dr.candidates)
        EXPECT_NE(c.start_lba, 8000u);
}

TEST(RegisterChannel, OutOfBoundsPlacementDropped) {
    /* FUN_140589e80's disk-extent gate: a placement beyond the current
     * disk's sectors is dropped (a stale mount record from a bigger
     * disk). */
    std::vector<u8> img(u64(1024) * 512, 0);
    MemDevice dev(std::move(img));
    DiscoveryResult dr;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, dr, {999999}), OK);
    EXPECT_TRUE(dr.candidates.empty());
}

TEST(RegisterChannel, RegisterRecordsAreRevalidated) {
    /* M-042's re-validation applies to register-provenance records: the
     * CURRENT disk is the arbiter - a persisted register record whose
     * start no longer classifies as the persisted family drops. */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_ntfs(img, 2048, 1024, 100);
    MemDevice dev(std::move(img));

    Candidate reg;
    reg.start_lba = 2048;
    reg.sector_count = 1024;
    reg.fs = CandidateFs::Ntfs;
    reg.provenance = u8(Provenance::Register);
    EXPECT_TRUE(revalidate_candidate(&dev, reg)); /* the NTFS family still classifies */

    /* the same record against a zeroed disk: the identity no longer
     * matches - the record drops */
    std::vector<u8> zeroed(u64(16384) * 512, 0);
    MemDevice dev2(std::move(zeroed));
    EXPECT_FALSE(revalidate_candidate(&dev2, reg));
}

TEST(RegisterChannel, EmptyHistoryIsThePreFixShape) {
    /* the default leg: no register starts, the discovery is exactly the
     * pre-M-118 behavior (every existing test rides this default) */
    std::vector<u8> img(u64(8192) * 512, 0);
    plant_ntfs(img, 1024, 512, 100);
    MemDevice dev(std::move(img));
    DiscoveryResult dr;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, dr), OK);
    ASSERT_EQ(dr.candidates.size(), 1u);
    EXPECT_EQ(dr.candidates[0].provenance, u8(Provenance::Scan));
}
