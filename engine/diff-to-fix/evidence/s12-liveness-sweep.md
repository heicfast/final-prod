# Liveness sweep — 55 active findings at core HEAD (sub-agent S12/A)

**Repo:** `/home/z/my-project/audit-work/core_recovery` — **audited tree:** HEAD `36c1d3e5` (a worklog-only
commit on top of `c7fa527c`; the code state is byte-identical to `c7fa527c`, the HEAD named in the task).
**Method:** each finding file read; each cited core file/symbol re-located at HEAD (line numbers shifted —
code located by symbol/pattern); `git log --all --grep` swept for fix commits per finding ID and per
defect class. No file inside the repo was modified.

**Fix-commit landscape checked first:** no `fix(...)` commit in the whole history names any of the 55 active
IDs. The recent landings the prompt mentions are all for findings already retired from the active board:
M-110 `b95c924b` (btrfs @eaDir), N-111 `461825d8` (JP2), M-108 `aac75bba` (TxtViewFiles), M-077 `04707ba1`,
M-074 `d6c02914`, M-072 `f6fd86fb`, N-093 `03ad5523`, M-102/M-083/N-128(old) `740d9b56`, M-079 `0b76757c` +
convergence `53eb9fff`, M-080 `5ccc0dae`. The session-11D lane (`bcf31316`) *claimed* the SEVERE tier
(S-051→S-049→S-048→S-052→S-050→S-040) but **no fix commits for any of them ever landed** — verified by code
inspection below and by the absence of any `fix`-prefixed commit naming them.

## Verdict summary

| Verdict | Count | IDs |
|---|---|---|
| **LIVE** (defect still present) | **41** | all 7 SEVERE; M-073,M-075,M-076,M-078,M-098,M-099,M-101,M-103,M-104,M-105,M-106,M-107,M-109,M-111 (14 MAJOR); N-090,N-096,N-112,N-113,N-115,N-121,N-122,N-125,N-126,N-128,N-129,N-130,N-131,N-133,N-134,N-137,N-139,N-140,N-141 (19 MINOR); I-029 (1 INFO) |
| **PARTIALLY-FIXED** | **4** | N-132 (by `aac75bba`), N-135 (by `5fecdae1`+`2cf3f4d0`+`f166440b`+`c7fa527c`), N-138 (by `d50a54dc`), I-030 (by `32ef33b5`) |
| **N/A-DRIVER-REPO** (defect lives in heicfast/rda-driver, not present in this workspace; not verifiable here) | **5** | M-086, M-087, M-089, N-107, N-127 |
| **REGISTER** (informational by design — no core defect exists to fix; register intact at HEAD) | **5** | I-026, I-027, I-028, I-031, I-032 |

No finding's cited **core** file is absent at HEAD — every cited file under `src/`, `include/`, `docs/`,
`diff-to-fix/` exists; only line numbers shifted (noted per row). The five driver-repo findings cite files
(`src/driver/rda_version.h`, `scripts/winpe/regdrv-rda.reg`, `src/bench/rdabench.c`, driver-repo docs) that
do not exist anywhere in this workspace.

---

## SEVERE (7)

