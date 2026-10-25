/* ntddscsi.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale.
 *
 * ATA_PASS_THROUGH_EX layout is LOAD-BEARING (cr_health.cpp
 * ata_passthrough relies on DataBufferOffset being ULONG_PTR-width on
 * x64 and indexes CurrentTaskFile[0..7]); the shape below matches the
 * SDK header in the fields the code touches.
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
    USHORT Length;             /* sizeof(ATA_PASS_THROUGH_EX) */
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
