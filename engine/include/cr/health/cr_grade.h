/*
 * cr_grade.h - disk health grading (S-024): the og DiskHealth 4-status
 * model (Unknown / Good / Caution / Bad).
 *
 * Corpus (DiskHealth.exe, re-workspace/reversed/ghidra-wave2/
 * DiskHealth_decompiled.c - every rule below was re-decoded first-hand
 * for this fix; line cites are that file):
 *   - 4 statuses, list {"Unknown","Good","Caution","Bad"} (:37087-37100);
 *   - per-disk defaults (:37004-37019): caution counts for attrs
 *     5/197/198 = 1 each (0x182a/0x182c/0x182e), SSD-life caution = 10%
 *     (0x1830); temperature caution 50/60 (0x1808) is DISPLAY-ONLY in
 *     DiskHealth.exe - the grader never reads 0x1808 or 0x17bc (the
 *     grade function FUN_0040cdf0 :5755-6130 has no temperature branch;
 *     temp attrs 194/184 are forced Good in the per-attr chain :33596);
 *   - is-SSD flag (0x1724) writer (:6300-6344): model string prefix
 *     OCZ / SPCC / PATRIOT / PHOTOFAST / STT_FTM / "Super Talent", or
 *     contains Solid / SSD / SiliconHardDisk;
 *   - vendor family (0x181c) writer: FUN_0040dad0 cascade :6300-7000
 *     (per-family attr-signature + model-string branches; see
 *     cr_family.cpp for the decoded table);
 *   - disk grade FUN_0040cdf0 :5755-6130 (see cr_grade.cpp for the
 *     rule-by-rule mapping);
 *   - per-attr display chain :34148-34167: 00458350 (5/197/198) ->
 *     004584b0 (WDC 230) -> 004580f0 (SSD-wear family attrs) ->
 *     004582c0 (attr 1, SandForce) -> 00458010 (general threshold),
 *     default Good;
 *   - SSD-life derived fields :9660-9810 (family-gated attr sources,
 *     value/raw/100-raw forms) and NVMe conversion :26985-27005
 *     (temp = K-273, life = 100 - percentage_used, page+0 =
 *     critical_warning);
 *   - NVMe grade: family 0x13 path in FUN_0040cdf0 :5795-5810 -
 *     "Parallels Virtual NVMe Disk" -> Good, critical_warning != 0 ->
 *     Bad, life > 10 -> Good, == 10 -> Caution, < 10 -> Bad;
 *   - vendor thresholds via SMART READ_THRESHOLDS 0xD1 (:14108-14118,
 *     stored per-attr for the Bad rule).
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
#include <string>
#include <string_view>
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
    u8 raw[6] = {0};          /* vendor raw (little-endian) */
    u16 vendor_threshold = 0; /* 0xD1 read; 0 = unavailable */
    bool failing_bit = false; /* SMART flags bit 3 (FAILED NOW) / the
                               * pre-failure-decayed bit the old boolean
                               * modeled - deliberate superset input
                               * (PARITY-DELTAS), not an og rule */
};

/* Disk-level grading inputs. The og stores these on the disk struct;
 * family/subfamily/forms come from classify_family() (cr_family.cpp). */
struct SmartGradeInputs {
    bool is_ssd = false;            /* og 0x1724 - model_says_ssd() */
    u16 family = 0;                 /* og 0x181c - vendor family */
    u16 subfamily = 0;              /* og 0x1818 - vendor attr-shape code */
    bool life_raw_form = false;     /* og 0x17e4: life = raw[0], not value */
    bool life_invert_form = false;  /* og 0x17e8: life = 100 - raw[0] */
    bool san_disk_raw_form = false; /* og 0x17ec: attr-230 life =
                                     * raw16 / 100 instead of raw[1] */
    bool san_disk_g1001 = false;    /* og 0x17f4: attr 233 allowed */
    bool skip_attr_grading = false; /* og 0x16ec (Indilinx): SSD path
                                     * skips the general threshold rule */
    bool smart_valid = true;        /* og 0x16d8: SMART data present */
    bool ata_smart_valid = true;    /* og 0x16dc: ATA-side data present
                                     * (HDD gate with smart_valid) */
    bool read_failed = false;       /* og 0x16f8: read error -> Unknown */
    u16 caution_realloc = 1;        /* og 0x182a (attr 5) */
    u16 caution_pending = 1;        /* og 0x182c (attr 197) */
    u16 caution_offline = 1;        /* og 0x182e (attr 198) */
    u16 ssd_life_caution = 10;      /* og 0x1830: life <= this -> Caution */
    std::vector<SmartAttrInput> attrs;
};

/* Grade every attribute (attr_grades[i] corresponds to attrs[i]; the
 * og's display chain :34148-34167 semantics) and derive the disk's
 * overall grade (FUN_0040cdf0 semantics: Bad count > Caution flag >
 * any graded signal > Unknown). failing_bit is a deliberate superset
 * (PARITY-DELTAS): a FAILED-NOW bit forces the disk to Bad even when
 * the og's rule set would say Good. */
HealthGrade grade_smart(const SmartGradeInputs& in, std::vector<HealthGrade>& attr_grades);

/* NVMe grading - the og's family-0x13 path (FUN_0040cdf0 :5795-5810):
 * "Parallels Virtual NVMe Disk" -> Good; critical_warning != 0 -> Bad;
 * remaining life (100 - percentage_used) > 10 -> Good, == 10 ->
 * Caution, < 10 -> Bad; unavailable -> Unknown. The og's NVMe
 * temperature (K-273) is display-only, never graded. */
HealthGrade grade_nvme(const NvmeHealthInfo& h, bool parallels_virtual_disk = false);

/* ---- vendor classification (pure; the og's FUN_0040dad0 cascade) ---- */

/* The og's is-SSD string list (0x1724 writers :6300-6344): prefix
 * OCZ / SPCC / PATRIOT / PHOTOFAST / STT_FTM / "Super Talent", or
 * contains "Solid" / "SSD" / "SiliconHardDisk". */
bool model_says_ssd(std::string_view model);

/* Classification result feeding SmartGradeInputs. family codes are the
 * og's 0x181c values (see cr_family.cpp for names + evidence). */
struct FamilyClassify {
    u16 family = 0;    /* 0 generic-HDD / 1 generic-SSD fallback */
    u16 subfamily = 0; /* og 0x1818 */
    bool life_raw_form = false;
    bool life_invert_form = false;
    bool san_disk_raw_form = false;
    bool san_disk_g1001 = false;
    bool skip_attr_grading = false;
};

/* Classify a disk by model string + the SMART attribute-id table
 * (order matters; the og's cascade order). attr_ids are the ids of the
 * 0xD0 attribute table in order (up to 30). The undecoded exotic-vendor
 * thunks fall through to the generic families (documented residual in
 * S-024's Resolution - the grading engine itself consumes every decoded
 * family code exactly). */
FamilyClassify classify_family(std::string_view model, const std::vector<u8>& attr_ids);

/* Fill a SmartGradeInputs from the raw read results (model + attrs):
 * runs model_says_ssd + classify_family and applies the og's defaults
 * (caution counts 1/1/1, SSD-life caution 10). */
SmartGradeInputs make_grade_inputs(std::string_view model, const std::vector<SmartAttrInput>& attrs,
                                   bool nvme_disk = false);

} // namespace health
} // namespace cr

#endif // CR_HEALTH_GRADE_H_
