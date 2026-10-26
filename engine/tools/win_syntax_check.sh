#!/usr/bin/env bash
# tools/win_syntax_check.sh - Linux-side PARSE GATE over the Windows-only
# platform layer (src/win/*.cpp).
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

# The app layer (src/app/*.cpp) is Windows-only for the same reason the
# win layer is (wmain, _wfopen_s, the cr::win surface); N-009c's fix
# edits it, so the parse gate now covers it too (an app-layer structural
# break was exactly the M-055 failure class, one directory over).
for f in "$SRC_DIR"/src/app/*.cpp; do
    name=$(basename "$f")
    if ! out=$("$CXX" $CXXFLAGS "$f" 2>&1); then
        echo "win_syntax_check: FAIL: app/$name does not parse" >&2
        echo "$out" | grep -E "error:" | head -20 >&2
        rc=1
    else
        echo "win_syntax_check: ok: app/$name"
    fi
done
exit $rc
