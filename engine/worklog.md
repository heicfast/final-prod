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

---
Task ID: 2
Task: Pure-core foundation: types/errors, device/slice/cached/devmap, MBR+EBR+GPT table parse, NTBS wire structs (spec-exact), USA fixup, runlist codec, bootstrap ($MFT+$MFTMirr+$ATTRIBUTE_LIST fragmented path), result tree (dual-class nodes).

- First build+test loop on Linux (gcc 14.2, -Werror + ASan): 29/29 tests green.
- Real defects caught by the loop (impl side): fixup compared per-sector USA slots instead
  of the single USN slot0; DevMap merge branch returned before absorb pass; FullNode
  destruction skipped the role u16string (ASan leak); NonresidentHeader/SI/IndexEntry/
  BootSector/GptEntry layouts corrected to spec-exact (static_asserts green).
- Test-side authoring errors corrected (runlist LE byte arrays + terminators, GPT header
  field offsets, devmap overlap math, 4Kn slot mapping).
- CMake: ASan compile+link options on both lib and tests.

Stage Summary: pure core foundation locked; ready for NTFS attribute/walker/bitmap engines.
