/* cr_health.cpp - disk health: SMART via ATA passthrough, NVMe, TRIM.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_health.h"
#include "cr/cr_error.h"
#include <windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace cr {
namespace win {
namespace health {

namespace {

/* The system ATA_PASS_THROUGH_EX (ntddscsi.h) is used verbatim: its
 * DataBufferOffset is ULONG_PTR (8 bytes on x64) - a hand-inlined layout
 * with a 32-bit offset field would silently misplace the data buffer on
 * x64, so the header shape is not duplicated here. */

/* Build+issue one ATA command with a 512-byte data-in phase. */
bool ata_passthrough(HANDLE h, u8 feature, u8 count, u8 command, u8* out512) {
    struct {
        ATA_PASS_THROUGH_EX apt;
        u8 data[512];
    } req;
    std::memset(&req, 0, sizeof(req));
    req.apt.Length = sizeof(ATA_PASS_THROUGH_EX);
    req.apt.AtaFlags = ATA_FLAGS_DATA_IN;
    req.apt.DataTransferLength = 512;
    req.apt.TimeOutValue = 10;
    req.apt.DataBufferOffset = offsetof(decltype(req), data);
    /* current task file: feature, sector count, LBA (unused), drive, command */
    req.apt.CurrentTaskFile[0] = feature;
    req.apt.CurrentTaskFile[1] = count;
    req.apt.CurrentTaskFile[6] = 0xA0; /* LBA mode + master */
    req.apt.CurrentTaskFile[7] = command;

    DWORD br = 0;
    if (!DeviceIoControl(h, IOCTL_ATA_PASS_THROUGH, &req, sizeof(req), &req, sizeof(req), &br,
                         nullptr))
        return false;
    std::memcpy(out512, req.data, 512);
    return true;
}

/* Legacy fallback (audit row 43): SMART_RCV_DRIVE_DATA - the pre-Vista
 * driver interface, still required by old storage filter drivers that
 * reject IOCTL_ATA_PASS_THROUGH. Same 512-byte data-in semantics. */
#ifndef SMART_RCV_DRIVE_DATA
#define SMART_RCV_DRIVE_DATA 0x0007c088
#endif

bool smart_rcv_legacy(HANDLE h, u8 feature, u8 count, u8 command, u8* out512) {
    /* SENDCMDINPARAMS: cBufferSize + IDEREGS(8) + bDriveNumber + pad */
    struct {
        u32 cBufferSize;
        u8 regs[8]; /* features, count, sector, cyl_lo, cyl_hi, drive, cmd, rsvd */
        u8 drive_number;
        u8 bReserved[3];
        u32 dwReserved[4];
    } in;
    std::memset(&in, 0, sizeof(in));
    in.regs[0] = feature;
    in.regs[1] = count;
    in.regs[5] = 0xA0; /* LBA mode + master */
    in.regs[6] = command;
    in.cBufferSize = 512;
    /* SENDCMDOUTPARAMS: cBufferSize + DRIVERSTATUS(12) + data[512] */
    struct {
        u32 cBufferSize;
        u8 driver_status[12];
        u8 data[512];
    } out;
    DWORD br = 0;
    if (!DeviceIoControl(h, SMART_RCV_DRIVE_DATA, &in, sizeof(in), &out, sizeof(out), &br, nullptr))
        return false;
    std::memcpy(out512, out.data, 512);
    return true;
}

u16 word_at(const u8* p, u32 idx) {
    return u16(p[idx * 2]) | (u16(p[idx * 2 + 1]) << 8);
}

u64 triple(const u8* raw) {
    return u64(raw[0]) | (u64(raw[1]) << 8) | (u64(raw[2]) << 16);
}

} // namespace

const char* smart_attr_name(u8 id) {
    switch (id) {
    case 1:
        return "Read error rate";
    case 3:
        return "Spin-up time";
    case 4:
        return "Start/stop count";
    case 5:
        return "Reallocated sectors";
    case 7:
        return "Seek error rate";
    case 9:
        return "Power-on hours";
    case 10:
        return "Spin retry count";
    case 11:
        return "Calibration retries";
    case 12:
        return "Power cycles";
    case 13:
        return "Soft read errors";
    case 170:
        return "Reserved blocks (SSD)";
    case 171:
        return "Program failures (SSD)";
    case 172:
        return "Erase failures (SSD)";
    case 175:
        return "Program failures";
    case 183:
        return "SATA downshifts";
    case 187:
        return "Uncorrectable errors";
    case 190:
        return "Airflow temperature";
    case 194:
        return "Temperature";
    case 196:
        return "Reallocated events";
    case 197:
        return "Current pending sectors";
    case 198:
        return "Offline uncorrectable";
    case 199:
        return "UDMA CRC errors";
    case 231:
        return "SSD life left";
    case 232:
        return "Endurance remaining";
    case 233:
        return "Media wearout (SSD)";
    case 234:
        return "Thermal throttling";
    case 241:
        return "Total LBAs written";
    case 242:
        return "Total LBAs read";
    case 249:
        return "NAND writes (SSD)";
    default:
        return "";
    }
}

