/* test_health_grade.cpp - S-024 acceptance: the og DiskHealth 4-status
 * grading rules (Unknown/Good/Caution/Bad) re-decoded first-hand from
 * DiskHealth_decompiled.c: the disk grader FUN_0040cdf0 (:5755-6130)
 * - 5/197/198 value-vs-threshold + raw16-count caution with the all-FF
 * guard, attr 1 SandForce rule (:33523), the family-gated SSD-life
 * attr set (:5977-6098), the WDC/SanDisk attr-230 path (:6084), the
 * duplicate-id Unknown, the Unknown gates, the bad-count/caution/
 * graded-signal aggregation - plus the per-attr display chain
 * (:34148-34167), the vendor-family classifier (FUN_0040dad0
 * :6300-7000), the is-SSD model-string list, and the NVMe family-0x13
 * grade (:5795-5810).
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
    std::vector<HealthGrade> grades;
    SmartGradeInputs in;
    in.attrs.push_back(attr(5, 4, 0, 6)); /* reallocated, value 4 < thr 6 */
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

TEST(Health, Attr5RawAllFFSkipsCautionCheck) {
    /* the all-FF guard lives in the DISK grader only (:5943): vendors
     * fill "unsupported" with FFs, so raw16 0xFFFF must not read as a
     * huge pending count at disk level. The per-attr display rule
     * (FUN_00458350) has no such guard - it shows Caution. The og's
     * asymmetry, pinned exactly. */
    std::vector<HealthGrade> grades;
    SmartGradeInputs in;
    SmartAttrInput a = attr(197, 100, 0xFFFFFFFFu);
    a.raw[3] = 0xFF; /* the guard reads raw[0..3] (:5943) */
    in.attrs.push_back(a);
    EXPECT_EQ(HealthGrade::Good, grade_smart(in, grades));
    EXPECT_EQ(HealthGrade::Caution, grades[0]);
}

TEST(Health, DuplicateAttrIdYieldsUnknown) {
    /* :5826 - a duplicated attribute row means the table is corrupt:
     * the whole disk verdict is Unknown */
    std::vector<HealthGrade> grades;
    SmartGradeInputs in;
    in.attrs.push_back(attr(5, 100, 0));
    in.attrs.push_back(attr(5, 100, 0));
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(in, grades));
}

TEST(Health, NoGradedSignalYieldsUnknown) {
    /* the graded-signal semantics (:5825/:5831/:6016): a disk whose
     * attrs prove nothing gradeable is Unknown even when all rows are
     * healthy - an out-of-range attr alone never proves gradeability */
    std::vector<HealthGrade> grades;
    SmartGradeInputs in;
    in.attrs.push_back(attr(14, 100, 0)); /* out of the og's range table */
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(in, grades));
    /* a 5/197/198 row (HDD) proves gradeability */
    SmartGradeInputs in2;
    in2.attrs.push_back(attr(5, 100, 0));
    EXPECT_EQ(HealthGrade::Good, grade_smart(in2, grades));
    /* an SSD row with a vendor threshold proves it */
    SmartGradeInputs in3;
    in3.is_ssd = true;
    in3.attrs.push_back(attr(9, 90, 1234, 50));
    EXPECT_EQ(HealthGrade::Good, grade_smart(in3, grades));
}

TEST(Health, SsdAnyAttrBelowThresholdIsBad) {
    /* :5902-5906 - SSDs grade EVERY attr (except 194) against its
     * vendor threshold; HDDs only the in-range table */
    std::vector<HealthGrade> grades;
    SmartGradeInputs ssd;
    ssd.is_ssd = true;
    ssd.attrs.push_back(attr(190, 2, 40, 10)); /* airflow temp, value 2 < 10 */
    EXPECT_EQ(HealthGrade::Bad, grade_smart(ssd, grades));
    SmartGradeInputs hdd;
    hdd.attrs.push_back(attr(14, 2, 40, 10)); /* out of range for HDDs */
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(hdd, grades));
}

