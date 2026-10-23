# PARITY-CORE-DOM — Durable Sessions (CRSF vs .rsf), Recovery Writer, Health/SMART, RawFixer

**Task ID:** 2-a4 · **Agent:** parity-auditor (read-only sub-agent, Parity-A wave, domain split)
**Date:** 2026-10-23 · **Mode:** READ-ONLY — no repo modifications, commits, or pushes.

## 0. Pins & coverage

| Repo | Pin |
|---|---|
| core_recovery (ours) | `d42bb00` on `2473c03e` ("parity-audit(T1): session bootstrap…") |
| easeus-drw-research (corpus) | `18d6f14` |

**Our code read line-by-line (100% of in-scope files, 2,604 LoC incl. doc):**
`include/cr/session/cr_crsf.h` (98), `cr_records.h` (84), `src/core/session/cr_crsf.cpp` (312), `cr_records.cpp` (359),
`include/cr/recovery/cr_safewriter.h` (117), `src/core/recovery/cr_safewriter.cpp` (323),
`include/cr/win/cr_destio.h` (38), `src/win/cr_destio.cpp` (81),
`include/cr/win/cr_health.h` (96), `src/win/cr_health.cpp` (350),
`include/cr/repair/cr_rawfixer.h` (132), `src/core/repair/cr_rawfixer.cpp` (548), `docs/crsf-format.md` (66).
Supporting (partial): `include/cr/cr_error.h` (full), `include/cr/tree/cr_node.h` (props/node model),
`include/cr/exfat/cr_exfat_wire.h` + `src/core/exfat/cr_exfat_dentry.cpp` (checksum fn),
`src/core/orchestrator/cr_scan.cpp` (save/load_session), `src/app/rrecov.cpp` (cmd_scan/recover/health/fix).

**Tests read:** `tests/unit/test_session.cpp` (full), `tests/unit/test_rawfixer.cpp` (full),
`tests/unit/test_orchestrator.cpp` (SafeWriter/Overlap/Collision/Resident + session round-trip sections).
No dedicated test_safewriter.cpp exists; writer tests live in test_orchestrator.cpp.

**Docs read:** crsf-format.md, ground-truth-contracts.md, requirements.md, error-fix-registry.md (full);
AUDIT.md rows 33/35/36/43/45 (prior internal audit — treated as claims, re-verified below).

