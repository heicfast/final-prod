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

# S42/W4 — core-side citation-gap sweep + active-board liveness re-triage

**Task ID:** S42/W4 · **Mode:** READ-ONLY audit sub-agent (no repository file modified; this sheet is the only output, written under `s42-scratch/`).
**Tree pins:** core_recovery HEAD `d10b893a` (a worklog-only commit on top of the audited code baseline `869da779` — `git diff --stat 869da779 HEAD` touches only `worklog.md` + `worklog/session-42-continuation-audit-prompt.md`, so every code-level verdict below is a verdict about the `869da779` code tree); corpus `be0cfe9`. Board at HEAD: findings/SEVERE 3 · MAJOR 21 · MINOR 17 · INFORMATIONAL 8 = 49 active (+ 17 retired files in findings/FIXED/, indexed by `diff-to-fix/FIXED-LEDGER.md`).
**Scope note:** the last full core-tree citation sweep was the session-6 delta (SUMMARY:4935+; its "first-time-cited core files (8)" register: cr_elevate, cr_destio, cr_snapshot, cr_cached, cr_decomp, cr_sha256, cr_error, cr_diskmap). This sweep re-runs the census at the current tree (~200 commits later) and re-triages every active finding for liveness.

---

## Part 1 — the citation-gap sweep (mark-all-remaining, core-side)

### P1.0 Method

1. Census: `rg --files` over `src/` (85 `.cpp`; the 5 `CMakeLists.txt` + `src/ui/cr_ui_exports.def` sit outside the task's 85-file enumeration and are themselves trail-cited — the `.def` via `s4-suite-green-and-abi-check.md`/`modules/abi.md`), `include/` (83 headers), `tests/unit/` (64 files — the task's "tests/ (64)"; the rest of `tests/` is `win-stub/` stubs + `data/` fixtures + `tests/ui_abi_check.c`, all cited by the ABI/CI evidence files).
2. Citation check per file: `rg -F -c <basename> diff-to-fix/` (exact-name citation count across findings/, modules/, evidence/, and the top-level registers SUMMARY.md / CROSS-REF-MATRIX.md / FIXED-LEDGER.md / API-DELTA.md / SCAN-ENGINE-DELTA.md / SURFACE-INVENTORY.md / CORPUS-ERRATA.md / PARAMETERS.md). For every file with 0 exact hits I also ran the stem (basename minus extension) and inspected the matching lines to distinguish "the stem hit names THIS file" from "the stem hit names a sibling" (e.g. every `cr_dict` stem hit cites `cr_dict_data.cpp`, every `cr_report` stem hit cites `cr_report.h`).
3. Disposition check for the uncited set: which module doc (`diff-to-fix/modules/*.md`) or register (I-031/I-032/I-033/I-034, SURFACE-INVENTORY, FIXED-LEDGER rows, API-DELTA rows, CORPUS-ERRATA rows) covers the file's surface; directory-level cites (`include/cr/ntfs/ (7 headers)`) and symbol-level cites (`text::utf8_to_u16`, `storage::FreeMap`, `cr::health::read_smart_ata`) counted as surface citations, not file citations — recorded as such.

### P1.1 Census arithmetic (at HEAD, exact-name citations)

| tree | files | exact-name cited | uncited by exact name | uncited in ANY form (stem too) |
|---|---|---|---|---|
| src/ (.cpp) | 85 | 74 | 11 | **1** (`src/core/health/cr_family.cpp`) |
| include/ | 83 | 59 | 24 | **7** (`cr_bits.h`, `cr_text.h`, `cr_exfat_engine.h`, `cr_exfat_wire.h`, `cr_fat_engine.h`, `cr_freemap.h` — plus `cr_dict.h` whose every stem hit names `cr_dict_data.cpp`) |
| tests/unit/ | 64 | 37 | 27 | **27** (no test stem hits at all) |
| **total** | **232** | **170 (73%)** | **62** | **35** |

For comparison, the session-6 register's 8 "first-time-cited" files are now: 4 stem-cited (cr_elevate, cr_destio, cr_snapshot, cr_decomp — via `modules/storage.md:74-77`, the SUMMARY:4970 register, `M-105:19`), 3 exact-cited (cr_error.cpp ×3, cr_diskmap.h ×1, cr_cached.cpp ×2), 1 exact-cited via probe evidence (cr_sha256.cpp ×1 + `modules/storage.md:77`). The session-6 sweep's coverage has held — no file it dispositioned has since gone stale.

The heaviest-cited files (context for the parent lane): `src/ui/cr_ui.cpp` (106), `src/app/rrecov.cpp` (77), `src/core/orchestrator/cr_scan.cpp` (68), `src/core/carve/cr_analysers.cpp` (61), `src/core/carve/cr_registry.cpp` (44), `include/cr/cr_ui.h` (101), `include/cr/tree/cr_node.h` (29) — the carve/UI/orchestrator cores are the trail's center of gravity.

---

### P1.2a src/ census (85 .cpp files; the 5 CMakeLists.txt + cr_ui_exports.def excluded from the 85-count per the task's enumeration)

