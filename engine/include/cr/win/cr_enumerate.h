/*
 * cr_enumerate.h - disk/volume enumeration (Windows).
 *
 * Corpus roles (Device.mo / DeviceManager.mo): physical + removable
 * enumeration, volume identification, partition tables via the pure-core
 * parse_disk_table, R6 filter data.
 *
 * SCOPE (N-071, registered PARITY-DELTAS D-80): the engine's device
 * truth is the \\.\PhysicalDriveN namespace + the volume-letter
 * companion path. The og's five-stack discovery fan-in (WMI, SetupAPI,
 * VDS, Storage Spaces, Function Discovery - mom-module-system--
 * architecture.md :57-59) is MOM device-manager/UI machinery; its
 * load-bearing effect for a recovery engine - a busy disk must stay
 * listed - is covered by the metadata-only enumeration retry in
 * query_disk (0-access open when the data open is share-blocked).
 * Devices that do not surface as PhysicalDriveN at all (none observed
 * on any CI/host in this project's fleet; the Storage Spaces claim in
 * the corpus is a design description, not an observed miss) remain out
 * of scope until a live miss-case is recorded - the D-80 revisit
 * trigger.
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
    bool busy = false;      /* N-071: metadata-only open (share-blocked data
                               open) - listed, scans re-open later */
    table::DiskTable table; /* parsed layout */
    bool table_ok = false;
    /* N-071: the PnP disk-interface instance path when this entry came
     * from the SetupAPI/cfgmgr pass (empty for numbered-loop entries that
     * the interface pass did not re-report). The dedup key for the merge. */
    std::wstring interface_path;
    /* N-066: the og record-3 identity axes — firmware revision (the
     * descriptor's ProductRevision string), WWN/serial-2 (the
     * StorageDeviceIdProperty device identifier, ascii or hex), and the
     * interface description (the bus-type text, "USB Device" family). */
    std::wstring firmware;
    std::wstring wwn;
    std::wstring interface_desc;
};

/* N-080 (NEW-FINDING, fixed with N-066): decode a STORAGE_DEVICE_DESCRIPTOR
 * buffer with the SDK-exact offsets — VendorIdOffset@+12, ProductIdOffset@+16,
 * ProductRevisionOffset@+20, SerialNumberOffset@+24, BusType@+28 (the
 * pre-fix code read the product at +24 — the SERIAL offset — and the serial
 * at +36, inside RawDeviceProperties). PURE (Linux-lane testable): fills
 * model/serial/firmware/removable from the buffer's packed strings, each
 * bounded by `len`. Returns false when the buffer cannot hold the fixed
 * header (36 bytes). */
inline bool decode_device_descriptor(const u8* buf, size_t len, PhysicalDisk& d) {
    if (buf == nullptr || len < 36)
        return false;
    auto u32at = [&](size_t off) -> u32 {
        return u32(buf[off]) | (u32(buf[off + 1]) << 8) | (u32(buf[off + 2]) << 16) |
               (u32(buf[off + 3]) << 24);
    };
    auto put_str = [&](std::wstring& out, u32 off) {
        out.clear();
        if (off == 0 || off >= len)
            return; /* offset 0 = field absent (SDK semantics) */
        size_t i = off;
        while (i < len && buf[i] != 0 && out.size() < 128)
            out.push_back(wchar_t(buf[i++]));
    };
    put_str(d.model, u32at(16));    /* ProductIdOffset */
    put_str(d.firmware, u32at(20)); /* ProductRevisionOffset */
    put_str(d.serial, u32at(24));   /* SerialNumberOffset */
    const u32 bus = u32at(28);      /* BusType */
    d.removable = (bus == 7 /*Usb*/ || bus == 11 /*Sd*/ || bus == 12 /*Mmc*/);
    return true;
}

/* N-066: the bus-type text for the interface axis (the og record-3's
 * +0x414 "USB Device" family — census-level strings, the standard bus
 * names). PURE. */
inline const wchar_t* bus_interface_name(u32 bus) {
    switch (bus) {
    case 7:
        return L"USB Device";
    case 0:
        return L"Unknown";
    case 1:
        return L"SCSI";
    case 2:
        return L"ATAPI";
    case 3:
        return L"ATA";
    case 4:
        return L"1394";
    case 6:
        return L"Fibre Channel";
    case 8:
        return L"RAID";
    case 10:
        return L"SAS";
    case 11:
        return L"SD";
    case 12:
        return L"MMC";
    case 13:
        return L"Virtual";
    case 14:
        return L"File-Backed Virtual";
    case 15:
        return L"Storage Spaces";
    case 17:
        return L"NVMe";
    default:
        return L"";
    }
}

/* N-066: decode a STORAGE_DEVICE_ID_DESCRIPTOR buffer into the WWN/serial-2
 * axis — the FIRST device-associated identifier (Association==0), rendered
 * ascii verbatim or binary as hex, bounded to 31 chars. PURE (Linux-lane
 * testable). Layout: Version@0, Size@4, NumberOfIdentifiers@8, then the
 * identifiers; each identifier = Code u32@0 (0=Reserved,1=Binary,2=Ascii),
 * Association u32@4 (0=device,1=port), IdentifierSize u16@8, then the bytes
 * (the NVMe/SATA standard query contract). Returns false on a short buffer
 * or when no device identifier fits. */
inline bool decode_device_id_descriptor(const u8* buf, size_t len, std::wstring& wwn) {
    wwn.clear();
    if (buf == nullptr || len < 12)
        return false;
    const u32 n = u32(buf[8]) | (u32(buf[9]) << 8) | (u32(buf[10]) << 16) | (u32(buf[11]) << 24);
    size_t off = 12;
    for (u32 i = 0; i < n && off + 10 <= len; ++i) {
        const u32 code = u32(buf[off]) | (u32(buf[off + 1]) << 8) | (u32(buf[off + 2]) << 16) |
                         (u32(buf[off + 3]) << 24);
        const u32 assoc = u32(buf[off + 4]) | (u32(buf[off + 5]) << 8) | (u32(buf[off + 6]) << 16) |
                          (u32(buf[off + 7]) << 24);
        const u32 size = u32(buf[off + 8]) | (u32(buf[off + 9]) << 8);
        const size_t body = off + 10;
        if (body + size > len)
            return false; /* torn identifier - refuse, do not truncate */
        if (assoc == 0 && size > 0) {
            if (code == 2) { /* Ascii */
                for (u32 k = 0; k < size && wwn.size() < 31; ++k) {
                    const u8 c = buf[body + k];
                    if (c < 0x20 || c > 0x7E)
                        break;
                    wwn.push_back(wchar_t(c));
                }
            } else { /* Binary (or reserved): hex render */
                static const wchar_t kHex[] = L"0123456789abcdef";
                for (u32 k = 0; k < size && wwn.size() < 31; ++k) {
                    if (wwn.size() >= 30)
                        break;
                    wwn.push_back(kHex[buf[body + k] >> 4]);
                    wwn.push_back(kHex[buf[body + k] & 0xF]);
                }
            }
            return !wwn.empty();
        }
        off = body + size;
    }
    return false;
}

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
                                  const PhysicalDiskList& interface_disks, PhysicalDiskList& out) {
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
