/* cr_health.cpp - disk health: SMART via ATA passthrough, NVMe, TRIM.
 *
 * N-032: the SMART command flow lives in the pure seam
 * (cr::health::read_smart_ata); this file provides the transports:
 *   1. IOCTL_ATA_PASS_THROUGH (ATA_PASS_THROUGH_EX)
 *   2. SMART_RCV_DRIVE_DATA 0x7C088 (pre-Vista legacy)
 *   3. CSMI CC-0x19 SCSI-miniport pass-through (the og DiskHealth
 *      companion's GetSmartAttributeCsmi route for Intel RST /
 *      AMD rcraid arrays) - wire-decoded from the corpus decompile
 *      (DiskHealth_decompiled.c FUN_00425450; see cr_csmi.h).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_health.h"
#include "cr/health/cr_csmi.h"
#include "cr/health/cr_smart_ata.h"
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
    /* current task file: feature, sector count, LBA (unused), drive, command.
     * (The og's ATA_PASS_THROUGH task-file bytes are corpus-undecoded -
     * its generic issuer FUN_00424cb0 fills the registers from params;
     * the decoded SMART magic LBA lives in its legacy + CSMI routes.) */
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
    /* SMART commands carry the og's exact legacy IDEREGS (evidence:
     * SendAtaCommandPd - SMART_READ_THRESHOLDS, DiskHealth_decompiled.c
     * :14166-14176: dword 0x4F0101D1 + 0xC2 = features D1, count 1,
     * sector 1, cyl low 4F, cyl high C2, command B0). */
    if (command == 0xB0) {
        in.regs[1] = 0x01; /* sector count */
        in.regs[2] = 0x01; /* sector number */
        in.regs[3] = 0x4F; /* cyl low (SMART magic) */
        in.regs[4] = 0xC2; /* cyl high (SMART magic) */
    }
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

/* --- transport 1: ATA passthrough --- */
class AtaPassthroughChannel final : public cr::health::AtaChannel {
    HANDLE h_;

public:
    explicit AtaPassthroughChannel(HANDLE h) : h_(h) {}
    const char* name() const override { return "ata-passthrough"; }
    bool issue(u8 feature, u8 count, u8 command, u8* out512) override {
        return ata_passthrough(h_, feature, count, command, out512);
    }
};

/* --- transport 2: legacy SMART_RCV_DRIVE_DATA --- */
class SmartRcvLegacyChannel final : public cr::health::AtaChannel {
    HANDLE h_;

public:
    explicit SmartRcvLegacyChannel(HANDLE h) : h_(h) {}
    const char* name() const override { return "smart-rcv-drive-data"; }
    bool issue(u8 feature, u8 count, u8 command, u8* out512) override {
        return smart_rcv_legacy(h_, feature, count, command, out512);
    }
};

/* --- transport 3: CSMI CC-0x19 SCSI-miniport pass-through (N-032) ---
 *
 * The og companion's Intel-RST route (GetSmartAttributeCsmi): the
 * physical-drive handle is mapped to its SCSI address
 * (IOCTL_SCSI_GET_ADDRESS - the og's own probe, DiskHealth
 * FUN_00417750), the matching \\.\Scsi%u: port device is opened, and
 * every command runs through the wire-exact request built by
 * cr::health::csmi::build_ata_request. */
