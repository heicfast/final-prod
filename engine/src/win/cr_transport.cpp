/* cr_transport.cpp - Windows transports: full RDA driver client + direct handles.
 *
 * RDA wiring (rda-driver v0.2.0 contract):
 *   - RdaClient: control-device IOCTLs (GET_INFO / GET_VERSION /
 *     OPEN_TARGET structured opens, identity IOCTLs, copy-session protocol)
 *   - legacy PID-token grammar opens (EPMNtDrv / EUEDKEPM wire compat)
 *   - F11 unscale: ReadFile results through RDA handles are divided by
 *     512 (the driver reports Information = Length * 512)
 *   - reference duality (corpus Device.mo): driver path for locked/damaged
 *     volumes + direct \\.\PhysicalDriveN / HarddiskVolumeN fallbacks
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_transport.h"
#include "cr/win/cr_rda.h"
#include "cr/table/cr_table_parse.h"
#include <windows.h>
#include <winioctl.h>
#include <cstdio>
#include <cstring>

namespace cr {
namespace win {

namespace {

/* ---------------------------------------------------------------------------
 * Shared geometry probing.
 * ------------------------------------------------------------------------- */

/* (W4, AUDIT-ARCH 8c #3: a local u32_from_bytes_le re-implementation was
 * deleted here - the identical cr_types.h fold is used unqualified.) */

/* Exact byte length for volume AND disk handles (IOCTL_DISK_GET_LENGTH_INFO
 * works on both; DISK_GEOMETRY_EX rounds to the disk geometry). */
bool probe_length(HANDLE h, u64* out) {
    GET_LENGTH_INFORMATION li = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &li, sizeof(li), &br, nullptr)) {
        *out = u64(li.Length.QuadPart);
        return true;
    }
    return false;
}

Geometry probe_geometry(HANDLE h, u64* out_size) {
    Geometry g{};
    DISK_GEOMETRY_EX dg = {};
    DWORD br = 0;
    u64 exact = 0;
    const bool have_exact = probe_length(h, &exact);
    if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &dg, sizeof(dg), &br,
                        nullptr)) {
        g.logical_sector = dg.Geometry.BytesPerSector;
        g.physical_sector = dg.Geometry.BytesPerSector;
        /* corrected via the alignment query below (512e/4Kn) */
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageAccessAlignmentProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                            nullptr)) {
            /* STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR authoritative layout
             * (winioctl.h / MS-DDK): Version@0, Size@4, BytesPerCacheLine@8,
             * BytesOffsetForCacheAlignment@0xC, BytesPerLogicalSector@0x10,
             * BytesPerPhysicalSector@0x14, BytesOffsetForSectorAlignment@0x18.
             * The previous code read +8/+12 - the CACHE-LINE fields - so
             * on VHDs (cache line 0) every direct-path device reported
             * logical_sector = 0 and each PartitionSlice creation for a
             * volume scan failed (both benchmark legs, slice create
             * ERR_BAD_PARAMETER). Values accepted only in a sane sector
             * range; otherwise the geometry-EX value stands. */
            const u8* d = buf;
            const u32 align_logical = u32_from_bytes_le(d + 0x10);
            const u32 align_physical = u32_from_bytes_le(d + 0x14);
            if (align_logical >= 256 && align_logical <= 65536)
                g.logical_sector = align_logical;
            if (align_physical >= 256 && align_physical <= 65536)
                g.physical_sector = align_physical;
        } else {
            g.physical_sector = g.logical_sector;
        }
        g.total_bytes = have_exact ? exact : u64(dg.DiskSize.QuadPart);
        g.sector_count = g.total_bytes / (g.logical_sector ? g.logical_sector : 512);
    } else if (have_exact) {
        /* volume handles on some stacks: geometry IOCTL not forwarded */
        g.logical_sector = 512;
        g.physical_sector = 512;
        g.total_bytes = exact;
        g.sector_count = exact / 512;
    }
    if (out_size)
        *out_size = g.total_bytes;
    return g;
}

