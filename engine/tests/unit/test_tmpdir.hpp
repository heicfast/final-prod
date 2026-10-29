/* test_tmpdir.hpp - cross-platform temp paths for the unit suite.
 *
 * MSVC's fopen/CRT does NOT resolve POSIX paths like "/tmp/..." (it maps
 * them to <current-drive>\tmp, which does not exist on CI runners) - the
 * Windows lane failed 3 tests over exactly this (GAP-CI-1). Everything
 * now resolves through std::filesystem::temp_directory_path() (TMPDIR /
 * TMP / TEMP / /tmp fallback - identical to /tmp on the Linux runners,
 * where the ASan job's fixed-path validator consumer still works).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#ifndef CR_TEST_TMPDIR_HPP_
#define CR_TEST_TMPDIR_HPP_

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#if defined(_WIN32)
#include <process.h> /* _getpid (POSIX getpid is C4996-deprecated on MSVC) */
#define CR_TEST_GETPID _getpid
#else
#include <unistd.h>
#define CR_TEST_GETPID getpid
#endif

namespace cr_test {

/* Unique per-process per-call path: <tmpdir>/<prefix>_<pid>_<n><suffix>.
 * Safe for parallel ctest shards and repeated runs. */
inline std::string tmp_path(const char* prefix, const char* suffix) {
    static std::atomic<int> n{0};
    char tail[96];
    snprintf(tail, sizeof(tail), "%s_%d_%d%s", prefix, CR_TEST_GETPID(), n.fetch_add(1), suffix);
    return (std::filesystem::temp_directory_path() / tail).string();
}

/* Fixed per-run name (no uniquifier): for consumers that must know the
 * path up front (the CI lane validates cr_report_sample.json after the
 * suite exits). One test binary writes it exactly once. */
inline std::string tmp_file(const char* name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

/* Whole-file read for the register-vs-source doc-truth pins (N-040 /
 * N-045 lane): reads a repo file relative to CR_SOURCE_DIR (stamped by
 * tests/CMakeLists.txt). ADD_FAILURE + empty string when the tree runs
 * out-of-source without the define. */
inline std::string read_repo_file(const char* relpath) {
    const std::string path = std::string(CR_SOURCE_DIR) + "/" + relpath;
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        ADD_FAILURE() << "cannot open " << path << " (out-of-source run without CR_SOURCE_DIR?)";
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/* N-235: the source-truth pins' reader - read_repo_file is byte-exact
 * (binary fixtures ride it: junk-gate-fat16.raw carries a natural 0x0D
 * byte; a CR strip there corrupts the image - first-hand at the v0.6.0
 * close). The PINS match LF patterns, and the Windows runner's git
 * checkout (autocrlf) materializes CRLF - so the pin sites read through
 * THIS helper, which normalizes CRLF -> LF. An LF checkout has no CRs:
 * byte-identical for it. First observed at the v0.6.0 release
 * validation: M-148's pin (test_orchestrator.cpp:3319) never ran on
 * Windows before (every post-v0.5.2 checks run died before ctest); the
 * binary read saw \r\n and the \n pattern failed. */
inline std::string read_repo_text(const char* relpath) {
    std::string raw = read_repo_file(relpath);
    raw.erase(std::remove(raw.begin(), raw.end(), '\r'), raw.end());
    return raw;
}

} // namespace cr_test

#endif // CR_TEST_TMPDIR_HPP_
