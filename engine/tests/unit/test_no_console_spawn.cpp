/* test_no_console_spawn.cpp — the D2 guard (the debug-and-release session).
 *
 * The user's report: "on opening disk health some kind of popup shells are
 * appearing, probably cmd or powershell". Root cause: engine code spawned a
 * child process through the CRT's _popen — which always creates a console
 * window in a GUI-subsystem host (cr_ui.dll is loaded in-process by the
 * Tauri shell, so the console flashed on the user's desktop).
 *
 * This suite pins the fix at the source level (the repo's established
 * source-pin pattern — test_enumerate_win.cpp's M-076 pin):
 *   1. NO console-spawning CRT/Win32 call sites exist in shipping engine
 *      code (_popen/_pclose/system/WinExec/ShellExecuteEx...). The ONLY
 *      sanctioned subprocess vehicle is CreateProcessW with
 *      CREATE_NO_WINDOW (+ SW_HIDE for the stdio piped form).
 *   2. The health surface's per-disk TRIM datum comes from the open device
 *      handle (query_trim_state_handle — no subprocess at all).
 *   3. The OS-global fsutil vehicle (query_trim_state) carries the hidden
 *      window discipline: CREATE_NO_WINDOW + SW_HIDE + piped stdio.
 *
 * These pins run on every lane (the Linux lane compiles the same sources
 * against tests/win-stub), so a future change that reintroduces a visible
 * console fails CI before it ships.
 */

#include <gtest/gtest.h>
#include "test_tmpdir.hpp" /* cr_test::read_repo_text (the source-pin pattern) */

#include <string>

TEST(D2NoConsole, NoConsoleSpawningCallsInShippingEngineCode) {
    /* The files that interact with the OS: the win platform layer, the ui
     * shim, the CLI app. If a new file needs process creation it must join
     * this list AND the CREATE_NO_WINDOW discipline. */
    const char* files[] = {
        "src/win/cr_health.cpp",      "src/win/cr_fsck.cpp",         "src/win/cr_bitlocker.cpp",
        "src/win/cr_enumerate.cpp",   "src/win/cr_transport.cpp",    "src/win/cr_image_win.cpp",
        "src/win/cr_mountpoints.cpp", "src/win/cr_elevate.cpp",      "src/win/cr_destio.cpp",
        "src/win/cr_bootfix.cpp",     "src/win/cr_image_create.cpp", "src/ui/cr_ui.cpp",
        "src/app/rrecov.cpp",
    };
    for (const char* f : files) {
        const std::string src = cr_test::read_repo_text(f);
        ASSERT_FALSE(src.empty()) << f << " could not be read (moved?)";
        EXPECT_EQ(std::string::npos, src.find("_popen"))
            << f << ": _popen allocates a console in a GUI host (D2)";
        EXPECT_EQ(std::string::npos, src.find("_pclose"))
            << f << ": _pclose implies a _popen console (D2)";
        EXPECT_EQ(std::string::npos, src.find("WinExec"))
            << f << ": WinExec cannot hide the child console (D2)";
        EXPECT_EQ(std::string::npos, src.find("ShellExecute"))
            << f
            << ": ShellExecute* shows windows by default (D2); use "
               "CreateProcessW + CREATE_NO_WINDOW";
    }
}

TEST(D2NoConsole, SystemCallBan) {
    /* CRT system() is a console by definition — banned everywhere in the
     * engine (all of src/, not just the win layer: the ban is cheap). */
    const std::string health = cr_test::read_repo_text("src/win/cr_health.cpp");
    ASSERT_FALSE(health.empty());
    EXPECT_EQ(std::string::npos, health.find("system("))
        << "CRT system() spawns a visible console (D2)";
}

