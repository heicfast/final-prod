# src/ui - the G-01 C-ABI shim (cr_ui shared library).
add_library(cr_ui SHARED cr_ui.cpp)
# PIC: the core static lib links into the .so on Linux
set_property(TARGET cr_ui PROPERTY POSITION_INDEPENDENT_CODE ON)
add_library(cr::ui ALIAS cr_ui)
target_include_directories(cr_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(cr_ui PUBLIC cr::core)
if(WIN32)
    target_link_libraries(cr_ui PUBLIC cr::win)
    # the DLL must stay free of std::filesystem (Win7 CRT posture; the
    # handoff note from the UI session)
    target_compile_definitions(cr_ui PRIVATE _WIN32_WINNT=0x0601 NTDDI_VERSION=0x06010000)
endif()

# W2-3: the plain-C acceptance consumer (full lifecycle, -std=c11).
add_executable(cr_ui_abi_check ../../tools/cr_ui_abi_check.c)
target_include_directories(cr_ui_abi_check PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(cr_ui_abi_check PRIVATE cr_ui)
if(NOT WIN32)
    target_compile_options(cr_ui_abi_check PRIVATE -Wall -Wextra -Werror)
    # the shared object carries ASan on this preset; the consumer must match
    target_compile_options(cr_ui_abi_check PRIVATE -fsanitize=address -g)
    target_link_options(cr_ui_abi_check PRIVATE -fsanitize=address)
else()
    # MSVC: same strictness, MSVC spellings (the /W4 /WX posture)
    target_compile_options(cr_ui_abi_check PRIVATE /W4 /WX)
    # the C consumer must not pull the ASan DLL on Windows lanes
endif()
add_test(NAME cr_ui_abi_check
         COMMAND cr_ui_abi_check ${CMAKE_CURRENT_BINARY_DIR}/abi-tmp)
cr_deploy_vcpkg_binaries(cr_ui_abi_check)
