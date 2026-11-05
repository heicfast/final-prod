/* PartitionSlice bounds + ReadErrorMap tests (R1.5, R1.6). */
#include <gtest/gtest.h>
#include "cr/storage/cr_slice.h"
#include "cr/cr_errmod.h"
#include <cstring>
#include "test_tmpdir.hpp" /* cr_test::read_repo_file (doc-truth pin) */

using namespace cr;

namespace {
class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(u64 bytes, u32 sector = 512) {
        data_.resize(bytes);
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = u8(i);
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = bytes / sector;
        geom_.total_bytes = geom_.sector_count * sector;
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
} // namespace

TEST(Slice, BoundsEnforced) {
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 20 * 512, &err);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(err, OK);
    /* read at partition start */
    u8 buf[512];
    EXPECT_EQ(slice->read_at(0, buf, 512), OK);
    EXPECT_EQ(buf[0], u8((10 * 512) & 0xFF));
    /* M-119 (supersedes the D-74 pin): a read crossing the end CLAMPS to
     * the in-bounds prefix, reads it, and zero-fills the tail - the og's
     * engine device contract (HFSPlusSearchDLL FUN_180009290 :7219-7264,
     * re-confirmed first-hand at the M-119 fix-time probe; the DRWUI twin
     * FUN_1400d0510 + the real-run log line "i64CountofSectors -
     * i64RealReadSector = 10175" at DRWUI.log:638). The og's own tail
     * memset is under-zeroed (sector-count bytes); ours zeroes the FULL
     * tail - the arithmetic defect stays unadopted (D-74's decode). */
    u8 cross[32];
    std::memset(cross, 0xAA, sizeof(cross));
    EXPECT_EQ(slice->read_at(20 * 512 - 16, cross, 32), OK);
    EXPECT_EQ(cross[0], u8((10 * 512 + 20 * 512 - 16) & 0xFF)) << "real prefix bytes";
    for (int i = 16; i < 32; ++i)
        EXPECT_EQ(cross[i], 0u) << "the past-end tail is zero-filled at byte " << i;
    /* read fully outside - still the parameter error (the og ladder) */
    EXPECT_EQ(slice->read_at(20 * 512 + 1, buf, 512), ERR_RANGE_OUT_OF_BOUNDS);
    /* read at very end edge - fine */
    EXPECT_EQ(slice->read_at(20 * 512 - 512, buf, 512), OK);
}

/* M-119 (diff-to-fix): the D-74 strict-refusal pin is SUPERSEDED - the
 * og's engine device clamps + zero-fills tail-crossing reads
 * (FUN_180009290 :7219-7264, the M-119 fix-time probe re-confirmed it
 * first-hand), the real-run DRWUI.log shows the clamp firing during
 * lost-partition probing (the og's scan flows ISSUE past-end reads and
 * proceed on the zero-filled result), and the CLI/UI whole-volume
 * refusal on table-length overrun was a divergence D-74 never claimed
 * to cover. The D-74 safety core KEPT: offset >= length and count >
 * length remain parameter errors (both sides refuse); the og's
 * under-zeroed tail memset stays unadopted (the FULL tail is zeroed). */
TEST(Slice, TailReadClampsAndZeroFills) {
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 20 * 512, &err);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(err, OK);
    u8 buf[512] = {0};
    std::memset(buf, 0xAA, sizeof(buf));
    /* the og semantics: the straddle SUCCEEDS with real prefix bytes and
     * a zero-filled past-end tail */
    EXPECT_EQ(slice->read_at(20 * 512 - 16, buf, 32), OK);
    EXPECT_EQ(buf[0], u8((10 * 512 + 20 * 512 - 16) & 0xFF)) << "16 real prefix bytes";
    for (int i = 16; i < 32; ++i)
        EXPECT_EQ(buf[i], 0u) << "tail byte " << i << " zero-filled";
    /* the parameter-error ladder stays hard */
    EXPECT_EQ(slice->read_at(20 * 512, buf, 512), ERR_RANGE_OUT_OF_BOUNDS)
        << "offset >= length: the parameter error (both sides refuse)";
    EXPECT_EQ(slice->read_at(0, buf, 20 * 512 + 1), ERR_RANGE_OUT_OF_BOUNDS)
        << "count > length: the parameter error (the og's count gate)";
    /* prefix-only reads still succeed */
    EXPECT_EQ(slice->read_at(20 * 512 - 512, buf, 512), OK);
}

