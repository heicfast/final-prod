/*
 * cr_transport.h - the Windows transport layer: RDA driver grammar path
 * ONLY for the data path (driver-only architecture, docs/DRIVER-REQUIRED.md),
 * plus the driver-status query surface (cr_driver_status.h).
 *
 * The reference duality is GONE by deliberate divergence: the og gates on
 * environment (WinPE -> direct) and silently NULLs failed driver opens
 * (corpus Device.mo; REFERENCE-DRIVER-BEHAVIOR section 4); our core had
 * added a failure-gated direct fallback on top of it (CORE-DISK-ACCESS
 * section d, F1-F12) - that superset is REMOVED (PLAN-DRIVER-ONLY section 4,
 * PARITY-DELTAS). The driver is a prerequisite: every data-path open below
 * fails fast with its ERR_DRIVER_* class when the driver is absent, not
 * started, unauthorized, wrong-version, or errored.
 *
 * Classified driver-INDEPENDENT (never used the driver; og evidence per row
 * in docs/DRIVER-REQUIRED.md): enumeration (cr_enumerate.cpp - og
 * GetLogicalDrives/WMI topology), health (cr_health.cpp - og DiskHealth
 * direct SMART), BitLocker volume state, destination arming
 * (volume_extent(s)/destination_volume_letter), and the post-apply layout
 * refresh (refresh_disk_properties - og PR:46873-46880). Disk images are
 * driver-independent by design (S-030, cr_image.h).
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
#include "cr/win/cr_driver_status.h"
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace win {

/* ---------------------------------------------------------------------------
 * Driver-status surface (docs/DRIVER-REQUIRED.md; ABI: cr_ui_driver_status;
 * CLI: rrecov driver). Fills every facet, then classify via
 * driver_state()/driver_state_error() (cr_driver_status.h). Threading:
 * synchronous, thread-safe (opens its own SCM/database and device handles,
 * closes them before returning); no state is retained between calls.
 * Returns OK when the query itself completed (state is in *out - including
 * non-Ready states); returns ERR_BAD_PARAMETER on null arguments. The
 * QUERY never fails on driver condition - it REPORTS it (that is the whole
 * point of the diagnostic surface).
 * ------------------------------------------------------------------------- */
u32 query_driver_status(const wchar_t* service_name, DriverStatus* out);

/* Open the RDA driver's 3-segment EPMNtDrv grammar path
 * \\.\SVC\<token>\<disk>\<part> (FB-L01: no cascade - the 2-segment and
 * Volume\<n> variants are separate grammar functions, TRANSPORT-CONTRACT
 * 2.1; the structured-open RdaClient was deleted by N-041 - zero callers
 * since introduction). The DATA-PATH open: requires the driver (the caller
 * checks driver state first - see require-driver usage in rrecov.cpp /
 * open_*_impl in cr_ui.cpp); returns null + err (ERR_OPEN_FAILED and the
 * CreateFile GLE via rda probe diagnostics) when the driver is usable but
 * the TARGET cannot be opened. */
std::unique_ptr<IStorageDevice> open_rda_partition(const wchar_t* service_name, u32 disk, u32 part,
                                                   u32* err);
std::unique_ptr<IStorageDevice> open_rda_volume(const wchar_t* service_name, u32 volume, u32* err);

/* S-030 disk images: the platform-owned ImageFileIo (CreateFileW-backed)
 * used by rrecov --image; feeds cr::storage::open_image. Driver-independent
 * by classification (docs/DRIVER-REQUIRED.md). */
storage::ImageFileIo& image_file_io();

/* CONTROL-PATH whole-disk open (\\.\PhysicalDriveN, read-only share) - used
 * by ENUMERATION only (cr_enumerate.cpp; the og's own enumeration never
 * touches the driver, getdriverinfo:20-29). NOT a data-path open: the scan/
 * recover/fix/apply/bench data path is open_rda_partition / open_disk below.
 * (Kept for the classified-(b) control consumers; open_harddisk_volume and
 * the data-path direct fallbacks were removed with the driver-only
 * architecture.) */
