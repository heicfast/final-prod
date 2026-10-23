# Session 05 Prompt — Verification & Reconciliation Session

Date: 2026-10-23
Role: Senior developer (25+ yrs, low-level systems / storage / filesystems / Windows internals).
Mandate: AUDIT existing implementation of `core_recovery` against the reverse-engineering
research corpus (`easeus-drw-research`), find every discrepancy / gap / bug / inconsistency /
precision error / missing detail, then fix all of them to production-grade quality.

## Protocol

1. Recover full context (all 3 repos + all research artifacts + all documentation).
2. Read the research corpus line by line (mandatory, no skipping).
3. Read the current `core_recovery` implementation line by line (mandatory, no skipping).
4. Cross-check every module against its RE source (artifact + decompiled code + raw RE files).
5. Identify: missing features, wrong constants/offsets, byte-order bugs, incorrect semantics,
   incomplete handlers, stubbed functions, TODOs, approximation errors, tests that assert
   wrong behavior, and anything that fails to match the documented architecture.
6. Fix everything properly — production grade, no shortcuts, no fallbacks, no
   miniaturized/DIY implementations.
7. Run full checks: build (Linux + Windows /W4 /WX), ASan/UBSan, all tests, fuzz harnesses,
   CI, benchmarks.
8. Commit and push ASAP at every milestone with detailed commit messages.
9. Maintain the worklog.

## Scope exclusions (ONLY these)

- Cloud recovery (remote)
- RAID / VM / cloud features (remote)
- NAS redirector / SSH agent / EuMrx (remote)
- Any other remote / network recovery feature

Everything else MUST be implemented in this session, fully, at production grade:
- All filesystem engines at full depth: NTFS, FAT12/16/32, exFAT, ext4, HFS+, Btrfs, ZFS, ReFS.
- All carving formats at full depth.
- All remaining modules, engines, features, functions.
- FULL ReFS and FULL LDM (docs claiming otherwise are wrong and must be corrected first).

## Repo URLs

- Core (target of audit + fixes): https://github.com/heicfast/core_recovery
- Kernel driver (reference + CI/benchmark pattern reference): https://github.com/heicfast/rda-driver
- RE research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token: [REDACTED — rotate after session]
- Driver artifact: tag `v0.2.0` (also v0.2.1 exists) of rda-driver.
- Driver benchmark workflow: rda-driver/.github/workflows/benchmark.yml

## Context recovery protocol

1. Read prompt twice.
2. Check workspace state; re-clone if reset.
3. Save prompt verbatim to worklog/session-05-prompt.md (token redacted).
4. Establish worklog.md entry.
5. Launch parallel agents:
   - Agent A: easeus-drw-research — every artifact under /artifacts, /re-workspace/reversed,
     docs, raw RE files → per-module "contract sheet".
   - Agent B: core_recovery — every source file, header, test, CMake target, workflow, doc →
     "current-state sheet" per module.
   - Agent C: rda-driver — wire protocol headers, client contract, benchmark workflow,
     release assets → "transport contract sheet".
   - Agent D: cross-reference MATCH / MISMATCH / MISSING / UNVERIFIED per module.
6. Consolidate into AUDIT.md (push when complete).
7. Only after AUDIT.md is on main, start fixing.

## Testing & validation

- Raise test coverage; tests for every module and edge case.
- Add TSan job on Linux.
- Structure-aware libFuzzer harnesses for every parser (NTFS, FAT, exFAT, ext4, HFS+, Btrfs,
  ZFS, ReFS, carve validators, CRSF reader).
- Re-run full ASan/UBSan suite, fix every finding.
- Full Windows build /W4 /WX clean.
- Win7-inclusive build posture (import allowlist unchanged, Win7 loadability gate).

## Benchmarks & comparison

- Implement `rrecov bench` with throughput/latency JSON output.
- Seed VHD with realistic files of every supported format; delete / wipe / delete-partition /
  recycle-bin scenarios.
- Run against our RDA driver and against EaseUS (og-codebase in ProgramData).
- Capture logs + results for both; comparison report with recall/precision/throughput/error
  handling. Publish baseline artifacts in CI.

## Security

- ASan/UBSan/TSan full suite.
- Manual review for peak heaps, leaks, UAF, double-free, buffer overflows, access control.
- Verify SDDL on driver handle, admin gating, identity IOCTL exposure.
- No unsafe practices.

## SOPs (strict)

1. Read before writing.
2. Verify, don't assume — every constant/offset traceable to RE source, spec, or
   authoritative doc.
3. No stub, no placeholder, no TODO in shipped code.
4. No fallback paths that silently mask failures.
5. Tests first for pure logic, tests alongside for integration.
6. Every commit compiles and passes tests locally.
7. Every push has a detailed commit message.
8. Security > features.

## Deliverables by end of session

- AUDIT.md — full audit matrix (module → RE reference → status → findings → fixes).
- Corrected docs (roadmap, architecture, README) with ReFS/LDM scope correction.
- All modules at full depth including full ReFS and full LDM.
- Full green CI: builds + tests (ASan, UBSan, TSan), Windows /W4 /WX + tests, CodeQL,
  clang-format, libFuzzer smoke campaign, benchmark run.
- benchmark.yml producing recall/precision/throughput vs seeded VHD (our driver + EaseUS).
- v0.2.0 release with artifacts and notes.
- Updated worklog.md with session summary and handoff notes.