TEST(Slice, StartMustBeAligned) {
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    EXPECT_EQ(PartitionSlice::create(dev.get(), 10 * 512 + 3, 20 * 512, &err), nullptr);
    EXPECT_EQ(err, ERR_BAD_PARAMETER);
}

TEST(Slice, RaggedTailAllowed) {
    /* lost-partition candidates have unaligned tails - allowed by design */
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 20 * 512 - 100, &err);
    ASSERT_NE(slice, nullptr);
    u8 buf[16];
    EXPECT_EQ(slice->read_at(20 * 512 - 100 - 16, buf, 16), OK);
}

TEST(DevMap, AddMergeSerialize) {
    ReadErrorMap m;
    m.add(100, 50, 3);  /* [100,150) */
    m.add(200, 10, 2);  /* [200,210) */
    m.add(145, 100, 5); /* [145,245): bridges BOTH -> single [100,245) */
    ASSERT_EQ(m.ranges().size(), 1u);
    EXPECT_EQ(m.ranges()[0].offset, 100u);
    EXPECT_EQ(m.ranges()[0].length, 145u); /* [100..245) */
    EXPECT_EQ(m.total_bytes(), 145u);

    std::vector<u8> out;
    m.serialize(out);
    u32 used = 0;
    ReadErrorMap m2 = ReadErrorMap::deserialize(out.data(), out.size(), &used);
    EXPECT_EQ(used, out.size());
    ASSERT_EQ(m2.ranges().size(), 1u);
    EXPECT_EQ(m2.ranges()[0].offset, 100u);
    EXPECT_EQ(m2.ranges()[0].length, 145u);
}

/* ---- M-049: the per-range module attribution (cr_errmod.h) ------------ */

TEST(DevMap, ModuleAttributionRecordedAndNewestWinsOnCoalesce) {
    /* the attribution recorded at failure time travels on the range; when
     * runs coalesce, the NEWEST constituent's attribution wins (the
     * record keeps the latest failure's context - the og's error-track
     * semantics). Every absorb in one add() involves the incoming range,
     * so "newest wins" is well-defined. */
    ReadErrorMap m;
    m.add(100, 50, 3, errmod::kFat);
    ASSERT_EQ(m.ranges().size(), 1u);
    EXPECT_EQ(m.ranges()[0].module, u32(errmod::kFat));

    /* disjoint range, disjoint attribution */
    m.add(400, 10, 3, errmod::kCarve);
    ASSERT_EQ(m.ranges().size(), 2u);
    EXPECT_EQ(m.ranges()[1].module, u32(errmod::kCarve));

    /* overlapping range with a DIFFERENT module: absorbed geometrically,
     * the incoming (newest) attribution wins, attempts take the max */
    m.add(120, 60, 5, errmod::kLost); /* [120,180) bridges into [100,150) */
    ASSERT_EQ(m.ranges().size(), 2u); /* the [400,410) range is untouched */
    EXPECT_EQ(m.ranges()[0].offset, 100u);
    EXPECT_EQ(m.ranges()[0].length, 80u); /* [100,180) */
    EXPECT_EQ(m.ranges()[0].attempts, 5u);
    EXPECT_EQ(m.ranges()[0].module, u32(errmod::kLost)) << "newest constituent's attribution";
    EXPECT_EQ(m.ranges()[1].module, u32(errmod::kCarve));

    /* merge() preserves attributions (feeds add() in insertion order) */
    ReadErrorMap other;
    other.add(500, 10, 2, errmod::kNtfs);
    m.merge(other);
    ASSERT_EQ(m.ranges().size(), 3u);
    EXPECT_EQ(m.ranges()[2].module, u32(errmod::kNtfs));
}

