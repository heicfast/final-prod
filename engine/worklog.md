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

---
Task ID: S4-FINAL
Agent: main
Task: Session 4 continuation - CI green + spec-critical runlist fix

Work Log:
- CI fully GREEN on 9c1d229: lint + linux + windows + fuzz + codeql (first complete-green run; the chain of fixes: cr_health MSVC rewrite, windows.h + health namespace in rrecov, TRIM %hs, fuzz harness OOB gate, PRNG overflow in bench_seed)
- CRITICAL SPEC FIX (9c1d229): the NTFS runlist header nibble order was INVERTED (len=high nibble, inherited from a misread corpus artifact; the decompiled size-walk is order-agnostic). Ground truth = Linux fs/ntfs3/run.c: LOW nibble = length width, HIGH = offset width. With the old order every REAL NTFS volume would misparse; synthetic fixtures never caught it (self-round-trip). Fixtures rewritten to spec order; encode moved to u64-space deltas (also fixes two real UB findings from the first fuzz campaign: shift-by-64 in width_of, i64 delta overflow).
- Fuzz campaigns: harness OOB (fixed), runlist UB x2 (fixed), timeout guards added (25s/input, 2560MiB rss)
- Benchmark workflow: PRNG int64 overflow fixed (System.Random); two runs in flight reached the scan step (seed + driver install working)
- Security audit T16: all P0/P1/P2 findings fixed (see 6bde909)
- Docs: roadmap/architecture/requirements updated for v0.2; ground-truth-contracts.md nibble correction with kernel citation

Stage Summary:
- v0.2.0 ready to tag: 106/106 tests (ASan), full engine set, full CI green, fuzz-clean (post-fix campaign), benchmark + comparison workflows live

---
Task ID: S5-START
Agent: main
Task: Session 5 kickoff - verification & reconciliation (audit + fix everything)

Work Log:
- Context recovered: all 3 repos cloned fresh, git state verified (core_recovery @ e53ba13,
  rda-driver @ 2b1db5f, easeus-drw-research @ 18d6f14)
- Session prompt saved verbatim (token redacted) to worklog/session-05-prompt.md
- Mandate: full audit of every module against the RE corpus, then production-grade fixes.
  Scope correction required: ReFS + LDM are IN scope (docs currently defer them - wrong).
- Parallel line-by-line reading agents launched (A: corpus, B: core_recovery, C: rda-driver),
  then cross-reference matrix, then AUDIT.md, then fixes.

Stage Summary:
- Session 5 opened. AUDIT.md is the gate: no fixes until it is on main.

---
Task ID: S5-LDM
Agent: main (session 5 continuation, post-workspace-reset)
Task: Full LDM implementation - the audit's largest single rewrite (AUDIT.md row 30), lost to a workspace reset mid-session and rebuilt from the verified ground truth.

Work Log:
- Workspace was reset between sessions; recovered state from GitHub (5 session-5 commits
  intact: prompt, scope docs, AUDIT.md, btrfs/ext4 fixes, ZFS rewrite). The in-flight LDM
  work was lost; the Linux kernel ldm.c/h reference (42 KB) survived in scripts/ and was
  re-read line by line.
- Ground truth re-verified from three independent sources before writing a line:
  * Linux kernel block/partitions/ldm.{c,h} (deployed MBR parser): record bases
    0x18/0x1D/0x2D/0x2E with fixed gaps, VBLK_SIZE_* length checks, VMDB array geometry
    (vblk_offset start, vblk_size*vblk_last bound), TOCBLOCK copies at config+1/2/2045/2046,
    PRIVHEAD at LBA 6 + config+1856/+2047, version gates (2.11/2.12, 4.10).
  * flatcap LDM technical docs (linux-ntfs): per-type field lists/names, variable-width
    encodings, "Stripe Size LBA" = sectors, update-status semantics, GUID text slots.
  * libldm (mdbooth/ldmtool, fetched and read): GPT dynamic-disk discovery (PRIVHEAD in the
    LAST LBA of the LDM metadata partition), full VMDB field map (committed_seq@0x75, group
    name@0x16/guid@0x35 - the old "disk_group_id@0x75" was actually committed_seq), composite
    reassembly as payload concatenation, volume-kind rules, RAID-5 layout "raid5_ls" (= md
    left-symmetric), chunk size in 512-byte sectors.
