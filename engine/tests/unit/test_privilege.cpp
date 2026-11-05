/* test_privilege.cpp - the direct-era refusal-taxonomy pins (session-96:
 * the kernel-driver replacement). Platform-independent: the error code,
 * its text, and the header-surface truth live where BOTH lanes compile
 * them; the Windows runner proves the real TokenElevation probe and the
 * exit-4 refusal against the real Win32 surface (checks.yml's
 * driverless-refusal probes, now the elevation-refusal probes).
 *
 * What this file pins (the replacements of the driver-era
 * test_driver_status.cpp, deleted with the driver-status surface):
 *   - ERR_ELEVATION_REQUIRED is 0x500C0001 (a FRESH value slot - the
 *     removed 0x500B family is never re-used)
 *   - the 0x500B values are absent from cr_error.h (the family must
 *     never return)
 *   - err_text carries the class name
 *   - the ABI header no longer declares the driver-status surface
 *   - the transport header no longer declares the grammar opens or the
 *     status query; open_disk is the direct 2-arg form
 */
#include <gtest/gtest.h>
#include <string>
#include "cr/cr_error.h"
#include "cr/win/cr_transport.h"
#include "test_tmpdir.hpp"

using namespace cr;

TEST(Privilege, ElevationRequiredIsFreshCodeSlot) {
    /* The single refusal class of the direct era. FRESH value: 0x500C0001,
     * never a re-used 0x500B slot - a host compiled against the driver-era
     * header must never silently decode a re-used value as its old meaning
     * (the storage-family append-only discipline). */
    EXPECT_EQ(u32(ERR_ELEVATION_REQUIRED), 0x500C0001u);
    EXPECT_NE(err_text(ERR_ELEVATION_REQUIRED), "unknown error")
        << "err_text must carry the class name";
    EXPECT_NE(err_text(ERR_ELEVATION_REQUIRED), nullptr);
}

TEST(Privilege, DriverErrorFamilyGone) {
    /* The 0x500B family is REMOVED, not stubbed: the values, the names,
     * and the err_text entries must never return. */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_error.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("ERR_DRIVER_ABSENT"), std::string::npos);
    EXPECT_EQ(hdr.find("ERR_DRIVER_NOT_STARTED"), std::string::npos);
    EXPECT_EQ(hdr.find("ERR_DRIVER_UNAUTHORIZED"), std::string::npos);
    EXPECT_EQ(hdr.find("ERR_DRIVER_VERSION_MISMATCH"), std::string::npos);
    EXPECT_EQ(hdr.find("ERR_DRIVER_ERRORED"), std::string::npos);
    EXPECT_EQ(hdr.find("0x500B000"), std::string::npos)
        << "no 0x500B000x value may linger in the error space (the comment"
        << " may cite the removed family by name for history - the VALUES"
        << " must not)";
}

TEST(Privilege, TransportSurfaceIsDirectOnly) {
    /* The transport header: no grammar opens, no status query, no
     * service-name parameters; open_disk/open_disk_rw/open_disk_apply_rw
     * are the direct 2-arg forms. */
    const std::string hdr = cr_test::read_repo_text("include/cr/win/cr_transport.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("open_rda_partition"), std::string::npos);
    EXPECT_EQ(hdr.find("open_rda_volume"), std::string::npos);
    EXPECT_EQ(hdr.find("query_driver_status"), std::string::npos);
    EXPECT_EQ(hdr.find("DriverStatus"), std::string::npos);
    EXPECT_EQ(hdr.find("service"), std::string::npos)
        << "no service-name parameter survives the driver removal";
    EXPECT_NE(hdr.find("open_disk(u32 disk, u32* err)"), std::string::npos)
        << "open_disk is the direct 2-arg form";
    EXPECT_NE(hdr.find("open_disk_rw(u32 disk, u32* err)"), std::string::npos);
    EXPECT_NE(hdr.find("open_disk_apply_rw(u32 disk, u32* err)"), std::string::npos);
}

TEST(Privilege, RdaHeadersDeleted) {
    /* The wire-contract header and the status header are deleted, not
     * stubbed (read_repo_file asserts on absence, so existence is probed
     * through the filesystem directly). */
    EXPECT_FALSE(cr_test::repo_file_exists("include/cr/win/cr_rda.h")) << "cr_rda.h must not exist";
    EXPECT_FALSE(cr_test::repo_file_exists("include/cr/win/cr_driver_status.h"))
        << "cr_driver_status.h must not exist";
}

TEST(Privilege, DisconnectClassificationSurvives) {
    /* The N-125 classification is transport-independent and must survive
     * the driver removal byte-for-byte. */
    EXPECT_TRUE(win::disconnect::gle_is_disconnect(21));
    EXPECT_TRUE(win::disconnect::gle_is_disconnect(55));
    EXPECT_TRUE(win::disconnect::gle_is_disconnect(1110));
    EXPECT_TRUE(win::disconnect::gle_is_disconnect(1112));
    EXPECT_TRUE(win::disconnect::gle_is_disconnect(1619));
    EXPECT_FALSE(win::disconnect::gle_is_disconnect(1117));
    EXPECT_FALSE(win::disconnect::gle_is_disconnect(23));
    EXPECT_FALSE(win::disconnect::gle_is_disconnect(87));
}
