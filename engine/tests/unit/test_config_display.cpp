/* test_config_display.cpp - N-132: the two display-config data layers
 * (the FilterFolder.ini SumaryFileType unification groups + the
 * PriorityDisplayType.xml Type-tree ordering), verbatim from the corpus
 * config files.
 */

#include <gtest/gtest.h>

#include <cr/orchestrator/cr_scan.h>
#include <cr/query/cr_query.h>

using namespace cr;

TEST(ConfigDisplay, SummaryFamilyUnificationGroups) {
    /* N-132: the 11 groups, verbatim from FilterFolder.ini [Filter]
     * SumaryFileType - the members fold into their family. */
    struct Case {
        const char* ext;
        const char* family;
    };
    for (const Case& c : {Case{"aiff", "aiff_aif"},
                          Case{"aif", "aiff_aif"},
                          Case{"m4p", "m4p_m4a_m4v_mp4"},
                          Case{"m4a", "m4p_m4a_m4v_mp4"},
                          Case{"m4v", "m4p_m4a_m4v_mp4"},
                          Case{"mp4", "m4p_m4a_m4v_mp4"},
                          Case{"html", "html_htm"},
                          Case{"htm", "html_htm"},
                          Case{"rm", "rm_rmvb"},
                          Case{"rmvb", "rm_rmvb"},
                          Case{"mkv", "mkv_webm"},
                          Case{"webm", "mkv_webm"},
                          Case{"jpg", "jpg_jpeg"},
                          Case{"jpeg", "jpg_jpeg"},
                          Case{"tif", "tif_tiff"},
                          Case{"tiff", "tif_tiff"},
                          Case{"ai", "ai_pdf"},
                          Case{"pdf", "ai_pdf"},
                          Case{"zip", "zip_docx_pptx_xlsx"},
                          Case{"docx", "zip_docx_pptx_xlsx"},
                          Case{"pptx", "zip_docx_pptx_xlsx"},
                          Case{"xlsx", "zip_docx_pptx_xlsx"}}) {
        EXPECT_STREQ(query::summary_family_for_extension(c.ext), c.family) << "extension " << c.ext;
    }
}

TEST(ConfigDisplay, SummaryFamilyUngroupedStandsAlone) {
    /* N-132: an extension in NO group returns NULL (stands alone) - e.g.
     * "png" is Picture-category but no group folds it. */
    for (const char* e : {"png", "gif", "doc", "mp3", "exe", "txt"}) {
        EXPECT_EQ(query::summary_family_for_extension(e), nullptr) << "extension " << e;
    }
}

TEST(ConfigDisplay, SummaryFamilyNullAndEmptySafe) {
    EXPECT_EQ(query::summary_family_for_extension(nullptr), nullptr);
    EXPECT_EQ(query::summary_family_for_extension(""), nullptr);
}

TEST(ConfigDisplay, TypeDisplayOrderVerbatim) {
    /* N-132: PriorityDisplayType.xml - the alias ranks within the 5
     * categories, verbatim (jpg=00 < png=01 < jpeg=02... zip-family
     * first in Archive). */
    struct Case {
        const char* ext;
        query::TypeDisplayCategory cat;
        u32 rank;
    };
    for (const Case& c : {Case{"jpg", query::TypeDisplayCategory::Picture, 0},
                          Case{"png", query::TypeDisplayCategory::Picture, 1},
                          Case{"jpeg", query::TypeDisplayCategory::Picture, 2},
                          Case{"ico", query::TypeDisplayCategory::Picture, 8},
                          Case{"mp4", query::TypeDisplayCategory::Video, 0},
                          Case{"mov", query::TypeDisplayCategory::Video, 1},
                          Case{"wmv", query::TypeDisplayCategory::Video, 6},
                          Case{"doc", query::TypeDisplayCategory::Document, 0},
                          Case{"pdf", query::TypeDisplayCategory::Document, 2},
                          Case{"pptx", query::TypeDisplayCategory::Document, 6},
                          Case{"mp3", query::TypeDisplayCategory::Audio, 0},
                          Case{"flac", query::TypeDisplayCategory::Audio, 5},
                          Case{"aac", query::TypeDisplayCategory::Audio, 6},
                          Case{"zip", query::TypeDisplayCategory::Archive, 0},
                          Case{"rar", query::TypeDisplayCategory::Archive, 1},
                          Case{"7z", query::TypeDisplayCategory::Archive, 2},
                          Case{"gzip", query::TypeDisplayCategory::Archive, 4}}) {
        query::TypeDisplayCategory cat = query::TypeDisplayCategory::Picture;
        u32 rank = 0xFFFF;
        ASSERT_TRUE(query::type_display_order(c.ext, cat, rank)) << "extension " << c.ext;
        EXPECT_EQ(u32(cat), u32(c.cat)) << "extension " << c.ext;
        EXPECT_EQ(rank, c.rank) << "extension " << c.ext;
    }
}

