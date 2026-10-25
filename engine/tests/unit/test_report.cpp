/* test_report.cpp - G-09 acceptance: the machine-readable recovery
 * report. Escaping correctness (quotes, controls, invalid UTF-8 -> U+FFFD),
 * schema shape, per-item truthful outcomes over a mixed writer queue
 * (done / refused / error / resumed / dir), hash-status honesty, and the
 * CLI-parity sample emitted for the CI schema validator.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "test_tmpdir.hpp"
#include "cr/recovery/cr_report.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <fstream>
#include <functional>
#include <set>
#include <string>

using namespace cr;
using namespace cr::recovery;
using namespace cr::recovery::report;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    /* FB-R02: reads inside [fail_from, fail_to) return ERR_READ_FAILED
     * (simulates an unreadable span - the Skip policy zero-fills it) */
    void fail_range(u64 from, u64 to) {
        fail_from = from;
        fail_to = to;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (fail_from != fail_to && off < fail_to && off + len > fail_from)
            return ERR_READ_FAILED;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
    u64 fail_from = 0, fail_to = 0;
};

/* Mock DestIo: existing-path driven outcomes */
struct ReportIo final : DestIo {
    std::set<std::u16string> existing;
    std::set<std::u16string> broken_temp; /* create_temp fails for these */
    std::set<std::u16string> made;

    u32 create_temp(u64 id, const std::u16string& final_path, const std::u16string&) override {
        if (broken_temp.count(final_path)) {
            (void)id;
            return ERR_DEST_UNWRITABLE;
        }
        return 0;
    }
    u32 mkdir(const std::u16string& p) override {
        made.insert(p);
        existing.insert(p);
        return 0;
    }
    u32 write(u64, u64, const void*, u32) override { return 0; }
    u32 commit(u64) override { return 0; }
    void close(u64) override {}
    bool exists(const std::u16string& p) override { return existing.count(p) != 0; }
};

tree::FileNode* add_node(tree::ResultTree& t, tree::FileNode* parent, const char* name, u64 off,
                         u64 len, bool dir = false) {
    auto* p = new tree::FileProperties();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    if (!dir)
        p->extents.push_back({off, len});
    p->logical_size = dir ? 0 : len;
    if (dir)
        p->attributes = 0x10;
    tree::FileNode* n = t.add_file(parent, p);
    p->release();
    return n;
}

} // namespace

TEST(Report, EscapingStrictJson) {
    /* structural chars */
    EXPECT_EQ(escape_json("a\"b\\c"), "a\\\"b\\\\c");
    EXPECT_EQ(escape_json("tab\t nl\n cr\r bs\b ff\f"), "tab\\t nl\\n cr\\r bs\\b ff\\f");
    /* controls -> \u00XX */
    EXPECT_EQ(escape_json(std::string("x\x01y")), "x\\u0001y");
    EXPECT_EQ(escape_json(std::string("\x1F")), "\\u001f");
    /* valid UTF-8 passes through untouched */
    EXPECT_EQ(escape_json("\xC3\xA9\xE2\x82\xAC\xF0\x9F\x92\xBF"),
              "\xC3\xA9\xE2\x82\xAC\xF0\x9F\x92\xBF");
    /* invalid UTF-8 -> U+FFFD per bad byte (document stays valid JSON) */
    EXPECT_EQ(escape_json(std::string("a\x80"
                                      "b")),
              "a\xEF\xBF\xBD"
              "b");
    EXPECT_EQ(escape_json(std::string("a\xC3")), "a\xEF\xBF\xBD"); /* truncated 2-byte */
    EXPECT_EQ(escape_json(std::string("a\xE0\x80"
                                      "b")),
              "a\xEF\xBF\xBD\xEF\xBF\xBD"
              "b"); /* bad 3-byte: lead + stray cont */
}

