/* cr_grade.cpp - disk health grading rules (S-024), decoded-truth engine.
 *
 * Every rule below maps 1:1 to the og DiskHealth.exe grader
 * (DiskHealth_decompiled.c; see cr_grade.h for the citation block and
 * evidence/decompiled-quotes/s6-health-grading-decode.md for the
 * full decode). Pure core: no I/O, explicit inputs.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_grade.h"

namespace cr {
namespace health {

const char* health_grade_name(HealthGrade g) {
    switch (g) {
    case HealthGrade::Good:
        return "Good";
    case HealthGrade::Caution:
        return "Caution";
    case HealthGrade::Bad:
        return "Bad";
    default:
        return "Unknown";
    }
}

namespace {

u16 raw16(const u8 raw[6]) {
    return u16(raw[0]) | (u16(raw[1]) << 8);
}

bool raw4_all_ff(const u8 raw[6]) {
    /* the og skips the 5/197/198 caution check when raw[0..3] are all
     * 0xFF - vendors fill "unsupported" with FFs (FUN_0040cdf0 :5943) */
    return raw[0] == 0xFF && raw[1] == 0xFF && raw[2] == 0xFF && raw[3] == 0xFF;
}

/* the og's per-disk SSD-life caution (0x1830, default 10) */
u16 ssd_life_caution(const SmartGradeInputs& in) {
    return in.ssd_life_caution;
}

/* ---- the per-attr display chain (:34148-34167 rule order) ---- */

bool rule_5_197_198(const SmartGradeInputs& in, const SmartAttrInput& a, HealthGrade& g) {
    /* FUN_00458350 :33548-33583 - HDD only */
    if (in.is_ssd)
        return false;
    if (a.id != 5 && a.id != 197 && a.id != 198)
        return false;
    if (a.vendor_threshold != 0 && a.value < a.vendor_threshold) {
        g = HealthGrade::Bad;
        return true;
    }
    const u16 caution = a.id == 5     ? in.caution_realloc
                        : a.id == 197 ? in.caution_pending
                                      : in.caution_offline;
    if (caution != 0 && caution <= raw16(a.raw)) {
        g = HealthGrade::Caution;
        return true;
    }
    g = HealthGrade::Good;
    return true;
}

bool rule_wdc_230(const SmartGradeInputs& in, const SmartAttrInput& a, HealthGrade& g) {
    /* FUN_004584b0 :33607-33632 - attr 230 on WDC(0xb)/SanDisk(0xd) */
    if (a.id != 230 || (in.family != 0xb && in.family != 0xd))
        return false;
    unsigned used = in.san_disk_raw_form ? (unsigned(raw16(a.raw)) / 100u) : a.raw[1];
    int life = 100 - int(used);
    if (life < 0)
        life = 0;
    if (life == 0) {
        g = HealthGrade::Bad;
        return true;
    }
    g = (life <= int(ssd_life_caution(in))) ? HealthGrade::Caution : HealthGrade::Good;
    return true;
}

bool ssd_life_family_gate(const SmartGradeInputs& in, u8 id) {
    /* FUN_0040cdf0 :5977-6084 family gates (same sets as FUN_004580f0) */
    switch (id) {
    case 169:
        return in.family == 0x14 || in.family == 0x1a || (in.family == 0x11 && in.subfamily == 3);
    case 173:
        return in.family == 0xf || in.family == 0x16;
    case 177:
        return in.family == 6;
    case 187:
        return in.family == 2;
    case 202:
        return in.family == 8 || in.family == 0x12 || in.family == 0x18;
    case 209:
        return in.family == 3;
    case 231:
        switch (in.family) {
        case 7:
        case 0x10:
        case 0x11:
        case 0x15:
        case 0x14:
        case 0xd:
        case 0x17:
        case 0x19:
        case 4:
        case 0xa:
        case 0x1b:
        case 0x1d:
        case 0x1e:
            return true;
        default:
            return false;
        }
    case 232:
        return in.family == 0xc;
    case 233:
        return in.family == 5 || in.family == 9 || in.family == 0xe || in.family == 0x15 ||
               in.san_disk_g1001;
    default:
        return false;
    }
}

bool rule_ssd_life(const SmartGradeInputs& in, const SmartAttrInput& a, HealthGrade& g) {
    /* FUN_004580f0 :33469-33521 (LAB_0045820a) */
    if (!ssd_life_family_gate(in, a.id))
        return false;
    if (in.life_raw_form) {
        /* raw-form vendors have no threshold logic - always Good */
        g = HealthGrade::Good;
        return true;
    }
    if (a.value != 0 && a.vendor_threshold <= a.value) {
        g = (int(ssd_life_caution(in)) < int(a.value)) ? HealthGrade::Good : HealthGrade::Caution;
        return true;
    }
    g = HealthGrade::Bad;
    return true;
}