TEST(ConfigDisplay, TypeDisplayOrderUnrankedReturnsFalse) {
    /* N-132: extensions outside the 35-entry tier have no rank - the og's
     * Type tab lists them after the ranked set, unordered. */
    query::TypeDisplayCategory cat = query::TypeDisplayCategory::Picture;
    u32 rank = 0;
    for (const char* e : {"nef", "cr2", "m4r", "cab", "iso", ""}) {
        EXPECT_FALSE(query::type_display_order(e, cat, rank)) << "extension " << e;
    }
    EXPECT_EQ(query::type_display_order(nullptr, cat, rank), false);
}

TEST(ConfigDisplay, TypeDisplayCategoryNamesVerbatim) {
    /* N-132: the XML's Description attributes verbatim. */
    EXPECT_STREQ(query::type_display_category_name(query::TypeDisplayCategory::Picture), "Picture");
    EXPECT_STREQ(query::type_display_category_name(query::TypeDisplayCategory::Archive), "Archive");
}

/* ---- M-125: the og's progress display math (the DRWUI decode) ---- */

TEST(ProgressDisplay, RemapCurveTable) {
    /* M-125: FUN_1401af820 verbatim - the five segments with the
     * corpus's constants. The finding's own table carried four
     * transcription errors against these constants (raw 20 -> "22" not
     * 21.8; raw 29 -> "23.8" not 23.6; raw 98 -> "99.4" not 99.0; raw
     * 99 -> "99.99" not 99.0 - the stall cap only fires for raw > 100):
     * the corpus constants win (DRWUI :314931-314959 - 0.2/20.0/0.8/
     * 2.7/72.0 and the identity tail). */
    const struct {
        double raw, want;
    } cases[] = {
        {0.0, 0.0},   {5.0, 10.0},   {10.0, 20.0}, {10.9, 21.8},   {11.0, 20.0}, {20.0, 21.8},
        {29.0, 23.6}, {29.9, 23.78}, {30.0, 24.0}, {50.0, 40.0},   {90.0, 72.0}, {90.1, 77.67},
        {98.0, 99.0}, {98.5, 98.5},  {99.0, 99.0}, {100.0, 100.0},
    };
    for (const auto& c : cases)
        EXPECT_NEAR(query::progress_display_remap(c.raw), c.want, 1e-9) << "raw " << c.raw;
    /* the 98-boundary discontinuity: the 2.7 segment ends at 98 (-> 99.0)
     * and the identity tail takes over (98.5 -> 98.5, LOWER than 99.0) -
     * the og's curve is non-monotonic at the boundary, which is exactly
     * what FUN_1401afa60's monotonic publish guard exists to hide (the
     * display never goes DOWN; ProgressDisplay.MonotonicPublishGuard) */
    EXPECT_GT(query::progress_display_remap(98.0), query::progress_display_remap(98.5));
}

TEST(ProgressDisplay, StallCapIsTheDefensiveBranch) {
    /* M-125: the 99.99 stall cap fires ONLY when the remap output
     * exceeds 100 with raw != 100 - the 2.7 segment tops at 99.0 (raw
     * 98) and the identity tail stays <= 100 for raw in (98,100], so
     * the cap is the og's dead defensive branch, reachable only for
     * the out-of-range raw > 100 (ported as decoded and proven live
     * for that input). */
    EXPECT_NEAR(query::progress_display_remap(98.5), 98.5, 1e-9); /* the identity tail, no cap */
    EXPECT_NEAR(query::progress_display_remap(101.0), 99.99, 1e-9);
    EXPECT_NEAR(query::progress_display_remap(250.0), 99.99, 1e-9);
    EXPECT_NEAR(query::progress_display_remap(100.0), 100.0, 1e-9);
}