**Corpus read:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` (full, 495 ln),
`re-workspace/artifacts/PartitionRecovery/rawfixer-fs-repair.md` (full),
`rawfixer-v2--method-inventory.md` (full), `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md` (full),
`architecture/04-recovery-workflow.md` (§5–§12), `architecture/06-repair-chain.md` (full — FileRepair world, out of scope),
**all four reversed-grammar scripts read in full**: `scripts/rsf_parser.py`, `rsf_record4_grammar.py`,
`rsf_field_census.py`, `rsf_dump_records.py`. Real samples confirmed present:
`og-codebase/SaveScan/auto_Local Disk (G_…).rsf`, `(H_…)`, `(I_…)`; **rsf_parser.py executed read-only on
sample I_** — output independently confirms container/record/chunk framing (§1 below).
Targeted decompile reads: DiskHealth_decompiled.c (IOCTL sites, SAT CDB builder), DRWUI_decompiled.c
(ExportThread, export naming/sanitize loop, mkdir, TRIM query+set, WMI SSD), RawFixer_decompiled.c (ExFATFixer::ChecksumDBR).

**Not covered (stated plainly):** `DRW-exe--launcher.md` / `DRWReport-exe--crash-reporter.md` (crash
reporting, judged not writer-relevant — DRWReport handles minidumps, not recovery results); the three
`ifileproperties-part{1,2,3}*.md` deep-dives (their content is summarized and byte-validated inside
rsf-format §5.4.5 + the grammar script, which I did read); the full 86,575-line RawFixer and 93,317-line
DiskHealth decompiles (targeted reads only); `rda-wizard` (different sub-agent's scope); GitHub issues
G-01..G-23 (not on disk — see §5 note); og `diskhealth/` runtime DLLs beyond the artifact.

---

## 1. Audit grid

Verdicts: MATCH / MISMATCH / MISSING / DELIBERATE-DELTA (documented) / UNVERIFIED (corpus silent).

| Subsystem / Axis | Verdict | One-line basis |
|---|---|---|
| **Session container semantics** | | |
| Constants & offsets (chunk framing) | **MATCH** | u16 hdr, bit15=deflate, bits0-11=size-1, ≤4096, stored-when-not-shrunk, level-5 — ours = rsf grammar scripts, verified live on sample I_ |
| Constants (our container/record header) | **DELTA (documented)** | Own magic/version/CRC layout; og = 16B magic + 16B session-id + chain, no CRC — documented in crsf-format.md + F10 |
| Data model (record roles) | **MATCH (semantics) / MISMATCH (fields)** | 5 roles ≈ og's 4 GUID-keyed records; but our records lack volume/device identity + scan time (F-A4-3) |
| Data model (tree grammar) | **MATCH (semantics)** | Dual node class + per-node marker + refcounted props {flags,name,size,times,attributes,runlist} = rsf §5.4/§9 |
| State machine (finalize/patch-back) | **MATCH** | Append at EOF, patch chain at finalize = og FUN_14051af90/14051b2a0 mechanism |
| Error surface (corruption) | **DELTA (documented, F10)** | Per-record CRC + skip-forward + per-chunk raw fallback (og has raw fallback only) |
| Threading/cancel during save | **MISSING** | No cancel/progress in session::Writer; og checks cancel per child + 40%→65% progress (rsf §5.4.4) |
| **Recovery writer** | | |
| Behavior (atomic temp+rename) | **DELTA (documented, F11)** | Ours temp+`.part-<id>`+fsync+rename; og writes via normal file APIs directly |
| Behavior (collision naming) | **MISMATCH** | og `name_N.ext` (≤10000 tries) vs ours `name (2).ext`…`_dup` (F-A4-10) |
| Behavior (mkdir / dirs) | **MISSING (P1)** | og CreateDirectoryW per path level; ours never creates dirs (F-A4-9) |
| Behavior (per-item error isolation) | **MISMATCH** | og collects failures and continues; ours aborts queue on first error (F-A4-8) |
| Safety (overlap refusal) | **DELTA (documented, F12)** | Ours: code-level extent-intersection refusal; og: UI-level "Source Drive!" + disabled button, engine checks writability only |
| Sanitize rules | **MISMATCH (detail)** | og export sanitizer: `:?\|*><"` → **'#'** incl. backslash; ours: 7-char set → '_' (F-A4-11) |
| Hash verification | **DELTA (ours-only)** | Truncated-128-bit SHA-256 verify for carved ≤8MiB; no og equivalent found (UNVERIFIED in corpus) |
| Unreadable-span zero-fill | **UNVERIFIED (og)** | Ours: zero-fill + intentional hash mismatch (salvage); og behavior not found in corpus |
| Threading | DELTA (acceptable) | Ours synchronous single-thread; og ExportThread dedicated thread |
| **Health / SMART** | | |
| Constants (IOCTL transport) | **MISMATCH** | Header claims corpus 0x4D004 (verified in DiskHealth); code uses IOCTL_ATA_PASS_THROUGH (F-A4-16) |
| Constants (legacy fallback) | **MATCH** | SMART_RCV_DRIVE_DATA 0x7C088 fallback present = DiskHealth:9010/13438/14176 |
| Data model (SMART table) | **MATCH** | 30×12B entries at +2, value/worst/raw6, attrs 5/9/10/190/194/197/198/231/233 derived |
| Data model (NVMe log page 2) | **MISMATCH (P1)** | 4 of 8 field offsets wrong vs NVMe spec (F-A4-17); corpus silent on og mapping |
| Behavior (D1/D8 commands) | **MISSING** | READ_THRESHOLDS 0xD1 / RETURN_STATUS 0xD8 never issued (corpus: DiskHealth:14112/4423) |
| Behavior (multi-transport) | **MISSING** | No CSMI / NVMe-Realtek / USB-bridge degradation paths (DiskHealth §2) |
| Behavior (TRIM) | **DELTA (documented) + nit** | Ours query-only (og also sets — FUN_1400c9450); first-vs-last line parse difference |
| SSD detection | **MISMATCH (doc-vs-code)** | Header claims WMI MSFT_PhysicalDisk MediaType==4 (corpus-verified); code does SeekPenalty (F-A4-20) |
| **RawFixer** | | |
| Behavior (transactional protocol) | **MATCH** | Snapshot-before-write → write → read-back verify → reverse rollback = CopySectors/Undo contract |
| Behavior (ChecksumDBR) | **MATCH** | 11-sector sum + whole-sector fill consistency = RawFixer_decompiled.c:10486/10510/10543-10552 |
| Constants (backup sectors) | **MATCH** | FAT backup@6 (rawfixer-v2 §5), exFAT backup@12 (§4); NTFS volume-end = spec, corpus-UNVERIFIED |
| Behavior (geometry rederivation) | **PARTIAL MISMATCH** | Only heap-bounds path; og has 2 strategies + CheckRoot revalidation (F-A4-22) |
| Scope (S6: boot/checksum/geometry only) | **DELIBERATE-DELTA (documented)** | og FATFixer/NTFSFixer also probe FAT chains/MFT/root dir — ours refuses by scope |
| Undo log durability | **DELTA (ours-only)** | CRUNDO01+CRC32 container; og undo is in-memory CopySectors replay |
| Error surface | nit | ERR_REPAIR_UNDO_FAILED returned even when rollback fully succeeded after S5 verify failure |
| Threading/progress | **MISSING** | og repair is PPL-agent async with progress callbacks; ours synchronous, no progress/cancel |

---

## 2. Session / CRSF vs .rsf — detailed findings

### 2.1 What matches (verified against the reversed grammar tooling — mandatory check)

The chunk framing in `crsf_chunk_frame`/`crsf_chunk_parse`
(cr_crsf.cpp:20-62) is byte-semantically identical to the ground truth in
`scripts/rsf_parser.py:41-72`, `scripts/rsf_record4_grammar.py:216-233`, `scripts/rsf_dump_records.py:7-22`:

- u16 LE header; bit 15 = zlib-deflated; bits 0-11 = `payload_size − 1`; payload ≤ 4096
  (rsf-format §3.1; scripts compute `total = (h & 0xfff) + 3`).
- Stored form when deflate does not strictly shrink — ours `comp_len < len` (cr_crsf.cpp:28);
  og "explicit `uVar8 < uVar3` comparison" (rsf-format §3.2).
- Level-5 deflate: ours `compress2(..., 5)` (cr_crsf.cpp:26); og CZlibCompressor init(5) (§3.2).
- Reader raw-payload fallback on inflate failure: ours cr_crsf.cpp:293-295 (per chunk, caller-side);
  og "falls back to memcpy of the raw payload" (§3.2); scripts do the same
  (rsf_parser.py:60-63 `except zlib.error: out += payload`).