bool rule_attr1_sandforce(const SmartGradeInputs& in, const SmartAttrInput& a, HealthGrade& g) {
    /* FUN_004582c0 :33523-33545 - attr 1 on SandForce(7) only */
    if (a.id != 1 || in.family != 7)
        return false;
    if ((a.value == 0 && a.raw[0] == 0 && a.raw[1] == 0) || a.vendor_threshold == 0 ||
        a.vendor_threshold <= a.value) {
        g = HealthGrade::Good;
    } else {
        g = HealthGrade::Bad;
    }
    return true;
}

bool rule_general_threshold(const SmartGradeInputs& in, const SmartAttrInput& a, HealthGrade& g) {
    /* FUN_00458010 :33419-33446 - the ranges the specific rules own are
     * excluded for HDDs; SSDs grade every attr by threshold unless the
     * vendor skip flag (0x16ec, Indilinx) is set */
    if (!in.is_ssd || in.skip_attr_grading) {
        const unsigned id = a.id;
        const bool special = (id >= 1 && id <= 13) || (id >= 187 && id <= 189) ||
                             (id >= 191 && id <= 193) || (id >= 195 && id <= 209) ||
                             (id >= 211 && id <= 212) || (id >= 220 && id <= 228) || id == 230 ||
                             id == 231 || id == 240 || id == 250 || id == 254;
        if (special)
            return false;
    }
    if (a.vendor_threshold == 0 || a.vendor_threshold <= a.value)
        g = HealthGrade::Good;
    else
        g = HealthGrade::Bad;
    return true;
}

HealthGrade grade_one_attr(const SmartGradeInputs& in, const SmartAttrInput& a) {
    /* the og gates the whole display chain on data presence
     * (:34129/:34137: 0x16d8==0 || 0x16dc==0 || 0x16f8!=0 -> Unknown) */
    if (!in.smart_valid || !in.ata_smart_valid || in.read_failed)
        return HealthGrade::Unknown;
    HealthGrade g = HealthGrade::Good;
    if (rule_5_197_198(in, a, g))
        return g;
    if (a.id == 194 || a.id == 184) {
        /* temperature (194) / soft-ECC (184): always Good in the grader
         * (:33596 - the 0x1808 caution default is display-only) */
        return HealthGrade::Good;
    }
    if (rule_wdc_230(in, a, g))
        return g;
    if (rule_ssd_life(in, a, g))
        return g;
    if (rule_attr1_sandforce(in, a, g))
        return g;
    if (rule_general_threshold(in, a, g))
        return g;
    return HealthGrade::Good;
}

/* ---- the disk grade (FUN_0040cdf0 :5755-6130) ---- */

/* attrs the HDD general threshold rule grades (the og's range test
 * :5916-5935 - inverted: the special attrs below DO get the rule) */
bool hdd_threshold_range(u8 id) {
    const unsigned v = id;
    return (v >= 1 && v <= 13) || (v >= 187 && v <= 189) || (v >= 191 && v <= 193) ||
           (v >= 195 && v <= 209) || (v >= 211 && v <= 212) || (v >= 220 && v <= 228) || v == 230 ||
           v == 231 || v == 240 || v == 250 || v == 254;
}

