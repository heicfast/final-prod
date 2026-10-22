# tests - GoogleTest suite over the pure core (runs on Linux AND Windows).
# The Windows-only layers (transport/enumeration) are exercised by integration
# scripts on real hosts - see docs/testing.md.

add_executable(cr_tests
    unit/main.cpp
    unit/test_fixup.cpp
    unit/test_runlist.cpp
    unit/test_crc32_gpt.cpp
    unit/test_slice_devmap.cpp
    unit/test_cached.cpp
    unit/test_node_tree.cpp
    unit/test_ntfs_volume.cpp
    unit/test_fat_volume.cpp
    unit/test_exfat_volume.cpp
    unit/test_lostpart.cpp
    unit/test_sha256.cpp
    unit/test_carve.cpp
)

target_link_libraries(cr_tests PRIVATE cr::core GTest::gtest_main)

if(CR_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(cr_tests PRIVATE -fsanitize=address)
    target_link_options(cr_tests PRIVATE -fsanitize=address)
endif()

include(GoogleTest)
gtest_discover_tests(cr_tests)
