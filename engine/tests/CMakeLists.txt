# tests - GoogleTest suite over the pure core (runs on Linux AND Windows).
# The Windows-only layers (transport/enumeration) are exercised by integration
# scripts on real hosts - see docs/testing.md.

add_executable(cr_tests
    unit/main.cpp
    unit/test_fixup.cpp
    unit/test_image.cpp
    unit/test_nvme_health.cpp
    unit/test_runlist.cpp
    unit/test_crc32_gpt.cpp
    unit/test_slice_devmap.cpp
    unit/test_cached.cpp
    unit/test_directalign.cpp
    unit/test_retry.cpp
    unit/test_parallel.cpp
    unit/test_node_tree.cpp
    unit/test_m101_branches.cpp
    unit/test_grading.cpp
    unit/test_health_grade.cpp
    unit/test_health_csmi.cpp
    unit/test_enumerate_win.cpp
    unit/test_ntfs_volume.cpp
    unit/test_fat_volume.cpp
    unit/test_exfat_volume.cpp
    unit/test_ext4_volume.cpp
    unit/test_ext4_m144_large_volume.cpp
    unit/test_ext4_m143_journal_inode_image.cpp
    unit/test_hfsplus_volume.cpp
    unit/test_btrfs_volume.cpp
    unit/test_zfs_volume.cpp
    unit/test_refs_ldm.cpp
    unit/test_ldm.cpp
    unit/test_lostpart.cpp
    unit/test_folderfilter.cpp
    unit/test_reghive_edit.cpp
    unit/test_mountpoints2.cpp
    unit/test_sha256.cpp
    unit/test_bitlocker_xts.cpp
    unit/test_dict.cpp
    unit/test_enumerate_merge.cpp
    unit/test_resume_revalidation.cpp
    unit/test_text.cpp
    unit/test_carve.cpp
    unit/test_config_display.cpp
    unit/test_vollabel.cpp
    unit/test_session.cpp
    unit/test_orchestrator.cpp
    unit/test_transport_logic.cpp
    unit/test_driver_status.cpp
    unit/test_scan_truth.cpp
    unit/test_pause_resume.cpp
    unit/test_telemetry.cpp
    unit/test_writer_dirs.cpp
    unit/test_writer_readpipe.cpp
    unit/test_ldm_recovery.cpp
    unit/test_report.cpp
    unit/test_reghive.cpp
    unit/test_bcdfix.cpp
    unit/test_nvramfix.cpp
    unit/test_preview.cpp
    unit/test_thumbnail.cpp
    unit/test_snapshot.cpp
    unit/test_query.cpp
    unit/test_fscheck.cpp
    unit/test_fscheck_launch.cpp
    unit/test_fve_policy.cpp
    unit/test_query_perf.cpp
    unit/test_diskmap.cpp
    unit/test_apply.cpp
    unit/test_rawfixer.cpp
    unit/test_consistency.cpp
    unit/test_refs_volume.cpp
)

target_link_libraries(cr_tests PRIVATE cr::core GTest::gtest_main)

# S-030 image fixtures (qemu-generated): copied beside the test binary so
# the compat tests find data/images/ regardless of the launch directory
add_custom_command(TARGET cr_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/images
            $<TARGET_FILE_DIR:cr_tests>/data/images
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/exfat
            $<TARGET_FILE_DIR:cr_tests>/data/exfat)
# cr_thumbnail fixtures (scripts/gen_thumbnail_fixtures.py output)
add_custom_command(TARGET cr_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/thumbnails
            $<TARGET_FILE_DIR:cr_tests>/data/thumbnails)