TEST(DevMap, ModuleNotSerializedWireStaysTrio) {
    /* the session wire format is the geometric trio (offset/length/
     * attempts) - unchanged in both directions by M-049; a deserialized
     * range is honestly UNATTRIBUTED (module 0), and re-serializing a
     * reloaded map produces the byte-identical trio stream. */
    ReadErrorMap m;
    m.add(100, 50, 3, errmod::kFat);
    m.add(200, 10, 2, errmod::kCarve);
    std::vector<u8> out;
    m.serialize(out);
    /* count + 2 ranges x 5 u32s = 11 u32s = 44 bytes: NO module words */
    EXPECT_EQ(out.size(), 44u);

    u32 used = 0;
    ReadErrorMap re = ReadErrorMap::deserialize(out.data(), out.size(), &used);
    EXPECT_EQ(used, out.size());
    ASSERT_EQ(re.ranges().size(), 2u);
    EXPECT_EQ(re.ranges()[0].module, 0u) << "unattributed after reload";
    EXPECT_EQ(re.ranges()[1].module, 0u);

    std::vector<u8> out2;
    re.serialize(out2);
    EXPECT_EQ(out, out2) << "round-trip byte-identical - the wire never carried modules";
}

/* ---- N-046: geometry refresh after removable-media change ------------- */

namespace {
/* A parent whose geometry can change mid-session (the removable-reader
 * swap model). Data backing store is irrelevant here; only geometry is
 * consumed by create/refresh. */
class MutableGeomDevice final : public IStorageDevice {
public:
    void set_geometry(u32 logical, u32 physical, u64 total_bytes) {
        geom_.logical_sector = logical;
        geom_.physical_sector = physical;
        geom_.total_bytes = total_bytes;
        geom_.sector_count = logical ? total_bytes / logical : 0;
    }
    u32 read_at(u64, void*, u32) override { return OK; }
    const Geometry& geometry() const override { return geom_; }

private:
    Geometry geom_{};
};
} // namespace

TEST(Slice, GeometryRefreshAfterMediaChange) {
    /* og FUN_140643870 (DRWUI_decompiled.c:1044889-1044895): the virtual
     * volume device re-derives its bytes-per-sector from the parent - the
     * media-change refresh our slice lacked (N-046). Pre-fix behavior
     * (fail-before by construction: no refresh path existed - the call
     * did not compile): the slice served the stale 512 snapshot forever. */
    MutableGeomDevice dev;
    dev.set_geometry(512, 512, u64(100) * 4096);
    u32 err = 0;
    auto slice = PartitionSlice::create(&dev, 10 * 4096, 20 * 4096, &err);
    ASSERT_NE(slice, nullptr);
    EXPECT_EQ(err, OK);
    EXPECT_EQ(slice->geometry().logical_sector, 512u);
    EXPECT_EQ(slice->geometry().sector_count, u64(20 * 4096) / 512);

    /* the reader swaps: same capacity, 4Kn media */
    dev.set_geometry(4096, 4096, u64(100) * 4096);
    /* the stale snapshot is still served until an explicit refresh (the
     * finding's premise - geometry was frozen at creation) */
    EXPECT_EQ(slice->geometry().logical_sector, 512u);
    ASSERT_EQ(slice->refresh_geometry(), OK);
    EXPECT_EQ(slice->geometry().logical_sector, 4096u);
    EXPECT_EQ(slice->geometry().physical_sector, 4096u);
    EXPECT_EQ(slice->geometry().sector_count, u64(20 * 4096) / 4096);
    EXPECT_EQ(slice->geometry().total_bytes, u64(20 * 4096));
    /* reads are unaffected: the bound was always in bytes */
    u8 buf[16];
    EXPECT_EQ(slice->read_at(0, buf, sizeof(buf)), OK);
}