u32 read_smart(void* disk_handle, SmartReport& out) {
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return ERR_BAD_PARAMETER;

    /* 1. IDENTIFY DEVICE (0xEC): confirm ATA + read ATA version. The
     * modern ATA_PASS_THROUGH path first; old filter drivers take the
     * legacy SMART_RCV_DRIVE_DATA fallback (audit row 43). */
    u8 identify[512];
    bool via_legacy = false;
    if (ata_passthrough(h, 0, 0, 0xEC, identify)) {
        out.transport = "ata-passthrough";
    } else if (smart_rcv_legacy(h, 0, 0, 0xEC, identify)) {
        via_legacy = true;
        out.transport = "smart-rcv-drive-data";
    } else {
        return ERR_HEALTH_UNAVAILABLE;
    }

    out.ata_version = word_at(identify, 80);
    out.available = true;

    /* 2. SMART READ DATA (command 0xB0, feature 0xD0). */
    u8 table[512];
    const bool got_table = via_legacy ? smart_rcv_legacy(h, 0xD0, 0, 0xB0, table)
                                      : ata_passthrough(h, 0xD0, 0, 0xB0, table);
    if (!got_table)
        return ERR_HEALTH_UNAVAILABLE;
    out.smart_version = u16(table[0]) | (u16(table[1]) << 8);

    /* 2b. SMART READ_THRESHOLDS (feature 0xD1, S-024): the vendor
     * thresholds the Bad rule compares against. Same 30x12 entry table
     * at +2; the threshold value sits at entry+1 (the public ATA
     * thresholds layout - id, threshold, reserved[10]). Failure is
     * non-fatal: grading falls back to raw-count-only rules (threshold
     * 0 = unavailable). */
    u8 thr[512];
    const bool got_thr = via_legacy ? smart_rcv_legacy(h, 0xD1, 0, 0xB0, thr)
                                    : ata_passthrough(h, 0xD1, 0, 0xB0, thr);

    /* The 362-byte attribute table holds 30 twelve-byte entries at +2.
     * Entry layout (ATA-ATAPI-7): id@+0, flags(u16)@+1, value@+3,
     * worst@+4, raw[6]@+5, reserved@+11. */
    for (u32 e = 0; e < 30; ++e) {
        const u8* p = table + 2 + e * 12;
        if (p[0] == 0)
            continue; /* unused slot */
        const u16 flags = u16(p[1]) | (u16(p[2]) << 8);
        SmartAttribute a;
        a.id = p[0];
        a.value = p[3];
        a.worst = p[4];
        std::memcpy(a.raw, p + 5, 6);
        if (got_thr) {
            const u8* tp = thr + 2 + e * 12;
            if (tp[0] == a.id)
                a.vendor_threshold = tp[1];
        }
        /* flags bit0 = pre-failure (advisory) attribute; bit3 = FAILED NOW.
         * A pre-failure attribute that has decayed to zero is failing. */
        a.failing = (flags & 0x0008) != 0 || ((flags & 0x0001) != 0 && a.value == 0);
        a.name = smart_attr_name(a.id);
        out.attributes.push_back(a);
        if (a.failing)
            out.failing_now = true;
        switch (a.id) {
        case 5:
            out.reallocated_sectors = triple(a.raw);
            break;
        case 9:
            out.power_on_hours = triple(a.raw);
            break;
        case 10:
            out.spin_retry = triple(a.raw);
            break;
        case 190:
        case 194:
            if (a.raw[0] != 0)
                out.temperature_c = a.raw[0];
            break;
        case 197:
            out.pending_sectors = triple(a.raw);
            break;
        case 198:
            out.uncorrectable = triple(a.raw);
            break;
        case 231:
        case 233:
            out.ssd_wear_level = triple(a.raw);
            break;
        default:
            break;
        }
    }

    /* S-024: the 4-status grading pass (pure-core rules; the og DiskHealth
     * model). The drive model string comes from IDENTIFY words 27-46
     * (byte-swapped pairs); the og's is-SSD flag (0x1724) and vendor
     * family (0x181c) are both derived from it (DiskHealth_decompiled.c
     * :6300-6344, FUN_0040dad0) - see health/cr_family.cpp. */
    std::string model;
    for (u32 w = 27; w <= 46; ++w) {
        const char c1 = char(identify[w * 2 + 1]);
        const char c2 = char(identify[w * 2]);
        if (c1 != ' ')
            model.push_back(c1);
        if (c2 != ' ')
            model.push_back(c2);
    }
    out.is_ssd = health::model_says_ssd(model) || looks_like_ssd(h);
    {
        std::vector<health::SmartAttrInput> ais;
        ais.reserve(out.attributes.size());
        for (const SmartAttribute& a : out.attributes) {
            health::SmartAttrInput ai;
            ai.id = a.id;
            ai.value = a.value;
            ai.worst = a.worst;
            std::memcpy(ai.raw, a.raw, 6);
            ai.vendor_threshold = a.vendor_threshold;
            ai.failing_bit = a.failing;
            ais.push_back(ai);
        }
        const health::SmartGradeInputs gi = health::make_grade_inputs(model, ais);
        std::vector<health::HealthGrade> grades;
        out.grade = health::grade_smart(gi, grades);
        for (size_t k = 0; k < grades.size() && k < out.attributes.size(); ++k)
            out.attributes[k].grade = grades[k];
    }
    return OK;
}