- Record append + finalize patch-back: ours writes the 24B header with chain=0 then patches
  chain at finalize (cr_crsf.cpp:117-191) — same mechanism as og FUN_14051af90/FUN_14051b2a0
  (rsf-format §2.1: "chain values 13, 66, 588, 8,995,756" patched back at saved offset).
- Decompression-bomb budget 64 MiB per record (cr_crsf.cpp:283-299) — our addition (P1-7), sound.

**Independently re-verified live:** `python3 scripts/rsf_parser.py "og-codebase/SaveScan/auto_Local
Disk (I_)10_14_23_13_21.rsf"` → 4 records, chains 13/66/588/8,995,756, chunk `hdr=0x800a` =
compressed 11→12 B, `hdr=0x803f` = 64→147 B, record-2 payload starts `7f 00 00 00 "Local Disk (I:)"`
+ SYSTEMTIME `ea 07 0a 00 03 00 0e 00 17 00 0d 00` (2026-10-14 Wed 23:13) — matches rsf-format §5.2
field-for-field. The corpus evidence chain is intact.

Tree grammar semantics: our `serialize_tree`/`deserialize_tree` keep the corpus patterns —
dual node class with light/full **marker byte per node** (cr_records.cpp:126/188; og: u32 marker
per child, 0=CFileNode 0x93 / 1=CFileSystemNode 0x290, rsf-format §5.4.1), refcounted content
objects with the `{flags, name, size, times, attributes, runlist}` field map
(cr_node.h:40-80; rsf-format §5.4.5 + ground-truth-contracts §9), full nodes = roots/virtuals only
(cr_node.h:147), root role names (our "Restored"/roles ≈ og "Local Disk(I:)", "File Path Lost",
"Reconstructed"). Ours is DFS + explicit parent ids vs og BFS + 0xFFFFFFFF sentinel — equivalent
expressiveness, documented own-format delta. Our additions (extent codec/psize, resident_data via
v2 node flags, carve provenance, sha128) are backward-compatible extensions in the reserved slot.

### 2.2 Findings

**F-A4-1 (P3 · doc/code drift · Data model).** `docs/crsf-format.md:54` documents the per-node
grammar with `u16 reserved`, but `cr_records.cpp:114-125,187` now stores v2 node flags there
(bit0 = extent {codec,psize}, bit1 = resident_data blob). The format doc was never updated for v2.
*Fix:* update crsf-format.md node grammar (the code's backward-compat design is sound: old readers
skip the fields, old files read back flags==0).

**F-A4-2 (P3 · micro-delta vs reference rule · Constants).** Our writer skips the deflate attempt
for chunks < 64 bytes (`try_deflate = take >= 64`, cr_crsf.cpp:141). The reference rule has no
threshold: every block is compressed and emitted only if smaller (rsf-format §3.2; live evidence:
sample I_ record 1 tail chunk `hdr=0x800a`, an **11-byte** compressed chunk, and record 3 tail
`hdr=0x8018`, 25 B → 3,089 B). Both produce valid, mutually readable streams — semantics preserved —
but the comment "reference: stored wins" (cr_crsf.cpp:140) misstates the reference rule.
*Fix:* either drop the threshold or fix the comment; ideally document the delta in crsf-format.md.

**F-A4-3 (P2 · MISSING · Data model).** Sessions record **no volume/device identity and no scan
timestamp**. `ScanController::save_session` (cr_scan.cpp:583-589) fills only `fs_kind`, `flags`,
`scanned_bytes` of `VolumeInfo` — `slice_offset`, `slice_bytes`, `logical/physical_sector`, `label`,
`elapsed_ms` are left zero, and there is no SYSTEMTIME anywhere. Og record 2 stores
signature 0x7F + `volumeName[100]` + deviceType + SYSTEMTIME scanTime + elapsed-ms + tick counter
(rsf-format §5.2, live-verified above), and og record 3 is a full "device identity card":
1,781-byte partition struct + 5,400-byte volume-device blob (model/serial/firmware/WWN/interface +
512-byte boot-sector snapshot) used "to verify on restore that the same volume is being re-attached"
(rsf-format §5.3). Consequence: our `recover --in file.crsf --disk N` cannot detect that the session
was taken from a different disk/volume than `--disk N` — restore-side identity verification is
impossible. This is a semantic gap beyond the documented G-11/G-13/G-14/G-23 set.
*Fix:* populate the existing VolumeInfo fields (they exist but are unfilled) + add scan SYSTEMTIME
+ device identity (model/serial/serial-number) and a boot-sector hash; refuse/warn in cmd_recover
on mismatch.

**F-A4-4 (P3 · MISMATCH code-vs-own-semantics · State machine).** `SessionState.stage_done_mask`
is documented as "bit0 quick, bit1 deep, bit2 carve" **done** (cr_records.h:62) but written from
the *config* flags, not completion (cr_scan.cpp:573-574 `st.stage_done_mask = (cfg_.run_quick?1:0)|…`).
A scan cancelled mid-deep persists deep=done. (load_session currently ignores the field, so no
consumer is broken today — but any future resume logic would be.)
*Fix:* derive the mask from stage completion stats, or rename the field.