class CsmiMiniportChannel final : public cr::health::AtaChannel {
    HANDLE port_ = INVALID_HANDLE_VALUE;
    u8 phy_ = 0;
    u8 path_ = 0;

public:
    explicit CsmiMiniportChannel(HANDLE disk_handle) {
        if (disk_handle == nullptr || disk_handle == INVALID_HANDLE_VALUE)
            return;
        /* SCSI address of this drive: PortNumber opens the port device,
         * PathId/TargetId route the CC-0x19 request. */
        struct { /* SCSI_ADDRESS (ntddscsi.h shape, 5 bytes + pad) */
            u8 length;
            u8 port;
            u8 path_id;
            u8 target_id;
            u8 lun;
            u8 pad[3];
        } addr;
        std::memset(&addr, 0, sizeof(addr));
        DWORD br = 0;
        if (!DeviceIoControl(disk_handle, cr::health::csmi::kIoctlScsiGetAddress, nullptr, 0, &addr,
                             sizeof(addr), &br, nullptr))
            return;
        wchar_t port_path[32];
        swprintf_s(port_path, 32, L"\\\\.\\Scsi%u:", unsigned(addr.port));
        port_ = CreateFileW(port_path, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (port_ == INVALID_HANDLE_VALUE)
            return;
        path_ = addr.path_id;
        phy_ = addr.target_id;
    }
    ~CsmiMiniportChannel() override {
        if (port_ != INVALID_HANDLE_VALUE)
            CloseHandle(port_);
    }
    CsmiMiniportChannel(const CsmiMiniportChannel&) = delete;
    CsmiMiniportChannel& operator=(const CsmiMiniportChannel&) = delete;

    const char* name() const override { return "csmi-miniport"; }
    bool issue(u8 feature, u8 count, u8 command, u8* out512) override {
        if (port_ == INVALID_HANDLE_VALUE)
            return false;
        cr::health::csmi::AtaRequest req;
        req.phy = phy_;
        req.port = path_;
        req.command = command;
        req.feature = feature;
        req.sector_count = count;
        u8 buf[cr::health::csmi::kRequestSize];
        if (cr::health::csmi::build_ata_request(req, buf, sizeof(buf)) == 0)
            return false;
        DWORD br = 0;
        if (!DeviceIoControl(port_, cr::health::csmi::kIoctlScsiMiniport, buf, sizeof(buf), buf,
                             sizeof(buf), &br, nullptr)) {
            /* the og treats these three errors as "controller does not
             * speak this route" - a clean transport miss, not a disk
             * failure (FUN_00425450's GetLastError ladder). */
            const u32 le = u32(GetLastError());
            (void)cr::health::csmi::transport_unavailable(le);
            return false;
        }
        return cr::health::csmi::extract_response_data(buf, sizeof(buf), out512);
    }
};

} // namespace

const char* smart_attr_name(u8 id) {
    return cr::health::smart_attr_name(id);
}

u32 read_smart(void* disk_handle, SmartReport& out) {
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return ERR_BAD_PARAMETER;

    /* The og CAtaSmart model: try the transports in order; the first
     * that answers IDENTIFY serves the whole command set (N-032). */
    AtaPassthroughChannel ch_ata(h);
    SmartRcvLegacyChannel ch_legacy(h);
    CsmiMiniportChannel ch_csmi(h);
    const std::vector<cr::health::AtaChannel*> chain{&ch_ata, &ch_legacy, &ch_csmi};

    cr::health::SmartInfo info;
    const u32 rc = cr::health::read_smart_ata(chain, info);
    if (rc != OK)
        return rc;

    out.available = true;
    out.transport = info.transport;
    out.ata_version = info.ata_version;
    out.smart_version = info.smart_version;
    out.failing_now = info.failing_now;
    out.grade = info.grade;
    out.reallocated_sectors = info.reallocated_sectors;
    out.pending_sectors = info.pending_sectors;
    out.uncorrectable = info.uncorrectable;
    out.power_on_hours = info.power_on_hours;
    out.spin_retry = info.spin_retry;
    out.ssd_wear_level = info.ssd_wear_level;
    out.temperature_c = info.temperature_c;
    out.attributes.clear();
    out.attributes.reserve(info.attributes.size());
    for (const cr::health::SmartAttrInfo& a : info.attributes)
        out.attributes.push_back(
            SmartAttribute{a.id,
                           a.value,
                           a.worst,
                           {a.raw[0], a.raw[1], a.raw[2], a.raw[3], a.raw[4], a.raw[5]},
                           a.vendor_threshold,
                           a.failing,
                           a.name,
                           a.grade});
    /* platform SSD probe: the model-string check already ran in the
     * pure seam; add the seek-penalty heuristic (HEALTH-5). */
    out.is_ssd = info.model_says_ssd || looks_like_ssd(h);
    return OK;
}

