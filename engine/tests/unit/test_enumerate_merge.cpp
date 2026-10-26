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