/* ---------------------------------------------------------------------------
 * Direct (non-RDA) device: true byte counts from the IO manager.
 * ------------------------------------------------------------------------- */
class DirectDevice final : public IStorageDevice {
public:
    DirectDevice(HANDLE h, Geometry g) : h_(h), geom_(std::move(g)) {}
    ~DirectDevice() override {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        OVERLAPPED ov = {};
        ov.Offset = u32(off);
        ov.OffsetHigh = u32(off >> 32);
        DWORD got = 0;
        if (!ReadFile(h_, buf, len, &got, &ov)) {
            const DWORD e = GetLastError();
            if (e == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(h_, &ov, &got, TRUE))
                    return ERR_READ_FAILED;
            } else {
                return ERR_READ_FAILED;
            }
        }
        return got == len ? OK : ERR_READ_FAILED;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 size() const { return geom_.total_bytes; }

private:
    HANDLE h_;
    Geometry geom_;
};

std::unique_ptr<IStorageDevice> open_path(const wchar_t* path, u32* err) {
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        /* last resort for exotic handles: file-system size */
        LARGE_INTEGER sz = {};
        if (GetFileSizeEx(h, &sz) && sz.QuadPart > 0) {
            g.logical_sector = 512;
            g.physical_sector = 512;
            g.total_bytes = u64(sz.QuadPart);
            g.sector_count = g.total_bytes / 512;
        }
    }
    if (err)
        *err = OK;
    return std::unique_ptr<IStorageDevice>(new DirectDevice(h, g));
}

/* ---------------------------------------------------------------------------
 * RDA target device: bound handle with the F11 unscale.
 * ------------------------------------------------------------------------- */
class RdaTargetDevice final : public IStorageDevice {
public:
    RdaTargetDevice(HANDLE h, Geometry g) : h_(h), geom_(std::move(g)) {}
    ~RdaTargetDevice() override {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        OVERLAPPED ov = {};
        ov.Offset = u32(off);
        ov.OffsetHigh = u32(off >> 32);
        DWORD got = 0;
        if (!ReadFile(h_, buf, len, &got, &ov)) {
            const DWORD e = GetLastError();
            if (e == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(h_, &ov, &got, TRUE))
                    return ERR_READ_FAILED;
            } else {
                return ERR_READ_FAILED;
            }
        }
        /* F11 contract: the driver reports Information = Length * 512. */
        if (got == len * rda::kLegacyInformationScale)
            return OK;
        if (got == len)
            return OK; /* future driver contract change: true counts */
        return ERR_READ_FAILED;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 size() const { return geom_.total_bytes; }

private:
    HANDLE h_;
    Geometry geom_;
};

} // namespace

/* =========================================================================
 * rda namespace: the driver client (declared in cr_rda.h).
 * ======================================================================= */
namespace rda {

u32 compute_pid_token(u32 pid) {
    const u8 b0 = u8(pid), b1 = u8(pid >> 8), b2 = u8(pid >> 16), b3 = u8(pid >> 24);
    const auto add = [](unsigned a, unsigned b) -> unsigned { return (a + b) & 0xFF; };
    return u32(b0) | (u32(add(b0, b1)) << 8) | (u32(add(b1, b2)) << 16) | (u32(add(b2, b3)) << 24);
}

std::wstring control_device_path(const wchar_t* service_name) {
    std::wstring p = L"\\\\.\\";
    p += service_name ? service_name : L"RDA";
    return p;
}

RdaClient::RdaClient() = default;

RdaClient::~RdaClient() {
    if (info_mutex_) {
        DeleteCriticalSection(static_cast<CRITICAL_SECTION*>(info_mutex_));
        free(info_mutex_);
    }
    if (ctrl_ && ctrl_ != INVALID_HANDLE_VALUE)
        CloseHandle(static_cast<HANDLE>(ctrl_));
}

void* RdaClient::open_bare_handle_(bool need_write) {
    const std::wstring path = control_device_path(service_[0] ? service_ : L"RDA");
    const DWORD access = need_write ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
    HANDLE h = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE && need_write)
        h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    return (h == INVALID_HANDLE_VALUE) ? nullptr : static_cast<void*>(h);
}

