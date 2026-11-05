# s70-lead-verification.md — the S79 lane's lead-verification sheet (every filed candidate, first-hand)

**Session:** 70 continuation audit lane · **Core at filing:** `3799dee2` (code-identical to `f3da99f1`; the S70 worklog commit `c736e9b3` carries no code delta) · **Corpus:** `be0cfe9`
**Method:** every divergence candidate from the five agent context sheets (`s79-lane-sheets/`) was cross-checked against the existing trail (duplicate-filing guard, the M-117/N-191 collision class), then lead-verified FIRST-HAND — the exact decompile lines re-read and the exact core lines re-read — before filing. Candidates that failed the coverage check were dropped with the reason recorded here.

## A. FILED (the eight)

### S-067 — exFAT chain walker: the out-of-range FAT-entry clamp absent (→ findings/SEVERE)

- **Og (first-hand):** `exFATSearchDLL_decompiled.c` :41315-41328 — the chain-walk loop tests `uVar30 < 0xfffffff6`, calls `FUN_180021c60` (the FAT-entry lookup), then `if (*(uint *)(param_2 + 0x18) <= uVar15) { uVar15 = 0xffffffff; }` — **an entry ≥ the volume's cluster count is mapped to EOC; the chain ends cleanly.** `FUN_180021c60` (:28039-28166) itself implements the FREE-continuation (`iVar4 == 0 → flag; return param_2 + 1`) — the core's EXFAT-2 port cites this function and is accurate.
- **Core (first-hand):** `src/core/exfat/cr_exfat_quick.cpp:71-82` — `next()` maps FREE/BAD → 0 and `>= 0xFFFFFFF8` → EOC only; values in `[cluster_count, 0xFFFFFFF7]` are returned RAW. `follow_exfat_chain` (:92-135) then pushes the garbage cluster as a member (:110), the out-of-FAT-bounds `entry()` read returns FREE (:58/:67-68), the bounds probe at :123-126 sets `broken = true`.
- **Coverage check:** the s12 lane sheet's EOC-family row covers `≥0xFFFFFFF8 / ==0xFFFFFFF7 / ==0`; EXFAT-3 (in-tree) landed the 0xFFFFFFF8..E family; **no trail row covers the ≥ cluster_count clamp** (grep: "0xfffffff6" zero hits in findings/modules; the exfat deep-dive artifact has no clamp row).
- **Impact:** corrupt/stale FAT entries (post-format remnants, torn writes) — the og recovers a clean partial chain at full confidence; the core attaches a wild out-of-volume extent AND downgrades to broken-chain confidence.

### S-068 — FAT32 deleted first-cluster high-16 re-base absent (→ findings/SEVERE)

- **Og (first-hand):** `FATSearchDLL_decompiled.c` :13758 (the `DAT_1800728c8` last-good-hint update after a successful non-dir walk), :14753-14755 (the low-16-only re-base: `*param_7 = (DAT_1800728c8 & 0xffffffffffff0000) + uVar5; *param_8 = 1;` under `uVar5 < 0x10000 && uVar5 < hint`), :14834-14841 (the retry walk under `0x310000 < hint`, the write-back to dentry +0x14/+0x1a and the hint update). The XOR-form sites :14228-29/:14435/:14447 are the same machinery in adjacent paths.
- **Core (first-hand):** `src/core/fat/cr_fat_quick.cpp:286` — `first_cluster = (u32(e.first_cluster_high) << 16) | e.first_cluster_low;` — verbatim, no hint tracking, no low-16-only detection, no re-base retry.
- **Coverage check:** fat.md:57's `0x10000` row is the small-volume gate (a different consumer of the same constant); "high word"/"hint"/"re-base" — zero hits in findings/modules; the fat-engine deep-dive artifact has no row (grep verified). N-172 is the deleted-first-CHAR naming; M-130 is the dentry shape validator — neither owns this.

### M-149 — lost-partition scoring: the +500 denominator fold + the count==0 numerator (→ findings/MAJOR)