TEST(Health, TemperatureNeverAffectsTheDiskGrade) {
    /* re-decode: FUN_0040cdf0 has NO temperature branch; the 50/60
     * caution defaults (0x1808) are never read by the grader and temp
     * attrs display Good (:33596). A hot disk with only temp rows is
     * Unknown (nothing gradeable), and a hot disk with a healthy 5-row
     * stays Good */
    std::vector<HealthGrade> grades;
    SmartGradeInputs hot_only;
    hot_only.attrs.push_back(attr(194, 200, 51)); /* 51C */
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(hot_only, grades));
    EXPECT_EQ(HealthGrade::Good, grades[0]);

    SmartGradeInputs hot;
    hot.attrs.push_back(attr(5, 100, 0));
    hot.attrs.push_back(attr(194, 200, 95)); /* 95C - still Good */
    EXPECT_EQ(HealthGrade::Good, grade_smart(hot, grades));
    EXPECT_EQ(HealthGrade::Good, grades[1]);

    SmartGradeInputs airflow;
    airflow.attrs.push_back(attr(190, 200, 95));
    airflow.attrs.push_back(attr(5, 100, 0));
    EXPECT_EQ(HealthGrade::Good, grade_smart(airflow, grades));
    EXPECT_EQ(HealthGrade::Good, grades[0]);
}

TEST(Health, SsdLifeFamilyGatesAndBoundaries) {
    /* the family-gated SSD-life attr set (:5977-6098) - each attr with
     * the decoded family that gates it; value-form life 100 -> Good,
     * 11 -> Good, 10 -> Caution, 0 -> Bad */
    struct Case {
        u8 id;
        u16 family;
    };
    const Case cases[] = {
        {169, 0x14}, {173, 0xf}, {177, 6},   {187, 2}, {202, 8},
        {209, 3},    {231, 7},   {232, 0xc}, {233, 5},
    };
    for (const Case& c : cases) {
        SmartGradeInputs in;
        in.family = c.family;
        in.attrs.push_back(attr(c.id, 100, 0));
        std::vector<HealthGrade> g1;
        EXPECT_EQ(HealthGrade::Good, grade_smart(in, g1)) << "id " << int(c.id) << " @100";

        SmartGradeInputs in2;
        in2.family = c.family;
        in2.attrs.push_back(attr(c.id, 11, 0));
        std::vector<HealthGrade> g2;
        EXPECT_EQ(HealthGrade::Good, grade_smart(in2, g2)) << "id " << int(c.id) << " @11";

        SmartGradeInputs in3;
        in3.family = c.family;
        in3.attrs.push_back(attr(c.id, 10, 0));
        std::vector<HealthGrade> g3;
        EXPECT_EQ(HealthGrade::Caution, grade_smart(in3, g3)) << "id " << int(c.id) << " @10";

        SmartGradeInputs in4;
        in4.family = c.family;
        in4.attrs.push_back(attr(c.id, 0, 0));
        std::vector<HealthGrade> g4;
        EXPECT_EQ(HealthGrade::Bad, grade_smart(in4, g4)) << "id " << int(c.id) << " @0";

        /* no family match -> the attr is not SSD-life-graded: attr
         * display Good, disk Unknown (no graded signal from it) */
        SmartGradeInputs off;
        off.family = 0; /* generic - no gate fires */
        off.attrs.push_back(attr(c.id, 5, 0));
        std::vector<HealthGrade> g5;
        EXPECT_EQ(HealthGrade::Unknown, grade_smart(off, g5)) << "id " << int(c.id) << " ungated";
        EXPECT_EQ(HealthGrade::Good, g5[0]) << "id " << int(c.id) << " ungated display";
    }
}

