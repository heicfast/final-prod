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
# ---------------------------------------------------------------------------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LZ4 QUIET IMPORTED_TARGET liblz4)
    pkg_check_modules(ZSTD QUIET IMPORTED_TARGET libzstd)
endif()
if(NOT TARGET PkgConfig::LZ4)
    find_library(LZ4_LIB NAMES lz4)
    find_path(LZ4_HEADER lz4.h)
    if(LZ4_LIB AND LZ4_HEADER)
        add_library(PkgConfig::LZ4 UNKNOWN IMPORTED)
        set_target_properties(PkgConfig::LZ4 PROPERTIES
            IMPORTED_LOCATION "${LZ4_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${LZ4_HEADER}/..")
    endif()
endif()
if(NOT TARGET PkgConfig::ZSTD)
    find_library(ZSTD_LIB NAMES zstd)
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
