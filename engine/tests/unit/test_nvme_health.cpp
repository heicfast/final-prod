/* NVMe health log page parser tests — PARITY-AUDIT F-A4-17.
 *
 * The regression: the previous inline parse (src/win/cr_health.cpp) read
 * power_cycles / power_on_hours / unsafe_shutdowns / media_errors from
 * offsets 64/80/96/112 — the Host Reads / Host Writes / Controller Busy
 * Time / Power Cycles positions. Spec (and NVMe 1.x hardware): 112/128/
 * 144/160. Every field gets a DISTINCT value in the fixture so a wrong
 * offset cannot pass by coincidence. */
#include <gtest/gtest.h>
#include "cr/health/cr_nvme.h"
#include "cr/cr_error.h"
#include <cstring>
#include <string>
#include <vector>

#include "test_tmpdir.hpp" /* cr_test::read_repo_file (N-074 doc-truth pin) */

using namespace cr;
using cr::health::parse_nvme_health_page;

namespace {

void put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + size_t(i)] = u8(x >> (8 * i));
}

std::vector<u8> build_page() {
    std::vector<u8> p(512, 0);
    p[0] = 0x03; /* critical warning */
    p[1] = 0xE4;
    p[2] = 0x0C;          /* 3300 K */
    p[3] = 100;           /* available spare */
    p[4] = 10;            /* spare threshold */
    p[5] = 42;            /* percentage used */
    put64(p, 32, 1111);   /* data units read */
    put64(p, 48, 2222);   /* data units written */
    put64(p, 64, 999001); /* host reads */
    put64(p, 80, 999002); /* host writes */
    put64(p, 96, 999003); /* controller busy time */
    put64(p, 112, 3333);  /* power cycles */
    put64(p, 128, 4444);  /* power on hours */
    put64(p, 144, 5555);  /* unsafe shutdowns */
    put64(p, 160, 6666);  /* media errors */
    put64(p, 176, 7777);  /* error log entries */
    return p;
}

} // namespace

TEST(NvmeHealth, PageFieldOffsets) {
    auto p = build_page();
    health::NvmeHealthInfo h;
    ASSERT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
    EXPECT_TRUE(h.available);
    EXPECT_EQ(h.critical_warning, 0x03);
    EXPECT_EQ(h.temperature_k, 3300u);
    EXPECT_EQ(h.available_spare, 100u);
    EXPECT_EQ(h.available_spare_threshold, 10u);
    EXPECT_EQ(h.percentage_used, 42u);
    EXPECT_EQ(h.data_units_read, 1111u);
    EXPECT_EQ(h.data_units_written, 2222u);
    EXPECT_EQ(h.power_cycles, 3333u);     /* was 999001 (host reads) */
    EXPECT_EQ(h.power_on_hours, 4444u);   /* was 999002 (host writes) */
    EXPECT_EQ(h.unsafe_shutdowns, 5555u); /* was 999003 (busy time) */
    EXPECT_EQ(h.media_errors, 6666u);     /* was 3333 (power cycles) */
}

TEST(NvmeHealth, FullFieldSurface) {
    /* M-036 (diff-to-fix): the og's NVMe row set includes host reads /
     * writes, controller busy time and the error-log count - the four
     * fields the original parser dropped. Every field gets a distinct
     * fixture value so a wrong offset cannot pass by coincidence. */
    auto p = build_page();
    health::NvmeHealthInfo h;
    ASSERT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
    EXPECT_EQ(h.host_reads, 999001u);
    EXPECT_EQ(h.host_writes, 999002u);
    EXPECT_EQ(h.controller_busy_time, 999003u);
    EXPECT_EQ(h.num_error_log_entries, 7777u);
}

TEST(NvmeHealth, DataUnitsConvertedToBytes) {
    /* M-036: data units are 512,000-byte units; the og DiskHealth converts
     * with a full 64-bit multiply (DiskHealth_decompiled.c :5589-5614,
     * the 5-byte LE chain x 0x7d000). Raw counts stay alongside. */
    auto p = build_page();
    health::NvmeHealthInfo h;
    ASSERT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
    EXPECT_EQ(h.data_bytes_read, 1111u * 512000u);
    EXPECT_EQ(h.data_bytes_written, 2222u * 512000u);
    EXPECT_EQ(h.data_units_read, 1111u); /* raw count preserved */
    EXPECT_EQ(h.data_units_written, 2222u);
    /* a large realistic count: 1e9 units = 512,000,000,000 bytes */
    put64(p, 32, 1000000000ull);
    ASSERT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
    EXPECT_EQ(h.data_bytes_read, 1000000000ull * 512000u);
}

