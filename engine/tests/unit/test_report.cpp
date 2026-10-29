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
#include "cr/orchestrator/cr_scan.h"
#include "cr/cr_log.h"
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
    ASSERT_EQ(OK, writer.enqueue(between_a, u"out\\"));             /* between spans */
    ASSERT_EQ(ERR_DEST_OVERLAP, writer.enqueue(spanned, u"out\\")); /* span[1] catches */
    ASSERT_EQ(OK, writer.enqueue(between_b, u"out\\"));             /* between spans */

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
    EXPECT_EQ(Outcome::Done, items[0].outcome);  /* a.bin committed */
    EXPECT_EQ(Outcome::Error, items[1].outcome); /* b.bin hit ENOSPC */
    EXPECT_EQ(u32(ERR_DEST_FULL), items[1].reason);
    EXPECT_EQ(Outcome::Pending, items[2].outcome); /* c.bin: never attempted */
    EXPECT_EQ(Outcome::Pending, items[3].outcome); /* d.bin: never attempted */
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
    (void)na;
    (void)nb;
    (void)nc;
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

TEST(Report, ReadErrorOutcomeMarksTruncatedPartials) {
    /* FB-R02 (W2-18) re-pinned by M-135: a committed item whose device
     * read failed mid-stream is NO LONGER zero-filled-and-salvaged - the
     * og's runlist path BREAKS at the failing read, KEEPS the partial on
     * disk (DRWUI :875684-875696), and the file lands in the failed list
     * with its numbered cause. damaged.bin carries a readable 2048-byte
     * first extent + a second extent inside the bad range: the partial
     * commits 2048 bytes, outcome read_error, cause 5, the boundary at
     * the first unreadable logical byte. Never silently "done".
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
    /* damaged: extent 1 readable [0,2048), extent 2 in the bad range */
    auto* damaged = new tree::FileProperties();
    damaged->name = u"damaged.bin";
    damaged->extents.push_back({0, 2048});
    damaged->extents.push_back({2048, 512});
    damaged->logical_size = 2560;
    tree::FileNode* damaged_n = tree.add_file(found, damaged);
    damaged->release();
    auto* victim = add_node(tree, found, "victim.bin", 512, 512);
    auto* never = add_node(tree, found, "never.bin", 0, 4);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    writer.set_destination(1ull << 40, 512); /* dest far away: no overlap */
    ASSERT_EQ(OK, writer.enqueue(clean, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(damaged_n, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(victim, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(never, u"out\\"));

    /* fires on victim's first streaming poll (poll #4: clean streams
     * one chunk, damaged streams TWO - its readable extent, then the
     * failing read truncates it - so victim's first poll is #4) */
    struct FireOnFourthToken final : CancelToken {
        mutable std::atomic<u64> polls{0};
        bool cancelled() const override { return polls.load() >= 4; }
        bool checkpoint() const override { return ++polls >= 4; }
    } tok;

    WriterStats stats;
    const u32 rc = writer.process(&tok, nullptr, nullptr, stats);
    EXPECT_EQ(ERR_CANCELLED, rc);
    EXPECT_EQ(u64(1), stats.read_error);

    const std::vector<report::Item> items = writer.report_items();
    ASSERT_EQ(4u, items.size());
    EXPECT_EQ(Outcome::Done, items[0].outcome);      /* clean.txt */
    EXPECT_EQ(Outcome::ReadError, items[1].outcome); /* damaged.bin: the
                                                        og's error-9 family */
    EXPECT_EQ(5u, items[1].cause_id);                /* the sector location cannot
                                                        be found */
    EXPECT_EQ(2048u, items[1].first_failed_offset);  /* the bad extent's
                                                        logical position */
    EXPECT_EQ(2048u, items[1].bytes_written);        /* the committed partial */
    EXPECT_EQ(2560u, items[1].bytes);                /* the attempted size */
    EXPECT_EQ(Outcome::Error, items[2].outcome);     /* victim: cancelled
                                                        mid-flight */
    EXPECT_EQ(u32(ERR_CANCELLED), items[2].reason);
    EXPECT_EQ(Outcome::Pending, items[3].outcome); /* never attempted */

    const Totals t2 = writer.report_totals("src", "dst", 9, stats);
    EXPECT_EQ(u64(1), t2.read_error);
    EXPECT_EQ(u64(0), t2.salvaged); /* nothing zero-filled here anymore */
    EXPECT_EQ(u64(1), t2.pending);
    EXPECT_EQ(u64(1), t2.done);        /* clean only - the truncated partial
                                          is NOT a done (the og's failed list) */
    EXPECT_EQ(u64(0), t2.errors);      /* cancellation is not an error - the
                                          victim's Error outcome carries
                                          ERR_CANCELLED per-item instead */
    EXPECT_EQ(3072u, t2.failed_bytes); /* damaged 2560 + the cancelled
                                        * victim 512: every failed row's
                                        * attempted size */
    EXPECT_EQ(4u, t2.queued);          /* truthful even for the aborted pass */

    /* the emitted JSON carries the new totals keys + the failure row */
    const std::string doc = build_report(items, t2);
    EXPECT_NE(std::string::npos, doc.find("\"read_error\""));
    EXPECT_NE(std::string::npos, doc.find("\"cause_id\":5"));
    EXPECT_NE(std::string::npos, doc.find("\"first_failed_offset\":2048"));
    EXPECT_NE(std::string::npos, doc.find("\"bytes_written\":2048"));
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

TEST(Report, FailedRowsCarryTreeClassification) {
    /* N-159: the og's failed-files HTML report classifies every row by
     * its results-tree branch (CSaveFailedFilesWithHtml::FUN_1403c6fc0,
     * DRWUI :642755-642822, decoded first-hand: the group/kind-to-label
     * mapping). The core derives the same axis from the node's root and
     * surfaces it on EVERY report row (the structured superset over the
     * og's failed-only funnel, D13). */
    std::vector<u8> img(16 * 512, 0);
    std::memcpy(img.data(), "DONE", 4);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* deleted = tree.add_root(u"Deleted");
    auto* lost = tree.add_root(u"File Path Lost");
    auto* reconstructed = tree.add_root(u"Reconstructed");
    auto* exist1 = add_node(tree, found, "exist.bin", 0, 100);
    auto* fail_deleted = add_node(tree, deleted, "fail1.bin", 512, 100);
    auto* fail_lost = add_node(tree, lost, "fail2.bin", 512, 100);
    auto* fail_reconstructed = add_node(tree, reconstructed, "fail3.bin", 512, 100);
    auto* exist2 = add_node(tree, found, "exist2.bin", 0, 100);
    tree.refresh_stats();

    ReportIo io;
    io.broken_temp.insert(u"out\\fail1.bin");
    io.broken_temp.insert(u"out\\fail2.bin");
    io.broken_temp.insert(u"out\\fail3.bin");
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    for (tree::FileNode* n : {exist1, fail_deleted, fail_lost, fail_reconstructed, exist2})
        ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));

    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));

    const std::vector<report::Item> items = writer.report_items();
    ASSERT_EQ(5u, items.size());
    /* the classification axis rides every row (found/deleted/lost/
     * reconstructed), failures and successes alike */
    EXPECT_EQ("existing_files", items[0].tree_class);
    EXPECT_EQ("lost_files_deleted", items[1].tree_class);
    EXPECT_EQ("lost_files", items[2].tree_class);
    EXPECT_EQ("lost_files_reconstructed", items[3].tree_class);
    EXPECT_EQ("existing_files", items[4].tree_class);
    /* the failed rows are errors (broken temps) - the axis names WHICH
     * KIND of file failed, the og's recovery-log contract */
    EXPECT_EQ(Outcome::Error, items[1].outcome);
    EXPECT_EQ(Outcome::Error, items[2].outcome);
    EXPECT_EQ(Outcome::Error, items[3].outcome);
}

