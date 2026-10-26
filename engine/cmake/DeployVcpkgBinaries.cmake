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
# SINGLE DEPLOYER (release run 2, v0.5.0-rc2): z-applocal and this backstop
# used to run together and RACED under parallel MSBuild — z-applocal for
# rrecov died with 'Access is denied' (MSB3075, exit 5) while this backstop
# was copying the same DLLs into the same directory. The win presets now
# set VCPKG_APPLOCAL_DEPS=OFF: this script is the ONLY deployer. It also
# RETRIES each copy: several targets share a deploy directory (five test
# executables all land in tests/<CFG>), and under --parallel their POST_BUILD
# deploys can overlap on the same destination file — a Windows sharing
# violation at that moment is transient, not fatal, and is handled as such.
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
        # BOTH trees: release names first, then debug/bin overwrites on
        # collisions - covers import-name drift in either direction
        # (evidence from the 0d0f706 diagnostics: the exe wanted the
        # release name lz4.dll while only lz4d.dll was deployed).
        file(GLOB rel_dlls "${triplet}/bin/*.dll")
        file(GLOB dbg_dlls "${triplet}/debug/bin/*.dll")
        set(dlls ${rel_dlls} ${dbg_dlls})
    else()
        file(GLOB dlls "${triplet}/bin/*.dll")
    endif()
    foreach(dll ${dlls})
        if(EXISTS "${dll}")
            set(deployed FALSE)
            foreach(attempt RANGE 3) # 4 attempts, 250 ms apart - covers a
                # concurrent POST_BUILD deploy from a sibling target
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E copy "${dll}" "${OUT_DIR}"
                    RESULT_VARIABLE copy_rc)
                if(copy_rc EQUAL 0)
                    set(deployed TRUE)
                    break()
                endif()
                execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 0.25)
            endforeach()
            if(NOT deployed)
                message(FATAL_ERROR
                    "DeployVcpkgBinaries: ${dll} -> ${OUT_DIR} failed after 4 attempts")
            endif()
            message(STATUS "deployed ${dll} -> ${OUT_DIR}")
        endif()
    endforeach()
endforeach()
