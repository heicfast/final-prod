/* cr_driver_status.h - the driver-status surface: facet struct, refusal
 * taxonomy and the PURE classification state machine (docs/DRIVER-REQUIRED.md,
 * docs/PLAN-DRIVER-ONLY.md sections 2-3).
 *
 * Platform-independent BY DESIGN (the cr_winenv.h pattern): the decision
 * logic lives where BOTH lanes compile it, so the Linux unit lane pins the
 * classifier (tests/unit/test_driver_status.cpp) and the Windows runner
 * proves the real query against the real driver (the S-DRV-REFUSE scenario).
 *
 * Facet -> mechanism mapping (docs/DRIVER-CONTRACT.md section 8: the
 * driver's entire user-mode surface is the control-device open + GET_INFO;
 * there is no health/heartbeat IOCTL):
 *   present    SCM OpenService (service installed)
 *   loaded     SCM QueryServiceStatus (service running)
 *   started    CreateFile on \\.\RDA succeeds
 *   authorized the open does not fail ERROR_ACCESS_DENIED (device DACL)
 *   healthy    GET_INFO + GET_VERSION round-trip consistent
 *   version    GET_INFO major/minor, cross-checked against GET_VERSION
 *
 * Registered divergence (docs/PARITY-DELTAS.md): the og exposes NO
 * driver-status surface (REFERENCE-DRIVER-BEHAVIOR section 2-3: the
 * service-start result is discarded; failed driver opens are silent NULL
 * devices; no version negotiation). This surface is the product-scope
 * truthfulness addition the driver-only mandate requires.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#ifndef CR_DRIVER_STATUS_H_
#define CR_DRIVER_STATUS_H_

#include "../cr_types.h"
#include "../cr_error.h"

namespace cr {
namespace win {

/* The default deployed service name (rda-driver install.ps1 -ServiceName;
 * docs/DRIVER-RELEASE.md section 5). Overridable per call. */
constexpr const wchar_t* kDefaultDriverService = L"RDA";

/* Wire-compatibility contract with the driver (cr_rda.h kVersionMajor; the
 * driver reports (major<<32|minor) via GET_VERSION 0x6002448 and the same
 * pair in GET_INFO 0x6002440's DriverInfo). The driver side has no ABI
 * enforcement (DRIVER-CONTRACT section 5) - the consumer probes, which is
 * exactly what query_driver_status does. Major must match; minor is
 * informational (additive). */
constexpr u32 kDriverWireMajorRequired = 1u;

/* The refusal taxonomy (error family 0x500B, cr_error.h). Each condition is
 * a DISTINCT error class; every driver-requiring operation fails fast with
 * exactly one of them (docs/DRIVER-REQUIRED.md carries the full contract
 * text + CLI stderr + exit codes). */
enum class DriverState {
    Ready,           /* every facet green                       */
    Absent,          /* ERR_DRIVER_ABSENT      0x500B0001       */
    NotStarted,      /* ERR_DRIVER_NOT_STARTED 0x500B0002       */
    Unauthorized,    /* ERR_DRIVER_UNAUTHORIZED 0x500B0003      */
    VersionMismatch, /* ERR_DRIVER_VERSION_MISMATCH 0x500B0004  */
    Errored,         /* ERR_DRIVER_ERRORED     0x500B0005       */
};

/* The facet snapshot. Filled by query_driver_status (Win32; src/win/
 * cr_transport.cpp); classified by driver_state() below. */
struct DriverStatus {
    bool service_present = false; /* installed   (SCM OpenService)      */
    bool service_running = false; /* loaded      (SCM state == RUNNING) */
    bool device_openable = false; /* started     (control-device open)  */
    bool authorized = false;      /* open not denied by the device DACL */
    bool info_valid = false;      /* healthy     (GET_INFO+GET_VERSION
                                   * consistent round-trip)             */
    u32 major = 0;                /* version (GET_INFO, cross-checked)  */
    u32 minor = 0;
    u32 flags = 0; /* DriverInfo capability echo         */
    u32 max_disks = 0;
    u32 ioctl_base = 0;
    u32 last_gle = 0; /* diagnostic: the last Win32 error
                       * from the probe that failed         */
};

/* The classification state machine - PURE, first-match order (PLAN-DRIVER-
 * ONLY section 2). Linux-lane pinned; the Windows runner proves it against
 * the real service/driver matrix.
 *
 * Order rationale: service existence separates ABSENT from NOT_STARTED;
 * the DACL denial (ERROR_ACCESS_DENIED) separates UNAUTHORIZED before any
 * other open failure becomes ERRORED (the v0.2.0 error-87 class lands
 * here: service RUNNING + open fails 87 -> Errored); a GET_INFO failure or
 * inconsistent version pair on an OPENED device is a wrong/ancient driver
 * (ERRORED - the FB-T06 contract); a GET_INFO major mismatch is
 * VERSION_MISMATCH. */
constexpr DriverState driver_state(const DriverStatus& st) {
    if (!st.service_present)
        return DriverState::Absent;
    if (!st.service_running)
        return DriverState::NotStarted;
    if (!st.device_openable)
        return st.last_gle == 5u /* ERROR_ACCESS_DENIED */ ? DriverState::Unauthorized
                                                           : DriverState::Errored;
    if (!st.info_valid)
        return DriverState::Errored;
    if (st.major != kDriverWireMajorRequired)
        return DriverState::VersionMismatch;
    return DriverState::Ready;
}

/* The error code for a state (the ERR_DRIVER_* class; OK when Ready). */
constexpr u32 driver_state_error(DriverState s) {
    switch (s) {
    case DriverState::Ready:
        return OK;
    case DriverState::Absent:
        return ERR_DRIVER_ABSENT;
    case DriverState::NotStarted:
        return ERR_DRIVER_NOT_STARTED;
    case DriverState::Unauthorized:
        return ERR_DRIVER_UNAUTHORIZED;
    case DriverState::VersionMismatch:
        return ERR_DRIVER_VERSION_MISMATCH;
    case DriverState::Errored:
        return ERR_DRIVER_ERRORED;
    }
    return ERR_DRIVER_ERRORED;
}

/* Human-readable state name (CLI `driver` verb, err_text companions). */
constexpr const char* driver_state_name(DriverState s) {
    switch (s) {
    case DriverState::Ready:
        return "ready";
    case DriverState::Absent:
        return "absent";
    case DriverState::NotStarted:
        return "not-started";
    case DriverState::Unauthorized:
        return "unauthorized";
    case DriverState::VersionMismatch:
        return "version-mismatch";
    case DriverState::Errored:
        return "errored";
    }
    return "errored";
}

} // namespace win
} // namespace cr

#endif // CR_DRIVER_STATUS_H_
