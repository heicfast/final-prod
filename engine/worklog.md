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

---
Task ID: 3
Task: NTFS engine core complete + first end-to-end integration test.
- cr_attributes (enum + typed extract + attrlist entries), cr_bitmap ($Bitmap
  loader + reused_ratio), cr_mftwalk (namespace ranking, hard links with
  PER-LINK names via props clone, ADS children, viability grading F5, phase-B
  orphan reparent incl. root self-parent, remnant scanner).
- Real defects caught by the ASan/test loop: parse_file_name had
  name_length/namespace swapped (d[64]/d[65]); mirror loader parsed record-0
  copy instead of $MFTMirr record-1 copy; hard-link double-release of the
  ctor ref (add_file contract switched to reference semantics: +1 per call,
  caller releases once); FullNode role-string leak (dtor dispatch).
- Fixture: 256-cluster synthetic volume, 12 records, $MFTMirr, $Bitmap
  in-use bits engineered for Excellent/Overwritten/PartialRisk grading;
  floating-record remnant scan test.
- 31/31 tests green (gcc + ASan).
Stage Summary: R2.1-R2.5 core paths all implemented and integration-tested. Next: lost-partition candidates, carving, FAT/exFAT, sessions, orchestrator, writer, BitLocker layer, then Windows platform + CI bring-up.

---
Task ID: 4
Task: FAT12/16/32 engine complete + integration tests.
- FatTable: flavor-polymorphic entries (12/16/32), mirrored FAT2 with
  choose-healthy + divergence count, canonical BAD/EOC constants (corpus).
- follow_chain + reconstruct_deleted_chain with visited-set cycle guards.
- LFN assembly (checksum-validated, 8.3 fallback), 8.3 conversion, FAT
  date/time -> unix (civil algorithm).
- scan_fat: root area (12/16) vs root cluster chain (32); dentry walk with
  0x00 end / 0xE5 deleted / volume-id skip / '.'-'..' skip; deleted-chain
  reconstruction with honest confidence grading (FREE-first-cluster cap);
  backup-boot fallback; dir recursion with cycle guard.
- Defects caught: FAT32 fixture wrote FAT16-position fields (0x26 sig
  clobbering spf32@0x24-27); FAT 0x00 end-marker semantics (dir ends at first
  0x00 - full-cluster fixture); LFN checksum; time decode (0x5A46=2025-02-06).
- 36/36 tests green.

---
Task ID: 5
Task: exFAT engine complete + integration tests.
- Wire structs rewritten SPEC-EXACT after reference verification (elm-chan/
  SDCFF): SetCheckSum@2, packed u32 timestamps, tz 7-bit two's complement
  quarter-hours (0x00=unused), SecondaryCount, NoFatChain bit1.
- Boot-region checksum CORRECTED: value is FILL-REPEATED in sectors 11/23
  (NOT +508 of every sector - that variant clobbers 0x55AA); computed over
  sectors 0..10 with VolumeFlags+PercentInUse of the boot sector excluded.
  Fixed a nonsense heap-after-FAT check (was fat_length*shift - now sectors).
- Engine: entry sets (0x85+0xC0+0xC1s), deleted = InUse 0x80 cleared on all
  members, NoFatChain contiguous resolution, FAT chains, allocation-bitmap
  reused-ratio grading, entry-set 16-bit checksum validation (damaged sets
  kept), dir recursion with parent threading + cycle guard.
- Fixture defects caught: root-cluster 16-entry overflow (chain extended
  2->3), volume_length raised to spec 1MB minimum.
- 38/38 tests green.

---
Task ID: 6
Task: Lost-partition discovery (R3) complete.
- discover(): 64KiB-window stride scan for 55AA + OEM classification
  (NTFS/FAT/exFAT/GPT), format probes with volume-relative->absolute offset
  math, $MFT 'FILE'+fixup probe (strong NTFS evidence), BPB validation,
  dedup (nested same-fs), overlap scoring vs live table, score-desc sort.
- materialize(): bounded virtual read-only slices (R3.2 - never repairs).
- Test: planted lost NTFS (mft probe) + exFAT + incoherent 55AA noise;
  overlap classification matrix.
- 40/40 tests green.

---
Task ID: 7
Task: Carving engine (R4) complete.
- SHA-256 (FIPS 180-4, NIST vectors verified) for hashing/dedup/writer.
- Signature registry v1: 16 formats, VERSIONED table, F8 load-time gate
  (validator mandatory, bounds checked, id-order enforced).