TEST(NvmeHealth, ShortBufferRejected) {
    std::vector<u8> p(191, 0);
    health::NvmeHealthInfo h;
    EXPECT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), ERR_TRUNCATED);
    EXPECT_FALSE(h.available);
    /* exactly 192 (through num_error_log_entries, +176..191) is accepted
     * - the gate moved from 176 when M-036 surfaced the last field */
    p.resize(192);
    EXPECT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
}

TEST(NvmeHealth, NullRejected) {
    health::NvmeHealthInfo h;
    EXPECT_EQ(parse_nvme_health_page(nullptr, 512, h), ERR_TRUNCATED);
}

TEST(NvmeHealth, PercentageUsedIsU8Width) {
    /* N-009a (diff-to-fix): the NVMe health log defines percentage_used
     * as a single u8 at +5; the byte at +6 is reserved. The old u16 read
     * consumed the reserved byte - a torn page with garbage there
     * produced absurd percentages. */
    auto p = build_page();
    p[5] = 42;
    p[6] = 0xFF; /* reserved byte garbage */
    health::NvmeHealthInfo h;
    ASSERT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
    EXPECT_EQ(h.percentage_used, 42u) << "the reserved byte at +6 must not leak in";
    p[5] = 100; /* spec max */
    p[6] = 0x01;
    ASSERT_EQ(parse_nvme_health_page(p.data(), u32(p.size()), h), OK);
    EXPECT_EQ(h.percentage_used, 100u);
}

TEST(Nvme, CliTempFractionIsCelsiusDecidegrees) {
    /* N-042: the pre-fix CLI printed the fractional digit as the KELVIN
     * remainder (k % 10) - wrong by up to ~0.55 C and systematically
     * biased (309 K printed "36.5"; true 35.9 C). The shared accessor
     * (decidegrees, k*10-2731) is the single source for the CLI print
     * and the ABI rows; assert the corrected table and the floor guard
     * (the ABI's 0-below-273.1K rule). */
    struct Case {
        u16 k;
        u32 want_dec;
        const char* spelling;
    };
    const Case cases[] = {
        {309, 359, "35.9"},   /* true 35.85 C; pre-fix printed 36.5 */
        {300, 269, "26.9"},   /* true 26.85 C; pre-fix printed 27.0 */
        {273, 0, "0.0"},      /* below the 273.1 K floor: 0, never wrap */
        {0, 0, "0.0"},        /* unset sensor: 0 */
        {310, 369, "36.9"},   /* true 36.85 C; pre-fix printed 37.0 */
        {2731 / 10, 0, "0.0"} /* exactly 273.1 K: 0 decidegrees */
    };
    for (const auto& c : cases) {
        health::NvmeHealthInfo h;
        h.temperature_k = c.k;
        /* the CLI prints %u.%u from (dec/10, dec%10): the spelling column
         * documents the visible output; dec is the single source of truth */
        EXPECT_EQ(h.decidegrees_c(), c.want_dec)
            << "k=" << c.k << " (CLI prints " << c.spelling << ")";
    }
    /* truncation bias is bounded (~-0.05 C): 309.99 K stays 35.9 */
    health::NvmeHealthInfo h;
    h.temperature_k = 310; /* the page's 16-bit field carries whole K only */
    EXPECT_EQ(h.decidegrees_c(), 369u);
}

