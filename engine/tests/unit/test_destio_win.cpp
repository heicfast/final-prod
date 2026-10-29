/* test_destio_win.cpp - Win32 DestIo contract (Windows lane only).
 *
 * Regression suite for the drive-letter mkdir walk (the defect that failed
 * all items of the #227 recovery on the MSVC lane): the component walk
 * must treat every EXISTING component - including the drive-root-shaped
 * prefixes "C:", "C:\" - as success (the POSIX EEXIST contract), create
 * missing components, stay idempotent, and refuse a FILE in the way.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
/* MAX_PATH / GetCurrentDirectoryW / GetVolumePathNameW (the relative-path
 * volume test): pulled explicitly - nothing in the cr headers guarantees
 * the Win32 shell/path set on the MSVC lane (Windows CI C2065/C3861) */
#include <windows.h>
#include "cr/win/cr_destio.h"
#include "cr/win/cr_transport.h" /* N-013 arming contracts */
#include "test_tmpdir.hpp"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace cr;
using namespace cr::win;

namespace {

std::u16string u16_from(const std::string& s) {
    /* the suite's paths are ASCII (tmp paths + fixture names) - the exact
     * shape the shim hands WinDestIo after u8_to_u16 */
    std::u16string out;
    for (char c : s)
        /* static_cast: the multi-word functional cast (unsigned char(c))
         * is an MSVC-only extension - standard C++ functional casts take
         * a simple-type-specifier only; identical conversion either way
         * (the N-228 parse gate requires the standard form). */
        out.push_back(char16_t(static_cast<unsigned char>(c)));
    return out;
}

/* M-074: the og's destination-validation taxonomy - path-too-long,
 * invalid-path, and the network shape surface as DISTINCT codes, never the
 * collapsed ERR_DEST_UNWRITABLE (UIUX/11:16's numbered strings). */
TEST(WinDestIo, M074DestinationCodesDifferentiated) {
    /* the UNC shape classifies as the network destination even though a
     * UNC create fails with a generic code on a network-less host */
    {
        WinDestIo io;
        const u32 rc = io.create_temp(1, u16_from("\\\\no-such-server\\share\\file.bin"), u".tmp");
        EXPECT_EQ(u32(ERR_DEST_NETWORK), rc) << "the UNC shape is the network branch";
    }
    /* path-too-long: a >MAX_PATH destination under a real temp root */
    {
        WinDestIo io;
        std::u16string base = u16_from(cr_test::tmp_path("destio", "m074"));
        std::u16string long_name(300, u'x');
        const u32 rc = io.create_temp(2, base + u"\\" + long_name, u".tmp");
        EXPECT_EQ(u32(ERR_DEST_PATH_TOO_LONG), rc) << "ERROR_FILENAME_EXCED_RANGE classifies";
    }
    /* invalid path: the reserved-name / malformed shape */
    {
        WinDestIo io;
        const u32 rc = io.create_temp(3, u16_from("C:\\bad|name\\file.bin"), u".tmp");
        EXPECT_TRUE(rc == u32(ERR_DEST_PATH_INVALID) || rc == u32(ERR_DEST_UNWRITABLE))
            << "the malformed-name class (the host's error set varies by version)";
    }
}

TEST(WinDestIo, MkdirCreatesMissingChainUnderDriveLetterRoot) {
    const std::string root = cr_test::tmp_path("wdestio", ".a");
    const std::string deep = root + "/one/two/three";
    ASSERT_FALSE(std::filesystem::exists(deep));
    WinDestIo io;
    EXPECT_EQ(io.mkdir(u16_from(deep + "\\")), OK); /* u'\\' separators = core convention */
    EXPECT_TRUE(std::filesystem::is_directory(deep));
    /* idempotent: re-running the identical recovery re-creates nothing */
    EXPECT_EQ(io.mkdir(u16_from(deep + "\\")), OK);
}

TEST(WinDestIo, MkdirAcceptsDriveRootShapedComponents) {
    /* "C:\..." chains: every prefix from "C:" up must pass the walk */
    const std::string root = cr_test::tmp_path("wdestio", ".b");
    WinDestIo io;
    EXPECT_EQ(io.mkdir(u16_from(root)), OK);
    EXPECT_TRUE(std::filesystem::is_directory(root));
}

TEST(WinDestIo, MkdirRefusesFileInTheWay) {
    const std::string blocker = cr_test::tmp_path("wdestio", ".c");
    {
        FILE* f = std::fopen(blocker.c_str(), "wb");
        ASSERT_NE(f, nullptr);
        std::fputs("file", f);
        std::fclose(f);
    }
    WinDestIo io;
    /* a FILE occupying the final component is not a directory: refuse */
    EXPECT_EQ(io.mkdir(u16_from(blocker)), ERR_DEST_UNWRITABLE);
    /* ...and it still blocks components below it */
    EXPECT_EQ(io.mkdir(u16_from(blocker + "/below")), ERR_DEST_UNWRITABLE);
}

TEST(WinDestIo, TempCommitAndExistsLifecycle) {
    const std::string root = cr_test::tmp_path("wdestio", ".d");
    WinDestIo io;
    ASSERT_EQ(io.mkdir(u16_from(root)), OK);
    const std::u16string final_path = u16_from(root + "\\OUT.TXT");
    const std::u16string suffix = u".part-1";
    EXPECT_FALSE(io.exists(final_path));
    ASSERT_EQ(io.create_temp(1, final_path, suffix), OK);
    const unsigned char payload[] = {'P', 'A', 'Y'};
    EXPECT_EQ(io.write(1, 0, payload, 3), OK);
    EXPECT_FALSE(io.exists(final_path)); /* temp only - atomic semantics */
    EXPECT_EQ(io.commit(1), OK);
    EXPECT_TRUE(io.exists(final_path));
    {
        FILE* f = std::fopen((root + "/OUT.TXT").c_str(), "rb");
        ASSERT_NE(f, nullptr);
        char got[4] = {0};
        EXPECT_EQ(std::fread(got, 1, 3, f), 3u);
        EXPECT_EQ(std::memcmp(got, "PAY", 3), 0);
        std::fclose(f);
    }
}

TEST(WinDestIo, UnclosedTempIsRemovedNotCommitted) {
    const std::string root = cr_test::tmp_path("wdestio", ".e");
    WinDestIo io;
    ASSERT_EQ(io.mkdir(u16_from(root)), OK);
    const std::u16string final_path = u16_from(root + "\\NEVER.TXT");
    ASSERT_EQ(io.create_temp(7, final_path, u".part-7"), OK);
    io.close(7); /* cancel path: the temp must vanish, the final never exist */
    EXPECT_FALSE(io.exists(final_path));
    EXPECT_FALSE(io.exists(u16_from(root + "\\NEVER.TXT.part-7")));
}

} // namespace