u32 read_nvme_health(void* disk_handle, NvmeHealth& out) {
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return ERR_BAD_PARAMETER;

    /* NVMe health log (log page 2) through the protocol-specific storage
     * query (Windows 10 storage stack; the corpus DiskHealth uses the
     * same route through its NVMe transport). */
#pragma pack(push, 1)
    struct {
        STORAGE_PROPERTY_QUERY header; /* PropertyId + QueryType only used */
        STORAGE_PROTOCOL_SPECIFIC_DATA proto;
        u8 data[512];
    } req;
#pragma pack(pop)
    std::memset(&req, 0, sizeof(req));
    req.header.PropertyId = StorageDeviceProtocolSpecificProperty;
    req.header.QueryType = PropertyStandardQuery;
    req.proto.ProtocolType = ProtocolTypeNvme;
    req.proto.DataType = NVMeDataTypeLogPage;
    req.proto.ProtocolDataRequestValue = 2; /* health log page */
    req.proto.ProtocolDataRequestSubValue = 0;
    req.proto.ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    req.proto.ProtocolDataLength = 512;

    DWORD br = 0;
    if (!DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &req, sizeof(req), &req, sizeof(req), &br,
                         nullptr))
        return ERR_HEALTH_UNAVAILABLE;

    const u8* d = req.data;
    /* pure-core parser (spec offsets — PARITY-AUDIT F-A4-17: the previous
     * inline mapping read power_cycles/hours/unsafe/media at 64/80/96/112,
     * i.e. Host Reads / Host Writes / Controller Busy Time / Power Cycles) */
    return cr::health::parse_nvme_health_page(d, sizeof(req.data), out);
}

u32 query_trim_state(bool& enabled) {
    /* fsutil behavior query disabledeletenotify: 0 = TRIM on (corpus
     * DRWUI:178742-178749 parsing: takes the FIRST "= " match -
     * HEALTH-6: our last-line-wins could read a trailing NTFS line on
     * localized/multi-line output) */
    FILE* fp = _popen("fsutil behavior query disabledeletenotify 2>nul", "r");
    if (fp == nullptr)
        return ERR_HEALTH_UNAVAILABLE;
    char line[256];
    enabled = false;
    bool known = false;
    while (fgets(line, sizeof(line), fp)) {
        const char* eq = std::strchr(line, '=');
        if (eq == nullptr)
            continue;
        const long v = strtol(eq + 1, nullptr, 10);
        enabled = (v == 0);
        known = true;
        break; /* first match wins (og parity) */
    }
    _pclose(fp);
    return known ? OK : ERR_HEALTH_UNAVAILABLE;
}

bool looks_like_ssd(void* disk_handle) {
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return false;
    STORAGE_PROPERTY_QUERY q = {};
    q.PropertyId = StorageDeviceSeekPenaltyProperty;
    q.QueryType = PropertyStandardQuery;
    struct {
        DEVICE_SEEK_PENALTY_DESCRIPTOR d;
        u8 pad[32];
    } r = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), &r, sizeof(r), &br,
                        nullptr)) {
        return r.d.IncursSeekPenalty == FALSE; /* no seek penalty = SSD */
    }
    return false;
}

} // namespace health
} // namespace win
} // namespace cr
