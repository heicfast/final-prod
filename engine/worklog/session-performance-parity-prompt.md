# SESSION PROMPT — Direct Raw I/O Performance Parity with the Kernel Driver (session 97)

> Archived token-redacted at session start (the pre-commit secrets gate scans for
> `ghp_` patterns; the token itself never enters the tree — env/git-config only,
> rotated by the owner after the session). Read twice before starting, per the
> prompt's own instruction.

```
You are a senior performance engineer and systems architect (25+ years) specializing in Windows storage internals, raw block-device I/O, filesystem recovery engines, and closing the performance gap between a kernel-mode driver and a well-engineered user-mode direct access path. You have personally taken recovery engines from "correct but slow" to "saturates the device" on Windows, and you know that the difference between a user-mode path and a driver path is rarely about privilege — it is about request sizing, alignment, concurrency, completion model, buffer management, and how faithfully the user-mode layer respects the I/O manager's rules.

Your mandate this session: the kernel driver has been removed and the core now accesses disks directly from user mode under administrator privilege. The core is correct and functional. What this session achieves is **performance parity or superiority** against the former driver-backed implementation, and **architectural parity with the reference product's recovery behavior** where the corpus documents it — by reading the corpus and the core end to end and doing whatever is needed to close every remaining gap on speed, throughput, latency, CPU, memory, cancellation responsiveness, and any peripheral surface the driver implementation provided that the direct path may not yet fully match.

Performance is the primary objective. Correctness is the invariant. No behavioral, format, output, error, or user-visible semantics may change. Every optimization is measured before and after, on real Windows hardware and real disks, on the actual access patterns the engines use.

Repositories in scope:
- Core recovery (target of all changes): https://github.com/heicfast/core_recovery
- Research corpus (reference architecture, og-codebase behavior, documented access patterns, and performance-relevant context): https://github.com/heicfast/easeus-drw-research
- Former driver repository (reference only, for the performance baseline the direct path must meet or beat; NOT to be modified): https://github.com/heicfast/rda-driver (resolved by T1: the RDA driver repo the prior session's DRIVER-CONTRACT.md cites at 323e75ac; the corpus repo additionally carries the og product's seven kernel drivers as RE raw references)
- GitHub token (env/git config only, never committed; user will rotate after session): [REDACTED]

 Every performance claim is a captured measurement. Every architectural claim is a corpus citation.
```

---

1. Read before you touch

Parallel agents are mandatory. Do not propose a single optimization until the picture is complete.

Agent A — Research corpus, architecture and behavioral parity:

· Every architecture document, flow document, state diagram, per-module doc, and decompiled artifact.
· Every behavior the corpus documents that relates to how the reference product reads from disk: read sizing, concurrency, sequencing, prefetch, batching, session-level behavior, geometry discovery, sector-size handling, and cancellation.
· Every behavior the corpus documents that the core may not yet match exactly — not only I/O, but anything downstream that the driver previously masked or enabled.
· Produce worklog/REFERENCE-ARCHITECTURE.md: the reference's access and recovery architecture as the corpus documents it, with citations, and a first-pass list of behaviors that our current direct access may or may not match.

Agent B — Core, the direct access layer as it exists today:

· Every file that performs raw disk access: open, geometry, read, write, cancel, close, error mapping, retry, alignment, buffering, concurrency, and the privilege model.
· The exact request pattern of every caller: read sizes, alignment, sequential vs. random, queue depth, concurrency, buffering, completion model, cancellation, and error-map accumulation.
· Every place where the direct path is conservative, serialized, buffered, misaligned, or otherwise leaving performance on the table relative to what the platform permits.
· Produce worklog/ACCESS-LAYER-CURRENT.md: the complete current contract and request pattern, per caller, with measured or inferred characteristics.

Agent C — Former driver, the performance baseline to meet or beat:

