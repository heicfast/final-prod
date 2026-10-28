> **S42-C pin-time + renumber note (added at the third reconciliation):** this
> sheet records the tree state as verified at the lane's pin **`869da779`** (core) /
> **`be0cfe9`** (corpus). THREE hot windows landed while this lane worked; the
> sheet's internal draft IDs are superseded as follows: this lane's "M-118"/"M-121"
> (the Stbio large-IDAT finding) → final **M-122**; "I-035"/"I-037" (the diskhealth
> register) → final **I-038**; "I-036"/"I-038" (the corpus-worklog register) → final
> **I-039**; the red-main draft "N-154" → folded as the N-157 corroboration addendum.
> The remote lanes' same-numbered findings (the S12 MountPoints2 M-118, the DBX
> chain's M-121, the decode register I-037, the S42 lane's config-layer I-036) are
> different subjects. The sheet's liveness verdicts for findings the windows have
> since FIXED (S-048, M-117/M-121-dbx, N-152, N-125, N-126, N-135, N-148,
> S-054-the-N-013-regression, N-149, M-076, the INFO wave, N-115's N-158 tail
> aside) are pin-time-historical (true at the pin, superseded at the reconciled
> tree). The landing-parity VERIFIED-PARITY verdicts are unaffected. This lane is
> the S42-C lane (rebranded twice: the remote's S42 and S42-B lanes pushed first).

# W2 — Landing-Parity Verification (23 landings, commit window c7fa527c..869da779)

**Sub-agent:** S42/W2 (read-only verification). **Date:** 2026-10-30 session.
**Core repo:** /home/z/my-project/core_recovery — code verified at HEAD `d10b893a` (= 869da779 + one worklog-only commit `d10b893a` that touches no product file; all code cites below are identical at 869da779).
**Corpus:** /home/z/my-project/easeus-drw-research @ be0cfe9 (read-only ground truth).
**Method per landing:** (a) read the Resolution record (findings/FIXED file, or the fix-commit message where the file was retired to history — the ledger convention), (b) read the CURRENT code at HEAD, (c) verify the ONE most load-bearing corpus claim by direct grep/read of the decompiled tree, (d) run the landing's named tests from the existing build (`build/linux-release`, no rebuild — binaries are the parent lane's fresh build of HEAD).

**Executed suite evidence at HEAD:** full `cr_tests` run: **1 FAILED TEST — `Carve.OoxmlRelsPathSplit`** (the known parent-lane capture; NOT re-investigated here), all other tests green. `cr_ui_tests` 61/61 green. Every landing-specific test named below was executed green individually.

Legend: VERIFIED-PARITY = code at HEAD matches the Resolution claim + corpus cite holds + tests green. OVERSTATE = a Resolution claim that does not hold (noted precisely). Bookkeeping observations (ledger/SHA/comment drift) are recorded but are not landing-parity defects.

---

## 1. A-012 — carve registry FMT_SVG row restored (record commit eba47f11; product fix e4273f2b per the Resolution)

**Claim (Resolution, findings/FIXED/A-012-...md:58-63):** the 81ee9775 rebase sweep dropped M-093's FMT_SVG row (kTable 119 vs FMT_COUNT 120); the row restored verbatim in id order (…FMT_XML, FMT_XML_BOM, FMT_SVG_CD01, FMT_SVG, FMT_MOBI); the F8 gate (validate_registry) and the `CarveEngine.RegistryF8Gate` pin are the regression proof. Note: the Resolution says the CONVERGENT remote repair `e4273f2b` (one-line row restore, verified in git) stands; `eba47f11` is this lane's record commit (diff touched only diff-to-fix/ + worklog — the fix itself was already on the remote).

**Code at HEAD:**
- FMT_SVG row present: src/core/carve/cr_registry.cpp:1140-1150 — `{FMT_SVG, "svg", {'<','s','v','g'}, 4, 0, 0, 0x100000ull, 620, validate_xmlsvg, "SVG image", true}` — in exact id order between FMT_SVG_CD01 (:1129) and FMT_MOBI (:1155), matching the enum order FMT_XML(220) FMT_XML_BOM(221) FMT_SVG_CD01(222) FMT_SVG(223) FMT_MOBI(229) in include/cr/carve/cr_signatures.h.
- Row/enum census (first-hand, scripted): kTable has **142 rows**; the FormatId enum has **142 format values**; FMT_COUNT == 142; every enum id appears exactly once in kTable, in order. (The "120 rows" in the claim was the count at fix time; later landings N-111 JP2 family, N-112 LZH×4, N-113 text school×6, N-115 OOXML/ODF×9 grew both sides identically. The load-bearing claim is the INVARIANT `registry_size() == FMT_COUNT`, which holds.)
- F8 gate: validate_registry size check cr_registry.cpp:1371-1373, id-vs-index check :1393-1394; carve_slice's early refusal cr_carve.cpp:1099 (`return ERR_BAD_PARAMETER; /* F8 gate */`).
- Test pin: tests/unit/test_carve.cpp:518-522 `TEST(CarveEngine, RegistryF8Gate)` — **executed: PASSED**.

**Corpus spot-check:** the finding itself declares corpus references "none needed" (core-internal regression). Provenance anchor (optional, done anyway): the "<svg" 4-byte magic appears in the og's RAWSearchDLL.dll strings dump (re-workspace/reversed/RAWSearchDLL.dll/strings_ascii.txt:26, :656-657), consistent with M-093's decoded probe the restored row carries.

**Verdict: VERIFIED-PARITY** (with the SHA nuance recorded above: the task lists eba47f11; the Resolution and the product code point to e4273f2b as the standing fix; both exist and are consistent).

---

## 2. S-049 — universal carved-file naming contract (d4b487c8)

**Claim (Resolution, findings/FIXED/S-049-...md:74-79):** `<PREFIX>_<%06d>.<UPPEREXT>` contract; per-format counters; uppercase-ext default prefix; content-derived prefixes — JPEG EXIF Model/Software over SOF dims, MP4/MOV mvhd duration, MP3 ID3 TIT2, PDF Title (25-char cap) else Creator/Producer + plain D: date, DOCX docProps created, ZIP first CD entry, WAV LIST/INFO INAM; %d fallback past 99999; `File%06u` retired; N-063's MP4/MOV template superseded on the carve-tree application.