- Grammar validators: JPEG marker walk (SOF/SOS/EOI), PNG chunk walk
  (IHDR dims + IEND), TIFF IFD walk, CR2 (CR submagic), WebP RIFF, HEIC/MP4/
  MOV ISO-BMFF box walks, PDF %%EOF, ZIP/DOCX EOCD + cd-geometry
  (fragmentation flag R4.5), 7z StartHeader, SQLite page math, AVI RIFF+hdrl,
  MKV EBML vint element walk, RAR4 block walk to END.
- Pipeline: 1MiB windows + 4KiB overlap, first-byte dispatch, specificity
  trial order (DOCX-before-ZIP), F9 extent dedup vs structured + prior
  carved, 128-bit content ids for files <= 8MiB, host namer callback
  (corpus: naming lives in the host).
- Real defects caught by the loop: PNG IHDR length read LE instead of BE;
  fixture noise overlapping the JPEG tail (validated via instrumentation).
- 46/46 tests green.

---
Task ID: 8
Task: Session container CRSF (R6.3/R6.6) complete.
- Chunked-zlib framing (reference-proven scheme): u16 header bit15=deflate,
  bits0-11=payload-1, <=4KiB, level-5, stored-when-larger; per-chunk inflate
  fallback to raw (F10 graceful degradation).
- Our additions (F10): CRC32 per record + skip-forward indexing on corrupt
  headers (resumable by construction); versioned container + record magics.
- Typed payloads: tree DFS grammar (parent ids, light/full markers, full
  props field map, hashes), error map, volume info, session state,
  partition context. Deserializer: lazy synthetic root for top-level nodes.
- 49/49 tests green (round-trip, corruption skip, tree round-trip).

---
Task ID: 9
Task: Orchestrator (R6) + safe recovery writer (R7) complete.
- ScanController: quick -> deep -> carve staged pipeline; weighted MONOTONIC
  progress (exact single-division math); family probe (OEM + BPB heuristic);
  per-stage NTFS/FAT/exFAT engine dispatch; remnant sweep in deep;
  lost-partition discovery in whole-disk mode (per-candidate virtual slices
  + engines under per-candidate roots); session save/load (5 record types).
- SafeWriter: enqueue with hard dest-overlap refusal (F12: extent-vs-dest
  intersection, code-level), collision naming "name (N).ext", corpus
  sanitizer, atomic temp+commit via DestIo abstraction, sha256 verification
  with per-item mismatch accounting, zero-fill Skip policy, cancel support.
- 54/54 tests green (incl. session round-trip through the orchestrator).

---
Task ID: 10
Task: CI matrix GREEN + full docs + release pipeline.
- CI on HEAD: lint + linux (gcc+ASan, 54 tests) + windows (MSVC /W4 /WX,
  full stack incl. win layer + rrecov) + codeql - ALL SUCCESS.
- MSVC-specific defects fixed through the CI loop: C4244 narrowings
  (data_sectors u64->u32; carve_confidence u32 field; voff; test masks),
  C4477 wprintf %s/%hs + wide literals, BusTypeMmc spelling, cr_error
  includes for bitlocker, destio <map>, rrecov <functional>.
- vcpkg via the runner-bundled installation (no bootstrap action).
- Docs set completed: architecture, crsf-format, build, testing, roadmap,
  benchmarking.
- Release pipeline: tag-triggered; builds + tests + stages rrecov.exe,
  docs, pdb, SHA256SUMS; gh-release upload.

---
Task ID: 11 (final)
Task: v0.1.0 released.

- Release pipeline green on the v0.1.0 tag: rrecov.exe + pdb + docs +
  SHA256SUMS published as rrecov-v0.1.0.zip (109,355 B).
- Final state: 22 commits, 102 tracked files; CI matrix green on HEAD
  (lint, linux+ASan 54/54, windows MSVC /W4 /WX full stack, codeql).
- rda-driver synced (benchmark parity work landed there; no wire-contract
  changes - our transport layer unaffected).

Stage Summary: core_recovery v0.1.0 - the complete base recovery engine
layer: R1 device/partition, R2 NTFS, R3 lost partitions, R4 carving,
R5 FAT/exFAT, R6 orchestration+sessions, R7 safe writer, R8 BitLocker-aware.
Wizard UI + extended formats + benchmarks follow per docs/roadmap.md.