u32 read_nvme_health(void* disk_handle, NvmeHealth& out) {
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return ERR_BAD_PARAMETER;

    /* NVMe health log (log page 2) through the protocol-specific storage
     * query (Windows 10 storage stack; the corpus DiskHealth uses the
     * same route through its NVMe transport).
     *
     * D-NVME-01 (the debug-and-release session's second pass): the
     * request buffer is the MICROSOFT-DOCUMENTED single-buffer shape
     * ("Working with NVMe Drives", Microsoft Learn): the
     * STORAGE_PROTOCOL_SPECIFIC_DATA lives at the STORAGE_PROPERTY_QUERY's
     * AdditionalParameters field — FIELD_OFFSET(STORAGE_PROPERTY_QUERY,
     * AdditionalParameters) == 8 — NOT at sizeof(STORAGE_PROPERTY_QUERY).
     * The pre-fix buffer was `#pragma pack(push,1)`-ed, which placed the
     * protocol data at offset 9: the storage stack then read ProtocolType
     * at offset 8 as 0x00000300 (the AdditionalParameters byte merged
     * under the little-endian ProtocolTypeNvme=3), the query failed on
     * EVERY real NVMe controller, and NVMe-only drives surfaced no health
     * at all — the user's "switch to another disk and it shows nothing"
     * (disk 0 SATA answered ATA passthrough; the NVMe sibling never
     * answered). The misplacement is invisible on Azure VHDX seeds (no
     * NVMe passthrough to test against) — the class the forensic probe
     * now pins by layout contract (test_nvme_health's source-truth pins).
     *
     * Layout (natural alignment, no packing - the SDK structs verbatim):
     *   [0..7]   STORAGE_PROPERTY_QUERY fixed header (PropertyId+QueryType)
     *   [8..47]  STORAGE_PROTOCOL_SPECIFIC_DATA (AdditionalParameters)
     *   [48..559] the 512-byte log-page payload (ProtocolDataOffset=40
     *            relative to the protocol-data start, per the doc sample)
     * The same buffer serves as the output descriptor
     * (STORAGE_PROTOCOL_DATA_DESCRIPTOR also places its
     * ProtocolSpecificData at offset 8), and the payload is re-derived
     * from the RETURNED descriptor's ProtocolDataOffset — never assumed
     * to sit where the request put it. */
    static_assert(offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) == 8,
                  "the SDK header moved - the protocol data placement must follow it");
    static_assert(sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) == 40,
                  "the SDK header moved - the payload offset math must follow it");
    /* The doc sample's own shape: one byte buffer, the query header at 0,
     * the protocol data at AdditionalParameters (offset 8). A member-of-
     * enclosing-struct layout CANNOT express this (sizeof(STORAGE_PROPERTY_
     * QUERY) is 12 - the AdditionalParameters byte plus 3 tail padding -
     * and a following member lands at 12, four bytes past the driver's
     * parse offset). Explicit placement into an aligned byte buffer is
     * the only shape that satisfies both the documented contract and the
     * C layout rules. */
    constexpr size_t kQueryOff = 0;
    constexpr size_t kProtoOff = offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters); /* 8 */
    constexpr size_t kPayloadOff = kProtoOff + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);   /* 48 */
    constexpr size_t kBufSize = kPayloadOff + 512;                                       /* 560 */
    static_assert(kProtoOff == 8, "protocol data must sit at AdditionalParameters");
    static_assert(kPayloadOff == 48, "payload must sit at proto + ProtocolDataOffset");

    alignas(STORAGE_PROPERTY_QUERY) u8 buf[kBufSize];
    std::memset(buf, 0, sizeof(buf));
    auto* query = reinterpret_cast<STORAGE_PROPERTY_QUERY*>(buf + kQueryOff);
    auto* proto = reinterpret_cast<STORAGE_PROTOCOL_SPECIFIC_DATA*>(buf + kProtoOff);
    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;
    proto->ProtocolType = ProtocolTypeNvme;
    proto->DataType = NVMeDataTypeLogPage;
    proto->ProtocolDataRequestValue = 2; /* health log page */
    proto->ProtocolDataRequestSubValue = 0;
    proto->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    proto->ProtocolDataLength = 512; /* >= 512: the doc's log-page minimum */

    DWORD br = 0;
    if (!DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, buf, sizeof(buf), buf, sizeof(buf), &br,
                         nullptr))
        return ERR_HEALTH_UNAVAILABLE;

    /* Validate the returned descriptor (the doc sample's own checks) and
     * re-derive the payload pointer from the ANSWER's ProtocolDataOffset. */
    const auto* descr = reinterpret_cast<const STORAGE_PROTOCOL_DATA_DESCRIPTOR*>(buf);
    if (descr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR) ||
        descr->Size < sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))
        return ERR_HEALTH_UNAVAILABLE;
    const auto* answer = &descr->ProtocolSpecificData; /* also offset 8 */
    if (answer->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) ||
        answer->ProtocolDataLength < 512)
        return ERR_HEALTH_UNAVAILABLE;
    if (size_t(reinterpret_cast<const u8*>(answer) - buf) + answer->ProtocolDataOffset + 512 >
        kBufSize)
        return ERR_HEALTH_UNAVAILABLE; /* the answer must land inside the buffer */
    const u8* d = reinterpret_cast<const u8*>(answer) + answer->ProtocolDataOffset;
    /* pure-core parser (spec offsets — PARITY-AUDIT F-A4-17: the previous
     * inline mapping read power_cycles/hours/unsafe/media at 64/80/96/112,
     * i.e. Host Reads / Host Writes / Controller Busy Time / Power Cycles) */
    return cr::health::parse_nvme_health_page(d, 512, out);
}