**Code at HEAD:**
- `og_carve_name` cr_carve.cpp:273-324: per-format counter map `ctx.name_counters` keyed by uppercase ext, mutex-guarded (:281-282); `%06u` until 99999 then `%u` (:285); uppercase-ext default prefix (:288-305); name composed `<prefix>_<counter>.<UPPEREXT>` with the 254-char prefix cap (:306-323).
- `default_namer` cr_carve.cpp:1051-1090 — the same contract through the public namer hook (the `index` parameter is the caller's per-format counter; comment block cites FUN_140c44770 :1878330-81).
- Emit site cr_carve.cpp:552-565: `props->name = ctx.namer ? ctx.namer(...) : og_carve_name(ctx, eff.ext, cr.name_prefix)` — the supersession of the N-063 embedded_name full-name chain is documented in the comment (:555-563).
- Content prefixes in src/core/carve/cr_analysers.cpp: `jpeg_name_prefix` :545 (SOF dims + EXIF APP1→TIFF IFD0 walk, Model tag 0x0110 / Software 0x0131; wired at :671), `mp4_duration_prefix` :1036 (moov/mvhd walk, v0 timescale@+20/duration@+24, v1 @+28/@+32; wired for MP4 :1135, MOV :1144/:1186), `id3_title_prefix` :2443 (TIT2 frame at :2461; wired :2548), `pdf_name_prefix` :261 (kStrCap **25** at :263; /Title,/Creator,/Producer,/CreationDate keys; plain `D:` + 14-digit date → `YYYY-MM-DD HH-MM-SS` at :323-345; Title-first then Creator-then-Producer + date :391-407; wired :415), `docx_created_prefix` :1262 (wired :1694), `zip_first_entry_prefix` :1243/:1250 (wired :1562/:1694/:1765/:1776), `wav_inam_prefix` :2559 (wired :2627).
- Tests executed green: `CarveEngine.OgNamingUniversalExtCounter`, `OgNamingJpgSofDimensions`, `OgNamingJpgExifModelAndSoftware`, `OgNamingCounterPast99999Unpadded`, `OgNamingMp3Id3Title`, `OgNamingPdfCreatorAndDatePrefixes`, `OgNamingDocxCreatedTimestamp`, `OgNamingWavInamTitle`, `OgNamingZipFirstCentralDirectoryEntry`, `Carve.Mp4NamedByDurationPrefix`, `Carve.MovNamedByDurationPrefix`, `Carve.MovV1MvhdDurationRead`, `Orchestrator.CarveSingleNamingFamily`, `CarveEngine.FileNnnnFallbackRemoved` (the retired `File%06u` pinned absent) — 14 tests, all PASSED.

**Corpus spot-check (the load-bearing claim):** DRWUI_decompiled.c `FUN_140c44770` at ghidra-drwui (read :1878348-1881): exactly the claimed shape —
```
pwVar1 = L"%06d"; if (99999 < param_3) { pwVar1 = L"%d"; } snprintf(local_228,0xff,pwVar1,param_3);
...
pwVar1 = L"%s_%06d"; if (99999 < param_3) { pwVar1 = L"%s_%d"; } snprintf(local_228,0xff,pwVar1,param_2);
```
with the 0xff buffer — the `%s_%06d`/`%06d`/`>99999` contract and the 0xff snprintf all byte-exact as the Resolution cites (:1878330-81 range).

**Verdict: VERIFIED-PARITY.**

---

## 3. S-051 — whole-disk scan targets (ad0535d6 local landing; convergent remote 6ecd6bb4; DU merge 3f7c190e)

**Claim (Resolution, findings/FIXED/S-051-...md:63-87):** `run_engine_pass`'s per-volume target block — when the slice basis is whole-disk with an unforced family hint, the partition table is parsed and every used, non-container, non-LDM partition becomes a per-volume target (PartitionSlice at the entry's start LBA), probed per-target, skipped honestly when Unknown (counted), dispatched under its own root, rebased F-1; no double scan (discovery candidates at scanned LBAs dropped; skipped ones stay discoverable).

**Code at HEAD:** src/core/orchestrator/cr_scan.cpp:1662-1753:
- gate `if (!host_fs_hint_)` (:1662; host_fs_hint_ set at :1045 from `cfg_.fs_hint != FsFamily::Unknown` — the DU-merged S40 gate),
- `slice->absolute_base() == 0 && table::parse_disk_table(slice, t) == OK && t.layout != None` (:1666-1667),
- target filter `p.used && p.sector_count != 0 && fs != Extended && fs != Protective && fs != Ldm` (:1669-1672),
- `PartitionSlice::create` per target (:1680), per-target `probe_family` (:1688), skip-honesty `++stats.table_volumes_skipped` with Info log (:1690-1698),
- per-volume root + dispatch `run_engine_dispatch(fam, tgt.get(), deep_mode, true, …)` (:1706-1727), F-1 rebase (:1731),
- stats `table_volumes` / `table_volumes_scanned` / `table_volumes_skipped`; scanned-LBA recording `table_target_lbas_.push_back(p.start_lba)` (:1744) and the discovery reconciliation (:1889-1894).

**Tests executed green (12):** `Orchestrator.WholeDiskScansTableVolumes`, `WholeDiskGptTwin`, `WholeDiskMixedFamilies`, `WholeDiskFsMarkerNamesForeignVolume`, `WholeDiskDamagedVolumeSkippedHonestly`, `WholeDiskSuperfloppyUnchanged`, `WholeDiskNoDoubleScan`, `WholeDiskVolumeRootsCarryDisplayNames`, `WholeDiskKnownVsLostDistinguishable`, `WholeDiskVolumeRootNamesRoundTripSession`, `FatEngine.ReadVolumeLabelDecodesBootSector`, `FatEngine.ReadVolumeLabelRejectsLabelLessAndNonFat`.

**Corpus spot-check:** architecture/04-recovery-workflow.md:43 (`A partition/disk | PartitionStartWnd | CDoScanPartition → CGenericRecoveryController`) and :67-68 ("reads the partition list, inserts lost partitions as additional scan targets … 'Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d'"); decompile confirmed first-hand: ghidra-drwui/DRWUI_decompiled.c ~:904076 (`FUN_140162e00("Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d\r\n", …)`) and :904198 (`for (; lVar10 != lVar14; lVar10 = lVar10 + 0xa8)` — the 0xa8-stride partition-info loop, same insertion trace inside).

**Verdict: VERIFIED-PARITY.**

---

## 4. M-104 — per-volume display-name roots, 4 tiers (same landing family)

**Claim (Resolution, findings/FIXED/M-104-...md:61-76):** tier 1 the on-disk volume label (FAT families via fat::read_volume_label), tier 2 the GPT entry name, tier 3 the FS-marker `"NTFS @ <LBA>"`, tier 4 the anonymous form last-resort-only (reserved for genuinely-lost discovery candidates); one tree per volume with deleted-as-flag; session round-trip.

**Code at HEAD:** `volume_target_name` cr_scan.cpp:1353-1411: FAT label first (:1355-1358 via `fat::read_volume_label`), GPT entry name for `p.from_gpt` (:1364-1370), FS-marker `"%s @ %llu"` (:1371-1410), no anonymous fallback (unknown-family table targets get no root — skipped honestly per S-051). The anonymous `"Candidate @"` construction now lives ONLY in the discovery-candidate path: cr_scan.cpp:1933-1937 (`std::u16string role = u"Candidate @"; … snprintf(buf, …, " %llu", c.start_lba)`). One-tree-per-volume: the volume root is passed as root_active+root_deleted+root_lost at :1726-1727.

**Tests:** the M-104 set above (VolumeRootsCarryDisplayNames / KnownVsLostDistinguishable / VolumeRootNamesRoundTripSession / WholeDiskFsMarkerNamesForeignVolume / WholeDiskNoDoubleScan) — executed green.

**Corpus spot-check:** same as S-051 (the insertion trace's `%s` target names at DRWUI :904076/:904198; the real G-session display-name roots are the finding's executed evidence, `evidence/executions/s10-wholedisk-scan-targets.md`).

**Verdict: VERIFIED-PARITY.**

---

## 5. S-052 — LDM span recovery basis (f4e500de; pre-rebase ea9c8ed5 per conform commit c5545f00)