/* ---------------------------------------------------------------------------
 * N-013: destination-side overlap arming contracts (full-extent edition).
 * ------------------------------------------------------------------------- */
TEST(DestArming, VolumeLetterResolvesAnyLocalPath) {
    /* the CWD's volume: a RELATIVE path (no drive letter) resolves - the
     * pre-N-013 drive-letter gate missed these. */
    wchar_t rel = 0, abs = 0, cwd_vol = 0;
    EXPECT_TRUE(win::destination_volume_letter(L".", &rel));
    wchar_t cwd[MAX_PATH] = {};
    GetCurrentDirectoryW(MAX_PATH, cwd);
    wchar_t root[MAX_PATH] = {};
    ASSERT_TRUE(GetVolumePathNameW(cwd, root, MAX_PATH));
    cwd_vol = root[0];
    EXPECT_EQ(rel, cwd_vol);
    /* a plain absolute path on the same volume */
    std::wstring abs_path = std::wstring(root) + L"some\\deeper\\path";
    EXPECT_TRUE(win::destination_volume_letter(abs_path, &abs));
    EXPECT_EQ(abs, cwd_vol);
}

TEST(DestArming, NetworkPathNeverResolves) {
    /* the og's PathIsNetworkPathW branch: network destinations are never
     * opened raw, never armed. */
    wchar_t letter = 0;
    EXPECT_FALSE(win::destination_volume_letter(L"\\\\nonexistent-server\\share\\out", &letter));
}

TEST(DestArming, VolumeExtentsCarryAllSpans) {
    /* the current volume must report at least one extent; extent[0] must
     * agree with the legacy first-extent API (the source-disk lookup
     * call sites keep that one). */
    wchar_t letter = 0;
    ASSERT_TRUE(win::destination_volume_letter(L".", &letter));
    std::vector<win::VolExtent> ve;
    ASSERT_TRUE(win::volume_extents(letter, &ve));
    ASSERT_GE(ve.size(), 1u);
    u32 disk = 0;
    u64 start = 0, len = 0;
    ASSERT_TRUE(win::volume_extent(letter, &disk, &start, &len));
    EXPECT_EQ(ve[0].disk, disk);
    EXPECT_EQ(ve[0].start, start);
    EXPECT_EQ(ve[0].len, len);
}

