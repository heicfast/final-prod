# Formats Guide — what core_recovery reads and writes

The authoritative per-capability status is
[CAPABILITY-MATRIX.md](CAPABILITY-MATRIX.md); this guide is the
format-oriented view: every on-disk structure the core parses, every
container it writes, and where the behavioral details live.

---

## 1. Filesystem engines

All engines emit into the shared result tree with per-node deleted
grading (four tiers: Excellent / PartialRisk / Overwritten / MetadataOnly
where the family has a bitmap), provenance (source engine + record
identity), and the shared recycle-bin `$I`/`$R` pairing model across
NTFS / FAT / exFAT (original name/size/deletion-time restoration; the
og's four bin-root set incl. the `$RECYCLE.BI0` variant, M-052).

| Family | Coverage (live + deleted) | Notable behaviors | Deep doc |
|---|---|---|---|
| **NTFS** 3.x | $MFT walk (all record types), fixups, $ATTRIBUTE_LIST chase, resident + non-resident $DATA, LZNT1-compressed streams (writer re-inflates), hard links per-parent, ADS as child nodes, metafile filtering, $MFTMirr bootstrap fallback (records 0..3), remnant stride salvage, USN $J harvest (DELETE-filtered), $Bitmap grading | backup-DBR fallback at volume end (M-043); N-040 metafile name list; N-079 bin-scope guard | `AUDIT-NTFS.md` |
| **FAT12/16/32** | BPB gates, LFN (deleted-slot reversal), chain guards, mirrored-FAT salvage (FAT1 unreadable → scan FAT2), whole-table validation + copy retry (M-015), entry-hole/duplicate/dead-zone audits | alternate-EOC families (0x0FFFFFF0..F7); FAT32-gated sector-6 backup boot | `AUDIT-FAT.md` |
| **exFAT** | boot-region checksum, entry-set checksums, NoFatChain, FREE-entry continuation salvage, structural entry gates (M-014: secondary-count, timestamp sanity, stream non-zero, name-flag gates) | upcase-table validation (N-067); oracle re-walk bounds | `AUDIT-FAT.md` |
| **ext2/3/4** | SB/GDT/inodes/extent trees, deleted inodes, orphan sweep, dtime harvest, journal block-walk salvage (E-2: every block in [s_first, s_maxlen)), backup-superblock rescue with s_block_group_nr rebasing (S-027) | meta_bg/backup-SB grid; invalid-UTF-8 names carried losslessly with raw bytes (D-92) | `AUDIT-LINUX-FS.md` |
| **HFS+/HFSX** | volume header, catalog + extents B-trees, fork model, attributes tree (S-002: decmpfs inline + resource forks), orphan sweep, journal deleted-record harvest (S-021: ring walk, wrap-aware, differential vs live catalog, extents-overflow side map) | bounded ring walk (hardening over the og's unbounded loop); CNID self-parent skip | `AUDIT-LINUX-FS.md` |
| **Btrfs** | mirror-SB selection, chunk mapping, tree walks, subvolume roots, inline + compressed extents (zlib/lz4/zstd), old-generation parity, default-subvolume pick (M-020), xattrs (S-028) | re-scan metadata cache (N-072: own format, caller-supplied dir, OFF by default, D-89) | `AUDIT-LINUX-FS.md` |
| **ZFS** | label nvlist, uberblock ring (S-034: EVERY txg state walked newest-first with cross-state identity dedup), MOS/ZPL walk, dnode taxonomy, embedded + gang blocks, compressed extents (lzjb/gzip/zle), snapshots, clones (kernel-exact dsl_dir decode, A-008) | multi-vdev remains single-vdev (honest PARTIAL, D-05/D-09); label delay-execution retry (N-072) | `AUDIT-LINUX-FS.md` |
| **ReFS** (first-class) | full MSB chain: page signatures (first-DWORD dispatch + partial-signature grading, N-007), dual-checkpoint max-clock, versioned pointer lists (minor ≥ 14), next-chains, ObjectID/directory tables, $DATA runs, deleted/ID2 harvest, allocator grading, attribute layer (resident inline + all attribute rows + paged tables, S-020/M-022) | container-map translation tracked (N-036); system-table set {0x500, 0x600, 0x701} | `AUDIT.md` refs rows |
| **LDM** (first-class) | TOCAB01 databases, VBLKs, component/partition/volume assembly, RAID-0 stripe + RAID-1 mirror concatenation, cross-disk resolver, GPT-dynamic discovery with the REAL Microsoft type GUIDs | RAID-5 rotation dormant both sides (I-008) | `CAPABILITY-MATRIX.md` |
| **UDF** (recognition) | the og's AVDP → Main-VDS descriptor probe (analyser result 0xc); no engine on either side — content recovers via carving | recognition is boot-anchored, exactly the og (M-044) | M-044 finding |

## 2. Container / disk formats read

| Format | Notes |
|---|---|
| Flat images (`.img`, raw `.dmg`) | direct |
| VMDK | descriptor extents (flat + zero), monolithic sparse |
| VHD / VHDX | fixed + dynamic (footer/ header-aware); qemu interop tests skipped when qemu absent |
| BitLocker-encrypted volumes | NOT bypassed — see the consent model ([CONNECTION-GUIDE.md](CONNECTION-GUIDE.md#consent)); with a user-supplied FVEK the in-stack XTS-AES `DecryptingDevice` (M-033) makes the volume readable to the engines |

## 3. Carving (RAW recovery)

Two layers over every readable byte range:

1. **Grammar validators** — structural validation, not just magic bytes:
   JPEG (SOF gates), PNG, ZIP (3-anchor + CD ownership + LFH sanity,
   N-068), 7z, PST, OFFICE family, TIFF (family size basis, M-056),
   WAV, MP3, PDF, GIF/BMP/RTF/OLE/MPEG-PS/ES/DWG, headerless MP3/AAC
   (10-frame walkers), CAB (`mscf` case key, N-069)…
2. **The 139-format raw-signature table** (raw_signs.json converted per
   the generated-data precedent) + **user-taught signatures**
   (`--user-signs`, the og's EUUserData JSON shape, M-048: extension →
   `signs["offset,HEX"]` + mime; built-in wins at the same position).

ZIP-content typing (the og's OfficeTagFromXML walk, N-115 + N-158):
archives are re-typed by their inner entries — the OOXML rels-path map
(DOCX/XLSX/PPTX), the mimetype/content tags (ODF trio, EPUB, HWP), the
name tags (iWork, CDR), and the five-branch document-tool tail (N-158:
XMind workbooks by the content.xml/meta.xml namespace/prolog probe,
MindManager `xsd/MindManagerApplication.xsd`, Edraw `rels/_rels.xml`
forms, Filmora `ProjectFolder/` forms, DaVinci `MediaPool/Master/
MpFolder.xml` — typed ids gate through the ABI's
`CarveFlags::disabled_formats` like the og's RawConfig [Filter] keys,
PARITY-DELTAS D-134).

Cross-cutting behaviors: the window-aware read view (each scan window's
reads served from memory — measured 9.9× fewer device reads, M-025),
the cross-engine occupancy gate (structured extents suppress carve
re-reads, M-039), overlap scoring with confidence ranking (N-070),
archive inner-entry inventories (ZIP/RAR/CAB member lists surface as
file metadata, S-032), per-analyser naming (the og's MP4/MOV
"Lost Name File (UTC timestamp)_EaseUS" template, N-063), carved files
marked Deleted (S-031), and the post-processor hook for host-side
repair passes (M-021).

`rrecov list-carve-formats` prints the capability projection; the ABI
exposes it (`cr_ui_carve_format_count/info`).

## 4. The `.crsf` session container

Own format (semantic parity with the og's `.rsf`, D-01): a chunked
container of **typed records**, each with its own CRC32, zlib-chunked
payloads (bit 15 = deflate, bits 0–11 = size−1, stored-when-not-smaller),
versioned records, and skip-forward reading (old readers skip unknown
record types). Record inventory (see `include/cr/session/cr_records.h`):

| Record | Payload |
|---|---|
| REC_RESULT_TREE | the file graph (nodes, properties, grades, recycle provenance, raw names) |
| REC_ERROR_MAP | read-error ranges with module attribution |
| REC_ALLOC_MAP | the downsampled engine free-map overlay (S-009) |
| REC_VOLUME_INFO | volume identity card (model/serial/CRC32 boot hash, N-066 firmware/WWN/interface + boot snapshot) |
| REC_SESSION_STATE | resume bookkeeping (stages, counters) |
| REC_PARTITION_CONTEXT | lost-partition candidates + content counts (ranked, v3) |
| REC_DISK_LAYOUT | whole-disk layout snapshot for resume revalidation (M-042) |
| REC_ASSISTANT_STATS | the grading pass statistics (S-023) |
| REC_FS_CHECK | the damage-audit report (M-041) |
| checkpoints | stage-boundary incremental state |

Atomic saves (temp-then-rename, N-060); a torn save never destroys the
previous good session. Resume revalidates device identity before
continuing (layout-changed detection is surfaced, never silently
dropped).

## 5. The recovery report (JSON)

Write-only machine-readable result (G-09), emitted by
`rrecov recover --report` and the ABI. Schema:
[recovery-report-schema.md](recovery-report-schema.md) — CI-validated on
every push; arbitrary recovered names are strictly JSON-escaped
(invalid UTF-8 → U+FFFD, so the document always parses).

## 6. Written outputs (recovery)

The safe writer materializes the selected set to a destination:
queue → collision-aware naming (`%s_%d` ladder, 10,000 cap, exhaustion
fails the item — N-065) → atomic temp+rename → optional hash verify →
directory tree reconstruction. Item-level truth (skipped/collision/
hash-mismatch/salvaged) is reported, never swallowed. Raw-byte names
restore via the og's conversion semantics (N-016b: `name_raw` band).

## 7. Repair/apply artifacts

- Repair undo logs (serialized `CRUNDO01` transactions) — written by
  `fix --apply` / `scan --repair-apply`, consumed by `--undo-apply`.
- Apply undo logs (lost-partition table restore) — `apply` /
  `undo` CLI, `cr_ui_apply_*` ABI.
- Boot-chain repair: BCD cell offset surgery + UEFI `Boot####` NVRAM
  updates (M-040 stages a+b), consent-gated, plan/execute/undo shape.

## 8. Health / identity

ATA SMART (SAT + CSMI routing), NVMe log pages (identify + health), the
4-status grading model (Unknown/Good/Caution/Bad with derived verdicts
and the og's 0xD1 vendor thresholds), and the device identity card
(model/serial/firmware/WWN/interface) — read-only surfaces.
