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
#include <memory>
#include <string>

namespace cr {
namespace win {

/* Open the RDA driver under the given service name (default RDA, matching
 * the deployed driver). Tries: \\.\SVC\<token>\<disk>\<part>,
 * \\.\SVC\<token>\<disk>, \\.\SVC\<token>\Volume\<n>. Returns null + err when
 * the driver is unavailable (caller falls back to direct handles). */
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

/* Map a volume letter (e.g. L'C') to its disk+partition (volume-to-disk
 * correlation via IOCTL_STORAGE_GET_DEVICE_NUMBER + extents). */
bool volume_letter_to_disk(wchar_t letter, u32* disk, u32* part);

/* Full extent of the volume containing a drive letter: disk number, byte
 * offset on that disk, byte length (used for recovery-overlap refusal). */
bool volume_extent(wchar_t letter, u32* disk, u64* start, u64* len);

/* Device existence probes used by enumeration. */
bool rda_available(const wchar_t* service_name);

} // namespace win
} // namespace cr

#endif // CR_TRANSPORT_H_
