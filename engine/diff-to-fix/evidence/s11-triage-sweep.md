# S11/L4 — the active-board liveness sweep (TRIAGE) at HEAD

- **Task ID:** S11/L4 (session-11, lane 4)
- **Date:** 2026-10-28/29 (HEAD authored 2026-10-28 17:51 UTC)
- **Core HEAD:** `2498b9fb` (= `8365956e` + the S11/T1 worklog-only commit; `git diff 8365956e 2498b9fb --stat` = worklog.md + worklog/*.md only — the code tree IS 8365956e, the mandated baseline)
- **Corpus HEAD:** `be0cfe9` (unmoved — verified)
- **Method:** every active finding read in full; its PRIMARY core-side defect claim re-verified at HEAD by targeted grep/Read of the cited sites; "X is ABSENT" claims re-grepped across src/include/tests; doc-truth claims re-read at the cited doc lines; og-side counts re-executed against the corpus tree where cheap. Read-only: zero writes inside either audited repo.

## Verdict key
STILL = defect/record stands at HEAD · FIXED = fixed at HEAD (fix commit named) · PARTIAL = some items fixed, some standing (enumerated) · NDV = NEEDS-DEEP-VERIFY (cannot be settled from this workspace; precise check in §4)

---

## §1 The 56-row liveness table

### SEVERE (6 files)

| ID | Verdict | Evidence (one line) | Fix commit |
|---|---|---|---|
| S-040 | STILL | `rg "image_create\|create_image\|cr_ui_image" include/ src/` → 0 hits; rrecov verb dispatch (rrecov.cpp:3125-3150) = 11 verbs, no `image`; no producer export | — |
| S-048 | STILL | `rg "Reconstructed" src/ include/ tests/` → 0 hits; cr_scan.cpp:205/:247 single `root_carved_` from `raw_root_name_()`; :1882 carve_slice under the one root | — |
| S-049 | STILL | default_namer still `File%06u.%s` (cr_carve.cpp:932-942, cite drifted from :907); embedded_name producers still only cr_analysers.cpp:727/:736 (mp4_og_name) | — |
| S-050 | STILL | `ls src/core/repair/` = cr_rawfixer.cpp only; `rg "imgrepair\|docrepair\|file.?repair\|RepairImage" src/ include/ tests/` → 0; CAPABILITY-MATRIX §3 exclusions (Cloud/NAS/BitLocker-derivation/signing/.rsf) carry no repair row (SURFACE-INVENTORY §5 now points at S-050 itself — the interim-truthful state its fix plan step 1 sanctioned) | — |
| S-051 | STILL | cr_scan.cpp:1034-1072 probe_family(pipe) on the whole slice (LBA 0 = MBR) unchanged; dispatch `default: rc = OK; /* unknown FS: quick yields nothing */` still at ~:1425; no partition-table target builder (whole_disk hits = discovery :1586 + resume revalidation only) | — |
| S-052 | STILL | cr_node.h:418-420 "LDM volume nodes keep their layout-relative basis (multi-disk bases are not expressible)"; cr_scan.cpp:1500 `table::LdmVolumeDevice vol(lay, resolver);` still a stack local; cr_safewriter.cpp zero LDM awareness (span hits = dest-spans helper only); recovery_create_impl src = d->dev/d->hold only (cr_ui.cpp:3826-3831) | — |

### MAJOR (24 files)

| ID | Verdict | Evidence (one line) | Fix commit |
|---|---|---|---|
| M-072 | STILL | `rg "path_substring\|state_mask" --glob '*.{h,hpp,cpp,cc}'` → 0 hits; cr_query.h:72-74 still path_prefix + `bool deleted_only` + origin_mask | — |
| M-073 | STILL | `rg "filter_folder\|folder_filter\|filter-folder" include/ src/ tests/` → 0 hits; ScanConfig carries no filter input | — |
| M-074 | STILL | cr_error.h:53-63 still the 5-code 0x5005 family (no TOO_LONG/INVALID/NETWORK); cr_destio.cpp:15-25 still collapses all-but-disk-full to ERR_DEST_UNWRITABLE (the PathIsNetworkPathW hits in cr_transport.cpp:1042 are N-013's overlap-arming, landed 3b5397a1 — different seam; UNC test still pins UNWRITABLE) | — |
| M-075 | STILL | `rg -i chkdsk src/` hits only in-engine checker comments/strings + bcdfix "registered UI-layer" note; `cr_ui_fsck_volume` → 0 hits; only CreateProcess in src/ is cr_bitlocker.cpp:62 (manage-bde); PARITY-DELTAS:2988 still "chkdsk delegation stays OUT OF CORE" | — |
| M-076 | STILL | cr_ui_disk_info (cr_ui.h:449-460) still number/size/sectors/removable/layout/table_ok/model/serial — no TRIM fields; cr_enumerate.cpp 0 trim hits (cr_ui.cpp:4237-4240 = the health-facet fill only); no cr_ui_enumerate_disks_ex / CR_ENUM_FLAG_TRIM | — |
| M-077 | STILL | preview Caps still {max_read 4MiB, max_work, text_snippet} (cr_preview.h); `rg "max_file_bytes\|max_decode_ms\|104857600" src/ include/` → 0; recent preview commits (S-043/S-045/M-085/M-102+M-083) added none of the og's 100 MB gate / 15 s cap | — |
| M-078 | STILL | `rg "turn_off\|TurnOff\|cr_ui_turn_off" src/ include/` → 0 hits; cr_bitlocker.cpp has no `-off` mode in the command builder | — |
| **M-083** | **FIXED** | cr_ui.cpp:3741-3745 `if (node->grade() != tree::Grade::High) node->set_grade(tree::Grade::High)` inside thumbnail_generate_impl (with the M-083/FUN_1402636e0 comment); `CrUi.ThumbnailSuccessPromotesGradeTo2` at test_cr_ui.cpp:3535; Resolution section in-file says FIXED — **but the file was never retired per the purge convention (still in findings/MAJOR/)** | **e56baac7** |
| M-086 | NDV | defect sites live in the rda-driver repo (rda_version.h:3-5, install.ps1:99-174) — repo absent from this workspace; core-side consumption (release.yml FloorVersion, no stamping step) unchanged | — |
| M-087 | NDV | defect site `scripts/winpe/regdrv-rda.reg:30-31` in the rda-driver repo — absent here; the devsign evidence snapshot is v0.2.2-dated (older than the audited 323e75a) | — |
| M-089 | NDV | defect site `src/bench/rdabench.c:390` in the rda-driver repo — absent here (rrecov.cpp:2435's "rdabench-compatible" comment is not the bench source) | — |
| M-096 | STILL | `rg -i "mobi\|BOOKMOBI\|0x0E98E0D0A" src/core/carve/` → 0 hits | — |
| M-098 | STILL | `rg -i "hik\|hikvision\|private_stream\|FMT_HIK" src/core/carve/` → 0 hits | — |
| M-099 | STILL | `rg -i "rifx\|xpacket\|FMT_AEP" src/core/carve/` → 0 hits | — |
| M-101 | STILL | `rg -i "version_group\|versions node\|Tags Extracted\|tags_extracted" src/ include/ tests/` → 0 hits | — |
| **M-102** | **FIXED** | cr_ui.h:1278-1285 `cr_ui_thumbnail_generate` (two-call) + `cr_ui_thumbnail_format_supported`; cr_ui.cpp:3685-3765 impl (caps pass-through, F-1 basis, ERR_NO_MEMORY size probe); tests test_cr_ui.cpp:3441/:3494/:3513; Resolution section in-file says FIXED; FIXED-LEDGER rows :151-152 exist (with "(this landing)" placeholder SHAs) — **file never retired (still in findings/MAJOR/)** | **e56baac7** |
| M-103 | STILL | root still `u"Lost Files"` (cr_scan.cpp:202/:244); `rg -i "LostDir\|File Path Lost" src/` → only 3 unrelated comments (cr_node.h:177, cr_exfat_quick.cpp:802, a test comment) — no grouping construction | — |
| M-104 | STILL | cr_scan.cpp:1633-1638 candidate root still `u"Candidate @" + " %llu"(start_lba)` (anonymous, identity-free); scan_candidate passes the same root for active/deleted/lost | — |
| M-105 | STILL | DRWService only in comments (cr_bitlocker.cpp etc.); `rg "Read FVE" src/` → 0; no SERVICE_AUTO_START in code; R-83's false note still at REFERENCE-API-GAP:321/:425 ("install lives in driver-repo CI scripts") | — |
| M-106 | STILL | validate_one (cr_analysers.cpp:4168+) still `64 << 20` cap at :4179 with the "DWG-pattern bounded extent stands in" comment :4166-4167; validate_3ds :4265/:4274 + ncsd :4296 same (cites drifted +320 lines from the M-084/M-091-era file) | — |
| M-107 | STILL | 15 "stands in\|recorded follow-up" comment sites still in cr_analysers.cpp (rg -c = 15); the 64<<20 stand-in caps unchanged | — |
| M-108 | STILL | cr_preview.cpp:868-876 head gate still `head[i] >= 0x7F → unsupported`; text_sniff :579 still 0x20-0x7E; `rg "TxtViewFiles" src/ include/ tests/` → 0 hits (no whitelist) — unaffected by the e56baac7 thumbnail-ABI landing (ABI layer only) | — |
| M-109 | STILL | ThumbnailCaps::max_read still `32u << 20` (cr_thumbnail.h:88) with the refuted "NO input byte cap" comment :84; test_thumbnail.cpp:186 still pins 32MiB with the refuted doctrine; no PARITY-DELTAS row for the bound | — |
| M-110 | STILL | `rg -c "eaDir" src/ include/ tests/` → 0 hits; scan_btrfs (cr_btrfs_engine.cpp:480) descends unconditionally; cr_filterfolder pools carry no @eaDir | — |

### MINOR (21 files — the N-129 collision means two files share one ID; both triaged)

| ID | Verdict | Evidence (one line) | Fix commit |
|---|---|---|---|
| N-090 | STILL | list_crsf_files (cr_ui.cpp:388 Win/:451 POSIX) still enumerates `*.crsf` with no `auto_` filter; cr_ui.h:856/:919 still `label[32]`, :918 `save_unix` u64 seconds; no device_type field | — |
| N-093 | STILL | cr_registry.cpp:710 MID `{14, 64ull<<20}` (og 10,000,000); :809 GZ `{18, 512ull<<20}` (og 20/400,000,000); :789-798 PSP `{0x20,128MiB}`; :799-808 SIT `{16,512MiB}` — all four pairs unchanged, MID/GZ still undocumented | — |
| N-096 | STILL | identity IOCTL constants defined at cr_rda.h:50-52 only; the DeviceIoControl sites in cr_transport.cpp issue kIoctlGetInfo/kIoctlGetVersion + Win32 disk IOCTLs — zero identity-IOCTL issuance | — |
| N-107 | NDV | all 14 stale-count sites live in the rda-driver repo (README/architecture/ground-truth-contracts/rda_ioctl.h/test_compat.c etc.) — repo absent from this workspace (core's DRIVER-CONTRACT.md:131/:202 already says 19 correctly) | — |
| N-111 | STILL | `rg -i "jp2\|jpx\|jpm\|jP \|0x0d0a870a\|6a502020" src/core/carve/` → 0 hits (HEIC ftyp rows don't cover the jP-signature family) | — |
| N-112 | STILL | `rg -i "lzh\|lh5\|lharc\|-lh" src/core/carve/` → 0 hits | — |
| N-113 | STILL | `rg -i "BEGIN:VCARD\|<gpx\|libopenshot\|diagram\|BiffProject" src/core/carve/` → 0 hits | — |
| N-115 | STILL | cr_analysers.cpp:847-854 DOCX gate still checks only the FIRST entry name `[Content_Types].xml`; no rels-path split, no ODF/EPUB/iWork/HWP/CDR tags (the ca701711 M-093 XML/SVG landing did not touch the ZIP typing) | — |
| N-121 | STILL | cr_bootfix.cpp has no ESP/GPT-system-partition discovery (rg MountGpt/esp/C12A7328 → namespace hits only); boot-fix still requires host-supplied `--boot-fix-bcd` (rrecov.cpp:23/:2521-2526) | — |
| N-122 | STILL | test_nvramfix.cpp:417 `MissingBootVariableStopsTheWalk` still pins stop-on-missing; no create/delete path in nvramfix; `rg "create/delete" docs/PARITY-DELTAS.md` → 0 (still unregistered) | — |
| N-125 | STILL | no disconnect classification anywhere (`rg -i "disconnect\|reattach"` src/ → 0 code hits; only a cr_csmi.h comment lists win error codes); no ERR_DEV_GONE/ERR_DISCONNECT in cr_error.h | — |
| N-126 | STILL | `rg -i "unsaved" src/ include/ tests/` → 0 hits; query Category enum unchanged (None..Bookmarks, mask 0x1FF) | — |
| N-127 | NDV | all six doc-vs-code mismatch sites live in the rda-driver repo (rda_ioctl.h:360-368, requirements.md:51, ground-truth-contracts §2.2, rda_trace.h:30, rda_ioctl.c default, security-review doc) — repo absent | — |
| N-128 | STILL | docs/CAPABILITY-MATRIX.md:21 still cites `.github/workflows/{driver-install,core-install,staged-scenarios,release}.yml; .github/actions/install-driver/action.yml` while the tree has {checks,installer,release,test-benchmark}.yml and NO .github/actions; DONE verdict still buries the ext4/HFS+/Btrfs/ZFS/ReFS staging follow-up | — |
| **N-129 (parity-og-core-inventory)** | **STILL (all six items — see §2)** | register values re-read at HEAD: `:53` "(44 DLLs)", `:267` "CORE_RL_* (19)", `:252` "all 25 .mo", `:73` "SURFACE-INVENTORY.md:84", `:30` "EUOfficeTools.exe", `:100` "og-codebase/ (dir)" row — all six unchanged; actuals re-counted from the corpus: 40 / 24 / 30 / :97 / EuOfficeTools.exe / no self-nest | — |
| **N-129 (fix-wave-bookkeeping)** | **PARTIAL** | item 1 FIXED (REFERENCE-API-GAP:376 R-28 row now "engine landed…; cr_ui exposure = M-102 | PARTIAL (engine PRESENT; ABI open)", by 5fecdae1); item 6 RESOLVED (no findings/FIXED/ dir); item 3 PARTIAL (5fecdae1 repaired the ledger per-row batch cites e4f3184/995ba5d/32ef33b, but FIXED-LEDGER:6 header + :342 S-041 row still cite phantom `771e2ec`, SUMMARY:213 still cites `95122bd`; all five SHAs re-verified ABSENT via cat-file); item 5 PARTIAL (M-080 row :150 / S-043 :303 / M-085 :307 exist; **M-082 still has no row**; M-085's row carries "(the parallel lane)" not a SHA); items 2/4/7/8 STILL (R-31 :379 "PARTIAL…missing" vs :225 "PRESENT"; ledger :154 M-090 row still `180053de0`; ABI-CONTRACT:525 §7g still cites cr_ui.h:916-944 which now UNDERSHOOTS the decl end ~:945-946 — the mutation the S11/T1 worklog recorded; cr_thumbnail.cpp:11-14 file-top TIFF matrix still lacks LZW while cr_thumbnail.h:40 carries it) | 5fecdae1 (items 1+3-rows) |
| N-130 | STILL | `rg -i "VideoFileRepairer" docs/` → 0 hits; the register's VideoViewer row (:107) names MediaGraph/ffmpeg/avcodec but not the exe; no disposition row exists | — |
| N-131 | STILL | cr_records.cpp:177 still `put32(out, parent_ids[i])` (structural index only — cite drifted from :175); `rg "parent_anchor\|parent_reference\|parent_frn" src/ include/` → 0; the faa547f7 N-100 landing added the +0x60 carrier (nflags bit 512 at :175/:272/:474) — the +0x40/+0x42/+0x54 PARENT anchors remain unpersisted | — |
| N-132 | STILL | API-DELTA.md rows 44-45 still say "landed" for TxtViewFiles (row 44) and PriorityDisplayType/SumaryFileType (row 45); `rg "TxtViewFiles\|SumaryFileType\|PriorityDisplay" src/ include/` → 0 code hits; cr_grading.h:115 still plain `category_counts[8]` | — |
| N-133 | STILL | fill_volume_label at rrecov.cpp:331-362 still fills image targets from the FILENAME STEM with the refuted premise comment at :329-332 ("the og has no image-scan naming semantics"); ext4 decode cr_ext4_engine.cpp:200-201 still zero-consumer dead code; no NTFS $VOLUME_NAME / FAT boot-label decode (the exfat/fat "volume label" hits are entry-type filters, not decodes); fill_identity_card (cr_ui.cpp:1850+) still fills model/serial/firmware — not the label | — |
| **N-134** | **PARTIAL** | **the finding's central premise is refuted at HEAD — and was refuted at filing:** all six named binaries + the telemetry set carry disposition rows in docs/PARITY-OG-CORE-INVENTORY.md (Boot.dll :119, BootDriver.dll :120, CloneModule.dll :124, peformat.dll :155, EuDownloader.exe :78, legacy.dll :92, AliyunWrap :52, EuUserRate :85, firebase* :88, send_dmp :106) — 18 name-hits at both 44f043a5 and deea0241 (rows landed with the b202f9ed agent3-inventory v2.1, long before the S10-F filing; the filing's "zero trail hits" grep was scoped to diff-to-fix/ + the I-010/I-019/I-027/SURFACE-INVENTORY registers and missed the inventory register in docs/). RESIDUAL still standing: the PARITY-DELTAS half of the claim — `rg "CloneModule\|EuDownloader\|BootDriver\|peformat\|legacy.dll\|Boot.dll" docs/PARITY-DELTAS.md` → 0 hits (the inventory's own CloneModule row still says "Register the exclusion D-59k-style" — that D-row cross-registration and the S-029 write-path cross-note are the true remainder). Recommend re-scope or retire-and-refile as the D-row cross-ref task | — (rows pre-exist at b202f9ed) |

### INFORMATIONAL (5 files)

| ID | Verdict | Evidence (one line) | Fix commit |
|---|---|---|---|
| I-026 | STILL-ACTIVE (accurate posture record) | DRIVER-RELEASE.md:26/:63/:134 still documents unsigned-by-design/SignMode=Off/OSSign-disabled; NOTE: its cite `.github/actions/install-driver/action.yml:44-64` is STALE at HEAD (no .github/actions — the CI consolidation N-128 documents) — cite-drift on an INFO record, not a defect flip | — |
| I-027 | STILL-ACTIVE (accurate register) | the five decoded contracts still absent from core (`rg "recoveryFree\|EnableTotalSize\|nMaxItemCount\|addtask" src/ include/` → 0 hits); corpus assets unmoved at be0cfe9 | — |
| I-028 | STILL-ACTIVE (accurate register) | the engine's five deliberate differences unchanged (max_edge 256 both-axis, RGBA, PSD/PNM/HDR via stb, guards — cr_thumbnail.h Caps re-read); the e56baac7/df08c07e preview landings did not alter them | — |
| I-029 | STILL-STANDING (the doc-truth overclaim) | docs/DRIVER-CONTRACT.md §4.4 (:131) still "EUDCPEPM contract — 19 recognized codes" + :202 "preserved byte-for-byte" — no provenance note added for the 4 corpus-unverifiable codes | — |
| I-030 | **PARTIAL** | item 1 FIXED: modules/ldm.md:1 now reads "MBR/GPT/EBR covered in this file's own table rows; the former `table.md` reference was dangling — N-104" (landed with batch C); items 2-3 STILL: `ls diff-to-fix/modules/` has NO reghive.md and NO apply.md | 32ef33b5 (batch C, via N-104) |

---

## §2 The S10-B N-129 (parity-og-core-inventory) six-item re-check at HEAD

All counts re-executed against `/home/z/my-project/audit/easeus-drw-research/og-codebase` at corpus `be0cfe9`:

| # | Register site (docs/PARITY-OG-CORE-INVENTORY.md) | Register says at HEAD | Actual (re-executed) | Verdict |
|---|---|---|---|---|
| 1 | `:53` (api-ms row) | "api-ms-win-* (**44** DLLs)" | `ls *.dll \| grep -c api-ms-win-` → **40** | **STILL WRONG** |
| 2 | `:267` (3-D decode matrix block, line "CORE_RL_* (19) + IM_MOD_RL_* (7)") | "CORE_RL_* (**19**)" | `ls EUFileRepair/ \| grep -c CORE_RL` → **24** | **STILL WRONG** |
| 3 | `:252` (3-F per-FS row) | "all **25** .mo runtime modules WRAPPER-VERIFIED" | `ls PartitionRecovery/*.mo \| wc -l` → **30** | **STILL WRONG** |
| 4 | `:73` (DRWReport row) | cites "SURFACE-INVENTORY.md**:**84" | the crash-supervisor/DRWReport row is at SURFACE-INVENTORY.md**:97** (:84 = the Imaging row — re-read both) | **STILL WRONG** |
| 5 | `:30` (UI/UX layer paragraph) | "**EUOfficeTools.exe**" | the file is `EuOfficeTools.exe` (lower-case u's; `find . -iname *officetools*` → ./EuOfficeTools.exe + BUILDPE copy) | **STILL WRONG** |
| 6 | `:100` (appendix row) | "og-codebase/ (dir) \| RESEARCH-COPY \| The corpus's own source snapshot inside the installer tree" | `ls -d og-codebase/og-codebase` → **NO such self-nested dir** | **STILL WRONG (stale row)** |

**All six items remain unfixed at HEAD.** Note: SURFACE-INVENTORY.md:222 (its §8 sweep record) already carries the correct values (25→30, :84→:97, …) — the corrections were recorded in the sweep narrative but never applied to the register itself. N-129-parity stays ACTIVE.

---

## §3 Board sanity

1. **File counts on disk (filesystem truth):** SEVERE 6 · MAJOR 24 · MINOR 21 · INFO 5 = **56 files**, 55 distinct IDs (the two N-129 files). No other collisions spotted (all other IDs unique — verified by filename survey).
2. **findings/FIXED/ purge convention: HOLDS** — no `findings/FIXED/` directory exists at HEAD (`ls` → NO-DIR).
3. **tracked-open: 4 files** (OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2) — matches the "4 tracked-open" close record.
4. **FIXED-LEDGER.md row arithmetic:** 283 pipe-rows − 1 separator − 1 header = **281 data rows** → the **"281 resolved" count (the b5da834 side of the conflict) is the true one; the HEAD-side top line claiming "282" is inflated by one.**
5. **NEW DEFECT — unresolved merge-conflict markers in FIXED-LEDGER.md:** lines 22/28/32 carry raw `<<<<<<< HEAD` / `=======` / `>>>>>>> b5da834 (repair(trail): the N-129-flagged residuals…)` markers around the counts stack — 5fecdae1's phantom-SHA repair was cherry-picked/rebased with the conflict left in, and it survived deea0241 and HEAD. This is the exact class 64a3b18c swept once before ("sweep the conflict markers the batch C conform's push window left"). Both count lines (282 vs 281) and both batch-cite sets (phantom 954c386/e1e5124/13245c0 vs real e4f3184/995ba5d/32ef33b) sit inside the markers.
6. **M-083/M-102 double-count:** the ledger already carries rows for M-083 (:151) and M-102 (:152) — with **"(this landing)" placeholder commits, never filled with e56baac7** — while the finding files still sit on the ACTIVE board. So the 281 "resolved" and the 56 "active" both count them. Board truth after retiring them: **54 genuinely-active + 2 retire now**.
7. **Ledger residuals re-confirmed (N-129-fix-wave items):** M-082 has NO row; the M-090 row :154 carries `180053de0` (a decompile function name, not a SHA); FIXED-LEDGER:6 (header) and :342 (S-041 row) still cite phantom `771e2ec` as M-090's landing; SUMMARY.md:213 still cites phantom `95122bd` for M-082.
8. **SUMMARY.md / README.md top-line board counts are STALE by the S10-F filings:** README.md:41-42 and SUMMARY.md:49-50 both say "**54 active (6 SEVERE · 24 MAJOR · 19 MINOR · 5 INFO) / 281 resolved / 4 tracked-open**" (the lane-B push state). The actual board is 56 files (6/24/**21**/5): the +2 = S10-F's N-133 + N-134 (deea0241), never folded into the top-line counts. The known N-129 collision accounts for MINOR-files (21) exceeding distinct-MINOR (20) by one; the README/SUMMARY "19" is short even of the distinct count. The worklog (S10-F close, worklog.md:2656) and the S11/T1 open record correctly say 56. (SUMMARY's narrative DOES list N-133/N-134 at :5455/:5461 — only the top-line counts lag.)
9. **Cite drift noted (no verdict change):** several findings' file:line cites shifted with the recent landings while their defects stand — S-049 (cr_carve.cpp:907→932), M-106/M-107 (cr_analysers.cpp +~320 lines), N-131 (cr_records.cpp:175→177), M-104 ("Candidate @ " → "Candidate @" + snprintf, same output), N-129-fix-wave item 7 (ABI-CONTRACT §7g cite now UNDERSHOOTS).

