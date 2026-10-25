# MINOR N-045 — Scan cache budget 8 MiB vs og 4 MiB (and per-run vs disk-wide placement nuance)

**Severity:** MINOR
**Module:** storage
**Parameters affected:** P49 (memory — steady state), P47 (throughput tuning)
**Corpus references:** DRWUI_decompiled.c :919080-:919092 — `CStorageDeviceWithCache` budget `0x400000 / bytes_per_sector` (4 MiB), fallback 0x2000 blocks; ctor :962250-:962292 (cache count 1000); constructed once per physical disk, shared across all partition scans.
**Core references:** `src/core/orchestrator/cr_scan.cpp:610` — `CachedReader(8 MiB, 64 KiB blocks)`; per-run pipe, shared across candidate slices (placement equivalent). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `N-025` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

Our scan cache is 8 MiB where the og uses 4 MiB (2x memory, deliberate perf superset). Placement matches (both below the slice layer, shared across candidates). No behavioral difference; recorded for the perf-parity ledger.

## Fix plan

Document the 2x budget as deliberate in `docs/benchmarking.md` intent (or align to 4 MiB if memory parity benches demand); no code change required.

## Tests to add

- `Storage.CacheBudgetDocumented` — doc-truth pin (comment assert or doc lint).

## Related findings

I-017 (slice-layer supersets — same ledger), M-032 (same layer).

## Open questions

None.

---

## Addendum (session 6 — the ALGORITHM diverges too: og prefetch-thread read-ahead vs core reactive LRU; cite refresh, 2026-10-26)

Two updates from the never-cited-files read (`src/core/storage/cr_cached.cpp` was never cited before):

1. **Citation drift**: the core reference moved — the call site is now `cr_scan.cpp:647` (`CachedReader::create(std::move(retry), 8ull << 20, 256 * 1024)`) after the a4053c4 transport wave; the 64 KiB block size in the original cite is stale (256 KiB now — D-51 registers it). The budget substance (8 vs 4 MiB) is unchanged and verified live.
2. **Algorithm divergence (new sub-item, same parameter family)**: the og's `CStorageDeviceWithCache` (DRWUI_decompiled.c:919080+, ctor :962250-962597) is a **prefetch read-ahead engine** — a dedicated prefetch thread, a 10-run look-ahead window, 1000-sector default runs, CRITICAL_SECTION locking — versus the core's reactive true-LRU with std::mutex (`cr_cached.cpp`, the 8 MiB/256 KiB pipe at cr_scan.cpp:647). Sequential-scan throughput characteristics differ (read-ahead hides latency the reactive cache only catches on the second pass). Fix plan unchanged in spirit (document deliberate or align); if aligned, the prefetch thread — not just the budget — is the work item.
