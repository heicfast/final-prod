/*
 * cr_transport.h - the Windows transport layer: DIRECT raw I/O, user mode,
 * administrator privilege (session-96: the kernel-driver replacement).
 *
 * The RDA driver era is GONE - removed, not gated, not deprecated, not left
 * as a fallback. The data path opens \\.\PhysicalDriveN directly with the
 * same access/share/disposition the grammar path used, reads through the
 * unified 64 KiB envelope (io::direct_envelope - the sector-rejection and
 * GAP-S6-13 defenses), and reports true byte counts (the driver-era F11
 * Information=Length*512 scaling is deleted with the driver).
 *
 * The privilege model (the mandate's honest-elevation contract): the direct
 * layer's single precondition is the process token. Every raw-disk open in
 * a non-elevated process refuses with ERR_ELEVATION_REQUIRED (0x500C0001);
 * image targets never touch raw disks and are exempt by classification.
 * The CLI prints the refusal and exits 4; the ABI host triggers its own
 * elevation flow and retries. Nothing elevates silently; nothing runs a
 * helper; nothing bypasses the OS flow.
 *
 * Classified driver-INDEPENDENT (never used the driver; the classification
 * predates the removal): enumeration (cr_enumerate.cpp - WMI topology),
 * health (cr_health.cpp - direct SMART), BitLocker volume state, destination
 * arming (volume_extent(s)/destination_volume_letter), and the post-apply
 * layout refresh (refresh_disk_properties). Disk images are file-backed by
 * design (S-030, cr_image.h).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TRANSPORT_H_
#define CR_TRANSPORT_H_

#include "cr/storage/cr_device.h"
#include "cr/storage/cr_image.h"
#include "cr/storage/cr_slice.h"
#include "cr/repair/cr_rawfixer.h"
#include "cr/apply/cr_apply.h"
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace win {

/* ---------------------------------------------------------------------------
 * The privilege precondition (the direct-access replacement of the
 * driver-era status-query gate). is_elevated() (cr_elevate.h) probes the
 * process token's TokenElevation: TRUE for an elevated token, FALSE for
 * standard users AND non-elevated admins (raw \\.\PhysicalDriveN access
 * needs the ELEVATED token, not mere group membership). Every data-path
 * consumer checks this FIRST and refuses with ERR_ELEVATION_REQUIRED
 * (0x500C0001) - the single refusal class of the direct era.
 * ------------------------------------------------------------------------- */

/* S-030 disk images: the platform-owned ImageFileIo (CreateFileW-backed)
 * used by rrecov --image; feeds cr::storage::open_image. File-backed by
 * classification (images never touch raw disks). */
storage::ImageFileIo& image_file_io();

/* Whole-disk read view: \\.\PhysicalDriveN opened GENERIC_READ, share
 * R|W|D (the exact open posture the grammar path used). The returned
 * device serves every engine read through the 64 KiB envelope with true
 * byte counts. A non-elevated token is the caller's precondition to check
 * (require_privilege in rrecov.cpp / the cr_ui open impls); an open
 * failure under an elevated token is a TARGET error: ERR_OPEN_FAILED
 * (absent disk number, protected device, or a busy disk). */
std::unique_ptr<IStorageDevice> open_disk(u32 disk, u32* err);

/* CONTROL-PATH whole-disk open (\\.\PhysicalDriveN, read-only share) - used
 * by ENUMERATION only (cr_enumerate.cpp; the og's own enumeration never
 * needed more than this). Same mechanism as the data path; kept as the
 * classified control-consumer entry point. */
std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32* err);

/* Write-capable whole-disk view for the repair engine (RawFixer):
 * \\.\PhysicalDriveN opened GENERIC_READ|GENERIC_WRITE. Writes are staged
 * through a sector-aligned buffer when the caller's is unaligned (the
 * direct-write rule - the storage stack requires sector-aligned buffers
 * on raw handles). The S-010 quiesce chain (LOCK -> dismount -> offline,
 * restore on release) guards every write to a mounted disk. Open the read
 * view separately with open_disk() for analyze(). */
std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, u32* err);

/* S-029 (apply): the APPLY-capable write view (lost-partition table
 * writes). Same direct transport posture as open_disk_rw; the returned
 * device additionally carries the corpus PartitionRecovery device-control
 * chain (IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100, IOCTL_DISK_SET_DRIVE_LAYOUT
 * 0x7C010, the 0x7C058 0x18-byte modern record, IOCTL_DISK_UPDATE_PROPERTIES
 * 0x70140 with the 3x Sleep(1000) retry tail - decompile cites in
 * cr_apply.h), issued directly on the \\.\PhysicalDriveN handle (the driver
 * merely passed these through; refresh_disk_properties is the in-tree proof
 * the direct handle accepts them). Open the read view with open_disk() for
 * planning. */