---

## §4 NEEDS-DEEP-VERIFY (5 findings — the rda-driver repo is NOT in this workspace)

All five were filed against driver `323e75a` (v0.2.4); the repo `heicfast/rda-driver` is absent from /home/z/my-project (only core_recovery + easeus-drw-research exist); the session-9 scope pin (SUMMARY.md:277) already records them as "stand as filed… not re-verified". The devsign evidence snapshot (evidence/executions/devsign-run-40549660074) resolved tag **v0.2.2** — older than the audited v0.2.4, so it cannot settle them either.

| ID | Precise check the lead must run (at the rda-driver repo's current HEAD) |
|---|---|
| M-086 | read `src/driver/rda_version.h:3-5` (still `RDA_VERSION_MAJOR 1 / MINOR 0`?) + whether any release workflow now stamps the RC (grep `stamp\|version` in the driver's release flow) + `install.ps1:99-174` tag derivation — the exit-2 upgrade deadlock stands iff the FileVersion is still 1.0.0.0 |
| M-087 | `od -t x1z scripts/winpe/regdrv-rda.reg` around :30-31 — is the UpperFilters value still the invalid `hex(7):r,d,a,\,0` (5 ASCII bytes) or the fixed `hex(7):72,00,64,00,61,00,00,00,00,00`? (+ the SA-9 Start-posture split) |
| M-089 | read `src/bench/rdabench.c` ~:390 — does the probe list still carry the phantom-zero `0x6000244x` values instead of the real `0x6002440/44/48` (or, better, the header-derived IOCTL_RDA_* names)? |
| N-107 | grep the driver repo for the 14 stale sites ("15-command", "18 codes", "15/18-command", the `0x800C2AC8..0x800C2B38` range) — README.md:31/:42, docs/architecture.md:19/:72, docs/ground-truth-contracts.md:194/:202-219, docs/requirements.md:56, src/shared/inc/rda_ioctl.h:11/:154, src/driver/inc/rda_p.h:13, src/driver/rda_registry.c:14, scripts/verify/verify_reference_binaries.py:30, tests/unit/test_compat.c:229, src/driver/rda_ioctl.c:6-7 |
| N-127 | the six doc-vs-code mismatches: `rg "SeCheck\|ZwQueryInformation\|Token\|PRIVILEGE\|admin" src/driver/rda_ioctl.c` (the claimed admin gate); requirements.md:51 ("512-byte sectors" prose); ground-truth-contracts.md §2.2 (pre-G2 empty-name prose); rda_trace.h:30 (RDA_DP_COMPONENT unparenthesized/dead); rda_ioctl.c RdaSessionDispatch trailing default; docs/audit/2026-10-22-security-review.md "56/57 checks" |

---

## §5 Headline results for the lead

- **2 FIXED-AT-HEAD, retirement pending:** M-102 + M-083 (both fixed by e56baac7 — exports, promotion wiring, tests, in-file Resolution sections, even FIXED-LEDGER rows all exist; the finding files were simply never moved off the active board; the ledger rows' "(this landing)" placeholders need the real SHA e56baac7).
- **3 PARTIAL:** N-129-fix-wave (items 1+6 done, 3+5 half-done, 2/4/7/8 standing), N-134 (central premise refuted by pre-existing register rows — re-scope to the PARITY-DELTAS D-row cross-refs or retire), I-030 (table.md cite fixed by N-104's batch C; reghive/apply rows still missing).
- **5 NDV:** the rda-driver-repo findings (M-086/M-087/M-089/N-107/N-127) — two-repo scope, driver repo absent here.
- **46 STILL** (incl. the 4 accurate INFO registers I-026/I-027/I-028/I-029).
- **Board-sanity catches:** FIXED-LEDGER raw merge-conflict markers + the 282-vs-281 count ambiguity; README/SUMMARY top-line "54 active / 19 MINOR" stale vs the actual 56 files / 21 MINOR files; M-082's ledger row still missing; N-129-parity's six register defects all standing (with the corrected values already sitting in SURFACE-INVENTORY's sweep record, unapplied).
