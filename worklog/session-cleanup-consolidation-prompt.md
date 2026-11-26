# Session prompt — Cleanup, Consolidation & Documentation Sync (2026-11-26)

> Redacted copy of the session prompt that opened this session. The GitHub
> token was supplied in env/git-config only and never appears in any file;
> it is replaced below with `***REDACTED***`.

```
Senior engineer (25+ years), codebase hygiene, technical writing, documentation architecture, build/CI, release management.

Take final-prod from its current scattered state to a clean, coherent, single-source-of-truth product repository. Remove slop, dead code, duplicate code, and stale artifacts. Update every document and artifact to the current architecture and state. Mark every issue accurately (fixed vs open). Consolidate all docs and artifacts under one organized roof. Validate everything.

Repos:
- Product: https://github.com/heicfast/final-prod.git
- Token (env/git config only, never committed): ***REDACTED***

Read the entire repository end to end with parallel agents before changing anything. Every deletion is evidenced. Every doc is validated. No assumption.
```

## Phase 0 — Read and inventory

Agents (mandatory):

- **A** — every source file: dead code (unreferenced, unreachable), duplicate code (near-duplicate functions/modules), commented-out blocks, TODO/FIXME/HACK/XXX markers, unused imports, unused dependencies → `worklog/CODE-HYGIENE.md`
- **B** — every document, artifact, worklog, audit trail, test report, benchmark report: current location, last-updated date, status (current / stale / obsolete / duplicate / orphaned) → `worklog/DOC-INVENTORY.md`
- **C** — every issue, finding, TODO tracked anywhere (docs, worklogs, diff-to-fix trails, code comments): status (fixed / open / unknown), evidence of resolution → `worklog/ISSUE-INVENTORY.md`
- **D** — every build file, dependency, action, toolchain: current version vs latest stable, still needed? → `worklog/DEPENDENCY-INVENTORY.md`
- **E** — every test, workflow, installer config, release artifact: what it covers, still valid after recent changes? → `worklog/ARTIFACT-INVENTORY.md`

Consolidate → push → then clean.

## Phase 1 — Remove slop

Every removal evidenced. No guessing.

- Dead code: unreferenced functions, unused types, unreachable branches, orphaned files, deprecated shims with no callers — remove, each removal citing evidence.
- Duplicate code: consolidate to one implementation, all callers updated; prove behavior unchanged with tests.
- Commented-out code: delete (history is in git).
- TODO/FIXME/HACK/XXX: resolved → remove; real → tracked issue in `worklog/OPEN-ITEMS.md`; never a bare marker in shipped code.
- Unused dependencies: remove from manifest/lockfile/build config; prove build works.
- Unused assets: remove or relocate to labeled samples//fixtures/ trees.
- Dead workflows and CI jobs: remove.
- Stale build artifacts committed accidentally: remove + .gitignore.
- Orphaned branches / stale tags: report only (no force-delete without user approval).

Every removal in its own commit with rationale and evidence.

## Phase 2 — Consolidate docs and artifacts under one roof

Target (adapt to what actually exists — do not invent):

```
docs/
├── README.md          # master index
├── ARCHITECTURE.md
├── BUILD.md
├── TESTING.md
├── RELEASE.md
├── SECURITY.md
├── SCOPE.md
├── CAPABILITY-MATRIX.md
├── OPEN-ITEMS.md
├── CHANGELOG.md
├── CONTRIBUTING.md    # if applicable
├── adr/NNNN-<title>.md
├── modules/<per-module docs>
├── worklog/session-<n>-<topic>.md
├── audits/{parity,safety,performance}
└── artifacts/{README.md,benchmarks,validations,reports}
```

Rules: one canonical location per document type; preserve audit history (move, never delete); delete stale artifacts or mark historical under docs/audits/; update every pointer in the same commit; no orphan files; docs/README.md is a complete map.

## Phase 3 — Update every doc to current reality

README, ARCHITECTURE, CAPABILITY-MATRIX (every surface at the correct status, ReFS/LDM included), BUILD/TESTING/RELEASE/SECURITY/SCOPE, per-module docs, versions, historical marking. Every updated doc gets a commit with a one-line note.

## Phase 4 — Mark fixed and open issues

Every issue resolved to FIXED (commit SHA + test), OPEN (precise scope/priority/next step in docs/OPEN-ITEMS.md), OBSOLETE, or INVALID. docs/OPEN-ITEMS.md is the single list: ID, title, severity, module, description, evidence, scope, next step.

