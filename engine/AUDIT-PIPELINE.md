# AUDIT-PIPELINE.md — Refactor-F pipeline-parity audit (carving, lost partitions, sessions, writer, health/SMART, RawFixer, preview/query/report, LDM)

> **Task ID:** Refactor-F · **Mode:** READ-ONLY (no source modifications; one output file + worklog append).
> **Pins:** core_recovery `08b44ac` (== `be8875f` code-wise; the two commits on top are worklog/docs
> only — `git diff be8875f..HEAD --stat` = `worklog.md` + `worklog/session-parity-refactor-prompt.md`),
> rda-driver `7e88e38`, rda-wizard (out of scope), easeus-drw-research `18d6f14` + og-codebase/
> installed tree. Suite state at pin: 179/179 green under ASan+LSan (per worklog, UI-SESSION-HANDOFF).
> **Method:** line-by-line read of all in-scope source + headers (10,043 LOC); corpus artifacts
> (SearchDLL carving/census/zip/granular-batch1, MOM PartitionRecoveryD/LdmManager/Partition/
> FileSystemCheck/Fixup, rawfixer-fs-repair + v2 inventory + pending-op engine, SaveScan rsf format,
> DiskHealth companion, architecture 04/06); **og-codebase shipped-config inspection** (raw_signs.json,
> config/RawConfig.ini, FileFilter.xml, config/usefullostfiles.xml, EUUserData, Dict, SaveScan/*.rsf,
> PartitionRecovery/, diskhealth/); every prior-audit claim (F-A3-*, F-A4-*, G-08/09/13/14, U-4/U-5,
> D-28/29) re-verified at HEAD; Grep-verified dead/duplicate-code hunt.
> **Verdict vocabulary:** MATCH / MISMATCH / MISSING / DELIBERATE-DELTA (documented) /
> DELTA-UNREGISTERED / UNVERIFIED / CLOSED (prior finding verified fixed) / OPEN (still present).

---

## 1. Carving

### 1.1 Registry shape (parameter table)

| Axis | Ours (HEAD) | og ground truth | Verdict |
|---|---|---|---|
| Grammar registry | 30-entry `Signature` array, first-byte dispatch, `kTrialOrder` specificity (cr_registry.cpp:45-248, cr_carve.cpp:35-39) | `std::map<u32 LE fourcc, chain>` + u16-key second registry + modern→legacy fallback chains (census §1-3) | DELIBERATE-DELTA (documented, REC grid) |
| Raw-sign table | `kFormatCount=139`, `kHeaderSignCount=199`, `kTailSignCount=62` (cr_rawsigns_data.cpp:14-16) | og-codebase/raw_signs.json: **139 formats / 199 header / 62 tail signs** (programmatic recount this audit; sign strings all `"<offset>,<hex>,<confidence>"`) | **MATCH** (F-A3-1 re-confirmed; prior full byte-diff stands) |
| Sign index | (offset, first-byte) buckets over sorted arrays, lazy once_flag (cr_rawsigns.cpp:18-49) | not RE'd | ours-only mechanism, sound |
| JPEG magic | 3-byte `FF D8 FF` (cr_registry.cpp:46) | 4 keys FFD8FF E0/E1/DB/EE (census §3; deep-dive §2) | DELTA (superset, F-A3-8 half) |
| ZIP anchor | `PK\x03\x04` only (cr_registry.cpp:80) | analyser accepts LFH **and** CD `PK\x01\x02` (preferred) **and** EOCD `PK\x05\x06` (zip-analyser §1) | **MISMATCH — CARVE-2** |
| TIFF family | `II*\0` only | og keys `2a004d4d` (MM), `2a4949`, `554949`, `4f524949`, plus ORF `53524949` (IIRS) / `524f4d4d` (MMOR) | **MISSING — CARVE-3** |
| ISO9660 | magic at +0x8001, min 33792 | og C7ZIPAnalyser counterpart exists | MATCH (late-magic design, F8 gate covers) |
| Per-extension runtime gate | none (CarveFlags = hash/threads only) | `config/RawConfig.ini` [Filter] — **34 toggles in the shipped file** (deep-dive says "37"; actual file count = 34) | MISSING — CARVE-9 |
| User-format layer | none | `EUUserData` file at og-codebase root: JSON ext→{signs, mime} second DB (verified: `123`, `cpl`, `epub`, `ttf`…) | MISSING — CARVE-9 |

### 1.2 Grammar validators (verified at HEAD)

| Validator | Key checks | Verdict |
|---|---|---|
| JPEG | SOI + marker walk, BE lengths, 0xFF desync, EOI requires SOF+SOS; **no SOF precision-8 check** (cr_analysers.cpp:196-197) | MATCH core / DELTA (F-A3-8 OPEN; og `bVar1==0xc0 && pbVar25[4]==8` @39653) |
| PNG | IHDR len 13 + dims ≤ 8000² + chunk walk to IEND, 4096-chunk cap | MATCH |
| PDF | `%PDF-` + `1..9`.` version grammar + **U-4 rule: last %%EOF before next %PDF- hard boundary** (cr_analysers.cpp:460-495), 1 MiB block reads | MATCH (self-justified boundary; og rule UNVERIFIED) |
| ZIP/DOCX | first geometry-consistent EOCD wins (cross-file absorption fix), entries>0, cd_offset≥30, CD-before-EOCD; DOCX `[Content_Types].xml` first-entry gate | MATCH core / MISMATCH salvage (CARVE-2) |
| 7Z | StartHeader nh_off/nh_size math | MATCH |
| SQLite | magic tail + page-size pow2 + page count | MATCH |
| MP4/MOV/M4A/HEIC | brand tables + BMFF box walk (largesize, to-EOF) | MATCH |
| AVI/WAV | RIFF+`AVI `+hdrl / RIFF+WAVE+fmt (WAV `fmt` field read is vestigial — CARVE-11) | MATCH |
| MKV | EBML vint + element walk, unknown-size stop | MATCH |
| RAR4/RAR5 | block walk to END / ENDARC; RAR5 min 32 (22-byte floor) | MATCH; RAR5 og-acceptance UNVERIFIED (comment over-claims, CARVE-10) |
| PST | !BDN + wMagicClient "MS" + version 0x0E→size@0xA8 / 0x17→size@0xB8, 1 MiB floor, AMap probe — **unknown version still REJECTED** (cr_analysers.cpp:733-734) | **MISMATCH — CARVE-4** (og FUN_180041eb0 accepts unknown versions at the 1 MiB floor) |
| Camera formats | CR2 (CR@+8), DNG (DNGVersion tag 0xC612), ORF (IIRO via tiff_common), RAF (FUJIFILMCCD-RAW + sections), RW2 (IIU\0) | MATCH; ORF BE variants missing (CARVE-3) |
| MP3 | ID3v2.3/2.4 syncsafe + strict frame walk, min 2 frames, stop-at-invalid (AUDIT Wave-3 fix verified in code) | MATCH (ours stricter than og decode) |
| CAB/ISO/ASF/FLV | MSCF counts / CD001 PVD+blocks / ASF GUID / FLV tag walk | MATCH |

### 1.3 Merge / dedup / naming / confidence / stats

- **Pass order**: grammar pass then raw-sign pass per window; sign layer skips `emitted_at`
  positions (grammar-validated or dedup-resolved) — matches census §5 "C++ chain first, then
  JSON signs for unclaimed extensions" **except** grammar-*rejected* starts are still probed by
  the sign layer (sqlite collision) — F-A3-12 OPEN, undocumented (CARVE-7).
- **F9 dedup**: first-extent + size-class (`len/65536`, ≥1 MiB → log2 class +64) vs
  `dedup_tree` (structured) and own tree; **structured wins** by construction (carve runs after
  engines, dups skipped). Content hash is computed *after* dedup for writer verification — it is
  NOT a dedup input (the "content-hash-when-available" phrasing overstates; no hash-based dedup).
- **Attach points**: carved → root `u"Other Deleted Files (Carved)"`; remnants/lost →
  `u"Lost Files"` (cr_scan.cpp:77-80). og: flat `LostTreeModel`/`LostTableModel` views
  "&Other deleted files" / "&Lost data not found" (04-recovery-workflow §5.1). Same role, different
  strings (see CARVE-5 family).
- **Naming (F-A3-7 OPEN, unregistered)**: orchestrator namer `"Carved%06u.%s"` (cr_scan.cpp:849),
  engine default `"File%06u.%s"` (cr_carve.cpp:313, raw-sign layer :249). og host string is
  **`FILE%d.%s`** + `"Lost Name File (%d).%s"` (DRWUI strings_ascii.txt:22735); architecture
  (host callback via vtable+0x48) is MATCH. Not in PARITY-DELTAS.md.
- **Confidence math**: grammar `(sig.base_confidence + cr.confidence)/2` capped 1000
  (cr_carve.cpp:140-142); raw-sign `(hdr+tail)/2 + 100` capped 1000 (cr_carve.cpp:207-209).
  og semantics UNVERIFIED (census: "Header+tail+size-gate confidence combination ranks overlapping
  candidates" — no formula decoded). Keep tracked.
- **CarveStats (F-A3-6)**: single `sign_hits`/`sign_validated` add with explanatory comment
  (cr_carve.cpp:422-426) — **CLOSED at f6468ba, verified at HEAD**. Honest fill-rate window
  attribution (`counted = min(want, WINDOW)`, :297) also verified.
- **Shared-magic tie-break (U-5)**: kTrialOrder = DOCX>ZIP, CR2>DNG>TIFF, RAR5>RAR4, M4A>MP4/MOV;
  first grammar-validated format at a magic position wins; raw-sign layer takes highest-confidence
  hit incl. shared-magic families (flp/fst on `46 4c 68 64`) — documented DELTA vs og multimap.
- **Threading**: window fan-out + tree mutex + per-worker `emitted_at` — ours-only, no og decode.

### 1.4 Carve findings

| ID | Sev | Verdict | Finding | Acceptance test |
|---|---|---|---|---|
| **CARVE-1** (=F-A3-2) | P2 | OPEN | GPT backup-salvage **dead code**: `last_lba` set only inside `if (rc_p==OK)` (cr_table_parse.cpp:207-213) so the backup probe (:241) and the `!gpt_primary_ok` salvage branch (:244-264) are unreachable; protective-MBR path (:286-290) never probes last LBA. Header contract (cr_table_parse.h:69-71 "partitions still extracted ... salvage-over-purity") is doc-fiction. og reads the backup at last LBA when LBA1 lacks `EFI PART` (Partition_decompiled.c:4205-4218) | Fixture: valid `EFI PART` at LBA1, corrupted header CRC, valid backup at last LBA → expect entries salvaged + `gpt_backup_ok`; today: GptDamaged with 0 partitions |
| **CARVE-2** (=F-A3-3) | P2 | OPEN | ZIP fails closed: single LFH anchor, no CD/EOCD anchors, no indexless-LFH salvage (og status 0xd still carved), no zero-padding end extension (og last-16-bytes-zero test, 5.24 MB cap) | Indexless ZIP (LFH chain, CD destroyed) → carved flagged low-confidence; archive whose first survivor is a CD entry → carved |
| **CARVE-3** (=F-A3-4) | P2 | OPEN | BE-TIFF (`MM\0*`) and ORF IIRS/MMOR unreachable — registry is II-only; `tiff_common` BE branch is dead code. NB ground-truth-contracts §7 already lists `2A004D4D` as a verified seed → contract-code drift as well | MM-TIFF fixture carved as `.tif`; ORF IIRS fixture carved as `.orf` |
| **CARVE-4** (=F-A3-5) | P2 | OPEN | PST unknown-version gate inverted: ours rejects (cr_analysers.cpp:733-734, comment "corpus floors others"); og **accepts** unknown versions with the 1 MiB floor (FUN_180041eb0 returns 1, size stays 0x100000) | PST header with version 0x15 + sane AMap → accepted, size=1 MiB floor |
| **CARVE-5** (=F-A3-7) | P3 | OPEN (delta unregistered) | Carved naming strings diverge (see §1.3); not in PARITY-DELTAS | Register D-entry citing DRWUI strings_ascii.txt:22735, or align to `FILE%d.%s` |
| **CARVE-6** (=F-A3-8) | P3 | OPEN | No SOF precision-8 gate; 3-byte JPEG prefilter is a superset of og's 4 keys | SOF0 with precision≠8 rejected (or explicit delta note in registry) |
| **CARVE-7** (=F-A3-12) | P3 | OPEN | Raw-sign layer re-claims grammar-rejected starts (sqlite is the live collision) — undocumented | Document in cr_rawsigns.h divergence list or suppress probing for grammar-covered extensions |
| **CARVE-8** (=F-A3-14) | P3 | OPEN | Tail-pair ≤2 MiB cap (cr_carve.cpp:200) vs header's unbounded `[start+min, start+max)` wording | Header note or build-time assert |
| **CARVE-9** (=F-A3-13) | P3 | OPEN | No per-extension runtime gate (og RawConfig.ini, **34 shipped toggles**) and no user-format layer (og EUUserData JSON DB) — both files verified present in og-codebase | CarveFlags format-enable mask or documented capability gap |
| **CARVE-10** (=F-A3-16) | P3 | OPEN | RAR5 comment "the og validators reject RAR5 entirely" — unverified (og binds `Rar!` to RARAnalyser chain incl. RAR5 first-4-bytes) | Soften comment or decode FileRARAnalyser |
| **CARVE-11** (new) | P3 | OPEN | WAV validator vestigial statement `const u16 fmt = u16(h[16-16]) | 0;` (cr_analysers.cpp:1105) — reads h[0] ('R'), `(void)fmt` | Delete the dead lines or implement the fmt-code check |
| **CARVE-12** (=F-A3-6) | P3 | **CLOSED** (f6468ba) | CarveStats sign double-count — single add verified at HEAD | n/a (test asserts EXPECT_EQ today) |

---

## 2. Lost-partition discovery

| Axis | Ours (HEAD) | Verdict |
|---|---|---|
| Stride scan | 64 KiB windows, per-sector 55AA@+510/511 + OEM@+3 dispatch (stride param default 1) | MATCH (NTFS OEM probe og-confirmed @PartitionRecoveryD_decompiled.c:10334-10365) |
| Probes | NTFS 15/25/30 (boot-sig/bpb-valid/mft-probe incl. fixup-applied $MFT), FAT 20+10, exFAT 15+20, GPT header 10+20 | our contract — scoring UNVERIFIED vs og (see LOST-1) |
| Sizing | DBR-derived, else device-end (row 31b fix verified :94-99, :248-252); clamped to device | MATCH (our contract) |
| Dedup | same-fs nested candidate suppressed (outer/earlier wins); overlap scoring vs **live table** (row 31a fix verified — cr_scan.cpp:665-674 passes parsed table) | MATCH (our contract) |
| Overlap | Inside −20 / Overlaps −10 / Contains −10; Inside still reported | MATCH (our contract) |
| Materialize | virtual read-only PartitionSlice; roots `u"Candidate @ <LBA>"` (cr_scan.cpp:691-696) | MATCH (our contract) |

Findings:
- **LOST-1 (P3, UNVERIFIED — carried)**: scoring constants/evidence tags have no corpus ground
  truth. PartitionRecoveryD.md is a class census (`CRecoverySearchEngine` ×18,
  `CGeneratorSetCompleteSearch`/`RegSearch`, `IBlockParser` family) + `lostpart.dat`/`partinfo.dat`
  strings; no strides/scores decoded anywhere (incl. og-codebase/log/CalcPartition.log samples).
  Header keeps the honest "our contract" framing — correct; keep tracked.
- **LOST-2 (P3, dead enum)**: `CandidateFs::MbrTable` never produced by any probe
  (cr_lostpart.h:25; only Ntfs/Fat/Exfat/GptTable constructed in cr_candidates.cpp).
- **LOST-3 (P3, field misuse)**: `Candidate::fs_probe` ("engine open rc when probed") is never
  set by `discover()`; the session-restore path stuffs `pc.score` into it "for diagnostics"
  (cr_scan.cpp:985) — different semantic on the same field.

---

## 3. Sessions (CRSF vs og .rsf)

| Axis | Ours (HEAD) | og ground truth | Verdict |
|---|---|---|---|
| Container | own magic `CRSFMT01`/version/64B header/record_count patch | 16B EaseUS magic + 0x24 headers + GUID-keyed records | DELIBERATE-DELTA (D-01/crsf-format.md) |
| Chunk framing | u16 hdr, bit15 deflate, bits0-11 size-1, ≤4096, stored-when-not-smaller, level-5, per-chunk raw fallback | rsf grammar scripts + live sample I_ re-verification (prior audit) | **MATCH** (byte-semantics) |
| Record headers | 24B (`CRSR` + type + session id + chain + crc32), patch-back at finalize | 0x24 headers, chain patch-back (FUN_14051af90/14051b2a0) | MATCH (mechanism) / DELTA (CRC ours-only) |
| Corruption | per-record CRC32 + skip-forward 512B re-index + 64 MiB bomb budget | raw-payload fallback only | DELTA (documented, F10) |
| Record roles | state/volume/partition-context/tree/error-map (5) | 4 GUID-keyed records + identity card record | MATCH semantics / identity gap (SESS-2) |
| stage_done_mask | **derived from actual completion** (quick_done_/pass+extras/carve_done_, cr_scan.cpp:878-880) + mask-⊆-configured check on load → ERR_SESSION_FORMAT | n/a | **F-A4-4 CLOSED** (verified) |
| VolumeInfo fill | slice offset/bytes/logical+physical sector/label/scanned/elapsed all populated (cr_scan.cpp:891-902) | og record 2: volumeName+deviceType+SYSTEMTIME+elapsed | **G-14 CLOSED** (fields that exist); SYSTEMTIME/device identity still absent |
| Resume | skips stages whose mask bit is done; restores fs_hint; G-23 candidate round-trip | og resume via session-id record lookup | MATCH (semantics) |
| Auto-checkpoint | `ScanConfig::CheckpointSink` + stage-boundary `emit_checkpoint_()` (G-13, be8875f) — verified in cr_scan.h:66-82 + run() boundaries | og auto-saves `auto_<vol>_<MM_DD_HH_MM_SS>.rsf` at scan completion into SaveScan/ | DELTA (see SESS-3) |

Findings:
- **SESS-1 (P2→CLOSED, verified)**: G-14 volume facts fully filled at HEAD.
- **SESS-2 (P2, OPEN — residual of F-A4-3)**: no scan timestamp and no device identity card
  (og record 3 = 552-byte partition struct + 5400-byte volume-device blob: model/serial/
  firmware/WWN/interface + 512-byte boot-sector snapshot, used "to verify on restore that the
  same volume is being re-attached"). Restore-side identity verification remains impossible;
  `recover --in file.crsf --disk N` cannot detect a wrong disk beyond the CLI warning.
- **SESS-3 (P3, OPEN — F-A4-5 refined)**: no default auto-save. CheckpointSink is opt-in and
  **rrecov never wires one** (only `scan --out`); og saves unconditionally at scan end.
  D-03's "Auto-save mirrors the og behavior (session saved at scan completion)" is true only
  for a host that configures the sink — the delta text overstates core behavior.
- **SESS-4 (P3, OPEN — F-A4-2)**: writer skips deflate for chunks <64 B (`take >= 64`,
  cr_crsf.cpp:141); reference rule compresses every chunk (live sample has an 11-byte
  compressed chunk). Comment "reference: stored wins" misstates the rule.
- **SESS-5 (P3, OPEN — F-A4-7)**: `chain==0 && crc==0` read as unfinalized tail — a legitimate
  zero-length record would terminate indexing (no current caller writes one).
- **SESS-6 (P3, CLOSED)**: F-A4-4 mask truth — fixed and test-enforced (mask tamper →
  ERR_SESSION_FORMAT per worklog UI-W1 tests).

---

## 4. Recovery writer

| Axis | Ours (HEAD) | og ground truth | Verdict |
|---|---|---|---|
| Lifecycle | enqueue → process → commit; temp `.part-<id>` + fsync + atomic rename | direct writes via normal file APIs (04 §7.5) | DELTA (documented, F11/D-19) |
| mkdir | `ensure_dir_` (cached) + `DestIo::mkdir` recursive per-level `CreateDirectoryW`, file-in-the-way surfaced (cr_destio.cpp:80-112; cr_safewriter.cpp:133-142,175-182) | CreateDirectoryW per level (DRWUI:357842) | **G-08 CLOSED (91053b0) — verified incl. Win32 backend** |
| Empty dirs | directory nodes enqueueable (no extents → never overlap-refused), mkdir+Done | og preserves empty folders | MATCH |
| Collision | `ConflictPolicy {Rename(default)/Skip/Overwrite}`; intra-queue always rename via claimed-name set; Skip = idempotent re-run (resume detection row 35a reachable) | og: existence → `name_N.ext` (suffix before extension, counter from 0), cap 10000 (DRWUI:357110-357136) | DELTA registered (D-19) + loop-bounds nit (WRITER-3) |
| Error isolation | `process()` **still returns on first per-item error** (cr_safewriter.cpp:399-400) | og collects failures and continues ("%1 files/%2 failed; ... recovery log") | **MISMATCH — WRITER-1 (OPEN)** |
| Hash | streamed SHA-256 always finalized; hash_status ∈ verified/computed/mismatch/not_computed; carved-only stored sha128 compare (≤8 MiB); salvage-on-mismatch commits + reports | no og hash verification found (UNVERIFIED) | DELTA (ours-only, honest) |
| Overlap refusal | code-level extent-intersection ERR_DEST_OVERLAP per node + refused items reported; CLI same-disk-different-volume warning | engine opens `\\.\X:` (writability + same-volume detection); source-drive block is UI-level (D-17/F12) | DELTA (documented, stronger) |
| Report | G-09 JSON (schema v1, docs/recovery-report-schema.md; CI validator tools/validate_report.py); `--report` wired (rrecov.cpp:610,731-753) | og has **no** recovery-results report (DRWReport.exe = crash reporter/minidumps — verified in og-codebase) | ours-only improvement — **registered as D-03-adjacent; recommend explicit D-entry** |
| Stats | queued stamped upfront; done/dirs/refused/errors/hash_mismatch/resumed; per-item duration + Pending outcome truth | n/a | MATCH (honest) |

Findings:
- **WRITER-1 (P2, OPEN — F-A4-8)**: the handoff claim "writer mkdir+abort fixed by the
  UI-readiness agent" is **half-true**: mkdir landed (G-08), the queue-abort did NOT.
  `if (u32 rc = recover_item(it, cancel, stats)) return rc;` aborts the whole queue on the
  first per-item error (create_temp/write/truncated/commit failures; only hash-mismatch
  continues). One unreadable file still cancels the remaining 10,000; `stats.errors` caps at 1.
  *Acceptance:* 3-item queue where item 2's dest is unwritable → items 1+3 Done,
  `stats.errors==1`, summary rc non-fatal; cancel remains the only hard abort.
- **WRITER-2 (P2, OPEN — F-A4-15, registry error)**: docs/error-fix-registry.md row F11
  still states the reference behavior is "collision = overwrite" — contradicted by the
  decompile (rename `name_N`, cap 10000). Future agents could "re-fix" our correct rename into
  an overwrite. *Acceptance:* F11 reference column corrected.
- **WRITER-3 (P3, OPEN — F-A4-10 residual)**: `rename_attempts=64` yields suffixes " (2)"…"(64)"
  (63 numbered candidates) then `_dup`; og = `name_N` before the extension, counter from 0,
  cap 10000. Delta registered (D-19) but the off-by-one arithmetic nit persists.
- **WRITER-4 (P3, OPEN — F-A4-11)**: sanitizer replaces 7 chars with `_` (the og **session-file**
  sanitizer rule); the og **export** sanitizer replaces 9 chars (incl. backslash) with `#`
  (DRWUI:357076-357080). Header comment cites the wrong og function for this call site.
- **WRITER-5 (P3, OPEN — F-A4-13)**: `WinDestIo::commit` closes the handle before
  `MoveFileExW`; on rename failure `close()` skips the temp delete (handle==nullptr) → leftover
  `.part`. Collision was resolved at enqueue-time, not commit-time (single-writer assumption).
- **WRITER-6 (P3, doc-code drift)**: `WriterConfig::verify_hash` comment "refuse-after-write
  mismatch is an error" (cr_safewriter.h:85) — behavior is salvage-on-mismatch (commit +
  `hash_mismatch` + `Outcome::HashMismatch`). Stale pre-35(b) wording.
- **WRITER-7 (P3, CLI gap)**: `ConflictPolicy` unreachable from rrecov (no `--conflict` flag);
  Skip/Overwrite are library-only.

---

## 5. Health / SMART

| Axis | Ours (HEAD) | og ground truth | Verdict |
|---|---|---|---|
| ATA transport | `IOCTL_ATA_PASS_THROUGH` (ATA_PASS_THROUGH_EX, header shape used verbatim) with 0xEC IDENTIFY → 0xB0/0xD0 READ_DATA | corpus DiskHealth = SAT `IOCTL_SCSI_PASS_THROUGH` 0x4D004 (18 sites) + CDB builder 0xB0/0x4F/0xC2 | DELTA — **undocumented in header** (HEALTH-2) |
| Legacy fallback | `SMART_RCV_DRIVE_DATA 0x7C088` (cr_health.cpp:55-85) | DiskHealth:9010/13438/14176 | MATCH |
| Attribute table | 512B, 30×12B at +2, id/flags/value/worst/raw6; derived 5/9/10/190/194/197/198/231/233 | ATA-ATAPI-7 shape; og table | MATCH |
| failing derivation | flags bit3 ‖ (pre-failure && value==0) | not decoded | UNVERIFIED (tracked) |
| NVMe page 2 | pure `cr::health::parse_nvme_health_page`, offsets 32/48/**112/128/144/160** (cr_nvme.cpp:32-37) via STORAGE_QUERY_PROPERTY | corpus silent on og mapping | **F-A4-17 CLOSED (a1ff58e) — verified + 3 unit tests (test_nvme_health.cpp)** |
| D1/D8 | never issued | corpus confirms 0xD1/0xD8 commands (DiskHealth:14112/:4423) | MISSING (HEALTH-3) |
| Multi-transport | ATA → legacy → give up | CSMI/NVMeRealtek/USB bridge table (CrystalDiskInfo-lineage, companion §2) | MISSING (HEALTH-4) |
| TRIM | fsutil query, **last line wins** (cr_health.cpp:290-297) | og: query + set toggle; takes FIRST "= " match (DRWUI:178742-178749) | DELTA documented (no-set) + parse nit (HEALTH-6) |
| SSD detection | `StorageDeviceSeekPenaltyProperty` (cr_health.cpp:302-319) | og DRWUI: WMI MSFT_PhysicalDisk MediaType==4 (DRWUI:178880-178887) | MISMATCH doc-vs-code (HEALTH-5) |

Findings: **HEALTH-1 = F-A4-17 CLOSED (verified)**. HEALTH-2 (P3, F-A4-16): header presents the
corpus 0x4D004 route without stating "we use IOCTL_ATA_PASS_THROUGH as an equivalent deliberate
delta" nor registering it. HEALTH-3 (P3, F-A4-18): no 0xD1 thresholds / 0xD8 status. HEALTH-4
(P3, F-A4-19): no CSMI/USB-bridge degradation. HEALTH-5 (P3, F-A4-20): three different SSD-detection
stories (header corpus claim, header inline description, SeekPenalty code) — align. HEALTH-6 (P3,
F-A4-21): TRIM parse first-vs-last divergence.

---

## 6. RawFixer

| Axis | Ours (HEAD) | og ground truth | Verdict |
|---|---|---|---|
| Transaction | snapshot-before-write (sector-coalesced spans), patch, read-back verify, reverse rollback keeping unrestored entries, bad_alloc-safe (cr_rawfixer.cpp:435-528) | CopySectors snapshot + Undo replay (rawfixer-fs-repair "Repair strategy") | MATCH |
| UndoLog | durable `CRUNDO01` + CRC32 container, round-trip/corruption/truncation tests | in-memory CopySectors | DELTA (ours-only, stronger) |
| Repair kinds | RestoreBootFromBackup (FAT@6 / exFAT@12 / NTFS volume-end), RepairBootChecksum (0..10 sum, fill@11, spec-exact skip of 106-107/112), RepairGeometryField (spc shift @0x6D) | rawfixer-v2 §4/§5 method tables | MATCH (constants og-verified where decoded) |
| Refusals | S2 clean-refuse, S3 ambiguous-refuse (multi-candidate spc) | og ambiguity handling not in corpus | ours stronger (documented) |
| Scope | S6 boot/checksum/geometry only — refuses FAT chains / MFT / dir content | og FATFixer/NTFSFixer also CheckFatTable/CheckRootDir/CheckCluster/CheckMFT/IsValidRootDir | DELIBERATE-DELTA (documented; header could annotate "(og-only; out of S6 scope)" per F-A4-23) |
| Geometry rederivation | heap-bounds fit only (uniqueness → S3) | og: **2 strategies** (FAT-chain + heap bounds) + CheckRoot revalidation after fix | PARTIAL MISMATCH (FIX-1) |

Findings: **FIX-1 (P3, OPEN — F-A4-22)** single derivation path + no post-execute structural
re-validation; header still says "2 candidate paths". **FIX-2 (P3, OPEN — F-A4-24)**: S5
verify-failure returns `ERR_REPAIR_UNDO_FAILED` even when `rollback_all()` fully succeeded
(cr_rawfixer.cpp:513-517) — conflates clean rollback with rollback failure. **FIX-3 (P3, OPEN —
F-A4-25)**: analyze-side 512-hardcodes (`sector=512` :183, `12*512` :202/:237) make the 4Kn
exFAT backup probe read the wrong offset (execute() is sector-correct via `sector_bytes()`).
**FIX-4 (P3, UNVERIFIED — carried)**: "corpus 0x19" citation (cr_rawfixer.h:19, .cpp:429) still
untraceable in any artifact. CLI consent model (dry-run default, `--apply`, `--undo-apply`
with mandatory `--disk` since d83442e) verified consistent with S1.

---

## 7. Preview / Query / Report (G-10 / G-12 / G-09)

- **Mutation check (mandate)**: `PreviewReader` holds `(device, const props*)` and its own
  ReadErrorMap/diagnostics — no tree mutation; `extract_metadata` touches only the reader.
  `query::run` takes `const ResultTree&`, builds an index vector, never mutates. Report is
  write-only over writer outcomes. **Core scan behavior is not altered by any of the three —
  verified.**
- **Delta documentation (mandate)**: D-28 (preview) and D-29 (filters) exist, **but both still
  say "core has no preview API (G-10)" / "core has no query API (G-12 — truthful)"** — stale
  since 35f4652 (G-10) and 365c263 (G-12) landed the core `PreviewReader`/`extract_metadata`
  and `query::run`. **PIPE-1 (P2, doc-drift): refresh D-28/D-29 "WHAT WE DO" to cite the core
  surfaces** (and D-30's taxonomy note now applies to the core `query::Category`).
- **PIPE-2 (P3)**: core category map = 6 categories / 57 extensions (cr_query.cpp:22-45) vs og
  FileFilter.xml = **8 categories (Graphic/Document/Email/Audio/Video/Other/Archive/Bookmarks) /
  1,481 unique extensions** (verified this audit; docs cite "1,489"). Missing Email + Bookmarks;
  `pst` (a first-class carve format) falls into Other. Header comment "mirrors the corpus
  FileFilter.xml families" over-claims a subset.
- G-09 report: schema v1 + CI validator + 4 tests (escaping/determinism/outcome-honesty/CI
  sample); no og equivalent exists (DRWReport.exe = crash reporter) — improvement, correctly
  never claimed as parity; recommend an explicit delta entry so the asymmetry is discoverable.
- Perf contract honored: O(1) tree appends (`FileNode::last_child_`), count path allocation-free.

---

## 8. LDM (first-class) + partition tables

| Axis | Ours (HEAD) | Verdict |
|---|---|---|
| Discovery | MBR type 0x42 → PRIVHEAD@LBA6; GPT LDM-metadata GUID → PRIVHEAD at partition **last LBA** (cr_ldm.cpp:979-1027); damaged-primary fallback to end-of-disk copies | MATCH (kernel/libldm/og-verified) |
| MBR 0x42 + GPT GUID recognition | `mbr_type_to_kind(0x42)=Ldm` (cr_table_parse.cpp:94-95); `GPT_TYPE_LDM_DATA`→FsKind::Ldm (:225-226) | MATCH |
| Database | PRIVHEAD 2.11/2.12 gates + geometry @0x11B/0x123/0x12B/0x133 + GUID@0x30; TOCBLOCK copies 1/2/2045/2046; VMDB 4.10 gate, vblk_size==0→0x80 default, TOC-size agreement → degraded; backups cross-checked | MATCH (constants og/kernel-verified; recovery stance documented) |
| VBLK graph | types 0x32/0x33/0x34/0x44/0x35/0x45/0x51, per-type size gates, composite ≤4 fragments + duplicate-fragment poison (kernel ldm.c semantics), `LdmStats.vblks_bad` counting | MATCH — full decode, not recognition-only |
| Volume kinds | Simple/Spanned (vol_offset order), Striped (column index, chunk), Mirrored (copies, each concat component), Raid5 (≥3 columns, left-symmetric P=(N-1)-(row%N)), Broken fallback kept visible | MATCH (vs libldm; og math UNDECODED — LDM-1) |
| LdmVolumeDevice read path | mirror **copies tried in order**, all-fail fails (cr_ldm.cpp:1330-1338); RAID-5 single-column XOR reconstruction over surviving columns incl. parity, 2+ missing → ERR_READ_FAILED (never zeroed) (:1372-1389); per-member PRIVHEAD re-anchoring via resolver (:1262-1287) | MATCH (libldm verdict; tests: Raid5LeftSymmetricReads / Raid5ReconstructsMissingColumn / Raid5TwoMissingColumnsUnrecoverable / MirroredReadFailover / MirroredBothCopiesFail) |
| Cross-disk resolver | `ScanConfig::ldm_resolver` (platform registry) with local-disk fallback (cr_scan.cpp:504-510); rrecov builds the GUID→device map (rrecov.cpp:453-473) | **G-19 CLOSED — verified** |
| Roots | `u"Dynamic Volume <name>"` per resolved volume (cr_scan.cpp:514-517) | MATCH (our contract; og naming not decoded) |
| Stats | vblks_total/parsed/bad, composites(+incomplete), privheads/tocblocks matched, `degraded` flag | MATCH (honest) |

Findings:
- **LDM-1 (P2, doc-honesty, OPEN — F-A3-11)**: cr_ldm.h:26-28 claims "corpus LdmManager.mo:
  full og parity confirmed (… Raid5VolumeStorage incl. WriteParityDisk /
  ComputeDataWithOtherDisk)". `WriteParityDisk` is **not findable anywhere in the 91,475-line
  LdmManager decompile** (prior auditor's case-insensitive search; this audit re-confirmed via
  the census artifact — only `ComputeDataWithOtherDisk` exists, as a crash-string :88616). The
  og's parity rotation is UNDECODED; our layout is libldm `raid5_ls`. Reword the ledger.
- **LDM-2 (P3, doc stale — mandated flag)**: docs/ground-truth-contracts.md §6:85 still says the
  LDM database "is fully decoded from session 5 on **(recognition only through v0.2)**" — the
  parenthetical is stale and contradicts the first-class state verified above.
- **LDM-3 (P3, OPEN — F-A3-10)**: config-size clamp 4 MiB (8192 sectors, cr_ldm.cpp:1106-1107,
  comment "spec: 1 MiB") vs og hard reject >2048 sectors (error 0xfffffff8,
  LdmManager:59753-59760); also no real-disk-size public-region validation (og
  "PRIVHEAD size doesn't match real disk size" :59718-59727) — ours only checks lds+ldsize ≤
  config_start.
- GPT/MBR table constants otherwise MATCH (CRC32 IEEE, header-CRC-over-92-with-zeroed-field,
  entry CRC, 0xEE protective, EBR chains with cycle guard); GPT validation remains shallower
  than og (no revision/reserved/current-LBA cross-checks — F-A3-9 P3, unchanged, disposition
  "degradation signals" pending).

---

## 9. Dead / duplicate code register (Grep-verified at HEAD)

| # | Item | Location | Status |
|---|---|---|---|
| 1 | GPT backup-salvage branch unreachable (`last_lba` only set on primary OK) | cr_table_parse.cpp:207-213, 240-266 | **DEAD (bug-shaped — CARVE-1)** |
| 2 | GPT protective-MBR dead store (`layout=Gpt` immediately overwritten by `GptDamaged`) | cr_table_parse.cpp:287-289 | DEAD STORE (P3 nit) |
| 3 | `tiff_common` big-endian branch (no registry magic can reach `bo[0]!='I'`) | cr_analysers.cpp:286-291, 300, 306-312 | **DEAD (bug-shaped — CARVE-3)** |
| 4 | WAV `fmt` vestigial read (`u16(h[16-16])|0`, `(void)fmt`) | cr_analysers.cpp:1105-1106 | DEAD CODE (CARVE-11) |
| 5 | `SignBucket` public struct never referenced (SignIndex uses private `Range`) | cr_rawsigns.h:62-67 | DEAD TYPE |
| 6 | `CandidateFs::MbrTable` enum value never constructed | cr_lostpart.h:25 | DEAD ENUM |
| 7 | `WindowScanner::window_base()` accessor never called | cr_candidates.cpp:48 | DEAD ACCESSOR |
| 8 | `CarveResult::fragmented` set by `zip_common`, consumed nowhere (no stat, no tree flag, no test) | cr_signatures.h:62, cr_analysers.cpp:544-548 | DEAD FIELD (R4.5 remnant) |
| 9 | `Candidate::fs_probe` never set by discover(); overloaded as score dump at session restore | cr_lostpart.h:38, cr_scan.cpp:985 | DEAD/MISUSED FIELD |
| 10 | F9 size-class computation duplicated verbatim (grammar pass vs sign pass) | cr_carve.cpp:108-115 vs 224-231 | DUPLICATION (minor, semantics identical) |
| 11 | Prior-claim disprovals: no dead `PoolCancelToken` in carve path (used, cr_carve.cpp:360); no duplicate stats adds (F-A3-6 fixed); query `SortKey::Path` is handled (second block), not dead | — | CLEAN |

---

## 10. UNVERIFIED register (tracked; never guessed)

1. og confidence formulas — grammar (base+validator)/2 and raw-sign (hdr+tail)/2+100 have no
   decoded og counterpart (census wording only).
2. og lost-partition scoring/strides/dedup order — corpus is a class census +
   CalcPartition.log samples; no constants decoded.
3. og carved-file naming beyond the string table (`FILE%d.%s`, `Lost Name File (%d).%s`).
4. og RAR5 acceptance/rejection inside FileRARAnalyser (CARVE-10 comment).
5. og ZIP end-padding stop conditions + LFH +8 field semantics (artifact vs decompile differ).
6. og writer hash verification (none found — ours-only).
7. og zero-fill of unreadable spans during recovery (ours-only salvage).
8. og NVMe health-log field mapping (spec verdict used; F-A4-17 fix is spec-correct).
9. og RAID-5 parity rotation + mirror failover order (libldm verdict used; LDM-1).
10. og PDF %%EOF selection rule (ours: last %%EOF before next %PDF-).
11. og same-disk-different-volume destination behavior (D-17 residual).
12. "corpus 0x19" unknown-FS constant (FIX-4).
13. og auto-save trigger set beyond scan completion; multi-session .rsf in the wild.

## 11. og-codebase shipped-config discoveries (for F-D-5/6/7 dispositions)

| File (verified present) | Content | Parity relevance |
|---|---|---|
| `raw_signs.json` (root, 35,342 B) | 139 formats / 199 header / 62 tail signs, `off,hex,conf` strings, min/max | compiled byte-exact into cr_rawsigns_data.cpp (MATCH) |
| `config/RawConfig.ini` | `[Filter]` per-extension carve toggles — **34** entries (deep-dive says 37) | CARVE-9 gap + doc correction |
| `FileFilter.xml` (root, 54,597 B) | 8 FileCategory (Graphic/Document/Email/Audio/Video/Other/Archive/Bookmarks), **1,481 unique `<Extention>`** | PIPE-2 (docs cite 1,489 — recount needed) |
| `config/usefullostfiles.xml` | 27 `<ext>` priority-display list (jpg, cr2, cr3, …) | og priority-display layer we lack (minor) |
| `EUUserData` (root, JSON) | second magic/MIME DB (ext → signs + mime; e.g. `123`, `cpl`, `epub`, `ttf`) | CARVE-9 gap |
| `Dict` (root, text) | ~100-entry common-password dictionary | og password-attack layer (out of scope; note only) |
| `recoveryFree.ini` | free-tier binary config | D-32 (no monetization) — evidence pinned |
| `SaveScan/` | 3 real auto-saved .rsf samples | rsf ground truth (prior audit verified live) |
| `PartitionRecovery/` | MOM modules + **RawFixer.dll, lostpart.dat, partinfo.dat**, eprdtdrv.sys | rawfixer/lostpart binaries pinned |
| `diskhealth/` | **DiskHealth.exe** + Qt Quick/QML runtime (internal name DiskSmart) | HEALTH section ground truth |
| `log/CalcPartition.log` | lost-part engine log sample | LOST-1 (no scoring constants in it) |

## 12. Bottom line

Data-faithfulness pillars hold: **raw-sign table byte-exact vs the shipped raw_signs.json
(139/199/62)**, CRSF chunk framing byte-equivalent to the rsf grammar, LDM full-decode with
tested mirror/RAID-5 reconstruction, NVMe offsets fixed and unit-tested, writer mkdir + report
+ checkpoint waves genuinely landed. The open defects cluster exactly where the prior audit
filed them and **nothing in the P2 carving batch (F-A3-2/3/4/5) has moved at HEAD**; the one
handoff claim this audit was asked to verify ("writer mkdir+abort fixed") is half-true — the
abort-on-first-error semantics (WRITER-1) and the F11 registry error (WRITER-2) are the two
P2s to land next, followed by the carving salvage batch and the two delta-registry refreshes
(PIPE-1, CARVE-5).
