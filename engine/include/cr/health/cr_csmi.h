/*
 * cr_csmi.h - CSMI (Common Storage Management Interface) SCSI-miniport
 * ATA pass-through: the og DiskHealth companion's Intel-RST/AMD-rcraid
 * SMART transport, wire-decoded from the corpus decompile.
 *
 * Corpus: re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c
 * (N-032 evidence; first-hand decode this session):
 *   - FUN_00425450 "GetSmartAttributeCsmi" transport: CreateFileW of the
 *     \\.\Scsi%u: port device, DeviceIoControl 0x0004D008
 *     (IOCTL_SCSI_MINIPORT), request = SRB_IO_CONTROL header
 *     (HeaderLength 0x1C, Signature "CSMISAS", Timeout 60, ControlCode
 *     0x19, ReturnCode 0, Length = data + 0x8C) followed by the vendor
 *     fixed area (phy/port/SAS address, the 20-byte Register-FIS-to-
 *     device task file, the 0x11 vendor field, the data length).
 *   - SMART commands carry the classic ATA magic LBA (cyl low 0x4F,
 *     cyl high 0xC2, device 0xA0 -> little-endian dword 0xA0C24F00 at
 *     task-file offset +4); every other command (IDENTIFY 0xEC) uses
 *     0xA0000000 (zeros + device 0xA0).
 *   - DoIdentifyDeviceCsmi = op -0x14 (command 0xEC), GetSmartAttribute/
 *     ThresholdCsmi = op -0x50 (command 0xB0, features 0xD0/0xD1).
 *   - The 512-byte response data lands at request offset 0xA4.
 *   - Transport-unavailable errors: DeviceIoControl FALSE with
 *     GetLastError() in {ERROR_INVALID_FUNCTION (1),
 *     ERROR_NOT_SUPPORTED (0x32), ERROR_DEV_NOT_EXIST (0x37)}.
 *
 * DELTA (documented, see the N-032 Resolution + PARITY-DELTAS): the og
 * routes CC 0x19 through its companion-side RAID census (GET_DRIVER_INFO
 * / GET_RAID_INFO / GET_RAID_CONFIG / GET_PHY_INFO walks; AMD "rcraid" +
 * Intel RST), which fills phy/port/target-SAS-address from the array
 * inventory. Our channel routes through IOCTL_SCSI_GET_ADDRESS (the og's
 * own address probe, DiskHealth FUN_00417750 :12254) - the census is
 * companion device-manager machinery. The og's LSILOGIC-signature
 * sibling transport (FUN_00426040) and its SMART auto-enable write
 * (command 0xB0 feature 0xD8, case 0x16 :4427-4438) are NOT ported:
 * the latter is a device write, and this module is report-only.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CSMI_H_
#define CR_CSMI_H_

#include "../cr_types.h"
#include <cstddef>

namespace cr {
namespace health {
namespace csmi {

/* --- wire constants (og-exact, DiskHealth FUN_00425450) --- */

inline constexpr u32 kIoctlScsiMiniport =
    0x0004D008u; /* CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, R|W) */
inline constexpr u32 kIoctlScsiGetAddress =
    0x00041018u; /* the og's own address probe (FUN_00417750) */

inline constexpr u32 kSrbHeaderLength = 0x1Cu; /* SRB_IO_CONTROL size */
inline constexpr char kSignature[8] = {'C', 'S', 'M', 'I', 'S', 'A', 'S', '\0'};
inline constexpr u32 kTimeoutSeconds = 60u; /* 0x3C */
inline constexpr u32 kControlCodeRstPassthru =
    0x19u; /* Intel RST / AMD rcraid physical-drive ATA pass-through */

