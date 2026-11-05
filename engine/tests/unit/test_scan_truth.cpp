/* test_scan_truth.cpp - the read-truth contract tests (the fake-scan fix;
 * the driver-era doc set's section 7, PARITY-DELTAS D-DRV-6).
 *
 * The defect class these pin (user report: "the scan never works and
 * starts from 40% and shows no progress - it looks like fake scanning"):
 * Skip-mode zero-fill makes every failing read LOOK successful downstream,
 * stages complete over zeroed data (the quick stage's 40% weight lands
 * instantly when the family is Unknown because the probe was BLIND), and
 * the pre-fix verdict was "scan complete, 0 files found" + exit 0 - the
 * product appeared to work while reading nothing at all.
 *
 * The contract now:
 *   - a fully-unreadable target REFUSES with ERR_READ_FAILED (never a
 *     fake-complete);
 *   - partial damage keeps the salvage semantics (og parity) AND surfaces
 *     the truth: bytes_unreadable == the failed coverage;
 *   - a blind probe is flagged (probe_unreadable) - family=Unknown from an
 *     unreadable header is distinguishable from filesystem-absent.
 *
 * The devices here are deterministic in-memory fakes (the same harness
 * class test_orchestrator.cpp uses) - the Linux lane pins the CONTRACT;
 * the real-transport behavior is proven on the Windows runner against the
 * real disks (the staged scenarios, docs/CI-SCENARIOS.md).
 */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include "cr/cr_error.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::orchestrator;

namespace {

/* A device whose reads fail for a configurable set of byte ranges and
 * succeed (zero data) everywhere else. read failures return
 * ERR_READ_FAILED exactly like the real transports do on exhausted
 * retries - the retry pipe's Skip mode is what turns them into zero-fill
 * downstream, which is the production posture under test. */
class PartialFailDevice final : public IStorageDevice {
public:
    PartialFailDevice(u64 bytes, std::vector<std::pair<u64, u64>> fail_ranges)
        : fail_(std::move(fail_ranges)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = bytes / 512;
        geom_.total_bytes = bytes;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > geom_.total_bytes || len > geom_.total_bytes - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        for (const auto& r : fail_) {
            if (off < r.first + r.second && r.first < off + len)
                return ERR_READ_FAILED; /* any overlap with a dead range fails */
        }
        std::memset(buf, 0, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<std::pair<u64, u64>> fail_;
    Geometry geom_;
};

ScanConfig truth_cfg() {
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = true;
    cfg.whole_disk = false;
    cfg.threads = 1; /* deterministic; the pipe is the subject, not the pool */
    return cfg;
}

} // namespace

TEST(ScanTruth, FullyUnreadableVolumeRefusesInsteadOfFakeCompleting) {
    /* THE fake-scan pin: every read on the target fails. Pre-fix, the
     * cascade was: probe read fails -> Skip zero-fill -> family Unknown ->
     * no engine runs -> quick completes instantly (the 40% weight lands
     * with zero work) -> deep skipped (quick_done_) -> carve walks
     * zero-filled windows -> "scan complete: 0 file(s)" + exit 0.
     * Post-fix: ERR_READ_FAILED - the refusal is the whole verdict. */
    const u64 kSize = 512ull * 1024; /* 512 KiB: small enough for the
                                      * bisection grind to stay fast */
    PartialFailDevice dev(kSize, {{0, kSize}});
    tree::ResultTree tree;
    ScanConfig cfg = truth_cfg();
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    EXPECT_EQ(rc, u32(ERR_READ_FAILED)) << "a fully-unreadable target must refuse, not "
                                           "fake-complete; got "
                                        << err_text(rc);
    EXPECT_EQ(stats.files_found, 0u);
    EXPECT_EQ(stats.carved_found, 0u);
    /* the truth fields carry the evidence */
    EXPECT_TRUE(stats.probe_unreadable) << "the blind probe must be flagged";
    EXPECT_GE(stats.bytes_unreadable + 64ull * 1024, kSize)
        << "coverage must span (essentially) the whole slice";
    EXPECT_FALSE(stats.errors.ranges().empty());
}

TEST(ScanTruth, PartialDamageKeepsSalvageSemanticsAndSurfacesCoverage) {
    /* og parity boundary: a target with a dead MIDDLE range but readable
     * data around it stays a salvage scan (OK, Skip-mode semantics) - the
     * fix must NOT turn partial damage into a refusal. The truth surfaces
     * instead: bytes_unreadable == the failed coverage. */
    const u64 kSize = 512ull * 1024;
    const u64 kDeadStart = 128ull * 1024, kDeadLen = 8192;
    PartialFailDevice dev(kSize, {{kDeadStart, kDeadLen}});
    tree::ResultTree tree;
    ScanConfig cfg = truth_cfg();
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    EXPECT_EQ(rc, OK) << "partial damage is salvage, not refusal; got " << err_text(rc);
    EXPECT_FALSE(stats.probe_unreadable) << "sector 0 read fine";
    EXPECT_GT(stats.bytes_unreadable, 0u) << "the dead range must be visible";
    /* the recorded coverage is the dead range (possibly fragmented by the
     * bisection into touching sub-ranges - ReadErrorMap coalesces them) */
    EXPECT_LE(stats.bytes_unreadable, kDeadLen);
    EXPECT_GE(stats.bytes_unreadable, kDeadLen / 2)
        << "bisection records the failed span at (at least) half granularity";
}

TEST(ScanTruth, BlindProbeIsFlaggedWhileTheRestReads) {
    /* the distinction the probe could not make: sector 0 dead, everything
     * else readable. family stays Unknown (honestly: BLIND, not absent),
     * the flag is set, the scan itself proceeds (salvage) and completes. */
    const u64 kSize = 256ull * 1024;
    PartialFailDevice dev(kSize, {{0, 512}}); /* exactly the probe sector */
    tree::ResultTree tree;
    ScanConfig cfg = truth_cfg();
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    EXPECT_EQ(rc, OK) << "a dead header with readable data is deep-scan salvage";
    EXPECT_TRUE(stats.probe_unreadable)
        << "Unknown-from-blind must be distinguishable from Unknown-from-absent";
    EXPECT_GT(stats.bytes_unreadable, 0u);
    EXPECT_LT(stats.bytes_unreadable, kSize / 2)
        << "only the header region failed - coverage must reflect that";
}

TEST(ScanTruth, HealthyZeroVolumeStillCompletesCleanly) {
    /* no failures at all: the gate must not fire (bytes_unreadable == 0,
     * probe fine, rc OK) - the refusal is strictly a ZERO-success line,
     * never a threshold on small maps. */
    const u64 kSize = 256ull * 1024;
    PartialFailDevice dev(kSize, {});
    tree::ResultTree tree;
    ScanConfig cfg = truth_cfg();
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    EXPECT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.bytes_unreadable, 0u);
    EXPECT_FALSE(stats.probe_unreadable);
    EXPECT_TRUE(stats.errors.ranges().empty());
}

TEST(ScanTruth, StatsFieldsAreZeroInitialized) {
    /* the ABI appends bytes_unreadable/probe_unreadable to
     * cr_ui_scan_stats (zero-init safe by the house rule): the core-side
     * defaults must match - a default-constructed ScanStats claims NO
     * unreadable bytes and a readable probe, never the reverse. */
    ScanStats st;
    EXPECT_EQ(st.bytes_unreadable, 0u);
    EXPECT_FALSE(st.probe_unreadable);
}
