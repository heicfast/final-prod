// s10 probe: whole-disk scan of a healthy basic MBR disk with TWO known
// FAT16 volumes (the real og G-session shape: G: + System Reserved).
// Mirrors the CLI `scan --disk N [--whole-disk]` path: the slice is the
// whole disk, fs_hint unset (Unknown -> probe_family reads LBA 0).
#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include "cr/cr_log.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::orchestrator;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
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

void dump_tree(tree::ResultTree& tree, const char* tag) {
    std::printf("  [%s] roots:\n", tag);
    for (tree::FileNode* r : tree.roots()) {
        std::printf("    - \"%s\" (children: %s)\n", r->name_utf8().c_str(),
                    r->first_child() ? "yes" : "none");
    }
    // count nodes named DOC.TXT anywhere
    u64 doc = 0, delfile = 0, total = 0;
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* n) {
        ++total;
        const std::string nm = n->name_utf8();
        if (nm == "DOC.TXT")
            ++doc;
        if (nm == "DELFILE.TXT")
            ++delfile;
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
            walk(c);
    };
    for (tree::FileNode* r : tree.roots())
        walk(r);
    std::printf("  [%s] tree nodes: %llu, DOC.TXT: %llu, DELFILE.TXT: %llu\n", tag,
                (unsigned long long)total, (unsigned long long)doc,
                (unsigned long long)delfile);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <disk.img>\n", argv[0]);
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    std::vector<u8> img((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::printf("image: %s (%llu bytes, %llu sectors)\n", argv[1],
                (unsigned long long)img.size(), (unsigned long long)(img.size() / 512));

    // Run 1: the CLI default — fs_hint unset (Unknown), whole_disk on
    {
        MemDevice dev(img);
        tree::ResultTree tree;
        ScanConfig cfg; // run_quick/run_deep/run_carve defaults = on
        cfg.whole_disk = true;
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
        std::printf("\nRUN 1 (the CLI `scan --disk N --whole-disk` shape: Unknown hint + "
                    "whole-disk):\n  rc=%u (0x%08x)\n  files_found=%llu deleted_found=%llu "
                    "carved=%llu lost_partitions=%llu\n",
                    rc, rc, (unsigned long long)stats.files_found,
                    (unsigned long long)stats.deleted_found,
                    (unsigned long long)stats.carved_found,
                    (unsigned long long)stats.lost_partitions);
        dump_tree(tree, "run1");
    }

    // Run 2: same, without --whole-disk (plain `scan --disk N`)
    {
        MemDevice dev(img);
        tree::ResultTree tree;
        ScanConfig cfg;
        cfg.whole_disk = false;
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
        std::printf("\nRUN 2 (plain `scan --disk N`: Unknown hint, no discovery):\n  rc=%u "
                    "(0x%08x)\n  files_found=%llu deleted_found=%llu carved=%llu\n",
                    rc, rc, (unsigned long long)stats.files_found,
                    (unsigned long long)stats.deleted_found,
                    (unsigned long long)stats.carved_found);
        dump_tree(tree, "run2");
    }

    // Run 3 (control): the SAME disk, but fs_hint forced to Fat — does the
    // engine pass then find the volumes? (slice is still the WHOLE DISK)
    {
        MemDevice dev(img);
        tree::ResultTree tree;
        ScanConfig cfg;
        cfg.whole_disk = true;
        cfg.fs_hint = FsFamily::Fat;
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
        std::printf("\nRUN 3 (control: fs_hint FORCED to Fat on the whole-disk slice):\n  rc=%u "
                    "(0x%08x)\n  files_found=%llu deleted_found=%llu carved=%llu\n",
                    rc, rc, (unsigned long long)stats.files_found,
                    (unsigned long long)stats.deleted_found,
                    (unsigned long long)stats.carved_found);
        dump_tree(tree, "run3");
    }
    return 0;
}
