# Session: Cross-Repository Feature Harvest & Implementation

- **Date**: 2026-11-19
- **Operator**: ZCode agent (senior systems engineer role, 25+ years filesystem internals / recovery engines / forensic tooling)
- **Product repo**: https://github.com/heicfast/final-prod.git @ `201100aa2978e26c218c73c00f7c15c23757d554` (main, clean at session start, VERSION 0.9.5)
- **Reference repos (all cloned read-only under `/home/z/my-project/repos/`)**:
  - A/target — final-prod @ `201100aa2978e26c218c73c00f7c15c23757d554` (MIT)
  - B — terraseraph/sectorforge @ `b292a299c18013706e81d670657ced6c43f200ca` (MIT)
  - C — htoklu/halim-recovery @ `3b5d66d8537432f54b76489e717387f9da198635` (MIT)
  - D — cgsecurity/testdisk @ `02941e1b0fce14a33932724de85cbe8385bb70d0` (GPLv2+, COPYING)
  - E — sleuthkit/sleuthkit @ `3dcb43668d811548206fbeaf5e0c9753202f672f` (IPL 1.0 core + bundled, licenses/)
  - F — mjgil-rust/ntfs-recover @ `5072bbc729b293efc0b2070b38ad30c3b7e043cf` (0BSD)
  - G — hasithahaththella/mft-rescue @ `9b0e5ecc9d24ae9918f83e3a81e4a8616f610a22` (MIT)
- **Token handling**: stored in env/git config only; never committed. This prompt saved with the token redacted.

## Original session prompt (verbatim, token redacted)