**F-A4-5 (P3 · MISSING · Capability).** No auto-save of scan sessions. Og auto-saves
`auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` into the module-dir `SaveScan\` folder
(FUN_140273940/§5.5; three real samples ship in the installer). Ours persists only via explicit
`rrecov scan --out` (rrecov.cpp:402). Crash-mid-scan loses everything.
*Fix (minimal):* auto-save at scan end (and optionally at stage boundaries) using the og naming
convention adapted to `.crsf`.

**F-A4-6 (P3 · MISSING · Threading/cancel).** `session::Writer` has no cancel token and no
progress callback; `save_session` serializes the whole tree in one uninterruptible call. Og's
record-4 writer checks `IsCancelled` after every child and reports 40%→~65% progress
(rsf-format §5.4.4: "progress callback: 40% + 50%·done/(2·total)… cancel check @186644").
*Fix:* thread the orchestrator's CancelToken + progress into Writer::add_record (tree serialization
is the long pole — 36 MB for sample I_).

**F-A4-7 (P3 · edge · Behavior).** `Reader::open` treats `chain==0 && crc==0` as an "unfinalized
tail record" and stops indexing (cr_crsf.cpp:243-246). CRC32 of a **zero-length** payload is 0, so a
legitimate empty record (writer allows `add_record(type, data, 0)`) both writes chain=0/crc=0 and
terminates the index walk early. No current caller writes empty records.
*Fix:* reserve one bit in the record header (e.g. the +0x18 reserved dword) as a finalized flag, or
forbid zero-length records in `add_record`.

**F-A4-8 note (session side, MATCH):** multi-session containers: the og reader chain-walks until
**SessionId matches** (rsf-format §2.1), i.e. one .rsf may hold several sessions keyed by 16-byte id;
our reader indexes all records in file order and `read_record` never checks `session_id` against the
container's (cr_crsf.cpp:259-309). Own-format simplification — acceptable, but undocumented; add a
line to crsf-format.md ("one session per file; session_id is informational").

---

## 3. Recovery writer — detailed findings

**F-A4-8 (P2 · MISMATCH · Behavior/error surface).** `SafeWriter::process` aborts the entire queue
on the first per-item error (cr_safewriter.cpp:313-314 `if (u32 rc = recover_item(...)) return rc;`),
which also caps `stats.errors` at ≤ 1 despite the aggregate counter's shape. Og collects per-item
failures and continues: "Failures collected for the recovery log … '%1 files/%2 failed; for details,
you may check the recovery log'" (architecture/04-recovery-workflow.md §7.5, DRWUI strings).
One unreadable file must not cancel the remaining 10,000.
*Fix:* on item error, mark `ItemState::Error`, increment `stats.errors`, continue; return a
summary rc at the end. (Keep hard-abort only for cancel.)

**F-A4-9 (P1 · MISSING · Behavior — primary-path break).** No directory creation anywhere in the
writer or CLI (`rg CreateDirectory src/` → only repair-unrelated hits; rrecov's planner recurses into
`is_dir` children appending `name\` to the path, rrecov.cpp:680-684, but never creates it). Og creates
destination directories per path level: `CreateDirectoryW` in the export loop
(DRWUI_decompiled.c:357842 inside FUN_1401f23a0 "create if not a directory"; also 353909/355495/356147/
356234; 04-recovery-workflow §7.5 "creates destination directories (CreateDirectoryW)"). Compounding
with F-A4-8: recovering **any tree with subdirectories into a fresh destination** fails at the first
nested file (`CreateFileW` → `ERROR_PATH_NOT_FOUND` → `ERR_DEST_UNWRITABLE`, cr_destio.cpp:16-19)
and — because of F-A4-8 — aborts the whole recovery. This is the normal product path (every real scan
result has folders; og even preserves empty folders by creating them). G-08 documents the mkdir gap,
but the queue-abort interaction is undocumented and turns a cosmetic gap into a functional failure.
*Fix:* create dest dirs (recursively) in `enqueue`/`recover_item` (DestIo gains `mkdir`), then fix
F-A4-8. This also restores empty-directory presence, matching og.

**F-A4-10 (P2 · MISMATCH · Constants/Behavior).** Collision naming diverges from the reference.
Ours: `name (2).ext` … `name (62).ext` then `name_dup` (cr_safewriter.cpp:45-69; with
`rename_attempts=64` the loop probes base + (2)…(62) — 61 numbered candidates, " (63)" is computed
but never checked — so even our own "64 attempts" arithmetic is off by ~3). Og export loop:
existence check → `sprintf(L"%s_%d", base)` / `sprintf(L"%s_%d%s", base, n, ext)` (suffix **before**
the extension, counter starting at 0 per decompile), retry cap **10000**
(DRWUI_decompiled.c:357110-357136; extension split via find_last at 357096-357108).
*Fix:* either adopt `name_N.ext` + 10000 cap for corpus parity, or register ` (N)` as a
deliberate delta in PARITY-DELTAS.md with the og citation; fix the loop bounds either way.

**F-A4-11 (P3 · MISMATCH · Constants).** Sanitizer character set/replacement. Ours replaces
`/ * : ? " < > |` with `_` (cr_safewriter.cpp:28-43) — that is the og **session-file-name**
sanitizer (FUN_140286930, rsf-format §5.5). The og **export-name** sanitizer replaces the 9-char set
`: ? \ / | * > < "` (includes backslash) with **`'#'`** (DRWUI_decompiled.c:357076-357080).
The header comment "corpus sanitizer replaces / * : ? " < > | with _" (cr_safewriter.h:98-99) cites
the wrong og function for this call site.
*Fix:* match the export rule (`#` + backslash) or document the delta; correct the comment.

