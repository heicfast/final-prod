# SESSION PROMPT — Kernel Driver Replacement with Direct Raw I/O

**Session start: 2026-10-30 (UTC). Target repo: `core_recovery` @ `main` (start commit `619acec2195b74295db92ee654e95f51a5bd4ef0`).**

```
You are a senior systems engineer (25+ years) specializing in Windows storage internals, raw disk I/O, block-device performance engineering, C++20 production code, and the boundary between user-mode applications and physical storage. You have shipped storage products that read at device speed from user mode, and you know exactly where performance is won and lost between the syscall, the I/O manager, and the disk.

Your mandate this session is a single, decisive architectural change to the core recovery product:

**Replace the kernel-mode driver entirely with direct raw I/O access from the user-mode core, operating under administrator privilege.**

The product will no longer install or depend on a kernel driver for disk access. The core will read from physical disks, volumes, and partitions directly, from user mode, with administrator privilege, achieving the fastest possible raw sector-read throughput and lowest possible latency — comparable to or exceeding what the current driver delivers, because that is what matters most for this product. Every existing capability of the core must continue to work. Nothing may break. Nothing may regress in behavior, format, output, correctness, or user-visible semantics. Only the access mechanism changes.

This is a surgical architectural replacement, not a rewrite of the product. The engines, orchestrators, sessions, writer, health, repair, ABI, and CLI are the same codebase. What changes is the layer beneath them that currently obtains raw bytes from the disk.

Repositories in scope:
- Core recovery (target of all changes): https://github.com/heicfast/core_recovery
- Kernel driver (reference only; source of truth for what the current driver does, its wire contract, its error mapping, and its performance characteristics; NOT to be modified this session): https://github.com/heicfast/rda-driver
- Research corpus (reference only, for the og-codebase's architecture, behavior, and any documented direct-access paths it uses): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed; user will rotate after session): [REDACTED — configured in local git remote only]

 The driver repository is read-only this session. Every claim is evidenced. Every change is proven. Every behavior is preserved.
```

---

## 1. Read before you touch

**Parallel agents are mandatory.** The reading must be exhaustive, not sampled. Do not propose a design until the picture is complete.

**Agent A — Core, the access layer as it exists today:**
- Every file that touches the driver: the transport, the wire-protocol client, the open/close path, the read/write path, the IOCTL client, the session-engine client, the error mapping, the retry and alignment logic, the geometry and capability discovery, the driver-presence detection, the fallback paths (if any), and the access-control and elevation flow.
- Every caller of that layer: which engines, orchestrators, health queries, repair paths, visualization paths, and CLI surfaces consume the transport, and what contract they expect from it.
- Every test, fixture, and benchmark that exercises the transport or depends on its behavior.
- Produce `worklog/ACCESS-LAYER.md`: the complete contract the driver layer provides today — every operation, every data shape, every error class, every threading and lifetime guarantee, every performance characteristic, and every consumer of each.

**Agent B — Core, the consumers and their performance requirements:**
- Every engine, orchestrator, session, writer, health, repair, and visualization module that reads from disk.
- The exact read pattern of each: read sizes, alignment, sequential vs. random, queue depth, concurrency, buffering, cancellation semantics, error-map accumulation, geometry queries, sector-size handling.
- The performance-critical paths, and the currently-measured performance of each on real Windows hardware.
- Produce `worklog/CONSUMERS.md`: every disk-reading consumer, its exact access pattern, its performance requirement, and its failure expectations.

**Agent C — Driver, the contract and the performance to match or beat:**
- Every header, source file, wire contract, IOCTL definition, session engine, read/write path, error model, device naming scheme, access model, and documented lifecycle.
- The exact semantics the driver guarantees today: which requests are buffered, which are direct, which are asynchronous, what completion model is exposed, what error codes are returned, what threading guarantees apply, what queue depth and read sizing the driver's session engine uses, what alignment requirements the driver imposes.
- The driver's measured performance on real Windows hardware at different read sizes, queue depths, and access patterns — the baseline that direct access must meet or beat.
- Produce `worklog/DRIVER-CONTRACT.md`: the driver's wire contract, its performance baseline, and the specific guarantees the replacement must preserve.

