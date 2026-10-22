# Session 05 Prompt — Verification & Reconciliation Session

**Date:** 2026-10-23
**Mandate:** Audit + fix session (QA + senior reviewer + fixer), NOT blind feature building.

## Role

Take over `heicfast/core_recovery` as a senior developer with 25+ years of experience in
low-level systems, storage, filesystems, and Windows internals. The mandate this session is
**not** to continue building features blindly — it is to **audit the existing implementation
against the reverse-engineering research corpus, find every discrepancy, gap, bug,
inconsistency, precision/accuracy error, and missing detail introduced or missed by prior
implementation agents, and then fix all of them to production-grade quality**.

## Workflow

1. Recover full context (all 3 repos + all research artifacts + all documentation).
2. Read the research corpus line by line (mandatory, no skipping).
3. Read the current `core_recovery` implementation line by line (mandatory, no skipping).
4. Cross-check every module against its RE source (artifact + decompiled code + raw RE files).
5. Identify: missing features, wrong constants/offsets, byte-order bugs, incorrect semantics,
   incomplete handlers, stubbed functions, TODOs, approximation errors, tests that assert wrong
   behavior, and anything that fails to match the documented architecture.
6. Fix everything properly — production grade, no shortcuts, no fallbacks, no miniaturized/DIY
   implementations.
7. Run full checks: build (Linux + Windows /W4 /WX), ASan/UBSan, all tests, fuzz harnesses, CI,
   benchmarks.
8. Commit and push ASAP at every milestone with detailed commit messages.
9. Maintain the worklog.

## Scope

**Scope exclusions (ONLY):**
- Cloud recovery (remote)
- RAID / VM / cloud features (remote)
- NAS redirector / SSH agent / EuMrx (remote)
- Any other remote / network recovery feature

**Everything else MUST be implemented, fully, at production grade:**
- All filesystem engines at full depth: NTFS, FAT12/16/32, exFAT, ext4, HFS+, Btrfs, ZFS, ReFS.
- All carving formats at full depth.
- All remaining modules, engines, features, functions, etc.
- Full ReFS and full LDM (prior docs incorrectly deferred these — correction required).

## Repositories

- Core (target of audit + fixes): https://github.com/heicfast/core_recovery
- Kernel driver (reference + CI/benchmark pattern): https://github.com/heicfast/rda-driver
- RE research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — rotated after session]

## Context Recovery Protocol

1. Read this prompt twice, top to bottom, before touching anything.
2. Check workspace state; re-clone if reset.
3. Save the current prompt verbatim to `worklog/session-05-prompt.md` (token redacted).
4. Establish `worklog.md` entry for this session.
5. Launch parallel agents to read line by line:
   - Agent A: `easeus-drw-research` — every artifact under `/artifacts`, `/re-workspace/reversed`,
     docs, raw RE files. Per-module "contract sheet".
   - Agent B: `core_recovery` — every source file, header, test, CMake target, workflow, doc.
     "Current-state sheet" per module.
   - Agent C: `rda-driver` — wire protocol headers, client contract, benchmark workflow, release
     assets. "Transport contract sheet".
   - Agent D: Cross-reference. For each module in B, find counterpart in A. Mark every constant,
     struct, offset, algorithm, error code, control-flow contract as MATCH / MISMATCH / MISSING /
     UNVERIFIED.
6. Consolidate into `AUDIT.md` (push when complete).
7. Only after `AUDIT.md` is on `main` start fixing.

## Testing & Validation

- Raise test coverage; tests for every module and edge case.
- Add TSan job on Linux.
- Add structure-aware libFuzzer harnesses for every parser.
- Re-run full ASan/UBSan suite and fix every finding.
- Full Windows build /W4 /WX clean.
- Win7-inclusive build posture (import allowlist unchanged, Win7 loadability gate).

## Benchmarks & Comparison

- `rrecov bench` with throughput/latency JSON output.
- Seed VHD with realistic files of every supported format; delete/wipe/delete-partition/
  recycle-bin scenarios.
- Run against our RDA driver and against EaseUS (og-codebase in `ProgramData`).
- Comparison report: recall/precision/throughput/error handling.
- Publish baseline artifacts in CI.

## Security

- ASan/UBSan/TSan full suite.
- Manual review for peak heaps, leaks, UAF, double-free, buffer overflows, access control.
- Verify SDDL on the driver handle, admin gating, identity IOCTL exposure.
- No unsafe practices.

## Methodology

Research → Plan → Review → Implement → Re-review → Fix → Finalize → Test → Validate →
Benchmark → Compare → Loop.

- No shortcuts. No fallbacks. No compromises. No miniaturized or DIY implementations.
- Do not hallucinate. Verify every constant, offset, and semantic against the RE corpus, the
  driver's wire headers, Microsoft docs, or authoritative filesystem specs (NTFS, FAT, exFAT,
  ext4, HFS+, Btrfs, ZFS, ReFS, LDM). When sources conflict, decompiled code + spec win;
  document any deliberate divergence.
- Same tech stack as original module where sensible (C/C++ for engines; Windows API for
  platform; C for kernel-adjacent).
- Install reverse-engineering tools as needed (Ghidra headless, objdump, pefile, 7z, ntfs-3g,
  btrfs-progs, zfs tools, debugfs, HFS+ tools) to independently verify.
- Long-horizon loops. 20+ TODOs, complete 19, spawn 20 more, repeat.
- Commit and push ASAP after every logical unit. Detailed commit messages.
- Maintain worklog.md continuously.
- Re-read prompt + docs + artifacts periodically.
- Run CI locally where possible before pushing.

## SOPs (strict)

1. Read before writing.
2. Verify, don't assume.
3. No stub, no placeholder, no TODO in shipped code.
4. No fallback paths that silently mask failures.
5. Tests first for pure logic, tests alongside for integration.
6. Every commit compiles and passes tests locally.
7. Every push has a detailed commit message.
8. Security > features.

## Deliverables

- `AUDIT.md` — full audit matrix (module → RE reference → status → findings → fixes).
- Corrected docs (roadmap, architecture, README) with ReFS/LDM scope correction.
- All modules at full depth including full ReFS and full LDM.
- Full green CI: build + tests (ASan, UBSan, TSan), Windows /W4 /WX + tests, CodeQL,
  clang-format, libFuzzer smoke campaign, benchmark run.
- `benchmark.yml` producing recall/precision/throughput against seeded VHD.
- `v0.2.0` release with artifacts and notes.
- Updated `worklog.md` with session summary and handoff notes.

## User directives (verbatim)

- "check line by line for inconsistencies, missing implementation, errors, bugs, etc and fix
  everything, install reverse engineering tools and packages yourself and verify independently
  that every algorithm, dll, modules is properly re-written accurately and precisely with
  nothing else remaining or missing. also refer raw RE source files for context."
- "no fallback or compromise is allowed at any point. strictly."
- "this is senior production grade project, not another ai/llm vibe coded slop, so act
  accordingly."