HealthGrade grade_disk(const SmartGradeInputs& in) {
    /* Unknown gates (:5843-5852): no SMART data; HDDs additionally need
     * the ATA-side data; a read error forces Unknown */
    if (!in.smart_valid)
        return HealthGrade::Unknown;
    if (!in.is_ssd && !in.ata_smart_valid)
        return HealthGrade::Unknown;
    if (in.read_failed)
        return HealthGrade::Unknown;

    bool graded = false; /* the og's bVar4 (true = no graded signal) */
    unsigned bad_count = 0;
    bool caution = false;

    for (size_t i = 0; i < in.attrs.size(); ++i) {
        const SmartAttrInput& a = in.attrs[i];

        /* duplicate attribute id -> the whole disk is Unknown (:5826) */
        bool dup = false;
        for (size_t j = 0; j < i; ++j) {
            if (in.attrs[j].id == a.id) {
                dup = true;
                break;
            }
        }
        if (dup)
            return HealthGrade::Unknown;

        /* SandForce attr-1-all-zero rows are skipped entirely (:5888) */
        const bool sandforce_zero =
            in.family == 7 && a.id == 1 && a.value == 0 && a.raw[0] == 0 && a.raw[1] == 0;
        const bool is_temp = a.id == 194;

        if (!sandforce_zero && !is_temp) {
            if (in.is_ssd) {
                if (in.skip_attr_grading) {
                    /* 0x16ec (Indilinx): no threshold rule for this disk */
                } else if (a.vendor_threshold != 0 && a.value < a.vendor_threshold) {
                    ++bad_count; /* :5902-5906 - any attr below threshold */
                }
            } else if (hdd_threshold_range(a.id)) {
                if (a.vendor_threshold != 0 && a.value < a.vendor_threshold)
                    ++bad_count; /* :5936-5942 */
            }
        }

        /* graded signal: SSD with any vendored attr (:5947) */
        if (in.is_ssd && a.vendor_threshold != 0)
            graded = true;

        /* 5/197/198 caution (:5949-5969, raw16 + the all-FF guard) */
        if (a.id == 5 || a.id == 197 || a.id == 198) {
            if (!raw4_all_ff(a.raw)) {
                const u16 count = a.id == 5     ? in.caution_realloc
                                  : a.id == 197 ? in.caution_pending
                                                : in.caution_offline;
                if (count != 0 && count <= raw16(a.raw) && !in.is_ssd)
                    caution = true;
            }
            if (!in.is_ssd)
                graded = true; /* :5971 - HDD: these attrs prove gradeability */
            continue;
        }

        /* SSD-life family paths (:5977-6098) */
        if (ssd_life_family_gate(in, a.id)) {
            graded = true; /* LAB_0040d4d5 :6006 */
            int life;
            if (in.life_invert_form)
                life = 100 - int(a.raw[0]);
            else if (in.life_raw_form)
                life = int(a.raw[0]);
            else
                life = int(a.value);
            if (life < 0)
                life = 0;
            if (life >= 101)
                life = 100; /* :6014 cap */
            if (life == 0) {
                /* LAB_0040d694 :6100 - exhausted */
                graded = true;
                bad_count = 1;
                continue;
            }
            /* LAB_0040d54e :6017: raw-form vendors skip the threshold
             * test; value-form needs value >= threshold */
            if (in.life_raw_form || a.vendor_threshold <= u16(life)) {
                if (int(ssd_life_caution(in)) < life)
                    continue; /* healthy - next attr */
                caution = true;
                continue;
            }
            graded = true; /* value below threshold - LAB_0040d694 */
            bad_count = 1;
            continue;
        }

        /* WDC/SanDisk attr-230 path (:6084-6110) */
        if (a.id == 230 && (in.family == 0xb || in.family == 0xd)) {
            graded = true;
            const unsigned used =
                in.san_disk_raw_form ? (unsigned(raw16(a.raw)) / 100u) : unsigned(a.raw[1]);
            int life = 100 - int(used);
            if (life < 0)
                life = 0;
            if (life >= 101)
                life = 100;
            if (life == 0) {
                graded = true;
                bad_count = 1;
                continue;
            }
            if (int(ssd_life_caution(in)) < life)
                continue;
            caution = true;
            continue;
        }
    }

    if (bad_count > 0)
        return HealthGrade::Bad;
    if (graded)
        return caution ? HealthGrade::Caution : HealthGrade::Good;
    return HealthGrade::Unknown;
}

} // namespace

HealthGrade grade_smart(const SmartGradeInputs& in, std::vector<HealthGrade>& attr_grades) {
    attr_grades.clear();
    attr_grades.reserve(in.attrs.size());
    for (const SmartAttrInput& a : in.attrs)
        attr_grades.push_back(grade_one_attr(in, a));
    HealthGrade disk = grade_disk(in);
    /* deliberate superset (PARITY-DELTAS): a FAILED-NOW bit forces the
     * disk to Bad even when the og's rule set would say Good - kept
     * from the S-024 first landing, registered as a safety hardening */
    bool any_failing = false;
    for (const SmartAttrInput& a : in.attrs)
        any_failing = any_failing || a.failing_bit;
    if (any_failing && disk != HealthGrade::Unknown)
        disk = HealthGrade::Bad;
    return disk;
}

HealthGrade grade_nvme(const NvmeHealthInfo& h, bool parallels_virtual_disk) {
    if (!h.available)
        return HealthGrade::Unknown;
    /* the og's family-0x13 path (FUN_0040cdf0 :5795-5810) */
    if (parallels_virtual_disk)
        return HealthGrade::Good;
    if (h.critical_warning != 0)
        return HealthGrade::Bad;
    int life = 100 - int(h.percentage_used); /* :27003 */
    if (life < 0)
        life = 0; /* :27008 clamp */
    if (life > 10)
        return HealthGrade::Good;
    if (life == 10)
        return HealthGrade::Caution;
    return HealthGrade::Bad;
}

SmartGradeInputs make_grade_inputs(std::string_view model, const std::vector<SmartAttrInput>& attrs,
                                   bool nvme_disk) {
    SmartGradeInputs in;
    in.is_ssd = model_says_ssd(model);
    in.attrs = attrs;
    if (nvme_disk) {
        in.family = 0x13; /* SmartNVMe */
        return in;
    }
    const FamilyClassify fc = classify_family(model, [&] {
        std::vector<u8> ids;
        ids.reserve(attrs.size());
        for (const SmartAttrInput& a : attrs)
            ids.push_back(a.id);
        return ids;
    }());
    in.family = fc.family;
    in.subfamily = fc.subfamily;
    in.life_raw_form = fc.life_raw_form;
    in.life_invert_form = fc.life_invert_form;
    in.san_disk_raw_form = fc.san_disk_raw_form;
    in.san_disk_g1001 = fc.san_disk_g1001;
    in.skip_attr_grading = fc.skip_attr_grading;
    /* og defaults (:37004-37019): caution counts 1/1/1, SSD-life 10 */
    in.caution_realloc = 1;
    in.caution_pending = 1;
    in.caution_offline = 1;
    in.ssd_life_caution = 10;
    return in;
}

} // namespace health
} // namespace cr
