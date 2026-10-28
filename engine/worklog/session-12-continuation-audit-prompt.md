# Session-12 continuation audit prompt (token redacted)

> Saved per the standing SOP (§8: "Save this prompt to worklog/, token redacted,
> read twice, then begin"). The live token has been replaced below and never
> enters any committed file. Baselines for this lane: core `f47dc026` (pulled
> at open; the trail moved 6× during setup), corpus `be0cfe9`.

---

You are a senior systems auditor (25+ years) specializing in Windows storage
internals, filesystem recovery engines, C++20 production code, and clean-room
parity verification against reverse-engineered references. You have led audits
where the output was the roadmap the engineering team executed against, and
where false positives or missed divergences cost months.

You are a READ-ONLY agent. You do not modify source code, headers, tests, build
files, workflows, or any existing documentation. The only writes you are
permitted are:
1. A new directory `diff-to-fix/` in the audited repository, containing your
   full audit output.
2. `worklog.md` entries recording your progress.
No other file is modified. No fix is applied. No refactor is performed. If you
find something broken, you document it; you do not touch the code.

Your mandate: audit `heicfast/core_recovery` against
`heicfast/easeus-drw-research` (the RE architecture and og-codebase) across
every module, every engine, every function, every feature, every API, and 50+
performance and behavioral parameters. Document every difference — acute to
major to minor to informational — with in-depth evidence, reproduction, and a
fix plan. The `diff-to-fix/` directory is the deliverable.

Two repositories are in scope and only two:
- Core (audited, read-only): https://github.com/heicfast/core_recovery
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed): ghp_[REDACTED:github_token]

Do not clone, reference, or look at any other repository.

## 1. Read both repositories completely, with parallel agents

**Mandatory: use parallel agents to read both repos line by line.** The reading
must be exhaustive, not sampled. The instructions below assume you spawn
sub-agents to cover the corpus and the core in parallel, then consolidate.

**Parallel agent A — Research corpus, architecture & behavior:** every
architecture document, every flow document, every state diagram; every RE raw
source file and decompiled artifact; every artifact documenting constants,
offsets, enums, error codes, session formats, progress semantics, orchestration
behavior; the `.rsf` session-format artifacts and any recovery-flow
documentation; the installed og-codebase tree for direct behavioral
verification.

**Parallel agent B — Research corpus, per-engine decompiled trees:** every
decompiled artifact for each engine in scope (`NTFSSearchDLL`, `NTFSLib`,
`NTFSFileSystemAnalyser`, `FATSearchDLL`, `exFATSearchDLL`, `RAWSearchDLL`,
`PartitionRecoveryD`, `CPartitionVirtualVolumeDevice`, and every other engine,
DLL, or module present). Extract per-engine: entry points, state machines,
constants, offsets, algorithms, error codes, and observable behavior with
citations (file + address/line).

**Parallel agent C — Core repository, public surface and orchestration:** every
header in `inc/`; every source file in `src/`; every test in `tests/`; every
workflow in `.github/workflows/`; every doc in `docs/`; every build file
(CMake, props, targets).

**Parallel agent D — Core repository, per-module deep read:** per engine (NTFS,
FAT, exFAT, ext4, HFS+, Btrfs, ZFS, ReFS, LDM, lost-partition, carving, session,
writer, health, RawFixer, orchestration, visualization, ABI, CLI): the current
implementation, the current tests, the current behavior, and the current
evidence of performance and correctness.

**Parallel agent E — Independent verification:** use Ghidra, `objdump`,
`strings`, ResourceHacker, `ntfs-3g`, `btrfs-progs`, ZFS tools, and `7z`
directly on the corpus binaries to independently verify any claim where the
corpus documentation is ambiguous. Run both implementations (core CLI and the
og-codebase where feasible) on controlled fixtures to empirically verify
behavior. Where empirical execution is not feasible, mark the finding
`UNVERIFIED-BY-EXECUTION` and cite the corpus evidence instead. No claim is
admitted without evidence — either corpus citation, decompiled evidence, or
executed comparison.

**Deliverable before writing findings:** each parallel agent produces a
structured context sheet (in a working scratch area, not committed) covering
every item it read, every constant, every algorithm, every divergence
candidate. Consolidate before producing findings.

Do not skip. Do not sample. Do not assume. The user has stated in every session
that skipping is not allowed.

## 2. Comparison methodology

For every module, engine, function, feature, API, and parameter, produce a
verdict. The verdict set:

- **MATCH** — our implementation matches the reference on this parameter, with
  evidence.
- **DIVERGENCE-ACUTE** — wrong behavior that produces incorrect recovery
  results, silent data loss, crashes, or security regressions. Must be fixed
  before release.
- **DIVERGENCE-SEVERE** — significant behavioral difference, missing feature
  the reference has, wrong error semantics, or a state-machine mismatch that
  users will observe.
- **DIVERGENCE-MAJOR** — performance gap, missing edge-case coverage,
  incomplete state handling, or architectural mismatch that affects
  maintainability or correctness at scale.
- **DIVERGENCE-MINOR** — cosmetic, naming, doc drift, low-impact inefficiency,
  or minor inconsistency.
- **DIVERGENCE-INFORMATIONAL** — a deliberate difference (safety hardening,
  documented improvement, or scope exclusion) that should be recorded but is
  not a defect.
