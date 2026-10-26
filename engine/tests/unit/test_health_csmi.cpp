/* test_health_csmi.cpp - N-032 proofs: the CSMI CC-0x19 wire format
 * (og-exact against the DiskHealth decompile) and the multi-transport
 * SMART fallback chain (the og CAtaSmart accessor model).
 *
 * Fail-before: executed via the two-line chain revert documented in the
 * N-032 Resolution - the pre-fix chain shape [ata-passthrough,
 * smart-rcv-drive-data] returns ERR_HEALTH_UNAVAILABLE on the
 * RST-shaped mock; the three-transport chain reaches the CSMI channel.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_csmi.h"
#include "cr/health/cr_smart_ata.h"
#include "cr/cr_error.h"
#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>

namespace {

using cr::u16;
using cr::u32;
using cr::u64;
using cr::u8;
using cr::health::AtaChannel;
using cr::health::SmartInfo;

/* A scripted channel: a map (command, feature) -> 512-byte response. */
class ScriptedChannel final : public AtaChannel {
public:
    ScriptedChannel(const char* n, bool answer_identify) : name_(n), identify_(answer_identify) {}
    const char* name() const override { return name_; }
    bool issue(u8 feature, u8 count, u8 command, u8* out512) override {
        ++calls_;
        last_command_ = command;
        last_feature_ = feature;
        last_count_ = count;
        if (command == 0xEC)
            return identify_ && fill(out512, make_identify());
        if (command == 0xB0 && feature == 0xD0)
            return fill(out512, make_read_data());
        if (command == 0xB0 && feature == 0xD1)
            return fill(out512, make_thresholds());
        return false;
    }
    int calls_ = 0;
    u8 last_command_ = 0, last_feature_ = 0, last_count_ = 0;

private:
    static bool fill(u8* out512, const std::vector<u8>& page) {
        std::memset(out512, 0, 512);
        std::memcpy(out512, page.data(), page.size() < 512 ? page.size() : 512);
        return true;
    }
    /* IDENTIFY: ata version at word 80, model string at words 27-46
     * (byte-swapped pairs, space padded - the parser strips every space,
     * interior included: the S-024 model-string rule). */
    static std::vector<u8> make_identify() {
        std::vector<u8> p(512, 0);
        p[80 * 2] = 0x7E; /* ATA/ATAPI-7 */
        std::string model = "WDC WD30EZRS-11YF0B0";
        model.resize(40, ' '); /* firmware pads the field with spaces */
        for (size_t i = 0; i < 40; ++i) {
            const size_t word = 27 + i / 2;
            /* byte-swapped pairs: even char -> high byte, odd -> low */
            if (i % 2 == 0)
                p[word * 2 + 1] = u8(model[i]);
            else
                p[word * 2] = u8(model[i]);
        }
        return p;
    }
    /* SMART READ DATA: version u16 at 0, 30x12 entries at +2. */
    static std::vector<u8> make_read_data() {
        std::vector<u8> p(512, 0);
        p[0] = 0x10; /* smart version */
        auto put = [&](u32 e, u8 id, u8 value, u8 worst, u16 flags, u32 raw3) {
            u8* q = p.data() + 2 + e * 12;
            q[0] = id;
            q[1] = u8(flags);
            q[2] = u8(flags >> 8);
            q[3] = value;
            q[4] = worst;
            q[5] = u8(raw3);
            q[6] = u8(raw3 >> 8);
            q[7] = u8(raw3 >> 16);
        };
        put(0, 5, 100, 100, 0x0022, 4);    /* reallocated: 4 */
        put(1, 9, 98, 90, 0x0032, 12000);  /* power-on hours */
        put(2, 10, 100, 99, 0x0012, 0);    /* spin retry */
        put(3, 194, 118, 100, 0x0022, 36); /* temperature 36 C */
        put(4, 197, 100, 100, 0x0012, 2);  /* pending: 2 */
        put(5, 198, 100, 100, 0x0008, 1);  /* offline uncorrectable: failing bit set */
        return p;
    }
    /* SMART READ THRESHOLDS: id + threshold at each 12-byte entry's +1. */
    static std::vector<u8> make_thresholds() {
        std::vector<u8> p(512, 0);
        const u8 ids[6] = {5, 9, 10, 194, 197, 198};
        const u8 thr[6] = {13, 0, 0, 0, 0, 0};
        for (u32 e = 0; e < 6; ++e) {
            u8* q = p.data() + 2 + e * 12;
            q[0] = ids[e];
            q[1] = thr[e];
        }
        return p;
    }
    const char* name_;
    bool identify_;
};

/* --- the og-exact wire format (DiskHealth FUN_00425450 decode) --- */