TEST(ProgressDisplay, PercentTypeDispatch) {
    /* M-125: FUN_1401af950 - type 0 -> 0.0 (the reset), type 1 ->
     * REMAP(raw) with the outer 100.0 clamp, the other types -> raw. */
    EXPECT_NEAR(query::progress_display_percent(0, 500, 1000), 0.0, 1e-9);
    /* raw 5 (done 50 of 1000) -> type 1 -> REMAP(5) = 10 */
    EXPECT_NEAR(query::progress_display_percent(1, 50, 1000), 10.0, 1e-9);
    /* raw 50 -> type 1 -> 40 (the og's mid-scan ~30% display dip) */
    EXPECT_NEAR(query::progress_display_percent(1, 500, 1000), 40.0, 1e-9);
    /* raw 100 -> 100 (the completion) */
    EXPECT_NEAR(query::progress_display_percent(1, 1000, 1000), 100.0, 1e-9);
    /* total < 1 -> 0 (the og's divide guard) */
    EXPECT_NEAR(query::progress_display_percent(1, 10, 0), 0.0, 1e-9);
    /* the non-remapped types: raw as-is */
    EXPECT_NEAR(query::progress_display_percent(2, 500, 1000), 50.0, 1e-9);
}

TEST(ProgressDisplay, MonotonicPublishGuard) {
    /* M-125: FUN_1401afa60 - a lower value never lowers the display;
     * the completion message bypasses the guard with its forced 100. */
    EXPECT_NEAR(query::progress_display_publish(40.0, 39.0), 40.0, 1e-9);
    EXPECT_NEAR(query::progress_display_publish(40.0, 41.0), 41.0, 1e-9);
    EXPECT_NEAR(query::progress_display_publish(99.0, 100.0), 100.0, 1e-9);
}

TEST(ProgressDisplay, CorePermilleStaysLinear) {
    /* M-125: the engine's own telemetry stays the TRUTHFUL linear
     * weighted permille (cr_scan's weighted_progress, 40/35/25) - the
     * remap is the og's DISPLAY layer, a query-side helper; the
     * engine never lies in its telemetry (the deliberate divergence
     * registered in PARITY-DELTAS). */
    const orchestrator::StageWeights w{40, 35, 25};
    EXPECT_EQ(orchestrator::weighted_progress(w, 0, 0, 0), 0u);
    EXPECT_EQ(orchestrator::weighted_progress(w, 1000, 0, 0), 400u);
    EXPECT_EQ(orchestrator::weighted_progress(w, 1000, 1000, 0), 750u);
    EXPECT_EQ(orchestrator::weighted_progress(w, 1000, 1000, 1000), 1000u);
    EXPECT_EQ(orchestrator::weighted_progress(w, 500, 500, 500), 500u);
}

/* ===================================================================== */
/* M-126 (diff-to-fix): the og's ETA model (FUN_1401af410,             */
/* DRWUI :314835-314887, re-read first-hand) - the whole-run base ETA, */
/* the 0.2/0.8 rate EMA, the type-gated time-budget variant (x0.6),   */
/* the "3m00s" placeholder, and the 1 Hz "XmYs" countdown.             */
/* ===================================================================== */