**Claim (Resolution rides the fix-commit message; ledger row FIXED-LEDGER.md:438):** per-root basis ids (`set_root_basis`, persisted as node-flags bit10 in the tree record), the scan controller keeps the virtual volume devices past run() (heap registry), the SafeWriter resolves per-item read sources through the borrowed `LdmBasisTable` (basis 0/unresolvable = absolute source, honest per-extent failures), R7.2/F12 overlap refusal maps layout-relative extents to PHYSICAL member spans, session persists `REC_LDM_LAYOUTS` (type 10) + root stamps; GAP-S6-15 closed; corpus ground truth = LdmManager.mo volume-storage family (cr_ldm.h:17-24).

**Code at HEAD (all five links verified):**
- heap registry: include/cr/orchestrator/cr_scan.h:729-730 `std::vector<std::unique_ptr<table::LdmVolumeDevice>> ldm_devices_` (+ accessor :642-643); construction cr_scan.cpp:1605-1608 (`std::make_unique<table::LdmVolumeDevice>(lay, resolver)` … `ldm_devices_.push_back(std::move(vol))`) — no stack device remains.
- basis ids: cr_scan.cpp:1607 `tree_.set_root_basis(vroot, u32(ldm_devices_.size() + 1))`; `ResultTree::set_root_basis` src/core/tree/cr_node.cpp:117-121; `FileNode::basis()` documented include/cr/tree/cr_node.h:306-310.
- SafeWriter: include/cr/recovery/cr_safewriter.h:150 `class LdmBasisTable` (+ :186 `set_basis_table`); routing src/core/recovery/cr_safewriter.cpp:176-186 (basis≠0 → `bases_->at(basis)`; unresolvable → absolute source with honest per-extent failure semantics); overlap mapping :210-216 via `bases_->spans(basis, e.offset, e.length)` (`LdmBasisTable::spans` :36-56 → `dev->physical_spans`).
- session persistence: `REC_LDM_LAYOUTS = 10` include/cr/session/cr_crsf.h:83; serialize/deserialize wire include/cr/session/cr_records.h:65-80 (`serialize_ldm_layouts`/`deserialize_ldm_layouts`); bit10 stamp src/core/session/cr_records.cpp:180 (`nflags |= 1024`), read-back :292 (`if (nflags & 1024)`) and :507 (`const u32 root_basis = (nflags & 1024) ? c.g32() : 0u;`).
- GAP-S6-15: docs/CORE-GAP-LIST.md:139 — `**CLOSED** (S-052)` with the full closure text.

**Tests executed green (8):** `LdmRecovery.SpanRecovery*`, `StripeRecoveryRoundTrips`, `Raid5RecoveryWithMissingMember`, `OverlapRefusalMapsToPhysicalSpans`, `SimpleVolumeRecoveryRoundTrip`, `PreviewMatchesRecovery`, `SessionRoundTripsLayoutsAndRootBasis`, `Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips`.

**Corpus spot-check:** ghidra-mo/LdmManager_decompiled.c — 91,475 lines (wc verified); the volume-storage class family present: `class_CMomObject<class_Raid5VolumeStorage>` :38927/:38958, `class_StripeVolumeStorage` :39054/:39074, `class_SpanVolumeStorage` :39112/:39132. cr_ldm.h:17-24 carries the same ground-truth note.

**Verdict: VERIFIED-PARITY.**

---

## 6. S-053 + N-141 — ReFS name-destroyed rows emit + zero-size-no-dot filter (e10ec381; files retired to history, Resolution rides the commit)

