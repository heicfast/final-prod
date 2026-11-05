# S69 lane — liveness battery of the active board (first-hand, at core `3799dee2`)

Method: every active finding's CORE-SIDE claim re-verified first-hand at the
pinned HEAD with targeted greps/reads of the cited files and lines. The
driver-repo-resident quintet (M-086/M-087/M-089/N-107/N-127) is out of the
core repo's write scope per OQ-S10-3 — verified as doc-standing only.

| Finding | Core-side claim checked | Evidence at `3799dee2` | Verdict |
|---|---|---|---|
| M-073 scan-time folder filter absent | no caller folder-filter in ScanConfig/ABI | `cr_scan.h` ScanConfig carries only the fixed junk blocklist (`cr_filterfolder.h` applied once per stage at `cr_scan.cpp:1202`-band; the PushFolderFilter cites are comments of the M-018/M-073 trail) | STILL |
| M-075 chkdsk launch absent | only in-engine detection | `cr_fscheck.cpp:971` "Please use 'Chkdsk'" message decode; `cr_bcdfix.h:36` "post-op ChkDsk.exe delegation is registered UI-layer" — no launch surface anywhere in src/win | STILL |
| M-078 bitlocker turn-off absent | detect/unlock only | only `cr_decrypt_device.cpp` (M-033 XTS in-stack); no turn-off/decrypt op in pending-op surface | STILL |
| M-086 driver FileVersion pin | driver repo | OQ-S10-3 boundary — doc-standing | ACTIVE-OPEN |
| M-087 regdrv UpperFilters encoding | driver repo | OQ-S10-3 boundary — doc-standing | ACTIVE-OPEN |
| M-089 rdabench phantom-zero fuzz gap | driver repo | OQ-S10-3 boundary — doc-standing | ACTIVE-OPEN |
| M-101 Versions/Tags-Extracted branches absent | 4 roots only | ScanController ctor `cr_scan.cpp:308-317` adds exactly Found / Deleted / File Path Lost / carve root; `lost_files_tags_extracted` exists only as a report-kind string (`cr_safewriter.cpp:804`) | STILL |
| M-105 DRWService install lifecycle absent | no SCM lifecycle | grep CreateService/install_service in src/include: zero hits | STILL |
| M-107 batch1 size-walk stand-in residue | 64 MiB stand-ins present | `cr_registry.cpp:955` "N-148: the M-107-verified 64 MiB stand-in (was 16)"; `cr_analysers.cpp:4522` `u64 end = hit + (64ull << 20)` | STILL |
| M-111 explorer partition browse absent | no ExplorerPartition | grep zero hits | STILL |
| M-113 recovered-OS boot surgery absent | offlineos read-only | `cr_offlineos.cpp:7` "API, no writes, no allocation beyond the caller's record"; only find_key/reads of \MountedDevices | STILL |
| M-114 system-restore purge absent | no restore-point surface | grep restore-point/SystemRestore: zero hits | STILL |
| M-118 three-source assembly (register channel) absent | no MountPoints2 walk | grep MountPoints2/from-register: zero hits | STILL |
| M-135 recovery read-failure semantics batch | zero-fill+continue | `cr_safewriter.cpp` Skip-policy comments: "zero-fill + Salvaged outcome" (:179-:181), partial-span zero-fill (:277-:281, :440, :462) | STILL |
| M-136 recovery reads bypass retry/cache pipe | bare device to writer | `cr_safewriter.cpp:67` `SafeWriter(..., IStorageDevice* source, ...)`; `:326` `read_dev = it.read_src != nullptr ? it.read_src : source_` — no RetryDevice/CachedReader wrap | STILL |
| M-143 ext4 journal inode-image harvest absent | only dir-pair leg | harvest band `cr_ext4_engine.cpp:820+`: journal_dirs map, read_inode(journal_inum); no inode-table-image stride walk, no i_size!=0+EXTENTS_FL+0xF30A accept leg | STILL |
| M-144 ext4 large-volume caps refuse | caps present | `cr_ext4_engine.cpp:785` `gdt_bytes > (8ull << 20)` refuse; `:83` MAX_GROUPS = 1<<20 | STILL |
| N-107 driver stale command-count prose | driver repo | OQ-S10-3 — doc-standing | ACTIVE-OPEN |
| N-127 driver doc-vs-code truthfulness | driver repo | OQ-S10-3 — doc-standing | ACTIVE-OPEN |
| N-131 FS-native parent anchors not persisted | no parent-ref fields | grep parent_ref/fs_parent in session: zero hits | STILL |
| N-140 carve per-extension stats surface absent | no live map<ext,count> | only per-extension skeleton folders (`cr_carve.cpp:235`); no analyser-fed stats map served to host | STILL |
| N-144 txtview text-snapshot thumbnails absent | eligibility gate landed (M-108), render absent | `cr_preview.h:147-165` M-108 gate (whitelist membership); no text→BMP snapshot renderer in thumbnail path | STILL |
| N-145 pending marked-disk marker absent | no marker | grep marked-disk/pending-marker: zero hits | STILL |
| N-178 CLI pause legs absent | no CLI verb | `rrecov.cpp` pause hits are doc comments on the engine token; no pause command surface | STILL |
| N-181 RAW-first start ordering absent | sequential stages | `cr_scan.cpp:1401` run_quick → `:1410` run_deep → `:1419` run_carve_stage — carve strictly after deep | STILL |
| N-182 deep-range derivation absent | no mode-1 resolver | scan-start log carries extent+hint only (`cr_scan.cpp:1323-1330`); no sub-range/300MB/neighbor-extend derivation | STILL |
| N-200 lostpart NTFS probe acceptance weaker | +30 MFT, no retry | `cr_candidates.cpp:108` `{"mft-probe", 30}` evidence push; no $MFTMirr retry, no boot-pair cross-check gate | STILL |
| N-201 FAT media-word flavor refinement absent | chain-side check only | `cr_chain.cpp:184-196` FAT16-exact media-word checks in chain validation; no post-BPB FAT1/FAT2 read → re-flavor | STILL |

**Board verdict: 28/28 STILL ACTIVE (23 core-side STILL + 5 driver-repo
ACTIVE-OPEN per OQ-S10-3). Zero falsely-active. Zero phantom-fixed.** The
remove-the-fixed sweep is clean: the fix lanes' retirements (through N-203 /
M-146 at `f3da99f1`) are all in findings/FIXED/ already; nothing they landed
left a stale active row.

Battery date: 2026-10-29. Pin: core `3799dee2` (lane open), corpus `be0cfe9`.