- **Og (first-hand):** `DRWUI_decompiled.c` FUN_14050fda0 :842186-842240 — the max do-loop computes `weighted + *(+0x68)` per candidate with **no +500 walk**; :842244-245 the null/count-0 numerator is `0.0`; :842246-842274 the per-entry loop adds the +500 table-match bonus (:842258-842266, "Found once partition") BEFORE the `count > 1 ? ln : (double)count` numerator.
- **Executed evidence:** `og-codebase/log/CalcPartition.log` (the og's own runtime output) — the last round's row 6: `files:0 folders:0 dSuggestted:0.000000` (count==0 → 0.0, refuting the core's 1.0); rows 2-5 (`files:0 folders:0`) score 0.632748 (a nonzero dSuggested base — the composer's variable output); the round-1 fit: `ln(429)/ln(168878) = 0.503577` vs the logged `0.503579` (residual 2e-6) under d_max=60/d_row=0 — the numeric fit script + output in `evidence/executions/s79-calcpartition-score-fit/`.
- **Core (first-hand):** `src/core/lost/cr_candidates.cpp:1054-1065` — the single-pass loop folds `count += 500` BEFORE `max_count` accumulation (the denominator includes the bonus); `:655-665` — `num = count > 1 ? log(count) : 1.0` (count==0 → 1.0, not 0.0).
- **The regression history (first-hand, git):** the M-017 correction `48a6c099` landed the og-exact two-pass form in `cr_scoring.cpp` (`normalize()`: `num = raw > 1 ? ln : (double)raw`; pass-1 max WITHOUT the +500); the convergence merge `9c36efba` then dropped that implementation per FIRST-LANDING-WINS ("the parallel lane's canonical implementation stands (e1b5dcf + the N-035 data-area reconcile), this leg's convergent cr_scoring implementation dropped cleanly … the convergence record with the two decode divergences for the next pass") — **and the next pass never ran.** The e1b5dcf0-era single-pass form (with the +500 in the max) survived every later refactor into today's cr_candidates.cpp.
- **Coverage check:** lost-partition.md records M-017 FIXED with "the og's edge cases" and separately notes "0.1/0.3 gap keeps … FAT multi-child keep — no core counterpart" (the survival rules — a DIFFERENT gap, still open, not re-filed here); no trail row records the denominator form or the count==0 edge. `git grep rank_and_adjust` at HEAD: zero hits (the corrected implementation is fully gone).

### M-150 — session-load count pre-allocation (→ findings/MAJOR)

- **Core (first-hand):** `src/core/session/cr_records.cpp:345` — `std::vector<tree::FileNode*> by_id(count, nullptr);` — `count` is a hostile u32 read at :322 (`c.g32()`), value-initialized (every byte touched) — up to 34.4 GB at count=0xFFFFFFFF. The CRSF container caps the record payload at 64 MiB (`cr_crsf.cpp:304`, CRSF_RECORD_MAX) but nothing caps the in-record count field against the buffer length. The ABI's `guarded()` wrapper (`src/ui/cr_ui.cpp:135-143`) catches `std::bad_alloc` → ERR_INTERNAL on Windows; on Linux overcommit the zero-fill can OOM-kill (uncatchable SIGKILL) before bad_alloc.
- **Og:** the record-4 reader re-materializes via per-FS node factories (`re-workspace/artifacts/SaveScan/rsf-record4--tree-node-grammar.md:16,26` — "group framing, node-type factory"; the reader consumes per-node records so buffer exhaustion bounds allocation).
- **Coverage check:** "deserialize_tree" — 4 hits, all in evidence/lane sheets (no finding owns it); N-139 is the carve-side OOM policy; the in-tree hostile-input tests cover name/role length bombs (Session.HostileNameAndRoleLengthBombs) — no count-cap test exists.

### M-151 — NTFS truncated-MFT run-growth probe absent (→ findings/MAJOR)

- **Og (first-hand):** `NTFSSearchDLL_decompiled.c` FUN_180007670 :5617-5700 — for each $MFT $DATA run, after the declared length: reads 4 KiB windows (`(**(code **)(*param_2 + 8))(param_2, lVar2, uVar11, 0x1000, 0)`), counts fixup-valid records (`FUN_180041bb0` at the 0x400 stride), and EXTENDS the run's length (`*(longlong *)(lVar1 + 0x10) += (uVar10 / *(short*)(param_1+0x1a)) / sector_size;`) — growing the walkable MFT past a truncated runlist while records validate.
- **Core (first-hand):** `src/core/ntfs/cr_mftwalk.cpp:422,481,488-490` — the walk is bounded by `vol->max_record_no()` (the declared MFT size); `read_record` → `ERR_RANGE_OUT_OF_BOUNDS` → `continue`. No extension probe exists (grep "extend|grow|probe.*record" — the hits are unrelated).
- **Coverage check:** `grep FUN_180007670 diff-to-fix/` — zero hits. S-063 (FIXED) is the $MFTMirr BOOTSTRAP fallback (a different torn-MFT mechanism); the M-043 fallback retry is the MFT-load-failure rung. Neither covers post-load run growth.

### M-152 — ext4 sweep: the orphan-reserved skip (→ findings/MAJOR)

- **Og (first-hand):** `EXT4SearchDLL_decompiled.c` FUN_180005e50, the per-inode harvest filter at :4419-4420 — skip iff `i_size_lo(+0x04)==0 ∨ !(i_flags(+0x20)&0x80000) ∨ magic(+0x28)!=0xF30A ∨ eh_entries(+0x2A)==0`. **No dtime/links predicate.** (Agent C's "depth==0" attribution corrected here: `puVar11[0x15]` = byte +42 = `eh_entries`, not the depth word at +0x2E.) The sweep driver calls it per-inode over inode-table blocks (:12246, inside the 1 MiB-read loop with the dx_root 0xC03B3998 context).
- **Core (first-hand):** `src/core/ext4/cr_ext4_engine.cpp:1145-1146` — `if (iv.dtime == 0 && iv.links != 0) continue; /* live but unreferenced (orphan-reserved) - skip */`.
- **Coverage check:** "orphan-reserved"/"live but unreferenced" — zero hits in diff-to-fix and docs/PARITY-DELTAS.md; PARITY-DELTAS' two "orphan" hits are FAT/CheckIssue topics. The emission-side of the og's sweep map (the +0x4a8 std::map; the consumer FUN_180008b80 is a name/mode lookup helper) was NOT traced end-to-end this session — recorded honestly in the finding's verification + open questions.

### N-213 (renumbered from N-210) — NTFS per-record pre-gate band (→ findings/MINOR)

- **Og (first-hand):** `NTFSSearchDLL_decompiled.c` FUN_180046160 :55187-55220 — the record-header pre-gate: magic "FILE"; `usa_count(+0x0C) ≤ 0x10`; `alloc_size(+0x1C) ≤ 0x1000 ∧ % 0x400 == 0`; `alloc_size/0x200 ≥ usa_count-1`; `usa_offset(+0x04) ∈ [0x2A, 0x100]`. Called from FUN_180041bb0 (:52524) before the fixup apply.
- **Core (first-hand):** `src/core/ntfs/cr_fixup.cpp:28-54` — gates: `record_bytes ≥ 0x400`, sector-size band, `usa_offset != 0 ∧ even`, `usa_count ≥ 2`, the two fit checks. No usa_offset band, no alloc-size alignment/granularity gate (the core's record_bytes is walk-supplied, not record-declared).
- **Coverage check:** the >4 KiB / odd record acceptance is REGISTERED (D-143, N-196's deliberate-superset stance) — this filing is specifically the UNREGISTERED half: the `usa_offset ∈ [0x2A,0x100]` band and the alloc-size {1024,2048,3072,4096} set. "usaCount"/"allocSize" — zero hits in findings/modules.

### N-211 — cr_scan.h autosave contract text vs the S-057 landing (→ findings/MINOR)

- **Core (first-hand):** `include/cr/orchestrator/cr_scan.h:322-323` — "a cancelled run never auto-saves - the og's flow is the completion window" (in the autosave_dir field contract); `src/app/rrecov.cpp:1927-1936` — S-057's landing: "the og's stop window auto-saves … the stopped scan's partial results land in the auto session exactly like a finished run's" (`autosave_if_due(true)` on the cancel path).
- **Coverage check:** the doc_truth_gate covers docs/*.md + README, NOT headers — this drift class (a header contract comment contradicted by a later landing) has no gate and no finding.

### OQ-S70-1 — the $LogFile replay machinery's result contribution (→ tracked-open)

- **Og (first-hand):** the volume-open path loads and walks $LogFile at BOTH open sites — `NTFSSearchDLL_decompiled.c` :753-755 and :31653-31655: `FUN_18001cf20` (:22856+, the loader: runlist read, restart-page dual validation via FUN_18001cdd0, the 0x200/0x800 sector-size handling, the +0x29 second-restart flag) then `FUN_18001d460` (:23100+, the walk driver: the RCDR check `*(int *)(lVar2 + 0x2000) == 0x44524352` at :23132, the restart-position walk, FUN_180029dd0 at :23133/:23155/:23157). `FUN_180029dd0` (:33577+) walks RCDR pages parsing client/entry restart pairs.
- **Core:** zero $LogFile machinery anywhere (grep LogFile over src/include — zero hits).
- **Why tracked-open, not a finding:** the structural absence is certain and first-hand verified, but the walk's OUTPUT semantics (what the harvested log records contribute to the RESULT SET — files, names, runlists, or only internal volume state) was not decoded to the emission site this session. The blocker and the precise decode list are in the OQ file. (Agent B's USN-content claim was dropped separately: the same-name merge EXISTS in the core — `cr_mftwalk.cpp:1659-1701`, phase E — and FUN_18001d690's call sites are the MFT-emission path, not the USN path.)

## B. DROPPED after the coverage check (each with the reason)

1. **Agent A DC-A1** (the pending/continuation model + 16-entry cap) — REGISTERED: I-031 + D-101's boundary addenda (the pending-16 cap, oldest-first eviction, Stop()-emits-partials).
2. **Agent A DC-A2** (first-accept vs score-max winner selection) — N-143 (FIXED, the two-step dispatch) + N-030 (the dead-confidence decode) own the dispatch-precedence surface; a re-audit needs the N-070 score-max premise re-verification, deferred to the next lane (not first-hand verified this session).
3. **Agent A DC-A4** (per-sector vs per-1-MiB-window cancellation) — the engine-emits-no-progress + per-chunk continue-check boundary is D-78's registered note; the cancel-granularity half is subsumed by the stateless-architecture registration (D-101).
4. **Agent A DC-A7** (JPEG last-EOI vs first-EOI) — S-018 was reclassified INVALID with the backward-EOI-vs-forward-walk question explicitly moved to tracked-open by the s3-jpeg-eoi-encoder-correction sheet; not re-filed without new decode.
5. **Agent A DC-A3/DC-A5/DC-A6** — repair-line OOS (D-137 adjacency), the D-45 host-contract registration, and the S-039 caps family respectively.
6. **Agent B #1/#2** (viability probe granularity + claim-map provenance) — COVERED by S-012 (FIXED): the pass-2 claim-map walk landed; the main-walk per-cluster $Bitmap grading is the registered D15 informational superset.
7. **Agent B #3** (USN same-name content) — the same-name merge EXISTS: `cr_mftwalk.cpp:1659-1701` (phase E, the S-012 size-0 rescue post-walk merge, 5000-bounded). FUN_18001d690's call sites (:24096/:24107, inside FUN_18001e6c0) are the MFT-emission path — the og's USN nodes do not route through it either.
8. **Agent B #5** (USN harvest bounds) — N-039 (FIXED) owns the tail-window stance.
9. **Agent B #9** (directory tree from INDX first-class) — a documented scope refusal (ntfs.md:141-142 "INDX D-N").
10. **Agent B #12** ($Tops/$TxfLog/$Repair suppression) — sub-MINOR naming-table residue; recorded here for the next NTFS pass, not filed (the metafile filter surface is I-001's family).
11. **Agent C FAT-D1** (FAT12 0xFF7 followed) — I-002 REGISTERED the raw-BAD delta verbatim ("FAT12 0xFF7 raw", the s12 sheet's MATCH row).
12. **Agent C FAT-D5** (strict date day>29) — N-010c owns the dentry date sanity; the day-bound edge is within its registered scope (not re-filed without re-decoding N-010c's exact bounds — noted for the next FAT pass).
13. **Agent C EXFAT-D3** (deleted-cluster bitmap reasoning) — unproven at function level per Agent C itself; needs the CQuickSearchAndBuildBitmap decode before any filing.
14. **Agent C BTRFS-D1** (mirror abandonment — core superset) — the core's mirror continuation is the I-003-adjacent deliberate-superset family ("btrfs core exceeds og deep scan and mirror probing"); D-59c/I-003 registration.
15. **Agent C BTRFS-D3/D4** (short EXTENT_DATA edges; the NULL-stub parity) — the og's `EUCreateBtrfsComplateSearcher` NULL stub means no og deleted-engine exists (parity posture, RAW carving owns that lane); the extent-data edge needs its own decode pass.
16. **Agent C HFS-D1** (colon→slash folding) — comparator-local fold (`HFSPlusSearchDLL` :11816-11821, the 2-level Unicode table + the 0x3a→0x2f fold inside a catalog-order comparator); the core's full-leaf-scan access never consults the comparator — no user-visible path established, lane-sheet note only.
17. **Agent C HFS-D2/D3** (journal-replay child resolution; leading-NUL trim) — HFS-D2 needs a journal-volume fixture to falsify; HFS-D3 (NUL trim + 255 cap at :7623-7633) is real but cosmetic-tier; both recorded for the next HFS+ pass.
18. **Agent D risk #1** (no read deadline + mutex across I/O, `cr_cached.cpp:31/61`) — the og's deadline/watchdog lives in the DRWUI host layer (the S69 racing lane's claimed territory: the DRWUI stuck-watchdog decode); the OS-level disk timeout bounds both engines equally. Recorded as a core-side hardening note, not a parity finding.
19. **Agent D risk #4/#7/#10** (O(n²) dedup; legacy mask-truth no-op resumes; huge-but-valid table entries) — performance/registerposture notes; the wall-clock-budget question folds into OQ-S4-3's chunking-relative-perf scope.
20. **Agent E #3/#6/#7/#8/#9/#10** — the dSuggested composer (OQ-S2M-2 RESOLVED, the flat-value mapping registered); the save-progress curve (N-060 + D-108 own it); the small-result population flag-gate and the second DAT_141be5818 tick site and the gauge mechanism (M-134/N-205/N-183's landed grading family + the OQ-S42-3 residual registrations); the 0.1/0.3 survival rules (lost-partition.md's standing "no core counterpart" note — M-017's known remainder, NOT re-filed as a duplicate).
21. **Agent E #4/#5** — artifact-erratum material (the viability `0x3FF` off-by-one and the assistant-grading-preview misreads): recorded in the lane sheets for the corpus-errata queue; the CORE side is correct on both (verified: `cr_mftwalk.h:80` strict `>`).

## C. The baseline build + suite

The linux-release baseline build was launched first-hand at lane open (gcc 14.2, `-Wall -Wextra -Werror -Wshadow` per the CMakeLists global) and the full ctest suite run is the lane's execution leg — the run record is appended to this sheet at close (see the worklog S70 close entry). The lane's findings rest on decompile + runtime-log evidence (verification classes 2 and 1 respectively per PARAMETERS §6); the suite run guards the read-only guarantee (the tree builds green untouched).

## D. The push-rebase reconciliation (recorded for truthfulness)

The racing S71..S76 fix lanes landed 29 commits mid-audit (N-131, M-136, M-135, M-148,
M-143, M-144, N-200, N-201, M-147 riders, D-149) while this lane verified and filed. At
the push-rebase:

- **The lane renumbered S70 → S77** per SA-9 (the racing fix lane's S70 section pushed
  first; the S71..S76 seats also taken). All filings re-attributed; the audited commit
  pin (`3799dee2`) stands — every finding was re-verified LIVE at the rebase head
  `0da3490b` (all eight divergences confirmed still present; the line cites refreshed
  in-place with the audited-pin originals preserved).
- **N-210 → N-213** per SA-9 (the remote S71 repair lane's committed-marker finding
  `N-210-committed-conflict-markers-recurred-f3da99f1.md` keeps the seat — it found the
  SAME marker recurrence at this lane's open commit `c736e9b3` and additionally located
  THREE more marker-bearing files (I-033, N-135, the s51 prompt archive) that this lane's
  narrower grep missed, and fixed all four).
- **This lane's N-212 (the marker recurrence) RETIRED as a convergent duplicate** — the
  remote N-210 owns the defect; this lane's independent discovery, the local matrix sweep,
  and the gate's-ninth-check proposal are recorded here and in the worklog as the
  convergence corroboration (the SA-9 convergence-note convention). The fix plans CONVERGE:
  their landing swept the markers AND landed the prevention gate (ci-local.sh's
  N-210 step — the full-text-tree marker grep with the M-120 exemption, verified
  present in this tree); this lane's independent doc_truth_gate ninth-check
  proposal is redundant against it — the convergence corroboration.
- **The board at the rebase head (before this lane's filings): 0 ACUTE · 0 SEVERE ·
  13 MAJOR · 9 MINOR · 0 INFO active / 163 FIXED / 4 tracked-open** (counted first-hand
  from the origin/main filesystem: 13 MAJOR files, 9 MINOR files incl. the remote's
  N-212-asf, zero SEVERE/ACUTE/INFO; N-107/N-127 + M-086/M-087/M-089 driver-repo-resident
  per OQ-S10-3).
- **After this lane's seven live filings + one OQ: 0 ACUTE · 2 SEVERE · 17 MAJOR ·
  11 MINOR · 0 INFO active / 163 FIXED / 5 tracked-open.**
