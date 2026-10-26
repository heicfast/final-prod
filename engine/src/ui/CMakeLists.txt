# src/ui - the cr_ui C-ABI shim (GAP G-01, issue #1).
#
# SHARED library: cr_ui.dll on Windows (links cr_core + cr_win - the
# platform layer with the RDA transport, enumeration, health and WinDestIo),
# libcr_ui.so elsewhere (links cr_core; the win-backed entries return
# ERR_UNSUPPORTED - the honest platform answer, documented in cr_ui.h).
#
# The C-consumer ABI check lives in tests/ (cr_ui_abi_check.c) and builds
# against this target with a plain C compiler on every lane.

add_library(cr_ui SHARED cr_ui.cpp)
add_library(cr::ui ALIAS cr_ui)

target_compile_definitions(cr_ui PRIVATE CR_UI_BUILD)
target_include_directories(cr_ui PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(cr_ui PRIVATE cr::core)
# N-062: GetProcessMemoryInfo (psapi; on MSVC the K32- alias also exists
# in kernel32, but the explicit lib is the Win7-inclusive form)
if(WIN32)
    target_link_libraries(cr_ui PRIVATE Psapi)
endif()
if(WIN32)
    target_link_libraries(cr_ui PRIVATE cr::win)
endif()

# Export ONLY the CR_UI_API surface (gcc/clang: default visibility off;
# MSVC exports dllexport symbols only by default). One documented
# exception (cr_ui_exports.def): the cr::log sink seam the shim's own
# test binary drives - mirrors what Linux's .dynsym exposes for the
# embedded static core anyway (see the .def header comment).
if(MSVC)
    target_sources(cr_ui PRIVATE cr_ui_exports.def)
endif()
set_target_properties(cr_ui PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON)

if(CR_ENABLE_ASAN AND NOT MSVC)
    target_compile_options(cr_ui PRIVATE -fsanitize=address)
    target_link_options(cr_ui PRIVATE -fsanitize=address)
endif()

# vcpkg runtime DLL deployment (Windows; no-op on Linux).
cr_deploy_vcpkg_binaries(cr_ui)
