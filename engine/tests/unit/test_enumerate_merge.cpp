/* test_enumerate_merge.cpp - the two-stack disk-enumeration merge
 * (N-071, diff-to-fix).
 *
 * Proves the pure merge_disk_enumeration contract (the Linux-lane half of
 * the finding's test list; the acquisition seam - the real cfgmgr32 walk -
 * is Windows-lane):
 *   1. Enumerate.SetupApiMerge: a mock interface set merges into the
 *      numbered-loop list; devices the loop missed are APPENDED with
 *      their interface paths; devices the loop already has are dropped
 *      (the loop's entry is richer - the parsed on-disk table).
 *   2. Enumerate.DedupByInstancePath: repeated interface paths collapse
 *      to one entry; unknown-number entries dedup by path only.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/win/cr_enumerate.h"
#include <string>
#include <vector>

using namespace cr;
using namespace cr::win;

namespace {

PhysicalDisk loop_disk(u32 number, const wchar_t* model) {
    PhysicalDisk d;
    d.number = number;
    d.model = model;
    d.table_ok = true; /* the loop parses the on-disk table */
    return d;
}

PhysicalDisk iface_disk(u32 number, const wchar_t* path) {
    PhysicalDisk d;
    d.number = number;
    d.interface_path = path;
    return d;
}

} // namespace

TEST(Enumerate, SetupApiMerge) {
    /* The loop found disks 0 and 2; the interface pass reports disks
     * 0 (dup), 1 (the loop's miss - e.g. a device whose PhysicalDrive
     * open failed but whose interface opens), 2 (dup) and 7 (new). */
    PhysicalDiskList loop;
    loop.push_back(loop_disk(0, L"disk0"));
    loop.push_back(loop_disk(2, L"disk2"));
    PhysicalDiskList ifaces;
    ifaces.push_back(iface_disk(0, L"\\?\\usbstor#disk0#{53f56307-...}"));
    ifaces.push_back(iface_disk(1, L"\\?\\usbstor#disk1#{53f56307-...}"));
    ifaces.push_back(iface_disk(2, L"\\?\\scsi#disk2#{53f56307-...}"));
    ifaces.push_back(iface_disk(7, L"\\?\\scsi#disk7#{53f56307-...}"));

    PhysicalDiskList merged;
    ASSERT_EQ(OK, merge_disk_enumeration(loop, ifaces, merged));
    ASSERT_EQ(size_t(4), merged.size());
    /* loop entries keep their order and their richer records */
    EXPECT_EQ(u32(0), merged[0].number);
    EXPECT_EQ(L"disk0", merged[0].model);
    EXPECT_TRUE(merged[0].table_ok);
    EXPECT_EQ(u32(2), merged[1].number);
    EXPECT_TRUE(merged[1].table_ok);
    /* the loop's miss and the new disk are appended, carrying the path */
    EXPECT_EQ(u32(1), merged[2].number);
    EXPECT_EQ(L"\\?\\usbstor#disk1#{53f56307-...}", merged[2].interface_path);
    EXPECT_EQ(u32(7), merged[3].number);
    EXPECT_EQ(L"\\?\\scsi#disk7#{53f56307-...}", merged[3].interface_path);
}

TEST(Enumerate, DedupByInstancePath) {
    /* The same interface instance reported twice (the multi-instance
     * shape a device with several interface registrations produces)
     * collapses to one entry; unknown-number entries carry no number
     * key, so ONLY the path dedups them. */
    PhysicalDiskList loop;
    loop.push_back(loop_disk(0, L"disk0"));
    PhysicalDiskList ifaces;
    ifaces.push_back(iface_disk(kUnknownDiskNumber, L"\\?\\ spaces#member1#{53f56307-...}"));
    ifaces.push_back(iface_disk(kUnknownDiskNumber, L"\\?\\ spaces#member1#{53f56307-...}"));
    ifaces.push_back(iface_disk(kUnknownDiskNumber, L"\\?\\ spaces#member2#{53f56307-...}"));
    ifaces.push_back(iface_disk(3, L"\\?\\usbstor#disk3#{53f56307-...}"));
    ifaces.push_back(iface_disk(3, L"\\?\\usbstor#disk3#{53f56307-...}"));

    PhysicalDiskList merged;
    ASSERT_EQ(OK, merge_disk_enumeration(loop, ifaces, merged));
    ASSERT_EQ(size_t(4), merged.size());
    EXPECT_EQ(kUnknownDiskNumber, merged[1].number);
    EXPECT_EQ(L"\\?\\ spaces#member1#{53f56307-...}", merged[1].interface_path);
    EXPECT_EQ(L"\\?\\ spaces#member2#{53f56307-...}", merged[2].interface_path);
    EXPECT_EQ(u32(3), merged[3].number);
}

