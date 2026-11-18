# SESSION PROMPT — Performance Diagnostic & Optimization Agent (final-prod vs og-codebase)

> Saved per the session mandate (read twice before T1). The GitHub token
> supplied in the prompt is **redacted here and everywhere in-tree** — it
> lives in env/git-credential config only, never committed. Rotate it after
> the session: it was transmitted in plaintext chat.

```
Senior performance engineer (25+ years), Windows storage internals, overlapped
I/O, IOCP, threading architecture, recovery engine throughput.

Find why final-prod is slow compared to the reference (og-codebase + research
corpus), diagnose the exact bottleneck in our I/O pipeline, threading model,
queue depth, and CPU/memory limits, and implement the fix to make final-prod
the fastest without compromise. Every diagnosis backed by measurement. Every
fix proven with before/after benchmarks on real Windows.

Repos:
- Product (target): https://github.com/heicfast/final-prod.git
- Reference corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- Token (env/git config only, never committed): [REDACTED — env/credential
  store only; plaintext-chat exposure noted, rotation advised]

Read both repos end to end with parallel agents before touching anything. No
guess. Every claim is a captured measurement or a corpus citation.

Phase 0 — Parallel read, then plan
Agents (mandatory):
· A: final-prod full state — every module, engine, I/O path, threading model,
  queue depth, buffer management, cancellation, workflow, CI.
  → worklog/PRODUCT-STATE.md
· B: corpus architecture — every documented I/O model, threading discipline,
  read sizing, queue depth, concurrency, buffering, prefetch, cancellation.
  → worklog/REFERENCE-ARCHITECTURE.md
· C: corpus raw/decompiled artifacts — exact I/O parameters, threading
  parameters, queue depths, buffer sizes, overlapped/IOCP usage,
  per-transport behavior. → worklog/REFERENCE-RAW.md
· D: independent research — Windows overlapped I/O, IOCP, thread pool, direct
  I/O alignment, queue depth, per-transport performance on Win7/10/11.
  → worklog/PLATFORM-PERF.md
· E: baseline measurement — capture final-prod throughput, latency, CPU,
  memory per engine on seeded fixtures; compare against reference where
  feasible. → worklog/BASELINE.md
Consolidate → push → only then optimize.

Phase 1 — Diagnose the bottleneck
Instrument the pipeline end to end. Every hypothesis is a measurement, not a
guess: I/O model (synchronous vs overlapped/IOCP), queue depth, threading
model, locking, buffer management, alignment (physical sector via
IOCTL_STORAGE_QUERY_PROPERTY), CPU vs I/O bound, per-transport behavior
(NVMe, SATA SSD, HDD, USB, SD).
Deliverable: worklog/DIAGNOSIS.md — the exact bottleneck, quantified: read
size, queue depth, thread count, lock contention, CPU utilization, memory
pressure, measured throughput of each stage. Push before any optimization.

Phase 2 — Implement the fix
Fix at the root cause, priority order:
1. I/O model: overlapped I/O + IOCP (or CreateThreadpoolIo/StartThreadpoolIo);
   sliding window of N outstanding reads, refill on completion.
2. Queue depth: sweep 1,2,4,8,16,32 per device class, measure, pick plateau.
3. Threading: I/O off UI/async-orchestration threads; dedicated I/O threads
   or IOCP pool; spawn_blocking-equivalent for blocking calls; CPU-parallel
   parsing where CPU-bound.
4. Locking: remove global locks in read path/result tree; per-bucket locks or
   lock-free.
5. Buffer management: pooled reused buffers, aligned (VirtualAlloc /
   _aligned_malloc), large reads (16 MiB class) where beneficial.
6. Alignment: physical-sector alignment on every direct read; 64 KiB NVMe
   slow-path property preserved.
7. Per-transport tuning per class; record optimal per class.
8. CPU parallelism: parallelize per-record parsing if CPU-bound; do not
   parallelize read path for I/O-bound workloads without measurement.
Every change measured before/after on the same fixture. Accept or revert by
the number.
Deliverable: worklog/PERFORMANCE.md — every optimization, before/after
throughput, latency, CPU, memory, cancellation responsiveness, per device
class, environment recorded.

Phase 3 — Validate
Every engine throughput/latency/CPU-per-byte/memory/cancellation meets or
beats reference on same fixture; no behavioral/format/output/error change;
full Windows CI green; release candidate validated on clean Windows with
seeded data.

Rules: no hallucination (measurement or corpus citation only); no
shortcuts/stubs/TODOs; root-cause only; no regression on any existing
surface; preserve audit trail; token env/git-config only; push after every
coherent unit, never force-push, main green.

Deliverables: five Phase 0 sheets; DIAGNOSIS.md; PERFORMANCE.md; the
optimized I/O pipeline; BASELINE.md; BEHAVIOR-FINDINGS.md (limitations with
precise scope); full Windows CI green; RC validated on clean Windows with
seeded data; worklog.md chronological handoff (every SHA, measurement, CI
link); all pushed with evidence-citing messages.

TODOs T1–T20 as specced (clone+record; agents A–E; diagnosis; baseline;
I/O-model audit; queue/lock/buffer/alignment audit; per-transport sweep;
T8–T14 implement each fix with before/after; validate vs reference; CI sweep;
RC validation; docs update; final verification; handoff).

Final reminders: measure first — the bottleneck is a number, not a guess.
Standard patterns: overlapped I/O + IOCP sliding window; one thread per CPU
for completions; large aligned reads (16 MiB class); physical-sector
alignment via IOCTL_STORAGE_QUERY_PROPERTY; no global lock in the hot path;
pooled buffers. No fake anything. Production grade only. Work long-horizon;
commit and push often; main green at every push.
```

Session-start state recorded by T1 (2026-11-18):

- final-prod HEAD: `8217bc26f85004c3da910ba6072aaf4cf7573c60` (main, clean)
- easeus-drw-research HEAD: `be0cfe95ecf22ac67e0f3a823dd003163b196d38` (main, clean)
- Sandbox: Linux, 2 vCPU, 4 GiB RAM, no Windows host — all real-Windows
  measurements ride the repo's established GitHub-hosted windows-2022
  runner lanes (engine-ci / bench dispatch), exactly as the corpus repo's
  own workflow does its heavy lifting. Local container does: static code
  audit, Linux-runnable unit/lock/microbench evidence, build verification
  through CI.
- Known prior-session frontier (worklog.md T20 handoff): v0.9.3 bumped but
  untagged; bridge-integration two new gates mid first-execution cycle with
  a NULL-POINTER PANIC queued for the next debug session (not this
  session's mandate — performance; noted so it is not lost).
