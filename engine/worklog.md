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