TEST(Enumerate, MergeWithEmptyInterfaceStackKeepsLoop) {
    /* The degrade path: cfgmgr fails or reports nothing - the numbered
     * loop's list passes through unchanged. */
    PhysicalDiskList loop;
    loop.push_back(loop_disk(0, L"disk0"));
    loop.push_back(loop_disk(1, L"disk1"));
    PhysicalDiskList merged;
    ASSERT_EQ(OK, merge_disk_enumeration(loop, {}, merged));
    ASSERT_EQ(size_t(2), merged.size());
    EXPECT_EQ(u32(0), merged[0].number);
    EXPECT_EQ(u32(1), merged[1].number);
}

TEST(Enumerate, MergeWithEmptyLoopTakesAllInterfaces) {
    /* The inverse edge: no numbered disk opened (the exotic-stack host)
     * - every interface entry surfaces. */
    PhysicalDiskList ifaces;
    ifaces.push_back(iface_disk(4, L"\\?\\scsi#disk4#{53f56307-...}"));
    ifaces.push_back(iface_disk(kUnknownDiskNumber, L"\\?\\ spaces#member#{53f56307-...}"));
    PhysicalDiskList merged;
    ASSERT_EQ(OK, merge_disk_enumeration({}, ifaces, merged));
    ASSERT_EQ(size_t(2), merged.size());
    EXPECT_EQ(u32(4), merged[0].number);
    EXPECT_EQ(kUnknownDiskNumber, merged[1].number);
}

/* ---------------------------------------------------------------- */
/* N-080: the descriptor-offset transposition (fixed with N-066)     */
/* + N-066: the WWN/serial-2 decode + the bus interface text         */
/* ---------------------------------------------------------------- */

namespace {

/* Build a synthetic STORAGE_DEVICE_DESCRIPTOR with the SDK layout:
 * VendorIdOffset@+12, ProductIdOffset@+16, ProductRevisionOffset@+20,
 * SerialNumberOffset@+24, BusType@+28, then the packed strings. */
std::vector<u8> build_descriptor(u32 bus) {
    std::vector<u8> buf(36 + 128, 0);
    auto put32 = [&buf](size_t at, u32 v) {
        for (int i = 0; i < 4; ++i)
            buf[at + i] = u8(v >> (8 * i));
    };
    /* packed strings at fixed offsets past the header */
    const size_t vendor_at = 36;
    const size_t product_at = 36 + 16;
    const size_t revision_at = 36 + 48;
    const size_t serial_at = 36 + 80;
    std::memcpy(buf.data() + vendor_at, "ACME", 4);
    std::memcpy(buf.data() + product_at, "WD40TESTDISK", 12);
    std::memcpy(buf.data() + revision_at, "01.02D-04", 9);
    std::memcpy(buf.data() + serial_at, "SER-12345-XYZ", 14);
    put32(12, u32(vendor_at));   /* VendorIdOffset */
    put32(16, u32(product_at));  /* ProductIdOffset */
    put32(20, u32(revision_at)); /* ProductRevisionOffset */
    put32(24, u32(serial_at));   /* SerialNumberOffset */
    put32(28, bus);              /* BusType */
    return buf;
}

} // namespace

TEST(Enumerate, DeviceDescriptorFieldsDecode) {
    /* The N-080 transposition pin: with the SDK-exact offsets, the
     * product string lands in model, the REVISION lands in firmware
     * (the N-066 axis), the SERIAL lands in serial - the pre-fix code
     * read the product at +24 (the serial's offset) and the serial at
     * +36 (inside RawDeviceProperties). Fail-before executed on the
     * transposed tree (model == the SERIAL text). */
    auto buf = build_descriptor(7 /* BusTypeUsb */);
    PhysicalDisk d;
    ASSERT_TRUE(decode_device_descriptor(buf.data(), buf.size(), d));
    EXPECT_EQ(L"WD40TESTDISK", d.model);
    EXPECT_EQ(L"01.02D-04", d.firmware);
    EXPECT_EQ(L"SER-12345-XYZ", d.serial);
    EXPECT_TRUE(d.removable); /* bus 7 = USB */

    auto buf2 = build_descriptor(3 /* BusTypeAta */);
    PhysicalDisk d2;
    ASSERT_TRUE(decode_device_descriptor(buf2.data(), buf2.size(), d2));
    EXPECT_FALSE(d2.removable);
    EXPECT_STREQ(L"USB Device", bus_interface_name(7));
    EXPECT_STREQ(L"NVMe", bus_interface_name(17));
    EXPECT_STREQ(L"", bus_interface_name(99));
}