u32 query_trim_state(bool& enabled) {
    /* fsutil behavior query disabledeletenotify: 0 = TRIM on (corpus
     * DRWUI:178742-178749 parsing: takes the FIRST "= " match -
     * HEALTH-6: our last-line-wins could read a trailing NTFS line on
     * localized/multi-line output).
     *
     * D2 (the debug-and-release session): the query runs WITHOUT a visible
     * console. The pre-fix CRT pipe-spawn (cmd.exe) — in the GUI-subsystem
     * product process (cr_ui.dll is loaded in-process by the Tauri shell)
     * that allocates a CONSOLE WINDOW on the user's desktop: the "popup
     * shells on opening disk health" the user reported. The og's own
     * vehicle is CreatePipe + CreateProcessA with wShowWindow=SW_HIDE and
     * piped stdout/stderr (DRWUI_decompiled.c:178726-178733); this is that
     * pattern, one step cleaner — fsutil.exe runs DIRECTLY (no shell
     * process at all), stdout+stderr both piped, CREATE_NO_WINDOW, and a
     * bounded 10 s wait (the og's own timeout) that terminates a hung
     * child. Same command, same first-'=' parse, zero windows. */
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE out_read = nullptr;
    HANDLE out_write = nullptr;
    if (!CreatePipe(&out_read, &out_write, &sa, 4096))
        return ERR_HEALTH_UNAVAILABLE;
    /* the parent's read end must not leak into the child (inherit only the
     * write end through STARTUPINFO) */
    SetHandleInformation(out_read, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = out_write;
    si.hStdError = out_write;
    si.hStdInput = nullptr;
    PROCESS_INFORMATION pi = {};
    wchar_t cmd[] = L"fsutil behavior query disabledeletenotify";
    const BOOL created = CreateProcessW(nullptr, cmd, nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                        nullptr, nullptr, &si, &pi);
    CloseHandle(out_write); /* the parent's copy: EOF when the child exits */
    if (!created) {
        CloseHandle(out_read);
        return ERR_HEALTH_UNAVAILABLE;
    }
    /* bounded wait (10 s — the og's own wait budget): a hung fsutil is
     * terminated, never waited out forever */
    if (WaitForSingleObject(pi.hProcess, 10'000) == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(out_read);
        return ERR_HEALTH_UNAVAILABLE;
    }
    /* fsutil's output is a few hundred bytes — it fits the 4 KiB pipe
     * buffer, so post-exit reading cannot deadlock */
    std::string out;
    char buf[1024];
    DWORD got = 0;
    while (ReadFile(out_read, buf, sizeof(buf), &got, nullptr) && got > 0)
        out.append(buf, got);
    CloseHandle(out_read);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    /* the same first-'=' parse as before (og parity, HEALTH-6) */
    enabled = false;
    bool known = false;
    const size_t eq = out.find('=');
    if (eq != std::string::npos) {
        const long v = strtol(out.c_str() + eq + 1, nullptr, 10);
        enabled = (v == 0);
        known = true; /* first match wins (og parity) */
    }
    return known ? OK : ERR_HEALTH_UNAVAILABLE;
}

u32 query_trim_state_handle(void* disk_handle, bool& enabled) {
    /* D2 companion: the PER-DEVICE TRIM truth, straight from the storage
     * stack — no subprocess of any kind. StorageDeviceTrimProperty answers
     * whether delete notifications (TRIM/UNMAP) are enabled for THIS
     * device: the honest per-disk datum for a per-disk health report (the
     * OS-global fsutil value — the same answer on every disk — is what
     * query_trim_state provides for the landing-page chip shape; a mixed
     * SSD+HDD machine shows real per-device states here). Registered as a
     * deliberate divergence in PARITY-DELTAS: same user-visible field,
     * per-device truth instead of the global value. */
    HANDLE h = static_cast<HANDLE>(disk_handle);
    if (h == nullptr || h == INVALID_HANDLE_VALUE)
        return ERR_HEALTH_UNAVAILABLE;
    STORAGE_PROPERTY_QUERY q = {};
    q.PropertyId = StorageDeviceTrimProperty;
    q.QueryType = PropertyStandardQuery;
    struct {
        DEVICE_TRIM_DESCRIPTOR d;
        u8 pad[16];
    } r = {};
    DWORD br = 0;
    if (!DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), &r, sizeof(r), &br,
                         nullptr))
        return ERR_HEALTH_UNAVAILABLE;
    if (br < sizeof(DEVICE_TRIM_DESCRIPTOR))
        return ERR_HEALTH_UNAVAILABLE;
    enabled = r.d.TrimEnabled != FALSE;
    return OK;
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