TEST(Health, CsmiRequestMatchesOgWireFormat) {
    cr::health::csmi::AtaRequest req;
    req.phy = 3;
    req.port = 1;
    req.sas_address = 0x5000C50012345678ull;
    req.command = cr::health::csmi::kAtaCmdSmart;
    req.feature = 0xD0;
    u8 buf[cr::health::csmi::kRequestSize];
    ASSERT_EQ(cr::health::csmi::build_ata_request(req, buf, sizeof(buf)),
              cr::health::csmi::kRequestSize);

    /* SRB_IO_CONTROL header: 0x1C / "CSMISAS" / 60 / 0x19 / 0 / len */
    const u32 hdr_len = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (u32(buf[3]) << 24);
    EXPECT_EQ(hdr_len, 0x1Cu);
    EXPECT_EQ(std::memcmp(buf + 4, "CSMISAS", 7), 0);
    const u32 timeout = buf[0x0C] | (buf[0x0D] << 8) | (buf[0x0E] << 16) | (u32(buf[0x0F]) << 24);
    EXPECT_EQ(timeout, 60u);
    const u32 cc = buf[0x10] | (buf[0x11] << 8) | (buf[0x12] << 16) | (u32(buf[0x13]) << 24);
    EXPECT_EQ(cc, 0x19u);
    const u32 length = buf[0x18] | (buf[0x19] << 8) | (buf[0x1A] << 16) | (u32(buf[0x1B]) << 24);
    EXPECT_EQ(length, 512u + 0x8Cu); /* the og's Length = data + 0x8C */

    /* routing + task file + vendor fields */
    EXPECT_EQ(buf[0x1C], 3); /* phy */
    EXPECT_EQ(buf[0x1D], 1); /* port */
    u64 sas = 0;
    for (u32 i = 0; i < 8; ++i)
        sas |= u64(buf[0x20 + i]) << (8 * i);
    EXPECT_EQ(sas, 0x5000C50012345678ull);
    const u16 fis = u16(buf[0x2C]) | (u16(buf[0x2D]) << 8);
    EXPECT_EQ(fis, 0x8027u);    /* Register-FIS-to-device + C bit */
    EXPECT_EQ(buf[0x2E], 0xB0); /* command */
    EXPECT_EQ(buf[0x2F], 0xD0); /* feature */
    const u32 lba = buf[0x30] | (buf[0x31] << 8) | (buf[0x32] << 16) | (u32(buf[0x33]) << 24);
    EXPECT_EQ(lba, 0xA0C24F00u); /* SMART magic: LBA 00/4F/C2, device A0 */
    const u32 vendor = buf[0x40] | (buf[0x41] << 8) | (buf[0x42] << 16) | (u32(buf[0x43]) << 24);
    EXPECT_EQ(vendor, 0x11u);
    const u32 dlen = buf[0x44] | (buf[0x45] << 8) | (buf[0x46] << 16) | (u32(buf[0x47]) << 24);
    EXPECT_EQ(dlen, 512u);
    EXPECT_EQ(sizeof(buf), 0x2A8); /* total request: 0x200 + 0xA8 */
}

TEST(Health, CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic) {
    u8 buf[cr::health::csmi::kRequestSize];
    cr::health::csmi::AtaRequest req;
    req.command = cr::health::csmi::kAtaCmdIdentify; /* 0xEC */
    ASSERT_EQ(cr::health::csmi::build_ata_request(req, buf, sizeof(buf)), sizeof(buf));
    EXPECT_EQ(buf[0x2E], 0xEC);
    const u32 lba_id = buf[0x30] | (buf[0x31] << 8) | (buf[0x32] << 16) | (u32(buf[0x33]) << 24);
    EXPECT_EQ(lba_id, 0xA0000000u); /* zeros + device A0 */

    req.command = cr::health::csmi::kAtaCmdSmart; /* 0xB0, feature 0xD1 */
    req.feature = 0xD1;
    ASSERT_EQ(cr::health::csmi::build_ata_request(req, buf, sizeof(buf)), sizeof(buf));
    EXPECT_EQ(buf[0x2F], 0xD1);
    const u32 lba_st = buf[0x30] | (buf[0x31] << 8) | (buf[0x32] << 16) | (u32(buf[0x33]) << 24);
    EXPECT_EQ(lba_st, 0xA0C24F00u);
}

TEST(Health, CsmiBuilderRejectsUndersizedBuffer) {
    cr::health::csmi::AtaRequest req;
    u8 small[cr::health::csmi::kRequestSize - 1];
    EXPECT_EQ(cr::health::csmi::build_ata_request(req, small, sizeof(small)), 0u);
}