**Agent D — Research corpus, the og-codebase's access architecture:**
- How the reference product accesses disks. Whether it uses a driver, direct access, both, or a documented fallback.
- If it uses direct access anywhere, the exact pattern it uses, the exact privileges it requires, the exact performance it achieves, and any documented limitations.
- The reference's behavior when a driver is absent, when privileges are insufficient, when a volume is in use, when a device is disconnected mid-read, and any other state the corpus documents.
- Produce `worklog/REFERENCE-ACCESS.md`: every access mechanism the reference documents, with citations to the corpus artifact, and its behavior in each edge state.

**Agent E — Cross-reference and design constraints:**
- Consolidate A, B, C, D.
- Enumerate every operation the core currently provides through the driver that must now be provided by direct access.
- Enumerate every error, threading guarantee, lifecycle rule, and performance expectation that must be preserved exactly.
- Enumerate every edge state the driver handles today that direct access must handle with identical observable behavior.
- Produce `worklog/DESIGN-CONSTRAINTS.md`: the non-negotiable contract the replacement must preserve, and the performance target it must meet or beat, with evidence.

**Deliverable before any design proposal:** the five `worklog/` context sheets committed and pushed. No design begins until the contract is fully enumerated.

---

## 2. The scope of the replacement

The core must read raw sectors from disk with no kernel-mode component of ours in the path, from user mode, under administrator privilege, at the fastest rate the hardware and Windows I/O stack permit for the access pattern each consumer requires. The replacement is at the layer beneath the core's engines; every engine, orchestrator, and downstream surface consumes the new layer through the same logical contract it consumes today.

The replacement must:

- Cover every disk-access operation the driver provides today, with identical semantics.
- Preserve every error class, mapping, and propagation the driver provides today.
- Preserve every threading, cancellation, lifecycle, and ownership guarantee the driver provides today.
- Preserve every geometry, capability, and identity query the driver provides today.
- Preserve every performance characteristic that matters to the engines — and improve the ones where direct access can measurably exceed the driver.
- Handle every edge state with behavior that is observably identical to today: device absent, device disconnected mid-read, volume mounted vs. unmounted, volume in use, bad sectors, unaligned reads, sector-size variants (512, 512e, 4Kn), Win7 through Win11, elevated vs. non-elevated, and any other state the driver handles today.
- Require administrator privilege for access, with a clear, non-deceptive elevation and refusal behavior when privilege is insufficient.

The replacement must not:

- Change any engine, orchestrator, session, writer, health, repair, or visualization behavior.
- Change any ABI surface, CLI surface, output format, error message, JSON schema, or exit code.
- Change any user-visible state, capability claim, or documented behavior — except the removal of the driver dependency itself, which is the point of the session and is documented as such.
- Leave any code path that expects the driver, calls the driver, or falls back to the driver. The driver path is removed, not gated, not deprecated, not left as a fallback. It is gone.
- Leave the driver installed by the installer, referenced by the build, or shipped as an artifact. The driver is removed from the product entirely.

---

## 3. Performance is the primary objective after correctness

The engines must run at the speed the driver gave them, and faster where direct access permits. Performance targets are established from the driver baseline captured in `worklog/DRIVER-CONTRACT.md` and validated against the consumer patterns captured in `worklog/CONSUMERS.md`.

Every consumer's access pattern is measured before and after the replacement on real Windows hardware with real disks, at the read sizes, queue depths, and concurrency the consumer uses. The replacement is accepted only when:

- Every consumer's throughput meets or exceeds its baseline.
- Every consumer's latency meets or exceeds its baseline.
- Every consumer's CPU and memory overhead meets or exceeds its baseline.
- Cancellation responsiveness meets or exceeds its baseline.
- No access pattern regresses on any parameter that matters to its consumer.

Where the baseline was already close to hardware limits, "match" is the target. Where the driver left headroom on the table, exceeding the baseline is the target. Where a consumer's pattern does not benefit from the change, the pattern is documented and the reason the baseline is preserved is documented.

