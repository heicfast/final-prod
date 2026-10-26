/* winioctl.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale; the
 * structure layouts below follow the SDK's authoritative shapes (the
 * STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR field order is load-bearing for
 * the 512e/4Kn geometry probe - see cr_transport.cpp probe_geometry).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include "windows.h"

typedef unsigned char UCHAR;
typedef enum _STORAGE_BUS_TYPE { BusTypeUnknown = 0, BusTypeScsi, BusTypeAtapi, BusTypeAta, BusType1394, BusTypeSsa, BusTypeFibre, BusTypeUsb, BusTypeRAID, BusTypeiScsi, BusTypeSas, BusTypeSata, BusTypeSd, BusTypeMmc, BusTypeVirtual, BusTypeFileBackedVirtual, BusTypeSpaces, BusTypeNvme, BusTypeSCM } STORAGE_BUS_TYPE;

struct GET_LENGTH_INFORMATION { LARGE_INTEGER Length; };
struct DISK_GEOMETRY { LARGE_INTEGER Cylinders; DWORD TracksPerCylinder; DWORD SectorsPerTrack; DWORD BytesPerSector; };
struct DISK_PARTITION_INFORMATION { LARGE_INTEGER PartitionOffset; LARGE_INTEGER PartitionLength; DWORD HiddenSectors; DWORD PartitionNumber; };
struct DISK_GEOMETRY_EX { DISK_GEOMETRY Geometry; LARGE_INTEGER DiskSize; unsigned char Data[1]; };

enum STORAGE_PROPERTY_ID { StorageDeviceProperty = 0, StorageAccessAlignmentProperty = 6, StorageDeviceSeekPenaltyProperty = 7, StorageDeviceProtocolSpecificProperty = 49 };
enum STORAGE_QUERY_TYPE { PropertyStandardQuery = 0, PropertyExistsQuery = 1 };
struct STORAGE_PROPERTY_QUERY { STORAGE_PROPERTY_ID PropertyId; STORAGE_QUERY_TYPE QueryType; unsigned char AdditionalParameters[1]; };

struct DEVICE_SEEK_PENALTY_DESCRIPTOR { DWORD Version; DWORD Size; BOOLEAN IncursSeekPenalty; };
struct STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR { DWORD Version; DWORD Size; DWORD BytesPerCacheLine; DWORD BytesOffsetForCacheAlignment; DWORD BytesPerLogicalSector; DWORD BytesPerPhysicalSector; DWORD BytesOffsetForSectorAlignment; };
struct STORAGE_DEVICE_DESCRIPTOR { DWORD Version; DWORD Size; UCHAR DeviceType; UCHAR DeviceTypeModifier; BOOLEAN RemovableMedia; BOOLEAN CommandQueueing; DWORD VendorIdOffset; DWORD ProductIdOffset; DWORD ProductRevisionOffset; DWORD SerialNumberOffset; STORAGE_BUS_TYPE BusType; DWORD RawPropertiesLength; UCHAR RawDeviceProperties[1]; };

struct DISK_EXTENT { LARGE_INTEGER StartingOffset; LARGE_INTEGER ExtentLength; DWORD DiskNumber; };
struct VOLUME_DISK_EXTENTS { DWORD NumberOfDiskExtents; DISK_EXTENT Extents[1]; };

#define IOCTL_DISK_GET_LENGTH_INFO 0x0007405CUL
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX 0x000700A0UL
#define IOCTL_STORAGE_QUERY_PROPERTY 0x002D1400UL
#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS 0x00560000UL
#define FSCTL_LOCK_VOLUME 0x00090018UL
#define FSCTL_DISMOUNT_VOLUME 0x00090020UL
#define FSCTL_IS_VOLUME_MOUNTED 0x00090048UL

inline BOOL DeviceIoControl(HANDLE, DWORD, void*, DWORD, void*, DWORD, DWORD*, OVERLAPPED*) { return 1; }

/* NVMe health-log route (cr_health.cpp read_nvme_health) */
typedef enum _STORAGE_PROTOCOL_TYPE { ProtocolTypeUnknown = 0, ProtocolTypeNvme = 3, ProtocolTypeAta, ProtocolTypeScsi } STORAGE_PROTOCOL_TYPE;
typedef enum _STORAGE_PROTOCOL_NVME_DATA_TYPE { NVMeDataTypeUnknown = 0, NVMeDataTypeIdentify, NVMeDataTypeLogPage, NVMeDataTypeFeature } STORAGE_PROTOCOL_NVME_DATA_TYPE;

typedef struct _STORAGE_PROTOCOL_SPECIFIC_DATA {
    STORAGE_PROTOCOL_TYPE ProtocolType;
    DWORD DataType;
    DWORD ProtocolDataRequestValue;
    DWORD ProtocolDataRequestSubValue;
    DWORD ProtocolDataOffset;
    DWORD ProtocolDataLength;
    DWORD FixedProtocolReturnData;
    DWORD Reserved[3];
} STORAGE_PROTOCOL_SPECIFIC_DATA, *PSTORAGE_PROTOCOL_SPECIFIC_DATA;
#define IOCTL_DISK_UPDATE_PROPERTIES 0x00070140UL

/* N-071 (disk-interface pass): the device-number query. */
typedef DWORD DEVICE_TYPE;
typedef struct _STORAGE_DEVICE_NUMBER {
    DEVICE_TYPE DeviceType;
    DWORD DeviceNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;
#define IOCTL_STORAGE_GET_DEVICE_NUMBER 0x002D1080UL