TEST(Health, SsdLifeBelowVendorThresholdIsBad) {
    /* LAB_0040d54e (:6017): value-form life must also beat the vendor
     * threshold; raw-form vendors skip that test */
    std::vector<HealthGrade> g;
    SmartGradeInputs in; /* family 0x15 SKhynix gates 231+233 */
    in.family = 0x15;
    in.attrs.push_back(attr(233, 40, 0, 80)); /* value 40 < thr 80 */
    EXPECT_EQ(HealthGrade::Bad, grade_smart(in, g));

    /* Kingston (family 0x11) gates attr 231; raw-form: life = raw[0].
     * NOTE: "KINGSTON SA400..." matches NONE of the og's is-SSD strings
     * (0x1724 stays 0) - so the HDD in-range threshold rule ALSO
     * applies to its 231 row; with a threshold below value the disk
     * would go Bad from that rule. Threshold 0 isolates the life
     * path (the og's own behavior for drives with no 0xD1 entry). */
    SmartGradeInputs raw_form;
    raw_form.family = 0x11;
    raw_form.life_raw_form = true;
    raw_form.attrs.push_back(attr(231, 40, 7, 0)); /* life = raw[0] = 7 */
    std::vector<HealthGrade> g2;
    EXPECT_EQ(HealthGrade::Caution, grade_smart(raw_form, g2)); /* 7 <= 10 */
}

TEST(Health, SsdLifeInvertFormReads100MinusRaw) {
    /* SKhynix else-class: life = 100 - raw[0] (0x17e8) - the same
     * boundaries: life 0 -> Bad (exhausted), 1..10 -> Caution */
    std::vector<HealthGrade> g;
    SmartGradeInputs in;
    in.family = 0x15;
    in.life_invert_form = true;
    in.attrs.push_back(attr(231, 200, 3)); /* life = 97 */
    EXPECT_EQ(HealthGrade::Good, grade_smart(in, g));
    SmartGradeInputs in2;
    in2.family = 0x15;
    in2.life_invert_form = true;
    in2.attrs.push_back(attr(231, 200, 95)); /* life = 5 */
    std::vector<HealthGrade> g2;
    EXPECT_EQ(HealthGrade::Caution, grade_smart(in2, g2));
    SmartGradeInputs in3;
    in3.family = 0x15;
    in3.life_invert_form = true;
    in3.attrs.push_back(attr(231, 200, 100)); /* life = 0 */
    std::vector<HealthGrade> g3;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(in3, g3));
}

TEST(Health, WdcAttr230LifeFromRawByte) {
    /* the WDC(0xb)/SanDisk(0xd) attr-230 path (:6084-6110):
     * life = 100 - raw[1]; the G1001/family-0xd raw16/100 variant */
    std::vector<HealthGrade> g;
    SmartGradeInputs wdc;
    wdc.family = 0xb;
    wdc.attrs.push_back(attr(230, 0, 0x0C00)); /* raw[1] = 12 -> life 88 */
    EXPECT_EQ(HealthGrade::Good, grade_smart(wdc, g));
    SmartGradeInputs wdc2;
    wdc2.family = 0xb;
    wdc2.attrs.push_back(attr(230, 0, 0x5F00)); /* raw[1] = 95 -> life 5 */
    std::vector<HealthGrade> g2;
    EXPECT_EQ(HealthGrade::Caution, grade_smart(wdc2, g2));
    SmartGradeInputs wdc3;
    wdc3.family = 0xb;
    wdc3.attrs.push_back(attr(230, 0, 0x6400)); /* raw[1] = 100 -> life 0 */
    std::vector<HealthGrade> g3;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(wdc3, g3));

    /* SanDisk generic form: raw16 / 100 (0x17ec, :24222) */
    SmartGradeInputs sd;
    sd.family = 0xd;
    sd.san_disk_raw_form = true;
    sd.attrs.push_back(attr(230, 0, 0x9C4F)); /* raw16 = 39999 -> /100 = 399 -> clamp */
    std::vector<HealthGrade> g4;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(sd, g4)); /* 100-399 < 0 -> 0 */
    SmartGradeInputs sd2;
    sd2.family = 0xd;
    sd2.san_disk_raw_form = true;
    sd2.attrs.push_back(attr(230, 0, 0x012C)); /* raw16 = 300 -> 3 used -> 97 */
    std::vector<HealthGrade> g5;
    EXPECT_EQ(HealthGrade::Good, grade_smart(sd2, g5));
}

