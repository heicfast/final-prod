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

---
Task ID: S5-WAVE2
Agent: main (session 5 continuation)
Task: Wave-2 wiring/semantic fixes - audit rows 9, 10, 11, 28, 31, 33, 35, 38, 40, 41, 42, 43, 44, 45.

Work Log:
- Row 9 (FAT): backup-boot fallback reads at 6 * logical_sector (4Kn aware), not
  the hardcoded 3072.
- Row 10 (exFAT): volume_length is the full u64 from the spec field (the u32
  cast capped volumes at 2 TiB@512 / 8 TiB@4Kn), with overflow-safe bounds.
- Row 11 (exFAT): og stream-dentry reserved-field gates (reserved1/2/3 must be
  zero on live streams) - damaged-set counting parity.
- Row 28 (ReFS detect): the identification fallback reads the REAL boot-sector
  major/minor (+0x28/+0x29) instead of fabricating the string "ReFS".
- Row 31 (lost): the orchestrator passes the live parsed partition table to
  lost::discover (the Inside/Overlaps penalties were dead code), and the
  unknown-BPB candidate size derives from the device end instead of a
  fabricated 2048.
- Row 33 (CRSF): Writer::open writes the session-id hi dword at +0x1C (64-bit
  ids were silently truncated).
- Row 35 (SafeWriter): DestIo::create_temp carries the per-item suffix
  (".part-<id>") - WinDestIo's bare ".part" let concurrent same-final-name
  items collide; resume detection counts existing finals (stats.resumed,
  idempotent re-runs).
- Row 38 (CachedReader): failed blocks are no longer cached as permanently
  invalid - the old code pinned them forever so later reads failed without
  re-attempting the transport. Regression test added.
- Row 40 (orchestrator): every production scan read now goes through the R1.6
  pipe - RetryingDevice (bisection, Skip-mode zero-fill, live error map) +
  8 MiB CachedReader - with the borrowed slice entering via SliceShim; the
  error map and cache counters surface in ScanStats.
- Row 41 (transport): session_get_next_block's granted estimate clamps to the
  plan end (DataFirstSec+TotalSecs tracked per session handle); the cursor can
  no longer overrun short plans.
- Row 42 (BitLocker): auto-unlock extracts the registry values' BINARY data
  (BEK-class blobs) to temp .BEK files and unlocks via the documented
  -KeyFilePath (the old code passed value NAMES as recovery passwords -
  GUIDs, could never succeed); status()'s dead Locked branch removed.
- Row 43 (health): legacy SMART_RCV_DRIVE_DATA (0x7C088) fallback when
  IOCTL_ATA_PASS_THROUGH is refused (old-drivers parity, header contract
  honored).
- Row 44 (enumerate): model/serial string scans bounded by the 4096-byte
  descriptor buffer (malformed-descriptor overread).
- Row 45 (fs_kind): canonical FsKindCode enum in cr_types.h adopting the
  corpus factory map completely - Btrfs 7->11 (was colliding with exFAT),
  carved 0xFF->Raw(6), ZFS 13 (already fixed in the ZFS wave); session load
  remaps legacy 12->13; the indistinguishable legacy Btrfs-7 documented as
  F16 in the error-fix-registry.