std::unique_ptr<RdaClient> RdaClient::open(const wchar_t* service_name, u32* err) {
    auto client = std::unique_ptr<RdaClient>(new RdaClient());
    if (service_name && service_name[0]) {
        wcsncpy_s(client->service_, 64, service_name, _TRUNCATE);
    } else {
        wcscpy_s(client->service_, 64, L"RDA");
    }
    client->ctrl_ = client->open_bare_handle_(true);
    if (!client->ctrl_) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    client->info_mutex_ = malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(static_cast<CRITICAL_SECTION*>(client->info_mutex_));

    DriverInfo info = {};
    if (!client->query_info(&info)) {
        /* TRANSPORT-CONTRACT §9: "GET_INFO failure on a successfully-opened
         * device = wrong/ancient driver under that service name → treat as
         * 'wrong driver'". Refuse instead of proceeding at ioctl_base_=0:
         * the tolerated-failure path could address session IOCTLs at a
         * foreign driver under the same service name (AUDIT-FALLBACKS
         * FB-T06 - the silent capability degrade is removed). */
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    client->info_ = info;
    client->have_info_ = true;
    client->ioctl_base_ = info.ioctl_base;
    if (err)
        *err = OK;
    return client;
}

bool RdaClient::ctl_ioctl(u32 code, void* in, u32 in_len, void* out, u32 out_len, u32* info_out) {
    if (!ctrl_ || !info_mutex_)
        return false;
    /* Control-handle IOCTLs serialize (one CRITICAL_SECTION for all). */
    EnterCriticalSection(static_cast<CRITICAL_SECTION*>(info_mutex_));
    DWORD br = 0;
    const BOOL ok =
        DeviceIoControl(static_cast<HANDLE>(ctrl_), code, in, in_len, out, out_len, &br, nullptr);
    LeaveCriticalSection(static_cast<CRITICAL_SECTION*>(info_mutex_));
    if (info_out)
        *info_out = br;
    return ok != FALSE;
}

bool RdaClient::query_info(DriverInfo* out) {
    DriverInfo di = {};
    if (!ctl_ioctl(kIoctlGetInfo, nullptr, 0, &di, sizeof(di), nullptr))
        return false;
    if (di.struct_size == 0 || di.struct_size > sizeof(di) * 4)
        return false;
    if (out)
        *out = di;
    return true;
}

bool RdaClient::query_version(u64* out) {
    u64 v = 0;
    if (!ctl_ioctl(kIoctlGetVersion, nullptr, 0, &v, sizeof(v), nullptr))
        return false;
    if (out)
        *out = v;
    return true;
}

std::unique_ptr<IStorageDevice> RdaClient::open_target(u32 kind, u32 disk, u32 part, u32 volume,
                                                       u32* err) {
    /* A bound handle is one bare handle + one OPEN_TARGET - the driver
     * binds FsContext2 on THAT handle. Open a fresh bare handle so the
     * client's control handle stays global-mode. */
    HANDLE h = static_cast<HANDLE>(open_bare_handle_(false));
    if (!h) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    OpenTargetIn in = {};
    in.struct_size = sizeof(in);
    in.kind = kind;
    in.disk_number = disk;
    in.part_number = part;
    in.volume_number = volume;
    DWORD br = 0;
    if (!DeviceIoControl(h, kIoctlOpenTarget, &in, sizeof(in), nullptr, 0, &br, nullptr)) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    if (err)
        *err = OK;
    return std::unique_ptr<IStorageDevice>(new RdaTargetDevice(h, g));
}

namespace {

/* Wrap a grammar-path target: \\.\SVC\<token><suffix>. */
std::unique_ptr<IStorageDevice> wrap_grammar_target(const wchar_t* service, const wchar_t* suffix,
                                                    u32* err) {
    wchar_t path[256];
    const u32 tok = compute_pid_token(GetCurrentProcessId());
    _snwprintf_s(path, 256, _TRUNCATE, L"\\\\.\\%s\\%u%s", service, tok, suffix);
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    if (err)
        *err = OK;
    return std::unique_ptr<IStorageDevice>(new RdaTargetDevice(h, g));
}

} // namespace

std::unique_ptr<IStorageDevice> RdaClient::open_disk_grammar(u32 disk, u32 part, u32* err) {
    wchar_t suffix[64];
    _snwprintf_s(suffix, 64, _TRUNCATE, L"\\%u\\%u", disk, part);
    return wrap_grammar_target(service_[0] ? service_ : L"RDA", suffix, err);
}

std::unique_ptr<IStorageDevice> RdaClient::open_volume_grammar(u32 volume, u32* err) {
    wchar_t suffix[64];
    _snwprintf_s(suffix, 64, _TRUNCATE, L"\\Volume\\%u", volume);
    return wrap_grammar_target(service_[0] ? service_ : L"RDA", suffix, err);
}

bool RdaClient::get_proc_name(u32 pid, wchar_t* out, u32 out_chars) {
    if (!out || out_chars == 0)
        return false;
    u32 in = pid;
    std::vector<wchar_t> buf(kIdentMinOutLen / 2 + 1, 0);
    if (!ctl_ioctl(kIoctlGetProcName, &in, kIdentInLen, buf.data(), kIdentMinOutLen, nullptr))
        return false;
    wcsncpy_s(out, out_chars, buf.data(), _TRUNCATE);
    return true;
}

u64 RdaClient::session_open(u32 disk_id, u64 data_first_sec, u64 total_secs, u32* spb_out,
                            u32* sector_size_out, u32* err) {
    OpenSession os = {};
    os.disk_id = disk_id;
    os.data_first_sec = data_first_sec;
    os.total_secs = total_secs;
    os.bitmap_size = 1; /* one full-range block-map region */
    os.bitmap_count = 1;
    os.spb_in_out = 0; /* auto */
    const u32 code = kSessOpenSession + ioctl_base_;
    if (!ctl_ioctl(code, &os, sizeof(os), &os, sizeof(os), nullptr)) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return 0;
    }
    if (spb_out)
        *spb_out = os.spb_in_out;
    if (sector_size_out)
        *sector_size_out = os.sector_size;
    session_sector_size_ = os.sector_size ? os.sector_size : 512;
    session_plans_[os.session_handle] = {data_first_sec, total_secs};
    if (err)
        *err = OK;
    return os.session_handle;
}