TEST(Report, FailedBytesSplitFromTotal) {
    /* N-159: the og accumulates the failed files' BYTES separately from
     * the committed totals (the results singleton's +0x268 accumulator,
     * DRWUI :642729-642746). The core's totals gain the same split:
     * failed_bytes counts the non-committed files (Error + Refused),
     * bytes stays the committed sum. */
    std::vector<u8> img(16 * 512, 0);
    std::memcpy(img.data(), "DONE", 4);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* ok_file = add_node(tree, found, "ok.txt", 0, 4);
    auto* refuse_me = add_node(tree, found, "refuse.bin", 1024, 100);
    auto* fail_me = add_node(tree, found, "fail.bin", 512, 100);
    tree.refresh_stats();

    ReportIo io;
    io.broken_temp.insert(u"out\\fail.bin");
    WriterConfig cfg;
    cfg.conflict = ConflictPolicy::Skip;
    SafeWriter writer(io, &dev, cfg);
    writer.set_destination(1024, 100); /* the overlap refusal */
    ASSERT_EQ(OK, writer.enqueue(ok_file, u"out\\"));
    ASSERT_EQ(ERR_DEST_OVERLAP, writer.enqueue(refuse_me, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(fail_me, u"out\\"));

    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    const Totals t = writer.report_totals("src", "dst", 5, stats);
    EXPECT_EQ(4u, t.bytes);          /* the committed ok.txt */
    EXPECT_EQ(100u, t.failed_bytes); /* fail.bin's 100 (the enqueue-time
                                        overlap refusal never entered the
                                        queue - the og's export loop has no
                                        pre-enqueue refusal class either) */

    /* and the split surfaces in the serialized document */
    const std::string doc = build_report(writer.report_items(), t);
    EXPECT_NE(std::string::npos, doc.find("\"failed_bytes\":100"));
    EXPECT_NE(std::string::npos, doc.find("\"bytes\":4,"));
}

TEST(Report, RawCategoryNameClassification) {
    /* N-159: the og's custom branch - a node whose category carries a
     * custom name classifies BY THAT NAME (FUN_14008a650's non-zero
     * path appends the name itself). The core's configured raw root
     * role IS the custom category name. */
    std::vector<u8> img(16 * 512, 0);
    std::memcpy(img.data(), "DONE", 4);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* raw = tree.add_root(u"RAW Summer 2024");
    auto* carved = add_node(tree, raw, "carved.bin", 0, 100);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(carved, u"out\\"));

    const std::vector<report::Item> items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ("RAW Summer 2024", items[0].tree_class);
    const std::string doc = build_report(items, Totals{});
    EXPECT_NE(std::string::npos, doc.find("\"tree_class\":\"RAW Summer 2024\""));
}

