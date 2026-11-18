# FEATURE-MATRIX — Cross-Repository Harvest (master extraction)

> Agent H (Task 2-H/T3), consolidated from the seven Phase-0 context sheets at HEAD `22dcfddb` (sheets committed `d7b8ad65`, rebased over `795f5328`). Date: 2026-11-19. Product baseline: `201100aa` (v0.9.5) per `worklog/PRODUCT-STATE.md`. Every row cites its context sheet (which cites reference source files); nothing here is implemented yet — per mandate, implementation starts only after this matrix is committed and pushed.

## How to read

- **Source**: SF=SECTORFORGE-FEATURES.md, HR=HALIM-FEATURES.md, TD=TESTDISK-FEATURES.md, SK=SLEUTHKIT-FEATURES.md, NR=NTFS-RECOVER-FEATURES.md, MR=MFT-RESCUE-FEATURES.md, PS=PRODUCT-STATE.md. § numbers are the sheet's sections.
- **Product status @201100aa**: EXISTS (done, cite PS §) / EXISTS-PARTIAL (present with named deltas) / MISSING / AHEAD (product exceeds all references) / N/A.
- **Priority**: P0 = core recovery capability, P1 = high-value, P2 = valuable, P3 = nice-to-have (session prompt Phase 2).
- **Wave**: the implementation wave (1-9) the row lands in.
- **License note**: MIT/0BSD = behavioral harvest with citation; GPLv2+ (testdisk) and IPL 1.0 (sleuthkit core) = behavioral re-implementation ONLY — no code, no verbatim tables/threshold strings; on-disk format facts are uncopyrightable. Full per-feature decisions: `worklog/LICENSING.md`.

## The named feature (priority of priorities — Wave 5)

**Reconstructed files with original directory structure preserved, DELETED ONLY, as a post-scan option — matching SectorForge's behavior exactly.**
Reference behavior (SF §3, verified at `crates/sf-core/src/ntfs/extract.rs:1-4,14,90-257` + `sf-cli/src/recover.rs:224-253`): post-scan flow "List files → tick files/folders → Extract selected"; directory reconstruction is inherent to extract (ancestry → `dir_for`, orphans → `_orphaned/` with id-suffixed orphan roots so two orphaned "Documents" folders never merge); `--deleted-only` is a listing-side pre-filter (`recover.rs:241-243`) that never changes extraction semantics; never overwrites anything (collision → `name (id N).ext`, then fail); every path component sanitized cross-OS (reserved names, trailing dots/spaces, UTF-16 cap 240); created dirs canonicalized and refused if they resolve outside the output; per-item Complete/Incomplete/Failed outcome + plain-English notes (compressed/encrypted → empty + note; unreadable bytes counted; reused clusters noted); FS-agnostic through the `Volume`/`Index` abstraction (NTFS/FAT/exFAT/ext same path).
Product gap (PS §2.9, §5): SafeWriter already reconstructs directory structure recursively (G-08) with name_N collisions and truthful Salvaged outcomes, and the results explorer has deleted-only filtering — but there is **no post-scan "reconstruct deleted files with original folder structure" option** with SectorForge's semantics (orphan-root id suffixing, `(id N)` collision scheme, never-overwrite-with-failure-not-silent-rename, per-item outcome events with content-honesty notes, deleted-only listing fast-path). This is matrix row **F-01** and gates Wave 5.

## Master matrix

### Wave 1 — Foundations (verify + extend what already exists)

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-00a | Recovered-entry node + provenance | PS §2.5 | FileNode/FileProperties with Origin/DeletedState/Confidence/Grade, RAW provenance (M-051), codec-tagged extents | — | EXISTS | verify | ours |
| F-00b | Tree model + deleted/live classification | PS §2.5 | ResultTree roots (active/deleted/lost/carved), "File Path Lost" orphan root, LostDir groups (M-103) | F-00a | EXISTS | verify | ours |
| F-00c | Parser primitives + abstract access layer | PS §1.2 | IStorageDevice → Slice/Retrying/CachedReader/DecryptingDevice; 64 KiB direct envelope; per-module error space | — | EXISTS | verify | ours |
| F-00d | Extract-plan model for the named feature | SF §3 | plan(): DFS parents-before-children, `seen` set, metadata never implicitly pulled, unknown ids surface as failures | F-00a | MISSING | P0 (feeds F-01) | SF MIT |

