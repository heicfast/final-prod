# FOLLOW-UP — precise scope for every feature not landed by the harvest sessions

> Rows resolved IMPLEMENTED / DELIBERATELY-DIVERGENT / NOT-APPLICABLE live in
> `worklog/PARITY.md`. This sheet is the register of everything NOT implemented,
> each with the precise scope an implementer needs to pick it up: the reference
> behavior (with sheet citation), the product's entry points, the tests to add,
> and the validation that proves it. No stubs, no vague promises — a row here is
> a scoped work order, not a wishlist item.
>
> Product state at this sheet: post-F-01/F-33/F-34/F-03 harvest; engine `cr_mftwalk`
> parent gate landed; see `worklog.md` chronology for SHAs.
>
> **Implementation specs now exist for the P1 rows** (concurrent lane
> `f01f630a`, 2026-11-26): `worklog/VALIDATION/specs/F-02-mft-runlist-patch.md`,
> `worklog/VALIDATION/specs/F-12-13-14-19-21-testdisk-photorec.md`,
> `worklog/VALIDATION/specs/F-18-20-22-23-28-29-sf-halim.md` — behavioral
> detail extracted from the pinned reference clones with file:line citations.
> This sheet's scopes + those specs + `worklog/PARITY.md`'s verdicts are the
> complete pickup package for the remaining waves.

## P1 rows

### F-02 — $MFT run-list reconstruction + in-place patch (NTFS deep repair)

Reference: NR §5a-5d (`worklog/NTFS-RECOVER-FEATURES.md`; ntfs-recover
`repairer.rs:467 patch_record_data_runs`, delta-grouping `delta = disk_off −
rec#×rec_size`), 0BSD.
Entry points: `engine/src/core/repair/cr_rawfixer.cpp` (the consent-gated repair
seam, S1-S7; today record 0 is restored from the `$MFTMirr` slot-0 copy only),
`engine/src/core/ntfs/cr_runlist.cpp` (runlist encode/decode — the patch writes
record 0's unnamed `$DATA` run bytes), `cr_fixup.{h,cpp}` (un-fixup → patch →
USA bump → re-fixup).
Scope:
1. Scan the delta groups over the remnant-record map the walk already emits
   (constant `delta` per extent; cluster-align; drop <4-cluster fragments; merge).
2. Patch ONLY record 0's unnamed `$DATA` run bytes; sign-extension-safe run
   encoding (ntfs-3g sign-extends lengths — spare the sign bit); VCN cap via
   record 0's end_vcn preserves `$ATTRIBUTE_LIST`/`$BITMAP`.
3. Backup + confirm + dry-run; never rebuild record 0 from scratch (NR's
   proven failure); consent-gated, undo-file, the RawFixer transaction posture.
Tests: synthetic $MFT with torn record-0 runlist + remnant records proving the
delta grouping reconstructs the extent map (the test_ntfs_volume.cpp RecBuilder
harness extends naturally). Validation: a seeded VHDX with a corrupted record-0
runlist, scanned through `rrecov scan` + repair, engine-ci + fixture-lane
evidence.
Estimated: the largest single follow-up (a repair transaction, not a read-path
tweak).

### F-18 — carve registry breadth (294 → ~353 types)

Reference: TD §2.1 (353 types / 285 default-on; magic constants are format
facts, GPLv2+ — table re-derived, never copied).
Entry points: `engine/src/core/carve/cr_registry.cpp` (grammar registrations),
`cr_rawsigns_data.cpp` (the raw layer-1 table: 139 entries today), per-type
validators in `cr_analysers.cpp`.
Scope: the ~59-type gap, enumerated by diffing TD's `src/file_*.c` basename
inventory (the TD sheet lists the count; the type list is re-derived from the
PhotoRec documentation page each family cites, not the C files) against the
product's registry; each type needs (a) magic + ext, (b) a real header/footer
validator or an honest raw-sign confidence class, (c) a fixture sample test,
(d) the doc-truth count updates (`engine/docs/CAPABILITY-MATRIX.md`, the
contracts type maps). Deliver as a family-batched series (photos → docs →
archives → media → data), each batch with its own tests.

### F-19 — bounded fragmentation reassembly (opt-in carve pass)