TEST(Eta, WholeRunModel) {
    /* the finding's worked example: 40 of 100 (GB/permille/any unit)
     * in 400 s -> eta = (100-40)*400/40 = 600 s remaining */
    const query::EtaModel em = query::eta_compute(40, 100, 400'000ull, 0, false, 0);
    EXPECT_EQ(600, em.eta_seconds);
    EXPECT_EQ(0u, em.smoothed_rate) << "first sample, no history: rate passes through 0? no -";
    /* the rate: (100-40)/600 = 0 units/s (integer!) - the og's integer
     * math yields rate 0 here; with prev==0 the smoothed state stores
     * 0 (no history established until a nonzero rate). Verify the
     * model on a shape where the rate is nonzero: 40/100 in 4 s. */
    const query::EtaModel em2 = query::eta_compute(40, 100, 4'000ull, 0, false, 0);
    EXPECT_EQ(6, em2.eta_seconds);
    EXPECT_EQ(10u, em2.smoothed_rate) << "(100-40)/6 = 10 units/s, no history";
}

TEST(Eta, EmaDamping) {
    /* the base shape: 80/100 in 10 s -> eta = 20*10000/80/1000 = 2 s,
     * instant rate = 20/2 = 10 units/s */
    const query::EtaModel first = query::eta_compute(80, 100, 10'000ull, 0, false, 0);
    ASSERT_EQ(2, first.eta_seconds);
    ASSERT_EQ(10u, first.smoothed_rate) << "prev == 0: the first sample passes through";
    /* settled: prev == instant == 10 -> stays 10 */
    const query::EtaModel settled = query::eta_compute(80, 100, 10'000ull, 10, false, 0);
    EXPECT_EQ(10u, settled.smoothed_rate) << "(2*10 + 8*10)/10 = 10";
    /* a DOWN burst against history 100: (2*10 + 8*100)/10 = 82 - the
     * state moves exactly 20% of the gap (100 -> 82), the og's 0.2/0.8
     * arithmetic with integer truncation */
    const query::EtaModel down = query::eta_compute(80, 100, 10'000ull, 100, false, 0);
    EXPECT_EQ(82u, down.smoothed_rate) << "(2*10 + 8*100)/10 = 82";
    /* an UP step against history 2: (2*10 + 8*2)/10 = 3.6 -> 3 */
    const query::EtaModel up = query::eta_compute(80, 100, 10'000ull, 2, false, 0);
    EXPECT_EQ(3u, up.smoothed_rate) << "(2*10 + 8*2)/10 = 3.6 -> 3 (integer)";
    /* a slower-progress shape: 20/100 in 4 s -> eta = 16 s, rate = 5;
     * against history 28: (2*5 + 8*28)/10 = 23.4 -> 23 */
    const query::EtaModel slow = query::eta_compute(20, 100, 4'000ull, 28, false, 0);
    EXPECT_EQ(16, slow.eta_seconds);
    EXPECT_EQ(23u, slow.smoothed_rate) << "(2*5 + 8*28)/10 = 23.4 -> 23 (integer)";
}

TEST(Eta, TimeBudgetVariant) {
    /* the type-gated leg: projected = total*elapsed_s/done (floored 1);
     * field = (budget - budget*elapsed_s/projected)*0.6.
     * Worked: total=100, done=40, elapsed=400s -> projected = 100*400/40
     * = 1000 (the whole run); budget=1000s -> field = (1000 -
     * 1000*400/1000)*0.6 = 600*0.6 = 360 */
    const query::EtaModel em = query::eta_compute(40, 100, 400'000ull, 0, true, 1000ull);
    EXPECT_EQ(360, em.eta_seconds) << "(1000 - 1000*400/1000)*0.6 = 360";
    /* over-budget shape: budget 500 vs projected 1000, 400s elapsed ->
     * (500 - 500*400/1000)*0.6 = (500-200)*0.6 = 180 */
    const query::EtaModel over = query::eta_compute(40, 100, 400'000ull, 0, true, 500ull);
    EXPECT_EQ(180, over.eta_seconds);
}

TEST(Eta, PlaceholderAndCountdownFormat) {
    /* the computed render: both fields zero-padded (the runtime anchor
     * "07m43s"); the placeholder is the og's literal "3m00s" */
    EXPECT_EQ("07m43s", query::eta_format_m_s(463));
    EXPECT_EQ("10m00s", query::eta_format_m_s(600));
    EXPECT_EQ("00m59s", query::eta_format_m_s(59));
    EXPECT_EQ("3m00s", query::eta_format_m_s(-1)) << "the empty-label placeholder";
    /* the countdown: parse "<m>m<s>s", subtract a second; stop at zero */
    EXPECT_EQ(462, query::eta_countdown_tick("07m43s"));
    EXPECT_EQ(462, query::eta_countdown_tick("7m43s")) << "unpadded re-renders parse too";
    EXPECT_EQ(179, query::eta_countdown_tick("3m00s")) << "the placeholder ticks like any label";
    EXPECT_EQ(-1, query::eta_countdown_tick("0m00s")) << "reached zero: the slot stops";
    EXPECT_EQ(-1, query::eta_countdown_tick("")) << "unparsable: no tick";
    EXPECT_EQ(-1, query::eta_countdown_tick("garbage")) << "unparsable: no tick";
}

TEST(Eta, ParameterErrorClass) {
    /* FUN_1401af410's opening gate: done==0, total<done, total==0 -> -1 */
    EXPECT_EQ(-1, query::eta_compute(0, 100, 60'000ull, 0, false, 0).eta_seconds);
    EXPECT_EQ(-1, query::eta_compute(150, 100, 60'000ull, 0, false, 0).eta_seconds);
    EXPECT_EQ(-1, query::eta_compute(50, 0, 60'000ull, 0, false, 0).eta_seconds);
    /* eta <= 0 (too little elapsed): no estimate */
    EXPECT_EQ(-1, query::eta_compute(99, 100, 100ull, 0, false, 0).eta_seconds);
}
