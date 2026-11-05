# MAJOR M-137 — the lost-partition surfaced shape: naming template, relabel, presentation metadata, the 10-target insertion cap, the per-candidate "Quick" registration, and the 512-byte fallback

**Severity:** MAJOR
**Module:** lost-partition / orchestrator (partition-recovery focus battery)
**Parameters affected:** #30 Candidate scoring (the surfaced set); #12 Session persistence format (candidate names); #13 report schema; #17 sort model (the surfaced-result ordering the naming feeds)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:634817-634838` (`FUN_1403b9330`: the "Lost Partition %d(%s)" template with the per-scan counter at +0x400, the `":)"`-label keep, the `"%s (%s)"` fallback); `:525588-525598` (the "Lost Partition" strstr relabel to "Unallocated" in the device listing); `:904185-904187` (sector size 0 → 512 fallback); `:904195-904234` (the 0xA8-stride insertion loop with the `if (9 < uVar19) break` cap); `:904239-904240` (the per-candidate `L"Quick"` registration); `re-workspace/artifacts/UIUX/08-scan-phase-deep-dive.md` §2 (the found-partition list rows: Disk / Basic / Dynamic / MBR / GPT / File System); `re-workspace/reversed/PartitionRecovery/rabin2_exports.txt:53-59` (the `CreateEnumPartitionObject` factory)
**Core references:** `src/core/orchestrator/cr_scan.cpp:1933-1939` (the `Candidate @ <LBA>` root naming; re-pinned at the merged HEAD: `cr_scan.cpp:2052-2058` after the S-048 window landings); `src/core/lost/cr_candidates.cpp:759-760` (the `ERR_BAD_PARAMETER` refusal on `logical_sector == 0`); `:915-1014` (the scoring pass — no insertion cap); `src/core/orchestrator/cr_scan.cpp:1900-2010` (every candidate materialized + deep-scanned, always-deep semantics); `src/app/rrecov.cpp:2665-2672` (the CLI candidate listing: LBA/sectors/score/fs only); `include/cr/cr_ui.h:449-486` (enumerate_disks/volumes — OS-level, no table-entry listing) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og surfaces lost partitions through a precise shape this session decoded end-to-end: unnamed candidates are named `"Lost Partition <n>(<FS>)"` by a per-scan counter (named ones keep their volume label; whole devices relabeled "Unallocated"); the found-partition review rows carry Disk/Basic/Dynamic/MBR/GPT/FS properties; the scan-target insertion loop **caps at 10 lost-partition targets** and registers each for a **"Quick"** search (the file recovery of a lost partition starts quick-first, advanced later); and a failed geometry query falls back to 512-byte sectors rather than refusing. The core has none of these: candidates are named `Candidate @ <LBA>`, there is no insertion cap (all candidates deep-scanned), no per-candidate Quick staging, no table-entry presentation metadata, and a zero geometry query refuses the whole discovery.

## What the reference does

All decoded first-hand this session (see the worksheet):

1. **Naming** (`FUN_1403b9330`, `:634817-838`): volume name NULL → `sprintf_s(buf, 100, "%s %d(%s)", "Lost Partition", counter, fsname)` with the counter incremented at `param_1+0x400` per unnamed partition; a name containing `":)"` (a drive-letter-bearing label like `Local Disk (I:)`) is kept verbatim; otherwise `"%s (%s)"` with the volume name. The device-listing pass relabels anything still named "Lost Partition" to **"Unallocated"** (`:525588-598`).
2. **Insertion** (`:904195-904234`): the assembled candidate list (0xA8-stride entries from the three-source assembly `FUN_140588820`) is iterated; each is logged `"Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d"` at `genericrecoverycontroller.cpp:0xa2`, turned into a scan target via `FUN_14017b110`, and registered via `FUN_140118650(…, L"Quick", 5, …)` — **the "Quick" mode registration** (:904239). The loop **breaks after the 10th** (`uVar19 = param_1[0x58]; … if (9 < uVar19) break`, :904229-234).
3. **Geometry fallback** (`:904185-187`): `if (local_19b8 == 0) local_19b8 = 0x200;` — a failed device sector-size query proceeds at 512.
4. **Presentation** (UIUX/08 §2): the "We found partition(s):" rows carry Disk / Basic / Dynamic / MBR / GPT / File System properties per partition, with per-disk "Preview" and Recover/Stop/Home controls; the home screen's partition list comes from the `CreateEnumPartitionObject` enumeration factory (rabin2 exports :54).

## What our core does