Stage Summary:
- 132/132 tests green on gcc+ASan, clang-format clean. Wave 2 closed. The
  win/* changes (bitlocker/health/transport/destio/enumerate) compile on the
  Windows CI (not locally verifiable). Remaining: wave-3 carving depth (row 32
  - partially covered by the parallel session's P1-12 perf fix), wave-4 infra
  (node indexes, per-parser fuzz, TSan job, doc refresh, release).
Task ID: S6-WINDOWS-DLL
Agent: main
Task: The 0xc0000135 postmortem (three CI cycles to root cause)

Work Log:
- Cycle 1 (35876b3): added find_package(liblz4 CONFIG) - but guarded
  behind 'NOT TARGET PkgConfig::LZ4'. With the vcpkg toolchain exposing
  the manifest tree to pkg-config, pkg_check_modules ran FIRST and won -
  the CONFIG fix never executed.
- Cycle 2 (705df9a): deployment backstop (cr_deploy_vcpkg_binaries +
  DeployVcpkgBinaries.cmake, verified locally with a fake tree) copies
  every config-matching DLL next to the exes at POST_BUILD. The exe
  STILL died - decisive evidence: the missing import was not a vcpkg
  DLL name that ANY deployment could satisfy.
- Root cause: pc files are release-flavoured, no per-config mapping ->
  MSVC Debug linked the RELEASE import lib -> exe imports liblz4.dll
  while the debug tree (CMAKE_DEBUG_POSTFIX=d) only has liblz4d.dll.
  applocal's silent name-skip + the exe's startup = 0xc0000135.
- Fix (0d0f706): discovery order inverted - CONFIG package FIRST
  (per-config aware: Debug links debug/lib/liblz4d.lib), pkg-config
  demoted to the Linux fallback, find_library last (naming widened).
  Deploy backstop kept (belt and braces, prints each deployed DLL);
  Diagnostics step now if:always() (was skipped exactly when needed).

Stage Summary:
- Lesson recorded: on MSVC + vcpkg, ANY lib discovered via
  pkg-config/find_library in a multi-config build carries release-path
  bias - always prefer the CONFIG package. Evidence trail (dumpbin
  imports + tree listings) now prints in every windows job.

---
Task ID: S5-WAVE3-CARVE
Agent: main (session 5 continuation)
Task: Raw-sign layer 1 (audit row 32) - the og's raw_signs.json runtime.

Work Log:
- scripts/gen_rawsigns.py compiles the corpus raw_signs.json (139 formats,
  199 header signs, 62 tail signs; format "offset,hex,confidence" with min/max
  extents) into cr_rawsigns_data.cpp - generated data committed alongside the
  generator.
- cr_rawsigns.{h,cpp}: SignIndex with (offset, first-byte) dispatch buckets
  built lazily over the sorted arrays; header probes at 512-byte strides;
  tail scans at the same stride within the bounded [start+min, start+max)
  window (<= 2 MiB block reads - the P1-12 lesson).
- cr_carve.cpp integration: after the grammar pass in every window, the sign
  pass probes each unclaimed sector stride; grammar hits and deduped grammar
  candidates claim their start positions (pass-2 semantics fixed); sign
  candidates dedup through the same F9 extent-class index; small sign hits
  hash like grammar hits. Header-only hit: extent = format max (device-
  clamped), sign confidence. Header+tail paired: the tail sector closes the
  file, confidence = avg + 100 bonus. Shared-magic families (flp/fst on
  46 4c 68 64): highest-confidence sign wins - the og's table is inherently
  ambiguous there and we take the best evidence.
- Stats: CarveStats::sign_hits / sign_validated.
- Tests: GrammarValidationAndSpecificity now asserts the jfif sign hit on the
  JPEG noise plant (the og's layer-1 semantics: header signs accept what
  grammar rejects - F8 applies to the grammar layer only); SecondPassFully-
  Deduped asserts the true F9 invariant (pass 2 adds zero nodes) instead of a
  brittle duplicate count; new RawSignLayerHeaderAndTail test covers
  header-only extents, tail-paired closing, and the shared-magic precedence.

Stage Summary:
- 133/133 tests green on gcc+ASan and the TSan build. Audit row 32's runtime
  layer is complete; the remaining carve item (MP3 frame walk) was closed by
  the parallel session's af4bc8e (P1-12). AUDIT.md rows remaining: none
  blocking - the matrix is fully worked except items owned by the parallel
  session (bench/comparison/Windows CI).

---
Task ID: S5-TAKEOVER
Agent: main (session-5 take-over: verification & reconciliation)
Task: Re-verify the session-5 rewrites + the parallel agent's waves against
upstream ground truth; fix everything found; keep the tree green and pushed.

Work Log:
- Context recovered: all 3 repos re-cloned; found the remote advanced
  concurrently (parallel agent pushed LDM full, NTFS rows 3/6/7, wave-2
  wiring, TSan/fuzz infra, raw-signs carve, benchmark workflows while this
  session ran).
- Dispatched 6 verification agents (ZFS, btrfs/ext4, LDM corpus contract,
  NTFS corpus contract, LDM audit, NTFS audit); personally re-verified
  every disputed constant against fetched upstream sources (OpenZFS
  dmu.h/spa.h/zfs_sa.h/nvpair.c/spl-xdr.c/vdev_label.c, torvalds
  btrfs_tree.h/ext4.h/jbd2.{h,journal.c}, libldm ldm.c).
- Round 1 (ZFS): 7 real-pool-fatal defects fixed; XDR label parser written
  from nvpair.c; fixture rewritten to real XDR (2406257).
- Round 2 (btrfs/ext4): timestamps-from-reserved-bytes, s_inode_size@0x58,
  GDT hi words, ei_leaf_hi@6, jbd2 BIG-endian rewrite, BE jbd2 fixture
  (ee18f75).
- Round 3 (parallel-wave audit): SafeWriter resident-prefix zero-overwrite
  (content destruction, PROVEN); LDM _int_type=3 constant (all striped/
  mirrored volumes dropped); LDM dup-fragment/kernel bounds/degraded/
  per-member re-anchoring; NTFS bootstrap chase gate + fixup + aligned
  stride + vcn ordering; walker UAF; $J sparse/V3/resync/file-ref harvest;
  FILE0 fixup-optional salvage; unnamed resident $DATA capture (6ef4db2).
- Round 4: rebased onto the parallel agent's concurrent pushes (kept their
  $DATA-authoritative logical_size fix merged with resident capture);
  137/137 tests green; all work pushed; CI running.

Stage Summary:
- The 'already fixed' state was audited and found still broken in 3
  engines; four verification rounds produced 20+ production-grade fixes
  each with corpus/upstream evidence and regression coverage where the
  behavior is testable synthetically.
- Remaining from the original session-5 plan: benchmark comparison runs
  (workflows live, running), v0.2.0 tag (blocked on benchmark results),
  cross-disk LDM session registry (single-disk resolver works; the
  LdmVolumeDevice re-anchoring landed the correct primitive).

---
Task ID: S6-BENCH-LOOP
Agent: main
Task: Benchmark take-over loop - 4 defect generations found and fixed

Work Log:
- Gen 1 (183dfcd): geometry fix verified (512e: logical 512/physical 4096);
  volume slices create; scans complete in 2-3.5 min (no stalls). rda
  mystery CLOSED: CreateFile error 87 = the driver's classifier rejected
  (NULL, 0) - the null-buffer guard ran before the empty-name GLOBAL gate
  (IO manager leaves FileName.Buffer NULL for empty names). Fixed in
  rda-driver 7e88e38, tagged v0.2.1; all three workflows now fetch v0.2.1.
- Gen 2 (f553be2): every NTFS logical_size was 0 while exFAT dentry sizes
  were exact - emission took $FILE_NAME's size (0 for fresh files on real
  volumes). Fixed: data_real_size captured from $DATA (actual @+0x30 /
  value_length), $FILE_NAME only as fallback; probe_size too.
- Gen 3 (d0e41a0): recall 42.2%, keep FAIL -> four defects:
  (a) $Recycle.Bin invisible (blanket $-skip) - recycle deletions (26/113)
      vanished; now known-metafiles-only + $I/$R mapping (original name,
      $I size/time, Deleted state; $I metadata never surfaces)
  (b) PDF cross-file absorption (~300 MiB carved extents) - last-%%EOF
      bounded by the next %PDF- (hard boundary)
  (c) enum_attributes tail offsets SWAPPED (file-size into 'initialized',
      initialized into 'actual') - the d##.txt size-0 class; fixture wrote
      real_size at both +48/+56 masking it
  (d) remnant scan: dedup by (parent,name) vs the tree + $DATA size
      preferred; bench_seed pdf footer reserved from $Size (+6 drift)
- Rebase conflicts with the agents' second verification round resolved
  (WalkStats both fields; dedup data_real_size declaration). 137/137.

Stage Summary:
- Benchmark defect loop converged: each run's artifact diffs drove the
  next fix. All three workflows retriggered on 22cc182. Awaiting the
  first keep-control PASS.

---
Task ID: PARITY-T1
Agent: parity-auditor (independent third party — session start)
Task: Parity-audit session bootstrap (mandate: worklog/session-parity-prompt.md,
token redacted). Independent audit of core_recovery + rda-wizard vs the RE corpus;
audit-before-fix.

Work Log:
- All 4 repos cloned at session start; exact pins:
  core_recovery 2473c03e ("fix(win,style): PhysicalDiskList IS the vector"),
  rda-wizard c2eaad6 ("feat(tests)+fix(parsers): 99 tests green"),
  rda-driver 7e88e38 (v0.2.1), easeus-drw-research 18d6f14.
- Calibration reads: docs/ground-truth-contracts.md (all 10 sections),
  docs/architecture.md, README.md, worklog (S5/S6 history), AUDIT-UI.md,
  rda-wizard/docs/CORE-CONTRACT.md in full (contract pins core @ 183dfcd —
  core has since advanced; contract-drift is a live audit axis).
- Open issue triage snapshot: #1..#10 = UI-integration wave (G-01..G-23),
  all updated 2026-10-23T08:29Z. NOTE: docs/UI-READINESS-PLAN.md and
  inc/cr_ui.h named in the mandate do NOT exist yet at 2473c03e (in-flight
  from the core UI-readiness agent).
- Redacted prompt committed to worklog/session-parity-prompt.md.

Stage Summary:
- T1 done. Next: parallel parity sub-agents (engines FS1/FS2/REC/DOM,
  orchestration+CLI, UI, architecture, deltas-registry) → verified
  consolidation into docs/PARITY-AUDIT.md (gate doc, pushed BEFORE any
  fix) → triage → fixes with tests.

---
Task ID: PARITY-T2/T3
Agent: parity-auditor (main + 8 read-only sub-agents)
Task: Full parity audit (engines, orchestration, CLI, UI, architecture,
deltas draft) and consolidation of the gate document.

Work Log:
- 8 parallel read-only sub-agents completed with evidence-cited reports:
  FS1 (NTFS/FAT/exFAT), FS2 (ext4/HFS+/Btrfs/ZFS/ReFS), REC
  (carve/lost/LDM), DOM (session/writer/health/repair), ORCH
  (orchestrator+CLI+contract drift), UI, ARCH, DELTAS draft (35 entries).
  Full reports committed under docs/parity/ (appendices).
- Auditor personally re-verified every P0/P1 against cited lines:
  * F-C-1 P0: wizard list-parser regex wants bracketed table kind; engine
    prints unbracketed (rrecov.cpp:237-242) -> zero disks parsed; root
    cause CORE-CONTRACT §13 transcription error inherited by fixture.
  * F-B-4 P1: fix --undo-apply without --disk passes the guard
    (rrecov.cpp:1049) and writes to PhysicalDrive0 (disk=0 at :175,
    open_disk_rw at :1077). Data safety.
  * F-A1-1 P1: FAT12 entry_at no BAD/EOC canonicalization (cr_chain.cpp:30-37).
  * F-A1-2 P1: NTFS viability probe uses $FILE_NAME real_size
    (cr_mftwalk.cpp:480-483) while emission prefers $DATA (:531).
  * F-A1-3 P1: og fixup validates only first 2 tails — VERIFIED in
    ghidra-searchdll/NTFSSearchDLL_decompiled.c:52129 (local_18 < 2);
    apply-all confirmed at :52452. Ours validates all tails.
  * F-A2-17/18 P1: HFS+ node-kind comment inverted + i16(u8) zero-extend
    (cr_hfsplus_engine.cpp:55,306); BTHeaderRec comment self-inconsistent
    (rootNode u32@0x02 then leafRecords u32@0x04 — 2-byte overlap).
  * F-A4-17 P1: NVMe page-2 128-bit fields at 64/80/96/112 are Host
    Reads/Writes/BusyTime/PowerCycles; spec = 112/128/144/160
    (cr_health.cpp:292-304).
  * F-D-1 P1: fuzz/clang++ broken symlink breaks repo tooling (verified live).
- Contract drift verified: §5.1/§5.2/§12 LDM trio (ldm_resolver, unconditional
  hook, cross-disk registry — G-19 stale); §13 list line + trailing spaces;
  §5.3 0..10000 claim false. PhysicalDiskList drift DISPROVED (vector alias
  since pin).
- docs/PARITY-AUDIT.md (authoritative gate) + docs/parity/*.md appendices
  committed and pushed BEFORE any fix, per mandate.

Stage Summary:
- Audit phase closed: 1 P0 + 9 P1 verified, ~35 P2 + ~50 P3 registered with
  dispositions (FIX/ISSUE/DOC/DELTA/UNVERIFIED), 35 delta entries drafted
  (27 ACCEPTED / 9 NEEDS-WORK). Next: fix wave 1 (F-B-4, F-C-1 pair,
  F-A1-1/2/3, F-A2-17/18, F-A4-17, F-D-1) with tests, then issue filings
  and #1-#10 triage comments.

---
Task ID: UI-W1
Agent: Core UI-Readiness Sub-Agent (session continuation)
Task: Close the W1 milestone of docs/UI-READINESS-PLAN.md — pause/resume (G-07), scan observability (G-04/05/06), truthful session masks + save-on-cancel, G-14 volume info, G-23 candidate restore — with acceptance tests.

Work Log:
- Context recovery after session interruption: inventoried 20 modified files + 2 new test files left uncommitted; lesson applied — commit+push cadence now immediate.
- Fixed telemetry() paused/elapsed live refresh: snapshot copy only stamped at stage boundaries; now refreshes from the caller's token under the telemetry mutex (teardown-safe: stamp() clears tel_retry_/tel_cancel_/tel_running_ inside the same lock a query holds).
- Fixed non-monotonic boundary ticks: run_quick forwarded hardcoded weighted(1000,0,0)=400 while the covers_both bridge had already forwarded 750 mid-walk; boundary calls now forward the post-stamp telemetry overall (tel_overall_()); stale LDM-branch direct call removed.
- CRITICAL fix: run() stored tel_retry_ from a moved-from unique_ptr (use-after-move → nullptr) — the live error map AND the end-of-run stats merge were silently dead. Raw pointer captured before the move; published only while the cache provably owns the device.
- CRITICAL fix: serialize_tree never emitted roots — loaded sessions collapsed all top-level children under a synthetic "Restored" root, destroying the Found/Deleted/Lost/Carved role structure resume() recovers roots by. Roots now serialized (marker=1, parent 0xFFFFFFFF); legacy files load unchanged.
- SessionState v2: deleted_found + carved_found appended (24-byte v1 records still parse, zeroed); load_session_summary restores them so reloaded sessions report truthful totals.
- Carve honest fill-rate: window OVERLAP tails were double-counted (bytes_scanned exceeded the device by 16 sectors); per-window attribution clamped to the window's own [base, base+WINDOW) range.
- FAT/exFAT walk: per-dentry incremental progress ticks (every 64th dentry + end-of-directory).
- Acceptance tests (12 new, deterministic — no polling races): token semantics incl. cancel-while-paused wake; pause compensation proven against measured wall time (elapsed+paused ≈ wall); save-on-cancel truthful mask + resume-skips-quick compared against a reference scan (node counts + found totals equal, quick not re-run); mask tampering → ERR_SESSION_FORMAT; G-14/G-23 volume-info/candidate round-trip over a partition slice; monotonic overall; deterministic FAT incremental ticks via the user callback; per-stage shape; mid-scan live error map via a device that parks a later-stage read AFTER the range is recorded; concurrent telemetry queries (TSan).
- Rebased over PARITY-T2/T3 (docs-only), re-ran suite, pushed.

Stage Summary:
- Commit 804972a on main: 150/150 tests green under ASan (138 prior + 12 new).
- W1 closed: G-04/G-05/G-06 (progress/ETA inputs/live error map), G-07 (pause/resume/save-on-cancel/truthful mask), G-14, G-23. D-3 architecture-doc correction still pending (docs sweep at T17).
- Three silent-failure bugs eliminated (tel_retry_ null, root serialization, carve double-count) — all caught by the new acceptance tests, exactly the no-failure-masking mandate.
- Next: W2 per UI-READINESS-PLAN — writer dest dirs + empty-folder nodes (G-08, issue #5), recovery report JSON (G-09, issue #6), then preview reader (G-10) and query/filter/sort/paging (G-12).

---
Task ID: PARITY-W1/W2 (fix waves + coordination)
Agent: parity-auditor (main)
Task: Fix every auditor-verified P0/P1 with tests; land CI unblock; file
coordination evidence; promote the deltas registry.

Work Log:
- Fix wave 1 (all with before/after stash proofs where testable):
  * 1ecf446 F-D-1/F-D-8: fuzz/clang++ broken symlink + temp_data.txt removed.
  * d83442e F-B-4: 'fix --undo-apply' requires --disk (was writing PD0).
  * 0329f89 F-A1-1: FAT12 BAD/EOC canonicalization + FAT12 fixture/tests
    (proven failing before via stash).
  * e60bb25 F-A1-2: NTFS viability probe uses $DATA size (Excellent ->
    PartialRisk on partially-reused deleted files; stash-proven).
  * c5b0467 F-A1-3: fixup validates first two tails (og FUN_1800411f0
    :52129 'local_18 < 2' — verified in the decompile by the auditor);
    apply-all kept; contract §3 row added; TornRecordRejected moved into
    the checked window + TornThirdTailStillSalvaged added.
  * 4cac981 F-A2-17/18: HFS+ node kinds (-1/0/1/2, sign-extended SInt8)
    + BTHeaderRec offsets (nodeSize@+0x12 etc. — TN1150/libyal/og
    FUN_180010720); fixture rewritten to the real layout; stash-proven.
  * a1ff58e F-A4-17: NVMe page-2 offsets 112/128/144/160 via new pure
    cr::health module (unit-tested on Linux; win layer delegates).
  * f6468ba F-A3-6: carve sign_hits/sign_validated double-count removed
    (literal duplicated lines); assertion tightened to EXPECT_EQ.
- F-C-1 P0 (wizard, coordinated pair bd83be3 + core contract): list
  parser matched zero real engine lines (bracketed table kind vs the
  engine's unbracketed final token) — regex + golden fixtures + guard
  tests + CORE-CONTRACT §13 corrected. Wizard CI green (ci + ui-smoke).
- Doc wave: CORE-CONTRACT §5.1/§5.2 LDM trio + §5.3 0..10000 + §13
  trailing spaces + G-18/G-19 staleness corrected (042e081);
  docs/PARITY-DELTAS.md promoted to authoritative in core (5eaddab) +
  UI mirror in wizard; cr_mftwalk.h stale scale comment fixed.
- Coordination: core issues #1-#10 all carry parity triage comments
  (og evidence attached: writer mkdir DRWUI:357842, og pause, og
  progress model, FileFilter.xml 8/1489, preview free-tier framing);
  rda-wizard#1 filed for the parser P0; core#11 filed for the red
  804972a milestone (lint/tsan/windows root causes + mechanical unblock
  eefbb63 landed by the auditor: clang-format-18, format-truncation
  buffer, _getpid()).
- Concurrency observations: F-B-8 (abort-path error map) CLOSED by the
  UI-readiness agent's stamp() lambda; writer G-08 mkdir landed (91053b0);
  pause/telemetry evolving live — audit verdicts for those areas are
  against the audited pins, re-verify after their wave completes.

Stage Summary:
- 10 fixes landed (8 wave-1 verified findings + carve double-count + CI
  unblock), suite 159/159 green (gcc+ASan), lint clean (clang-format-18),
  wizard 101/101 green locally + CI green. PARITY-DELTAS.md live in both
  repos. Remaining registered scope: carving salvage batch (F-A3-2/3/4/5),
  Btrfs chunk-tree uuid + inline extents, ReFS next-chains + MSB tests,
  ZFS gang tests, F-A1 P2 code items — all filed with evidence pointers
  in docs/PARITY-AUDIT.md §3 and issues #3/#5/#8/#10.

---
Task ID: UI-W2
Agent: Core UI-Readiness Sub-Agent (session continuation)
Task: Close G-08 (writer dest dirs + empty folders) and G-09 (recovery report JSON) per docs/UI-READINESS-PLAN.md.

Work Log:
- G-08 (commit 91053b0): DestIo::mkdir (pure virtual; Win32 recursive CreateDirectoryW chain, already-exists tolerated, file-in-the-way surfaced); WriterConfig::reconstruct_dirs (default on) — per-item dest_dir ensured (cached set) before create_temp, directory NODES enqueueable (mkdir items, no extents → never overlap-refused) so empty leaves materialize; CLI recover walk enqueues dirs.
- ConflictPolicy {Rename (default, corpus R7.1 og parity), Skip (idempotent re-runs), Overwrite}: reconciled the contradiction where resolve_collision renamed on pre-existing finals, making the audit-row-35a resume detection unreachable (re-runs duplicated everything as "(2)"). Intra-queue duplicates always rename via a claimed-name set. Structural tests over a filesystem-backed DestIo (std::filesystem, portable).
- G-09 (commit 1803d2b, rebased over the parity agent's 11 fix commits): recovery/cr_report.h — deterministic strictly-escaped JSON (schema v1 in docs/recovery-report-schema.md); Outcome enum includes "pending" (aborted-pass truth, never claimed done); hash honesty — writer ALWAYS finalizes the streamed SHA-256, hash_status ∈ verified/computed/mismatch/not_computed (never faked); QueueItem captures duration (timed wrapper on every exit path) + resumed + sha hex; stats.queued stamped upfront (aborted passes report true queue size); CLI --report file.json; CI validates the emitted sample via tools/validate_report.py in the Linux ASan job.
- D-3 architecture.md corrected (pause/resume two-layer model).
- Rebased twice over parallel parity-audit agent commits (docs + engine fixes; carve raw-sign double-count fix coexists with my window-attribution fix — suite green post-rebase).

Stage Summary:
- 163/163 tests green under ASan + LeakSanitizer (includes the parity agent's 6 new tests post-rebase).
- Closed this session so far: G-04/G-05/G-06, G-07, G-08, G-09, G-14, G-23 (+D-3 docs).
- Lesson applied: commit+push after every coherent unit; pull before push (rejected twice, rebased twice).
- Next: G-10 bounded preview reader + metadata extractors (W1-7), then G-12 query/filter/sort/paging (W1-8), then the C ABI shim (W2).

---
Task ID: UI-W3
Agent: Core UI-Readiness Sub-Agent (session continuation)
Task: Close G-10 (bounded preview reader + metadata extractors) per docs/UI-READINESS-PLAN.md.

Work Log:
- New module include/cr/preview/cr_preview.h + src/core/preview/cr_preview.cpp: PreviewReader (resident-prefix-then-extents stream per the writer contract, sparse tail zero, 4 MiB/call cap, 256 MiB decode guard, EOF short reads, zero-fill + unreadable_spans() reporting with position-preserving advancement) and extract_metadata (JPEG SOF+EXIF orientation/datetime, PNG/GIF/BMP/TIFF/WEBP dims, MP4/MOV brand+mvhd duration with head+tail windows, WAV, PDF/DOCX/text snippets, hostile-size bails).
- Fuzz harness fuzz/fuzz_preview.cpp registered in the fuzz suite (45s smoke): arbitrary extent lists + all extractors, device-traffic bound trap.
- Bug fixes during acceptance: TIFF value offset is at entry+8 (was reading the count field); jpeg_exif must WALK to APP1 (APP0-first files); position advancement on failed reads keeps multi-extent alignment; test-side use-after-move sizes and APP1 length-field off-by-2.
- Rebased over the parity agent's commits (incl. their MSVC fix for test_writer_dirs path::native on Windows).

Stage Summary:
- Commit 35f4652 on main: 172/172 green under ASan + LeakSanitizer.
- Closed this session: G-04/05/06, G-07, G-08, G-09, G-10, G-14, G-23, D-3.
- Remaining: G-12 query/filter/sort/paging (next), G-01/02/03 C ABI shim + snapshot, #9 auto-checkpoint, #10 disposition + docs matrix, release tag + handoff.

---
Task ID: PARITY-T20 (release & handoff)
Agent: parity-auditor (main)
Task: CI restoration, release tags, handoff for the next agent.

Work Log:
- CI unblock series (main was red on the UI-readiness milestone 804972a
  and follow-ups): eefbb63 (clang-format-18 on their files + 2 -Werror
  test warnings), 9b4e656 (test_writer_dirs C2676 wstring concat),
  685243a (TSan data race in the telemetry test captures — mutex-guarded
  + the auditor's own files formatted with the CI pin), 0beba6a (last
  getpid() C4996 swept tree-wide). Root causes + attribution filed as
  issue #11. Local evidence at each step: ASan 172/172, TSan 172/172,
  telemetry stress 6x clean, lint clean under 18.1.8.
- Contract note for the UI-readiness agent (filed on #11): the progress
  bridge invokes callbacks from engine worker threads, not only the scan
  thread — CORE-CONTRACT §5.3 wording needs that thread-safety clause.
- Deliverables state:
  * docs/PARITY-AUDIT.md — authoritative grid + findings + dispositions
    (status header updated: fix waves landed 1ecf446..0beba6a).
  * docs/PARITY-DELTAS.md — authoritative (35 entries; UI mirror in
    rda-wizard).
  * Issues #1-#10 triaged with parity evidence; #11 coordination;
    rda-wizard#1 (P0 notice).
  * Release tags: v0.2.1-parity (core) / v0.1.1-parity (wizard) on the
    parity fix series — tag messages reference PARITY-AUDIT.md.

Handoff for the next agent:
- PIN: core 0beba6a (all parity fixes + CI unblocks; verify the CI run
  on it is green before building on top). Wizard: db5a9da (CI green).
- REMAINING PARITY SCOPE (all evidence-cited, see PARITY-AUDIT.md §3):
  * Carving batch F-A3-2 (GPT backup-salvage dead code), F-A3-3 (ZIP
    3-anchor + indexless salvage), F-A3-4 (BE-TIFF/ORF dead branch),
    F-A3-5 (PST unknown-version accept w/ 1 MiB floor).
  * F-A1-7 (exFAT first-cluster gate), F-A1-10 (0x3FF boundary '>'),
    F-A1-11 (FAT cancel polling), F-A1-12 (deleted-name raw bytes).
  * F-A2-2/5 (ReFS og next-chains + MSB multi-level/dual-clock/backup-SB
    tests), F-A2-12/13/14 (Btrfs chunk-tree uuid source, inline-extent
    content, crc32c policy), F-A2-7/8 (ZFS gang tests + stale header
    comments).
  * F-A4 session identity fields (ties G-14), collision naming delta
    wording, F11 registry text correction.
  * UNVERIFIED register (docs/parity/* UNVERIFIED sections) — close with
    corpus evidence or register as deltas.
- OPEN QUESTIONS: og TRIM-toggle caller semantics (D-…); og writer hash
  verification; og NVMe mapping (spec used); og session identity fields
  (record 2/3 "identity card"); multi-session .rsf in the wild.
- CONCURRENCY: the UI-readiness agent lands fast (G-08/G-09/G-10 closed
  during this audit; pause/telemetry live). Re-verify PARITY-ORCH
  verdicts against their landing commits before fixing anything in
  cr_scan.cpp — that file is theirs right now.

Stage Summary:
- Parity session complete: audit gate (8 appendices) -> 11 fixes with
  tests -> registry + contract corrections -> issue triage -> CI
  restoration -> tags. Main green trajectory restored; remaining scope
  precisely filed.
---
Task ID: S5-CONT-1
Agent: Session-5 continuation take-over (verification & reconciliation, round 2)
Task: Re-run the session-5 mandate: independent line-by-line audit vs the RE corpus,
fix everything, close the registered remainder (PARITY wave-2 batch, UI-facing gaps),
full CI/benchmark/release.

Work Log:
- Context recovered: all 3 repos re-cloned at main (core 0beba6a, driver 7e88e38, corpus
  18d6f14); 117-commit history reviewed; AUDIT.md + PARITY-AUDIT.md + PARITY-DELTAS.md +
  docs/parity/* read in full; prior session-5 rounds 1-4 and the parity-auditor waves mapped.
- CI state at take-over: runs 95-102 red (lint/tsan/windows cascades from the parallel
  agents' landing cadence); 103/104 in flight at HEAD.
- Immediate unblock: run 104 lint failed on clang-format drift in 4 files
  (cr_scan.h / cr_records.h / cr_device.h / cr_fat_quick.cpp) left by the
  pause/telemetry + writer-dirs waves; formatted with clang-format 18.1.8
  (CI scope: src include tests only — fuzz/ + scripts/ldm_reference.* intentionally
  left at their committed formatting, they are outside the lint gate).
- Local gate before push: full build + 172/172 tests green (gcc 14.2 -Werror + ASan).
- Session prompt re-saved (worklog/session-05-prompt.md, token redacted).

Stage Summary:
- Lint drift fixed at HEAD; main re-gated. Registered remainder to audit-verify then fix:
  carve batch F-A3-2/3/4/5, Btrfs chunk-tree uuid + inline extents (F-A2-12/13),
  ReFS next-chains + MSB tests (F-A2-2/5), ZFS gang tests (F-A2-7/8), F-A1 P2 code items
  (A1-7/10/11/12), F-B abort-path merge + P3 batch, G-12 query/filter/sort paging (#8),
  G-13 auto-checkpoint (#9), G-01/02/03 C ABI + snapshot (#1/#2), doc corrections
  (D-family), then benchmark + v0.2.0 release.

---
Task ID: UI-W4
Agent: Core UI-Readiness Sub-Agent (session continuation)
Task: Close G-12 (core-side query/filter/sort/paging) per docs/UI-READINESS-PLAN.md.

Work Log:
- New module include/cr/query/cr_query.h: composable filter (category mask via the core-side extension map, size/date ranges, confidence ordinal floor, extension list, case-insensitive substring, deleted-only, origin mask), stable sort (name/size/modified/confidence/path), offset+limit paging, count-only. Name-dependent predicates last + configured-only (count path never allocates strings); rows built only for the page.
- CRITICAL fix surfaced by the perf gate: ResultTree::add_file sibling-tail walk was O(n) per insert => O(n^2) for a million-file carve root. FileNode::last_child_ added (add_file + reparent surgery) -> O(1) appends. The architecture's million-node claim was time-broken; now honest.
- Perf gate in-suite: 1M nodes filter+count 31-36 ms UNDER ASan (200 ms budget holds without sanitizer scaling); sorted page ~105 ms informational.
- Correctness: deterministic predicate/sort/paging cases + randomized-vs-brute-force reference (20 rounds).
- Rebased over parity-agent commits (NTFS runlist INT64_MIN fix, TSan race fix in my telemetry test captures, MSVC getpid fix).

Stage Summary:
- Commit 365c263 on main: 176/176 green under ASan + LeakSanitizer.
- Closed this session: G-04/05/06 (progress/ETA/live map), G-07 (pause/resume), G-08 (dirs), G-09 (report), G-10 (preview), G-12 (query), G-14, G-23, D-3 + the O(n^2) tree fix.
- REMAINING for UI-readiness: #1/#2 (C ABI shim cr_ui.dll + mid-scan snapshot ADR + TSan), #9 (auto-checkpoint CheckpointSink), #10 remainder (G-11 disposition note, G-15..G-22 table), CLI tree --json (W2-2), README capability matrix, session #8's stage-weights doc, release tag + handoff pin for the UI agent.
- Handoff note for the shim agent: telemetry()/resume()/report_items()/query run()/PreviewReader are the five surfaces to wrap; all are pure-core, no OS deps, and testable on Linux CI before the Windows DLL lane.

---
Task ID: UI-SESSION-HANDOFF
Agent: Core UI-Readiness Sub-Agent (session continuation — FINAL)
Task: G-13 auto-checkpoint + session handoff record.

Work Log:
- G-13 closed (commit be8875f): ScanConfig::CheckpointSink; stage-boundary emission of a full truthful session; crash-resume + corrupt-magic tests; 179/179 green.
- Evidence comments posted on issues #3 #4 #5 #6 #7 #8 #9 with commits + acceptance proof.
- Session issue-tracker state: #3-#9 have implementation + tests + evidence comments. Remaining open work: #1 (C ABI shim), #2 (mid-scan snapshot + ADR), #10 (umbrella remainder).

Stage Summary (what the next agent MUST read first):
- COMMITS this session: 804972a (W1 pause/telemetry/session-truth + 3 silent-failure fixes), 91053b0 (G-08 writer dirs + ConflictPolicy), 1803d2b (G-09 report JSON + CI validator), fdc2cf9 (D-3 docs + worklog), 35f4652 (G-10 preview + extractors + fuzz), 365c263 (G-12 query + O(1) tree appends), be8875f (G-13 checkpoint). All rebased over the parity agent's waves; main green at be8875f (179/179 ASan+LSan).
- Surfaces ready to wrap in the cr_ui shim: ScanController::run/telemetry/resume/load_session_summary/save_session; SafeWriter queue/process/report_items/report_totals; report::build_report; query::run; PreviewReader + extract_metadata. All pure-core (Linux-testable before the Windows DLL lane).
- Known remaining scope (do NOT re-derive): #1/#2 = C ABI header inc/cr_ui.h per CORE-CONTRACT.md section 2 + snapshot ADR (copy-on-write at stage boundaries is the preferred shape; telemetry()/checkpoint hooks already give the boundary points; TSan tests); #10 = G-11 (.rsf import) disposition note + G-15..G-22 table + README capability matrix + ground-truth-contracts extension; CLI tree --json projection (query::run rows; --csv grammar frozen byte-identical).
- Coordination: the parity agent (issue comments + PARITY-AUDIT docs) and this agent have been pushing concurrently; pull before push EVERY time (3 rebases this session); their fixes touched my headers/tests (format, TSan race, MSVC) - re-run the full suite after every rebase.
- Win7/MSVC notes for the shim: cr_tests now links std::filesystem (test_writer_dirs) - the DLL target itself must stay free of it; /W4 /WX + a small C consumer test per the plan.

---
Task ID: S5-CONT-2
Agent: Session-5 continuation take-over (round 2, post workspace-reset)
Task: Context re-recovery after an unexpected session stop + workspace reset;
resume the verification-and-fix loop on the current main (now including the
principal-engineer parity-refactor session's W1/W2/W3 waves).

Work Log:
- Workspace reset recovery: all 4 repos re-cloned (core b505799, driver, corpus,
  + rda-wizard); the un-pushed benchmark.yml stray-brace fix was lost to the
  reset - verified the concurrent refactor session had fixed it at HEAD (the
  delete-partition step now parses; the Math::Round 'corruption' was a false
  alarm - hexdump shows $([math]::Round, valid pwsh).
- Mapped the parity-refactor session's landed waves vs the PARITY-REFACTOR-PLAN:
  W1 (fallback-truth), W2 items 1-18 mostly landed (FAT/exFAT/NTFS/ext4/ReFS +
  Btrfs trio + writer collect-and-continue + carve batch), W3 partial
  (parallel deleted merge, PoolCancelToken deleted, RawFixer, health, LDM
  honesty). NOT landed: W2-9 FAT cancel, W2-19 GPT backup-salvage, W3-4/5/9/10,
  W4 dead-code sweep, W5 docs wave, W6 (CI green / Win7 pins / release).
- CI at take-over: runs 114/115/116 red in 3 lanes (MSVC C4267 in the new
  btrfs test; TSan 263ms>200ms QueryPerf gate; fuzzer heap-buffer-overflow in
  extract_metadata - the preview short-window class, exactly the P1 my earlier
  Agent C2 audit had filed). All three fixed with regression tests (commit
  f93f38e): C4267 cast; TSan-scaled budget (2000ms under __SANITIZE_THREAD__,
  200ms otherwise); Window::fetch split into strict fetch / lenient fetch_any
  with last_got; WAV fmt-chunk window fix; DOCX raw-deflate windowBits -15 +
  honest zip labels; malformed-caps extent advance; + 5 new tests incl. the
  fuzzer crash shape and a 3000-round deterministic stress (runs in every CI
  lane, including the ones without libFuzzer).
- Local gate: 204/204 green (gcc -Werror + ASan), lint clean (18.1.8), pushed.

Stage Summary:
- main should go green on f93f38e (watch CI; react to any lane noise).
- Registered remainder now: W2-9 FAT cancel polling, W2-19 GPT backup-salvage,
  W2-18 writer salvaged outcome flag, W3-4 CLI Ctrl-C, W3-9 SESS identity card,
  W3-10 dedup, W4 dead-code sweep, W5 docs wave (D-36..D-44, capability
  matrix, benchmark report, corpus errata), W6 (CI green + Win7 user-mode
  import gate + release tag), plus my audit's own items: NTFS LZNT1/XPRESS
  decompression (P1 silent corruption), lzjb source bound, recycle $I/$R
  remap engine bug (bench: ours 0/26 vs og 26/26), carved_found counter,
  transport elevation probe, session identity card, benchmark scoring fixes
  (numeric enum schema, keep-gate ACTIVE, precision accounting, rsf
  contamination guard, service name rda), G-02 snapshot + G-01 C ABI shim,
  #10 umbrella remainder.

---
Task ID: PARITY-W4
Agent: principal-engineer (deep-parity refactor session, main)
Task: W4 dead-code sweep (all five audit registers) + DC-12/13 consolidation; P1-2/P1-3
duplicate resolution; scope declaration.

Work Log:
- W4 landed at 0c6a1f5 (net -420 lines): NTFS register 1-18+21 (INDX family deleted per
  ruling D-N; three hand-rolled attribute walkers consolidated on enum_attributes with
  uniform bounds + shared 64-attr cap; child-record partial salvage preserved), FAT/exFAT
  register 1-20 (FAT-8 backup-boot fallback gated to FAT32-validating candidates +
  regression test; keep_deleted removed from Fat/Exfat/Btrfs/Zfs - BtrfsScanFlags and
  ZfsScanFlags deleted entirely with signature updates; entry_set_checksum composed from
  checksum16; shared fat_datetime_to_unix_seconds), LINUX-FS DC-1..DC-11 (22 dead
  constants; FE_COMPRESSION kept - live since W2's BTR-4; detect_refs marked
  diagnostic/test surface honestly; cr_bits.h shared readers replacing 7 private copies;
  unix_to_filetime_ns shared fold replacing 3 private folds), PIPELINE register 2/5/6/7/8/9
  (MbrTable name removed with value 4 reserved - serialized positions; fs_probe
  score-duplicate deleted; CarveResult::fragmented dead field + 4 sets deleted), ARCH
  register (open_disk_view, status_text, trim fields, failed_bytes, inner x2, index_node
  wrapper, unreachable cached branch, round_up, temp_data.txt; volume_letter_to_disk KEPT
  + documented as the writer overlap preflight candidate; devmap redundant merge loop;
  fs_kind enum style x15; hex literals -> named errors).
- P1-2/P1-3: I implemented both in parallel with S5-CONT-2 (worklog declarations crossed
  in flight). Their 632edc4 is MORE complete on P1-3 (they also caught the extension
  convention: $R keeps the original extension, $I does not - my full-suffix token would
  still have missed real volumes). My duplicate was dropped cleanly; theirs adopted;
  verified 211/211 green at 632edc4.

Stage Summary:
- CLAIMED NEXT by this session (to avoid the next collision): the W5 docs wave -
  PARITY-DELTAS D-36..D-44 + refreshes (D-06/07/13c/17, D-28/29/30, U-8/12),
  CAPABILITY-MATRIX.md, BENCHMARK-REPORT.md, error-fix-registry F11/F15,
  ground-truth-contracts §2/§3/§6, CORE-CONTRACT §13 coordination, corpus errata feed.
  These are this session's own registry deliverables.
- SUGGESTED for S5-CONT-2 (they have the audit context): GAP-P1-1 (NTFS compressed
  $DATA LZNT1/XPRESS/LZX), GAP-P1-5 (elevation probe), GAP-BENCH-1..5 (their scripts),
  G-01/G-02 (their UI plan).
- This session will then close out: W3-9a/b (SESS identity card + compress rule),
  W3-10 leftovers (dispatch dedup, OVERLAPPED dedup), W6 (Win7 pins + release tag).

---
Task ID: S5-CONT-3
Agent: Session-5 continuation (S5-CONT-2 line, main)
Task: P1 correctness wave + benchmark truth wave per the CORE-GAP-LIST
closure protocol (coordinated with PARITY-W4: they own W5 docs, W3-9a/b,
W3-10, W6; this agent owns P1s, BENCH-1..5, G-01/G-02, tree --json).

Work Log:
- GAP-P1-1 (464d8e5): NTFS compressed $DATA recovers real content. kLznt1
  codec in cr_decomp (MS-XCA 3.3 tagged groups, dynamic split 12..4,
  full bounds discipline) PROVEN against the specification's own 142-byte
  documented vector (Lznt1MsXcaDocumentedVector decodes byte-exact).
  Engine: build_props surfaces data_compressed/data_cunit (encryption
  refused per F13); expand_compressed_data walks the per-unit envelope
  (hdr==0 hole / bit15 raw / else LZNT1 {psize=(hdr&0xFFF)+1}); failure
  => no extents (zero-fill, never raw envelope bytes = the silent
  corruption). WalkStats.compressed_blocks/failed. XPRESS/LZX registered
  as a deliberate exclusion (NTFS $DATA is LZNT1-only; WOF payload lives
  in reparse data - og parity: NTFSSearchDLL is LZNT1-only too).
- GAP-P1-2/P1-3 (632edc4): lzjb source bound (every read bounded vs
  psize, documented divergence from OpenZFS which trusts checksums);
  recycle $I/$R pairing fixed for the REAL Windows convention ($R keeps
  the original extension, $I does not - token = first-dot-stripped) +
  post-walk fixup for $I-at-higher-record-number (bench 0/26 -> the
  fixture asserts 3 remaps incl. the post-pass one). Codec round-trip
  tests (gzip + reference lzjb) close the C2-audit test gap.
- GAP-CI-1 (0a28a23): tests/unit/test_tmpdir.hpp - MSVC does not resolve
  /tmp; all three test fixtures + the ci.yml validator consumer go
  through temp_directory_path()/tempfile.gettempdir().
- GAP-P1-5 + GAP-BENCH-1..5 (837a543): elevation probe (TokenElevation,
  UAC-accurate) replaces the four '(admin?)' guesses; scorers decode the
  frozen numeric CSV enums (deleted 2/3 were classified active via the
  accidental '1' collision); keep gate active-only; per-result precision
  accounting; compare_report dialect normalization (numeric vs og words -
  fixes the 536-hidden-carved-rows report distortion); comparison service
  renamed rdatest->rda (the device name derives from the SERVICE name -
  the comparison never exercised our driver) + hard transport assertion
  in the scan log; og .rsf contamination guard (pre-launch snapshot).
- Earlier this session: f93f38e (3-lane CI restoration: MSVC C4267, TSan
  budget scaling, the preview short-window OOB class the CI fuzzer found
  live - strict fetch/fetch_any window contract + WAV fmt-window fix +
  DOCX raw-deflate -15 + honest zip labels + malformed-caps advance + 6
  new tests); 3267d26 (W2-9 FAT per-step cancel, W2-19 GPT two-step
  salvage per the decompile, W2-18 writer Salvaged outcome + Totals.

Stage Summary:
- Closes: GAP-CI-1, GAP-P1-1..5, GAP-BENCH-1..5, GAP-W2-9, GAP-W2-19,
  (W2-18 was GAP-P1-4).
- Suite: 213/213 gcc -Werror + ASan green; MS-XCA vector byte-exact.
- Remaining for this agent: G-02 snapshot (cr_snapshot, the UI-plan
  shape), G-01 C-ABI shim (cr_ui), tree --json (GAP-TREE-JSON), #10
  remainder (README capability matrix lives with PARITY-W4's wave).

---
Task ID: PARITY-SESSION-WRAP
Agent: principal-engineer (deep-parity refactor session, main)
Task: W3-9 + W3-10 leftovers + W6-WIN7 + gap-register closures + session handoff.

Work Log:
- W3-9 (cd4bb41): SESS-2 identity card (VolumeInfo v2 tail: model/serial/boot-hash,
  boot-hash computed by the orchestrator at run start, adopt-on-resume preserves a
  no-run re-save, CLI fill_identity_card + resume verify warning; v1 records
  deserialize zero-card = unverifiable) + SESS-4 compress-everything (the >=64 tail
  carve-out removed; strictly-smaller rule is the whole story). Tests:
  VolumeInfoIdentityCardRoundTrip (80+84 layout pinned), TinyChunkDeflatesWhenSmaller.
- W3-10 (34728b2 + 800e80a): single run_engine_dispatch for the main pass and the
  lost-candidate path (the audited drift - candidates always ran deep extras via
  defaults - now one explicit deep_extras parameter; per-engine candidate counting
  shapes preserved exactly; 215/215 behavior-identical) + shared overlapped_read in
  the win layer (three private ReadFile/OVERLAPPED dances collapsed; RwDevice
  write path keeps its own staging loop).
- W6-WIN7 (74f42c0): the user-mode Win7 envelope is machine-enforced like the driver
  repo - SDK pins (_WIN32_WINNT/WINVER/NTDDI 0x0601), stamp_osversion_um.py
  (post-link PE patcher), verify_pe_um.py + ref_imports_um.json (strict
  kernel32/advapi32 function allowlist from the source census; name-gated
  CRT/zlib DLLs), CI windows-lane gate step. First CI run may fail visibly on
  un-censused linker imports - conscious amend is the designed path.
- Gap register: closures applied with citing commits (this commit). Still open by
  owner: S5-CONT-2 - GAP-P1-1 (NTFS compressed $DATA), GAP-P1-5, GAP-BENCH-1..5,
  GAP-G-01/G-02 (cr_ui shim + snapshot), GAP-TREE-JSON; this session - none
  claimed beyond this wrap.

Stage Summary:
- SESSION DELIVERABLES LANDED: six audit docs + PARITY-REFACTOR-PLAN gate (ffd9dc8);
  W1 fallback-truth (0f88fd9); W2 engine parity x2 (435bc5e, 7577f74); W3
  orchestration truth (b505799); W4 dead-code sweep, net -420 lines (0c6a1f5);
  W5 docs wave - PARITY-DELTAS D-36..D-44 + CAPABILITY-MATRIX + F15 + BENCHMARK-REPORT
  (046889d, e9e1176) + corpus errata (research 03ad46c) + CORE-CONTRACT 13 sync
  (wizard 8bfc508); W3-9/W3-10 leftovers (cd4bb41, 34728b2, 800e80a); W6-WIN7
  (74f42c0).
- RELEASE TAG NOT CUT (deliberate): v0.3.0-parity waits for (a) the windows CI
  lane proving the Win7 gate green on real binaries, (b) the fuzz lane re-run over
  the W4-touched engines, (c) the parallel agent's GAP-P1-1 (NTFS compressed data
  is a recovery-output P1 - a release with silent raw-compressed-bytes output would
  be dishonest). Tag recipe: on verified-green HEAD, tag v0.3.0-parity with notes
  referencing PARITY-DELTAS + BENCHMARK-REPORT + the closure list in
  CORE-GAP-LIST.
- COORDINATION RECORD: one clean duplicate-resolution (my P1-2/P1-3 dropped for
  their more complete 632edc4 - they caught the extension-convention bug I
  missed); one textual rebase conflict resolved keeping both; scope claims
  published via this worklog; pull-before-push every time (three races total).
- NEXT AGENT SHOULD: verify CI green on 74f42c0+ (all lanes), consciously amend
  ref_imports_um.json if the gate trips, then execute the tag recipe above; the
  P1-1/G-01/G-02/BENCH work continues per the register owners.

---
Task ID: S5-CONT-4
Agent: Session-5 continuation (S5-CONT-2 line, main)
Task: G-01/G-02 UI integration surfaces + tree --json (the remaining
items on this agent's queue after the P1/bench waves).

Work Log:
- G-02 (28309b2): cr::tree::Snapshot - the copy-on-write value snapshot
  (ResultTree::capture_snapshot under the tree mutex; SNAP_PARTIAL truth;
  DFS byte-parity with the live serializer) + SafeWriter value-enqueue
  (writer-owned props store; mid-scan recovery touches zero live-tree
  state). ADR-UI-SNAPSHOT written. 4 acceptance tests incl. the TSan
  concurrent emit/capture/walk + recovery-determinism byte parity.
- GAP-TREE-JSON (57403a0): rrecov tree --json - the query::run
  projection with the frozen schema (same numeric enums as --csv;
  report::escape_json for all strings). --csv byte-identical.
- G-01 (e79d23f): include/cr/cr_ui.h + the cr_ui shared object - the
  stable C ABI (ABI version, thread-local last_error, opaque handles,
  abi_size-versioned PODs, catch-all per entry). Scan lifecycle (image-
  file device w/ 64-bit stdio, pause/cancel/resume, telemetry POD),
  snapshot+children, query (G-12 semantics, lock-free over the snapshot),
  recovery (callback DestIo portable + Win32 path), report, preview,
  sessions; Windows-only list/health/scan_disk exports guarded.
  tools/cr_ui_abi_check.c: plain-C consumer (-std=c11, -Wall -Wextra
  -Werror; /W4 /WX on MSVC) builds a FAT16 image from scratch and runs
  the ENTIRE lifecycle - green under ASan with zero leaks (caught one
  real null-deref + one handle leak in the shim during acceptance).
  Registered in ctest for every lane.
- e8da7ef: MSVC flag-spelling guard for the consumer target.

Stage Summary:
- This agent's register queue is EMPTY: GAP-P1-1..5, GAP-BENCH-1..5,
  GAP-CI-1, GAP-W2-9, GAP-W2-19 (P1-4), GAP-G-01, GAP-G-02, GAP-G-02b,
  GAP-TREE-JSON - all closed with regression evidence.
- Remaining repo-wide (PARITY-W4's lane): W6 Win7 user-mode import gate
  pins, the release tag + artifacts, GAP-CI-2 lane watching, the #10
  umbrella disposition (README matrix pieces live in their W5 wave).

---

## Session 6 — WIRE: the UI-core connection (G-01 landed)

**Task**: user directive: "how the og architecture and codebase connects wire
core with ui check it out, its something like shib api or something? our ui
wizard docs says its unavailable and uses cli for now, verify and validate
this, independently, and implement this if og architecture used this and we
have not implemented so, top priority."

**Verification (independent, corpus-direct)**:
- OG wire = IN-PROCESS C-export factory binding: DRWUI LoadLibraryW's the 10
  *SearchDLL.dll engines, GetProcAddress'es EUCreate<FS>Searcher /
  EUCreate<FS>QuickSearcher -> ICompleteFileSystemSearchController pointers
  (searchdll-family sec.1; DRWUI-exe sec.9.2: loader FUN_14056c0d0, map
  FUN_140577d40). MOM (.mo) for partition ops. IPC only auxiliary (BitLocker
  FVE pipe, preview TCP/zmq). "shib api" = the SHIM = our GAP-G-01.
- Wizard docs verified: ADR-1 "CLI-subprocess now, native shim later" exists
  precisely because G-01 was open.

**Implementation (2a93c45, rebased over the parallel S5-CONT-4 push)**:
- include/cr/cr_ui.h: C11, 57 exports, cr::Err space, zero-init-safe
  configs, two-call pattern, thread-local last_error, ABI version policy.
- src/ui/cr_ui.cpp: binding layer only; CLI-exact semantics (identity card,
  LDM resolver, checkpoint sink, BitLocker gate, [F12] arming, plan() walk);
  ImageFileDevice + POSIX FileDestIo for non-win hosts; win surfaces answer
  ERR_UNSUPPORTED elsewhere (documented platform truth).
- tests: cr_ui_tests (9 gtests) + cr_ui_abi_check (plain C, full lifecycle).
  Separate exes from cr_tests (the .so embeds cr_core -> ASan ODR if both
  link into one binary).
- Validation: 228/228 plain + ASan (leak-detect) green; C check exit 0;
  clang-format-18 clean. Bugs caught by the acceptance loop and fixed:
  u8_to_u16 ASCII->U+FFFD (every ASCII path mangled), subtree file-node
  no-op, snapshot-subtree root-walk dead branch.

**Parallel-collision resolution**: S5-CONT-4 landed e79d23f (independent G-01)
in the same window. Resolved on rebase to THIS implementation (the
UI-READINESS-PLAN-#1-conformant one): their variant invented CR_UI_CANCELLED
0x30000001 outside the cr::Err space, made zero-init configs invalid, and
used JSON where the plan mandates flat structs; their MSVC consumer-flag
guard and Win7 no-std::filesystem posture were absorbed; their
tools/cr_ui_abi_check.c (written against their header) was superseded by
tests/ui_abi_check.c. Their G-02/TREE-JSON worklog hash citations (28309b2 /
57403a0) do not resolve in main; the features themselves are code-verified
(fc656d2 / cmd_tree --json) and the register rows now cite the real hashes.

**Register maintenance (this push)**: G-01 CLOSED (2a93c45); stale rows
flipped with verified citations: P1-1 (464d8e5), P1-5 + BENCH-1..5 (837a543),
G-02/G-02b (fc656d2), TREE-JSON (code-verified). PARITY-DELTAS: D-24 updated
(shim half of the ADR-1 hedge landed) + D-45 added. README architecture
block + UI-READINESS-PLAN statuses updated.

---
Task ID: S5-CONT-5
Agent: Session-5 continuation (context-recovery line, main)
Task: Second workspace-reset recovery + restore green main: the two
shim-defect root causes (windows/codeql C2039, TSan data race) + the
W6 Win7 gate's designed first-run conscious amend.

Work Log:
- Workspace reset again at session start (second time; lesson reinforced:
  push ASAP). All 4 repos re-cloned; remote was at 1b2660a. Nothing
  uncommitted was lost this round. Post-recovery state reads: user
  reverted the agent@example.com trio (GAP-CLI-COMMANDS 77ef393, v0.3
  SDK bundle f97f5dd, gate reconciliation 317a911) at bd2e82b/ff3a538/
  0b3f029; session-6 landed the G-01 rework (2a93c45, OG factory-export
  binding, supersedes this lane's e79d23f - resolved on rebase per its
  worklog note; MSVC consumer-flag guard absorbed). CI red from 74f42c0
  onward (runs 39992119849..40007485773).
- Root-cause extraction from CI logs (40007485773): windows + codeql =
  cr_ui.cpp:2130 C2039 'is_elevated' is not a member of 'cr::win' - the
  session-6 shim's health_read_impl calls the GAP-P1-5 probe without
  including cr/win/cr_elevate.h (my probe; Linux never sees #ifdef _WIN32
  bodies). linux-tsan = REAL data race, scan_progress_bridge_ read
  0x...2e0 line 560 vs write line 562: the orchestrator fires the bridge
  from EVERY carve/pool worker, the shim's dedup pair (last_pm/
  last_stage, plain u32s, comment claimed "scan thread only") and the
  user callback were unsynchronized.
- Fix 1: include cr/win/cr_elevate.h in the _WIN32 block.
- Fix 2: scan_progress_bridge_ serialized under CrScan::progress_mu;
  strictly-monotonic emission (stale out-of-order deliveries dropped,
  sentinel excluded from the comparison). MY OWN regression suite
  caught the first draft's sentinel bug (0xFFFFFFFF participates in
  '<' -> zero progress ever fires; g_progress_calls==0 in both the
  gtest and the C consumer) - fixed to last_pm != 0xFFFFFFFFu guard.
- Fix 3 (TSan, second shim race surfaced after fix 2): run-thread ctrl
  creation (1103) vs any-thread cr_ui_scan_telemetry (1153) /
  cr_ui_scan_checkpoint_count (1195) on the unique_ptr pointer - the
  header documents telemetry "any thread, any time". Added
  CrScan::ctrl_mu serializing the pointer (creation block,
  telemetry_impl, checkpoint_count_impl; resume adoption completes
  before handle handout; progress-bridge read is pool-dispatch
  happens-after creation, no lock needed; lock order documented,
  no inversion with progress_mu / tel_.m).
- Win7 gate conscious amend (the designed path after 74f42c0's first
  visible run 39992119849 - evidence, not speculation): linker-verified
  additions UnhandledExceptionFilter / SetUnhandledExceptionFilter /
  IsDebuggerPresent / SRWLock family / ConditionVariable family /
  InitOnce family (all XP- or Vista-era, all Win7-safe; Win8-only
  WaitOnAddress family deliberately NOT added - the guard must bite);
  DLL name table corrected to the ACTUAL vcpkg-2026 names (z.dll,
  zd.dll, lz4.dll, zstd.dll; old spellings kept) + debug CRT + gtest
  names. cr_ui.dll now STAMPED + GATED in the windows lane (the SDK
  binary was previously neither - a (10,0)-versioned dll would refuse
  to load on Win7). release.yml now stamps + gates the shipped
  rrecov.exe (the driver-repo release pattern) - the artifact itself
  must satisfy the machine-enforced contract.
- tools/test_verify_pe_um.py: gate self-test (synthesized valid PE32+
  images): pass-path with the exact real MSVC import surface, stamp
  path ((10,0) rejected -> stamped -> passes, regression-tests the
  checksum math), three fail-paths (WaitOnAddress Win8-only, rogue
  gdi32.dll, strict user32). Wired into the linux lane so the GATE
  itself is machine-tested every push, not just exercised on Windows.

Stage Summary:
- Validation: plain 229/229, ASan 229/229, TSan 229/229 (was 7 failed),
  lint (clang-format 18.1.8) clean, gate self-test 6/6.
- Expect after push: windows/codeql compile unblocked; windows lane's
  gate step exercises the amended allowlist on real binaries; codeql
  vcpkg step was env/transient-class, watch it.
- REGISTERED follow-ups (NOT done in this push): (1) release bundle
  ships rrecov.exe alone - NO vcpkg runtime DLLs (z/lz4/zstd + CRT),
  the zip cannot run on a clean machine; the reverted trio attempted
  the SDK-bundle answer - the honest bundle composition needs an
  explicit decision (tracked, do not silently resurrect the revert).
  (2) Issues #11/#12 are stale (both items fixed by 0a28a23/f93f38e) -
  close with citations. (3) Release tag v0.3.0-parity recipe (PARITY-
  WRAP): now blocked only on windows-lane green + fuzz rerun over the
  W4-touched engines. (4) Driver repo 7e88e38/2b1db5f (classifier error
  87 fix + runner comparison: parity throughput, +52% hostile dispatch)
  - fold into the release notes when cutting.

---
Task ID: S5-CONT-5b
Agent: Session-5 continuation (same lane)
Task: Doc-wave closures (GAP-W5-CONTRACTS/-REGISTRY/-TODO, GAP-UI10-DOCS)
+ the GAP-W6-RELEASE release-workflow upgrade + release notes.

Work Log:
- Register audit found two of the four PARTIAL rows already satisfied
  (F11/F15 corrections present in error-fix-registry.md; the two counted
  TODOs are policy sentences). Closed as verified-stale / verified-zero.
- ground-truth-contracts.md: S2 cache-coalesced contract point added (the
  reader is the batching point - NTFS-2/D-4x); S11 UI binding surface
  added (og-parity wire statement, cr_ui ABI policy, threading contract,
  single-flight progress, G-02 snapshot semantics, G-12 query, the FROZEN
  tree --csv/--json schemas with the full numeric enum table). S3/S6
  verified already present.
- README capability summary + roadmap v0.3 truth + wizard CORE-CONTRACT
  S13 tree --json grammar sync (rda-wizard b2e32ae, cites core 1005d14 +
  the S11 normative table).
- Rejected-trio audit (f97f5dd): structure was reasonable, paths were
  never build-validated (its CI runs died at the 2a93c45 compile error).
  Re-implemented FRESH at HEAD with verified paths (cr_core/cr_win STATIC,
  cr_ui SHARED at src/{core,win,ui}/Release per the VS multi-config
  default) + the pieces the trio missed: app-local VC143 CRT + UCRT
  (Win7 RTM ships no UCRT - a bundle without it does not load on a clean
  Win7), existence-checked required artifacts, in-lane stamp+gate of BOTH
  shipped PEs, release body from RELEASE-NOTES.md.
- RELEASE-NOTES.md drafted: scope, bundle manifest, parity highlights
  (26/26 recycle, LZNT1, GPT salvage, EXCEEDS rows), UI surface, validation
  (229/229 plain/ASan/TSan, W4/WX, gate, fuzz, CodeQL, driver bench
  2b1db5f: throughput parity, +52% hostile dispatch, 64/64 byte-identical),
  truth-document links, honest-gaps section. Tag decision: v0.3.0-parity
  (v0.2.1-parity convention + the PARITY-WRAP recipe; the register's
  'v0.3.0-core' was the outlier).
- BENCHMARK-REPORT S2: the blocked-on register flipped to CLOSED-837a543
  (scoring validity wired; two-leg runs pending only machine availability).
- Register: GAP-W6-RELEASE -> IN FLIGHT (workflow landed; tag after
  windows+fuzz+codeql green).

Stage Summary:
- Pushed: 2dd6e01 (docs wave), this commit (release wave). Wizard:
  b2e32ae. CI on bb6a647: linux+tsan+lint GREEN (TSan race fixes proven in
  CI); windows/codeql/fuzz in flight at press time.
- Next: watch lanes to green on HEAD, then cut v0.3.0-parity (triggers the
  upgraded release workflow - the SDK bundle build gets its first live
  validation there), verify the release artifacts, flip GAP-W6-RELEASE to
  CLOSED, then #10 umbrella disposition.

---
Task ID: S5-CONT-5c
Agent: Session-5 continuation (same lane)
Task: The Win7 gate's designed convergence to green (the cr_tests +
cr_ui.dll iterations) - tracked evidence.

Work Log:
- bb6a647 windows lane: rrecov.exe PASSED the gate ('OK 145 imports') -
  the first amend (SRWLock/CV/InitOnce + real vcpkg DLL names) was
  correct. Lane died gating cr_tests.exe for the first time (the original
  74f42c0 run exited at rrecov and never reached it): 14 kernel32
  imports = the std::filesystem machinery the test fixtures legitimately
  pull (test_tmpdir.hpp / writer-dirs / cr_ui acceptance fixtures) +
  locale/format helpers. All XP- or Vista-era; added with reasons
  (46c2cf8). Self-test real-surface list grows the same 14.
- Pre-empted the next iteration: every MSVC-linked DLL imports
  DisableThreadLibraryCalls (CRT __DllMainCRTStartup) - added with reason
  BEFORE cr_ui.dll's first gate verdict (b4ee4f6).
- 46c2cf8 windows lane CONFIRMED the convergence: rrecov OK (145),
  cr_tests OK (250), cr_ui.dll flagged exactly and only
  DisableThreadLibraryCalls - prediction precise; b4ee4f6 carries the fix.
- codeql GREEN on bb6a647 + 2dd6e01 (the C2039 was breaking its build;
  the vcpkg env step was never the problem). fuzz GREEN everywhere.
- Driver-repo commits audited (7e88e38 classifier error-87 fix: the null
  guard inverted the reference's Chars==0 global gate - regression tests
  added; 2b1db5f runner comparison: throughput parity, +52% hostile
  dispatch, 64/64 byte-identical verify). Both consistent with the
  release-notes claims.

Stage Summary:
- Gate state after three designed iterations: rrecov PASS, cr_tests PASS,
  cr_ui.dll expected PASS on b4ee4f6 (in flight). The Win8-only guard
  (WaitOnAddress et al.) deliberately absent and the self-test proves it
  still bites.
- Next: b4ee4f6 windows lane green (incl. the MSVC 229-test step that
  runs for the first time post-G-01-rework) -> cut v0.3.0-parity.

---
Task ID: S5-CONT-5-FINAL
Agent: Session-5 continuation (final wave of this lane)
Task: The last CI red -> full green, the WinDestIo Windows recovery
defect, the issue-tracker truth pass, and the v0.3.0-parity readiness.

Work Log:
- ccc1dca: WinDestIo::mkdir drive-chain fix - the ACCESS_DENIED-vs-EEXIST
  class defect (CreateDirectoryW fails on drive-root-shaped prefixes
  "C:"/"C:\Users" with non-ALREADY_EXISTS codes though the component
  exists as a directory) made EVERY Windows recovery into a nonexistent
  drive-letter destination fail (MSVC #227: 4 errors / 0 done - the
  only WinDestIo exercise in CI; test_writer_dirs uses a test-local
  FsIo). Fix: exist-as-directory beats any create error (the POSIX
  EEXIST contract). New WIN32-only cr_destio_tests target (5 contract
  tests incl. deep chains, FILE-in-the-way, atomic lifecycle, cancel).
  MSVC PROOF: gate 3x OK (145/250/125 imports) + 230/230 tests, run
  40018530391.
- Register truth pass: GAP-CI-2 CLOSED (fuzz/codeql/windows evidence);
  GAP-W6-WIN7 flipped to CI-PROVEN with the three-iteration convergence
  record.
- Issue-tracker truth pass: all ten UI-integration issues closed with
  commit-cited evidence - #1..#9 focused (G-01..G-13), #10 umbrella
  with the full G-11..G-23 disposition table; G-22's beyond-og remainder
  (session-persisted digests) explicitly registered as an enhancement,
  not a parity debt (the og neither verifies nor stores hashes).
- v0.3.0-parity readiness: RELEASE-NOTES.md (truthful: scope, bundle
  manifest, parity evidence, validation state incl. driver bench 2b1db5f,
  honest-gaps), release.yml (SDK bundle + app-local CRT/UCRT + stamp+gate
  of both PEs + existence checks), tag recipe per PARITY-WRAP - cut on
  fully-green HEAD (windows+linux+tsan+lint+fuzz+codeql).

Stage Summary:
- HEAD ccc1dca: windows/lint/linux/linux-tsan/fuzz GREEN; codeql in
  flight (expected green - same code passed at bb6a647/2dd6e01).
- Suite truth: 229/229 plain + ASan + TSan (Linux), 230/230 MSVC /W4 /WX,
  Win7 import gate green on all three shipped PEs, fuzz zero-crash.
- Next: codeql green -> tag v0.3.0-parity -> release workflow live
  validation (SDK bundle build) -> verify artifacts -> flip GAP-W6-RELEASE
  CLOSED -> session handoff notes.

---
Task ID: S5-CONT-6 (session close)
Agent: Session-5 continuation (final)
Task: v0.3.0-parity release - live, verified, production-clean.

Work Log:
- Full six-lane green at 849f0f9 (linux, tsan, windows 230/230, lint,
  fuzz, codeql) - first fully-green HEAD since 74f42c0's gate wave.
- Tag v0.3.0-parity cut per the PARITY-WRAP recipe (notes referencing
  CORE-GAP-LIST + PARITY-DELTAS + BENCHMARK-REPORT).
- The release workflow's live validation loop (the first real SDK-bundle
  runs) caught four defects in sequence, all fixed with evidence:
  1. SDK 26100's UCRT moved bin->Redist/ucrt layout AND the bin-dir
     exists-but-empty trap -> content-filtered resolution across three
     SDK-generation layouts.
  2. PowerShell multi-line @() trailing-comma parsed into -ErrorAction
     -> unambiguous accumulation.
  3. Name-sort picked the empty bin-dir over the populated Redist ->
     Where-Object {Test-Path ucrtbase.dll} content filter.
  4. gtest/gmock DLLs in the production bundle (test-tree deployment
     backstop bled into the release-dir scan) -> excluded by pattern;
     PDB claim in the notes corrected to what Release actually ships.
- RELEASE LIVE: tag v0.3.0-parity at 2140eb4, release run 40031221660
  success. Bundle verified by download + enumeration: 117 entries,
  3.94 MB - bin/ (rrecov.exe 674KB, cr_ui.dll 684KB, z.dll/lz4.dll/
  zstd.dll, vcruntime140*/msvcp140* family, ucrtbase.dll + the full
  api-ms-win-crt-* set = the app-local Win7 story), lib/ (cr_ui.lib
  import 14.5KB + cr_core.lib 4.5MB + cr_win.lib), include/ (53
  headers), docs/ (26), LICENSE/README/RELEASE-NOTES, SHA256SUMS over
  all 116 files. Body = the honest notes.