bool RdaClient::session_flush(u64 handle) {
    SessionHandleIn in = {handle};
    return ctl_ioctl(kSessSessionOp + ioctl_base_, &in, sizeof(in), nullptr, 0, nullptr);
}

bool RdaClient::session_start_backup(u64 handle, void* pump_buffer, u32 pump_bytes, u64 start_sec,
                                     u32 block_count, u32 spb, const u8* user_bitmap) {
    StartBackupIn in = {};
    in.handle = handle;
    in.pump_buffer = reinterpret_cast<u64>(pump_buffer);
    in.pump_bytes = pump_bytes;
    in.user_bitmap = reinterpret_cast<u64>(user_bitmap);
    in.start_sec = u32(start_sec);
    in.block_count = block_count;
    in.spb = spb;
    pump_bytes_ = pump_bytes;
    return ctl_ioctl(kSessSessionOp2 + ioctl_base_, &in, sizeof(in), nullptr, 0, nullptr);
}

bool RdaClient::session_get_next_block(u64 handle, u64 cursor_sec, u32 max_sec, u64* next_cursor,
                                       u8* progress, bool* stream_done, u32* err) {
    GetNextBlock io = {};
    io.handle = handle;
    io.cursor_sec = cursor_sec;
    io.max_sec = max_sec;
    if (!ctl_ioctl(kSessGetNextBlock + ioctl_base_, &io, sizeof(io), &io, sizeof(io), nullptr)) {
        /* Driver truth (rda-driver rda_session.c, pump worker): GETNEXTBLOCK
         * completes with 0xC0000120 ONLY for the no-plan/drain case - "No
         * active plan (pre-StartBackup or finalized): reference returns
         * 0xC0000120 with the progress fields filled" - and the finalize
         * path drains queued pumps with the same status. Every OTHER
         * failure is a real error (forwarded storage NTSTATUS on a bad
         * read, STATUS_INVALID_HANDLE, STATUS_NO_MEMORY on queue
         * exhaustion) and must surface as one; the reference client
         * (rdacli.c CmdSession) hard-fails on any GETNEXTBLOCK error.
         * The old blanket "any IOCTL failure = stream done, 100%, OK"
         * masked real session failures as completed imaging
         * (AUDIT-FALLBACKS FB-T07). DeviceIoControl maps the status via
         * RtlNtStatusToDosError (STATUS_CANCELLED -> ERROR_CANCELLED);
         * accept a raw NTSTATUS pass-through as well. */
        const DWORD gle = GetLastError();
        if (gle != ERROR_CANCELLED && gle != 0xC0000120u) {
            if (stream_done)
                *stream_done = false;
            if (err)
                *err = ERR_READ_FAILED;
            return false;
        }
        if (stream_done)
            *stream_done = true;
        if (progress)
            *progress = 100;
        if (next_cursor)
            *next_cursor = 0;
        if (err)
            *err = OK;
        return true;
    }
    if (progress)
        *progress = u8(io.progress);
    /* The worker pumps min(max_sec, plan-remaining, pump_capacity) sectors
     * into the registered buffer; the client knows pump capacity AND the
     * plan bounds (audit row 41a: the old estimate could run the cursor
     * past DataFirstSec+TotalSecs, overstating progress on short plans). */
    const u32 pump_secs = session_sector_size_ ? (pump_bytes_ / session_sector_size_) : 0;
    u32 granted = pump_secs && pump_secs < max_sec ? pump_secs : max_sec;
    const auto pit = session_plans_.find(handle);
    if (pit != session_plans_.end()) {
        const u64 plan_end = pit->second.first + pit->second.second;
        if (cursor_sec + granted > plan_end)
            granted = u32(plan_end > cursor_sec ? plan_end - cursor_sec : 0);
    }
    if (next_cursor)
        *next_cursor = cursor_sec + granted;
    /* The driver completes an exhausted plan with STATUS_SUCCESS and
     * Progress=100 but pumps no data (rda_session.c "Stream complete"
     * path) - a zero-grant round IS the stream-complete signal. */
    if (stream_done)
        *stream_done = granted == 0;
    if (err)
        *err = OK;
    return true;
}