**F-A4-12 (P3 · doc/comment drift · State machine).** `WriterStats.resumed` comment says
"detected complete .part files finalized" (cr_safewriter.h:71) and AUDIT row 35(a) prescribed
"size+hash match on existing part → finalize + count", but the implementation counts **existing
finals** only (cr_safewriter.cpp:119-124). `.part-<id>` names are runtime-counter-based (unstable
across runs), `create_temp` uses `CREATE_ALWAYS` (clobbers stale parts), and nothing ever finalizes a
leftover `.part`. Net: crash recovery restarts the interrupted item from zero (idempotent, safe, but
not the claimed .part resume).
*Fix:* either implement part-resume (size+hash check on `<final>.part-*`) or fix the comment +
AUDIT row disposition.

**F-A4-13 (P3 · safety nit · Behavior).** `WinDestIo::commit` closes the handle before
`MoveFileExW` (cr_destio.cpp:51-57); if flush or rename fails, `close()` then skips the temp
cleanup (it only deletes when `handle != nullptr`, cr_destio.cpp:69) — a leftover `.part` file
remains on failure. Also, collision resolution happens at enqueue-time, not commit-time, so a file
created in between is clobbered by `MOVEFILE_REPLACE_EXISTING` (single-writer assumption).
*Fix:* track a `committed` flag and delete the temp in `close()` unless committed; re-check
existence immediately before rename (or use no-replace rename + retry).

**F-A4-14 (DELTA verified — keep, improve evidence).** Overlap refusal: ours is a code-level hard
refusal per node-extent vs destination-volume-extent intersection (cr_safewriter.cpp:76-92,
ERR_DEST_OVERLAP; tests test_orchestrator.cpp:308-352). Og ground truth, now pinned precisely:
engine-level `CRecoveryMgr::ExportThread` validates the destination only by opening `\\.\X:`
read/write (writability) with `PathFileExistsW` fallback (DRWUI_decompiled.c:355049-355093);
the source-drive block is **UI-level**: `FUN_140684ef0` mismatch → "Source Drive! Change another
one." + confirm button `setEnabled(false)` (DRWUI_decompiled.c:1084101-1084133; UIUX
09-results-phase-deep-dive.md §6). F12's characterization ("validation is UI-level") is confirmed;
our library-level refusal is the documented stronger delta. Residual difference worth documenting:
ours allows same-disk-different-volume (extents don't intersect) with only a CLI warning
(rrecov.cpp:645-656) — og's UI check appears drive-letter-based; same-disk-different-volume og
behavior is UNVERIFIED.

**F-A4-15 (P2 · docs/registry error · Capability claims).** error-fix-registry.md F11 states the
reference behavior is "collision = overwrite". Contradicted by the decompile: the export loop
renames on existence (DRWUI_decompiled.c:357110-357136). F11's *other* half (direct write, no
temp+rename — "writes go to destination via normal file APIs", 04-recovery-workflow §7.5) is
confirmed. *Fix:* correct F11's reference-behavior column (collision = `_N` rename; direct write =
true) so future agents don't "re-fix" a non-defect; our temp+rename remains a justified delta.

**Matches (writer):** per-item `.part-<id>` suffix implemented and unique (AUDIT row 35c fix
verified: cr_destio.cpp:15); compressed-extent whole-block decode with 256 MiB psize/lsize guard
(cr_safewriter.cpp:209-250) ✓ as claimed; cancel mid-item closes + deletes the temp, item state
Error/ERR_CANCELLED, no partial final (cr_safewriter.cpp:165-170 + cr_destio.cpp:61-73) ✓;
resident-prefix streaming + zero-padded resident-only tail (test_orchestrator.cpp:370-419) ✓;
hash: SHA-256 truncated-128-bit compare vs `sha128` set only for carved ≤8MiB
(cr_carve.cpp:30 `MAX_FILE_READ_FOR_HASH = 8ull<<20`, cr_safewriter.cpp:270-284) ✓ internally
consistent; hash-less structured files recover (skip_hashless=false default) ✓; salvage-on-mismatch
commits + counts + CLI surfaces it (rrecov.cpp:720-722) ✓ per AUDIT row 35(b) — but the
`verify_hash` comment "refuse-after-write mismatch is an error" (cr_safewriter.h:61) still carries
the pre-fix ambiguity AUDIT 35(b) said to remove.

---

## 4. Health / SMART — detailed findings