TEST(Health, Attr1RuleIsSandForceGated) {
    /* FUN_004582c0 fires only on family 7 (SandForce); on other disks
     * attr 1 follows the in-range HDD threshold rule (disk) and the
     * default Good (display) */
    std::vector<HealthGrade> g;
    SmartGradeInputs sf;
    sf.family = 7;
    sf.attrs.push_back(attr(1, 0, 0));                   /* all-zero row -> Good (:33532) */
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(sf, g)); /* no graded signal */
    EXPECT_EQ(HealthGrade::Good, g[0]);
    SmartGradeInputs sf2;
    sf2.family = 7;
    sf2.attrs.push_back(attr(1, 60, 456, 100)); /* value 60 < thr 100 */
    std::vector<HealthGrade> g2;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(sf2, g2));
    EXPECT_EQ(HealthGrade::Bad, g2[0]);

    /* non-SandForce: attr 1 value 60 < thr 100 -> disk Bad via the
     * in-range HDD threshold rule; display Good (attr 1 is owned by
     * the SandForce rule, which does not fire) */
    SmartGradeInputs other;
    other.family = 6;
    other.attrs.push_back(attr(1, 60, 456, 100));
    std::vector<HealthGrade> g3;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(other, g3));
    EXPECT_EQ(HealthGrade::Good, g3[0]);
    /* ...but with no threshold the row is not graded-at-all at disk
     * level (no graded signal -> Unknown), and displays Good */
    SmartGradeInputs other2;
    other2.family = 6;
    other2.attrs.push_back(attr(1, 60, 456, 0));
    std::vector<HealthGrade> g4;
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(other2, g4));
    EXPECT_EQ(HealthGrade::Good, g4[0]);
}

TEST(Health, FailingBitForcesDiskBadButAttrGradesFollowOg) {
    /* deliberate superset (PARITY-DELTAS): any FAILED-NOW bit forces
     * the disk verdict to Bad; the per-attr display grades follow the
     * og rules exactly (no flags consumption) */
    std::vector<HealthGrade> g;
    SmartGradeInputs in;
    in.attrs.push_back(attr(197, 100, 3));
    in.attrs.push_back(attr(187, 0, 0, 0));
    in.attrs.back().failing_bit = true;
    EXPECT_EQ(HealthGrade::Bad, grade_smart(in, g));
    EXPECT_EQ(HealthGrade::Caution, g[0]);
    EXPECT_EQ(HealthGrade::Good, g[1]); /* og display: 187 has no rule here */
    /* empty -> Unknown (superset does not fire on Unknown) */
    SmartGradeInputs empty;
    std::vector<HealthGrade> g3;
    EXPECT_EQ(HealthGrade::Unknown, grade_smart(empty, g3));
}

TEST(Health, NvmeGradeMapping) {
    /* the og's family-0x13 path (:5795-5810): any nonzero
     * critical_warning -> Bad; remaining life (100 - percentage_used,
     * :27003) > 10 -> Good, == 10 -> Caution, < 10 -> Bad */
    NvmeHealthInfo h;
    h.available = true;
    h.percentage_used = 5; /* life 95 */
    h.critical_warning = 0;
    h.media_errors = 0;
    EXPECT_EQ(HealthGrade::Good, grade_nvme(h));

    h.percentage_used = 90; /* life 10 -> Caution */
    EXPECT_EQ(HealthGrade::Caution, grade_nvme(h));
    h.percentage_used = 91; /* life 9 -> Bad (NOT Caution) */
    EXPECT_EQ(HealthGrade::Bad, grade_nvme(h));
    h.percentage_used = 100; /* life 0 -> Bad */
    EXPECT_EQ(HealthGrade::Bad, grade_nvme(h));

    /* every nonzero warning byte is Bad - bits 1/4 included (the og
     * never splits the byte) */
    h.percentage_used = 5;
    for (u8 cw : {u8(0x01), u8(0x02), u8(0x04), u8(0x08), u8(0x10)}) {
        h.critical_warning = cw;
        EXPECT_EQ(HealthGrade::Bad, grade_nvme(h)) << "cw " << int(cw);
    }
    h.critical_warning = 0;

    /* media errors are display data - the og's NVMe grade never reads
     * them (no rule at :5795-5810) */
    h.media_errors = 7;
    EXPECT_EQ(HealthGrade::Good, grade_nvme(h));
    h.media_errors = 0;

    /* the Parallels early-Good (:5798) and the unavailable path */
    EXPECT_EQ(HealthGrade::Good, grade_nvme(h, /*parallels_virtual_disk=*/true));
    NvmeHealthInfo unavailable;
    EXPECT_EQ(HealthGrade::Unknown, grade_nvme(unavailable));
}