TEST(D2NoConsole, HealthTrimComesFromTheOpenHandle) {
    /* The per-disk health report must source TRIM from the open device
     * handle — the no-subprocess path (the user-visible health surface). */
    const std::string ui = cr_test::read_repo_text("src/ui/cr_ui.cpp");
    ASSERT_FALSE(ui.empty());
    const size_t at = ui.find("u32 health_read_impl(");
    ASSERT_NE(std::string::npos, at) << "health_read_impl moved?";
    const size_t end = ui.find("\nu32 ", at + 10);
    const std::string body = ui.substr(at, (end == std::string::npos ? ui.size() : end) - at);
    EXPECT_NE(std::string::npos, body.find("query_trim_state_handle"))
        << "the health report's TRIM must come from the per-device IOCTL, "
           "not a subprocess (D2)";
}

TEST(D2NoConsole, GlobalFsutilVehicleCarriesTheHiddenWindowDiscipline) {
    /* The OS-global TRIM primitive still shells fsutil (og-parity vehicle,
     * corpus DRWUI_decompiled.c:178726-178733) — so its implementation MUST
     * carry the full hidden-window discipline. */
    const std::string h = cr_test::read_repo_text("src/win/cr_health.cpp");
    ASSERT_FALSE(h.empty());
    const size_t at = h.find("u32 query_trim_state(bool& enabled)");
    ASSERT_NE(std::string::npos, at) << "query_trim_state moved?";
    const size_t handle_at = h.find("u32 query_trim_state_handle");
    const size_t end = (handle_at != std::string::npos ? handle_at : h.size());
    const std::string body = h.substr(at, end - at);
    EXPECT_NE(std::string::npos, body.find("CREATE_NO_WINDOW"))
        << "the fsutil child must set CREATE_NO_WINDOW (D2)";
    EXPECT_NE(std::string::npos, body.find("SW_HIDE"))
        << "the fsutil child must set wShowWindow=SW_HIDE (D2)";
    EXPECT_NE(std::string::npos, body.find("STARTF_USESTDHANDLES"))
        << "the fsutil child's stdio must be piped, not attached to a console (D2)";
    EXPECT_NE(std::string::npos, body.find("TerminateProcess"))
        << "the fsutil child must be bounded (terminate on timeout, D2)";
}

TEST(D2NoConsole, ExistingSubprocessSitesKeepTheNoWindowFlag) {
    /* The pre-existing subprocess sites (chkdsk launch, BitLocker
     * manage-bde) must keep CREATE_NO_WINDOW — the regression pin. */
    const std::string fsck = cr_test::read_repo_text("src/win/cr_fsck.cpp");
    ASSERT_FALSE(fsck.empty());
    EXPECT_NE(std::string::npos, fsck.find("CREATE_NO_WINDOW"))
        << "cr_fsck.cpp lost CREATE_NO_WINDOW (D2 regression)";
    const std::string bl = cr_test::read_repo_text("src/win/cr_bitlocker.cpp");
    ASSERT_FALSE(bl.empty());
    EXPECT_NE(std::string::npos, bl.find("CREATE_NO_WINDOW"))
        << "cr_bitlocker.cpp lost CREATE_NO_WINDOW (D2 regression)";
}

TEST(BF28ElevationCode, HealthOpenRefusalCarriesTheElevationCode) {
    /* BF-28: an unelevated ACCESS_DENIED on the health path must return
     * ERR_ELEVATION_REQUIRED (the uniform raw-open refusal code the bridge
     * maps to 'elevation-required'), never ERR_OPEN_FAILED with an
     * "administrator rights required" message-string standing in for the
     * code. Source pin over health_read_impl's open-failure branch. */
    const std::string ui = cr_test::read_repo_text("src/ui/cr_ui.cpp");
    ASSERT_FALSE(ui.empty());
    const size_t at = ui.find("u32 health_read_impl(");
    ASSERT_NE(std::string::npos, at) << "health_read_impl moved?";
    const size_t end = ui.find("\nu32 ", at + 10);
    const std::string body = ui.substr(at, (end == std::string::npos ? ui.size() : end) - at);
    EXPECT_NE(std::string::npos, body.find("ERR_ELEVATION_REQUIRED"))
        << "the unelevated ACCESS_DENIED refusal must carry the elevation CODE (BF-28)";
    EXPECT_NE(std::string::npos, body.find("le == ERROR_ACCESS_DENIED"))
        << "only the access-denied class is the elevation gate (not-ready/io stay open-failures)";
}