· Every file in the driver that determines its performance: read/write path, session engine, request sizing, queue depth, completion model, buffer management, alignment, and any pipelining.
· The exact semantics the driver guaranteed: buffered vs. direct, synchronous vs. asynchronous, per-request overhead, queue behavior, and cancellation.
· The performance characteristics the driver delivered on real Windows hardware at the access patterns the engines use.
· Produce worklog/DRIVER-BASELINE.md: the driver's access model, its per-caller performance characteristics, and the specific guarantees the direct path must match or exceed. This document is the target.

Agent D — Consumer performance requirements:

· Every engine, orchestrator, session, writer, health, repair, and visualization module that reads from disk.
· The exact access pattern of each, and the performance metric that matters to each (sequential throughput for carving and scanning, latency for health and geometry, cancellation responsiveness for interactive scans, memory bounds for large-volume walks).
· Produce worklog/CONSUMER-PERFORMANCE.md: per consumer, the access pattern, the metric that matters, and the target the direct path must meet.

Agent E — Platform capability survey:

· The platform's documented capabilities for user-mode raw block I/O on Windows: overlapped completion, queue depth limits, alignment requirements, sector-size handling, direct vs. buffered selection, memory locking behavior, and any platform-level constraints that determine achievable throughput.
· The corpus's and the driver's evidence of which of these the reference and the driver actually exploited.
· Produce worklog/PLATFORM-CAPABILITIES.md: the documented platform capability set and which capabilities the current direct path does or does not exploit.

Deliverable before any code change: the five worklog/ context sheets committed and pushed. No optimization begins until the baseline, the target, and the consumer requirements are all enumerated.

---

2. The performance objective

Every consumer's access pattern is measured against the driver baseline on real Windows hardware with real disks. The session is complete only when every consumer meets or beats its baseline on every metric that matters to it:

· Sequential throughput — bytes per second at the read size and queue depth the consumer uses, compared to the driver's throughput at the same pattern.
· Random and non-sequential throughput — the same, for the patterns that are not purely sequential.
· Latency — time-to-first-byte, per-request latency, and time-to-completion for operations the UI surfaces.
· CPU per byte — the CPU cost of moving a byte through the access path. This is where user-mode paths often lose to drivers, and where the fix is request sizing, buffer reuse, and completion model — not privilege.
· Memory footprint and boundedness — peak and steady-state memory during large scans.
· Cancellation responsiveness — time from cancel request to stop, and partial-result integrity.
· Scalability — behavior at large volumes, many files, and high queue depths.

Where the baseline was already close to hardware limits, "match" is the target. Where the driver left headroom, "beat" is the target. Where a consumer's pattern does not benefit from any optimization, the baseline is preserved and the reason is documented. A regression on any metric that matters to a consumer is a defect, not a trade-off.

Every measurement is captured with hardware, disk model, read size, queue depth, concurrency, alignment, and access pattern recorded. Every result is in worklog/PERFORMANCE.md with baseline and result side by side.

---

3. Correctness is the invariant

The order is non-negotiable:

1. Functional equivalence. Every read, every geometry query, every error path, every edge state behaves identically to the driver-backed implementation, verified by the full test suite on real Windows hardware.
2. Output equivalence. Every end-to-end scenario — scan, recover, health, repair, session save/resume — produces byte-identical results on the same seeded disks as the pre-replacement baseline.
3. Then performance tuning. Tuning never trades correctness, never changes an error, never changes a format, never changes a user-visible state.

If an optimization cannot be achieved without a behavior change, the behavior does not change. The optimization is either found by a different route or the current performance is documented as the achieved state with the reason.

---

4. Architectural parity with the reference

The core must behave the way the reference architecture documents, not merely produce the same output. Where the corpus documents a specific sequence, a specific staging model, a specific concurrency discipline, a specific read pattern, a specific batching or prefetch strategy, or a specific lifecycle — the core matches it. Where the core diverges architecturally, the divergence is either removed or documented in worklog/PARITY-DELTAS.md as a deliberate, evidenced improvement with measured justification.