TEST(Report, SchemaShapeAndDeterminism) {
    std::vector<Item> items;
    Item a;
    a.item_id = 1;
    a.path = "out/dir one/photo.jpg";
    a.name = "photo.jpg";
    a.outcome = Outcome::Done;
    a.bytes = 4096;
    a.duration_ms = 12;
    a.sha256_hex = "aa00";
    a.hash_status = HashStatus::Computed;
    items.push_back(a);
    Item b;
    b.item_id = 2;
    b.path = "out/dir one/locked.doc";
    b.name = "locked.doc";
    b.outcome = Outcome::Refused;
    b.reason = 0x50030012;
    b.hash_status = HashStatus::NotComputed;
    items.push_back(b);

    Totals t;
    t.queued = 2;
    t.done = 1;
    t.refused = 1;
    t.bytes = 4096;
    t.elapsed_ms = 30;
    t.source = "\\\\?\\Volume{abc}";
    t.destination = "D:\\Restore";

    const std::string r1 = build_report(items, t);
    const std::string r2 = build_report(items, t);
    EXPECT_EQ(r1, r2); /* deterministic */

    /* schema: version first, identities escaped, totals object, items
     * (compact form: no space after ':') */
    EXPECT_EQ(0u, r1.find("{\n  \"schema_version\": 1,\n"));
    EXPECT_NE(std::string::npos, r1.find("\"engine_version\":\""));
    EXPECT_NE(std::string::npos, r1.find("\"source\":\"\\\\\\\\?\\\\Volume{abc}\""));
    EXPECT_NE(std::string::npos, r1.find("\"destination\":\"D:\\\\Restore\""));
    EXPECT_NE(std::string::npos, r1.find("\"elapsed_ms\":30"));
    EXPECT_NE(std::string::npos, r1.find("\"totals\": {"));
    EXPECT_NE(std::string::npos, r1.find("\"queued\":2"));
    EXPECT_NE(std::string::npos, r1.find("\"hash_mismatch\":0"));
    /* item fields in schema order */
    EXPECT_NE(std::string::npos,
              r1.find("\"item_id\":1,\"path\":\"out/dir one/photo.jpg\",\"name\":\"photo.jpg\","
                      "\"kind\":\"file\",\"outcome\":\"done\",\"reason\":0,\"bytes\":4096,"));
    EXPECT_NE(std::string::npos, r1.find("\"outcome\":\"refused\""));
    EXPECT_NE(std::string::npos, r1.find("\"hash_status\":\"not_computed\""));
    /* document ends cleanly */
    EXPECT_EQ(std::string("}\n"), r1.substr(r1.size() - 2));

    /* empty queue: valid empty array */
    const std::string empty = build_report({}, Totals{});
    EXPECT_NE(std::string::npos, empty.find("\"items\": []"));
    EXPECT_EQ(std::string("}\n"), empty.substr(empty.size() - 2));
}