| file | exact-name cites in diff-to-fix/ | verdict / covering disposition |
|---|---|---|
| `src/app/rrecov.cpp` | 77 | cited |
| `src/core/apply/cr_apply.cpp` | 5 | cited |
| `src/core/bcdfix/cr_bcdfix.cpp` | 0 | UNCITED-exact; stem-cited (M-113:9,:25); surface = M-040 FIXED + modules/boot-repair.md P25 |
| `src/core/btrfs/cr_btrfs_cache.cpp` | 0 | UNCITED-exact; .h cited (modules/btrfs.md:158 P47/P48 N-072) |
| `src/core/btrfs/cr_btrfs_engine.cpp` | 4 | cited |
| `src/core/carve/cr_analysers.cpp` | 61 | cited |
| `src/core/carve/cr_carve.cpp` | 39 | cited |
| `src/core/carve/cr_rawsigns.cpp` | 2 | cited |
| `src/core/carve/cr_rawsigns_data.cpp` | 12 | cited |
| `src/core/carve/cr_registry.cpp` | 44 | cited |
| `src/core/carve/cr_usersigns.cpp` | 0 | UNCITED-exact; .h cited (S-039 SUMMARY:1561; N-108 s10-reverif:119) |
| `src/core/check/cr_fscheck.cpp` | 5 | cited |
| `src/core/compress/cr_decomp.cpp` | 3 | cited |
| `src/core/cr_parallel.cpp` | 4 | cited |
| `src/core/crypto/cr_aes.cpp` | 3 | cited |
| `src/core/crypto/cr_sha256.cpp` | 1 | cited |
| `src/core/err/cr_errmod.cpp` | 1 | cited |
| `src/core/err/cr_error.cpp` | 3 | cited |
| `src/core/err/cr_log.cpp` | 3 | cited |
| `src/core/exfat/cr_exfat_dentry.cpp` | 3 | cited |
| `src/core/exfat/cr_exfat_quick.cpp` | 8 | cited |
| `src/core/ext4/cr_ext4_engine.cpp` | 36 | cited |
| `src/core/fat/cr_chain.cpp` | 11 | cited |
| `src/core/fat/cr_fat_quick.cpp` | 3 | cited |
| `src/core/fat/cr_lfn.cpp` | 3 | cited |
| `src/core/health/cr_csmi.cpp` | 0 | UNCITED-exact; stem-cited (SUMMARY:3011; s11-triage:70); health.md:171 CSMI MATCH |
| `src/core/health/cr_family.cpp` | 0 | UNCITED any-form (0 stem too); health.md:108 D16 vendor-classification MATCH |
| `src/core/health/cr_grade.cpp` | 1 | cited |
| `src/core/health/cr_nvme.cpp` | 3 | cited |
| `src/core/health/cr_smart_ata.cpp` | 0 | UNCITED-exact; stem-cited (health.md:157,172; SUMMARY:3011) |
| `src/core/hfsplus/cr_hfsplus_engine.cpp` | 16 | cited |
| `src/core/lost/cr_candidates.cpp` | 20 | cited |
| `src/core/lost/cr_consistency.cpp` | 0 | UNCITED-exact; stem-cited (lost-partition.md:230 'cr_consistency.h/.cpp') |
| `src/core/ntfs/cr_attributes.cpp` | 1 | cited |
| `src/core/ntfs/cr_bitmap.cpp` | 4 | cited |
| `src/core/ntfs/cr_bootstrap.cpp` | 3 | cited |
| `src/core/ntfs/cr_fixup.cpp` | 2 | cited |
| `src/core/ntfs/cr_mftwalk.cpp` | 31 | cited |
| `src/core/ntfs/cr_runlist.cpp` | 2 | cited |
| `src/core/nvramfix/cr_nvramfix.cpp` | 1 | cited |
| `src/core/orchestrator/cr_filterfolder.cpp` | 3 | cited |
| `src/core/orchestrator/cr_filterfolder_data.cpp` | 7 | cited |
| `src/core/orchestrator/cr_scan.cpp` | 68 | cited |
| `src/core/orchestrator/cr_vollabel.cpp` | 1 | cited |
| `src/core/preview/cr_preview.cpp` | 22 | cited |
| `src/core/preview/cr_preview_text_exts.cpp` | 2 | cited |
| `src/core/preview/cr_preview_text_exts_data.cpp` | 6 | cited |
| `src/core/preview/cr_thumbnail.cpp` | 17 | cited |
| `src/core/query/cr_config_display_data.cpp` | 2 | cited |
| `src/core/query/cr_filefilter_data.cpp` | 7 | cited |
| `src/core/query/cr_query.cpp` | 8 | cited |
| `src/core/recovery/cr_dict.cpp` | 0 | UNCITED (stem hits = cr_dict_data.cpp); N-064 FIXED + API-DELTA row 43 + I-033 |
| `src/core/recovery/cr_dict_data.cpp` | 3 | cited |
| `src/core/recovery/cr_report.cpp` | 0 | UNCITED (stem hits = cr_report.h); modules/writer.md D13 report-schema MATCH |
| `src/core/recovery/cr_safewriter.cpp` | 10 | cited |
| `src/core/refs/cr_refs_detect.cpp` | 4 | cited |
| `src/core/refs/cr_refs_engine.cpp` | 19 | cited |
| `src/core/reghive/cr_offlineos.cpp` | 1 | cited |
| `src/core/reghive/cr_reghive.cpp` | 3 | cited |
| `src/core/repair/cr_rawfixer.cpp` | 10 | cited |
| `src/core/session/cr_crsf.cpp` | 9 | cited |
| `src/core/session/cr_records.cpp` | 30 | cited |
| `src/core/storage/cr_cached.cpp` | 2 | cited |
| `src/core/storage/cr_decrypt_device.cpp` | 0 | UNCITED (.h cited x3); M-033 FIXED + modules/storage.md:22 |
| `src/core/storage/cr_devmap.cpp` | 2 | cited |
| `src/core/storage/cr_image.cpp` | 4 | cited |
| `src/core/storage/cr_retry.cpp` | 1 | cited |
| `src/core/storage/cr_slice.cpp` | 3 | cited |
| `src/core/table/cr_crc32.cpp` | 1 | cited |
| `src/core/table/cr_ldm.cpp` | 10 | cited |
| `src/core/table/cr_table_parse.cpp` | 3 | cited |
| `src/core/tree/cr_diskmap.cpp` | 3 | cited |
| `src/core/tree/cr_grading.cpp` | 6 | cited |
| `src/core/tree/cr_node.cpp` | 4 | cited |
| `src/core/tree/cr_snapshot.cpp` | 1 | cited |
| `src/core/zfs/cr_zfs_engine.cpp` | 15 | cited |
| `src/ui/cr_ui.cpp` | 106 | cited |
| `src/win/cr_bitlocker.cpp` | 14 | cited |
| `src/win/cr_bootfix.cpp` | 9 | cited |
| `src/win/cr_destio.cpp` | 5 | cited |
| `src/win/cr_elevate.cpp` | 0 | UNCITED-exact; stem-cited (modules/storage.md:74; SUMMARY:4970 register; M-105:19,68) |
| `src/win/cr_enumerate.cpp` | 7 | cited |
| `src/win/cr_health.cpp` | 9 | cited |
| `src/win/cr_image_win.cpp` | 6 | cited |
| `src/win/cr_transport.cpp` | 29 | cited |

### P1.2b include/ census (83 headers)

