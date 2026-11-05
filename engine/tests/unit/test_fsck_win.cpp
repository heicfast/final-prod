/* test_fsck_win.cpp - the M-075 win-lane launch endpoint tests.
 *
 * The seam battery: the wiring pins (what the spawn receives, what the
 * refusal paths return) run without spawning a real chkdsk; the real
 * end-to-end (chkdsk on a staged volume, exit 0) is the CI-SCENARIOS.md
 * row. The pure grammar battery is test_fscheck_launch.cpp (both
 * lanes).
 */

#include "cr/win/cr_fsck.h"
#include "cr/check/cr_fsck_launch.h"
#include "cr/cr_error.h"
#include <gtest/gtest.h>
#include <string>

using namespace cr;
using namespace cr::win::fsck;
using cr::check::fsck_launch::CHKDSK_FIX;
using cr::check::fsck_launch::CHKDSK_SCAN_ONLY;

namespace {

struct Recorder {
    static std::wstring last_cmd;
    static u32 last_timeout;
    static int next_exit;
};
std::wstring Recorder::last_cmd;
u32 Recorder::last_timeout = 0;
int Recorder::next_exit = 0;

int record_spawn(const std::wstring& cmd, u32 timeout_ms) {
    Recorder::last_cmd = cmd;
    Recorder::last_timeout = timeout_ms;
    return Recorder::next_exit;
}

bool probe_yes(wchar_t) {
    return true;
}
bool probe_no(wchar_t) {
    return false;
}

} // namespace

TEST(FsckWin, WiresTheCommandToTheSpawn) {
    Recorder::last_cmd.clear();
    Recorder::next_exit = 0;
    int32_t exit_code = -1;
    const u32 rc =
        detail::fsck_volume_seam(L'q', CHKDSK_FIX, 60000, &exit_code, probe_yes, record_spawn);
    /* the spawn receives the exact grammar from the pure builder (the
     * live system root + the uppercased letter + /f) */
    EXPECT_TRUE(Recorder::last_cmd.size() >= 30u);
    EXPECT_NE(Recorder::last_cmd.find(L"chkdsk.exe Q: /f"), std::wstring::npos)
        << "the spawn must receive the fix-posture command, got: " << Recorder::last_cmd.c_str();
    EXPECT_EQ(Recorder::last_timeout, 60000u);
    EXPECT_EQ(rc, OK);
    EXPECT_EQ(exit_code, 0);
}

TEST(FsckWin, ScanOnlyPostureOmitsTheFixFlag) {
    Recorder::last_cmd.clear();
    Recorder::next_exit = 0;
    int32_t exit_code = -1;
    const u32 rc =
        detail::fsck_volume_seam(L'c', CHKDSK_SCAN_ONLY, 1000, &exit_code, probe_yes, record_spawn);
    EXPECT_EQ(rc, OK);
    EXPECT_NE(Recorder::last_cmd.find(L"chkdsk.exe C:"), std::wstring::npos);
    EXPECT_EQ(Recorder::last_cmd.find(L"/f"), std::wstring::npos)
        << "the check posture never fixes";
}

TEST(FsckWin, RefusesAbsentVolumeLoudly) {
    /* the loud refusal: no volume at the letter -> ERR_OPEN_FAILED and
     * NOTHING is spawned (the recorder stays empty) */
    Recorder::last_cmd.clear();
    int32_t exit_code = -1;
    const u32 rc =
        detail::fsck_volume_seam(L'y', CHKDSK_FIX, 1000, &exit_code, probe_no, record_spawn);
    EXPECT_EQ(rc, ERR_OPEN_FAILED);
    EXPECT_EQ(exit_code, -1);
    EXPECT_TRUE(Recorder::last_cmd.empty()) << "no process may be spawned on refusal";
}

TEST(FsckWin, RefusesInvalidLetterLoudly) {
    Recorder::last_cmd.clear();
    int32_t exit_code = -1;
    /* a digit is not a volume letter: refused before any probe */
    const u32 rc =
        detail::fsck_volume_seam(L'7', CHKDSK_FIX, 1000, &exit_code, probe_no, record_spawn);
    EXPECT_EQ(rc, ERR_BAD_PARAMETER);
    EXPECT_TRUE(Recorder::last_cmd.empty());
}

TEST(FsckWin, SpawnFailureMapsOpenFailed) {
    Recorder::next_exit = -1; /* the spawn's own failure class */
    int32_t exit_code = 0;
    const u32 rc =
        detail::fsck_volume_seam(L'q', CHKDSK_FIX, 1000, &exit_code, probe_yes, record_spawn);
    EXPECT_EQ(rc, ERR_OPEN_FAILED);
    EXPECT_EQ(exit_code, -1);
}

TEST(FsckWin, ChkdskVerdictPassesThroughRaw) {
    /* chkdsk's documented classes ride the ABI raw: 1 (errors fixed)
     * is OK-with-verdict, never remapped or swallowed */
    Recorder::next_exit = 1;
    int32_t exit_code = -1;
    const u32 rc =
        detail::fsck_volume_seam(L'q', CHKDSK_FIX, 1000, &exit_code, probe_yes, record_spawn);
    EXPECT_EQ(rc, OK);
    EXPECT_EQ(exit_code, 1);
}