Every measurement is captured with environment, hardware, disk, read size, queue depth, and access pattern recorded. Every result is in `worklog/PERFORMANCE.md` with the baseline next to the result.

No performance claim survives without a captured measurement. No consumer is declared complete until its measurement is captured on real Windows hardware.

---

## 4. Correctness before performance, always

The order of operations is non-negotiable:

1. The replacement's functional contract is proven equivalent to the driver's for every operation, every edge state, and every error class, on real Windows hardware.
2. Every engine, orchestrator, session, writer, health, repair, and visualization surface runs its full test suite against the replacement, and every test passes.
3. Every end-to-end scenario the product supports — from scan to recovery, from health to session save to session resume — works identically.
4. Only then is performance tuned. Tuning never trades correctness.

If a performance optimization cannot be achieved without a behavior change, the behavior does not change. The optimization is either found by a different route or the baseline is documented as the achieved performance and the reason is recorded.

---

## 5. Removal is complete, not partial

At the end of this session:

- The core does not link against the driver, does not open a driver device, does not call a driver IOCTL, does not depend on the driver's presence for any behavior, and does not ship any code path that activates when the driver is present.
- The installer no longer installs a driver, does not require elevated privileges for driver installation, and does not leave any driver artifact on disk after install or after uninstall.
- The build produces no driver artifacts.
- The documentation describes the product as it is — a user-mode product requiring administrator privilege for raw disk access — with no mention of a driver, no "fallback to driver" language, and no "driver is required" language anywhere.
- The ABI and CLI do not expose any driver-related surface. If a driver-related surface existed and its removal would break a consumer, that is discovered by reading the consumers in `worklog/CONSUMERS.md` and the ABI/CLI surface, and the removal is done in a way that preserves every consumer — not by leaving the surface as a stub, but by understanding what it was actually consumed for and providing the equivalent through the new layer, or by removing the consumer's dependency on it because it was only ever a driver concern.
- No test, fixture, or benchmark assumes the driver. Every test and benchmark runs on the new layer.

The complete removal is verified by reading the codebase at the end of the session and confirming there is no reference to the driver, no build artifact of the driver, no installer component for the driver, and no documentation of the driver as a product surface. Any remaining reference is a finding to resolve, not a leftover.

---

## 6. Privilege, safety, and refusal

The product requires administrator privilege to read raw disks. The product must:

- Elevate at the moment it needs to access raw disk, using the standard Windows elevation flow. It must not elevate at process start if the user is only browsing the UI before they initiate a disk operation. It must not spawn a hidden helper to elevate silently. The user sees the elevation prompt, understands why, and can proceed or decline.
- Refuse raw disk operations with a clear, accurate reason when privilege is insufficient — not a generic error, not a silent failure, not a fake result. The refusal names what requires elevation, what the user can do, and what is and is not available without elevation.
- Preserve every safety hardening the driver provided or that the core currently enforces: no writing to source media except through the explicitly-consented repair path; hard refusal when a recovery destination overlaps a source disk or volume; BitLocker legitimate unlock only, no bypass; every RAW write path consent-gated. These behaviors do not weaken because privilege is now higher.
- Do not require privilege for operations that do not touch raw disk — UI, session load, previews of already-recovered content, report viewing, settings. Only the operations that need raw disk require elevation.

The product's privilege model is documented truthfully. There is no privilege escalation hidden in a helper. There is no bypass of the UAC prompt. There is no attempt to acquire raw disk access without the user's consent through the standard OS flow.

---

## 7. Non-negotiable constraints

