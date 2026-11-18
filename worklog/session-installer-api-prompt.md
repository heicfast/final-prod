# Session charter — Installer Integration & API Surface Expansion (2026-11-18)

> Saved verbatim from the session prompt per T1, token redacted (the token
> lives in git config / environment only — never in a committed file).
> Original charter text below; the integration boundary recorded in
> `worklog/INSTALLER-CURRENT.md` §0 applies throughout.

## Mandate

Senior systems engineer (25+ years), Windows installer architecture, module
integration, API surface design, reference parity.

Two mandates: (1) integrate every library, module, loader, engine, and
algorithm into the final installer the way og-codebase does — organized
directories, correct locations, everything present after install. (2) Expand
the API surface to match and exceed og-codebase's — more functions, features,
support, detail, speed, efficiency — following the reference architecture.

Repos:

- Product (target): https://github.com/heicfast/final-prod.git
- Reference corpus (ground truth): https://github.com/heicfast/easeus-drw-research
- Token (env/git config only, never committed): [REDACTED — token redacted per rule]

Read both repos end to end with parallel agents before touching anything.
Every claim is a corpus citation or a captured measurement.

## Phase 0 — Parallel read, then plan

Agents (mandatory):

- A: final-prod installer — every script, manifest, packaging config, staging
  step, file placement rule, current installed footprint. → worklog/INSTALLER-CURRENT.md
- B: corpus installed og footprint — every file, directory, library, module,
  loader, engine, and their exact install locations, with citations. → worklog/REFERENCE-FOOTPRINT.md
- C: corpus API/CLI/module surface — every function, parameter, output, edge
  case, performance characteristic documented. → worklog/REFERENCE-API.md
- D: final-prod current API surface — every public function, struct, enum,
  error, callback, CLI command, output format. → worklog/PRODUCT-API.md
- E: cross-reference — every footprint gap (missing module/file/dir), every
  API gap (missing function/param/feature), every placement divergence. → worklog/INTEGRATION-MATRIX.md

Consolidate → push → only then implement.

## Phase 1 — Installer integration

- Match og's installed layout: every directory, every file class, every
  library, every module, every loader, every engine, placed in the equivalent
  reference location (or industry-standard equivalent if the reference's
  choice is idiosyncratic — document the deviation).
- Every module the product claims is present on disk after install. Every
  file placed by the installer is referenced by the product. Every file needed
  by the product is placed.
- Organized directory structure: binaries, libraries, engines, loaders,
  modules, resources, configurations, data — each in its own documented tree.
- Install, uninstall, update, repair — each validated on clean Windows. No
  orphans on uninstall. No data loss on update. No breakage on repair.
- No visible console, shell, or system dialog during install or first launch.
- Reproducible from clean checkout with one command.
- Deliverable: worklog/FOOTPRINT-DELTA.md — every reference file/dir vs
  product, resolution per item.
- Validation: worklog/VALIDATION/INSTALLER/ with captured install transcript,
  directory listing, uninstall transcript.

## Phase 2 — API surface expansion

- Every API function, parameter, output format, error class, and edge case
  documented in og is present in the product with matching semantics.
- New capabilities the reference exposes are implemented — file/module
  support, per-format handling, detailed metadata, advanced options,
  streaming, cancellation, threading, batch operations.
- Performance: every new and existing API function is fast and efficient — no
  accidental full materialization, no redundant copies, bounded memory,
  correct concurrency, correct cancellation.
- API surface is coherent: consistent naming, consistent error model,
  consistent threading contract, consistent lifecycle, documented for every
  function.
- CLI, ABI, and library bindings all stay in sync — one source of truth per
  capability.
- Deliverable: worklog/API-DELTA.md — every reference function/feature vs
  product, resolution per item.
- Validation: worklog/VALIDATION/API/ — per-function test + real-Windows
  evidence + performance measurement.

## Rules

- No hallucination. Reference = corpus citation. Product = test or captured
  artifact. Performance = measured.
- No shortcuts, stubs, TODOs, placeholders. Truthful disable with precise
  scope if incompletable.
- Root-cause only. No symptom patches.
- No regression on any existing surface — every existing API preserves its
  contract; every existing install behavior preserved.
- Match og's architecture for placement and API design; deviate only with
  documented rationale.
- Preserve audit trail.
- Token in env/git config only.
- Push after every coherent unit. Never force-push. main green.

## Validation gates

1. Installed footprint matches reference functionally — every module
   present, placed correctly, referenced. Evidence: captured directory
   listing vs REFERENCE-FOOTPRINT.md.
2. Install, uninstall, update, repair validated on clean Windows. No orphans.
   No data loss. No consoles.
3. Every reference API function/param/output present with matching semantics
   and proven by test.
4. Every new API function: unit test + real-Windows validation + performance
   measurement.
5. API coherent — one source of truth, consistent contracts, documented.
6. Full Windows CI green.
7. Release candidate validated end-to-end on clean Windows.
8. Docs updated to current reality; stale claims removed.

## Integration boundary (this session's recorded scope rule)

`og-codebase/` in the corpus is an extracted EaseUS Data Recovery Wizard
installation — proprietary EaseUS binaries (DRW.exe/DRWUI.exe/DRWService.exe,
the per-FS SearchDLLs, the EU* repair tools, Qt runtimes, drivers). Those
files are READ-ONLY research artifacts. Nothing from `og-codebase/` may be
copied into, shipped by, or redistributed through this product's installer.
"Integration" in this session means exactly what this repository's established
discipline means (docs/PARITY-DELTAS.md): every reference CAPABILITY present
in this product's own original code, placed in an organized layout, with
divergences recorded and rationale given. Capability parity, never file
copying.