- Discrepancies between sources resolved and documented in the header: the kernel's cmp3
  "chunksize" reads the first vnum after the post-parent-id pad (dead code there, but the
  position is real); flatcap's field lists are off by one in two gaps vs the deployed kernel
  geometry; the kernel's raw-byte partnum read aliases a 1-byte vnum. Where the kernel and
  libldm agree on geometry, they are followed exactly; per-type length checks mirror the
  kernel (== for cmp3/dsk3/dgr3/dgr4, floor for prt3/vol5; dsk4 takes the floor as a
  documented divergence - the kernel's == only holds for empty device names).
- cr_ldm.{h,cpp} rewritten from 85 recognition-only lines to a full implementation:
  PRIVHEAD (3 copies, cross-checked, backup fallback when the primary is damaged),
  TOCBLOCK (4 copies), VMDB, the complete VBLK walk (standalone + composite reassembly with
  duplicate/incomplete-group accounting), all 7 record types, the object graph, and volume
  layout resolution (simple/spanned/striped/mirrored/RAID-5 with capacities and
  missing-member counts). Damaged records are counted in LdmStats and skipped - never
  silently, never aborting the walk (kernel/libldm abort; a recovery scanner must not).
- LdmVolumeDevice: read-only virtual volume with 512-byte logical geometry, sector-granular
  run batching (max 2 staged partial sectors per request), mirror copy failover, striped
  interleave, RAID-5 left-symmetric translation P(row)=N-1-(row%N) with XOR reconstruction
  of a missing/unreadable data column and honest failure when two columns are gone.
- GPT LDM type GUIDs fixed in cr_table_wire.h: the data GUID was a fabricated
  3B8F0848-... value; now the Microsoft PARTITION_LDM_*_GUID byte-for-byte (metadata
  5808C8AA-..., data AF9B60A0-..., cross-checked kernel+libldm).
- Orchestrator wiring: run_engine_pass detects dynamic disks on Unknown-family slices
  (one 512-byte probe rejects non-LDM disks), parses the database, and scans every
  reconstructed volume through scan_candidate under per-volume "Dynamic Volume <name>"
  roots. scan_candidate extended to report deleted counts (all engines).
- Tests: 12 new Ldm.* tests (database/graph parse, simple/spanned/striped/mirrored/RAID-5
  resolution and reads, composite VBLK spanning two slots, missing-column reconstruction
  with real parity, double-failure, mirror failover with a hole device, non-LDM rejection)
  + the end-to-end Orchestrator.DynamicDiskVolumeScan (FAT16 inside an LDM simple volume,
  files + deleted found through the virtual device). The two recognition-era tests that
  asserted fabricated field placements were removed from test_refs_ldm.cpp. Shared fixtures
  extracted to test_fixtures.hpp. Fuzz: engines harness extended with cases 14/15 (LDM
  parse; parse + volume-device reads incl. reconstruction).
- Real defects caught by the loop: per-type parsers took the cursor by value (consumed
  always 0 - the floor checks passed vacuously, the equality checks failed); LdmGuid::
  to_string indexed b[] with text positions; the composite emitter passed a full slot as
  the view (payload shifted 16 bytes); the first RAID-5 fixture had overlapping stripe
  members and no real parity (the reconstruction math was right; the fixture was wrong).

Stage Summary:
- 124/124 tests green on gcc+ASan, clang-format clean, no stubs. LDM is now full-depth:
  both MBR and GPT dynamic disks, all five volume kinds, reconstruction. AUDIT.md row 30
  closed; rows 31+ (NTFS wave, wiring/semantic wave, carving wave) remain.
Task ID: S6-START
Agent: main
Task: Session 6 - context recovery + P1-12 + benchmark split (user direction)

Work Log:
- Context recovered: all 3 repos re-cloned (workspace reset again);
  core_recovery @ 8084cbb had 3 red CI runs from the parallel engine-truth
  wave (lint drift + windows 0xc0000135). Parallel agents (audit + fix)
  confirmed active: their AUDIT.md (60 rows, 4-wave fix plan) + Btrfs/ext4
  + ZFS rewrites are on main; LDM/NTFS/UsnJrnl/carve-depth waves still open.
- CI repaired (35876b3): root cause of 0xc0000135 = bare find_library(NAMES
  lz4) resolves the RELEASE import lib even in Debug -> imports liblz4.dll
  which the debug bin dir cannot satisfy; vcpkg CONFIG packages
  (liblz4::liblz4 / zstd::libzstd) now preferred (per-config import names,
  applocal deploys correctly). clang-format-18 applied tree-wide.
- P1-12 FIXED (af4bc8e) - the 35-min/hours scan stall root cause: carve
  validators' forward rescans stepped in 0.5-1 KiB reads through the
  UNCACHED slice (ZIP EOCD: up to 2,097,152 read_at/hit, no early exit;
  PDF %%EOF: 524,288; JPEG SOS: 196,608). All now 1 MiB block scans
  (1024x fewer syscalls, full scan distance kept, seam overlap,
  per-call thread-safe buffers). ZIP: first-geometry-consistent-EOCD-wins
  replaces last-wins (cross-file extent corruption - the carve fixture
  passed ON the bug; fixture corrected to 140 B own-extent). MP3: strict
  frame walk (AUDIT Wave-3; fabricated 32 MiB ceiling gone; min 2 frames;
  v2.3/2.4 only; extents clamped to slice end). 114/114 tests green.
- bench_score.py crash fixed: manifest loop now guarded (the
  compare_report.py pattern); single malformed rows skip instead of
  killing the scoring run.
- Benchmark division (user direction - "separate for ours and separate
  for easeus but same methodology and environment"): comparison.yml split
  into comparison-ours.yml + comparison-easeus.yml. Both legs: same
  windows-2022 env, same 8 GiB/GPT/5 GB NTFS layout, same bench_seed.ps1
  DEFAULT parameters (identical manifests by construction), both drivers
  side-by-side (non-fatal on the leg that does not need them), single
  scoring truth (compare_report.py). The easeus leg fetches the latest
  comparison-ours artifact and emits the combined report (graceful
  degradation when absent). benchmark.yml: two per-volume capped scans
  (attribution unambiguous - both volumes share identical relpaths) +
  exFAT scoring added (was uploaded-but-never-scored dead weight).

Stage Summary:
- P1-12 done; benchmark stack restructured; CI unblocked for the parallel
  agents. Open: LDM full, NTFS attrlist/UsnJrnl/FILE0, carve depth
  (raw_signs layer), CachedReader wiring (#40) - parallel agents' waves.

---
Task ID: S5-NTFS
Agent: main (session 5 continuation)
Task: NTFS wave - audit rows 3, 6, 7 (attrlist depth, resident ADS, ADS attach, FILE0 salvage, UsnJrnl harvest).

Work Log:
- Bootstrap (row 3): nonresident $ATTRIBUTE_LIST on $MFT no longer returns
  ERR_UNSUPPORTED - runlist decode + bounded stream read (1 MiB cap, sparse runs
  zero-filled), feeding the same fragmented-MFT reassembly as the resident path
  (og FUN_18001a540 parity).
- Latent bug found while testing that path: max_record_no() gated on the primary
  $DATA run only, so fragmented-MFT extension records were UNREACHABLE (the F6
  chase assembled extents nothing could read). The gate now covers extent runs.
- Walker (row 6a): the $ATTRIBUTE_LIST chase is breadth-first multi-level
  (FRAG_DEPTH_LIMIT 64, cycle-guarded) - child records carrying their own
  $ATTRIBUTE_LIST queue their entries for the next level.
- Walker (row 6b): resident ADS content is stored in props->resident_data
  (bounded by the record) so the writer emits real bytes instead of a size.
- Walker (row 6c): ADS nodes attach to the canonical (first-emitted) hard link,
  not emitted.back().
- Walker (row 7): $UsnJrnl harvest - the $-skip branch captures $UsnJrnl's $J
  runs during the walk; phase C reads the stream (64 MiB cap), parses
  USN_RECORD_V2 (winioctl layout: rlen@0, major@4, fileref@8, parent@0x10,
  timestamp@0x20, reason@0x28, namelen@0x38, nameoff@0x3C), keeps FILE_DELETE
  evidence, dedups against structured (parent,name) pairs and within the
  journal, and emits the missed names under root_lost (MetadataOnly, Deleted).
- Remnant scanner (row 6d): og shifted-header salvage implemented per the
  corpus decompile (NTFSSearchDLL FUN_18002a780): when a 'FILE' candidate
  fails to yield a record, probe "FILE"+0x30 at +0x58 then +0x60 and parse the
  record from there. Improvement over the og: a full record is read when the
  block buffer reaches past the shift (the og's truncation was a buffer
  artifact); at the buffer edge we keep the og's truncated form.
- NEW BUG the new tests exposed (pre-existing, not in the audit): heap
  use-after-free in the ADS block - for multi-link records both links use
  cloned props, so the builder's release() freed the original before
  sp->times = props->times ran. ASan caught it; the ADS emission now happens
  before the release.
- Tests: 6 new NtfsVolume.* tests (nonresident attrlist bootstrap + extension
  read, nested 2-level chase with merged extents, resident ADS byte-exact
  content, ADS-under-first-link, UsnJrnl harvest incl. kept-vs-gone filtering,
  shifted-header salvage). RecBuilder gained attrlist/vcn-fragment/named-$J
  emitters.

Stage Summary:
- 131/131 tests green on gcc+ASan, clang-format clean. AUDIT.md rows 3, 6, 7
  closed. Remaining: wave-2 wiring fixes (rows 9-11, 28, 31, 33-35, 38, 40,
  42-45), wave-3 carving (row 32, partially done by the parallel session),
  wave-4 infra (rows 34, 47-49).
