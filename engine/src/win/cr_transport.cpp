/* cr_transport.cpp - Windows transports: RDA grammar path + direct handles.
 *
 * RDA wiring (rda-driver v0.2.0 contract):
 *   - legacy PID-token grammar opens (EPMNtDrv / EUEDKEPM wire compat) -
 *     the SOLE production surface (N-041: the structured-open RdaClient
 *     had zero callers since introduction and was deleted; the wire
 *     structs stay documented in cr_rda.h)
 *   - F11 unscale: ReadFile results through RDA handles are divided by
 *     512 (the driver reports Information = Length * 512)
 *   - reference duality (corpus Device.mo): driver path for locked/damaged
 *     volumes + direct \\.\PhysicalDriveN / HarddiskVolumeN fallbacks
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_transport.h"
#include "cr/win/cr_rda.h"
#include "cr/win/cr_enumerate.h" /* S-010: volume enumeration for the lock sequence */
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
            return ERR_READ_FAILED;
        }
        if (!GetOverlappedResult(h, &ov, &got, TRUE)) {
            if (gle_out)
                *gle_out = GetLastError();
            return ERR_READ_FAILED;
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

} // namespace

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

    /* S-010 volume quiesce (corpus RecoveryModule_decompiled.c:44686-44720
     * and its PartitionRecovery mirror): per MOUNTED volume on this disk -
     *   FSCTL_LOCK_VOLUME (0x90018)   :44686 - a busy volume = refuse
     *   DeleteVolumeMountPointA        :44699 - the OS must not re-open it
     *   FlushFileBuffers               :44703
     *   Sleep(2000) settle             :44707
     *   FSCTL_IS_VOLUME_MOUNTED (0x90020) :44708 - verify the lock took
     *   FSCTL_DISMOUNT_VOLUME (0x9001c)  :44713
     * The volume handles are held for the device lifetime (the lock is
     * released at close). Unmounted volumes (no letter) have nothing to
     * quiesce. A lock that cannot be acquired = ERR_VOLUME_LOCK_FAILED:
     * execute() refuses the whole repair before the first write - the
     * concurrent user keeps the volume, never a raced write. */
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
            /* the letter must go so the OS does not hand the volume back
             * out (corpus :44699); unmount + settle + verify + dismount */
            wchar_t mount[8] = {v.letter, L':', L'\\', 0};
            DeleteVolumeMountPointW(mount);
            FlushFileBuffers(vh);
            Sleep(2000); /* corpus :44707 settle */
            DeviceIoControl(vh, FSCTL_IS_VOLUME_MOUNTED, nullptr, 0, nullptr, 0, &bytes_ret,
                            nullptr); /* corpus :44708 verify */
            DeviceIoControl(vh, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0, &bytes_ret,
                            nullptr); /* corpus :44713 */
            locked_volumes_.push_back(vh);
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

    /* close every held volume handle - that is what releases the locks */
    void release_locked_volumes_() {
        for (HANDLE vh : locked_volumes_)
            if (vh != INVALID_HANDLE_VALUE)
                CloseHandle(vh);
        locked_volumes_.clear();
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
                return std::unique_ptr<cr::repair::IWriteDevice>(
                    new RwDevice(h, g, true, disk, std::wstring(path), GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE));
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
    return std::unique_ptr<cr::repair::IWriteDevice>(
        new RwDevice(h, g, false, disk, std::wstring(path), GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE));
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


} // namespace win
} // namespace cr

/* ---------------------------------------------------------------------------
 * N-013: destination-side overlap arming, full-extent edition.
 * ------------------------------------------------------------------------- */
bool volume_extents(wchar_t letter, std::vector<VolExtent>* out) {
    if (!out)
        return false;
    out->clear();
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h =
        CreateFileW(vol, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    BOOL io = DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext,
                              sizeof(ext), &br, nullptr);
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
    HANDLE h = CreateFileW(path.c_str(), 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, nullptr);
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