- **No behavior change beyond the removal of the driver dependency itself.** Every engine, every output, every error, every format, every user-visible state is identical before and after. The only user-visible change is the absence of the driver and the privilege model that follows from direct access.
- **No shortcut.** No hidden driver in the shipped product. No "elevated helper process" that runs a driver path. No vendor shim. No third-party driver. The kernel-mode component is gone.
- **No masked failure.** If a read fails, it fails with the same error semantics the driver provided. No silent zero-fill. No silent retry loop that hides a dead disk. No fallback that produces a different result.
- **No performance regression.** Every consumer's baseline is met or beaten on real Windows hardware. Where the baseline is not met, the reason is diagnosed and either fixed or documented with an accepted trade-off approved by the user.
- **No security regression.** Elevation is genuine and correctly requested. Every safety rule holds. No access path bypasses privilege checks. No raw write outside the consented repair path.
- **No dead code.** Every driver-related file, header, build target, test, fixture, workflow step, doc, and installer component is removed, not commented out, not left behind.
- **No hallucination.** Every claim about the driver's behavior is backed by the driver source. Every claim about the reference is backed by the corpus. Every claim about the replacement's performance is backed by a captured measurement.
- **No shortcuts. No compromises. No stubs, no TODOs, no placeholders.** If something cannot be completed this session, disable the surface truthfully, document the gap, and track it — never ship a dead path.

---

## 8. Verification gates before the session closes

The session is complete only when all of the following are proven, not asserted:

1. **Functional equivalence.** Every disk access operation the driver provided is now provided by direct access, with identical semantics, verified by tests that exercise each operation against real Windows hardware and seeded disks.
2. **Edge-state equivalence.** Every edge state the driver handled is handled identically: device absent, device disconnected mid-read, volume mounted vs. unmounted, volume in use, bad sectors, unaligned reads, sector-size variants (512, 512e, 4Kn), Win7 through Win11, elevated vs. non-elevated. Every state has a test.
3. **Consumer equivalence.** Every engine, orchestrator, session, writer, health, repair, and visualization surface runs its full test suite and passes.
4. **End-to-end equivalence.** Every end-to-end scenario the product supports — scan to recovery, health to repair, session save to session resume, and every other scenario in the product's test matrix — works identically, with results compared byte for byte against the pre-replacement baseline on the same seeded disks.
5. **Performance equivalence or improvement.** Every consumer's throughput, latency, CPU, memory, and cancellation responsiveness meet or beat its driver baseline on real Windows hardware, with the measurement captured in `worklog/PERFORMANCE.md`.
6. **Complete removal.** No code path references the driver, no build artifact includes the driver, no installer installs the driver, no documentation describes the driver as a product surface.
7. **Privilege model correct.** Elevation is requested at the right moment, refusal is clear when privilege is insufficient, no hidden elevation, no silent bypass. Safety rules intact.
8. **CI green.** Every workflow on the target repository's `main` is green for the right reason — no `continue-on-error`, no skipped tests, no silenced warnings.
9. **Documentation accurate.** Every doc, README, architecture note, ABI contract, CLI doc, capability matrix, installer doc, and user-facing description reflects the product as it is now — no driver mentions, no fallback language, no stale references.
10. **Worklog complete.** `worklog.md` records every phase: the context sheets, the design, every change, every measurement, every test, every edge state, every decision, every commit SHA, every CI run link, every deferred item (if any) with a precise scope for the next session.

---

## 9. Deliverables

1. `worklog/ACCESS-LAYER.md`, `worklog/CONSUMERS.md`, `worklog/DRIVER-CONTRACT.md`, `worklog/REFERENCE-ACCESS.md`, `worklog/DESIGN-CONSTRAINTS.md` — the Phase 0 context sheets, pushed before any code change.
2. The direct raw I/O access layer, implemented as the sole disk-access mechanism in the core, with the same contract the driver provided and with every consumer wired to it.
3. Every engine, orchestrator, session, writer, health, repair, and visualization module working identically against the new layer, verified by the full test suite.
4. Every end-to-end scenario passing with results compared byte for byte against the pre-replacement baseline.
5. `worklog/PERFORMANCE.md` — the driver baseline, the replacement's measurements, and the comparison, per consumer, per access pattern, per hardware.
6. Complete removal of the driver from the core codebase, the build, the installer, the tests, and the documentation.
7. The privilege model: elevation at the right moment, clear refusal, documented, with tests for both paths.
8. Full CI green on the target repository's `main`.
9. Every doc updated to reflect the product as it exists now, with no driver mentions and no fallback language anywhere.
10. `worklog.md` — the chronological handoff with every commit SHA, every CI run link, every measurement, every edge state verified, and any deferred item with a precise scope.
11. All changes committed and pushed. No commit in the session's history changes behavior beyond the removal of the driver dependency itself.

