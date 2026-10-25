/* test_health_grade.cpp - S-024 acceptance: the og DiskHealth 4-status
 * grading rules (Unknown/Good/Caution/Bad) - attr 5/197/198 value-vs-
 * threshold + raw-count rules, attr 1 read-error-rate, the SSD-life
 * vendor set, temperature class, and the NVMe mapping.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/health/cr_grade.h"

using namespace cr;
using namespace cr::health;

namespace {

SmartAttrInput attr(u8 id, u16 value, u64 raw, u16 threshold = 0, bool failing = false) {
    SmartAttrInput a;
    a.id = id;
    a.value = value;
    a.raw[0] = u8(raw & 0xFF);
    a.raw[1] = u8((raw >> 8) & 0xFF);
    a.raw[2] = u8((raw >> 16) & 0xFF);
    a.vendor_threshold = threshold;
    a.failing_bit = failing;
    return a;
}

} // namespace

TEST(Health, Attr197Raw3IsCaution) {
    /* the finding's falsifiable case: attr 197 raw=3 value=100 ->
     * og Caution; ours was failing=false */
    SmartGradeInputs in;
    in.attrs.push_back(attr(197, 100, 3));
    std::vector<HealthGrade> grades;
    EXPECT_EQ(HealthGrade::Caution, grade_smart(in, grades));
    ASSERT_EQ(size_t(1), grades.size());
    EXPECT_EQ(HealthGrade::Caution, grades[0]);
}

TEST(Health, ValueBelowVendorThresholdIsBad) {
    SmartGradeInputs in;
    in.attrs.push_back(attr(5, 4, 0, 6)); /* reallocated, value 4 < thr 6 */
    std::vector<HealthGrade> grades;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(in, grades));
    /* threshold unavailable: only the raw-count rule applies */
    SmartGradeInputs in2;
    in2.attrs.push_back(attr(5, 4, 0, 0));
    EXPECT_EQ(HealthGrade::Good, grade_smart(in2, grades));
    /* value at/above threshold + raw 0 -> Good */
    SmartGradeInputs in3;
    in3.attrs.push_back(attr(198, 6, 0, 6));
    EXPECT_EQ(HealthGrade::Good, grade_smart(in3, grades));
}

TEST(Health, SsdLifeTenPercentIsCaution) {
    /* the decoded SSD-life rule: 100 -> Good; >10 -> Good; <=10 ->
     * Caution; 0 -> Bad (attrs {169,173,177,187,202,209,230,231,232,233}
     * gated by is-SSD) */
    for (u8 id : {u8(169), u8(173), u8(177), u8(187), u8(202), u8(209), u8(230), u8(231), u8(232),
                  u8(233)}) {
        SmartGradeInputs in;
        in.is_ssd = true;
        in.attrs.push_back(attr(id, 100, 0));
        std::vector<HealthGrade> g1;
        EXPECT_EQ(HealthGrade::Good, grade_smart(in, g1)) << int(id) << " @100";

        SmartGradeInputs in2;
        in2.is_ssd = true;
        in2.attrs.push_back(attr(id, 11, 0));
        std::vector<HealthGrade> g2;
        EXPECT_EQ(HealthGrade::Good, grade_smart(in2, g2)) << int(id) << " @11";

        SmartGradeInputs in3;
        in3.is_ssd = true;
        in3.attrs.push_back(attr(id, 10, 0));
        std::vector<HealthGrade> g3;
        EXPECT_EQ(HealthGrade::Caution, grade_smart(in3, g3)) << int(id) << " @10";

        SmartGradeInputs in4;
        in4.is_ssd = true;
        in4.attrs.push_back(attr(id, 0, 0));
        std::vector<HealthGrade> g4;
        EXPECT_EQ(HealthGrade::Bad, grade_smart(in4, g4)) << int(id) << " @0";
    }
    /* the gate: HDDs do not SSD-grade these attrs (they follow the generic
     * Good path - a 231 raw-value on HDD is not a wear verdict) */
    SmartGradeInputs hdd;
    hdd.is_ssd = false;
    hdd.attrs.push_back(attr(231, 5, 0));
    std::vector<HealthGrade> gh;
    EXPECT_EQ(HealthGrade::Good, grade_smart(hdd, gh));
}