# N-045 doc-truth pin lane: the register-vs-source consistency tests
# (e.g. Cached.ScanCacheBudgetIsTheRegisteredDivergence) read repo files
# at runtime - hand them the source root (works on MSVC and gcc lanes).
target_compile_definitions(cr_tests PRIVATE
    CR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# vcpkg DLL deployment backstop (no-op on Linux): applocal import-name
# resolution skipped the lz4 debug DLL on windows CI (0xc0000135)
cr_deploy_vcpkg_binaries(cr_tests)

# ---------------------------------------------------------------------------
# Windows-only layer contracts (transport/enumeration/destio) - built and run
# on the MSVC lane only; the portable core is fully covered by cr_tests.
# ---------------------------------------------------------------------------
if(WIN32)
    add_executable(cr_destio_tests unit/test_destio_win.cpp)
    target_link_libraries(cr_destio_tests PRIVATE cr::win GTest::gtest_main)
    target_include_directories(cr_destio_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    # test_tmpdir.hpp's read_repo_file needs the source root (the cr_tests
    # define above; the win-only target was missing it - Windows CI C2065)
    target_compile_definitions(cr_destio_tests PRIVATE
        CR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    cr_deploy_vcpkg_binaries(cr_destio_tests)
    add_test(NAME destio COMMAND cr_destio_tests)

    # S-014: BitLocker auto-unlock contracts (live-path seam, per-GUID
    # enumeration with corpus caps, offline-hive harvest + selection)
    add_executable(cr_bitlocker_tests unit/test_bitlocker_win.cpp)
    target_link_libraries(cr_bitlocker_tests PRIVATE cr::win GTest::gtest_main)
    cr_deploy_vcpkg_binaries(cr_bitlocker_tests)
    add_test(NAME bitlocker COMMAND cr_bitlocker_tests)

    # M-075: the chkdsk launch endpoint's wiring contracts (the seam
    # battery; the pure grammar battery rides cr_tests on both lanes)
    add_executable(cr_fsck_win_tests unit/test_fsck_win.cpp)
    target_link_libraries(cr_fsck_win_tests PRIVATE cr::win GTest::gtest_main)
    cr_deploy_vcpkg_binaries(cr_fsck_win_tests)
    add_test(NAME fsck_win COMMAND cr_fsck_win_tests)

    # S-040: the flat-raw image producer's file disposition contract
    # (atomic temp+rename commit, .sha256 sidecar, the explicit .partial
    # on cancel/failure; the class-(a) driver path rides the
    # staged-scenarios lane)
    add_executable(cr_image_create_tests unit/test_image_create_win.cpp)
    target_link_libraries(cr_image_create_tests PRIVATE cr::win GTest::gtest_main)
    target_include_directories(cr_image_create_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    target_compile_definitions(cr_image_create_tests PRIVATE
        CR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    cr_deploy_vcpkg_binaries(cr_image_create_tests)
    add_test(NAME image_create COMMAND cr_image_create_tests)
endif()

# ---------------------------------------------------------------------------
# G-01 / W2-1..W2-3: the cr_ui binding-layer checks.
#
# Two SEPARATE executables (never linked into cr_tests): cr_tests statically
# links cr_core, and libcr_ui embeds cr_core too - linking both into one
# binary duplicates every core symbol (ASan ODR). The shim is tested exactly
# as a host consumes it: through the shared library.
#
# cr_ui_tests    - GoogleTest suite over the flat-C surface (C++ caller)
# cr_ui_abi_check- PLAIN-C consumer: proves the header compiles/links under
#                  a C compiler and drives the full lifecycle (scan ->
#                  session -> snapshot -> query -> preview -> recover ->
#                  report) against a synthetic FAT16 image, 1:1 with the
#                  CLI flows.
# ---------------------------------------------------------------------------
add_executable(cr_ui_tests unit/test_cr_ui.cpp)
target_link_libraries(cr_ui_tests PRIVATE cr::ui GTest::gtest_main)
cr_deploy_vcpkg_binaries(cr_ui_tests)
if(WIN32)
    # The MSVC shim exports only the cr_ui_* surface (dllexport), so the
    # test's DIRECT core-API uses (the DiskMap/ResultTree fixtures, the
    # log-sink seam, ReadErrorMap) cannot resolve from cr_ui.dll the way
    # Linux's default-visibility .so provides them - the Windows lane's
    # LNK2019 batch. Link the static core on Windows ONLY: the Linux ASan
    # design above (never link core twice into one binary) is untouched,
    # and on MSVC the exe's static copy and the DLL's embedded copy never
    # interbreed - the flat-C surface never crosses a core type.
    target_link_libraries(cr_ui_tests PRIVATE cr::core)
endif()
# doc-truth pins live in both binaries (N-040/N-045 lanes)
target_compile_definitions(cr_ui_tests PRIVATE
    CR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_executable(cr_ui_abi_check ui_abi_check.c)
target_link_libraries(cr_ui_abi_check PRIVATE cr::ui)
cr_deploy_vcpkg_binaries(cr_ui_abi_check)

if(WIN32)
    # keep cr_ui.dll beside the shim test executables (they live in tests/)
    foreach(t cr_ui_tests cr_ui_abi_check)
        add_custom_command(TARGET ${t} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:cr_ui> $<TARGET_FILE_DIR:${t}>)
    endforeach()
endif()

add_test(NAME cr_ui_abi_check
    COMMAND cr_ui_abi_check ${CMAKE_CURRENT_BINARY_DIR}/cr_ui_abi_check.img)

if(CR_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(cr_tests PRIVATE -fsanitize=address)
    target_link_options(cr_tests PRIVATE -fsanitize=address)
    # the shim executables load the ASan-instrumented libcr_ui.so: the
    # runtime must be linked first (otherwise "ASan runtime does not come
    # first")
    foreach(t cr_ui_tests cr_ui_abi_check)
        target_compile_options(${t} PRIVATE -fsanitize=address)
        target_link_options(${t} PRIVATE -fsanitize=address)
    endforeach()
endif()

include(GoogleTest)
gtest_discover_tests(cr_tests)
gtest_discover_tests(cr_ui_tests)

# ---------------------------------------------------------------------------
# M-055: Linux-side PARSE GATE over the Windows-only platform layer.
#
# src/win/*.cpp compiles only under MSVC; two build-breaking edits there
# (cr_destio.cpp free_bytes nested inside write(); cr_transport.cpp
# volume_extents outside namespace cr::win) survived multiple pushes
# because no Linux-visible check parses these files. This gate runs
# g++ -fsyntax-only over the whole win layer against the declaration-only
# SDK stubs in tests/win-stub/ - structural corruption now fails the
# Linux suite immediately. (The stubs declare, never link or run; a new
# SDK symbol in src/win grows the stub - that failure is the gate
# working.) Windows lane unaffected: MSVC compiles these files for real.
# ---------------------------------------------------------------------------
if(NOT WIN32 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_test(NAME win_syntax_check
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tools/win_syntax_check.sh
                ${CMAKE_SOURCE_DIR})
endif()