| ID | Verdict | Evidence at HEAD (file:symbol) | Fix commit |
|---|---|---|---|
| S-040 | **LIVE** | `rg "image_create\|create_image\|cr_ui_image"` over `include/`+`src/` → 0 hits; `src/core/storage/cr_image.cpp` header still "disk-image scan targets (S-030): flat / VMDK / VHD / VHDX" (reader only); `src/win/cr_image_win.cpp:37` opens images `GENERIC_READ` only; `src/app/rrecov.cpp` CLI dispatch has no `image` verb; no image-create export in `cr_ui.h` | none |
| S-048 | **LIVE** | `src/core/orchestrator/cr_scan.cpp:145` `raw_root_name_()` (single configured/default root), `:205`/`:247` the ONE `root_carved_` global root, `:1888` `carve_slice(..., root_carved_, ...)` — every carved node under one root; `"Reconstructed"` → 0 hits in `src/`,`include/`,`tests/` | none |
| S-049 | **LIVE** | `src/core/carve/cr_carve.cpp:949` `default_namer` → `"File%06u.%s"`; namer chain `:459`/`:660`/`:848`; `src/core/carve/cr_analysers.cpp:727`/`:736` still the ONLY two `embedded_name` producers (both `mp4_og_name`, N-063); no content-derived prefixes anywhere | none |
| S-050 | **LIVE** (implementation half) | `src/core/repair/` still contains only `cr_rawfixer.cpp` (transactional boot-sector/FS-structure repair); no photo/video/office/archive/HWP/raw file-repair capability, ABI op, or CLI verb. NOTE: the registration-drift half of the finding (SURFACE-INVENTORY §5's false "OUT-OF-SCOPE (CAPABILITY-MATRIX §3)" cite) was corrected on the board at the finding's own filing — `diff-to-fix/SURFACE-INVENTORY.md:98` now truthfully records "unimplemented AND the prior row's OOS citation was FALSE" | registration half repaired by the S-050 filing wave (board-side); no code fix |
| S-051 | **LIVE** | `src/core/orchestrator/cr_scan.cpp:~1040` the `fs_hint == Unknown` probe still calls `probe_family(pipe)` (reads LBA 0 — MBR/GPT on a whole-disk view); `:1432` dispatch default `rc = OK; /* unknown FS: quick yields nothing; deep/carve still run */`; `disk_layout_` still used only for session records/resume revalidation (`:847-890`, M-042), never to build scan targets; CLI: `src/app/rrecov.cpp:1267` `cfg.whole_disk = false` default, `:1316` opt-in `--whole-disk` — plain `scan --disk N` never consults the partition list | none |
| S-052 | **LIVE** (STALE-CITE lines) | `src/core/orchestrator/cr_scan.cpp:1506` `table::LdmVolumeDevice vol(lay, resolver);` — still a STACK local that does not survive the scan; `include/cr/tree/cr_node.h:420-421` still documents "LDM volume nodes keep their layout-relative basis (multi-disk bases are not expressible; registered limitation)"; `src/ui/cr_ui.cpp:3803` `recovery_create_impl` builds the SafeWriter over the source device (cite was :3698 — shifted ~+105 lines) | none (the F-1 absolute-extent rebase `cd9ea168` predates the finding and does not cover multi-disk LDM layouts) |
| S-053 | **LIVE** | `src/core/refs/cr_refs_engine.cpp:1101-1103` — verbatim unchanged: `const std::u16string name16 = name_from_key(ev.key, ev.key_len); if (name16.empty() || ev.val_len < 0x28) return;` — unnamed $DATA-only rows still dropped before `parse_node` | none |

## MAJOR (17)

| ID | Verdict | Evidence at HEAD | Fix commit |
|---|---|---|---|
| M-073 | **LIVE** | No caller folder-filter input in `include/cr/orchestrator/cr_scan.h` `ScanConfig` nor the ABI twin `include/cr/cr_ui.h` (`rg "filter_folder\|folder_filter"` → 0 hits in headers); the only scan-time filter is still the fixed junk blocklist (`cr_filterfolder.h`, included at `cr_scan.cpp:39`) | none |
| M-075 | **LIVE** | `rg -i "chkdsk\|fsck"` over `src/`+`include/` → only the in-engine checker (`cr_fscheck.*`) + comments; `cr_ui_fsck_volume` → 0 hits; external launch absent; `include/cr/bcdfix/cr_bcdfix.h:36` still registers ChkDsk.exe delegation as UI-layer | none |
| M-076 | **LIVE** | `include/cr/cr_ui.h:449-461` `cr_ui_disk_info` fields: number/size/sectors/removable/layout/table_ok/model/serial — **no TRIM fields**; TRIM still only in the health facet `cr_ui_smart_report` (`cr_ui.h:1392` `trim_known, trim_enabled`) | none |
| M-078 | **LIVE** | `src/win/cr_bitlocker.cpp` function census at HEAD: `is_fve_volume`/`unlock`/`unlock_with_dictionary`/`build_unlock_command`/auto-unlock family — **no turn_off/decrypt function**; `rg -i "turn_off\|TurnOff"` over `src/`+`include/` → 0 hits | none |
| M-086 | **N/A-DRIVER-REPO** | Entire fix surface (`src/driver/rda_version.h`, `rda.rc`, `scripts/install.ps1`, release stamping) lives in heicfast/rda-driver — repo not present in this workspace (only `core_recovery` + `easeus-drw-research` exist). Core-side hook unchanged: `.github/workflows/release.yml:291` still consumes with `-FloorVersion v0.2.4`. Board status: PUSHED-FORWARD (tracked-open OQ-S10-3) | none possible here |
| M-087 | **N/A-DRIVER-REPO** | `scripts/winpe/regdrv-rda.reg` is a driver-repo file — absent here; the corpus contrast is on-file in the research repo | none possible here |
| M-089 | **N/A-DRIVER-REPO** | `src/bench/rdabench.c` + `rda_ioctl.h/c` are driver-repo files — absent here | none possible here |
| M-098 | **LIVE** | No HIK format: `rg -i "hikvision\|private_stream\|FMT_HIK"` over `src/core/carve/` → 0 hits. `FMT_MPEGPS` generic row exists at `src/core/carve/cr_registry.cpp:391` (key `00 00 01 BA`) with no private-stream-1 discriminator — a HIK file carves as generic MPEG-PS at best, never `.hik` | none |
| M-099 | **LIVE** | No AEP format: `rg "RIFX"` and `-i "aftereffects\|\.aep"` over `src/core/carve/` → 0 hits; no row/validator/raw-sign entry | none |
| M-101 | **LIVE** | No "Versions" node concept (NTFS walk still dedups multi-version record siblings) and no "Tags Extracted" grouping: `rg -i "TagsExtracted\|tags_extracted\|multi-version"` over `src/`,`include/`,`tests/` → 0 hits; tree roles unchanged | none |
| M-103 | **LIVE** | `src/core/ntfs/cr_mftwalk.cpp:1082-1107` phase B still reparents orphans FLAT to `root_lost` (`tree.reparent(e.node, root_lost); ++stats.orphans`); `"File Path Lost"` and `"LostDir"` → 0 hits in `src/`+`include/`+`tests/`; roots still `u"Lost Files"` (`cr_scan.cpp:202/:244`; `cr_mftwalk.h:14`) | none |
| M-104 | **LIVE** (STALE-CITE lines: :1629→1639, :1728-1763→~1755-1790) | `src/core/orchestrator/cr_scan.cpp:1639` `std::u16string role = u"Candidate @";` — identity-free anonymous candidate roots; `:1645` candidates pushed under `add_root(role)`; `run_engine_dispatch(fam, cand_slice, ..., tree_, croot, croot, croot)` still passes the SAME croot as active/deleted/lost roots; Found/Deleted roots (`:200-201`/`:242-243`) stay empty on the whole-disk flow | none |
| M-105 | **LIVE** | No DRWService install lifecycle: `rg -i "CreateService\|SERVICE_AUTO_START\|DRWService"` over `src/`,`installer/`,`.github/` → only comments (`cr_bitlocker.cpp:22,368,445`) and the RDA driver path (`Install-ReleaseDriver.ps1:113` comment); `cr_transport.cpp` is still an OpenSCManager status query only | none |
| M-106 | **LIVE** (STALE-CITE lines: :3846→4405, :3936→4496 — file grew ~560 lines from later carve landings) | `src/core/carve/cr_analysers.cpp:4403-4404` stand-in comment verbatim ("The chunk-chain size walk (FUN_180044e30) is the recorded follow-up; the DWG-pattern bounded extent stands in") above `validate_one` (:4405); `:4493-4494` same for `validate_3ds` (:4496) | none |
| M-107 | **LIVE** | All stand-in sites present with "stands in"/"recorded follow-up" comments: `validate_x3f` :3639, `validate_crw` :3664, `validate_wpg` :3752, `validate_wpd` :3769, `validate_psp` :3791, `validate_sit` :3808, `validate_ape` :3884, `validate_eps` :4257, `validate_ai` :4275, `validate_dxf` :4296, `validate_dwg` :3194, `validate_swf` :4469; spot-verified psp/sit bodies still `r.end = std::min(hit + u64(64 << 20), probe_limit(...))`. NOTE: N-093 (`03ad5523`) landed og caps as ROW caps in the registry — the size-walk defect itself is untouched | none |
| M-109 | **LIVE** | `include/cr/preview/cr_thumbnail.h:88` `u32 max_read = 32u << 20;` (cite was :82 — shifted); `tests/unit/test_thumbnail.cpp:186` still pins `EXPECT_EQ(32u << 20, c.max_read)` with the refuted "M-080: the og GDI+ path has no input cap" comment; `src/core/preview/cr_thumbnail.cpp:1227` byte-oversize leg → `:1256` named `BudgetExceeded` refusal (the 32–200 MB no-embedded-thumb band still refused); no PARITY-DELTAS row for the 32 MiB bound (`rg "32 MiB"` in `docs/PARITY-DELTAS.md` → 0 hits). M-077's landing (`04707ba1`) is the PREVIEW fetch caps — a different surface; the thumbnail anchor is unchanged | none |
| M-111 | **LIVE** | Only `FindFirstFile*` uses remain the `.crsf` session listings (`src/ui/cr_ui.cpp:397`, `src/app/rrecov.cpp:976`); `rg -i "CWindowsFS\|ExplorerPartition"` over `src/`+`include/` → 0 hits; no live-mounted-FS browsing mode; no PARITY-DELTAS row | none |

## MINOR (24)

| ID | Verdict | Evidence at HEAD | Fix commit |
|---|---|---|---|
| N-090 | **LIVE** | `src/ui/cr_ui.cpp:388ff` `list_crsf_files` — pattern `*.crsf` + long-name tail check only, **no `auto_` filter** (`rg "auto_"` in the listing path → only the auto-save NAME generator at :2032/:2054); `include/cr/cr_ui.h` `cr_ui_session_row` still `label[32]`, `save_unix` u64 seconds, no device-type field | none |
| N-096 | **LIVE** | `include/cr/win/cr_rda.h:50-52` defines `kIoctlGetProcName/kIoctlGetFileObjName/kIoctlGetDiskContextMode`; **zero uses outside cr_rda.h** (client never issues them); `src/win/cr_transport.cpp` open path unchanged | none |
| N-107 | **N/A-DRIVER-REPO** | All 14 stale "15/18 commands" prose sites live in the rda-driver repo (README/architecture/ground-truth-contracts/requirements) — not present in this workspace | none possible here |
| N-112 | **LIVE** | No LZH: `rg -i "lzh\|lha\|lharc\|-lh"` over `src/core/carve/` (incl. `cr_registry.cpp`, `cr_rawsigns*`) → 0 hits; no row/validator/probe | none |
| N-113 | **LIVE** | No text-school probes: `rg -i "BEGIN:VCARD\|<gpx\|libopenshot\|BiffProject\|diagram"` over `src/core/carve/` → 0 hits | none |
| N-115 | **LIVE** | `src/core/carve/cr_analysers.cpp:847-854` — the DOCX row still validates only the FIRST ZIP entry name == `[Content_Types].xml`; no XLSX/PPTX rels split, no ODF/EPUB/iWork/HWP/CDR inner tags, no metadata extraction (`:1076` is just a note string) | none |
| N-121 | **LIVE** | `rg -i "MountGpt\|UnmountGpt\|CUpdateGptSystemBootPartition\|find_esp\|esp_path"` over `src/`+`include/` → 0 hits; CLI still requires host-supplied `--boot-fix-bcd file` (`src/app/rrecov.cpp:23` usage, `:2530` override note) | none |
| N-122 | **LIVE** | `include/cr/nvramfix/cr_nvramfix.h:18` "no create/delete (the finding's stage-b scope)"; `tests/unit/test_nvramfix.cpp:417` `NvramFix.MissingBootVariableStopsTheWalk` still pins the walk-stop; D-90 (`docs/PARITY-DELTAS.md:2977`) still registers only boot.ini + chkdsk dispositions — the create/delete exclusion has no register row (only `modules/boot-repair.md:19`, which cites the finding itself) | none |
| N-125 | **LIVE** | `rg -i "disconnect\|reattach\|reconnect"` over `src/`+`include/` → 0 hits; device loss still surfaces as the generic read-error path; no retry/reattach model, no registration | none |
| N-126 | **LIVE** | `rg -i "unsaved"` over `src/`+`include/`+`tests/` → 0 hits; no category equivalent in `cr_query` | none |
| N-127 | **N/A-DRIVER-REPO** | Doc-vs-code truthfulness defects are in the rda-driver repo's docs/code — not present here | none possible here |
| N-128 | **LIVE** | `docs/CAPABILITY-MATRIX.md` "Prebuilt-artifact CI" row still cites `.github/workflows/{driver-install,core-install,staged-scenarios,release}.yml; .github/actions/install-driver/action.yml` — actual tree has only `checks.yml, installer.yml, release.yml, test-benchmark.yml` and **no `.github/actions/`** (verified by listing); verdict column still "**DONE** (v1 matrix: … ext4/HFS+/Btrfs/ZFS/ReFS staging is recorded follow-up)" — the unowned follow-up still buried in DONE | none |
| N-129 | **LIVE** (file unchanged since filing) | All six defects at the SAME lines in `docs/PARITY-OG-CORE-INVENTORY.md`: `:30` "EUOfficeTools.exe" (case mismatch), `:53` "(44 DLLs)", `:73` "SURFACE-INVENTORY.md:84" cite, `:99` self-nested "og-codebase/ (dir)" row, `:253` "all **25** .mo runtime modules", `:267` "CORE_RL_* (19)" | none |
| N-130 | **LIVE** | `rg "VideoFileRepairer" docs/PARITY-OG-CORE-INVENTORY.md` → 0 hits; the VideoViewer row (`:107`) still enumerates MediaGraph/ffmpeg without the exe; no disposition row anywhere on the board (N-138's refutation addendum names it, but the inventory row was never added) | none |
| N-131 | **LIVE** | `src/core/session/cr_records.cpp:177` `put32(out, parent_ids[i])` — structural tree-index parent still the ONLY persisted parent datum; `rg "parent_anchor\|parent_reference\|parent_frn"` over `src/`+`include/` → 0 hits | none |
| N-132 | **PARTIALLY-FIXED** | TxtViewFiles member LANDED: `aac75bba` (M-108) added `src/core/preview/cr_preview_text_exts.cpp` + `cr_preview_text_exts_data.cpp` (byte-exact whitelist) and the API-DELTA row 44 was re-truthed in the same commit ("TxtViewFiles landed at the M-108 fix … byte-exact"). STILL LIVE: `SumaryFileType` and `PriorityDisplayType` remain absent (`rg "SumaryFileType\|PriorityDisplayType\|priority_display\|display_order\|usefullostfiles"` over `src/`+`include/` → 0 hits) while API-DELTA row 45 still claims "landed (query/filefilter data layers)" — the false-landed claim persists for 2 of the 3 surfaces | `aac75bba` (partial) |
| N-133 | **LIVE** | `src/core/ext4/cr_ext4_engine.cpp:200-201` still decodes `s_volume_name` into `Volume::volume_name` (`include/cr/ext4/cr_ext4_engine.h:79`) with ZERO consumers (the only two hits are decode + declaration); NTFS `$Volume` appears only in the system-file blocklist (`cr_mftwalk.cpp:218/:247`) — never read for the label; FAT label dentry skipped as "not a result" (`cr_fat_quick.cpp:170`); `fill_volume_label` in `src/app/rrecov.cpp:~323-338` (cite :331 shifted) still carries the refuted premise verbatim ("the og has no image-scan naming semantics"). `53eb9fff` added behavioral tests + resume label adoption only — no engine-side label decode | none (M-079 record's premise addendum landed with the N-133 filing itself) |
| N-134 | **LIVE** | The trail registers the finding demands are still empty: `rg` over `diff-to-fix/SURFACE-INVENTORY.md` → 0 mentions of the six; `docs/PARITY-DELTAS.md` → 0; I-027 carries no telemetry-binary rows. (Nuance: `docs/PARITY-OG-CORE-INVENTORY.md` does carry rows for all six — `:78` EuDownloader, `:92` legacy.dll, `:119` Boot.dll, `:120` BootDriver.dll, `:124` CloneModule.dll, `:155` peformat.dll — but those predate the filing (`b202f9ed`, 10-28 10:14 < filing 17:41) and the finding's fix plan (SURFACE-INVENTORY/I-010/I-019/I-027/PARITY-DELTAS registration) never landed) | none |
| N-135 | **PARTIALLY-FIXED** | REPAIRED: item 1 — `docs/REFERENCE-API-GAP.md:376` R-28 row re-truthed to match :222 (`5fecdae1`); item 3/4 — the phantom-SHA class swept in the ledger (M-090 row now `9daa787` with the `180053de0` mis-paste annotated repaired, `FIXED-LEDGER.md:181`; `5fecdae1` + `2cf3f4d0` 19 landing-cite repairs + `f166440b` session-11 truth pass; the M-110 row's mis-cite repaired by `c7fa527c`); item 5 — missing rows M-080/M-082/M-085/S-043 added (`:174/:180/:332/:337`) and the four re-created `findings/FIXED/` files gone. STILL LIVE: item 2 — `:379` R-31 still "PARTIAL … display priority missing" vs `:225` "PRESENT" (self-contradiction); residual phantom cites outside the ledger — `diff-to-fix/SUMMARY.md:385` (`dd0ffd5`), `:416` (`95122bd`) still cite unresolvable SHAs (`git cat-file -t` fails for all five named tokens at HEAD); item 7 — `src/core/preview/cr_thumbnail.cpp:11-12` file-top matrix still lacks LZW; plus the addendum's own open set (54 ancient rows' backfill; `:222` R-28 now stale in the other direction — "no cr_ui thumbnail export yet / ABI open" is false since the thumbnail ABI landed `740d9b56`) | `5fecdae1`, `2cf3f4d0`, `f166440b`, `c7fa527c` (partial) |
| N-137 | **LIVE** (STALE-CITE lines: cr_ui.h :1272-1274→~1276-1278, :1242-1245→1249-1256; cr_ui.cpp :3677-3678→3694-3695) | `include/cr/cr_ui.h` thumbnail block still states "0x50030004 ERR_TRUNCATED never returned for thumbnails …" while `src/ui/cr_ui.cpp:3694-3695` still maps `case preview::ThumbStatus::DecodeFailed: return ERR_TRUNCATED;`; the "validated … allow_embedded 0/1" claim (`cr_ui.h:1250`) vs the actual `caps->allow_embedded != 0` mapping (`cr_ui.cpp:3730-3731`) and the engine's boolean use (`cr_thumbnail.cpp:1229`) — both contradictions intact | none |
| N-138 | **PARTIALLY-FIXED** | The OQ record now carries the **REFUTATION ADDENDUM** (`diff-to-fix/findings/FIXED/OQ-S10-2-videoviewer-sfc-unknown-payload.md:74+`, added by `d50a54dc` — the same commit that filed N-138) explicitly superseding the false resolution, and N-134's own S10-F addendum carries the corrected disposition. STILL LIVE residue: the record's top line (`:9`) still asserts "Status: CLOSED — RESOLVED … dead payload" — the refuted verdict remains the record's standing status | `d50a54dc` (addendum; status line unrepaired) |
| N-139 | **LIVE** (STALE-CITE line: :919→924) | `src/core/carve/cr_carve.cpp` nothrow-return sites (`:439/:547/:650/:831`) + the throwing window buffer `std::vector<u8> win(...)` at `:924` and the tail-scan buffer — still no try/catch in `carve_slice` or upstream (`rg "catch\|bad_alloc"` in `src/app/rrecov.cpp` + `cr_scan.cpp` → 0); no OOM diagnostic (cr_carve.cpp includes no logging header) | none |
| N-140 | **LIVE** | `include/cr/carve/cr_carve.h` `CarveStats` still 13 scalar counters — no per-format/per-extension map; `rg "per_format\|per_extension\|format_counts"` over the stats/report paths → 0 hits; the orchestrator still copies scalars; no API tallies carve_format over results | none |
| N-141 | **LIVE** | `src/core/refs/cr_refs_engine.cpp:1102` emission gate still checks only name-emptiness + record shape (`name16.empty() || ev.val_len < 0x28`); no zero-size∧no-dot condition anywhere in the emit path — the og's junk-suppression gate remains unported | none |

## INFORMATIONAL (7)

| ID | Verdict | Evidence at HEAD | Fix commit |
|---|---|---|---|
| I-026 | **REGISTER** (no core defect; deliberate posture intact) | `docs/DRIVER-RELEASE.md` §7 still documents SignMode=Off / unsigned-by-design / OSSign prepared-but-disabled (`:52-65`); the posture is unchanged and truthfully recorded — nothing to fix by design | n/a |
| I-027 | **REGISTER** (informational) | Register of og data-asset ground truths (quota contract, raw-results shield, telemetry caps, DiskHealth autostart, filter_generalraw) — "no defect in core" by the finding's own status; state unchanged | n/a |
| I-028 | **REGISTER** (informational) | Thumbnail superset register — deliberate improvements + unverified residuals, status "deliberate differences, recorded"; engine features stand at HEAD | n/a |
| I-029 | **LIVE** (doc-truth defect) | `docs/DRIVER-CONTRACT.md:131` §4.4 header still asserts "the EUDCPEPM contract — 19 recognized codes … preserved byte-for-byte" with the DISCOVERY `0x800C2AC0` and RRWR trio rows presented as reference wire — no grounding caveat/annotation was added for the 4 core-driver-wire codes (`rg -i "core-driver wire\|grounding\|I-029"` in the doc → 0 hits) | none |
| I-030 | **PARTIALLY-FIXED** | Item 1 REPAIRED: `diff-to-fix/modules/ldm.md:1` now reads "MBR/GPT/EBR covered in this file's own table rows; the former `table.md` reference was dangling — N-104" (repair landed in `32ef33b5`, docs-truth batch C). Items 2-3 STILL LIVE: no `reghive.md` and no `apply.md` in `diff-to-fix/modules/` (28 module files, none for those surfaces) | `32ef33b5` (partial) |
| I-031 | **REGISTER** (informational) | RAWSearchDLL infra register — deliberate-divergence set (D-101/N-017/D-78/I-009) + decoded boundary set; no defect sought; register stands | n/a |
| I-032 | **REGISTER** (informational) | ZFS/ReFS function-walk register — OQ-T6 resolution, grading byte, volume-label closure; "informational; the two defect-class outcomes are filed separately (S-053, N-141)" — both of which are LIVE per this sweep | n/a |

---

## Cross-cutting observations

1. **The prompt's "recently landed fixes" did not touch any of the 55 active defects' code paths in a
   resolving way**, with three narrow exceptions found by this sweep:
   - `aac75bba` (M-108) landed the TxtViewFiles surface that N-132's row 44 falsely claimed as landed —
     turning that third of N-132's false-landed claim true (N-132 → PARTIALLY-FIXED).
   - The phantom-SHA/ledger truth repairs (`5fecdae1`, `2cf3f4d0`, `f166440b`, `c7fa527c`) resolved several
     of N-135's items (N-135 → PARTIALLY-FIXED).
   - `d50a54dc` appended the refutation addendum to the OQ-S10-2 record N-138 refutes (N-138 →
     PARTIALLY-FIXED).
2. **The session-11D fix lane (`bcf31316`) opened with the SEVERE tier claimed but produced no landings** —
   all seven SEVERE findings are verifiably LIVE at HEAD (each checked in code above).
3. **Line-number drift is the only "stale" element in most findings** (the carve file grew ~560 lines from
   the M-090/M-084/M-091/M-096/N-111/J P2 landings; cr_ui.cpp grew ~105 lines from the ABI landings) —
   every defect was re-located by symbol and remains live: S-052, M-104, M-106, M-107, M-109, N-133, N-137,
   N-139 all carry shifted cites.
4. **No cited core file has disappeared**; the only wholly-missing cited files belong to the five
   driver-repo-resident findings (M-086/M-087/M-089/N-107/N-127), whose target repo (heicfast/rda-driver)
   is not checked out in this workspace — those cannot be adjudicated here and remain PUSHED-FORWARD per the
   board (tracked-open OQ-S10-3).
5. **Board-count note:** HEAD is `36c1d3e5`, one worklog-only commit above the `c7fa527c` named in the task;
   the code tree is identical, so all verdicts apply to both.
