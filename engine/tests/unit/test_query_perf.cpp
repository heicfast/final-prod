/* test_query_perf.cpp - G-12 performance gate: filter+count over 1M
 * synthetic nodes must complete in < 200 ms on CI-class hardware (the
 * measured value is printed; the gate is the assertion). The paged sorted
 * path is additionally measured (informational, no gate).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/query/cr_query.h"
#include <chrono>

using namespace cr;
using namespace cr::query;

namespace {

tree::FileNode* add_fast(tree::ResultTree& t, tree::FileNode* parent, const std::u16string& name,
                         u64 size, u64 modified) {
    auto* p = new tree::FileProperties();
    p->name = name;
    p->logical_size = size;
    p->times.modified = modified;
    tree::FileNode* n = t.add_file(parent, p);
    p->release();
    return n;
}

} // namespace

TEST(QueryPerf, MillionNodesFilterCountUnder200ms) {
    /* 1M flat nodes under one root (the arena is built for exactly this) */
    tree::ResultTree t;
    auto* found = t.add_root(u"Found");
    constexpr u64 kN = 1'000'000;
    std::u16string base;
    char name[32];
    for (u64 i = 0; i < kN; ++i) {
        snprintf(name, sizeof(name), "f%09llu", (unsigned long long)i);
        base.clear();
        for (const char* q = name; *q; ++q)
            base.push_back(u16(u8(*q)));
        add_fast(t, found, base, i % 100'000, i);
    }
    t.refresh_stats();
    ASSERT_EQ(kN, t.node_count() - 1); /* minus the root */

    /* filter + count (no name-dependent predicates: the hot path) */
    Query q;
    q.filter.size_min = 10'000;
    q.filter.size_max = 90'000;
    q.filter.modified_from = 500'000;
    q.filter.confidence_min = 0;
    q.count_only = true;
    const auto t0 = std::chrono::steady_clock::now();
    const Result r = run(t, q);
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0)
            .count();
    printf("[query-perf] 1M nodes filter+count: %lld ms (matched %llu)\n", (long long)ms,
           (unsigned long long)r.total_matched);
    /* sizes are i%100000; [10000,90000] inclusive covers 80001 values
     * per 100000 residues over 1M nodes => 10 * 80001 - (100000-10000) */
    u64 expect = 0;
    for (u64 i = 0; i < 1'000'000; ++i) {
        const u64 sz = i % 100'000;
        const u64 mod = i;
        if (sz >= 10'000 && sz <= 90'000 && mod >= 500'000)
            ++expect;
    }
    EXPECT_EQ(expect, r.total_matched);
    EXPECT_LT(ms, 200) << "1M-node filter+count exceeded the 200 ms budget";

    /* informational: sorted paged query over the same tree */
    Query q2;
    q2.filter = q.filter;
    q2.sort = SortKey::Size;
    q2.dir = SortDir::Desc;
    q2.limit = 100;
    const auto t1 = std::chrono::steady_clock::now();
    const Result r2 = run(t, q2);
    const auto ms2 =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t1)
            .count();
    printf("[query-perf] 1M nodes sorted page(100): %lld ms\n", (long long)ms2);
    ASSERT_EQ(100u, r2.rows.size());
    EXPECT_EQ(u64(90'000), r2.rows[0].size);      /* descending: largest first */
    EXPECT_EQ(u64(90'000), r2.rows[1].size);      /* duplicates exist (5x per
                                                     qualifying size value) */
    EXPECT_GE(r2.rows[0].size, r2.rows[99].size); /* monotonic page */
}
