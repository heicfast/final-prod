# src/win - Windows-only platform layer (MSVC).
add_library(cr_win STATIC
    cr_transport.cpp
    cr_enumerate.cpp
    cr_bitlocker.cpp
    cr_health.cpp
    cr_destio.cpp
    cr_elevate.cpp)
add_library(cr::win ALIAS cr_win)
target_include_directories(cr_win PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_link_libraries(cr_win PUBLIC cr::core)
# advance.sys / cfgmgr32 for bus types + setup APIs
target_link_libraries(cr_win PUBLIC cfgmgr32 Advapi32)
