/* cr_enumerate.cpp - disk/volume enumeration.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_bitlocker.h"
#include "cr/win/cr_transport.h"
#include "cr/table/cr_table_parse.h"
#include "cr/storage/cr_slice.h"
#include <windows.h>
#include <winioctl.h>
#include <cstdio>

namespace cr {
namespace win {

namespace {

bool query_disk(u32 n, PhysicalDisk& d) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", n);
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    bool ok = false;
    /* geometry */
    DISK_GEOMETRY_EX dg = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &dg, sizeof(dg), &br,
                        nullptr)) {
        d.logical_sector = dg.Geometry.BytesPerSector;
        d.physical_sector = dg.Geometry.BytesPerSector;
        d.size_bytes = u64(dg.DiskSize.QuadPart);
        ok = true;
    }
    /* storage access alignment for 4Kn/512e */
    {
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageAccessAlignmentProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                            nullptr)) {
            d.logical_sector = u32_from_bytes_le(buf + 8);
            d.physical_sector = u32_from_bytes_le(buf + 12);
        }
    }
    /* device descriptor for model/serial/removable */
    {
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageDeviceProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                            nullptr)) {
            /* STORAGE_DEVICE_DESCRIPTOR: offsets relative to buf start */
            const u8* base = buf;
            const DWORD prod_off = u32_from_bytes_le(base + 24);
            const DWORD serial_off = u32_from_bytes_le(base + 36);
            const DWORD bus = u32_from_bytes_le(base + 28);
            if (prod_off && prod_off < sizeof(buf)) {
                const char* p = reinterpret_cast<const char*>(base + prod_off);
                /* bounded scan (audit row 44): a malformed descriptor
                 * must not walk past the 4096-byte storage query buffer */
                const char* mlim = reinterpret_cast<const char*>(base) + sizeof(buf);
                while (p < mlim && *p)
                    d.model.push_back(wchar_t(*p++));
            }
            if (serial_off && serial_off < sizeof(buf)) {
                const char* p = reinterpret_cast<const char*>(base + serial_off);
                const char* slim = reinterpret_cast<const char*>(base) + sizeof(buf);
                while (p < slim && *p)
                    d.serial.push_back(wchar_t(*p++));
            }
            d.removable = (bus == BusTypeUsb || bus == BusTypeSd || bus == BusTypeMmc);
        }
    }
    CloseHandle(h);
    if (!ok)
        return false;
    d.number = n;
    return true;
}

} // namespace

u32 enumerate_disks(u32 max_disks, std::vector<PhysicalDisk>& out) {
    for (u32 n = 0; n < max_disks; ++n) {
        PhysicalDisk d;
        if (!query_disk(n, d))
            continue;
        /* parse the on-disk table through the pure core */
        u32 err = 0;
        auto view = open_physical_drive(n, &err);
        if (view) {
            d.table_ok = table::parse_disk_table(view.get(), d.table) == OK;
        }
        out.push_back(std::move(d));
    }
    return OK;
}

u32 enumerate_volumes(std::vector<VolumeEntry>& out) {
    wchar_t letters[64];
    DWORD len = GetLogicalDriveStringsW(64, letters);
    if (len == 0 || len > 64)
        return ERR_BAD_PARAMETER;
    const wchar_t* p = letters;
    while (*p) {
        VolumeEntry v;
        v.letter = p[0];
        v.letter_path = std::wstring(p); /* "C:\" */
        const UINT type = GetDriveTypeW(p);
        if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
            wchar_t vol[64];
            _snwprintf_s(vol, 64, _TRUNCATE, L"\\\\.\\%c:", p[0]);
            HANDLE h = CreateFileW(vol, GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                VOLUME_DISK_EXTENTS ext[8] = {};
                DWORD br = 0;
                if (DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, ext,
                                    sizeof(ext), &br, nullptr) &&
                    ext->NumberOfDiskExtents >= 1) {
                    v.disk = ext->Extents[0].DiskNumber;
                    v.offset_on_disk = u64(ext->Extents[0].StartingOffset.QuadPart);
                    v.bytes = u64(ext->Extents[0].ExtentLength.QuadPart);
                }
                CloseHandle(h);
            }
            wchar_t fs[32] = {};
            wchar_t label[64] = {};
            GetVolumeInformationW(p, label, 64, nullptr, nullptr, nullptr, fs, 32);
            v.fs_name = fs;
            v.label = label;
            v.bitlocker = bitlocker::is_fve_volume(p[0]);
            out.push_back(std::move(v));
        }
        p += wcslen(p) + 1;
    }
    return OK;
}

std::unique_ptr<IStorageDevice> open_disk_view(const PhysicalDisk& d, const wchar_t* rda_service,
                                               u32* err) {
    u32 e = 0;
    if (rda_service) {
        if (auto dev = open_rda_partition(rda_service, d.number, 0, &e))
            return dev;
    }
    return open_physical_drive(d.number, err);
}

} // namespace win
} // namespace cr
