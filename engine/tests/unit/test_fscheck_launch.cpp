/* test_fsck_launch.cpp - the M-075 escalation-endpoint grammar tests.
 *
 * The pure builder seam (the N-064 build_unlock_command precedent): the
 * exact command line for each posture of the guidance string, the letter
 * validation, the documented exit-code classes. The win-lane spawn and
 * the end-to-end scenario (chkdsk on a staged volume) live on the
 * Windows runner (test_fsck_win.cpp + CI-SCENARIOS.md).
 */

#include "cr/check/cr_fsck_launch.h"
#include <gtest/gtest.h>

using namespace cr::check::fsck_launch;

TEST(FsckVolume, CommandConstruction) {
    /* the check posture (strings_ascii.txt:126's first half): no /f */
    EXPECT_EQ(build_chkdsk_command(L'c', CHKDSK_SCAN_ONLY, L"C:\\Windows"),
              L"C:\\Windows\\System32\\chkdsk.exe C:");
    /* the fix posture (the second half): /f appended */
    EXPECT_EQ(build_chkdsk_command(L'q', CHKDSK_FIX, L"C:\\Windows"),
              L"C:\\Windows\\System32\\chkdsk.exe Q: /f");
    /* the system root is taken verbatim, separator normalized once */
    EXPECT_EQ(build_chkdsk_command(L'a', CHKDSK_FIX, L"D:\\WinRoot\\"),
              L"D:\\WinRoot\\System32\\chkdsk.exe A: /f");
    /* unknown future flag bits are ignored - only /f is wired */
    EXPECT_EQ(build_chkdsk_command(L'e', CHKDSK_FIX | 0xF0, L"C:\\Windows"),
              L"C:\\Windows\\System32\\chkdsk.exe E: /f");
}

TEST(FsckVolume, RefusesInvalidVolume) {
    /* the loud-refusal grammar: anything that is not a volume letter is
     * invalid BEFORE any command is built - digits, separators, NUL and
     * the non-ASCII range all refuse (never a silent skip) */
    EXPECT_FALSE(valid_volume_letter(L'0'));
    EXPECT_FALSE(valid_volume_letter(L'9'));
    EXPECT_FALSE(valid_volume_letter(L'\\'));
    EXPECT_FALSE(valid_volume_letter(L':'));
    EXPECT_FALSE(valid_volume_letter(L'\0'));
    EXPECT_FALSE(valid_volume_letter(0x4E2D)); /* CJK codepoint */
    EXPECT_TRUE(valid_volume_letter(L'A'));
    EXPECT_TRUE(valid_volume_letter(L'Z'));
    EXPECT_TRUE(valid_volume_letter(L'z'));
    /* the builder never crashes on an invalid letter (the '?' sentinel
     * keeps the seam total; the win layer refuses on the probe first) */
    EXPECT_EQ(build_chkdsk_command(L'0', CHKDSK_FIX, L"C:\\Windows"),
              L"C:\\Windows\\System32\\chkdsk.exe ?: /f");
}

TEST(FsckVolume, ExitCodeClass) {
    /* the documented chkdsk exit classes (see the header): 0 clean,
     * 1 fixed, 2 cleanup/scheduled - all "no errors remain" outcomes
     * the host can report as scan-again-when-suspicious */
    EXPECT_FALSE(chkdsk_exit_means_errors_remain(0));
    EXPECT_FALSE(chkdsk_exit_means_errors_remain(1));
    EXPECT_FALSE(chkdsk_exit_means_errors_remain(2));
    EXPECT_FALSE(chkdsk_exit_means_errors_remain(259)) << "timeout is not a verdict";
    /* 3 = errors found and NOT fixed; negatives = launch-failure class
     * (the win layer's -1 spawn failure rides the same surface) */
    EXPECT_TRUE(chkdsk_exit_means_errors_remain(3));
    EXPECT_TRUE(chkdsk_exit_means_errors_remain(4));
    EXPECT_TRUE(chkdsk_exit_means_errors_remain(-1));
}
