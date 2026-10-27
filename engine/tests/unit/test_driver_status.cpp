/* test_driver_status.cpp - the refusal-taxonomy classification pins
 * (docs/DRIVER-REQUIRED.md section 2; docs/PLAN-DRIVER-ONLY section 7).
 *
 * The classifier (driver_state) is PURE and platform-independent
 * (cr/win/cr_driver_status.h, the cr_winenv.h pattern) so the Linux lane
 * pins every state transition; the Windows runner proves the real
 * query against the real service/driver matrix (the S-DRV-REFUSE
 * staged scenario - no mock on the truth path).
 */
#include <gtest/gtest.h>
#include "cr/win/cr_driver_status.h"

using namespace cr;
using namespace cr::win;

namespace {

DriverStatus base_ready() {
    DriverStatus st{};
    st.service_present = true;
    st.service_running = true;
    st.device_openable = true;
    st.authorized = true;
    st.info_valid = true;
    st.major = 1;
    st.minor = 0;
    st.flags = 0x3; /* filter + sessions */
    st.max_disks = 200;
    st.ioctl_base = 0;
    return st;
}

} // namespace

TEST(DriverStatus, ReadyWhenAllFacetsGreen) {
    const DriverStatus st = base_ready();
    EXPECT_EQ(driver_state(st), DriverState::Ready);
    EXPECT_EQ(driver_state_error(driver_state(st)), u32(OK));
}

TEST(DriverStatus, AbsentWhenServiceNotInstalled) {
    DriverStatus st = base_ready();
    st.service_present = false;
    st.service_running = false; /* downstream facets collapse with it */
    st.device_openable = false;
    st.authorized = false;
    st.info_valid = false;
    EXPECT_EQ(driver_state(st), DriverState::Absent);
    EXPECT_EQ(driver_state_error(DriverState::Absent), u32(ERR_DRIVER_ABSENT));
    EXPECT_EQ(u32(ERR_DRIVER_ABSENT), 0x500B0001u);
}

TEST(DriverStatus, NotStartedWhenInstalledButStopped) {
    DriverStatus st = base_ready();
    st.service_running = false;
    st.device_openable = false;
    st.authorized = false;
    st.info_valid = false;
    EXPECT_EQ(driver_state(st), DriverState::NotStarted);
    EXPECT_EQ(driver_state_error(DriverState::NotStarted), u32(ERR_DRIVER_NOT_STARTED));
    EXPECT_EQ(u32(ERR_DRIVER_NOT_STARTED), 0x500B0002u);
}

TEST(DriverStatus, UnauthorizedWhenOpenDeniedByDacl) {
    /* ERROR_ACCESS_DENIED = 5: the SDDL D:P(A;;GA;;;BA)(A;;GA;;;SY) denial
     * a non-elevated caller sees (DRIVER-CONTRACT section 7) */
    DriverStatus st = base_ready();
    st.device_openable = false;
    st.authorized = false;
    st.info_valid = false;
    st.last_gle = 5;
    EXPECT_EQ(driver_state(st), DriverState::Unauthorized);
    EXPECT_EQ(driver_state_error(DriverState::Unauthorized), u32(ERR_DRIVER_UNAUTHORIZED));
    EXPECT_EQ(u32(ERR_DRIVER_UNAUTHORIZED), 0x500B0003u);
}

TEST(DriverStatus, ErroredWhenOpenFailsWithAnyOtherGle) {
    /* the v0.2.0 regression class: service RUNNING but every bare
     * control-device open fails with error 87 (INVALID_PARAMETER) -
     * the classifier must never collapse this into Absent/NotStarted */
    DriverStatus st = base_ready();
    st.device_openable = false;
    st.authorized = false;
    st.info_valid = false;
    st.last_gle = 87;
    EXPECT_EQ(driver_state(st), DriverState::Errored);
    EXPECT_EQ(driver_state_error(DriverState::Errored), u32(ERR_DRIVER_ERRORED));
    EXPECT_EQ(u32(ERR_DRIVER_ERRORED), 0x500B0005u);
}