TEST(Enumerate, DeviceDescriptorBoundedAndAbsent) {
    /* offset 0 = field absent (SDK semantics); a torn buffer is refused;
     * the string scan never walks past the buffer. */
    auto buf = build_descriptor(3);
    PhysicalDisk d;
    ASSERT_TRUE(decode_device_descriptor(buf.data(), buf.size(), d));
    EXPECT_EQ(L"WD40TESTDISK", d.model);
    /* kill the product offset: absent */
    buf[16] = buf[17] = buf[18] = buf[19] = 0;
    PhysicalDisk d2;
    ASSERT_TRUE(decode_device_descriptor(buf.data(), buf.size(), d2));
    EXPECT_TRUE(d2.model.empty());
    /* short buffer: refused */
    EXPECT_FALSE(decode_device_descriptor(buf.data(), 20, d2));
    EXPECT_FALSE(decode_device_descriptor(nullptr, 0, d2));
}

/* ---------------------------------------------------------------- */
/* D-ENUM-02: the SHIFTED bus-type table (SATA flagged removable)    */
/* and the RemovableMedia flag the decode never read                 */
/* ---------------------------------------------------------------- */

TEST(Enumerate, SataDiskIsNotRemovable) {
    /* THE fail-before pin: the pre-fix derive was
     *   bus == 7 || bus == 11 || bus == 12
     * built on a table shifted by one from the SDK's STORAGE_BUS_TYPE
     * (11 is SATA, 12 is SD, 13 is MMC) - so a SATA disk (bus 11)
     * shipped removable=true and the UI sorted every internal SATA
     * drive into the external/USB group: the user's "not all of my
     * disks are visible" (listed, wrong group, wrong kind). The
     * fail-before run: this pin fails on the pre-fix tree. */
    PhysicalDisk d;
    auto buf = build_descriptor(11 /* BusTypeSata */);
    ASSERT_TRUE(decode_device_descriptor(buf.data(), buf.size(), d));
    EXPECT_FALSE(d.removable) << "SATA (bus 11) must not be removable";
    EXPECT_EQ(11u, d.bus_type);

    auto buf_nvme = build_descriptor(17 /* BusTypeNvme */);
    PhysicalDisk d_nvme;
    ASSERT_TRUE(decode_device_descriptor(buf_nvme.data(), buf_nvme.size(), d_nvme));
    EXPECT_FALSE(d_nvme.removable) << "NVMe (bus 17) must not be removable";
    EXPECT_EQ(17u, d_nvme.bus_type);

    /* the truly-removable buses keep the class */
    auto buf_sd = build_descriptor(12 /* BusTypeSd */);
    PhysicalDisk d_sd;
    ASSERT_TRUE(decode_device_descriptor(buf_sd.data(), buf_sd.size(), d_sd));
    EXPECT_TRUE(d_sd.removable) << "SD (bus 12) must be removable";
    auto buf_mmc = build_descriptor(13 /* BusTypeMmc */);
    PhysicalDisk d_mmc;
    ASSERT_TRUE(decode_device_descriptor(buf_mmc.data(), buf_mmc.size(), d_mmc));
    EXPECT_TRUE(d_mmc.removable) << "MMC (bus 13 - missed by the pre-fix table) must be removable";
}

