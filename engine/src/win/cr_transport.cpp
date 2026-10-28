/* cr_transport.cpp - the Windows transport layer: RDA grammar path for
 * the DATA path, driver-only (docs/DRIVER-REQUIRED.md).
 *
 * RDA wiring (rda-driver v0.2.x contract):
 *   - legacy PID-token grammar opens (EPMNtDrv / EUEDKEPM wire compat) -
 *     the SOLE production surface (N-041: the structured-open RdaClient
 *     had zero callers since introduction and was deleted; the wire
 *     structs stay documented in cr_rda.h)
 *   - F11 unscale: ReadFile results through RDA handles are divided by
 *     512 (the driver reports Information = Length * 512)
 *   - driver-only architecture (PLAN-DRIVER-ONLY R1-R14): the direct
 *     data-path fallbacks, the WinPE gate, --no-rda and the boolean
 *     rda_available probe are REMOVED; query_driver_status + the
 *     ERR_DRIVER_* taxonomy replace them. Control-path direct opens
 *     (enumeration/health/layout-refresh, classified (b) in
 *     DRIVER-REQUIRED.md) stay where they always lived.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_transport.h"
#include "cr/win/cr_rda.h"
#include "cr/win/cr_enumerate.h" /* S-010: volume enumeration for the lock sequence */
#include "cr/win/cr_winenv.h"    /* M-030: the PE-gate + quiesce-chain pins */
#include "cr/cr_log.h"           /* N-048 seam: the OFFLINE failure line */
#include "cr/table/cr_table_parse.h"
#include "cr/storage/cr_directalign.h"
#include <windows.h>
#include <winioctl.h>
#include <shlwapi.h>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace cr {
namespace win {

namespace {

/* ---------------------------------------------------------------------------
 * Shared geometry probing.
 * ------------------------------------------------------------------------- */

/* (W4, AUDIT-ARCH 8c #3: a local u32_from_bytes_le re-implementation was
 * deleted here - the identical cr_types.h fold is used unqualified.) */

/* Shared OVERLAPPED read completion (W3-10, AUDIT-ARCH 8c #1): one
 * spelling of the ReadFile + ERROR_IO_PENDING + GetOverlappedResult
 * dance with the F11 unscale - the driver reports Information =
 * Length * 512 on RDA handles (kLegacyInformationScale); direct
 * handles and any future true-count RDA contract return plain len. */
u32 overlapped_read(HANDLE h, u64 off, void* buf, u32 len, bool rda_scaled, DWORD* gle_out) {
    if (len == 0)
        return OK;
    OVERLAPPED ov = {};
    ov.Offset = u32(off);
    ov.OffsetHigh = u32(off >> 32);
    DWORD got = 0;
    if (!ReadFile(h, buf, len, &got, &ov)) {
        const DWORD e = GetLastError();
        if (e != ERROR_IO_PENDING) {
            if (gle_out)
                *gle_out = e;
            /* N-125: the device-gone family classifies distinctly (the
             * reattach-then-resume contract); media errors stay
             * ERR_READ_FAILED (the dying-disk storm keeps its semantics) */
            return disconnect::gle_is_disconnect(e) ? ERR_DEVICE_DISCONNECTED : ERR_READ_FAILED;
        }
        if (!GetOverlappedResult(h, &ov, &got, TRUE)) {
            const DWORD e2 = GetLastError();
            if (gle_out)
                *gle_out = e2;
            return disconnect::gle_is_disconnect(e2) ? ERR_DEVICE_DISCONNECTED
                                                     : ERR_READ_FAILED;
        }
    }
    if (got == len)
        return OK;
    if (rda_scaled && got == len * rda::kLegacyInformationScale)
        return OK; /* F11 contract */
    return ERR_READ_FAILED;
}

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
 * THE 64 KiB DIRECT-READ ENVELOPE - UNIFIED across BOTH transports
 * (core-api session; mandate section 3, non-negotiable).
 *
 * Two original constraints, one envelope:
 *
 * (a) Sector rejection (bench run 40073093997): the Windows disk stack
 *     rejects ReadFile IRPs whose LENGTH or offset is not a multiple of
 *     the logical sector size - evidence: every recovered file with
 *     size % 512 != 0 was zero-filled 'salvaged' while all sector-multiple
 *     files hash-verified, on BOTH transports. 64 KiB is a multiple of
 *     every real Windows logical sector size (512 / 4096 / 65536), so the
 *     64 KiB envelope subsumes sector alignment.
 *
 * (b) GAP-S6-13 (mandate section 4): direct reads whose length is a
 *     multiple of 4 KiB but not 64 KiB take the ~3.5x slow path in the
 *     Windows storage stack (~830 MB/s vs ~2900 MB/s on NVMe). The
 *     mandate requires this property on EVERY direct read - through the
 *     driver OR through the direct fallback. The corpus confirms the
 *     reference satisfies it by construction: every chunk size the og
 *     issues is a 64 KiB multiple (4 MiB device chunks
 *     Device_decompiled.c:56545, 7 MiB volume chunks :56912, 2 MiB MFT
 *     batches NTFSSearchDLL_decompiled.c:5649-5678, 1 MiB $UsnJrnl, 64 KiB
 *     boot region).
 *
 * The pre-unification RDA path used a sector-only envelope (the 'kernel
 * IRP path is exempt' argument - technically true for the cache-manager
 * slow path, but it left the mandate's property unproven for the driver
 * transport and let probe/tail reads issue sector-granular lengths).
 * Unified: BOTH paths round offset down / length up to 64 KiB multiples,
 * clamped at the device end; already-aligned requests pass through with
 * no bounce copy. The F11 Information-scale compensation is orthogonal
 * (it keys on the ISSUED length, not the caller's).
 *
 * Threading: reads through the engine pipe arrive serialized under the
 * CachedReader mutex; per-handle single-flight is the ABI contract
 * (ABI-CONTRACT section 3), so the shared HANDLE is safe here. The
 * per-thread bounce below additionally tolerates concurrent callers.
 * ------------------------------------------------------------------------ */
u32 aligned_read_at(HANDLE h, u64 off, void* buf, u32 len, bool rda_scaled, u64 dev_size,
                    DWORD* gle_out) {
    if (len == 0)
        return OK;
    const io::DirectEnvelope e = io::direct_envelope(off, len, dev_size);
    if (!e.ok)
        return ERR_RANGE_OUT_OF_BOUNDS;
    if (e.direct)
        return overlapped_read(h, off, buf, len, rda_scaled, gle_out); /* fast path: no copy */
    /* bounded bounce: the writer chunks at 1 MiB; anything bigger means
     * caller-side chunking broke - refuse loudly instead of allocating
     * wildly */
    if (e.issued_len > (1u << 20) + io::kDirectAlignBytes)
        return ERR_BAD_PARAMETER;
    /* per-thread bounce (raw-handle reads can arrive from multiple
     * worker items concurrently) */
    thread_local std::vector<u8> bounce;
    bounce.resize(size_t(e.issued_len));
    const u32 rc =
        overlapped_read(h, e.issued_off, bounce.data(), u32(e.issued_len), rda_scaled, gle_out);
    if (rc != OK)
        return rc;
    if (e.take != u64(len))
        return ERR_READ_FAILED; /* envelope did not cover the request */
    std::memcpy(buf, bounce.data() + size_t(e.skip), size_t(e.take));
    return OK;
}

/* ---------------------------------------------------------------------------
 * Handle self-healing (og parity, core-api session).
 *
 * The reference's device layer recovers a dead handle IN PLACE: on
 * ERROR_INVALID_HANDLE (6) during the read loop it CloseHandle's, re-
 * CreateFile's the same target and retries the seek
 * (Device_decompiled.c:55947-55959, CWindowsStorageDevice::Read; the
 * same guard pattern rides the write loop :56182-56183). Without this,
 * our retry layer cannot heal a stale handle (every attempt fails
 * identically, bisects, and Skip-mode zero-fills data that a reopen
 * would have recovered).
 *
 * One heal attempt per failing call; the reopen keeps the ORIGINAL open
 * parameters (path / access / share - the og reopens with its clone
 * default 0xC0000000 share-3; we keep the per-site flags, which is the
 * same semantics: a fresh handle to the same target). Geometry is NOT
 * re-probed (the og doesn't either). heal_mu_ serializes concurrent
 * healers; the winning reopen swaps the handle under the lock and the
 * loser's retry rides the fresh handle too.
 * ------------------------------------------------------------------------ */
bool reopen_handle(const wchar_t* path, DWORD access, DWORD share, HANDLE* h_inout) {
    if (!path || !path[0])
        return false;
    HANDLE nh =
        CreateFileW(path, access, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nh == INVALID_HANDLE_VALUE)
        return false;
    HANDLE old = *h_inout;
    *h_inout = nh;
    if (old != INVALID_HANDLE_VALUE && old != nh)
        CloseHandle(old);
    return true;
}

/* ---------------------------------------------------------------------------
 * Direct (non-RDA) device: true byte counts from the IO manager.
 * ------------------------------------------------------------------------- */
class DirectDevice final : public IStorageDevice {
public:
    DirectDevice(HANDLE h, Geometry g, std::wstring path, DWORD access, DWORD share)
        : h_(h), geom_(std::move(g)), path_(std::move(path)), access_(access), share_(share) {}
    ~DirectDevice() override {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        DWORD gle = 0;
        u32 rc = aligned_read_at(h_, off, buf, len, false, size(), &gle);
        if (rc == ERR_READ_FAILED && gle == ERROR_INVALID_HANDLE) {
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_handle(path_.c_str(), access_, share_, &h_))
                rc = aligned_read_at(h_, off, buf, len, false, size(), nullptr);
        }
        return rc;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 size() const { return geom_.total_bytes; }

private:
    HANDLE h_;
    Geometry geom_;
    std::wstring path_;
    DWORD access_ = GENERIC_READ;
    DWORD share_ = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    std::mutex heal_mu_;
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
    return std::unique_ptr<IStorageDevice>(
        new DirectDevice(h, g, std::wstring(path), GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
}

/* ---------------------------------------------------------------------------
 * RDA target device: bound handle with the F11 unscale; grammar-opened
 * (reopen = re-CreateFile the same grammar path - the driver re-binds the
 * target at IRP_MJ_CREATE) or structured-opened (reopen = bare handle +
 * re-issued OPEN_TARGET).
 * ------------------------------------------------------------------------- */
class RdaTargetDevice final : public IStorageDevice {
public:
    RdaTargetDevice(HANDLE h, Geometry g, std::wstring path, DWORD access, DWORD share)
        : h_(h), geom_(std::move(g)), path_(std::move(path)), access_(access), share_(share) {}
    ~RdaTargetDevice() override {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        DWORD gle = 0;
        u32 rc = aligned_read_at(h_, off, buf, len, true, size(), &gle);
        if (rc == ERR_READ_FAILED && gle == ERROR_INVALID_HANDLE) {
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_and_rebind_())
                rc = aligned_read_at(h_, off, buf, len, true, size(), nullptr);
        }
        return rc;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 size() const { return geom_.total_bytes; }

private:
    /* grammar path: reopen is just CreateFileW (the driver validates the
     * PID token + resolves the target at CREATE). N-041: the structured
     * OPEN_TARGET reopen variant died with RdaClient (zero callers). */
    bool reopen_and_rebind_() { return reopen_handle(path_.c_str(), access_, share_, &h_); }
    HANDLE h_;
    Geometry geom_;
    std::wstring path_;
    DWORD access_ = GENERIC_READ;
    DWORD share_ = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    std::mutex heal_mu_; /* M-055: N-041's deletion dropped the member but
                          * kept read_at's heal lock; restored (DirectDevice
                          * line ~271 keeps the same pattern) */
};

} // namespace

/* =========================================================================
 * rda namespace: the PID-token handshake + service-name helpers (the
 * RdaClient driver-client class was deleted by N-041 - zero callers
 * since introduction; wire structs stay documented in cr_rda.h).
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

} // namespace rda

/* =========================================================================
 * Public transport API (cr_transport.h).
 * ======================================================================= */

/* The driver-status query (docs/DRIVER-REQUIRED.md section 2-3; the pure
 * classifier lives in cr_driver_status.h where both lanes compile it).
 *
 * Mechanism per facet (DRIVER-CONTRACT.md section 8 - the driver's entire
 * user-mode surface is the control-device open + GET_INFO):
 *   present/loaded  SCM: OpenSCManagerW(CONNECT) + OpenServiceW(
 *                    QUERY_STATUS) + QueryServiceStatus
 *   started/
 *   authorized      CreateFileW on \\.\RDA (the bare control device;
 *                    ERROR_ACCESS_DENIED = the DACL denial)
 *   healthy/
 *   version         GET_INFO 0x6002440 (24-byte DriverInfo) +
 *                    GET_VERSION 0x6002448 (u64 (major<<32)|minor),
 *                    cross-checked against each other
 *
 * The query never fails on a driver CONDITION - it reports it; the caller
 * classifies (driver_state) and refuses (driver_state_error). Every handle
 * (SCM, service, device) is closed before returning; the call retains no
 * state and is thread-safe under its own handles. */
u32 query_driver_status(const wchar_t* service_name, DriverStatus* out) {
    if (!out)
        return ERR_BAD_PARAMETER;
    if (!service_name || !service_name[0])
        service_name = kDefaultDriverService;
    *out = DriverStatus{};

    /* present + loaded (SCM). OpenSCManager failure is itself diagnostic
     * (rare: SCM down) - reported as service_present=false and the caller
     * classifies Absent; last_gle carries the SCM error for the log. */
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scm) {
        SC_HANDLE svc = OpenServiceW(scm, service_name, SERVICE_QUERY_STATUS);
        if (svc) {
            out->service_present = true;
            SERVICE_STATUS ss = {};
            if (QueryServiceStatus(svc, &ss))
                out->service_running = ss.dwCurrentState == SERVICE_RUNNING;
            else
                out->last_gle = GetLastError();
            CloseServiceHandle(svc);
        } else {
            out->last_gle = GetLastError(); /* ERROR_SERVICE_DOES_NOT_EXIST (1060) = absent */
        }
        CloseServiceHandle(scm);
    } else {
        out->last_gle = GetLastError();
    }

    /* started + authorized (control-device open - the exact production
     * posture rda_available used, minus the boolean collapse). */
    const std::wstring path = rda::control_device_path(service_name);
    SetLastError(0);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        out->last_gle = GetLastError();
        return OK; /* absent/not-started/unauthorized/errored - classified upstream */
    }
    out->device_openable = true;
    out->authorized = true;

    /* healthy + version: GET_INFO (the first core code to ever issue the
     * documented extended IOCTLs - cr_rda.h's structs stop being
     * documentation-only) cross-checked against GET_VERSION. FB-T06's
     * contract rides here: GET_INFO failure on an OPENED device = wrong /
     * ancient driver -> info_valid stays false -> Errored. */
    rda::DriverInfo info = {};
    DWORD br = 0;
    if (DeviceIoControl(h, rda::kIoctlGetInfo, nullptr, 0, &info, sizeof(info), &br, nullptr) &&
        br >= sizeof(info) && info.struct_size == sizeof(info)) {
        u64 wire = 0;
        if (DeviceIoControl(h, rda::kIoctlGetVersion, nullptr, 0, &wire, sizeof(wire), &br,
                            nullptr) &&
            br >= sizeof(wire) && u32(wire >> 32) == info.major_version &&
            u32(wire) == info.minor_version) {
            out->info_valid = true;
            out->major = info.major_version;
            out->minor = info.minor_version;
            out->flags = info.flags;
            out->max_disks = info.max_disks;
            out->ioctl_base = info.ioctl_base;
        } else if (!(br >= sizeof(wire))) {
            out->last_gle = GetLastError();
        }
    } else {
        out->last_gle = GetLastError();
    }
    CloseHandle(h);
    return OK;
}

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
    return std::unique_ptr<IStorageDevice>(
        new RdaTargetDevice(h, g, std::wstring(path), GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
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
    return std::unique_ptr<IStorageDevice>(
        new RdaTargetDevice(h, g, std::wstring(path), GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
}

/* CONTROL-PATH open (enumeration's whole-disk view; classified (b) in
 * docs/DRIVER-REQUIRED.md - the og's own enumeration never touches the
 * driver, getdriverinfo:20-29). NOT a data-path leg: the scan/recover/
 * fix/apply/bench data path is open_rda_partition/open_disk. The
 * open_harddisk_volume sibling and the data-path direct fallbacks were
 * removed with the driver-only architecture. */
std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32* err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    return open_path(path, err);
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

std::unique_ptr<IStorageDevice> open_disk(u32 disk, const wchar_t* service, u32* err) {
    /* Driver-only (PLAN-DRIVER-ONLY R2): the grammar path is the ONLY leg.
     * The direct fallback and the WinPE gate are REMOVED - a deliberate
     * divergence from the og's environment-gated duality (the og opens the
     * PhysicalDrive device directly in WinPE and never retries the other
     * transport on failure, Device_decompiled.c :50653-50663; our product
     * requires the driver everywhere, WinPE included - the driver ships
     * scripts/winpe/regdrv-rda.reg for PE registration, DRIVER-RELEASE
     * section 6). Callers needing the classified refusal check driver
     * state first (query_driver_status); a failure here with a Ready
     * driver is a TARGET error, not a driver-state refusal. */
    return open_rda_partition(service ? service : kDefaultDriverService, disk, 0, err);
}

/* ---------------------------------------------------------------------------
 * Write-capable device for the repair engine (RawFixer) and the apply
 * engine (S-029) - the base is apply::IApplyDevice (IS-A IWriteDevice),
 * so one class carries the S-010 quiesce + the corpus PartitionRecovery
 * device-control chain.
 * ------------------------------------------------------------------------- */
namespace {

class RwDevice final : public cr::apply::IApplyDevice {
public:
    RwDevice(HANDLE h, Geometry g, bool via_rda, u32 disk = 0, std::wstring path = std::wstring(),
             DWORD access = GENERIC_READ | GENERIC_WRITE,
             DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE)
        : h_(h), geom_(g), via_rda_(via_rda), disk_(disk), path_(std::move(path)), access_(access),
          share_(share) {}
    ~RwDevice() override {
        /* S-010: closing the held volume handles releases their locks
         * (the Win32 contract: a volume lock lasts until every handle
         * that took it closes). */
        release_locked_volumes_();
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }

    /* S-010 volume quiesce + M-030's terminal step (corpus
     * RecoveryModule_decompiled.c :60401-60447 - the repair-write
     * quiesce chain; the earlier :44686-44720 cite was stale, corrected
     * this session). Per MOUNTED volume on this disk -
     *   FSCTL_LOCK_VOLUME (0x90018)    :60401 - a busy volume = refuse
     *   DeleteVolumeMountPointA         :60412 - the OS must not re-open it
     *   FlushFileBuffers                :60422
     *   Sleep(2000) settle              :60428
     *   FSCTL_DISMOUNT_VOLUME (0x90020) :60431
     *   FSCTL_UNLOCK_VOLUME (0x9001c)   :60436 - the og unlocks before
     *     the terminal step; the core holds its lock to the close below
     *     (the unlock releases at handle close - same terminal state)
     *   IOCTL_VOLUME_OFFLINE (0x56c00c) :60440 - M-030: THE TERMINAL
     *     STEP, previously missing: a mounted-but-locked volume can
     *     still receive third-party I/O through pre-existing handles;
     *     OFFLINE closes that window (the og offlines, never just locks).
     * The volume handles are held for the device lifetime. Unmounted
     * volumes (no letter) have nothing to quiesce. A lock that cannot
     * be acquired = ERR_VOLUME_LOCK_FAILED: execute() refuses the whole
     * repair before the first write - the concurrent user keeps the
     * volume, never a raced write. */
    u32 lock_volume() override {
        if (locked_)
            return OK; /* idempotent */
        VolumeEntryList vols;
        if (enumerate_volumes(vols) != OK)
            return OK; /* enumeration unavailable (e.g. no letters at all) */
        for (const auto& v : vols) {
            if (v.disk != disk_ || v.letter == 0)
                continue;
            wchar_t vol_path[16];
            _snwprintf_s(vol_path, 16, _TRUNCATE, L"\\\\.\\%c:", v.letter);
            HANDLE vh = CreateFileW(vol_path, GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
            if (vh == INVALID_HANDLE_VALUE)
                return ERR_VOLUME_LOCK_FAILED; /* in use: refuse, never race */
            DWORD bytes_ret = 0;
            if (!DeviceIoControl(vh, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0, &bytes_ret,
                                 nullptr)) {
                CloseHandle(vh);
                return ERR_VOLUME_LOCK_FAILED; /* busy volume: clean refusal */
            }
            /* M-030: capture the volume's GUID path BEFORE the mount
             * point goes away - the OFFLINE terminal step below removes
             * the volume from the mountable set, and the restore path
             * (release_locked_volumes_) needs \??\Volume{...}\ to bring
             * it back online. The corpus restore (FUN_1800459b0
             * :34465-34502) reopens the volume device by identity. */
            wchar_t mount[8] = {v.letter, L':', L'\\', 0};
            wchar_t guid[64] = {};
            VolumeRestore vr;
            vr.letter = v.letter;
            (void)GetVolumeNameForVolumeMountPointW(mount, guid, 64);
            vr.guid = guid; /* empty on failure: the restore degrades to
                             * handle-close only (never worse than the
                             * pre-M-030 behavior) */
            /* the letter must go so the OS does not hand the volume back
             * out (corpus :60412); flush + settle + verify + dismount */
            DeleteVolumeMountPointW(mount);
            FlushFileBuffers(vh);
            Sleep(2000); /* corpus :60428 settle */
            DeviceIoControl(vh, FSCTL_IS_VOLUME_MOUNTED, nullptr, 0, nullptr, 0, &bytes_ret,
                            nullptr); /* verify */
            DeviceIoControl(vh, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0, &bytes_ret,
                            nullptr); /* corpus :60431 */
            /* M-030: the terminal OFFLINE step (corpus :60440,
             * IOCTL_VOLUME_OFFLINE 0x56c00c). Failure is recorded, not
             * fatal: the og's chain treats every step's error the same
             * way (GetLastError() into the chain's error slot) and
             * proceeds - the volume is still dismounted and locked; a
             * failed OFFLINE degrades to exactly the pre-M-030 posture
             * (surfaced through the N-048 log seam, never silent). */
            if (!DeviceIoControl(vh, IOCTL_VOLUME_OFFLINE, nullptr, 0, nullptr, 0, &bytes_ret,
                                 nullptr)) {
                const DWORD offline_gle = GetLastError();
                cr::log::write(cr::log::Level::Warn,
                               "quiesce: volume offline failed gle=%lu (degraded "
                               "to dismount+lock posture)",
                               (unsigned long)offline_gle);
            }
            locked_volumes_.push_back(vh);
            restores_.push_back(vr);
        }
        locked_ = true;
        return OK;
    }
    void unlock_volume() override {
        /* closing the handles releases their locks (the destructor does
         * the same); dropping them here matches the symmetric contract
         * for callers that finish early */
        release_locked_volumes_();
    }

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        /* M-009 (GAP-S6-13 mandate) + core-api unification: route through
         * the 64 KiB direct-I/O envelope exactly like the scan-path devices
         * (DirectDevice, RdaTargetDevice) - BOTH transports, one envelope. */
        DWORD gle = 0;
        u32 rc = aligned_read_at(h_, off, buf, len, via_rda_, size(), &gle);
        if (rc == ERR_READ_FAILED && gle == ERROR_INVALID_HANDLE) {
            /* og parity: heal the dead handle, retry once */
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_handle(path_.c_str(), access_, share_, &h_))
                rc = aligned_read_at(h_, off, buf, len, via_rda_, size(), nullptr);
        }
        return rc;
    }

    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        u32 rc = write_impl_(off, buf, len);
        if (rc == ERR_WRITE_FAILED && write_gle_ == ERROR_INVALID_HANDLE) {
            /* og parity (Device_decompiled.c:56182-56183 guard pattern):
             * heal the dead handle, retry the write once. */
            std::lock_guard<std::mutex> g(heal_mu_);
            if (reopen_handle(path_.c_str(), access_, share_, &h_))
                rc = write_impl_(off, buf, len);
        }
        return rc;
    }

    u64 size() const override { return geom_.total_bytes; }
    u32 sector_bytes() const override { return geom_.logical_sector ? geom_.logical_sector : 512; }

    /* ---- S-029: the corpus PartitionRecovery device-control chain ----
     * (PartitionRecovery_decompiled.c FUN_1800566d0/FUN_180038b10; the
     * raw hex values are the decompile's own - cited per method in
     * cr_apply.h. Error convention: OK = the DeviceIoControl succeeded;
     * failures return the transport's ERR_WRITE_FAILED so execute_apply
     * treats them as hard device errors and rolls back.) */
    u32 delete_drive_layout() override {
        /* 0x7C100 IOCTL_DISK_DELETE_DRIVE_LAYOUT (legacy path, pre-write) */
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x7C100, nullptr, 0, nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 set_drive_layout_mbr(u32 signature, u32 entry_count) override {
        /* 0x7C010 IOCTL_DISK_SET_DRIVE_LAYOUT with the 0xA8-byte
         * DRIVE_LAYOUT_INFORMATION: PartitionCount = entry_count (the
         * corpus stamps 4 in the rebuild flow), Mbr.Signature = the
         * effective signature; the entry array mirrors the sector-0
         * bytes the core already wrote (the kernel validates/commits
         * them into its layout view). */
        struct {
            u32 partition_count;
            u32 signature;
            u8 entries[4 * 40]; /* 4 x PARTITION_INFORMATION-shaped slots */
        } dli = {};
        dli.partition_count = entry_count ? entry_count : 4;
        dli.signature = signature;
        /* the per-entry payload is zero except RewriteBoot on the FIRST
         * entry: the sector-0 write already carried the real bytes; the
         * ioctl's job here is the layout-signature commit (corpus
         * FUN_1800566d0 zeroes the whole 0xA8 buffer and stamps count+sig). */
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x7C010, &dli, sizeof(dli), nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 set_partition_info_ex(const PartitionInfoExRecord& rec) override {
        /* 0x7C058 with the 0x18-byte record (FUN_180056bc0 tail:
         * {u32 partition number, 16-byte identity, u32 boot flag}) */
        struct {
            u32 partition_number;
            u8 identity[16];
            u32 boot_flag;
        } rec18 = {};
        rec18.partition_number = rec.partition_number;
        for (int i = 0; i < 16; ++i)
            rec18.identity[i] = rec.identity[i];
        rec18.boot_flag = rec.boot_flag;
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x7C058, &rec18, sizeof(rec18), nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 update_properties() override {
        /* 0x70140 IOCTL_DISK_UPDATE_PROPERTIES */
        DWORD br = 0;
        if (!DeviceIoControl(h_, 0x70140, nullptr, 0, nullptr, 0, &br, nullptr))
            return ERR_WRITE_FAILED;
        return OK;
    }
    void settle_refresh() override {
        Sleep(1000); /* corpus PR:47015-47025: settle between retries */
    }
    bool legacy_os_path() const override {
        /* corpus GetVersionExA gate (legacy = 5.0 or older). The build's
         * envelope is Win7 SP1+ (6.1+): the modern path is the only live
         * branch - the constant keeps the corpus contract visible. */
        return false;
    }
    u32 generate_disk_signature() override {
        /* corpus GPT_GenerateSignature (PARTITION.MO) - a fresh 32-bit
         * disk signature. Entropy from kernel32-only sources (the import
         * allowlist stays untouched - no Ole32 dependency for this):
         * performance counter + tick count + the disk number, mixed
         * FNV-style; two consecutive rebuilds never stamp the same value. */
        LARGE_INTEGER pc = {};
        QueryPerformanceCounter(&pc);
        const u64 mix = u64(pc.QuadPart) ^ (u64(GetTickCount()) << 32) ^ (u64(disk_) << 48);
        u32 h = 0x811C9DC5u;
        for (int shift = 0; shift < 64; shift += 8) {
            h ^= u32((mix >> shift) & 0xFF);
            h *= 0x01000193u;
        }
        return h ? h : 1u;
    }

private:
    /* The staged WriteFile (direct path needs a sector-aligned DATA
     * BUFFER; the RDA path MDL-locks the user buffer at any alignment).
     * Records the failing GLE in write_gle_ for the heal decision. F11:
     * the RDA driver reports Information = Length * 512 on writes too -
     * either the scaled or true count is success. */
    u32 write_impl_(u64 off, const void* buf, u32 len) {
        write_gle_ = 0;
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
        const BOOL ok = WriteFile(h_, src, len, &put, &ov);
        if (!ok) {
            const DWORD e = GetLastError();
            if (e != ERROR_IO_PENDING) {
                if (staged)
                    _aligned_free(staged);
                write_gle_ = e;
                return ERR_WRITE_FAILED;
            }
            if (!GetOverlappedResult(h_, &ov, &put, TRUE)) {
                if (staged)
                    _aligned_free(staged);
                write_gle_ = GetLastError();
                return ERR_WRITE_FAILED;
            }
        }
        if (staged)
            _aligned_free(staged);
        if (via_rda_ && put != len && put != len * rda::kLegacyInformationScale)
            return ERR_WRITE_FAILED;
        if (!via_rda_ && put != len)
            return ERR_WRITE_FAILED;
        return OK;
    }

    /* close every held volume handle - that is what releases the locks.
     * M-030: for every volume the quiesce OFFLINED, run the og's restore
     * chain first (corpus FUN_1800459b0, RecoveryModule_decompiled.c
     * :34465-34502, decoded this session - OQ-2 resolved): reopen the
     * volume by its GUID path, IOCTL_VOLUME_ONLINE (0x56c008 :34484),
     * re-create the mount point (SetVolumeMountPointW - the saved
     * letter; the og's vtable +0x78 restore step), Sleep(1000) settle
     * (:34499), close. Without this the quiesced volume would stay
     * letterless after repair - the pre-M-030 core deleted the mount
     * point and never restored it. A volume whose GUID was not
     * captured degrades to the handle-close only (the pre-M-030
     * behavior, never worse). */
    struct VolumeRestore {
        wchar_t letter = 0;
        std::wstring guid; /* \\??\\Volume{...}\\ or empty */
    };

    void release_locked_volumes_() {
        for (size_t k = 0; k < locked_volumes_.size(); ++k) {
            HANDLE vh = locked_volumes_[k];
            if (vh == INVALID_HANDLE_VALUE)
                continue;
            CloseHandle(vh); /* releases the volume lock first */
            if (k < restores_.size() && !restores_[k].guid.empty()) {
                const VolumeRestore& vr = restores_[k];
                HANDLE vh2 = CreateFileW(vr.guid.c_str(), GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL, nullptr);
                if (vh2 != INVALID_HANDLE_VALUE) {
                    DWORD br = 0;
                    (void)DeviceIoControl(vh2, IOCTL_VOLUME_ONLINE, nullptr, 0, nullptr, 0, &br,
                                          nullptr);
                    CloseHandle(vh2);
                }
                wchar_t mount[8] = {vr.letter, L':', L'\\', 0};
                (void)SetVolumeMountPointW(vr.guid.c_str(), mount);
                Sleep(1000); /* corpus :34499 settle */
            }
        }
        locked_volumes_.clear();
        restores_.clear();
        locked_ = false;
    }

    HANDLE h_;
    Geometry geom_;
    bool via_rda_ = false;
    u32 disk_ = 0;
    std::wstring path_;
    DWORD access_ = GENERIC_READ | GENERIC_WRITE;
    DWORD share_ = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD write_gle_ = 0;
    std::mutex heal_mu_;
    bool locked_ = false;
    std::vector<HANDLE> locked_volumes_;
    std::vector<VolumeRestore> restores_; /* pushed in lockstep with
                                           * locked_volumes_ (same loop) */
};

} // namespace

/* shared RW open (driver-only, PLAN-DRIVER-ONLY R3): the concrete RwDevice
 * is an apply::IApplyDevice (IS-A IWriteDevice), so the two public factories
 * are typed views of one open path. The direct \\.\PhysicalDriveN RW tail
 * is REMOVED - the grammar path is the only leg; a failure here with a
 * Ready driver is a target error (ERR_OPEN_FAILED), not a driver-state
 * refusal (callers check query_driver_status first when they need the
 * classified ERR_DRIVER_* contract). */
namespace {
std::unique_ptr<RwDevice> open_rw_device(u32 disk, const wchar_t* service, u32* err) {
    const wchar_t* svc = service ? service : kDefaultDriverService;
    const u32 tok = rda::compute_pid_token(GetCurrentProcessId());
    wchar_t path[256];
    _snwprintf_s(path, 256, _TRUNCATE, L"\\\\.\\%s\\%u\\%u\\%u", svc, tok, disk, 0);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
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
    return std::unique_ptr<RwDevice>(
        new RwDevice(h, g, true, disk, std::wstring(path), GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
}
} // namespace

std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, const wchar_t* service, u32* err) {
    return open_rw_device(disk, service, err);
}

std::unique_ptr<cr::apply::IApplyDevice> open_disk_apply_rw(u32 disk, const wchar_t* service,
                                                            u32* err) {
    return open_rw_device(disk, service, err);
}

u32 refresh_disk_properties(u32 disk) {
    /* S-029 (apply): the corpus settle pattern - IOCTL_DISK_UPDATE_PROPERTIES
     * (0x70140) with up to 3 x Sleep(1000) retries (PartitionRecovery
     * PR:46873-46880). The table write already succeeded when this runs;
     * the refresh makes the OS re-read the layout so the recovered volume
     * becomes visible. Best-effort by contract: a refusal (permissions,
     * transient busy) reports its code, the caller surfaces it without
     * failing the apply. */
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return ERR_OPEN_FAILED;
    DWORD bytes = 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (DeviceIoControl(h, IOCTL_DISK_UPDATE_PROPERTIES, nullptr, 0, nullptr, 0, &bytes,
                            nullptr)) {
            CloseHandle(h);
            return OK;
        }
        Sleep(1000); /* corpus :46877 settle between retries */
    }
    CloseHandle(h);
    return ERR_WRITE_FAILED;
}

/* ---------------------------------------------------------------------------
 * N-013: destination-side overlap arming, full-extent edition.
 * (M-055: these two definitions must sit INSIDE namespace cr::win to match
 * cr_transport.h's declarations - they were born after the namespace close,
 * which both broke the VolExtent lookup and left the declared functions
 * undefined at link time.)
 * ------------------------------------------------------------------------- */
bool volume_extents(wchar_t letter, std::vector<VolExtent>* out) {
    if (!out)
        return false;
    out->clear();
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h = CreateFileW(vol, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    BOOL io = DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext, sizeof(ext),
                              &br, nullptr);
    CloseHandle(h);
    /* ERROR_MORE_DATA: the volume has more than 8 extents - the first 8
     * are filled and used (documented cap). */
    if (!io && GetLastError() != ERROR_MORE_DATA)
        return false;
    for (DWORD i = 0; i < ext->NumberOfDiskExtents && i < 8; ++i) {
        VolExtent ve{};
        ve.disk = u32(ext->Extents[i].DiskNumber);
        ve.start = u64(ext->Extents[i].StartingOffset.QuadPart);
        ve.len = u64(ext->Extents[i].ExtentLength.QuadPart);
        out->push_back(ve);
    }
    return !out->empty();
}

bool destination_volume_letter(const std::wstring& path, wchar_t* letter) {
    if (!letter || path.empty())
        return false;
    /* The og's PathIsNetworkPathW branch: network destinations are never
     * opened raw, never armed (cannot be evaluated). */
    if (PathIsNetworkPathW(path.c_str()))
        return false;
    /* Resolve reparse points (junctions/mount points) to the FINAL path:
     * a junction into another volume must yield the TARGET's volume, not
     * the junction's host volume. */
    std::wstring final_path = path;
    HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        wchar_t buf[1024];
        const DWORD n = GetFinalPathNameByHandleW(h, buf, 1024, VOLUME_NAME_DOS);
        if (n > 0 && n < 1024) {
            final_path = buf;
            /* strip the \\?\ prefix if present (GetVolumePathNameW wants a
             * plain drive path) */
            if (final_path.rfind(L"\\\\?\\", 0) == 0)
                final_path.erase(0, 4);
        }
        CloseHandle(h);
    }
    /* Fall back (destination folder may not exist yet): GetVolumePathNameW
     * on the path itself, no reparse resolution possible. */
    wchar_t root[MAX_PATH] = {};
    if (!GetVolumePathNameW(final_path.c_str(), root, MAX_PATH) || root[0] == L'\0')
        return false;
    if (root[0] == L'\\') /* mapped network root slipped past the probe */
        return false;
    *letter = root[0];
    return true;
}

} // namespace win
} // namespace cr
