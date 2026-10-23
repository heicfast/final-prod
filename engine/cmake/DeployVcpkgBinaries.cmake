# DeployVcpkgBinaries.cmake - post-build DLL deployment backstop.
#
# vcpkg's z-applocal deploys the runtime DLLs of a target by resolving the
# PE import-table names against the installed tree. Any name-mapping quirk
# (d-postfix DLLs, toolchain find_library resolving the release import lib
# in Debug builds) silently skips the copy and the exe dies at startup with
# 0xc0000135 (STATUS_DLL_NOT_FOUND) - observed on the windows CI for the
# lz4/zstd compression wave. This script instead copies EVERY runtime DLL
# of the matching configuration from the manifest-installed tree next to
# the target binary: deterministic, name-agnostic, no import parsing.
#
# Expected invocation (see cr_deploy_vcpkg_binaries() in the top-level
# CMakeLists.txt): cmake -DBIN_ROOT=... -DOUT_DIR=... -DCFG=Debug -P ...

if(NOT DEFINED BIN_ROOT OR NOT DEFINED OUT_DIR OR NOT DEFINED CFG)
    message(FATAL_ERROR "DeployVcpkgBinaries: BIN_ROOT, OUT_DIR and CFG are required")
endif()

file(GLOB triplet_dirs "${BIN_ROOT}/vcpkg_installed/*")
if(triplet_dirs STREQUAL "")
    return() # not a vcpkg-manifest build (Linux system libs) - nothing to do
endif()

foreach(triplet ${triplet_dirs})
    if(NOT IS_DIRECTORY "${triplet}")
        continue()
    endif()
    if(CFG STREQUAL "Debug")
        file(GLOB dlls "${triplet}/debug/bin/*.dll")
    else()
        file(GLOB dlls "${triplet}/bin/*.dll")
    endif()
    foreach(dll ${dlls})
        if(EXISTS "${dll}")
            file(COPY "${dll}" DESTINATION "${OUT_DIR}")
            message(STATUS "deployed ${dll} -> ${OUT_DIR}")
        endif()
    endforeach()
endforeach()
