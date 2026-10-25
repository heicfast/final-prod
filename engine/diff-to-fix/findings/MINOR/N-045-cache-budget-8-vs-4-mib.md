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