bool RdaClient::session_finalize(u64 handle, u32* percent_out) {
    u8 io[kGetProgressOutLen] = {};
    memcpy(io, &handle, sizeof(handle)); /* handle @+0; percent lands @+8 */
    u32 br = 0;
    if (!ctl_ioctl(kSessGetProgress + ioctl_base_, io, kGetProgressOutLen, io, kGetProgressOutLen,
                   &br))
        return false;
    if (percent_out)
        *percent_out = u32_from_bytes_le(io + 8);
    return true;
}

bool RdaClient::session_cancel(u64 handle) {
    SessionHandleIn in = {handle};
    return ctl_ioctl(kSessCancel + ioctl_base_, &in, sizeof(in), nullptr, 0, nullptr);
}

bool RdaClient::session_close(u64 handle) {
    session_plans_.erase(handle);
    SessionHandleIn in = {handle};
    return ctl_ioctl(kSessCloseSession + ioctl_base_, &in, sizeof(in), nullptr, 0, nullptr);
}

} // namespace rda

/* =========================================================================
 * Public transport API (cr_transport.h).
 * ======================================================================= */

std::unique_ptr<IStorageDevice> open_rda_partition(const wchar_t* service_name, u32 disk, u32 part,
                                                   u32* err) {
    const u32 tok = rda::compute_pid_token(GetCurrentProcessId());
    wchar_t path[256];
    _snwprintf_s(path, 256, _TRUNCATE, L"\\\\.\\%s\\%u\\%u\\%u", service_name, tok, disk, part);
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    if (err)
        *err = OK;
    return std::unique_ptr<IStorageDevice>(new RdaTargetDevice(h, g));
}