TEST(Slice, GeometryRefreshRefusesInvalidatingChange) {
    /* Registered hardening over the og's blind store (FUN_140643870
     * returns 1 unconditionally): a media change that invalidates the
     * slice is REFUSED and the snapshot stays untouched. */
    MutableGeomDevice dev;
    dev.set_geometry(512, 512, u64(100) * 4096);
    u32 err = 0;
    auto slice = PartitionSlice::create(&dev, 9 * 4096, 20 * 4096, &err);
    ASSERT_NE(slice, nullptr);

    /* (a) new sector size misaligns the start: 9*4096 % 8192 == 4096 */
    dev.set_geometry(8192, 8192, u64(100) * 4096);
    EXPECT_EQ(slice->refresh_geometry(), ERR_BAD_PARAMETER);
    EXPECT_EQ(slice->geometry().logical_sector, 512u)
        << "refused refresh leaves the snapshot unchanged";

    /* (b) parent shrank below the slice end */
    dev.set_geometry(512, 512, 9 * 4096 + 512);
    EXPECT_EQ(slice->refresh_geometry(), ERR_RANGE_OUT_OF_BOUNDS);
    EXPECT_EQ(slice->geometry().logical_sector, 512u);
    EXPECT_EQ(slice->geometry().sector_count, u64(20 * 4096) / 512);

    /* (c) degenerate zero sector size */
    dev.set_geometry(0, 0, u64(100) * 4096);
    EXPECT_EQ(slice->refresh_geometry(), ERR_BAD_PARAMETER);
    EXPECT_EQ(slice->geometry().logical_sector, 512u);
}

/* ---- M-034 doc-truth pin: the NAS scope exclusion ---------------------- */

TEST(Storage, ParityDeltasListsNasExclusion) {
    /* M-034's disposition: NAS/remote scanning is EXCLUDED from the core's
     * device scope, registered as PARITY-DELTAS D-88 (the finding existed
     * to force exactly this registration - the capability was never
     * dispositioned anywhere before). This pin fails when either artifact
     * drifts: the D-88 row or the cr_device.h divergence block. Reopening
     * the row (a corpus decode of the EuMrx scan's deleted-file yield)
     * must update both in one commit. */
    const auto deltas = cr_test::read_repo_text("docs/PARITY-DELTAS.md");
    ASSERT_FALSE(deltas.empty());
    EXPECT_NE(std::string::npos, deltas.find("D-88"))
        << "PARITY-DELTAS must keep the NAS-scope-exclusion row";
    EXPECT_NE(std::string::npos, deltas.find("excluded from the core's device scope"))
        << "D-88 must state the exclusion plainly";
    EXPECT_NE(std::string::npos, deltas.find("CRemoteDevice"))
        << "D-88 must cite the og's remote-device layer (the M-034 evidence)";
    EXPECT_NE(std::string::npos, deltas.find("EuMrx"))
        << "D-88 must cite the redirector (the M-034 evidence)";

    const auto hdr = cr_test::read_repo_text("include/cr/storage/cr_device.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(std::string::npos, hdr.find("REGISTERED DIVERGENCE (M-034, PARITY-DELTAS D-88)"))
        << "cr_device.h must carry the M-034 divergence block";
}

/* ===================================================================== */
/* M-119 (diff-to-fix): the og's partition-boundary semantics -        */
/* creation tolerant (an over-long declared window clamps to the       */
/* device end), per-read tolerant (a straddling read clamps + zero-    */
/* fills + succeeds). The engine-side device confirmed first-hand at   */
/* the fix-time probe (HFSPlusSearchDLL FUN_180009290 :7219-7264) and  */
/* the DRWUI twin (FUN_1400d0510 :183633-66) + the real-run log        */
/* (DRWUI.log:638). The CLI/UI whole-volume refusals on table-length   */
/* overrun are gone (the clamped open + the surfaced delta).           */
/* ===================================================================== */

TEST(Slice, CreateClampsOverlongDeclaredLength) {
    /* the finding's CLI/UI leg: a table entry declaring 50 sectors past
     * the 100-sector device (start 10*512, declared 90*512 -> would end
     * at 100*512 exactly... use a true overrun: declared 95*512 from
     * 10*512 ends at 105*512 > the device's 100*512). Fail-before:
     * nullptr + ERR_RANGE_OUT_OF_BOUNDS (the whole volume refused). */
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 95 * 512, &err);
    ASSERT_NE(slice, nullptr) << "the over-long declared window opens clamped";
    EXPECT_EQ(err, OK);
    EXPECT_EQ(slice->length(), 90u * 512) << "clamped to the device end (100-10 sectors";
    EXPECT_EQ(slice->declared_bytes(), 95u * 512) << "the declared window is recorded";
    EXPECT_EQ(slice->clamp_delta(), 5u * 512);
    EXPECT_TRUE(slice->was_clamped());
    /* the clamped tail reads real bytes up to the device end */
    u8 buf[512];
    EXPECT_EQ(slice->read_at(90u * 512 - 512, buf, 512), OK);
    /* a straddle past the CLAMPED end zero-fills */
    u8 cross[32];
    std::memset(cross, 0xAA, sizeof(cross));
    EXPECT_EQ(slice->read_at(90u * 512 - 16, cross, 32), OK);
    for (int i = 16; i < 32; ++i)
        EXPECT_EQ(cross[i], 0u);
}

