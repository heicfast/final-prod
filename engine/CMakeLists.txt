cmake_minimum_required(VERSION 3.25)

project(core_recovery
    VERSION 0.1.0
    DESCRIPTION "User-mode Windows data-recovery stack on the RDA driver"
    LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global hygiene: identical strictness on every compiler we run (MSVC /W4,
# gcc/clang -Wall -Wextra -Wpedantic). /WX everywhere. C++20 for the engine
# layer; the pure wire core is C-compatible where practical.
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /permissive- /Zc:preprocessor /utf-8
        /external:W0 /wd28251)   # WDK/SDK headers stay quiet via /external
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wno-missing-field-initializers)
    endif()
endif()

option(CR_ENABLE_ASAN       "Build pure core with AddressSanitizer"     OFF)
option(CR_BUILD_TOOLS       "Build host tools (fixture generator)"      ON)
option(CR_BUILD_APP         "Build the rrecov CLI (Windows only)"       ON)
option(CR_ENABLE_TESTS      "Build the unit-test suite"                ON)

# ---------------------------------------------------------------------------
# Zlib (session container compression - the .rsf proven scheme).
# On Windows vcpkg supplies it; on Linux the system package.
# ---------------------------------------------------------------------------
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    # CI images and dev containers ship zlib; if the headers are still missing
    # the session library fails at generate time with a clear message.
    message(FATAL_ERROR "zlib not found - install zlib (apt: zlib1g-dev; vcpkg: zlib)")
endif()

# ---------------------------------------------------------------------------
# LZ4 + zstd (ZFS compressed-extent decoding). Optional at build time: the
# decompressor reports unavailable codecs instead of failing the build, but
# CI builds must carry both so real pools decode (apt: liblz4-dev libzstd-dev;
# vcpkg: lz4 zstd).
#
# Discovery order (0xc0000135 postmortem, three CI cycles):
#   1. CONFIG package (vcpkg)  - the ONLY source with per-config import
#      mapping: Debug links debug/lib/liblz4d.lib -> the exe imports
#      liblz4d.dll, which is exactly what debug/bin contains. MUST be
#      tried FIRST.
#   2. pkg-config - the pc files are release-flavoured and carry NO
#      config mapping: on an MSVC Debug build the linker resolves the
#      RELEASE import lib, the exe imports liblz4.dll, and the debug
#      tree (which only has liblz4d.dll) cannot satisfy it -> the test
#      exe dies at startup with STATUS_DLL_NOT_FOUND. Keep ONLY as the
#      Linux fallback (single-config systems - correct there).
#   3. bare find_library - last resort (same release-bias caveat; also
#      widen to the liblz4/zstd/liblz4 naming variants).
# ---------------------------------------------------------------------------
find_package(liblz4 CONFIG QUIET)
if(TARGET liblz4::liblz4 AND NOT TARGET PkgConfig::LZ4)
    add_library(PkgConfig::LZ4 ALIAS liblz4::liblz4)
endif()
find_package(PkgConfig QUIET)
if(NOT TARGET PkgConfig::LZ4 AND PkgConfig_FOUND)
    pkg_check_modules(LZ4 QUIET IMPORTED_TARGET liblz4)
endif()
if(NOT TARGET PkgConfig::LZ4)
    find_library(LZ4_LIB NAMES lz4 liblz4)
    find_path(LZ4_HEADER lz4.h)
    if(LZ4_LIB AND LZ4_HEADER)
        add_library(PkgConfig::LZ4 UNKNOWN IMPORTED)
        set_target_properties(PkgConfig::LZ4 PROPERTIES
            IMPORTED_LOCATION "${LZ4_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${LZ4_HEADER}/..")
    endif()
endif()

find_package(zstd CONFIG QUIET)
if(TARGET zstd::libzstd AND NOT TARGET PkgConfig::ZSTD)
    add_library(PkgConfig::ZSTD ALIAS zstd::libzstd)
endif()
if(NOT TARGET PkgConfig::ZSTD AND PkgConfig_FOUND)
    pkg_check_modules(ZSTD QUIET IMPORTED_TARGET libzstd)
endif()
if(NOT TARGET PkgConfig::ZSTD)
    find_library(ZSTD_LIB NAMES zstd libzstd)
    find_path(ZSTD_HEADER zstd.h)
    if(ZSTD_LIB AND ZSTD_HEADER)
        add_library(PkgConfig::ZSTD UNKNOWN IMPORTED)
        set_target_properties(PkgConfig::ZSTD PROPERTIES
            IMPORTED_LOCATION "${ZSTD_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_HEADER}/..")
    endif()
endif()

if(TARGET PkgConfig::LZ4)
    set(CR_LZ4_LINK "PkgConfig::LZ4")
endif()
if(TARGET PkgConfig::ZSTD)
    set(CR_ZSTD_LINK "PkgConfig::ZSTD")
endif()

# ---------------------------------------------------------------------------
# vcpkg runtime DLL deployment backstop (windows, manifest mode).
# z-applocal resolves import-table names against the installed tree and
# silently skips mismatches - the exe then dies with 0xc0000135 (observed
# with the lz4/zstd wave). cr_deploy_vcpkg_binaries() instead copies every
# DLL of the matching config next to the target at POST_BUILD time.
# ---------------------------------------------------------------------------
function(cr_deploy_vcpkg_binaries target)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed")
        return() # Linux/system-libs builds: nothing to deploy
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DBIN_ROOT=${CMAKE_BINARY_DIR}
            -DOUT_DIR=$<TARGET_FILE_DIR:${target}>
            -DCFG=$<CONFIG>
            -P ${CMAKE_SOURCE_DIR}/cmake/DeployVcpkgBinaries.cmake
        COMMENT "deploy vcpkg runtime DLLs for ${target} ($<CONFIG>)")
endfunction()

add_subdirectory(src)
if(CR_ENABLE_TESTS)
    enable_testing()
    find_package(GTest)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL "https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz")
        FetchContent_MakeAvailable(googletest)
    endif()
    add_subdirectory(tests)
endif()