std::unique_ptr<cr::apply::IApplyDevice> open_disk_apply_rw(u32 disk, u32* err);

/* S-029 (apply): IOCTL_DISK_UPDATE_PROPERTIES on \\.\PhysicalDriveN with
 * the corpus settle pattern (3 x Sleep(1000) retries, PartitionRecovery
 * PR:46873-46880) - after a table write the OS must re-read the layout or
 * the new entry stays invisible to the volume manager. Returns OK when
 * the refresh was accepted (or the settle exhausted - the table write
 * itself already succeeded; the refresh is best-effort by contract, the
 * caller reports it, never fails the apply on it). */
u32 refresh_disk_properties(u32 disk);

/* Full extent of the volume containing a drive letter: disk number, byte
 * offset on that disk, byte length (used for recovery-overlap refusal).
 * N-013: reports the FIRST extent only - kept for the source-disk lookup
 * call sites; destination arming must use volume_extents() (all spans). */
bool volume_extent(wchar_t letter, u32* disk, u64* start, u64* len);

/* N-013: ALL disk extents of the volume containing a drive letter. A
 * spanned/RAID destination has more than one extent; first-extent-only
 * arming (the pre-N-013 behavior) misses overlap on the second+ spans.
 * The IOCTL buffer holds 8 extents; a volume reporting more is truncated
 * to the first 8 (documented cap - the kernel rarely exceeds 8 on real
 * spanned volumes). Returns false if the volume cannot be opened or
 * reports none. */
struct VolExtent {
    u32 disk;
    u64 start;
    u64 len;
};
bool volume_extents(wchar_t letter, std::vector<VolExtent>* out);

/* N-013: resolve ANY local destination path (relative, junction, mount
 * point) to the volume that CONTAINS it. GetFinalPathNameByHandleW
 * resolves reparse points (a junction into another volume yields the
 * TARGET's volume - the og compares the resolved placement, never the
 * junction's host); GetVolumePathNameW then maps the final path to its
 * volume root. Network paths return false (the og's PathIsNetworkPathW
 * branch: cannot be evaluated, never armed). If the path cannot be
 * opened (not yet created), falls back to GetVolumePathNameW on the path
 * itself without reparse resolution. */
bool destination_volume_letter(const std::wstring& path, wchar_t* letter);

/* ---- N-125: the drive-disconnect classification ----
 * The og detects target removal and offers a reconnect-then-continue
 * flow ("Drive Disconnected - Unable to continue scanning. Reconnect
 * your drive and try again." Retry/Cancel; UIUX/08:39-47 - the same
 * triad for NAS/Linux/Cloud offline). The core's engine-side contract:
 * the device-gone Win32 GLE family classifies as
 * ERR_DEVICE_DISCONNECTED (a distinct code, not a read-failure storm),
 * and the documented reattach flow is the session-checkpoint resume
 * (the machinery exists; hosts detect the code, wait for reattach in
 * their own loop, then resume). The classification is a pure predicate
 * over the numeric GLE so BOTH CI lanes compile and test it (the
 * M-030 cr_winenv.h pattern); the engine retry window stays a
 * registered deliberate difference (PARITY-DELTAS D-121). */
namespace disconnect {
/* Win32 GLEs (numeric - no windows.h dependency): the device-gone set.
 * ERROR_NOT_READY(21) covers not-ready removable media; ERROR_IO_
 * DEVICE(1117)/ERROR_CRC(23) and friends stay read failures (the
 * dying-disk storm keeps the old semantics - the classification must
 * be precise, not a catch-all). */
constexpr unsigned kDevNotExist = 55;     /* ERROR_DEV_NOT_EXIST */
constexpr unsigned kNotReady = 21;        /* ERROR_NOT_READY */
constexpr unsigned kMediaChanged = 1110;  /* ERROR_MEDIA_CHANGED */
constexpr unsigned kNoMedia = 1112;       /* ERROR_NO_MEDIA_IN_DRIVE */
constexpr unsigned kDeviceRemoved = 1619; /* ERROR_DEVICE_REMOVED */

inline bool gle_is_disconnect(unsigned gle) {
    return gle == kDevNotExist || gle == kNotReady || gle == kMediaChanged || gle == kNoMedia ||
           gle == kDeviceRemoved;
}
} // namespace disconnect

} // namespace win
} // namespace cr

#endif // CR_TRANSPORT_H_
