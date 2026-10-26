/*
 * cr_enumerate.h - disk/volume enumeration (Windows).
 *
 * Corpus roles (Device.mo / DeviceManager.mo): physical + removable
 * enumeration, volume identification, partition tables via the pure-core
 * parse_disk_table, R6 filter data.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ENUMERATE_H_
#define CR_ENUMERATE_H_

#include "cr/storage/cr_device.h"
#include "cr/table/cr_table_parse.h"
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace win {

struct PhysicalDisk {
    u32 number = 0;
    std::wstring model; /* ST500DM0... */
    std::wstring serial;
    u64 size_bytes = 0;
    u32 logical_sector = 0;
    u32 physical_sector = 0;
    bool removable = false; /* BusType USB/SD */
    table::DiskTable table; /* parsed layout */
    bool table_ok = false;
    /* N-071: the PnP disk-interface instance path when this entry came
     * from the SetupAPI/cfgmgr pass (empty for numbered-loop entries that
     * the interface pass did not re-report). The dedup key for the merge. */
    std::wstring interface_path;
};

/* N-071: disks the numbered loop could not open carry this number. */
constexpr u32 kUnknownDiskNumber = 0xFFFFFFFFu;

struct VolumeEntry {
    std::wstring letter_path; /* \\.\C: */
    wchar_t letter = 0;
    u32 disk = 0;
    u64 offset_on_disk = 0;
    u64 bytes = 0;
    std::wstring fs_name; /* NTFS/FAT32/... */
    std::wstring label;
    bool bitlocker = false; /* FVE detected (R8) */
};

using PhysicalDiskList = std::vector<PhysicalDisk>;
using VolumeEntryList = std::vector<VolumeEntry>;

/* Enumerate physical disks (0..max_disks-1) with geometry + tables. */
u32 enumerate_disks(u32 max_disks, PhysicalDiskList& out);

/* N-071: the cfgmgr32 disk-interface acquisition (the second stack's
 * seam - GUID_DEVINTERFACE_DISK present instances opened and queried;
 * merged into enumerate_disks' output by merge_disk_enumeration). Every
 * returned entry carries interface_path; number is the
 * IOCTL_STORAGE_GET_DEVICE_NUMBER value or kUnknownDiskNumber. Degrades
 * to an empty list on any cfgmgr failure (stack 1 stands alone). */
u32 enumerate_disk_interfaces(PhysicalDiskList& out);

/* N-071 (diff-to-fix): the two-stack enumeration merge.
 *
 * Corpus: the og's CDiskEnumerator merges the PhysicalDrive namespace
 * with the PnP interface stacks (DeviceManager.mo's WMI/SetupAPI/
 * FunctionDiscovery fan-in; the QA boot census shows storage-spaces init
 * + VDS service load alongside the plain disk enumerate - vendor-logs
 * :13-29). Our second stack is the cfgmgr32 disk-interface pass
 * (GUID_DEVINTERFACE_DISK symbolic links, Win7-inclusive): devices the
 * numbered loop could not open but that expose a disk interface are
 * surfaced through it.
 *
 * The merge is PURE (header-inline, Linux-lane testable):
 *   - every numbered-loop disk is kept (its entry is richer: parsed
 *     on-disk table);
 *   - an interface entry is appended when its disk number is absent from
 *     the loop set AND not already appended (dedup by number) and its
 *     interface path was not seen before (dedup by instance path);
 *   - entries with kUnknownDiskNumber dedup by path only.
 * Order: loop disks in loop order, then interface-only disks in pass
 * order. Returns OK; out is fully replaced. */
inline u32 merge_disk_enumeration(const PhysicalDiskList& loop_disks,
                                  const PhysicalDiskList& interface_disks,
                                  PhysicalDiskList& out) {
    out = loop_disks;
    for (const PhysicalDisk& iface : interface_disks) {
        if (iface.number != kUnknownDiskNumber) {
            bool have = false;
            for (const PhysicalDisk& d : out)
                if (d.number == iface.number) {
                    have = true;
                    break;
                }
            if (have)
                continue;
        }
        if (!iface.interface_path.empty()) {
            bool seen = false;
            for (const PhysicalDisk& d : out)
                if (d.interface_path == iface.interface_path) {
                    seen = true;
                    break;
                }
            if (seen)
                continue;
        }
        out.push_back(iface);
    }
    return OK;
}

/* Enumerate logical volumes (letters) with disk correlation. */
u32 enumerate_volumes(VolumeEntryList& out);

/* W4 (AUDIT-ARCH 8a #5): the win-layer open_disk_view was deleted - the
 * CLI (rrecov) opens with its own local view helper with different
 * semantics; nothing else called this one. */

} // namespace win
} // namespace cr

#endif // CR_ENUMERATE_H_
