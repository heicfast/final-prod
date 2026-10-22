/* cr_transport.cpp - Windows transports (RDA client + direct handles).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_transport.h"
#include "cr/ntfs/cr_ntfs_wire.h" /* RdaComputePidToken-compatible impl below */
#include <windows.h>
#include <winioctl.h>
#include <cstdio>

namespace cr {
namespace win {

namespace {

/* PID token (rda-driver contract, both sides verified):
 *   b0 | (b0+b1)<<8 | (b1+b2)<<16 | (b2+b3)<<24 over the CALLING PID. */
u32 pid_token() {
    const u32 pid = GetCurrentProcessId();
    const u8 b0 = u8(pid), b1 = u8(pid >> 8), b2 = u8(pid >> 16), b3 = u8(pid >> 24);
    const auto add = [](unsigned a, unsigned b) -> unsigned { return (a + b) & 0xFF; };
    return u32(b0) | (u32(add(b0, b1)) << 8) | (u32(add(b1, b2)) << 16) | (u32(add(b2, b3)) << 24);
}

class HandleDevice final : public IStorageDevice {
public:
    HandleDevice(HANDLE h, Geometry g) : h_(h), geom_(std::move(g)) {}
    ~HandleDevice() override {
        if (h_ != INVALID_HANDLE_VALUE) CloseHandle(h_);
    }
    u32 read_at(u64 off, void *buf, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE) return ERR_OPEN_FAILED;
        if (off > size() || len > size() - off) return ERR_RANGE_OUT_OF_BOUNDS;
        /* overlapped reads at arbitrary offsets */
        OVERLAPPED ov = {};
        ov.Offset = u32(off);
        ov.OffsetHigh = u32(off >> 32);
        /* the RDA driver contract reports Information = len*512; raw Win32
         * handles report true byte counts - normalize by probing once:
         * simplest correct path: use ReadFile and trust the returned count */
        DWORD got = 0;
        if (!ReadFile(h_, buf, len, &got, &ov)) {
            const DWORD e = GetLastError();
            if (e == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(h_, &ov, &got, TRUE)) return ERR_READ_FAILED;
            } else {
                return ERR_READ_FAILED;
            }
        }
        return got == len ? OK : ERR_READ_FAILED;
    }
    const Geometry &geometry() const override { return geom_; }
    u64 size() const { return geom_.total_bytes; }

private:
    HANDLE h_;
    Geometry geom_;
};

Geometry probe_geometry(HANDLE h, u64 *out_size) {
    Geometry g{};
    DISK_GEOMETRY_EX dg = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &dg, sizeof(dg),
                        &br, nullptr)) {
        g.logical_sector = dg.Geometry.BytesPerSector;
        g.physical_sector = dg.Geometry.BytesPerSector;
        /* corrected via the alignment query below (512e/4Kn) */
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageAccessAlignmentProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf),
                            &br, nullptr)) {
            /* STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR: BytesPerLogicalSector @8
             * (u32), BytesPerPhysicalSector @12 (u32) */
            const u8 *d = buf;
            g.logical_sector = u32_from_bytes_le(d + 8);
            g.physical_sector = u32_from_bytes_le(d + 12);
        } else {
            g.physical_sector = g.logical_sector;
        }
        g.sector_count = dg.DiskSize.QuadPart / (g.logical_sector ? g.logical_sector : 512);
        g.total_bytes = dg.DiskSize.QuadPart;
    }
    if (out_size) *out_size = g.total_bytes;
    return g;
}

std::unique_ptr<IStorageDevice> open_path(const wchar_t *path, u32 *err) {
    HANDLE h = CreateFileW(path, GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (err) *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    Geometry g = probe_geometry(h, nullptr);
    if (g.logical_sector == 0) {
        /* not a disk device (e.g. volume handle): derive from size */
        LARGE_INTEGER sz = {};
        GetFileSizeEx(h, &sz);
        g.logical_sector = 512;
        g.physical_sector = 512;
        g.total_bytes = u64(sz.QuadPart);
        g.sector_count = g.total_bytes / 512;
    }
    if (err) *err = OK;
    return std::unique_ptr<IStorageDevice>(new HandleDevice(h, g));
}

std::unique_ptr<IStorageDevice> open_rda_path(const wchar_t *service, const wchar_t *suffix,
                                              u32 *err) {
    wchar_t path[256];
    const u32 tok = pid_token();
    _snwprintf_s(path, 256, _TRUNCATE, L"\\\\.\\%s\\%u%s", service, tok, suffix);
    return open_path(path, err);
}

} // namespace

std::unique_ptr<IStorageDevice> open_rda_partition(const wchar_t *service_name, u32 disk,
                                                   u32 part, u32 *err) {
    wchar_t suffix[64];
    _snwprintf_s(suffix, 64, _TRUNCATE, L"\\%u\\%u", disk, part);
    return open_rda_path(service_name, suffix, err);
}

std::unique_ptr<IStorageDevice> open_rda_volume(const wchar_t *service_name, u32 volume,
                                                u32 *err) {
    wchar_t suffix[64];
    _snwprintf_s(suffix, 64, _TRUNCATE, L"\\Volume\\%u", volume);
    return open_rda_path(service_name, suffix, err);
}

std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32 *err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", disk);
    return open_path(path, err);
}

std::unique_ptr<IStorageDevice> open_harddisk_volume(u32 volume, u32 *err) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\HarddiskVolume%u", volume);
    return open_path(path, err);
}

bool rda_available(const wchar_t *service_name) {
    u32 e = 0;
    wchar_t suffix[16];
    _snwprintf_s(suffix, 16, _TRUNCATE, L"\\%u", 0); /* whole-disk form: \\.\SVC\tok\0 */
    /* probing disk 0 via the 2-segment grammar */
    auto dev = open_rda_partition(service_name, 0, 0, &e);
    return dev != nullptr;
}

std::unique_ptr<IStorageDevice> open_disk(u32 disk, bool prefer_rda, const wchar_t *service,
                                          u32 *err) {
    if (prefer_rda) {
        /* RDA first (locked volumes), direct fallback (reference duality) */
        u32 e = 0;
        if (auto dev = open_rda_partition(service, disk, 0, &e)) return dev;
    }
    return open_physical_drive(disk, err);
}

bool volume_letter_to_disk(wchar_t letter, u32 *disk, u32 *part) {
    wchar_t vol[8];
    _snwprintf_s(vol, 8, _TRUNCATE, L"\\\\.\\%c:", letter);
    HANDLE h = CreateFileW(vol, GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool ok = false;
    VOLUME_DISK_EXTENTS ext[8] = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext,
                        sizeof(ext), &br, nullptr) &&
        ext->NumberOfDiskExtents >= 1) {
        if (disk) *disk = ext->Extents[0].DiskNumber;
        if (part) *part = u32(ext->Extents[0].StartingOffset.QuadPart >> 20); /* approx;
            corrected by callers via parse_disk_table when exact numbers matter */
        ok = true;
    }
    CloseHandle(h);
    return ok;
}

} // namespace win
} // namespace cr