std::unique_ptr<IStorageDevice> open_rda_volume(const wchar_t* service_name, u32 volume, u32* err) {
    const u32 tok = rda::compute_pid_token(GetCurrentProcessId());
    wchar_t path[256];
    _snwprintf_s(path, 256, _TRUNCATE, L"\\\\.\\%s\\%u\\Volume\\%u", service_name, tok, volume);
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    if (err)
        *err = OK;
    return std::unique_ptr<IStorageDevice>(new RdaTargetDevice(h, g));
}

std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32* err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    return open_path(path, err);
}

std::unique_ptr<IStorageDevice> open_harddisk_volume(u32 volume, u32* err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\HarddiskVolume%u", volume);
    return open_path(path, err);
}

bool rda_available(const wchar_t* service_name, u32* last_error) {
    const std::wstring path = rda::control_device_path(service_name);
    SetLastError(0);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (last_error)
            *last_error = GetLastError();
        return false;
    }
    CloseHandle(h);
    return true;
}

bool volume_extent(wchar_t letter, u32* disk, u64* start, u64* len) {
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h =
        CreateFileW(vol, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    bool ok = false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext, sizeof(ext), &br,
                        nullptr) &&
        ext->NumberOfDiskExtents >= 1) {
        if (disk)
            *disk = u32(ext->Extents[0].DiskNumber);
        if (start)
            *start = u64(ext->Extents[0].StartingOffset.QuadPart);
        if (len)
            *len = u64(ext->Extents[0].ExtentLength.QuadPart);
        ok = true;
    }
    CloseHandle(h);
    return ok;
}

std::unique_ptr<IStorageDevice> open_disk(u32 disk, bool prefer_rda, const wchar_t* service,
                                          u32* err) {
    if (prefer_rda) {
        /* RDA first (locked volumes, damaged media), direct fallback
         * (reference duality - Device.mo). */
        u32 e = 0;
        if (auto dev = open_rda_partition(service, disk, 0, &e))
            return dev;
    }
    return open_physical_drive(disk, err);
}

/* ---------------------------------------------------------------------------
 * Write-capable device for the repair engine (RawFixer).
 * ------------------------------------------------------------------------- */