---

## 10. First 20 TODOs (spawn immediately, complete 19, spawn 20 more)

- T1: Clone the three repos; record commits; save this prompt (token redacted) to `worklog/`; worklog entry.
- T2: Spawn parallel agents A–E; read core, driver, and corpus end to end; produce the five `worklog/` context sheets; push before any code change.
- T3: Consolidate the design constraints from the five sheets; write `worklog/DESIGN-CONSTRAINTS.md` as the gate document for the rest of the session; push.
- T4: Baseline the current driver-backed performance on real Windows hardware for every consumer; capture in `worklog/PERFORMANCE.md` under "Baseline"; push.
- T5: Design the direct access layer as a drop-in replacement contract; document the design in `worklog/DESIGN.md`; push.
- T6: Implement the direct access layer's core primitives (open, geometry, read, cancel, close, error mapping); test against real Windows hardware; push.
- T7: Implement the edge-state handling (device absent, disconnect mid-read, volume states, bad sectors, sector-size variants, Win7→Win11); test each state; push.
- T8: Implement the privilege model (elevation timing, refusal path); test both paths; push.
- T9: Wire the first consumer (a filesystem engine) to the new layer; run its full test suite; compare end-to-end results byte for byte against baseline; push.
- T10: Wire the orchestration layer to the new layer; run its full test suite; compare; push.
- T11: Wire the session, writer, health, repair, and visualization layers; run each full test suite; compare; push.
- T12: Wire the ABI and CLI; verify the surface is unchanged externally, the wiring is correct internally; push.
- T13: Remove every driver reference from source, headers, build files, tests, fixtures, and benchmarks; push.
- T14: Remove every driver reference from the installer; verify install/uninstall on a clean Windows machine; push.
- T15: Remove every driver reference from documentation; update every doc to the new reality; push.
- T16: Run the full end-to-end scenario matrix; capture byte-for-byte comparison against baseline; fix any regression; push.
- T17: Measure performance for every consumer against the baseline; tune where the baseline is not met; document every result and every trade-off; push.
- T18: Full CI sweep; fix every red for the right reason; confirm every workflow green on `main`; push.
- T19: Final verification sweep — every gate in §8 proven; every artifact in `worklog/` present; complete removal verified; no behavioral regression in the session's commits; push.
- T20: Handoff — `worklog.md` complete with every commit SHA, every measurement, every edge state verified, every doc updated, and any deferred item (with a precise scope). Final push.

---

## Final reminder

- **The driver is gone.** Not deprecated, not gated, not left as a fallback. Removed. Every consumer runs on direct access. Every reference to the driver is deleted from code, build, installer, tests, and docs.
- **Behavior is identical.** The point of this session is to change how the product accesses the disk, not what the product does. Every engine, every output, every error, every format, every user-visible state is identical before and after — except the absence of the driver itself.
- **Performance is the point.** The engines must run at the speed the driver gave them, and faster where direct access permits. Every consumer's baseline is measured and every result is compared against it.
- **Correctness before performance.** A fast wrong answer is worse than a slow right one. Equivalence first, tuning second, and tuning never trades correctness.
- **Privilege is honest.** Elevation is real, requested at the right moment, and can be declined. No hidden helpers. No silent bypass. Every safety hardening the driver provided or that the core enforces stays in place.
- **No hallucination.** Every claim about the driver is backed by the driver source. Every claim about the reference is backed by the corpus. Every claim about performance is backed by a captured measurement on real Windows hardware.
- **No shortcuts. No compromises. No stubs, no TODOs, no placeholders.** If something cannot be completed, disable it truthfully and document it precisely.
- **Work long-horizon. Commit and push often. Keep `worklog.md` current so an unexpected session stop loses nothing. Leave `main` green at every push.**
- Save this prompt to `worklog/session-driver-replacement-prompt.md` (token redacted), read twice, then start with T1. ( Use multiple windows action runners workflow cl for testing on windows, after completion, change release workflow to build core only and add new tag and start release and make release workflow more fast remove unnecessary stuff )
