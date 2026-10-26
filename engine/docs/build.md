# Build

## Prerequisites
- CMake >= 3.25
- C++20 compiler: MSVC 17 (VS2022) on Windows; gcc 12+ / clang 15+ on Linux
- zlib + **lz4 + zstd** (REQUIRED on every platform — no silent codec-less
  builds: a build without them zero-fills compressed ZFS/Btrfs extents;
  vcpkg manifest supplies them on Windows, `zlib1g-dev liblz4-dev
  libzstd-dev` on Linux) + GoogleTest via vcpkg (Windows) or system
  packages / FetchContent (Linux)

## Presets
```
cmake --preset win-x64-debug      # MSVC + vcpkg (needs VCPKG_ROOT)
cmake --preset win-x64-release
cmake --preset linux-gcc          # gcc + ASan on the pure core
cmake --preset linux-release
cmake --preset linux-ubsan        # gcc + -fsanitize=undefined,
                                  #   -fno-sanitize-recover=undefined
```

The CI linux-tsan lane uses the same generator with
`-DCMAKE_CXX_FLAGS="-fsanitize=thread -fPIE"` (see .github/workflows/ci.yml).

## Tests
```
ctest --preset linux-gcc          # or win-x64-release / linux-ubsan
```

The pure core builds and tests on Linux identically to Windows - the win
layer + CLI compile only on Windows. CI runs the full matrix on every push.
