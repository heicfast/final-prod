# Build

## Prerequisites
- CMake >= 3.25
- C++20 compiler: MSVC 17 (VS2022) on Windows; gcc 12+ / clang 15+ on Linux
- zlib + GoogleTest via vcpkg (Windows) or system packages (Linux)

## Presets
```
cmake --preset win-x64-debug      # MSVC + vcpkg (needs VCPKG_ROOT)
cmake --preset win-x64-release
cmake --preset linux-gcc          # gcc + ASan on the pure core
cmake --preset linux-release
```

## Tests
```
ctest --preset linux-gcc          # or win-x64-release
```

The pure core builds and tests on Linux identically to Windows - the win
layer + CLI compile only on Windows. CI runs the full matrix on every push.
