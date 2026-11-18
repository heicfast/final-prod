# LICENSING — per-feature sourcing decisions for the cross-repo harvest

> Task T4. Date: 2026-11-19. Applies to `worklog/FEATURE-MATRIX.md` rows. Product license: MIT (`LICENSE`, heicfast/final-prod contributors). Every reference license below was read directly from the pinned clone (file + SHA recorded); decisions follow the session mandate: "Harvest, do not clone… Re-implement from understanding. Never copy code verbatim. Cite the reference for behavior. Any capability from an incompatible license is re-implemented from the documented algorithm (not copied)."

## 1. Reference license inventory (verified at pinned HEADs)

| Ref | Repo @ HEAD | License | Evidence | Compatible with direct code copying into MIT product? |
|---|---|---|---|---|
| SF | terraseraph/sectorforge @ b292a299 | **MIT** (2026 terra seraph) | `LICENSE:1-3`; no third-party/vendored code (SECTORFORGE-FEATURES §0, §5) | Yes — but session rule forbids verbatim copying regardless; behavioral re-implementation with citation |
| HR | htoklu/halim-recovery @ 3b5d66d8 | **MIT** (2026 Halim Toklu) | `LICENSE:1-3`; deps System.Management + CommunityToolkit.Mvvm both MIT (HALIM §0) | Same as above |
| TD | cgsecurity/testdisk @ 02941e1b | **GPLv2+** | `COPYING` (GPL v2, "or any later version" grant in every header, e.g. `src/photorec.c:7-9`) | **No** — GPL code never enters an MIT product; behavioral re-implementation only |
| SK | sleuthkit/sleuthkit @ 3dcb4366 | **IPL 1.0 core** (tsk/, tools/) + bundled: sqlite3.c PD, lzvn BSD-3, guid.cpp MIT, rejistry++/java Apache-2.0, srch_strings GPLv2, md5c RSA terms, tsk_unicode Unicode Inc. terms | `licenses/README.md`, `licenses/IBM-LICENSE` (SLEUTHKIT §0, §10) | **No for IPL core** — behavioral re-implementation only; bundled GPL/RSA/Unicode files are never copy sources under any policy |
| NR | mjgil-rust/ntfs-recover @ 5072bbc7 | **0BSD** (BSD Zero Clause) | `LICENSE` ("add license" commit 5072bbc7) | Yes (most permissive) — but session rule: still re-implement, cite |
| MR | hasithahathella/mft-rescue @ 9b0e5ec | **MIT** (2026 Hasitha Haththella) | `LICENSE:1-3` | Same as SF |

## 2. Standing policy (applies to ALL rows regardless of license)

1. **No verbatim code** from any reference enters the product — including MIT/0BSD references. All algorithms are re-implemented from the behavioral descriptions in the context sheets, in the product's C++20 architecture and conventions.
2. **No verbatim struct layouts, comments, identifier names, log strings, or UI copy** from TD (GPLv2+) or SK (IPL 1.0). On-disk format facts (magic numbers, field offsets, structures defined by public filesystem specifications — 0xAA55, "NTFS", "EFI PART", MFT record layout, FAT backup-BS at sector 6, ext 0xEF53@+1024, JPEG/PNG/ZIP signatures, ddrescue mapfile status characters as a format contract) are unprotectable facts and freely used; each such use cites the sheet, which cites the reference file.
3. **Creative thresholds are re-derived, not translated**: e.g. PhotoRec's 10-header stop, 500-files-per-dir rotation, 5000-block brute-force skip bound, ≤5-fragment recursion cap, 32-logical EBR cap; SectorForge's 5-second mapfile cadence, ` (id N)` collision format, `_orphaned/` naming, 240-unit UTF-16 truncation. Where a constant is a behavior-visible contract (ddrescue mapfile interop format), the format is followed as a specification and cited; where it is an internal tuning choice, we pick our own value informed by the reference's documented behavior.
4. **Citations**: every harvested behavior cites its context sheet section (which carries reference file:line), in code docblocks, tests, and worklog entries — preserving the audit trail without copying expression.
5. The product remains MIT; no additional attribution obligation is created by behavioral re-implementation, but the worklog/FEATURE-MATRIX traceability is maintained as good practice.

## 3. Per-feature decisions (by FEATURE-MATRIX row)