std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32* err);

/* Whole-disk view with probed geometry over the RDA grammar path. The
 * driver is REQUIRED: the grammar open is the only leg (the direct
 * \\.\PhysicalDriveN fallback and the WinPE gate are REMOVED -
 * PLAN-DRIVER-ONLY R2/R12). Callers that need a refusal with the specific
 * ERR_DRIVER_* class check driver state first (query_driver_status); an
 * open failure here with a Ready driver is a target error (ERR_OPEN_FAILED
 * with the GLE), not a driver-state refusal. */
std::unique_ptr<IStorageDevice> open_disk(u32 disk, const wchar_t* service, u32* err);

/* Write-capable whole-disk view for the repair engine (RawFixer): an RDA
 * handle opened GENERIC_READ|GENERIC_WRITE (the driver forwards IRP_MJ_WRITE
 * with byte-offset semantics). Writes are staged through a sector-aligned
 * buffer when the caller's is unaligned (the direct-path alignment rule -
 * retained defensively for exotic alignment even though the RDA path
 * MDL-locks any alignment); reads honor the F11 Information=Length*512
 * contract on the RDA path. Open the read view separately with open_disk()
 * for analyze(). Driver REQUIRED (the direct RW fallback is REMOVED -
 * PLAN-DRIVER-ONLY R3). */
std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, const wchar_t* service, u32* err);

/* S-029 (apply): IOCTL_DISK_UPDATE_PROPERTIES on \\.\PhysicalDriveN with
 * the corpus settle pattern (3 x Sleep(1000) retries, PartitionRecovery
 * PR:46873-46880) - after a table write the OS must re-read the layout or
 * the new entry stays invisible to the volume manager. Returns OK when
 * the refresh was accepted (or the settle exhausted - the table write
 * itself already succeeded; the refresh is best-effort by contract, the
 * caller reports it, never fails the apply on it). Classified (b): the
 * og issues this device-management IOCTL on its own direct handle. */
u32 refresh_disk_properties(u32 disk);

/* S-029: the APPLY-capable write view (lost-partition table writes).
 * Same RDA-only transport posture as open_disk_rw; the returned device
 * additionally carries the corpus PartitionRecovery device-control chain
 * (IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100, IOCTL_DISK_SET_DRIVE_LAYOUT
 * 0x7C010, the 0x7C058 0x18-byte modern record, IOCTL_DISK_UPDATE_PROPERTIES
 * 0x70140 with the 3x Sleep(1000) retry tail - decompile cites in
 * cr_apply.h) - issued through the RDA bound handle (the driver passes
 * non-session IOCTLs through to the bound target, DRIVER-CONTRACT 3).
 * Open the read view with open_disk() for planning. Driver REQUIRED. */
std::unique_ptr<cr::apply::IApplyDevice> open_disk_apply_rw(u32 disk, const wchar_t* service,
                                                            u32* err);
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
constexpr unsigned kDevNotExist = 55;       /* ERROR_DEV_NOT_EXIST */
constexpr unsigned kNotReady = 21;          /* ERROR_NOT_READY */
constexpr unsigned kMediaChanged = 1110;    /* ERROR_MEDIA_CHANGED */
constexpr unsigned kNoMedia = 1112;         /* ERROR_NO_MEDIA_IN_DRIVE */
constexpr unsigned kDeviceRemoved = 1619;   /* ERROR_DEVICE_REMOVED */

inline bool gle_is_disconnect(unsigned gle) {
    return gle == kDevNotExist || gle == kNotReady || gle == kMediaChanged || gle == kNoMedia ||
           gle == kDeviceRemoved;
}
} // namespace disconnect

} // namespace win
} // namespace cr



#endif // CR_TRANSPORT_H_
