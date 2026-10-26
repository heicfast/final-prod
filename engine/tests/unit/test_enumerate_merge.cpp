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