TEST(Report, TreeClassIsAdditiveWithinSchemaV1) {
    /* N-159: the schema doc's own version contract - additive fields
     * appear WITHIN a version (only breaking changes bump); tree_class
    and failed_bytes are additive, so schema_version stays 1 and the
     * new fields append AFTER the fixed v1 fields (append-only order). */
    std::vector<Item> items;
    Item a;
    a.item_id = 1;
    a.name = "x.bin";
    a.outcome = Outcome::Error;
    a.reason = 4;
    a.bytes = 9;
    a.tree_class = "lost_files";
    items.push_back(a);
    Totals t;
    t.queued = 1;
    t.errors = 1;
    t.failed_bytes = 9;
    const std::string doc = build_report(items, t);
    EXPECT_NE(std::string::npos, doc.find("\"schema_version\": 1,"));
    /* the new item field lands after hash_status (the v1 tail) */
    const size_t pos_hash = doc.find("\"hash_status\":");
    const size_t pos_class = doc.find("\"tree_class\":\"lost_files\"");
    ASSERT_NE(std::string::npos, pos_hash);
    ASSERT_NE(std::string::npos, pos_class);
    EXPECT_LT(pos_hash, pos_class);
    /* an unclassified row simply omits the field (older consumers and
     * value-enqueued rows are unchanged) */
    Item b;
    b.item_id = 2;
    b.name = "y.bin";
    items.push_back(b);
    const std::string doc2 = build_report(items, t);
    EXPECT_EQ(std::string::npos, doc2.find("\"tree_class\":\"y.bin\""));
    EXPECT_EQ(
        1u, u64(doc2.find("\"tree_class\":\"lost_files\"") != std::string::npos) +
                u64(doc2.find("\"tree_class\":\"lost_files_reconstructed\"") != std::string::npos));
}

