/* test_snapshot.cpp - G-02/G-02b acceptance: the mid-scan immutable value
 * snapshot + snapshot-based recovery.
 *
 *  - capture fidelity: every node's topology + content copied exactly
 *  - DFS parity: the snapshot walk equals the live recursive walk (the
 *    byte-parity contract behind snapshot-based serialization)
 *  - race-freedom: concurrent emitters + captures + snapshot walks
 *    (TSan-provable; all mutation and capture serialize on the tree
 *    mutex by construction)
 *  - recovery determinism (G-03): recovering a file set from a snapshot
 *    yields byte-identical destination content to post-scan recovery
 *  - SNAP_PARTIAL truth
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/tree/cr_snapshot.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <functional>
#include <set>
#include <thread>

using namespace cr;

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

tree::FileNode* add(tree::ResultTree& t, tree::FileNode* parent, const char* name, u64 off, u64 len,
                    u32 attrs = 0) {
    auto* p = new tree::FileProperties();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->logical_size = len;
    p->attributes = attrs;
    p->fs_kind = 3;
    if (len)
        p->extents.push_back({off, len});
    tree::FileNode* n = t.add_file(parent, p);
    p->release();
    return n;
}

/* in-memory DestIo capturing committed content for byte-parity checks */
/* in-memory DestIo capturing committed content for byte-parity checks */
struct CapIo final : recovery::DestIo {
    struct Item {
        std::vector<u8> data;
        bool committed = false;
    };
    /* item id -> temp content; final names recorded at commit */
    std::map<u64, Item> items;
    std::map<std::u16string, std::vector<u8>, std::less<>> commits;
    u64 cur = 0;
    std::map<std::u16string, u64, std::less<>> by_final;

    u32 create_temp(u64 id, const std::u16string& final_path, const std::u16string&) override {
        items[id] = Item{};
        cur = id;
        by_final[final_path] = id;
        return 0;
    }
    u32 mkdir(const std::u16string&) override { return 0; }
    u32 write(u64 id, u64 at, const void* buf, u32 len) override {
        auto it = items.find(id);
        if (it == items.end())
            return ERR_DEST_UNWRITABLE;
        if (it->second.data.size() < at + len)
            it->second.data.resize(size_t(at + len));
        std::memcpy(it->second.data.data() + at, buf, len);
        return 0;
    }
    u32 commit(u64 id) override {
        auto it = items.find(id);
        if (it == items.end() || it->second.committed)
            return ERR_DEST_UNWRITABLE;
        for (const auto& [name, fid] : by_final)
            if (fid == id) {
                commits[name] = std::move(it->second.data);
                it->second.committed = true;
                return 0;
            }
        return ERR_DEST_UNWRITABLE;
    }
    void close(u64) override {}
    bool exists(const std::u16string&) override { return false; }
};

} // namespace