TEST(DriverStatus, ErroredWhenGetInfoFailsOnOpenedDevice) {
    /* FB-T06's contract: GET_INFO failure on a successfully-opened device
     * = wrong/ancient driver - info_valid false, everything else green */
    DriverStatus st = base_ready();
    st.info_valid = false;
    EXPECT_EQ(driver_state(st), DriverState::Errored);
}

TEST(DriverStatus, VersionMismatchWhenMajorDiffers) {
    DriverStatus st = base_ready();
    st.major = 2; /* a future (or foreign) wire major */
    st.minor = 7;
    EXPECT_EQ(driver_state(st), DriverState::VersionMismatch);
    EXPECT_EQ(driver_state_error(DriverState::VersionMismatch), u32(ERR_DRIVER_VERSION_MISMATCH));
    EXPECT_EQ(u32(ERR_DRIVER_VERSION_MISMATCH), 0x500B0004u);
}

TEST(DriverStatus, MinorOnlyDivergenceStaysReady) {
    /* the compat contract is major-match, minor-informative: a driver
     * reporting 1.3 against a core requiring major 1 is Ready */
    DriverStatus st = base_ready();
    st.minor = 3;
    EXPECT_EQ(driver_state(st), DriverState::Ready);
}

TEST(DriverStatus, ClassificationOrderIsFailFastFirstMatch) {
    /* the 8-step machine's ordering: service existence separates Absent
     * from NotStarted BEFORE the device probe; the DACL denial separates
     * Unauthorized from Errored before GET_INFO runs. A status with
     * EVERYTHING wrong classifies as the FIRST unmet condition. */
    DriverStatus st{};
    EXPECT_EQ(driver_state(st), DriverState::Absent);
    st.service_present = true;
    EXPECT_EQ(driver_state(st), DriverState::NotStarted);
    st.service_running = true;
    EXPECT_EQ(driver_state(st), DriverState::Errored); /* not openable, gle 0 */
    st.device_openable = true;
    st.authorized = true;
    EXPECT_EQ(driver_state(st), DriverState::Errored); /* info invalid */
    st.info_valid = true;
    st.major = 9;
    EXPECT_EQ(driver_state(st), DriverState::VersionMismatch);
    st.major = 1;
    EXPECT_EQ(driver_state(st), DriverState::Ready);
}

TEST(DriverStatus, StateNamesAndErrorCodesAreTotalFunctions) {
    /* every enum value has a name and a mapped error - the CLI/ABI text
     * never falls into an unhandled switch arm */
    const DriverState all[] = {DriverState::Ready,           DriverState::Absent,
                               DriverState::NotStarted,      DriverState::Unauthorized,
                               DriverState::VersionMismatch, DriverState::Errored};
    for (DriverState s : all) {
        EXPECT_NE(driver_state_name(s), nullptr);
        EXPECT_STRNE(driver_state_name(s), "");
        if (s == DriverState::Ready)
            EXPECT_EQ(driver_state_error(s), u32(OK));
        else
            EXPECT_NE(driver_state_error(s), u32(OK));
    }
}

TEST(DriverStatus, DefaultStatusIsAbsentNotReady) {
    /* zero-init safety: a default-constructed DriverStatus (e.g. a host
     * that ignores the return code) classifies as Absent - never a
     * false Ready (the zero-init-safe ABI rule, cr_ui.h) */
    DriverStatus st{};
    EXPECT_EQ(driver_state(st), DriverState::Absent);
    EXPECT_EQ(driver_state_error(driver_state(st)), u32(ERR_DRIVER_ABSENT));
}

TEST(DriverStatus, RequiredWireMajorMatchesDriverContract) {
    /* cr_rda.h kVersionMajor == 1; the driver reports (major<<32)|minor
     * via GET_VERSION 0x60002448 - the core requires the major match */
    EXPECT_EQ(kDriverWireMajorRequired, 1u);
}