TEST(Health, ModelSaysSsdOcList) {
    /* the og's is-SSD string list (:6300-6344) - the exact list; note
     * "KINGSTON SV300" matches NOTHING (no SSD/Solid substring, no
     * listed prefix) and stays an HDD-class disk for grading */
    EXPECT_TRUE(model_says_ssd("OCZ-VERTEX4"));
    EXPECT_TRUE(model_says_ssd("SPCC Solid State Disk"));
    EXPECT_TRUE(model_says_ssd("PATRIOT Burst"));
    EXPECT_TRUE(model_says_ssd("KINGSTON SSDNow V300")); /* contains SSD */
    EXPECT_TRUE(model_says_ssd("Samsung SSD 850"));
    EXPECT_TRUE(model_says_ssd("SiliconHardDisk 100"));
    EXPECT_TRUE(model_says_ssd("PHOTOFAST G-Monster"));
    EXPECT_TRUE(model_says_ssd("STT_FTM100GX25H"));
    EXPECT_TRUE(model_says_ssd("Super Talent SSD"));
    EXPECT_FALSE(model_says_ssd("KINGSTON SV300S37A"));
    EXPECT_FALSE(model_says_ssd("WDC WD40EFRX-68N32N0"));
    EXPECT_FALSE(model_says_ssd("TOSHIBA DT01ACA100"));
}

TEST(Health, ClassifyFamilyDecodedVendors) {
    /* FUN_0040dad0 decoded branches - model strings */
    EXPECT_EQ(0xb, classify_family("WDC WD40EFRX-68N32N0", {}).family);
    EXPECT_EQ(0xa, classify_family("Seagate BarraCuda", {}).family);
    EXPECT_EQ(2, classify_family("MTRON MSD-SATA3025", {}).family);
    EXPECT_EQ(5, classify_family("INTEL SSDSC2KB019T8", {}).family);
    EXPECT_EQ(6, classify_family("SAMSUNG MZ7LN512HMJP", {}).family);
    EXPECT_EQ(6, classify_family("MZ-7KM512A", {}).family);
    EXPECT_EQ(8, classify_family("MICRON M500DC", {}).family);
    EXPECT_EQ(0x12, classify_family("CRUCIAL CT500MX200SSD1", {}).family);
    EXPECT_EQ(7, classify_family("SandForce-driven V+100", {}).family);
    EXPECT_EQ(9, classify_family("OCZ-TRION150", {}).family);
    EXPECT_EQ(0xd, classify_family("SanDisk SD5", {}).family);
    EXPECT_EQ(0x15, classify_family("SK hynix SC311 SATA", {}).family);
    EXPECT_EQ(0x16, classify_family("KIOXIA XG8", {}).family);
    EXPECT_EQ(0x19, classify_family("APACER AS2280P4", {}).family);
    EXPECT_EQ(1, classify_family("Generic SSD Model X", {}).family);
    EXPECT_EQ(0, classify_family("Some HDD Model 7K2000", {}).family);
}