TEST(Health, Attr1ReadErrorRateRule) {
    /* :33527-33551: value==0 && raw==0 or threshold<=value -> Good else Bad */
    std::vector<HealthGrade> g;
    SmartGradeInputs in;
    in.attrs.push_back(attr(1, 0, 0)); /* zero everywhere -> Good */
    EXPECT_EQ(HealthGrade::Good, grade_smart(in, g));
    SmartGradeInputs in2;
    in2.attrs.push_back(attr(1, 100, 456)); /* value fine, raw high */
    EXPECT_EQ(HealthGrade::Good, grade_smart(in2, g));
    SmartGradeInputs in3;
    in3.attrs.push_back(attr(1, 60, 456, 100)); /* value 60 < thr 100 -> Bad */
    EXPECT_EQ(HealthGrade::Bad, grade_smart(in3, g));
}

TEST(Health, TemperatureClassCaution) {
    std::vector<HealthGrade> g;
    SmartGradeInputs hdd;
    hdd.attrs.push_back(attr(194, 200, 51)); /* raw[0] = 51C on HDD */
    EXPECT_EQ(HealthGrade::Caution, grade_smart(hdd, g));
    SmartGradeInputs hdd_ok;
    hdd_ok.attrs.push_back(attr(194, 200, 49)); /* 49C HDD */
    EXPECT_EQ(HealthGrade::Good, grade_smart(hdd_ok, g));
    SmartGradeInputs ssd;
    ssd.is_ssd = true;
    ssd.attrs.push_back(attr(194, 200, 51)); /* 51C on SSD: below 60 */
    EXPECT_EQ(HealthGrade::Good, grade_smart(ssd, g));
    SmartGradeInputs ssd_hot;
    ssd_hot.is_ssd = true;
    ssd_hot.attrs.push_back(attr(194, 200, 61)); /* 61C on SSD */
    EXPECT_EQ(HealthGrade::Caution, grade_smart(ssd_hot, g));
}

TEST(Health, FailingBitIsHardBadAndWorstWins) {
    std::vector<HealthGrade> g;
    SmartGradeInputs in;
    in.attrs.push_back(attr(197, 100, 3)); /* Caution */
    in.attrs.push_back(attr(187, 0, 0));   /* generic id */
    in.attrs.back().failing_bit = true;    /* hard Bad */
    EXPECT_EQ(HealthGrade::Bad, grade_smart(in, g));
    EXPECT_EQ(HealthGrade::Caution, g[0]);
    EXPECT_EQ(HealthGrade::Bad, g[1]);
    /* empty -> Unknown */
    SmartGradeInputs empty;
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(empty, g));
}

TEST(Health, NvmeGradeMapping) {
    NvmeHealthInfo h;
    h.available = true;
    h.percentage_used = 5;
    h.critical_warning = 0;
    h.media_errors = 0;
    EXPECT_EQ(HealthGrade::Good, grade_nvme(h));

    h.percentage_used = 91; /* remaining 9 <= 10 -> Caution */
    EXPECT_EQ(HealthGrade::Caution, grade_nvme(h));
    h.percentage_used = 100; /* exhausted -> Bad */
    EXPECT_EQ(HealthGrade::Bad, grade_nvme(h));

    h.percentage_used = 5;
    h.critical_warning = 0x04; /* reliability degraded -> Bad */
    EXPECT_EQ(HealthGrade::Bad, grade_nvme(h));
    h.critical_warning = 0x01; /* spare below threshold -> Bad */
    EXPECT_EQ(HealthGrade::Bad, grade_nvme(h));
    h.critical_warning = 0x03; /* bit 1 (temp) only escalates within Caution */
    EXPECT_EQ(HealthGrade::Bad, grade_nvme(h));
    h.critical_warning = 0x02; /* temperature -> Caution */
    EXPECT_EQ(HealthGrade::Caution, grade_nvme(h));
    h.critical_warning = 0x10; /* volatile backup -> Caution */
    EXPECT_EQ(HealthGrade::Caution, grade_nvme(h));

    h.critical_warning = 0;
    h.media_errors = 1; /* the pending-sectors analog */
    EXPECT_EQ(HealthGrade::Caution, grade_nvme(h));

    NvmeHealthInfo unavailable;
    EXPECT_EQ(HealthGrade::Unknown, grade_nvme(unavailable));
}

TEST(Health, GradeNames) {
    EXPECT_STREQ("Unknown", health_grade_name(HealthGrade::Unknown));
    EXPECT_STREQ("Good", health_grade_name(HealthGrade::Good));
    EXPECT_STREQ("Caution", health_grade_name(HealthGrade::Caution));
    EXPECT_STREQ("Bad", health_grade_name(HealthGrade::Bad));
}