| file | exact-name cites in diff-to-fix/ | verdict / covering disposition |
|---|---|---|
| `include/cr/apply/cr_apply.h` | 4 | cited |
| `include/cr/bcdfix/cr_bcdfix.h` | 0 | UNCITED-exact; stem-cited (M-113); boot-repair.md:9 'cr/bcdfix' |
| `include/cr/btrfs/cr_btrfs_cache.h` | 1 | cited |
| `include/cr/btrfs/cr_btrfs_engine.h` | 0 | UNCITED-exact; directory-cited (btrfs.md:8) + .cpp cited x4 |
| `include/cr/carve/cr_carve.h` | 13 | cited |
| `include/cr/carve/cr_rawsigns.h` | 8 | cited |
| `include/cr/carve/cr_signatures.h` | 14 | cited |
| `include/cr/carve/cr_usersigns.h` | 2 | cited |
| `include/cr/check/cr_fscheck.h` | 6 | cited |
| `include/cr/compress/cr_decomp.h` | 0 | UNCITED-exact; .cpp cited (storage.md:76; ntfs.md:323 LZNT1) |
| `include/cr/cr_bits.h` | 0 | UNCITED any-form; no diff-to-fix disposition (see P1.4 residual) |
| `include/cr/cr_errmod.h` | 1 | cited |
| `include/cr/cr_error.h` | 14 | cited |
| `include/cr/cr_log.h` | 2 | cited |
| `include/cr/cr_parallel.h` | 0 | UNCITED-exact; .cpp cited (CORPUS-ERRATA:112-113 FIFO-not-work-stealing) |
| `include/cr/cr_recycle.h` | 2 | cited |
| `include/cr/cr_text.h` | 0 | UNCITED by name; symbol-cited text::utf8_to_u16 (ext4.md:31; btrfs.md:33; zfs.md:35) |
| `include/cr/cr_types.h` | 1 | cited |
| `include/cr/cr_ui.h` | 101 | cited |
| `include/cr/crypto/cr_aes.h` | 0 | UNCITED-exact; .cpp cited (storage.md:22; M-033) |
| `include/cr/crypto/cr_sha256.h` | 0 | UNCITED-exact; stem-cited (storage.md:77; SUMMARY:4970 register) |
| `include/cr/exfat/cr_exfat_engine.h` | 0 | UNCITED-exact; directory-cited (exfat.md:10 'include/cr/exfat/') |
| `include/cr/exfat/cr_exfat_wire.h` | 0 | UNCITED-exact; directory-cited (exfat.md:10) |
| `include/cr/ext4/cr_ext4_engine.h` | 4 | cited |
| `include/cr/fat/cr_fat_engine.h` | 0 | UNCITED-exact; directory-cited (fat.md:8 'include/cr/fat/') |
| `include/cr/fat/cr_fat_wire.h` | 1 | cited |
| `include/cr/health/cr_csmi.h` | 1 | cited |
| `include/cr/health/cr_grade.h` | 2 | cited |
| `include/cr/health/cr_nvme.h` | 0 | UNCITED-exact; .cpp cited (health.md:6,:93) |
| `include/cr/health/cr_smart_ata.h` | 0 | UNCITED-exact; stem-cited (health.md:172 read_smart_ata seam) |
| `include/cr/hfsplus/cr_hfsplus_engine.h` | 1 | cited |
| `include/cr/lost/cr_consistency.h` | 1 | cited |
| `include/cr/lost/cr_lostpart.h` | 5 | cited |
| `include/cr/ntfs/cr_attributes.h` | 0 | UNCITED-exact; directory-cited (ntfs.md:10 '7 headers') |
| `include/cr/ntfs/cr_bitmap.h` | 1 | cited |
| `include/cr/ntfs/cr_bootstrap.h` | 1 | cited |
| `include/cr/ntfs/cr_fixup.h` | 0 | UNCITED-exact; directory-cited (ntfs.md:10); .cpp cited (ntfs-viability:81) |
| `include/cr/ntfs/cr_mftwalk.h` | 9 | cited |
| `include/cr/ntfs/cr_ntfs_wire.h` | 4 | cited |
| `include/cr/ntfs/cr_runlist.h` | 0 | UNCITED-exact; directory-cited (ntfs.md:10); .cpp cited (ntfs-viability:99) |
| `include/cr/nvramfix/cr_nvramfix.h` | 0 | UNCITED-exact; unit-cited (boot-repair.md:10 'cr/nvramfix') |
| `include/cr/orchestrator/cr_filterfolder.h` | 1 | cited |
| `include/cr/orchestrator/cr_scan.h` | 21 | cited |
| `include/cr/orchestrator/cr_vollabel.h` | 0 | UNCITED-exact; .cpp cited (N-133 Resolution) |
| `include/cr/preview/cr_preview.h` | 22 | cited |
| `include/cr/preview/cr_thumbnail.h` | 17 | cited |
| `include/cr/query/cr_query.h` | 11 | cited |
| `include/cr/recovery/cr_dict.h` | 0 | UNCITED (stem hits = cr_dict_data); N-064 FIXED ledger row |
| `include/cr/recovery/cr_report.h` | 3 | cited |
| `include/cr/recovery/cr_safewriter.h` | 2 | cited |
| `include/cr/refs/cr_refs_detect.h` | 2 | cited |
| `include/cr/refs/cr_refs_engine.h` | 3 | cited |
| `include/cr/reghive/cr_offlineos.h` | 3 | cited |
| `include/cr/reghive/cr_reghive.h` | 1 | cited |
| `include/cr/repair/cr_rawfixer.h` | 0 | UNCITED-exact; modules/rawfixer.md:7-8 covers the surface (.cpp cited x10) |
| `include/cr/session/cr_crsf.h` | 3 | cited |
| `include/cr/session/cr_records.h` | 1 | cited |
| `include/cr/storage/cr_cached.h` | 4 | cited |
| `include/cr/storage/cr_decrypt_device.h` | 3 | cited |
| `include/cr/storage/cr_device.h` | 11 | cited |
| `include/cr/storage/cr_directalign.h` | 1 | cited |
| `include/cr/storage/cr_freemap.h` | 0 | UNCITED by name; type-cited storage::FreeMap (visualization.md:85; abi.md:74; API-DELTA #23) |
| `include/cr/storage/cr_image.h` | 1 | cited |
| `include/cr/storage/cr_retry.h` | 2 | cited |
| `include/cr/storage/cr_slice.h` | 3 | cited |
| `include/cr/table/cr_ldm.h` | 17 | cited |
| `include/cr/table/cr_table_parse.h` | 2 | cited |
| `include/cr/table/cr_table_wire.h` | 7 | cited |
| `include/cr/tree/cr_diskmap.h` | 1 | cited |
| `include/cr/tree/cr_grading.h` | 7 | cited |
| `include/cr/tree/cr_node.h` | 29 | cited |
| `include/cr/tree/cr_snapshot.h` | 0 | UNCITED-exact; stem-cited (SUMMARY:4970 register; s11-board-truth:60) |
| `include/cr/win/cr_bitlocker.h` | 6 | cited |
| `include/cr/win/cr_bootfix.h` | 0 | UNCITED-exact; .cpp cited x9 (boot-repair.md; N-122; M-113) |
| `include/cr/win/cr_destio.h` | 0 | UNCITED-exact; .cpp cited (writer.md:59,:101; s11-triage:33; s10-reverif:71) |
| `include/cr/win/cr_driver_status.h` | 7 | cited |
| `include/cr/win/cr_elevate.h` | 1 | cited |
| `include/cr/win/cr_enumerate.h` | 1 | cited |
| `include/cr/win/cr_health.h` | 5 | cited |
| `include/cr/win/cr_rda.h` | 16 | cited |
| `include/cr/win/cr_transport.h` | 3 | cited |
| `include/cr/win/cr_winenv.h` | 1 | cited |
| `include/cr/zfs/cr_zfs_engine.h` | 7 | cited |

### P1.2c tests/unit census (64 files)

| file | exact-name cites in diff-to-fix/ | verdict / covering disposition |
|---|---|---|
| `tests/unit/main.cpp` | 2 | cited |
| `tests/unit/test_apply.cpp` | 2 | cited |
| `tests/unit/test_bcdfix.cpp` | 0 | covered: boot-repair.md P25 stages a+b (M-040) |
| `tests/unit/test_bitlocker_win.cpp` | 1 | cited |
| `tests/unit/test_bitlocker_xts.cpp` | 2 | cited |
| `tests/unit/test_btrfs_volume.cpp` | 2 | cited |
| `tests/unit/test_cached.cpp` | 0 | covered: storage module; cr_cached.cpp cited x2; SUMMARY:5093 register |
| `tests/unit/test_carve.cpp` | 12 | cited |
| `tests/unit/test_config_display.cpp` | 0 | covered: N-132 FIXED (config-display data landed) |
| `tests/unit/test_consistency.cpp` | 0 | covered: modules/lost-partition.md:230 |
| `tests/unit/test_cr_ui.cpp` | 17 | cited |
| `tests/unit/test_crc32_gpt.cpp` | 2 | cited |
| `tests/unit/test_destio_win.cpp` | 1 | cited |
| `tests/unit/test_dict.cpp` | 0 | covered: N-064 FIXED; API-DELTA row 43 |
| `tests/unit/test_directalign.cpp` | 0 | covered: rawfixer.md:77 (cr_directalign.h mandate) |
| `tests/unit/test_diskmap.cpp` | 0 | covered: visualization.md S-009; cr_diskmap.cpp cited x3 |
| `tests/unit/test_driver_status.cpp` | 0 | covered: cr_driver_status.h cited x7 (R-84 6-facet surface) |
| `tests/unit/test_enumerate_merge.cpp` | 0 | covered: cr_enumerate.cpp cited x7; N-071 census |
| `tests/unit/test_enumerate_win.cpp` | 0 | covered: cr_enumerate.cpp cited x7; N-071 census |
| `tests/unit/test_exfat_volume.cpp` | 1 | cited |
| `tests/unit/test_ext4_volume.cpp` | 4 | cited |
| `tests/unit/test_fat_volume.cpp` | 2 | cited |
| `tests/unit/test_fixtures.hpp` | 3 | cited |
| `tests/unit/test_fixup.cpp` | 0 | covered: cr_fixup.cpp cited x2 (ntfs-viability:81) |
| `tests/unit/test_fscheck.cpp` | 0 | covered: modules/check-phase.md; M-041 |
| `tests/unit/test_grading.cpp` | 1 | cited |
| `tests/unit/test_health_csmi.cpp` | 0 | covered: health.md:171 CSMI MATCH |
| `tests/unit/test_health_grade.cpp` | 0 | covered: cr_grading.cpp cited x6; health.md B15/D16 |
| `tests/unit/test_hfsplus_volume.cpp` | 3 | cited |
| `tests/unit/test_image.cpp` | 0 | covered: modules/imaging.md P1; cr_image.cpp cited x4 |
| `tests/unit/test_ldm.cpp` | 9 | cited |
| `tests/unit/test_ldm_recovery.cpp` | 1 | cited |
| `tests/unit/test_lostpart.cpp` | 1 | cited |
| `tests/unit/test_node_tree.cpp` | 0 | covered: cr_node.cpp cited x4; query/tree modules |
| `tests/unit/test_ntfs_volume.cpp` | 5 | cited |
| `tests/unit/test_nvme_health.cpp` | 1 | cited |
| `tests/unit/test_nvramfix.cpp` | 2 | cited |
| `tests/unit/test_orchestrator.cpp` | 3 | cited |
| `tests/unit/test_parallel.cpp` | 1 | cited |
| `tests/unit/test_pause_resume.cpp` | 1 | cited |
| `tests/unit/test_preview.cpp` | 6 | cited |
| `tests/unit/test_query.cpp` | 1 | cited |
| `tests/unit/test_query_perf.cpp` | 2 | cited |
| `tests/unit/test_rawfixer.cpp` | 2 | cited |
| `tests/unit/test_refs_ldm.cpp` | 2 | cited |
| `tests/unit/test_refs_volume.cpp` | 2 | cited |
| `tests/unit/test_reghive.cpp` | 2 | cited |
| `tests/unit/test_report.cpp` | 1 | cited |
| `tests/unit/test_resume_revalidation.cpp` | 2 | cited |
| `tests/unit/test_retry.cpp` | 0 | covered: cr_retry.cpp cited x1 (storage) |
| `tests/unit/test_runlist.cpp` | 0 | covered: cr_runlist.cpp cited x2 |
| `tests/unit/test_scan_truth.cpp` | 0 | covered: orchestration module; cr_scan.cpp cited x68 |
| `tests/unit/test_session.cpp` | 5 | cited |
| `tests/unit/test_sha256.cpp` | 0 | covered: storage.md:77; sha lineage (S-032-era) |
| `tests/unit/test_slice_devmap.cpp` | 0 | covered: cr_slice.cpp x3 + cr_devmap.cpp x2 (storage) |
| `tests/unit/test_snapshot.cpp` | 0 | covered: cr_snapshot.cpp cited (s11-board-truth:60) |
| `tests/unit/test_telemetry.cpp` | 0 | covered: orchestration.md O39/N-016/N-075/D-78 (G-04..06 progress) |
| `tests/unit/test_text.cpp` | 0 | covered: S-005/M-010/N-029 codec lineage (cr_text.h symbol-cited) |
| `tests/unit/test_thumbnail.cpp` | 9 | cited |
| `tests/unit/test_tmpdir.hpp` | 0 | test-infra helper (no product surface) |
| `tests/unit/test_transport_logic.cpp` | 0 | covered: modules/transport.md; cr_transport.cpp cited x29 |
| `tests/unit/test_vollabel.cpp` | 0 | covered: N-133 FIXED (cr_vollabel.cpp) |
| `tests/unit/test_writer_dirs.cpp` | 1 | cited |
| `tests/unit/test_zfs_volume.cpp` | 1 | cited |

### P1.3 The uncited set — per-file dispositions

Every file below has **zero exact-name citations** in `diff-to-fix/`. Each entry: what the file implements (read from its header/top at HEAD), its module doc, and the disposition state of its surface. Files are grouped by tree; within a group the format is *file — implementation; module; disposition*.

**src/ (11 files).**

- `src/core/bcdfix/cr_bcdfix.cpp` — the BCD partition-offset rewriter ("M-040 stage a" per its header): the corpus's 0xffffffa0-cell decode, `new_lba << 9` rewrite, plan/execute/undo over an injected writer. Module: boot-repair. **Stem-cited** — M-113:9/:25 names "cr_bcdfix" twice (as the contrast class), and the surface is fully dispositioned by M-040 (FIXED-LEDGER row, commit 94c9dda) + `modules/boot-repair.md:9` (P25 MATCH, "stages a+b landed"). No gap.
- `src/core/btrfs/cr_btrfs_cache.cpp` — the Btrfs re-scan metadata cache codec (header: "N-072"): fsid-keyed, size-bounded warm-scan cache, generation-gated match-or-discard. Module: btrfs. **Uncited** (the `.h` is the cited twin: `modules/btrfs.md:158` "answered by `cr_btrfs_cache.h`" — P47/P48 re-scan throughput MATCH, format registered as ours). No gap.
- `src/core/carve/cr_usersigns.cpp` — the user-extensible signature database loader (header: "M-048"): JSON parse of EUUserData-format user signs under the S-039 caps. Module: carve. **Uncited** (`.h` cited: S-039's caps at `cr_usersigns.h:40-47` via `evidence/s10-reverification-sweep.md:119`; `SUMMARY:1561`). No gap.
- `src/core/health/cr_csmi.cpp` — the CSMI CC-0x19 ATA pass-through wire format (pure core; the DiskHealth FUN_00425450 decode). Module: health. **Stem-cited** — `SUMMARY:3011` "module (cr_csmi) + the pure transport-chain seam", `s11-triage-sweep:70`; behavior dispositioned `modules/health.md:171` (F60-adjacent CSMI route MATCH, `Health.CsmiRequestMatchesOgWireFormat`). No gap.
- `src/core/health/cr_family.cpp` — vendor family classification for health grading (header: "S-024"; the FUN_0040dad0 model-string cascade, WDC/Seagate/…/Kioxia/Apacer + NVMe + fallbacks, exotic-vendor residual honestly declared). Module: health. **The only src file with zero citations in any form** — but the surface is dispositioned twice: `modules/health.md:108` (D16-vendor classification "MATCH (decoded set)" enumerating this exact cascade + the exotic-thunk residual) and S-024's FIXED-LEDGER row (commit 00458350). Behavior covered; only the file name is dark. No gap.
- `src/core/health/cr_smart_ata.cpp` — SMART read flow over the transport chain (pure; the 30x12 attribute parse + model-string + grading pass moved from the win layer, "N-032"). Module: health. **Stem-cited** — `modules/health.md:157`/`:172` name the seam `cr::health::read_smart_ata` (P42 transport-chain contract MATCH); `SUMMARY:3011`. No gap.
- `src/core/lost/cr_consistency.cpp` — the lost-partition per-candidate consistency walk ("considered (non-directory)" checks). Module: lost-partition. **Stem-cited** — `modules/lost-partition.md:230` "(cr_consistency.h/.cpp): per walked candidate file" names both files. No gap.
- `src/core/recovery/cr_dict.cpp` — the password-dictionary unlock aid (header: "N-064"): line/CR/comma grammar parse + the 228-entry pool consumer for consented BitLocker dictionary unlocks. Module: writer (recovery). **Uncited** (all three `cr_dict` stem hits cite `cr_dict_data.cpp`: API-DELTA:109, I-033:7, s11r:86). Surface dispositioned: N-064's FIXED-LEDGER row (commit 97382b7) + API-DELTA row 43 (Dict 228-entry password dictionary, "shipped") + I-033's byte-identity register. No gap.
- `src/core/recovery/cr_report.cpp` — the recovery report JSON serializer (header: "G-09, R7.3"): schema_version-1 report, per-item outcomes, byte-safe escaping. Module: writer. **Uncited** (all `cr_report` stem hits cite `cr_report.h`: S3-REFS:29, s11-rawsearchdll-infra-walk:151/:193). Surface dispositioned: `modules/writer.md` D13 row ("report schema — M: … validated by CI (`validate_report.py`)") + the module's Core line cites `src/core/recovery/` + `test_report.cpp`/`tools/validate_report.py` are themselves cited. No gap.
- `src/core/storage/cr_decrypt_device.cpp` — the in-stack XTS decrypting device (header: "M-033"): `DecryptingDevice : IStorageDevice`, per-sector LE-index tweaks, whole-sector staging ≤2 sectors. Module: storage. **Uncited** (the `.h` is cited ×3: `modules/storage.md:22`, CROSS-REF-MATRIX:480, SUMMARY:3978). Surface fully dispositioned by M-033 (FIXED-LEDGER row, commit 180042350). No gap.
- `src/win/cr_elevate.cpp` — the definitive process-elevation probe (header: "GAP-P1-5"; D-60a non-elevated-run support). Module: storage/win. **Stem-cited** — `modules/storage.md:74` ("cr_elevate's probe is a byte-shape…"), the session-6 first-time-cited register (`SUMMARY:4970/:5093/:5111`), M-105:19 (`include/cr/win/cr_elevate.h:19-23`) and M-105:68 (fix-plan option (a) rides the cr_elevate seam). No gap.

**include/ (24 files).**

- `cr_bcdfix.h` — BCD-cell patch contract (plan/apply/undo + consent gate). **Stem-cited** (M-113:9); surface = boot-repair.md:9 "cr/bcdfix" P25. No gap.
- `cr_btrfs_engine.h` — the Btrfs engine Volume/ScanStats types. **Uncited by name; directory-cited** (`modules/btrfs.md:8` `include/cr/btrfs/`), the .cpp cited ×4. No gap.
- `cr_decomp.h` — MS-XCA LZNT1/XPRESS decompression contract. **Uncited by name; .cpp cited** (`modules/storage.md:76` N-058 LZNT1 parity; `modules/ntfs.md:323` "cr_decomp.cpp:131"; `SUMMARY:4970`). No gap.
- `cr_bits.h` — the shared byte-order readers (le16/le32/le64 delegating to cr_types, be16/be32/be64; header: "W4 dedup (AUDIT-LINUX-FS DC-12): seven engines carried private near-identical copies"). **Zero citations in any form; no diff-to-fix disposition exists** — the only disposition is the core-side `AUDIT-LINUX-FS.md` DC-12 note. See P1.4: judged a **non-gap** (pure utility with no og-parity behavior of its own; every consumer engine is dispositioned).
- `cr_parallel.h` — the `cr::ThreadPool` contract (plain MPMC FIFO). **Uncited by name; .cpp cited** — CORPUS-ERRATA:112-113 explicitly dispositions the pool ("plain MPMC FIFO, NOT work-stealing; treat as unverified"). No gap.
- `cr_text.h` — the shared Unicode codecs: `utf8_to_u16` (RFC 3629 maximal-subpart U+FFFD policy, S-005/N-029a), `utf8_to_u16_lossless` (U+E000+byte private-use mapping, M-010, ext4/Btrfs), `lossless_u16_to_utf8`, `name_has_raw_band` (N-016b), `wide_to_utf8` (N-009c). **Uncited by filename; symbol-cited as `text::utf8_to_u16`** in `modules/ext4.md:31`, `modules/btrfs.md:33`, `modules/zfs.md:35`, `SUMMARY:4480`, `evidence/decompiled-quotes/s31-oq-s2m-2...md:203`. No gap.
- `cr_exfat_engine.h` / `cr_exfat_wire.h` — exFAT Volume/engine stats + on-wire structures. **Uncited by name; directory-cited** (`modules/exfat.md:10` `include/cr/exfat/`). No gap.
- `cr_fat_engine.h` — FAT Volume/engine types. **Uncited by name; directory-cited** (`modules/fat.md:8` `include/cr/fat/`). No gap.
- `cr_aes.h` — AES-128/256 + XTS contract (FIPS-197 KAT-pinned). **Uncited by name; .cpp cited** (`modules/storage.md:22`, CROSS-REF-MATRIX:480, M-033). No gap.
- `cr_sha256.h` — SHA-256 contract. **Stem-cited** (`modules/storage.md:77` "cr_sha256 is a deliberate core addition (carve ids + writer verify — D-19 side registered)"; `SUMMARY:4970`). No gap.
- `cr_nvme.h` — NVMe health-page parser types. **Uncited by name; .cpp cited** (`modules/health.md:6`/:93). No gap.
- `cr_smart_ata.h` — the pure read_smart_ata seam contract. **Stem-cited** (`modules/health.md:172`). No gap.
- `cr_attributes.h` / `cr_fixup.h` / `cr_runlist.h` — NTFS $DATA attribute walk / update-array fixup / runlist decode contracts. **Uncited by name; directory-cited** (`modules/ntfs.md:10` `include/cr/ntfs/ (7 headers)`); the .cpp twins are individually cited (`cr_fixup.cpp` ×2, `cr_runlist.cpp` ×2, `cr_attributes.cpp` ×1). No gap.
- `cr_nvramfix.h` — the UEFI NVRAM Boot####/BootOrder updater contract. **Uncited by name; unit-cited** (`modules/boot-repair.md:10` `cr/nvramfix` P42 MATCH row; N-122's row). No gap.
- `cr_vollabel.h` — the volume-label probe contract (family detection + four decoders). **Uncited by name; .cpp cited** (N-133 Resolution names `cr_vollabel.cpp`). No gap.
- `cr_dict.h` — the dictionary parse/unlock-aid API (kDictEntryCount, kMaxPasswordLen). **Uncited** (stem hits all name `cr_dict_data.cpp`); N-064's ledger row dispositions the surface. No gap.
- `cr_rawfixer.h` — the RawFixer contract (CheckDBR/FixFromDBR/CheckMFT/FAT mirror restores, S-008 consent). **Uncited by name; surface covered by `modules/rawfixer.md`** (its Core line cites the .cpp + tests; the .cpp itself is cited ×10). No gap.
- `cr_freemap.h` — `storage::FreeMap`: cluster-granular free/used map (S-009 engine side of IBuildVolumeBitmap; bit=free, out-of-range≠free). **Uncited by filename; the TYPE is cited** — `modules/visualization.md:85` "storage::FreeMap produced by the engines…" (B1 bitmap-surface MATCH), `modules/abi.md:74`, `API-DELTA.md:74` row 23. No gap.
- `cr_snapshot.h` — the scan snapshot carrier (error map + free map under the controller lock). **Stem-cited** (`SUMMARY:4970` register; `s11-board-truth:60` cites cr_snapshot.cpp; `visualization.md:87` describes the cr_ui snapshot surface it feeds). No gap.
- `cr_bootfix.h` — the win-layer BCD file-repair contract (bcd_file_plan/apply/execute/undo). **Uncited by name; .cpp cited ×9** (boot-repair.md, N-122, M-113, SUMMARY:1855/:2078). No gap.
- `cr_destio.h` — `WinDestIo` write/mkdir win-side destination I/O. **Uncited by name; .cpp cited** (`modules/writer.md:59`/:101, `s11-triage-sweep:33`, `s10-reverification-sweep:71`, `s6-writer-collision-and-mkdir:61`). No gap.

**tests/unit/ (27 files).** Each uncited test exercises a surface the trail has dispositioned; the test file itself is never named. One-line dispositions: `test_bcdfix.cpp`→boot-repair P25/M-041-era stages; `test_cached.cpp`→storage module (cr_cached.cpp cited, SUMMARY:5093 register); `test_config_display.cpp`→N-132 FIXED; `test_consistency.cpp`→lost-partition.md:230; `test_dict.cpp`→N-064/API-DELTA row 43; `test_directalign.cpp`→`modules/rawfixer.md:77`; `test_diskmap.cpp`→visualization.md S-009; `test_driver_status.cpp`→R-84 facet surface (cr_driver_status.h cited ×7); `test_enumerate_merge.cpp`/`test_enumerate_win.cpp`→N-071 + cr_enumerate.cpp (cited ×7); `test_fixup.cpp`→ntfs-viability:81; `test_fscheck.cpp`→check-phase module (M-041); `test_health_csmi.cpp`→health.md:171; `test_health_grade.cpp`→health.md B15/D16 (cr_grading.cpp cited ×6); `test_image.cpp`→imaging module P1; `test_node_tree.cpp`→query/tree (cr_node.cpp cited ×4); `test_retry.cpp`→storage retry (cr_retry.cpp cited); `test_runlist.cpp`→cr_runlist.cpp (cited ×2); `test_scan_truth.cpp`→orchestration (cr_scan.cpp cited ×68, the doc-truth gates); `test_sha256.cpp`→storage.md:77; `test_slice_devmap.cpp`→cr_slice/cr_devmap (cited ×3/×2); `test_snapshot.cpp`→s11-board-truth:60; `test_telemetry.cpp`→the G-04/G-05/G-06 per-stage progress/permille/elapsed/error-map/TSan-telemetry contract, dispositioned by orchestration.md's O39/N-016/N-075/D-78 rows; `test_text.cpp`→the S-005/M-010/N-029 codec contract (cr_text.h symbol-cited); `test_tmpdir.hpp`→test-infra helper (no product surface); `test_transport_logic.cpp`→transport module (cr_transport.cpp cited ×29); `test_vollabel.cpp`→N-133 FIXED.

### P1.4 The genuine-gap verdict

**Zero genuine behavioral gaps found.** Every one of the 62 uncited files maps to a surface that a module doc, register, or FIXED-LEDGER row dispositions. The sweep's residual notes (for the parent lane, not finding candidates on the behavior axis):

1. **`include/cr/cr_bits.h`** — the only core file with neither a file citation nor a diff-to-fix disposition (its dedup rationale lives in the core-side `AUDIT-LINUX-FS.md` DC-12 note, outside the audit trail). Assessed a **non-gap**: pure byte-order utility, no og-parity behavior of its own, and every consuming engine is dispositioned. If the parent lane wants a zero-cost bookkeeping row, an I-register line naming it (the I-031/I-034 precedent) would close the navigability hole.
2. **`src/core/health/cr_family.cpp`** — the only src file uncited in ANY form; its behavior is fully dispositioned (health.md:108 D16). Same optional-register note.
3. **`cr_dict.cpp` / `cr_report.cpp`** — src files whose citations all land on their data/header siblings (`cr_dict_data.cpp`, `cr_report.h`). Covered, but a future grep-by-basename will re-derive this — the register note covers them too.
4. **The 27 uncited test files** — all test covered surfaces; `test_tmpdir.hpp` is the only one with no product surface at all. The session-6 register precedent ("first-time-cited core files") is the right landing spot if the lane wants them enumerated.

**Comparison to the last sweep (session 6):** the trail's coverage has *improved* since — the 8 files session-6 first-cited are now cited or stem-cited, and no new file class appeared since. The board's mark-all-remaining posture on the core tree is: **no never-dispositioned core surface remains.**

---

## Part 2 — the active-board liveness re-triage (remove-the-fixed)

### P2.0 Method

For each of the 49 active findings: read Summary + "What our core does" + Gap, then re-run the finding's own falsifiable check at HEAD (`869da779`-code = HEAD `d10b893a` minus worklog-only files — verified by `git diff --stat 869da779 HEAD`). Verdict scale: **LIVE** (the gap holds verbatim at HEAD) · **PARTIALLY-FIXED** (a later landing repaired part; re-scope candidate per the trail convention) · **STALE** (fully fixed; retire candidate) · **LIVE-BY-HANDOFF** (the defect lives in the excluded rda-driver repo, absent from this workspace; the trail's OQ-S10-3 register still lists it OPEN — unverifiable here, marked uncertain). Every verdict cites a grep or code line I actually ran/read this session.

### P2.1 The 49-row verdict table

**SEVERE (3)**

| ID | Verdict | Evidence (one line) |
|---|---|---|
| S-040 | **LIVE** | `rg "image_create\|create_image\|cr_ui_image" include/ src/` → 0 hits; `cr_image_win.cpp:37` still `GENERIC_READ` reader-only; rrecov usage block (rrecov.cpp:10-28) has no image verb. |
| S-048 | **LIVE** | `rg "Reconstructed" src/ include/` → 0 (only cr_scan.cpp:1565's LDM "reconstructed volume" comment); single `root_carved_` at cr_scan.cpp:206/247-248 fed to `carve_slice` at :2182; S-049's Resolution (d4b487c8) names only the naming contract — no per-volume grouping landed. |
| S-050 | **LIVE** | `src/core/repair/` holds only cr_rawfixer.cpp; the only EUVideoRepair hits are the M-021 hook comments (cr_carve.h:98/:107); CAPABILITY-MATRIX §3 (docs :77-92) still lists only Cloud/NAS, BitLocker FVEK-derivation, signing, .rsf import — no repair row. |

**MAJOR (21)**

| ID | Verdict | Evidence (one line) |
|---|---|---|
| M-073 | **LIVE** | `rg "filter_folder" include/cr/orchestrator/cr_scan.h include/cr/cr_ui.h src/core/orchestrator/cr_scan.cpp src/app/rrecov.cpp` → 0 hits; ScanConfig still has no caller path filter. |
| M-075 | **LIVE** | `cr_ui_fsck_volume` absent; the only CreateProcess in src/win is cr_bitlocker.cpp:62 (the manage-bde/PowerShell unlock seam); cr_transport.cpp:386-410 remains an OpenSCManagerW(CONNECT) status query; the chkdsk mentions in code are comments only (cr_bcdfix.h:36). |
| M-076 | **LIVE** | `cr_ui_disk_info` (cr_ui.h:449-460) still has no TRIM fields; the only trim logic is the health facet fill (cr_ui.cpp:4472-4475, filling cr_ui_smart_report); cr_enumerate.cpp has zero TRIM; no `cr_ui_enumerate_disks_ex`. |
| M-078 | **LIVE** | `rg -i "turn_off\|turnoff" src/win/cr_bitlocker.* src/ui/cr_ui.cpp` → 0; no `cr_ui_turn_off_bitlocker` in the 86-CR_UI_API-line census; cr_bitlocker.cpp unchanged surface (unlock/dictionary/auto-unlock only). |
| M-086 | **LIVE-BY-HANDOFF** | Driver-repo-resident (rda_version.h/install.ps1/release.yml — not in this workspace); OQ-S10-3 (diff-to-fix/tracked-open/) still lists it OPEN with the fix lane queued. |
| M-087 | **LIVE-BY-HANDOFF** | Driver-repo-resident (scripts/winpe/regdrv-rda.reg); OQ-S10-3 still OPEN. |
| M-089 | **LIVE-BY-HANDOFF** | Driver-repo-resident (src/bench/rdabench.c:390); OQ-S10-3 still OPEN. |
| M-098 | **LIVE** | `rg -i "hik\|hikvision\|private_stream" src/core/carve/` → 0 hits; no HIK analyser/registry row. |
| M-099 | **LIVE** | `rg -i "rifx\|xpacket" src/core/carve/` → 0 hits; no AEP/RIFX probe. |
| M-101 | **LIVE** | `rg -i "versions node\|tags extracted\|multi.version" src/ include/` → 0 hits; no Versions/Tags-Extracted branch concept. |
| M-103 | **LIVE** | `rg "File Path Lost" src/ include/ tests/` → 0; root still `u"Lost Files"` (cr_scan.cpp:203/:245); LostDir appears only in decode comments (cr_node.h:177, cr_exfat_quick.cpp:802, test_exfat_volume.cpp:185) — no LostDir<NNNN> construction. |
| M-105 | **LIVE** | `CreateService` code only at .github/scripts/Install-ReleaseDriver.ps1 (the RDA driver); no SERVICE_AUTO_START/"Read FVE" code; cr_transport.cpp:410 OpenSCManagerW(CONNECT) query only. |
| M-106 | **LIVE** | cr_analysers.cpp:5517-5518 (ONE) and :5607-5608 (3DS) still carry "DWG-pattern bounded extent stands in"; `64 << 20` caps at :5530/:5616/:5625/:5647. |
| M-107 | **LIVE** | 15 "stands in"/"recorded follow-up" comment sites remain in cr_analysers.cpp (`rg -c "bounded extent stands in\|recorded follow-up"` = 15); no per-format walk landed. |
| M-111 | **LIVE** | `rg -i "cwindowsfs\|explorerpartition\|createexplorerpartitionobject" src/ include/` → 0 hits; no live-FS browse seam. |
| M-112 | **LIVE** | `0x44c` exists only in the residual comment (cr_analysers.cpp:3631-3632); validate_xmlsvg's size assignment unchanged (`</svg>` walk / 1 MiB cap). |
| M-113 | **LIVE** | `rg "TmpBCD\|TempSystem\|TmpSoftWare\|bootstat\|9dea862c\|23000003" src/ include/ docs/` → 0 hits; cr_bootfix.cpp remains BCD-cell + NVRAM only. |
| M-114 | **LIVE** | `rg -i "srclient\|SRRemoveRestorePoint\|SystemRestore" src/ include/ tests/` → 0 hits; apply chain still zero host-environment side-effects. |
| M-115 | **LIVE** | cr_ui.cpp:3925 returns on any non-Ok thumb status (DecodeFailed→ERR_TRUNCATED at :3865-3866) and the promotion fires only on the Ok path (:3937-3938 `set_grade(High)`); still decode-gated, no read-verdict/text-snapshot/frame triggers. |
| M-116 | **LIVE** | `cr_ui_metadata` (cr_ui.h) still has NO entries field; preview_metadata_impl (cr_ui.cpp:3784-3802) never touches `m.entries`; engine half intact (cr_preview.h:134/:191); no archive ABI export (86 CR_UI_API lines, none archive-listing); no CLI archive verb (rrecov "archive" grep = 1 category-comment). |
| M-117 | **LIVE** | `rg -i "dbx\|0x36464d4a\|0xfe12adcf" src/core/carve/ include/cr/carve/` → 0 hits; no Dbx row/validator/sign entry. |

**MINOR (17)**

| ID | Verdict | Evidence (one line) |
|---|---|---|
| N-090 | **LIVE** | Session list still bare `*.crsf` (cr_ui.cpp:382-394, no `auto_` filter); row shape still `label[32]` (cr_ui.h:856/:919) + `save_unix`; `rg "device_type\|deviceType" include/cr/cr_ui.h` → 0. |
| N-107 | **LIVE-BY-HANDOFF** | Driver-repo-resident (14 stale prose sites in rda-driver); OQ-S10-3 still OPEN; the core repo's DRIVER-CONTRACT.md already states 19 (per the finding). |
| N-125 | **LIVE** | `rg -i "disconnect\|reattach\|ERR_DEV_GONE\|ERR_DISCONNECT" src/ include/` → 0 code hits (cr_error.h carries no such code). |
| N-126 | **LIVE** | `rg -i "unsaved" src/ include/` → 0 hits; no Unsaved-File category axis. |
| N-127 | **LIVE-BY-HANDOFF** | Driver-repo-resident (rda_ioctl.h/docs claims); OQ-S10-3 still OPEN. |
| N-131 | **LIVE** | `rg "parent_anchor\|parent_reference\|parent_frn" src/ include/` → 0; cr_records.cpp:181 still persists only `put32(out, parent_ids[i])`. |
| N-135 | **PARTIALLY-FIXED** | Items 1/3/4 repaired by the session-39 truth wave (REFERENCE-API-GAP:376 re-worded; FIXED-LEDGER header:6 + M-090 row :184 carry the repaired SHAs, "repaired session 39" in-row); item 5 repaired (ledger rows M-080:175/M-082:183/S-043:335/M-085:340 now exist; FIXED/ holds no re-created files); items 2/6/7 STILL LIVE (see P2.3). |
| N-139 | **LIVE** | cr_carve.cpp:1036 still the throwing `std::vector<u8> win(want, 0)` with no upstream catch (`rg -c "catch\|bad_alloc" src/app/rrecov.cpp` → 0) while the nothrow sites remain (:538/:648/:762); cr_carve.cpp's include list has no logging header. |
| N-140 | **LIVE** | CarveStats (cr_carve.h) still 13 scalar counters, no per-format map; `rg "per_format\|format_counts\|per_extension" include/ src/` → 0. |
| N-143 | **LIVE** | `rg -i "ispecial\|two.step" src/core/carve/` → 0; the trial loop still runs one validator per candidate with outright reject. |
| N-144 | **LIVE** | No text-snapshot render anywhere; the only TxtView code is the M-108 preview-eligibility gate (cr_preview.h:147-168, kTxtViewPool); cr_thumbnail.h's v1 matrix carries no text family. |
| N-145 | **LIVE** | `rg "pendingMarkedDisk\|rwConfig\|QuickPartition" src/ include/` → 0 hits; CRUNDO01 has no cross-run marker. |
| N-146 | **LIVE** | cr_analysers.cpp:4353-4379 unchanged: "og returns a CZeroEndRawFileInfo … no size grammar - bounded extent" + `end = hit + (32ull << 20)`; still absent from M-106/M-107's site lists and modules/carve.md's S-037 row. |
| N-148 | **LIVE** | cr_ldm.h:31 still "UNDECODED"; cr_zfs_engine.h:58 comment unchanged; cr_ui.cpp:3934 still cites ":422498-518"; EPS/AI still `16 << 20` (cr_analysers.cpp:5379/:5400) vs registry `64ull << 20` rows; cr_grading.cpp:10 still points at the purged S-023 Resolution; cr_refs_engine.cpp:1254 still `(void)og_prop_flags` (unconsumed). |
| N-149 | **LIVE** | `rg -i "exempt\|0x1000801" src/core/carve/` → 0; carve_token_listed (cr_registry.cpp) disables any listed format; N-115's landing (67de3276) touched only the typing pass + ZIP inner-LFH (stat-checked) — no filter semantics. |
| N-150 | **LIVE** | API.md:92 "81 exported functions"; CORE-API-CURRENT:16 "error codes **34 + OK**"; API-SURFACE-AUDIT:32 "81 exports"; REFERENCE-API-GAP:345 "PRESENT 41 · PARTIAL 15 · GAP 29" + :322/:426 stale rda_available rows + :236 R-37 "GAP"; my own census confirms 38 distinct 0x5-codes in cr_error.h vs the "34" claim. |
| N-152 | **LIVE** | `rg "0x3939\|\"999\"" src/core/carve/` → 0 hits; validate_dxf (cr_analysers.cpp:5410-5422) still `memcmp(h, "  0\r\nSECTION", 8)` at offset 0 with no leading-comment branch; `git log --all --oneline -- src/core/carve` post-S41 landings (3acdbf97, d7ba41f6, d4b487c8, 67de3276) — none DXF. |

**INFORMATIONAL (8)**

| ID | Verdict | Evidence (one line) |
|---|---|---|
| I-026 | **LIVE (register accurate)** | docs/DRIVER-RELEASE.md:63/:180 still carry SignMode=Off/unsigned-by-design; the devsign execution evidence (run 40549660074) remains in the trail. |
| I-027 | **LIVE (register accurate)** | The five og data-asset ground truths remain absent from core and mapped to registered exclusions; no superseding register filed. |
| I-028 | **LIVE (register accurate)** | The registered superset claims still match the header: straight-alpha RGBA8 (cr_thumbnail.h:24), PSD/PNM/HDR (:42); no retirement condition triggered. |
| I-029 | **LIVE** | docs/DRIVER-CONTRACT.md:131 "EUDCPEPM contract — 19 recognized codes" + :202 "preserved byte-for-byte … (19 codes)" — the corpus-grounding overstatement for the 4 core-wire codes stands. |
| I-031 | **LIVE** | The register remains the ONLY carrier of its four boundary details — PARITY-DELTAS' D-101 row (:2480, read in full) carries no pending-16/oldest-first/Stop-flush text; the fix plan's doc addenda never landed. |
| I-032 | **LIVE (register accurate)** | The refs landing after filing (e10ec381 = S-053+N-141) is exactly the register's rows 4/5 predicted; the zfs rows untouched (no zfs behavioral landing since). |
| I-033 | **LIVE** | Its optional doc-row applications did NOT land: docs/PARITY-OG-CORE-INVENTORY.md:98 still reads "needcopy.txt | INSTALLER | File-copy manifest | none | skip" (the entry-2 AliyunWrap correction absent). |
| I-034 | **LIVE (register accurate)** | The 23-artifact register stands; its optional hygiene item (SURFACE-INVENTORY basename list) un-landed; no superseding census filed. |

**Tally: 43 LIVE · 5 LIVE-BY-HANDOFF · 1 PARTIALLY-FIXED (N-135) · 0 STALE.** No finding on the active board is a retire candidate at HEAD; N-135 is the one re-scope candidate (P2.3).

### P2.2 The four special cases, handled as instructed

1. **M-116 (archive-inventory ABI half):** no later landing added the entries-surface. Checked: (a) `cr_ui_metadata` at include/cr/cr_ui.h — still kind/format/dims/orientation/datetime/duration_ms/codec/text_snippet/truncated, no entries field; (b) `preview_metadata_impl` (cr_ui.cpp:3784-3802) read in full — copies every Metadata member except `m.entries`; (c) the ABI export census (86 `CR_UI_API` lines) — no archive-listing export; (d) the CLI — no archive verb (the only "archive" hit in rrecov.cpp is the category-count comment :1880); (e) the N-115/N-153 landing (67de3276, the only post-filing carve-adjacent landing) stat-checked — typing pass + ZIP inner-LFH only. **Verdict: LIVE.**
2. **N-149 (carve filter exempt set):** N-115's landing did NOT touch it. The 67de3276 diff touches cr_signatures.h (+27), cr_analysers.cpp (+140), cr_carve.cpp (+5 — the N-153 inner-LFH marker), cr_registry.cpp (+46 — override-target rows sharing the LFH key), test_carve.cpp (+230); `carve_token_listed` (cr_registry.cpp) still disables any listed ext/name with no exempt mask (`rg -i "exempt|0x1000801" src/core/carve/` → 0). **Verdict: LIVE.**
3. **S-048 (carve tree-shape Reconstructed/<volume>):** neither the S-049 landing nor its tree application landed any part of the shape. S-049's Resolution (in findings/FIXED/S-049…md:79+) describes only the naming contract (per-format counters, uppercase-ext prefixes, content-derived families, the N-063 template supersession) — no volume grouping; at HEAD `rg "Reconstructed" src/ include/` → 0 hits, `root_carved_` remains ONE global root (cr_scan.cpp:206/:247-248) passed to `carve_slice` at :2182, and cr_carve.cpp contains no per-volume node construction (its "per-volume" hits are the S-051 whole-disk display-naming and bitmap contexts). **Verdict: LIVE.**
4. **N-152 (DXF 999 key):** the 0x3939 key has NOT landed. `rg "0x3939|\"999\"" src/core/carve/` → 0 hits; validate_dxf (cr_analysers.cpp:5410-5422) still matches only `"  0\r\nSECTION"` at offset 0; `git log --all --oneline -- src/core/carve` shows the post-S41 landings (3acdbf97 nested-declaration N-150→N-151, d7ba41f6 S-054 TS/M2TS cap + the pre-validator quadratic repair, d4b487c8 S-049, 67de3276 N-115+N-153) — no DXF work in any of them. **Verdict: LIVE.**

### P2.3 Notes beyond the one-liners

- **N-135 (the only PARTIALLY-FIXED)** — item status at HEAD, first-hand: item 1 (the missed R-28 site REFERENCE-API-GAP:376) — the row now carries the updated thumbnail text (the b5da834 session-39 repair the finding's own NOTE records); item 3 (phantom SHAs) — the ledger header (:6) and the M-090 row (:184) now carry the real SHAs with in-row "repaired session 39" notes; item 4 (the `180053de0` FUN_-mis-paste) — same row, repaired; item 5 — the M-080/M-082/M-085/S-043 ledger rows now exist (:175/:183/:335/:340) and findings/FIXED/ holds none of the re-created files. **Still live:** item 2 (the R-31 self-contradiction: REFERENCE-API-GAP:379 still "PARTIAL … per-format carve toggles, usefullostfiles whitelist, display priority missing" vs :225 "PRESENT"); item 6 (ABI-CONTRACT.md:543 still cites "(cr_ui.h:916-944)" — at HEAD that range falls inside `cr_ui_session_row`, so the stale-range defect persists in drifted form); item 7 (cr_thumbnail.cpp's file-top matrix at :8-16 still lists only uncompressed/PackBits/Deflate, no LZW, vs the header's LZW-inclusive matrix). Recommended re-scope: strike items 1/3/4/5, keep 2/6/7.
- **S-050 (LIVE, with the file's own re-scope)** — the finding's :63 addendum already records that Leg 2 (the false SURFACE-INVENTORY registration) was repaired in its own session (`ea2f3ea7`) and the board row was re-scoped; the capability half (Leg 1) is fully live at HEAD and the formal CAPABILITY-MATRIX §3 disposition row is still owed. No action needed by the board beyond what the addendum already encodes.
- **N-148** — one finding-side defect found during verification: the finding's item-5 cite "src/core/ntfs/cr_lfn.cpp:119-120" is path-wrong at HEAD — cr_lfn.cpp lives at `src/core/fat/cr_lfn.cpp` (the N-148 substance — a "(diff-to-fix)" pointer at a purged finding — is verifiable there). The parent lane may want to correct the path when it next touches the file.
- **I-031/I-033** — both registers' optional fix-plan outputs (the D-78/D-101/I-009 boundary addenda; the PARITY-OG-CORE-INVENTORY row corrections) remain un-landed; the registers are therefore still the only carriers of their data — i.e. still LIVE as records, with a doc-lane follow-up available.
- **The driver quintet (M-086/M-087/M-089/N-107/N-127)** — all five are PUSHED-FORWARD by their own Status lines and by OQ-S10-3 (diff-to-fix/tracked-open/OQ-S10-3-driver-repo-resident-fixes.md, read this session); the rda-driver repo is absent from this workspace so no HEAD-level verification was possible. Marked **uncertain/live-by-handoff** rather than verified-LIVE.

### P2.4 Rigor + uncertainty register

- Every LIVE verdict above cites a grep or file read executed this session against the `869da779`-code tree (HEAD `d10b893a` = worklog-only delta, verified by `git diff --stat`). No verdict relies on a trail claim alone; where a trail claim was the only prior evidence (e.g. S-049's Resolution text for S-048), the HEAD-side negative was re-established first-hand (the `Reconstructed`/root_carved_ greps).
- Line-number drift is pervasive (cr_analysers.cpp grew ~700 lines across S-049/N-115; cr_ui.cpp grew with N-133/S-052): every cite above was re-located at HEAD rather than trusted from the finding file.
- Uncertain items, explicitly: the 5 LIVE-BY-HANDOFF driver-repo rows (no workspace copy to verify); I-027/I-028's "no superseding register" negatives (established by grep of diff-to-fix/ only, which is the whole trail); N-148 item 2's D-09 "corrected wording" (I verified the comment text is unchanged, not that D-09's row text matches a hypothetical corrected form — the item's textual claim stands either way).
- Nothing in this sweep touched any repository file; the only writes were to `s42-scratch/`.

---

*Generated by S42/W4 (read-only audit sub-agent). Census protocol: exact-basename `rg -F` over diff-to-fix/ + stem fallback + disposition lookup; liveness protocol: per-finding falsifiable-check re-run at HEAD. Sheet: `s42-scratch/w4-citation-gap-liveness.md`.*