/* ---- M-136: the recovery pass reads through the scan's pipe --------- */

/* a transient-fault device: the FIRST read overlapping [flaky_from,
 * flaky_to) fails, every later attempt succeeds (the USB-hiccup / busy
 * virtual-disk class the scan's RetryingDevice survives on attempt 2);
 * successful reads are counted (cache-warmth pin) */
class TransientDevice final : public IStorageDevice {
public:
    explicit TransientDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    void flaky_range(u64 from, u64 to) {
        flaky_from_ = from;
        flaky_to_ = to;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (!armed_ && flaky_from_ != flaky_to_ && off < flaky_to_ && off + len > flaky_from_) {
            armed_ = true; /* exactly one transient failure, then readable */
            return ERR_READ_FAILED;
        }
        ++reads_;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u32 reads() const { return reads_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
    u64 flaky_from_ = 0, flaky_to_ = 0;
    bool armed_ = false;
    u32 reads_ = 0;
};

TEST(Writer, ReadsRetryOnTransientFailure) {
    /* M-136: a span that fails ONCE (the transient class the scan's
     * RetryingDevice retries past) recovers byte-identical through the
     * recovery pipe - Done, zero Salvaged. Handed the bare device (the
     * pre-fix construction), the same fault zero-fills the span. */
    std::vector<u8> img(64 * 1024, 0xCD);
    TransientDevice dev(std::move(img));
    dev.flaky_range(8 * 1024, 12 * 1024);

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* f = add_node(tree, found, "flaky.bin", 0, 32 * 1024);
    tree.refresh_stats();

    orchestrator::ReadPipe rp = orchestrator::build_read_pipe(&dev, 8ull << 20, 256 * 1024);
    ReportIo io;
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter writer(io, rp.pipe, cfg);
    ASSERT_EQ(OK, writer.enqueue(f, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(0), stats.salvaged);
    EXPECT_EQ(u64(0), stats.errors);
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(Outcome::Done, items[0].outcome);
}

TEST(Writer, BareDeviceTruncatesWhatThePipeRetries) {
    /* the negative control, re-pinned by M-135: handed the BARE device
     * (no retry layer - the pre-M-136 production construction), one
     * transient fault truncates the item at the failing read (the og's
     * error-9 family: ReadError, cause 5) - the pipe path retries the
     * same fault and completes Done. The scan-vs-export disagreement
     * M-136 named is exactly this delta; the outcome contrast is the
     * pin. */
    std::vector<u8> img(64 * 1024, 0xCD);
    TransientDevice dev(std::move(img));
    dev.flaky_range(8 * 1024, 12 * 1024);

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* f = add_node(tree, found, "flaky.bin", 0, 32 * 1024);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(f, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.read_error);
    EXPECT_EQ(u64(0), stats.salvaged);
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(Outcome::ReadError, items[0].outcome);
    EXPECT_EQ(5u, items[0].cause_id);
    EXPECT_EQ(0u, items[0].bytes_written); /* the first read is the one
                                              that failed */
}

TEST(Writer, RecoveryCacheServesSharedExtents) {
    /* M-136: the CachedReader rides the writer's chain - two items over
     * the same extent read the DEVICE once (the second item is served
     * from the cache, the og's stream-mediated export posture) */
    std::vector<u8> img(64 * 1024, 0x5A);
    TransientDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* a = add_node(tree, found, "a.bin", 0, 16 * 1024);
    auto* b = add_node(tree, found, "b.bin", 0, 16 * 1024);
    tree.refresh_stats();

    orchestrator::ReadPipe rp = orchestrator::build_read_pipe(&dev, 8ull << 20, 256 * 1024);
    ReportIo io;
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter writer(io, rp.pipe, cfg);
    ASSERT_EQ(OK, writer.enqueue(a, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(b, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(0), stats.salvaged);
    /* one 256 KiB cache block fetch serves both items' 16 KiB reads */
    EXPECT_EQ(1u, dev.reads());
}

TEST(Writer, RecoveryEntryPointsConstructThroughBuildReadPipe) {
    /* M-136: both recovery entry points (the CLI recover verb + the ABI
     * recovery_open) build the writer's source through the scan's own
     * seam with the scan's constants - grep pin, the M-058/test_cached
     * precedent for construction-site proofs */
    const auto cli = cr_test::read_repo_file("src/app/rrecov.cpp");
    ASSERT_FALSE(cli.empty());
    EXPECT_NE(std::string::npos,
              cli.find("build_read_pipe(src.device.get(), 8ull << 20, 256 * 1024)"))
        << "the CLI recover verb must construct the writer's source through build_read_pipe";
    const auto abi = cr_test::read_repo_file("src/ui/cr_ui.cpp");
    ASSERT_FALSE(abi.empty());
    EXPECT_NE(std::string::npos, abi.find("build_read_pipe(src, 8ull << 20, 256 * 1024)"))
        << "the ABI recovery_open must construct the writer's source through build_read_pipe";
}

/* ---- M-135: the og's error-9 family (truncate + keep + cause) ------- */

TEST(Writer, HardReadFailureTruncatesAndKeeps) {
    /* the finding's headline case: through the M-136 pipe, a HARD read
     * failure (all 3 retry attempts exhausted - the pipe's error map
     * carries the range) truncates the item at the FIRST unreadable
     * byte, commits the partial, and records cause 5. A 32 KiB file
     * whose bad spot sits at 12 KiB commits exactly 12 KiB. */
    std::vector<u8> img(64 * 1024, 0x7E);
    std::unique_ptr<IStorageDevice> bare(new MemDevice(std::move(img)));
    static_cast<MemDevice*>(bare.get())->fail_range(12 * 1024, 12 * 1024 + 512);

    /* the production chain: build_read_pipe over the flaky device */
    orchestrator::ReadPipe rp = orchestrator::build_read_pipe(bare.get(), 8ull << 20, 256 * 1024);
    ASSERT_NE(nullptr, rp.pipe);
    ASSERT_NE(nullptr, rp.retry);

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* f = add_node(tree, found, "holed.bin", 0, 32 * 1024);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter writer(io, rp.pipe, cfg);
    writer.set_read_error_source(rp.retry);
    ASSERT_EQ(OK, writer.enqueue(f, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    /* the pass COMPLETES (WRITER-1) and the partial IS committed - the
     * og's break-and-keep, never a delete and never a zero-fill */
    EXPECT_EQ(u64(1), stats.read_error);
    EXPECT_EQ(u64(0), stats.errors);
    EXPECT_EQ(u64(0), stats.salvaged);
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(Outcome::ReadError, items[0].outcome);
    EXPECT_EQ(5u, items[0].cause_id);
    EXPECT_EQ(u64(12 * 1024), items[0].first_failed_offset);
    EXPECT_EQ(u64(12 * 1024), items[0].bytes_written);
    EXPECT_EQ(u64(32 * 1024), items[0].bytes);
}

TEST(Writer, ReadErrorLogTriFormat) {
    /* the og's three-line read-failure family, format strings verbatim
     * (DRWUI :875684-875691): "Failed to ReadSector! " + the
     * m_s64StartSector/pDataRuns64LCN/m_nSectorPerCluster line + the
     * StartSector/ReadCount/SectorTotalSize line, then the recorder's
     * "Failed to recover file: %s". Captured through the log sink. */
    std::vector<std::pair<cr::log::Level, std::string>> lines;
    cr::log::set_sink(
        [](cr::log::Level lv, const char* line, void* user) {
            static_cast<std::vector<std::pair<cr::log::Level, std::string>>*>(user)->emplace_back(
                lv, line);
        },
        &lines);

    std::vector<u8> img(64 * 512, 0);
    MemDevice dev(std::move(img));
    dev.fail_range(0, 512); /* the very first read fails (bare device) */

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* f = add_node(tree, found, "dead.bin", 0, 4096);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(f, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));

    cr::log::set_sink(nullptr, nullptr);

    /* the trio + the recorder line, in order, Error level */
    ASSERT_GE(lines.size(), 4u);
    EXPECT_EQ(cr::log::Level::Error, lines[0].first);
    EXPECT_EQ("Failed to ReadSector! ", lines[0].second);
    EXPECT_EQ(cr::log::Level::Error, lines[1].first);
    EXPECT_NE(std::string::npos, lines[1].second.find("m_s64StartSector:0,"));
    EXPECT_NE(std::string::npos, lines[1].second.find("pDataRuns64LCN:0,"));
    EXPECT_NE(std::string::npos, lines[1].second.find("m_nSectorPerCluster:1"));
    EXPECT_EQ(cr::log::Level::Error, lines[2].first);
    EXPECT_NE(std::string::npos, lines[2].second.find("StartSector:0,"));
    EXPECT_NE(std::string::npos, lines[2].second.find("ReadCount:8,"));
    EXPECT_NE(std::string::npos, lines[2].second.find("SectorTotalSize:64"));
    EXPECT_EQ(cr::log::Level::Error, lines[3].first);
    EXPECT_NE(std::string::npos, lines[3].second.find("Failed to recover file: dead.bin"));
}

TEST(Writer, FailureCauseTableMatchesOgRuntimeStrings) {
    /* the compiled-in cause list (DRWUI strings :30580-30596), seven
     * entries, corrected first-hand: cause 4 is the sector-location
     * string (NOT the qm file's "file name is most likely lost" - that
     * is a search-help string) and cause 7 exists ("Other unknown
     * reasons") - the finding's six-string citation corrected. */
    EXPECT_EQ(nullptr, failure_cause_text(0));
    EXPECT_STREQ("1.Insufficient storage space.", failure_cause_text(1));
    EXPECT_STREQ("2.The recovery path is too long.", failure_cause_text(2));
    EXPECT_STREQ("3.The file name contains garbage characters.", failure_cause_text(3));
    EXPECT_STREQ("4.The sector reading location exceeds total sector number of the source "
                 "partition.",
                 failure_cause_text(4));
    EXPECT_STREQ("5.The correct storage sector location cannot be found.", failure_cause_text(5));
    EXPECT_STREQ("6.The data amount you want to recover has exceeded the free quota.",
                 failure_cause_text(6));
    EXPECT_STREQ("7.Other unknown reasons.", failure_cause_text(7));
    EXPECT_EQ(nullptr, failure_cause_text(8));
}

TEST(Writer, BeyondVolumeReadRecordsCauseFourAndClampsStraddle) {
    /* the og's boundary ladder: a read that STARTS inside the volume
     * but ENDS past it reads the remaining tail (the :875666-68 count
     * clamp) and the item CONTINUES - the next read starts at/after the
     * end, the engine volume device's start gate fails (the S-052/M-119
     * probe: `(0 <= start < length) && count <= length`), the runlist
     * read fails, and the file truncates THERE: ReadError with cause 4
     * ("the sector reading location exceeds total sector number of the
     * source partition") keeping the in-volume tail. A read that starts
     * BEYOND the end truncates at zero. Neither is a whole-item
     * refusal and neither deletes the partial. */
    std::vector<u8> img(8 * 1024, 0x33);
    MemDevice dev(std::move(img)); /* 8 KiB volume */

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    /* straddler: [4 KiB, 12 KiB) - the clamp reads [4 KiB, 8 KiB), the
     * next read starts at the end: ReadError at 4 KiB committed */
    auto* straddler = add_node(tree, found, "straddler.bin", 4096, 8192);
    /* beyond: starts past the end - cause 4, truncated at 0 bytes */
    auto* beyond = add_node(tree, found, "beyond.bin", 12 * 1024, 4096);
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(straddler, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(beyond, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));

    const auto items = writer.report_items();
    ASSERT_EQ(2u, items.size());
    EXPECT_EQ(Outcome::ReadError, items[0].outcome); /* truncated at the
                                                        volume end */
    EXPECT_EQ(4u, items[0].cause_id);
    EXPECT_EQ(u64(4096), items[0].bytes_written); /* the clamped tail
                                                      IS committed */
    EXPECT_EQ(u64(4096), items[0].first_failed_offset);
    EXPECT_EQ(Outcome::ReadError, items[1].outcome);
    EXPECT_EQ(4u, items[1].cause_id);
    EXPECT_EQ(0u, items[1].bytes_written);
}

TEST(Writer, ShortExtentMetadataCommitsPartial) {
    /* the old ERR_TRUNCATED refusal deleted the partial; the og's
     * runlist path simply ENDS the file where the runs end (the loop
     * drains, no failure on the og side - its engines never emit short
     * runlists). The core commits the partial and records cause 7 (the
     * report's superset truth: no og cause string names a metadata
     * shortfall). */
    std::vector<u8> img(8 * 1024, 0x44);
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* shortf = new tree::FileProperties();
    shortf->name = u"short.bin";
    shortf->extents.push_back({0, 2048}); /* logical 4096, extents 2048 */
    shortf->logical_size = 4096;
    tree::FileNode* short_n = tree.add_file(found, shortf);
    shortf->release();
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(short_n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.read_error);
    EXPECT_EQ(u64(0), stats.errors); /* the item is no longer a hard error */
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(Outcome::ReadError, items[1 - 1].outcome);
    EXPECT_EQ(7u, items[0].cause_id);
    EXPECT_EQ(2048u, items[0].bytes_written);
    EXPECT_EQ(4096u, items[0].bytes);
}

TEST(Writer, CompressedUndecodableBlockStillSalvages) {
    /* M-135 keeps the zero-fill policy for the class the og's chunked
     * path itself zeroes: an undecodable COMPRESSED block zero-fills
     * and the item completes at full logical size as Salvaged (FB-R02's
     * surviving surface - the per-failure-class split the finding's fix
     * plan step 1 prescribed). A zlib extent (codec 1) whose packed
     * bytes are garbage. */
    std::vector<u8> img(8 * 1024, 0);
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    auto* comp = new tree::FileProperties();
    comp->name = u"comp.bin";
    tree::FileProperties::Extent e;
    e.offset = 0;
    e.length = 4096; /* logical */
    e.psize = 512;   /* packed garbage */
    e.codec = 1;     /* zlib - decompress_block fails on garbage */
    comp->extents.push_back(e);
    comp->logical_size = 4096;
    tree::FileNode* comp_n = tree.add_file(found, comp);
    comp->release();
    tree.refresh_stats();

    ReportIo io;
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter writer(io, &dev, cfg);
    ASSERT_EQ(OK, writer.enqueue(comp_n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.salvaged);
    EXPECT_EQ(u64(0), stats.read_error);
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(Outcome::Salvaged, items[0].outcome);
    EXPECT_EQ(0u, items[0].cause_id);
}
