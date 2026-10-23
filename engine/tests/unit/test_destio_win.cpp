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
#include "cr/win/cr_destio.h"
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
        out.push_back(char16_t(unsigned char(c)));
    return out;
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