- GAP-W6-RELEASE flipped CLOSED with the full evidence chain.
- Issue-tracker: #1-#10 all closed with commit-cited evidence; #11/#12
  closed as resolved-stale earlier this session. Zero open issues.

Stage Summary:
- The session deliverables are landed: green CI on every lane, the
  Win7 posture machine-enforced AND CI-proven on all three shipped PEs,
  the TSan races fixed (shim), the Windows recovery defect fixed
  (WinDestIo), the SDK+CLI bundle shipped with self-contained runtime,
  and the release register CLOSED.
- Registered remainders (all enhancement-class, not parity debts):
  G-22 session-persisted digests (beyond-og), ZFS multi-vdev scoping
  (D-09), HFS+ journal replay / Btrfs tree-log (D-L), two-leg og
  comparison pending benchmark-machine availability (harness wired,
  scoring valid since 837a543).

---
Task ID: BENCH-SESSION-7
Agent: bench session 7 (comparative-bench takeover)
Task: Take over the failing comparative bench (10-commit failure streak), drive it to green with evidence-grade artifacts, ours vs og EaseUS DRW free.

Work Log:
- Full failure forensics on runs 40045370477..40064925245 (artifacts + logs + screenshots):
  * BENCH-F1 (torn checkpoint): hard-killed scan leaves a header claiming N records
    with the Nth torn (chain/crc unpatched); Reader::open exposed the header count,
    read_record(N-1) -> 0x50040001 'session format error' on --resume, defeating
    the documented F10 salvage contract. Fixed by clamping count_ to the indexed
    prefix + unit test (runs 1..11 of the streak all failed here).
  * nodriver probe: '[transport]' lines go to scan STDOUT, not the --log file; the
    probe grepped the wrong stream (and tree 'session load failed' error text was
    miscounted as a readable 1-row tree). Both probe checks fixed.
  * og volume selection (run 40048192077 evidence chain): (a) the periodic
    RetentionActiveDlg nag (corpus atlas #15: every 30-60 s on ANY page) sat over
    the landing tiles and ate clicks - the fixed (809,135) X-click missed it and
    the blind ESC PROVOKES the exit-retention gate; fixed with census-based
    per-dialog X-dismissal + WM_CLOSE fallback, no blind ESC. (b) Tile geometry
    re-measured by pixel analysis + 6x-zoom VLM cross-checks: row-1 cards
    y175-271 (C: left x310-581, D: right), BENCH(E:) is a white tile at
    x310-580 y310-440 (center (445,370)); the old primary (440,455) clicked empty
    page below the card. (c) stop-confirm (512,420) missed the dialog buttons
    (pixel-measured Stop=(439,473)/Continue=(565,473)) so the dialog persisted and
    swallowed all later attempts; wrong-tile recovery now clicks the real Stop and
    RESTARTS the app (deterministic, dialog-drift-immune).
- F-1 (the other lane's finding, adopted after their handoff note in f571e83):
  complete root-cause + fix. Extents were slice-relative end-to-end while
  cmd_recover reads the raw disk (16 MiB backward shift; 0/584 hash matches incl.
  0/8 live keep controls; 593 ERR_TRUNCATED). Implemented the documented
  merge-time rebase: ResultTree::rebase_new_extents/rebase_subtree_extents with
  the NODE_FLAG_EXT_SHIFTED idempotency guard, stage-boundary rebases before each
  G-13 checkpoint, per-candidate subtree rebases after the pool joins, container
  flag CRSF_FLAG_ABS_EXTENTS + legacy-session load-time normalization, UI
  recovery/preview parent-disk unwrap. 223/223 plain+ASan+TSan (3 new
  ExtentBasis tests). Commits 49af5c2 (bench fixes) + cd9ea16 (F-1).

Stage Summary:
- Runs dispatched: push cd9ea16 (ours smoke, F-1 validation) + tools=both dispatch
  (og leg first exercise with the fixed driver). Full both-legs matrix after both
  are green.
- Registered remainders: LDM layout-relative extents (recover from LDM scans was
  already misbased pre-F-1; needs per-node multi-disk bases - follow-up), mid-stage
  snapshot extent basis for pool-added nodes (remnants/carve are rebased at stage
  end; mid-stage UI previews of those nodes can shift - end state always correct).