TEST(Slice, CreateStartBeyondDeviceStillRefused) {
    /* N-046's honest posture kept: only the LENGTH clamps - a start past
     * the device is genuinely unopenable */
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    EXPECT_EQ(PartitionSlice::create(dev.get(), 100 * 512, 10 * 512, &err), nullptr);
    EXPECT_EQ(err, ERR_RANGE_OUT_OF_BOUNDS);
    EXPECT_EQ(PartitionSlice::create(dev.get(), 120 * 512, 10 * 512, &err), nullptr);
    EXPECT_EQ(err, ERR_RANGE_OUT_OF_BOUNDS);
}

TEST(Slice, ExactFitNotClamped) {
    /* the healthy case: no clamp telemetry fires */
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    auto slice = PartitionSlice::create(dev.get(), 10 * 512, 90 * 512, &err);
    ASSERT_NE(slice, nullptr);
    EXPECT_FALSE(slice->was_clamped());
    EXPECT_EQ(slice->clamp_delta(), 0u);
    EXPECT_EQ(slice->length(), slice->declared_bytes());
}

TEST(Slice, OrchestratorTargetClampScansRaggedTable) {
    /* the end-to-end shape the CLI/UI legs consume: a whole-disk table
     * entry whose length overruns the media now produces a CLAMPED
     * target that probes content (the og scans such volumes) instead of
     * a skipped volume. */
    auto dev = std::make_unique<MemDevice>(u64(100) * 512);
    u32 err = 0;
    /* the table declares a partition at LBA 10 spanning 95 sectors on a
     * 100-sector disk: 10+95 = 105 > 100 */
    auto tgt = PartitionSlice::create(dev.get(), 10 * 512, 95 * 512, &err);
    ASSERT_NE(tgt, nullptr);
    EXPECT_TRUE(tgt->was_clamped());
    EXPECT_EQ(tgt->geometry().sector_count, 90u);
    /* the clamped target is a fully functional device for the probe
     * family: reads inside succeed, straddles zero-fill */
    u8 buf[512];
    EXPECT_EQ(tgt->read_at(0, buf, 512), OK);
    EXPECT_EQ(tgt->read_at(tgt->length() - 512, buf, 512), OK);
    u8 cross[64];
    std::memset(cross, 0x55, sizeof(cross));
    EXPECT_EQ(tgt->read_at(tgt->length() - 16, cross, 64), OK);
    for (int i = 16; i < 64; ++i)
        EXPECT_EQ(cross[i], 0u) << "byte " << i;
}
