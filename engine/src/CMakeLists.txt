# src/CMakeLists.txt - dispatches to the core (cross-platform) and,
# on Windows, the platform layer + CLI app.

add_subdirectory(core)

if(CR_BUILD_UI)
    add_subdirectory(ui)
endif()

if(WIN32)
    add_subdirectory(win)
    if(CR_BUILD_APP)
        add_subdirectory(app)
    endif()
endif()
