/* ntddscsi.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale.
 *
 * ATA_PASS_THROUGH_EX layout is LOAD-BEARING (cr_health.cpp
 * ata_passthrough relies on DataBufferOffset being ULONG_PTR-width on
 * x64 and indexes CurrentTaskFile[0..7]); the shape below matches the
 * SDK header in the fields the code touches.
 *
 * SCSI_PASS_THROUGH_DIRECT / SCSI_ADDRESS are the SAT channel's shapes
 * (cr_health.cpp SatPassThroughChannel): Length, the address triple,
 * CdbLength/SenseInfoLength/DataIn, DataTransferLength/TimeOutValue,
 * the DataBuffer POINTER (8 bytes on x64 — the request embeds it, not an
 * offset), SenseInfoOffset and Cdb[16]; the field ORDER is load-bearing
 * exactly like ATA_PASS_THROUGH_EX above.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include "windows.h"

typedef unsigned short USHORT;
typedef USHORT* PUSHORT;

typedef struct _IDEREGS {
    UCHAR bFeaturesReg;
    UCHAR bSectorCountReg;
    UCHAR bSectorNumberReg;
    UCHAR bCylLowReg;
    UCHAR bCylHighReg;
    UCHAR bDriveHeadReg;
    UCHAR bCommandReg;
    UCHAR bReserved;
} IDEREGS, *PIDEREGS;

typedef struct _ATA_PASS_THROUGH_EX {
    USHORT Length; /* sizeof(ATA_PASS_THROUGH_EX) */
    USHORT AtaFlags;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR ReservedAsUchar;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG ReservedAsUlong;
    ULONG_PTR DataBufferOffset;
    UCHAR CurrentTaskFile[8];
} ATA_PASS_THROUGH_EX, *PATA_PASS_THROUGH_EX;

#define ATA_FLAGS_DATA_IN 0x02
#define ATA_FLAGS_DATA_OUT 0x04
#define ATA_FLAGS_DRDY_REQUIRED 0x01
#define ATA_FLAGS_48BIT_COMMAND 0x08
#define ATA_FLAGS_USE_DMA 0x10

#define IOCTL_ATA_PASS_THROUGH 0x0004D02CUL

/* ---- the SAT channel (cr_health.cpp SatPassThroughChannel) ---- */

typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT Length; /* sizeof(SCSI_PASS_THROUGH_DIRECT) */
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    void* DataBuffer; /* a POINTER (unlike ATA_PASS_THROUGH_EX's offset) */
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
} SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

typedef struct _SCSI_ADDRESS {
    ULONG Length;
    UCHAR PortNumber;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
} SCSI_ADDRESS, *PSCSI_ADDRESS;

#define SCSI_IOCTL_DATA_IN 0x01

/* The values the SDK defines (and the corpus decompile confirms:
 * DiskHealth_decompiled.c FUN_00419280 issues 0x4d004). */
#define IOCTL_SCSI_PASS_THROUGH_DIRECT 0x0004D004UL
#define IOCTL_SCSI_GET_ADDRESS 0x00041018UL