TEST(Enumerate, RemovableMediaFlagIsTheAuthority) {
    /* The descriptor's OWN RemovableMedia byte (+10) is the OS's
     * classification - the axis the reference's WMI/SetupAPI stacks
     * read. A removable-flagged device on an internal-numbered bus
     * (e.g. a hot-plug SAS/SCSI enclosure) must decode removable
     * even though the bus alone would say internal. */
    auto buf = build_descriptor(10 /* BusTypeSas - not a removable bus class */);
    buf[10] = 1; /* RemovableMedia = TRUE */
    PhysicalDisk d;
    ASSERT_TRUE(decode_device_descriptor(buf.data(), buf.size(), d));
    EXPECT_TRUE(d.removable) << "the OS's RemovableMedia flag must be honored";
    EXPECT_EQ(10u, d.bus_type);

    /* and the clear flag on an internal bus stays internal */
    auto buf2 = build_descriptor(10);
    buf2[10] = 0;
    PhysicalDisk d2;
    ASSERT_TRUE(decode_device_descriptor(buf2.data(), buf2.size(), d2));
    EXPECT_FALSE(d2.removable);
}

TEST(Enumerate, BusInterfaceNamesFollowTheSdkOrder) {
    /* the pre-fix table labeled a SATA disk "SD", an SD card "MMC",
     * a Virtual disk "File-Backed Virtual"... everything from 11 up
     * shifted by one (NVMe only landed right because the shift
     * wrapped past it). The SDK order: ...10 SAS, 11 SATA, 12 SD,
     * 13 MMC, 14 Virtual, 15 FileBackedVirtual, 16 Spaces, 17 NVMe. */
    EXPECT_STREQ(L"SATA", bus_interface_name(11));
    EXPECT_STREQ(L"SD", bus_interface_name(12));
    EXPECT_STREQ(L"MMC", bus_interface_name(13));
    EXPECT_STREQ(L"Virtual", bus_interface_name(14));
    EXPECT_STREQ(L"File-Backed Virtual", bus_interface_name(15));
    EXPECT_STREQ(L"Storage Spaces", bus_interface_name(16));
    EXPECT_STREQ(L"NVMe", bus_interface_name(17));
    /* the pre-fix gaps: SSA(5) and iSCSI(9) now named */
    EXPECT_STREQ(L"SSA", bus_interface_name(5));
    EXPECT_STREQ(L"iSCSI", bus_interface_name(9));
}

TEST(Enumerate, DeviceIdWwnDecode) {
    /* N-066: the WWN/serial-2 axis — the first DEVICE-associated
     * identifier from STORAGE_DEVICE_ID_DESCRIPTOR; ascii passes through,
     * binary renders hex; a port-associated identifier is skipped; a
     * torn identifier refuses the whole decode. */
    auto put32 = [](std::vector<u8>& b, size_t at, u32 v) {
        for (int i = 0; i < 4; ++i)
            b[at + i] = u8(v >> (8 * i));
    };
    /* header + one port identifier (skipped) + one device ascii id */
    std::vector<u8> b(12 + 10 + 4 + 16 + 10 + 8, 0);
    put32(b, 8, 2); /* NumberOfIdentifiers */
    size_t off = 12;
    put32(b, off, 2);     /* Code = Ascii */
    put32(b, off + 4, 1); /* Association = port -> skipped */
    b[off + 8] = 4;
    b[off + 9] = 0; /* size 4 */
    std::memcpy(b.data() + off + 10, "iSCS", 4);
    off += 10 + 4;
    put32(b, off, 2);     /* Code = Ascii */
    put32(b, off + 4, 0); /* Association = device */
    b[off + 8] = 8;
    b[off + 9] = 0; /* size 8 */
    std::memcpy(b.data() + off + 10, "50014ee2", 8);
    std::wstring wwn;
    ASSERT_TRUE(decode_device_id_descriptor(b.data(), b.size(), wwn));
    EXPECT_EQ(L"50014ee2", wwn);

    /* binary device identifier -> hex render */
    std::vector<u8> b2(12 + 10 + 4, 0);
    put32(b2, 8, 1);
    put32(b2, 12, 1); /* Code = Binary */
    put32(b2, 16, 0); /* Association = device */
    b2[20] = 4;
    b2[21] = 0; /* size 4 */
    b2[22] = 0xDE;
    b2[23] = 0xAD;
    b2[24] = 0xBE;
    b2[25] = 0xEF;
    std::wstring wwn2;
    ASSERT_TRUE(decode_device_id_descriptor(b2.data(), b2.size(), wwn2));
    EXPECT_EQ(L"deadbeef", wwn2);

    /* torn identifier: size walks past the buffer -> refuse */
    b2[20] = 0xFF;
    std::wstring wwn3;
    EXPECT_FALSE(decode_device_id_descriptor(b2.data(), b2.size(), wwn3));
}