TEST(Health, ClassifyFamilyAttributeSignatures) {
    /* the decoded attribute-id signatures */
    std::vector<u8> jmicron60x = {1, 9, 12, 0xC2, 0xE5, 0xE8, 0xE9};
    EXPECT_EQ(4, classify_family("", jmicron60x).family);
    std::vector<u8> sandforce = {1, 5, 9, 12, 13, 100, 0xC0};
    EXPECT_EQ(7, classify_family("", sandforce).family);
    std::vector<u8> ironwolf = {1,    5,    9,    12,   100,  0x66, 0x67, 0xAA,
                                0xAB, 0xAC, 0xB3, 0xB5, 0xB7, 0xB9, 0xBB};
    FamilyClassify fc = classify_family("", ironwolf);
    EXPECT_EQ(0xa, fc.family);
    EXPECT_EQ(4, fc.subfamily);
    std::vector<u8> plextor = {1, 5, 9, 12, 0xB1, 0xB2, 0xB5, 0xB6};
    EXPECT_EQ(0xc, classify_family("", plextor).family);
    std::vector<u8> ocz_trion = {1, 3, 4, 5, 9, 12, 0xE8, 0xE9};
    EXPECT_EQ(9, classify_family("OCZ something", ocz_trion).family);
}

TEST(Health, ClassifyFamilyFormsAndFallbacks) {
    /* SanDisk G1001 -> the 233 gate flag (0x17f4) */
    FamilyClassify g1001 = classify_family("SanDisk G1001 drive", {});
    EXPECT_EQ(0xd, g1001.family);
    EXPECT_TRUE(g1001.san_disk_g1001);
    /* SK hynix HFS-class -> raw life form (0x17e4) */
    FamilyClassify hfs = classify_family("SKhynix HFS001T9A", {});
    EXPECT_EQ(0x15, hfs.family);
    EXPECT_TRUE(hfs.life_raw_form);
    EXPECT_FALSE(hfs.life_invert_form);
    /* Kingston -> raw form + family 0x11 */
    FamilyClassify kc4 = classify_family("KINGSTON SA400S37240G", {});
    EXPECT_EQ(0x11, kc4.family);
    EXPECT_TRUE(kc4.life_raw_form);
    /* Kioxia -> family 0x16, sub 3 */
    FamilyClassify kioxia = classify_family("KIOXIA XG8", {});
    EXPECT_EQ(0x16, kioxia.family);
    EXPECT_EQ(3, kioxia.subfamily);
    /* generic SSD -> 1, HDD -> 0 */
    EXPECT_EQ(1, classify_family("Generic SSD Model X", {}).family);
    EXPECT_EQ(0, classify_family("WDC-like HDD without WDC prefix", {}).family);
}

TEST(Health, MakeGradeInputsAppliesOgDefaults) {
    /* caution counts 1/1/1 and SSD-life caution 10 (:37004-37019) */
    std::vector<SmartAttrInput> attrs;
    attrs.push_back(attr(197, 100, 1));
    SmartGradeInputs in = make_grade_inputs("Samsung SSD 850 EVO", attrs);
    EXPECT_TRUE(in.is_ssd);
    EXPECT_EQ(6, in.family);
    EXPECT_EQ(4, in.subfamily);
    EXPECT_EQ(1, in.caution_realloc);
    EXPECT_EQ(1, in.caution_pending);
    EXPECT_EQ(1, in.caution_offline);
    EXPECT_EQ(10, in.ssd_life_caution);
    /* NVMe disks take the family-0x13 code (SmartNVMe) */
    SmartGradeInputs nv = make_grade_inputs("whatever", attrs, true);
    EXPECT_EQ(0x13, nv.family);
}

TEST(Health, GradeNames) {
    EXPECT_STREQ("Unknown", health_grade_name(HealthGrade::Unknown));
    EXPECT_STREQ("Good", health_grade_name(HealthGrade::Good));
    EXPECT_STREQ("Caution", health_grade_name(HealthGrade::Caution));
    EXPECT_STREQ("Bad", health_grade_name(HealthGrade::Bad));
}