| Matrix rows | Source ref(s) | Source license | Decision |
|---|---|---|---|
| F-00d, F-01, F-31, F-36, F-37, F-45 (named feature + imaging UX + deleted-only listing) | SF | MIT | RE-IMPLEMENT from sheet §2.1-2.3/§3 behavior; cite SectorForge in docblocks. No code copying (session rule stricter than MIT requires). |
| F-33, F-34, F-35, F-38 (two-pass imager, mapfile, resume, honest-partial) | SF (+ MR superseded) | MIT (+ MR MIT) | RE-IMPLEMENT (ddrescue mapfile text format followed as an interop specification — GNU ddrescue's mapfile format itself is a de-facto format contract, and SectorForge documents its compatibility with ddrescueview; cite both). |
| F-39 direct-access modes | SF | MIT | RE-IMPLEMENT behavior (SCSI READ(16)/ATA DMA EXT usage patterns are Win32/API facts). |
| F-28, F-29 (FAT/exFAT orphan harvesting, bitmap authority) | HR (+ SF) | MIT | RE-IMPLEMENT; the "children of a deleted tree keep their entries" invariant is an OS/filesystem fact; Halim's weighting and notes re-derived. |
| F-44 evidence scoring | HR | MIT | RE-IMPLEMENT the evidence model (55/35/10 weights re-derived as our own tuning informed by the sheet); HealthNotes-style strings written fresh. |
| F-47 NLQ filter | HR | MIT | RE-IMPLEMENT if scheduled (P3); TR-specific stop-word lists written fresh, not copied. |
| F-51 column store | SF | MIT | RE-IMPLEMENT concept only if verification shows a gap. |
| F-02, F-07, F-08 ($MFT delta-grouping patch, numbering, dedup) | NR | 0BSD | RE-IMPLEMENT (0BSD would even allow copying; session rule still forbids). The ntfs-3g sign-extension lesson is a filesystem-driver behavior fact — cite NR's DEBUGGING finding. |
| F-03, F-04, F-05, F-56 (parent-seq, zero-sector fixup, attr-list ownership, FAT copy fallback) | SF | MIT | RE-IMPLEMENT as verification+fix rows against existing product code. |
| F-12, F-13, F-14, F-15, F-16, F-17, F-24, F-25 (partition lattice, GPT repair, MFT-mirror adjudication, EBR caps, Mac/BSD/Sun, rename, forget) | TD | **GPLv2+** | RE-IMPLEMENT FROM DOCUMENTED BEHAVIOR ONLY. No code, no tables (the ~90-row i386 type-name table and GUID→name table are re-derived from public specs/our own naming), no threshold strings, no log text. Candidate-acceptance semantics (is_part_known + size>1 + bounds) re-expressed in our candidate model. |
| F-18, F-19, F-20, F-21 (registry breadth, fragmentation, index, blocksize) | TD | **GPLv2+** | Signature magic constants = format facts (free); registry organization, bucket index structure, pass machine, and ALL thresholds re-implemented from the sheet's behavioral description. Our registry stays fail-closed validator-gated (F8) — a product invariant, not PhotoRec's design. |
| F-06, F-30, F-48, F-49, F-50, F-57, F-58 (index-slack names, ext dentry slack, mmls presentation, timelines, hashdb, encryption errors, caps) | SK | **IPL 1.0 core** | RE-IMPLEMENT FROM DOCUMENTED BEHAVIOR ONLY. No struct field order copying, no comments, no output-format strings verbatim (the fls/mmls output grammars are documented in prose in the sheet; any similar output is re-expressed). fiwalk/sqlite are PD but not needed. srch_strings (GPL) and md5c/tsk_unicode never consulted as copy sources. |
| F-40 (E01/AFF/split-raw reading) | SK | IPL 1.0 | RE-IMPLEMENT from public format specifications (EWF/AFF format docs are public; Libewf is the canonical implementation and is NOT consulted beyond format facts). Split-raw naming matrix re-derived from common conventions. |
| F-22, F-23 (structural end-detectors, honesty) | SF + HR | MIT | RE-IMPLEMENT validators; product already has its own grammar layer — these are targeted additions in product style. |
| F-42, F-41, F-54, F-55 | SF / ours | MIT / ours | Verification/polish rows in product's own code. |
| F-09, F-11, F-43, F-26, F-27, F-32, F-52, F-53 | — | — | No code involved (decision register rows). |

## 4. Boundary cases recorded

- **ddrescue mapfile format** (F-34): treated as an interop format (like a file format), not GPL expression — GNU ddrescue's documentation describes the format; SectorForge (MIT) independently implemented it and documents compatibility. We follow the format contract and cite both; our parser/serializer is written from the format description in SECTORFORGE-FEATURES §2.2.
- **PhotoRec signature constants** (F-18): magic byte sequences are facts of file formats (many predate PhotoRec). The registry TABLE (descriptions, categories, defaults) is re-derived: our FormatId/kTable conventions stay product-native; new rows added only where the product lacks the type, with product-written validators (fail-closed, F8).
- **NTFS/filesystem structures**: all structures (MFT records, fixup, runlists, BPB, GPT headers, FAT BPB, exFAT entry sets, ext superblocks/inodes/dentries) are specified by Microsoft/ECMA/public filesystem specifications; references are cited for observed BEHAVIOR (edge cases, validation gates), not for structure definitions.
- **Sleuth Kit IPL nuance**: IPL is OSI-approved but GPL-incompatible-ish for code; we take zero code, zero headers. The behavioral descriptions in our sheet were produced by reading (Agent E) and are re-implemented by engineers working from that prose — the same discipline the sheet itself prescribes (SLEUTHKIT §10: "reproduce interface facts from prose descriptions like this sheet").
- **Not legal advice**: this file records engineering sourcing discipline per the session mandate; final legal review of third-party reuse remains the maintainers' responsibility.

## 5. Verification hook

CI-adjacent check added to the session workflow: every harvested-feature commit message and docblock cites its sheet section; worklog/PARITY.md (final deliverable) must resolve every matrix row to IMPLEMENTED / DELIBERATELY-DIVERGENT / NOT-APPLICABLE with its licensing decision from this file.
