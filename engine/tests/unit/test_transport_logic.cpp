/* test_transport_logic.cpp - M-030's environment-gate and quiesce-chain
 * corpus pins (platform-independent: the decision predicates and chain
 * constants live in cr/win/cr_winenv.h so BOTH lanes compile them; the
 * Windows lane's behavior tests run the real DeviceIoControl chain).
 *
 * The finding's claims (a)/(d)/(e)/(f) were rg-verified absences in the
 * pre-fix tree (no GetSystemDirectory probe, no ERROR_INVALID_HANDLE...
 * no - those landed; no IOCTL_VOLUME_OFFLINE, a 32-disk LDM cap, a
 * silent RDA fallback). This file's pins fail at COMPILE time pre-fix
 * (cr_winenv.h and the cr_ui_last_transport_error export did not exist)
 * and pin the corpus constants + decisions post-fix; the full behavioral
 * fail-befores (a PE-gated open, the OFFLINE-in-chain sequence, the
 * error-surfaced fallback) run on the Windows CI lane against the real
 * Win32 surface.
 */
#include <gtest/gtest.h>
#include "cr/win/cr_winenv.h"
#include "cr/win/cr_transport.h" /* N-125: the disconnect classification */

using namespace cr;
using namespace cr::win;

TEST(Transport, PeGateDecisionMatchesOgProbe) {
    /* FUN_1800577e0 (Device_decompiled.c :50943-50992, re-verified this
     * session): GetSystemDirectoryA's FIRST BYTE decides - 'X'/'x' is
     * WinPE's system drive; anything else is live Windows. The probe's
     * \\.\X: open discards its result - the drive letter alone is the
     * decision input (the finding's OQ-3, resolved by the four callers'
     * branch patterns: nonzero return = PE = the direct
     * PhysicalDrive/HarddiskVolume paths). */
    EXPECT_TRUE(pe_environment_by_system_drive('X'));
    EXPECT_TRUE(pe_environment_by_system_drive('x'));
    EXPECT_FALSE(pe_environment_by_system_drive('C'));
    EXPECT_FALSE(pe_environment_by_system_drive('c'));
    EXPECT_FALSE(pe_environment_by_system_drive('D'));
    EXPECT_FALSE(pe_environment_by_system_drive('\0'));
    EXPECT_FALSE(pe_environment_by_system_drive('0'));
}

TEST(Transport, QuiesceChainConstantsMatchCorpus) {
    /* The repair-write quiesce chain, decompile-pinned
     * (RecoveryModule_decompiled.c :60401-60447):
     *   LOCK 0x90018 :60401 -> DeleteVolumeMountPoint :60412 ->
     *   Flush :60422 -> Sleep(2000) :60428 -> DISMOUNT 0x90020 :60431 ->
     *   UNLOCK 0x9001c :60436 -> OFFLINE 0x56c00c :60440 (terminal).
     * The restore chain (FUN_1800459b0 :34465-34502, the finding's
     * OQ-2 decoded): ONLINE 0x56c008 :34484 -> mount-point restore ->
     * Sleep(1000) :34499. */
    EXPECT_EQ(FSCTL_LOCK_VOLUME_HEX, 0x90018u);
    EXPECT_EQ(FSCTL_DISMOUNT_VOLUME_HEX, 0x90020u);
    EXPECT_EQ(FSCTL_UNLOCK_VOLUME_HEX, 0x9001cu);
    EXPECT_EQ(IOCTL_VOLUME_OFFLINE_HEX, 0x56c00cu);
    EXPECT_EQ(IOCTL_VOLUME_ONLINE_HEX, 0x56c008u);
}

TEST(Transport, EnumerateBoundMatchesDriverGrammarSpace) {
    /* The og's driver CREATE grammar accepts disk# <= 199
     * (eprdtdrv_decompiled.c :184) - the product's enumeration space is
     * 200 disks. The pre-fix core enumerated 32 (and the LDM resolver
     * still did until this fix - hosts with >32 disks are exactly the
     * multi-disk LDM population). */
    EXPECT_EQ(ENUMERATE_DISK_BOUND, 200u);
}

TEST(Transport, DisconnectClassifiedDistinctly) {
    /* N-125: the device-gone GLE family classifies as
     * ERR_DEVICE_DISCONNECTED (the reattach-then-resume contract) - a
     * distinct code, never a read-failure storm; the og's "Drive
     * Disconnected" Retry/Cancel dialog (UIUX/08:39-47) drives the same
     * flow from its UI layer. */
    EXPECT_TRUE(disconnect::gle_is_disconnect(disconnect::kDevNotExist));    /* 55 */
    EXPECT_TRUE(disconnect::gle_is_disconnect(disconnect::kNotReady));       /* 21 */
    EXPECT_TRUE(disconnect::gle_is_disconnect(disconnect::kMediaChanged));   /* 1110 */
    EXPECT_TRUE(disconnect::gle_is_disconnect(disconnect::kNoMedia));        /* 1112 */
    EXPECT_TRUE(disconnect::gle_is_disconnect(disconnect::kDeviceRemoved));  /* 1619 */
}

TEST(Transport, MediaErrorsStillReadFailed) {
    /* N-125's precision requirement: the dying-disk storm keeps the old
     * semantics - IO-device and CRC errors are media failures, not
     * disconnects (the classification must not become a catch-all). */
    EXPECT_FALSE(disconnect::gle_is_disconnect(1117)); /* ERROR_IO_DEVICE */
    EXPECT_FALSE(disconnect::gle_is_disconnect(23));   /* ERROR_CRC */
    EXPECT_FALSE(disconnect::gle_is_disconnect(87));   /* ERROR_INVALID_PARAMETER */
    EXPECT_FALSE(disconnect::gle_is_disconnect(0));
    /* the code itself: append-only in the storage I/O family */
    EXPECT_EQ(u32(ERR_DEVICE_DISCONNECTED), 0x50020007u);
}
