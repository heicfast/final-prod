/* cr_grade.cpp - disk health grading rules (S-024). See cr_grade.h for
 * the corpus citation of every rule. Pure core: no I/O, explicit inputs.
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

u64 raw_triple(const u8 raw[6]) {
    return u64(raw[0]) | (u64(raw[1]) << 8) | (u64(raw[2]) << 16);
}

/* the worst non-Unknown grade in a list (Unknown when empty) */
HealthGrade worst(const std::vector<HealthGrade>& grades) {
    HealthGrade out = HealthGrade::Unknown;
    for (HealthGrade g : grades)
        if (g != HealthGrade::Unknown && u8(out) < u8(g))
            out = g;
    return out;
}

bool is_hdd_bad_sector_attr(u8 id) {
    return id == 5 || id == 197 || id == 198; /* reallocated / pending / offline */
}

bool is_ssd_life_attr(u8 id) {
    /* the decoded vendor set {169,173,177,187,202,209,230,231,232,233} */
    switch (id) {
    case 169:
    case 173:
    case 177:
    case 187:
    case 202:
    case 209:
    case 230:
    case 231:
    case 232:
    case 233:
        return true;
    default:
        return false;
    }
}

} // namespace

HealthGrade grade_smart(const SmartGradeInputs& in, std::vector<HealthGrade>& attr_grades) {
    attr_grades.clear();
    attr_grades.reserve(in.attrs.size());
    for (const SmartAttrInput& a : in.attrs) {
        HealthGrade g = HealthGrade::Unknown;
        const u64 raw = raw_triple(a.raw);
        if (is_hdd_bad_sector_attr(a.id)) {
            /* FUN_00458350 :33540-33582: value < vendor threshold -> Bad;
             * raw >= caution count (default 1) -> Caution; else Good. */
            if (a.vendor_threshold != 0 && a.value < a.vendor_threshold)
                g = HealthGrade::Bad;
            else if (raw >= 1)
                g = HealthGrade::Caution;
            else
                g = HealthGrade::Good;
        } else if (a.id == 1) {
            /* attr 1 read-error-rate (:33527-33551): value==0 && raw==0,
             * or threshold <= value -> Good; else Bad. With no threshold
             * available (0), any value passes (the conservative default). */
            if ((a.value == 0 && raw == 0) || a.vendor_threshold <= a.value)
                g = HealthGrade::Good;
            else
                g = HealthGrade::Bad;
        } else if (in.is_ssd && is_ssd_life_attr(a.id)) {
            /* SSD remaining-life (:6010-6016, :6076-6090): 100 -> Good;
             * >10 -> Good; <=10 -> Caution; 0 -> Bad. */
            if (a.value == 0)
                g = HealthGrade::Bad;
            else if (a.value <= 10)
                g = HealthGrade::Caution;
            else
                g = HealthGrade::Good;
        } else if (a.id == 190 || a.id == 194) {
            /* temperature: caution at >= 50C (HDD) / >= 60C (SSD) - the
             * decoded per-disk defaults (:37004-37019). raw[0] is the
             * Celsius byte (vendor; 0 = not reported). */
            const unsigned c = a.raw[0];
            if (c != 0 && c >= (in.is_ssd ? 60u : 50u))
                g = HealthGrade::Caution;
            else
                g = HealthGrade::Good;
        } else if (a.failing_bit) {
            g = HealthGrade::Bad; /* the FAILED-NOW flags bit the old
                                   * boolean modeled - a hard Bad */
        } else {
            g = HealthGrade::Good;
        }
        attr_grades.push_back(g);
    }
    return worst(attr_grades);
}

HealthGrade grade_nvme(const NvmeHealthInfo& h) {
    if (!h.available)
        return HealthGrade::Unknown;
    HealthGrade g = HealthGrade::Good;
    /* SSD-life analog over percentage_used: remaining <= 10% -> Caution,
     * exhausted (>= 100%) -> Bad (the decoded 10%/0% rule). */
    if (h.percentage_used >= 100)
        g = HealthGrade::Bad;
    else if (100 - h.percentage_used <= 10)
        g = HealthGrade::Caution;
    /* critical_warning bits (NVMe base spec): 0 spare-below-threshold,
     * 2 reliability degraded, 3 read-only media -> Bad; 1 temperature,
     * 4 volatile backup failed -> Caution. (The og's exact bit mapping is
     * not in the decode - the spec meaning is our documented analog.) */
    if (h.critical_warning & 0x0D) /* bits 0, 2, 3 */
        return HealthGrade::Bad;
    if (h.critical_warning & 0x12) /* bits 1, 4 */
        g = HealthGrade::Caution;
    /* media errors: the pending-sectors caution analog (raw >= 1). */
    if (h.media_errors >= 1 && g == HealthGrade::Good)
        g = HealthGrade::Caution;
    return g;
}

} // namespace health
} // namespace cr