- Genuinely-lost discovery candidates get the synthetic root `u"Candidate @ " + LBA` (`cr_scan.cpp:2052-2058`), persisted in the session tree — no per-scan counter, no FS name, no "Lost Partition" template, no "Unallocated" relabel.
- Every surviving candidate is materialized and scanned with **deep semantics always true** (`cr_scan.cpp:2020-2105` + the internal note at `:1307-1317` documenting the drift); no insertion cap exists anywhere in the pass-3 chain (`cr_candidates.cpp:915-1014`); there is no quick-first staging per candidate.
- `discover()` refuses `logical_sector == 0` with `ERR_BAD_PARAMETER` (`cr_candidates.cpp:759-760`).
- The candidate surfaces carry LBA/sectors/score/fs only (`rrecov.cpp:2665-2672`; `cr_ui_apply_candidate{start_lba, sector_count, fs}`); the Basic/Dynamic/MBR/GPT presentation fields exist only inside the orchestrator (LDM precedence) and are not surfaced; the ABI has no parsed-table-entry listing (only OS-level disks/volumes).

## Gap

Stated falsifiably: a disk with two unnamed lost NTFS partitions shows og roots `Lost Partition 1(NTFS)` / `Lost Partition 2(NTFS)` in the persisted tree and a found-list with Disk/Basic/Dynamic/MBR/GPT/FS rows, and scans them quick-registered with at most 10 such targets; the core shows `Candidate @ 2048` / `Candidate @ 1234`, surfaces no presentation metadata, scans all candidates unconditionally at deep semantics, and on a failed geometry query aborts discovery where the og proceeds at 512.

## Impact

Session trees and host UIs built on the core cannot reproduce the og's persisted shape (the tree roots are the user-visible partition names in the og's product); the >10-candidate case diverges in results (core is a superset — more files found, but different scan shape and timing); the mid-scan "recover the partition before file scan" staging (quick-first) diverges from always-deep; the geometry edge case refuses instead of proceeding. Partition recovery is a named focus area of this session's directive; the surfaced-shape cluster is filed MAJOR (presentation + edge-shape divergence, superset scan behavior — not wrong recovery results, which S-051/M-017/M-118 already own their slices of).

## Reproduction / verification