```
Senior systems engineer (25+ years), filesystem internals, recovery engines, forensic tooling, Rust/C++ systems programming, production product delivery.

Harvest the strongest features, algorithms, heuristics, and UX behaviors from six mature open-source recovery projects and implement them in our product. This is not a clone of any one reference — it is a best-of-breed extraction: for each capability, take the reference that does it best, understand its algorithm, re-implement it from understanding in our stack, and validate it against seeded ground truth. Every feature is sourced, licensed-checked, tested, and proven on real Windows.

Repos:
- Product (target): https://github.com/heicfast/final-prod.git
- Reference A (mature, same stack, UI features): https://github.com/terraseraph/sectorforge.git
- Reference B (mature, same stack, UI features): https://github.com/htoklu/halim-recovery.git
- Reference C (partition & carving): https://github.com/cgsecurity/testdisk.git
- Reference D (forensic engine library): https://github.com/sleuthkit/sleuthkit.git
- Reference E (NTFS deep recovery, Rust): https://github.com/mjgil-rust/ntfs-recover.git
- Reference F (MFT analysis & rescue): https://github.com/hasithahaththella/mft-rescue.git
- Token (env/git config only, never committed): ghp_[REDACTED]

Read every repository end to end with parallel agents before proposing a single change. Every feature claim is a source citation. Every implementation is proven by a test and a captured validation artifact.

Phase 0 — Read all seven repositories end to end

Parallel agents (mandatory):

· A — Product (final-prod): every module, engine, parser, UI surface, workflow, installer, and test. → worklog/PRODUCT-STATE.md
· B — SectorForge: every algorithm, UI feature, output format, and recovery mode. Specifically extract: ddrescue-style two-pass imaging with bad-sector isolation, progress map persistence, partition detection, NTFS deleted-file recovery, 95+ file-type carving, direct hardware access, drive-inspector UX, and the reconstructed-files-with-directory-structure feature. → worklog/SECTORFORGE-FEATURES.md
· C — Halim Recovery: every algorithm, feature, output format, and UX behavior. → worklog/HALIM-FEATURES.md
· D — TestDisk: partition detection and recovery (MBR, GPT, extended, backup GPT, backup boot sectors, MFT mirror repair), deep-scan heuristics, and PhotoRec's signature registry, header/footer matching, fragmentation handling, and output naming. → worklog/TESTDISK-FEATURES.md
· E — Sleuth Kit: filesystem abstraction, partition-layer parsing (mmls), directory enumeration (fls, allocated and deleted), extent extraction (icat), unallocated-space extraction (blkls), inode listing, hash databases, timelines, image formats, and every supported filesystem's key structures. → worklog/SLEUTHKIT-FEATURES.md
· F — ntfs-recover: the MFT-corruption recovery path — brute-force scan for MFT records, directory-tree reconstruction, and $MFT data-run-list reconstruction and in-place patching. → worklog/NTFS-RECOVER-FEATURES.md
· G — mft-rescue: deleted-record detection, $LogFile replay, $UsnJrnl analysis, MFT slack-space carving, parent-reference and path reconstruction, and volume-level enumeration of live/deleted/orphaned entries. → worklog/MFT-RESCUE-FEATURES.md
· H — Cross-reference: consolidate A–G into a feature matrix: every feature, source file, behavior, prerequisite, product status, priority, and licensing note. → worklog/FEATURE-MATRIX.md

Nothing is implemented until the feature matrix is committed and pushed.

Phase 1 — Licensing and sourcing check

For every feature in the matrix, read the source repository's license. Any capability from an incompatible license is re-implemented from the documented algorithm (not copied) and the reference is cited for behavior. Record every decision in worklog/LICENSING.md.

Phase 2 — Implementation waves

Implement in priority order (P0 = core recovery capability, P1 = high-value, P2 = valuable, P3 = nice-to-have):

· Wave 1 — Foundations: shared data structures (recovered-entry node, tree model, deleted/live classification, provenance), parser primitives, abstract access layer.
· Wave 2 — NTFS deep recovery: combine ntfs-recover's MFT-corruption path, mft-rescue's journal/slack/parent reconstruction, and Sleuth Kit's structure parsing. Validate against seeded NTFS fixtures.
· Wave 3 — Partition layer: TestDisk's MBR/GPT/backup/boot-sector detection, deep-scan heuristics, lost-partition candidate scoring, and virtual-volume construction.
· Wave 4 — Carving: TestDisk/PhotoRec signature registry, SectorForge's 95+ type validators, header/footer matching, fragmentation handling, deduplication, and result merging.
· Wave 5 — Recovered-file tree: deleted-only reconstruction with original directory structure (the named feature), every edge case.
· Wave 6 — Imaging & drive health: SectorForge's ddrescue-style two-pass imaging with bad-sector isolation, pause/resume, progress map persistence, direct hardware access, and drive inspector UX.
· Wave 7 — UX and output: every screen, filter, view, and output format the references present, including the reconstructed-files option.
· Wave 8 — Persistence, session, performance: session save/load, checkpoint, resume, read sizing, concurrency, buffering, cancellation, memory bounds.
· Wave 9 — Hardening: every edge case, error path, unsupported-format path; fuzz smoke; sanitizers clean.

Each wave is committed and pushed as a coherent unit with tests and validation artifacts.

Phase 3 — Validation standard

· Seeded ground truth: a deterministic VHD/VHDX or disk image with a seed manifest recording every file's path, size, timestamps, attributes, and SHA-256 before deletion/damage.
· Real Windows: every user-visible surface exercised on a GitHub Windows runner or self-hosted Windows with real disks.
· Byte-for-byte comparison: recovered files compared against the seed manifest by SHA-256; recovered tree compared against the expected tree.
· Reference comparison: where the reference is runnable, the product's behavior is compared against it on the same fixture. Where not, the documented behavior is the standard, cited from source.
· Captured evidence: every "works" claim has an artifact in worklog/VALIDATION/.

Rules

· Harvest, do not clone. Best-of-breed extraction from six references, implemented in our stack and architecture.
· Re-implement from understanding. Never copy code verbatim. Cite the reference for behavior.
· Licensing checked. Every reference's license read and recorded. Incompatible licenses re-implemented, not copied.
· No behavior change beyond new features. Every existing product feature continues to work identically. New features are additive or opt-in.
· No stubs, TODOs, placeholders. Truthful disable with precise scope if incompletable.
· No masked failure. Every error propagates. No retry loop hiding failures.
· No hallucination. Reference = source citation. Product = test or captured artifact.
· No shortcuts. No compromises. Production grade only.
· Preserve audit trail.
· Token in env/git config only.
· Push after every coherent unit. Never force-push. main green.

Validation gates

1. Every P0 and P1 feature implemented, tested, and validated on real Windows. Evidence: worklog/VALIDATION/.
2. The named feature — reconstructed files with directory structure preserved, deleted only — matches the reference's behavior with every edge case handled.
3. Every existing product feature still works identically (regression tests).
4. Every P2/P3 feature either implemented or documented in worklog/FOLLOW-UP.md with precise scope.
5. Licensing decisions documented per feature.
6. Full Windows CI green.
7. Release candidate tagged, built, installed on clean Windows, and validated against the seeded fixture. Evidence: worklog/VALIDATION/RELEASE/.

Deliverables

1. Eight Phase 0 context sheets, pushed before any code.
2. worklog/FEATURE-MATRIX.md — master extraction with source, behavior, prerequisite, status, priority, licensing.
3. worklog/LICENSING.md — per-feature licensing decision.
4. Every P0/P1 feature implemented, tested, validated with evidence.
5. Every P2/P3 feature implemented or documented in worklog/FOLLOW-UP.md with precise scope.
6. worklog/PARITY.md — final feature matrix with every item resolved to IMPLEMENTED, DELIBERATELY-DIVERGENT, or NOT-APPLICABLE.
7. Full Windows CI green.
8. Release candidate validated on clean Windows.
9. worklog.md — chronological handoff with every SHA, every CI link, every validation artifact.
10. All pushed with reference-citing messages.

TODOs (spawn 20+, complete 19, spawn 20 more)

· T1: Clone all seven repos, record commits, save prompt (redacted), worklog entry.
· T2: Spawn agents A–G, read every repo end to end, produce seven context sheets, push.
· T3: Consolidate worklog/FEATURE-MATRIX.md, push.
· T4: Licensing check per feature, push worklog/LICENSING.md.
· T5: Establish seeded validation fixture with manifest, push.
· T6: Wave 1 — foundations (data structures, primitives, access layer), push.
· T7: Wave 2 — NTFS deep recovery (ntfs-recover + mft-rescue + Sleuth Kit), validate, push.
· T8: Wave 3 — partition layer (TestDisk), validate, push.
· T9: Wave 4 — carving (PhotoRec + SectorForge), validate, push.
· T10: Wave 5 — recovered-file tree and deleted-only reconstruction, validate against reference, push.
· T11: Wave 6 — imaging, bad-sector isolation, drive inspector, push.
· T12: Wave 7 — UX and output surfaces, push.
· T13: Wave 8 — persistence, session, performance, push.
· T14: Wave 9 — hardening, edge cases, fuzz smoke, sanitizers, push.
· T15: Full Windows CI sweep, fix reds, push.
· T16: Regression sweep — existing features still work, push.
· T17: Release candidate on clean Windows, exercise every new feature, capture evidence, push.
· T18: Update docs to current reality, push.
· T19: Final verification — every gate proven, push.
· T20: Handoff — complete worklog.md, final push.

Final reminder

· Harvest, don't clone. The strongest capability from each reference, implemented in our stack.
· The named feature is priority. Reconstructed files with directory structure preserved, deleted only, as a post-scan option — matching SectorForge's behavior exactly.
· The feature matrix is the roadmap. Every feature sourced, cited, prioritized, and tracked from extraction to validation.
· Licensing is checked. No code copied. Algorithms re-implemented; references cited.
· Validation is real. Seeded ground truth. Real Windows. Byte-for-byte comparison. Captured evidence.
· No stubs. No fake results. No hallucination. No shortcuts. Production grade only.
· Work long-horizon. Commit and push often. main green at every push.
· Save to worklog/session-cross-repo-harvest-prompt.md (token redacted), read twice, start T1.

Good luck.
```

## Session framing notes (added at T1, not part of the original prompt)

- The product already contains substantial recovery capability built through prior sessions (nine FS engines, lost-partition recovery, grammar-aware carving, .crsf sessions, SafeWriter, SMART/NVMe health, BitLocker unlock, imaging; see `README.md` and `engine/docs/CAPABILITY-MATRIX.md`). The harvest therefore lands as **gaps + upgrades + edge cases** against the references, not as a from-scratch build; the feature matrix's "product status" column records what exists at `201100aa` before any change.
- Environment truth for this session: the executing sandbox is Linux (no Windows host locally). Windows proof comes from the repo's GitHub Actions Windows lanes (engine-ci, bridge-integration, ui-ci, ui-e2e, release, benchmark-src, forensic-repro, validation-evidence) — run links captured in `worklog/VALIDATION/`. Anything not provable on a Windows runner this session is recorded as open with precise scope, never claimed.
- The prior session (`worklog/session-ci-optimization-prompt.md`) left main at `201100aa` with all lanes green after `7d196cda`/`a19afb56`; baseline CI state for this session is recorded below in the worklog entry.
