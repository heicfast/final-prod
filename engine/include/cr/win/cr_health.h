/*
 * cr_health.h - disk health: SMART via ATA passthrough, NVMe health, TRIM.
 *
 * Corpus: DiskHealth.exe (DiskSmart; CAtaSmart engine with multi-transport
 * accessors GetSmartAttributeSat/Csmi/Si/NVMeRealtek). DELTA, documented
 * (AUDIT-PIPELINE HEALTH-2): our implementation runs the same ATA SMART
 * command set (0xB0 feature 0xD0 READ_DATA / 0xD1 READ_THRESHOLDS —
 * issued in the pure seam src/core/health/cr_smart_ata.cpp since N-032 —
 * / 0xEC IDENTIFY) over a three-transport chain:
 *   1. IOCTL_ATA_PASS_THROUGH (ATA_PASS_THROUGH_EX) - the direct route;
 *   2. SMART_RCV_DRIVE_DATA 0x7C088 - the legacy fallback the corpus also
 *      carries (task file matches the og's IDEREGS decode: count 1,
 *      sector 1, cyl 4F/C2 for SMART commands);
 *   3. CSMI CC-0x19 SCSI-miniport pass-through (N-032) - the og's
 *      GetSmartAttributeCsmi route for Intel RST / AMD rcraid arrays,
 *      wire-decoded from DiskHealth_decompiled.c FUN_00425450 (see
 *      include/cr/health/cr_csmi.h). The og's RAID-census routing and
 *      its LSILOGIC sibling transport stay companion-side; its SMART
 *      auto-enable (the ENABLE-OPERATIONS feature write) is a device
 *      write this report-only module does not perform.
 * DRWUI's SSD detection used WMI MSFT_PhysicalDisk MediaType==4; ours
 * uses the seek-penalty IOCTL heuristic (HEALTH-5: one story, documented
 * here). TRIM state: fsutil behavior query disabledeletenotify, first
 * "= " match (HEALTH-6).
 *
 * Report-only: nothing here writes to the device. TRIM toggle is exposed
 * as a policy recommendation, never executed silently.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_HEALTH_H_
#define CR_HEALTH_H_

#include "../cr_types.h"
#include "../health/cr_nvme.h"
#include "../health/cr_grade.h"
#include <string>
#include <vector>

namespace cr {
namespace win {
namespace health {

/* One SMART attribute row (ATA spec 512-byte table, 12-byte entries). */
struct SmartAttribute {
    u8 id = 0;
    u16 value = 0; /* current normalized */
    u16 worst = 0;
    u8 raw[6] = {0};          /* vendor raw */
    u16 vendor_threshold = 0; /* 0xD1 READ_THRESHOLDS (0 = unavailable) */
    bool failing = false;
    std::string name;
    cr::health::HealthGrade grade = cr::health::HealthGrade::Unknown; /* S-024 */
};

struct SmartReport {
    bool available = false;
    std::string transport; /* "ata-passthrough" / "nvme" / "legacy" */
    u16 ata_version = 0;
    u16 smart_version = 0;
    std::vector<SmartAttribute> attributes;
    /* derived verdicts */
    bool failing_now = false;
    cr::health::HealthGrade grade = cr::health::HealthGrade::Unknown; /* S-024:
        the og DiskHealth 4-status verdict, worst of the attr grades */
    bool is_ssd = false;         /* the SSD-wear gate's input (seek-penalty) */
    u64 reallocated_sectors = 0; /* attr 5 raw */
    u64 pending_sectors = 0;     /* attr 197 raw */
    u64 uncorrectable = 0;       /* attr 198 raw */
    u64 power_on_hours = 0;      /* attr 9 raw */
    u64 spin_retry = 0;          /* attr 10 raw */
    u64 ssd_wear_level = 0;      /* attr 231/233 (vendor) */
    u64 temperature_c = 0;       /* attr 194/190 raw (vendor) */
};

/* NVMe health log (log page 2, 512 bytes) — parsed by the pure-core
 * cr::health::parse_nvme_health_page (spec offsets; PARITY-AUDIT F-A4-17
 * fixed the previous inline 64/80/96/112 mapping). */
struct NvmeHealth : cr::health::NvmeHealthInfo {};

/* Read the SMART attribute table through ATA passthrough (admin handle
 * required). Returns ERR_HEALTH_UNAVAILABLE when the transport rejects. */
u32 read_smart(void* disk_handle, SmartReport& out);

/* NVMe: identify + health log page through IOCTL_STORAGE_QUERY_PROPERTY /
 * NVMe passthrough. */
u32 read_nvme_health(void* disk_handle, NvmeHealth& out);

/* TRIM state: native per-device IOCTL_STORAGE_QUERY_PROPERTY
 * (StorageDeviceTrimProperty -> DEVICE_TRIM_DESCRIPTOR). Never spawns a
 * child process - the fsutil _popen path (a visible console window per
 * health read) is removed (NG-8). Failure = unknown, never a fallback. */
u32 query_trim_state(void* disk_handle, bool& enabled);

/* SSD detection: physical drive media type via IOCTL_STORAGE_QUERY_PROPERTY
 * (bus type NVMe => SSD; HDD registry + rotation rate 0 => SSD). */
bool looks_like_ssd(void* disk_handle);

/* Human-readable attribute names for the well-known IDs. */
const char* smart_attr_name(u8 id);

} // namespace health
} // namespace win
} // namespace cr

#endif // CR_HEALTH_H_