namespace {

class RwDevice final : public cr::repair::IWriteDevice {
public:
    RwDevice(HANDLE h, Geometry g, bool via_rda) : h_(h), geom_(g), via_rda_(via_rda) {}
    ~RwDevice() override {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        OVERLAPPED ov = {};
        ov.Offset = u32(off);
        ov.OffsetHigh = u32(off >> 32);
        DWORD got = 0;
        if (!ReadFile(h_, buf, len, &got, &ov)) {
            const DWORD e = GetLastError();
            if (e == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(h_, &ov, &got, TRUE))
                    return ERR_READ_FAILED;
            } else {
                return ERR_READ_FAILED;
            }
        }
        if (!via_rda_)
            return got == len ? OK : ERR_READ_FAILED;
        /* F11 contract: the driver reports Information = Length * 512. */
        if (got == len * rda::kLegacyInformationScale)
            return OK;
        if (got == len)
            return OK; /* future driver contract change: true counts */
        return ERR_READ_FAILED;
    }

    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        /* Direct raw-disk handles require the DATA BUFFER to be
         * sector-aligned; stage through an aligned buffer when needed.
         * (The RDA path MDL-locks the user buffer at any alignment.) */
        const u32 ssec = geom_.logical_sector ? geom_.logical_sector : 512;
        void* src = const_cast<void*>(buf);
        void* staged = nullptr;
        if (!via_rda_ && (reinterpret_cast<uintptr_t>(buf) % ssec) != 0) {
            staged = _aligned_malloc(len, ssec);
            if (staged == nullptr)
                return ERR_NO_MEMORY;
            memcpy(staged, buf, len);
            src = staged;
        }
        OVERLAPPED ov = {};
        ov.Offset = u32(off);
        ov.OffsetHigh = u32(off >> 32);
        DWORD put = 0;
        const BOOL ok = WriteFile(h_, src, len, &put, &ov) || GetLastError() == ERROR_IO_PENDING;
        bool done = ok && GetOverlappedResult(h_, &ov, &put, TRUE);
        if (staged)
            _aligned_free(staged);
        /* The RDA driver reports Information = Length * 512 on writes too
         * (F11); treat either the scaled or true count as success. */
        if (!done)
            return ERR_WRITE_FAILED;
        if (via_rda_ && put != len && put != len * rda::kLegacyInformationScale)
            return ERR_WRITE_FAILED;
        if (!via_rda_ && put != len)
            return ERR_WRITE_FAILED;
        return OK;
    }

    u64 size() const override { return geom_.total_bytes; }
    u32 sector_bytes() const override { return geom_.logical_sector ? geom_.logical_sector : 512; }

private:
    HANDLE h_;
    Geometry geom_;
    bool via_rda_;
};

} // namespace

std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, bool prefer_rda,
                                                       const wchar_t* service, u32* err) {
    if (prefer_rda && service != nullptr) {
        const u32 tok = rda::compute_pid_token(GetCurrentProcessId());
        wchar_t path[256];
        _snwprintf_s(path, 256, _TRUNCATE, L"\\\\.\\%s\\%u\\%u\\%u", service, tok, disk, 0);
        HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            Geometry g = probe_geometry(h, nullptr);
            if (g.total_bytes != 0) {
                if (err)
                    *err = OK;
                return std::unique_ptr<cr::repair::IWriteDevice>(new RwDevice(h, g, true));
            }
            CloseHandle(h);
        }
    }
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.total_bytes == 0) {
        CloseHandle(h);
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    if (err)
        *err = OK;
    return std::unique_ptr<cr::repair::IWriteDevice>(new RwDevice(h, g, false));
}

bool volume_letter_to_disk(wchar_t letter, u32* disk, u32* part) {
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h =
        CreateFileW(vol, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    bool ok = false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext, sizeof(ext), &br,
                        nullptr) &&
        ext->NumberOfDiskExtents >= 1) {
        const u32 dn = u32(ext->Extents[0].DiskNumber);
        const u64 off = u64(ext->Extents[0].StartingOffset.QuadPart);
        CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
        if (disk)
            *disk = dn;
        if (part) {
            /* Exact partition number: match the extent start against the
             * disk's parsed table (GPT entries / MBR-EBR chain). */
            *part = 0;
            u32 e = 0;
            if (auto dev = open_physical_drive(dn, &e)) {
                table::DiskTable t;
                if (table::parse_disk_table(dev.get(), t) == OK) {
                    const Geometry& g = dev->geometry();
                    for (size_t i = 0; i < t.partitions.size(); ++i) {
                        if (t.partitions[i].start_lba * g.logical_sector == off) {
                            *part = u32(i + 1); /* Windows numbers from 1 */
                            break;
                        }
                    }
                }
            }
        }
        ok = true;
    }
    if (h != INVALID_HANDLE_VALUE)
        CloseHandle(h);
    return ok;
}

} // namespace win
} // namespace cr
