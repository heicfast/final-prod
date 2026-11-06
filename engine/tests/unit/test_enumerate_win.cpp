/* test_enumerate_win.cpp - N-071 source-truth pins: the enumeration
 * scope contract (PARITY-DELTAS D-79) and the busy-disk hardening in
 * the win layer's query_disk. The behavior itself is Windows-only
 * (compiled on the MSVC lane; parse-gated on Linux by win_syntax_check)
 * - these pins are the repo's established doc-truth pattern (N-074 /
 * N-040 family): the registered divergence and its hardening must stay
 * shipped and in lockstep.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "test_tmpdir.hpp" /* cr_test::read_repo_file (N-074 doc-truth pin pattern) */
#include <string>

namespace {

/* The N-071 convergence: the two-stack port row (D-79, the canonical
 * landing) AND the companion scope/hardening row (D-80) must stay
 * registered, with the revisit trigger. */
TEST(Enumerate, ScopeDeltaRegisteredWithRevisitTrigger) {
    const std::string deltas = cr_test::read_repo_text("docs/PARITY-DELTAS.md");
    ASSERT_FALSE(deltas.empty());
    EXPECT_NE(deltas.find("D-84 — disk enumeration: the two-stack merge"), std::string::npos)
        << "the canonical N-071 two-stack port registration must stay";
    EXPECT_NE(deltas.find("D-85 — device enumeration scope"), std::string::npos)
        << "the companion scope row (residual scope + busy-disk hardening) must stay";
    EXPECT_NE(deltas.find("REVISIT TRIGGER"), std::string::npos)
        << "the revisit trigger (live miss-case reopens the row) must stay";
}

/* The busy-disk hardening: the og's stacks enumerate without opening;
 * our numbered loop retries a 0-access metadata open so a share-blocked
 * disk stays listed (with busy=true), and only a truly unopenable disk
 * stays absent. */
TEST(Enumerate, BusyDiskMetadataRetryShipsInQueryDisk) {
    const std::string src = cr_test::read_repo_text("src/win/cr_enumerate.cpp");
    ASSERT_FALSE(src.empty());
    EXPECT_NE(src.find("ERROR_SHARING_VIOLATION"), std::string::npos)
        << "the share-blocked retry classification must ship";
    EXPECT_NE(src.find("ERROR_ACCESS_DENIED"), std::string::npos)
        << "the access-denied retry classification must ship";
    /* the retry must be a 0-ACCESS open (the point of the hardening:
     * no read/write share conflict) - not just a second data open */
    const size_t retry = src.find("CreateFileW(path, 0,");
    EXPECT_NE(retry, std::string::npos) << "the metadata-only retry must request zero access";
    EXPECT_NE(src.find("bool& busy, DWORD* last_error)"), std::string::npos)
        << "the retry must report the busy flag to its caller";
    EXPECT_NE(src.find("open_disk_for_query(p, d.busy, &open_le)"), std::string::npos)
        << "BOTH stacks (the numbered loop + the cfgmgr32 interface pass) must use the retry";
}

/* The scope contract must stay documented at the enumeration surface
 * (the header is where a future SetupAPI extension gets designed
 * against - the D-79 revisit trigger names it). */
TEST(Enumerate, ScopeContractDocumentedInHeader) {
    const std::string hdr = cr_test::read_repo_text("include/cr/win/cr_enumerate.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(hdr.find("SCOPE (N-071"), std::string::npos)
        << "the enumeration scope statement must stay in the header";
    EXPECT_NE(hdr.find("bool busy = false"), std::string::npos)
        << "PhysicalDisk.busy must stay declared";
}

} // namespace

/* M-076: the TRIM chip datum ships in all three layers, in the opt-in
 * shape the finding demanded - the ABI fields + the _ex flag, the impl's
 * one-fresh-query-per-invocation stamp (the datum is OS-global; a failed
 * query leaves trim_known == 0 - never a guessed value), and the CLI's
 * --trim (three sites: the Commands header, the usage text, the flag
 * parse + the print). */
TEST(Enumerate, TrimChipOptInShipsInAbiAndCli) {
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(hdr.find("CR_ENUM_FLAG_TRIM"), std::string::npos)
        << "the opt-in flag must be defined";
    EXPECT_NE(hdr.find("cr_ui_enumerate_disks_ex"), std::string::npos)
        << "the _ex export must be declared";
    EXPECT_NE(hdr.find("never a guessed value"), std::string::npos)
        << "the failed-query rule must be stated in the contract";
    EXPECT_NE(hdr.find("uint32_t trim_known;"), std::string::npos)
        << "the per-row TRIM fields must exist (append-only)";
    const std::string src = cr_test::read_repo_text("src/ui/cr_ui.cpp");
    ASSERT_FALSE(src.empty());
    const size_t at = src.find("u32 enumerate_disks_impl(");
    ASSERT_NE(std::string::npos, at);
    const size_t end = src.find("\nu32 ", at + 10);
    const std::string body = src.substr(at, (end == std::string::npos ? src.size() : end) - at);
    EXPECT_NE(std::string::npos, body.find("flags & CR_ENUM_FLAG_TRIM"))
        << "the TRIM query must be gated by the opt-in flag";
    EXPECT_NE(std::string::npos, body.find("win::health::query_trim_state"))
        << "the chip datum must come from the health facet's primitive";
    /* the unknown-bits refusal precedes the platform check */
    const size_t ex = src.find("cr_ui_enumerate_disks_ex(");
    ASSERT_NE(std::string::npos, ex);
    const std::string exsrc = src.substr(ex, 700);
    EXPECT_NE(std::string::npos, exsrc.find("~CR_ENUM_FLAG_TRIM"))
        << "unknown flag bits must be refused";
    const std::string cli = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(cli.empty());
    EXPECT_NE(std::string::npos, cli.find("rrecov list [--trim]"))
        << "the usage text must carry --trim";
    EXPECT_NE(std::string::npos, cli.find("L\"--trim\""))
        << "the flag must be parsed by the list verb";
    EXPECT_NE(std::string::npos, cli.find("TRIM:ON")) << "the chip must render in the row output";
    EXPECT_NE(std::string::npos, cli.find("TRIM ?"))
        << "the unknown form must render on a failed query (never guessed)";
}

/* ---- D3 (the debug-and-release session): never drop a PRESENT device ---- */

TEST(D3Enumerate, PresentButUnopenableDisksStayListed) {
    /* The user's report: two internal drives + one SD card, only one disk
     * appeared. Root-cause class: both enumeration stacks DROPPED any disk
     * whose open failed with anything other than ACCESS_DENIED/SHARING
     * (cr_enumerate.cpp) — while the reference lists devices WITHOUT
     * opening them (five-stack metadata fan-in). The fix pins:
     *   1. the numbered loop distinguishes ABSENT (the namespace-gap error
     *      set) from PRESENT-but-unopenable (everything else: NOT_READY,
     *      IO_DEVICE, GEN_FAILURE, exhausted access-denied) and KEEPS the
     *      present ones listed with honest empties + busy marking;
     *   2. the cfgmgr interface pass lists a PRESENT interface even when
     *      its open fails or its geometry refuses (the interface list IS
     *      the presence truth);
     *   3. a geometry failure on an OPENED handle still lists (not-ready
     *      media stacks answer the open and refuse the IOCTL). */
    const std::string src = cr_test::read_repo_text("src/win/cr_enumerate.cpp");
    ASSERT_FALSE(src.empty());

    /* 1. the namespace-gap set gates ONLY the absent case */
    const size_t qd = src.find("bool query_disk(u32 n, PhysicalDisk& d)");
    ASSERT_NE(std::string::npos, qd) << "query_disk moved?";
    const std::string qd_body = src.substr(qd, 1200);
    EXPECT_NE(std::string::npos, qd_body.find("ERROR_FILE_NOT_FOUND"))
        << "the absent-vs-present discrimination must keep the namespace-gap set";
    EXPECT_NE(std::string::npos, qd_body.find("ERROR_PATH_NOT_FOUND"))
        << "the namespace-gap set must be complete";
    EXPECT_NE(std::string::npos, qd_body.find("return false"))
        << "absent drives still skip (the loop's probe miss)";
    /* the present-but-unopenable branch: number stamped + busy + true */
    const size_t present_branch = qd_body.find("PRESENT but unopenable");
    ASSERT_NE(std::string::npos, present_branch)
        << "the present-but-unopenable branch must exist (D3)";
    const std::string tail = qd_body.substr(present_branch, 400);
    EXPECT_NE(std::string::npos, tail.find("d.busy = true"))
        << "unopenable-present disks carry the busy marker (scans re-open)";
    EXPECT_NE(std::string::npos, tail.find("return true"))
        << "unopenable-present disks are LISTED, not dropped";

    /* 2. the interface pass lists on open failure */
    const size_t iface = src.find("u32 enumerate_disk_interfaces(PhysicalDiskList& out)");
    ASSERT_NE(std::string::npos, iface) << "enumerate_disk_interfaces moved?";
    const std::string iface_body = src.substr(iface, 2600);
    EXPECT_NE(std::string::npos, iface_body.find("the interface is PRESENT"))
        << "an unopenable PRESENT interface must still be listed (D3)";
    /* 3. geometry refusal still lists */
    EXPECT_NE(std::string::npos, iface_body.find("opened but no geometry"))
        << "a geometry refusal on an opened handle must still list (D3)";
}
