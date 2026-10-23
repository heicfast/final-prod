# src/core - pure cross-platform engine core.
# Compiles on Linux (CI: gcc + ASan) and Windows (MSVC /W4 /WX) identically.
# No OS headers allowed anywhere below this line.

add_library(cr_core STATIC
    cr_parallel.cpp
    err/cr_error.cpp
    health/cr_nvme.cpp
    storage/cr_devmap.cpp
    storage/cr_slice.cpp
    storage/cr_cached.cpp
    storage/cr_retry.cpp
    table/cr_crc32.cpp
    table/cr_table_parse.cpp
    table/cr_ldm.cpp
    refs/cr_refs_detect.cpp
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
    ext4/cr_ext4_engine.cpp
    hfsplus/cr_hfsplus_engine.cpp
    btrfs/cr_btrfs_engine.cpp
    zfs/cr_zfs_engine.cpp
    lost/cr_candidates.cpp
    crypto/cr_sha256.cpp
    compress/cr_decomp.cpp
    carve/cr_registry.cpp
    carve/cr_analysers.cpp
    carve/cr_carve.cpp
    carve/cr_rawsigns.cpp
    carve/cr_rawsigns_data.cpp
    session/cr_crsf.cpp
    session/cr_records.cpp
    recovery/cr_safewriter.cpp
    recovery/cr_report.cpp
    repair/cr_rawfixer.cpp
    refs/cr_refs_engine.cpp
    orchestrator/cr_scan.cpp
    tree/cr_node.cpp
)

add_library(cr::core ALIAS cr_core)
target_include_directories(cr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
# G-09: the engine version baked into every recovery report
target_compile_definitions(cr_core PRIVATE
    CR_ENGINE_VERSION="${PROJECT_VERSION}")
target_link_libraries(cr_core PUBLIC ZLIB::ZLIB)
if(CR_LZ4_LINK)
    target_link_libraries(cr_core PUBLIC ${CR_LZ4_LINK})
    target_compile_definitions(cr_core PUBLIC CR_HAVE_LZ4=1)
endif()
if(CR_ZSTD_LINK)
    target_link_libraries(cr_core PUBLIC ${CR_ZSTD_LINK})
    target_compile_definitions(cr_core PUBLIC CR_HAVE_ZSTD=1)
endif()

if(CR_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(cr_core PRIVATE -fsanitize=address)
    target_link_options(cr_core PRIVATE -fsanitize=address)
endif()