TEST(Health, SmartCommandListMatchesImplementation) {
    /* N-074 doc-truth pin (the N-040 family's doc-lint, applied to the
     * health header): cr_health.h's DELTA comment may only claim SMART
     * commands the implementation issues. 0xD1 READ_THRESHOLDS ships
     * (src/core/health/cr_smart_ata.cpp - S-024's fix, moved to the pure
     * seam by N-032); 0xD8 RETURN_STATUS was a claimed-but-never-issued
     * command and must not return to the comment until the code issues it. */
    const std::string hdr = cr_test::read_repo_text("include/cr/win/cr_health.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(hdr.find("0xD1 READ_THRESHOLDS"), std::string::npos)
        << "S-024's threshold reads ship - the header must keep claiming them";
    EXPECT_EQ(hdr.find("0xD8"), std::string::npos)
        << "0xD8 RETURN_STATUS is not issued - the header "
           "must not claim it (re-adding requires issuing it in code first)";
    /* the issuance lives in the pure seam since N-032 (the win layer
     * supplies transports only) */
    const std::string seam = cr_test::read_repo_text("src/core/health/cr_smart_ata.cpp");
    ASSERT_FALSE(seam.empty());
    EXPECT_NE(seam.find("0xD1"), std::string::npos) << "the threshold read must actually ship";
    EXPECT_EQ(seam.find("0xD8"), std::string::npos)
        << "if 0xD8 ever ships, update cr_health.h's comment in the same "
           "change (this pin forces the lockstep)";
    const std::string win_src = cr_test::read_repo_text("src/win/cr_health.cpp");
    ASSERT_FALSE(win_src.empty());
    EXPECT_EQ(win_src.find("0xD8"), std::string::npos)
        << "the win layer issues no SMART commands of its own - transports only";
}

/* ---------------------------------------------------------------- */
/* D-NVME-01: the protocol query's LAYOUT contract (the pack(1)      */
/* misplacement that killed every real-NVMe health read)             */
/* ---------------------------------------------------------------- */

TEST(NvmeHealth, ProtocolQueryPlacesProtocolDataAtAdditionalParameters) {
    /* The Microsoft-documented single-buffer shape ("Working with NVMe
     * Drives", Microsoft Learn): the STORAGE_PROTOCOL_SPECIFIC_DATA sits
     * at the STORAGE_PROPERTY_QUERY's AdditionalParameters field -
     * FIELD_OFFSET(..., AdditionalParameters) == 8 - and the payload at
     * protocol-data start + ProtocolDataOffset (== 48). The pre-fix
     * buffer was #pragma pack(push,1)-ed, placing the protocol data at
     * offset 9: the storage stack read ProtocolType at 8 as 0x00000300
     * and EVERY real NVMe controller refused the query - NVMe-only
     * drives surfaced no health at all (the user's "switch to another
     * disk and it shows nothing"). Invisible on Azure VHDX seeds (no
     * NVMe passthrough) - this pin is the lane-visible proof the layout
     * stays doc-exact. Fail-before: the pack(1) tree fails this pin. */
    const std::string src = cr_test::read_repo_text("src/win/cr_health.cpp");
    ASSERT_FALSE(src.empty());
    /* the offset-8 placement, expressed as the doc's own field-offset */
    EXPECT_NE(src.find("offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters)"),
              std::string::npos)
        << "the protocol data placement must derive from AdditionalParameters";
    /* the load-bearing offsets are static_asserted in the source itself
     * (they run on BOTH lanes - the win-stub SDK shapes are byte-exact) */
    EXPECT_NE(src.find("static_assert(kProtoOff == 8"), std::string::npos)
        << "the offset-8 pin must stay compiled-in";
    EXPECT_NE(src.find("static_assert(kPayloadOff == 48"), std::string::npos)
        << "the payload-offset pin must stay compiled-in";
    /* the answer is re-derived from the RETURNED descriptor, never
     * assumed to sit where the request put it (the doc sample's shape) */
    EXPECT_NE(src.find("answer->ProtocolDataOffset"), std::string::npos)
        << "the payload pointer must come from the returned descriptor";
    /* the pack(1) class must never return to this query */
    EXPECT_EQ(src.find("#pragma pack(push, 1)"), std::string::npos)
        << "packing the protocol query misplaces the protocol data by one "
           "byte - the SDK structs must stay naturally aligned";
}

TEST(NvmeHealth, WinStubSdkShapesAreLayoutFaithful) {
    /* D-NVME-01's Linux-lane half: the parse gate's SDK stub must carry
     * the byte-exact shapes the static_asserts need (the pre-fix stub
     * typed DWORD as `unsigned long` - 8 bytes on LP64 Linux vs 4 on
     * every Windows target - so no layout assert could ever run here).
     * The stub's own sizes are pinned THROUGH the win layer's compile:
     * if the stub drifts, cr_health.cpp's static_asserts fail the
     * win_syntax_check gate; this pin keeps the stub's fixed-width
     * contract visible in the test suite. */
    const std::string stub = cr_test::read_repo_text("tests/win-stub/windows.h");
    ASSERT_FALSE(stub.empty());
    EXPECT_NE(stub.find("typedef unsigned int DWORD;"), std::string::npos)
        << "the stub's DWORD must be 4 bytes (LLP64 truth), not unsigned long";
    EXPECT_NE(stub.find("typedef unsigned int ULONG;"), std::string::npos)
        << "the stub's ULONG must be 4 bytes";
    EXPECT_NE(stub.find("typedef unsigned long long ULONG_PTR;"), std::string::npos)
        << "ULONG_PTR stays 8 bytes on x64";
    const std::string ioctl = cr_test::read_repo_text("tests/win-stub/winioctl.h");
    ASSERT_FALSE(ioctl.empty());
    EXPECT_NE(ioctl.find("_STORAGE_PROTOCOL_DATA_DESCRIPTOR"), std::string::npos)
        << "the output descriptor shape must exist in the stub (the "
           "single-buffer pattern's answer side)";
}
