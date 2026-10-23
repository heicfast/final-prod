/*
 * cr_transport.h - Windows transports: RDA driver client + direct handles.
 *
 * Reference duality (corpus Device.mo): \\.\EPRDTDRV\<d>\<p> driver path for
 * locked/damaged volumes + direct \\.\PhysicalDriveN / HarddiskVolumeN
 * fallbacks. Our RDA driver speaks the same grammar under its service name.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TRANSPORT_H_
#define CR_TRANSPORT_H_

#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"
#include "cr/repair/cr_rawfixer.h"
#include <memory>
#include <string>

namespace cr {
namespace win {

/* Open the RDA driver under the given service name (default RDA, matching
 * the deployed driver). open_rda_partition opens the 3-segment EPMNtDrv
 * grammar path \\.\SVC\<token>\<disk>\<part> ONLY (FB-L01: no cascade -
 * the 2-segment and Volume\<n> variants are separate functions /
 * RdaClient::open_disk_grammar / open_volume_grammar, TRANSPORT-CONTRACT
 * §2.1). Returns null + err when the driver is unavailable (caller falls
 * back to direct handles). */
std::unique_ptr<IStorageDevice> open_rda_partition(const wchar_t* service_name, u32 disk, u32 part,
                                                   u32* err);
std::unique_ptr<IStorageDevice> open_rda_volume(const wchar_t* service_name, u32 volume, u32* err);

/* Direct Win32 handles: \\.\PhysicalDriveN (whole disk, read-only share) and
 * \\.\HarddiskVolumeN (volume handle). */
std::unique_ptr<IStorageDevice> open_physical_drive(u32 disk, u32* err);
std::unique_ptr<IStorageDevice> open_harddisk_volume(u32 volume, u32* err);

/* Whole-disk view with probed geometry + optional RDA preference. */
std::unique_ptr<IStorageDevice> open_disk(u32 disk, bool prefer_rda, const wchar_t* service,
                                          u32* err);

/* Write-capable whole-disk view for the repair engine (RawFixer): an RDA
 * handle opened GENERIC_READ|GENERIC_WRITE when the driver is available
 * (the driver forwards IRP_MJ_WRITE with byte-offset semantics), else a
 * direct \\.\PhysicalDriveN handle. Writes are staged through a
 * sector-aligned buffer (raw direct-handle writes require buffer
 * alignment); reads honor the F11 Information=Length*512 contract on the
 * RDA path. Open the read view separately with open_disk() for analyze(). */
std::unique_ptr<cr::repair::IWriteDevice> open_disk_rw(u32 disk, bool prefer_rda,
                                                       const wchar_t* service, u32* err);

/* Map a volume letter (e.g. L'C') to its disk+partition (IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
 * for disk number + extent start, then the disk's parsed table for the 1-based partition
 * number - FB-L02 comment fix).
 *
 * W4 KEEP-note (AUDIT-ARCH 8a #4, plan ruling): currently 0 callers - the
 * CLI uses volume_extent instead. Retained as the natural preflight for
 * the writer's destination-overlap refusal (destination letter -> source
 * disk comparison); wiring candidate for the writer hardening track. */
bool volume_letter_to_disk(wchar_t letter, u32* disk, u32* part);

/* Full extent of the volume containing a drive letter: disk number, byte
 * offset on that disk, byte length (used for recovery-overlap refusal). */
bool volume_extent(wchar_t letter, u32* disk, u64* start, u64* len);

/* Device existence probes used by enumeration. When the control-device
 * open fails, *last_error (if given) receives the CreateFile GetLastError
 * code - the diagnostic for the runner mystery (service running, \\.\Rda
 * unopenable). */
bool rda_available(const wchar_t* service_name, u32* last_error = nullptr);

} // namespace win
} // namespace cr

#endif // CR_TRANSPORT_H_
