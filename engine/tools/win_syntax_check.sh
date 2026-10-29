#!/usr/bin/env bash
# tools/win_syntax_check.sh - Linux-side PARSE GATE over the Windows-only
# platform layer (src/win/*.cpp) AND the Windows-only test files
# (tests/unit/test_*_win.cpp).
#
# Why this exists (M-055): the win layer compiles only under MSVC, and a
# build-breaking edit there is otherwise invisible on the Linux lane until
# a Windows CI run executes. Two such breaks (cr_destio.cpp's free_bytes
# nested inside write() since 915bce3; cr_transport.cpp's volume_extents
# defined outside namespace cr::win since 3b5397a) survived multiple
# pushes because no Linux-visible check parses these files. This gate
# runs g++ -fsyntax-only against the declaration-only SDK stubs in
# tests/win-stub/, so structural corruption of the win layer fails the
# Linux suite immediately.
#
# N-228 (the same lesson, the test directory): test_fsck_win.cpp landed
# (d10ae333) with unresolved names - bare u32 and Err constants that
# only resolve through cr:: - and every local lane was green because
# the file is WIN32-gated out of the Linux build; the first MSVC
# compile was checks.yml run 321 (red). The test files now parse here
# too, so the class dies on the Linux lane.
#
# The stubs are DECLARATIONS ONLY (never linked, never run): they
# re-declare the exact SDK surface src/win consumes, with SDK-faithful
# shapes (ULONG_PTR width, 16-bit wchar_t via -fshort-wchar). A new SDK
# symbol in src/win grows the stub here - that is the gate working.
#
# Licensed under the MIT license. See LICENSE.
set -u

SRC_DIR="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
STUB_DIR="$SRC_DIR/tests/win-stub"
CXX="${CXX:-g++}"

if [ ! -d "$STUB_DIR" ]; then
    echo "win_syntax_check: stub dir missing: $STUB_DIR" >&2
    exit 2
fi

# -fshort-wchar: wchar_t is 16-bit on MSVC/x64; L"" literals and
#   wchar_t buffers must have the same width the MSVC lane sees.
# -D_WIN32: the layer's #ifdef _WIN32 branches (e.g. cr_bitlocker's
#   registry detail namespace) must parse exactly as MSVC sees them.
# -D__declspec(x)=: ELF-target g++ rejects __declspec (it is a Windows
#   target extension); neutralize it for the PARSE ONLY (N-033's CLI
#   landing made src/app include cr/cr_ui.h, whose _WIN32 export branch
#   uses __declspec(dllimport) - MSVC-legal, ELF-g++-unparseable). The
#   gate checks STRUCTURE, not import semantics; the real macro is
#   exercised by the MSVC lane on Windows CI.

# -D'__declspec(x)=': the app layer includes cr_ui.h (whose CR_UI_API
# expands to __declspec(dllexport/dllimport) under _WIN32) possibly BEFORE
# windows.h - a command-line function-like no-op is include-order-proof
# (the gate never links, so the storage class is irrelevant to parsing).
CXXFLAGS="-std=c++20 -fshort-wchar -D_WIN32 -D__declspec(x)= -fsyntax-only -I$STUB_DIR -I$SRC_DIR/include"

rc=0
for f in "$SRC_DIR"/src/win/*.cpp; do
    name=$(basename "$f")
    if ! out=$("$CXX" $CXXFLAGS "$f" 2>&1); then
        echo "win_syntax_check: FAIL: $name does not parse" >&2
        echo "$out" | grep -E "error:" | head -20 >&2
        rc=1
    else
        echo "win_syntax_check: ok: $name"
    fi
done

# The app layer (src/app/*.cpp) and the UI shim (src/ui/*.cpp) are
# Windows-only for the same reason the win layer is (wmain, _wfopen_s,
# the cr::win surface, the SDK-heavy _WIN32 blocks); the gate covers
# them too (structural breaks in Windows-only directories were exactly
# the M-055 failure class, one directory over - the cr_ui ENUMERATE_
# DISK_BOUND/psapi/share breakage of the M-030/S-008/N-062 window was
# the same class, invisible until the MSVC lane ran).
for f in "$SRC_DIR"/src/app/*.cpp "$SRC_DIR"/src/ui/*.cpp; do
    name=$(basename "$f")
    if ! out=$("$CXX" $CXXFLAGS "$f" 2>&1); then
        echo "win_syntax_check: FAIL: app/$name does not parse" >&2
        echo "$out" | grep -E "error:" | head -20 >&2
        rc=1
    else
        echo "win_syntax_check: ok: app/$name"
    fi
done

# N-228: the Windows-only TEST files are the same invisible-until-MSVC
# class (the M-055 lesson, one directory over, twice). test_fsck_win.cpp
# shipped with unresolved names (cr::u32 / the Err constants) and NO
# Linux-visible gate ever saw it - the first MSVC compile was CI run
# 321 (checks.yml red at 374c54cb). The gate now parses every
# tests/unit/test_*_win.cpp the same way. Two additions the src lanes
# never needed:
#   - gtest's headers (GTEST_OS_WINDOWS pulls <direct.h>/<io.h> - the
#     stubs grew them, plus the _fileno/_stat/_stricmp/_wfopen surface)
#   - CR_SOURCE_DIR, which the CMake test targets define for
#     test_tmpdir.hpp's fixture reads (a quoted path, same shape)
# The gtest include dir is located from the build tree (FetchContent
# materializes it at configure time, so ctest always finds it post-build);
# GTEST_INCLUDE may override. NOT FOUND IS A FAILURE, never a skip.
# N-231: both build layouts are covered - the preset subdir
# (build/<preset>/_deps/..., the CI lanes) AND the flat build dir
# (build/_deps/..., a direct cmake configure - the N-228 section failed
# on the flat layout, a gate-robustness defect, not a tree defect).
GTEST_INCLUDE_DIR="${GTEST_INCLUDE:-}"
if [ -z "$GTEST_INCLUDE_DIR" ]; then
    for cand in "$SRC_DIR"/build/*/_deps/googletest-src/googletest/include \
                "$SRC_DIR"/build/_deps/googletest-src/googletest/include; do
        if [ -d "$cand" ]; then
            GTEST_INCLUDE_DIR="$cand"
            break
        fi
    done
fi
if [ ! -d "$GTEST_INCLUDE_DIR" ]; then
    echo "win_syntax_check: FAIL: no gtest include dir found (build one preset " >&2
    echo "  first - FetchContent drops it at build/*/_deps/googletest-src/googletest/include -" >&2
    echo "  or export GTEST_INCLUDE). The N-228 test-file section cannot run; a" >&2
    echo "  missing gate is a failure, never a skip." >&2
    exit 2
fi
TESTCXXFLAGS="$CXXFLAGS -DCR_SOURCE_DIR=\"$SRC_DIR\" -I$GTEST_INCLUDE_DIR"
for f in "$SRC_DIR"/tests/unit/test_*_win.cpp; do
    name=$(basename "$f")
    if ! out=$("$CXX" $TESTCXXFLAGS "$f" 2>&1); then
        echo "win_syntax_check: FAIL: tests/$name does not parse" >&2
        echo "$out" | grep -E "error:" | head -20 >&2
        rc=1
    else
        echo "win_syntax_check: ok: tests/$name"
    fi
done
exit $rc
