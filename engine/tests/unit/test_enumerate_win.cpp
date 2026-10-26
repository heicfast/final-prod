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
    const std::string deltas = cr_test::read_repo_file("docs/PARITY-DELTAS.md");
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
    const std::string src = cr_test::read_repo_file("src/win/cr_enumerate.cpp");
    ASSERT_FALSE(src.empty());
    EXPECT_NE(src.find("ERROR_SHARING_VIOLATION"), std::string::npos)
        << "the share-blocked retry classification must ship";
    EXPECT_NE(src.find("ERROR_ACCESS_DENIED"), std::string::npos)
        << "the access-denied retry classification must ship";
    /* the retry must be a 0-ACCESS open (the point of the hardening:
     * no read/write share conflict) - not just a second data open */
    const size_t retry = src.find("CreateFileW(path, 0,");
    EXPECT_NE(retry, std::string::npos) << "the metadata-only retry must request zero access";
    EXPECT_NE(src.find("bool& busy)"), std::string::npos)
        << "the retry must report the busy flag to its caller";
    EXPECT_NE(src.find("open_disk_for_query(p, d.busy)"), std::string::npos)
        << "BOTH stacks (the numbered loop + the cfgmgr32 interface pass) must use the retry";
}

/* The scope contract must stay documented at the enumeration surface
 * (the header is where a future SetupAPI extension gets designed
 * against - the D-79 revisit trigger names it). */
TEST(Enumerate, ScopeContractDocumentedInHeader) {
    const std::string hdr = cr_test::read_repo_file("include/cr/win/cr_enumerate.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(hdr.find("SCOPE (N-071"), std::string::npos)
        << "the enumeration scope statement must stay in the header";
    EXPECT_NE(hdr.find("bool busy = false"), std::string::npos)
        << "PhysicalDisk.busy must stay declared";
}

} // namespace