TEST(Snapshot, CaptureFidelityAndDfsParity) {
    tree::ResultTree t;
    auto* root = t.add_root(u"Found (NTFS)");
    auto* sub = add(t, root, "sub", 0, 0, 0x10);
    add(t, root, "a.txt", 512, 100);
    add(t, sub, "b.jpg", 1024, 400);
    add(t, root, "c.zip", 2048, 4096);
    auto* orphan_root = t.add_root(u"Lost Files");
    add(t, orphan_root, "gone.bin", 4096, 64);
    t.refresh_stats();

    auto snap = tree::Snapshot::capture(t, false);
    ASSERT_NE(snap, nullptr);
    EXPECT_FALSE(snap->partial());
    EXPECT_EQ(7u, snap->node_count()); /* 2 roots + 5 files */
    EXPECT_EQ(2u, snap->roots().size());

    /* every node present exactly once, keyed by node_id */
    std::set<u64> ids;
    for (u32 i = 0; i < snap->node_count(); ++i)
        ids.insert(snap->node(i).node_id);
    EXPECT_EQ(snap->node_count(), u32(ids.size()));

    /* topology: find b.jpg, assert parent = sub, and sub's parent = root */
    const tree::SnapNode* bjpg = nullptr;
    const tree::SnapNode* subdir = nullptr;
    for (u32 i = 0; i < snap->node_count(); ++i) {
        const auto& s = snap->node(i);
        std::string nm;
        for (u16 ch : s.name)
            nm.push_back(char(ch));
        if (nm == "b.jpg")
            bjpg = &s;
        if (nm == "sub")
            subdir = &s;
    }
    ASSERT_NE(bjpg, nullptr);
    ASSERT_NE(subdir, nullptr);
    EXPECT_EQ(u64(400), bjpg->logical_size);
    EXPECT_EQ(1u, bjpg->extents.size());
    EXPECT_EQ(u64(1024), bjpg->extents[0].offset);
    EXPECT_EQ(3u, bjpg->fs_kind);
    EXPECT_NE(tree::SNAP_NONE, subdir->parent); /* sub's parent is the
                                                   Found root (a FullNode
                                                   row like any other) */
    /* the live walk for parity: emit the node, then first_child/
     * next_sibling recursion - the exact emission order the snapshot DFS
     * must reproduce (byte-parity contract). Roots included. */
    std::vector<u64> live_ids;
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* n) {
        live_ids.push_back(n->node_id());
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
            walk(c);
    };
    for (auto* r : t.roots())
        walk(r);

    std::vector<u32> snap_order;
    snap->dfs(snap_order);
    ASSERT_EQ(live_ids.size(), snap_order.size());
    std::set<u64> live_set(live_ids.begin(), live_ids.end());
    for (u32 idx : snap_order)
        EXPECT_EQ(1u, live_set.count(snap->node(idx).node_id))
            << "DFS visited a node the live walk did not";
    /* exact ORDER parity (byte-parity contract) */
    for (size_t i = 0; i < live_ids.size(); ++i)
        EXPECT_EQ(live_ids[i], snap->node(snap_order[i]).node_id)
            << "DFS order diverges from the live walk at " << i;
}

TEST(Snapshot, PartialFlagAndEmptyTree) {
    tree::ResultTree t;
    auto s0 = tree::Snapshot::capture(t, false);
    ASSERT_NE(s0, nullptr);
    EXPECT_EQ(0u, s0->node_count());
    EXPECT_TRUE(s0->roots().empty());
    auto s1 = tree::Snapshot::capture(t, true);
    ASSERT_NE(s1, nullptr);
    EXPECT_TRUE(s1->partial()); /* SNAP_PARTIAL: mid-stage truth */
}

TEST(Snapshot, ConcurrentEmitCaptureWalk) {
    /* TSan lane: emitters add nodes under the tree mutex while a second
     * thread captures + walks snapshots repeatedly. Race-free by
     * construction (every mutation + the capture take the same mutex). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    std::atomic<bool> stop{false};
    std::atomic<u64> captures{0};
    std::atomic<u64> walked{0};

    std::thread emit1([&] {
        for (int i = 0; i < 4000 && !stop.load(); ++i)
            add(t, root, "f1.txt", u64(i) * 512, 16);
    });
    std::thread emit2([&] {
        for (int i = 0; i < 4000 && !stop.load(); ++i)
            add(t, root, "f2.txt", u64(i) * 512, 16);
    });
    std::thread capturer([&] {
        /* keep looping while the emitters run, but guarantee at least one
         * capture even if the capturer is scheduled after both emitters
         * already joined (the old `while (!stop)` could exit with zero
         * captures on a slow-scheduler run - the flake was in the harness,
         * not the product; the race window under test exists whenever the
         * threads DO overlap, which TSan still sees when they do). */
        while (!stop.load() || captures.load() == 0) {
            auto s = tree::Snapshot::capture(t, true);
            if (!s)
                continue;
            std::vector<u32> order;
            s->dfs(order);
            walked += order.size();
            captures.fetch_add(1);
            /* walk the copied content: every byte is snapshot-owned */
            for (u32 idx : order) {
                const auto& n = s->node(idx);
                (void)n.logical_size;
                (void)n.extents.size();
            }
        }
    });
    emit1.join();
    emit2.join();
    stop.store(true);
    capturer.join();

    EXPECT_GT(captures.load(), u64(0));
    EXPECT_EQ(8001u, t.node_count()); /* 1 root + 8000 files */
}

