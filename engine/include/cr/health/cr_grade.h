/*
 * cr_grade.h - disk health grading (S-024): the og DiskHealth 4-status
 * model (Unknown / Good / Caution / Bad).
 *
 * Corpus (DiskHealth.exe, decoded in
 * diff-to-fix/evidence/decompiled-quotes/s3-filefilter-health.md Part B,
 * two-leg converged):
 *   - 4 statuses (:37087-37100);
 *   - per-disk threshold defaults (:37004-37019): temperature caution
 *     50/60 by disk class (the 0x1724 flag, INFERRED is-SSD), caution
 *     counts for attrs 5/197/198 = 1, SSD-life caution = 10%;
 *   - HDD bad-sector attrs 5/197/198 (FUN_00458350 :33540-33582):
 *     value < vendor threshold -> Bad; raw >= caution count -> Caution;
 *   - attr 1 read-error-rate (:33527-33551): value==0 && raw==0 or
 *     threshold <= value -> Good, else Bad;
 *   - SSD remaining-life attrs {169,173,177,187,202,209,230,231,232,233}
 *     gated by drive-interface family (aggregation :5930-6130; the exact
 *     family enum is NOT decoded - our analog gate is is-SSD):
 *     100 -> Good; >10 -> Good; <=10 -> Caution; 0 -> Bad;
 *   - vendor thresholds via SMART READ_THRESHOLDS 0xD1 (:14108-14118).
 *
 * The rule set is PURE CORE so Linux CI executes it (the win layer is
 * compile-verified only, G-17): explicit inputs, no I/O.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_HEALTH_GRADE_H_
#define CR_HEALTH_GRADE_H_

#include "../cr_types.h"
#include "cr_nvme.h"
#include <vector>

namespace cr {
namespace health {

enum class HealthGrade : u8 {
    Unknown = 0,
    Good = 1,
    Caution = 2,
    Bad = 3,
};

const char* health_grade_name(HealthGrade g);

/* One SMART attribute's grading inputs (everything the rules read). */
struct SmartAttrInput {
    u8 id = 0;
    u16 value = 0; /* current normalized value */
    u16 worst = 0;
    u8 raw[6] = {0};          /* vendor raw (little-endian triple) */
    u16 vendor_threshold = 0; /* 0xD1 read; 0 = unavailable */
    bool failing_bit = false; /* SMART flags bit 3 (FAILED NOW) / the
                               * pre-failure-decayed bit the old boolean
                               * modeled - a hard Bad input */
};

struct SmartGradeInputs {
    bool is_ssd = false; /* the interface-family gate's analog:
                          * seek-penalty heuristic on Windows */
    std::vector<SmartAttrInput> attrs;
};

/* Grade every attribute (attr_grades[i] corresponds to attrs[i]) and
 * derive the disk's overall grade = the worst non-Unknown grade.
 * Temperature (190/194) caution: >= 50C HDD / >= 60C SSD (the decoded
 * per-disk defaults; raw[0] is the Celsius byte). */
HealthGrade grade_smart(const SmartGradeInputs& in, std::vector<HealthGrade>& attr_grades);

/* NVMe grading: the SSD-life rule over percentage_used (remaining <= 10
 * -> Caution, exhausted -> Bad), critical_warning bits (0/2/3 -> Bad,
 * 1/4 -> Caution per the NVMe spec meaning of the bits - the og's exact
 * NVMe bit mapping is not in the decode; documented in S-024's
 * Resolution), media errors > 0 -> Caution (the pending-sectors analog). */
HealthGrade grade_nvme(const NvmeHealthInfo& h);

} // namespace health
} // namespace cr

#endif // CR_HEALTH_GRADE_H_