TEST(Health, CsmiResponseDataExtractedAtOgOffset) {
    u8 buf[cr::health::csmi::kRequestSize];
    std::memset(buf, 0, sizeof(buf));
    for (u32 i = 0; i < 512; ++i)
        buf[cr::health::csmi::kResponseDataOffset + i] = u8(i);
    u8 out[512];
    ASSERT_TRUE(cr::health::csmi::extract_response_data(buf, sizeof(buf), out));
    for (u32 i = 0; i < 512; ++i)
        ASSERT_EQ(out[i], u8(i));
    /* short buffer: no silent partial read */
    EXPECT_FALSE(cr::health::csmi::extract_response_data(
        buf, cr::health::csmi::kResponseDataOffset + 511, out));
}

TEST(Health, CsmiTransportUnavailableClassifiesTheOgErrorSet) {
    EXPECT_TRUE(cr::health::csmi::transport_unavailable(1));    /* ERROR_INVALID_FUNCTION */
    EXPECT_TRUE(cr::health::csmi::transport_unavailable(0x32)); /* ERROR_NOT_SUPPORTED */
    EXPECT_TRUE(cr::health::csmi::transport_unavailable(0x37)); /* ERROR_DEV_NOT_EXIST */
    EXPECT_FALSE(cr::health::csmi::transport_unavailable(5));   /* ACCESS_DENIED: a real failure */
    EXPECT_FALSE(cr::health::csmi::transport_unavailable(0));
}

/* --- the fallback chain (the og CAtaSmart accessor model) --- */

TEST(Health, CsmiTransportFallsBack) {
    /* the RST-array shape: passthrough + legacy both reject IDENTIFY,
     * the CSMI miniport route answers (the N-032 scenario). */
    ScriptedChannel ata("ata-passthrough", false);
    ScriptedChannel legacy("smart-rcv-drive-data", false);
    ScriptedChannel csmi("csmi-miniport", true);
    SmartInfo info;
    const u32 rc = cr::health::read_smart_ata({&ata, &legacy, &csmi}, info);
    ASSERT_EQ(rc, u32(cr::OK));
    EXPECT_EQ(info.transport, "csmi-miniport");
    EXPECT_GT(csmi.calls_, 0);
    EXPECT_GT(ata.calls_, 0);    /* tried first */
    EXPECT_GT(legacy.calls_, 0); /* tried second */

    /* every transport rejects: the clean final fallback stays */
    ScriptedChannel dead_ata("ata-passthrough", false);
    ScriptedChannel dead_legacy("smart-rcv-drive-data", false);
    ScriptedChannel dead_csmi("csmi-miniport", false);
    SmartInfo info2;
    EXPECT_EQ(cr::health::read_smart_ata({&dead_ata, &dead_legacy, &dead_csmi}, info2),
              cr::ERR_HEALTH_UNAVAILABLE);
}

TEST(Health, CsmiReadsAttributes) {
    ScriptedChannel csmi("csmi-miniport", true);
    SmartInfo info;
    ASSERT_EQ(cr::health::read_smart_ata({&csmi}, info), u32(cr::OK));

    /* IDENTIFY-derived fields */
    EXPECT_EQ(info.ata_version, 0x7Eu);
    EXPECT_EQ(info.model, "WDCWD30EZRS-11YF0B0"); /* the S-024 space-strip rule */

    /* the 30x12 table parse */
    ASSERT_EQ(info.attributes.size(), 6u);
    EXPECT_EQ(info.attributes[0].id, 5);
    EXPECT_EQ(info.attributes[0].value, 100);
    EXPECT_EQ(info.attributes[0].worst, 100);
    EXPECT_EQ(info.attributes[0].vendor_threshold, 13); /* 0xD1 matched row */
    EXPECT_EQ(info.reallocated_sectors, 4u);
    EXPECT_EQ(info.power_on_hours, 12000u);
    EXPECT_EQ(info.temperature_c, 36u);
    EXPECT_EQ(info.pending_sectors, 2u);
    EXPECT_EQ(info.uncorrectable, 1u);
    /* attr 198 carries the FAILED-NOW flag (flags bit 3) */
    EXPECT_TRUE(info.failing_now);
    EXPECT_TRUE(info.attributes[5].failing);

    /* the winning channel served the whole command set */
    EXPECT_EQ(csmi.last_command_, 0xB0);
}

TEST(Health, SmartChainEmptyIsBadParameter) {
    SmartInfo info;
    EXPECT_EQ(cr::health::read_smart_ata({}, info), cr::ERR_BAD_PARAMETER);
}

TEST(Health, SmartAttrNameTableCoversTheCuratedIds) {
    using cr::health::smart_attr_name;
    EXPECT_STREQ(smart_attr_name(5), "Reallocated sectors");
    EXPECT_STREQ(smart_attr_name(194), "Temperature");
    EXPECT_STREQ(smart_attr_name(198), "Offline uncorrectable");
    EXPECT_STREQ(smart_attr_name(231), "SSD life left");
    EXPECT_STREQ(smart_attr_name(0), "");
}

} // namespace