## Phase 5 — Update to latest

Dependencies (semver-compatible), actions (latest major, SHAs pinned for third-party), toolchain, runtime requirements, API versions, CI matrix.

## Phase 6 — Validate

Build clean (zero warnings at configured strictness), tests pass, workflows green, release candidate installs/runs/uninstalls on clean Windows, doc accuracy spot-check, navigation check (no dead links).

## Rules

Every removal evidenced; preserve audit trail; no behavior change (structure/docs/dead code only — anything behavior-affecting goes to OPEN-ITEMS); no regression; no stale doc survives; no hallucination; no shortcuts; token in env/git config only; push after every coherent unit; never force-push; main green.

## Validation gates (1–10)

1. worklog/CODE-HYGIENE.md shows every dead code, duplicate, and TODO resolved.
2. Every doc under docs/ reflects current codebase or is marked historical.
3. docs/OPEN-ITEMS.md is the complete, accurate list of unfixed items.
4. docs/README.md is a complete map.
5. Every pointer/cross-reference/link valid — no dead links.
6. Every dependency/action/toolchain/API at latest stable (or documented as pinned).
7. Build clean, tests pass, all workflows green on default branch.
8. Release candidate installs on clean Windows, runs, uninstalls cleanly.
9. No source file has an unreferenced symbol, a bare TODO/FIXME, or a commented-out block.
10. Every commit message cites the evidence for the change.

## Deliverables (1–12)

1. Five Phase 0 context sheets, pushed before any change.
2. docs/ fully consolidated with a complete docs/README.md.
3. Every doc updated to current state or marked historical.
4. docs/OPEN-ITEMS.md — single, complete list of open items.
5. docs/CAPABILITY-MATRIX.md — accurate, current, no stale claims.
6. Dead code, duplicates, orphan files removed — every removal evidenced.
7. Dependencies, actions, toolchain, APIs at latest stable.
8. docs/CHANGELOG.md — accurate history of this session's changes.
9. All workflows green.
10. Release candidate validated on clean Windows.
11. worklog.md — chronological handoff with every SHA, evidence link, final structure.
12. All pushed with evidence-citing messages.

## TODOs (T1–T20)

T1 clone/record/save prompt/worklog entry · T2 spawn agents A–E, context sheets, push · T3 CODE-HYGIENE · T4 DOC/ISSUE/DEPENDENCY/ARTIFACT inventories · T5 design consolidated docs tree · T6 remove dead code · T7 consolidate duplicates · T8 delete commented-out code · T9 TODO/FIXME audit · T10 unused dependencies · T11 unused assets · T12 dead workflows · T13 move docs into structure, update pointers · T14 update every doc · T15 CAPABILITY-MATRIX/OPEN-ITEMS/CHANGELOG · T16 latest-stable updates · T17 doc-vs-code verification · T18 navigation check · T19 full validation sweep · T20 handoff worklog.md + final push.

## Final reminder

Everything under one roof; every removal evidenced; every doc reflects reality; preserve the audit trail; no behavior change; no hallucination; no shortcut; no stale claim survives; work long-horizon; commit and push often; main green at every push.
```

---

## Read-back (twice, per instructions)

**Read 1 — what this session actually is.** A hygiene + documentation-sync pass over the unified `final-prod` repo (engine + ui + contracts + installer), *not* a feature session. The hard constraints: (a) nothing changes product behavior — dead code, docs, and structure only, anything behavior-affecting becomes an OPEN-ITEMS entry; (b) every deletion carries evidence in its commit message; (c) audit history (worklogs, diff-to-fix trails) is moved and marked historical, never destroyed; (d) Phase 0 inventories by agents A–E are pushed *before* any change; (e) never force-push, main green at every push.

**Read 2 — constraints and environment realities.** This sandbox is Linux with **no Rust toolchain, no cargo, no MSVC, no Windows** — so "build clean / tests pass / release candidate on clean Windows" cannot be executed locally; CI (the repo's own GitHub Actions lanes) is the build/test authority, and where CI cannot validate inside this session I must say so plainly and record it in OPEN-ITEMS rather than claim success. Dependency updates cannot be proven "latest stable" without network/registry access to check versions — inventory them, mark what can be verified, and record the rest as assumptions or open items. The repo is large (3,267 files, 842 markdown docs, 2,559 commits, HEAD `f01f630a`); agents A–E run read-only over the tree in parallel, each writing one context sheet. Tag `v0.9.6` / VERSION `0.9.6` is the current release line.