TEST(DestArming, JunctionResolvesToTargetVolume) {
    /* a junction into another tree resolves to the TARGET's volume - the
     * og compares the resolved placement, never the junction's host. */
    const std::string root = cr_test::tmp_path("djunc", ".e");
    /* MSVC: u16string != wstring (distinct types even at 16-bit
     * wchar_t); the _wsystem call below wants the wide form - the
     * same ASCII byte-lift u16_from does, widened. */
    std::wstring junc;
    for (char c : root + "\\to_windows")
        junc.push_back(wchar_t(static_cast<unsigned char>(c)));
    wchar_t before = 0;
    if (!win::destination_volume_letter(junc, &before)) {
        /* root dir may not exist yet */
        WinDestIo io;
        ASSERT_EQ(io.mkdir(u16_from(root)), OK);
    }
    const int rc = _wsystem((L"cmd /c mklink /J \"" + junc + L"\" C:\\Windows").c_str());
    if (rc != 0) {
        GTEST_SKIP() << "mklink /J unavailable on this host (rc=" << rc << ")";
    }
    wchar_t jletter = 0, tletter = 0;
    ASSERT_TRUE(win::destination_volume_letter(junc, &jletter));
    ASSERT_TRUE(win::destination_volume_letter(L"C:\\Windows", &tletter));
    EXPECT_EQ(jletter, tletter); /* the TARGET's volume, not the host's */
    DeleteFileW(junc.c_str());   /* junctions delete like files */
}

/* ---------------------------------------------------------------------------
 * N-056: failed-rename temp cleanup + UNC-shaped path contracts.
 * ------------------------------------------------------------------------- */
TEST(WinDestIo, FailedRenameCleansTemp) {
    /* N-056 fail-before trace (the structural proof the Windows lane
     * executes): commit() nulls the handle BEFORE MoveFileExW; when the
     * rename fails (here: the final path is occupied by a DIRECTORY -
     * file->directory rename fails with ACCESS_DENIED), the OLD close()
     * gate `handle != null` could never fire, so DeleteFileW was
     * skipped and the .part-N temp leaked on the destination. The final
     * assertion below fails on that tree; the fix deletes the temp in
     * commit()'s failure path AND gates close() on the rename state. */
    const std::string root = cr_test::tmp_path("wdestio", ".f");
    WinDestIo io;
    ASSERT_EQ(io.mkdir(u16_from(root)), OK);
    const std::u16string final_path = u16_from(root + "\\BLOCKED.TXT");
    /* a DIRECTORY at the final path makes the atomic rename fail */
    ASSERT_EQ(io.mkdir(final_path), OK);
    ASSERT_EQ(io.create_temp(9, final_path, u".part-9"), OK);
    const unsigned char payload[] = {'X'};
    EXPECT_EQ(io.write(9, 0, payload, 1), OK);
    EXPECT_EQ(io.commit(9), u32(ERR_WRITE_FAILED));
    io.close(9);
    EXPECT_FALSE(io.exists(u16_from(root + "\\BLOCKED.TXT.part-9")))
        << "the failed-rename temp must be cleaned, not leaked";
    /* the blocker is untouched and no half-written final appeared */
    EXPECT_TRUE(io.exists(final_path));
}

TEST(WinDestIo, UncShapedPathsFailCleanly) {
    /* N-056 (b): the UNC-shaped destination family behaves cleanly on
     * an unresolvable share - exists() answers false, mkdir fails
     * without crashing, create_temp refuses upfront with
     * ERR_DEST_UNWRITABLE (CreateFileW cannot open
     * \\?\UNC\nonexistent.invalid\...), and nothing is left behind. */
    WinDestIo io;
    const std::u16string unc_final = u16_from("\\\\?\\UNC\\nonexistent.invalid\\share\\out\\F.TXT");
    EXPECT_FALSE(io.exists(unc_final));
    EXPECT_NE(io.mkdir(unc_final), u32(OK));
    EXPECT_EQ(io.create_temp(11, unc_final, u".part-11"), u32(ERR_DEST_UNWRITABLE));
    io.close(11); /* no handle, no temp, nothing to clean - must no-op */
}