Reference: TD §2.5 (walk remaining space backwards, split points ±6 sectors /
2 blocks of the error offset, next-header jumps, ≤5 fragments, +1000-block pad
re-validate; GPLv2+ — behavior only).
Entry points: `engine/src/core/carve/cr_carve.cpp` (the windowed scan pass),
`cr_analysers.cpp` (per-family end detection that surfaces the error offset).
Scope: an opt-in second pass (flag on the carve config) that retries families
whose validator fails mid-file with a bounded split-point search over the
remaining window; strict caps (fragments ≤5, bounded backtrack); the outcome
must grade Salvaged/Partial with the fragment count in `Item.note` — never a
silent full-content claim. Tests: a synthetic image with a file fragmented by
one interposed foreign block.

### F-28 — FAT32/exFAT orphan-directory subtree harvesting

Reference: HR §2.4/§2.5, SF §2.6 (`worklog/HALIM-FEATURES.md`,
`worklog/SECTORFORGE-FEATURES.md`; both MIT).
Entry points: `engine/src/core/fat/cr_fat_quick.cpp` (deleted-slot +
deleted-chain passes), `engine/src/core/exfat/cr_exfat_quick.cpp` +
`cr_exfat_dentry.cpp`.
Scope: a directory whose first cluster is FREE (or a 0xE5 slot with a free
cluster) is a whole deleted subtree — children entries were never rewritten;
strict parse gates (must start with `.`/`0xE5` entry, stop at implausible
dentry, cluster bounds 64-256), FAT-correct chain walk, bitmap/FAT authority
(F-29: for deleted sets the FAT is stale — exFAT's allocation bitmap is
authoritative; NoFatChain → exact layout). Tests: fixture with a deleted
directory whose FAT entries were zeroed but the cluster chain is walkable from
the dentry side.

### F-37 — imaging presets + health-map UX

Reference: SF §2.1 (Healthy 4 MiB/1 retry; Some bad 1 MiB/3; Badly damaged
64 KiB/16 MiB jumps/5 s timeout; Thorough 128 KiB/8/30 s; 1000-cell worst-status
map at 1 Hz; map viewer with Resume/Retry-bad-areas), MIT.
Entry points: `ui/src/features/device-selection/create-image-dialog.tsx` (the
imaging dialog), the engine's rescue imager (`engine/src/core/storage/cr_rescue.{h,cpp}`
— chunk-size/skip-size/retries knobs already exist; the CLI surface
`rrecov image --create --rescue [--chunk-size --skip-size --retries]` already
exposes them), the bridge `commands/images.rs`.
Scope: the four presets as a radio group mapping to (chunk, skip, retries,
timeout) tuples passed through the bridge into the imager config; the
1000-cell worst-status map driven off the mapfile status the imager already
persists; the map viewer + Resume action riding the existing resume contract.
Tests: connector-mock + tauri shapes; the engine side needs NO change beyond
config plumbing (the semantics are already proven by F-33/F-34 tests).

## P2 rows

- **F-04** USN fixup all-zero rejection (`cr_fixup.cpp:fixup_validate` — a
  zero `usn` with all-zero tails passes today; SF §2.5: reject the zero-USN
  record outright). Note the deliberate tension: the product's fixup is
  og-parity salvage-forward (two-tail check, `FUN_1800411f0`); the F-04 rule
  is SF strictness. Land it as a gate on `usn == 0` with a test pinning both
  sides (zero record rejected, salvage cases unaffected).
- **F-05** $ATTRIBUTE_LIST extension ownership match (`cr_mftwalk.cpp`
  multi-hop BFS M-008 — adopt extension only if base_ref record+sequence AND
  attrlist-entry sequence match the base).
- **F-06** NTFS index-slack deleted names (SK §4: entry UNALLOC if parent
  deleted / strlen==0 / end > endaddr_alloc; advance by guessed idxlen). Entry
  point: the INDX remnant pass in `cr_mftwalk.cpp` (harvests in-use entries
  only today; slack never harvested by design — reopening that is an og-parity
  decision to record).
- **F-13** quick-mode scan lattice + hint list (TD §1.5; the 1 MiB grid /
  2048·512 / 32/63 / GPT-Intel offsets; rides the existing lost-partition
  probe in `src/core/lost/`).
