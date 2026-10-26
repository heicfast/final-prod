/* cr_winenv.h - the M-030 environment-gate + quiesce-chain corpus pins
 * (platform-independent: the decision logic is shared by the Windows
 * layer's implementation and the Linux-lane regression tests, so the
 * corpus constants are pinned where BOTH lanes compile them).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#ifndef CR_WINENV_H_
#define CR_WINENV_H_

#include <cstdint>

namespace cr {
namespace win {

/* M-030 (diff-to-fix): the og's WinPE environment probe decision input.
 * exFATSearchDLL... no - Device.mo's FUN_1800577e0
 * (ghidra-mo/Device_decompiled.c :50943-50992, re-verified this session):
 * GetSystemDirectoryA into a 0x104 buffer, then the FIRST BYTE is
 * compared against 'X'/'x' - WinPE's system drive is X: by convention.
 * The probe also opens \\.\X: + IOCTL 0x560000 (GET_VOLUME_DISK_
 * EXTENTS) but DISCARDS the result (the buffer is freed; the device's
 * decision input is the drive letter alone - the open is a presence
 * confirmation). Callers (:352, :7816, :50653, :52437) branch on the
 * return: 0 (live Windows) -> the PID-token service grammar;
 * nonzero (PE) -> \\.\PhysicalDrive%d / \\.\HarddiskVolume%d DIRECT.
 * The decompiler typed the probe void (OQ-3 of the finding) - the four
 * caller branch patterns pin the polarity: NONZERO = WinPE. */
inline bool pe_environment_by_system_drive(char system_drive_letter) {
    return system_drive_letter == 'X' || system_drive_letter == 'x';
}

/* The og's disk-number space: the driver CREATE grammar accepts
 * disk# <= 199 (ghidra-drivers/eprdtdrv_decompiled.c :184) - the
 * product's enumeration space is 200 disks. The pre-M-030 core
 * enumerated 32 (and still does on the LDM resolver path until the
 * fix); every enumeration site pins to this bound. */
constexpr uint32_t ENUMERATE_DISK_BOUND = 200;

/* M-030: the og's volume-quiesce chain constants, decompile-pinned
 * (ghidra-partition/RecoveryModule_decompiled.c :60401-60447 - the
 * per-volume repair chain; the FSCTL hex values are the decompile's
 * own):
 *   FSCTL_LOCK_VOLUME            0x90018  :60401
 *   DeleteVolumeMountPointA               :60412
 *   FlushFileBuffers                      :60422
 *   Sleep(2000)                           :60428
 *   FSCTL_DISMOUNT_VOLUME       0x90020  :60431
 *   FSCTL_UNLOCK_VOLUME         0x9001c  :60436
 *   IOCTL_VOLUME_OFFLINE        0x56c00c :60440  <- the terminal step
 * The restore path (OQ-2, decoded this session - FUN_1800459b0
 * :34465-34502, mirrored across the module family):
 *   open the volume device, IOCTL_VOLUME_ONLINE 0x56c008 :34484,
 *   re-create the mount point (the saved letter), Sleep(1000) :34499. */
constexpr uint32_t FSCTL_LOCK_VOLUME_HEX = 0x90018;
constexpr uint32_t FSCTL_DISMOUNT_VOLUME_HEX = 0x90020;
constexpr uint32_t FSCTL_UNLOCK_VOLUME_HEX = 0x9001c;
constexpr uint32_t IOCTL_VOLUME_OFFLINE_HEX = 0x56c00c;
constexpr uint32_t IOCTL_VOLUME_ONLINE_HEX = 0x56c008;

} // namespace win
} // namespace cr

#endif // CR_WINENV_H_