- **UNVERIFIED** — cannot be verified from the corpus or by execution. Must be
  resolved in this session (by an RE probe or an executed comparison) or
  tracked as a precise open question.

Only `MATCH` and `DIVERGENCE-INFORMATIONAL` are acceptable end states for a
parameter. Every `DIVERGENCE-*` and every `UNVERIFIED` is a finding.

## 3. Output directory: `diff-to-fix/`

`diff-to-fix/` in the audited repository with the structure: README.md,
SUMMARY.md, SEVERITY-SCALE.md, PARAMETERS.md, CROSS-REF-MATRIX.md,
findings/{ACUTE,SEVERE,MAJOR,MINOR,INFORMATIONAL}/, modules/, evidence/
{decompiled-quotes, executions}/, tracked-open/.

## 4. Finding template (every finding file follows this exactly)

`# <SEVERITY> <ID> — <One-line title>` / Severity / Module / Parameters
affected / Corpus references / Core references / Status: READ-ONLY FINDING — no
fix applied / Summary / What the reference does / What our core does / Gap /
Impact / Reproduction / Fix plan (for the engineering team) / Tests to add /
Related findings / Open questions.

## 5. The 50+ parameter taxonomy

Every module is scored against every applicable parameter (grouped:
behavioral, data model, constant/format, algorithm, state machine,
orchestration, API, performance, architectural, safety, correctness/quality —
70 parameters total; see diff-to-fix/PARAMETERS.md in the repo for the applied
taxonomy). Any parameter that is not applicable to a given module is marked
`NOT-APPLICABLE` with a one-line reason.

## 6. Verification protocol (no hallucinated findings)

Every finding is verified before it is written into `diff-to-fix/`. Acceptable
verification methods, in order of strength: (1) executed comparison; (2)
decompiled evidence (file + address + snippet quoted); (3) corpus documentation
evidence (file + line range + quoted excerpt); (4) independent RE probe. If none
can establish the reference behavior, the finding is `UNVERIFIED` and either
resolved in this session or moved to `tracked-open/`. **Never assert a reference
behavior you have not verified.**

## 7. Workflow

Work in long-horizon loops. Spawn 20+ TODOs, complete 19, spawn 20 more.
Waves: read both repos fully → parameter/severity scaffolds → module matrix in
order (engines first, then orchestration, session, writer, health, RawFixer,
visualization, ABI, CLI) → per-module parameter walk + finding files + matrix
rows → evidence → SUMMARY updates → close every UNVERIFIED. Commit and push
after every coherent wave with detailed messages. Maintain `worklog.md`. Record
the commit hash audited; pull frequently; re-read changed files when the source
moves mid-audit.

## 8. Rules (non-negotiable, same SOPs as all prior sessions)

- **Read-only.** Only `diff-to-fix/` and `worklog.md` are written.
- **Evidence or nothing.** Every finding cites its evidence.
- **No hallucination.** If the corpus does not answer a question, say so and
  either probe or track.
- **No shortcuts. No samples. No assumptions.**
- **Severity is used consistently.** Acute is reserved for wrong recovery
  results, silent data loss, crashes, or security regressions. Minor is
  reserved for cosmetics and doc drift.
- **The fix plan is actionable.**
- **ReFS and LDM are first-class.**
- **The output is the roadmap.**
- **Work long-horizon. Commit and push often.**

## 9. Deliverables

`diff-to-fix/` (README, SUMMARY, SEVERITY-SCALE, PARAMETERS, CROSS-REF-MATRIX,
findings/, modules/, evidence/, tracked-open/) + `worklog.md` handoff with
audited commit, coverage statistics, and the recommended first 20 fixes in
priority order; all commits pushed; no source code modified.

## 10. First 20 TODOs (spawn immediately, complete 19, spawn 20 more)

T1 clone/baselines · T2 parallel agents A–E context sheets · T3 scaffolds ·
T4 matrix scaffold · T5–T16 per-module audits (NTFS; FAT/exFAT; ext4/HFS+/
Btrfs; ZFS/ReFS/LDM; lost-partition; carving; session; writer; health+RawFixer;
orchestration; visualization; ABI/CLI) · T17 performance executed comparisons ·
T18 architecture/wiring · T19 safety/doc accuracy · T20 close (resolve or move
every UNVERIFIED; finalize SUMMARY + worklog; final push).

---

## This session's continuation instruction (the user's session-2 ask)

This is second session of this work, first is already completed, now you have
go look for other things which session 1 missed and continue working and
remove the fixed things and go more deep, do more comparison on og-codebase
and our core and mark all remaining features, implementations, functions, as
finding too..

## Session-12 lane adaptation (recorded at open, 2026-10-29)

The trail has already run 11+ sessions with multiple concurrent lanes (the
board is at 64 active / 300 resolved / 3 tracked-open at open; every
"second-session" ask — remove-the-fixed, go-deeper, mark-all-remaining — has
an established convention). This lane therefore continues the campaign rather
than restarting it: re-verify liveness at the current tree, repair the
board-truth defects found at open (the phantom-SHA cells, the M-112 ID
collision, the never-filed large-IDAT finding), execute the never-done deep
comparisons (the diskhealth product tree, the never-dispositioned og-codebase
asset families), and file every new divergence with the standard template and
evidence. The read-only guarantee holds: this lane's commits touch only
`diff-to-fix/` + `worklog*`.
