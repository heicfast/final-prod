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
};

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

/* Enumerate logical volumes (letters) with disk correlation. */
u32 enumerate_volumes(VolumeEntryList& out);

/* Whole-disk view bound to enumeration (RDA-preferred when available). */
std::unique_ptr<IStorageDevice> open_disk_view(const PhysicalDisk& d, const wchar_t* rda_service,
                                               u32* err);

} // namespace win
} // namespace cr

#endif // CR_ENUMERATE_H_