TEST(Report, WriterOutcomesAndHashHonesty) {
    /* mixed queue over one run: done (hash computed - none stored),
     * dir item, refused (overlap), error (broken temp), resumed (pre-existing) */
    std::vector<u8> img(16 * 512, 0);
    std::memcpy(img.data(), "DONE", 4);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* ok_file = add_node(tree, found, "ok.txt", 0, 4);
    auto* dir_node = add_node(tree, found, "sub", 0, 0, true);
    auto* refuse_me = add_node(tree, found, "refuse.bin", 1024, 100);
    auto* fail_me = add_node(tree, found, "fail.bin", 512, 100);
    auto* skip_me = add_node(tree, found, "skip.txt", 512, 100);
    tree.refresh_stats();

    ReportIo io;
    io.existing.insert(u"out\\skip.txt");    /* pre-existing final -> resumed */
    io.broken_temp.insert(u"out\\fail.bin"); /* temp creation fails -> error */

    WriterConfig cfg;
    cfg.conflict = ConflictPolicy::Skip;
    SafeWriter writer(io, &dev, cfg);
    writer.set_destination(1024, 100); /* refuse.bin's extent IS the dest:
                                          hard refusal (F12) */
    ASSERT_EQ(OK, writer.enqueue(ok_file, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(dir_node, u"out\\"));
    ASSERT_EQ(ERR_DEST_OVERLAP, writer.enqueue(refuse_me, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(fail_me, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(skip_me, u"out\\"));

    WriterStats stats;
    const u32 rc = writer.process(nullptr, nullptr, nullptr, stats);
    /* WRITER-1 (AUDIT-PIPELINE, F-A4-8): per-item failures are COLLECTED
     * and the pass COMPLETES - the og's ExportThread keeps going and
     * reports "X files/Y failed" at the end. The old abort-on-first-error
     * cancelled every item after the first failure. */
    ASSERT_EQ(OK, rc);
    EXPECT_EQ(u64(1), stats.errors);

    const std::vector<report::Item> items = writer.report_items();
    ASSERT_EQ(5u, items.size());

    /* ok.txt: committed, hash computed (no stored hash) - never verified */
    EXPECT_EQ(Outcome::Done, items[0].outcome);
    EXPECT_EQ(HashStatus::Computed, items[0].hash_status);
    EXPECT_EQ(64u, items[0].sha256_hex.size());
    EXPECT_EQ("ok.txt", items[0].name);
    EXPECT_EQ(0u, items[0].reason);

    /* sub: directory item, done, no hash */
    EXPECT_EQ(Outcome::Done, items[1].outcome);
    EXPECT_TRUE(items[1].is_dir);
    EXPECT_EQ(HashStatus::NotComputed, items[1].hash_status);
    EXPECT_TRUE(items[1].sha256_hex.empty());

    /* refuse.bin: refused with the overlap code */
    EXPECT_EQ(Outcome::Refused, items[2].outcome);
    EXPECT_EQ(u32(ERR_DEST_OVERLAP), items[2].reason);
    EXPECT_EQ(HashStatus::NotComputed, items[2].hash_status);

    /* fail.bin: error with the temp-creation code */
    EXPECT_EQ(Outcome::Error, items[3].outcome);
    EXPECT_EQ(u32(ERR_DEST_UNWRITABLE), items[3].reason);

    /* skip.txt: the queue CONTINUED past the failure - the item is
     * processed (Skip policy: pre-existing final = idempotent resume) */
    EXPECT_EQ(Outcome::Resumed, items[4].outcome);
    EXPECT_EQ(HashStatus::NotComputed, items[4].hash_status);

    /* totals reflect the full pass: 1 dir, 1 collected error */
    const Totals t = writer.report_totals("src", "dst", 5, stats);
    EXPECT_EQ(5u, t.queued);
    EXPECT_EQ(u64(1), t.dirs);
}

TEST(Report, MultiExtentSpannedVolumeRefused) {
    /* N-013: a spanned/RAID destination has MORE THAN ONE disk extent.
     * First-extent-only arming (the pre-N-013 behavior) misses overlap
     * on the second+ spans; every span must be checked. The finding's
     * named test: Writer.MultiExtentSpannedVolumeRefused.
     * span[0] = (0,100); span[1] = (1<<20, 512) - catches spanned.bin at
     * (1<<20)+256; the other two nodes sit between the spans. */
    std::vector<u8> img(16 * 512, 0);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* between_a = add_node(tree, found, "between-a.bin", 2048, 100);
    auto* spanned = add_node(tree, found, "spanned.bin", (1 << 20) + 256, 100);
    auto* between_b = add_node(tree, found, "between-b.bin", 4096, 100);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    writer.set_destination_spans({{0, 100}, {1 << 20, 512}});
    ASSERT_EQ(OK, writer.enqueue(between_a, u"out\\"));       /* between spans */
    ASSERT_EQ(ERR_DEST_OVERLAP, writer.enqueue(spanned, u"out\\")); /* span[1] catches */
    ASSERT_EQ(OK, writer.enqueue(between_b, u"out\\"));       /* between spans */

    /* single-span arming stays equivalent to the legacy setter: with only
     * span[1] armed, spanned.bin is still caught (the sole span), the
     * between nodes pass - and a node on the DISARMED first span
     * (0,100) would pass (first-extent-only semantics preserved). */
    SafeWriter legacy(io, &dev, cfg);
    legacy.set_destination(1 << 20, 512);
    ASSERT_EQ(OK, legacy.enqueue(between_a, u"out\\"));
    ASSERT_EQ(ERR_DEST_OVERLAP, legacy.enqueue(spanned, u"out\\"));
    ASSERT_EQ(OK, legacy.enqueue(between_b, u"out\\"));
}

/* N-015 mocks: destination-full behavior driven by the IO layer */
struct FullIo final : DestIo {
    u32 create_temp_calls = 0;
    u32 write_calls = 0;
    u32 fail_on_write = 2; /* ERR_DEST_FULL on the 2nd write call */
    u64 free_at = 0;       /* 0 = unknown (pre-check skipped) */
    u32 create_temp(u64, const std::u16string&, const std::u16string&) override {
        ++create_temp_calls;
        return 0;
    }
    u32 mkdir(const std::u16string&) override { return 0; }
    u32 write(u64, u64, const void*, u32) override {
        ++write_calls;
        if (write_calls == fail_on_write)
            return ERR_DEST_FULL;
        return 0;
    }
    u32 commit(u64) override { return 0; }
    void close(u64) override {}
    bool exists(const std::u16string&) override { return false; }
    u64 free_bytes(const std::u16string&) const override { return free_at; }
};

TEST(Report, DestFullFastFailStopsAttempting) {
    /* N-015 fail-before/pass-after:
     * BEFORE (pre-fix): write fails with a generic code, the pass
     * COLLECTS it (WRITER-1) and attempts EVERY remaining item -
     * rc == OK, 3 errors, 4 create_temp cycles.
     * AFTER: ERR_DEST_FULL aborts the pass at the failing item - item 2
     * Error(ERR_DEST_FULL), items 3-4 stay Pending (never attempted:
     * create_temp_calls == 2), rc == ERR_DEST_FULL, stats.dest_full. */
    std::vector<u8> img(16 * 512, 0);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* a = add_node(tree, found, "a.bin", 0, 512);
    auto* b = add_node(tree, found, "b.bin", 512, 512);
    auto* c = add_node(tree, found, "c.bin", 1024, 512);
    auto* d = add_node(tree, found, "d.bin", 1536, 512);
    tree.refresh_stats();

    FullIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(a, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(b, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(c, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(d, u"out\\"));

    WriterStats stats;
    EXPECT_EQ(ERR_DEST_FULL, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_TRUE(stats.dest_full);
    EXPECT_EQ(2u, io.create_temp_calls); /* items 3-4 never attempted */
    EXPECT_EQ(2u, io.write_calls);

    const std::vector<report::Item> items = writer.report_items();
    ASSERT_EQ(4u, items.size());
    EXPECT_EQ(Outcome::Done, items[0].outcome);       /* a.bin committed */
    EXPECT_EQ(Outcome::Error, items[1].outcome);      /* b.bin hit ENOSPC */
    EXPECT_EQ(u32(ERR_DEST_FULL), items[1].reason);
    EXPECT_EQ(Outcome::Pending, items[2].outcome);    /* c.bin: never attempted */
    EXPECT_EQ(Outcome::Pending, items[3].outcome);    /* d.bin: never attempted */
}

TEST(Report, DestFullPreCheckRefusesUpfront) {
    /* N-015 pre-check (the og computes destination free space before
     * writing, RecoveryModule :47033-47038): when the figure is KNOWN
     * and cannot hold the queue, the pass refuses upfront - zero write
     * cycles, every item Pending, rc == ERR_DEST_FULL at 0%. */
    std::vector<u8> img(16 * 512, 0);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* na = add_node(tree, found, "a.bin", 0, 512);
    auto* nb = add_node(tree, found, "b.bin", 512, 512);
    auto* nc = add_node(tree, found, "c.bin", 1024, 512);
    (void)na; (void)nb; (void)nc;
    tree.refresh_stats();

    FullIo io;
    io.free_at = 100; /* known: 100 bytes < 1536 queued */
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    writer.enqueue(na, u"out\\");
    writer.enqueue(nb, u"out\\");
    writer.enqueue(nc, u"out\\");

    WriterStats stats;
    EXPECT_EQ(ERR_DEST_FULL, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_TRUE(stats.dest_full);
    EXPECT_EQ(0u, io.create_temp_calls); /* nothing attempted at all */
    EXPECT_EQ(0u, io.write_calls);
    for (const auto& it : writer.report_items())
        EXPECT_EQ(Outcome::Pending, it.outcome);
}

TEST(Report, SalvagedOutcomeMarksZeroFilledCommits) {
    /* FB-R02 (W2-18): a committed item whose device read failed mid-stream
     * is ZERO-FILLED (Skip policy) and must surface as outcome "salvaged"
     * - never silently "done". Totals gain the salvaged sub-bucket; the
     * pending count keeps the partial-run reconciliation truthful.
     *
     * Writer cancel semantics (pinned by this test): the item IN FLIGHT
     * when the token fires becomes Error(ERR_CANCELLED); items after it
     * stay Pending - the pass stops without inventing outcomes. */
    std::vector<u8> img(16 * 512, 0);
    std::memcpy(img.data(), "DATA", 4);
    MemDevice dev(std::move(img));
    dev.fail_range(2048, 2560); /* bytes [2048,2560) unreadable */
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* clean = add_node(tree, found, "clean.txt", 0, 4);
    auto* damaged = add_node(tree, found, "damaged.bin", 2048, 512);
    auto* victim = add_node(tree, found, "victim.bin", 512, 512);
    auto* never = add_node(tree, found, "never.bin", 0, 4);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    writer.set_destination(1ull << 40, 512); /* dest far away: no overlap */
    ASSERT_EQ(OK, writer.enqueue(clean, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(damaged, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(victim, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(never, u"out\\"));

    /* fires on victim's first streaming poll (poll #3: clean, damaged,
     * victim each stream exactly one read chunk) */
    struct FireOnThirdToken final : CancelToken {
        mutable std::atomic<u64> polls{0};
        bool cancelled() const override { return polls.load() >= 3; }
        bool checkpoint() const override { return ++polls >= 3; }
    } tok;

    WriterStats stats;
    const u32 rc = writer.process(&tok, nullptr, nullptr, stats);
    EXPECT_EQ(ERR_CANCELLED, rc);
    EXPECT_EQ(u64(1), stats.salvaged);

    const std::vector<report::Item> items = writer.report_items();
    ASSERT_EQ(4u, items.size());
    EXPECT_EQ(Outcome::Done, items[0].outcome);     /* clean.txt */
    EXPECT_EQ(Outcome::Salvaged, items[1].outcome); /* damaged.bin: FB-R02 */
    EXPECT_EQ(u32(0), items[1].reason);             /* the commit is clean;
                                                       the content is not */
    EXPECT_EQ(Outcome::Error, items[2].outcome);    /* victim: cancelled
                                                        mid-flight */
    EXPECT_EQ(u32(ERR_CANCELLED), items[2].reason);
    EXPECT_EQ(Outcome::Pending, items[3].outcome); /* never attempted */

    const Totals t2 = writer.report_totals("src", "dst", 9, stats);
    EXPECT_EQ(u64(1), t2.salvaged);
    EXPECT_EQ(u64(1), t2.pending);
    EXPECT_EQ(u64(2), t2.done);   /* clean + damaged: done subsumes the
                                     salvaged sub-bucket (both are commits) */
    EXPECT_EQ(u64(0), t2.errors); /* cancellation is not an error - the
                                     victim's Error outcome carries
                                     ERR_CANCELLED per-item instead */
    EXPECT_EQ(4u, t2.queued);     /* truthful even for the aborted pass */

    /* the emitted JSON carries the new totals keys + "salvaged" outcome */
    const std::string doc = build_report(items, t2);
    EXPECT_NE(std::string::npos, doc.find("\"salvaged\""));
    EXPECT_NE(std::string::npos, doc.find("\"pending\""));
}

TEST(Report, EmitsCiSchemaSample) {
    /* the fixed sample the CI validator (tools/validate_report.py) checks:
     * every outcome + hash status appears, realistically paired */
    std::vector<Item> items;
    struct Row {
        const char *name, *outcome, *hash_status;
        u32 reason;
        const char* sha; /* nullptr = empty */
        bool dir;
        u64 bytes;
    };
    const Row rows[] = {
        {"photo.jpg", "done", "verified", 0, "aaaa", false, 2048},
        {"locked.doc", "refused", "not_computed", 0x50030012, nullptr, false, 512},
        {"torn.db", "error", "not_computed", 0x50030004, nullptr, false, 128},
        {"again.txt", "resumed", "not_computed", 0, nullptr, false, 256},
        {"never.bin", "pending", "not_computed", 0, nullptr, false, 64},
        {"bad.jpg", "hash_mismatch", "mismatch", 0x50030005, "bbbb", false, 1024},
        {"folder", "done", "not_computed", 0, nullptr, true, 0},
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        const Row& row = rows[i];
        Item it;
        it.item_id = u64(i + 1);
        it.path = std::string("out/") + row.name;
        it.name = row.name;
        it.outcome = Outcome(uint8_t(i));
        /* map by string to keep this table readable and the enum honest */
        if (std::string(row.outcome) == "done")
            it.outcome = Outcome::Done;
        else if (std::string(row.outcome) == "refused")
            it.outcome = Outcome::Refused;
        else if (std::string(row.outcome) == "error")
            it.outcome = Outcome::Error;
        else if (std::string(row.outcome) == "resumed")
            it.outcome = Outcome::Resumed;
        else if (std::string(row.outcome) == "pending")
            it.outcome = Outcome::Pending;
        else
            it.outcome = Outcome::HashMismatch;
        it.reason = row.reason;
        it.bytes = row.bytes;
        it.duration_ms = 2 * (i + 1);
        if (row.sha)
            it.sha256_hex.assign(64, row.sha[0]);
        if (std::string(row.hash_status) == "verified")
            it.hash_status = HashStatus::Verified;
        else if (std::string(row.hash_status) == "computed")
            it.hash_status = HashStatus::Computed;
        else if (std::string(row.hash_status) == "mismatch")
            it.hash_status = HashStatus::Mismatch;
        else
            it.hash_status = HashStatus::NotComputed;
        it.is_dir = row.dir;
        items.push_back(it);
    }
    Totals t;
    t.queued = 7;
    t.done = 2; /* photo.jpg + folder (dir) */
    t.dirs = 1;
    t.refused = 1;
    t.errors = 1;
    t.hash_mismatch = 1;
    t.resumed = 1;
    t.bytes = 3712; /* synthetic: shape is what the validator checks */
    t.elapsed_ms = 42;
    t.source = "PhysicalDrive0 \\\\?\\partition3";
    t.destination = "D:\\Restored Files";

    const std::string doc = build_report(items, t);
    ASSERT_FALSE(doc.empty());
    std::ofstream f(cr_test::tmp_file("cr_report_sample.json"), std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(f.is_open());
    f.write(doc.data(), std::streamsize(doc.size()));
    f.close();
    SUCCEED();
}