- **Decompiled evidence (first-hand, this session):** the template + counter + relabel sites; the insertion cap + Quick registration at `:904229-240`; the 512 fallback at `:904185-187`; the UIUX/08 §2 rows; the exports list. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-lostpart-surface-batch.md`.
- **Core evidence (first-hand):** the `Candidate @` naming at `cr_scan.cpp:1933-1939`; the no-cap pass at `cr_candidates.cpp:915-1014`; the refusal at `:759-760`; the always-deep note at `cr_scan.cpp:1307-1317`; the listing surfaces at `rrecov.cpp:2665-2672` + `cr_ui.h:449-486`.
- **Execution status:** the og's real-run log (`og-codebase/log/DRWUI.log:630-648`) carries the insertion trace for one candidate — corroborated read-only; no execution comparison is possible for the >10 case without a fixture disk (the fix lane can synthesize one with 12 ghost boot sectors).

## Fix plan (for the engineering team)

1. **Naming** (`cr_scan.cpp:2052-2058`): replace the `Candidate @ <LBA>` root with the og template — `"Lost Partition %d(%s)"` using a per-scan counter (the ScanController already has per-scan state), the FS-family name, the volume-label-keep rule (if the candidate's boot-sector label parses, use `"%s (%s)"`), and keep the LBA as a fallback only when no FS name resolves. Session round-trip: the root is a string already — no format change.
2. **Presentation metadata:** extend the lost-candidate ABI/report row with `{layout: MBR|GPT|EBR, basic_or_dynamic, table_ok}` — all three already computed inside the orchestrator (the S-051 table parse + LDM detection); surface them in `rrecov`'s listing and `cr_ui_apply_candidate`.
3. **Insertion cap + staging:** add the 10-target cap to the materialize loop (`cr_scan.cpp:1920-2009`) with the count surfaced in stats (`lost_candidates_inserted` vs `lost_candidates`), and register candidates through the same quick-then-deep stage model the main volume uses (the `covers_both` bridge already models quick-deep composition — the candidate scan should reuse it rather than the always-deep special case documented at `:1307-1317`).
4. **Geometry fallback:** `discover()` maps `logical_sector == 0` to 512 with a Warn log instead of `ERR_BAD_PARAMETER` (the og's `:904185-187` shape).
5. Tests (below). Acceptance: a 12-candidate fixture scans 10 (stat `lost_candidates_inserted=10`), roots named per the template, the listing shows layout/basic-dynamic fields, and the zero-geometry case proceeds at 512.

## Tests to add

- `LostPart.UnnamedCandidateNamedLostPartitionN` (fails today: `Candidate @`).
- `LostPart.NamedCandidateKeepsVolumeLabel`
- `LostPart.InsertionCapTenTargets` (fails today: no cap)
- `LostPart.ZeroSectorSizeFallsBackTo512` (fails today: refused)
- `LostPart.CandidateListingCarriesLayoutFields` (fails today: LBA/size/fs only)

## Related findings

- **M-111** (active) — the ExplorerPartition browse chain (this finding's presentation batch is the *listing* side of the same surface; M-111 owns the in-app browse).
- **M-118** (active) — the register channel (the third source feeding the candidate list this finding's cap governs).
- **M-104** (FIXED) — its own open question 1 (the lost-partition display-name template) is **resolved by this finding's decode**; the fix lands here.
- **N-142** (active) — the device-descriptor record tail (adjacent session surface).

## Open questions

Whether the surfaced-result cap (2/4/5/6 per the AdjustPartitionInfoOrder decode) and this insertion cap (10) compose in any observed order in the og's real logs — the vendor QA log shows 3 rounds under both caps; the composition question is academic until a >10 fixture exists (the fix lane's test resolves it empirically for the core's own semantics).

## Resolution

**Status:** FIXED
**Fixed in commit:** e7030bf2
**Verified by:** tests Orchestrator.LostPartUnnamedCandidateNamedLostPartitionN / Orchestrator.LostPartNamedCandidateKeepsVolumeLabel / Orchestrator.LostPartInsertionCapTenTargets / Orchestrator.LostPartCandidateListingCarriesLayoutFields / LostDiscovery.ZeroSectorSizeFallsBackTo512 (the three behavioral ones RED-proven by behavior-revert: the naming/cap/fallback reverted → all three FAILED, restored → GREEN) + WholeDiskKnownVsLostDistinguishable re-pinned to the new template. Suite 1244: 1202 pass / 42 env-skip / 0 fail. All five corpus cites re-read first-hand this session (the :634817-838 template + counter + ":)"-keep + "%s (%s)" fallback; :904185-187 the 512 fallback; :904195-234 the 0xA8-stride insertion loop with `if (9 < uVar19) break`; :904239-240 the L"Quick" registration; :525588-598 the relabel [UI-layer, out of the core's scope]).
**Resolution notes:** The full fix plan executed. (1) Naming: the og template re-implemented at the materialize loop — unnamed candidates take "Lost Partition <n>(<FS>)" (the per-scan counter as the +0x400 semantics), the boot-label path takes "<label> (<FS>)" with ":)"-bearing labels kept verbatim, the LBA form demoted to the no-FS-name fallback (the finding's own rule); the FAT12/16 boot label (+0x2B, printable-validated, right-trimmed) captured by probe_fat — the only on-disk label discovery reads (NTFS/exFAT labels live in metadata the discovery never touches; the register-channel names remain M-118's open scope, cross-referenced in the code comment). (2) Presentation: disk_layout/disk_dynamic/disk_table_ok stamped from the orchestrator's own table parse + LDM probe onto every candidate, persisted per-record (PartitionContext v3 tail, 61 bytes, append-only — v1/v2 sessions load with zeros per the skip-unknown rule), surfaced in rrecov's apply listing ("MBR/basic, table ok, label \"VOLTWO\"") and the cr_ui_apply_candidate ABI tail (append-only, zero-init contract documented in the header + API.md). (3) The cap: the ten-target insertion loop with stats.lost_candidates_inserted (the surfaced set keeps ALL candidates — apply-able, the og's per-hit persistence; the cap log line carries the DRWUI cite). (4) The fallback: discover() wraps the zero-geometry device in FallbackGeomDevice (logical_sector → 512) — the shim approach because every probe reads geometry() directly for its LBA math; the Warn log replaces the refusal. (5) The quick-then-deep staging half of fix-plan step 3 was NOT taken as a separate mechanism: the core's single-walk D-36 design already runs the candidate engine pass with deep flags in ONE pass — the og's "Quick" registration (its two-generation architecture) maps onto the core's default-on stage model exactly as the SCAN-ENGINE-DELTA C1/I-044 decode established; the acceptance criteria (the four named behaviors) are all met, and the staging difference is the registered D-36 delta, not an open defect. The related M-104 open question 1 (the display-name template) is resolved by this landing as the finding predicted; M-118 (the register channel) and M-111 (the browse surface) remain open on their own files.
