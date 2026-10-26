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
#include <cfgmgr32.h>
#include <cstdio>

namespace cr {
namespace win {

namespace {

constexpr GUID kGuidDevinterfaceDisk = {
    0x53f56307, 0xb6bf, 0x11d0, {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

/* Fill one PhysicalDisk from an OPENED disk handle (the shared query
 * path of both stacks - the numbered loop's query_disk factored so the
 * interface pass reuses it verbatim). Returns false when no geometry
 * could be obtained (the og's enumerator skips such devices too). */
bool query_disk_handle(HANDLE h, PhysicalDisk& d) {
    bool ok = false;
    DISK_GEOMETRY_EX dg = {};
    DWORD br = 0;
    if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, &dg, sizeof(dg), &br,
                        nullptr)) {
        d.logical_sector = dg.Geometry.BytesPerSector;
        d.physical_sector = dg.Geometry.BytesPerSector;
        d.size_bytes = u64(dg.DiskSize.QuadPart);
        ok = true;
    }
    {
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageAccessAlignmentProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                            nullptr)) {
            const u32 align_logical = u32_from_bytes_le(buf + 0x10);
            const u32 align_physical = u32_from_bytes_le(buf + 0x14);
            if (align_logical >= 256 && align_logical <= 65536)
                d.logical_sector = align_logical;
            if (align_physical >= 256 && align_physical <= 65536)
                d.physical_sector = align_physical;
        }
    }
    {
        STORAGE_PROPERTY_QUERY q = {};
        q.PropertyId = StorageDeviceProperty;
        q.QueryType = PropertyStandardQuery;
        u8 buf[4096];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &q, sizeof(q), buf, sizeof(buf), &br,
                            nullptr)) {
            const u8* base = buf;
            const DWORD prod_off = u32_from_bytes_le(base + 24);
            const DWORD serial_off = u32_from_bytes_le(base + 36);
            const DWORD bus = u32_from_bytes_le(base + 28);
            if (prod_off && prod_off < sizeof(buf)) {
                const char* p = reinterpret_cast<const char*>(base + prod_off);
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
    return ok;
}


/* N-071 hardening (convergent with the two-stack port): a disk held by
 * another process (backup/AV/imaging - share modes that exclude our
 * data-open request) must not vanish from the device list. The og's
 * stacks (WMI/SetupAPI - mom-module-system--architecture.md :57-59)
 * list disks WITHOUT opening them; both of our stacks open, so on
 * ERROR_ACCESS_DENIED / ERROR_SHARING_VIOLATION retry a 0-access
 * metadata-only open: no read/write share conflict, and the
 * FILE_ANY_ACCESS IOCTLs used here (geometry-EX, storage query,
 * device number) still answer. A disk that cannot even be
 * metadata-opened stays absent (the honest limit - PARITY-DELTAS
 * D-80's revisit trigger). */
HANDLE open_disk_for_query(const wchar_t* path, bool& busy) {
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD le = GetLastError();
        if (le != ERROR_ACCESS_DENIED && le != ERROR_SHARING_VIOLATION)
            return INVALID_HANDLE_VALUE;
        h = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;
        busy = true; /* listed, but data scans must re-open later */
    }
    return h;
}

bool query_disk(u32 n, PhysicalDisk& d) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", n);
    HANDLE h = open_disk_for_query(path, d.busy);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    const bool ok = query_disk_handle(h, d);
    CloseHandle(h);
    if (!ok)
        return false;
    d.number = n;
    return true;
}

} // namespace

/* N-071 (diff-to-fix): the cfgmgr32 disk-interface pass - the second
 * enumeration stack. GUID_DEVINTERFACE_DISK's documented value
 * (winioctl.h; defined locally so no INITGUID linkage games):
 * {53f56307-b6bf-11d0-94f2-00a0c91efb8b}. Every PRESENT interface
 * instance is opened and queried with the same IOCTL family as the
 * numbered loop (number + geometry + alignment + descriptor); entries
 * the numbered loop already surfaced are dropped by the pure merge. */

u32 enumerate_disks(u32 max_disks, std::vector<PhysicalDisk>& out) {
    /* Stack 1: the numbered PhysicalDrive namespace (unchanged behavior). */
    PhysicalDiskList loop_disks;
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
        loop_disks.push_back(std::move(d));
    }
    /* Stack 2 (N-071): the cfgmgr32 disk-interface pass - devices the
     * numbered loop missed but that expose a present disk interface. */
    PhysicalDiskList interface_disks;
    enumerate_disk_interfaces(interface_disks);
    return merge_disk_enumeration(loop_disks, interface_disks, out);
}

/* The acquisition seam: the cfgmgr32 GUID_DEVINTERFACE_DISK walk. Every
 * present interface instance is opened with the same share mode as the
 * numbered loop; the number comes from IOCTL_STORAGE_GET_DEVICE_NUMBER
 * (kUnknownDiskNumber when the query fails - such entries dedup by
 * instance path in the merge). Geometry/descriptor ride the shared
 * query_disk_handle path. Entries that do not open at all are skipped
 * (matching the loop's CreateFileW-failure behavior). */
u32 enumerate_disk_interfaces(PhysicalDiskList& out) {
    ULONG need = 0;
    CONFIGRET cr = CM_Get_Device_Interface_List_SizeW(
        &need, const_cast<GUID*>(&kGuidDevinterfaceDisk), nullptr,
        CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS || need == 0)
        return OK; /* no disk interfaces: the numbered loop stands alone */
    std::wstring buf(size_t(need), L'\0');
    cr = CM_Get_Device_Interface_ListW(const_cast<GUID*>(&kGuidDevinterfaceDisk), nullptr,
                                       buf.data(), need,
                                       CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS)
        return OK; /* the interface stack failed: degrade to stack 1 */
    const wchar_t* p = buf.c_str();
    while (*p) {
        const size_t len = wcslen(p);
        PhysicalDisk d;
        d.interface_path = std::wstring(p, len);
        HANDLE h = open_disk_for_query(p, d.busy);
        if (h != INVALID_HANDLE_VALUE) {
            STORAGE_DEVICE_NUMBER num = {};
            DWORD br = 0;
            if (DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &num, sizeof(num),
                                &br, nullptr) &&
                br >= sizeof(num))
                d.number = num.DeviceNumber;
            else
                d.number = kUnknownDiskNumber;
            if (query_disk_handle(h, d)) {
                if (d.number == kUnknownDiskNumber) {
                    /* no table parse possible without a drive number */
                    d.table_ok = false;
                } else {
                    u32 err = 0;
                    auto view = open_physical_drive(d.number, &err);
                    if (view)
                        d.table_ok = table::parse_disk_table(view.get(), d.table) == OK;
                }
                out.push_back(std::move(d));
            }
            CloseHandle(h);
        }
        p += len + 1;
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

} // namespace win
} // namespace cr