- **F-14** GPT table dual-write repair (TD §1.3; rebuild primary+backup
  headers + entries + protective MBR, disk GUID preserved, CRCs recomputed;
  consent + undo via the RawFixer transaction seam).
- **F-15** $MFTMirr repair adjudication (TD §1.10: compare first
  max(cluster, 4×record) bytes; pick the winner by listing files through each
  copy; mirror-over-MFT flagged DANGEROUS).
- **F-20** two-level signature index (256 first-byte buckets, longest-first,
  first-match-wins — verify the current dispatch and adopt if linear).
- **F-21** auto block-size detection (safe-header-only pre-pass, stop at 10
  headers, halve from 64 KiB until all starts congruent).
- **F-22** remaining structural end-detectors: PDF incremental %%EOF,
  MP4 moov+mdat completeness, OLE FAT-based sizing (JPEG/ZIP/TIFF/PST/MOV
  gates already exist).
- **F-29** exFAT allocation-bitmap authority for deleted sets (with F-28).
- **F-30** ext deleted dentry-slack names (advance by MINIMUM rec_len;
  dellen gap tracking; inode ≤ max, sane rec_len, no NUL/slash, filetype 1..7).
- **F-36** sparse-destination on reverse imaging — LANDED with F-33/F-34
  (FSCTL_SET_SPARSE best-effort); remaining sliver: FAT/exFAT-destination
  advisory when sparse is unavailable (a note in the imaging report).
- **F-38** map-as-prior honest-partial reads for the LIVE-device carve path
  (RetryingDevice zero-fill + ReadErrorMap exist; feeding a persisted rescue
  mapfile in as the read prior is the gap).
- **F-39** SCSI READ(16) / ATA READ DMA EXT read-path modes with per-mode
  timeouts (ATA passthrough exists for SMART; the read path is OS-mode only).
- **F-40** E01/AFF/split-raw image reading (SK §6; EWF specs are public;
  libewf-style behavior re-implemented; rides the image reader in
  `src/core/storage/cr_image.cpp`).
- **F-44** evidence-weighted numeric health score (0-100; reuse 55 / structure
  35 / layout 10; unknown → 25; reuse ≥0.99 → RED ≤10) + HealthNotes journal.
- **F-54** MFT 2 MiB batched reads (D-51 registered divergence; measure
  before adopting — the IOCP ring may already amortize it).

## P3 rows

- **F-07** record-number triple fallback (fragment-map → header → positional).
- **F-08** sequence-aware record dedup vs single_link_per_record (M-007) —
  cross-check the tie-break order.
- **F-16** EBR chain caps (32-logical cap; 1 data + 1 link per EBR; the
  visited-set cycle guard exists — caps unverified).
- **F-17** Mac/BSD/Sun partition systems (MBR/GPT/LDM only today).
- **F-24** embedded-filename rename post-recovery (printable-char scoring).
- **F-25** low-memory forget() behind the scan position.
- **F-41** same-drive guard "can't tell = refuse" — the unknown-backing case.
- **F-42** per-drive verify-history + startup survey banner.
- **F-47** natural-language smart filter (offline TR/EN phrase → FileFilter).
- **F-48** mmls-style slot/start/end/length presentation (own table exists —
  cosmetic parity only).
- **F-49** timeline / body-format export.
- **F-50** hash-database lookup (NSRL/md5sum/SQLite).
- **F-51** million-row column store + batched events (virtual list exists).
- **F-56** FAT per-sector copy fallback (all fail → 0xFF allocated —
  "damage never makes a file look better").
- **F-57** distinct encryption-signature errors (LUKS/FileVault/Safeboot…).
- **F-58** defensive hard caps cross-check (dir-size cap, walk depth 128,
  path 4096, GPT entries 8192).

## Standing exclusions (reaffirmed, not pending)

- XPRESS/LZX decode (no reference implements it; `worklog/PRODUCT-STATE.md` §5).
- $LogFile replay (F-09: no reference implements it — NR §3.3 truth table;
  product's I-049 register).
- MFT slack carving (F-11: mft-rescue's truth table says NOT FOUND IN CODE).