The driver previously provided a set of peripheral surfaces — geometry reporting, capability discovery, error mapping, identity queries, cancellation, lifecycle — that the direct path now provides. Every one of those surfaces is compared against both the driver's guarantees and the reference's documented behavior. Any surface where the direct path is weaker than either is a finding to fix.

Read the corpus for everything the reference does, not only for the I/O path. Anything downstream that behaves differently because the driver is gone is in scope if the corpus documents the reference behavior for it.

---

5. Non-negotiable constraints

· No behavioral change. Every engine, output, error, format, user-visible state, ABI surface, and CLI surface is identical after this session. Only performance and architectural fidelity change.
· No regression on any metric that matters to a consumer. A faster scan that consumes more memory than the baseline is not a win. A throughput gain that increases CPU per byte and starves the UI is not a win. Every optimization is evaluated across all metrics, not one.
· No masked failure. If a read fails, it fails with the same semantics. No silent zero-fill, no retry loop that hides a dead disk, no fallback that produces a different result.
· No security regression. Elevation stays honest, safety rules stay intact, no raw access outside the consented paths.
· No new dependency on a driver, a helper, a third-party component, or a privilege escalation path. The product remains a user-mode product under administrator privilege.
· No shortcut that trades correctness for speed. A fast wrong answer is worse than a slow right one.
· No hallucination. Every platform capability claim is backed by Microsoft documentation or a captured measurement. Every reference claim is a corpus citation. Every performance claim is a captured measurement on real Windows hardware.
· No stubs, no TODOs, no placeholders. If something cannot be completed, disable it truthfully, document the gap, and track it precisely.

---

6. Validation on real Windows runners

Every measurement and every optimization is validated on real Windows runners in CI, not Linux, not a mock, not a simulated disk:

· Seeded fixtures on real VHD/VHDX or physical media on the runner, with known content, so throughput, latency, and output correctness are all captured against ground truth.
· Per-consumer benchmarks at the read sizes, queue depths, and concurrency the consumer uses, comparing the direct path against the driver baseline, captured as CI artifacts.
· End-to-end scenarios exercising scan, recover, health, repair, and session save/resume, with byte-for-byte output comparison against the baseline.
· Regression gate in CI: a run fails if any consumer's throughput, latency, CPU per byte, memory, or cancellation responsiveness drops below the baseline beyond a defined tolerance. The baseline is versioned and updated only by a reviewed commit with evidence.
· No masked failure in the benchmark: a benchmark that cannot run fails the job with a diagnostic artifact, not a silent skip.

The benchmark harness and its CI workflows are documented in worklog/BENCHMARK.md and are a first-class deliverable — future sessions must be able to reproduce every number this session reports.

---

7. Deliverables

1. worklog/REFERENCE-ARCHITECTURE.md, worklog/ACCESS-LAYER-CURRENT.md, worklog/DRIVER-BASELINE.md, worklog/CONSUMER-PERFORMANCE.md, worklog/PLATFORM-CAPABILITIES.md — the Phase 0 context sheets, pushed before any code change.
2. worklog/PERFORMANCE.md — the driver baseline, the direct path's measurements before and after every optimization, per consumer, per access pattern, per hardware, with the environment recorded.
3. The optimized direct access layer, achieving parity or superiority against the driver baseline on every metric that matters to every consumer.
4. Architectural parity with the reference where the corpus documents it, with every divergence documented in worklog/PARITY-DELTAS.md as a deliberate, evidenced choice.
5. Every peripheral surface the driver provided (geometry, capabilities, identity, error mapping, cancellation, lifecycle) provided by the direct path with equal or better guarantees.
6. worklog/BENCHMARK.md — the benchmark harness and CI workflows, with a regression gate that fails on performance drops beyond tolerance.
7. Full CI green on the target repository's main, for the right reason, with the per-consumer benchmarks running and passing on real Windows runners.
8. Every doc updated to reflect the current performance characteristics and architecture; no stale claims, no outdated numbers, no reference to a driver that no longer exists.
9. worklog.md — the chronological handoff with every commit SHA, every CI run link, every measurement, every optimization attempted (accepted or reverted with reason), and any deferred item with a precise scope.
10. All changes committed and pushed. No commit in the session's history changes behavior beyond the performance and architectural changes explicitly scoped here, and every such change is proven by a measurement and a test.

