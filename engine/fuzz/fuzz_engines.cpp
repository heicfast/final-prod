/* fuzz_engines.cpp - libFuzzer harness for the device-driven engines.
 *
 * Input: [selector byte][device image]. The image is wrapped in a
 * bounded MemDevice and fed to one engine per selector value; every
 * engine must terminate and return an error code on any input.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_scan.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/hfsplus/cr_hfsplus_engine.h"
#include "cr/btrfs/cr_btrfs_engine.h"
#include "cr/zfs/cr_zfs_engine.h"
#include "cr/refs/cr_refs_engine.h"
#include "cr/refs/cr_refs_detect.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/repair/cr_rawfixer.h"
#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_parse.h"
#include <cstring>
#include <memory>
#include <vector>

namespace {

class MemDevice final : public cr::IStorageDevice {
public:
    MemDevice(const uint8_t* d, size_t n) : d_(d), n_(n) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = n;
        g_.sector_count = n / 512;
    }
    cr::u32 read_at(cr::u64 off, void* buf, cr::u32 len) override {
        if (off > n_ || len > n_ - off)
            return cr::ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_ + off, len);
        return cr::OK;
    }
    const cr::Geometry& geometry() const override { return g_; }

private:
    const uint8_t* d_;
    size_t n_;
    cr::Geometry g_{};
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 2)
        return 0;
    const uint8_t sel = data[0];
    MemDevice dev(data + 1, size - 1);
    cr::tree::ResultTree tree;
    auto* ra = tree.add_root(u"a");
    auto* rd = tree.add_root(u"d");
    auto* rl = tree.add_root(u"l");
    switch (sel % 16) {
    case 0:
        (void)cr::orchestrator::ScanController::probe_family(&dev);
        break;
    case 1: {
        cr::table::DiskTable t;
        (void)cr::table::parse_disk_table(&dev, t);
        break;
    }
    case 2: {
        cr::u32 err = 0;
        auto vol = cr::ntfs::NtfsVolume::open(
            std::unique_ptr<cr::IStorageDevice>(cr::SliceShim::shim(&dev)), &err);
        if (vol) {
            std::unique_ptr<cr::ntfs::VolumeBitmap> bm;
            (void)cr::ntfs::VolumeBitmap::load(vol.get(), bm);
            std::vector<cr::u8> rec;
            (void)vol->read_record(0, rec);
        }
        break;
    }
    case 3: {
        cr::fat::FatScanFlags fl;
        cr::fat::FatScanStats st;
        (void)cr::fat::scan_fat(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 4: {
        cr::exfat::ExfatParams p;
        (void)cr::exfat::validate_boot_region(&dev, p);
        cr::exfat::ExfatScanFlags fl;
        cr::exfat::ExfatScanStats st;
        (void)cr::exfat::scan_exfat(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 5: {
        cr::ext4::Ext4ScanFlags fl;
        fl.scan_deleted_inodes = true;
        cr::ext4::Ext4ScanStats st;
        (void)cr::ext4::scan_ext4(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 6: {
        cr::hfsplus::HfsScanFlags fl;
        cr::hfsplus::HfsScanStats st;
        (void)cr::hfsplus::scan_hfsplus(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 7: {
        cr::btrfs::BtrfsScanStats st;
        (void)cr::btrfs::scan_btrfs(&dev, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 8: {
        cr::zfs::ZfsScanStats st;
        (void)cr::zfs::scan_zfs(&dev, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 9: {
        cr::refs::RefsScanFlags fl;
        cr::refs::RefsScanStats st;
        (void)cr::refs::scan_refs(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
        break;
    }
    case 10: {
        cr::refs::RefsInfo ri;
        (void)cr::refs::detect_refs(&dev, ri);
        break;
    }
    case 11: {
        cr::lost::DiscoveryResult dr;
        (void)cr::lost::discover(&dev, {}, 1, nullptr, nullptr, nullptr, dr);
        for (size_t i = 0; i < dr.candidates.size() && i < 4; ++i)
            (void)cr::lost::materialize(&dev, dr.candidates[i]);
        break;
    }
    case 12: {
        cr::repair::RepairPlan plan;
        (void)cr::repair::analyze(&dev, plan);
        break;
    }
    case 13: {
        /* full staged scan (quick only, bounded) */
        cr::orchestrator::ScanConfig cfg;
        cfg.run_quick = true;
        cfg.run_deep = false;
        cfg.run_carve = false;
        cfg.threads = 1;
        cr::orchestrator::ScanStats st;
        cr::orchestrator::ScanController sc(tree, cfg);
        (void)sc.run(&dev, nullptr, nullptr, nullptr, st);
        break;
    }
    case 14: {
        /* LDM dynamic-disk database: the full parse (PRIVHEAD/TOCBLOCK/
         * VMDB/VBLK walk, composite reassembly, graph resolution) */
        cr::table::LdmInfo info;
        (void)cr::table::parse_ldm_database(&dev, info);
        break;
    }
    case 15: {
        /* LDM parse + volume device reads: exercises the spanned/stripe/
         * raid5 translation and the XOR reconstruction paths against
         * arbitrary member content */
        cr::table::LdmInfo info;
        if (cr::table::parse_ldm_database(&dev, info) == cr::OK) {
            const cr::table::LdmGuid& self = info.ph.disk_guid_bin;
            for (const auto& lay : info.layouts) {
                if (lay.kind == cr::table::LdmVolumeKind::Broken || lay.copies.empty())
                    continue;
                cr::table::LdmVolumeDevice vol(
                    lay, [&self, &dev](const cr::table::LdmGuid& g) -> cr::IStorageDevice* {
                        return g == self ? &dev : nullptr;
                    });
                cr::u8 buf[512];
                if (vol.size() >= 512) {
                    (void)vol.read_at(0, buf, 512);
                    (void)vol.read_at(vol.size() - 512, buf, 512);
                    (void)vol.read_at(511, buf, 512); /* head+tail partials */
                }
            }
        }
        break;
    }
    }
    tree.reset();
    return 0;
}