**F-A4-16 (P2 · MISMATCH · Constants/offsets).** Transport mismatch between header claim and code.
`cr_health.h:6-8` states the corpus transports are "IOCTL_SCSI_PASS_THROUGH (0x4D004) with ATA SMART
commands" — this is **verified in the corpus** (DiskHealth_decompiled.c: 18 `DeviceIoControl(…,
0x4d004, …)` sites, e.g. lines 7936/8163/8582; SAT CDB builder with command 0xB0 and SAT signature
0x4F/0xC2 at 7833-7846). But the code issues `IOCTL_ATA_PASSROUGH` via `ATA_PASS_THROUGH_EX`
(cr_health.cpp:27-50) — a different IOCTL (0x4D02C-family) that never appears in the DiskHealth
decompile. Functionally both can read SMART on ATA, but the header presents the corpus route as
what we implement; parity on this axis is claim-only.
*Fix:* either implement the SAT route (SCSI_PASS_THROUGH_DIRECT with ATA_12/ATA_16 CDB — the
corpus-proven path, also what CrystalDiskInfo-lineage code does) or rewrite the header to say
"corpus uses 0x4D004; we use IOCTL_ATA_PASS_THROUGH as an equivalent deliberate delta" and register
it in PARITY-DELTAS.md.

**F-A4-17 (P1 · MISMATCH · Data model).** NVMe health-log (page 2) field offsets are wrong for 4 of
8 fields (cr_health.cpp:281-304): `power_cycles` read at byte 64, `power_on_hours` at 80,
`unsafe_shutdowns` at 96, `media_errors` at 112. Per the NVMe base spec health-log layout, byte 64
is **Host Read Commands**, 80 **Host Write Commands**, 96 **Controller Busy Time**, 112 **Power
Cycles**; the correct offsets are Power Cycles 112, Power On Hours 128, Unsafe Shutdowns 144,
Media and Data Integrity Errors 160 (Data Units Read @32 and Written @48, and the byte 0-5 fields,
are correct). Every NVMe health report shows wrong values for these four counters. The corpus does
not document the og's NVMe offsets (diskhealth--smart-companion.md is a vftable census; the
DiskHealth decompile's NVMe parse was not offset-mapped), so corpus parity here is UNVERIFIED — the
verdict is against the spec our own struct names imply.
*Fix:* correct the four offsets; add a fixture test with a hand-built page-2 image.

**F-A4-18 (P3 · MISSING · Data model).** SMART READ_THRESHOLDS (0xD1) and RETURN_STATUS (0xD8) are
never issued — the header lists both as corpus transports (cr_health.h:7-8) and the corpus confirms
them (DiskHealth_decompiled.c:14112 `uStack_15 = 0xd1`, :4423 `uStack_295 = 0xd8`, :12330 `0xd0`).
Missing: per-attribute threshold/worst comparison ("failing" currently inferred from flags-bit3 or
pre-failure-attribute==0, cr_health.cpp:205-207 — a heuristic with no corpus backing, see UNVERIFIED)
and the drive-level overall health verdict from D8's cylinder-low/high.
*Fix:* issue 0xD1 (thresholds table, same 30×12 layout) and 0xD8 (status), derive `failing_now`
from value ≤ threshold for pre-failure attributes.

**F-A4-19 (P3 · MISSING · Capability).** No multi-transport degradation: the og CAtaSmart engine has
`GetSmartAttributeSat -1A/-2/-2A`, `GetSmartAttributeCsmi` (Intel RST), `GetSmartAttributeNVMeRealtek`,
and a USB-bridge table (`AddDisk - USB11…USB20`, ASM1352R, Si) — diskhealth--smart-companion.md §2;
DiskHealth_decompiled.c:4208-4236, 10761, 11043-11103. Ours: ATA passthrough → legacy 0x7C088 →
give up (ERR_HEALTH_UNAVAILABLE). USB bridges that reject both routes get no SMART at all.
*Fix (parity):* add at least the CSMI route and an NVMe fallback; document the bridge-table gap.

**F-A4-20 (P3 · MISMATCH doc-vs-code · Behavior).** SSD detection: header claims the corpus behavior
"DRWUI's SSD detection (WMI MSFT_PhysicalDisk MediaType==4)" (cr_health.h:9-10) — corpus-verified
(DRWUI_decompiled.c:178880-178887: `ROOT\Microsoft\Windows\Storage` / `MSFT_PhysicalDisk` /
`local_20 == 4` → SSD). The code implements **neither** that nor the header's own inline description
("bus type NVMe => SSD; HDD registry + rotation rate 0 => SSD", cr_health.h:86) — it queries
`StorageDeviceSeekPenaltyProperty` (cr_health.cpp:329-346). Three different stories; the code's
SeekPenalty is a reasonable mechanism but is undocumented and untested against the claim.
*Fix:* align header comment with the implementation or implement the WMI route; register the delta.

**F-A4-21 (P3 · DELTA confirmed + parse nit · Behavior).** TRIM: og **toggles** TRIM via
`fsutil behavior set disabledeletenotify 0|1` (FUN_1400c9450, DRWUI_decompiled.c:178768-178789) in
addition to querying (FUN_1400c91a0/1400c91d0, :178673-178763). Ours is query-only, documented in
the header as policy ("never executed silently") — a legitimate DELTA, now with exact corpus
citations; keep, but note the og exposes a user-driven toggle we lack entirely (capability MISSING
if parity of capability is required). Parse nit: og reads the **first** `"= "` occurrence and checks
the following char (`StrStrA(…,"= ")` → `lpString[2]=='0'`, :178742-178749); ours scans all lines
and lets the **last** one win (cr_health.cpp:317-324) — on multi-line fsutil output (per-FS rows)
the two disagree.
*Fix:* take the first match; document the no-toggle delta with the FUN_1400c9450 citation.

**Matches (health):** legacy `SMART_RCV_DRIVE_DATA 0x7C088` fallback present (cr_health.cpp:52-85)
= DiskHealth:9010/13438/14176 (AUDIT row 43 fix verified); IDENTIFY 0xEC first, then 0xB0/0xD0
READ_DATA (cr_health.cpp:172-190) = corpus command set (0xec/0xb0/0xd0 sites above); 512-byte table,
30×12-byte entries at +2, id/flags/value/worst/raw6 layout (cr_health.cpp:192-209) = ATA-ATAPI-7
and the og's table shape; attribute-name table covers the og-relevant IDs; derived fields
5/197/198/9/10/231/233/194/190 all present (cr_health.cpp:212-239); ATA version word 80, SMART
version from table bytes 0-1 ✓. CLI surfaces unstable-media warnings before scanning
(rrecov.cpp:775-776) — good recovery-first advisory consistent with the product's health pairing
(diskhealth artifact §3). Cosmetic: the NVMe Celsius fraction print `K % 10` is not the decimal
fraction (rrecov.cpp:785-787).

---

## 5. RawFixer — detailed findings

**F-A4-22 (P3 · PARTIAL MISMATCH · Behavior).** Geometry rederivation: our header claims
"TryFixFromSectorsPerCluster (re-derive spc from cluster count invariants, **2 candidate paths**)"
(cr_rawfixer.h:12-14) but the code implements only the cluster-heap-bounds fit
(cr_rawfixer.cpp:360-392, uniqueness → S3 refusal). Og has **two derivation strategies** ("one
computes from the FAT chain, one from the cluster heap bounds", rawfixer-v2 §4) and **re-validates
via CheckRoot** after the fix. Ours lacks the FAT-chain path and any post-repair structural
re-validation (execute's S5 only verifies the bytes landed, not that the FS parses).
*Fix:* add the FAT-chain derivation as a second candidate source and an optional post-execute
structural re-check (probe_family/validate_boot on the written device); fix the header wording.

**F-A4-23 (P3 · DELIBERATE-DELTA (documented) + header clarity).** Scope: ours repairs
boot/checksum/geometry only (S6, cr_rawfixer.h:35-36); the og FATFixer also validates FAT tables,
root dir, clusters (`CheckFatTable`/`CheckRootDir`/`CheckCluster`) and NTFSFixer validates the MFT
(`CheckMFT`, incl. the negative-clusters-per-record log2 sizing — our `ntfs_boot_sane` keeps only
the byte-shape gate at cr_rawfixer.cpp:76-80) and `IsValidRootDir` (rawfixer-fs-repair.md method
table; rawfixer-v2 §3/§5). The S6 refusal is a documented, defensible safety delta — but the
header's "Corpus contract" block lists those methods in a way that reads like implemented behavior.
*Fix:* annotate the header corpus-contract block with "(og-only; out of S6 scope)" per method.

**F-A4-24 (P3 · error-surface nit).** `execute` returns `ERR_REPAIR_UNDO_FAILED` after an S5
read-back/verify failure even when `rollback_all()` fully succeeded (cr_rawfixer.cpp:512-517) —
conflates "repair verify failed, rolled back cleanly" with "rollback itself failed". The test
suite only exercises write-failure rollback (test_rawfixer.cpp:408-440).
*Fix:* distinct rc for verify-failure-after-successful-rollback.

**F-A4-25 (P3 · edge · Constants).** 4Kn handling inside analyze(): the exFAT backup probe and the
via-backup restore hardcode 512-byte sector math (`12 * 512` at cr_rawfixer.cpp:202 and 315-317;
restore `length = 512`), and the primary probes read fixed 512-byte sectors (`sector = 512`,
cr_rawfixer.cpp:183-187). On a 4Kn exFAT volume the backup boot sector lives at byte 12×4096; the
probe reads the wrong offset and `exfat_boot_sane(bak, total, 512)` would reject bps_shift=12
anyway — 4Kn exFAT volumes fall to "no recognizable family". (FAT/NTFS paths restore 512 of the
4096-byte boot sector — semantically tolerable since fields live in the first 512 — but the exFAT
backup path is functionally blind.) The writer-side `execute` gets sector-granularity right via
`IWriteDevice::sector_bytes()` (cr_rawfixer.cpp:468-474) — the gap is analyze-side only.
*Fix:* probe with the device's `geometry().logical_sector` (the `RamDevice`/slice exposes it), and
restore the full sector span.

**Matches (rawfixer, with byte-level corpus evidence):**
- **Transactional protocol** = og contract "transactional — every fixer implements Undo;
  CopySectors snapshots before any write, replayed back by the Undo path" (rawfixer-fs-repair.md
  "Repair strategy" + rawfixer-v2 §4). Ours: snapshot-before-write (cr_rawfixer.cpp:491-496),
  patch-in-span write, read-back verify, reverse-order rollback with unrestored-entry retention,
  sector-coalesced spans (4Kn-safe), `bad_alloc` safety (475-526). Our durable CRC-guarded
  `CRUNDO01` undo log (cr_rawfixer.cpp:125-167) is an ours-only strengthening (og undo is
  in-memory); round-trip + corruption + truncation tests exist (test_rawfixer.cpp:442-462).
- **ChecksumDBR semantics**: og reads 0xB (11) sectors from dbrStartSector, then 1 sector at
  +0xB, compares the stored u32 against the computed sum and verifies **every 4-byte group in the
  checksum sector equals the first** (RawFixer_decompiled.c:10486 `local_240 = 0xb`, :10510
  `param_2 + 0xb`, :10521-10552 fill-compare loop stepping 4). Ours: `exfat::boot_region_checksum`
  over sectors 0..10 (spec-exact skip of VolumeFlags 106-107 / PercentInUse 112,
  cr_exfat_dentry.cpp:26-37) + stored-value + whole-sector fill consistency
  (cr_rawfixer.cpp:294-306) — same shape, spec-exact sum. Note: AUDIT.md row 36's parenthetical
  "VHD-style CRC over 11..209+510" repeats the corpus artifact's *inferred* (and garbled)
  description (rawfixer-v2 §7 flags it INFERRED) — our implementation is right; fix the AUDIT
  wording.
- **Backup-sector constants**: FAT32 backup boot at sector 6 (rawfixer-v2 §5 "CheckBackupDBR —
  FAT32's backup boot sector (sector 6)") ✓ cr_rawfixer.cpp:409; exFAT backup at LBA 12
  (rawfixer-v2 §4) ✓ cr_rawfixer.cpp:202/315; NTFS volume-end backup is spec-correct
  (cr_rawfixer.cpp:213) but the og-side source sector is corpus-UNVERIFIED (see §6).
- **S2/S3 refusal semantics** (clean never touched; ambiguous → refusal) — our safety standard,
  stronger than the documented og behavior (og ambiguity handling is not in the corpus);
  tested (test_rawfixer.cpp:138-159, 270-301, 367-398).
- **CLI consent model**: dry-run default, `--apply` opt-in, `--undo-apply` replay
  (rrecov.cpp:1010-1180) — consistent with S1.

**F-A4-26 (P3 · MISSING · Threading/progress).** Og repair runs async via Concurrency PPL agents
with CCallBackOperate progress streaming "identically to scan progress" (rawfixer-fs-repair.md
"Progress is async"; rawfixer-v2 §3 "the body runs inside PPL agents"). Ours: synchronous
analyze/execute, no progress or cancel hooks. Acceptable for CLI; required for UI parity later.
*Fix (when UI lands):* progress callback + cancel between actions in execute.

---

## 6. UNVERIFIED list (corpus silent — no guess made)

1. **Og recovery-writer hash verification** — no Crypt/hash calls found in the DRWUI export region
   (355200–359500 grepped); our verify-after-write is an ours-only addition (F11 implies it as a fix).
2. **Og zero-fill of unreadable spans during recovery** (our salvage-over-purity behavior,
   cr_safewriter.cpp:252-256) — no corpus evidence either way.
3. **Og cancel-mid-item writer semantics** — only "cancel checks" at ExportThread level
   (04-recovery-workflow §7.3); per-item cancel behavior not decoded.
4. **Og TRIM toggle trigger** — FUN_1400c9450 exists (DRWUI_decompiled.c:178768) but its UI caller
   and consent flow were not traced.
5. **Og SMART "failing attribute" derivation** — DiskHealth's decision logic not extracted; our
   flags-bit3/pre-failure==0 heuristic (cr_health.cpp:205-207) has no corpus backing.
6. **Og NVMe health-log field offsets** — corpus does not map them (see F-A4-17; verdict is
   spec-based).
7. **"Corpus 0x19" unknown-FS constant** — cited in cr_rawfixer.h:19 and
   cr_rawfixer.cpp:429 ("leaves carving path, corpus 0x19"); not found in any artifact
   (factory map in AUDIT row 45 has no 0x19). Citation untraceable → treat as unverified until
   pinned.
8. **Og NTFS backup-boot restore source** (volume-end last sector) — spec-correct in ours; the
   NTFSFixer::FixFromDBR decompile was not read for the source-sector math.
9. **G-08/G-11/G-13/G-14/G-23 issue texts** — they live in the GitHub tracker, not in either repo;
   this audit could only verify G-08 (mkdir absence) against code, not read the issue's own scoping.
10. **Og same-disk-different-volume destination** behavior (see F-A4-14 residual).
11. **Multi-session .rsf containers in the wild** — the reader supports id-keyed lookup
    (rsf-format §2.1) but all 3 shipped samples hold one session; og multi-session behavior is
    design-inferred, not sample-verified.

## 7. Deliberate-delta candidates (for PARITY-DELTAS.md)

| Delta | Status | Evidence |
|---|---|---|
| CRSF own container (magic/version/CRC/type field) vs .rsf GUID-keyed records | documented (crsf-format.md, F10) — keep | rsf-format §2/§2.1 |
| Per-record CRC32 + skip-forward + bomb budget | documented (F10, P1-7) — keep | ours-only vs rsf §2.1 |
| Writer temp+`.part-<id>`+fsync+atomic-rename vs og direct write | documented (F11) — keep, **but fix F11's wrong "collision=overwrite" claim** (F-A4-15) | 04-recovery-workflow §7.5 |
| Overlap refusal in code vs og UI-level block | documented (F12) — keep; add the exact UI citations (F-A4-14) | DRWUI:1084101-1084133, 355049-355093 |
| TRIM query-only (no set) | documented in header — keep; add FUN_1400c9450 citation + first-match parse fix (F-A4-21) | DRWUI:178768-178789 |
| Hash verification of carved ≤8MiB (truncated-128-bit) | undocumented ours-only — register | cr_carve.cpp:30, cr_safewriter.cpp:270-284 |
| Durable CRUNDO01 undo log vs in-memory CopySectors | undocumented ours-only — register | cr_rawfixer.cpp:125-167 vs rawfixer-v2 §4 |
| S6 repair scope (no FAT chains/MFT/dir content) | documented (S6) — keep; annotate header (F-A4-23) | rawfixer method tables |
| Single-session .crsf files | undocumented — add a line to crsf-format.md (F-A4-8 note) | rsf-format §2.1 |

## 8. Suggested fix order

1. **F-A4-9** (P1, mkdir + nested-path failure — primary product path) with F-A4-8 (P2, per-item
   error isolation) in the same change; regression test: tree with subdirs + one failing item →
   others recovered, dirs created.
2. **F-A4-17** (P1, NVMe offsets) + fixture test.
3. **F-A4-3** (P2, session volume/device identity + timestamp), F-A4-10 (P2, collision scheme
   decision + registry), F-A4-15/F-A4-16/F-A4-20 (P2, doc/registry truthfulness — cheap, high value).
4. P3 wave: F-A4-1/2/4/5/6/7 (session), F-A4-11/12/13 (writer), F-A4-18/19/21 (health),
   F-A4-22/23/24/25/26 (rawfixer).
