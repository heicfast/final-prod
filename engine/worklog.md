# core_recovery worklog

Append-only. One entry per work phase; messages carry full context.

---
Task ID: 1
Task: Repository bootstrap; requirements + ground-truth contracts locked from RE corpus.

- Corpus studied: easeus-drw-research (architecture 04/05, SearchDLL artifacts, MOM artifacts,
  SaveScan rsf reversal, raw_signs.json, bitlockerlib; rda-driver wire contract rda_ioctl.h).
- Requirements locked: docs/requirements.md (R1..R8 + non-functional).
- Ground-truth constants locked: docs/ground-truth-contracts.md (10 sections, evidence-cited).
- Reference defects catalogued: docs/error-fix-registry.md (F1..F15).
- Build system: CMake 3.25 + CMakePresets; vcpkg manifest (zlib, gtest); MSVC /W4 /WX;
  gcc -Werror + ASan on pure core; CI matrix (lint / linux / windows / codeql).