### Wave 2 — NTFS deep recovery (NR + MR + SK + SF)

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-02 | $MFT run-list reconstruction + in-place patch | NR §5a-5d | Delta-grouping (`delta = disk_off − rec#×rec_size` constant per extent), cluster-align, drop <4-cluster fragments, merge; patch ONLY record 0's unnamed $DATA run bytes (un-fixup → patch → USA bump → re-fixup); sign-extension-safe run encoding (ntfs-3g sign-extends lengths — spare sign bit); VCN cap via record 0 end_vcn preserves $ATTRIBUTE_LIST/$BITMAP; backup + confirm + dry-run; never rebuild record 0 from scratch (proven failure) | F-00c; consent-gated repair seam (RawFixer S1-S7, PS §2.11) | MISSING (RawFixer restores record 0 from $MFTMirr slot 0 only, PS §2.3/§2.11) | P1 | NR 0BSD |
| F-03 | Parent-sequence validation | SF §2.5; NR §4 (gap) | Accept parent iff pseq == parent.seq, or pseq+1 for a deleted parent (delete bumps seq); parent must be a dir, not self. NR explicitly lacks this — a reimplementation must close it | F-00b | UNVERIFIED in product walk (PS §2.3 silent) → verify + close | P1 | SF MIT |
| F-04 | USN fixup all-zero-sector rejection | SF §2.5 | A sector of all zeros must fail fixup (unreadable sectors read as zeros and would pass a zero USN) | F-00c | UNVERIFIED (product has fixup + tests, PS §2.3) → verify | P2 | SF MIT |
| F-05 | $ATTRIBUTE_LIST extension ownership | SF §2.5 | Adopt extension record only if base_ref record+sequence AND list-entry seq match the base (post-delete reuse guard); missing pieces → incomplete | F-00b | EXISTS-PARTIAL (multi-hop BFS chase M-008; ownership-match rule unverified) | P2 | SF MIT |
| F-06 | NTFS index-slack deleted names | SK §4 | Names in $INDEX_ALLOCATION slack: entry UNALLOC if parent deleted, strlen==0, or end > endaddr_alloc; advance past deleted entry by guessed idxlen = align4(16+66+2·nlen) | F-00c | MISSING (product name recovery is MFT $FILE_NAME-based, PS §2.3) | P2 | SK IPL → behavior only |
| F-07 | Record-number triple fallback | NR §2.4 | Fragment-map lookup → header record# → positional; resumable scan index | F-02 | EXISTS-PARTIAL (remnant scan emits records; numbering strategy differs, PS §2.3) | P3 | NR 0BSD |
| F-08 | Sequence-aware record dedup | NR §3.3 | Same record number: in-use beats deleted, then highest sequence | F-00b | EXISTS-PARTIAL (verify vs single_link_per_record M-007) | P3 | NR 0BSD |
| F-09 | $LogFile analysis/replay | MR §3; SK §4 | **NOT FOUND in either reference** (SK parses LSN only; MR never mentions it). Product absence is documented (I-049: the og's own journal machinery is dead weight) | — | N/A — decision row | N/A | — |
| F-10 | $UsnJrnl deleted-name harvest | PS §2.3; SK §4 | Product harvests V2/V3 filtered on USN_REASON_FILE_DELETE; SK usnjls is V2-only display | — | AHEAD | no action | — |
| F-11 | MFT slack-space carving | MR §5 | **NOT FOUND in code** (aligned scan only; `_detect_record_size` is measuring, not carving). No reference implements it | — | N/A — decision row (product remnant scan + GUID-dump pass already covers adjacent ground, PS §2.3) | N/A | — |

### Wave 3 — Partition layer (TD + SF + SK)

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-12 | Backup boot-sector position lattice | TD §1.5, §1.9 | FAT backup at sector 7 (per 1 MiB unit), exFAT at sector 13, NTFS/HFS at last-sector-of-track + (2048−1)·512 per MiB, ext2 group-3 backup superblocks (1K/2K/4K); identity compare ignoring FAT32 0x3E8-0x3F0; NTFS-backup back-projection (probe 32..1 sectors before found offset) | lost-probe framework (PS §2.1) | EXISTS-PARTIAL (GPT backup salvage closed; NTFS boot_from_backup M-043 report-only; FAT/exFAT backup-BS probe positions absent) | P1 | TD GPLv2+ → facts free, lattice re-derived |
| F-13 | Quick-mode scan lattice + hint list | TD §1.5 | Head 0/1/2 sector-1 + 1 MiB grid; hints: partition ends, 2048·512 Vista, 32/63, GPT-as-Intel 2·sec+16384, RAID follow-ups; quick mode jumps to found partition end | F-12 | EXISTS-PARTIAL (64 KiB window stride full sweep M-137 — thorough but slower; no hint acceleration) | P2 | TD GPLv2+ → behavior only |
| F-14 | GPT table repair (dual write) | TD §1.3 | Rebuild primary+backup headers + entries + protective MBR, disk GUID preserved, CRCs recomputed; consent + undo | apply seam (PS §2.11) | MISSING (GPT-damaged reported, not reconstructed — PS §2.1 edge) | P2 | TD GPLv2+ → behavior only |
| F-15 | MFT mirror repair adjudication | TD §1.10 | Compare first max(cluster, 4×record) bytes; pick winner by listing files through each copy (redirect); mirror-over-MFT flagged DANGEROUS | F-00c | EXISTS-PARTIAL (fscheck $MFTMirr proofs + RawFixer record-0 restore; no adjudication heuristic) | P2 | TD GPLv2+ → behavior only |
| F-16 | EBR chain caps | TD §1.2 | 32-logical cap; 1 data + 1 link per EBR; bounds containment; >1023-cyl type switch on re-synthesis | — | EXISTS-PARTIAL (visited-set cycle guard + torn-chain salvage M-002; caps unverified) | P3 | TD GPLv2+ → behavior only |
| F-17 | Mac/BSD/Sun partition systems | SK §2; TD §1.1 | SK supports dos/mac/bsd/sun/gpt; TD adds xbox/humax | — | MISSING (MBR/GPT/LDM only, PS §2.1) | P3 | SK IPL/TD GPL → behavior only |

### Wave 4 — Carving (TD/PhotoRec + SF + HR)

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-18 | Signature registry breadth | TD §2.1 | 353 types / 285 default-on / 930 registered signatures; magic constants are format facts | registry seam (PS §3.6) | EXISTS-PARTIAL (155 grammar + 139 raw + user = 294; breadth gap vs 353) | P1 | TD GPL → constants free, table re-derived |
| F-19 | Bounded fragmentation reassembly | TD §2.5 | Opt-in brute-force pass: walk remaining space backwards, try split points ±6 sectors/2 blocks of error offset, skip −2..5000 blocks (next-header jumps), ≤5 fragments, pad +1000 blocks re-validate; normal passes stop at first gap | carve engine (PS §2.4) | MISSING (format-specific structures only; "prime harvest surface" PS §2.4) | P1 | TD GPLv2+ → behavior only |
| F-20 | Two-level signature index | TD §2.1 | Offset-keyed nodes → 256 first-byte buckets, sorted longest-first, first-match-wins — O(1) dispatch per block | F-18 | UNVERIFIED (kTable dispatch mechanism not detailed in PS) → verify, adopt if linear | P2 | TD GPL → behavior only |
| F-21 | Auto block-size detection | TD §2.2 | Safe-header-only pre-pass, stop at 10 headers, halve from 64 KiB until all starts congruent | F-19 | MISSING (product scans every sector in 1 MiB windows, PS §2.4) | P2 | TD GPL → behavior only |
| F-22 | Targeted structural end-detectors | SF §2.8; HR §2.6 | PDF last-%%EOF incremental-update-aware; MP4 moov+mdat completeness; OLE FAT-based sizing + type from root-storage names; ZIP EOCD cd_off+cd_size==eocd_pos (already CARVE-2); TIFF IFD walk raw-typing | F-18 | EXISTS-PARTIAL (JPEG/ZIP/TIFF/PST/MOV gates exist; PDF-incremental, MP4-completeness, OLE-FAT-sizing unverified) | P2 | SF MIT / HR MIT |
| F-23 | Carve outcome honesty | SF §2.8 | complete=false when end not found / runs off source / overlaps unreadable range; per-type max_size false-match rejection; next-header termination for headerless kinds | F-18 | EXISTS-PARTIAL (confidence + Salvaged exist; complete/incomplete surface unverified) | P2 | SF MIT |
| F-24 | Embedded-filename rename | TD §2.7 | Post-recovery rename from embedded names (zip/doc), printable-char scoring decides keep/drop | F-18 | UNVERIFIED → likely MISSING | P3 | TD GPL → behavior only |
| F-25 | Low-memory forget() | TD §2.6 | Free search-space history >10000 elements behind current position | F-19 | UNVERIFIED | P3 | TD GPL → behavior only |
| F-26 | Whole/free-space gating | TD §2.6 | PhotoRec free-only shrinks search space per-FS | — | AHEAD (free-map cluster gate M-005 + occupancy gate M-039, PS §2.4) | no action | — |
| F-27 | Dedup + result merging | PS §2.4 | (first-cluster, size-class, hash) + resume-safe extent-key dedup | — | AHEAD | no action | — |

### Wave 5 — The named feature + deleted-tree edge cases

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-01 | **Named feature**: deleted-only reconstruction with original directory structure, post-scan option | SF §3 | See callout above: extract plan, dir_for ancestry, `_orphaned/` + id-suffixed orphan roots, ` (id N)` collisions, never-overwrite, cross-OS sanitize (reserved names, trailing dots/spaces, UTF-16 240), canonicalize-inside-out guard, per-item Complete/Incomplete/Failed + notes, FS-agnostic, deleted-only listing pre-filter | F-00d; SafeWriter (PS §2.9); ABI append (PS §3.3); connector domain (PS §3.5) | MISSING (components exist: G-08 dir reconstruction, name_N collisions, deleted filter; the cohesive post-scan option with SF semantics does not) | **P0** | SF MIT |
| F-28 | FAT32/exFAT orphan-directory subtree harvesting | HR §2.4/§2.5; SF §2.6 | Live-looking dir whose first cluster is FREE (or 0xE5 dir with free cluster) → whole deleted subtree recoverable (children entries never rewritten); strict parse (must start with `.`/0xE5, stop at implausible), ≤64-256 cluster bounds, bitmap/FAT authority | FAT/exFAT engines (PS §2.2) | UNVERIFIED → likely MISSING (product has deleted-slot + deleted-chain, not subtree harvesting) | P1 | HR MIT / SF MIT |
| F-29 | exFAT allocation-bitmap authority | HR §2.5 | For deleted sets the FAT is stale; the allocation bitmap is authoritative for cluster state; NoFatChain → exact layout note | F-28 | UNVERIFIED (parked-set oracle exists M-013; bitmap authority unverified) | P2 | HR MIT |
| F-30 | ext deleted dentry-slack names | SK §5; SF §2.7 | Advance by MINIMUM rec_len (not recorded) to see deleted entries; dellen gap tracking → UNALLOC; validate inode ≤ max, sane rec_len, no NUL/slash, filetype 1..7; skip if inode links≠0 (rename leftover) | ext4 engine (PS §2.2) | MISSING (product: deleted-inode pass i_dtime + journal salvage only) | P2 | SK IPL / SF MIT → behavior only |
| F-31 | Deleted-only listing fast-path | SF §3 | `--deleted-only` skips in-use entries at listing time ("quicker on very large volumes"); GUI starts deleted-only runs in flat mode | F-01 | MISSING | P1 (part of named feature) | SF MIT |
| F-32 | FAT LFN deleted-slot checksum recovery | PS §2.2 | Product recovers slot-set + first char via checksum brute-force (GAP-S6-1 closed) | — | AHEAD (HR/SF replace first char with `_` only) | no action | — |

### Wave 6 — Imaging & drive health (SF + MR + SK)

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-33 | Two-pass ddrescue-style imaging | SF §2.1 | Pass 1 copy: 1 MiB chunks, on failure mark skip (max(skip,chunk)) NonTrimmed and jump; reverse mode (find_prev, sparse dest); pass 2 retry: binary-split damaged regions to sector granularity, ≤N retries per sector, checkpoint+progress INSIDE damaged regions | producer loop seam (PS §3.9) | MISSING (single-pass; read failure aborts with committed prefix, PS §2.6) | **P0** | SF MIT |
| F-34 | Rescue map persistence + resume | SF §2.2 | ddrescue-compatible mapfile (NonTried/NonTrimmed/NonScraped/BadSector/Finished); atomic tmp+rename (5×50ms PermissionDenied retry); dest-flush-before-map; 5 s cadence; resume refuses map/dest size mismatch; status --map block list for UI | F-33 | MISSING (ReadErrorMap exists scan-side only, PS §2.6) | **P0** (part of F-33) | SF MIT |
| F-35 | Imaging pause/resume | SF §2.1-2.2 | Kill-safe: map survives; resume re-runs same command; `.partial`-style continuation | F-34 | MISSING (`.partial` kept but not continuable, PS §2.6) | P1 | SF MIT |
| F-36 | Sparse destination on reverse imaging | SF §2.1 | FSCTL_SET_SPARSE so first write at end doesn't zero-fill whole image; best-effort on FAT/exFAT | F-33 | MISSING | P2 | SF MIT |
| F-37 | Imaging presets + health-map UX | SF §2.1 | Healthy (4 MiB/1 retry) / Some bad (1 MiB/3) / Badly damaged (64 KiB/16 MiB jumps/5 s timeout) / Thorough (128 KiB/8/30 s); 1000-cell worst-status map at 1 Hz; map viewer with Resume/Retry-bad-areas | F-34; UI (PS §3.5) | MISSING | P1 | SF MIT |
| F-38 | "Honest partial" reads with map-as-prior | SF §2.3 | Unreadable = zero-fill + counted; map holes unreadable WITHOUT touching device; binary-split granularity (64 KiB for live-device carve) | F-34 | EXISTS-PARTIAL (RetryingDevice zero-fills + ReadErrorMap; map-as-prior absent) | P2 | SF MIT |
| F-39 | Direct-access modes (OS/SCSI/ATA) with timeouts | SF §2.9 | SCSI READ(16) + ATA READ DMA EXT passthrough, per-mode TimeOutValue, ≤64 KiB command split, up-front probe with per-mode hints | win transport (PS §1.2) | EXISTS-PARTIAL (ATA passthrough exists for SMART; read-path OS-mode only) | P2 | SF MIT |
| F-40 | E01/AFF/split-raw image reading | SK §6 | EWF + AFF + split-raw segment naming matrix; 3-pointer img plug-in; ambiguous-detect errors | image reader (PS §2.6) | MISSING (flat/VMDK/VHD/VHDX only; product AHEAD on VHDX log-replay) | P2 | SK IPL → behavior only; EWF format specs public |
| F-41 | Same-drive guard "can't tell = refuse" | SF §2.10 | Refuse outputs whose backing disk cannot be determined, with plain-English copy | — | EXISTS-PARTIAL (F12 extent-based hard refusal; unknown-backing case unverified) | P3 | SF MIT |
| F-42 | Drive inspector verify-history + survey banner | SF §2.10 | Per-drive SHA-256 verify history; startup survey error banner with "image it first" primary action | — | EXISTS-PARTIAL (inspector + health UX exist NG-5..10) | P3 | SF MIT |
| F-43 | Single-pass + inline sector salvage imager | MR §8 | 1 MiB chunks, sector-by-sector salvage on chunk error, bad-ranges in atomic `<out>.progress.json`, --resume (never re-retries failed ranges) | — | SUBSUMED by F-33/F-34 (SF design strictly stronger; MR format simpler) | N/A — superseded | MR MIT |

### Wave 7 — UX and output (SF + HR + SK)

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-44 | Evidence-weighted health scoring + notes journal | HR §2.7 | 0-100 confidence: reuse 55 / structure 35 / layout 10; unknown → 25; reuse ≥0.99 → RED ≤10; user-visible HealthNotes evidence strings | grading (PS §2.5) | EXISTS-PARTIAL (4-tier Confidence + og grading tree; numeric evidence model + notes journal absent) | P2 | HR MIT |
| F-45 | Per-item outcome badges + content-honesty notes | SF §3 | Complete/Incomplete/Failed per item; notes for compressed/encrypted/unreadable/reused; tooltips | F-01 | EXISTS-PARTIAL (Done/Salvaged + report evidence; per-item note surface partial) | P2 (part of named feature) | SF MIT |
| F-46 | Orphaned files pseudo-folder | SF §3 | `:orphans:` group, collision-proof keying | — | EXISTS (File Path Lost root M-103) | no action | — |
| F-47 | Natural-language smart filter | HR §2.15 | Offline TR/EN phrase → FileFilter (category, extension, time/season ranges, stop-words) | query layer (PS §2.5) | MISSING (generated filefilter vocabulary exists; NLQ absent) | P3 | HR MIT |
| F-48 | mmls-style slot/start/end/length presentation | SK §2.4 | Sorted partitions + synthesized Unallocated gaps + Meta rows | — | EXISTS-PARTIAL (own table UI + diskmap) | P3 | SK IPL → behavior only |
| F-49 | Timeline / body-format export | SK §3.7 | 11-field pipe body, macb letters, NTFS SI+FN double rows | — | MISSING | P3 (forensic output) | SK IPL → behavior only |
| F-50 | Hash-database lookup | SK §7.1 | NSRL/md5sum/SQLite formats, vtable, index-of-index | — | MISSING | P3 (forensic) | SK IPL → behavior only |
| F-51 | Million-row column store + batched events | SF §2.11 | Parallel arrays, shared folder strings, cached sorts, incremental search index; 2000 rows/80 ms batching | — | EXISTS-PARTIAL (virtual list + coalesced events; column-store unverified) | P3 | SF MIT |
| F-52 | Recovery reports (text + JSON) | HR §2.10 | Timestamped txt+json with per-item SHA-256/confidence | — | AHEAD (cr_report.cpp, schema CI-validated, PS §2.9) | no action | — |

### Wave 8 — Persistence, session, performance

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-53 | Session save/resume cadence | TD §2.9 | photorec.ses per pass + 5 min, deleted on completion | — | AHEAD (CRSF + checkpoints + stop-window autosave N-211, PS §2.8) | no action | — |
| F-54 | MFT batched reads (2 MiB) | PS §1.4 (prior sheet); og parity D-51 | Og batches MFT records at 2 MiB; product reads per-record under 256 KiB cache blocks | — | EXISTS-PARTIAL (registered divergence D-51) | P2 | ours (og parity) |
| F-55 | Read sizing / concurrency / buffering | SF §2.9; TD §2.3 | SF aligned buffers + 4 MiB envelope; PhotoRec 2×blocksize sliding window | — | AHEAD (IOCP ring + direct envelope + CachedReader, PS §1.2) | no action | — |

### Wave 9 — Hardening

| ID | Feature | Source | Behavior essence | Prereq | Product status | Pri | License |
|---|---|---|---|---|---|---|---|
| F-56 | FAT per-sector copy fallback | SF §2.6 | Unreadable sector of FAT copy n re-read from copy n+1; all fail → 0xFF (allocated — "damage never makes a file look better") | — | UNVERIFIED (table-level choose-healthy exists) | P3 | SF MIT |
| F-57 | Distinct encryption-signature errors | SK §6 | LUKS/FileVault/Safeboot… signatures → distinct error codes, never "unknown" | — | EXISTS-PARTIAL (BitLocker FVE distinct; others generic) | P3 | SK IPL → behavior only |
| F-58 | Defensive hard caps | SK §10 | dir-size cap, walk depth 128, path 4096, GPT entries 8192, recurse 64 | — | UNVERIFIED (product has own caps; cross-check) | P3 | SK IPL → behavior only |

## Decision register (resolved without implementation)

| Row | Decision | Rationale (cited) |
|---|---|---|
| F-09 $LogFile | **NOT-ADOPTED** | No reference implements it (MR §3 NOT FOUND; SK §4 LSN-only). Product's documented absence stands (I-049 register, PS §2.3). Revisit only if a future reference supplies a real algorithm. |
| F-11 MFT slack carving | **NOT-APPLICABLE** | Mission assumed mft-rescue had it; truth table says NOT FOUND IN CODE (MR §5). Product's remnant scan + GUID-dump pass already covers aligned records outside the live MFT (PS §2.3). |
| F-43 MR imager | **SUPERSEDED** | SectorForge's two-pass + ddrescue mapfile (F-33/F-34) strictly subsumes mft-rescue's single-pass + progress.json (MR §8: failed ranges never retried). |
| F-26/F-27/F-32/F-52/F-53/F-55 | **NO ACTION — AHEAD** | Product already exceeds every reference on these (citations in rows). |
| XPRESS/LZX decode | **OUT OF HARVEST SCOPE** | No reference implements it (SF lists-but-doesn't-decode; NR flags only). Existing standing exclusion (PS §5). |

## Priority counts (implementation backlog)

- **P0**: F-01 (named feature, with F-00d/F-31 as its prerequisites), F-33+F-34 (two-pass imaging + map). → Waves 5 and 6.
- **P1**: F-02, F-03, F-12, F-18, F-19, F-28, F-35, F-37. → Waves 2-6.
- **P2**: F-04, F-05, F-06, F-13, F-14, F-15, F-20, F-21, F-22, F-23, F-29, F-30, F-36, F-38, F-39, F-40, F-44, F-45, F-54.
- **P3**: F-07, F-08, F-16, F-17, F-24, F-25, F-41, F-42, F-47, F-48, F-49, F-50, F-51, F-56, F-57, F-58.
- **UNVERIFIED rows** (F-03/F-04/F-05/F-20/F-23/F-28/F-29/F-41/F-51/F-56/F-58) resolve to concrete EXISTS/MISSING verdicts inside their wave, before implementation, by reading the cited product files — never assumed.

## Traceability

Every row above derives from a context sheet section; every sheet claim carries a reference `file:line` citation (sheets: `worklog/{PRODUCT-STATE,SECTORFORGE-FEATURES,HALIM-FEATURES,TESTDISK-FEATURES,SLEUTHKIT-FEATURES,NTFS-RECOVER-FEATURES,MFT-RESCUE-FEATURES}.md`). Spot-checks performed at consolidation: SF named-feature paths (`crates/sf-core/src/ntfs/extract.rs:14` ORPHANS const; `sf-cli/src/recover.rs:241` deleted_only filter; tests asserting `_orphaned/orphan (id 23).txt`), NR `patch_record_data_runs` at `repairer.rs:467`, TD 353 `file_*.c` count, MR $LogFile/$UsnJrnl absence, product `CR_UI_ABI_VERSION 4` — all verified against the cloned sources.
