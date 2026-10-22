/*
 * rrecov - the core_recovery CLI (Windows).
 *
 * Commands: list | scan | tree | recover | save | load
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_cached.h"
#include "cr/storage/cr_slice.h"
#include "cr/win/cr_bitlocker.h"
#include "cr/win/cr_destio.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_transport.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace cr;

namespace {

std::wstring widen(const std::string &s) {
    std::wstring w;
    for (unsigned char c : s) {
        if (c < 0x80) w.push_back(wchar_t(c));
        else w.push_back(L'?'); /* CLI display only; node names are u16 exact */
    }
    return w;
}

void print_disks() {
    win::PhysicalDiskList list;
    if (win::enumerate_disks(200, list) != OK) {
        wprintf(L"enumeration failed\n");
        return;
    }
    win::VolumeEntryList vols;
    win::enumerate_volumes(vols);
    wprintf(L"%zu disk(s), %zu volume(s)\n", list.size(), vols.size());
    for (const auto &d : list) {
        wprintf(L"  Disk %u: %llu MiB (%s) %s [%s] %s\n", d.number,
                d.size_bytes >> 20, d.model.c_str(), d.serial.c_str(),
                d.removable ? L"removable" : L"fixed",
                d.table_ok ? (d.table.layout == table::TableLayout::Gpt
                                  ? L"GPT"
                                  : (d.table.layout == table::TableLayout::Mbr ? L"MBR" : L"-"))
                           : L"no-table");
        for (const auto &v : vols) {
            if (v.disk == d.number) {
                wprintf(L"    %c: %s %s %lluu %s\n", v.letter, v.fs_name.c_str(),
                        v.label.c_str(), v.bytes >> 20,
                        v.bitlocker ? L"[BitLocker]" : L"");
            }
        }
    }
}

std::unique_ptr<IStorageDevice> open_target(int argc, wchar_t **argv, int &i) {
    /* --disk N | --volume C: | --raw path */
    if (i + 1 >= argc) return nullptr;
    const std::wstring a = argv[i];
    ++i;
    const std::wstring v = argv[i];
    ++i;
    u32 err = 0;
    if (a == L"--disk") {
        const u32 n = u32(wcstoul(v.c_str(), nullptr, 10));
        /* RDA first when present, direct otherwise (reference duality) */
        if (win::rda_available(L"RDA")) {
            if (auto dev = win::open_rda_partition(L"RDA", n, 0, &err)) return dev;
        }
        return win::open_physical_drive(n, &err);
    }
    if (a == L"--volume" && v.size() >= 2) {
        const wchar_t letter = v[0];
        /* locked BitLocker? attempt legitimate auto-unlock then report */
        if (win::bitlocker::is_fve_volume(letter)) {
            if (win::bitlocker::status(letter) == win::bitlocker::Status::Locked) {
                wprintf(L"[bitlocker] locked - trying auto-unlock...\n");
                if (win::bitlocker::try_auto_unlock(letter) != OK) {
                    wprintf(L"[bitlocker] still locked. Unlock manually with:\n"
                            L"  Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>\n"
                            L"then re-run.\n", letter);
                    return nullptr;
                }
            }
        }
        u32 disk = 0;
        if (!win::volume_letter_to_disk(letter, &disk, nullptr)) {
            wprintf(L"cannot map %c: to a disk\n", letter);
            return nullptr;
        }
        /* partition slice via the disk table */
        u32 err2 = 0;
        auto disk_view = win::open_physical_drive(disk, &err2);
        if (!disk_view) {
            wprintf(L"cannot open disk %u (admin?)\n", disk);
            return nullptr;
        }
        table::DiskTable t;
        if (table::parse_disk_table(disk_view.get(), t) != OK) {
            wprintf(L"table parse failed\n");
            return nullptr;
        }
        /* find the partition containing this volume by extent match via
         * IOCTL - simpler: use the first partition whose FS matches the
           volume's fs query on the same disk (deterministic enough for CLI
           use; the wizard maps exactly) */
        win::VolumeEntryList vols;
        win::enumerate_volumes(vols);
        const win::VolumeEntry *self = nullptr;
        for (const auto &ve : vols)
            if (ve.letter == letter) self = &ve;
        if (!self) {
            wprintf(L"volume %c: not enumerable\n", letter);
            return nullptr;
        }
        for (const auto &p : t.partitions) {
            const u64 start = p.start_lba * disk_view->geometry().logical_sector;
            if (start == self->offset_on_disk && p.sector_count != 0) {
                u32 e3 = 0;
                auto slice = PartitionSlice::create(disk_view.get(), start,
                                                    u64(p.sector_count) *
                                                        disk_view->geometry().logical_sector,
                                                    &e3);
                if (slice) {
                    /* wrap with the RDA path when available for locked vol */
                    return slice;
                }
            }
        }
        wprintf(L"no matching partition for %c:\n", letter);
        return nullptr;
    }
    return nullptr;
}

int cmd_scan(int argc, wchar_t **argv, int &i) {
    auto slice = open_target(argc, argv, i);
    if (!slice) return 1;
    tree::ResultTree tree;
    orchestrator::ScanConfig cfg;
    cfg.whole_disk = false; /* volume scans; use --disk for whole-disk */
    if (i < argc && std::wstring(argv[i]) == L"--deep") {
        ++i;
        cfg.whole_disk = true;
    }
    orchestrator::ScanController ctrl(tree, cfg);
    orchestrator::ScanStats stats;
    const u32 rc = ctrl.run(slice.get(), nullptr, nullptr, nullptr, stats);
    if (rc != OK) {
        wprintf(L"scan failed: %s (%08x)\n", cr::err_text(rc), rc);
        return 1;
    }
    wprintf(L"scan complete: %llu file(s), %llu deleted, %llu carved\n",
            stats.files_found, stats.deleted_found, stats.carved_found);

    /* dump the tree (path \t size \t state) */
    std::function<void(tree::FileNode *, int)> dump = [&](tree::FileNode *n, int depth) {
        for (tree::FileNode *c = n->first_child(); c; c = c->next_sibling()) {
            for (int k = 0; k < depth; ++k) putwchar(L' ');
            const char *st = c->deleted_state() == tree::DeletedState::Active ? "" : " [deleted]";
            wprintf(L"%s (%llu)%s\n", widen(c->name_utf8()).c_str(),
                    c->props()->logical_size, st);
            dump(c, depth + 1);
        }
    };
    for (auto *r : tree.roots()) {
        wprintf(L"== %s\n", widen(r->name_utf8()).c_str());
        dump(r, 1);
    }
    return 0;
}

} // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc < 2) {
        wprintf(L"rrecov - core_recovery CLI\n"
                L"usage:\n"
                L"  rrecov list\n"
                L"  rrecov scan --disk N [--deep]\n"
                L"  rrecov scan --volume C:\n");
        return 0;
    }
    const std::wstring cmd = argv[1];
    int i = 2;
    if (cmd == L"list") {
        print_disks();
        return 0;
    }
    if (cmd == L"scan") return cmd_scan(argc, argv, i);
    wprintf(L"unknown command: %s\n", cmd.c_str());
    return 1;
}