/* request buffer layout (offsets from the SRB_IO_CONTROL start) */
inline constexpr size_t kOffHeaderLength = 0x00; /* u32 = 0x1C */
inline constexpr size_t kOffSignature = 0x04;    /* char[8] "CSMISAS" */
inline constexpr size_t kOffTimeout = 0x0C;      /* u32 = 60 */
inline constexpr size_t kOffControlCode = 0x10;  /* u32 = 0x19 */
inline constexpr size_t kOffReturnCode = 0x14;   /* u32 = 0 */
inline constexpr size_t kOffLength = 0x18;       /* u32 = kDataLength + 0x8C */
inline constexpr size_t kOffPhy = 0x1C;          /* u8 phy identifier */
inline constexpr size_t kOffPort = 0x1D;         /* u8 port identifier */
inline constexpr size_t kOffSasAddress = 0x20;   /* u64 target SAS address */
inline constexpr size_t kOffFisType = 0x2C;      /* u16 0x8027: Register-FIS-to-device, C bit */
inline constexpr size_t kOffCommand = 0x2E;      /* u8 ATA command */
inline constexpr size_t kOffFeature = 0x2F;      /* u8 ATA features */
inline constexpr size_t kOffLbaDevice = 0x30;    /* u32 LBA low/mid/high + device */
inline constexpr size_t kOffSectorCount = 0x38;  /* u8 sector count */
inline constexpr size_t kOffVendorField = 0x40;  /* u32 0x11 (ATA ops; 4 for firmware ops) */
inline constexpr size_t kOffDataLength = 0x44;   /* u32 = 512 */

inline constexpr size_t kDataLength = 512;        /* the SMART/IDENTIFY sector */
inline constexpr size_t kFixedRequestSize = 0xA8; /* header + vendor fixed area */
inline constexpr size_t kRequestSize = kFixedRequestSize + kDataLength; /* 0x2A8 */
inline constexpr size_t kResponseDataOffset =
    0xA4; /* response data position (FUN_00425450 copy at +0x29 dwords) */

/* Task-file constants: the Register-FIS-to-device prologue and the
 * classic SMART magic LBA (cyl low 4Fh, cyl high C2h, device A0h). */
inline constexpr u16 kFisHostToDevice = 0x8027u; /* FIS type 0x27 | C-bit 0x80 */
inline constexpr u8 kAtaCmdIdentify = 0xEC;
inline constexpr u8 kAtaCmdSmart = 0xB0;
inline constexpr u32 kLbaSmartMagic = 0xA0C24F00u; /* SMART: LBA 00/4F/C2, device A0 */
inline constexpr u32 kLbaZeroDevice = 0xA0000000u; /* others: LBA 0, device A0 */

/* One CC-0x19 ATA request: routing + task file. */
struct AtaRequest {
    u8 phy = 0;          /* phy identifier (og: census-filled) */
    u8 port = 0;         /* port identifier (og: census-filled) */
    u64 sas_address = 0; /* target SAS address (og: census-filled) */
    u8 command = 0;      /* 0xEC IDENTIFY / 0xB0 SMART */
    u8 feature = 0;      /* 0xD0 READ DATA / 0xD1 READ THRESHOLDS */
    u8 sector_count = 0;
};

/* Build the og-exact request buffer (kRequestSize bytes). Returns the
 * number of bytes written, or 0 when cap is smaller than kRequestSize. */
size_t build_ata_request(const AtaRequest& req, u8* buf, size_t cap);

/* Extract the 512-byte response data at the og's +0xA4 offset. False
 * when the buffer cannot hold the data area. */
bool extract_response_data(const u8* buf, size_t len, u8* out512);

/* The og's transport-unavailable classification: a failed miniport call
 * with one of these GetLastError() codes means "this controller does not
 * speak CC 0x19", not "the disk failed". */
inline constexpr u32 kErrInvalidFunction = 1;
inline constexpr u32 kErrNotSupported = 0x32;
inline constexpr u32 kErrDevNotExist = 0x37;
inline constexpr bool transport_unavailable(u32 last_error) {
    return last_error == kErrInvalidFunction || last_error == kErrNotSupported ||
           last_error == kErrDevNotExist;
}

} // namespace csmi
} // namespace health
} // namespace cr

#endif // CR_CSMI_H_
