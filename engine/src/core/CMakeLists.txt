# src/core - pure cross-platform engine core.
# Compiles on Linux (CI: gcc + ASan) and Windows (MSVC /W4 /WX) identically.
# No OS headers allowed anywhere below this line.

add_library(cr_core STATIC
    cr_parallel.cpp
    err/cr_error.cpp
    err/cr_log.cpp
    err/cr_errmod.cpp
    health/cr_nvme.cpp
    health/cr_smart_ata.cpp
    health/cr_csmi.cpp
    health/cr_grade.cpp
    health/cr_family.cpp
    storage/cr_devmap.cpp
    storage/cr_image.cpp
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
    btrfs/cr_btrfs_cache.cpp
    zfs/cr_zfs_engine.cpp
    lost/cr_candidates.cpp
    lost/cr_consistency.cpp
    crypto/cr_sha256.cpp
    crypto/cr_aes.cpp
    storage/cr_decrypt_device.cpp
    compress/cr_decomp.cpp
    carve/cr_registry.cpp
    carve/cr_analysers.cpp
    carve/cr_carve.cpp
    carve/cr_rawsigns.cpp
    carve/cr_usersigns.cpp
    carve/cr_rawsigns_data.cpp
    session/cr_crsf.cpp
    session/cr_records.cpp
    recovery/cr_safewriter.cpp
    recovery/cr_report.cpp
    recovery/cr_dict.cpp
    recovery/cr_dict_data.cpp
    orchestrator/cr_filterfolder.cpp
    orchestrator/cr_filterfolder_data.cpp
    preview/cr_preview.cpp
    preview/cr_preview_text_exts.cpp
    preview/cr_preview_text_exts_data.cpp
    preview/cr_thumbnail.cpp
    query/cr_query.cpp
    query/cr_filefilter_data.cpp
    check/cr_fscheck.cpp
    repair/cr_rawfixer.cpp
    apply/cr_apply.cpp
    reghive/cr_reghive.cpp
    reghive/cr_offlineos.cpp
    bcdfix/cr_bcdfix.cpp
    nvramfix/cr_nvramfix.cpp
    refs/cr_refs_engine.cpp
    orchestrator/cr_scan.cpp
    tree/cr_node.cpp
    tree/cr_grading.cpp
    tree/cr_diskmap.cpp
    tree/cr_snapshot.cpp
)

add_library(cr::core ALIAS cr_core)
# PIC so the static core can also link into the cr_ui SHARED binding
# (G-01); PIC objects are valid in plain executables too.
set_target_properties(cr_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(cr_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
# cr_thumbnail's vendored public-domain decoder (third_party/stb) - a
# PRIVATE include: the stb header never appears in any public cr/ header.
target_include_directories(cr_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/stb)
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

# G-01: the core must be PIC to link into the cr_ui shared object
