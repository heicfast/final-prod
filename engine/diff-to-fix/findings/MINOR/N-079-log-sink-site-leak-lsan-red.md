# MINOR N-079 — The log seam's sink Site leaks at exit (LeakSanitizer red on every sink-registering test; the ASan lane was pushed red by the N-048 landing window)

**Severity:** MINOR (CI-integrity: the ASan lane fails 4 tests at main; no functional defect — the leaked object is process-lifetime by intent)
**Module:** err (the N-048 logging seam)
**Parameters affected:** P46 (doc truthfulness — the code comment said "leaked by design" without registering the LSan consequence), CI gates (linux-gcc ASan lane)
**Corpus references:** none (core-internal defect, discovered by the ASan lane — the NEW-FINDING class of §8)
**Core references:** `src/core/err/cr_log.cpp:55` (set_sink's `new Site`), the "leaked by design" comment (:49-53 pre-fix), the N-048 landing `b98baff` and the N-062 landing `14229f2` (the tests that register sinks: Orchestrator.LogSilentByDefault / LogSinkReceivesScanBrackets / MemoryMonitorEmitsUnderPressure, CrUi.HeartbeatDuringStall)
**Status:** FIXED

## Summary

The N-048 logging seam's `set_sink()` allocates a 16-byte `Site` per registration and never frees it — the old site was deliberately leaked ("a use-after-free on a racing write is worse than a bounded leak"), and the final site also leaked at process exit. LeakSanitizer flags the exit leak on every test that registers a sink: 4 tests red on the linux-gcc ASan lane at main HEAD `f5d4981` (verified first-hand on a clean worktree before any of this session's changes — this is a pushed-red main from the N-048/N-062 parallel landing window, not a regression of this session).

## What the reference does

Not a parity item — a core-internal defect class (the session prompt's §8 NEW-FINDING lane).

## What our core does

Pre-fix: `write_va` loaded the site pointer lock-free (so a swapped-out site could not be safely deleted — the stated reason for the leak), `set_sink` replaced the pointer under the mutex and dropped the old pointer.

## Gap

LSan exit leak (16 bytes per process) + 4 red ASan tests. Falsifiable: run the linux-gcc preset at `f5d4981` → the 4 tests fail with `LeakSanitizer: detected memory leaks` pointing at `cr_log.cpp:55`.

## Impact

CI integrity (the green-main protocol) — no user-visible behavior.

## Reproduction / verification

Executed first-hand: clean worktree at `f5d4981`, linux-gcc preset, ctest → 4 FAILED (the LSan reports quoted above); post-fix tree → 758/758 green (53.4 s).

## Fix plan

Deliveries are serialized under `g_mu` by contract anyway, so the site load moves INSIDE the mutex (a retired site can never be held past the lock), `set_sink` deletes the retired site after the swap, and a static-duration `SinkReleaser` deletes the live site at exit. The default-silent posture is unchanged (a post-exit log call sees a null site).

## Tests to add

The 4 pre-existing tests ARE the proof set (fail-before executed at the clean baseline worktree; pass-after on the fix tree). No new test: the leak only manifests at exit, which the LSan-instrumented lane itself observes.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-079 commit)
**Verified by:** fail-before: linux-gcc ctest at `f5d4981` (clean worktree, none of this session's changes) — Orchestrator.LogSilentByDefault / LogSinkReceivesScanBrackets / MemoryMonitorEmitsUnderPressure + CrUi.HeartbeatDuringStall all FAILED with `LeakSanitizer: detected memory leaks` (16 bytes, `cr::log::set_sink` cr_log.cpp:55); pass-after: linux-gcc 758/758 PASS including all four.
**Resolution notes:** The site pointer is now replaced AND deleted under `g_mu`, with `write_va`'s load moved inside the same mutex (the seam is synchronous by its registered contract — the lock was already the delivery serializer, so the fast path loses nothing). A static-duration `SinkReleaser` deletes the live site at process exit. The old "leaked by design" comment is replaced with the N-079 rationale. The never-a-UAF property the old design bought is preserved by construction: no reader holds a site past the mutex.