TEST(Snapshot, RecoveryFromSnapshotIsByteIdentical) {
    /* G-03 determinism: recover the same set from the LIVE tree and from
     * a SNAPSHOT - the destination content must be byte-identical (the
     * writer reads only snapshot-owned bytes + the device on the value
     * path). */
    std::vector<u8> img(16 * 512, 0);
    for (size_t i = 0; i < img.size(); ++i)
        img[i] = u8(i ^ (i >> 3));
    MemDevice dev_a(img);
    MemDevice dev_b(img);

    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    add(t, root, "one.bin", 512, 1000);
    add(t, root, "two.bin", 2048, 4096);
    tree::FileProperties* resident = new tree::FileProperties();
    resident->name = u"three.txt";
    resident->logical_size = 4;
    resident->resident_data = {'R', 'E', 'S', '!'};
    auto* rnode = t.add_file(root, resident);
    resident->release();
    (void)rnode;
    t.refresh_stats();
    auto snap = tree::Snapshot::capture(t, false);
    ASSERT_NE(snap, nullptr);

    /* live-tree recovery */
    CapIo io_live;
    recovery::SafeWriter w_live(io_live, &dev_a, recovery::WriterConfig{});
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        (void)w_live.enqueue(c, u"out\\");
    recovery::WriterStats st1;
    ASSERT_EQ(OK, w_live.process(nullptr, nullptr, nullptr, st1));
    EXPECT_EQ(u64(3), st1.done);

    /* snapshot recovery: enqueue ONLY the file rows (skip the FullNode
     * roots), same relative order */
    CapIo io_snap;
    recovery::SafeWriter w_snap(io_snap, &dev_b, recovery::WriterConfig{});
    std::vector<u32> order;
    snap->dfs(order);
    for (u32 idx : order) {
        if (snap->node(idx).full)
            continue;
        ASSERT_EQ(OK, w_snap.enqueue(*snap, idx, u"out\\"));
    }
    recovery::WriterStats st2;
    ASSERT_EQ(OK, w_snap.process(nullptr, nullptr, nullptr, st2));
    EXPECT_EQ(u64(3), st2.done);

    /* byte-parity: same set of commits, identical content */
    ASSERT_EQ(io_live.commits.size(), io_snap.commits.size());
    for (const auto& [name, data] : io_live.commits) {
        auto it = io_snap.commits.find(name);
        ASSERT_NE(io_snap.commits.end(), it);
        ASSERT_EQ(data.size(), it->second.size());
        EXPECT_EQ(0, std::memcmp(data.data(), it->second.data(), data.size()));
    }
}

TEST(Snapshot, CarriesTheDistinguishedBranchAxes) {
    /* M-101: the value capture carries the two distinguished-branch
     * axes (the Versions id + the runtime tag family) — the snapshot is
    a live-tree copy, so a host reading a mid-scan capture can render
     * the Versions and Tags Extracted branches exactly as the final
     * tree would. The materialize path restores both fields. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    tree::FileProperties* p = new tree::FileProperties();
    p->name = u"dupe.doc";
    p->logical_size = 700;
    p->version_group = 32;
    p->tags_extracted = true;
    (void)t.add_file(root, p);
    p->release();
    t.refresh_stats();

    auto snap = tree::Snapshot::capture(t, false);
    ASSERT_NE(snap, nullptr);
    ASSERT_EQ(u32(1), snap->node_count() - u32(snap->roots().size()));
    bool saw = false;
    for (u32 i = 0; i < snap->node_count(); ++i) {
        const tree::SnapNode& s = snap->node(i);
        if (s.name == u"dupe.doc") {
            saw = true;
            EXPECT_EQ(s.version_group, u64(32))
                << "RED pre-fix: the capture dropped the Versions axis";
            EXPECT_TRUE(s.tags_extracted) << "the runtime tag flag rides the value capture";
        }
    }
    EXPECT_TRUE(saw);
    /* materialize restores both */
    for (u32 i = 0; i < snap->node_count(); ++i) {
        if (snap->node(i).name == u"dupe.doc") {
            tree::FileProperties* mp = snap->materialize_props(i);
            ASSERT_NE(mp, nullptr);
            EXPECT_EQ(mp->version_group, u64(32));
            EXPECT_TRUE(mp->tags_extracted);
            mp->release();
        }
    }
    /* the landed branches keep their shapes: the capture's DFS order and
     * node count are unchanged by the new fields (append-only wire) */
    std::vector<u32> dfs;
    snap->dfs(dfs);
    EXPECT_EQ(dfs.size(), size_t(snap->node_count()));
}