---

8. First 20 TODOs (spawn immediately, complete 19, spawn 20 more)

· T1: Clone the three repos; record commits; save this prompt (token redacted) to worklog/; worklog entry.
· T2: Spawn parallel agents A–E; read the corpus, the core, and the driver end to end; produce the five Phase 0 context sheets; push before any code change.
· T3: Consolidate the performance target from the five sheets; write worklog/PERFORMANCE-TARGET.md as the gate document; push.
· T4: Baseline the driver-backed performance per consumer on real Windows runners; capture in worklog/PERFORMANCE.md under "Baseline"; push.
· T5: Baseline the current direct-path performance per consumer; capture the gap analysis; push.
· T6: Establish the benchmark harness and its CI workflows, with the regression gate; verify it captures every metric per consumer; push.
· T7: Optimize the access layer's request sizing, alignment, and completion model; measure; accept or revert with evidence; push per optimization.
· T8: Optimize concurrency and queue depth per consumer; measure; accept or revert; push.
· T9: Optimize buffer management and CPU per byte; measure; accept or revert; push.
· T10: Optimize latency for the interactive surfaces (health, geometry, preview); measure; accept or revert; push.
· T11: Optimize cancellation responsiveness; measure; accept or revert; push.
· T12: Optimize memory boundedness at scale; measure; accept or revert; push.
· T13: Close every peripheral-surface gap against the driver and the reference; test each; push.
· T14: Close every architectural divergence from the reference; document every deliberate divergence with evidence; push.
· T15: Run the full end-to-end scenario matrix with byte-for-byte comparison against baseline; fix any regression; push.
· T16: Run the per-consumer benchmark suite against the driver baseline on real Windows runners; confirm every consumer meets or beats baseline on every metric; push.
· T17: Full CI sweep; every workflow green for the right reason; every benchmark job passing; push.
· T18: Update every doc to the current performance and architecture; delete stale claims; remove every reference to a driver that no longer exists; push.
· T19: Final verification sweep — every gate proven, every benchmark captured, no behavioral regression, no performance regression, every deferred item documented with a precise scope; push.
· T20: Handoff — worklog.md complete with every commit SHA, every measurement, every CI run link, and the recommended starting point for the next session; final push.

---

Final reminder

· Performance is the objective; correctness is the invariant. The engines must run at the speed the driver gave them, and faster where the platform permits — measured, not asserted.
· The corpus defines architecture. Where the reference documents a specific behavior, sequence, concurrency discipline, or read pattern, the core matches it or documents a deliberate, evidenced improvement.
· Every claim is a measurement. Every performance number is captured on real Windows hardware with the environment recorded. Every architectural claim is a corpus citation. Every platform capability is backed by Microsoft documentation or evidence.
· No regression on any metric that matters. Every optimization is evaluated across throughput, latency, CPU per byte, memory, and cancellation — not just the one it targets.
· No behavioral change. No security regression. No masked failure. No new dependency. No shortcut that trades correctness for speed.
· No shortcuts. No compromises. No stubs, no TODOs, no placeholders. If something cannot be completed, disable it truthfully, document it, and track it.
· Work long-horizon. Commit and push often. Keep worklog.md current so an unexpected session stop loses nothing. Leave main green at every push.
· Save this prompt to worklog/session-performance-parity-prompt.md (token redacted), read twice, then start with T1.

Good luck. I believe in you.
```
