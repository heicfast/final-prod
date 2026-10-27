/*
 * cr_transport.h - Windows transport: the RDA kernel driver, REQUIRED.
 *
 * Mandatory-driver session (2026-10-27, user directive): the RDA driver
 * is the ONLY raw-disk transport. The reference's duality (Device.mo:
 * driver path + direct \\.\PhysicalDriveN fallback) is DELIBERATELY NOT
 * reproduced — every read/write path rides the driver grammar, and its
 * absence is the distinct ERR_DRIVER_REQUIRED refusal (see
 * docs/PARITY-DELTAS.md D-FALL for the rationale and the divergence
 * registration). Enumeration/geometry/health METADATA IOCTLs stay on
 * the OS storage stack (cr_enumerate/cr_health) — the disk LIST is
 * stack metadata, not sector reads; the og lists disks the same way.
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

/* Open the RDA driver under the given service name (default RDA, matching
 * the deployed driver). open_rda_partition opens the 3-segment EPMNtDrv
 * grammar path \\.\SVC\<token>\<disk>\<part> ONLY (FB-L01: no cascade -
 * the 2-segment and Volume\<n> variants are separate grammar functions,
 * TRANSPORT-CONTRACT §2.1; the structured-open RdaClient was deleted by
 * N-041 - zero callers since introduction). Returns null + err when the
 * grammar open fails (probe rda_available first to distinguish an absent
 * driver from an open failure). */
std::unique_ptr<IStorageDevice> open_rda_partition(const wchar_t* service_name, u32 disk, u32 part,
                                                   u32* err);
std::unique_ptr<IStorageDevice> open_rda_volume(const wchar_t* service_name, u32 volume, u32* err);

/* S-030 disk images: the platform-owned ImageFileIo (CreateFileW-backed)
 * used by rrecov --image; feeds cr::storage::open_image. */
storage::ImageFileIo& image_file_io();

/* Whole-disk view through the RDA grammar with probed geometry. THE
 * product transport - there is no direct-handle fallback. When the
 * driver's control device cannot be opened (not installed / not
 * started / blocked by signing policy) returns null with
 * *err = ERR_DRIVER_REQUIRED; a grammar-open failure on a live driver
 * returns the open error (ERR_OPEN_FAILED et al). */
std::unique_ptr<IStorageDevice> open_disk(u32 disk, const wchar_t* service, u32* err);

/* Write-capable whole-disk view for the repair engine (RawFixer): an RDA
 * handle opened GENERIC_READ|GENERIC_WRITE (the driver forwards
 * IRP_MJ_WRITE with byte-offset semantics; the F11 Information=
 * Length*512 contract is honored on reads and writes). THE product
 * write transport - no direct fallback: an absent driver refuses with
 * ERR_DRIVER_REQUIRED, never a silently degraded write path. Writes
 * are staged through a sector-aligned buffer. Open the read view
 * separately with open_disk() for analyze(). */
std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, const wchar_t* service, u32* err);

/* S-029 (apply): IOCTL_DISK_UPDATE_PROPERTIES on \\.\PhysicalDriveN with
 * the corpus settle pattern (3 x Sleep(1000) retries, PartitionRecovery
 * PR:46873-46880) - after a table write the OS must re-read the layout or
 * the new entry stays invisible to the volume manager. Returns OK when
 * the refresh was accepted (or the settle exhausted - the table write
 * itself already succeeded; the refresh is best-effort by contract, the
 * caller reports it, never fails the apply on it). */
u32 refresh_disk_properties(u32 disk);

/* S-029: the APPLY-capable write view (lost-partition table writes).
 * Same mandatory-RDA transport and handle posture as open_disk_rw; the
 * returned device additionally carries the corpus PartitionRecovery
 * device-control chain (IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100,
 * IOCTL_DISK_SET_DRIVE_LAYOUT 0x7C010, the 0x7C058 0x18-byte modern
 * record, IOCTL_DISK_UPDATE_PROPERTIES 0x70140 with the 3x Sleep(1000)
 * retry tail - decompile cites in cr_apply.h). Open the read view with
 * open_disk() for planning. */
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

/* Device existence probe - the mandatory-driver gate. When the
 * control-device open fails, *last_error (if given) receives the
 * CreateFile GetLastError code (the diagnostic for a service that is
 * registered but not started: signing policy, missing binary, driver
 * load failure - the System event log carries the kernel-side reason). */
bool rda_available(const wchar_t* service_name, u32* last_error = nullptr);

} // namespace win
} // namespace cr

#endif // CR_TRANSPORT_H_
