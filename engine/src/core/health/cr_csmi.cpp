/* cr_csmi.cpp - CSMI CC-0x19 ATA pass-through wire format (pure core).
 *
 * Corpus: DiskHealth_decompiled.c FUN_00425450 (see cr_csmi.h for the
 * full decode). Re-implemented from understanding; nothing pasted.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_csmi.h"
#include <cstring>

namespace cr {
namespace health {
namespace csmi {

namespace {

void put_u16(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}

void put_u32(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

void put_u64(u8* p, u64 v) {
    for (u32 i = 0; i < 8; ++i)
        p[i] = u8(v >> (8 * i));
}

} // namespace

size_t build_ata_request(const AtaRequest& req, u8* buf, size_t cap) {
    if (buf == nullptr || cap < kRequestSize)
        return 0;
    std::memset(buf, 0, kRequestSize);

    /* SRB_IO_CONTROL header (FUN_00425450: *lp=0x1c, strncpy_s 8 "CSMISAS",
     * [3]=0x3c, [4]=control code, [5]=0, [6]=length) */
    put_u32(buf + kOffHeaderLength, kSrbHeaderLength);
    std::memcpy(buf + kOffSignature, kSignature, 8);
    put_u32(buf + kOffTimeout, kTimeoutSeconds);
    put_u32(buf + kOffControlCode, kControlCodeRstPassthru);
    put_u32(buf + kOffReturnCode, 0);
    put_u32(buf + kOffLength, u32(kDataLength + 0x8C));

    /* vendor fixed area: routing (og: RAID-census filled) */
    buf[kOffPhy] = req.phy;
    buf[kOffPort] = req.port;
    put_u64(buf + kOffSasAddress, req.sas_address);

    /* Register-FIS-to-device task file: type 0x27 + C bit, command,
     * features, LBA/device dword (SMART magic 4F/C2/A0 vs zeros+A0),
     * sector count. */
    put_u16(buf + kOffFisType, kFisHostToDevice);
    buf[kOffCommand] = req.command;
    buf[kOffFeature] = req.feature;
    put_u32(buf + kOffLbaDevice, req.command == kAtaCmdSmart ? kLbaSmartMagic : kLbaZeroDevice);
    buf[kOffSectorCount] = req.sector_count;

    /* the og's vendor field (0x11 for ATA ops) + data length */
    put_u32(buf + kOffVendorField, 0x11u);
    put_u32(buf + kOffDataLength, u32(kDataLength));
    return kRequestSize;
}

bool extract_response_data(const u8* buf, size_t len, u8* out512) {
    if (buf == nullptr || out512 == nullptr || len < kResponseDataOffset + kDataLength)
        return false;
    std::memcpy(out512, buf + kResponseDataOffset, kDataLength);
    return true;
}

} // namespace csmi
} // namespace health
} // namespace cr
