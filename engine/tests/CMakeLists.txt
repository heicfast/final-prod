# tests - GoogleTest suite over the pure core (runs on Linux AND Windows).
# The Windows-only layers (transport/enumeration) are exercised by integration
# scripts on real hosts - see docs/testing.md.

add_executable(cr_tests
    unit/main.cpp
    unit/test_fixup.cpp
    unit/test_nvme_health.cpp
    unit/test_runlist.cpp
    unit/test_crc32_gpt.cpp
    unit/test_slice_devmap.cpp
    unit/test_cached.cpp
    unit/test_retry.cpp
    unit/test_parallel.cpp
    unit/test_node_tree.cpp
    unit/test_ntfs_volume.cpp
    unit/test_fat_volume.cpp
    unit/test_exfat_volume.cpp
    unit/test_ext4_volume.cpp
    unit/test_hfsplus_volume.cpp
    unit/test_btrfs_volume.cpp
    unit/test_zfs_volume.cpp
    unit/test_refs_ldm.cpp
    unit/test_ldm.cpp
    unit/test_lostpart.cpp
    unit/test_sha256.cpp
    unit/test_carve.cpp
    unit/test_session.cpp
    unit/test_orchestrator.cpp
    unit/test_pause_resume.cpp
    unit/test_telemetry.cpp
    unit/test_writer_dirs.cpp
    unit/test_report.cpp
    unit/test_preview.cpp
    unit/test_snapshot.cpp
    unit/test_query.cpp
    unit/test_query_perf.cpp
    unit/test_rawfixer.cpp
    unit/test_refs_volume.cpp
)

target_link_libraries(cr_tests PRIVATE cr::core GTest::gtest_main)

# vcpkg DLL deployment backstop (no-op on Linux): applocal import-name
# resolution skipped the lz4 debug DLL on windows CI (0xc0000135)
cr_deploy_vcpkg_binaries(cr_tests)

if(CR_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(cr_tests PRIVATE -fsanitize=address)
    target_link_options(cr_tests PRIVATE -fsanitize=address)
endif()

include(GoogleTest)
gtest_discover_tests(cr_tests)

if(WIN32 AND CR_BUILD_APP AND CR_BUILD_UI)
    # Real C ABI scan -> real CRSF -> real rrecov process. The fixture keeps
    # its saved session only for this dependent CLI contract test; the normal
    # cr_ui_abi_check lifecycle test remains self-cleaning.
    set(CLI_FIXTURE_DIR ${CMAKE_CURRENT_BINARY_DIR}/cli-fixture)
    add_test(NAME cli_fixture
             COMMAND cr_ui_abi_check ${CLI_FIXTURE_DIR} --keep-session)
    add_test(NAME cli_contract
             COMMAND pwsh -NoProfile -ExecutionPolicy Bypass
                     -File ${CMAKE_CURRENT_SOURCE_DIR}/cli/cli_contract.ps1
                     -Exe $<TARGET_FILE:rrecov>
                     -Session ${CLI_FIXTURE_DIR}/cr_ui_session.crsf
                     -WorkDir ${CLI_FIXTURE_DIR}/cli-output)
    set_tests_properties(cli_contract PROPERTIES DEPENDS cli_fixture)
endif()
