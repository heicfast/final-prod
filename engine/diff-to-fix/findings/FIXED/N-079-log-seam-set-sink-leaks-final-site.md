# MINOR N-079 — cr::log set_sink leaks the final Site (LeakSanitizer red on the ASan lane since the N-062 landing)

**Severity:** MINOR (16-byte process-lifetime leak; ASan-lane CI red — 4 tests)
**Module:** err (the N-048 logging seam)
**Parameters affected:** Q67-adjacent (build/test hygiene), D81 (the logging-seam divergence's own implementation quality)
**Corpus references:** none — a core-internal defect introduced by the N-062 landing (`14229f2`, 2026-10-26); not a parity divergence. The og's Log.mo seam owns its sink objects (MOM census); nothing in the corpus mandates a heap leak.
**Core references:** `src/core/err/cr_log.cpp:52-63` (the landed `set_sink`), ASan-lane failures `Orchestrator.LogSilentByDefault`, `Orchestrator.LogSinkReceivesScanBrackets`, `Orchestrator.MemoryMonitorEmitsUnderPressure`, `CrUi.HeartbeatDuringStall` — all four LeakSanitizer reports name `cr::log::set_sink` `cr_log.cpp:55` (verified first-hand this session at the baseline tree with the fix stashed: 2+ of the 4 reproduce RED at `02b950f`-lineage; the first M-014 landing's Resolution also recorded the 4 LSan flags as pre-existing).
**Status:** NEW-FINDING (DFX24) — filed and fixed in the same session

## Summary

The N-062 landing's `set_sink` publishes each sink through a bare `new Site` and never frees: the design comment says "leaks the old site — sinks are set once per process in practice, and a use-after-free on a racing write is worse than a bounded leak on an exotic swap". The no-UAF rationale is sound, but the implementation leaves the FINAL Site (16 bytes) unreachable at process exit, so every process that installs a sink reports a LeakSanitizer direct leak — red on the ASan lane (4 tests), contradicting the repo's LSan-clean discipline (N-077 fixed a 528-byte leak of exactly this class).

## What the reference does

Not a parity surface — the og's logging seam (Log.mo / PubLog) is UI-layer plumbing with owned sink registries; there is no corpus-mandated leak.

## What our core does

`set_sink(fn, user)`: `next = new Site()` → atomic publish → the previous Site (and at exit the current one) is unreachable heap. The M-014 first-landing Resolution documented the resulting 4 LSan flags as "environment-only; CI runners disable LSan" — a workaround note, not a fix.

## Gap

Falsifiable: build the linux-gcc (ASan) lane at any commit since `14229f2` and run the four tests named above → each exits with `LeakSanitizer: detected memory leaks — Direct leak of 16 byte(s) ... cr_log.cpp:55`.

## Impact

ASan-lane CI red (the "full green CI on Linux" deliverable); a 16-byte per-process leak in production; the pattern invites copy-paste into future seams.

## Reproduction / verification

Executed this session (DFX24): `cmake --preset linux-gcc && ctest` at the pre-fix tree → 4/750 failures, every report naming `cr_log.cpp:55`; with the fix → 750/750 (and 826/826 at the post-union tree).

## Fix plan

1. Own the storage without changing the concurrency contract: a function-local `static std::deque<Site>` pool — `push_back` under the write mutex (deque never moves existing elements, so published `&pool.back()` pointers stay valid for racing readers), the atomic publish stays identical, retired sites stay alive exactly as the leak-by-design intended, but the storage is owned and destroyed at exit → LSan-clean.
2. Keep `set_sink(nullptr)` semantics (publish null = silence) — unchanged.
3. Pin: the four named tests stay green on the ASan lane (they were the red signal).

## Tests to add

- The four existing ASan-lane tests ARE the proof (they fail on the pre-fix tree under LSan and pass after); no new test needed beyond keeping the lane LSan-enabled.

## Related findings

- N-062 (the memory monitor through the logging seam — the landing that introduced the leak)
- N-048 (the seam itself)
- N-077 (the same leak class, fixed)

## Open questions

None — the defect and the fix are both fully local.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-079 commit — DFX24 baseline repair)
**Verified by:** the four ASan-lane tests (`Orchestrator.LogSilentByDefault`, `Orchestrator.LogSinkReceivesScanBrackets`, `Orchestrator.MemoryMonitorEmitsUnderPressure`, `CrUi.HeartbeatDuringStall`) — RED at the pre-fix tree (leak reports naming `cr_log.cpp:55`, reproduced first-hand with the fix stashed), GREEN after; ASan lane 750/750 at the fix tree, 826/826 at the post-union tree.
**Resolution notes:** `set_sink` now publishes from a `static std::deque<Site>` pool under the existing write mutex (deque `push_back` preserves element addresses, so the lock-free reader's `g_site` load keeps its no-UAF guarantee — the landed design's invariant, kept exactly). The `fn == nullptr` path publishes null (silence) without allocating. Retired sites remain alive in the pool for the process lifetime (the intentional never-free-while-racing posture), but the storage is owned and destroyed at static destruction — LeakSanitizer-clean. No concurrency contract, no emission-site behavior, and no ABI surface changed.
