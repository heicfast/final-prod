/*
 * cr_surgery.h - the recovered-OS boot-environment surgery (M-113,
 * diff-to-fix): the og's genuefisystemfile.cpp write-side family, ported
 * as pure hive-buffer operations over the reghive editor.
 *
 * Corpus (all sites re-read first-hand at the fix):
 *   - RecoveryModule_decompiled.c :73731-73790 + :73800 - the SYSTEM
 *     hive load as "TempSystem", the \MountedDevices open, the per-value
 *     enumeration through the FUN_1800aa1b0 callback (DAT_180003c3d),
 *     the unload - the mounted-devices rewrite pass;
 *   - :74837-74887 - the BCD surgery: load "<esp>\EFI\Microsoft\Boot\
 *     BCD" as "TmpBCD", read Objects\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\
 *     Elements\23000003\Element (the boot-application reference), and
 *     REWRITE that application's Objects\<guid>\Elements\12000004\
 *     Element with the new device (type 1 REG_SZ, the wide-string bytes,
 *     wcslen*2 - the finding's "REG_BINARY" annotation corrected by this
 *     decode: the og's write call is (hKey,"Element",type=1,bytes,len));
 *   - :73605-73621 - the SOFTWARE-hive leg is a READ (ProductName feeds
 *     the profile/branding pass) - covered by the offlineos profile
 *     walk; no ProductName WRITE exists in the decoded corpus;
 *   - :42845-42860 - bootstat.dat: open <root>\Windows\bootstat.dat,
 *     seek to offset 10, WRITE ONE BYTE 0x01 (the dirty-flag reset).
 *
 * Layering: pure core over hive BUFFERS (the host reads/writes the files
 * - the core owns no file API). The og's RegLoadKey form vs this direct
 * editor: the direct form needs no live registry service and no
 * elevation, and works on dead-system images - the recovery context
 * itself (registered in the finding's Resolution).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SURGERY_H_
#define CR_SURGERY_H_

#include "cr_reghive_edit.h"
#include <cstddef>

namespace cr {
namespace reghive {
namespace surgery {

/* ---- 1. the MountedDevices rewrite (the recovered SYSTEM hive) -------
 *
 * The og rewrites the \MountedDevices value family so the recovered OS
 * resolves its boot disk: each "\DosDevices\<letter>:" value carries the
 * 12-byte REG_BINARY the offlineos profile reads back (u32 disk
 * signature + u64 partition start - DeviceManager :62184-62195, the
 * read-side decode; the write mirrors it byte-for-byte). */
struct MountedDeviceMapping {
    char letter;            /* 'C'..'Z' (upper) */
    u32 disk_signature;     /* the NEW disk's MBR signature */
    u64 partition_offset;   /* the NEW partition's start offset, bytes */
};

/* Rewrite every mapping's value. The key \MountedDevices must exist and
 * each mapped "\DosDevices\<letter>:" value must exist (the surgeries
 * rewrite existing values only - an absent value is ERR_BAD_PARAMETER,
 * the honest refusal; a partial rewrite cannot happen: every value is
 * located BEFORE the first byte changes). Returns OK or the first
 * failing mapping's error. */
u32 rewrite_mounted_devices(std::vector<u8>& system_hive, const MountedDeviceMapping* mappings,
                            size_t count);

/* ---- 2. the BCD device-element re-point (the recovered ESP's BCD) ----
 *
 * The og's exact walk: read the boot manager object's 23000003 element
 * (the boot-application reference, a wide GUID string), then rewrite
 * that application's 12000004 device element with the new device
 * description (wide-string bytes, type REG_SZ). The boot manager's own
 * GUID literal is the corpus constant below. When the referenced GUID
 * IS the boot manager's (the self-reference case the og guards with the
 * wcslen compare), the rewrite is a no-op error - never a self-repoint.
 *
 * new_device_utf16: the new device description, NUL-terminated UTF-16
 * (the bytes written are wcslen*2 - the terminator is NOT written, the
 * og's exact length arithmetic). */
constexpr const char* kBcdBootManagerGuid = "{9dea862c-5cdd-4e70-acc1-f32b344d4795}";

u32 repoint_bcd_device_element(std::vector<u8>& bcd_hive, const char16_t* new_device_utf16);

/* ---- 3. the bootstat.dat dirty-flag reset --------------------------------
 *
 * The og's byte-exact contract: file "<root>\Windows\bootstat.dat",
 * offset 10, one byte 0x01 (RecoveryModule :42845-42860). The WRITE is
 * host-side file I/O by architecture; the core pins the contract. */
constexpr u64 kBootstatOffset = 10;
constexpr u8 kBootstatValue = 0x01;

/* ---- plan/undo model -----------------------------------------------------
 *
 * Every surgery is reversible by the caller's own pre-image (the host
 * snapshots the file bytes before apply; undo = write them back). The
 * core's contract: surgery functions mutate the PASSED BUFFER only -
 * byte-exact undo is trivially available to the host that kept the
 * original. The apply-transaction integration (the pending-op class
 * riding CRUNDO01) is the recorded follow-up consumer. */

} // namespace surgery
} // namespace reghive
} // namespace cr

#endif // CR_SURGERY_H_
