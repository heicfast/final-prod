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
            /* N-080 (fixed with N-066): the SDK-exact offset decode lives
             * in the pure header function — product@+16, revision@+20 (the
             * firmware axis), serial@+24, bus@+28. The pre-fix code read
             * the product at +24 (the SERIAL offset) and the serial at
             * +36 (inside RawDeviceProperties). */
            decode_device_descriptor(buf, sizeof(buf), d);
            /* N-066: the interface axis from the bus type (the og's
             * +0x414 "USB Device" family) + the WWN/serial-2 axis from
             * the device-id descriptor (the first DEVICE-associated
             * identifier, ascii or hex). */
            const u32 bus = u32_from_bytes_le(buf + 28);
            if (const wchar_t* name = bus_interface_name(bus))
                if (*name)
                    d.interface_desc = name;
        }
        STORAGE_PROPERTY_QUERY qid = {};
        qid.PropertyId = StorageDeviceIdProperty;
        qid.QueryType = PropertyStandardQuery;
        u8 idbuf[1024];
        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &qid, sizeof(qid), idbuf,
                            sizeof(idbuf), &br, nullptr))
            decode_device_id_descriptor(idbuf, sizeof(idbuf), d.wwn);
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
 * device number) still answer. D3 (the debug-and-release session): a
 * PRESENT device that cannot even be metadata-opened no longer vanishes -
 * it is listed with honest empties (see query_disk below; the reference
 * lists devices without opening them at all). */
HANDLE open_disk_for_query(const wchar_t* path, bool& busy, DWORD* last_error) {
    HANDLE h =
        CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const DWORD le = GetLastError();
        if (le != ERROR_ACCESS_DENIED && le != ERROR_SHARING_VIOLATION) {
            if (last_error)
                *last_error = le;
            return INVALID_HANDLE_VALUE;
        }
        h = CreateFileW(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            if (last_error)
                *last_error = GetLastError();
            return INVALID_HANDLE_VALUE;
        }
        busy = true; /* listed, but data scans must re-open later */
    }
    return h;
}

/* D3 (the debug-and-release session): a PRESENT device the OS refuses to
 * open must NOT vanish from the device list. The pre-fix code dropped any
 * disk whose open failed with anything other than ACCESS_DENIED/
 * SHARING_VIOLATION (or whose geometry query failed) — the user's
 * "only one disk of three appears" on real hardware. The reference lists
 * devices WITHOUT opening them (its five-stack fan-in reads WMI/SetupAPI
 * metadata; mom-module-system--architecture.md :57-59, Device.md :45-50):
 * an unopenable-but-present device is listed there with an honest
 * unavailable state. The numbered namespace distinguishes ABSENT from
 * PRESENT-but-unopenable through the open error itself: FILE_NOT_FOUND /
 * PATH_NOT_FOUND / INVALID_NAME are the namespace's "no such drive" gaps
 * (the loop's normal probe misses); every other failure (ERROR_NOT_READY,
 * ERROR_IO_DEVICE, ERROR_GEN_FAILURE, exhausted access-denied retries)
 * means the device object EXISTS — keep it listed, geometry unknown,
 * busy-marked, and let the scan attempt surface the real open error. */
bool query_disk(u32 n, PhysicalDisk& d) {
    wchar_t path[64];
    _snwprintf_s(path, 64, _TRUNCATE, L"\\\\.\\PhysicalDrive%u", n);
    DWORD le = 0;
    HANDLE h = open_disk_for_query(path, d.busy, &le);
    if (h == INVALID_HANDLE_VALUE) {
        if (le == ERROR_FILE_NOT_FOUND || le == ERROR_PATH_NOT_FOUND || le == ERROR_INVALID_NAME)
            return false; /* the namespace gap: no such drive (normal) */
        /* PRESENT but unopenable: listed with honest empties — the device
         * row renders and the scan attempt reports the true failure */
        d.number = n;
        d.busy = true; /* data opens are known-bad; re-open surfaces the rc */
        return true;
    }
    const bool ok = query_disk_handle(h, d);
    CloseHandle(h);
    if (!ok) {
        /* opened, but no geometry answerable (some stacks refuse geometry
         * on not-ready media): still PRESENT — list with unknown geometry */
        d.number = n;
        d.busy = true;
        return true;
    }
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
 * query_disk_handle path. D3: a PRESENT interface that does not open (or
 * answers no geometry) is still listed - honest empties, busy-marked;
 * the merge dedups it against the numbered loop's row when the number
 * is known, or by instance path when it is not. */
u32 enumerate_disk_interfaces(PhysicalDiskList& out) {
    ULONG need = 0;
    CONFIGRET cr =
        CM_Get_Device_Interface_List_SizeW(&need, const_cast<GUID*>(&kGuidDevinterfaceDisk),
                                           nullptr, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS || need == 0)
        return OK; /* no disk interfaces: the numbered loop stands alone */
    std::wstring buf(size_t(need), L'\0');
    cr = CM_Get_Device_Interface_ListW(const_cast<GUID*>(&kGuidDevinterfaceDisk), nullptr,
                                       buf.data(), need, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS)
        return OK; /* the interface stack failed: degrade to stack 1 */
    const wchar_t* p = buf.c_str();
    while (*p) {
        const size_t len = wcslen(p);
        PhysicalDisk d;
        d.interface_path = std::wstring(p, len);
        DWORD open_le = 0;
        HANDLE h = open_disk_for_query(p, d.busy, &open_le);
        if (h == INVALID_HANDLE_VALUE) {
            /* D3: the interface is PRESENT (cfgmgr listed it) - list the
             * device even though nothing about it can be queried; the
             * instance path is the identity the merge dedups by. */
            d.number = kUnknownDiskNumber;
            d.table_ok = false;
            d.busy = true;
            out.push_back(std::move(d));
        } else {
            STORAGE_DEVICE_NUMBER num = {};
            DWORD br = 0;
            if (DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &num, sizeof(num),
                                &br, nullptr) &&
                br >= sizeof(num))
                d.number = num.DeviceNumber;
            else
                d.number = kUnknownDiskNumber;
            if (!query_disk_handle(h, d)) {
                /* opened but no geometry: still PRESENT (D3) */
                d.table_ok = false;
                d.busy = true;
                out.push_back(std::move(d));
            } else {
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
    /* E-01 (the debug-and-release session): the fixed 64-wchar buffer
     * failed WHOLESALE at 16+ letters (each drive string is 4 wchars + the
     * list's terminating NUL — GetLogicalDriveStringsW returns the needed
     * size on overflow, the >64 check refused, and EVERY volume vanished:
     * a machine with many mounts — dev VHDs, containers, card readers —
     * lost its volume rows entirely). The SDK's own shape: a generous
     * stack buffer, and on overflow the returned requirement drives an
     * exact-size retry. */
    wchar_t stack_buf[512]; /* 128+ letters on the stack; the heap path covers the absurd */
    DWORD cap = 512;
    DWORD len = GetLogicalDriveStringsW(cap, stack_buf);
    std::wstring heap;
    if (len > cap) {
        /* overflow: len IS the required size (final null included) */
        heap.resize(size_t(len), L'\0');
        cap = len;
        len = GetLogicalDriveStringsW(cap, heap.data());
        if (len == 0 || len > cap)
            return ERR_BAD_PARAMETER;
    } else if (len == 0) {
        return ERR_BAD_PARAMETER;
    }
    const wchar_t* p = heap.empty() ? stack_buf : heap.c_str();
    if (heap.empty() && len > 0 && stack_buf[0] == L'\0')
        return OK; /* zero drives: the honest empty */
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
