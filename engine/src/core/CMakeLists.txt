# src/core - pure cross-platform engine core.
# Compiles on Linux (CI: gcc + ASan) and Windows (MSVC /W4 /WX) identically.
# No OS headers allowed anywhere below this line.

add_library(cr_core STATIC
    err/cr_error.cpp
    storage/cr_devmap.cpp
    storage/cr_slice.cpp
    storage/cr_cached.cpp
    table/cr_crc32.cpp
    table/cr_table_parse.cpp
    ntfs/cr_fixup.cpp
    ntfs/cr_runlist.cpp
    ntfs/cr_bootstrap.cpp
    ntfs/cr_attributes.cpp
    ntfs/cr_bitmap.cpp
    ntfs/cr_mftwalk.cpp
    fat/cr_chain.cpp
    fat/cr_lfn.cpp
    fat/cr_fat_quick.cpp
    exfat/cr_exfat_dentry.cpp
    exfat/cr_exfat_quick.cpp
    lost/cr_candidates.cpp
    crypto/cr_sha256.cpp
    carve/cr_registry.cpp
    carve/cr_analysers.cpp
    carve/cr_carve.cpp
    session/cr_crsf.cpp
    session/cr_records.cpp
    recovery/cr_safewriter.cpp
    orchestrator/cr_scan.cpp
    tree/cr_node.cpp
)

add_library(cr::core ALIAS cr_core)
target_include_directories(cr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(cr_core PUBLIC ZLIB::ZLIB)

if(CR_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(cr_core PRIVATE -fsanitize=address)
    target_link_options(cr_core PRIVATE -fsanitize=address)
endif()