**Claim (fix-commit message = Resolution; ledger rows :433-434):** the `name16.empty()` half of the emission gate removed (the og's name builder no-ops on keySize<=4, ReFSSearchDLL :8674-75; its emit path has no name gate, :8478-81); the `val_len < 0x28` minimum-record half stays; stored name stays EMPTY; CLI display-layer placeholder only; session round-trip pinned. N-141: the og's zero-size∧no-dot noise filter (FUN_18000ba90 :8317-37; FileValues +0x30 size field; compound semantics — size==0 with a '.' survives, nonzero survives regardless); counted in `stats.zero_size_nodot_filtered`.

**Code at HEAD:** src/core/refs/cr_refs_engine.cpp:1102-1137 — the gate is now `if (ev.val_len < 0x28) return;` (:1114) with the S-053 comment block (:1102-1113); the N-141 filter `if (gi.size == 0 && name16.find(u'.') == std::u16string::npos) { ++stats.zero_size_nodot_filtered; return; }` (:1134-1137); stats field include/cr/refs/cr_refs_engine.h (`u64 zero_size_nodot_filtered = 0;` with the N-141 comment). CLI placeholder: src/app/rrecov.cpp `unnamed_display_name` + `print_tree`'s `shown` substitution (display layer only).

**Tests executed green (8):** `Refs.UnnamedDataOnlyRowEmitted`, `UnnamedRowGradeIsContentBased`, `UnnamedOddLengthKeyEmittedNoCrash`, `ZeroSizeNoDotRowSuppressed`, `ZeroSizeWithExtensionEmitted`, `NonZeroNoDotEmitted`, `UnnamedZeroSizeRowSuppressed`, `UnnamedNameSessionRoundTrip`.

**Corpus spot-check:** ghidra-searchdll/ReFSSearchDLL_decompiled.c:8317-8337 — `if (local_e8 < 1)` (the size==0 class) then the `memchr(puVar18, 0x2e, local_96)` dot-scan with the `LAB_18000bbfb` else branch — the compound filter exactly as claimed; :8674-8675 — the name builder's `iVar4 = (int)((…+0x20) - lVar2) - 4U >> 1); if (0 < iVar4)` gate (the keySize≤4 no-op); :8478-8481 — `FUN_18000e020` then `FUN_18000d970` emit call with no name gate.

**Verdict: VERIFIED-PARITY.**

---

## 7. S-054 + M-116→M-117 — TS/M2TS 200 MB cap + suppress_overlap pre-validator covered check (d7ba41f6; renumber trail 0f5d57fb)

**Claim (fix-commit message = Resolution; ledger rows :435-436):** both FMT_TS/FMT_M2TS `max_size` go from 2000000000ull to 200000000ull (the og's 200 MB controller global; the x10 provenance refuted — 2e9/0x77359400 appears nowhere); the discovered quadratic mid-stream re-walk fixed (the covered checks move BEFORE the validator call); renumbered M-116 → M-117 per the ID-collision rule.

**Code at HEAD:** src/core/carve/cr_registry.cpp:1057-1067 (FMT_TS `max_size 200000000ull`, comment "the og's TS walk clamps to the 200 MB controller global") and :1068-1078 (FMT_M2TS same). The covered-check-before-validator at src/core/carve/cr_carve.cpp:453-476 — `emitted_at.count(cand_abs)` break and the `sig.suppress_overlap` overlap_covered scan both precede the `sig.validator(...)` call (:479+), with the renumber note in the comment (:453-454 "M-117 … renumbered from the draft M-116 at the push per the ID-collision rule").

**Tests executed green:** `Carve.TsM2TsCapIsTheOg200MB`, `Carve.TsStreamClampedAt200MB` (12.3 s — the once->600 s fixture now completes).

**Corpus spot-check:** ghidra-searchdll/RAWSearchDLL_decompiled.c:35507-35509 — `uVar7 = …; if (200000000 < uVar7) { uVar7 = 200000000; } *(undefined8 *)(unaff_RBX + 0x10) = uVar7;` and :35513 `*(undefined8 *)(unaff_RBX + 0x10) = 200000000;` (the size-unknown fallback); :9049 — `*(undefined8 *)(puVar3 + 0x70) = 200000000;` (the controller global at +0x70). Re-verified the negative claim myself: `rg "2000000000|0x77359400"` over the file → **0 hits**.

**Verdict: VERIFIED-PARITY.**

---

## 8. M-109 + the StbioSource read-loop finding (76e2d351)

**Claims:**
- M-109 (Resolution in the landing commit, findings/MAJOR/M-109-m080-landing-stopped-at-32mib-…md): ThumbnailCaps::max_read default 32 MiB → 0xC800000 (the og's still-image eligibility gate, DRWUI FUN_1402636e0 :422425-429), re-pinned at all four comment/doc sites; the raised I/O bound cannot OOM the decode path (max_pixels 64 MP bounds decode memory).
- The StbioSource finding (filed **M-112** in the landing commit/finding file; ledger row :180): `read_cb` looped across `read_at`'s 4 MiB clamp so a >4 MiB PNG IDAT decodes; regression proof `Thumbnail.LargeStillImageWithinOgBand`.

**Code at HEAD:**
- include/cr/preview/cr_thumbnail.h:84-86 `u32 max_read = 0xC800000;` with the M-109 doctrine comment (the refuted M-080 no-cap text replaced); the bounding-policy block :73-79 (200 MB, 0xC800000).
- src/core/preview/cr_thumbnail.cpp:1237-1245 — the byte-oversized leg comment (M-080/M-109) + `const bool byte_oversized = pr.size() > caps.max_read;` (:1241).
- include/cr/cr_ui.h:1258 — `max_read; /* 0 = 200 MB / 0xC800000 (the og's still-image eligibility gate, M-109) */`; ABI wiring src/ui/cr_ui.cpp:3897-3898.
- The read loop: src/core/preview/cr_thumbnail.cpp:163-181 — `/* N-112 (discovered during M-109's band test): LOOP the read - … */ u32 got = 0; while (got < want) { const u32 n = s->pr_.read_at(s->pos_ + got, data + got, want - got); if (n == 0) break; s->b_.spend_read(n); got += n; }`.

**Tests executed green:** `Thumbnail.LargeStillImageWithinOgBand`, `Thumbnail.CapsDefaultsPinTheCorpusAnchors`, `Thumbnail.OversizePastOgGateRefused`.

**Corpus spot-check:** ghidra-drwui/DRWUI_decompiled.c ~:422425-429 — `lVar4 = FUN_140089170(*(undefined8 *)(param_1 + 0x48)); if (0xc800000 < lVar4) { FUN_140263c10(local_78); goto LAB_140263bad; }` — the size read + the >200 MB skip, exactly as cited.

**Verdict: VERIFIED-PARITY** (both fixes), with **three bookkeeping observations** (not code defects):
1. FIXED-LEDGER.md:180 (the Stbio finding's row) cites fix commit `5103ecd8` — **a phantom SHA that exists in no ref; the real fix is 76e2d351** (the commit message itself names M-109+M-112). This is the phantom-SHA class the S12 lane flagged and the S42 parent lane absorbed; the row is still wrong at HEAD.
2. The in-code comment cr_thumbnail.cpp:163 says **"N-112"** while the finding file, the commit message, and the ledger row all say **M-112** — comment-truth drift landed with the fix itself (introduced verbatim in 76e2d351's diff, verified via `git show`).
3. The Stbio finding's M-112 ID collides with the ACTIVE S11B finding `M-112-csl-sizefield-0x44c-branch-unported-unowned.md` (earlier-push-wins kept CSL as M-112; the Stbio row at :180 still claims M-112 too) — the S12 lane's recorded ID-collision, unresolved at HEAD.
4. Task-description note: the W2 task brief said "M-116-renumber-M-117 same commit" for this landing — that renumber actually belongs to d7ba41f6's companion (item 7); 76e2d351's second finding is the Stbio M-112. The verified code above is unaffected.

---

## 9. N-096 — identity-IOCTL non-issuance registered (dbbcc6bc, doc)

**Claim (Resolution, findings/FIXED/N-096-...md:59-64):** fix-plan option 2 executed — D-DRV-6 registered in PARITY-DELTAS; DRIVER-UTILIZATION's control-IOCTL MATCH row downgraded to "MATCH (wire) / OMITTED (client issuance — deliberate, D-DRV-6)"; no code change.

**Register at HEAD:** docs/DRIVER-UTILIZATION.md:64 — the row now reads `**MATCH (wire)** / **OMITTED (client issuance — deliberate, D-DRV-6)**` with the 0x60002408 rationale. docs/PARITY-DELTAS.md:3120-3132 — `D-DRV-6 — GET_DISK_CONTEXT_MODE (0x60002408) is deliberately NOT issued (N-096)` with REFERENCE/CORE/RATIONALE/VERDICT: ACCEPTED.

**Corpus spot-check:** ghidra-mo/Device_decompiled.c:52598 — `local_6cc = (*local_6a8)(local_6b0,0x60002408,0,0);` followed by the 0xf comparison setting param_1+0x14 — the og's per-open identity query and the informational mode record, exactly as the row describes.

**Verdict: VERIFIED-PARITY.**

---

## 10. N-112 — LZH entry-chain walk (d61474d9)

**Claim (fix-commit message = Resolution; ledger row :386):** four method-key rows (FMT_LZH/LZH5/LZHD/LZH7, one row per method key per the M-092 duplicate-key policy, all ext "lzh", the og stamps one id 0x24), cap 200 MiB (the FileInfo ctor bound 0xC800000); the gate `*(u32*)(buf+2) ∈ {-lh0 0x30686c2d, -lh5 0x35686c2d, -lhd 0x64686c2d, -lh7 0x37686c2d} && buf[6]=='-'` (:46937-46973); the per-method strides (Case A hdr+comp+2 latching the CRC stride for -lh0/-lhd; -lh5 hdr-only latching the ext flag; -lh7 hdr+comp latching ext; ext suppresses the +2), the -2/+2 step-back dance, end = pos+1.

**Code at HEAD:**
- Registry rows src/core/carve/cr_registry.cpp:1220-1265: FMT_LZH (`{'-','l','h','0','-'}`, offset 2, min 0x0B, `0xC800000ull`, validate_lzh) and FMT_LZH5/LZHD/LZH7 same shape — all four present, all ext "lzh", all `0xC800000ull`.
- `validate_lzh` src/core/carve/cr_analysers.cpp:3959-4054: method constants at :3989 (`kLh0 = 0x30686c2du, kLh5 = 0x35686c2du, kLhd = 0x64686c2du, kLh7 = 0x37686c2du`), per-entry `e[6] == '-'` gate + per-method dispatch (:3999-4009), the kind-1 stride `pos += hdr + comp + 2` with the `crcf` latch / ext-suppressed form (:4028-4034), the -lh5 hdr-only + ext latch (:4035-4037), the -lh7 hdr+comp + ext latch (:4038-4041), the step-back dance `pos -= 2 … pos += 2` with `r.end = pos + 1` (:4010-4026).

**Tests executed green (6):** `Carve.LzhMethodFamilyAccepted`, `LzhEntryChainExtent`, `LzhBrokenChainSalvagesLeadingEntries`, `LzhLh5HeaderOnlyStridePinned`, `LzhExtFlagSuppressesTheCrcStride`, `LzhWrongDashByteRejects`.

**Corpus spot-check:** ghidra-searchdll/RAWSearchDLL_decompiled.c:46937-46973 (FUN_18003db20) — read first-hand: the gate body contains exactly `local_38 = 0x35686c2d; local_30 = 0x30686c2d; local_28 = 0x64686c2d; local_20 = 0x37686c2d;` with the `iVar1 == <const> && *(char *)(param_2 + 6) == '-'` conjunctions, stamping `*(undefined4 *)(param_3 + 0x18) = 0x24` — the four constants, the dash byte, and the id-0x24 stamp all byte-exact.

**Verdict: VERIFIED-PARITY.** (Bookkeeping observation: ledger row :386 cites `findings/FIXED/N-112-lzh-carve-absent-…md`, which does NOT exist at HEAD — purged in the 0f9e4345 findings/FIXED sweep; the row's "cite-by-title" Resolution-on-commit convention covers the record, but the file path is stale. Same for N-113's row :387.)

---

## 11. N-113 — text school carve VCF/GPX/OSP/POS/CHFP (b6e0a9ef)

**Claim (fix-commit message = Resolution):** five RawConfig-token-gated list probes landed — VCF (BEGIN:VCARD magic; END:VCARD\r\n walk in 0x400 chunks incl. the 10-byte over-read; multi-card continuation; cap 20 MiB), GPX + GPX_BOM (declaration at 0/+3, the <gpx window [+0x16,+0x216), </gpx> walk; cap 100 MiB), OSP (10-byte `{\r\n "di": ` prefix, the libopenshot needle, extent = find+14+0xF, cap 2 MiB, reject-on-missing), POS ('ProcessOn Schema File' needle, extent = find+21+0x13, cap 0x6400000), CHFP (case-sensitive declaration + case-insensitive <BiffProject at the position-sensitive +0x28 anchor, </BiffProject> walk; cap 100 MiB).

**Code at HEAD:** enum rows include/cr/carve/cr_signatures.h:309-314 (FMT_VCF/GPX/GPX_BOM/OSP/POS/CHFP with the per-probe key comments); kTable rows cr_registry.cpp:1269-1330; validators cr_analysers.cpp:4109-4290 — `validate_vcf` :4109 (0x400 chunks, `"END:VCARD\r\n"` 11-byte needle, `size += rel + 11`, `ci_eq(next, "BEGIN:VCARD", 11)` continuation, cap 0x1400000), `validate_gpx` :4148 (head window, `<gpx` in [+0x16,+0x216)), `validate_osp` :4197, `validate_pos` :4225, `validate_chfp` :4254.

**Tests executed green (10):** the whole `Carve.TextSchool*` family.

**Corpus spot-check:** ghidra-searchdll/RAWSearchDLL_decompiled.c — `_memicmp(param_2,"BEGIN:VCARD",0xb)` :45330 and the continuation `_memicmp(_Buf1,"BEGIN:VCARD",0xb)` :45420 (inside the FUN_18003ba40 walk range the claim cites); `"<BiffProject"` strncpy :19962; `"</BiffProject>"` :20060/:20068; `"ProcessOn Schema File"` :20312/:20320; `"\"libopenshot\":"` :50205 — every needle present at the cited sites.

**Verdict: VERIFIED-PARITY.**

---

## 12. N-115 + N-153 — OfficeTagFromXML typing pass + ZIP inner-LFH double-carve (67de3276)

**Claims:**
- N-115 (Resolution, findings/FIXED/N-115-...md): the og's OfficeTagFromXML pass ported — the office branch walks the LFH chain (bounded 32 entries / 256 KiB) matching rels paths anywhere (word/xl/ppt + the ≥5-char "word"-prefix fallback, office-gated), mimetype/[Content_Types].xml content tags (≤10 KiB/0x2800, stored or deflated), name tags (iWork Index/*.iwa + index.apxl, EPUB OPS/chapter1.html, CDR content/riffData.cdr); **the [Content_Types].xml first-entry REQUIREMENT removed** (reordered zips still type); nine override rows appended (XLSX/PPTX + ODT/ODS/ODP/EPUB/IWORK/HWP/CDR); an unrefinable office zip stays DOCX.
- N-153 (Resolution, findings/FIXED/N-153-...md): multi-entry zips double-carved every inner LFH — suppress_overlap on the ZIP family's **twelve rows** (the family's three + the nine new ones); the S-032 inventory presents entries instead.

**Code at HEAD:**
- The typing pass: src/core/carve/cr_analysers.cpp:1346-1470 — the walk loop `for (int ent = 0; ent < 32 && pos + 30 <= walk_limit; ++ent)` with `walk_limit = min(hit + 256 KiB, probe_limit)`; rels map :1395-1399 (`word/_rels/document.xml.rels`→FMT_DOCX, `xl/_rels/workbook.xml.rels`→FMT_XLSX, `ppt/_rels/presentation.xml.rels`→FMT_PPTX); word-prefix fallback :1400-1401 (nlen>=5, memcmp "word", `typed == 0 && content_types`); name tags :1402-1410 (content/riffData.cdr→CDR; Index/*.iwa + index.apxl→IWORK; OPS/chapter1.html→EPUB); content tags :1411-1443 (mimetype / [Content_Types].xml ≤0x2800 stored-or-deflated via `compress::decompress_block(kGzip…)`: opendocument.text/spreadsheet/presentation, epub+zip, hwp+zip); the fallthrough semantics :1445-1450 (`typed==0 && !content_types` → not this row; unrefinable office stays DOCX — no override).
- The nine rows: cr_registry.cpp:1348-1356 — FMT_XLSX … FMT_CDR, single-line rows, all `validate_office_zip`, all ending `true` (suppress_overlap).
- N-153's twelve suppress rows verified by direct row reads: FMT_ZIP :154-167 (with the N-153 comment), FMT_DOCX :168-181 (N-153 comment :178), FMT_ZIP_CD :372-375 (N-153 comment :372), + the nine :1348-1356. (No other ZIP-family row lacks it.)
- Tests executed green: `Carve.ZipEntriesReorderedStillTyped`, `Carve.OdfEpubTypedByMimetype`, `Carve.IworkAndCdrTypedByEntryName`, `Carve.OfficeUnrefinableStaysDocxAndPlainZipStaysZip`, `Carve.MultiEntryZipCarvesOnce`.

**Corpus spot-check:** ghidra-searchdll/RAWSearchDLL_decompiled.c — the rels-path strcmp sites :52886 (`strcmp(_Str1,"word/_rels/document.xml.rels")`), :52906 (`xl/_rels/workbook.xml.rels`), :52911 (`ppt/_rels/presentation.xml.rels`), and the singleton-walker copies :53176/:53178 — the OfficeTagFromXML map exactly as cited (:53164-53194 region).

**Verdict: VERIFIED-PARITY for the landed code**, with **ONE OVERSTATE (known to the parent lane, recorded here for the filing)**: the N-115 Resolution's "Verified by" list includes `Carve.OoxmlRelsPathSplit` and states "Suite 1119/1119 … 0 fail" — that test is RED at HEAD and per the parent lane's d10b893a worklog was already failing at the landing commit 67de3276 itself. Per my instructions the test is not re-investigated; the Resolution's 0-fail claim is false at the pushed tree. (My own full-suite run at HEAD confirms: exactly 1 failed test, `Carve.OoxmlRelsPathSplit`.)

---

## 13. N-121 — ESP auto-mount step registered (23429142, doc)

**Claim (Resolution + PARITY-DELTAS row):** the ESP auto-mount half registered as a documented HOST CONTRACT (D-90/N-121): the host discovers the ESP by type GUID C12A7328-…, mounts it, passes the BCD path, unmounts after (exception-safe); the core implementation needs the DeviceManager :49956 behavioral decode first (registered next step); the CLI help carries the contract.

**Register at HEAD:** docs/PARITY-DELTAS.md:3035-3060 — the full N-121 row (REGISTERED CONTRACT + RATIONALE + NEXT STEP + VERDICT: ACCEPTED); diff-to-fix/modules/boot-repair.md:25 — `**N-121 (FIXED — registration):** …`; src/app/rrecov.cpp:25-30 — the `--boot-fix-bcd` help text naming the host contract ("auto-mount is a registered host contract, PARITY-DELTAS D-90/N-121").

**Corpus spot-check:** re-workspace/reversed/PartitionRecovery/DeviceManager.mo/strings_ascii.txt:592-595 — `CUpdateGptSystemBootPartition::InitLayout pLayOut=0x%x , disk=%u`, `CUpdateGptSystemBootPartition::MountGptPartition secSize=%u , llSS=%I64u`, `…MountGptPartition %u` — the InitLayout/Mount trace strings the row cites; ghidra-mo/DeviceManager_decompiled.c:49956 — `*param_1 = CUpdateGptSystemBootPartition::vftable;` (the class the :49956 decode pass names).

**Verdict: VERIFIED-PARITY.**

---

## 14. N-122 — NVRAM Boot#### create/delete exclusion registered (ef74eb83, doc)

**Claim (Resolution):** D-90's addendum carries the create/delete exclusion with the EFIBoot cites, the honest-stop pin, the rationale, and the next concrete step; modules/boot-repair.md carries the row.

**Register at HEAD:** docs/PARITY-DELTAS.md:3017+ — `**N-122 addendum — the NVRAM Boot#### create/delete halves are deliberately …**` (with the EFIBoot state-machine cites and the NvramFix.MissingBootVariableStopsTheWalk pin named); diff-to-fix/modules/boot-repair.md:10 (the module row: "No create/delete (v1 scope)") and :19 (the session-8 history row). The pin test exists: tests/unit/test_nvramfix.cpp:417 `TEST(NvramFix, MissingBootVariableStopsTheWalk)`.

**Verdict: VERIFIED-PARITY.**

---

## 15. N-128 — CAPABILITY-MATRIX prebuilt-CI row truth (21a3bd2a, doc)

**Claim (Resolution):** the prebuilt-artifact CI row's dead citations repaired and the buried follow-up given an owner (the ext4/HFS+/Btrfs/ZFS/ReFS staging matrix).

**Register at HEAD:** docs/CAPABILITY-MATRIX.md:21 — the Prebuilt-artifact CI row now cites the four real workflow files (`checks.yml`/`installer.yml`/`release.yml`/`test-benchmark.yml`) with the lane inventory, and the FOLLOW-UP clause reads "**FOLLOW-UP (unowned before N-128, now this row owns it):** the ext4/HFS+/Btrfs/Zfs/ReFS staging matrix — the CI-SCENARIOS.md:81-83 boundary note stays the detail record … next concrete step: a mkfs-class staging script per filesystem …".

**Verdict: VERIFIED-PARITY.**

---

## 16. N-129 — og-tree disposition register numeric defects (98e4e48f, doc)

**Claim (fix-commit message):** six defects repaired, counts re-run against the corpus: api-ms-win-* 44→40 (25 core + 15 crt; 40 unique basenames across 200 copies in 5 payload roots; the corpus's own inventory.csv agrees), CORE_RL_* 19→24, the .mo census 25→30, the DRWReport cite SURFACE-INVENTORY:84→:97, the EuOfficeTools lower-case-u casing, the stale self-nested og-codebase/ (dir) row deleted.

**Register at HEAD (each re-verified first-hand):**
- docs/PARITY-OG-CORE-INVENTORY.md:53 — `api-ms-win-* (40 DLLs: 25 core + 15 crt; 40 unique basenames across 200 copies in 5 payload roots — the corpus's own inventory.csv agrees: 40)`.
- :253 — "all **30** .mo runtime modules"; :267 — "CORE_RL_* (24)".
- :73 — the DRWReport row now cites `SURFACE-INVENTORY.md:97`; I read diff-to-fix/SURFACE-INVENTORY.md line 97 — it is the crash-reporter/disposition row ("Crash supervisor / launcher chain … DRW.exe → DRWUI supervision | OUT-OF-SCOPE"), i.e. the corrected cite points at the right row.
- The `og-codebase/ (dir)` row: absent (grep 0 hits).
- **Independent corpus re-count executed for this report:** parsed re-workspace/inventory.csv — **40 unique api-ms-win-* basenames, 200 total copies** — the register's claim is byte-true against the corpus.

**Verdict: VERIFIED-PARITY.**

---

## 17. N-130 + N-138 — VideoFileRepairer disposition + the OQ-S10-2 refutation superseded (b3e6c287, doc)

**Claims:** N-130 — the VideoFileRepairer.exe register row lands (SIDECAR-OOS with the full evidence chain, enriched with N-138's sfc decode); N-138 — the OQ-S10-2 "dead payload" verdict REFUTED (the sfc is VideoFileRepairer.exe's live encrypted engine-init database: the loader FUN_00514f80 :248131 builds `<dir>\sfc`, gates the " cfs" magic 0x73666320, XOR-decrypts with the 16-byte key, custom-LZ parses); the ledger row SUPERSEDED.

**Register at HEAD:** docs/PARITY-OG-CORE-INVENTORY.md:106 — the VideoViewer/VideoFileRepairer.exe row: "SIDECAR-OOS | Fixo video-repair console engine (1.9 MB … the `<dir>\sfc` encrypted engine-init database — N-138's decode, magic \" cfs\" + the 16-byte XOR key + custom-LZ) … | none | **OOS (verified — N-130)**". FIXED-LEDGER.md:294 — the OQ-S10-2 row: "**RESOLVED (SUPERSEDED by N-138's decode)** … the zero-hit grep could not match DAT_ symbols or hex constants — hypothesis 1 … was the correct answer".

**Corpus spot-check:** re-workspace/reversed/ghidra-wave2/VideoFileRepairer_decompiled.c exists; `FUN_00514f80` present (2 references) — the loader function the decode names. (The " cfs"/sfc constants are DAT_/hex — the finding itself documents why a plain grep cannot see them; consistent.)

**Verdict: VERIFIED-PARITY.**

---

## 18. N-132 — display-config data layers (4e74ee2f)

**Claim (Resolution, findings/FIXED/N-132-...md):** both tables landed as query-layer data + public lookups: `query::summary_family_for_extension` (the FilterFolder.ini SumaryFileType 11 groups, the ini's order verbatim) and `query::type_display_order` / `type_display_category_name` (PriorityDisplayType.xml's 5-category priority tier with the 35 alias-ranked extensions, "read directly from og-codebase/config/PriorityDisplayType.xml this session"); registered as D-113; R-31/API-DELTA row repaired.

**Code at HEAD:** include/cr/query/cr_query.h:73-76 (the three public lookups); src/core/query/cr_config_display_data.cpp:43-55 — `kSummaryGroups` with the **11 groups**; :67-88 `summary_family_for_extension`; :100-119 `kOrder` — **35 entries, 5 categories**, ranks 0..N per category; :90-128 `type_display_order`; :130-145 `type_display_category_name`.

**Corpus verification (byte-exact, both files):**
- og-codebase/config/FilterFolder.ini:6 — `SumaryFileType=aiff_aif;m4p_m4a_m4v_mp4;html_htm;rm_rmvb;mpg_mpeg_mov;mkv_webm;asf_wma_wmv;jpg_jpeg;tif_tiff;ai_pdf;zip_docx_pptx_xlsx;` — identical 11 groups in identical order to kSummaryGroups.
- og-codebase/config/PriorityDisplayType.xml:3-46 — `<type Description="Picture">` (alias 00jpg,01png,02jpeg,03gif,04tif,05eps,06psd,07bmp,08ico), `Video` (00mp4…06wmv), `Document` (00doc…06pptx), `Audio` (00mp3…06aac), `Archive` (00zip,01rar,027z,03tar,04gzip) — the core's kOrder table is a verbatim transcription (same extensions, same per-category ranks), and type_display_category_name returns the XML's Description strings verbatim (Picture/Video/Document/Audio/Archive).

**Tests executed green (6):** the whole `ConfigDisplay.*` family (SummaryFamilyUnificationGroups, SummaryFamilyUngroupedStandsAlone, NullAndEmptySafe, TypeDisplayOrderVerbatim, TypeDisplayOrderUnrankedReturnsFalse, TypeDisplayCategoryNamesVerbatim).

**Verdict: VERIFIED-PARITY.**

---

## 19. N-133 — engine-side volume labels (838fe939)

**Claim (Resolution, findings/FIXED/N-133-...md):** `orchestrator::probe_slice_volume_label` with the four family decoders — NTFS via the real NtfsVolume bootstrap (boot + $MFT + read_record(3) + fixup + the 0x60 $VOLUME_NAME attribute, wide→narrow), FAT the +0x2B boot field, exFAT the ROOT-DIRECTORY 0x83 dentry (EntryType@0, CharacterCount@1, UTF-16 label@4 — the finding's "+0x80" corrected), ext4 the superblock s_volume_name (sb+120); wiring into EngineRun.volume_label at run_engine_dispatch's tail; the derived-label (image-stem) precedence replaced by the engine label; CLI/UI precedence --label > engine > stem; ScanStats.volume_label surfaced; HFS+ returns ERR_UNSUPPORTED (registered residual).

**Code at HEAD:** src/core/orchestrator/cr_vollabel.cpp (whole file, 194 lines):
- FAT `fat_label_from_boot` :69-79 (the 11-byte +0x2B field, pad-trimmed, label-less → empty),
- exFAT `exfat_label_from_root` :81-106 (root-dir first cluster, `buf[off] == 0x83 && buf[off+1] != 0 && buf[off+1] <= 11` → `narrow_utf16_to_label(buf + off + 4, …)` — the dentry form, not a boot offset),
- NTFS `ntfs_label_from_volume` :108-142 (`NtfsVolume::open` :112, `read_record(3, rec)` :116, RECORD_MAGIC_FILE check, `fixup_apply` :125-126, `enum_attributes`, `a.type != ntfs::ATTR_TYPE_VOLUME_NAME || a.nonresident` filter :132, resident value wide→narrow :138),
- ext4 `ext4_label_from_superblock` :144-161 (sb@1024, magic 0xEF53 @56, `s_volume_name` 16 bytes @120),
- dispatcher `probe_slice_volume_label` :165-191 ("EXFAT"/"NTFS" at +0x03, FAT strings at +0x36/+0x52, ext4 superblock, else ERR_UNSUPPORTED).
- Wiring: include/cr/orchestrator/cr_scan.h:354 `char volume_label[33]` in EngineRun; cr_scan.cpp:1532 `(void)probe_slice_volume_label(slice, r.volume_label);` at the dispatch tail; stats surface :1778-1783; src/app/rrecov.cpp:342-373 `fill_volume_label` with `volume_label_derived = true` (the replaceable stem).

**Tests executed green (7):** `VolumeLabel.Ext4FromSuperblock`, `Ext4LabellessYieldsEmpty`, `NtfsFromVolumeRecord`, `FatFromBootSector`, `FatLabellessYieldsEmpty`, `CallerValueWins`, `UnknownFamilyLeavesFallback`.

**Corpus spot-check:** ghidra-searchdll/NTFSSearchDLL_decompiled.c:50963-50981 (FUN_18003f3a0) — `lVar4 = FUN_180041850((longlong)param_1,0x60,0)` (the type-0x60 attribute lookup), the `(lVar4 - param_1) + 0x18 + uVar2 < 0x400` bound, and `FUN_18002cc30((LPCWSTR)(lVar4 + 0x18), uVar2/2, param_2, …)` — the +0x18 value read with wide→narrow, exactly the decode chain the Resolution cites (:50963-50980).

**Verdict: VERIFIED-PARITY.**

---

## 20. N-137 — thumbnail ABI header truth (896c8895)

**Claim (Resolution, findings/FIXED/N-137-...md):** two contract-truth defects repaired — (1) the cr_ui_thumbnail_generate section comment's "0x50030004 ERR_TRUNCATED never returned for thumbnails" denial replaced with the truth (a decode REFUSAL returns ERR_TRUNCATED with info->truncated also set; in-band truncation stays an Ok-path disclosure); (2) the caps block's "allow_embedded validated 0/1" claim now ENFORCED — values above 1 refused with ERR_BAD_PARAMETER, 0 = default-ON, 1 = explicit-ON, the no-off-position design documented.

**Code at HEAD:**
- include/cr/cr_ui.h:1278-1290 — the status-mapping comment now reads "0x50030004 ERR_TRUNCATED is returned for a decode REFUSAL (a damaged/short stream the strict decoders reject — ABI-CONTRACT 7i's mapping; the refusal branch also sets info->truncated); in-band truncation … NOT an error".
- include/cr/cr_ui.h:1249-1256 — the caps comment: "allow_embedded 0 or 1 - values above 1 are refused with ERR_BAD_PARAMETER … There is no off position by design."
- Enforcement: src/ui/cr_ui.cpp:3901-3909 — `if (caps->allow_embedded > 1) return ret_(ERR_BAD_PARAMETER, "caps->allow_embedded must be 0 or 1 (got %u)", …)`.

**Tests executed green:** `CrUi.ThumbnailCapsAllowEmbeddedValidated`, `CrUi.ThumbnailHeaderMatchesContract` (the doc-truth pin).

**Corpus note:** N/A (ABI-comment truth repair; no corpus constant). **Task-description note:** the W2 brief described N-137 as "(entries field)" — that is NOT N-137's content. The entries-field truth ("cr_ui_metadata has no entries field; preview_metadata_impl discards Metadata.entries") is the **M-116** re-open annotation (S11-R lane, ledger row :345), a different finding; I verified that annotation's factual basis stands at HEAD (cr_ui.h's cr_ui_metadata block has no entries member), but N-137 itself is the ERR_TRUNCATED/allow_embedded pair verified above.

**Verdict: VERIFIED-PARITY** (with the task-brief description corrected as above).

---

## 21. N-142 — dead CrPreview origin pair removed (c26f77d9)

**Claim (fix-commit message):** `src_session`/`src_node_id` in struct CrPreview were written-never-read (the rebase-dropped variant's leftover); the pair removed; the comment re-truthed to the landed promotion's own-argument addressing per ABI-CONTRACT 7i; grep census zero; cr_ui_tests 59/59.

**Code at HEAD:** src/ui/cr_ui.cpp:903-912 — `struct CrPreview` now holds only `owned` + `reader` plus the re-truthed comment ("…addresses the node through cr_ui_thumbnail_generate's own session/node_id arguments, never through this handle (ABI-CONTRACT §7i)… N-142 removed the surviving dead pair"). The remaining `src_session` occurrences in the file (:886, :891, :3113, :3717-3718) belong to the SNAPSHOT/RECOVERY structs (the S-052 lazily-armed basis source — a different, live member), not CrPreview. `preview_open_session_impl` no longer stamps any origin fields.

**Tests executed:** full `cr_ui_tests` — **61/61 PASSED** (superset of the 59/59 claim; two tests were added by later landings N-137/M-109).

**Verdict: VERIFIED-PARITY.**

---

## 22. N-151 — nested-declaration double-carves repaired (3acdbf97; renumbered from this lane's N-150 at 6f7d33e8)

**Claim (Resolution, findings/FIXED/N-151-...md):** suppress_overlap=true on the four M-093 rows (FMT_XML, FMT_XML_BOM, FMT_SVG_CD01, FMT_SVG) and the three N-113 rows (FMT_VCF, FMT_GPX, FMT_GPX_BOM) — the interior re-match of an emitted declaration stream is the same document; the count pins close the parallel landing's missed shapes.

**Code at HEAD (all seven rows read directly):** src/core/carve/cr_registry.cpp:
- FMT_XML :1107-1117 (ends `true},`), FMT_XML_BOM :1118-1128, FMT_SVG_CD01 :1129-1139, FMT_SVG :1140-1150 (the A-012-restored row also carries the flag),
- FMT_VCF :1269-1282 (with the N-151-class comment naming `TextSchoolVcfInnerCardIsSameFile`), FMT_GPX :1283-1297, FMT_GPX_BOM :1298-1308 — all seven end with `true},`.
(FMT_OSP / FMT_POS deliberately do NOT carry the flag — the finding scoped the fix to the nested-declaration shapes.)

**Tests executed green:** `Carve.SvgDeclaredDocumentCarvesOnce`, `Carve.GpxBomFormCarvesOnce`, `Carve.TextSchoolVcfInnerCardIsSameFile`.

**Verdict: VERIFIED-PARITY.**

---

## 23. Item 23 — "N-142(older) N-133's suite etc." — SKIPPED per the task brief.

---

## Cross-cutting executed evidence

- Full `cr_tests` at HEAD: **only `Carve.OoxmlRelsPathSplit` fails** (parent-lane capture; per instructions not re-investigated). Every other landing test named in the 22 verified landings ran green individually in this session.
- Full `cr_ui_tests` at HEAD: 61/61.
- Registry structural state at HEAD: 142 kTable rows == FMT_COUNT 142, id order == index order (validate_registry 0 rejects; RegistryF8Gate green).

## Bookkeeping observations (not landing-parity defects; feed the parent lane's filings)

1. **Phantom SHA in the ledger (M-112 Stbio row, FIXED-LEDGER.md:180):** "Fixed in commit 5103ecd8" — 5103ecd8 exists in no ref; the real fix is 76e2d351. Already discovered by the S12 lane and absorbed by S42; **still wrong at HEAD**.
2. **Comment-truth drift in the landed code (cr_thumbnail.cpp:163):** the read-loop comment says "N-112" while the finding/commit/ledger all say M-112 (the drift was introduced by the landing commit 76e2d351 itself).
3. **M-112 ID collision unresolved at HEAD:** the Stbio row (:180) and the ACTIVE S11B finding `M-112-csl-sizefield-0x44c-branch-unported-unowned.md` both claim M-112.
4. **Stale ledger file paths for retired-then-purged findings:** rows :386 (N-112) and :387 (N-113) cite `findings/FIXED/N-112-…md` / `N-113-…md` which do not exist at HEAD (purged in the 0f9e4345 sweep). The Resolution-on-commit convention covers the record; the paths are dangling.
5. **A-012's "kTable must have 120 rows" (task framing) is a fix-time snapshot:** at HEAD kTable is 142 rows and the F8 invariant — the actual load-bearing claim — holds. Not a defect; recorded so the parent lane's filing cites the invariant, not the stale count.
6. **Task-brief description corrections:** (a) A-012's fix commit is e4273f2b per the Resolution (eba47f11 is the record commit); (b) the "M-116-renumber-M-117 same commit" attribution belongs to d7ba41f6 (S-054's companion), not 76e2d351; (c) N-137 is not the "entries field" finding (that is M-116's annotation).

## Verdict table

| # | Finding | Fix commit | Verdict |
|---|---------|-----------|---------|
| 1 | A-012 FMT_SVG row / F8 gate | e4273f2b (record: eba47f11) | VERIFIED-PARITY |
| 2 | S-049 naming contract | d4b487c8 | VERIFIED-PARITY |
| 3 | S-051 whole-disk targets | ad0535d6/6ecd6bb4 + 3f7c190e | VERIFIED-PARITY |
| 4 | M-104 display-name roots (4 tiers) | same | VERIFIED-PARITY |
| 5 | S-052 LDM basis | f4e500de | VERIFIED-PARITY |
| 6 | S-053+N-141 ReFS emit + junk filter | e10ec381 | VERIFIED-PARITY |
| 7 | S-054 TS cap 200 MB (+M-117 re-walk) | d7ba41f6 | VERIFIED-PARITY |
| 8 | M-109 200 MB anchor + Stbio read loop | 76e2d351 | VERIFIED-PARITY (bookkeeping obs. 1-3) |
| 9 | N-096 identity-IOCTL doc | dbbcc6bc | VERIFIED-PARITY |
| 10 | N-112 LZH chain walk | d61474d9 | VERIFIED-PARITY (obs. 4) |
| 11 | N-113 text school | b6e0a9ef | VERIFIED-PARITY (obs. 4) |
| 12 | N-115 typing pass + N-153 inner-LFH | 67de3276 | VERIFIED-PARITY; **OVERSTATE**: Resolution's "Verified by" includes the now-failing `Carve.OoxmlRelsPathSplit` + "0 fail" suite claim (known parent capture) |
| 13 | N-121 ESP auto-mount doc | 23429142 | VERIFIED-PARITY |
| 14 | N-122 NVRAM exclusion doc | ef74eb83 | VERIFIED-PARITY |
| 15 | N-128 CAPABILITY-MATRIX row | 21a3bd2a | VERIFIED-PARITY |
| 16 | N-129 inventory numeric repairs | 98e4e48f | VERIFIED-PARITY (counts re-verified vs corpus) |
| 17 | N-130+N-138 VideoFileRepairer/sfc | b3e6c287 | VERIFIED-PARITY |
| 18 | N-132 display-config layers | 4e74ee2f | VERIFIED-PARITY (byte-exact vs both corpus config files) |
| 19 | N-133 engine volume labels | 838fe939 | VERIFIED-PARITY |
| 20 | N-137 thumbnail ABI header truth | 896c8895 | VERIFIED-PARITY (task brief's "(entries field)" is M-116's annotation, not N-137) |
| 21 | N-142 dead CrPreview pair | c26f77d9 | VERIFIED-PARITY |
| 22 | N-151 nested-declaration suppress | 3acdbf97 | VERIFIED-PARITY |
| 23 | N-142(older)/N-133 suite | — | SKIPPED per brief |
