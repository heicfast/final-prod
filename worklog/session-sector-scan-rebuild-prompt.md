# Session: Sector Scanning, Detection & Real-Time Streaming Rebuild

- **Date**: 2026-11-18
- **Operator**: ZCode agent (senior systems engineer role)
- **Product repo**: https://github.com/heicfast/final-prod.git @ `976bced8d0dc7d8e8fa6e83fbaae1d5f3bcf5bd2` (main, clean at session start)
- **Reference corpus**: https://github.com/heicfast/easeus-drw-research.git @ `be0cfe95ecf22ac67e0f3a823dd003163b196d38` (main)
- **Token handling**: stored in git credential store only; never committed. This prompt saved with token redacted.

## Mandate (redacted original prompt summary)

Sector scanning, detection, real-time result streaming, sector counts, live progress,
percentage calculations, total sector count are broken in the product. Scan stalls,
shows only system files or phantom non-existent files. Rebuild these subsystems from
scratch using the reference architecture (easeus-drw-research corpus) as ground truth.

Phases:
- Phase 0: read both repos end-to-end with parallel agents A–E → five context sheets
  (PRODUCT-PIPELINE, REFERENCE-SCAN, REFERENCE-ALGORITHMS, MS-DOCS, REPRODUCTION).
- Phase 1: instrument every stage, produce worklog/DIAGNOSIS.md with quantified break points.
- Phase 2: rebuild sector scanning & progress (IOCP overlapped I/O, aligned reads,
  IOCTL geometry, weighted monotonic progress, throttled live streaming, stall elimination).
- Phase 3: rebuild detection & classification (signatures, validators, metadata, phantom filter,
  category model, prioritization, recovery chance).
- Phase 4: rebuild real-time result streaming (during-scan delivery, backpressure, ordering,
  cancellation).
- Phase 5: validation on real Windows with seeded fixtures and seed manifests.

Rules: no fake anything, no hard-coded progress, no phantom results, root-cause fixes only,
reference = corpus citation, platform = MS doc, product = captured artifact, push after every
coherent unit, main green.

## Environment constraints (this sandbox)

This ZCode sandbox is **Linux**, not Windows. "Every fix proven on real Windows with seeded
data" cannot be executed here directly. Mitigation strategy:
1. All reading, diagnosis, and code rebuild work proceeds here with corpus citations.
2. Windows-executable instrumentation/capture scripts are produced as product repo assets
   (PowerShell + C harnesses in engine/tools or tools/forensic) so the evidence capture can be
   run on a real Windows box and the outputs committed to worklog/VALIDATION/.
3. CI: the product's existing Windows CI definitions are the execution vehicle; this session
   can prepare, commit and push them but cannot run Windows-only workflows locally.
Any validation step that requires a live Windows host is documented in
worklog/BEHAVIOR-FINDINGS.md with precise scope, and the capture harness is delivered ready
to run. Truthful-disable per the session rules.

## Session log

| Time (UTC) | Entry |
|---|---|
| 2026-11-18 | T1 complete: repos cloned, commits recorded (see above), prompt saved redacted, git identity + credential store configured (token never committed). Next: T2 parallel read agents. |

## Session completion record (step-budget cut)

Phase 0 (T2) complete: five context sheets pushed at `d69893ed`.
Phase 1 (T3) complete: DIAGNOSIS.md (this directory) — stages quantified, fix plan F-A..F-G.

**Not started (truthful scope, next session picks up from F-A):**
- F-A row-carrying event + during-scan streaming (entry points in DIAGNOSIS.md)
- F-B/F-C byte-derived monotonic percent + single sector axis
- F-D manifest-vs-results gate (the phantom/accuracy instrument)
- F-E bridge filter unification, F-F grading coverage, F-G carve throughput (BF-30)
- T14 Windows CI sweep, T15 release validation — Linux sandbox, runners required
- worklog/VALIDATION/ remains honestly EMPTY for this session's fixes

Handoff: entry points + evidence live in worklog/session-sector-scan-rebuild/. The
concurrent installer-api lane's AsyncRing repair needs its Windows matrix re-run (OPEN-1).
