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

---
## Session: comparative bench buildout + first green both-legs smoke (main lane)

Task: build and run the fully instrumented comparative harness (mandate: 6 sections, no mocking, no masking). Parallel with the "bench session 7" lane (their commits: 49af5c2..85c0a5e + engine fixes); both lanes share this worklog.

Work Log (main-lane commits 144c71d..58d07f7, 19 commits):
- Built the bench/ harness from scratch (19+ files): seed v2 generator (deterministic 0x5EEDC0DE, 22 types, attribute/depth/CJK/long-path/fragmentation matrix, extents+MFT-record enrichment, seedb tagging for dual-volume delete_partition); volume.ps1 composite raw ops (lock/dismount/FSCTL retrieval pointers/NTFS data/MFT+$Bitmap corruption/partial overwrite/1MiB hash maps); apply-scenario (12 deletion modes incl. decay 50/80/100); run-ours (3 scan modes + recover --report + sampling + transport assertion + preservation probes); run-behavior (nodriver/cancel/resume); og-driver + run-og (DRW.exe coordinate driver, log-length-offset 'Scan drive:<letter>' verification, VLM-verified BENCH(E:) tile (440,455), lost-partition fallback, recover funnel); metrics (inventory/compute/compare/regression-gate/vlm-extract/vlm-crosscheck); pe_info.py; comparative-bench.yml (fs x scenario MATRIX - one scenario per job so artifacts survive job timeouts; pinned action SHAs; PR=ours-smoke; og on dispatch/nightly only; 90d retention; no continue-on-error anywhere).
- Deleted the broken predecessors: benchmark.yml (stray brace, 5 consecutive failed dispatches), comparison-ours.yml, comparison-easeus.yml.
- 13 CI failure iterations, each root-caused from logs+artifacts: ordered-cast, [byte[]]() empty parens, split method calls, C# usings, CreateProcessW charset, bare dash remnant, Math overload binding, $VAR: interpolation, JSONL pretty meta, FileStream-owns-handle, og log grammar (Scan drive:E is its own line + vendor QA lines in corpus -> log-offset), D: disk headroom (dynamic VHDX growth failure -> C:\bench), FSCTL_LOCK GLE=5 retry.
- FINDINGS registered (cross-lane): F-1 extent-basis 16MiB shift (0/584 hash matches incl. live keeps; volume->disk recover; fixed by session-7 lane cd9ea16 after my A/B isolation runs 40064925245 driver-absent + disk-target); F-2 hard-killed .crsf resume format error 0x50040001 (tree reader tolerates; fixed via BENCH-F1 salvage); BENCH-OG-F1 funnel ESC-cancel of the slow destination dialog (fixed, redrive logic); BENCH-F3 retrieval-pointers 4-byte shift (session-7); BENCH-F4 non-NTFS seed check + lock GLE=5 (session-7).
- FIRST COMPLETE BOTH-LEGS SMOKE: run 40073093997 (all 6 scenario jobs + compare green; ours-behavior outstanding). Real numbers: ours permanent_delete recall 87.2%/prec 30.3%; og quick_format 51.5%/75.7%; ours quick_format 0.0% (pre-transport-fix, re-measure pending); delete_partition 43.0 vs 42.7 (different failure modes: ours discovery-limited, og cap-limited).
- VLM protocol executed on 198 og screenshots (glm-5v-turbo, fixed template, confidence flags): key-screen extractions + crosscheck committed under bench/vlm-evidence/40073093997-smoke/ (4 CORROBORATED, 1 CONTRADICTED - delete_partition UI counts one lost partition while og-found.csv spans both, 3 UNCROSSCHECKABLE).
- Verdict basis corrected to RAW og recall (attempted-set is degenerate 1.0 under a hard cap).
- baseline-candidate.json generated from 40073093997 ours metrics (human-review gate preserved; bench/baseline.json untouched).
- Full-matrix dispatches (5+) repeatedly cancelled by an external canceller (same actor cancels push runs of this workflow too; recorded, not worked around - no masking either way).

Stage Summary:
- Deliverables: comparative-bench.yml green on workflow_dispatch (smoke, tools=both); versioned seed corpus + generators; 12-mode deletion scripts; og automation (coordinate driver + rsf harvest + cap funnel + FlaUI probe); docs/COMPARATIVE-BENCHMARK.md + benchmark-summary.json generated per run by compare.py; regression baseline candidate + gate; VLM evidence bundle; this handoff.
- Known environment limitations: external run canceller blocks long full-matrix runs (retry when quiet; nightly schedule also wired); ours-behavior job still red pending the resume-salvage revalidation; FlaUiProbe fails to build in CI (recorded, auxiliary only).
- Top BELOW/open items for the next session (root-causes.json carries full detail): quick_format ours 0.0% re-measure post-d763cdb; delete_partition dual-partition discovery (ours finds one of two); permanent_delete precision (MetadataOnly zero-fill FPs, 991 salvaged); keep-control 0/8 on volume scans (enrichment path fixed in c1982dd, revalidate); og permanent_delete re-measure post BENCH-OG-F1 (the 0.0% was an automation defect).

---
## Session: diff-to-fix parity audit (read-only roadmap session)

Task: Full read-only parity audit of core_recovery @ 064fcf44ba09823c8089cbfe0c53254fed7e351c against easeus-drw-research @ 03ad46ccd6086000e53d7d3871ef43cd7f32e462. Deliverable: diff-to-fix/ directory (findings, module verdicts, cross-ref matrix, evidence, tracked-open). No source files modified.

Work Log:
- T1: Cloned both repos; recorded commit hashes; saved redacted prompt to worklog/session-diff-audit-prompt.md; created diff-to-fix/ skeleton; this worklog entry.
- Next: parallel context-sheet reads (agents A-E), then PARAMETERS/SEVERITY/README/SUMMARY skeletons, then module-by-module audit waves.

---
Task ID: CORE-FINAL-1
Agent: core-final session (main lane)
Task: T1-T3 gate phase - read both repos line-by-line where it matters, verify every open finding against code + corpus decompiles + independent probes, produce the session-6 gate documents (CORE-GAP-LIST session-6 register, ABI-CONTRACT, PERFORMANCE-PROFILE), push before any code change.

Work Log:
- Pulled both repos (core @064fcf4, corpus @03ad46c); token kept in git credential store only (never committed, never logged).
- Saved the redacted session prompt to worklog/session-core-final-prompt.md; read twice.
- Read the truth docs end to end: CAPABILITY-MATRIX, CORE-GAP-LIST (all prior waves), PARITY-DELTAS roll-up, UI-READINESS-PLAN, BENCHMARK-REPORT, worklog.md tail (bench sessions 7 + comparative lanes).
- Established the local baseline: linux-gcc (ASan) 225/225 in 8.46 s; linux-release 225/225 in 3.04 s; QueryPerf 362 ms cold (1M nodes); engine suites 22 ms.
- Verified every open bench finding against the code AND the corpus decompiles:
  * FAT32 deleted-LFN (GAP-S6-1): code shows lfn_checksum over 0xE5 bytes (always fails for deleted); INDEPENDENT PROBE (mtools FAT32 image, mdel, raw dentry dump) proves deletion 0xE5's byte 0 of every entry - for LFN slots that is the SEQUENCE byte, name chunks fully intact, per-slot checksums equal; brute-forcing the 8.3 first byte against the stored checksum is deterministic (bijection). Probe image kept at ../probe/fat32.img.
  * FAT32 deleted-chain (GAP-S6-2): reconstruct_deleted_chain stops at first FREE entry -> 1-cluster extents -> zero-filled plausible-size files (46/10143 hash-true).
  * ReFS real-volume refusal (GAP-S6-3): our gate uses boot.minor where the og (FUN_180012650 decompile) gates on the checkpoint BODY minor (*(ushort*)(body+6) < 0xE); body offset also dynamic (0x20 + ref size) in the og vs our hardcoded 0x50.
  * quick_format (GAP-S6-4): remnant path emits $FILE_NAME only, never parses $DATA runlists (cr_mftwalk.cpp:1109-1194).
  * ntfs decay cliff (GAP-S6-5): registered root-cause hypothesis CONTRADICTED by code (extents emit regardless of grade; recover-all enqueues regardless of grade) -> corrected to physical-destruction analysis + physical-ceiling bench metric action.
  * delete_partition (GAP-S6-6): orchestrator scans all candidates (verified) -> defect bounded to discover's nested-same-fs dedup + the device-tail fallback size (cr_candidates.cpp:96-98,247-251,260-273).
  * MetadataOnly zero-fill FPs (GAP-S6-7) + resume carve dedup (GAP-S6-8): confirmed as registered.
- Surveyed the corpus for the visualization model: AnalysisGridWidget (54 v-methods, sector-grid painting, UIUX/08 + 03 + 04) fed by INotifyObserver -> CMessageProxy -> doDispatchMessage(int,void*,int&) (MOM-modules §4, UIUX/13 §3) + IBuildVolumeBitmap shared interface (SearchDLL family §2). The core's job: geometry/allocation/coverage/confidence/bad-sector producer - documented in ABI-CONTRACT §7.
- Verified absent at HEAD: SQLite layer (0 engine hits), typed event stream, disk-map surface, docs/ABI-CONTRACT.md, docs/PERFORMANCE-PROFILE.md, 64 KiB direct-I/O envelope (transport is 512-sector aligned).
- Wrote the three gate documents: docs/CORE-GAP-LIST.md session-6 register (GAP-S6-1..15 with closure order), docs/ABI-CONTRACT.md (normative: conformance, ownership table, threading, error model, lifecycle flows, typed event schema §6, disk-map model §7, SQLite sidecar §8, stability policy §9), docs/PERFORMANCE-PROFILE.md (local first-hand baselines + bench-run baselines + 64 KiB constraint + re-measure list).

Stage Summary:
- Gate phase complete per mandate §1 (docs before code). 15 session-6 GAP rows registered, every one evidence-cited (code line + corpus artifact or local probe).
- Next: P0 engine wave in closure order - GAP-S6-1 (FAT LFN brute-force), GAP-S6-2 (FAT contiguous chain), GAP-S6-3 (ReFS body-minor gate), GAP-S6-4 (remnant $DATA extents), each with fail-before/pass-after tests.
---
Task ID: 2-C
Agent: C (core public surface reader)
Task: Exhaustive read of core headers/build/CI/docs/CLI for diff-audit context sheet C

Work Log:
- Read all 53 headers under include/cr/ line by line (root cr_types/cr_bits/cr_error/cr_parallel/cr_ui + storage, ntfs x7, fat x2, exfat x2, ext4, hfsplus, btrfs, zfs, refs x2, table x3, tree x2, carve x3, session x2, lost, orchestrator, query, preview, crypto, compress, recovery x2, repair, health, win x7). Recorded every public type/enum (exact numeric values), function signature, callback type, and stated ownership/lifetime contract.
- Read build system: CMakeLists.txt, CMakePresets.json, vcpkg.json, cmake/DeployVcpkgBinaries.cmake, src/{,core/,app/,ui/,win/}CMakeLists.txt, tests/CMakeLists.txt, fuzz/Makefile. Captured targets (cr_core STATIC PIC, cr_ui SHARED hidden-visibility, cr_win STATIC, rrecov, cr_tests/cr_ui_tests/cr_ui_abi_check/cr_destio_tests), sanitizers (ASan preset, TSan CI job, fuzz fuzzer+address+undefined), the 13 fuzz targets and their run budgets, the Win7 posture (_WIN32_WINNT=0x0601 + post-link stamping), and the required-codec policy (zlib/lz4/zstd FATAL if missing, FB-X01).
- Read .github/workflows/ci.yml (lint/linux/linux-tsan/windows + fuzz and codeql both if:false bench-session skips), comparative-bench.yml (prepare/bench-ours/ours-behavior/bench-og/compare; matrix fs x 12 scenarios; no continue-on-error; RDA driver v0.2.1), release.yml (SDK bundle + CRT/UCRT + stamp/gate + SHA256SUMS).
- Read tools/verify_pe_um.py (PE32+ 6.1 gate + strict kernel32/advapi32 allowlists + name-only DLL gate), tools/stamp_osversion_um.py, tools/ref_imports_um.json (27 kernel32 + 6 advapi32 functions, exact list extracted), tools/validate_report.py (report schema v1 enforcement).
- Read README.md, RELEASE-NOTES.md (v0.3.0-parity), LICENSE (MIT).
- Read docs/: full read of CAPABILITY-MATRIX, architecture, requirements, ground-truth-contracts, crsf-format, recovery-report-schema, error-fix-registry, PARITY-DELTAS (D-01..D-45 structure + verdict roll-up + section J + D-45), CORE-GAP-LIST, PARITY-AUDIT; full read of all 7 docs/parity/PARITY-*.md (+ draft headings); structural pass over build/testing/roadmap/benchmarking/UI-READINESS-PLAN/PARITY-REFACTOR-PLAN/ADR-UI-SNAPSHOT; full read of BENCHMARK-REPORT.
- Read all 7 root AUDIT-*.md fully (AUDIT, AUDIT-NTFS, AUDIT-FAT, AUDIT-ARCH, AUDIT-LINUX-FS, AUDIT-PIPELINE, AUDIT-FALLBACKS): recorded finding-ID schemes (F-A1-*/F-A2-*/F-A3-*/F-A4-*/F-B-*/F-C-*/F-D-*, NTFS-*/FAT-*/EXFAT-*/BTR-*/REFS-*/E-*/HFS-*/DC-*, ARCH-*, FB-*, WRITER-*/HEALTH-*/CARVE-*/SESS-*/PIPE-*), dead-code registers, UNVERIFIED registers, and open-vs-fixed states.
- Read worklog.md head (100) and tail (200); read worklog/session-05-prompt.md, session-parity-prompt.md, session-parity-refactor-prompt.md fully (project history: session-5 verification, parity audit, parity refactor, bench sessions, current diff-to-fix session).
- Read bench/README.md and bench/root-causes.json (7 open root-cause entries: delete_partition discovery, permanent_delete MetadataOnly zero-fill FPs, quick_format names-only, resume carve-dedup, FAT32 names+content, ReFS real-volume ERR_BAD_MAGIC, NTFS decay_80 collapse).
- Read src/app/rrecov.cpp (1498 lines) fully: 7 commands, every flag, exit codes (scan 0/1/3, list 1, recover 0/1/2, health 0/1/3, fix 0/1/2, bench 0/1/2), Ctrl-C/save-on-cancel, LDM cross-disk resolver, checkpoint sink, [F12] arming, frozen --csv/--json schemas.
- Read src/ui/cr_ui.cpp (2265 lines) fully: all 57 exports, ImageFileDevice, portable FileDestIo, progress bridge single-flight monotonic, F-1 parent-disk unwrap for preview/recovery, ERR_UNSUPPORTED platform truths.
- Listed scripts/ (20 files); read compare_report.py, compare_og_rsf.py (og .rsf record-4 grammar + content-type table), gen_rawsigns.py, rematch_inventory.py, bench_score.py (GAP-BENCH-1..3 fixes encoded).
- Wrote /home/z/my-project/scratch/context-sheets/C-core-public-surface.md (single file, ~560 lines, 11 sections: file inventory, per-header API surface with exact enum values, module map, build system incl. exact import allowlist, CI pipeline, report/session formats, documented parity state per doc, CLI surface, known-issue inventory, project-admitted divergences with citations, 14 open questions).

Stage Summary:
- Files read: 53 headers, 9 build files, 3 workflows, 4 tools, 3 root docs, 7 root AUDIT docs, 7+1 parity docs, 10 fully-read docs + 8 structural, worklog.md + 3 prompts, bench README + root-causes.json, rrecov.cpp, cr_ui.cpp, 20 scripts listed / 5 read.
- Key outputs: scratch/context-sheets/C-core-public-surface.md. Highlights for the audit: the project's own open-issue register is large and specific (7 bench root-causes incl. ReFS real-volume rejection and FAT32 deleted recovery both-broken; D-09/D-30 NEEDS-WORK deltas; CAPABILITY-MATRIX vs CORE-GAP-LIST conflict on GAP-P1-1; ARCH-1 RdaClient dead-surface disposition; 14 open questions listed for verification waves).
- No source files modified; no commits made.
---
Task ID: 2-E
Agent: E (independent verification probes)
Task: Binary-level verification probes on corpus + core standalone-compilation check

Work Log:
- Tool availability survey: python3 3.12.15 (pefile 2024.8.26 installed into venv via `python3 -m pip install pefile`), objdump/readelf/strings/nm (binutils 2.44, pei-x86-64 supported), file 5.46, od 9.7, gcc/g++ 14.2.0, make 4.4.1; NOT available: 7z/7za, radare2/rabin2, xxd, clang/clang++, cmake, wine, pwsh. zlib/lz4/zstd dev headers present.
- PE surface dump (scratch/probes/pe_dump.py): 38 binaries (9 SearchDLLs, DRW/DRWUI/DRWService, 21 PartitionRecovery DLL/EXE/SYS, libvmdk, BUILDPE drivers epmdkdrv/ebrntdrv/EUEDKEPM/EUDCPEPM, EuShare EuMrx.sys). Per-binary dumps <name>-pe.txt with sections/directories/exports+ordinals/imports/version-info; summary.tsv + all-exports.txt. Confirmed .mo files are PE32+ DLLs (MOM modules).
- Export surface: every FS SearchDLL exports exactly 2 factory functions (EUCreate<FS>QuickSearcher + EUCreate<FS>Searcher; Btrfs full variant misspelled EUCreateBtrfsComplateSearcher); RAWSearchDLL exports 6 (adds EUGetAnalyserList/Map/Multimap + GetRawUserInfoMap + EUCreateRAWSearcherForCellTest); DRWUI exports 9 (C++ IObject); DRW.exe/DRWService.exe export 0.
- Import analysis: storage API surface per binary (CreateFileW/DeviceIoControl/DefineDosDevice/GetLogicalDrives/FSCTL-capable sets); found shared exported virtual `?DeviceIoControl@IStorageDevice@@UEAAHKPEAXK0KPEAK@Z` across MainModule/ToolsModule/PartitionRecovery/RecoveryModule/UILogic/RawFixer/peformat; DRWUI pulls NTFS USN/MFT FSCTLs (0x90064/0x90068/0x900B0/0x900BB); PartitionRecovery writers use SET/DELETE_DRIVE_LAYOUT_EX (0x70140/0x7C1D0; DELETE also inside NTFSSearchDLL @0x72378).
- IOCTL census (scratch/probes/ioctl_scan.py -> ioctl-census.txt): byte-scanned 31 binaries for 60+ codes; objdump-disassembled eprdtdrv.sys DEVICE_CONTROL dispatch (eprdtdrv-dispatch-disasm.txt) proving custom codes 0x60002400/0x60002404/0x60002408 (0x2404 computed via `sub $0x60002400` + `cmp $4`, out-buffer >= 0x208) + passthrough via IoGetAttachedDeviceReference/IoBuildDeviceIoControlRequest — matches re-workspace artifacts exactly. EUDCPEPM.sys: all 15 custom codes 0x800C2AC8..0x800C2B38 present as jump-table dwords @0x159e-0x190b; IoctlBase=0x10000 confirmed in BUILDPE regdrv.reg (DeviceName epmdiskcopydrv, volume-class UpperFilter). EuRdr codes 0x140320..0x140644 confirmed in DRWUI (7 literals) + EuMrx.sys (2).
- raw_signs.json full parse (raw_signs-structure/stats/grammar.txt): dict of 139 formats; schema {header:{signs:[..]}, tail:{signs:[..]}?, min, max}; sign grammar "<offset>,<HEX>,<score>" (261 signs, 199 header + 62 tail, 46 formats with tails, scores 0.50-1.00, zero wildcards); size gates min 1/100/1024/10240, max 51200/204800; UTF-16 magics embedded as hex (zdct, tbres); jpg/png/pdf/mp4/zip/office ABSENT -> common formats are hard-coded carvers in RAWSearchDLL, raw_signs.json is the long-tail layer.
- FileFilter.xml parse (filefilter-structure.txt): 8 FileCategory nodes, 9 FileType nodes, 1489 extension entries (1481 unique); tag misspelled <Extention>; Graphic 320 / Document 432 / Email 19 / Audio 281 / Video 217 / Other 0 / Archive 219 / Bookmarks 1.
- Strings probes (strings-probes.txt/-probes2.txt with exact offsets): NTFSSearchDLL UTF-16 system files ($MFT@0x6c130, $MFTMirr@0x6c140, $Bitmap@0x6c120, $BadClus@0x6c0e0, $LogFile@0x6c158, $Volume@0x6c0c0, $Secure@0x6c0f8, $UpCase@0x6c090, $Extend@0x6c170) + ASCII $Boot/$UsnJrnl/$Recycle/$RmMetadata + "FILE0"@0x71288 + PDB paths; RAWSearchDLL RTTI census: 199 classes, carving families BruteForce(30)/RawSpeed(30)/Brance(23)/RawFormat_xjq(12) + global(71) + FileZIP/FileOLE + tinyxml2; "raw_signs.json"@0x69630 + key "signs"@0x69640; DRWUI stage strings (Deep scan StartSector=...@0x12429d8, Quick/Advanced Scanning%1, Files Found:, DeepScanning, %s\FileFilter.xml); ReFSSearchDLL "ReFS"@0x18a08 + refs RTTI namespace; ZfsSearchDLL uberblock magic 0x00bab10c x6 + vdev_tree + "checksum can't pass".
- 7z/archive test: no 7z on Linux; PartitionRecovery dir is plain; static magic-scan: SetupUE.exe = 32-bit EaseUS bootstrapper (no embedded archive; _cabs + PDB), unins000.exe = Inno Setup, SaveScan/*.rsf = opaque binary session files (7.4/52.7/9.0 MB), BUILDPE/EaseUS-x64 = flat WinPE payload. No Windows binary executed.
- Driver family byte-diff: eprdtdrv.sys vs eprdtdrv_win7older.sys differ in 9703 bytes; eprdtdrv_win7older.sys == BUILDPE ebrntdrv.sys (identical md5 73ddde6e...); epmdkdrv.sys third distinct build. Device.mo holds \\.\EPRDTDRV\%d\Volume\%d / \\.\EPRDTDRV\%d\%d grammar + WMI MSFT_* namespaces.
- Core standalone build: g++ -std=c++20 -fsyntax-only -Wall -Wextra over all 45 src/core cpp files -> 45/45 pass; full compile+ar into scratch/build/libcr.a (35.3 MB), linked -lz -llz4 -lzstd -lpthread; zero Windows headers in src/core. fuzz/Makefile dry-run OK but libFuzzer needs clang (g++ rejects -fsanitize=fuzzer); cmake absent so CMakePresets not configurable here.
- Fixture probes (scratch/build): python-built GPT fixture (protective MBR + LBA1 header + CRCs + backup) -> core parse_disk_table: layout=GPT primary_ok=1 backup_ok=1, names/usable-range correct. Extended 8-GUID fixture exposed GPT type-GUID classification defects in include/cr/table/cr_table_wire.h: GPT_TYPE_BIOS_BOOT spells "Hah!IdeoNeedEFI\0" (canonical "Hah!IdontNeedEFI"); GPT_TYPE_MICROSOFT_RESERVED holds the EFI-System GUID in big-endian textual order (wrong GUID + wrong order); GPT_TYPE_LINUX_FS is a mangled hybrid (decodes 0657FD6D-C439-7541-86DF-0E49CEB2D19F, matches neither 0FC63DAF Linux-FS nor swap); GPT_TYPE_LDM_METADATA byte5 0x7E vs canonical 0x8E. Empirically: ESP/MSR/BIOS-boot/Linux-FS/LDM-metadata entries all classify as Other; only BasicData->Ntfs, LdmData->Ldm, swap->Other correct. MBR+EBR fixture: primaries + both logical volumes emitted correctly (relative-EBR semantics verified); noted salvage gap: a broken EBR link (ERR_BAD_MAGIC) makes the caller discard already-walked logical volumes (only OK/ERR_CYCLE_LIMIT kept).
- Deliverables written: scratch/context-sheets/E-verification-capabilities.md (tool matrix, capability verdicts, executability verdict, open questions) and scratch/context-sheets/E-probe-results.md (all results with commands/offsets). No repo files modified; no commits.

Stage Summary:
- Probes run: 38 PE dumps, 31-binary IOCTL census + dispatch disassembly proof, raw_signs.json full grammar decode (139 formats / 261 signs / offset,HEX,score), FileFilter.xml structure (8 cats / 1489 exts), strings+RTTI probes on 5 binaries with exact offsets, driver trio byte-diff, static installer ID, full standalone g++ build of core (45/45) + execution of parse_disk_table against 3 synthetic fixtures (GPT happy, 8-GUID matrix, MBR+EBR chain).
- Key results: engine surface = 2 factory exports per FS engine (RAW 6); custom driver IOCTLs 0x60002400/04/08 + EUDCPEPM 0x800C2AC8-0x800C2B38 + EuRdr 0x140320-0x140644 verified at byte/disasm level; carving families BruteForce/RawSpeed/Brance/RawFormat_xjq confirmed as RTTI namespaces; raw_signs.json is long-tail-only (no jpg/png/zip...); core fully Linux-compilable and its GPT/MBR/EBR parsing verified live; NEW FINDINGS for diff-audit: 4 wrong GPT type-GUID constants in cr_table_wire.h (BIOS_BOOT/MSR/LINUX_FS/LDM_METADATA) causing ESP/MSR/BIOS-boot/Linux/LDM-metadata misclassification, and EBR-chain salvage gap discarding partial logical volumes on mid-chain ERR_BAD_MAGIC.
- Not verifiable here: empirical og execution (no wine/Windows; bench/og PowerShell harness), Ghidra-grade new decompiles, libFuzzer campaigns (no clang), Authenticode validation, .rsf live round-trip.

---
Task ID: 2-A
Agent: A (corpus architecture & behavior reader)
Task: Exhaustive read of corpus architecture/behavior docs for diff-audit context sheet A

Work Log:
- Read all 9 architecture/*.md docs fully (00-INDEX, 01-build-tree, 02-kernel-drivers, 03-winpe, 04-recovery-workflow, 05-scan-engine, 05-uiux, 06-repair-chain, 07-user-flow).
- Read re-workspace/README.md + artifacts/CORRECTIONS.md (8 errata rows extracted verbatim).
- Read DRWUI-exe--main-ui-architecture.md, DRWUI-exe--thread-architecture.md, DRWUI-subsystems--assistant-grading-preview.md (grading engine + category IDs + preview protocol), plus DRW-exe--launcher.md and DRWService-exe--bitlocker-service.md for launch/IPC semantics.
- Read all 5 SaveScan/ artifacts fully (rsf-format master + record4 grammar + ifileproperties parts 1-3) — extracted every constant, offset, GUID, grammar rule.
- Read all 6 config-files/ artifacts (config-data-reference, drwcfg AES cache, filtering-priority layer, qa-logs session forensics, userdata-remoteconfig, vendor-logs forensics).
- Read all 38 UIUX/*.md artifacts fully (01-35 + 5 dynamic-ground-truth files).
- Read all 10 runtime-families/*.md; read per-binary census files for DRW, DRWService, DRWReport, DRWUI, legacy, deepening-batch3.
- Parsed og-codebase config artifacts directly: raw_signs.json (139 ext, key structure, confidence range, examples), FileFilter.xml (8 categories, 1489 elements / 1481 unique — reconciles CORRECTIONS #5), RawConfig.ini (34 toggles), recoveryFree.ini (hexdump), DataFile.ini, auto.txt, needcopy.txt, dlconfig.xml (61 OBJECTs), PEtools.xml, config/config.ini + version.ini + nasconfig.ini + FilterFolder.ini + PriorityDisplayType.xml + usefullostfiles.xml + themdark.ini + shellexe.ini, NasConfig/{NasModel.xml, ssh_config.ini}; listed SaveScan/ (3 auto_*.rsf samples).
- Read the most substantive files in Cloud/ (cloud-recovery-architecture), Licensing/ (licensing-activation-chain, euinapp), Installer/ (setupue pair), DiskHealth/ (smart-companion), EuShare/ (eumrx-nas-architecture, full), FileRepair/ (repair-architecture), Languages/ (family census), MOM-modules/ (ui-bridge + dependency matrix), WinPE/ (boot-config, pe-payload-census, eusendmessage).
- Wrote /home/z/my-project/scratch/context-sheets/A-corpus-architecture.md (1,329 lines): file inventory, recovery workflow, stage model & progress semantics, full .rsf session format, grading model, filter model, config formats, UI/UX flow, threading/orchestration, 7-driver architecture, WinPE, errata register, 40 numbered divergence candidates with citations, 22 open questions.

Stage Summary:
- ~115 corpus documents/config files read in full (9 architecture + 62 artifacts + 38 UIUX + 10 runtime-families + 7 per-binary + ~20 og-codebase configs).
- Key outputs: context sheet A at /home/z/my-project/scratch/context-sheets/A-corpus-architecture.md — the audit-critical extraction of the corpus ground truth (stage/progress semantics incl. bottomarea.cpp 838/856 nStatus=4 m_iScanType 3→4 and the 40→65% rsf save progress formula; complete .rsf container/record/tree grammar with all GUIDs and field orders; grading boundaries; filter bucket IDs 0x100-0x105/0x1000-0x1004; FS-code tables; 7-driver IOCTL contracts; 40 divergence candidates).
- Reconciled the FileFilter 1489-vs-1481 erratum (1489 elements, 1481 unique, 8 duplicated extensions) and RawConfig 34 toggles by direct parse.
- No repository files modified; only this worklog entry and the context sheet were written.

---
Task ID: 2-D
Agent: D (core per-module deep reader)
Task: Exhaustive read of core src/ implementation for diff-audit context sheets

Work Log:
- Verified scope with ls: 52 .cpp files under src/ (24,793 LOC) + 37 unit-test files (15,077 LOC); created /home/z/my-project/scratch/context-sheets/.
- Read every source file line-by-line, module by module, citing file:line for each fact:
  - NTFS: cr_mftwalk.cpp, cr_attributes.cpp, cr_bitmap.cpp, cr_fixup.cpp, cr_bootstrap.cpp, cr_runlist.cpp (+ cr_ntfs_wire.h, cr_mftwalk.h) — walk phases A/B/B2/C, viability gate (strict 0x3FF, NTFS-1), recycle $I/$R remap (GAP-P1-3), LZNT1 envelope expansion (GAP-P1-1), USN harvest caps, remnant scan + shifted-header salvage, fixup only-2-tails parity.
  - FAT: cr_fat_quick.cpp, cr_chain.cpp, cr_lfn.cpp — deleted-LFN reversal (FAT-1), FAT12 EOC canonicalization, backup-boot FAT32-only (FAT-8), deleted-chain grading + honesty cap.
  - exFAT: cr_exfat_quick.cpp, cr_exfat_dentry.cpp — boot-region checksum skip semantics, EXFAT-1..6 gates, assumed-contiguous salvage.
  - ext4: cr_ext4_engine.cpp — extents/indirect ladder, dtime||links==0 deleted predicate, dnode sweep w/ bitmap grading, jbd2 no-commit-gate salvage (E-2).
  - HFS+: cr_hfsplus_engine.cpp — B-tree leaf chain, SInt8 sign-extension fix (F-A2-17), freed-node sweep.
  - btrfs: cr_btrfs_engine.cpp — chunk-root/root-tree discovery, BTR-2 uuid adoption, BTR-3 inline, BTR-4 compression; (void)root_deleted/root_lost noted.
  - ZFS: cr_zfs_engine.cpp — label/uberblock probing (shifts 10..13), nvlist XDR+native, micro/fat ZAP, gang pieces, BPE decode, BE-pool refusal; no deleted recovery (documented parity).
  - ReFS: cr_refs_detect.cpp + cr_refs_engine.cpp — MSB probe, bootstrap chain, REFS-1 next-chain, REFS-2 minor>=14 layout, allocator polarity calibration, deleted grading (no Excellent tier).
  - Table/LDM: cr_table_parse.cpp, cr_crc32.cpp, cr_ldm.cpp — two-step GPT salvage, EBR chain, PRIVHEAD/TOCBLOCK/VMDB/VBLK decode, layout resolution, LdmVolumeDevice RAID-5 left-symmetric reconstruction + re-anchoring.
  - Lost: cr_candidates.cpp — 4 probe families, overlap scoring, audit row 31b device-tail sizes.
  - Carve: cr_carve.cpp, cr_analysers.cpp, cr_registry.cpp, cr_rawsigns.cpp, cr_rawsigns_data.cpp — window/overlap constants, trial order, F9 len-classes, P1-12 1 MiB probe blocks, all 31 grammar validators (JPEG/PNG/TIFF/WebP/BMFF/PDF/ZIP family incl. CD-anchor + indexless + zero-pad, 7z/SQLite/AVI/MKV/RAR4/5/PST/DNG/ORF/RAF/RW2/PSD/MP3/WAV/M4A/ASF/FLV/CAB/ISO), 139-format generated raw-sign layer.
  - Tree: cr_node.cpp, cr_snapshot.cpp — arena, O(1) append, dedup indexes, F-1 rebase idempotency, G-02 snapshot.
  - Session: cr_crsf.cpp, cr_records.cpp — chunk framing, torn-tail salvage (BENCH-F1), P1-7 64 MiB bomb cap, v2 record flags.
  - Writer/Report: cr_safewriter.cpp, cr_report.cpp — overlap refusal F12, WRITER-1 per-item collection, G-08 dirs, FB-R02 salvaged, G-09 report, U+FFFD-safe JSON.
  - Rawfixer: cr_rawfixer.cpp — boot-only repair surface, sector-span RMW transaction, FIX-2 error-code split.
  - Health/err: cr_nvme.cpp, cr_error.cpp — pure NVMe page parser, 25-entry error table.
  - Orchestrator: cr_scan.cpp — stage machine, retry->cache pipe wiring, probe_family offsets, LDM-first dispatch (ARCH-DUP-4), parallel candidates (F-B-10/11), telemetry G-04..G-06, checkpoints G-13, resume G-07/BENCH-F2, F-1 rebases.
  - Query/Preview: cr_query.cpp, cr_preview.cpp — fast-path ordering, Path sort, Window strict/lenient fetch, docx raw-deflate.
  - Storage family: cr_cached.cpp, cr_retry.cpp, cr_slice.cpp, cr_devmap.cpp, cr_decomp.cpp (lzjb/zle/gzip/LZNT1 byte-exact + hardening), cr_sha256.cpp (FIPS 180-4), cr_parallel.cpp (pool + parallel_for exception containment).
  - Win: cr_transport.cpp (RDA client + F11 unscale + alignment shim + FB-T06/T07), cr_enumerate.cpp, cr_bitlocker.cpp (FB-B01, BEK auto-unlock), cr_health.cpp (ATA passthrough + legacy SMART ioctl), cr_destio.cpp (atomic rename), cr_elevate.cpp.
  - App/UI: rrecov.cpp (7 commands, exit-code contract, save-on-cancel, LDM registry, identity verify) and cr_ui.cpp (C ABI, guarded entries, single-flight progress, snapshot/preview/recovery surface).
- Read all 36+1 unit-test files (case names + assertion targets + gaps) and built the tests inventory: 248 TEST() cases.
- Grep-verified ZERO TODO/FIXME/XXX/placeholder/stub/mock markers across src/ and include/ (only false-positive hits: an NTSTATUS hex and a \uXXXX escape in comments); recorded deliberate non-implementations documented in comments (btrfs/ZFS deleted recovery, NTFS INDX walker, LZO extent unmapping, BIGALLOC refusal, BE ZFS pools).
- Wrote 23 sheets: D-ntfs, D-fat, D-exfat, D-ext4, D-hfsplus, D-btrfs, D-zfs, D-refs, D-table-partition, D-lost, D-carve, D-tree, D-session, D-writer-report, D-rawfixer, D-health, D-orchestrator, D-query-preview, D-storage, D-win, D-cli-ui, D-tests-inventory, D-summary — each with the 10 mandated sections, magic-byte/deleted-classification/checksum/state-transition quotes, constants tables, and per-module test gaps.

Stage Summary:
- Files read: 52/52 src .cpp (24,793 LOC) fully, plus required headers (cr_ntfs_wire.h, cr_mftwalk.h, cr_fat_wire.h, cr_exfat_wire.h, cr_table_wire.h, cr_types.h folds) and all 37 tests/unit files (15,077 LOC); 824 fuzz LOC inventoried.
- Sheets produced: 23 markdown files under /home/z/my-project/scratch/context-sheets/ (D-*.md).
- Headline extractions for the parity audit: 0 TODO/stub markers anywhere; deleted-recovery absent by design on btrfs+ZFS; UTF-8 names byte-lifted (not decoded) on ext4/btrfs/ZFS; ZFS drops sub-second timestamps; NTFS viability gate strict-inequality (NTFS-1); every fallback carries an audit-row citation; Win layer has no dynamic loading and no OS-version gates (Win7 compatibility via pure Win32 + legacy SMART ioctl); test blind spots concentrated in the Windows platform layer, cap/budget boundaries, and 4Kn geometry.
- No repository files modified; only this worklog entry and the context sheets were written. No git commit performed.
---
Task ID: 2-B
Agent: B (corpus per-engine decompiled-tree reader)
Task: Exhaustive read of SearchDLL + PartitionRecovery artifacts and decompiled spot-verification for diff-audit context sheets

Work Log:
- Read ALL 30 SearchDLL artifact .md files (task said 31; dir holds 30 — every file present was
  read fully, incl. the six rawsearchdll--family-* rosters, both registry artifacts, zip + PST/RAR/
  MKV/HEIC granular docs, engine-matrix, per-engine-function-map).
- Read ALL 14 PartitionRecovery artifact .md files (mom architecture + mo-module-map, rawfixer v1+v2,
  eprdtdrv internals v1+v2, epmntdrv trio naming/byte-diff, EUDCPEPM/EUEDKEPM v1+v2, pending-op
  engine, GetDriverInfo, BitLockerLib, Burn).
- Read artifacts/CORRECTIONS.md (8 rows) and folded every row into the sheets (0xFFF resident cap
  retracted; FUN_180041970 = base-record test; FAT12 BAD raw-follow; ext4 backup-SB unverified;
  1481-ext/34-toggle recounts; ext4 journal {3,4} gate; strict 0x3ff viability; no Btrfs chunk-uuid).
- Spot-verified ~40 claims against the raw decompiled trees with exact snippets+line cites:
  * NTFS: FUN_18001acc0 batch loop (:21128-21300: 'FILE' magic, fixup FUN_180041bb0 :52513,
    flags@+0x23/&4/'$' filter, STRICT 0x3ff<size :21208, bitmap pair +0x40/+0x48, destroy iff
    intact==total :21250, add-flag byte@+0x3d|=1), FUN_18002c2e0 bitmap bit test :35643,
    fastsearch ini default ON :7163, 0x940 factory :39.
  * FAT: FUN_180041c80 FAT16 Next (0xFFF7->0xFFFFFF7, >=0xFFF8->0xFFFFFF8) :54125, FAT12 table
    (0xFFF->0xFFFFFFFF, 0xFF7 falls through RAW) :53790, EOC mask families :8030-8048, 0xE5 sites
    + <0x10000 guard :14634, LFN 0x40@+0x72 :11442, chain loop (c-2)*SPC+first_data :11237.
  * exFAT: dentry type classes (t-3/t-5/t-0x40/t+0x3F)&0x7F + reserved-zero + first-cluster gate
    :1846-1871, backward triple walk FUN_180003120, 0x8E0 quick object, FNV-1a FUN_180011160;
    absence of any entry-set-checksum loop in the engine; RawFixer boot checksum FUN_18000e7e0
    (c<<31 | (c>>1)+b over sector_size*0xB).
  * ext4: 0xEF53@+0x38 :3222, first_data_block cross-checks, 0x400<<log, 64-bit block count
    CONCAT44 :3232, GDT math, journal 0xC03B3998 + blocktype {3,4} :12207/:12306, CatchBuffer
    0xE00/0x400. NEW: the "+0x18 log_block_size guard" is mechanically byte@SB+0x60 =
    feature_incompat bit 0 (COMPRESSION) — artifact mislabels it; core already had it right.
  * HFS+: H+/HX + &0x1FF + <0x11 + clamps :10005-10010, error 0x5017900000002, leaf walker math.
  * Btrfs: _BHRfS_M@+0x40, sectorsize@+0x90/nodesize@+0x94, band [0x86,0x86+0xFF7B) no-pow2,
    generation@+0x48 newest-wins, mirror-abort-if-primary-fails. NEW: EUCreateBtrfsComplateSearcher
    is a NULL-returning stub (no plain EUCreateBtrfsSearcher exists).
  * ZFS: 0xBAB10C sites, 0x80-qword stride, txg@UB+0x10 comparisons, nvlist read 0x1C000/0x4000
    with type-9 "vdev_tree" recursion + type-2 BE u64 accumulation.
  * ReFS: SUPB/CHKP/MSB+ReFSFSRS page_type 1/2/3, first-DWORD compare, parse(page+8,size-0x20);
    RefsQuickSearchFiles 0x90+0x210 3-arg quick factory, RefsSearchController 0xC0.
  * RAW: 6 exports :72122+, registry keys (PDF/TIFF/PNG/JPEG) :1548-2053, ZIP probe version gate
    <=0x13 or ==0x62 (:18294 — artifact said <=10), PK dispatch + 0x500001 padding cap + 16-byte
    sector-tail zero test :18480-18510, status 0x48/0xd, PST 0x0E/0x17 + 0xA8/0xB8 + 1 MiB floor
    FUN_180041eb0, RAR4 gate FUN_180012bb0, HEIC 0x68656963 :21275, JPEG SOF0 height>400
    pixels/8 estimate :39648, EBML constants.
  * PartitionRecovery/RawFixer: 33-method inventory presence, NTFSFixer::CheckMFT signed
    clusters_per_record (1<<(-cpr&0x1F)) + 0xFFFFFFFF sentinel, InitMFTMirror record-1 fallback
    chain in NTFSLib :17310-17430, PartitionRecoveryD parser vftables, eprdtdrv IOCTL
    0x60002400/04/08 + sector<<9, boot-label FAT32/NTFS checks.
- Skimmed core_recovery src/core + include/cr per module (ntfs mftwalk/chain/bootstrap/fixup,
  fat chain/lfn/quick, exfat dentry/quick, ext4/btrfs/zfs/refs/hfsplus engines, carve
  registry/analysers/rawsigns, lost candidates, orchestrator, repair rawfixer) to pin divergence
  candidates with file:line.
- Wrote 11 deliverables to /home/z/my-project/scratch/context-sheets/: B-ntfs.md, B-fat.md,
  B-exfat.md, B-ext4.md, B-hfsplus.md, B-btrfs.md, B-zfs.md, B-refs.md, B-rawsearch-carving.md,
  B-partitionrecovery-rawfixer.md, B-summary.md. No repo files modified; no ghp_ strings written.

Stage Summary:
- Files read: 30 SearchDLL + 14 PartitionRecovery artifacts + CORRECTIONS.md, all fully; ~40
  decompile sites spot-verified across ghidra-searchdll/ghidra-partition/ghidra-mo/ghidra-drivers.
- Sheets produced: 10 per-engine sheets + B-summary engine matrix (~184 cited constants total;
  every constant carries an artifact citation; ~75 additionally [B-VERIFIED] with function+line).
- Key results: (a) all 8 CORRECTIONS rows folded in and respected; (b) 4 NEW errata candidates
  for the corpus (ext4 +0x60 incompat guard mislabel; ZIP version gate 0x13/0x62; Btrfs complete
  searcher is a NULL stub + no EUCreateBtrfsSearcher; exFAT ChecksumDBR byte-range description vs
  11-sector loop bound); (c) top divergence candidates with confidence: NTFS $-skip blanket vs
  known-metafiles (HIGH), NTFS fully-overwritten destroy vs grade (HIGH), FAT12 BAD raw vs
  canonicalize (HIGH, registered), exFAT boot-checksum coverage (HIGH), ext4 journal every-block
  vs descriptor-referenced (MED), Btrfs og-has-no-deep-scan (HIGH), ZFS nvlist 0x1C000 single read
  vs per-label (HIGH), RAW analyser coverage 106+chains vs 34 (HIGH); (d) NTFS flags-bit-0x10
  CONFLICT (folder vs in-use) documented as OQ-1 for audit resolution.
Do not git commit.

---
Task ID: 2-CONSOLIDATE
Agent: main auditor
Task: Consolidate parallel context sheets (A-E) — T2 close-out

Work Log:
- Agents A (corpus architecture, ~115 docs), B (SearchDLL+PartitionRecovery artifacts, 44 docs, 10 per-engine sheets, ~184 cited constants), C (core public surface, 53 headers + build/CI/docs), D (core src, 52 files / 24,793 LOC + 37 test files), E (binary probes: 38 PE dumps, IOCTL census, raw_signs.json, core Linux build + live table-parser fixture run) all complete.
- Sheets consolidated under scratch (outside repo, per read-only rule). Key empirical finds already surfaced: E-1 four wrong GPT type GUIDs in cr_table_wire.h; E-2 EBR salvage gap; B: NTFS $-skip blanket vs known-metafiles, FAT12 BAD cluster raw-follow, exFAT boot-checksum coverage, Btrfs og complete-searcher NULL stub, ZFS nvlist 0x1C000 single-read, RAW analyser 106+chains vs core 34, corpus OQ-1 NTFS flags-0x10 folder-vs-inuse conflict.
- Next: T3 skeleton docs (PARAMETERS/SEVERITY/README/SUMMARY), T4 matrix scaffold, then module waves.

---
Task ID: 5 (T5)
Agent: main auditor
Task: NTFS module audit — all applicable parameters

Work Log:
- Cross-read B-ntfs.md vs D-ntfs.md; spot-verified critical claims in code (is_metafile_name list, viability gate strict >, Overwritten emission path, mirror fallback record_no<4, bootstrap record-size rejection, Confidence/DeletedState enums).
- Wrote findings: S-001 (overwritten-deleted emitted vs og destroy — SEVERE), M-001 (record-size reject vs og clamp — MAJOR), N-001 (MFTMirr hardcoded 0..3 — MINOR), I-001 (metafile filter deviation — INFO), OQ-T1 tracked-open (secondary claim map).
- Wrote evidence/decompiled-quotes/ntfs-viability-and-filters.md (6 quotes with file+line), modules/ntfs.md (full parameter verdicts + 2 corpus errata candidates: runlist nibble prose, flags-0x10 conflict resolution).

---
Task ID: 6 (T6)
Agent: main auditor
Task: FAT + exFAT module audit

Work Log:
- Read B-fat/B-exfat/D-fat/D-exfat sheets; verified in code: FAT12 BAD canonicalization, EOC family handling in next()/follow_chain, mirror trigger, exFAT boot checksum skip bytes, entry-set checksum (set_ok → PartialRisk only, files kept), stream first-cluster gate.
- SETTLED POLARITY CONFLICT: read exFATSearchDLL_decompiled.c:1846-1875 directly — stream dentry valid iff first_cluster != 1 && < 0xFFFFFFF7 (goto TRUE-return path). Core's AUDIT-FAT reading is CORRECT; artifact fat-exfat-engine-algorithms-v2 §3 polarity is INVERTED (corpus erratum candidate #6). Agent B's sheet had propagated the artifact reading.
- Findings: I-002 (FAT12 BAD, INFO), N-002 (EOC families, MINOR), N-003 (exFAT entry-set checksum, MINOR), OQ-T2+OQ-T5 tracked-open (mirror trigger; ChecksumDBR predicate).
- Wrote modules/fat.md, modules/exfat.md, evidence quotes Q1-Q4.

---

Task ID: 7 (T7)
Agent: main auditor
Task: ext4 + HFS+ + Btrfs module audit

Work Log:
- Read B-ext4/B-hfsplus/B-btrfs vs D sheets; verified in code: ext4 SB+0x60 guard (core matches decompile; artifact wrong), journal salvage passes (descriptor-referenced only + no jsb gate), HFS+ CNID_ATTRIBUTES_FILE defined-but-unused, journal_info_block parsed-but-unused, rsrc forks present, blockSize bounds, Btrfs validate_superblock bounds + unconditional mirror loop, byte-lift loops in ext4/btrfs/zfs.
- Empirical probe: EXT4SearchDLL imports NO MultiByteToWideChar (agent E PE dumps) + rsf-format :326 names are UTF-8 → og passes UTF-8 through; core byte-lifts → mojibake (S-005).
- Findings: S-002 (HFS+ attributes/decmpfs missing), S-003 (HFS+ journal not replayed), S-004 (ext4 journal salvage surface + jsb gate), S-005 (names mojibake, 3 engines), N-004/N-005/N-006 (CatchBuffer layering, HFS+ blockSize bounds, btrfs nodesize bounds), I-003 (btrfs supersets: NULL-stub complete searcher + mirror-abort rule).
- Wrote modules/ext4.md, modules/hfsplus.md, modules/btrfs.md. Corpus errata candidates #7 (ext4 guard field) registered.

Task ID: 1 (T1)
Agent: main engineer (diff-to-fix remediation session)
Task: Pull both repos; record audited commit vs current commit; save redacted prompt; worklog entry

Work Log:
- Cloned both repos fresh. Corpus (read-only ground truth) at `03ad46c` — identical to the commit
  the audit cites (corpus has NOT moved since audit).
- Core `main` at session start: `9b7212d`. Audit baseline: `67193b6` (README-stated). Delta since
  baseline: `7797bd6` (fix: GAP-S6-3 + GAP-S6-4 — ReFS real-volume detection + NTFS remnant
  $DATA reconstruction, landed mid-audit) + three audit-trail commits (`90e9aba`, `2be324d`,
  `9b7212d`) that only touch diff-to-fix/. Consequence: finding line numbers cite `67193b6`;
  every finding re-verified against current HEAD before any change (ntfs.md already carries the
  post-baseline note for 7797bd6's remnant $DATA reader).
- Saved this session's prompt (token redacted) to `worklog/session-diff-fix-prompt.md`.
- Read `diff-to-fix/` in full: README, SUMMARY, SEVERITY-SCALE, PARAMETERS, CROSS-REF-MATRIX,
  all 8 findings (S-001 SEVERE; M-001 MAJOR; N-001/N-002/N-003 MINOR; I-001/I-002 INFO),
  both tracked-open files (OQ-T1, OQ-T2, OQ-T5), all 3 module docs, both evidence quote files.
- Audit state noted: audit waves completed = ntfs, fat, exfat only; 17 further module rows in
  CROSS-REF-MATRIX still pending (·). SUMMARY.md counts are stale (all "waves pending") — to be
  truthfully updated at close, not silently filled.
- Scope note: no ACUTE findings exist yet (0 audited waves in the severity folder). OQ items cite
  decompiled corpus files that are already present in `re-workspace/reversed/` — OQ-T1/T2/T5 may
  be resolvable this session by direct decompile reading, without new Ghidra work.

Stage Summary:
- Baseline `67193b6` vs session HEAD `9b7212d` recorded. Corpus unmoved at `03ad46c`.
- Session roadmap: verify+classify each of 8 findings and 3 OQs, fix confirmed ones with
  fail-before/pass-after tests, resolve or precisely advance the OQs, update matrix/SUMMARY/
  module docs/worklog, leave main green.


---
---
Task ID: 8 (T8)
Agent: main auditor
Task: ZFS + ReFS + table/LDM module audit

Work Log:
- Read B-zfs/B-refs/B-partitionrecovery-rawfixer vs D sheets; verified in code: ZFS read_raw vdev!=0 refusal + PARITY-DELTAS D-09, ReFS detect memcmp widths + version gate, GPT GUID constants, EBR caller gate, LDM GPT-path finder.
- EXECUTED PROOF (A-001): built canonical-GUID GPT fixture, ran core parser via libcr.a harness — parse_ldm_database = ERR_BAD_MAGIC on GPT dynamic disk; ESP→Reserved (dup GUID), MSR/Linux/LDM→Other. Cross-evidence: ESP/MSR canonical bytes found byte-for-byte in six og PartitionRecovery binaries. Header comment claiming triple-source verification is FALSE.
- Findings: A-001 (ACUTE), M-002 (EBR salvage), N-007+I-004 (ReFS detect), OQ-T6 (ZFS anchors/multi-vdev), OQ-T7 (LDM RAID-5 rotation).
- Wrote modules/zfs.md, modules/refs.md, modules/ldm.md; evidence/executions/gpt-guid-probe.md + harness.

---
Task ID: 9+10 (T9, T10)
Agent: main auditor
Task: lost-partition + carve module audit

Work Log:
- Read D-lost/D-carve + B-rawsearch-carving + PartitionRecoveryD census; verified in code: candidate dispatch (4 families only), registry magic table (no GIF/BMP/RTF/OLE/ADTS/MPEG/DWG), MP3 ID3-only magic, ZIP padding cap 5242880, orchestrator carve whole-slice (no bitmap gating).
- EXECUTED set-comparison: parsed og raw_signs.json (139 ext listed) vs core registry ∪ rawsigns → missing common formats proven.
- Reviewed in-window concurrent fixes: GAP-S6-6 (03ddcd2, nested-dedup containment), GAP-S6-8 (carve absolute-extent dedup), GAP-S6-7 (MetadataOnly skip), GAP-S6-13 (64KiB direct I/O) — all in audited tree.
- Findings: S-006 (lost-partition probe families), S-007 (carve format coverage), M-005 (carve target policy), N-008 (ZIP padding/version/ZIP64), OQ-T8/T9.
- Wrote modules/lost-partition.md, modules/carve.md.


---
Task ID: 2 (T2/T3 partial — verification reads)
Agent: main engineer (diff-to-fix remediation session)
Task: Corpus verification of NTFS findings (S-001, M-001, N-001, I-001, OQ-T1)

Work Log:
- Read `ntfs-engine-algorithms-v2--deleted-viability.md` in full; read decompiled
  `FUN_18001acc0` (NTFSSearchDLL :21095-21302) directly, first-hand. Confirmed:
  * record-size prologue :21134-21142 — `if < 1 -> 0x200`, `buf = max(0x400, rs)` — NO
    pow2/divisibility/rejection path (M-001 CONFIRMED vs our ERR_BAD_PARAMETER).
  * blanket `name[0]=='$'` filter :21186 (I-001 reference behavior CONFIRMED).
  * destroy branch :21249-21262 — `0 < iVar21 && iVar20 == iVar21 -> goto LAB_18001b06f`
    = props Release WITHOUT the list insert at LAB_18001afe6 (S-001 CONFIRMED: og drops
    fully-reallocated deleted files; viability gate only for non-folders, `(flags&0x10)==0`).
  * strict `0x3ff < size` at :21208 (core already strict — NTFS-1 stays green).
- OQ-T1 RESOLVED (decompile-first read, no new Ghidra needed):
  * Direction: the secondary check `bitmap[0xC] -> vt+0x20(sector)` composes via `||` into
    the SAME `iVar20++` (intact/re-allocated) counter as the primary bit test — H2 (inverse
    "still-free" semantics) REFUTED. A secondary hit swings toward destroy/keep exactly
    like a primary hit.
  * Object identity: the "bitmap" handed to the viability loop is a 0x70-byte scan-built
    claimed-sector bit-set (ctor FUN_18002c330: buffer@+0x00, size@+0x08, limit@+0x10,
    spc@+0x18, base@+0x50, secondary-ptr@+0x60, budget 0x5f400000@+0x68). Bits are SET
    during the scan by marking already-processed files' run sectors (FUN_18002c1e0 range
    setter, called from FUN_180017190 at :18409) — it is NOT the volume $Bitmap read from
    disk; the +0x48 fallback instance is constructed on demand, zeroed (:18526-18545).
  * The secondary pointer at +0x60 is ctor-initialized NULL and NO code path in the entire
    NTFSSearchDLL decompile ever assigns it — the secondary test is a dormant polymorphic
    hook in this build; it never fires. Our single-$Bitmap reused_ratio is therefore not
    biased by any missing secondary map.
- Read NTFSLib `InitMFTMirror` (ghidra-mo :17343-17414) first-hand: unnamed-$DATA-walk,
  resident -> "$MFTMirror::$DATA is resident!" abort (return 0), runlist to this+0x60,
  span this+0x70 = (last_run.offset + last_run.len) * factors, and `ReadFileRecord` gates
  mirror service on `param_1 <= *(this+0x70)` (:17460) — the mirror coverage is
  runlist-derived, NOT a hardcoded 4 (N-001 CONFIRMED).
- Found a finding-adjacent divergence the audit missed (sub-bug of S-001, same fix family):
  `VolumeBitmap::reused_ratio` (cr_bitmap.cpp:86-102) counts SPARSE runs fully into both
  `total` and `reused`; the og iterator skips `*it == -1` entries entirely (:21220) —
  sparse clusters are excluded from the ratio on the og side. Core-side consequence: a
  deleted sparse/compressed file with free real clusters can be graded Overwritten on
  bogus accounting. Will fix with S-001.
- Also confirmed for N-001 fix: our `enum_mirr_attrs` accepts RESIDENT $DATA on the mirror
  record as a runlist source (divergence vs og's hard abort) and takes the LAST decodable
  $DATA instead of the FIRST unnamed $DATA — both will be aligned in the N-001 fix.

Stage Summary:
- S-001, M-001, N-001, I-001 = CONFIRMED-DIVERGENCE (with first-hand decompile cites).
- OQ-T1 = RESOLVED this session (direction + object identity + dormancy), pending write-up
  into the tracked-open file.
- New sub-finding (sparse-run accounting in reused_ratio) to be fixed with S-001 and noted
  in its Resolution; no new finding file needed (same code path, same decompile evidence).
---
Task ID: 11-20 (T11..T20)
Agent: main auditor
Task: session/writer/health/rawfixer/orchestration/visualization/abi/cli audits + cross-cutting (T17 perf/arch/safety, T19 quality) + closeout

Work Log:
- T11 session: CRSF vs .rsf field-equivalence verified (chunk algebra identical, records equivalent, hardening supersets) — no findings, module file.
- T12 writer: M on overlap refusal/sanitization/streaming; N-009b (progress over-report), N-009d (ACL).
- T13 health+rawfixer: N-009a (u16 percentage_used); S-008 (RawFixer boot-sector subset + not wired pre-scan vs og 33-method pre-scan repair incl. CheckMFT/CheckFatTable).
- T14 orchestration: stage model/progress/threading/resume verified M; M-005 (carve whole-slice vs og bitmap-gated free-space) filed under carve.
- T15 visualization: S-009 (no disk-map/IBuildVolumeBitmap surface; ABI-CONTRACT §7 to-build); filter-category granularity re-verified open as project GAP.
- T16 ABI+CLI: cr_ui surface/versioning M; N-009c (--filter ASCII mangling).
- T17-T19 cross-cutting: X51 64KiB envelope verified post-GAP-S6-13 (superset vs og 512-sector driver); fuzz CI disabled → M-006; sanitizers/docs/stubs/Win7 verified clean; I-005 groups 7 reviewed supersets.
- T20 closeout: SUMMARY.md finalized (1 ACUTE / 9 SEVERE / 4 MAJOR / 12 MINOR / 5 INFO / 9 tracked-open; fix order ranked), CROSS-REF-MATRIX finalized, all 19 module files complete.

Stage Summary:
- Deliverable complete: diff-to-fix/ = README, SUMMARY, SEVERITY-SCALE, PARAMETERS, CROSS-REF-MATRIX, 15 finding files (31 finding IDs), 19 module files, evidence (decompiled-quotes x3, executions x2 incl. the A-001 canonical-GUID proof harness), tracked-open x4 files (9 OQs).
- Audited commit 67193b6 (concurrent GAP-S6 fixes through 7777e1c verified in-tree). Corpus 03ad46c.
- No source files modified. Read-only mandate held throughout.

---
Task ID: 20-FINAL
Agent: main auditor
Task: Session close — final verification

Work Log:
- Added the missing tracked-open file (OQ-T8 dispatch order + OQ-T9 CalcPartition scoring) referenced by carve/lost-partition module files.
- Verified read-only compliance: zero commits from this audit touch src/, include/, tests/, build, or workflows (git log 67193b6..HEAD filtered to source paths shows only the concurrent core-final session's GAP-S6 fixes).
- Noted the parallel diff-fix session's corroboration (085cedf: S-001/M-001/N-001/I-001 independently confirmed; OQ-T1 resolved by them as dormant/same-direction — tracked-open file kept with their resolution noted for the next reader).
- Final deliverable: 51 files in diff-to-fix/ (5 top-level docs, 15 finding files covering 31 finding IDs, 19 module files, 5 evidence files, 4 tracked-open files covering 9 OQs).

Stage Summary:
- Audit complete at baseline 67193b6 (concurrent fixes through 7777e1c verified in-tree). Corpus 03ad46c. Counts: 1 ACUTE, 9 SEVERE, 4 MAJOR, 12 MINOR, 5 INFORMATIONAL, 9 tracked-open.
- Recommended first 20 fixes + parallel RE track in diff-to-fix/SUMMARY.md. All commits pushed to main. No source modified.
---
Task ID: 3 (A-001 fix)
Agent: main engineer (diff-to-fix remediation session)
Task: ACUTE A-001 — six wrong GPT type GUIDs; GPT dynamic disks fail LDM discovery

Work Log:
- Reproduced the audit's executed probe on the PRE-FIX build (git-stash of the three
  source files): `parse_ldm_database rc=1342373889 (0x50030001 ERR_BAD_MAGIC)` —
  GPT-dynamic discovery FAILS, byte-identical to the audit's log.
- CORRECTED THE FINDING'S BYTE TABLE before fixing (corpus-wins rule): the original
  rows listed ESP/Linux in GUID-text order and a 31-hex-digit MSR row (transposed).
  Ran a fresh 556-binary og scan checking BOTH orders: ESP `28732ac11ff8d211ba4b00a0c93ec93b`,
  MSR `16e3c9e35c0bb84d817df92df00215ae`, basic-data `a2a0d0eb...` appear byte-for-byte
  in Partition.mo / LdmManager.mo / MainModule / RecoveryModule / UILogic / ToolsModule /
  CloneModule / DeviceManager — ZERO text-order hits anywhere. The audit's evidence log
  had transcribed the disk-order hits as text hex; corrected in the evidence file
  (append-only) + finding (table replaced, correction note added).
- Fix: cr_table_wire.h — all six constants to canonical mixed-endian disk bytes
  (GPT_TYPE_EFI_SYSTEM added; BIOS boot "Hah!IdontNeedEFI"; real MSR; real Linux FS;
  canonical LDM metadata/data). The false session-5 "verified in three sources" comment
  replaced with true provenance. cr_table_parse.h — FsKind::EfiSystem appended (no value
  shifts; PartitionInfo.fs has no scan-dispatch consumers — probe_family is content-based).
  cr_table_parse.cpp — classification: ESP→EfiSystem, LDM metadata→Reserved,
  MSR/BIOS-boot stay Reserved.
- Tests (fail-before proven via pre-fix probe; all green after):
  Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes (independent text→disk derivation,
  incl. the "Hah!IdontNeedEFI" ASCII sanity), Gpt.EspMsrLinuxBiosbootLdmClassification,
  Ldm.GptDynamicDiskCanonicalGuidsParse (real PRIVHEAD in canonical-GUID GPT layout →
  parse OK, is_gpt, 2 layouts, privheads_matched=2 — the config+2047 slot carries the
  backup GPT header in this geometry), Ldm.GptDynamicDiskTableClassifiesLdmEntries.
  Full suite: 263/263 green, Linux ASan preset.
- Docs in same commit: PARITY-CORE-REC "GPT — LDM GUIDs" row (claimed MATCH with wrong
  data-GUID text — corrected), AUDIT-PIPELINE GPT recognition row (MATCH verdict
  predates A-001 — annotated), CAPABILITY-MATRIX LDM row (+GPT-dynamic note),
  modules/ldm.md (B1/C19 → M post-fix), CROSS-REF-MATRIX table+ldm row, SUMMARY.md
  rewritten to the truthful running inventory (was stale all-zeros), A-001 finding
  Resolution, evidence file correction appendix.

Stage Summary:
- A-001 FIXED: canonical GUIDs in, ESP un-duplicated from MSR, GPT dynamic disks
  recover LDM volumes end-to-end (unit-proven), 4 permanent regression tests, 263/263.
- Finding corrected (byte-order table), evidence corrected (og-binary hits are
  disk-order, not text-order) — the audit's own conclusion was right, its byte
  transcription was not.

---
Task ID: 4 (S-005 fix)
Agent: main engineer (diff-to-fix remediation session)
Task: SEVERE S-005 — ext4/Btrfs/ZFS names byte-lifted to UTF-16 (mojibake)

Work Log:
- Re-verified the reference claim first-hand: .rsf record-4 name field is `char name[name_len]`
  UTF-8 (rsf-format--scan-session.md :326), and a fresh import scan of og EXT4SearchDLL shows
  ZERO MultiByteToWideChar/WideCharToMultiByte/CharToOem — the og passes on-disk UTF-8 through
  unconverted. CONFIRMED-DIVERGENCE.
- Fix: new shared codec include/cr/cr_text.h (text::utf8_to_u16 + append_codepoint) — strict
  RFC 3629 decoder with the codebase's established policy (U+FFFD per bad lead, per-byte resync;
  identical to cr_ui's u8_to_u16 and the report escaper — no third policy introduced).
- Replaced the byte-lift loops: ext4 parse_dir_block; Btrfs dir items; ZFS dnode names +
  zap child-name prefix + the pool display name (nvlist pool_name — a CJK pool name was
  mojibake at the result ROOT; the finding's core-references listed only the dnode site).
- Audited the orchestrator's synthesized-name set per the fix plan: snprintf-ASCII everywhere
  ("Candidate @", subvol-N, InodeNNNN, CarvedNNNN); volume_label/model/serial are char[32]
  UTF-8->UTF-8 copies into equally-typed session fields — no further byte-lift sites.
- Tests (fail-before reproduced by stashing only the three engine files — all 3 engine tests
  FAILED against byte-lift engines, pass after):
  * tests/unit/test_text.cpp (11 vectors: CJK, accent, astral surrogate pairs, overlong,
    surrogate-halves, beyond-U+10FFFF, per-byte resync, truncated tails, embedded NUL,
    round-trip) — registered in tests/CMakeLists.txt.
  * Ext4Engine.NonAsciiNamesDecodedNotByteLifted (root-dir appended CJK/accent/invalid
    entries; asserts u16 code points AND name_utf8() round-trip = the exact projection
    tree --json emits).
  * Btrfs.NonAsciiNamesDecodedNotByteLifted (FS-tree rebuilt with CJK/accent/invalid dir
    items; distinct child objectids — the engine collapses same-object names, hard-link
    semantics, noted).
  * Zfs.NonAsciiNamesDecodedNotByteLifted (root microzap rebuilt + in-place 4-byte nvlist
    patch "tank"->"\xe6\x96\x87x" so the pool DISPLAY name decodes; covers all 3 zfs sites).
- Full suite: 281/281 green (Linux ASan). Audit trail: S-005 Resolution written; ext4/btrfs/zfs
  module B10 rows -> M post-fix; CROSS-REF-MATRIX S-005 cells -> M (S-005 fixed); SUMMARY risk #2
  marked fixed + remediation log row.

Stage Summary:
- S-005 FIXED: non-ASCII filenames on ext4/Btrfs/ZFS now decode to true code points across the
  whole pipeline (disk UTF-8 -> u16 -> report/session UTF-8); CJK pool names included.
- Discovered-and-fixed sub-site: ZFS pool display name (not in the finding's reference list);
  noted in the Resolution.

---

Task ID: 1-R (continuation leg bootstrap + coordination claim)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: Session bootstrap — baseline, audit-trail repair, verification of in-flight parallel fixes

Work Log:
- Pulled both repos fresh. Start baseline: core 1cb6a43 (A-001 fix), corpus 03ad46c (unmoved).
  Mid-bootstrap, the parallel remediation leg pushed 2f3237d (S-005 fix, 281/281 green per
  its entry) — rebased onto it; this entry and the repair below now sit on 2f3237d.
- Token hygiene verified: real token string appears nowhere in either repo (worktree +
  full history, git grep + git log -S both empty). Session prompt re-read from
  worklog/session-diff-fix-prompt.md (token-redacted, already saved by prior leg).
- Repaired worklog.md: commit 1cb6a43 had landed with unresolved merge-conflict markers
  (auditor's T11-20/20-FINAL closeout vs the A-001 fix entry). Both sides kept, in commit
  order; markers and one duplicated separator removed. S-005's entry (appended by the
  parallel leg after the stale >>>>>>> marker) preserved untouched.
- Will independently verify the parallel leg's S-005 fix on this leg's environment (full
  suite + spot re-read of the decoder against the cited corpus evidence) before building
  on it — same rule the mandate applies to audit findings: trust nothing unverified.

COORDINATION CLAIM (worklog is the shared medium; read before each fix per SOP):
- This leg takes, in order: S-008 (RawFixer pre-scan wiring + subset), then S-006
  (lost-partition probe families), then S-007 (carve formats). Parallel leg: please
  continue the SUMMARY.md order from S-002 downward (decmpfs, journal, ext4-journal).
- All other findings untouched by this leg until claimed here or completed and pushed.

Stage Summary:
- Baseline on 2f3237d; audit trail repaired; S-005 verification queued before T3 code reads.

Task ID: 5 (S-001 fix + OQ-T1 close-out)
Agent: main engineer (diff-to-fix remediation session)
Task: SEVERE S-001 — fully-overwritten deleted files destroyed (og parity); OQ-T1 resolution

Work Log:
- S-001 fix (option (a) strict parity per the finding's plan):
  * WalkerFlags::destroy_fully_overwritten (default ON): after the viability grading,
    deleted non-directory files with every probe cluster re-allocated are dropped
    (FUN_18001acc0 :21249-21262 parity); folders never destroyed ((flags&0x10)==0
    gate verified :21190); stats.destroyed_overwritten counter added.
  * SUB-BUG fixed in the same commit: VolumeBitmap::reused_ratio counted SPARSE runs
    fully into both counters; the og iterator skips -1 entries (:21220). Sparse runs
    are excluded now - an all-sparse deleted file grades Excellent (og behavior),
    and was one bad accounting step from being destroyed.
- Fail-before/pass-after: stashing ONLY cr_bitmap.cpp (old ratio + new destroy rule)
    makes NtfsVolume.EndToEndWalk fail at destroyed_overwritten==1 (actual 2: holes.dat
    wrongly destroyed) AND holes.dat==nullptr; restored, passes. The destroy rule's
    pre-fix state is pinned by the test file's own prior expectations
    (overwrit present + graded Overwritten - the exact divergence, in git history).
- Tests: EndToEndWalk updated (overwrit destroyed + counter, dirdel kept with grade,
  holes.dat kept Excellent); new NtfsVolume.OverwrittenHonestModeKeepsAndGrades pins
  the flag-off keep-and-grade mode. Suite 282/282 (Linux ASan).
- OQ-T1 RESOLVED and written into tracked-open/OQ-T1 (no Ghidra needed - the corpus
  decompile contains everything): direction = re-allocated (H2 refuted); object = the
  0x70-byte scan-built claimed-sector bit-set (ctor FUN_18002c330, marked by
  FUN_180017190/:18409 - NOT the volume $Bitmap); secondary ptr @+0x60 is ctor-NULLed
  and NEVER assigned anywhere in the decompile -> the secondary test never fires in
  the og build (dormant polymorphic hook). Consequence: our single-$Bitmap grading
  misses no og input; the claimed-set-vs-$Bitmap damaged-media edge is documented in
  S-001's Resolution.
- Audit trail: S-001 Resolution; modules/ntfs.md B1/B4/D15 -> M post-fix; matrix ntfs
  row S-001 cells -> M (S-001 fixed); SUMMARY risk #6 -> fixed + remediation log row;
  OQ-T1 file Resolution section.

Stage Summary:
- S-001 FIXED (result-set membership now og-parity by default; honest-mode one flag away).
- OQ-T1 RESOLVED (tracked-open #1 closed with decompile evidence, no code change needed).

---
Task ID: 6 (M-001 fix)
Agent: main engineer (diff-to-fix remediation session)
Task: MAJOR M-001 — NTFS bootstrap rejects record sizes the og clamps

Work Log:
- Verified first-hand (FUN_18001acc0 :21134-21142): `if < 1 -> 0x200`, `buf = max(0x400, rs)`,
  NO pow2/divisibility/rejection path. CONFIRMED-DIVERGENCE.
- Fix: degradation ladder in validate_boot_sector — unusable encodings (0/non-pow2/non-divisible/
  neg-exponent overflow) -> og 0x200 fallback; usable values kept; 0x400 min clamp both paths.
  No strict flag: both callers (NtfsVolume::open, lost::probe_ntfs) benefit; the classifier
  now scores torn-DBR candidates instead of forfeiting bpb-valid(25)+mft-probe(30) points.
- Tests (fail-before reproduced by stashing the fix — all 5 fail on ERR_BAD_PARAMETER):
  BootNonPow2RecordSizeClampsAndScans (degraded size happens to equal the true geometry here —
  the walk still yields its normal 9+ nodes), BootZeroRecordSizeFallbackScans,
  BootNegativeExponentOverflowFallsBack, BootUsableLargeRecordSizeKept (4096 kept raw — the
  og keeps any >=1 usable value), RemnantReachableUnderTornDBR (end-to-end availability:
  torn DBR -> open -> remnant sweep -> shifted-header remnant emitted).
- Suite 287/287 at this commit (N-001's two tests staged separately for the next commit).

Stage Summary:
- M-001 FIXED: torn-DBR NTFS volumes (the product's primary scenario) are now attempted with
  a degraded-but-usable record size + remnant pass instead of refused whole.

---
Task ID: 7 (N-001 fix)
Agent: main engineer (diff-to-fix remediation session)
Task: MINOR N-001 — $MFTMirr fallback hardcoded records 0..3 vs runlist span

Work Log:
- Verified first-hand: InitMFTMirror runlist decode + span computation (:17411-17413) and the
  ReadFileRecord gate `record <= this+0x70` (:17460). CONFIRMED-DIVERGENCE.
- Fix: enum_mirr_attrs computes mirr_record_capacity_ from the $DATA runlist span (records =
  span_bytes/record_size, cap 1024); read_record gates on it. First unnamed $DATA decides;
  resident -> mirror invalid (og "$MFTMirror::$DATA is resident!" abort parity, :17378-17382).
  Finding corrected in passing: the old code didn't just "leave mirr_valid_ false" on resident
  content — it tried to decode the CONTENT as a runlist (fabricated-mapping risk); true og
  semantics implemented.
- Tests: MirrorCoversRunlistSize (16-cluster/8-record mirror + primary hole; record 5 — past
  the old <4 window, inside the span — MUST be served; fails ERR_RANGE pre-fix, proven against
  the M-001-only build), MirrorResidentDataRejected. Fixture quirks materialized explicitly
  (mirror slots overlap the MFT tail; record-2 slot was a build-order zero placeholder — the
  Q65 "mirror read path" test gap was real).
- Suite 289/289 (Linux ASan). Audit trail: N-001 Resolution, ntfs.md C20/S35 rows, matrix,
  SUMMARY remediation log.

Stage Summary:
- N-001 FIXED: legacy >4-record mirrors serve their full span; corrupt resident mirrors can no
  longer fabricate mappings; modern 4-record mirrors unchanged.

---

Task ID: 1-C (continuation leg 3 bootstrap)
Agent: main engineer (diff-to-fix remediation session, continuation)
Task: Session bootstrap — baseline record, environment bring-up, green-light verification

Work Log:
- Pulled both repos fresh: core HEAD 2545b83 (audited commit 67193b6; 22 commits between,
  including prior legs' A-001/S-005/S-001/M-001/N-001 fixes and OQ-T1 resolution),
  corpus 03ad46c (unmoved since audit).
- Token hygiene verified: real token string appears nowhere in either repo (worktree grep
  + git log -S both empty). Session prompt already saved token-redacted at
  worklog/session-diff-fix-prompt.md; re-read twice before starting.
- Environment: cmake 4.4.4 + ninja installed to ~/.venv (environment had none).
  Configured + built linux-gcc preset (RelWithDebInfo + ASan) clean.
- Baseline verification: cr_tests 275/275 PASS, cr_ui_tests 13/13 PASS,
  cr_ui_abi_check OK. main is green at 2545b83.
- Prior-session state reconciled against git log + SUMMARY.md remediation log:
  FIXED: A-001, S-001, S-005, M-001, N-001. RESOLVED: OQ-T1.
  The 448a57c coordination claim (S-008 -> S-006 -> S-007 lane) was NEVER executed —
  no commits exist for those findings. This leg picks them up.
- REMAINING SCOPE for this session: S-002, S-003, S-004, S-006, S-007, S-008, S-009;
  M-002, M-005, M-006; N-002..N-009; I-001..I-005; OQ-T2..OQ-T9.

Stage Summary:
- Baseline green at 2545b83 (275+13+ABI). Scope confirmed: 7 SEVERE, 3 MAJOR, 8 MINOR
  files, 5 INFO, 8 tracked-open remain. Starting with SEVERE tier per mandate order.

Task ID: 8 (S-002 fix)
Agent: main engineer (diff-to-fix remediation session)
Task: SEVERE S-002 — HFS+ Attributes B-tree not walked; decmpfs/named-fork content unrecoverable

Work Log:
- First-hand decompile verification (the audit's corpus cites were artifact-level):
  HFSPlusSearchDLL FUN_180009c10 — "com.apple.decmpfs" (string VA 0x18001f480) looked up in
  the ATTRIBUTES B-tree (cnid 8) with keyLength counting itself ((n+7)*2, :7771), fileId@+4,
  nameLen u16@0xC, name UTF16 units@0xE; record gate recordType==0x10 (:7825) with the
  unsupported-format log branch confirming non-0x10 records are SKIPPED; size@+12, payload@+16
  (:7826-29); the value's u64@+8 extracted in one native LE load (:7841). Binary scan: the
  string exists in og HFSPlusSearchDLL.dll. CONFIRMED-DIVERGENCE.
- Implementation: attributes B-tree leaf-chain walk (single pass, bounded) collecting
  {cnid, name, on-disk value offset, decmpfs facts}; decmpfs files (empty data fork + mode 3/4)
  emit COMPRESS-CODEC EXTENTS (new kZlib = RFC1950 in cr_decomp + writer case): mode 3 ->
  the zlib stream inside the attributes node; mode 4 -> the resource fork at the recorded
  offset. The recovery writer inflates through its existing compressed-extent contract -
  no memory-resident blob, no session bloat, sizes bounded only by the writer's pre-existing
  256 MiB guard. Named attributes -> ..namedfork/<name> child nodes with raw-byte extents.
  Flags resolve_decmpfs/named_forks (default on); stats attr_records/attr_unsupported/
  decmpfs_files/named_forks.
- Tests: DecmpfsInlineAndRsrcContentRecovered (mode 3 + mode 4 + named fork + unsupported
  record skip; decodes BOTH files through the writer's own decompress_block contract with
  exact-content memcmp; extent offset/psize/length asserted against the fixture bytes) +
  DecmpfsDisabledLeavesEmptyShell (flag-off pins the pre-S-002 shape). Runtime fail-before:
  engine .cpp reverted alone -> every decmpfs assertion fails; restored -> green. Suite
  291/291 (Linux ASan); fuzz_hfsplus drives scan_hfsplus so the new parser is fuzz-covered.
- Docs: PARITY-DELTAS D-46 (named-fork superset, registered per the finding's instruction);
  finding Resolution; modules/hfsplus.md B7 rows; matrix hfsplus B cell; SUMMARY risk #3
  + remediation log.

Stage Summary:
- S-002 FIXED: macOS-compressed files (decmpfs modes 3 and 4) recover their true content
  through the writer's codec-extent path; named-fork/extended-attribute content surfaces
  as recoverable children; the constant that was "defined and never used" is now a full
  subsystem, og-parity in membership and mechanism.

---
Task ID: 9 (S-004 fix)
Agent: main engineer (diff-to-fix remediation session)
Task: SEVERE S-004 — ext4 journal salvage descriptor-referenced-only; no jsb gate

Work Log:
- Both falsifiable claims re-verified against the finding's decompile cites (CORRECTIONS row 6
  folded in): the og gates on jsb magic + blocktype {3,4} at journal block 0 (:12207/:12212),
  walks the jsb-declared range in 1 MiB windows (:12219-24) and feeds EVERY block to the dir
  parser with per-block cancel (:12234-38). CONFIRMED-DIVERGENCE on both axes.
- Fix: jsb gate + s_first/s_maxlen range + every-block parse (headers self-reject through the
  dir grammar). The descriptor pass is demoted to the escape map: descriptor-tagged ESCAPE
  blocks get a second magic-restored parse (strictly better, accept set unchanged — the
  audit's prescribed keep). jsb-gate decision = strict parity (the claim-2 superset is
  closed); the self-contradictory D-07 comment rewritten.
- Tests: JournalUnreferencedDirBlockSalvaged (journal block 6 dir remnant, NO descriptor ->
  surfaces; junk block 7 tolerated), JournalFirstBlockNotJsbNoSalvage (descriptor-headed
  journal -> 0 salvaged, og parity). Fail-before proven by reverting the engine alone (both
  tests fail). Existing JournalSalvage + JournalUncommittedTailSurfaces fixtures updated with
  the jsb-declared range and still pass unchanged in their assertions.
- Suite 293/293 (Linux ASan). Audit trail: S-004 Resolution, ext4.md A25 rows, matrix ext4
  A/B cells, SUMMARY remediation log.

Stage Summary:
- S-004 FIXED: ext4 journal salvage now yields the og's full name set (unreferenced remnants
  included) and refuses exactly the journals the og refuses.

---

Task ID: 5-a (S-008 fix — this continuation leg's lane)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: SEVERE S-008 — RawFixer boot-sector-only subset + not wired pre-scan

Work Log:
- Re-verified the finding first-hand against the corpus BEFORE any change:
  rawfixer-v2--method-inventory.md (33-method table, §3 CheckMFT record-size math),
  rawfixer-fs-repair.md (strategy trees, pre-scan position), and the raw decompile
  (ghidra-partition/RawFixer_decompiled.c): FUN_18005de40 = 'FILE' magic + record-name
  check; FUN_18005df00 = $MFTMirr name check; FATFixer::CheckFatTable @0x10e reads BOTH
  copies (wFatTableStartSector / backupFatStartSector = reserved + sectors_per_fat) and
  flags damage; NTFSFixer::CheckSector classifies $MFT/$MFTMirr records by state.
  Finding CONFIRMED-DIVERGENCE, no corrections needed.
- Repair methods in cr_rawfixer.cpp/.h: CheckMFT (record 0: magic + fixup_validate +
  $FILE_NAME == "$MFT"; mirror slot 0 validated the same way -> RestoreMftRecordFromMirror;
  both invalid -> advisory only), CheckFatTable (64 KiB-block stream compare of both FAT
  copies; exactly-one-valid-signature -> sector-granular coalesced RestoreFatSectorFromMirror
  actions; equal validity + diff -> S3 advisory, never a coin-flip write), CheckRootDir /
  IsValidRootDir / CheckRoot report-only advisories (plan.advisories), FixByScanSector
  (bounded 64 MiB window, 64 KiB reads, family-masked, CRC32 content-dedupe; distinct
  multiples refused S3) wired as the deep fallback in the NTFS / exFAT / FAT / unknown
  branches. Safety standard S6 scope line rewritten (FAT mirror copies + $MFT record 0
  from mirror are now in scope; directory content never written).
- Pre-scan wiring: ScanConfig::pre_scan_repair + RepairHooks (consent/open_writer/
  close_writer); run() executes repair::analyze BEFORE the retry/cache pipe (writes bypass
  the cache; engines see repaired bytes); new event::Type::Repair (ready/applied/denied/
  failed outcomes); ScanStats repair fields incl. the undo log; denied/failed repair never
  aborts the scan (corpus fall-through).
- CLI: rrecov scan --repair (dry-run) / --repair-apply (the flag is the consent) /
  --repair-undo <file>; VolumeWriteSlice maps volume-scan writes through the slice;
  undo log persisted + summary printed. Usage text updated.
- cr_ui C ABI (ABI-since 1.2, append-only; cr_ui_event consumes the reserved slots ->
  layout unchanged): pre_scan_repair + cr_ui_repair_consent_fn + cr_ui_repair_plan POD,
  CR_EVT_REPAIR, repair fields in cr_ui_scan_stats, cr_ui_scan_repair_save_undo; shim
  bridges: image files reopened r+b (portable), Windows disks/volumes via open_disk_rw +
  UiVolumeWriteSlice; resumed scans keep the gate; ABI-CONTRACT.md §6 + header updated.
- Tests (16 new, ALL fail-before-proven by stashing only the two src files and re-running:
  9/11 rawfixer + 3/3 orchestrator failed pre-fix; the 2 trivially-green ones are
  clean-volume never-touched cases that also pass pre-fix by construction):
  RawFixer.MftRecord0RestoredFromMirror / MftCleanNeverTouched / MftBothInvalidNoWrite /
  MftRestoreMakesVolumeOpenable (claim (1) at the engine boundary: open fails torn ->
  succeeds repaired -> fails again after undo) / FatTornMirrorCopyRestoredSectorGranular /
  FatTornPrimaryRestoredFromMirror / FatMirrorAmbiguityNeverWritten / FatCleanMirrorNoAction /
  FixByScanSectorRebuildsBootFromOnlyCandidate / FixByScanSectorAmbiguousCandidatesRefused /
  FixByScanSectorIdenticalCandidatesNotAmbiguous;
  Orchestrator.PreScanRepairRestoresTornFatAndScans (claim (2) end-to-end + events) /
  PreScanRepairDeniedNeverWrites / PreScanRepairWithoutHooksNeverWrites;
  CrUi.PreScanRepairConsentGateAndUndoLog / PreScanRepairDeniedLeavesFileUntouched (C ABI,
  on-disk image repair verified) + tests/ui_abi_check.c extended (pure C consumer; one
  ASan-caught 512-byte-write-from-4-byte-buffer bug in MY test code fixed before landing).
- Full suite 297/297 (Linux ASan); cr_ui_abi_check runs green locally. Windows CI (MSVC
  /W4 /WX + the extended ABI check) to confirm on push.
- Audit trail: S-008 Resolution written; modules/rawfixer.md verdicts -> M (S-008 FIXED);
  CROSS-REF-MATRIX rawfixer row -> M (S-008 fixed) in B/A/F; SUMMARY risk #4 rewritten +
  remediation log row + per-module table annotations; CAPABILITY-MATRIX RawFixer row
  updated; ABI-CONTRACT.md §6 CR_EVT_REPAIR documented.

Stage Summary:
- S-008 FIXED end-to-end: both falsifiable claims proven with tests (torn $MFT record 0 +
  intact mirror -> engine opens post-repair; torn FAT1 + intact FAT2 -> repaired pre-scan
  after consent, structured scan proceeds), consent-gated per S1 with default-deny,
  transactional guarantees unchanged, og fall-through preserved. cr_ui ABI extended
  append-only (event layout unchanged via the reserved slots).

Task ID: 10 (N-002 + N-003 fixes)
Agent: main engineer (diff-to-fix remediation session)
Task: MINOR N-002 (FAT EOC families) + MINOR N-003 (exFAT entry-set checksum default)

Work Log:
- N-002: FAT32 entry_at applies the og's mask-24 tolerance on the RAW entry
  ((raw & 0x07FFFFF8) == 0x07FFFFF8 -> EOC, FATSearchDLL Next :8030-8048) before the
  canonical 28-bit tests; the test subsumes the og's raw 0xFFFFFFF8 form. The alternate
  family was already covered by the reserved-range branch (pinned by the test). The
  single-entry approximation for the og's two-slot forms documented in code.
- N-003: ExfatScanFlags::validate_entry_sets default flipped true -> false (og parity;
  the verified-absence decompile search holds). Honest-mode strictness kept behind the
  explicit flag (never drop, downgrade only); registered as PARITY-DELTAS D-47.
- Tests: FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean (chains {5,6,0x27FFFFF8},
  {20,0xFFFFFFF8}, {10,0x0FFFFFF5} all end clean - unit-level FatTable+follow_chain);
  ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile (new, default flags) +
  SetChecksumMismatchMarksDamaged (updated to the explicit honest-mode pin).
- Fail-before proven by stashing both source files: 3 failing tests; restored suite
  295/295 (Linux ASan).

Stage Summary:
- N-002 + N-003 FIXED: FAT chains written by BIOS/embedded formatters end cleanly like
  the og; exFAT grading is og-parity by default with the honest strictness one flag away.

---
Task ID: 11 (I-001..I-005 acknowledgements)
Agent: main engineer (diff-to-fix remediation session)
Task: INFORMATIONAL I-001..I-005 — verify rationale, ACKNOWLEDGE, register, pin

Work Log:
- PARITY-DELTAS coverage check per each finding's "check before duplicating":
  already registered: I-001 (D-12 known-metafiles+recycle), I-002 (D-39 verdict,
  FAT-4 cross-ref), I-005.1 RAR5 + I-005.2 PST (D-13a/b), I-005.3 ashift (D-04),
  driver-absent fallback (D-37/D-24). Missing rows registered as D-48:
  btrfs mirror probing + deep scan (I-003), ReFS version-gate superset (I-004),
  64 KiB transport envelope (I-005.4).
- Rationale verification: I-001 (first-hand: the blanket '$'-skip at FUN_18001acc0
  :21186 confirmed; the known-20 + recycle remap is a reviewed recovery-positive
  superset), I-002 (CORRECTIONS row 3 confirms the og's raw-follow; core keeps the
  honest truncation - the INFORMATIONAL classification holds, no code change),
  I-003 (NULL-stub + abort-rule decompiles verified in the finding), I-004 (gate
  semantics documented in code comment), I-005 (each sub-row spot-checked).
- Pin tests added: NtfsVolume.UserFileNamedDollarNotHidden (I-001's requested
  regression guard - active "$notes.txt" must surface; guards against anyone
  "fixing" parity by restoring the blanket skip), FatEngine.Fat16BadClusterStillCanonical
  (I-002's second test: FAT16 0xFFF7 stays CHAIN_BAD - the og canonicalizes there).
  I-003's requested pin ALREADY EXISTS as Btrfs.MirrorSelectionNewestGeneration
  (primary smashed -> mirror recovers) - noted in the resolution.

---
Task ID: 12 (N-010 fix — discovered divergence)
Agent: main engineer (diff-to-fix remediation session)
Task: NEW MINOR N-010 — FatTable::next swallows the BAD sentinel into the reserved-range EOC branch

Work Log:
- I-002's requested regression pin (FAT16 0xFFF7 stays CHAIN_BAD) FAILED against current
  main - the pin found a real divergence the audit's FAT wave missed: CHAIN_BAD
  (0x0FFFFFF7) sits inside the 0x0FFFFFF0..F7 reserved window, and next()'s range test
  re-interpreted it as a clean EOC end. The og canonicalizes 0xFFF7 -> 0xFFFFFF7 -> out
  of range -> chain BROKEN (FATSearchDLL :54125). Grading-only divergence (same bytes).
- Fix: the v == CHAIN_BAD test moved before the reserved-range branch in next().
- Filed as diff-to-fix MINOR N-010 (announced in the commit), fixed with the discovering
  test as its fail-before proof. Suite 313/313 (Linux ASan) after the fix.

Stage Summary:
- N-010 filed and FIXED: BAD-terminated chains grade broken on both sides again; the
  I-002 pin passes; chains_broken accounting counts them once more.

Task ID: 18-a (CI red repair — this leg)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: Repair red Windows/lint gates on main (both legs' accumulated commits)

Work Log:
- Both CI gates were red across the parallel legs' pushes (e93decf..51ac671 and mine):
  * windows: test_hfsplus_volume.cpp used std::search without <algorithm> - GCC's
    transitive includes masked it on Linux, MSVC refused (error C2039/C3861) - root
    cause of the S-002 commit's red build.
  * lint: 42 files accumulated clang-format violations from both legs' commits
    (cr_ui/cr_hfsplus/cr_carve/cr_btrfs/cr_zfs/test_* etc).
- Fixes: added <algorithm> to test_hfsplus_volume.cpp (S-002 fallout) and <utility> to
  test_orchestrator.cpp (MSVC-strict std::pair); ran clang-format 18.1.8 - THE CI
  VERSION, installed via pip to avoid version drift - over every violating file
  (formatting-only; verified behavior-preserved by the full suite).
- Full suite 309/309 (Linux ASan) after the repair; local clang-format gate now clean
  with the exact CI toolchain version.

Stage Summary:
- CI red root causes fixed (missing include + format debt); push to follow; Windows CI
  to be verified green before proceeding to the next finding in this leg's lane.
Task ID: S2-D
Agent: parallel auditor D (session 2)
Task: STATIC analysis — performance parameters (X47-X53), quality (Q65-Q70), API (P42-P46), CLI completeness; no og-execution.

Work Log:
- Read full content of 13 core source files (cr_cached.{cpp,h}, cr_slice.cpp, cr_devmap.cpp,
  cr_retry.cpp, cr_directalign.h, cr_transport.cpp [888 LOC], cr_parallel.{cpp,h}, cr_ui.{cpp,h}
  [~3100 LOC combined], rrecov.cpp [1499 LOC], cr_scan.cpp [1270 LOC]); plus cr_safewriter.cpp
  [586 LOC], cr_query.cpp [309 LOC], cr_carve.cpp [457 LOC], cr_device.h [203 LOC] for cross-checks.
- Read corpus: 05-scan-engine-architecture.md, ntfs-engine-internals.md (64 KiB MFT batch +
  GlobalMemoryStatus adaptive sizing claim), DRWUI-exe--thread-architecture.md (45-thread census),
  rawsearch-carving--deep-dive.md, NTFSSearchDLL.dll/strings_ascii.txt (GlobalMemoryStatusEx,
  SetFilePointerEx verified; no ReadFileScatter/ReadFileEx in imports), RAWSearchDLL.dll/strings_ascii.txt
  (GlobalMemoryStatusEx + HeapAlloc verified).
- Verified the existing audit findings against current HEAD 5d6da5f: A-001 fix in tree; S-001 fix
  in tree; S-005 fix in tree; M-001 fix in tree (just-landed this session); GAP-S6-11 disk-map ABI
  surface in tree (cr_ui_disk_map, cr_ui_disk_map_free at cr_ui.h:492-500 + impl cr_ui.cpp:1461+).
  Audit baseline 67193b6 → current HEAD 5d6da5f = 6 commits (1 of which is the worklog repair).
- Static-only findings (NEW divergence candidates the prior audit did not file):
  * X51 RAWFIXER BYPASS: cr_transport.cpp:748 — RwDevice::read_at calls overlapped_read DIRECTLY,
    bypassing aligned_read_at (the GAP-S6-13 64 KiB envelope). DirectDevice (line 218) and
    RdaTargetDevice (line 268) both go through aligned_read_at; RwDevice (RawFixer's read path)
    does NOT. Recovery writer's source (DirectDevice/PartitionSlice) IS covered. So the gap is
    specifically the repair path. SEVERITY candidate: MAJOR (perf regression only — slow path
    on NVMe for non-aligned reads; correctness OK because overlapped_read just calls ReadFile
    which honors sector alignment).
  * X53 INCONSISTENT CANCEL WATCHER: cr_carve.cpp:418-429 has a 50 ms cancel watcher thread
    that calls pool.abort() to drain queued windows. cr_scan.cpp's parallel candidate scan
    (lines 952-1003) does NOT have an equivalent watcher — workers poll the external cancel
    token inside scan_candidate (engine loop heads). Result: a queued candidate that hasn't
    started yet won't bail until its FIRST engine loop head — could be many records in if
    the engine is slow to reach its first poll. SEVERITY candidate: MINOR (worst-case latency
    is one candidate's first poll, bounded by ~1 record of work).
  * X49/X51 PERF-DOC DRIFT: docs/PERFORMANCE-PROFILE.md:52 says "CachedDevice 64 KiB block
    batching (1 MiB cache)" — the orchestrator actually creates CachedReader with 8 MiB at
    cr_scan.cpp:540. AUDIT-ARCH.md:37 cites 8 MiB correctly. Q68 finding.
  * Q65 missing tests: no test for hard-link destroy (S-001 fix added destroy_fully_overwritten
    but no hard-link-specific test), ADS emission accounting, sparse file accounting (sub-bug
    noted in S-001 fix worklog but no isolated regression test), FAT cross-linked chain,
    exFAT multi-fork, ext4 extent tree depth > 2, HFS+ resource fork recovery, Btrfs subvolume
    rooting, ZFS snapshot, ReFS integrity stream, LDM RAID-0 stripe alignment, LDM RAID-1
    mirror read, lost-partition overlap scoring (audit row 31a mentions the penalties but no
    test exercises them). Q65 partial-coverage finding.
  * Q66 FUZZ GAPS: fuzz/ has 13 harnesses (ntfs, fat, exfat, ext4, hfsplus, btrfs, zfs, refs,
    carve, preview, container, engines, pure). MISSING dedicated harnesses: ldm (only via
    fuzz_engines selector), table_parse (only via fuzz_engines/pure), rawfixer (via fuzz_pure
    + fuzz_engines but no dedicated), session (covered by fuzz_container), safewriter (NO
    coverage — DestIo + queue + collision logic), destio (NO coverage), bitlocker (NO coverage).
    Plus: fuzz CI is `if: false` (M-006 from prior audit; still in tree).
  * Q67 UBSan ABSENT: ASan is enabled on cr_core/cr_ui/cr_tests/fuzz (linux-gcc preset) and
    TSan runs on the linux-tsan CI lane. UBSan is NOT enabled anywhere — no -fsanitize=undefined
    in any CMake file. No sanitizer suppression files exist (good). Q67 = ASan clean / TSan
    clean / UBSan UNVERIFIED.
  * Q68 doc-drift:
    - AUDIT-PIPELINE.md:179-185 — WRITER-1 listed as "OPEN" but the code at cr_safewriter.cpp:455-465
      has the fix (only ERR_CANCELLED aborts; per-item errors collected). The doc describes the
      pre-fix behavior.
    - diff-to-fix/modules/cli.md:14 — claims "verify, query" commands exist; the actual CLI
      (rrecov.cpp:1459-1499) has only 7 commands: list, scan, tree, recover, health, fix, bench.
      No `verify` command, no `query` command. SESS-2 verify-identity is API-only
      (cr_ui_scan_verify_identity); query is API-only (cr_ui_query_run). The CLI's tree --json
      uses query::run internally but doesn't expose query predicates to the user.
    - diff-to-fix/modules/cli.md:12-13 — exit codes documented as "0 success / 1 usage / 2 scan
      error / 3 recover error" but rrecov.cpp:17-19 documents: scan 0/1/3 (3=cancelled), recover
      0/2 (2=partial), health 0/3 (3=no-source), list 0/1. cli.md conflates "scan error" and
      "recover error" with the wrong codes.
    - docs/PERFORMANCE-PROFILE.md:1 — "baseline measurements at HEAD 064fcf4 (session 6)" — current
      HEAD is 5d6da5f; baseline is stale (acceptable as a historical reference but should note
      "see diff-to-fix for current status").
    - docs/PERFORMANCE-PROFILE.md:50 — "BELOW the 64 KiB contract (GAP-S6-13)" — GAP-S6-13
      LANDED (commit 7777e1c, in tree). The "below" claim is stale; the envelope is in.
    - diff-to-fix/findings/SEVERE/S-009-MAJOR-M-006-diskmap-surface-and-fuzz-ci.md — S-009
      "Disk-map visualization surface absent" — disk-map surface LANDED via GAP-S6-11 (commit
      56cce87). cr_ui.h:492-500 + cr_ui.cpp:1461+ implement cr_ui_disk_map. The finding's
      Status field still says "READ-ONLY FINDING — no fix applied" — stale.
  * Q70 Win7 posture — verified: .github/workflows/ci.yml:114-135 runs
    stamp_osversion_um.py + verify_pe_um.py with allowlist tools/ref_imports_um.json on
    rrecov.exe, cr_tests.exe, cr_ui.dll. tools/test_verify_pe_um.py self-tests the gate on
    Linux. MATCH.
  * P42-P46 cr_ui ABI surface — verified MATCH-with-additions:
    - P42 surface shape: cr_ui.h exposes 57 C11 exports covering scan lifecycle, sessions,
      snapshots, query, preview, recovery, health, disk map, event stream. The og's analyser-
      list/map exports (EUGetAnalyserList/Map/Multimap + GetRawUserInfoMap) have NO core
      counterpart — the carve registry is queryable internally but not via ABI (minor; not
      blocking for a UI).
    - P43 ownership: opaque handles (cr_ui_device, cr_ui_scan, cr_ui_session, cr_ui_snapshot,
      cr_ui_preview, cr_ui_recovery) with close-every-handle semantics; borrowed devices must
      outlive scans/previews (documented in cr_ui.h:175-177). Mixed raw-pointer/smart-pointer
      internally — handle types are typedef'd structs; the shim uses unique_ptr internally.
      Match.
    - P44 async: PausableToken (cr_device.h:133-199) with pause/resume/cancel from any thread,
      checkpoint() blocks engines while paused. Backpressure is implicit (single-flight progress
      bridge in cr_ui.cpp:590-638). The og's event-bus + scan-abort-event split is documented
      as an implementation difference with equivalent guarantees. Match.
    - P45 ABI version: CR_UI_ABI_VERSION = 1u (cr_ui.h:69). ABI-CONTRACT.md §9 documents the
      stability policy: appended fields don't bump the version, only removal/rename/field-change
      does. The two appended fields (cr_ui_event.reserved[2], cr_ui_recovery_stats.nothing_to_recover)
      carry "ABI-since 1.1" inline markers — consistent with the policy. Match.
    - P46 documentation: docs/ABI-CONTRACT.md (291 LOC) is normative; tests/ui_abi_check.c
      (C consumer test, MSVC /W4 /WX) is the executable form. Match.
  * CLI completeness vs audit-prompt list:
    - PRESENT: --json, --filter, --report, --no-carve, --deep (sets whole_disk; semantically
      odd — deep stage is on by default, --deep enables lost-partition discovery, not the deep
      stage itself), --quick-only (not --quick), --out (not --output), --resume (not --session),
      fix command (not --repair flag).
    - MISSING: --help (implicit only via empty argv), --version, --quick, --no-bitlocker,
      --raw-fixer/--no-raw-fixer (uses `fix` command instead), --verify (API only), --query
      (API only), --output (alias for --out/--to/--in), --session (alias for --resume).
    - The documented CLI contract per PARITY-ORCH.md:49 and PARITY-UI.md:53 lists exactly the
      flags rrecov.cpp implements; the audit-prompt's expanded list was a supposition. CLI is
      MATCH with its own documented contract (C24 exit codes match rrecov.cpp's docstring, NOT
      cli.md's incorrect summary).
  * MATCH items re-verified (one-liner each):
    - X47 throughput: 64 KiB-aligned direct reads + 64 KiB cache block verified — cr_transport.cpp:148-170
      (aligned_read_at rda_scaled=false path uses io::direct_envelope), cr_scan.cpp:540 (CachedReader
      8 MiB / 64 KiB block).
    - X48 latency streaming: cr_ui_event typed stream + cr_ui_scan_telemetry + cr_ui_scan_snapshot
      all callable MID-RUN (cr_ui.h:312-334) — partial=1 stamped; events fire from worker threads
      at stage boundaries + coalesced progress. Match.
    - X49 memory: bounded CachedReader (capacity_ block count), bounded ThreadPool (≤64),
      bounded writer read_chunk (1 MiB default), carve window (1 MiB + 4 KiB overlap) — no
      unbounded growth identified. Per-stage/per-engine cap: no explicit per-engine budget
      (engines allocate freely within their scan; the tree is the dominant sink). No explicit
      og parity claim to verify against.
    - X50 CPU: zero SetThreadPriority anywhere in src/include (grep clean). ThreadPool
      auto_workers = hw-1 clamped 1..8 (cr_parallel.cpp:13-20). Resolve_threads in orchestrator
      also clamps 1..64 (cr_scan.cpp:43-53). The og's priority-7 workers and engine-side worker
      threads (FAT CFATQuickSearchWorker) have no direct core counterpart — core extends the
      host-fans-out model with first-class pool only (cr_parallel.h:7-15 documents this honestly).
      Match.
    - X52 carve dedup: extent-set-based via find_by_extent(cand_abs + ext_base, len_class)
      (cr_carve.cpp:128,130,249,251). len_class = coarse bucket (<1MiB: len/65536; >=1MiB:
      pow2 ceiling + 64). Writer collision: claimed_names_ set + io_.exists() + rename policy
      (cr_safewriter.cpp:47-79). Query Path sort: O(N*D) for path materialization (acceptable
      for D ≤ ~20). No O(N²) identified.
    - X53 cancel watcher: 50 ms watcher EXISTS in carve parallel path (cr_carve.cpp:418-429).
      Writer checks cancel per-chunk in recover_item_impl_ (cr_safewriter.cpp:277) — ~2 ms
      responsive at 1 MiB chunks. Per-stage-boundary checkpoints in orchestrator (cr_scan.cpp:607,
      615, 628 — three per scan). No per-record checkpoint (engine progress callbacks fire per
      record; checkpoints are stage-boundary only — matches the og's auto-save-at-stage-boundary
      model).
    - Q69 stub/TODO/FIXME: grep over src/include for TODO|FIXME|XXX|HACK|future work|not implemented|stub
      returns ZERO real matches (the four hits are coincidental: "U+XXXX" hex pattern, "kInfoFlagXxx"
      constants, "0x5002xxxx" error codes, "RtlNtStatusToDosError" comment). Clean.

Stage Summary:
- STATIC audit complete: 5 new divergence candidates identified (X51 RawFixer bypass, X53 carve
  vs orchestrator cancel-watcher inconsistency, Q65 missing per-parameter tests, Q66 fuzz gaps
  + UBSan absence, Q68 doc-drift across 5 spots), plus CLI completeness mapped. All cited with
  file:line evidence; no source modified; no og-execution required.
- The most actionable NEW finding is X51 RawFixer bypass (1 line fix: route RwDevice::read_at
  through aligned_read_at with rda_scaled=via_rda_). All other items are either documentation
  drift (Q68) or known gaps (Q65 test debt, Q66 UBSan, M-006 fuzz CI).
- Returned as a structured context sheet (this worklog entry is the long-form evidence; the
  final message to the main agent is the structured markdown report).

---
Task ID: S2-C
Agent: parallel auditor (session 2, parallel agent C)
Task: Safety + writer + session semantics line-by-line verification (F60-F64, writer
behavior beyond N-009b/d, session resume beyond D12, R54-R58 architecture parameters)

Work Log:
- Read completely (audited commit 5d6da5f):
  * src/core/recovery/cr_safewriter.cpp (586 LOC) + header
  * src/core/recovery/cr_report.cpp
  * src/core/session/cr_crsf.cpp (332 LOC) + header
  * src/core/session/cr_records.cpp (389 LOC) + header
  * src/win/cr_bitlocker.cpp + header
  * src/win/cr_destio.cpp + header
  * src/core/repair/cr_rawfixer.cpp + header
  * src/core/orchestrator/cr_scan.cpp (1,270 LOC) — full read
- Read corpus references:
  * architecture/04-recovery-workflow.md (writer + dest validation)
  * architecture/06-repair-chain.md (RawFixer chain)
  * SaveScan/rsf-format--scan-session.md (full rsf spec)
  * ghidra-partition/RawFixer_decompiled.c (CRawFixer::Start @73997, CheckPartition @74068,
    CCallBackOperate use, FixByScanSector strings)
  * ghidra-partition/BitLockerLib_decompiled.c (PSUnlockBitLocker @27557,
    UnlockVolumeAndRefresh @28607, AesDecryptSectors @25269)
  * ghidra-partition/RecoveryModule_decompiled.c (SetFileSecurityW @74671/78016/79886,
    GetFileSecurityW @74665/78076-82, SetFileAttributesW @79677, GetDiskFreeSpaceExA @47033,
    FSCTL_LOCK_VOLUME @44686, FSCTL_DISMOUNT_VOLUME @44713, FSCTL_IS_VOLUME_MOUNTED @44708)
  * PartitionRecovery/bitlockerhelp.mo strings (UnlockWithPassphrase/Numerical/External,
    GetKeyProtectors, FVEAutoUnlock registry path, ExecuteUnlock)
- Cross-checked prior audit outputs: N-009d (ACL), S-008 (rawfixer wiring), BENCH-F1
  (torn-tail salvage), F10 (CRC + chunk fallback), F-1 (extent basis), SESS-2 (identity
  card v2 tail), GAP-S6-10 (typed events).

Findings (this session):
1. CORROBORATES N-009d (F64 ACL/SDDL): first-hand decompile evidence — og uses
   Security_Proc::CFileSecurityInfo with GetFileSecurityW(OWNER|GROUP|DACL|SACL=1|2|4|8)
   then SetFileSecurityW(per-component). Core has zero ACL handling in cr_destio.cpp /
   cr_safewriter.cpp. Core also drops timestamps (no SetFileTime) and attributes (no
   SetFileAttributesW) — the FileProperties struct carries them but the writer never
   applies. Severity: MINOR (defensible per the N-009d rationale, but the timestamp/
   attribute drop is a NEW sub-divergence not previously noted).
2. NEW: Writer disk-full handling (F60-adjacent): core does NOT pre-check destination
   free space (no GetDiskFreeSpaceEx call in src/win/cr_destio.cpp or src/core/recovery).
   Og calls GetDiskFreeSpaceExA (RecoveryModule @47033) before writing. On ENOSPC, core
   returns ERR_WRITE_FAILED per-item and continues (every remaining item fails
   pointlessly). Severity: MINOR (waste, not data loss — temp files are deleted on
   close).
3. NEW: Stage checkpoint NOT atomic (S34/D12). cr_ui.cpp:1131 opens ck_path with
   fopen("wb") which TRUNCATES the existing file, then writes the new checkpoint.
   A crash mid-write leaves the file torn AND destroys the previous good checkpoint
   (the og's auto-save uses timestamped file names, so older checkpoints survive).
   Mitigated by F10 torn-tail salvage in the CRSF reader (cr_crsf.cpp:240-276).
   Severity: MINOR (mitigation in place; the og also lacks true atomicity but
   preserves history via new files).
4. NEW: Volume-lock divergence on REPAIR path (F63-adjacent / R58). Og's repair
   path does FSCTL_LOCK_VOLUME + DeleteVolumeMountPointA + FSCTL_DISMOUNT_VOLUME +
   FlushFileBuffers + Sleep(2000) before raw writes (RecoveryModule @44686-44720,
   PartitionRecovery @44686). Core's RwDevice (cr_transport.cpp:822) opens with
   GENERIC_READ|GENERIC_WRITE + FILE_SHARE_READ|FILE_SHARE_WRITE — NO lock/dismount.
   Concurrent processes can race with repair writes. Severity: MAJOR (transactional
   undo log mitigates the worst case but the lock is a real safety gate).
   Note: the SCAN path is MATCH — both og and core use non-exclusive reads
   (FILE_SHARE_READ|WRITE|DELETE) for the scan volume open.
5. NEW: F62 destination overlap check is OPTIONAL (have_dest_ gate). cr_safewriter.cpp:
   enqueue_common() only refuses on overlap if set_destination() was called (have_dest_
   true). cr_ui.cpp:2057-2063 conditions set_destination on (drive-letter path) AND
   (source disk_known) AND (transport != "image"). Image-file sources and UNC paths get
   NO overlap check. Og opens \\.\X: unconditionally for destination validation —
   catches every drive-letter dest. Severity: MINOR (image-source gap is documented
   platform truth; UNC gap is real but niche).
6. NEW: F62 only checks the FIRST disk extent. cr_transport.cpp:701-712 returns
   ext->Extents[0] only — spanned/RAID volumes with multiple disk extents are
   under-checked (the second+ extents are silently ignored). Severity: MINOR.
7. NEW: emit_boundary_events_ double-call bug. cr_scan.cpp:577-578 has the same
   call twice (4-space indent then 8-space indent — merge artifact). Causes
   double-emission of FilesFound + ErrorRange boundary events on every stamp()
   exit. Severity: MINOR (cosmetic, but inflates event counts in the UI).
8. CORROBORATES S-008 (F63 RawFixer consent): core's cr_rawfixer.cpp execute() has
   NO built-in consent gate — consent is implicit (caller invokes execute()). This
   matches the og's pattern (CRawFixer::Start takes a progress callback, not a
   consent callback). The S-008 finding (not wired pre-scan) covers the bigger gap.

MATCH items re-verified:
- F60 source bad-sector: RetryPolicy max_attempts=3, RetryMode::Skip + bisection +
  zero-fill + error_map (cr_scan.cpp:525-529). Og uses EPMNtDrv with retry/bisect.
  Dest writes have NO retry (asymmetric by design — dest bad sectors don't improve
  on retry). MATCH.
- F61 BitLocker: core uses PowerShell Unlock-BitLocker + manage-bde fallback +
  registry FVEAutoUnlock BEK extraction (cr_bitlocker.cpp:119-237). Og uses
  PSUnlockBitLocker + UnlockVolumeAndRefresh (manage-bde) + bitlockerhelp.mo
  UnlockWithPassphrase/Numerical/External. NO in-process XTS-AES decrypt path
  in core (the og's AesDecryptSectors @25269 is for content reads AFTER Windows
  unlock, which the core does via the file API on the unlocked volume). MATCH
  (recovery-relevant subset; the og has additional management APIs not needed for
  recovery).
- F63 RawFixer consent: MATCH (both rely on UI layer; engine has no consent flag).
- F64 ACL: divergence (N-009d, already filed).
- Writer atomic write: temp + FlushFileBuffers + MoveFileExW(REPLACE_EXISTING) in
  same dir (cr_destio.cpp:44-58) — same-volume rename = atomic. Og uses normal
  file APIs (corpus 04 §7.5). MATCH.
- Writer partial-overwrite: 3 policies (Rename default / Skip / Overwrite) in
  cr_safewriter.h:61 + recover_item_impl_:232. Og's behavior is Rename (numeric
  suffix) — corpus pattern. MATCH.
- Session resume from partial .crsf: torn-tail salvage in cr_crsf.cpp:240-276
  (magic skip-forward by CRSF_UNIT, unfinalized-tail stop, count clamp). MATCH
  (BENCH-F1 verified).
- Session CRC: per-record CRC32 (cr_crsf.cpp:166, 323-327). Og has NO explicit
  CRC (relies on zlib's Adler-32 + magic matching). Core is HARDENING superset
  (F10, already filed).
- Session identity verification: boot_sector CRC32 stored in VolumeInfo (cr_records
  :310, :334) + compared by UI (cr_ui.cpp:1319) and CLI (rrecov.cpp:598). Og
  stores full 512-byte boot sector (rsf-format §5.3 +0x518) — stronger identity
  but more space. MATCH (with caveats: CLI warns-only, UI verify is opt-in API).
- Cancel propagation: per-chunk in writer (1 MiB default, cr_safewriter.cpp:277)
  + per-item in process() (progress callback) + per-stage-boundary in orchestrator.
  Bounded by chunk time ~10-50ms. MATCH (likely better than og's per-record).
- Concurrent access (scan): both use non-exclusive FILE_SHARE_READ|WRITE|DELETE
  (cr_transport.cpp:696 and RecoveryModule scan path). MATCH.
- R54 module boundaries: core/win boundary clean (no core header includes win/).
  App/core boundary clean. MATCH.
- R55 dependency direction: NO engine includes orchestrator/query/preview/session/
  ui/win headers. The repair→engine and lost→engine includes (rawfixer uses
  exfat/fat validators; lost-partition uses ntfs/fat/exfat probers) are deliberate
  sharing patterns matching the og. MATCH.
- R56 interface shapes: IStorageDevice / DestIo / IWriteDevice abstractions with
  platform implementations. Matches og's IObject/GetInterfaceEx pattern. MATCH.
- R57 wiring: ScanController owns scan lifecycle; SafeWriter constructed by app
  with injected DestIo + IStorageDevice. Callbacks flow engine→bridge→orchestrator→UI.
  MATCH.
- R58 layering: no engine bypasses the storage layer (all device access via
  IStorageDevice). Cross-engine includes are documented sharing, not bypass.
  MATCH.

Open questions:
- OQ-S2C-1: og's exact cancel granularity in the recovery ExportThread is not
  specified in the corpus (only that cancel checks happen "PID/TID trace → cancel
  checks"). Core's per-chunk is likely finer; would need a targeted decompile of
  the og's ExportThread loop to confirm.
- OQ-S2C-2: og's ACL-preservation scope (DACL only vs OWNER+GROUP+DACL+SACL).
  The decompile shows GetFileSecurityW with all 4 SECURITY_INFORMATION values
  (1|2|4|8) — suggesting full preservation. The core preserves none.
- OQ-S2C-3: og's pre-scan RawFixer consent UX (auto vs prompted) — already
  noted in S-008's open questions.

Corpus errata candidates:
- 04-recovery-workflow.md §7.5: claims writes go via "normal file APIs" — accurate
  but understates that the og ALSO preserves ACLs via SetFileSecurityW (not just
  data). Should cross-link to the Security_Proc::CFileSecurityInfo evidence.
- rsf-format--scan-session.md §5.3: claims record 3 carries "512-byte boot sector
  snapshot" — confirmed in decompile. Should note that the core's VolumeInfo
  stores only a CRC32 of the boot sector (32-bit collision risk vs the og's
  byte-exact comparison).
- 04-recovery-workflow.md §10: mentions "Locked volumes: EUDCPEPM/EUEDKEPM hook
  drivers + volume lock via PartitionRecovery pending-op engine" — this is the
  repair-path lock (FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME) the core lacks.

Stage Summary:
- 5 new findings identified (1 MAJOR repair-path volume lock, 4 MINOR: writer
  disk-full pre-check, checkpoint atomicity, F62 optional+first-extent-only,
  emit_boundary_events_ double-call). N-009d ACL finding corroborated with
  first-hand decompile evidence (also surfaces timestamp/attribute drop as a
  sub-divergence). All R54-R58 architecture parameters MATCH. S-008 (rawfixer
  not wired pre-scan) corroborated for F63. Read-only mandate held.

---
Task ID: S2-B
Agent: parallel auditor B (session 2)
Task: Audit B9 (timestamps/timezones) + B10 (Unicode/path) for engines session 1
didn't deeply cover (FAT, exFAT, ext4, HFS+, Btrfs, ZFS, ReFS, NTFS-reread);
cross-cutting case-folding/path-separator at query+CLI layer.

Work Log:
- Read core files in full: cr_types.h (FileTimes struct, unix_to_filetime_ns
  helper with 100ns granularity fold), cr_node.h (FileProperties.times fields),
  cr_fat_wire.h (FAT DirEntry wire struct, fat_datetime_to_unix_seconds decoder),
  cr_exfat_wire.h (exFAT FileEntry with create_tz/modify_tz/access_tz at +0x16/17/18),
  cr_text.h (S-005 shared UTF-8 decoder), cr_fat_quick.cpp (FAT emit_entry time math),
  cr_lfn.cpp (fat_date_to_unix — ignores tenths via (void)tenths), cr_exfat_quick.cpp
  (exfat_time_to_unix100: applies 7-bit two's-complement quarter-hour tz_offset,
  subtracts tz_secs from base_secs to convert local-time-on-disk → UTC), cr_ext4_engine.cpp
  (decode_extra for i_*_extra fields, unix_to_filetime_ns preserves nsec), cr_hfsplus_engine.cpp
  (mac_to_filetime: (mac_sec + 9561628800)*10^7, matches og), cr_btrfs_engine.cpp
  (reads 4 timespecs at 0x70/0x7C/0x88/0x94), cr_zfs_engine.cpp (reads only the seconds
  u64 of each 16-byte SA timestamp — drops nsec), cr_refs_engine.cpp (raw le64 from
  ReFS GeneralInfo), cr_mftwalk.cpp (props->times = has_si ? si_times : best->times;
  SI-preferred-else-FILE_NAME precedence), cr_attributes.cpp (parse_standard_information
  + parse_file_name both read all 4 raw u64 100ns-1601), cr_records.cpp (session wire
  put64/g64 raw u64 — no TZ conversion at wire layer), cr_query.cpp (lower_ascii +
  contains_ci + std::string::compare — ASCII-only case folding; "/" path separator),
  cr_report.cpp (no timestamps emitted).
- Read corpus: fat-exfat-engine-algorithms-v2.md (no per-engine tz/tenths decode detail),
  fat-exfat-engine-internals.md (class model), exfat-engine--deep-dive.md (no tz detail),
  ext4-hfs-engine-algorithms-v2.md (SB+VH validation only), refs-btrfs-zfs-engine-algorithms-v2.md
  (Btrfs generation select + ZFS uberblock walk — no timestamp decode detail),
  ntfs-engine-internals.md (SI/FILE_NAME parsers, no precedence decode).
- Direct decompile reads (first-hand evidence):
  * FATSearchDLL_decompiled.c:32135-32154 — 3× DosDateTimeToFileTime (create@+0x10/+0xe,
    modify@+0x18/+0x16, access@+0x12 with time=0); on failure: stored slot = 0.
    NO creation_tenths read; NO LocalFileTimeToFileTime / FileTimeToLocalFileTime /
    GetTimeZoneInformation import.
  * exFATSearchDLL_decompiled.c:5215-5235 + 45174-45194 — 3× DosDateTimeToFileTime
    per file; create_tz/modify_tz/access_tz bytes NEVER read; NO tz conversion APIs
    imported. CONFIRMED: og ignores exFAT tz_offset.
  * EXT4SearchDLL_decompiled.c:5494-5515 + 6262-6282 — _gmtime32_s + SystemTimeToFileTime
    (wMilliseconds = 0) → SECOND precision only; reads param_2+0x0C (ctime) and param_2+0x10
    (mtime); props+0x4b and props+0x55 (3rd/4th FILETIME slots) initialized to 0 and
    NEVER assigned (atime and crtime dropped). SystemTimeToFileTime + GetLocalTime imported;
    NO FileTimeToSystemTime / FileTimeToLocalFileTime / GetTimeZoneInformation imported at
    the engine level.
  * HFSPlusSearchDLL_decompiled.c:8012,8027,8042 — 3 conversions:
    ((mac_sec + 0x239eae080) * 10000000) at param_1+0xc0/+0xc4/+0xc8 (create, content_mod,
    attr_mod). NO conversion for access_date (4th HFS+ catalog date). Matches core's
    mac_to_filetime() fold exactly; core additionally reads access_date (superset).
  * BtrfsSearchDLL_decompiled.c:4430-4446 — 3 conversions:
    (sec + 0x2b6109100)*10000000 + nsec/100 at inode+0x70/0x78 (atime), +0x7c/0x84 (ctime),
    +0x88/0x90 (mtime). NO conversion at +0x94/+0x9c (otime) — creation time dropped by og.
    Core reads all 4 (superset).
  * ZfsSearchDLL_decompiled.c:24441,24563,24685,24807 — SA attribute lookups for ZPL_ATIME/
    MTIME/CTIME/CRTIME by name; FUN_180010ef0 reads min(local_28, 8) bytes of the SA attr
    → drops the second u64 (nanoseconds) — core does the same (le64 of first 8 bytes only).
    MATCH — both drop nsec for ZFS.
  * ReFSSearchDLL_decompiled.c — no time-conversion APIs imported (only GetSystemTimeAsFileTime
    + WideCharToMultiByte). Engine reads raw le64 timestamps from ReFS GeneralInfo at
    +0x00..0x18 (matches core). MATCH.
  * NTFSSearchDLL_decompiled.c — no DosDateTimeToFileTime / FileTimeToLocalFileTime /
    FileTimeToSystemTime / GetTimeZoneInformation imported. NTFS engine reads raw 100ns-1601
    from $STANDARD_INFORMATION and $FILE_NAME, stores directly. MATCH with core.
- DRWUI.exe + DRWReport.exe + RAWSearchDLL.dll DO import FileTimeToSystemTime +
  FileTimeToLocalFileTime + GetTimeZoneInformation — but those are at the UI/report/raw-
  carve-display layer, NOT the engine layer. The og's RAWSearchDLL writes a formatted
  timestamp STRING (via "%d-%d-%d %d-%d-%d" sprintf after FileTimeToLocalFileTime +
  FileTimeToSystemTime) alongside the raw u64 in CRAWFileProperties (record-3 type 3,
  rsf-format--scan-session.md:400). The core's RAW carve doesn't generate this string
  (just stores u64). Core is leaner — not a parity break (no info lost).
- DRWUI.exe + DRWReport.exe import LCMapStringW / CompareStringW (Unicode-aware case
  folding) — used for sort/filter at UI/report layer. The SearchDLL engines do NOT
  import these. The core's cr_query.cpp uses ASCII-only contains_ci / lower_ascii /
  std::string::compare — divergence already noted as N-009c.

Findings (this session):
- NEW: exFAT tz_offset application — core applies the spec-correct 7-bit two's-complement
  quarter-hour tz_offset (subtract tz_secs from base_secs to convert local-time-on-disk
  → UTC), the og uses DosDateTimeToFileTime and IGNORES the tz_offset bytes (treating
  local time as UTC, matching FAT convention). SEVERE: byte-different session records
  (FileTime differs by tz_offset hours, typically ±8 for non-UTC users); the displayed
  time diverges from Explorer/dir which also use local-as-UTC. Core is "spec correct"
  but produces different user-visible timestamps than the og. Findings ID candidate:
  S2-B-001.
- NEW: FAT invalid-date divergence — core returns 116444736000000000 (= 1970-01-01
  00:00:00 UTC, the Unix-epoch FileTime) when fat_datetime_to_unix_seconds returns 0
  (month<1 or day<1 or month>12 or day>31). Og returns 0 (= 1601-01-01 00:00:00 UTC,
  zero FILETIME) for the same case via the `if (BVar5 == 0) stored_slot = 0` guard.
  exFAT engine in core returns 0 correctly (early `if (date == 0) return 0`); only
  FAT engine has this divergence. MINOR — observable only for zero/invalid FAT dates
  (rare; corrupt dentries). Findings ID candidate: N-010.
- NEW: ext4 nanoseconds superset — core uses unix_to_filetime_ns(sec, nsec) preserving
  nanoseconds at 100ns granularity; og uses _gmtime32_s + SystemTimeToFileTime (with
  wMilliseconds=0) → second-only precision. Core superset; .rsf byte stream differs
  (low bits). INFO. Findings ID candidate: I-006.
- NEW: ext4 timestamp completeness — core reads all 4 (atime/mtime/ctime/crtime) via
  the i_extra_isize-gated decode_extra path; og only reads ctime+mtime (param_2+0x0C
  and param_2+0x10), leaves atime + crtime slots at 0. Core superset; .rsf byte stream
  differs (created and accessed populated vs 0). INFO/MINOR — recovery products often
  use access time for filter/sort ("files accessed after date X"); og-side users get
  empty access times on ext4. Findings ID candidate: I-007 (grouped with HFS+/Btrfs
  superset notes below).
- NEW: HFS+ access_date superset — core reads all 4 HFS+ catalog dates (create_date,
  mod_date, attr_mod_date → changed, access_date); og reads only 3 (drops access_date).
  Core superset; .rsf differs. INFO. (Same findings group as ext4 above.)
- NEW: Btrfs otime superset — core reads all 4 Btrfs inode timespecs (atime, ctime,
  mtime, otime → crtime); og reads only 3 (drops otime, the creation time). Core
  superset; .rsf differs (created slot populated vs 0). INFO. (Same findings group.)
- NEW (sub-bug of S2-B-001): exFAT create_10ms / modify_10ms — both og and core ignore
  these bytes (core's exfat_time_to_unix100 doesn't take them as args; og's DosDateTime
  doesn't accept tenths). MATCH (no divergence, but documents the contract).
- Re-verified: NTFS B9 = MATCH (raw 100ns-1601; SI-preferred-else-FILE_NAME precedence
  in core code at cr_mftwalk.cpp:734; og's NTFS engine imports NO time-conversion APIs
  so reads/stores raw). Og precedence between SI/FILE_NAME times still not explicitly
  decoded in corpus — note only, no contradiction observable.
- Re-verified: FAT B9 = MATCH for the 10ms/tenths drop (both ignore), MATCH for valid
  date/time math (both produce same FileTime), DIVERGENCE for invalid date (above).
- Re-verified: HFS+ B9 Mac-epoch conversion MATCHES og exactly (same constants, same
  math); only divergence is the access_date superset.
- Re-verified: Btrfs B9 timestamp math MATCHES og exactly ((sec+EPOCH_DELTA)*10^7 +
  nsec/100); only divergence is the otime superset.
- Re-verified: ZFS B9 = MATCH (both drop nsec, reading only the seconds u64 of the
  16-byte SA timestamp attribute). Prior audit's "M-note" was correct.
- Re-verified: ReFS B9 = MATCH (raw 100ns-1601 stored, no TZ conversion).
- Re-verified: Unicode normalization (NFC/NFD) — NEITHER og engines NOR core do any
  normalization at engine level (no NormalizeString / FoldString imports in any
  SearchDLL). HFS+ NFD names pass through byte-identical. MATCH.
- Re-verified: path separator — core CLI uses "/" (cr_query.cpp:232/274/285);
  og has no CLI (GUI uses "\\"). .rsf stores tree structure (parent IDs), not path
  strings — no wire-level divergence. NA / INFO at the engine layer (only CLI output
  presentation differs).
- Re-verified: N-009c (CLI --filter ASCII mangling) — confirmed at cr_query.cpp
  contains_ci / lower_ascii (lines 47-52, 72-93): ASCII-only case folding. Og's UI
  uses CompareStringW (Unicode-aware). DIVERGENCE for non-ASCII filename search;
  already filed.

Open questions (no new OQ files; below is for the main agent's triage):
- OQ-S2-B-1: Does the og's $STANDARD_INFORMATION vs $FILE_NAME time precedence match
  the core's "SI-preferred-else-FILE_NAME" rule? Session 1 noted this; the NTFS decompile
  doesn't show an obvious attribute dispatch. Probably the og uses a virtual attribute
  lookup that prefers SI by default (NTFS spec behavior) — but unconfirmed. No
  observable contradiction in corpus; note only.

Corpus errata candidates:
- fat-exfat-engine-algorithms-v2.md §6 confidence row claims "100% [CODE] for the FAT16
  Next translation table" but does NOT cover the timestamp decode (DosDateTimeToFileTime
  call sites, tenths discard, tz_offset handling). The artifact is silent on timestamp
  handling — not strictly wrong, but the breadth claim could mislead. Suggest a note
  that timestamp decode is out of scope.
- ext4-hfs-engine-algorithms-v2.md §3 (HFS+ catalog leaf walker) doesn't cover the
  timestamp conversion math (the (mac_sec + 9561628800) * 10000000 fold). Not wrong,
  just incomplete. Same suggestion.
- refs-btrfs-zfs-engine-algorithms-v2.md §2 (Btrfs superblock selection) doesn't cover
  the inode timestamp decode (only superblock generation). Not wrong; same suggestion.

Stage Summary:
- 6 NEW divergence candidates identified (1 SEVERE: exFAT tz_offset; 1 MINOR: FAT
  invalid-date; 4 INFO: ext4 nsec + ext4 access/crtime + HFS+ access_date + Btrfs
  otime supersets — the 4 supersets group as one finding I-006/I-007).
- 5 re-verified MATCH items (NTFS, FAT valid-date, HFS+ math, Btrfs math, ZFS, ReFS,
  Unicode normalization, .rsf wire raw u64).
- N-009c (CLI case folding) re-confirmed.
- Read-only mandate held: zero commits from this session touch source.

---
Task ID: S2-A
Agent: parallel auditor A (session 2)
Task: Line-by-line audit of NTFS engine hard-link / ADS / sparse / reparse / $ATTRIBUTE_LIST behavior — verify session-1 B7 "MATCH" verdict against decompile.

Work Log:
- Read all assigned core files line-by-line:
  src/core/ntfs/cr_mftwalk.cpp (1,431 LOC), cr_attributes.cpp (194), cr_runlist.cpp
  (201), cr_bitmap.cpp (109); include/cr/ntfs/cr_mftwalk.h, cr_attributes.h,
  cr_ntfs_wire.h, cr_runlist.h; tests/unit/test_ntfs_volume.cpp (2,080).
- Read corpus artifacts: ntfs-engine--deep-dive.md (191), ntfs-engine-internals.md
  (90), ntfs-engine-algorithms-v2--deleted-viability.md (144); strings_ascii.txt
  for NTFSSearchDLL.dll + NTFSLib.mo + NTFSUtil.mo + NTFSFileSystemAnalyser.mo;
  the full decompile NTFSSearchDLL_decompiled.c (62,394 LOC) for first-hand
  verification at FUN_180006db0 (build_file_properties), FUN_1800069e0 ($DATA
  iteration), FUN_18001acc0 (deleted-record batch loop / destroy rule),
  FUN_180041970 (base_record gate — NOT in_use), FUN_1800448b0 (pick best
  $FILE_NAME), FUN_1800430b0 (find attribute), FUN_180041df0 ($ATTRIBUTE_LIST
  chase fallback), FUN_180042a80 (build child-record list from $ATTRIBUTE_LIST),
  FUN_180017390 (in-use phase walk), FUN_180017190 (cluster-claim setter).

Findings (new divergences session 1 missed):

1. S2-A-1 (MAJOR): Hard-link fan-out divergence.
   - OG (FUN_180006db0:5273 → FUN_1800448b0:54124-54162) calls FUN_1800448b0
     ONCE per record; that function fetches at most TWO $FILE_NAME attributes
     (index 0 and index 1 via FUN_1800430b0) and picks ONE by namespace ranking.
     The og emits ONE node per record (one name, one parent_ref).
   - CORE (cr_mftwalk.cpp:711-724 collects distinct parents; :796-841 emits one
     node per distinct parent, cloning props for each parent beyond the first).
     For a record with N distinct parents the core emits N nodes.
   - Test fixture EndToEndWalk (test_ntfs_volume.cpp:1314 stats.hardlinks==1,
     record 21 with 2 $FILE_NAME entries for parents 5 and 16) — the og would
     emit 1 node, the core emits 2.
   - Session-1 verdict "M one node per distinct parent with shared refcounted
     props (mirrors og LOCK/unlock AddRef/Release sharing)" is WRONG. The og
     emits one node per record. The corpus's "reference-counted property
     objects" / "LOCK()/UNLOCK() interlocked AddRef/Release at 21170-21205"
     (deep-dive §3 line 85-86) mis-attributes the viability+destroy block as
     hard-link refcounting; those lines are the destroy-rule accounting, not
     shared-props AddRef/Release.

2. S2-A-2 (MINOR): ADS divergence — og IGNORES named $DATA streams.
   - OG (FUN_1800069e0:5159-5160) calls FUN_180045440 to test if a $DATA
     attribute is NAMED (name_length > 0); if so, the unnamed-$DATA processing
     block is SKIPPED (the `if (bVar2 == 0)` gate). Named $DATA (ADS) is
     silently dropped; the props runlist (param_2+0x37) only carries unnamed
     $DATA fragments.
   - CORE (cr_mftwalk.cpp:854-899) emits ADS as child nodes with resident
     content capture (resident_span) or nonresident extents.
   - Test fixture ResidentAdsCarriesContent (test_ntfs_volume.cpp:894-933)
     expects stats.ads == 1u and resident payload; EndToEndWalk (:1315) the
     same. The og would emit 0 ADS nodes for the same fixture.
   - Session-1's note "Og-side result shape for ADS not fully decoded in
     corpus — no contradiction found" was incomplete: there IS a divergence
     (og skips ADS; core emits ADS as child nodes). Core is a SUPERSET.

3. S2-A-3 (MAJOR): $ATTRIBUTE_LIST chase scope + depth divergence.
   - OG chases $ATTRIBUTE_LIST via FUN_180041df0 (the fallback in
     FUN_1800430b0 when in-record find via FUN_180042020/FUN_180041850
     returns 0). This fallback requires the child-record list at
     parse_ctx+0x20, which is built by FUN_1800425d0 only when
     FUN_1800431d0's param_6 is non-NULL.
   - FUN_180017390 (in-use phase walk, decompile:18462) passes plVar9
     (non-NULL) to FUN_1800431d0 → child-record list IS built →
     $ATTRIBUTE_LIST chase WORKS for in-use records.
   - FUN_18001acc0 (deleted phase walk, decompile:21161-21162) passes NULL
     to FUN_1800431d0 → child-record list is EMPTY → $ATTRIBUTE_LIST chase
     does NOT work for deleted records.
   - The og's chase is also DEPTH-1 only: FUN_180041df0 iterates the base
     record's direct children but does NOT recursively chase $ATTRIBUTE_LIST
     in child records (no multi-hop). If a child record itself has
     $ATTRIBUTE_LIST pointing to grandchildren, the og misses them.
   - CORE (cr_mftwalk.cpp:475-572) chases $ATTRIBUTE_LIST for ALL records
     (in-use AND deleted) via a multi-hop BFS (depth 64, cycle guard,
     nested $ATTRIBUTE_LIST in child records queued for the next level —
     cr_mftwalk.cpp:557-566).
   - Test NestedAttrlistChase (test_ntfs_volume.cpp:830-892) exercises
     multi-hop chase for a regular file (record 23 → 24 → 25); the og
     would only see record 23's in-record fragments (and record 24's if
     processed via FUN_180017390, but NOT record 25's grandchild fragment).
   - The CORE is a SUPERSET: correctly merges multi-hop fragments for
     both in-use and deleted records. Recovery correctness improvement
     over the og (the og silently truncates deleted fragmented files).

4. S2-A-4 (MATCH): Sparse file extent accounting post-S-001 sub-fix.
   - OG (FUN_18001acc0:21220 `if (*plVar13 != -1)`) skips sparse runs
     (LCN == -1) in the viability iterator; both total and intact
     counters exclude sparse entries.
   - CORE (cr_bitmap.cpp:96-97 `if (r.sparse) continue;`) excludes sparse
     runs from both `total` and `reused` in reused_ratio. MATCH.
   - The destroy rule's "every probe cluster re-allocated" predicate:
     OG `0 < iVar21 && iVar20 == iVar21` (where both skip sparse);
     CORE `ratio >= 1.0` (where ratio excludes sparse). Equivalent.
   - The candidate-4 concern (sparse-but-allocated file falsely
     destroyed) is NOT a divergence: both sides correctly exclude
     sparse runs, so a sparse-but-allocated file (real clusters free)
     grades Excellent (not Overwritten) and is NOT destroyed.

5. S2-A-5 (MATCH): Reparse points.
   - OG: NTFSSearchDLL has NO $REPARSE_POINT (type 0xC0) parsing in
     FUN_180006db0 (build_file_properties only handles 0x10 SI, 0x30
     FILE_NAME, 0x80 DATA). The corpus strings.txt has
     `$RmMetadata$Reparse$Repair` which is a $-skip filter target, not
     an attribute parser.
   - CORE (cr_mftwalk.cpp build_props switch :78-153) doesn't handle
     0xC0 either. The reparse field is parsed from $FILE_NAME+60
     (cr_attributes.cpp:149 f.reparse) but not propagated to props.
   - MATCH: both ignore $REPARSE_POINT. The corpus's "reparse handling"
     for the Complete scan (internals §Quick vs Complete) is vague and
     likely refers to walking the $Extend/$RmMetadata directory (not
     parsing the attribute).

6. S2-A-6 (MATCH): $OBJECT_ID attribute.
   - OG: no $OBJECT_ID (0x40) parsing in FUN_180006db0.
   - CORE: no 0x40 handling in build_props. The wire header
     (cr_ntfs_wire.h:91) defines ATTR_TYPE_OBJECT_ID = 0x40 but the
     build_props switch doesn't dispatch on it.
   - MATCH: both ignore $OBJECT_ID.
   - Corpus errata: deep-dive §6 line 121 claims "0x40 / 0x80 —
     $OBJECT_ID / $DATA (checks at 51006/54487)" but line 51006 is a
     boot-sector sectors-per-cluster power-of-two check (FUN_18003f480),
     NOT $OBJECT_ID attribute parsing.

7. S2-A-7 (MATCH): $VOLUME_NAME / $VOLUME_INFORMATION.
   - OG: NTFSSearchDLL doesn't parse $VOLUME_INFORMATION (0x70) or
     $VOLUME_NAME (0x60). Volume label comes from elsewhere (NTFSLib's
     GetVolumeInformationA wraps the Windows API per NTFSLib.mo:648).
   - CORE: no 0x60/0x70 handling in build_props. Volume serial comes
     from the boot sector (BootSector.volume_serial at +0x48).
   - MATCH: both ignore $VOLUME_INFORMATION at the engine level.
   - The candidate-7 question (surface volume label from $VOLUME_INFO
     when boot label is empty) is N/A — neither side parses the
     attribute. Boot sector has no label field; volume label is an
     orchestrator/UI concern, not the engine's.

8. S2-A-8 (MATCH with minor note): $SI vs $FILE_NAME time precedence.
   - OG (FUN_180006db0:5338-5341 sets local_84/7c/74 from $FILE_NAME;
     :5347-5350 if $SI exists and is valid, OVERRIDES with $SI times
     via FUN_180045090/5040/4ff0). $SI times win when present.
   - CORE (cr_mftwalk.cpp:734 `props->times = has_si ? si_times :
     best->times;`). MATCH.
   - Minor note: the OG's props serializer field map (deep-dive §6)
     has THREE u64 time fields (+0x25 created, +0x2d modified, +0x35
     changed) — NO accessed time. The CORE's FileTimes stores FOUR
     (created, modified, changed, accessed — cr_attributes.cpp:142-145
     for $FILE_NAME, :172-175 for $SI). The CORE stores one extra time
     (accessed); informational only, no recovery impact.

9. S2-A-9 (MATCH): $FILE_NAME with parent ref to deleted dir.
   - OG (internals §Record processing: "names recovered from $FILE_NAME
     even when parent dirs are gone (reparenting into 'lost' tree)").
   - CORE (cr_mftwalk.cpp:908-935 phase-B parent resolution): for each
     emitted node, look up parent_record in by_record; if found and
     != self, reparent under it; if not found, reparent to root_lost.
   - MATCH: both reparent orphans (parent_record unresolved) to the
     lost tree. A deleted file whose parent dir was ALSO deleted
     (but still in the MFT) reparents under the deleted dir node
     (which is in by_record). A deleted file whose parent dir's MFT
     record was REUSED reparents to root_lost.

10. S2-A-10 (MINOR divergence): Compressed/sparse flag propagation.
    - OG (FUN_1800069e0:5163 sets props+0x12 |= 0x800 for sparse $DATA;
      :5167 sets props+0x12 |= 0x40 for compressed $DATA; :5153-5154
      converts sparse+compressed to compressed-only at the props level).
      These are INTERNAL flags at +0x12 (not in the serializer field map).
      The SERIALIZED flags (props+0x23, local_86) come from $SI's
      dos_flags (OR'd with $FILE_NAME's directory bit 0x10) — bits
      1/2/4/0x20/0x400 only; compressed 0x800 NOT propagated from $SI.
    - CORE (cr_mftwalk.cpp:129-134) sets internal data_compressed_out /
      data_cunit_out (build-time locals, not stored in props).
      props->attributes (cr_mftwalk.cpp:735) = $FILE_NAME's dos_flags &
      0xFF | (is_dir ? 0x10 : 0) — uses $FILE_NAME (not $SI) and
      doesn't propagate 0x400 (OFFLINE) or compressed bits.
    - DIVERGENCE (minor): OG's serialized flags source = $SI; CORE's =
      $FILE_NAME. OG's internal +0x12 has compressed/sparse; CORE has
      no equivalent persistent internal flag (data_compressed_out is
      discarded after expand_compressed_data). No recovery impact
      (the CORE always expands compressed data via GAP-P1-1; downstream
      code doesn't need the flag). Cosmetic difference in attributes
      shown to UI.

Corpus errata candidates (prose claims contradicting the decompile):

E1. ntfs-engine--deep-dive.md §3 line 77-82: claims
    `(props->flags & 0x10) == 0` is the "DELETED-FILE BRANCH" and that
    "NTFS in-use flag (0x1 in FILE record flags byte maps to props bit
    4)". WRONG: bit 0x10 of props+0x23 is the DIRECTORY bit (set at
    FUN_180006db0:5329 when FUN_180043700 returns non-zero). The actual
    in-use flag is at record header +0x16 (flags 0x0001), read by
    FUN_180043060. The check `(uVar7 & 0x10) == 0` is "NOT a directory",
    not "deleted".

E2. ntfs-engine-algorithms-v2--deleted-viability.md §1 line 25-26:
    claims `if record_in_use(rec): // FUN_180041970: parsed+0x20 != 0
    continue // ONLY NOT-IN-USE (deleted) records pass`. WRONG:
    FUN_180041970 (decompile:52411-52415) is `return *(longlong
    *)(param_1 + 0x20) != 0;` — checks the BASE_RECORD field (offset
    0x20 in record header), NOT the in-use bit. The correct
    interpretation: EXTENSION records (base_record != 0) are skipped;
    BOTH in-use and deleted non-extension records pass. FUN_18001acc0
    processes both, and the destroy rule fires for both (in-use records
    destroyed as dedup because their clusters were claimed by the
    in-use phase walk FUN_180017390).

E3. ntfs-engine-algorithms-v2--deleted-viability.md §1 line 44:
    claims `if size >= 0x3FF` (viability gate). WRONG: the decompile
    (FUN_18001acc0:21208) shows `if (0x3ff < lVar18)` — strict
    GREATER THAN. The CORE's strict `>` (cr_mftwalk.cpp:668,
    cr_mftwalk.h:58 viability_min_size=0x3FF) matches the decompile;
    the corpus's `>=` is wrong.

E4. ntfs-engine--deep-dive.md §6 line 121: claims "0x40 / 0x80 —
    $OBJECT_ID / $DATA (checks at 51006/54487)". Line 51006 is in
    FUN_18003f480 (boot-sector sectors-per-cluster power-of-two
    validation: `sVar2 == 0x40` is one of the pow2 checks). NOT
    $OBJECT_ID attribute parsing. The og doesn't parse $OBJECT_ID.

E5. ntfs-engine--deep-dive.md §3 line 85-86: claims "reference-counted
    property objects (`_Ref_count_base` RTTI; `LOCK()/UNLOCK()`
    interlocked AddRef/Release at 21170-21205 — the engine is
    thread-safe for the UI's async workers)". WRONG: lines 21170-21205
    are the viability+destroy block (system-flag check :21186,
    directory gate :21190, runlist iteration :21217-21248, destroy rule
    :21249-21262). The "LOCK()/UNLOCK() interlocked AddRef/Release"
    interpretation is a misread; those lines are viability logic, not
    refcount management. (The og DOES use shared_ptr-style refcounting
    via CNTFSFileProperties, but the cited lines are not where it
    happens.)

E6. ntfs-engine-algorithms-v2--deleted-viability.md §1 lines 60-66
    (the semantic interpretation paragraph): claims "bitmap = controller
    .field_+0x40 or +0x48 # $Bitmap objects" and "when NTFS deletes a
    file it frees the file's $Bitmap bits. A deleted MFT record whose
    data-run clusters still read as allocated in the live $Bitmap..."
    WRONG per OQ-T1 resolution (already documented): the bitmap is a
    scan-built claimed-sector bit-set (ctor FUN_18002c330, marked by
    FUN_180017190 at :18409), NOT the volume $Bitmap.

Open questions (need deeper RE work):

OQ-S2A-1: Og's regular-file $ATTRIBUTE_LIST handling for DELETED records.
   The decompile shows FUN_180017390 (in-use phase) builds the
   child-record list and passes it to FUN_1800431d0, enabling the
   FUN_180041df0 fallback. FUN_18001acc0 (deleted phase) passes NULL,
   disabling the fallback. The question: does FUN_180017390 process
   DELETED records too (building their child-record list, even though
   it won't claim clusters via FUN_180017190 due to the :18345-18352
   deleted-bit bail)? If yes, deleted records' $ATTRIBUTE_LIST IS
   chased in Walk 1 (but the props might not be emitted if Walk 1
   only emits in-use records). If no, deleted records never get
   $ATTRIBUTE_LIST chase. Needs caller-context tracing to resolve.

OQ-S2A-2: Og's hard-link handling downstream. The decompile shows
   FUN_180006db0 picks ONE $FILE_NAME per record. But the corpus
   mentions "reference-counted property objects" and NTFSLib.mo has
   CNtfsMultiFileRecord class. Does a downstream component (DRWUI
   tree builder, or NTFSLib's MultiFileRecord) fan out hard-links
   from a single record? Needs RE of the consumer side.

OQ-S2A-3: Og's ADS handling downstream. The decompile shows
   FUN_1800069e0 skips named $DATA. But the corpus mentions
   CResidentNTFSDataRunList and NTFSLib.mo has CNtfsStreamInfo
   (in NTFSUtil.mo). Does a downstream component (NTFSUtil's
   Ntfs2FAT converter, or the writer) handle ADS separately?
   Needs RE of NTFSLib/NTFSUtil.

OQ-S2A-4: Og's $ATTRIBUTE_LIST depth bound. The og's chase
   (FUN_180041df0) iterates the child-record list built by
   FUN_1800425d0 from the base record's $ATTRIBUTE_LIST entries.
   It does NOT recursively chase child records' own $ATTRIBUTE_LIST
   (depth-1 only). The CORE uses depth 64 (multi-hop BFS). What's
   the og's effective depth bound for the $MFT case (FUN_18001a540
   → FUN_180042a80)? Needs RE of FUN_18001a540's iteration structure.

Stage Summary:
- Session-1 B7 "MATCH" verdict for hard-link/ADS/sparse/refuted: 3 new
  divergences found (S2-A-1 hard-link fan-out MAJOR, S2-A-2 ADS MINOR,
  S2-A-3 $ATTRIBUTE_LIST scope+depth MAJOR), 1 minor divergence
  (S2-A-10 compressed/sparse flag propagation), 6 confirmed MATCHes
  (S2-A-4 sparse accounting, S2-A-5 reparse, S2-A-6 $OBJECT_ID, S2-A-7
  $VOLUME_INFORMATION, S2-A-8 time precedence, S2-A-9 deleted-parent
  reparenting).
- 6 corpus errata candidates identified (E1-E6) — prose claims in the
  corpus that contradict the first-hand decompile read.
- 4 open questions registered (OQ-S2A-1..4) for deeper RE work.
- Read-only mandate held throughout. No source files modified.

---
Task ID: S2-1 (session 2 bootstrap + parallel agent dispatch + finding consolidation)
Agent: main auditor (session 2)
Task: Session 2 — find what session 1 missed; continue the audit; remove the fixed things

Work Log:
- Pulled both repos fresh: core 5d6da5f (HEAD includes A-001/S-001/S-005/M-001 fixes + GAP-S6-11
  disk-map), corpus 03ad46c (unmoved). Audited commit for new findings: 5d6da5f.
- Read worklog.md to map session 1's coverage: 19 modules, 31 findings (1 ACUTE, 9 SEVERE, 4 MAJOR,
  12 MINOR, 5 INFO), 9 tracked-open. Concurrent remediation leg fixed 4 findings + resolved 1 OQ:
  A-001 (1cb6a43), M-001 (5d6da5f), S-001 (e93decf), S-005 (2f3237d), OQ-T1 (decompile read).
- Saved session-2 prompt (redacted) at worklog/session-2-diff-audit-prompt.md. Read twice.
- Wave 1 (this entry, T2): moved the 5 resolved findings (A-001, S-001, S-005, M-001, OQ-T1) from
  their active severity dirs into diff-to-fix/findings/FIXED/ via `git mv` (preserves git history).
  Authored diff-to-fix/findings/FIXED/RESOLVED-INDEX.md cataloging each resolved finding's title,
  fix commit, and resolution summary. Empty ACUTE/ dir removed from working tree.
- Wave 2 (this entry, T3-T6): dispatched 4 parallel agents (S2-A through S2-D) covering parameter
  families session 1 did NOT deeply audit:
  * S2-A: NTFS hard-link/ADS/sparse/reparse/$ATTRIBUTE_LIST — B7 line-by-line vs decompile.
  * S2-B: B9 (timestamps/timezone) + B10 (Unicode) across all engines session 1 only superficially
    covered (FAT, exFAT, ext4, HFS+, Btrfs, ZFS, ReFS).
  * S2-C: F60-F64 (safety), writer behavior beyond N-009b/d, session resume beyond D12, R54-R58
    (architecture). Used the ghidra-partition/*.c decompiled files session 1 did NOT use
    (session 1 used .mo files only).
  * S2-D: X47-X53 (perf static analysis), Q65-Q70 (quality per-module), P42-P46 (API), CLI
    completeness — all the parameters session 1 marked "MATCH" or "tracked-open" without deep
    verification.
- All 4 agents returned structured context sheets. Consolidated 17 new divergence candidates +
  18 corpus errata candidates. Of the 17 candidates: 2 SEVERE (S-010 repair-path volume lock,
  S-011 exFAT tz_offset), 3 MAJOR (M-007 hard-link fan-out, M-008 $ATTRIBUTE_LIST scope+depth,
  M-009 RawFixer bypasses 64 KiB envelope), 11 MINOR (N-010..N-020), 2 INFORMATIONAL (I-006
  ext4 nsec superset, I-007 ext4/HFS+/Btrfs extra-timestamps superset).
- Wave 2 (this entry): wrote all 17 new finding files in the mandated template with cited evidence
  (file:line on both sides, fix plan, tests to add, related findings, open questions). Findings
  live in diff-to-fix/findings/{SEVERE,MAJOR,MINOR,INFORMATIONAL}/.
- Top new divergences found:
  * S-010 (SEVERE, safety): RawFixer write device opens with FILE_SHARE_READ|WRITE and NO
    FSCTL_LOCK_VOLUME/FSCTL_DISMOUNT_VOLUME — concurrent processes can race with repair writes.
    The og's RecoveryModule.dll does the full lock+unmount+flush+sleep-2s-settle+verify sequence.
    Compounds S-008's wiring gap with a safety gap on the write path.
  * S-011 (SEVERE, parity): exFAT tz_offset — core applies (spec-correct UTC), og ignores
    (local-as-UTC via DosDateTimeToFileTime). Every non-UTC exFAT file's .rsf diverges by
    tz_offset hours.
  * M-007 (MAJOR, superset): NTFS hard-link fan-out — og emits 1 node per record (namespace-best
    name under namespace-best parent), core emits N nodes (one per distinct parent). Core is more
    useful but parity benchmarks diverge.
  * M-008 (MAJOR, superset): NTFS $ATTRIBUTE_LIST chase — og is depth-1 in-use-only (deleted
    records' extension fragments silently dropped), core is multi-hop BFS depth 64 all-records.
    Deleted fragmented files recover full content in core, truncated in og.
  * M-009 (MAJOR, perf): RawFixer RwDevice::read_at bypasses the 64 KiB direct-I/O envelope
    (GAP-S6-13 mandate). The scan-path devices comply; only the repair-path device skips. ~3.5×
    slow path on 4Kn NVMe (the documented GAP-S6-13 ratio).

Stage Summary:
- Session 2 wave 1+2 complete: 5 resolved findings moved to FIXED/ (with RESOLVED-INDEX.md),
  17 new findings authored (2 SEVERE, 3 MAJOR, 11 MINOR, 2 INFO). Active counts: 0 ACUTE,
  9 SEVERE, 4 MAJOR, 18 MINOR, 7 INFO (was 1/9/4/12/5 = 31 active; now 0/9/4/18/7 = 38 active
  after subtracting the 5 fixed and adding the 17 new — net +12 active findings over session 1).
- Read-only mandate held: zero source files modified. Only diff-to-fix/ and worklog.md touched.
- All commits will land in this wave's push. Next wave (T7): update SUMMARY, CROSS-REF-MATRIX,
  module files, README; author CORPUS-ERRATA.md; final push with worklog closeout.

---
Task ID: S2-A (parallel agent A, dispatched in S2-1)
Agent: parallel agent A (general-purpose)
Task: Audit NTFS hard-link/ADS/sparse/reparse/$ATTRIBUTE_LIST — B7 line-by-line vs decompile

Work Log:
- Read NTFSSearchDLL_decompiled.c (62,394 LOC, 1,931 functions) line-by-line for the hard-link,
  ADS, sparse, reparse, and $ATTRIBUTE_LIST paths. Cross-checked session-1's B7 "MATCH" verdict.
- REFUTED session-1's B7 "MATCH" on three counts:
  * S2-A-1 (MAJOR): hard-link fan-out — og emits ONE node per record (FUN_180006db0 calls
    FUN_1800448b0 once at :5273; FUN_1800448b0 fetches at most TWO $FILE_NAME via
    FUN_1800430b0 indices 0 and 1, picks ONE by namespace ranking). Core emits N nodes per
    record (one per distinct parent). Test EndToEndWalk stats.hardlinks == 1u on record 21
    proves the divergence.
  * S2-A-2 (MINOR): ADS — og silently drops named $DATA at FUN_1800069e0:5159-5160 (the
    is_named($DATA) check); core emits ADS as child nodes via cr_mftwalk.cpp:854-899.
  * S2-A-3 (MAJOR): $ATTRIBUTE_LIST chase — og is depth-1 in-use-only (FUN_180041df0 at
    :52615-52687 has no recursion; FUN_18001acc0 deleted-phase at :21161-21162 passes NULL
    param_6, disabling the chase). Core is multi-hop BFS depth 64 all-records (cr_mftwalk.cpp:475-572).
- Confirmed 6 MATCH items (sparse accounting post-S-001 sub-fix, reparse points, $OBJECT_ID,
  $VOLUME_INFORMATION, $SI vs $FILE_NAME time precedence, deleted-parent reparenting).
- Surfaced 6 corpus errata candidates (the LOCK/unlock viability+destroy block misread in
  ntfs-engine--deep-dive.md §3; the in-use-flag misread in
  ntfs-engine-algorithms-v2--deleted-viability.md §1 lines 25-26; the >= vs > viability gate
  in §1 line 44; the misleading 0x40 cite in §6 line 121; the dormant secondary claim map
  in §1 lines 60-66, already documented in OQ-T1's resolution).
- Open questions: OQ-S2A-1 (FUN_180017390 in-use-phase record selection), OQ-S2A-2 (downstream
  UI/writer hard-link fan-out), OQ-S2A-3 (NTFSUtil.mo ADS handling), OQ-S2A-4 ($MFT $ATTRIBUTE_LIST
  depth bound).

Stage Summary:
- Session-1 B7 "MATCH" verdict REFUTED. 3 new divergences (S2-A-1 → M-007, S2-A-2 → N-011,
  S2-A-3 → M-008) + 1 minor (S2-A-10 → N-012). 6 corpus errata candidates. Read-only mandate held.

---
Task ID: S2-B (parallel agent B, dispatched in S2-1)
Agent: parallel agent B (general-purpose)
Task: Audit B9 (timestamps) + B10 (Unicode) across all engines session 1 only superficially covered

Work Log:
- First-hand read of all 8 engine .cpp files + wire structs + cr_types.h + cr_text.h + cr_records.cpp
  + cr_query.cpp + cr_report.cpp. Cross-checked first-hand reads of the og decompiles for each
  engine (FATSearchDLL, exFATSearchDLL, EXT4SearchDLL, HFSPlusSearchDLL, BtrfsSearchDLL,
  ZfsSearchDLL, ReFSSearchDLL, NTFSSearchDLL) plus rabin2_imports.txt for every SearchDLL.
- Found 6 new divergences:
  * S2-B-001 (SEVERE → S-011): exFAT tz_offset — core applies (spec-correct UTC), og ignores
    (local-as-UTC). Every non-UTC exFAT file's .rsf diverges by tz_offset hours.
  * N-010 (MINOR): FAT invalid-date yields non-zero FileTime in core (1970-01-01), zero in og
    (1601-01-01). The exFAT engine in the same codebase handles this correctly (early-exit on
    date == 0). Only the FAT engine has the bug.
  * I-006 (INFORMATIONAL): ext4 nanoseconds superset — core preserves via unix_to_filetime_ns,
    og drops via _gmtime32_s + wMilliseconds=0.
  * I-007 (INFORMATIONAL, 3 sub-items): core reads more timestamps than og for ext4 (4 vs 2),
    HFS+ (4 vs 3), Btrfs (4 vs 3). Core superset.
- Confirmed 13 MATCH items (NTFS raw 100ns + SI-vs-FILE_NAME precedence note; FAT valid-date +
  tenths; exFAT 10ms; HFS+ Mac-epoch fold math; Btrfs timestamp math for the 3 og-read fields;
  ZFS seconds-only read; ReFS raw 100ns-1601; .rsf wire stores raw u64; Unicode normalization
  NONE on either side at engine layer; path separator NA at engine layer; N-009c re-confirmed).
- Surfaced 4 corpus errata candidates (timestamp-decode out-of-scope notes in 3 algorithm docs;
  NTFS precedence gap in internals doc).
- Open questions: OQ-S2-B-1 (NTFS SI-vs-FILE_NAME precedence undecoded — carried forward),
  OQ-S2-B-2 (FAT date=0 sentinel semantics — resolved in N-010 fix plan).

Stage Summary:
- 4 new findings authored (S-011, N-010, I-006, I-007). 13 MATCH items re-verified. 4 corpus
  errata candidates. Read-only mandate held.

---
Task ID: S2-C (parallel agent C, dispatched in S2-1)
Agent: parallel agent C (general-purpose)
Task: Audit F60-F64 (safety), writer behavior beyond N-009b/d, session resume beyond D12, R54-R58

Work Log:
- First-hand read of cr_safewriter.cpp (586 LOC), cr_report.cpp, cr_crsf.cpp (332 LOC),
  cr_records.cpp (389 LOC), cr_bitlocker.cpp, cr_destio.cpp, cr_rawfixer.cpp, cr_scan.cpp (1,270 LOC).
- Used the ghidra-partition/*.c decompiled files (session 1 used .mo files only) —
  RecoveryModule_decompiled.c, PartitionRecovery_decompiled.c, RawFixer_decompiled.c. These
  contained the canonical repair-path lock sequence (FSCTL_LOCK_VOLUME +
  FSCTL_DISMOUNT_VOLUME + FlushFileBuffers + Sleep(2000) + FSCTL_IS_VOLUME_MOUNTED).
- Found 6 new findings:
  * S2-C-1 (MAJOR → upgraded to SEVERE → S-010): REPAIR-path volume lock missing — og does
    FSCTL_LOCK_VOLUME + FSCTL_DISMOUNT_VOLUME + FlushFileBuffers + Sleep(2000) +
    FSCTL_IS_VOLUME_MOUNTED at RecoveryModule_decompiled.c:44686-44720. Core's RwDevice
    (cr_transport.cpp:822) opens with FILE_SHARE_READ|WRITE and NO lock. Concurrent processes
    can race with repair writes. Upgraded from MAJOR to SEVERE because it's a safety
    regression that can corrupt volumes.
  * S2-C-2 (MINOR → N-013): Destination overlap check is OPTIONAL (gated on drive-letter-only)
    and first-extent-only (the VOLUME_DISK_EXTENTS ext[8] buffer has room for 8 extents but
    only the first is read out at cr_transport.cpp:701-712). Image-source and multi-extent
    destinations miss the check.
  * S2-C-3 (MINOR → N-014): Stage checkpoint NOT atomic — _wfopen_s(&f, w.c_str(), L"wb")
    TRUNCATES the existing file on open at cr_ui.cpp:1131 + cr_ui.cpp:254-260. Previous
    checkpoint destroyed on crash. Torn-tail salvage (cr_crsf.cpp:240-276) mitigates the
    worst case but doesn't preserve history. Og's pattern writes each auto-save to a FRESH
    timestamped file.
  * S2-C-4 (MINOR → N-015): Writer does NOT pre-check destination free space — no
    GetDiskFreeSpaceEx call; ERROR_DISK_FULL returns generic ERR_WRITE_FAILED; the queue
    loops over the ENTIRE queue pointlessly attempting create_temp + write + delete for
    each remaining item. Og does GetDiskFreeSpaceExA at RecoveryModule_decompiled.c:47033.
  * S2-C-5 (MINOR → N-016): emit_boundary_events_ double-call bug at cr_scan.cpp:577-578
    (clear merge artifact — same call appears twice, once with 4-space indent, once with
    8-space indent). The other 3 call sites (:652, :1016, :1070) each call it once.
  * S2-C-6 (refined N-009d): Writer does NOT preserve timestamps or attributes — confirmed
    og ALSO resets attributes to FILE_ATTRIBUTE_NORMAL (:79677) and does NOT call SetFileTime
    (grep empty). Both og and core drop timestamps/attributes. The ACL drop IS the divergence
    (N-009d, already filed). Narrowed to: N-009d is the complete divergence; no additional
    timestamp/attribute gap.
- Confirmed 17 MATCH items (F60 source bad-sector, F61 BitLocker detect+legitimate-unlock-only,
  F63 RawFixer consent engine-level, writer atomic write via temp+rename, writer partial-overwrite
  policy, session resume from partial .crsf (torn-tail salvage), session CRC (per-record CRC32
  superset, og has none), session identity verification (boot_hash + model + serial vs og's full
  512-byte boot sector), cancel propagation latency (per-chunk in writer), concurrent access
  (non-exclusive scan opens both sides), R54 module boundaries, R55 dependency direction, R56
  interface shapes, R57 wiring, R58 layering no bypass).
- Surfaced 4 corpus errata candidates (04-recovery-workflow.md §7.5 ACL-preservation understated,
  rsf-format §5.3 boot-sector snapshot vs core's CRC32 (32-bit collision risk), 04-recovery-workflow.md
  §10 "locked volumes" should flag the repair-path-specific gap, rsf-format §2.1 record-header
  should note the core's CRSF per-record CRC32 hardening).
- Open questions: OQ-S2C-1 (og ExportThread cancel granularity), OQ-S2C-2 (og ACL-preservation
  scope — applies 4 components or just DACL?), OQ-S2C-3 (og pre-scan RawFixer consent UX),
  OQ-S2C-4 (image-source F62 gap exploitability), OQ-S2C-5 (emit_boundary_events double-call
  intent — git blame needed).

Stage Summary:
- 5 new findings authored (S-010, N-013, N-014, N-015, N-016) + N-009d narrowed (no additional
  gap). 17 MATCH items re-verified. 5 open questions. 4 corpus errata candidates. Read-only
  mandate held.

---
Task ID: S2-D (parallel agent D, dispatched in S2-1)
Agent: parallel agent D (general-purpose)
Task: Audit X47-X53 (perf), Q65-Q70 (quality), P42-P46 (API), CLI completeness

Work Log:
- First-hand read of cr_cached.cpp, cr_slice.cpp, cr_devmap.cpp, cr_retry.cpp, cr_directalign.h,
  cr_transport.cpp (Windows), cr_parallel.cpp, cr_ui.cpp, cr_ui.h, rrecov.cpp (1,498 LOC),
  cr_scan.cpp (1,270 LOC — focus on threading/parallelism).
- Static analysis of I/O patterns (no og-execution; the documented X47-X53 og-execution gap
  applies to all performance verifications).
- Found 6 new divergences + 1 expansion of M-006:
  * D1 (MAJOR → M-009): X51 — RawFixer RwDevice::read_at bypasses the 64 KiB direct-I/O
    envelope. cr_transport.cpp:743-749 calls overlapped_read directly; DirectDevice and
    RdaTargetDevice route through aligned_read_at. The 64 KiB envelope lives in
    aligned_read_at:148-170; RwDevice skips it. ~3.5× slow path on 4Kn NVMe (the
    documented GAP-S6-13 ratio).
  * D2 (MINOR → N-017): X53 — cancel watcher coverage inconsistent. Carve has 50 ms watcher
    (cr_carve.cpp:418-429); orchestrator parallel candidate scan does not (cr_scan.cpp:952-1003
    uses parallel_for with no watcher; cr_parallel.cpp:104-150 does not poll cancel between
    item dispatches).
  * D3 (MINOR → N-018): Q67 — UBSan is not enabled anywhere. Only ASan (linux-gcc lane) +
    TSan (linux-tsan lane). Grep across CMakeLists.txt for fsanitize=undefined returns empty.
    Q67's three-pronged definition (ASan/UBSan/TSan) is partially met.
  * D4 (MINOR → N-019): Q65 — missing per-parameter regression tests. 0 of 13 audit-prompt
    scenarios has an isolated test (hard-link destroy, ADS emission, sparse accounting, FAT
    cross-linked chain, ext4 extent tree depth > 2, HFS+ resource fork, Btrfs subvolume rooting,
    ZFS snapshot, ReFS integrity stream, LDM RAID-0/1, lost-partition overlap scoring).
  * D5 (MINOR → N-020): Q68 — documentation drift across 5 spots (AUDIT-PIPELINE.md WRITER-1
    marked OPEN but FIXED in tree; cli.md commands list includes verify/query that don't exist
    as CLI commands; cli.md exit codes conflate commands; PERFORMANCE-PROFILE.md "BELOW the
    64 KiB contract" stale post-GAP-S6-13; PERFORMANCE-PROFILE.md "1 MiB cache" should be 8 MiB
    at orchestrator call site; S-009 finding file stale post-GAP-S6-11).
  * D6 (expands M-006): Q66 — fuzz harness gaps. 13 harnesses exist (fuzz_{ntfs,fat,exfat,
    ext4,hfsplus,btrfs,zfs,refs,carve,preview,container,engines,pure}.cpp). Missing dedicated
    harnesses for: SafeWriter, DestIo, BitLocker, ldm, table_parse, rawfixer. Some covered
    indirectly via fuzz_engines.cpp + fuzz_pure.cpp + fuzz_container.cpp. CI fuzz job still
    `if: false` (M-006, still in tree).
- Confirmed 14 MATCH items (X47 throughput 64 KiB-aligned direct reads + 8 MiB LRU + cache block;
  X48 latency streaming results; X49 memory bounded cache + bounded writer chunk; X50 CPU no
  engine-side thread priority + parallelization where og does; X52 scalability query Path sort
  O(N*D) acceptable + carve dedup extent-set-based + writer collision policy; X53 cancel watcher
  50 ms in carve + per-record checkpoints + writer cancel per-chunk; Q67 ASan clean + no
  suppressions; Q69 zero stub markers grep-verified; Q70 Win7 posture + import allowlist; P42
  cr_ui public surface 57 C11 exports; P43 ownership/lifetimes; P44 async PausableToken; P45 ABI
  versioning CR_UI_ABI_VERSION=1; P46 documentation ABI-CONTRACT.md normative; CLI completeness
  vs documented contract).
- Surfaced 4 corpus errata candidates (ntfs-engine-internals.md:71 "batch sizes adapt to RAM"
  assertion needs decompile citation; ntfs-engine-internals.md:73 condition-variables claim
  may apply to a different binary (NTFSSearchDLL strings show CRITICAL_SECTION + events, not
  InitializeConditionVariable); DRWUI-exe--thread-architecture.md:56 "work-stealing" claim
  needs decompile citation; 05-scan-engine-architecture.md:122 "512B sectors" too narrow for
  RDA driver's configurable sector size).
- Open questions: 5 carried forward (X47 og's GlobalMemoryStatus adaptive sizing ladder; X50
  ReadFileScatter usage; X51 og's RawFixer read alignment; D2 og's cancel-latency bound;
  Q65 audit-trail check interpretation).

Stage Summary:
- 5 new findings authored (M-009, N-017, N-018, N-019, N-020) + M-006 expansion (D6 catalogues
  fuzz harness gaps). 14 MATCH items re-verified. 4 corpus errata candidates. 5 open questions.
  Read-only mandate held.

---
Task ID: S2-2 (session 2 wave 3 — module files + CORPUS-ERRATA + final commit)
Agent: main auditor (session 2)
Task: Wave 3 — update per-module files with session 2 findings; author CORPUS-ERRATA.md; final commit + push

Work Log:
- Updated 12 of 19 module files with session 2 sections:
  * ntfs.md (session 1 B7 refuted; new M-007, M-008, N-011, N-012 + 6 confirmed MATCH items)
  * fat.md (new N-010)
  * exfat.md (new S-011)
  * ext4.md (new I-006, I-007)
  * hfsplus.md (new I-007)
  * btrfs.md (new I-007)
  * zfs.md (B9 MATCH re-verified; S-005 covered the B10 filename divergence)
  * rawfixer.md (new S-010, M-009)
  * orchestration.md (new N-016, N-017)
  * session.md (new N-014)
  * writer.md (new N-013, N-015 + N-009d refinement)
  * cli.md (new N-020 partial)
  * health.md (no new findings — confirmed session 1 verdicts)
- The remaining 6 modules (ldm, lost-partition, carve, refs, visualization, abi)
  got brief "no new findings" session 2 notes for audit-trail completeness.
- Authored diff-to-fix/CORPUS-ERRATA.md cataloging 18 corpus errata candidates
  surfaced across session 1 + session 2:
  * E1-E6 (from S2-A): NTFS deep-dive/algorithms-v2 misreads (LOCK/unlock block,
    in-use-flag, viability gate >=, 0x40 attribute cite, secondary claim map).
  * E7-E10 (from S2-B): timestamp-decode out-of-scope notes for FAT/exFAT,
    ext4/HFS+, Btrfs/ZFS algorithm docs; NTFS precedence gap in internals doc.
  * E11-E14 (from S2-C): 04-recovery-workflow ACL/lock-volume understate,
    rsf-format boot-sector snapshot vs CRC32, locked-volumes repair-path gap,
    rsf record-header CRC32 hardening.
  * E15-E18 (from S2-D): ntfs-engine-internals.md GlobalMemoryStatus citation
    incomplete, condition-variables claim may apply to wrong binary,
    DRWUI thread-architecture work-stealing claim uncited, 05-scan-engine
    "512B sectors" too narrow for RDA driver.

Stage Summary:
- Wave 3 complete: 12 module files updated with session 2 sections; 6 module
  files got brief notes; CORPUS-ERRATA.md authored (18 entries).
- Session 2 deliverables:
  * diff-to-fix/findings/FIXED/ — 5 resolved findings moved here + RESOLVED-INDEX.md
  * diff-to-fix/findings/{SEVERE,MAJOR,MINOR,INFORMATIONAL}/ — 17 new finding files
  * diff-to-fix/CROSS-REF-MATRIX.md — session 2 changes marked (S2)
  * diff-to-fix/SUMMARY.md — active counts (0/9/6/18/7/8) + session 2 delta + remediation log
  * diff-to-fix/README.md — session 2 navigation
  * diff-to-fix/CORPUS-ERRATA.md — 18 corpus errata candidates
  * diff-to-fix/modules/*.md — 12 modules updated with session 2 sections; 6 with brief notes
- Final active counts: 0 ACUTE, 9 SEVERE, 6 MAJOR, 18 MINOR, 7 INFO, 8 tracked-open = 38 active.
- Resolved counts: 5 (in findings/FIXED/).
- Read-only mandate held throughout: zero source files modified. Only diff-to-fix/
  and worklog.md (and worklog/session-2-diff-audit-prompt.md) touched.
- Recommended next remediation wave (priority order):
  1. S-010 (safety regression on repair path; can corrupt volumes during concurrent access)
  2. S-011 (exFAT tz_offset; one-line fix; matches og's local-as-UTC)
  3. M-009 (RawFixer bypasses 64 KiB envelope; one-line fix)
  4. S-002 (HFS+ decmpfs — primary user scenario)
  5. S-008 (RawFixer pre-scan wiring; composes with S-010's lock fix)
  6. S-006 (lost-partition probe families)
  7. S-007 (carve formats)
  8. S-003 (HFS+ journal replay)
  9. S-004 (ext4 journal every-block surface)
  10. M-007/M-008 (hard-link fan-out + $AttrList chase; opt-in parity flags)
  11. M-002 (EBR torn-chain salvage)
  12. M-005 (carve bitmap gating)
  13. M-006 (re-enable fuzz CI + add 6 missing harnesses per N-019)
  14. N-010 (FAT invalid-date one-line guard)
  15. N-013/N-014/N-015 (writer destination validation trio; composes with S-010's lock fix)
  16. N-016 (emit_boundary_events double-call bug; one-line removal)
  17. N-017 (cancel watcher coverage; one-line parallel_for poll)
  18. N-018 (UBSan CI lane; mirror the TSan lane)
  19. N-019 (12 missing per-parameter regression tests)
  20. N-020 (6 stale doc claims — including the S-009 finding file that should
      move to FIXED/ per GAP-S6-11 having landed)

---
Task ID: 18-b (Windows red root cause — CRT sharing; this leg)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: S-008 Windows test failures — image repair writer open denied

Work Log:
- Windows CI failed only the two new image-repair tests (C++ + the C ABI check):
  st.repair_applied == 0 / denied == 1 with consent granted and a valid plan - the
  writer-open failed. Root cause (MS docs, fopen_s/_wfopen_s remarks): the CRT _s
  variants open EXCLUSIVE ("If you need to share the file, use _fsopen or _wfsopen
  ... _SH_DENYNO"). ImageFileDevice opened the image via _wfopen_s("rb") - exclusive -
  so (a) ImageWriteDevice's _wfopen_s("r+b") was denied, and (b) even the ABI check's
  own fixture-patch fopen("r+b") failed while the first scan's read handle was live
  (line-283 FAIL cascading into every later check).
- Fix: both image opens in the shim now use _wfsopen(..., _SH_DENYNO) on Windows
  (read view "rb", write view "r+b") - explicitly shared, per the documented path;
  POSIX keeps plain fopen (no sharing semantics). This is also the correct posture for
  any future consumer that opens the image concurrently.
- Local: suite 313/313, ABI check OK, clang-format gate clean; Windows CI to verify.

Stage Summary:
- Windows red root cause fixed at the source (CRT exclusive opens -> _SH_DENYNO);
  the S-008 image-repair lifecycle should now run identically on both platforms.
Task ID: S2-T7 (session-2 diff-audit: PartitionRecovery/LDM deep pass)
Agent: session-2 parity auditor (read-only)
Task: Deep decompile-vs-core pass over LdmManager/Partition (T7): resolve OQ-T7 (RAID-5
rotation), the GPT-LDM gating question raised by T2's A-001 re-verification, and hunt
for divergences session 1 missed.

Work Log:
- Read LdmManager_decompiled.c (91,475 lines) targeted: SEH crash-handler strings
  :88592-:88709 map function names -> RVAs; cross-checked LdmManager_functions.txt.
- RAID-5 (OQ-T7 RESOLVED): Raid5VolumeStorage::ReadSector = FUN_180086a90 :68777-68876.
  Four rotation branches on this+0x2a8: R=0 left-symmetric ((N-1-s%N)+1+f)%N; R=1
  left-asymmetric (fill-from-0, skip parity N-1-s); R=2 right-symmetric (s%N+1+f)%N;
  R=3 right-asymmetric (fill-from-0, skip parity s%N). Selector ctor-hard-zeroed
  (:68942, the ONLY write in 91,475 lines - grep-verified) -> R=1/2/3 dead code.
  Core cr_ldm.cpp:1349-1359 is ALGEBRAICALLY IDENTICAL to R=0. MATCH.
  ComputeDataWithOtherDisk = FUN_180085c30 :68393-68451: XOR-fold other columns,
  first-read memcpy; second missing member -> failure (0), matches core :1372-1389.
  Filed as INFORMATIONAL I-006 (dormant rotations) + OQ-T7 resolution note.
- GPT-LDM gating (A-001 follow-up): LdmManager FUN_1800724f0 :59393-59472 scans GPT
  entries against DAT_1800c4960 (extracted from LdmManager.mo: aac808588f7ee04285d2-
  e1e90434cfb3 = 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3 = the REAL PARTITION_LDM_METADATA_
  GUID); PRIVHEAD candidate = entry+0x28 (partition LAST LBA - the documented backup
  copy); no CRC checks (same as core). Partition.mo FUN_180002bd0 :1290-1340 scans both
  LDM GUIDs for its labels; DAT_180036630/0x36640 = the same REAL values.
- DISCOVERY -> ACUTE A-002: the A-001 fix (1cb6a43) replaced the core's LDM constants -
  which were byte-for-byte the REAL Microsoft GUIDs (pre-fix {AA,C8,08,58,8F,7E,E0,42,
  85,D2,E1,E9,04,34,CF,B3}/{A0,60,9B,AF,31,14,62,4F,BC,68,33,11,71,4A,69,AD}) - with
  fabricated values (5808c8aa-8e9f-4c87-.../af9b60a0-384e-4f7d-...) that appear in NO
  og binary, NO authoritative source (Microsoft Learn PARTITION_INFORMATION_GPT,
  ntdoc, MagnumDB, phnt ntbcd.h, Linux kernel efi.h, Wikipedia all carry the real
  values), and NO real disk. Session-1's "canonical" table fabricated them via
  mixed-endian mis-decode; its harness wrote the fabricated bytes into its fixture,
  the pre-fix core (correctly) rejected it -> false positive recorded; the "fix" then
  made the wrong fixture pass. Regression tests are self-referential (write the
  constants into the fixtures).
- EXECUTED PROOF (A-002): suite fixture GptDynamicDiskCanonicalGuidsParse body with
  ONLY the entry GUIDs swapped to the real Microsoft bytes: current tree FAILS
  parse_ldm_database (rc=1342373889 ERR_BAD_MAGIC); pre-1cb6a43 constants (scratch
  include copy, repo untouched) PASSES (2 layouts resolved, is_gpt). Evidence:
  diff-to-fix/evidence/executions/s2-ldm-real-guid-proof.md + scripts dir.
- Also found (T2 follow-through, S-005 residual): ZfsSearchDLL :24985-25014
  (FUN_18001c630) converts a name via MultiByteToWideChar(0xfde9=CP_UTF8, 0, ...),
  unlike EXT4/Btrfs (0 conversion imports - verified). Core bakes U+FFFD at scan time
  (cr_text.h policy) while og EXT4/Btrfs pass raw bytes to the UTF-8 .rsf. Filed
  MINOR N-010 (invalid-name fidelity) + evidence quote; ZFS path identification
  tracked as Z-1 open question.
- Audit trail updated: A-002 + I-006 + N-010 filed; A-001 REOPENED (LDM half false
  positive, addendum written); OQ-T7 tracked-open file RESOLVED section; modules/ldm.md
  B1/B2/B8/C19/A27 + findings index; CROSS-REF-MATRIX table+ldm/ext4/btrfs/zfs/session
  rows; SUMMARY session-2 header + counts + top risk.

Stage Summary:
- OQ-T7 RESOLVED: RAID-5 rotation = left-symmetric on both sides (MATCH); og's other
  three rotations are dormant dead code (I-006).
- A-002 ACUTE filed with executed proof: the current tree cannot recover ANY real GPT
  dynamic disk (A-001 fix regression; the fix's "canonical" LDM values are fabricated).
- A-001 reopened for its LDM half (false positive documented; its 4 non-LDM constants
  stand fixed).
- GPT-LDM gating resolved: og gates on the REAL metadata GUID + last-LBA PRIVHEAD
  anchor, no CRCs - matching the core's structure exactly.
- New: N-010 (invalid-UTF-8 name fidelity: og EXT4/Btrfs raw-bytes-to-.rsf vs core
  FFFD-baked; og ZFS CP_UTF8-converts at least one path).

---

---
Task ID: 9 (A-003 discovered + fixed; S-008 parallel-leg reconciliation)
Agent: main engineer (diff-to-fix remediation session, continuation leg 3)
Task: ACUTE A-003 (discovered) — fix --part wrote at disk LBA 0; volume-scan undo
logs slice-relative. Includes reconciliation with the parallel leg's S-008.

Work Log:
- Pulled mid-flight: the parallel remediation leg had landed S-004, S-008
  (independently implemented), N-002, N-003, N-010, M-006, I-001..I-005 acks,
  plus a whole diff-audit session-2 wave (17 new findings, findings/FIXED/
  layout, CORPUS-ERRATA, module updates). My in-flight S-008 commit
  conflicted on rebase; ABORTED it and reset to origin/main (their S-008 is
  complete and broader: $FILE_NAME identity checks, repair events, undo-save
  ABI, VolumeWriteSlice). My S-008 implementation was superseded - NOT
  re-landed (no duplicate code).
- Verified their S-008 first-hand (read the diff; ran their 16 new tests;
  suite 297/297 + 15/15 + ABI green at 1882847).
- A-002 verified STILL LIVE on their code: cmd_fix --part analyzes a
  PartitionSlice and executes on the whole PhysicalDrive device with no
  translation (grep: no rebase in their rrecov.cpp). Filed ACUTE/A-003 per
  section 8 (in their findings/FIXED/ layout, with Resolution).
- DISCOVERED the undo-direction instance while designing the fix: their S-008
  writer adapters (VolumeWriteSlice/UiVolumeWriteSlice/ImageWriteDevice-at-
  offset) map slice-relative plans onto the parent, so repairs land correctly,
  BUT the undo log records slice-relative offsets while the documented
  'fix --undo-apply' contract replays against the whole disk - a volume-scan
  undo restores at the WRONG disk offsets. Both instances documented in the
  A-002 finding.
- Fix: ONE basis mechanism - repair::rebase_plan (core, 4 lines, unit-proven)
  applied AFTER consent, BEFORE execute:
  * orchestrator: rebase by slice->absolute_base() (volume scans get
    parent-absolute plans; undo logs parent-absolute).
  * cmd_fix: rebase by part_base before the dry-run display (display shows
    "(disk-absolute offsets)").
  * CLI scan hook: returns the UNMAPPED parent writer (adapter removed).
  * UI hook: image writer opens at base 0; UiVolumeWriteSlice removed.
  * Consent callbacks still see slice-relative offsets (the scanned volume's
    own coordinates) - the rebase is an execution-basis concern.
- Test: RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0 (partition
  slice -> analyze -> rebase -> whole-disk execute: MBR at LBA 0 byte-identical,
  repair at part_base+MFT_OFF byte-equals the mirror record, undo log offset
  part_base-absolute). Fail-before is structural (without the rebase call the
  execute writes at disk offset 0 and the MBR assertions fail).
- Audit trail: findings/FIXED/A-003-*.md + RESOLVED-INDEX row (12 resolved);
  SUMMARY ACUTE rows (renumbered A-002->A-003 to clear the collision with the
  diff-audit-s2 leg's A-002 GUID-regression finding, which landed first); modules/rawfixer.md continuation addendum.
- Suite: 298/298 core + 15/15 UI + ABI check green (Linux ASan build).
- REBASE NOTE (vs 18-b): this entry was rebased onto the Windows CRT-sharing
  fix (_wfsopen _SH_DENYNO) - the A-002 changes and the CRT fix touch
  different lines of the same image-device code and compose cleanly; the
  summary counts above were re-merged with the parallel leg's S-002/M-010
  reconciliation (S-002 FIXED, M-010 filed, tracked-open 13, A-002 in FIXED:
  43 active / 12 fixed).

Stage Summary:
- A-002 FIXED in both instances (fix path + scan undo path) with a single
  parent-basis rule; the S-008 parallel implementation is preserved and now
  basis-correct for volume scans. No duplicate S-008 code landed.

---
Task ID: S2-T4+T6 (session-2 diff-audit: FAT/exFAT deep pass + ZFS vdev decode)
Agent: session-2 parity auditor (read-only, LDM-deep-pass leg)
Task: Resolve OQ-T2 (FAT mirror switch), OQ-T5 (exFAT ChecksumDBR skip-set), OQ-T6
(ZFS nvlist anchor + multi-vdev), refine OQ-T9 (CalcPartition machinery); file divergences
found on the way.

Work Log:
- OQ-T2 RESOLVED (first-hand): extracted CDoubleFATTable/IFATTable/CFAT12/16/32Table
  vtables from FATSearchDLL.dll via MSVC RTTI (TypeDescriptor -> COL -> vftable; probe
  scripts in evidence). Child slot1 = GetNext (canonicalized; BAD raw); CDouble slot2 =
  merged read; slots 3/4 = primary-only/mirror-only thunks (disassembled directly - the
  decompiler skipped them). The mirror consult trigger: primary GetNext == 0 (FREE) ONLY;
  BAD is served from the primary without mirror consult (:54215-54219). Mirror answers
  are GATED (dirty-EOC family >=0x0FFFFFF9 + three mirror self-consistency cross-checks)
  not served raw. Load-time FAT-location fallback: invalid declared FAT -> internal
  LBA-keyed map walk for an alternative FAT image (gated by volume flag 0x10, :3805-3949).
  Whole-FAT in-memory caching at construction (single bulk read) + per-flavor
  cluster-count sanity windows.
- N-022 filed (MINOR): core consults mirror on primary FREE *or BAD* (cr_chain.cpp:104-110,
  "choose-healthy" comment from the artifact doc's INFERRED semantics); og consults on
  FREE only -> core recovers longer chains on torn FAT pairs (superset the og never
  produces). Evidence: evidence/decompiled-quotes/s2-fat-double-fattable.md.
- OQ-T5 RESOLVED (MATCH): ChecksumDBR predicate decoded - the exception-machinery
  confound resolves to std::find (FUN_18003ab40 :41083-41092) over a 4-int array at
  RawFixer.dll RVA 0x180079a40, extracted from the binary: {106, 107, 112, -1} = exactly
  the exFAT spec skip-set (VolumeFlags@106-107, PercentInUse@112) + a -1 terminator no
  loop index matches. Loop bound sector_size x 11, spec rotation. Core spec-skip-set
  (cr_exfat_dentry.cpp:26-36) MATCHES. Corpus erratum confirmed (rawfixer-v2 S4's
  "bytes 11..209 + 510" is wrong). Evidence:
  evidence/decompiled-quotes/s2-exfat-checksumdbr.md.
- OQ-T6 PARTIALLY RESOLVED (bounded): the og's fixed nvlist anchor quote-confirmed
  (:21052: one 16 KiB read at 0x1C000, parse +4, limit 0x1BFFC); vdev_tree handling
  decoded (:21089-21150): only the "id" key per child is extracted into an id-keyed
  std::map (key constant DAT_180031570 = ASCII "id" extracted from the binary). Strings
  scan of ZfsSearchDLL.dll: ZERO vdev-type strings (mirror/raidz/replacing/spare/cache/
  log/disk/file) and ZERO geometry keys (ashift/asize/nparity/children) -> the og build
  is vdev-TYPE-BLIND: no mirror copy selection, no RAID-Z parity math possible. The
  core's unsupported_vdevs refusal is the honest superset. Refined residual: the read
  path through the id map (mirror first-child semantics) - recorded in tracked-open.
- OQ-T9 refined: CalcPartition machinery located (Fat32CalcPartition::vftable :5744,
  score via vtable slot 4 with Fat32 direct-field fast path :5745-5747; ICalcPartition
  dynamic casts :5592-5595/:5816-5818). Next decode target recorded in tracked-open.
- Also during this wave: the parallel fix leg landed 9888ec9 (their own discovered
  ACUTE - fix --part wrote at disk LBA 0; renumbered to A-003 to clear the collision
  with this leg's A-002, with an explicit ID note - clean coordination).
- Audit trail: OQ-T2/OQ-T5 resolutions + OQ-T6 bounded + OQ-T9 refinement written into
  tracked-open; N-022 filed; fat/exfat/zfs module files updated; SUMMARY counts updated
  (MINOR 17; tracked-open 12 with T2/T5/T7 resolved).

Stage Summary:
- OQ-T2 RESOLVED (per-entry zero-fallback mirror consult; N-022 divergence filed).
- OQ-T5 RESOLVED (spec skip-set {106,107,112} - core MATCH).
- OQ-T6 bounded decisively (og is vdev-type-blind; core's refusal is the honest
  superset; mirror first-child residual recorded).
- OQ-T9 machinery located for a future scoring decode.

---
Task ID: S2-CLOSE (session-2 diff-audit close-out — third leg)
Agent: session-2 parity auditor (read-only, LDM-deep-pass leg)
Task: Session close-out — handoff record.

Work Log:
- This leg's session-2 contributions (3 pushed waves: 23adad1, f620234, 5c3a5ce):
  * A-002 ACUTE filed with executed proof + og-binary extraction + 4 external sources
    (the A-001 fix corrupted both LDM GPT type GUIDs; real GPT dynamic disks fail LDM
    discovery on the current tree). A-001 reopened for its false-positive LDM half.
  * OQ-T2 RESOLVED (FAT mirror consult decoded; N-022 divergence filed).
  * OQ-T5 RESOLVED (exFAT ChecksumDBR = spec skip-set; core MATCH).
  * OQ-T7 RESOLVED (RAID-5 left-symmetric both sides; I-008 dormant rotations).
  * OQ-T6 bounded decisively (og vdev-type-blind; core's refusal = honest superset).
  * OQ-T9 machinery located. 5 evidence files + 3 finding files + module/matrix/
    SUMMARY/tracked-open updates; all reconciled with the two parallel session-2 legs
    (ID collisions cleared both directions: my I-006->I-008, my N-010 dropped as
    superseded by their M-010, their A-002->A-003 crediting mine).
- Verified first-hand this session: the 5 session-1 fixes (A-001 partial, S-001, M-001,
  N-001, S-005) — S-001/M-001/N-001 decompile cites re-read byte-exact, suite 288/288
  (ASan, gtest built from source); the parallel legs later re-ran 313/313 at newer
  baselines.
- Not completed by this leg (honest handoff): OQ-T8 (carve dispatch order in
  RAWSearchDLL - 80k lines, machinery not yet located), OQ-T9's scoring feature/weight
  table decode (machinery located: FATSearchDLL :5744 vtable slot 4), the ReFS ADS
  recovery gap (project GAP-ADS still open - the mandate's "ReFS first-class" clause),
  and the NTFS/ext4/HFS+/Btrfs per-engine deep passes (the parallel legs covered NTFS
  deeply: M-007/M-008/N-011/N-012; ext4/HFS+ timestamps: I-006/I-007; S-002/S-004
  verified fixed; S-003 deferred with a decoded-evidence addendum).

Stage Summary:
- Recommended first 20 fixes (updated for session-2 state): 1) A-002 (two-line constant
  restore + de-self-reference the regression tests - the ONLY ACUTE, blocks all GPT
  dynamic-disk recovery); 2) A-003 (fix --part LBA-0 write bug, landed 9888ec9 - verify);
  3) S-003 (HFS+ journal replay - evidence addendum decoded); 4) S-006 (lost-partition
  probe families); 5) S-007 (carve formats); 6) S-009/N-020 (disk-map finding split);
  7) S-010 (rawfixer write-path volume lock); 8) S-011 (exFAT TZ offset); 9) M-002 (EBR
  torn-chain salvage); 10) M-005 (carve bitmap gating); 11) M-007 (NTFS hard-link
  fan-out); 12) M-008 (attribute-list chase scope); 13) M-009 (rawfixer 64KiB envelope);
  14) M-010 (invalid-UTF-8 name fidelity - raw bytes to session); 15) N-022 (FAT mirror
  BAD-consult policy decision); 16) N-011..N-021 (NTFS ADS nodes, dest-overlap,
  checkpoint atomicity, writer pre-check, emit-boundary, cancel-watcher, UBSan CI,
  regression tests, doc drift, FAT invalid date); 17) I-008 (comment-only); 18-20) the
  remaining minors from the parallel legs' waves.
- Coverage statistics for this leg: read first-hand this session: LdmManager_decompiled.c
  (RAID-5 + GPT gate + PRIVHEAD paths, ~600 lines deep), Partition_decompiled.c (GPT
  entry scan), ZfsSearchDLL_decompiled.c (nvlist/vdev/name paths, ~350 lines), FATSearchDLL_decompiled.c
  (CDoubleFATTable cluster + child tables + CalcPartition sites, ~450 lines),
  RawFixer_decompiled.c (ChecksumDBR + predicate chain, ~120 lines),
  NTFSSearchDLL/NTFSLib decompiles (verification cites), plus binary extractions
  (LdmManager.mo, Partition.mo, ZfsSearchDLL.dll, FATSearchDLL.dll, RawFixer.dll
  - RTTI vtables, GUID constants, skip-set arrays, strings censuses).

---
Task ID: 5-b (S-010 fix — this leg)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: SEVERE S-010 — repair write path lacks the og's volume lock/dismount quiesce

Work Log:
- Verified the finding first-hand: corpus RecoveryModule_decompiled.c:44686-44720 (and the
  PartitionRecovery mirror) shows FSCTL_LOCK_VOLUME 0x90018 -> DeleteVolumeMountPointA ->
  FlushFileBuffers -> Sleep(2000) -> FSCTL_IS_VOLUME_MOUNTED 0x90020 verify ->
  FSCTL_DISMOUNT_VOLUME 0x9001c before repair writes; our RwDevice opened
  share-read+share-write with no quiesce. CONFIRMED-DIVERGENCE.
- Fix: IWriteDevice::lock_volume()/unlock_volume() (S7 in the rawfixer safety standard;
  file/RAM writers keep the documented OK default - no OS volume); execute() acquires the
  lock before the first write, releases on every exit via a guard, propagates the device's
  error (a busy volume = ERR_VOLUME_LOCK_FAILED, new code) with ZERO writes performed.
- Windows RwDevice::lock_volume(): per MOUNTED volume on the writer's disk (enumerate_volumes
  filtered by the new disk_ member), the corpus sequence verbatim; the volume handles are
  held for the device lifetime so the locks live until close (Win32 contract). The two
  volume-slice adapters (rrecov VolumeWriteSlice, cr_ui UiVolumeWriteSlice) forward the lock
  to the parent transport. CLI consent print now warns that drive letters are removed during
  repair (corpus behavior, consent-gated).
- CORRECTED THE FINDING (corpus wins): its fix-plan bullet "the writer's destination volume
  should ALSO take the lock (cr_destio.cpp)" mis-reads the corpus - 04-recovery-workflow.md
  §7.3 opens \\.\X: of the destination for WRITABILITY + SAME-VOLUME detection only, and
  recovery writes go through normal file APIs (§7.5), which REQUIRE a mounted destination;
  the og does not lock the destination and neither do we (the overlap refusal F62 covers
  the same-volume case). Correction recorded in the Resolution.
- Tests (fail-before proven by stashing only cr_rawfixer.cpp - 3/3 fail pre-fix):
  RawFixer.VolumeLockAcquiredBeforeFirstWrite (write path records a violation if it ever
  writes without a lock), RawFixer.VolumeLockFailureNeverWrites (refused lock = zero
  writes, volume bytes untouched, undo log empty), RawFixer.UnlockHappensOnRollbackPath
  (guard fires on the failure exit too). Suite 316/316 Linux ASan.
- Audit trail: S-010 moved to findings/FIXED/ with Resolution (incl. the correction);
  modules/rawfixer.md S-010 rows -> FIXED; CROSS-REF-MATRIX rawfixer B/F cells -> M
  (S-008+S-010 fixed); SUMMARY remediation log row; CAPABILITY-MATRIX; ABI-CONTRACT repair
  SAFETY line.

Stage Summary:
- S-010 FIXED: the repair write path now quiesces mounted volumes the way the og does
  (lock/delete-mountpoint/flush/settle/verify/dismount, locks held until writer close),
  refuses instead of racing a busy volume, and the destination-volume bullet was corrected
  against the corpus. Next in this leg's lane: M-009 (same file - RwDevice read path
  bypasses the 64 KiB envelope).
---
Task ID: 5-b-addendum (S-010 x A-002 integration)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: Rebase S-010 onto the parallel leg's A-002 parent-basis writer redesign

Work Log:
- The parallel audit/fix legs landed A-002 (ACUTE: slice-mapped undo logs carried
  slice-relative offsets - wrong restore basis for fix --undo-apply) and redesigned the
  repair write path: hooks return the UNMAPPED parent RW writer; the orchestrator calls
  repair::rebase_plan(plan, slice->absolute_base()) before execute. The two volume-slice
  adapters from my S-008/S-010 work were removed by that design.
- Rebased S-010 onto it: the S-010 semantics are unchanged and simpler now - the parent
  writer IS the RwDevice with lock_volume(), so execute()'s lock-before-write contract
  reaches the corpus LOCK/DISMOUNT sequence directly (no forwarding layer). The A-002 test
  (RebasedPartitionPlanWritesAtDiskBasisNeverLba0) and the three S-010 lock tests coexist.
- Resolution + worklog annotated; full local sweep re-verified below.

Stage Summary:
- S-010 and A-002 integrated cleanly; the repair write path now carries BOTH the
  disk-absolute offset truth (A-002) and the volume quiesce (S-010).

---
Task ID: 18-c (S-010 CI follow-ups: import gate + format)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: S-010 push tripped the Win7 import allowlist + format gate

Work Log:
- The Windows PE gate (verify_pe_um.py) failed on the S-010 import
  DeleteVolumeMountPointW (kernel32). The gate's documented process: conscious
  ALLOWLIST_ADDITIONS entry with a reason. Added: the corpus sequence removes the
  drive letter before dismounting (RecoveryModule_decompiled.c:44699, A-variant there,
  W = same syscall; Win2000-era API, Win7-inclusive by construction). The gate's own
  test (tools/test_verify_pe_um.py) passes with the addition.
- test_rawfixer.cpp re-formatted (the rebase-resolution merge left two drift lines).
- Local gates: 317/317, format clean (18.1.8), PE-gate unit tests pass.

Stage Summary:
- Both CI regressions from the S-010 push repaired at their documented roots.

---
Task ID: S3-BOOT
Agent: session-3 parity auditor (read-only continuation)
Task: Session-3 bootstrap — state reconciliation at HEAD 34b3441 (sessions 1+2 complete; engineering fixed S-010 in between)

Work Log:
- Cloned both repos at core `34b3441` / corpus `03ad46c` (unmoved). Prompt saved redacted
  to worklog/session-3-diff-audit-prompt.md, read twice.
- Read the full session-2 state: SUMMARY.md, README.md, CROSS-REF-MATRIX.md, all 34 active
  finding IDs, 3 tracked-open files, RESOLVED-INDEX, worklog tail (handoff).
- STATE FINDINGS (to reconcile this session):
  * S-010 (rawfixer write-path volume lock) — FIXED by engineer (3e34ed2 + 414f56c +
    34b3441, tests 317/317); already moved to findings/FIXED/ by the fix leg. The active
    SUMMARY/README still list it as top risk #1 -> STALE, will reconcile.
  * A-002 (LDM GPT GUID regression, the only ACUTE) — STILL LIVE: fabricated GUIDs
    confirmed in-tree at include/cr/table/cr_table_wire.h:87-90 + self-referential tests
    at tests/unit/test_crc32_gpt.cpp:385-386.
  * M-009 (RwDevice read path 64KiB envelope) — engineer's lane said "next"; to verify
    at HEAD this session.
- NEW DEEP-DIVE TARGETS IDENTIFIED (session-3 scope):
  * raw_signs.json semantics (RAWSearchDLL FUN_18001ea40 loads it via ifstream): the
    core's raw-sign layer (cr_rawsigns.h, landed 9b61246 pre-audit) DOCUMENTS its
    semantics as assumed ("whose exact hit handler was not RE'd to completion") —
    512-byte stride, tail-pairing bounds, conf formula all unverified. Decode from
    the decompile; file divergences.
  * OQ-T8 (RAWSearch dispatch order) + OQ-T9 (CalcPartition scoring) — both carry
    located machinery; decode this session.
  * ReFS first-class deep pass (mandate clause): ReFSSearchDLL_decompiled.c (545KB) +
    refs artifacts vs core cr_refs; the GAP-ADS open item.
  * CPartitionVirtualVolumeDevice (og virtual-volume device layer, HFSPlusSearchDLL +
    DRWUI RTTI) — never audited; compare against core storage/table layers.
  * FileFilter.xml (filter model, 8 cats / 1481 ext per CORRECTIONS #5) vs core filter.
  * Health module (matrix verdict THIN): diskhealth artifacts + SMART companion vs core.

Stage Summary:
- Session-3 scope locked: reconcile fixed findings, then six deep passes (raw-signs
  decode, OQ-T8, OQ-T9, ReFS, CPartitionVirtualVolumeDevice, filter/health), then
  consolidation + push waves.

Task ID: 9-a (M-009 fix — this leg)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: MAJOR M-009 — RwDevice read path bypasses the 64 KiB direct-I/O envelope

Work Log:
- Re-verified first-hand: RwDevice::read_at called overlapped_read directly while both
  scan-path devices route through aligned_read_at; the GAP-S6-13 mandate
  (cr_directalign.h) requires every direct read path to round up to 64 KiB multiples.
  CONFIRMED-DIVERGENCE (perf-only, correctness unaffected).
- Fix (one line + comment): RwDevice::read_at -> aligned_read_at(h_, off, buf, len,
  geom_, via_rda_, size()) - identical routing to DirectDevice/RdaTargetDevice; the
  previously-unused via_rda_ flag now carries the F11 unscale semantics on the RDA
  branch. Byte-identical RMW semantics (covered by the existing transactional tests).
- Verification: structural routing parity (the exact envelope call the scan devices
  make), envelope unit tests (test_directalign.cpp), full suite 317/317 Linux ASan;
  Windows CI compiles/runs the routed path (PE gate intact). Executed 4Kn before/after
  numbers not producible here (no Windows/4Kn host) - recorded honestly in the
  Resolution; the bench harness is the vehicle, per the finding's own reproduction note.
- Audit trail: M-009 moved to findings/FIXED/ with Resolution; module file, matrix X
  cell (M-009 fixed), SUMMARY remediation row.

Stage Summary:
- M-009 FIXED: every direct read path in the core now routes through the 64 KiB
  envelope - the GAP-S6-13 mandate holds repo-wide.=== session-2 MAIN-AUDITOR leg worklog entries (re-applied post-rebase; original order preserved) ===
---
Task ID: S2-0 (session bootstrap)
Agent: main auditor (session-2 diff-audit continuation, read-only)
Task: Session 2 — verify session-1 remediations, remove FIXED findings from the active tree, audit post-closeout code, hunt session-1 coverage gaps (decompiled-tree exhaustive reads + uncited corpus artifacts)
Work Log:
- Cloned both repos fresh. Baselines: core e93decf (main), corpus 03ad46c (unmoved since session 1).
- Token hygiene verified: token appears nowhere in either repo (worktree + full history, git grep + git log --all -S both empty).
- Read session-1 state in full: diff-to-fix/* (31 finding IDs), worklog.md audit closeout + remediation entries.
- Post-closeout commits in session-2 scope: 56cce87 (GAP-S6-10/11 disk-map producer + typed event stream), 1cb6a43 (A-001 fix), 2f3237d (S-005 fix), e93decf (S-001 fix + OQ-T1 resolved); 448a57c is worklog-only.
- Fix-state triage at bootstrap: A-001/S-001/S-005 claim FIXED with Resolution sections — to be independently verified this session, then removed from the active findings tree per the session-2 mandate. S-009 disk-map code present (cr_diskmap.cpp/h + cr_ui_disk_map ABI + tests) — completeness vs its own fix plan to be verified. M-006 fuzz CI confirmed still `if: false` (stays open).
- In-flight remediation lanes (worklog 448a57c): S-008 -> S-006 -> S-007, no commits landed. This session fixes nothing and does not trample those lanes.
- Session-2 prompt saved redacted at worklog/session-diff-audit-2-prompt.md; session-1 SOP prompt re-read twice per mandate.
- Spawned parallel agents S2-A..S2-F: (A) NTFSSearchDLL decompile exhaustive, (B) FAT/exFAT decompile + OQ-T2/T5, (C) EXT4/HFS+/Btrfs/ZFS/ReFS decompiles + OQ-T6, (D) PartitionRecovery/LdmManager/RAW/MO decompiles + OQ-T7/T8/T9, (E) post-closeout core code parity audit (fix verification + new-code divergences), (F) corpus coverage-gap sweep (uncited artifact dirs + raw-evidence + CORRECTIONS.md fold-in). Context sheets -> /home/z/my-project/scratch/ (outside both repos).
- Kicked off out-of-tree Linux build + full ASan test suite (../build-audit) for empirical verification of fix claims.
Stage Summary:
- Session-2 baseline recorded: core e93decf, corpus 03ad46c. Read-only compliance holds (only diff-to-fix/ + worklog writes).
- Planned removal set (pending verification): A-001, S-001, S-005, OQ-T1, and S-009 if the disk-map implementation satisfies its own fix plan.
---
Task ID: S2-F
Agent: corpus coverage-gap sweeper (session-2 parallel read, read-only)
Task: Cited-vs-uncited sweep of the corpus (artifacts/ + raw-evidence/ + architecture/) vs diff-to-fix/; extract recovery-relevant semantics from files session-1 never cited.
Work Log:
- Read session-1 context: diff-to-fix/{README,SUMMARY,PARAMETERS}.md, modules/{orchestration,health,session,rawfixer,writer,abi,carve,ldm,lost-partition,visualization,cli}.md, findings/*, tracked-open/*, worklog closeout entries.
- Built strict citation inventory (parent-dir/basename OR shorthand citations across diff-to-fix/; loose-basename hits triaged for false positives like Config/Device/Error/Log; brace/numbered shorthands recognized: carve.md:5 family docs, session.md:4 ifileproperties, visualization.md:3 UIUX/03/04/08/10): 424 files in scope (274 artifacts + 141 raw-evidence + 9 architecture); ~42 cited (~10%). Full table: scratch/S2-F-context.md.
- Coverage-gap admission proven: F61 (BitLocker) appears in diff-to-fix ONLY as PARAMETERS.md:123 taxonomy row + BitlockerParser name in S-006 — never scored in any module file. No transport module file exists (F59 unscored outside health/ldm NA rows).
- BitLocker deep-dive from UNCITED docs (DRWService-exe--bitlocker-service.md, PartitionRecovery/bitlockerlib--bitlocker-implementation.md, MOM-modules/bitlockerhelp.md, per-binary/BitLockerLib.md) + raw decompiles; compared vs src/win/cr_bitlocker.cpp. Found: (1) core reads HKLM\SYSTEM\CurrentControlSet\Services\FVEAutoUnlock (cr_bitlocker.cpp:219) but og reads HKLM\SYSTEM\CurrentControlSet\Control\FVEAutoUnlock — 4 independent raw-evidence sites (DRWService_decompiled.c:104, DRWService.exe/strings_ascii.txt:32, bitlockerhelp_decompiled.c:3139, bitlockerhelp.mo/strings_ascii.txt:454) → auto-unlock never finds keys the og applies (SEVERE); (2) og additionally reads auto-unlock keys from the TARGET volume's own OFFLINE SYSTEM hive "<vol>\Windows\System32\config\SYSTEM" + "\ControlSet001\Control\FVEAutoUnlock\" + "<GUID>\Data" via RegLib::OpenHiveByVolume (bitlockerhelp_decompiled.c:3128-3155, RegLib_decompiled.c:203/275; module-internals-wave2 §2) — the dead-system/WinPE path; core has no offline-hive reader at all (grep hive/regf/OpenHive → empty) (SEVERE); (3) core blind-enumerates every REG_BINARY>=32B without the og's volume-GUID->"Data"-value association or the 100-record/256B caps (MINOR).
- Read UNCITED driver-contract docs (eprdtdrv-sys--driver-internals.md, epmntdrv-internals-v2, epmntdrv-trio--naming-pid-handshake, eudcpepm x2, getdriverinfo, architecture/02-kernel-driver-architecture.md) + Device.mo decompile; verified vs core transport: PID-token transform byte-identical (cr_transport.cpp:285-289 vs trio §3 [CODE]); 200-disk cap matches (cr_rda.h:66); IOCTLs 0x60002400/04/08 match v2 (cr_rda.h:41-43); F59 driver-absent duality matches by design (Device_decompiled.c:7793-7822 EPRDTDRV→PhysicalDrive fallback = cr_transport.h:23-37). NEW corpus errata candidate: v1 driver doc §4 mislabels 0x60002400 ("global target") — decompile truth is GetProcName (:496 dispatch, :410/:419/:431 traces); core already matches v2.
- Read UNCITED config-files docs (config-data-reference.md, filtering-priority--config-layer.md, userdata-remoteconfig, drwcfg, vendor-logs): FilterFolder.ini junk filter (desktop.ini/.DS_Store/$UsnJrnl/localized "New Folder"/System Volume Information/BitLocker FVE artifacts hidden from og results, pushed at scan setup per runtime log PushFolderFilter) has NO core counterpart and NO registration anywhere in core docs (MAJOR); RawConfig.ini/EUUserData gating already registered project-side (PARITY-CORE-REC F-A3-13).
- Read UNCITED raw-evidence runtime logs (0/141 files cited by session-1): applog_log_CalcPartition.log pins actual og "dSuggestted" lost-partition scores (1.0 file-bearing, 0.632748 zero-file valid, 0.0 over-large; file-count monotone; :2/:83-85/:189-197) — a ready verification asset for tracked-open OQ-T9 that session-1 never used; EPMLOG pins enumeration chain + ReFS probing on all volumes; DRWUI.log confirms bottomarea state machine + quick→deep transitions live.
- Read UNCITED module-internals-wave2 + verified in LdmManager decompile: og TOCBLOCK candidates = config+2, config+0, config+1 first-valid (:59573-59606; +0x80=config_start from PRIVHEAD@0x12B :59748) vs core kernel slots {1,2,2045,2046} (cr_ldm.cpp:1112-1125) — detection gap (config+0) + superset (2045/2046) + different copy preference (MAJOR); VMDB 4.10 gate matches; VMDB status stance already registered (PARITY-DELTAS:175-181).
- Read UNCITED grading/props docs: CRecoverability is DRWUI-side (presentation, not claimed by core) — no finding; props-vtables.txt + ifileproperties-part1 pin og NTFS 8.3 shortName node field (CNTFSFileProperties +0x00/+0x08/+0x10 GetShortName*) — core node has single name (cr_node.h:68), contradicting session.md "every og field has a core counterpart" (MINOR); carved-file naming "File0001.jpg" (rawsearch-carving:75) vs core "Carved%06u.%s" (cr_scan.cpp:1056) (MINOR).
- CORRECTIONS.md fold-in: rows 2,3,4,6,7,8 folded; row 1 NOT folded (moot — no consumer, core has no 0xFFF gate); row 5 partial (visualization.md uses both 1,481 and stale 1,489 via CORE-GAP-LIST).
- Scope checks: Cloud/NAS = CAPABILITY-MATRIX.md §3 + PARITY-DELTAS D-26:580; FileRepair = PARITY-CORE-REC.md:164 + PARITY-CORE-DOM.md:33; WinPE-builder/Languages/Licensing/telemetry = D-26; partition-table write-back gap = already registered PARITY-ARCH.md F-D-5 but absent from diff-to-fix lost-partition.md (fold-in gap).
- No files in either repo modified; writes = scratch/S2-F-context.md + this entry only.
Stage Summary:
- Deliverable: citation inventory (424 files, ~10% cited) + 7 new divergence candidates + 2 coverage-gap admissions (F61 never scored; no transport module) + 1 new corpus errata candidate + OQ-T9 verification asset (raw CalcPartition.log scores). Headline: (SEVERE) core BitLocker auto-unlock reads the wrong registry path and lacks the og's offline-hive path — volumes the og auto-unlocks stay locked in core; (MAJOR) og's default result-set junk filter (FilterFolder.ini) absent and unregistered; (MAJOR) LDM TOCBLOCK candidate set diverges (og probes config+0, core probes 2045/2046 only og skips). Details + all citations in scratch/S2-F-context.md and the session report.
---
Task ID: S2-E
Agent: post-closeout core code parity auditor (session-2 parallel read, read-only)
Task: Audit the post-closeout commits (56cce87 disk-map+events, 1cb6a43 A-001, 2f3237d S-005, e93decf S-001) for og parity; verify session-1 fix claims; hunt divergences the new code introduced.
Work Log:
- Read diff-to-fix context (README/SUMMARY/PARAMETERS, S-001/S-005/S-009/A-001 findings, OQ-T1 resolution, ABI-CONTRACT sections 6-7, worklog remediation entries). Baselines: core e93decf, corpus 03ad46c.
- Read all four scope commits in full (git show + working tree). Full context sheet with quotes: /home/z/my-project/scratch/S2-E-context.md.
- S-009/GAP-S6-11 (56cce87): read cr_diskmap.cpp/h + disk_map_impl + tests vs the finding's 4-step fix plan, ABI-CONTRACT section 7, and corpus (searchdll-family :31 IBuildVolumeBitmap "free/used", ntfs-engine--deep-dive :28-36 controller-IS-bitmap-builder, UIUX/08:11 + 04:110 + 13:47 AnalysisGridWidget). Verdict PARTIALLY-FIXED: geometry/overlays/bad-ranges/zoom/64MiB budget implemented and ABI-documented, but (1) engine bitmaps NOT wired (cr_scan.cpp:706-707 loads VolumeBitmap with (void) and discards; allocation USED comes only from active-structured extents; kCellAllocFree(2) is never produced - the og grid's free/used distinction is unreachable), (2) NO versioned CRSF disk-map record (src/core/session grep: only pre-existing serialize_error_map; fix-plan step 3 missing - sessions recompute at 64 KiB because cluster_bytes is not persisted), (3) fix-plan tests DiskMap_Ntfs_Allocated_Free_Regions / DiskMap_Bad_Sectors_From_ErrorMap absent (both tests pass bad_ranges=nullptr), (4) snapshot path never wires the error map (CrSnapshot carries only the tree) so mid-scan maps cannot show bad/unreadable cells. M-006 (fuzz CI) still if:false - stays open.
- GAP-S6-10 typed events (56cce87): extracted the event model (6 types, POD, permille-CAS coalescing, boundary samples, pre-run-only sink) vs the og pipe (mom-modules--ui-bridge section 1: INotifyObserver::Notify(int,void*,int&) -> CMessageSend envelope -> thread-affinity dual connect -> doDispatchMessage -> 15 subscribers; UIUX/13 section 3; UIUX/25 section 2). og msgType constants NOT decoded anywhere in the corpus (checked artifacts + DRWUI decompile dispatch sites) - parity is architectural, not constant-level. Found TWO new-code defects: (a) cr_scan.cpp:340 emit_boundary_events_ fires ErrorRange via cfg_.event_fn while the :309 guard checks event_fn_ - a set_event_sink-only subscriber (the header-documented resumed-flow pattern) gets a null function-pointer call on the first read-error range (crash-class; cr_ui shim sets both so ABI/CLI users are safe); (b) cr_scan.cpp:577-578 duplicated emit_boundary_events_ (both lines added by 56cce87) - double FilesFound events at run teardown.
- A-001 (1cb6a43): hand-decoded all six constants vs winioctl.h text forms (byte-exact, incl. "Hah!IdontNeedEFI"); ran an independent 1470-file og-binary scan: ESP disk-order in 8 binaries, MSR in 7, basic-data in 6, ZERO text-order hits - matches the corrected evidence file. Ran fresh decompile probes: DRWUI :893855 memcpms the formatted type GUID against "C12A7328-F81F-11D2-BA4B-00A0C93EC93B" and sets a dedicated +0xc4 ESP flag (og treats ESP as its own category - supports FsKind::EfiSystem); DRWUI :609/:613 register legacy NON-canonical LDM GUIDs (5808C8AA-7E8F-42E0-85D2-E1E90434CFB3 / AF9B60A0-1431-4F62-BC68-3311714A69AD) - byte-identical to the core's OLD wrong constants (the likely source of the historical error; the og does not gate GPT-LDM on them). No "skip ESP when MSR present" logic exists in the og. Verdict FIXED-VERIFIED.
- S-005 / U+FFFD question (2f3237d): verified the decoder (per-bad-lead U+FFFD, +1 resync, overlong/surrogate/>10FFFF/truncated/lone-continuation all -> U+FFFD) and all five engine sites (ext4, btrfs, 3 zfs). Hunted the og end-to-end: DRWUI converts session/props names via MultiByteToWideChar(0xfde9 CP_UTF8, flags=0) - the node->wide helper FUN_1403121b0 (:534592-534612) and the RECOVERY filename builder's converter FUN_140163af0 (:264664-264694, called from :357529 with the name from FUN_14008a650) - flags=0 means invalid sequences become U+FFFD; the og writes the CONVERTED (substituted) filename at restore (Windows filenames are UTF-16; no byte-preserving writer on this path). .rsf record-4 keeps original UTF-8 (rsf-format :326). EXT4SearchDLL has 0 conversion imports (re-verified); Btrfs passthrough; ZfsSearchDLL has its OWN CP_UTF8 MB2WC site (:24985/:25014). VERDICT: MATCH in substance (both substitute at restore) - no new SEVERE finding; two MINOR residuals: replacement-count policy (core per-lead-byte+resync vs Windows maximal-subpart) and substitution timing (og session bytes pristine vs core session stores substituted names).
- S-001 (e93decf): verified the destroy gate (cr_mftwalk.cpp:704-708: deleted non-dir + conf==Overwritten, before node creation; folders never; live -> Excellent), sparse exclusion (cr_bitmap.cpp:96-97 skips both counters), counter, honest-mode flag, tests (EndToEndWalk: destroyed==1, dirdel kept, holes.dat Excellent), and read the og decompile directly (:21189-21256: skip gates, viability loop, `if (*plVar13 != -1)`, `0 < iVar21 && iVar20 == iVar21 -> goto LAB_18001b06f` props Release + skip list insert; no-$DATA-runlist records fall through keep). OQ-T1 write-up consistent. Verdict FIXED-VERIFIED with 2 flagged items: (1) og counts/tests PER RUN-ENTRY at the run start cluster (decompile :21220-21231; ntfs-engine--deep-dive section 4 :90-97; algorithms-v2 :44-56) while core counts/tests PER CLUSTER (reused_ratio r.lcn+i) - destroy sets diverge on fragmented deleted files (og destroys strictly more when all run-starts are allocated); (2) corpus self-contradiction on the props 0x10 bit (deep-dive :77-80 "in-use" vs algorithms-v2 :41 "not a folder") - core follows deep-dive.
- GAP spot-checks: 03ddcd2 GAP-S6-6 consistent with corpus (qa-logs--session-forensics :40-42: og surfaces 15+ overlapping candidates then AdjustPartitionInfoOrder fileCount-prunes; og's final-set scoring stays OQ-T9-open); GAP-S6-7 corpus-silent (NOT-FOUND), bench-derived only; GAP-S6-8 corpus-silent, internal-consistency fix (F-1 absolute model). 7797bd6 GAP-S6-3 verified against the decompile VERBATIM (ReFSSearchDLL :13744-13756 checkpoint-BODY minor gates 0x58/0x44 - fix matches; flagged: core hardcodes body@page+0x50 where the og reads the body offset dynamically via uVar14); GAP-S6-4 remnant $DATA consistent (og builds $DATA runlists for every record - the viability walk consumes them). 7777e1c GAP-S6-13 consistent with the og driver contract (eprdtdrv IRP_MJ_READ sector-granular passthrough; the 64 KiB envelope is user-mode-only - deliberate perf superset X51).
- Read-only compliance: no file in either repo modified; only scratch/S2-E-context.md and this worklog entry written; no build run; no tokens.
Stage Summary:
- Verdicts: S-009 PARTIALLY-FIXED (engine bitmaps unwired, no CRSF record, 2 of 3 acceptance tests missing, snapshot path lacks bad-sector overlay; FREE state unreachable vs og IBuildVolumeBitmap free/used). A-001 FIXED-VERIFIED (byte-exact constants, 1470-file og scan + DRWUI ESP-flag probe; legacy-LDM-GUID provenance note). S-005 FIXED-VERIFIED + U+FFFD MATCH (og substitutes at restore via MB2WC CP_UTF8 flags=0; two MINOR residuals). S-001 FIXED-VERIFIED (destroy gate, sparse, folders, live, counter, honest mode all verified against the decompile).
- New divergences filed: (1) cr_scan.cpp:340 ErrorRange fires through cfg_.event_fn - crash-class for set_event_sink-only core-API subscribers (from 56cce87); (2) S-001-family: og viability/destroy counts per-RUN-ENTRY at run start vs core per-CLUSTER - result-set divergence on fragmented deleted files (pre-existing ratio code, now result-set-relevant via the destroy rule); (3) props 0x10 bit corpus self-contradiction (RE-tracking); (4) og can destroy deleted dirs carrying $DATA, core never destroys dirs (edge); (5) duplicated boundary emission (double FilesFound at teardown, 56cce87); (6) ReFS body-offset hardcode vs og dynamic offset (edge); (7)+(8) U+FFFD count-policy and substitution-timing MINORs; plus writer-sanitizer observation (og '#'/incl-backslash vs core '_'/no-backslash, DRWUI :357536) routed to the N-009b/d owner. M-006 still open (fuzz CI if:false). Full evidence: scratch/S2-E-context.md.
---
Task ID: S2-B
Agent: FAT/exFAT decompile exhaustive reader (session-2 parallel read, read-only)
Task: Exhaustive FAT12/16/32 + exFAT decompile parity read (FATSearchDLL/exFATSearchDLL/RawFixer) resolving OQ-T2 + OQ-T5, verifying N-002/N-003/I-002/stream-gate, and hunting new divergences
Work Log:
- Read diff-to-fix context (PARAMETERS.md, modules/fat.md, modules/exfat.md, OQ-T2-T5 file, N-002/N-003/I-002, worklog) then the full core fat/exfat sources (cr_chain/cr_fat_quick/cr_lfn/cr_exfat_dentry/cr_exfat_quick + headers) at e93decf.
- OQ-T2 RESOLVED: CDoubleFATTable factory FUN_180006140 (FATSearchDLL_decompiled.c:3779-3960) + Next FUN_180041de0 (:54207-54251). Primary-first per-entry; mirror consulted ONLY when primary Next==0 (FREE/OOB — never on BAD); mirror answer scrutinized (backward: shifted-copy/cross-link/contiguity tests; forward: BAD/EOC-family rejected); primary child NULL -> mirror-only raw; factory retries whole-table construction from RB-tree FAT-region map when a copy fails. Never merges. Core deltas: cr_chain.cpp:102-110 consults mirror on FREE *or BAD*, no scrutiny, no mirror-only mode (FAT1 read fail -> ERR_READ_FAILED scan abort).
- OQ-T5 RESOLVED: exFATSearchDLL NEVER validates the boot checksum (zero hits for ROL31/0x1FC/x0xb across the 58043-line decompile; boot validator FUN_180021e80:28171-28213 has no checksum step). RawFixer ExFATFixer::ChecksumDBR lambda FUN_18000e7e0 (RawFixer_decompiled.c:9353-9381) sums ALL bps*11 bytes — NO spec skip-set (106/107/112); the "predicate" is pure exception_ptr plumbing (FUN_180023220=x+0x10, FUN_180001700=identity). CheckDBR (:10505-10547) compares stored dword + whole-sector fill-consistency + computed!=0; fix path (:11429-11450) recomputes and fill-repeats sector 11. Session-1 spec-skip prior REFUTED; scanner-side parity confirmed (both non-gating).
- N-003 REFUTED: og DOES have entry-set checksum validation — FUN_18001eba0 (:25153-25198, dual ROL15: raw + InUse-restored 0x85/0xC0/0xC1 form for DELETED sets) wired in FUN_18001fef0 (:26287-26292) and used as a reconstruction oracle for parked sets in FUN_1800203a0 (:26630-26664, reBindFileSet). Complete sets flow through per-entry validators (FUN_180002f90: secondary_count 2..18 + REQUIRED timestamp sanity + reserved zeros; FUN_180029280:34562-34564: stream requires data_length!=0 && first_cluster!=0 && name_length!=0 && next-is-NAME).
- N-002 mechanism CORRECTED: the :8030-8048 "EOC families" are FAT[0]/FAT[1] table-signature probes (lost-FAT discovery + CFAT32Table::validate:53853-53862), NOT chain termination. Real FAT32 semantics: CFAT32::Next FUN_1800419e0:53955-53962 returns the RAW unmasked u32; the dir-walk bound `while (c < 0x0FFFFFF8)` (:11231) ends chains cleanly for all high-bit-dirty EOC forms that core's &0x0FFFFFFF mask turns into bogus pointers -> broken. Observable divergence (0x27FFFFF8: og clean end, core broken) CONFIRMED; corpus evidence in the finding file must be replaced.
- I-002 CONFIRMED verbatim (CFAT12::Next FUN_1800416f0:53798-53804: 0xFFF->0xFFFFFFFF, >0xFF7->0x0FFFFFF8, 0xFF7 raw); FAT16 BAD canonicalized (FUN_180041c80:54135). Stream-gate polarity re-confirmed (FUN_180003090:1857-1859); EXFAT-2 assumed-contiguous re-confirmed verbatim (FUN_180021c60:28158-28163).
- New divergences filed in the session report: exFAT FAT >32 MiB silently dropped by core (1-cluster truncation) vs og 1 MiB windowed FAT accessor of unbounded size; og FAT32 table window 128 KiB..300 MiB vs core 16 MiB hard reject (scan failure); og flavor boundary 65524->FAT32 vs core FAT16; og requires "FAT16   "/"FAT32   " FS-type string + bps==device-sector; og CFAT16 strict-canonical FAT[0]==0xFFFFFFF8 + whole-table cross-link rejection; og exFAT empty-file (data_length==0) rejection; exFAT tz offset absent from og engine (core applies it); full details + falsifiable claims in the S2-B report; scratch evidence sheet at /home/z/my-project/scratch/S2-B-context.md.
- NOT-FOUND (searched, absent): LFN checksum ROR8 accumulator in FATSearchDLL; deleted-SFN first-char substitution char; exFAT name_hash reads (only FNV-1a 0xcbf29ce484222325 :13667 internal map keying); tz math in exFAT DLL; boot checksum32 in exFATSearchDLL; og sector-6 backup-boot mount fallback.
Stage Summary:
- OQ-T2 and OQ-T5 both RESOLVED with exact quotes; both contradict session-1 priors in part (OQ-T2: no whole-copy validation — per-entry with scrutiny, no BAD trigger; OQ-T5: no skip-set, fixer-only).
- N-003's corpus claim refuted (checksum exists, dual-form, deleted-tolerant, used for torn-set salvage + parked-set reconstruction — richer than both session-1 and core); N-002's conclusion survives with corrected mechanism; I-002 and the stream-gate/exFAT-2 settle verified verbatim.
- ~13 new divergence candidates identified (1 SEVERE-candidate exFAT huge-FAT truncation, several MAJOR around table windows/mirror mode/structural gates, rest MINOR), each with corpus FUN+line quotes and core file:line cites; read-only compliance held (only this worklog entry + scratch writes).
---
Task ID: S2-D2
Agent: carve/RawFixer/MO decompile reader (session-2 parallel read, read-only)
Task: Exhaustive RAWSearchDLL carve-engine read (dispatch OQ-T8, signature sets S-007, ZIP N-008), RawFixer.dll full repair-set enumeration (S-008), MO layer sweep vs cr_scan.cpp
Work Log:
- Read context first: diff-to-fix/modules/{carve,rawfixer,orchestration}.md, findings S-007/S-008/N-008+M-005, OQ-T8-T9 file, PARAMETERS.md, SearchDLL artifacts (census, dual-registry, carving deep-dive, family architecture), PartitionRecovery rawfixer artifacts, MOM-modules matrix; core side cr_carve.cpp/cr_registry.cpp/cr_analysers.cpp/cr_rawsigns_data.cpp/cr_rawfixer.cpp/cr_scan.cpp at e93decf.
- OQ-T8 RESOLVED (decompile): block scan = FUN_18000c820 ScanBlock(state, start_sector, sector_count, bitmap*), vtable+0x98 (CellTest FUN_18000c650 calls (this,start,count,0)). 4 MiB blocks (FUN_18000c430:9269-9287 `*(int*)(this+200)=0x400000/ss`; 8 MiB double buffer +0x1000, :9293-9296; ctor ss=0x200 FUN_18000bba0:8881). PROBING IS SECTOR-ALIGNED ONLY: candidate = buffer+half+ss*sector_index (:9496), loop :9616-9618; overlap via post-block memmove(buf, buf+half, half) (:9620-9622); progress = 1 callback per block (vtable+0x40, :9623-9627). Dispatch FUN_18000cf60 order: config gate ini("config/config.ini", only_common) (:9766-9842; absent in shipped config -> maps ACTIVE) -> (1) u32 fourcc EXACT map :9843-9885 -> (2) u16 multimap equal_range FUN_18000d3e0 :9932-10063 (skipped when first u32==0; tries every analyser node with that exact u16 key) -> (3) full LIST sweep :9894-9920 (every analyser quick-check, accept needs computed size). FIRST-ACCEPT-WINS at every layer; per-key two-generation chains (modern -> legacy File*Analyser, e.g. FUN_1800304f0 MP3Analyzer+FileMP3Analyser). Refutes the corpus artifact's "u16-first (cheap)" inference. Core: byte-stepped outer loop (cr_carve.cpp:78) + first-byte dispatch + kTrialOrder first-VALIDATED-wins (:83-183) - detection superset (core finds non-sector-aligned starts the og never probes) at 512x probe count.
- M-005 og-side CONFIRMED at decompile level: bitmap gate :9446-9460 - when a bitmap object is supplied (param_4 non-null, +0x1d==0), cluster=(start_sector+i)/spc; FUN_180058df0 (:67842-67856, `*(byte*)(base+bit>>3) & 1<<(bit&7)` with bias[10]/bound[2]) returns the allocation bit; used cluster + cluster-aligned -> `local_1e0 += spc-1` skips the WHOLE cluster and suppresses dispatch; mid-cluster positions in used clusters also suppressed. Core run_carve_stage (cr_scan.cpp:1040-1075, carve_slice at :1062) still carves the whole slice with no bitmap (line refs drifted from session-1's 482-492 to 1040-1075 via concurrent fixes - M-005 unchanged).
- S-007 verified BOTH directions: parsed og-codebase/raw_signs.json (139 exts - NO gif/bmp/rtf/doc/xls/ppt/mp3/aac/mid/dwg/chm/au/midi/ape/dbf/eps/dxf/jp2/mobi/dmg/mpg/m1v/mpeg); extracted og registry keys from the builders (u32: GIF8 :2009, BMP-u16 :2898, RTF :2087, OLE :2341, DWG :1997, MPEG-PS :1843/:1851, APE :1948, DXF :1936/:2732, MThd :2031, .snd :2473, ITSF :2497, dBase :1985, CAB :2449, EBML :2485, FORM :2413, ICO :2377-2389, HLP :2401, WPD :2258, WMF/EMF :2270-2282, TTF :2294, !BDN :2328, LowDoc :2353/:2365, MRW :1751/:2508, X3F :1803, PSP :2109, wpg :2189, Dbx :2203-2231, Stuf :2153, LZH :2175, OGG :1873, RMF :1896, MXF :1910, ASF :1922, XML/SVG :1811-1835; u16: "ID"->MP3 :2556, MPEG sync family ->MP3Analyzer :2565-2721, 0xFFF1/0xF9FF->ADTS :2761/:2775, "FL"->FLV :2840, <h/<H/<! ->HTML :2849-2875, FF FE->UTF16-HTML :2876, MM->3DS :2885, BM->BMP :2898, MS->MNY :2909, 1F8B->GZ :2796, 00 01->MDB :2818, 0x0000->MovAnalyzer :2746; LIST-only via FUN_180004fb0: SWF/LZH/MTS/M2TS/EMF/TAR/MOV/HEIC/VCF/M1V/JP2/MOBI/POS/OSP/HFP/HIK/7ZIP/GPX/AEP/3GP/AIF/AAC/FCPEVENT). ALL 8 session-1 claimed formats CONFIRMED + ~26 additional niche gaps session-1 missed (SWF, TAR, LZH, WPG, PSP, 3DS, MRW, X3F, HLP, ICO/CUR, WMF/EMF, TTF, WPD, MNY, MDB, MTS/M2TS, 3GP, AIF, AEP, GPX, VCF, HIK/POS/OSP/HFP, FCPEVENT, RM/RMVB, OGG, MXF, HTML/XML/SVG, gzip, StuffIt). Core superset: ISO9660 CD001 absent from RAWSearchDLL entirely (no 0x30304443 key, no ISO analyser) - core FMT_ISO is an og-lacking format. Core raw-signs layer = true 139-format mirror (cr_rawsigns_data.cpp:14-65).
- S-008 verified + extended: RawFixer.dll (86,576 lines; NTFSFixer/FATFixer/ExFATFixer/IRawFixer vtables) - full 33-method set re-verified from trace strings (NTFS CheckMFT :67805, IsValidRootDir :68068, FixFromDBR :65974; FAT CheckBackupDBR :60505, CheckFatTable :60891, CheckRootDir :61228; exFAT CheckDBR :9695, CheckSector :9853, FixFromDBR :9951, CheckRoot :10168, ReadCluster :10297, ChecksumDBR :10461, CopySectors :10637, FixDBR :10774, CheckMustBeZero :10912, TryFixFromSectorsPerCluster :11201/:11223, UpdateDBR :11322). CheckMFT spec-exact record-size math (:67878-67887 `if (cpr<0) rec=1<<(-cpr&0x1f); else rec=cpr*bps`) + 0xFFFFFFFF abort sentinel. CheckFatTable = FAT1 sanity -> FAT2 mirror restore WRITTEN TO DISK (:60968-60981, vtable+0x10 WriteSector). exFAT UpdateDBR builds a 24-sector image (11-sector template + fixed sector-0) + fill-repeated fresh checksum (:11429-11434), read-back verify (:11440), then WRITES 12 sectors at LBA 0 AND 12 at LBA 12 (:11450-11466) - og rewrites BOTH main and backup regions from a template; core patches only primary spans. Core (cr_rawfixer.cpp, 559 LOC): NTFS volume-end backup :211-231, exFAT backup-12+checksum+SPC-shift :234-399, FAT backup-6 :412-428, transactional execute/undo :440-555; still lacks CheckMFT/FixMFT, CheckFatTable, CheckRootDir/IsValidRootDir, CheckCluster, FixByScanSector, ReadCluster verify; still NOT wired pre-scan (cr_scan.cpp grep: zero repair references; rrecov.cpp:1388/:1431 manual CLI only). Consent: no user prompt inside RawFixer.dll; invoked pre-scan from DRW check-phase flows per rawfixer-fs-repair.md.
- OQ-T5 concur (S2-B beat me by minutes, same result): FUN_18000e7e0 :9353-9381 ROR+add over ALL bps*0xb bytes, NO skip-set; the in-loop FUN_180023220 calls are exception_ptr plumbing (always-true), not cancellation. Core cr_exfat_dentry.cpp:26-37 skips 106/107/112 (spec+Linux) - checksum values written/validated by the repair layer DIVERGE from the og's on any volume with nonzero VolumeFlags/PercentInUse; impact confined to repair+exfat_boot_sane paths (scanner non-gating per S2-B).
- N-008 verified with an important CORRECTION: the og's quick gate FUN_1800176b0 :18288-18296 is version_needed <=0x13 OR ==0x62 (confirmed); but the full walk FUN_1800176e0 gates a FIRST-record LFH at version <= 10 + namelen 1..259 (:18389-18399), accepts PK\x01\x02 and PK\x05\x06 ungated. The padding loop (:18480-18500) is INVERTED vs the core: og aligns the chain end DOWN to sector, starts extension only when the partial sector's tail is NONZERO (and file <= ~4 GiB via global 200000000*0x14), then advances sector-by-sector BREAKING at the first sector whose trailing 16 bytes are all zero (cap `uVar15 < 0x500001`) - the og ABSORBS NON-EMPTY sectors after a broken chain and never inflates a cleanly-ended archive; core cr_analysers.cpp:641-655 absorbs ZERO 16-byte blocks after the EOCD end (cap end+5,242,880) - inflates intact ZIPs with zero slack and provides no broken-chain salvage extension. Core ZIP64 indexless break at :602-603 (`csize==0xFFFFFFFF -> break`) verified; og chain-walk shows no such gate at the dispatch level (record parser FUN_180045690 decides) - PARTIALLY verified.
- Per-format spot-verifies: JPEG marker walk :39600-39675 (marker guard `0x3e < (byte)(m+0x40)`, SOF0 `(m==0xc0)&&(prec==8)` :39653, dims<20000, Exif 0x66697845); MP3 :37130-37205 ID3 major-in-{3,4} gate + 2-consecutive-frame headerless sync check (0xFF, b1&0xE0==0xE0, b2&0xF0!=0xF0, frame < 0x56220) - core parity on both (cr_analysers.cpp:1210-1214 ID3 2.3/2.4; :1287 >=2 frames); MP4 brand list :32852-32867 (15 ftyp brands, _memicmp) + moov/pnot/free atoms.
- MO layer: 120-file sweep (Thread.mo = CCriticalSection/CEvent/CThread wrappers; CallbackOperator.mo = event operators; DeviceManager.mo = enumerator hub importing 6 modules; CMessageProxy/CMessageSend thread-affinity Qt marshaling per the artifact). NEW deltas beyond orchestration.md: og 4 MiB read blocks + 8 MiB double buffer vs core 1 MiB window + 4 KiB overlap (X51); per-block progress cadence; per-block cancel poll via global enable object (FUN_1800424b0, :9439-9441); config-driven only_common dispatch-narrowing gate + RawConfig.ini [Filter] per-extension toggles (GetPrivateProfileSectionA "Filter", :13121+; shipped: STL=0, VPJ=0); EUVideoRepair.dll wired INTO the carve engine (FUN_18000beb0 :9210-9237 loads GetIMovMp4Repair; carved MOV/MP4 feed GetVideoInfo/repair-check at FUN_18000d670 :10130-10136) - core has no video-repair hook.
- Corpus-errata candidates: dual-registry artifact's 0x4D4D "MM = TIFF big-endian" is wrong (decompile: MM -> RawSpeed::New3DSAnalyser->File3DSAnalyser :2885-2888; TIFF BE rides u32 0x2a004d4d); "0x4449 ID = AIFF/IANA" is wrong (ID -> MP3Analyzer :2556); census's "0xd302020 = MPEG audio frame sync" is wrong ( -> BruteForce::CDXFAnalyser :1936, DXF "  0\r" leading).
- NOT-FOUND (searched): raw_signs.json consumption site within the block scan (loader FUN_1800098d0 :23836 + JSON parse traced; the sweep consumer not isolated - probing stride for that layer unverified, likely the same sector-aligned block loop); user-consent prompt for repair inside RawFixer.dll; ISO9660 anywhere in RAWSearchDLL; og byte-granular magic probing anywhere in the block loop.
- Read-only compliance: no file in either repo modified; only scratch/S2-D2-context.md and this worklog entry written; no build run; no tokens.
Stage Summary:
- OQ-T8 RESOLVED: og dispatch = sector-aligned probing (512-stride, never byte-stepped) through a 3-layer first-accept-wins pipeline (u32 exact map -> u16 exact-key multimap -> full list sweep), two-generation fallback chains, 4 MiB blocks/8 MiB double buffer, per-block progress+cancel, bitmap-gated cluster skipping on volume scans. Core = byte-stepped, specificity-ordered, first-validated-wins, whole-slice - a detection superset with different cost shape; collision resolution equivalent for shared formats (no og cross-layer ambiguity exists because each layer sees disjoint keys).
- S-007 payload: 8/8 common formats confirmed missing from core (GIF/BMP/headerless-MP3/AAC-ADTS/RTF/OLE/MPEG-video/DWG, all with exact og keys/lines) + ~26 additional niche formats (SWF, TAR, LZH, WPG, PSP, 3DS, MRW, X3F, HLP, ICO/CUR, WMF/EMF, TTF, WPD, MNY, MDB, MTS/M2TS, 3GP, AIF, AEP, GPX, VCF, HIK/POS/OSP/HFP, FCPEVENT, RM/RMVB, OGG, MXF, HTML/XML/SVG, gzip, StuffIt, JP2, MOBI, CHM, AU, MIDI, APE, DBF, EPS, DXF, DMG); reverse direction: core-only ISO9660 (informational superset).
- S-008 payload: full 33-method table re-verified with decompile line cites + 2 new og behaviors the fix plan must add (backup-region rewrite + extended-boot template in UpdateDBR; FAT2->FAT1 disk-write mirror). Core still boot-sector-only and unwired pre-scan (verified at current line numbers).
- N-008 payload: version gates (<=0x13/==0x62 quick; <=10 first-record walk), padding loop quoted with INVERTED semantics vs core (og absorbs non-empty sectors post-broken-chain, never inflates clean archives; core absorbs zero slack post-EOCD), cap/alignment deltas confirmed, ZIP64 core-break confirmed (og-side partially verified).
- New divergence candidates filed: og pre-scan video-repair wiring (MAJOR), exFAT checksum predicate full-hash vs spec-skip (MINOR, OQ-T5 substance), ZIP padding semantic inversion (sharpens N-008), only_common + RawConfig.ini runtime format gating (MINOR), carved-name Carved% vs File% split in core vs og (MINOR), 4 MiB block I/O shape (INFORMATIONAL), sector-aligned vs byte-stepped probing (INFORMATIONAL superset + X47 note), corpus-errata items for 3 artifact claims.
---
Task ID: S2-C2
Agent: Btrfs/ZFS/ReFS decompile exhaustive reader (session-2 parallel read, read-only)
Task: Exhaustive decompile read of BtrfsSearchDLL/ZfsSearchDLL/ReFSSearchDLL vs core src/core/{btrfs,zfs,refs}; resolve OQ-T6 (ZFS anchors/uberblock/multi-vdev) and ReFS discovery anchors; systematic sweep of detection, chunk/DVA mapping, walks, names, grading, error semantics.
Work Log:
- Read context: modules/{btrfs,zfs,refs}.md, findings I-003/N-004..N-006/N-007+I-004/S-005, OQ-T6-T7 file, PARAMETERS.md; core engines at e93decf (cr_zfs_engine.cpp 1361 LOC, cr_btrfs_engine.cpp 804, cr_refs_engine.cpp 927 + cr_refs_detect.cpp 84); corpus at 03ad46c.
- OQ-T6 RESOLVED by decoding FUN_180018220 signature (seek target = arg3, vector RESIZE to arg4; proof :20985-20988 seek(dev,0x4000,0) ret==0x4000 + FUN_18000b930 resize semantics :9647-9649): og label nvlist read = 112 KiB (0x1C000) AT OFFSET 0x4000 (:21052, parse from +4 limit 0x1BFFC :21053-21054, 8-byte XDR header skip :21259); uberblock ring read = 128 KiB at 0x20000 (:21365). Session-1/OQ-T6/artifact zfs-vdev--mftmirr--calcpartition.md:13 had the args swapped ("offset 0x1C000 size 0x4000") - corpus errata candidate. Core matches og exactly (VDEV_PHYS_OFFSET=16K cr_zfs_engine.cpp:51, UBRING_OFFSET=128K :53) - OQ-T6.1 resolves as MATCH, no divergence. Og probes ONLY the front window [0x4000,0x40000) of the single scanned disk (no 0x40000/0x80000 constants anywhere; no L1/L2/L3); core probes all 4 labels + ashift shifts 10..13 (:1000-1009,:1066) - core superset (I-003-pattern), INFORMATIONAL.
- OQ-T6.2 multi-vdev RESOLVED: og extracts ONLY "vdev_tree" (type-9, memcmp :21089-21090) and inside it ONLY "id" (bytes 'i','d' :21114-21117, BE-accumulate :21138-21141) - NO version/name/pool_guid/ashift reads anywhere (only quoted strings in DLL are vdev_tree/root_dataset/ZPL_*/SA_*/LAYOUTS/REGISTRY). Type-0x13 subtrees parsed then destroyed unused (:21171,:21184-21202). DVA vdev id = (w0>>32)&0xFFFFFF (:18025-18028) -> registry lookup FUN_1800168b0 (:19471) with **first-entry fallback** when unmatched (:19544-19556) - og NEVER refuses a vdev id; read offset = (dva_w1+0x2000)*0x200 (:17996) = DVA offset + 4 MiB vdev skip. Consequence: mirror members recover (vdev id 0 = local copy), stripe members partially recover (member-relative offsets), raidz fails checksums. Core REFUSES vdev!=0 (cr_zfs_engine.cpp:408-410). DIVERGENCE (candidate D-09 upgrade).
- OQ-T6.3/6.4: uberblock magic 0xbab10c LE, txg@slot+0x10, newest-txg-wins insertion sort (:32321,:32365), fixed 1 KiB slots; checksum depth: fletcher4 4-accumulator verify :18093-18117 (gate types {7,0} :18076), 32-byte memcmp, on fail retry next DVA copy (:18129) else drop (:18139); no sha256/fletcher2 constants; else-branch = BPE payload assembly minus words 6/10 (:18363-18375) = core bpe_payload parity. CORE DOES NO CHECKSUM VERIFY AND NO DVA-COPY RETRY (all reads read_block(bp,0,...) :489/:533/:584/:591/:602/:961) - modules/zfs.md B3 "checksum-aware reads" is WRONG for core; new MAJOR divergence.
- S-005 verification: og ZfsSearchDLL converts ZPL names via MultiByteToWideChar(0xFDE9=CP_UTF8) :24985/:25014 (FUN_18001c630, SA-attr accessor) - core utf8_to_u16 = og parity CONFIRMED. BtrfsSearchDLL has NO conversion imports - S-005's btrfs-side og anchor NOT-FOUND in that DLL (names leave as bytes; honest note).
- ReFS: DBR gate FUN_180012390 :13529-13531 = "ReFS"@+3 + "FSRS"@0x10 + major==3 + minor>3 (3.4+ ONLY; the major==1 branch :13547-13550 is DEAD under the outer gate - refines I-004's "og gates 3.4+/1.x"); bps in {256..4096} :13544-13545, cluster gate spc*bps in {0x1000,0x10000} ONLY :13544-13546; volume open reads 4 KiB at 0 (:2425), metadata base = spc*30 sectors (:2460, cluster 30), second copy via ((pos/unit)>>1)-2 mid-volume formula (:13244,:2360); minor-class dispatch 3/7/8/9 (:2466-2476). Page scan FUN_180008620: 512-byte steps, identification/DBR or SUPB 0x42505553/CHKP 0x504B4843 (:5855-5870) - core constants MATCH (cr_refs_engine.cpp:28-29); signature dispatch FUN_18000fc80 first-DWORD compare :11701 (N-007 confirmed; core full-memcmp cr_refs_detect.cpp:39-50). Checkpoint FUN_180012650: DYNAMIC body offset uVar14=*(uint*)(pagehdr+4) (:13743; =0x20+ref next per FUN_18000fc80 :11688/:11717) vs core HARDCODED page+0x50/0x54/0x56 (cr_refs_engine.cpp:546-547,:567) - parallel-agent claim CONFIRMED as real gap (GAP-S6-3 fixed the minor-gate source, not the dynamic body base); version gate body+4==3 && body+6>3 (:13744-13745) = core :546-550 MATCH; body size 0x44/0x58 by minor<0xE (:13754-13757) = core :567 MATCH; checkpoint newer-clock by obj[2] (:1029) = core :523-524 MATCH. Core superblock probes cluster 30 then TAIL n-3/n-2 (:461-479) vs og cluster-30 + MIDDLE-2 - second-region location divergence candidate. ReFS deleted/allocator: NO allocator classes in ReFSSearchDLL (class census: Dbr/SupperBlock/CheckPoint/MSTreeHeaderDesc/NodeRecord/DirectoryValues/FileValues/... only) - og deleted-grading machinery NOT in SearchDLL (stays .mo-side, modules/refs.md B4 stance holds). No volume-label code in the DLL (NOT-FOUND).
- Btrfs: SB scan FUN_18000d540 quoted in full - device min 0x11000 (:10283), mirror table DAT_180018d60 null-terminated, magic u64@+0x40 :10298, sectorsize pow2 :10299-10300, nodesize band (ns-0x86)<0xFF7B :10301 = [0x86,0x10000] INCLUSIVE (N-006 doc drift: session-1 wrote [0x86,0x86+0xFF7B); both engines accept 0x10000, so pow2+>=sectorsize remain the only deltas), ns%ss==0 :10302, newest-generation :10303-10304, ABORT rule `if (lVar5==0) break` :10311-10312 (I-003 exact quote re-confirmed; gate-fail on mirror0 continues, magic-miss aborts). Chunk parse: type bits DATA/SYSTEM/METADATA trackers :2148-2160; RAID10 sub_stripes sanity :2162-2168; profile decode RAID0/1/DUP/10 :2498-2529 with ROTATING mirror index (idx+1)%num_stripes :2534-2537 for 2-copy profiles (DUP copy rotation = real damaged-copy salvage; core parse_chunk uses stripe-0 only :199-207); error-map record sector-granular FUN_180015400 :2544-2550 (D14 og evidence). Default-subvolume resolution in og (name hash 0x8dbfc2d2 :3975, "default" :4016, DIR_ITEM 0x84 :4019, root-5 fallback :4060) vs core synthetic subvol names, no default pick (:569) - B5/A26 candidate. fs walk key types 0x18 (dir-item-shaped >=0x1E) and 0x6c EXTENT_DATA (type byte @item+0x14, REG/PREALLOC/INLINE) :3782-3826 vs core KEY set {1,108,84,132,228} - core walks INODE_ITEM which the og quick path does not surface here. NO crc32c constants in og btrfs (neither engine validates node checksums - modules/btrfs.md B3 phrasing inaccurate for core; behavior MATCH). BtrfsCache.txt mapped-file cache :1991 confirmed (I-003 note).
- Grading (D15): no Confidence/grade classes in any of the three og SearchDLLs (NOT-FOUND); og ZFS has ZfsDecider::addBlock grading blocks by from_txg/birth_txg (:2163-2165, :2607-2616) - txg-based block-liveness decider, partially decoded; og Btrfs records read errors into a sector-granular error map. Core: Confidence enum cr_node.h:39-45; only ReFS engine grades (:776-777,:844,:869); ZFS/Btrfs emit no grades. No per-engine og anchor exists in these DLLs to compare - D15 stays engine-side UNVERIFIED beyond ZfsDecider.
- Error semantics: og ZFS = checksum-fail -> DVA copy retry -> silent block drop; og Btrfs = per-read error-map + node skip; og ReFS = invalid page skip / open fail. Core = ERR_* at entry, damaged counters. FS codes: ZFS og 0xd=13 (:5533,:15223) = core FsKindCode::Zfs=13 (cr_types.h:92) MATCH; Refs 10 = core 10 MATCH.
- NOT-FOUND (searched, honestly): og ZFS uberblock version/timestamp tiebreak; sha256/fletcher2 in ZfsSearchDLL; ZAP magic 0x2F52AB as immediate in ZfsSearchDLL (core gates on it, cr_zfs_engine.cpp:13); LZJB/ZLE/GZIP decoders in ZfsSearchDLL (only LZ4-with-BE-length :17870-17876 + passthrough); btrfs DIR_INDEX(0x60)/INODE_ITEM(1) handling sites beyond EXTENT_DATA/0x18/default-dir-item; ReFS volume label + integrity streams + container/ObjectId classes in ReFSSearchDLL (.mo side); btrfs nlink/dir-index and timestamps emission sites; the meaning of og ZFS quick-probe second magic 0x210da7ab10c7a11 at slot+0x1d8 (:15070) and the 0x100000 vtable+0x78 call (:5512).
- Read-only compliance: no file in either repo modified; only scratch/S2-C2-context.md written and this worklog entry appended; no build run; no tokens.
Stage Summary:
- OQ-T6 RESOLVED with quotes: (1) og anchor = front-window fixed offsets nvlist@0x4000 (112 KiB) + uberring@0x20000 (128 KiB), single label region, single disk; core = 4 labels + ashift probe = superset; core anchor offsets MATCH og exactly (session-1's "og reads 0x1C000" was a swapped-args misreading - corpus errata). (2) nvlist anchor = vdev_tree/"id" ONLY (no version/name/pool_guid). (3) uberblock ring enumerated, magic-gated, newest-txg-wins, fixed 1 KiB slots, no checksum. (4) multi-vdev: og NEVER refuses - registry lookup with first-entry fallback + DVA offset+4MiB => mirror members fully recoverable, stripe members partially, raidz dead; core refuses vdev!=0 => mirror/stripe members yield LESS than og (or nothing for stripes).
- New divergence candidates (reported to lead, numbered in session report): Z-1 og-vs-core multi-vdev attempt-vs-refuse (SEVERE candidate, upgrades D-09/OQ-T6.2); Z-2 og fletcher4 verify + 3-copy DVA retry vs core no-verify/always-DVA0 (MAJOR, torn-block salvage + silent-garbage risk; modules/zfs.md B3 corrected); Z-3 LZJB/gzip pools likely unreadable by og (INFORMATIONAL, core codec superset); Z-4 4-label/ashift superset (INFORMATIONAL, I-003-pattern). R-1 ReFS checkpoint body offset: og dynamic 0x20+ref-next vs core hardcoded 0x50 (MINOR/edge MAJOR - GAP-S6-3 follow-up); R-2 ReFS 1.x/2.x og-gate refinement (major==1 branch DEAD; og is 3.4+-only - I-004 refinement); R-3 ReFS cluster/bps gates two-sided (og cluster 4K/64K only, bps 256..4096; core bps 512/4096, any pow2 cluster) MINOR; R-4 second-metadata-region mid-volume-minus-2 vs core tail n-3/n-2 (MINOR candidate); R-5 ReFS minor-class dispatch 3/7/8/9 (INFORMATIONAL). B-1 og default-subvolume resolution vs core synthetic-names (B5/A26 MINOR candidate); B-2 og chunk-profile machinery + DUP mirror rotation vs core stripe-0 (MINOR/MAJOR candidate - DUP damaged-copy salvage lost); B-3 og quick-path key-type set {0x18,0x6c} vs core {1,84,108,132,228} (B-tree walk scope difference - core superset); B-4 modules doc corrections (btrfs/refs B3 phrasing, N-006 band bound).
- MATCH confirmations (dozens, key ones): ZFS ub magic/txg/selection, DVA vdev bitfield, 4MiB skip, BPE word-exclusion layout, nvlist 0x40-stride type-9/2 model, root_dataset anchor, ZPL names CP_UTF8 (S-005 fix = og parity), FS codes 13/10; Btrfs SB magic/mirrors/newest-gen/min-size/nodesize-mod/generation, I-003 abort rule exact-quote, BtrfsCache.txt, no-crc32c-in-either; ReFS page signatures, SUPB/CHKP constants, ref field offsets +0x2C/+0x30/+0x38, checkpoint newer-clock, body-version gate 3.4+, minor>=14 0x44/0x58 list offset, cluster-30 anchor, first-DWORD-vs-full-memcmp (N-007 both sides), I-004 superset direction.
- Unresolved/honest gaps: 0x210da7ab10c7a11 probe magic, ZfsDecider full grading semantics, og btrfs timestamps/nlink emission, ReFS volume label source, og ZFS pool display-name source (label "name" never read; "$ORIGIN" string near vdev_tree unexplained), whether og btrfs fs walk also emits INODE_ITEM-based metadata elsewhere in the DLL.
---
Task ID: S2-A2
Agent: NTFS decompile exhaustive reader, relaunch (session-2 parallel read, read-only)
Task: Exhaustive NTFS decompile re-read (NTFSSearchDLL ground truth) resolving 5 top-priority parity questions (viability granularity, flags 0x10, record-size ladder, MFTMirr count, deleted-dir destroy) + systematic sweep vs core src/core/ntfs.
Work Log:
- Read context: diff-to-fix/modules/ntfs.md, findings S-001/M-001/N-001/I-001, PARAMETERS.md, evidence/decompiled-quotes/ntfs-viability-and-filters.md.
- Full re-read of FUN_18001acc0 (:21095-21302) + all helpers: FUN_1800340b0/150/1c0 (runlist iterator :41481-41544), FUN_180033aa0 ctor (:41171), FUN_1800347d0 runlist decoder (:41853-41970), FUN_18002c2e0 bit test (:35643), FUN_180017190 claimer (:18323-18418), FUN_180017cf0 main walk (:18875-18997), FUN_180017390 record processor (:18424-18636), FUN_18001a980 pass-2 driver (:20957-21069), FUN_180006db0 props claim (:5221-5437), FUN_1800069e0 $DATA walker (:5029-5215), FUN_180043700 dir-flag (:53481), FUN_1800415a0/1940 in-use/dir record flags (:52244/:52402), fixup chain FUN_180041bb0/1f0/290/370/a20 (:52119-52543), name conversion FUN_180006610/66f0 (:4860-4906), INDX walker FUN_180046530/FUN_180045510 (:55342/:54704), $Extend INDX lookup (:19575-19649), metafile 15-name table (:52006), recycle-GUID list append (:18584-18625).
- Cross-tree verification: ghidra-local NTFSSearchDLL_decompiled.c (sign constants), retdec tree (inconclusive on switch), rabin2 strings (GUID, metafile names).
- ghidra-mo NTFSLib: CNtfsVolume::LoadNtfsVolume boot/record-size ladder (:19292-19344), CNtfsMFT::InitMFT gates (:17232-17300), InitMFTMirror mirror capacity + ReadFileRecord gate (:17300-17533).
- Core comparison reads: cr_bitmap.cpp (reused_ratio :94-104), cr_mftwalk.cpp (:249-261 metafile list, :430-448 corruption policy, :620-749 grading/destroy/recycle), cr_bootstrap.cpp (:30-66 record-size, :254-306 mirror), cr_ntfs_wire.h (:208-217 record_size_from_dbr), cr_fixup.cpp, cr_runlist.cpp.
- Scratch sheet: /home/z/my-project/scratch/S2-A2-context.md (full quotes + offsets).
Stage Summary:
- Q1: og viability = PER-RUNLIST-ENTRY accounting, ONE bitmap probe at each run's START sector (S2-E confirmed; session-1 summary's "per-cluster" and core's per-cluster loop diverge; new SEVERE candidate). A deleted file with all run STARTs re-allocated but free interiors: og DESTROYS, core keeps+grades.
- Q2: flags bit 0x10 = DIRECTORY ($FILE_NAME flags 0x10000000 via FUN_180043700); deep-dive.md "in-use" reading confirmed WRONG, algorithms-v2 "not a folder" RIGHT.
- Q3: M-001's cited og ladder (:21130-21137) is actually bytes-per-sector acquisition (fallback 0x200=512) + 0x400 parse-buffer floor; the REAL record-size ladder is NTFSLib LoadNtfsVolume: cpr==0/0-result -> volume REJECTED ("It is not a NTFS volume!"), non-pow2 positive cpr ACCEPTED, no 0x400 clamp. M-001 gap stands for non-pow2; its "0 -> 0x200 fallback" fix step would itself diverge from og.
- Q4: og mirror capacity = runlist-derived (this+0x70 = (last_run.lcn+len)*spc*bps/record_size; ReadFileRecord gate param_1 <= capacity) - NOT 4; core record_no<4 divergence confirmed (N-001 verified at decompile level).
- Q5: directories CANNOT reach the destroy branch - (flags&0x10)!=0 routes straight to emit; viability runlist = $DATA (0x80) only (FUN_1800069e0); core !is_dir gate = parity. Also: FUN_18001acc0 is the lost-file/MFT-copy pass (fed by GUID-named deleted files, engine[0x65] list), NOT the main walk - main-walk deleted files get an 8 KiB content sample (FUN_18001e6c0) and are emitted ungated; S-001 pass attribution needs re-verification.
- New divergence candidates filed: per-entry probe (SEVERE), small-file (<=0x3FF) fall-through destroy in that pass (MAJOR), record-size ladder corrections (MAJOR), runlist 3/5/6/7-byte sign-adjust constants nonstandard og-side (MAJOR, core correct), pass-2-restricted destroy (tracked-open), 0x400 fixed stride on 512B-record volumes (MINOR), UTF-8/ACP name storage (INFO), INDX walk exists for $Extend lookup only (INFO).
- Verified matches: 0x3ff strict gate, folder gate, fixup 2-tail tolerance + apply-on-copy equivalence, skip-record corruption policy, extension-record gate, $-skip/system-flag/name filters, SI-over-FN timestamps, $DATA-authoritative sizes, runlist nibble order + sparse -1 encoding, TS/name/size field maps, LZNT1 presence, remnant 512-stride + 0x58/0x60 salvage.
- Honest gaps: USN $J harvest path not re-read; og primary claim-map provenance (engine[0x4a] wiring) unresolved; hardlink multi-FN merge depth partial; ADS naming format and deleted-dir orphan chains NOT-FOUND in budget; short-name field NOT-FOUND in NTFSSearchDLL (S2-F claim likely other DLL).
---
Task ID: S2-G1
Agent: NTFS pass-1 deleted-fate decoder (session-2 parallel read, read-only)
Task: Decode FUN_18001e6c0 (pass-1 deleted-record fate), pass-1 emit semantics, and N-001 $MFTMirr record count from the og decompiles.
Work Log:
- Read scratch/S2-A2-context.md and findings/SEVERE/S-001 (finding + Resolution) for orientation; no redo of pass-2 analysis.
- Decoded FUN_18001e6c0 (:24022-24168) end-to-end incl. helpers FUN_18001d5f0/d690/d960/db20/dd80/e120/e340, FUN_180002670/680, FUN_180003250, FUN_180020bd0, FUN_180033aa0 (CNone/CResident runlist classes), FUN_1800347d0 (runlist parser), FUN_180034660.
- Re-verified the call gate at :18492-18500: it is SIZE==0 (vt slot 3 = GetSize, confirmed against :42339-42), NOT directory (FUN_180041940), state bit0=deleted; FUN_18001e6c0 = extent/resident-content RESCUE (steal runlist+size from same-named collector records, extract resident $DATA from live record and MFT-copy sector list, parse extracted bytes as runlist and adopt under struct+0x70 cluster cap, register name in struct map, set props+0x72 |= 2/4). No claimed-map probe, no viability, no destroy, no drop.
- Traced pass-1 emit: :18630 vt[0x10] fires for every record passing the loop gates (fixup/base/metafile :18952-55, in-fn metafile E_FAIL :18567); deleted/live NOT filtered; no overwrite grading exists in pass 1. Established state(+0x3d) bit0 = deleted (FUN_180006db0 :5285/:5380/:5426 via ctx+0x58; GUID gate :18584 is bit0 CLEAR = live GUID files — S2-A2 polarity corrected), pass-2 emit ORs bit0 (:21275), value 8 only on the "File Path Lost" synthetic folder (:44735).
- Identified the engine as CNTFSQuickSearchBase (:17431); the e6c0 "class" = sub-struct at engine qword 0x3b (byte 0x1d8), collector = engine qword 0x42, consumed post-pass-2 by FUN_180016440 → FUN_18001f9c0 → re-emit code 0xfe0001; $I/$R pairing consumes +0x72 bit1 via FUN_18001c710 (:17633-39). Scan driver order mapped (:17964 pass1, :17983 claim copy, :17984 pass2, :18024 collector merge, :18025+ lost-file synthetics).
- N-001 (ghidra-mo NTFSLib): mirror capacity computed from $MFTMirr $DATA runlist LAST RUN (InitMFT :17407-13), read gate `param_1 <= this+0x70` INCLUSIVE (:17460), resident-$DATA abort (:17377-83); NOT hardcoded 4. Confirmed N-001 as written; new nuance: inclusive `<=` bound.
- Wrote scratch notes scratch/S2-G1-context.md. No files in either repo modified; no build run.
Stage Summary:
- FALSIFIABLE ANSWER: a fully-overwritten deleted file recovered from the LIVE MFT is PRESENT in the og result set (pass-1 emit :18630, state bit0=1, no grade). The og destroy rule fires ONLY in pass 2 (FUN_18001acc0, GUID-file MFT-dump records). Therefore the landed core rule destroy_fully_overwritten in the MAIN walk (cr_mftwalk.cpp:704) is MIS-SCOPED: the og's pass-1 never destroys; pre-fix keep-and-grade matched og pass-1 semantics; the og-equivalent destroy belongs to a (nonexistent) pass-2/GUID-dump analogue. S-001's core claim is true only for dump-sourced records.
- New divergence candidates: (1) core viability grading/destroy runs in main walk where og has none (cr_mftwalk.cpp:652-708 vs og pass-1); (2) og claim map (engine[0x4a]) claims DELETED size>0 extents too (FUN_180017190 gate on engine+0x258, likely 0) while core probes volume $Bitmap — pass-2 "intact" tests differ even on healthy volumes; (3) og e6c0 size-0 deleted rescue (MFT-copy resident-data → runlist adoption) has no core equivalent; (4) mirror inclusive `<=` capacity bound (og off-by-one) vs core `record_no < 4`.
- N-001 CONFIRMED: mirror record count = computed from mirror $DATA runlist extent (all spanned records), not hardcoded 4.
---
Task ID: S2-G2
Agent: EXT4/HFS+ decompile payload extractor (session-2 parallel read, read-only)
Task: Extract exact decompile-level semantics for HFS+ attributes-tree/decmpfs (S-002), HFS+ journal (S-003), ext4 journal (S-004) from the Ghidra corpus, plus a fast sweep of both engines' detection/walk gates; no files modified in either repo.
Work Log:
- Read S-002/S-003/S-004 findings + modules/{hfsplus,ext4}.md for session-1 claims and fix plans.
- HFS+ decompile: traced decmpfs end-to-end. FUN_180009940 (catalog record->node, :7662-7700): UF_COMPRESSED gate at record byte 41 (ownerFlags & 0x20); compressed files get data run built from the RESOURCE FORK (byte 168) or none when rsrc logicalSize==0 (inline); size := rsrc logicalSize. FUN_180009c10 (:7713-7859): builds keyLength=(17+7)*2=48 "com.apple.decmpfs" key, searches attributes fork (cnid 8) via FUN_18000c3a0, accepts recordType 0x10 (inline) only, copies logicalSize@+12/data@+16, then reads ONLY decmpfs header +8 (uncompressed length) into node.size and frees the payload — NO inflate (imports file has no zlib). Fallback map (volume+0x78) = journal-harvested deleted attribute records.
- HFS+ journal: CHFSPlusJournal ctor (ends :13579): VH.attributes&0x2000 gate, JIB read at blockSize*jibOffset, FUN_18000db70 BE-swap of {flags,offset@0x24,size@0x2c}, JIB.flags bit 0 required; journal header raw gates magic==0x4a4e4c78, endian==0x12345678, jhdr.size==JIB.size (big-endian PPC journals rejected). GetDeleteRecord FUN_180011300 (:13637-14252): walks ring from jhdr.start modulo JIB.size until back at start; per tx block-list header (max_blocks==blhdrsz/16-1), 16-byte entries {bnum,blksize}; node validity via FUN_180010720; LEAF-only; FUN_18000cb70 maps each block to catalog/extents/attributes/allocation/startup fork (with live extents-overflow chase); per record: search LIVE tree — absent/different => keep as DELETE record in volume maps +0x58(catalog)/+0x68(extents)/+0x78(attributes). FUN_18000c8f0 (:10017-10127) emits delete-records as nodes (deleted flag, grade 5), then traverses the live catalog tree as-is. Extents delete-map consumed by FUN_180009f40 data-run fallback (:7937-7982). NET: deleted-file salvage, NOT replay.
- ext4 journal: FUN_18000e5b0 (:12159-12260) pass 1: jsb gates bswap(magic)==0xC03B3998 + blocktype{3,4}; journal size = journal-inode i_size; 1 MiB windows; EVERY block from jsb.start fed to FUN_180005e50. FUN_180005e50 (:4340-4560) is a recognizer, not a dirent salvager: dir blocks ("."+ ".." leading dirents) are only registered in a map; other blocks scanned at inode-size strides for extent-header-magic (0xf30a) inode images -> S_IFDIR dirs registered by first block, non-dir inode images (0x40 bytes) copied into a salvage list. FUN_18000e890 pass 2 walks jsb.start..s_maxlen and for known blocks calls FUN_1800043f0 ("GotJournalDir"/"TraverseJournalDir") which binds dir inode->parent ("..") and per-dirent child inode->(parent,name) via FUN_1800068b0. Journal passes run in the MAIN scan (:2773/:2778), not only on damaged volumes.
- ext4 gates sweep: SB at +0x400 primary only; magic 0xEF53; s_feature_incompat bit 0 (COMPRESSION) must be clear (settles errata #7 in core's favor); inodes/blocks/bpg/ipg nonzero; rev-level/inode-size gate; blocksize 0x400<<(log&0x1F); 64-bit blocks via SB+0x150; groups math; no BIGALLOC gate found; no backup-SB probing. 10 MiB VirtualAlloc read buffer; 20 MiB content cap in binder.
- HFS+ gates sweep: VH gate :10005 (H+/HX, 512-align, version<0x11, alloc/extents fork clamps); collation dispatch byte 0xCF -> table-collated compare vs binary with ':'<->'/' fold; names via WideCharToMultiByte(CP_UTF8, default '_'); attribute names '/'->':' sanitized; result sink FUN_1800057d0 drops name[0]=='.' entries and the "HFS+ Private Data" dir; 'prdf'-fileType (0x66647270) records treated as folders.
- Core cites verified: cr_hfsplus_engine.cpp:43/:330-346/:356/:535-557; cr_ext4_engine.cpp:102-119/:141/:432-433/:593-595/:643-669/:670/:718-744/:746-794/:796-830.
- Wrote scratch/S2-G2-context.md with full quotes/line numbers. No build run; no repo files modified.
Stage Summary:
- S-002 payload CORRECTED: og recovers raw resource-fork bytes (type-4) or nothing (type-3 inline) + fixes displayed size from decmpfs header (magic/type/uncompressed-length); never decompresses (no zlib import). Core currently emits 0-byte files with size=0. Fix must implement: attributes-tree search with 48-byte key, recordType 0x10 gate, decmpfs header size fix, rsrc-fork data run, journal-fallback map; inflate = optional superset, not parity.
- S-003 payload CORRECTED: og journal = recently-DELETED harvester (catalog records -> deleted result entries; extents records -> data-run fallback; attribute records -> decmpfs size for deleted compressed files); no replay/overlay; live catalog walked as-is. BE journals (PPC) rejected. Fix plan should replace replay with delete-record salvage; S-003's "recency" direction is wrong.
- S-004 payload REFINED: og salvages EVERY journal block from jsb.start in 1 MiB windows, but as inode-image salvage (extent-magic 0xf30a gate) + dir-block registry + pass-2 dirent binding on dir blocks only; jsb gate {magic,3|4} mandatory; runs in main scan, core only on root_lost. Core misses: unreferenced blocks (S-004's claim, confirmed), ALL inode-image salvage (new), jsb gating direction, and journal-name binding for healthy volumes.
- New divergence candidates: HFS+ dotfile/Private-Data sink filter (core emits both); HFS+ rsrc forks (core `..namedfork/rsrc` = superset, og data-fork-only); 'prdf' file-type folderization.
- MATCH confirmations recorded for: ext4 SB gate (incl. incompat bit 0 = errata #7), HFS+ VH gate incl. fork clamps, journal ring/1 MiB/cancel discipline, jbd2 constants/escape restore, 64-bit GDT, InodeNNNN naming, og 0x50LL error style, extents-overflow follow.
---
Task ID: S2-G3
Agent: LDM rotation + scoring close-out (session-2 parallel read, read-only)
Task: Close OQ-T7 (RAID-5 rotation: find the og's VOLUME_SPEC builder and whether layout is config-driven or hardwired), verify the TOCBLOCK candidate lists, and decode the og's lost-partition scoring formula (OQ-T9) from the decompiles; compare all three against the core; read-only, no build.
Work Log:
- Read scratch/S2-D1-context.md, diff-to-fix/tracked-open/OQ-T6-T7-*.md and OQ-T8-T9-*.md, modules/ldm.md, core cr_ldm.cpp RAID-5/TOCBLOCK sections and scripts/ldm_reference.c.
- OQ-T7: corrected the S2-D1 offset model. struct VOLUME_SPEC is 0x275=629 BYTES (byte-copy loops: FUN_180056810 LdmManager_decompiled.c:47257-47309; FUN_180057a40 :47813-47853): +0x25d interleave bytes, +0x269 member count, +0x26d member-array ptr. Raid5 ctor FUN_180087150 :68924-68962 copies it to outer+0x18 (covers up to outer+0x28C only), so the layout selector (outer+0x2b0 = inner+0x2a8, zeroed :68943) is NOT part of the SPEC and is never written afterwards => og hardwired to layout 0 = left-symmetric. Grep confirmed no other writer of that field.
- OQ-T7: found the SPEC builder from LDM config = FUN_180073400 :59787+ (matches VOLUME by GUID at +0x32, COMPONENT by child extents, then :59907 count, :59924 interleave = comp_VBLK+0x4c sectors * 0x200, :59925 GUID, :59927 name, :59929-59944 kind enum; NO layout field). Interleave is config-driven with 0x10000-byte defaults (builder ctor :47665, Open :68632-68641); og rotation is not.
- OQ-T7: algebra-checked og read path FUN_180086a90 :68822-68858 (r=floor(off/S), g=r/(N-1), layout0 col=((N-1)-(g%N)+1+r%(N-1))%N, member off g*S+c) == core cr_ldm.cpp:1346-1359 (row, i, parity=(cols-1)-(row%cols), col=(parity+1+i)%cols, LBA row*chunk+in); degraded XOR sets identical (FUN_180085c30 vs cr_ldm.cpp:1372-1388). RESOLVED-MATCH.
- TOCBLOCK: verified og probe order config+2 -> config+0 -> config+1 (FUN_180072ad0 :59573-59608, magic 0x544f43424c4f434b after byte-swap, "config" name check :59612, bitmaps/VMDB range gates :59621-59636, config start = PRIVHEAD@0x12B :59748, config size gate <0x801 sectors :59750-59759) vs core {1,2,2045,2046} cr_ldm.cpp:1112-1131. Divergence stated both directions (+0 og-only, 2045/2046 core-only, +2-vs-+1 preference).
- OQ-T9: found the og scorer = DRWUI FUN_14050fda0 (DRWUI_decompiled.c:842164-842278): count = FS==8 ? files+folders*10 : FS<=2 ? (files+folders)*50 : files+folders*50; += bonus(struct+0x68, per-FS, from SearchDLLs over IPC); += 500 on partition-table match ("Found once partition:%I64d" :842263); score = (count>1 ? ln(count) : (double)count) / (maxCount>1 ? ln(maxCount) : 1.0), NATURAL log, written to struct+0x70. Verified to 6 decimals on 187 log rows incl. the f=1 anomaly (numerator stays 1.0 => score 1/ln(max): 0.081928/0.083079 exact), zero-file "base" (ln(bonus)/ln(max): 0.340154=ln60/ln168849, 0.278652=ln30/ln199964, 0.632748=ln60/ln646), FAT32 counts (0,25)+20=1270 => 0.585518, (0,1)+20=70 => 0.348069.
- OQ-T9: decoded ordering = merge sort on score desc (FUN_1405178a0, comparator :846927) and AdjustPartitionInfoOrder = FUN_140510410 :842495-842859 (FS grouping, 0.1/0.3 score-gap keep rules :842756-842769, ~16MiB over-large rule + score-copy-from-first re-score :842776-842851, FAT sub-array rule :842812-842821, dup removal + splice-to-front, driver FUN_140515630 :845666-845789 with "result" log).
- Core comparison: cr_candidates.cpp probes (NTFS 15/25/30, FAT 20/10, exFAT 15/20, GPT 10/20), c.score=pts :264, overlap penalties -20/-10 :289-304, first-nested-wins dedup :271-288, sort :305-306 — structural absolute points vs og's content-count log-ratio. DIVERGENT, documented.
- Wrote scratch/S2-G3-context.md with all quotes. No build run; no repo files modified; no tokens.
Stage Summary:
- OQ-T7 RESOLVED-MATCH: og layout selector hardwired 0 = left-symmetric (ctor zero at LdmManager_decompiled.c:68943; VOLUME_SPEC has no layout field — builder FUN_180073400 writes none; SPEC copy ends at outer+0x28C); og read-path layout-0 arithmetic is IDENTICAL to core cr_ldm.cpp:1352-1359; interleave both config-driven from the component VBLK chunk/interleave field (og *0x200 bytes, default 64KiB; core direct sectors). The layout 1-3 branches in the og are dead code.
- TOCBLOCK RESOLVED-DIVERGENT: og probes {config+2, config+0, config+1} first-valid-wins; core probes {config+1, config+2, config+2045, config+2046} first-valid-wins. Core misses a valid TOCBLOCK at config+0; og misses the kernel's tail copies 2045/2046; conflicting valid copies resolve differently (og prefers +2, core prefers +1). Recommend core add config+0 after 2046.
- OQ-T9 RESOLVED (formula extracted, core DIVERGENT): og score = ln(files + 50*folders [+50*files for FAT12/16/32 as (f+d)*50] + per-FS bonus(+0x68) + 500*table-match) / ln(max-over-scan), with count==1 quirk (numerator 1.0) and implicit 1.0 cap; scores are per-scan RELATIVE. Observed bonuses: NTFS 60/30, FAT32 20 (setter = search-DLL side, over IPC; not yet pinned to a function). No "0.632748/0.585518 base scores" exist — those are ln(bonus)/ln(normalizer) artifacts; S2-D1's 0.1913*log10(files) fit was the log10 re-statement of the ln-ratio.
- New divergence candidates: (1) LDM TOCBLOCK candidate set/order (A/B2); (2) og 1MiB config-size reject vs core 4MiB clamp (B2/X51); (3) og 64KiB interleave default vs core refuse-on-zero (B2/A27); (4) lost-partition scoring model: og content-count log-ratio + table-match +500 + bonus vs core structural points (A30); (5) og f=1 ranking quirk (1-file above 2-file) has no core analogue (A30); (6) og AdjustPartitionInfoOrder post-hoc keep/re-score rules absent in core (A30).
---
Task ID: S2-FINAL (session closeout)
Agent: main auditor (session-2 diff-audit continuation, read-only)
Task: Consolidate 11 parallel agent reports + first-hand verification; author session-2 findings; remove fixed findings; close all 9 tracked-open questions; update all deliverables; final push
Work Log:
- Wave 1 (agents S2-A..S2-F, parallel): FAT/exFAT exhaustive (S2-B), post-closeout core code audit (S2-E), corpus coverage sweep (S2-F) delivered; NTFS/Linux-engines/Partition agents hit execution deadlines -> relaunched as wave 2 (S2-A2, S2-C1, S2-C2, S2-D1, S2-D2): only S2-D2 delivered, but the four "failed" agents had written full context sheets to scratch (S2-A2/S2-C2/S2-D1 salvaged and verified first-hand; S2-C1's ext4/HFS+ scope relaunched as S2-G2). Wave 3 (S2-G1 NTFS pass-1 decode, S2-G2 ext4+HFS+ payloads, S2-G3 LDM closeout) delivered in full.
- First-hand verification by the main auditor before any finding was filed: cr_scan.cpp event-sink guard vs call (:309/:340/:76-77/:126-131) and double boundary emit (:577-578); cr_exfat_quick.cpp 32 MiB gate (:499-505) and 1-cluster fallback (:366-370); cr_bitlocker.cpp Services\FVEAutoUnlock path + blind enumeration (:176-236) + hive grep clean; cr_ldm.cpp TOCBLOCK list (:1112-1131) and chunk==0 refusal (:1344); cr_candidates.cpp absolute points; NTFSSearchDLL decompile spans :18488-18500 / :18625-18633 / :21246-21264 (pass-1 emit + pass-2 destroy); disk-map allocation overlay (cr_diskmap.cpp:55-75).
- Empirical: Linux Release build at e93decf -> ctest 282/282 PASS (evidence/executions/build-suite-session2.md). ASan build vehicle OOM-reaped by container; CI carries ASan/TSan.
- Removed fixed things (per session-2 mandate): A-001, S-001, S-005 -> resolved/findings/ with session-2 verification sections (S-001 with the MIS-SCOPED postscript -> S-010); OQ-T1..T9 -> resolved/tracked-open/ with resolution sections (T7 = RESOLVED-MATCH left-symmetric, safety-critical closure; T9 formula verified vs 187 log rows).
- New findings authored: A-002 (event-sink crash); S-010 (NTFS two-pass architecture: GUID-dump pass 2 + size-0 rescue missing, destroy mis-scoped into the main walk), S-011 (exFAT >32 MiB FAT -> 1-cluster truncation), S-012 (BitLocker FVEAutoUnlock wrong live path + no offline-hive path + unassociated enumeration); M-007..M-017 (FAT table windows, mirror mode, table validation; exFAT deleted-set checksums + structural gates; LDM TOCBLOCK; lost-partition scoring; FilterFolder junk filter; ZFS multi-vdev fallback + fletcher4/DVA-retry; Btrfs profiles + default subvol; carve video hook); M-005 given its canonical standalone file (og bitmap-gate decompile proof); N-010..N-017 grouped minors; I-006/I-007/I-008.
- Existing findings updated in place: S-002/S-003 direction-corrected with exact decompile payloads (fix plans replaced - both cheaper than session-1 versions); S-004 refined + journal inode-image salvage added; S-006/S-007/S-008 evidence upgraded (S-007: +26 formats beyond session-1's list; S-008: full 33-method repair table); S-009 re-scoped to its post-56cce87 remainder (M-006 unchanged); M-001 revised (ladder quote mis-attributed; real two-sided divergence); N-001 confirmed + inclusive-bound nuance; N-002 confirmed with corrected mechanism; N-003 refuted (superseded by M-009); N-008 refined (N-014d ZIP padding inversion); I-001 refined (pass-1 name-table+SI gate vs pass-2 blanket-$).
- New tracked-open: OQ-T10 (NTFS grading locus: DRWUI CRecoverability/$I-R pairing/engine +0x258-0x259), OQ-T11 (SearchDLL bonus setter +0x68, ZFS pool display name, Btrfs name-conversion anchor).
- Deliverables updated: CROSS-REF-MATRIX (session-2 cells), SUMMARY (new counts: 1 ACUTE, 10 SEVERE, 15 MAJOR, 15 MINOR files, 8 INFO, 2 OQ files; top risks re-ranked with S-010 first; fix order refreshed), README (session-2 section), all 19 module files (session-2 verdict sections), evidence/decompiled-quotes/*session2* + ntfs-pass2-guid-dump + ldm-raid5-and-scoring, evidence/executions/build-suite-session2.
- Corpus errata: 8 artifact claims corrected against the decompile (I-008) - recommended as CORRECTIONS.md feedback to the research repo; CORRECTIONS row-5 residue noted in modules/visualization.md.
- Read-only compliance re-verified pre-commit: git status shows only diff-to-fix/ + worklog.md + worklog/session-diff-audit-2-prompt.md (redacted) changed; no source/header/test/build/workflow/doc file touched; token grep clean in worktree and full diff.
Stage Summary:
- Session 2 complete at core e93decf vs corpus 03ad46c: 3 findings + 9 open questions removed as fixed/resolved; 20 new finding IDs filed (1 ACUTE, 3 SEVERE, 11 MAJOR, grouped minors, 3 INFO); 14 existing findings refined or corrected (2 direction corrections); all 9 session-1 open questions closed with evidence.
- Top new risk: S-010 - the already-landed S-001 destroy rule is mis-scoped (og pass-1 emits ungraded; destroy belongs to the missing GUID-dump pass 2) - the remediation team should read resolved/findings/S-001* postscript + S-010 before the next NTFS work.
- Safety-critical closure: LDM RAID-5 rotation is og-MATCH (left-symmetric, hardwired) - dynamic-disk reconstruction is unblocked; remaining LDM work is TOCBLOCK candidates + geometry minors (M-012/N-012).
---
Task ID: S2-RECON (main-auditor leg — rebased re-application)
Agent: main auditor (session-2 third leg, read-only)
Task: Re-apply this leg's session-2 output onto the reconciled main tree (34b3441) after the push revealed two parallel session-2 legs + a rapid-fix wave
Work Log:
- Push of this leg's first commit (8b8d90f on e93decf) was rejected: origin/main had moved to 34b3441 with 30+ commits from two parallel audit legs (17+ new findings with colliding IDs: A-002/A-003, S-010/S-011, M-007..M-010, N-010..N-021, I-006..I-008) and a remediation wave that FIXED S-002, S-004, S-008, N-001, N-002, N-003, M-001, M-006, N-010 and landed S-010/S-008-write-path fixes (e0175b3, 3e34ed2).
- Reset to origin/main (first commit preserved on branch backup-s2), re-applied this leg with full reconciliation:
  * All this leg's finding IDs renumbered past both prior legs: A-002->A-004, S-010..S-012->S-012..S-014, M-007..M-017->M-011..M-021, N-010..N-016->N-023..N-029, I-006..I-008->I-009..I-011, OQ-T10/T11->OQ-S2M-1/2.
  * Duplicates dropped: tz item (their S-011), double-emit item (their N-016); mirror-on-BAD sub-case kept as part of the fuller M-012 with N-022 cross-ref; N-029 cross-refs their M-010.
  * My earlier verification errors corrected in place: FIXED/A-001 note corrected (parallel leg's executed proof that 1cb6a43's LDM GUIDs are fabricated wins over my leg's static hand-decode); the exFAT fixer-checksum "no-skip" candidate (my N-017) RETRACTED after my own decompile re-read found the skip-table {106,107,112} (find==end over DAT_180079a40, -1-terminated; same table consumed at :10949-50).
  * Cross-verification WINS recorded against a prior leg: the fb44073 N-003 fix's "verified-absence" premise refuted with the FUN_18001eba0 dual-form checksum quote (og DOES validate entry sets incl. deleted sets) — appended to FIXED/N-003 with the fix direction corrected (M-013 carries it). The fb44073 N-002 behavior verified but its :8030-8048 citation corrected (that span is the lost-FAT region-signature validator; real termination evidence :53958-61 + :11231).
  * This leg's session-2 verification sections re-applied to the FIXED/ files (S-002 og-parity boundary for c9e8860; S-004 residue: journal inode-image salvage 0xf30a + main-scan condition; S-008 full 33-method table for e0175b3 auditing; S-001 mis-scope postscript -> S-012; S-005 U+FFFD anchoring; N-001 inclusive bound; M-001 re-derived ladder) and to the still-open findings (S-003 two-model reconciliation with consumption-side evidence; S-006 scoring handoff; S-007 full signature payload; S-009 remainder items with M-006 acknowledged FIXED).
  * Current-tree facts verified and recorded: A-002's constant restore NEVER landed (cr_table_wire.h last touched by 1cb6a43; still carries fabricated GUIDs with false Microsoft-identity comments at 34b3441) — A-002 remains roadmap #1.
  * Module files: session-2 sections re-appended (renumbered) + post-reconciliation addenda; CROSS-REF-MATRIX merged as a main-auditor cell table; SUMMARY closed with the third-leg section (verified facts + reconciled counts); README + CORPUS-ERRATA + FIXED/RESOLVED-INDEX addenda; evidence quote sheets carried with the checksum correction.
Stage Summary:
- Session-2 third leg complete and reconciled: 18 net-new active findings (1 ACUTE, 3 SEVERE, 11 MAJOR, 7 MINOR groups, 3 INFO) + 2 tracked-open files; 2 of the leg's own earlier claims corrected (A-001 verification, checksum predicate); 1 prior-leg fix premise refuted with quotes (N-003) and 1 prior-leg citation corrected (N-002); all 9 session-1 OQs resolved with evidence across the legs.
- Read-only compliance: only diff-to-fix/ + worklog touched in either repo; source tree at 34b3441 byte-identical to origin/main.
- Handoff: the roadmap's top three are A-002 (restore real LDM constants — still missing), A-004 (one-line event-sink fix), S-012 step 1 (one flag flip restoring og deleted-file membership). Then S-013/S-014 and the leg-3 majors.
---
Task ID: DF3-T1
Task: diff-fix session 3 bootstrap — repos pulled, state reconciled, baseline recorded.

Work Log:
- Cloned both repos fresh: core at 7fd00e0 (M-009 fix leg landed; SUMMARY's S-010/M-009
  rows verified present in-tree), corpus at 03ad46c (unmoved, matches audit baseline).
- Audited commit per diff-to-fix/README.md: 5d6da5f (session-2 baseline); current main
  has advanced through the parallel fix leg (S-010 3e34ed2, 414f56c, 34b3441) and M-009
  (7fd00e0). Re-read current state before acting on every finding - SUMMARY is known
  stale in places (d5282eb reconciliation note).
- Session prompt saved (token redacted) in worklog/session-diff-fix-prompt.md with a
  session-3 addendum recording the user's trailing deletion instruction and the
  contradiction ruling: findings/FIXED/ is preserved (per SS2/Deliverable 11.12/13);
  RESOLVED-INDEX.md kept current so closed findings are O(1) to skip.
- Toolchain stood up in-session: cmake 4.4.4 (pip), gcc 14.2, googletest v1.15.2 via
  FetchContent. linux-gcc preset (ASan) configures, builds clean, suite baseline
  301/301 PASSED at 7fd00e0.
- A-002 independent verification STARTED (own extraction, not the audit's evidence):
  wrote scripts/verify_ldm_guids.py (PE walker) - LdmManager.mo DAT_1800c4960 and
  Partition.mo DAT_180036630 both = 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3 (real
  PARTITION_LDM_METADATA_GUID), Partition.mo DAT_180036640 =
  af9b60a0-1431-4f62-bc68-3311714a69ad (real PARTITION_LDM_DATA_GUID); full-corpus
  1470-file scan: real bytes present in 8 og files, fabricated 8e9f/384e values in
  ZERO files. Corpus gate code re-read first-hand (LdmManager_decompiled.c
  FUN_1800724f0: sig/rev 0x10000/size 0x5C/reserved==0 header gate, per-entry memcmp
  vs DAT_1800c4960, PRIVHEAD at entry+0x28 last-LBA). CONFIRMED-DIVERGENCE.
- Core consumption confirmed: cr_table_wire.h:87-90 carries fabricated bytes; consumed
  at cr_ldm.cpp find_privhead (ERR_BAD_MAGIC on every real GPT dynamic disk) and
  cr_table_parse.cpp GPT entry classification; two self-referential tests found and
  read in full (test_crc32_gpt.cpp:369, test_ldm.cpp:519).

Stage Summary:
- Session bootstrapped; baseline green (301/301 ASan); A-002 classification
  CONFIRMED-DIVERGENCE on four independent sources (own PE extraction + corpus
  decompile re-read + Microsoft-documented values + whole-corpus absence of
  fabricated bytes). Fix next: restore constants, de-self-reference tests, add
  real-GUID positive + fabricated-GUID negative + GUID-text tests.

  envelope - the GAP-S6-13 mandate holds repo-wide.
---
Task ID: HANDOFF (this leg's close-out)
Agent: main engineer (diff-to-fix remediation, second parallel leg)
Task: Session close — handoff with exact commits, remaining scope, next steps

Work Log (this leg's landed work, all pushed to main with green CI):
1. worklog/audit-trail repair: 1cb6a43's unresolved merge-conflict markers resolved (both
   histories preserved); token hygiene verified repo-wide (worktree + full history).
2. S-008 (SEVERE) FIXED: RawFixer pre-scan consent-gated wiring + the og's method surface
   (CheckMFT with $MFTMirr restore, CheckFatTable sector-granular mirror restore,
   CheckRootDir/IsValidRootDir/CheckRoot advisories, FixByScanSector bounded 64 MiB probe)
   + CLI --repair/--repair-apply/--repair-undo + cr_ui ABI 1.2 consent surface. 16 tests,
   fail-before proven; engine-boundary proof: torn $MFT record 0 -> NtfsVolume::open fails,
   repaired -> opens. Commits: e0175b3 (+ CI follow-ups 0532415/8574039/fee4437/9d2cac2).
3. S-010 (SEVERE) FIXED: volume quiesce on the repair write path — IWriteDevice
   lock_volume/unlock_volume (S7), execute() lock-before-write + zero-write refusal,
   Windows RwDevice runs the corpus RecoveryModule LOCK/DeleteMountPoint/Flush/settle/
   verify/DISMOUNT sequence per mounted volume; finding's destination-lock bullet
   CORRECTED against the corpus (destination open = writability+detection only).
   3 tests, fail-before proven. Commits: 3e34ed2 (+ integration 414f56c + gates 34b3441).
4. M-009 (MAJOR) FIXED: RwDevice::read_at routed through the GAP-S6-13 64 KiB envelope
   (structural parity with the scan devices). Commit: 7fd00e0. CI green.
5. CI red repairs (both legs' debt): missing <algorithm> (MSVC std::search), CRT
   exclusive-open root cause (_wfopen_s -> _wfsopen _SH_DENYNO for image devices),
   clang-format sweeps at the CI-exact 18.1.8, DeleteVolumeMountPointW allowlist entry
   with reason, per the gate's documented process.
6. Verified the parallel legs' landings locally at each rebase (S-005, S-001, M-001,
   N-001, S-002, S-004, A-002, N-002/N-003, M-006, N-010, I-001..I-005): every integration
   point re-ran the full suite (309 -> 313 -> 316 -> 317 green).

Remaining scope (this leg's lane, NOT started - handed to the active s3/parallel legs
per their claimed targets):
- S-006 (lost-partition probe families: Linux/HFS/Btrfs/ZFS/ReFS) — the s3 leg's
  CPartitionVirtualVolumeDevice deep-pass covers adjacent surface; the finding file's
  fix plan (engine wiring only) remains the concrete next step.
- S-007 (carve formats GIF/BMP/MP3/AAC/RTF/OLE/MPEG/DWG) — the s3 raw-signs deep decode
  (bdcb77e, S-012/N-023/N-024/OQ-T8) is the correct prerequisite wave; the finding's
  format list stands.
- S-003 (HFS+ journal replay) — the prior leg deferred it with precise scope and a
  first-hand decoded evidence addendum (b85c084); the next step is in that addendum.
- S-009/S-011/S-012/M-002/M-007/M-008/M-010 and the N-* wave — owned by the active
  legs' queues; see diff-to-fix/SUMMARY.md's live remediation log.

Stage Summary:
- This leg closed: S-008, S-010, M-009 (+ the audit-trail/CI infrastructure repairs),
  each with fail-before/pass-after tests, corpus-first verification, honest Resolutions
  (including one finding correction), and green CI on Linux + Windows at every landing.
- main is green at 7fd00e0; the s3 leg has already bootstrapped on top of it. The corpus
  (03ad46c) was never modified. No token material ever entered either repository.
---
Task ID: DF3-T2 (A-002)
Task: ACUTE A-002 — restore the real Microsoft LDM GPT type-GUIDs corrupted by the A-001 fix; de-self-reference the regression tests; correct the audit trail.

Work Log:
- Re-verified first-hand before touching code (independent of the audit's evidence):
  own PE-section walker (scripts/verify_ldm_guids.py in session workspace) extracted
  LdmManager.mo DAT_1800c4960 + Partition.mo DAT_180036630 = 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3,
  Partition.mo DAT_180036640 = af9b60a0-1431-4f62-bc68-3311714a69ad; 1,470-file corpus
  scan: real bytes in 8 og binaries (9+9 occurrences), fabricated 8e9f/384e values in ZERO
  files; corpus gate re-read (LdmManager_decompiled.c FUN_1800724f0: memcmp vs the real
  metadata GUID, PRIVHEAD at entry+0x28). CONFIRMED-DIVERGENCE; the finding's claim
  verified exactly.
- Fail-before proof (tests edited FIRST, constants still fabricated): 5/5 FAILED -
  Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes (strings corrected to real winioctl.h
  forms), Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText (new bytes->text guard),
  Ldm.GptDynamicDiskCanonicalGuidsParse (fixture de-self-referenced to hard-coded real
  bytes -> ERR_BAD_MAGIC on the real-GUID disk), Ldm.GptDynamicDiskFabricatedGuidsRejected
  (new negative guard: fabricated-GUID disk WRONGLY parsed end-to-end),
  Ldm.GptDynamicDiskTableClassifiesLdmEntries (de-self-referenced; LDM entries classified
  Other).
- Fix: cr_table_wire.h GPT_TYPE_LDM_METADATA/GPT_TYPE_LDM_DATA restored to the real
  Microsoft bytes; provenance comment rewritten to the true two-source chain (winioctl.h/
  Microsoft Learn + og-binary offsets + kernel efi.h agreement) with the A-002 history note.
- Pass-after proof: same 5 tests 5/5 PASSED; full suite 303/303 Linux ASan green
  (301 baseline + 2 net-new tests).
- Audit trail, same commit: A-002 Resolution appended + moved to findings/FIXED/;
  RESOLVED-INDEX A-002 row + count 13; CROSS-REF-MATRIX table+ldm row cells -> M (A-002
  FIXED); modules/ldm.md B1/B8/C19 bullets + findings-table row updated to post-fix truth;
  SUMMARY combined counts (2->1 ACUTE) + remediation-log row + recommended order re-led
  by A-004; gpt-guid-probe-harness.cpp LDM rows rewritten to real bytes + correction note;
  gpt-guid-probe.md annotated (session-1 VERDICT was a fixture artifact);
  docs/parity/PARITY-CORE-REC.md "GPT - LDM GUIDs" row rewritten.
- Safety hardenings: none near this fix; both consumption sites byte-identical (only
  constant values changed). Win7-inclusive gate unaffected (data constants, no imports).

Stage Summary:
- A-002 FIXED: every real GPT dynamic disk parses end-to-end again (LDM discovery
  restored); fabricated values now carry a permanent negative guard; the audit trail
  records the A-001 false-positive history truthfully. Suite 303/303.

---
Task ID: S3-CLOSE (session-3 diff-audit close-out)
Agent: session-3 parity auditor (read-only continuation leg)
Task: Session close-out — handoff record for the "go deeper, remove the fixed, mark the remainder" session.

Work Log:
- Bootstrap at core 34b3441 / corpus 03ad46c; prompt saved redacted; state reconciled
  against both session-2 legs + the parallel s2-third-leg (2ce2e6b) that landed mid-session.
- RETIRED (each verified first-hand, each with a Resolution addendum):
  * S-010 (3e34ed2+414f56c+34b3441 — repair-path volume quiesce; stale SUMMARY row fixed)
  * M-009 (7fd00e0 — RwDevice read path through the 64 KiB envelope)
  * A-002 (7f2c83e — real Microsoft/og LDM GPT GUIDs; my INDEPENDENT og-binary scan of
    LdmManager.mo/Partition.mo/MainModule.dll/RecoveryModule.dll found 0434cfb3/714a69ad
    at 7 sites, converging with the third leg's evidence, the fix leg's own verification,
    and the 6-source external table; tests de-self-referenced with negative guards)
- NEW FINDINGS (7): S-015 SEVERE (raw_signs.json min/max are KiB in the og — max<<10,
  default 0x7d000 KiB — vs bytes in the core: 1024x recall-window loss on all 139
  raw-sign formats), M-022 (ReFS file-value grammar: og type@key+0, dual layout
  {0x178 old / >=0x29 modern} gated on system-dir-count >= 8, $I recycle handler,
  all-rows iteration vs core 0x80@key+8 first-match-break), M-023 (filter model:
  8-cat/1481-ext runtime XML vs 7-cat/64-ext hard-coded; Email+Bookmarks missing),
  N-030 (raw_signs confidence field dead in the og — parser consumes offset+hex only),
  N-031 (carve dispatch: u32->u16->linear->sign-DB order decoded + sector stride +
  zero-sector skip + only_common gate), N-032 (health: CSMI/USB-bridge transports),
  I-012 (GAP-ADS RESOLVED — the og does not recover ReFS ADS either; core MATCH).
- OQ-T8 RESOLVED (FUN_18000cf60 decode; disassembly-proven call order; the parallel
  leg's convergent evidence block had u16/sweep swapped — corrected via append-only
  addendum with the 0x18000d1e4/0x18000d2da/0x18000d2f3 call-site proof).
  OQ-T9 verified as resolved by the s2-third-leg (M-017, 187-row log verification).
- Corpus errata 19-21 filed (dispatch-order inversion; dead confidence field; KiB units).
- Architectural verification: CPartitionVirtualVolumeDevice = the core IStorageDevice +
  slice adapters (equivalent; devirtualized fast paths cited).
- Coverage added this session: RAWSearchDLL raw_signs loader/parser/index/scan-loop/
  dispatch (first deep decode in any session), ReFSSearchDLL file pipeline (first),
  FileFilter.xml (first), diskhealth companion transports (first), plus convergence
  reads of the s2-third-leg's M-017/FAT/exFAT work.
- Cross-leg coordination: 3 rebase/pull cycles against active parallel legs (fix leg +
  s2-third-leg + diff-fix-s3 leg); IDs renumbered past their S-012..S-014/N-023..N-029.

Stage Summary:
- Session-3 end state: 65 active (1 ACUTE: A-004 / 9 SEVERE / 18 MAJOR / 27 MINOR /
  10 INFO) + 19 FIXED. Recommended fix order update: 1) A-004 (only ACUTE),
  2) S-015 (raw-sign KiB units — one-generator fix, largest recall delta),
  3) S-011 (exFAT tz, one line), 4) S-003 (HFS+ journal — evidence decoded),
  5) S-006/S-007 (probe families / carve formats), then M-013 (exFAT dual-form
  checksums), M-022 (ReFS grammar — start with the real-volume key-layout dump),
  M-023 (filter table generation), M-016 (LDM TOCBLOCK), M-017 (lost-partition
  scoring port), N-030/N-031/N-032 (raw-signs semantics decisions + health transports).
- Honest not-completed-by-this-leg: M-022's key-offset reconciliation (needs one real
  ReFS volume dump — open question in the finding), the $I/$R handler body decode
  (FUN_18000ea30), ReFS quick-search dot-prefilter verdict (M-022 OQ-3), og-execution
  benches for the raw-sign unit fix (S-015 acceptance), N-032's RST/USB hardware
  verification.

---
Task ID: DF3-T3 (A-004 + S-016)
Task: ACUTE A-004 — ErrorRange delivered through the config sink (null-call crash /
mis-delivery); DISCOVERED during the fail-before proof: S-016 — the ErrorRange event
stream never fired at all (snapshot error map fed after the emitters). Both fixed in
one commit (one delivery path, each defect is the prerequisite of the other's symptom).

Work Log:
- A-004 verified first-hand at 7f2c83e: guard on event_fn_ (cr_scan.cpp:310) vs delivery
  cfg_.event_fn (:340); ctor seed :76-77/:90-91; set_event_sink :126-131; grep-audit of
  all cfg_.event reads = ctor seeds + cr_ui shim's pre-construction config write are the
  only legitimate ones; :340 the sole wrong-sink call.
- Fail-before test authored (ScanEvents.ErrorRangeGoesToLiveSink): FAILED with ZERO
  ErrorRange events, not the expected crash - the branch was unreachable.
- Root-cause chain probed with a session-local harness (FAT16 + one deterministically
  failing sector inside the FAT2 span, wholesale-read by FatTable::load cr_chain.cpp:100):
  stats.errors carried exactly one range (off=15360 len=512, rc=0) yet no event fired.
  Code walk: tel_.snap.errors merged ONLY inside stamp()'s finalize block (:656) which
  runs AFTER the boundary emits (:647-648), and the stage-end emits (:722/:1085/:1139)
  read the same never-fed snapshot -> count==0 at every emit -> delta gate never opened
  -> ErrorRange dead since the event stream's introduction (56cce87). S-016 filed
  (SEVERE; missing documented ABI-CONTRACT.md §6 error-delta stream; og surfaces the
  same state mid-run via BadSectorTip FB-O03).
- Fix: (1) tel_counts_ live-merges the retry pipe's error map into tel_.snap.errors
  under the telemetry lock (same error_map_copy() merge telemetry() performs; common
  ancestor of every boundary-emit call site; idempotent under ReadErrorMap
  normalization). (2) emit_boundary_events_ delivers through event_fn_(e, event_user_).
- Second test added (ScanEvents.SinkSwitchDeliversAllTypes): config sink A +
  set_event_sink(B) - ALL types incl. ErrorRange at B only, A receives nothing.
- Pass-after: 2/2 green; full suite 305/305 Linux ASan (303 + 2 net-new).
- Audit trail in-commit: A-004 Resolution + FIXED/ move; S-016 filed with full evidence
  (probe numbers, code cites) + FIXED/ move; RESOLVED-INDEX rows (count 15);
  CROSS-REF-MATRIX orchestrator O38/P42/R56 row -> both FIXED; SUMMARY remediation row;
  modules/orchestration.md bullets updated; N-016/N-014a noted as still-open MINORs.
- Probe harness removed after use (diagnostic only; never committed).

Stage Summary:
- A-004 FIXED + S-016 (new, discovered+fixed) CLOSED: the ErrorRange event stream is
  live for the first time, delivered to the right sink, pinned by two tests. ACUTE tier
  is now EMPTY (A-002, A-004 both closed this session). Suite 305/305.

Task ID: API-T1 (session-core-api bootstrap)
Agent: main engineer (core-api session)
Task: Session bootstrap — repos pulled, prompt saved, state reconciled.

Work Log:
- Baselines: core 7d0ffa6 (session-3 close-out), rda-driver 7e88e38 (classifier/gate/SLIST
  fix wave), corpus 03ad46c (unmoved). All three pulled fresh.
- Session prompt saved token-redacted at worklog/session-core-api-prompt.md; read twice.
- Prior-state reconciliation: this session's lane is DIFFERENT from the s1-s3 engine-parity
  lane. Target: (a) API surface (cr_ui C ABI) exact-to-reference per corpus, (b) kernel-driver
  utilization matching og's driver usage pattern per interaction, (c) measured performance
  (64 KiB direct-read alignment mandatory, read-size/queue-depth sweep, round-trip overhead).
  Gate docs BEFORE code: docs/API-SURFACE-AUDIT.md, docs/DRIVER-UTILIZATION.md,
  docs/PERFORMANCE-BASELINE.md.
- Note: prior session's X51 finding says 64KiB envelope "verified post-GAP-S6-13" in the
  RwDevice read path — this session must re-verify first-hand everywhere a direct read is
  issued (driver path AND direct fallback), not trust the claim.
- Reading wave launched (4 parallel legs): corpus driver-usage, driver repo full, core repo
  full, corpus API-facing behavior.

Stage Summary:
- Bootstrap complete; reading wave in flight; gate docs next; no source touched yet.
---
Task ID: 4-A-002 (ACUTE fix — first item of the fix session)
Agent: main engineer (diff-to-fix remediation, session 4)
Task: ACUTE A-002 — the A-001 fix (1cb6a43) corrupted both LDM GPT type-GUID constants with fabricated values; every real Windows GPT dynamic disk failed LDM discovery (ERR_BAD_MAGIC) — zero dynamic-volume recovery

Work Log:
- Baseline: cloned core at d5282eb / corpus at 03ad46c (unmoved); full suite 317/317
  Linux ASan (RelWithDebInfo + ASan) — matches the S-010 handoff claim.
- Independent verification BEFORE touching code (script scripts/verify_a002_guids.py,
  preserved in-session): PE-section extraction of LdmManager.mo DAT_0x1800c4960 =
  5808c8aa-7e8f-42e0-85d2-e1e90434cfb3, Partition.mo DAT_0x180036630 = same,
  DAT_0x180036640 = af9b60a0-1431-4f62-bc68-3311714a69ad (the REAL Microsoft
  PARTITION_LDM_*_GUID values); full 1,467-file og-codebase scan: real metadata
  GUID in 8 files, real data GUID in 7, FABRICATED values in 0; decompiled gates
  re-read (LdmManager FUN_1800724f0 memcmp vs DAT_1800c4960; Partition
  FUN_180002bd0 vs DAT_180036630/0x36640); git show 1cb6a43 confirms pre-fix =
  real bytes. Classification: CONFIRMED-DIVERGENCE.
- Fix: cr_table_wire.h constants restored byte-for-byte to the real Microsoft
  values; provenance comment rewritten (winioctl text + og binary compare-operand
  offsets + kernel/libldm agreement; A-001-era false provenance removed).
- Tests de-self-referenced (the root defect A-002 called out — the old tests copied
  the constants into fixtures and passed for ANY value):
  Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes now pins the real winioctl text
  forms; Ldm.GptDynamicDiskCanonicalGuidsParse + GptDynamicDiskTableClassifiesLdmEntries
  write hard-coded real byte literals via a shared build_gpt_dynamic_disk builder.
  Three new guards added: Ldm.GptDynamicDiskRealCanonicalGuidsParse (entry GUIDs
  derived independently from the winioctl text — two encodings of ground truth that
  must agree), Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText (reverse-decode
  constant -> GUID text vs string literals), Ldm.GptDynamicDiskFabricatedGuidsRejected
  (negative: the fabricated 8e9f/384e bytes must be refused ERR_BAD_MAGIC).
- Fail-before/pass-after PROOF: with the constants temporarily reverted to the
  fabricated values, 6/7 guard tests FAIL (CanonicalGuidsParse,
  RealCanonicalGuidsParse, FabricatedGuidsRejected [fails for the right reason:
  fabricated fixture + fabricated constant = parse succeeds], TableClassifies,
  Gpt.Canonical..., Gpt.LdmGuid...); with the fix, full suite 320/320 Linux ASan
  (317 baseline + 3 new), two consecutive full-suite runs green. One unrelated
  observation: Snapshot.ConcurrentEmitCaptureWalk flaked once under full -j load
  (passes 5/5 isolated + 2 full-suite reruns) — pre-existing timing sensitivity,
  NOT a regression; noted for a future CI-stability pass.
- Audit trail: A-002 Resolution section appended; finding moved to findings/FIXED/;
  RESOLVED-INDEX row + A-001 caveat; CROSS-REF-MATRIX table+ldm B/C cells -> M
  (A-002 fixed); modules/ldm.md B1/B8/C19 -> M with A-002-fixed notes; SUMMARY.md
  (counts 0 ACUTE / 43 active / 13 fixed, addendum resolved, top risks, remediation
  log); README.md status; PARITY-CORE-REC.md GPT-LDM row (had the fabricated values
  documented as canonical — corrected); AUDIT-PIPELINE.md LDM recognition row;
  CAPABILITY-MATRIX.md LDM row; evidence/executions/gpt-guid-probe.md annotated
  (session-1 "not present in og binaries" claim was a wrong-needle artifact) +
  gpt-guid-probe-harness.cpp rewritten to write the real bytes.

Stage Summary:
- A-002 FIXED: every real Windows GPT dynamic disk parses again (is_gpt, full volume
  graph, 2 layouts); the fabricated values are now regression-guarded from four
  directions (byte literal fixture, text-derived fixture, winioctl text compare,
  negative rejection). Zero ACUTE findings remain. Next per the recommended order:
  S-011 (exFAT tz_offset, one-line), then M-009 (64 KiB envelope, one-line).

---
Task ID: 4-A-002-rebase (A-002 reconciliation note)
Agent: main engineer (diff-to-fix remediation, session 4)

Note: this session's A-002 fix (commit 5f95828, local) and the parallel leg's
A-002 fix (7f2c83e) landed independently within the same window - both restored
the identical real Microsoft GUID bytes and converged on the same de-self-
referenced test architecture (hard-coded literals + shared builder + negative
guard + winioctl-text guard). Rebased onto 7f2c83e: the parallel leg's versions
are the landed base; this session's unique addition on top is the
GptDynamicDiskRealCanonicalGuidsParse cross-encoding test (entry GUIDs derived
independently from the winioctl text forms - two independent encodings of
ground truth that must agree). Suite re-run green post-rebase. The parallel leg
also landed M-009 (64 KiB envelope, 7fd00e0), the raw-signs decode (S-012,
N-023, N-024, OQ-T8 RESOLVED), the ReFS deep pass (M-022, I-012, GAP-ADS
resolved), and the s2-third-leg reconciliation (18 findings, 9 OQs resolved).

---
Task ID: DF-T1 (diff-to-fix session bootstrap)
Agent: main engineer (diff-to-fix fix session)
Task: Pull both repos, record audited commit vs current commit, save the redacted prompt, baseline the tree.

Work Log:
- Baselines: core e6d5b01 (session-3 close + A-004/S-016 fix c33236f + worklog merge
  f210b7e + api-session bootstrap), corpus 03ad46c (unmoved). Both pulled fresh; no
  other repository consulted.
- Session prompt saved token-redacted at worklog/session-diff-fix-prompt.md; read twice.
- Audited-commit reconciliation: session-3 audited 7f2c83e; commits since: c33236f
  (A-004 + S-016 fix — both findings already carry FIXED/ files and Resolutions),
  f210b7e (worklog conflict-marker repair), e6d5b01 (api-session bootstrap, docs only).
  No other finding-relevant code drift since the audit.
- Toolchain stood up in container: cmake 4.4.4 + ninja via pip --user; zlib/lz4/zstd
  system dev packages present; GTest v1.15.2 via FetchContent.
- Baseline build + suite at e6d5b01 (Linux, Release-ish default config, -Werror):
  cr_tests 305/305 PASS, cr_ui_tests 15/15 PASS, cr_ui_abi_check OK. main is green
  pre-session; this is the regression floor for every fix.

Stage Summary:
- Bootstrap complete. 65 active findings + 6 tracked-open files to verify and close:
  ACUTE 0 (A-004 closed by c33236f — re-verify first), SEVERE 9 (S-003, S-006, S-007,
  S-009-remainder, S-011, S-012, S-013, S-014, S-015), MAJOR 18 (M-002, M-005, M-007,
  M-008, M-010, M-011..M-023), MINOR 27, INFO 10, tracked-open 6 files. Reading wave
  (T2) next: full diff-to-fix/ then core tree.

---
Task ID: S4-T1B (main engineer, diff-to-fix remediation session 4 — reconciliation)
Task: Reconcile with the parallel session-4 legs that landed while this session bootstrapped.

Work Log:
- This session independently developed the A-002 fix (constant restore + de-self-
  referenced tests + negative guard) from a fresh clone at 805eebd, with own PE
  extraction (LdmManager.mo DAT_1800c4960 / Partition.mo DAT_180036630/0x36640 =
  real Microsoft GUIDs; 18 real-byte hits / 0 fabricated hits corpus-wide —
  scripts/s4_verify_ldm_guids.py). On push, origin/main had moved: the parallel
  session-4 fix leg landed A-002 first (7f2c83e) plus the reconciliation commit
  abe26b8 (cross-encoding guard GptDynamicDiskRealCanonicalGuidsParse), A-004+S-016
  (c33236f), M-009 (7fd00e0), and the s3 audit legs' new findings.
- Local duplicate fix DISCARDED (reset to origin/main) — the landed fix is
  byte-identical in substance and broader in guards. No duplicate commits pushed.
- First-hand verification of the landed state at abe26b8:
  * A-002: constants re-extracted and MATCH Microsoft/og (own script, four
    sources); the five guard tests present in tests/unit/test_ldm.cpp +
    test_crc32_gpt.cpp (FabricatedGuidsRejected, RealCanonicalGuidsParse,
    winioctl-text rows); finding in FIXED/ with Resolution.
  * A-004: live code src/core/orchestrator/cr_scan.cpp:359 delivers ErrorRange
    through event_fn_ (live sink); the cfg_.event_fn reads are only the ctor
    seeds (:76/:90) + setter (:130); S-016 (snapshot error-map feed) fixed in
    the same commit; finding in FIXED/ with Resolution.
- Baseline re-established on abe26b8: linux-gcc preset (RelWithDebInfo + ASan),
  full suite 322/322 PASSED.
- Active inventory at abe26b8: 0 ACUTE; 9 SEVERE (S-003, S-006, S-007, S-009
  remainder, S-011..S-015); 17 MAJOR (M-002, M-005, M-007, M-008, M-010..M-023);
  26 MINOR files (N-001 dup-in-MINOR noted — reconciliation candidate); 11 INFO;
  5 tracked-open files. N-001 exists in BOTH MINOR/ and FIXED/ — to reconcile
  when touched.

Stage Summary:
- Session 4 (this leg) reconciled onto the parallel legs' landings; no duplicate
  A-002 commits pushed; verified their A-002/A-004 fixes first-hand. Lane resumes
  at the SEVERE wave starting S-011 (exFAT tz), then S-013/S-015/S-014/S-012,
  pulling before each finding to avoid collisions.

---
Task ID: S3-W1R (session-3 wave-1 reconciliation — parallel-leg merge onto main)
Agent: session-3 continuation auditor (read-only, S3 lead)
Task: Wave-1 landed concurrently with two other session-3 legs; reconcile IDs and
content onto main without losing either leg's work.

Work Log:
- Push of wave 1 (9e4ad5c, preserved) was rejected: main had advanced 34b3441 → e6d5b01
  with parallel session-3 legs' work (A-002 fix 7f2c83e, M-009 fix 7fd00e0, A-004+S-016
  fix c33236f, 18+ new findings with renumbered IDs, grouped-minor restructure,
  OQ-T8 resolution, errata 19-21). Rebased-aborted; reset to origin/main; wave-1
  content restored from 9e4ad5c via script with the renumber map.
- ID-collision convention applied (IDs on main win): my S-012/S-013/S-014/S-015 →
  S-017/S-018/S-019/S-020; M-011/M-012/M-015/M-016 → M-024/M-025/M-026/M-027;
  N-025/N-027/N-028/N-030/N-031 → N-033..N-037; I-011 → I-013. Cross-refs fixed inside
  all restored files (M-013/M-014 → M-017, my carve N-026 → inventory wording,
  N-029 → N-026 group N-013b).
- Convergent merges as addenda: N-031 (chain semantics + id-8 records + TIFF-family
  equivalence), M-021 (per-chunk feed + stream geometry + pending state), M-017
  (RTTI proof CalcPartition = geometry calculators + full AdjustPartitionInfoOrder
  decode + stable-sort note), M-022 (e490/ea30 bodies = CRefsResidentDataRunList —
  lead re-verified first-hand at :10413-10462; answers their fix-plan step 4),
  I-009 (pin test), I-010 (probe-superset row). Drops: my N-029 (dup of N-013b),
  my I-009/I-010/M-013/M-014/N-023/N-024 standalones.
- S-007 CORRECTED with first-hand decompile re-verification (lead, this session):
  C5 D0 D3 C6 = CEPSAnalyser AI/EPS at :1985 (NOT dBase — no DBF analyser exists);
  E4 52 5C 7B = OneNote OneAnalyser/FileONEAnalyser at :2156-2177 (NOT LZH, and the
  addendum's byte was 2C — wrong); LZH = list-layer LZHAnalyser at :3097. The other
  leg's S-007 addendum carried both wrong rows; correction addendum + errata 22-28.
- Evidence restored: s3-rawsearch-inventory, s3-calcpartition-scoring,
  s3-refs-engine-inventory, s3-ext4-inventory, s3-hfsplus-inventory + 4 execution
  dirs (calcpartition-vtables, ftyp-signature-deadness, ehb-ext4-dirwalk,
  ntfs-usn-reason) + agent logs + PROTOCOL.
- Module appends reconciled onto the remote legs' sections: carve.md (S-017/S-018/
  M-024/M-025/N-033 + S-007 correction), refs.md (S-020/M-026/M-027/N-036/N-037/
  I-013 + convergence notes + MATCH rows), lost-partition.md (M-017 addendum
  pointers + S-019/N-034/N-035).
- Tracked-open OQ-T9 section replaced: stale UNVERIFIED header → full RESOLVED
  (both legs' decodes; residues → S-019/N-034/N-035 + OQ-S2M-2).
- SUMMARY parallel-leg addendum: 14 new standalone findings table + merge map +
  A-002 audit-trail chain + counts (ACUTE 0 — A-004 fixed by c33236f; 13 SEVERE,
  22 MAJOR, 32 MINOR, 11 INFO = 78 active / 20 FIXED by file inventory).
- Verified at HEAD before writing: A-002 constants + de-self-referenced tests
  (cr_table_wire.h:67-73, test_crc32_gpt.cpp:394-395/:432-433, test_ldm.cpp:616-617),
  M-009 aligned_read_at routing (cr_transport.cpp:814).

Stage Summary:
- Both parallel legs' work preserved; zero findings lost; five addenda enrich this
  tree's findings; one cross-leg factual error (S-007 dBase/LZH rows) corrected with
  first-hand evidence; one cross-leg open question (M-022 special-handler bodies)
  answered. Counts: 78 active / 20 FIXED. Next: wave 2 on the TRUE gaps (see
  S3-W2 entry): ext4/HFS+/Btrfs findings from the restored inventories, NTFS
  remainder past S-012/N-024, ZFS/LDM remainder past M-019/M-016, orchestration/
  session/writer remainder — while the api-session leg works the API/driver/perf lane.
---
Task ID: 4-S-011 (SEVERE fix — second item of the fix session)
Agent: main engineer (diff-to-fix remediation, session 4)
Task: SEVERE S-011 — exFAT tz_offset applied (core) vs ignored (og); FileTime diverges by tz hours

Work Log:
- Independent verification BEFORE touching code (first-hand, not the audit's
  recorded output): ghidra-searchdll/exFATSearchDLL_imports.txt = zero
  LocalFileTimeToFileTime / FileTimeToLocalFileTime / GetTimeZoneInformation /
  SystemTimeToFileTime / FileTimeToSystemTime imports (only DosDateTimeToFileTime
  itself); zero *900 / *0x384 quarter-hour arithmetic anywhere in
  exFATSearchDLL_decompiled.c; six DosDateTimeToFileTime call sites
  (:5215-5235 x3, :45174-45194 x3) each passing only (date_word, time_word, &ft).
  Classification: CONFIRMED-DIVERGENCE.
- Consistency check that decided the fix shape: the core's FAT engine already
  carries DOS time through the shared FAT-family datetime core host-independent,
  on-disk-as-UTC, and holds a MATCH verdict for it - the exFAT engine was the only
  FAT-family member applying a TZ interpretation. The parity model (og-on-UTC0-host
  byte-equivalence, host-independent) is the established project semantics.
- Fix: tz subtraction removed from exfat_time_to_unix100 (signature reduced to
  (ts); three call sites updated); wire tz fields stay parsed with the og-parity
  note in cr_exfat_wire.h (parsed-for-completeness, deliberately unapplied).
- Tests: SetBuilder::file_entry gained a tz parameter (checksum-correct fixtures);
  build_exfat_image(tz=0) threads it into photo.jpg. Three guards:
  ExfatEngine.TimestampsMatchOgLocalAsUtc (tz=0x88, +2h), TimestampsZeroTzPassThrough
  (tz=0x00 sentinel), TimestampsMatchExplorerDisplay (tz=0x20, +8h East-Asia
  camera case). Fail-before proven by stashing only cr_exfat_quick.cpp: the two
  behavior tests FAIL on the pre-fix engine (stored 10:00 / 04:00), the sentinel
  guard passes both sides exactly as the finding predicted. Full suite 325/325
  Linux ASan (322 post-A-004/S-016 baseline + 3 new), clang-format 18.1.8 clean.
- Audit trail: S-011 Resolution + move to findings/FIXED/; RESOLVED-INDEX row
  (plus 6 missing rows repaired: A-004, S-002, S-010, S-016, M-009, N-001 were in
  FIXED/ without index rows - parallel-leg gaps; count header corrected to 19
  pre-S-011); modules/exfat.md B9 -> M with the fix note; SUMMARY (SEVERE 9->8,
  ACUTE row corrected for the already-landed A-004/S-016 c33236f fix: totals now
  63 active / 21 FIXED, remediation-log row, top-risks retirement, fix-order row);
  CROSS-REF-MATRIX exfat B/D cells -> M; docs/PARITY-DELTAS.md D-49 (the
  deliberate-divergence record: parity-over-spec decision, --strict-utc
  considered-and-deferred rationale, pin tests).
- Note: the finding's fix-plan pointer "docs/AUDIT-FALLBACKS.md exFAT section"
  does not exist (that doc has no exFAT section; it is the fallback/quality-gate
  census) - the canonical homes used instead: docs/PARITY-DELTAS.md D-49 +
  modules/exfat.md. Recorded in the Resolution.

Stage Summary:
- S-011 FIXED: exFAT timestamps now byte-match the og's tz-ignored semantics
  through the same datetime core FAT uses; the spec-correct UTC interpretation
  is retired to a documented deliberate divergence (D-49). Next: S-015 (raw-signs
  KiB units, one-generator fix), then S-013 (exFAT FAT-over-32MiB truncation) or
  S-003 (HFS+ journal replay).
---
Task ID: DF3-T4 (S-015)
Task: SEVERE S-015 — raw_signs min/max KiB-vs-bytes unit inversion (every raw-sign
carve window 1024x too small, floor 1024x too low); plus the cross-format tail-pairing
divergence discovered during the fix.

Work Log:
- Re-verified first-hand against the corpus decompile (not just the finding's quotes):
  FUN_1800205b0 :25225-25240 (max@+8, 0x7d000 default, span < max*0x400 gate, size =
  max<<10) and the tail-acceptance LAB_180020acb (computed size < (fmt->min << 10) ->
  reject: min is KiB too, consumed at acceptance). CONFIRMED-DIVERGENCE.
- Fail-before (worktree stashed to pre-fix source): 6 tests FAILED — UnitsAreKib
  (byte-unit table), CarvesFiveMiBVob (no node / wrong stub), TailPairWindowScaled +
  RawSignTailPairWithinFormat (no far pairing), MinFloorOneKib (sub-KiB junk admitted),
  RawSignLayerHeaderOnlyAndSharedMagic (cross-format tail pairing).
- Fix: (1) gen_rawsigns.py converts KiB->bytes at ONE point, documents the decoded
  semantics, refuses zero-max entries (og 500 MiB default = silent divergence risk);
  cr_rawsigns_data.cpp regenerated (139 formats, e.g. vob 10 MiB..200 MiB). (2) tail
  walk rebuilt: chunked forward scan (1 MiB + kMaxSignBytes overlap, straddle-safe;
  unreadable chunk -> honest header-only fallback), bounded by NAMED
  kRawSignTailScanLimit (8 MiB) with the og's true full-window range documented
  (cr_rawsigns.h) — the old 2 MiB single-read gate would have disabled ALL pairing
  under true units. (3) DISCOVERED + fixed: per-format tail scoping (og pairs tails
  within the candidate's own format record — our scan_tail matched all 62 signs
  across formats and fabricated close positions; proven live: a .sample candidate
  closed by flp's tail, and the session-1 fixture itself relied on the artifact).
  New scan_tail_for_format; carve wired to it. Debug artifact: corpus sign offsets
  are DECIMAL (apk tail off 28, not 0x28) — fixture math corrected accordingly.
- Two pre-existing carve fixtures re-based to true units (GrammarValidationAndSpecificity:
  jfif noise needs a 256 KiB region for its 100 KiB floor, extent now max-clamped;
  SecondPassFullyDeduped: sign-layer-live invariant preserved on the enlarged fixture).
- Pass-after: 6/6 rawsign tests green; full suite 310/310 Linux ASan (305 + 5 net-new).
- Audit trail in-commit: S-015 Resolution + FIXED/ move; RESOLVED-INDEX row (count 16);
  CROSS-REF-MATRIX carve C/A/X cells + session-3 row; SUMMARY remediation row;
  modules/carve.md session-3 addendum; cr_rawsigns.h divergence block rewritten;
  PARITY-CORE-REC F-A3-14 marked RESOLVED (named bound supersedes the 2 MiB nit).
- Honest not-done (recorded in the Resolution): og-execution recall benches
  (100 KiB / 5 MiB / 150 MiB seeded payloads) need the Windows bench harness —
  same session limitation class as M-009; registered for the bench vehicle.

Stage Summary:
- S-015 FIXED: the raw-sign safety net recovers og-scale windows again (50 KiB ->
  50 MiB class extents, 200 KiB -> 200 MiB for media), the sub-KiB junk floor is
  restored, far tails pair in-format, and the units are pinned by table-level tests.
  ACUTE tier empty; SEVERE remaining: S-003, S-006, S-007, S-009-remnant, S-011,
  S-012, S-013, S-014. Suite 310/310.

---
Task ID: DF3-T5 (S-017)
Task: SEVERE S-017 — ftyp-family signatures structurally unreachable (zero box-size
bytes baked into the magic): MP4/MOV/M4A/HEIC could never be grammar-carved.

Work Log:
- Verified first-hand: registry entries {0,0,0,0,'f','t','y','p'} len 8 offset 4;
  scan memcmp of the full 8 bytes (cr_carve.cpp) -> only a degenerate size-0 ftyp
  box can match, and even that mis-shifts the candidate by 4 (brand read lands on
  the "ftyp" bytes -> every validator rejects). Corpus: og carves these via the
  LIST layer (MovAnalyzer FUN_180004fb0 :2919-4208; HEICAnalyser; the u16-key 0x0000
  registration at :2748 is dead under the zero-header guard :9954) - CONFIRMED.
- Fix: (1) re-keyed all four entries on {'f','t','y','p'} len 4 header_offset 4 -
  the box SIZE at candidate+0 is data; validators now read the brand at
  candidate+8 where real files carry it. (2) match primitive extracted to
  match_signature (cr_signatures.h) - ONE source of truth shared by the scan
  dispatch and a NEW F8 load-time self-probe (self_probe_registry): reachable
  magic + exact candidate-shift assertion. Fail-before VERIFICATION taught the
  probe's design: a synthetic buffer faithfully REPRODUCES a broken magic (the
  naive probe PASSED with the broken table) - the structural rule from the
  finding (four-zero-byte magic prefixes = a 32-bit data field asserted constant)
  is the gate that catches the class. (3) fix-plan step 3, conservatively:
  FMT_MOV_NAKED (appended id; serialized values stable) keyed on a top-level
  "moov" box, validator = coherent box chain (moov sane + one following
  concrete-size box), conf 620, tried LAST in kTrialOrder; og accept criteria
  undecoded (pending-continuation :10114) - documented follow-up, NOT invented
  parity. (4) fixture lesson (test-side): BMFF heads must be a COHERENT box
  chain - a mis-sized ftyp swallows the mdat header and the walk mis-parses
  (caught via end=24 stubs); builder fixed (0x14 ftyp + concrete mdat).
- Fail-before/pass-after: pre-fix table restored -> 5/5 real-head tests FAILED
  (zero BMFF grammar hits) + probe FAILED (structural rule); post-fix 6/6 PASS;
  full suite 320/320 Linux ASan (314 + 6 net-new).
- Audit trail in-commit: S-017 Resolution + FIXED/ move; RESOLVED-INDEX row
  (heading 21); CROSS-REF-MATRIX carve grid row restored to full 11-cell form
  (my earlier S-015 conflict merge had collapsed it into a detail-row shape -
  reconstructed with both S-015 and S-017 cell updates) + S-017 detail row;
  SUMMARY remediation row; modules/carve.md addendum; PARITY-CORE-REC HEIC/7z
  clean-room bullet updated (the four formats are now reachable, not aspirational).

Stage Summary:
- S-017 FIXED: MP4/MOV/M4A/HEIC carve end-to-end for the first time, the
  unreachable-magic class is a load-time gate (self-probe), and ftyp-destroyed
  MOV/MP4 salvage conservatively mirrors the og's list layer. Suite 320/320.

---
Task ID: DF3-T6 (S-018 — reclassification)
Task: SEVERE S-018 re-verification — the JPEG EOI premise.

Work Log:
- Re-read the cited corpus artifact first-hand (RAWSearchDLL FUN_180033a90 :39489-41110
  and the legacy body :57080-57567), reproducing the finding's own reproduction step
  (grep for the EOI constant).
- REFUTED at the reproduction step: the finding's "zero occurrences of 0xd9" is a grep
  artifact — Ghidra renders the byte 0xD9 as the SIGNED CHAR literal -0x27 in
  char-typed compares. Five FF D9 sites exist inside the modern analyser body
  (rel. lines 217/231/262/480/501, incl. the shared backward-EOI helper), and the
  size-accumulation accept path is explicitly EOI-gated: `if (uVar21 == 0) goto
  LAB_1800342b6` (the epilogue) — no EOI pair in the buffer => no size => the
  opposite of the finding's premise. The legacy dispatcher body indeed has no EOI
  reference (that half stands) but it dispatches to JFIF/Exif/generic sub-analysers
  (FUN_18004bbd0/b3b0/c1e0) whose accept criteria are NOT decoded.
- Classification per the corpus-wins rule + severity discipline: INVALID as filed.
  NO code change — loosening a working EOI-gated validator on a refuted premise
  would be a premise-driven regression. The core's forward-walk EOI policy is
  consistent with the EOI-gated primary analyser as far as the corpus proves
  (backward-scan vs forward-walk admission width = open).
- Residual filed honestly, not dropped: tracked-open/OQ-S3-JPEG-fallback-and-
  continuation-accept.md — (1) decode the three legacy sub-analysers' accept
  criteria; (2) decode the streamed continuation outcome (early return != final
  reject; the 0x7800000 default). Either could resurrect the salvage claim with
  corrected citations, in which case the original S-018 fix plan becomes valid.
- Audit trail: evidence quote sheet (s3-jpeg-eoi-encoder-correction.md, full quotes
  + line cites); S-018 Resolution (INVALID) + move to FIXED/ (record preserved);
  RESOLVED-INDEX row (count 22); SUMMARY remediation row; CROSS-REF-MATRIX detail
  row; modules/carve.md addendum.

Stage Summary:
- S-018 INVALID: a false-positive SEVERE caught at its own reproduction step —
  the audit's grep missed Ghidra's signed-char rendering. The validator stands;
  the genuinely-open residual is precisely tracked with the exact decode list.

---
Task ID: API-T2+T3 (reading wave + gate docs)
Agent: main engineer (core-api session)
Task: Four-leg corpus/driver/core read; three gate documents produced and pushed BEFORE code.

Work Log:
- Four parallel read legs completed (corpus driver-usage; corpus API-facing; driver repo full;
  core repo full) — every load-bearing claim then re-verified first-hand: cr_transport.cpp read
  line-by-line (grammar open :617-624, token :285-289, F11 unscale :43-59, 64KiB direct envelope
  :144-171, RDA sector envelope :172-201, RwDevice routing :805-815, driver-absent logic
  open_disk :718-728), cr_directalign.h in full, PERFORMANCE-PROFILE/BENCHMARK-REPORT in full.
- KEY RECONCILIATIONS (corpus-wins):
  * The og's recovery path uses NO driver session engine (EPMNtDrv stateless; EUDCPEPM sessions
    are WinPE copy-only with zero in-tree DRW openers — eudcpepm-euedkepm--hook-drivers.md
    "Verified role + consumer analysis"). Core matches by NOT using sessions for scans.
  * The og's completion model is strictly synchronous (LPOVERLAPPED NULL at every raw site —
    Device_decompiled.c:57010/55940/56143). Core's effective-sync matches. Mandate's async rule
    satisfied trivially (reference issues none).
  * Driver-absent = silent direct fallback in the og (probe FUN_1800577e0 then same code path
    opens PhysicalDrive/HarddiskVolume) — core matches + observable transport string.
  * EUEDKEPM weakness mechanics verified (plain IoCreateDevice :577, no SDDL, self-derivable
    token) — our SDDL D:P(A;;GA;;;BA)(A;;GA;;;SY) hardening kept, documented.
- GATE DOCS WRITTEN (docs/):
  * API-SURFACE-AUDIT.md — 37 reference operations audited: 24 MATCH / 4 MATCH+ / 2 PARTIAL /
    2 MISSING (carve-format enumeration ops #7-#8) / 1 WRONG (M-023 category taxonomy,
    engine-scope) / 8 N/A-by-layer; 6 infrastructure findings (A-ABI-1 versioning contradiction,
    A-ABI-2 SQLite sidecar documented-but-absent → CUT as REJECTED-BY-REFERENCE, A-ABI-3..6 doc
    drifts); ordered completion plan.
  * DRIVER-UTILIZATION.md — per-interaction audit: open/addressing MATCH, request shape MATCH,
    completion MATCH, error mapping PARTIAL (missing og's error-6 self-healing reopen —
    Device_decompiled.c:55947-55959), session lifecycle MATCH (reconciled: no sessions in
    recovery on both sides), SDDL deliberate divergence kept, read granularity PARTIAL
    (core 64KiB blocks vs og 4MiB chunks — 16-64x transitions), push/pull MATCH, driver-absent
    MATCH, buffering MATCH. 64 KiB envelope unification decision: BOTH paths.
  * PERFORMANCE-BASELINE.md — driver microbench (532.9 MiB/s seq 64K, 8217 ns open, +52%
    fuzz dispatch, 64/64 SHA-identical), core local baselines, transition-count analysis
    (og 256-512/GiB vs core ~16,384/GiB sequential), 7-entry NOT-MEASURED plan with
    acceptance targets.

Stage Summary:
- Gates pushed before any code change (mandate §1). Next: T4+ implementation wave (carve-format
  enumeration, versioning policy fix, SQLite claim cut, 64 KiB unification, self-healing reopen,
  1 MiB sequential blocks, C-consumer expansion, Windows CI benchmarks).
Task ID: 4-S-006 (SEVERE fix — third item of the fix session)
Agent: main engineer (diff-to-fix remediation, session 4)
Task: SEVERE S-006 — lost-partition probes cover only NTFS/FAT/exFAT/GPT; the og's deep scan also finds Linux/FATX partitions

Work Log:
- Lane discipline: the parallel s4 leg declared S-013/S-015/S-014/S-012 (their worklog
  at 8034de5); this leg took S-006 (lost-partition module, no collision) and pulled
  before starting.
- Corpus verification BEFORE coding (first-hand): decoded the parser-tree wiring in
  PartitionRecoveryD_decompiled.c - CBlockParseWin32 ctor (FUN_180001c50) wires
  CFatxBlockParser + CNTFSBlockParser + CExFatBlockParser + BitlockerParser;
  CBlockParserLinux ctor (FUN_180001b40) wires CExtXBlockParser ONLY. FINDING
  CORRECTED (corpus wins): the og has NO lost-scan parsers for HFS+/Btrfs/ZFS/ReFS -
  the finding's impact claim that a deleted Mac/ReFS partition yields og candidates
  was a false positive; only the ext family is og coverage. CExtX accept criteria
  decoded in full: FUN_180002880 (magic 0xEF53@SB+0x38; nonzero inodes/blocks/bpg/ipg;
  first_ino>=3; group cross-check (blocks-fdb-1)/bpg+1 == (inodes-1)/ipg+1;
  first_data_block == (block_size==1024)) + FUN_180002b90 (block_group_nr==0 -
  partition-start anchor). FATX chain (FUN_180006250 -> 180004070/180004610) entered
  but not resolvable at reasonable depth - tracked as scoped remainder, no invented
  gates.
- Fix (cr_candidates.cpp): probe_ext (engine validate_superblock + the og's lost-scan
  gates + GDT liveness probe + SB-derived size); probe_hfsplus (VH@+1024, engine
  validator, total_blocks size); probe_btrfs (SB@+64KiB, engine validator,
  flagged-fallback size); probe_zfs (label-0 uberblock ring head @+128KiB, LE magic
  0x00BAB10C, txg liveness, flagged-fallback size); probe_refs (DBR OEM "ReFS"
  rescued from the FAT catch-all; detect_refs over a bounded 8-MiB window;
  flagged-fallback size). Dispatch: 55AA families first (ReFS inserted before the
  FAT catch-all which stays LAST), then ext/HFS+ (stride-1, in-window anchors),
  then btrfs/zfs (1-MiB grid alignment - out-of-window reads cost-bounded, D-50).
  CandidateFs appended Ext2..Refs (6..10); orchestrator resume bound extended
  (old sessions with values <= 5 stay valid).
- Tests: 7 new in test_lostpart.cpp (plant helpers per family, spec-exact fields:
  ext SB with consistent group formulas + GDT; HFS+ BE VH; btrfs SB with pow2
  sizes; ZFS uberblock with txg; ReFS DBR + MSB page). Fail-before proven by
  stashing only cr_candidates.cpp: 5 discovery tests FAIL (no candidates), the 2
  negative guards (block-group copy, noise) pass both sides by design. With the
  fix: suite 332/332 Linux ASan (325 + 7), two rebuilds, clang-format 18.1.8 clean.
- First attempt at fail-before used a stash that broke compilation (tests reference
  the new enum) - the old binary ran; redone with the enum/ABI changes unstashed
  so the pre-fix build compiled. Lesson recorded.
- Audit trail: S-006 Resolution (with the og-coverage correction + FATX/M-017
  scoping) + move to FIXED/; RESOLVED-INDEX row; SUMMARY (SEVERE 7 active /
  62 total, remediation log, top-risk retirement); CROSS-REF-MATRIX lost-partition
  row; modules/lost-partition.md; docs/PARITY-DELTAS.md D-50 (superset + alignment
  gate + fallback-size rationale + FATX remainder).

Stage Summary:
- S-006 FIXED: deleted ext4 partitions now surface as structured lost-partition
  candidates with the og's exact accept semantics; HFS+/Btrfs/ZFS/ReFS follow as
  documented supersets; ReFS DBRs are no longer dropped as FAT noise. Remaining in
  this leg's lane: S-003 (HFS+ journal replay) or S-007 (carve formats) - pull first
  to re-check the parallel legs.
