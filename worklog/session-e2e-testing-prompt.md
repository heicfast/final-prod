# Session prompt — end-to-end testing & validation agent (2026-11-26)

Saved at session start as instructed ("Save to worklog/session-e2e-testing-prompt.md
(token redacted), read twice, start T1"). The GitHub token was supplied out-of-band
and is redacted here; it lives only in local git/credential configuration and is
never committed to any repo content.

---

## Original prompt (token redacted)

```
SESSION PROMPT — End-to-End Testing & Validation Agent

Senior test automation engineer (25+ years), Windows storage testing, VHD
orchestration, recovery tooling validation, CI/CD.

Build a complete end-to-end testing harness for final-prod: new workflows
that pull the latest release, spin up VHD/VHDX virtual disks, seed them
with realistic file trees and files across all categories and sizes, delete
them using multiple methods (recycle bin, permanent delete, format,
partition delete, overwrite-after-delete), then drive the product via both
CLI and API, capture every result, and store everything in the repo as
validated evidence with full documentation.

Repos:
- Product: https://github.com/heicfast/final-prod.git
- Token (env/git config only, never committed): ghp_[REDACTED]

Read the repo end to end — current CLI surface, API surface, existing
workflows, existing test infrastructure — before writing anything. Every
claim is a captured artifact. Every test is reproducible.

### Phase 0 — Read and baseline

Agents (mandatory):

- A: product CLI surface — every command, argument, output format, exit
  code, JSON mode. → worklog/CLI-SURFACE.md
- B: product API surface — every callable operation, struct, error code,
  threading contract. → worklog/API-SURFACE.md
- C: existing tests, fixtures, workflows, and validation artifacts — what
  exists, what can be reused, what is missing. → worklog/TEST-INVENTORY.md
- D: release artifact layout — what the installer places, what the portable
  build contains, what can be invoked headlessly. → worklog/RELEASE-LAYOUT.md

Consolidate → push → then build.

### Phase 1 — Build the test harness

Virtual disk orchestration

- Create VHD/VHDX per scenario using New-VHD (Hyper-V cmdlets available on
  Windows runners). Attach via Mount-VHD / Mount-DiskImage. Record physical
  disk number.
- Format as NTFS, exFAT, FAT32 (and ReFS where the runner supports it).
- Detach cleanly at end of each scenario. Zero leftover mounts.

Seed corpus (deterministic, versioned)

- Realistic directory trees: root, nested (5+ levels), CJK names, Unicode,
  long paths, empty folders, special characters.
- Files across every category: images (JPEG/PNG/HEIC/TIFF/RAW), videos
  (MP4/MOV/AVI/MKV), documents (PDF/DOCX/XLSX/PPTX), archives (ZIP/7z/RAR),
  email (PST/EML if supported), audio, code, text, SQLite, databases,
  executables.
- Every size class: <4 KB, 4 KB–1 MB, 1–50 MB, 50–500 MB, >500 MB.
- Attributes: read-only, hidden, system, ADS (NTFS), hard links (NTFS),
  sparse (NTFS), compressed (NTFS).
- Fragmentation: contiguous, deliberately fragmented (interleave writes),
  highly fragmented.
- Seed manifest: deterministic JSON recording path, size, SHA-256,
  timestamps, attributes for every file. This is ground truth.

Deletion scenarios (the full matrix)

- Recycle Bin delete (shell delete).
- Permanent delete (Shift+Delete).
- Programmatic unlink without recycle bin.
- Delete folder with contents, then empty recycle bin.
- Quick format of the volume.
- Full format (where feasible).
- Delete partition (remove entry, leave unallocated).
- Delete + overwrite (real-world decay): write new files until volume is
  50% / 80% / 100% full, then delete the new files too. Record exactly
  what was written.
- Partial overwrite: overwrite first N sectors of deleted file extents
  (targeted corruption).
- MFT/metadata damage (NTFS): corrupt a subset of MFT records or $Bitmap.

For each scenario: capture a raw disk image or a sector-range hash map
before/after so ground truth is provable. Store as compressed artifacts.

Run the product (CLI and API)

- CLI path: invoke rrecov (or the product's CLI binary) with full
  verbosity, --report JSON, --save session where applicable. Capture
  stdout, stderr, exit code, JSON report, session file, timing.
- API path: drive the C ABI / library surface from a small test harness.
  Capture every call, its arguments, its return code, its output, its
  timing. Compare against CLI for the same scenario.
- Coverage: every scan mode (quick / deep / deep+carve), every filter
  combination, every recovery path, every session save/load/resume, every
  health query, every unsupported-format path.
- Both paths must agree — CLI and API produce byte-identical results for
  the same scenario. Any divergence is a finding.

Capture and record everything

- Per scenario × per path × per mode:
  - Seeds manifest (ground truth).
  - Actual recovery result: recovered tree, per-file comparison against
    manifest (recovered yes/no, hash match, size match, path match,
    timestamps).
  - Metrics: recall, precision, F1, false-positive rate, name/path
    preservation, scan time, recovery time, throughput, peak memory, CPU,
    cancellation responsiveness.
  - Logs: product stdout/stderr, API trace, any product-internal logs.
  - Artifacts: virtual disk images (compressed), session files, JSON
    reports.
- All stored under tests/e2e/results/<run-id>/<scenario>/<path>/.

### Phase 2 — Workflows

Create new workflows under .github/workflows/:

- e2e-smoke.yml — PR-triggered, small matrix (3–4 scenarios, NTFS + exFAT
  + partition delete), fast feedback (~10 min). Runs CLI path only.
- e2e-full.yml — workflow_dispatch + nightly, full matrix (every
  filesystem × every deletion scenario × every scan mode), CLI and API
  paths, complete artifact capture (~60 min or sharded).
- e2e-regression.yml — compares current run metrics against a versioned
  baseline (tests/e2e/baseline.json). Fails if recall/precision drops
  beyond threshold or scan time regresses beyond threshold. Baseline
  updated only by reviewed commit.
- release-validation.yml — pulls the latest release tag artifact, installs
  on clean Windows runner, runs the full e2e matrix against the installed
  product. Confirms the shipped artifact works, not just the build tree.

Rules for every workflow:

- No continue-on-error. No skipped stage without recorded reason. No
  flaky masking.
- Pin action SHAs. Cache dependencies correctly. Artifact retention per
  run type.
- A run that cannot execute (e.g., VHD mount fails on runner) fails the
  job with a diagnostic artifact — never a silent skip.
- Self-hosted Windows runner preferred for full e2e if GitHub-hosted is
  insufficient for VHD. Document the choice.

### Phase 3 — Documentation

- tests/e2e/README.md — how to run locally, how to run in CI, how to add a
  scenario, how to read results.
- tests/e2e/SCENARIOS.md — every scenario, its seed, its deletion method,
  its expected recoverability characteristics.
- tests/e2e/METRICS.md — every metric, its definition, how it is measured,
  what the baseline is.
- tests/e2e/RESULTS-SCHEMA.md — the JSON schema of the results artifact.
- tests/e2e/BASELINE.md — the versioned baseline, how it is updated, when
  it may change.
- Root docs/TESTING.md — updated to describe the e2e suite and how to
  interpret it.

### Phase 4 — Run and validate

- Run e2e-smoke on a PR: green end to end, artifacts produced.
- Run e2e-full via workflow_dispatch: green, artifacts produced for every
  scenario.
- Run e2e-regression: baseline comparison works, threshold enforcement
  works.
- Run release-validation against the current release: green, evidence
  captured.
- Results committed to the repo under tests/e2e/results/ for at least one
  full run, with a README pointing to the artifacts.
- Documented analysis in tests/e2e/analysis/<run-id>.md: per-scenario
  recall/precision, per-mode behavior, any anomalies.

### Rules

- Every result is real. No mocked VHD, no fake seed, no simulated product
  output. The product runs, on real Windows, on real disks, with real
  data.
- Ground truth is the seed manifest. Results are compared against the
  manifest, not against the product's own output.
- Reproducible. Every run can be reproduced by the same workflow on the
  same commit.
- Bounded artifacts. Virtual disk images compressed; retention policy
  prevents unbounded growth.
- No hallucination. Every metric is measured, every finding has evidence.
- No shortcuts. No skipped scenario without documented reason. No
  threshold inflated to hide a regression.
- Preserve audit trail. Results and analysis committed for at least one
  full run.
- Token in env/git config only.
- Push after every coherent unit. Never force-push. main green.

### Validation gates

1. e2e-smoke.yml green on a PR, artifacts produced.
2. e2e-full.yml green on workflow_dispatch, artifacts produced for every
   scenario × filesystem × deletion method × scan mode.
3. e2e-regression.yml compares against baseline and enforces threshold.
4. release-validation.yml validates the current release end to end on a
   clean Windows runner.
5. CLI and API paths agree byte for byte on every scenario.
6. Results, analysis, and documentation committed to the repo.
7. Root docs/TESTING.md updated to reference the e2e suite.
8. No continue-on-error; every failure is a real failure with a
   diagnostic artifact.

### Deliverables

1. Four Phase 0 context sheets, pushed before any code.
2. tests/e2e/ — seed manifest generator, deletion scenario scripts, VHD
   orchestration, CLI runner, API runner, results collector, comparison
   logic.
3. Four new workflows: e2e-smoke.yml, e2e-full.yml, e2e-regression.yml,
   release-validation.yml.
4. tests/e2e/results/ — at least one full run's results committed, with
   README.
5. tests/e2e/analysis/<run-id>.md — per-scenario analysis.
6. Documentation: README.md, SCENARIOS.md, METRICS.md, RESULTS-SCHEMA.md,
   BASELINE.md, updated docs/TESTING.md.
7. tests/e2e/baseline.json — versioned baseline with the current metrics.
8. All workflows green; release validation green against current release.
9. worklog.md — chronological handoff with every SHA, run link, artifact
   location, baseline version.
10. All pushed with evidence-citing messages.

### TODOs (spawn 20+, complete 19, spawn 20 more)

T1–T20 as listed in the original prompt (clone → sheets → generators →
runners → workflows → runs → results → analysis → docs → handoff).

Final reminder

- Real everything. Real Windows, real VHDs, real files, real product, real
  results. No mocks in the product path.
- Ground truth first. The seed manifest is the only standard results are
  compared against.
- Delete realistically. Multiple methods, real overwrite decay, real
  partial overwrite, real MFT damage.
- Both paths. CLI and API must produce identical results. Divergence is
  a finding.
- Record everything. Every run, every scenario, every metric, every
  artifact — stored in the repo, versioned, documented.
- No shortcut, no fake green, no masked failure. Production grade only.
- Work long-horizon. Commit and push often. main green at every push.
- Save to worklog/session-e2e-testing-prompt.md (token redacted), read
  twice, start T1.
```

---

## Execution notes (this session)

- Baseline commit at session start: `126ea398ffa3123c2ff643b196deb1a7b030baab`
  ("feat(W2/F-03): the NTFS parent-sequence gate").
- Environment: the agent sandbox is Linux; all Windows execution (VHD
  orchestration, product runs) happens on real GitHub-hosted Windows
  runners via the workflows built here. No local Windows execution is
  possible or claimed.
- The token is stored at a sandbox-local path outside the repository tree
  and is used only for `git push` / GitHub REST calls. It is never written
  into any committed file; grep gates are added before every push.
