# MAJOR M-058 (NEW-FINDING, session 27) — run()'s cache-failure fallback dereferences the destroyed retry device (use-after-free on the defensive path), contradicting its own comment

**Severity:** MAJOR (latent memory-safety defect on a defensive path — unreachable
with run()'s constant cache geometry, armed for any parameterized caller; the
comment/code contradiction is the doc-truth defect in the same lines)
**Module:** orchestrator (cr_scan.cpp run()) · storage pipe construction
**Parameters affected:** Q46-adjacent (comment truthfulness — the comment documents
the bare-slice fallback; the code does the opposite), build-integrity of the
read-pipe construction
**Status:** FIXED (found + fixed in the same session — discovered during M-049's
emit-site verification: stamping the retry device's context required reasoning
about `retry_raw`'s lifetime on every path)

## Summary

run() constructed the scan read chain inline:

```cpp
auto retry = RetryingDevice::create(std::move(shim), policy);
RetryingDevice* retry_raw = retry.get();
auto cached = CachedReader::create(std::move(retry), 8ull << 20, 256 * 1024);
IStorageDevice* pipe = cached ? cached.get() : (retry_raw ? retry_raw : slice);
```

`CachedReader::create` takes its `inner` unique_ptr **by value** — on every
failure return (`!inner`, `block_bytes == 0`, `cache_bytes < block_bytes`,
`capacity == 0`) the retry device is **destroyed**. So whenever `cached` is
null, `retry_raw` is dangling, and the fallback `pipe = retry_raw` hands every
engine read to freed memory. The dead `: slice` sub-branch (retry_raw is null
only when RetryingDevice::create itself failed, which requires a null shim,
impossible for slice != null) means the fallback taken is ALWAYS the dangling
one. The adjacent comment claimed "the pipe falls back to the bare slice" —
the code did the opposite of its own documentation.

## Evidence

- Executed ASan reproduction (fail-before): `scripts/m058_uaf_repro.cpp` +
  `evidence/executions/s27-m058-readpipe-uaf-asan.md` —
  `heap-use-after-free`, freed by `RetryingDevice::~RetryingDevice` inside
  `CachedReader::create`'s failure return, read at the old fallback
  expression (scripts/m058_uaf_repro.cpp:59).
- Code first-hand: `src/core/orchestrator/cr_scan.cpp` run() at the audited
  tree `db3de2c` (the inline sequence + the contradicted comment).
- `src/core/storage/cr_cached.cpp` create(): by-value `inner` destroyed on
  every failure return path (verified line-by-line this session).

## Root cause

The construction mixed three lifetime regimes inline (unique_ptr chain, raw
observation pointer, fallback pointer) with the failure semantics of a
by-value-owning factory — the raw pointer's validity condition ("the cache
owns it") silently inverted on the failure path, and nothing pinned the
fallback contract. No test could reach the path (constants), so no test
existed.

## Fix

Extracted `build_read_pipe(slice, cache_bytes, block_bytes)` (declared in
cr_scan.h next to the `ReadPipe` struct it returns) — the single construction
point with the degraded path as a **pinned, tested contract**: on ANY creation
failure the pipe is the caller's bare slice (memory-safe; retry isolation and
cache lost on that path — surfaced in the contract comment, never hidden).
run() now consumes `ReadPipe rp`; `tel_retry_` is published only when the
chain owns the retry device. The cache/retry tuning commentary (N-045/D-72
deliberate divergences) moved with the construction into the function.

## Tests (fail-before executed / pass-after pinned)

- `Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry` — the healthy
  path: cache pipe, live retry source, readable boot sector.
- `Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails` — the
  pinned contract (pipe == caller's slice, no chain, no retry source,
  readable).
- `Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice` — the second
  failure geometry.
- Fail-before: the ASan reproduction above (the inline original UAFs); the
  extracted function makes the path testable at all.

## Resolution

**Status:** FIXED
**Fixed in commit:** (recorded by the session's push - `fix(M-058)`)
**Verified by:** ASan fail-before reproduction
(`evidence/executions/s27-m058-readpipe-uaf-asan.md`) + the three
ReadPipe pin tests green + full suite green (linux-release; see worklog
entry for the count at push time)
**Resolution notes:** The inline pipe construction's cache-failure fallback
dereferenced the retry device that CachedReader::create destroys on its
failure returns — a latent use-after-free (unreachable at run()'s constant
geometry, executed under ASan in the standalone repro to prove the class) and
the direct contradiction of the adjacent comment, which documented the
bare-slice fallback. Fixed by extracting build_read_pipe() with the degraded
path as a pinned contract (bare slice, no retry isolation, surfaced), covered
by three regression tests. No session-format/ABI surface touched; engine
behavior on the healthy path is byte-identical (same chain, same constants).
Safety hardenings untouched. Found during M-049 verification (the error-context
work needed retry_raw's lifetime reasoned on every path).
