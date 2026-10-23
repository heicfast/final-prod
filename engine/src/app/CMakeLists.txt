add_executable(rrecov rrecov.cpp)
target_link_libraries(rrecov PRIVATE cr::core cr::win)

# vcpkg DLL deployment backstop (no-op on Linux)
cr_deploy_vcpkg_binaries(rrecov)
