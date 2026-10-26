# M-058 execution evidence — the pre-fix use-after-free, reproduced under ASan

**Finding:** M-058 (NEW-FINDING, session 27 — DFX27 lane)
**Date:** 2026-10-26
**Tree at discovery:** `db3de2c` (post-merge with the window's parallel claims; the
defect itself dates to the core-api session landing that introduced the
`retry_raw` fallback expression — present in every tree since).

## Reproduction

`scripts/m058_uaf_repro.cpp` compiles the ORIGINAL run() inline construction
shape verbatim (SliceShim -> RetryingDevice -> `retry_raw` capture ->
CachedReader::create) with a failing cache geometry (4096 < 8192), then reads
through the old fallback expression `pipe = cached ? cached.get() : retry_raw`.

Build + run (gcc 14.2.0, -fsanitize=address):

```
$ g++ -std=c++20 -g -fsanitize=address -Iinclude scripts/m058_uaf_repro.cpp \
      src/core/storage/cr_cached.cpp src/core/storage/cr_retry.cpp \
      src/core/storage/cr_devmap.cpp src/core/storage/cr_slice.cpp \
      src/core/err/cr_error.cpp src/core/err/cr_log.cpp -o m058_repro
$ ./m058_repro
=================================================================
==6180==ERROR: AddressSanitizer: heap-use-after-free on address 0x508000000020
READ of size 8 at 0x508000000020 thread T0
    #0 0x56259b02b7ea in main scripts/m058_uaf_repro.cpp:59
...
0x508000000020 is located 0 bytes inside of 96-byte region
freed by thread T0 here:
    #1 cr::RetryingDevice::~RetryingDevice() include/cr/storage/cr_retry.h:34
    #2 std::default_delete<cr::IStorageDevice>::operator()(...)
    #3 std::unique_ptr<cr::IStorageDevice>::~unique_ptr()   [inside CachedReader::create's failure return]
```

The free is the retry device's destruction inside `CachedReader::create`'s
failure path (it takes `inner` BY VALUE); the read is the old fallback
dereferencing the captured raw pointer. Class: heap-use-after-free on the
defensive path.

## Reachability (why MAJOR, not ACUTE)

run() calls `CachedReader::create(std::move(retry), 8ull << 20, 256 * 1024)`
with compile-time constants — that call cannot fail (cache 8 MiB >= block
256 KiB, capacity 32). The UAF is therefore LATENT: unreachable at the only
current call site, but armed for any future caller that parameterizes the
cache geometry (the exact refactor M-049's context work was about to make,
which is how it was found). The adjacent defect is truthfulness: the code
comment documented "the pipe falls back to the bare slice" while the code
fell back to the destroyed pointer — the opposite.

## Post-fix pin

`Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails` +
`Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice` +
`Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry`
(tests/unit/test_orchestrator.cpp) — the extracted `build_read_pipe()`
degrades to the caller's slice on every creation failure and builds the full
cache-over-retry chain on valid geometry. Suite green with the fix (see the
finding's Resolution).
