# Agent E — NUMERIC-CONSTANT CENSUS: og calculation / grading / scoring math vs core_recovery

Read-only parity audit. Every constant below was re-verified first-hand against the
decompilation this session unless explicitly marked "(core-decoded)" (constant taken from a
core comment whose og cite I did not re-open).

**Cite conventions** — `DRWUI:N` = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`
line N; `NTFSSearchDLL:N` = `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
line N; artifact cites are `path:line` relative to `/home/z/my-project/easeus-drw-research/`;
core cites are relative to `/home/z/my-project/core_recovery/`.

**Exclusions honored** (per mission): the N-194 byte-size display formatters
(KB/MB/GB %.2f 1024-tiers at DRWUI:264525-264583; the B..TB ladder at DRWUI:1556295-1556400 —
core ports at `src/core/query/cr_query.cpp:681-740`, NOT re-censused); the N-205
rawCount/raw_tail_zero grading-counter semantics (referenced only, group E); ghidra-mo has NO
grading/assistant .mo module (grep for dSuggestted/grading across `reversed/ghidra-mo/` = 0
hits — the lost-partition scoring lives in DRWUI.exe itself, see group B).

---

## A. The recoverability grading model (per-file "Recovery Chances")

**Tier names.** The og model is a 3-value numeric grade on the node — **0 (poor/none), 1
(average, engine default), 2 (high)** — surfaced in the UI as the "Recovery Chances: High"
circular gauge (`UIUX/35-redesign-reference-atlas.md` v2 addendum in
`05-uiux-architecture.md:210-212`; recovery-chance column in `CListHeaderView`,
`UIUX/09-results-phase-deep-dive.md:55`). Engines emit only grade 1 (ctor default 0x20 = grade
1 in bits 5-7 of node+0x73, NTFSSearchDLL:39242/:39779 — core-decoded, `include/cr/tree/cr_grading.h:8-10`);
every 0/2 verdict is computed DRWUI-side at scan completion. There is **no 5-tier
Excellent/Good/Average/Poor/Very-Bad scale anywhere in the og**; the 5-tier
`tree::Confidence {Unknown=0, Excellent=1, PartialRisk=2, Overwritten=3, MetadataOnly=4}`
(`include/cr/tree/cr_node.h:57-63`) is a **core-internal allocation axis with no og
name-counterpart** (the og's viability walk only emits destroy-vs-keep + DRWUI grades).
The og's separate 4-status disk-health model {Unknown, Good, Caution, Bad}
(`include/cr/health/cr_grade.h:55-60`, corpus DiskHealth.exe :37087-37100) is a different
system (S-024, summarized in group G).

| # | Constant / rule | Exact value | og cite | Semantics | Core counterpart |
|---|---|---|---|---|---|
| A1 | size floor | `size < 1 → grade 0` | DRWUI:605726-605728 (`FUN_14038ac90`) | zero/negative logical size = unrecoverable | `src/core/tree/cr_grading.cpp:336-338` ✓ |
| A2 | category-0xC fast path | `type==0xC && grade!=2 → grade 2` | DRWUI:605731-605733 | 0xC = **email category**, not ZFS: mapper `FUN_14039aa40` rewrites type 6→0xC on bit 35 of node+0x89 (core-decoded, `cr_grading.h:16-18`) | `cr_grading.cpp:343-345` (cat==Email) ✓ |
| A3 | raw/carve arm | `vt+0xe8 != 0 → (size < 0x400 ? grade 2 : grade 0)` | DRWUI:605912-605921 (`FUN_14038b0e0`) | small carved results (<1 KiB) intact; large presumptively partial | `cr_grading.cpp:355-357` ✓ |
| A4 | NTFS device-index rule | `device vt+0xb0 index ∈ [0,9) → grade 2; DAT_141be5818++` | DRWUI:605895-605899 | first-9-device NTFS population auto-High + gauge tick | `cr_grading.cpp:363-372` (`ordinal < 9`, `ntfs_device_index_high`) ✓ |
| A5 | undecoded exclusion | `vt+0xb8 != 0 \|\| vt+0x100 == 0xa8 → grade 0` | DRWUI:605901-605907 | fixer/damage exclusion arm | **NO CORE COUNTERPART** (deliberate non-adoption, `cr_grading.h:26-27`, OQ-S42-3) |
| A6 | second 5818 tick | `props vt+0x100 != 0xa8 && grade != 2 → DAT_141be5818++; grade 2` | DRWUI:585040-585051 (insert path `FUN_140369b50`) | insert-time auto-High with the 0xa8 exclusion | **NO CORE COUNTERPART** (core models only the completion-pass site A4) |
| A7 | ReFS/Btrfs insert hook | `device vt+0xa0 == 10 (ReFS) → grade 2; DAT_141be5830++; DAT_141be5828++` (0xB/Btrfs twin at :605473-605488 incl. `DAT_141be5840++`) | DRWUI:605491-605496 | non-existing ReFS/Btrfs auto-High + gauge ticks (S-061) | `cr_grading.cpp:375-388` ✓ |
| A8 | existing-file rule | existing (bit 6 of +0x72) with size ≥ 1 → grade 2 at insert | `FUN_14037a8a0` family (core-decoded cite) | live files always High | `cr_grading.cpp:347-352` (DeletedState::Active) ✓ |
| A9 | twin-key formula | `key = base + len + (disk_ordinal − 2 if device vt+0xa0 ∈ {1,4}) · unit` | DRWUI:605768 / :605924-605927 / :605939 | the −2 is an **ADDRESS offset** for kinds {1,4} = FAT16 + EXT2/3-family (M-133), never a grade change | `cr_grading.cpp:119-153`; `kDeviceUnit = 1<<40` (`cr_grading.h:92`) vs og's undecoded `local_b8[0]` unit |
| A10 | twin detection | same extent-end key + same type/category → **both** grade 2 + overlap bit 2 | DRWUI:605775-605784 + `FUN_14038ba60` (NTFS branch inserts into map at ctx+0x20, `DAT_141be5840++` @ :606028) | duplicate-content detection | `cr_grading.cpp:504-516` + `cr_node.h:313-317` (set_overlap → grade High) ✓ |
| A11 | deep-read grade validation | key==0 (extentless): read sector 0 — first 32 bytes ALL ZERO **or** first 4 bytes 0xFF → grade 0; key>0 with zero index term: read sector `key−1` (the extent's LAST sector) — final 16 bytes ALL ZERO → grade 2; nonzero index term → read fails, no change; overlap flag → skip read entirely | `FUN_14038c470` DRWUI:606291-619 (core-decoded, M-134/M-145/M-146; entry cites :605798/:606318) | one bounded sector read flips grade on actual disk bytes | `cr_grading.cpp:232-296` (`deep_read_grade`) ✓; sector-size guard `ss==0 \|\| ss>4096 → skip` is a core-only safety valve (:243-244) |
| A12 | 169-entry extension table | 169 × 9-byte entries {8-char NUL-padded key, flag byte 0} at `.data 0x141bb0500`; ctor loop bound 2000 iterations, inserts `""` after the 169 real keys | DRWUI:605297-605304; lookup at :605740-605741 | **NOT-in-table** gates the twin/deep path (S-058: `FUN_1401bebe0` = iterator inequality; the `""` quirk makes dot-less names skip too) | `cr_grading.cpp:29-68` (table verbatim, UPPERCASED) + gate `cr_grading.cpp:494-496` ✓ |
| A13 | grade setter semantics | monotonic-max; grade-2 sticky (grade 0 never clears a 2); overlap bit set together with grade ≥ 2 | `FUN_140089630` / `FUN_1400895b0` (core-decoded cites) | | `cr_node.h:300-317` ✓ |
| A14 | one-shot re-grade gate | `grade==1 && graded-bit clear → re-run full FUN_14038ac90`; graded bit set via `FUN_140389540(node,1)` | DRWUI:340852-873 / :605800 | per-selection re-grade fires once per node | `cr_grading.cpp:607-616` ✓ |
| A15 | 0x77fffff size arm | type==0x23 (SearchDLL ctor-default FS code): `size ≤ 0x77fffff (125,829,119 ≈ 120 MiB) → viable(1) else 0`; other types: `size / settings[+0x70] != 0` | DRWUI:330974-330999 (`FUN_1401c76e0`) | the rawCount/gauge population size rule | `cr_grading.cpp:431` (`> 0x77fffffull` skip) ✓ — settings[+0x70] divisor value **NO CORE COUNTERPART** (undecoded) |
| A16 | generic-path branch | generic path runs when FS **NOT in {3, 10, 0xB}** (NTFS/ReFS/Btrfs handled by insert hook + device walk) | DRWUI:605736-605737 | NOTE: `artifacts/DRWUI-subsystems--assistant-grading-preview.md:42-44` states the **inverted** reading — see divergence D5 | `cr_grading.cpp:359-390` ✓ (correct polarity) |

---

## B. Lost-partition candidate scoring (dSuggestted + dedup/merge precedence)

Field map of `CSearchedPartitionInfo` (ctor DRWUI:942620-942648): FS type `+0x08` (default
0xE "Unknow", switch→names at :842340-842391: 1/2=FAT family, 3=NTFS, 4/5=EXT2/3, 6=RAW,
7=exFAT, 8=HFS+, 9=EXT4, 10=ReFS, 0xE=Unknow); StartSector `+0x18`; DataAreaStart `+0x10`;
totalSector `+0x60`; fileCount `+0x20`; folderCount `+0x28`; **dSuggestted (double) `+0x70`**;
flags `+0x78`/`+0x80` (removed); bytes-per-sector `+0x58` (default **0x200**, force-512 when
`(bps & 0x1ff)!=0 || bps==0`, :942643-942645); base count `+0x68` (ctor init **1**).
**EndSec = DataAreaStart + totalSector** (:842476-842479). Log gate: scoring/log output only
when `c:\log\CalcPartiton.log` exists (fopen "r", DRWUI:845547/:845642).

### B1. The dSuggestted formula — `FUN_14050fda0` (DRWUI:842164-842278)

```
raw_i = base(+0x68) + weighted(files(+0x20), folders(+0x28)) [+ 500 if table-match]
weighted:  FS==8 (HFS+):  files + 10*folders                    (DRWUI:842215, :842250)
           FS 1/2 (FAT):  (files + folders) * 0x32 (50)          (DRWUI:842218, :842253)
           else (NTFS etc): folders*0x32 (50) + files            (DRWUI:842221, :842256)
table-match ("Found once partition:%I64d", DRWUI:842260-266):
    same FS type AND StartSector(+0x18)+param_4 == a record from the history list
    (param_3 = the MountPoints2\CPC\Volume registry cache read by FUN_140516000,
     DRWUI:845827-845940)  → raw += 500
num_i = ln(raw_i) if raw_i > 1 else (double)raw_i ; 0.0 if candidate==NULL (DRWUI:842244-272)
den   = ln(max_raw) if max_raw > 1 else 1.0   — max over raw WITHOUT the +500
        (max loop DRWUI:842210-842230 has no bonus walk; den=1.0 bits 0x3ff0000000000000)
dSuggestted = num / den   (printed "%f", can exceed 1.0 for a table-matched leader)
```

**Log ground truth** (`og-codebase/log/CalcPartition.log`, 205 lines; forensics in
`artifacts/config-files/qa-logs--session-forensics.md:37-42`): round 1 (23:12, 101 NTFS
candidates on the I:-disk search): leader files:3618 folders:3304 → 1.000000; files:429 →
0.503579; files:373 → 0.491958; files:349 → 0.486433; files:284+1 folder → 0.482783 (ranks
above files:329 → 0.481530 — the 50-per-folder weight is visible); round 3 (00:43, 6
candidates): files:16 folders:2 → 1.000000; 0-file ghosts → 0.632748; one 0/0 candidate →
**0.000000** (raw==0 → num=0.0). My independent fit: `den = ln(168818)` (= leader raw
3618+3304·50, base≈0-2) reproduces **all** printed values to ≤7e-6; `den = ln(168818+500)`
is off by 1.1e-4 — i.e. the QA log supports **max WITHOUT the +500**.

| Constant | Value | og cite | Core counterpart |
|---|---|---|---|
| folder weight (NTFS family) | 50 (`0x32`) | DRWUI:842221 | `weighted_count` `cr_candidates.cpp:645-653` ✓ |
| folder weight (HFS+ FS 8) | 10 | DRWUI:842215 | ✓ :648-649 |
| FAT weight (FS 1/2) | (files+folders)·50 | DRWUI:842218 | ✓ :650-651 |
| found-once bonus | +500 | DRWUI:842264 | `table_match`/:1062 ✓ (but folded into max — see D2) |
| normalization | ln(raw)/ln(max); raw≤1 identity quirk; NULL→0.0 | DRWUI:842232-274 | `score_ratio` `cr_candidates.cpp:655-665` — **count==0 → 1.0, NOT 0.0** (see D1) |
| score storage | double 0..1 (`%f`) | +0x70 | permille `int(ratio*1000)` `cr_candidates.cpp:1070` |
| per-FS base (+0x68) | analyser-set, variable (log fits: ≈0-2 round 23:12; ≈26 ghosts / ≈60 leader round 00:43) | ctor 1 @ :942638; scorer add @ :842223 | fixed `fs_bonus`: NTFS 60 (mft-probe+3 evidence) / 30 partial, FAT 20, else 0 (`cr_candidates.cpp:781-794`, OQ-S2M-2) — see D3 |

### B2. Dedup / merge precedence — `AdjustPartitionInfoOrder` = `FUN_140510410` (DRWUI:842495-842859)

| Rule | Numeric key | og cite | Core counterpart |
|---|---|---|---|
| single-candidate flag | list size==1 → cand flag `+0x10 = 1` | DRWUI:842552-855 | (core has no single-candidate flag) |
| runner-up within 0.1 | `cand[1].dSugg − cand[0].dSugg ≤ 0.1 → cand[0]+0x80=1 (removed)` | DRWUI:842756-761 | **NO CORE COUNTERPART** (core has no 0.1 rule) |
| FAT runner-up within 0.3 | `ΔdSugg ≤ 0.3 && (FStype==2 \|\| FStype==1) → remove first` | DRWUI:842762-768 | **NO CORE COUNTERPART** |
| ~16 MiB size-match promotion | `\|totalSector(+0x60) − ref\| * 0x200 < 0x1000001` (16,777,217 bytes ≈ 16 MiB); first size-matching candidate AFTER ≥1 non-matching predecessor gets removed+logged (`AdjustPartitionInfoOrder …` :842783-787) and **spliced to result #2 with the leader's dSugg copied** (:842845-854); adjacent match breaks the scan (:842779-792) | DRWUI:842776-792, :842842-858 | `TOL = 16*1024*1024/512` sectors + splice `cr_candidates.cpp:1081-1112` ✓ (uses data-area totalSec, N-035) |
| FAT multi-table drop | FS∈{1,2} && `CFATSearchedPartitionInfo` FAT-entry count `((+0xb0)−(+0xa8))>>3 > 1` → removed | DRWUI:842812-821 | **NO CORE COUNTERPART** |
| all-removed fallback | remaining==0 && count>0 → first cand `+0x80=1` | DRWUI:842752-754 | (implicit in core's adjusted list) |
| candidate filter | `+0x7d flag==0 \|\| StartSec(+0x18) ≥ 0` | DRWUI:842568-569 | (n/a) |

### B3. Overlap resolution — `FUN_140510fb0` (DRWUI:842918-843060)

Candidates whose `[StartSec, StartSec+totalSector)` overlaps a claimed region: the one with
**max dSuggestted wins** (:842981-842987 — strict `>` on the double); losers get `+0x80=1`
and are spliced out of the list (:842995-843021); the claimed region is then
shrunk/re-based to the winner (:843023-843039). Core counterpart: same-fs nested dedup
(outer/earlier wins, `cr_candidates.cpp:987-1011`) + containment-in-known drop (M-017
:1013-1049 — replaces the old **−20/−10 overlap penalties**, which were the polarity
inversion of the og's +500) + stable descending sort by score, tie-break structural_points
(:1072-1079).

### B4. Content-consistency scorer (engine-side; `cr_consistency.cpp` og cites in SearchDLL corpus)

| Constant | Value | og cite | Core |
|---|---|---|---|
| probe read size | 512 bytes (`kProbeBytes`) | (SearchDLL walk, core-decoded cite) | `cr_consistency.cpp:67` ✓ |
| reject rule | `considered/2 ≤ filterable && matched/filterable < 0.3 && filterable != 0 → rejected` (decompile `≤0.3 && !=0.3` = strict <) | og :31152-31155 (core-decoded) | `cr_consistency.cpp:175-181` ✓ |
| name-family specials | TIFF/JPEG checkable; JPG/JPEG names match MOV/MP4/JPG families | og :30858-30920 | `cr_consistency.cpp:87-110` ✓ |
| directory skip | attributes & 0x10 | og (iVar14) | `cr_consistency.cpp:124-125` ✓ |
| out-of-bounds invalidation | data_start ≥ volume bytes → immediate reject | og :31077-31083 | `cr_consistency.cpp:138-142` ✓ |
| core-only safety valve | content-probe budget 200,000 progress calls | — | `cr_candidates.cpp:688` (registered divergence) |

---

## C. Progress / ETA math

### C1. Scan-phase display remap — `FUN_1401af820`/`FUN_1401af950`/`FUN_1401afa60` (DRWUI:314931-315030; verified verbatim)

| Piecewise arm (raw %) | Output | og cite | Core |
|---|---|---|---|
| raw < 11 | `raw * 2.0` | DRWUI:314952-953 | `progress_display_remap` `cr_query.cpp:570-591` ✓ |
| 11 ≤ raw < 30 | `(raw − 11.0) * 0.2 + 20.0` | :314948-949 | ✓ :583 |
| 30 ≤ raw ≤ 90 | `raw * 0.8` | :314940-941 | ✓ :576-577 |
| 90 < raw ≤ 98 | `(raw − 88.0) * 2.7 + 72.0` | :314943-944 | ✓ :578-579 |
| 98 < raw ≤ 100 | identity | :314937-939 | ✓ :574 |
| out > 100 && raw != 100 | clamp 99.99 (dead defensive stall cap) | :314955-957 | ✓ :588-589 |
| dispatch type 0 | 0.0 (reset message) | :314976-979 | ✓ :597-598 |
| dispatch type 1 | remap, outer clamp 100.0 | :314980-983 | ✓ :599-603 |
| other types | raw = done*100/total (total<1 → 0.0) | :314970-974 | ✓ :593-605 |
| publish | monotonic max (never decreases) | `FUN_1401afa60` :315014+ | ✓ :608-610 |

### C2. ETA model — `FUN_1401af410` (DRWUI:314835-314887; verified verbatim)

| Constant | Value | og cite | Core |
|---|---|---|---|
| no-estimate gate | done==0 \|\| total<done \|\| total==0 → −1 (0xffffffff) | :314846-848 | `eta_compute` `cr_query.cpp:618-621` ✓ |
| elapsed clock | GetTickCount, stored +0xf8; start +0xf0 = now if 0 or now<start (wrap guard) | :314850-857 | (core takes elapsed_ms param) |
| ETA seconds | `eta_s = (total−done)·elapsed_ms / done / 1000` (integer) | :314858-859 | ✓ :622 |
| rate smoothing | `rate = (rate*2 + prev*8) / 10` — **EMA 0.2/0.8**, prev at +0x118 | :314861-864 | ✓ :625-627 |
| time-budget variant | `projected = total·(elapsed_s)/done`, floor 1; `eta = (budget − budget·elapsed_s/projected) · 0.6` — **0.6 scale**, budget at +0x120 | :314871-879 | ✓ :633-641 |
| display format | `"%02dm%02dds"`; empty-label placeholder literal `"3m00s"` (og :1527664) | (core-decoded cite) | `eta_format_m_s` `cr_query.cpp:645-653` ✓ |
| countdown stop | `minutes*60 − 1 + seconds < 0` | (core-decoded cite) | `eta_countdown_tick` :655-677 ✓ |

### C3. The 40→65 % .rsf save/restore progress (RE-VERIFIED with exact constants)

Writer (`CFileNodeRS::SaveFileTree`): base `local_8c = 0x28` (40) at DRWUI:186486; per
node popped from the BFS queue (DRWUI:186569-576):
```
if percent_so_far < 0x2d (45):  percent = percent_so_far + 1        (ramp, :186571-573)
else:                           percent = (processed * 0x32) / (remaining * 2) + 0x28
                                = 40 + 50·processed/(2·remaining)   (:186575)
```
monotonic publish via `CSaveAndRestoreProgressInfo`/`IUIProgressInfo` vftable-stamped object
(:186577-583); per-child cancel check `param_4->vtable+0x20()` (:186644). **The denominator
is the REMAINING queue count (lVar11 decrements per pop), not total** — the two artifact
forms differ: `SaveScan/rsf-record4--tree-node-grammar.md:98` says `/(total*2)`;
`SaveScan/rsf-format--scan-session.md:367,371-373` says `/(2·remaining)` — **the remaining
form is what the decompile shows**. Reader (restore mirror): same 0x2d ramp then
`percent = (done * 0x32) / (remaining + done) + 0x28` = 40 + 50·done/(total+done)
(DRWUI:186810, :186959-966) → asymptote 90 %. So: **records 1-3 + ramp own 40→45; the tree
walk owns 45→~65 (writer, at processed≈remaining) / →90 (reader); remainder =
compression/finalize.**

| Constant | Value | og cite | Core counterpart |
|---|---|---|---|
| base percent | 40 (`0x28`) | DRWUI:186486/:186810 | **NO CORE COUNTERPART** — core save progress is linear `pm = done*1000/total` (`src/ui/cr_ui.cpp:2821`, N-060 adapter) — see D9 |
| span numerator | 50 (`0x32`) | :186575/:186965 | — |
| ramp threshold | 45 (`0x2d`) +1/node | :186571-573/:186961-963 | — |
| writer denominator | 2·remaining (queue) | :186575 | — |
| reader denominator | remaining+done | :186965 | — |
| completion force | progress forced to 100.0 (double bits 0x4059000000000000) on export completion | `architecture/04-recovery-workflow.md:139-140` | (export path, out of census scope) |

### C4. Other progress/time gates

- 500 pacing: `local_80[0]=500; FUN_14029b880(...)` in the assistant job (DRWUI:605953-957) —
  500 (units: ms presumed) one-shot pacing; **needs lead verification**.
- Preview caps: 15 s video QTimer `QTimer::start(…,15000)` (DRWUI:1507353-1507357, via
  `architecture/05-uiux-architecture.md:123`); "larger than 100MB not available for preview"
  (`UIUX/10-preview-subsystem.md:23`).
- Preview socket wait: 100 ms connect wait; poll 100 × 100 ms = 10 s bound
  (`artifacts/DRWUI-subsystems--assistant-grading-preview.md:73-83`).
- DRWLoader log flush: 5 s (`DRWUI-exe--thread-architecture.md:57`).

---

## D. Capacity / free-space math

| Constant | Value | og cite | Semantics | Core counterpart |
|---|---|---|---|---|
| results size total | `ctx+0x198 += node_size` per node (`FUN_140089170`) | DRWUI:331171-331172 | Σ logical sizes behind "Files Found: 1139286 (147.12 GB)" (`UIUX/dynamic-ground-truth--scan-phase-captured.md:125`) | `Row.size = logical_size` (`cr_query.cpp:417`); total_matched count (:237) — no Σ field (**NO direct counterpart**) |
| recovery totals | `RecoveryCnt:%d RecoveryFileSize:%I64d` (initDataSource) | `artifacts/DRWUI-exe--main-ui-architecture.md:227` (trace) | selected-set count/size totals | (recovery layer, out of scope) |
| removable-disk threshold | 549,755,813,888 = 512 GiB (`nMaxRemovebleDiskSize`) | `qa-logs--session-forensics.md:24` | camera/removable classification threshold | **NO CORE COUNTERPART** |
| sector size default | 512 (`0x200`), force when `(bps & 0x1ff)!=0 \|\| bps==0` | DRWUI:942643-942645 | CSearchedPartitionInfo bps normalization | fallback shim `cr_candidates.cpp:809-811` (DRWUI:904185-187) ✓ |
| 16 MiB tolerance in bytes | `\|ΔtotalSector\|·0x200 < 0x1000001` | DRWUI:842776-778 | size-match promotion (B2) | `TOL = 16 MiB / 512` sectors ✓ |
| size-bucket histogram | `bucket = size/granularity(+0x188) − 1`, capped vs count at +0x50; type 0x11 (17) records max bucket at +0x190 | DRWUI:331041-331069 (`FUN_1401c7780`) | results size-distribution stats | **NO CORE COUNTERPART** |
| free-tier limit | "The free version restores up to 2GB" | `UIUX/30-interaction-driver-ground-truth.md:48` | licensing gate | (UI/licensing layer, out of scope) |
| probe bound | ceiled `sector_count·logical_sector` | (N-206 core note) | addressable bound incl. driver-padded tail | `cr_candidates.cpp:45-49` ✓ |

---

## E. Stats / counters (AssistantStats-style; N-205 rawCount/raw_tail_zero referenced, not re-censused)

og object fields (from the RecoveryAssistant log emitters DRWUI:330913-330952 and the tick
function `FUN_1401c7d80` DRWUI:331183-311):

| Counter | Field | Tick condition (og) | Core counterpart |
|---|---|---|---|
| TotalFileCount | (log :330913) | every file node | `total_file_count` `cr_grading.h:136` ✓ |
| rawCount | +0x1a0 | NTFS(type 3) && overlap==0: grade==2 → tick; else grade==1 && forced probe `FUN_14038c470(node,addr,1,0)` ok && size-rule `FUN_1401c76e0` ok → tick (DRWUI:331272-291) — **N-205 landed**: core `raw_count` = Origin::Carved census; `raw_tail_zero` = og population (`cr_grading.h:126-144`) |
| lostPartitionExistFileCount | +0x1a4 | lost-partition node, non-deleted (:331203-206) | `lost_partition_exist_file_count` (Origin::Lost+Active) ✓ |
| lostPartitionDeleteFileCount | +0x1a8 | lost-partition && grade==2 (:331208-211) | `lost_partition_delete_file_count` ✓ |
| QuickScanExistFileCount | +0x1ac | structured non-deleted (:331232-235) | `quick_scan_exist_file_count` ✓ |
| QuickScanDeleteFileCount | +0x1b0 | structured && grade==2 (:331237-245) | `quick_scan_delete_file_count` ✓ |
| DeepScanFileCount | +0x1b4 | grade==2 && deep/lost branch (:331299-302) | `deep_scan_file_count` (nodes under "Lost" roots) ✓ |
| QuickScanDeleteExtend | +0x1b8 | every NON-existing node (bit FUN_1401b3550==0) (:331164-167); the else → ninth unlogged +0x1bc (:331168-170) | `quick_scan_delete_extend` ✓ (N-183 notes the og's +0x1b8 attribution) |
| category counts | map at ctx+0x40 | per node: type normalized `>9 \|\| ==7 → 0xB` then counted under IDs 5/6/7/8/9/0xB (:331159-162, :331032-952) | `category_counts[8]` (index = Category−1) ✓ |
| elapsed | +0x38 | GetTickCount64 delta at analyze finish (:330908-910) | (telemetry) |
| +0x1c8 | +0x1c8 | FUN_140089020(node)!=0 (:331150-153) — semantics undecoded | **NO CORE COUNTERPART** |
| size sum | +0x198 | += node size (:331171-172) | (D) |
| gauge globals | DAT_141be5818/20/28/30/38/40 | reset :605329-334 (and 5838/5820 again :605423-24); ticks: 5818 @:605899 + :585051; 5820 @:605518; 5828+5830 @:339906-907, :605478-79, :605487-88, :605494-95; 5840 @:605473, :606028; **5838 never ticks (dead accumulator)** | gauge re-derived by member-set walk (`cr_grading.cpp:577-592`) — mechanism differs, see D10 |
| gauge assembly | matched = base + 5838 + 5828 + 5818; total = base + 5820 + 5830 + 5840; clamp matched ≤ total; `total < 100 && flag(+0x10)==0 → matched=0` | DRWUI:340824-847 (`FUN_1401d6ff0`) | `small_result`/`score` (`cr_grading.cpp:555-604`) — population differs, see D7 |

---

## F. Time math

| Constant | Value | og cite | Core counterpart |
|---|---|---|---|
| FILETIME↔unix | `unix = ft/10^7 − 11644473600`; inverse `+11644473600 ·10^7` | (engine layer; core note "cr_fat_quick.cpp's unix*1e7 + 116444736000000000 inverse") | `cr_query.cpp:435-441` ✓ |
| FAT date | `DosDateTimeToFileTime(word[1], word[0])` | DRWUI:942758-769 (`FUN_1405aaa10`) | (fat engine) |
| CRAW timestamp | string `"%d-%d-%d %d-%d-%d"` formatted from FILETIME @+0x1A (`FUN_140c41aa0`) | `SaveScan/rsf-format--scan-session.md:400` | (carve layer) |
| local day floor | FileTimeToLocalFileTime→SystemTime→zero time-of-day→back | DRWUI `FUN_1402556f0`/`FUN_140256830` (core-decoded) | `local_day_floor` `cr_query.cpp:459-465` ✓ |
| day arithmetic | ±N days via tm_mday + mktime (COleDateTimeSpan semantics) | (core-decoded) | `local_day_plus` :471-478 ✓ |
| date buckets | Today; Yesterday; Last-7-days = days today−6..today; Last-1-month = today−29..today; Custom inclusive both ends; IDs 0x101-0x103/0x105, 0x104 skipped | DRWUI `FUN_140705c50` menu (core-decoded) | `apply_date_bucket` `cr_query.cpp:482-518` ✓ |
| assistant elapsed | GetTickCount64 delta | DRWUI:330908-910 | (telemetry) |
| ETA clock | GetTickCount + wrap guard | DRWUI:314850-857 | (elapsed_ms param) ✓ |
| NVMe temp | K − 273, display-only, never graded | DiskHealth :26985-27005 (core-decoded) | `cr_grade.h:28-30` ✓ |
| NVMe life | 100 − percentage_used | same | `cr_grade.h:33` ✓ |
| video preview cap | 15,000 ms QTimer | DRWUI:1507353-1507357 | (UI layer) |

---

## G. Percentage / probability math in result confidence

| Constant | Value | og cite | Core counterpart |
|---|---|---|---|
| dSuggestted | ln(raw)/ln(max) ratio 0..1+ | DRWUI:842164-278 (B1) | `score_ratio` ✓ permille |
| recoverability verdict score | `int((matched/total + 0.05) * 10)`; matched clamped ≤ total; 0 when total==0; log "Recovery ability matched=%d, total=%d" | DRWUI:374451-481 (`FUN_14021b030`) | `out.score` `cr_grading.cpp:597-604` ✓ exact |
| small-result heuristic | total < 100 → matched 0 (flag-gated) | DRWUI:340843-845 | `small_result` :594-596 (population differs, D7) |
| gauge population | 20-format member set {doc pdf mov ppt xls cr2 arw dng cr3 mp4 orf raf docx pptx xlsx heic nef jpeg jpg mp3} + auto-High classes (email + non-existing ReFS/Btrfs) | `.rdata` ctor map (core N-183 evidence dump; DRWUI:605502-513 is the sibling ext-table site) | `kGaugeCandidateExtensions[20]` `cr_grading.cpp:221-223` ✓ |
| consistency ratio | matched/filterable; reject `< 0.3` with `considered/2 ≤ filterable` | og :31152-31155 | `cr_consistency.cpp:176-181` ✓ |
| carve signature confidence | two-decimal fractions 0.51–1.00 per header/tail sign (e.g. `"0,44564456,0.96"`) | `og-codebase/raw_signs.json` (139 ext); `SearchDLL/searchdll-family--architecture.md:68-73` (header+tail accumulation ranks overlapping candidates) | `carve_confidence` 0..1000 permille (`cr_node.h:148`; `cr_signatures.h:467` `base_confidence` 0..1000 before validator delta) — scale ×1000 ✓ |
| NTFS viability (confidence tiers) | og: `0x3ff < size` strict gate → $Bitmap walk; ALL clusters re-allocated (intact==total) → **destroy** (S-001); partial → keep; <1 KiB fast-path keep | NTFSSearchDLL:21208 (verified this session); destroy :21249-21262 (core-decoded); **artifact `ntfs-engine-algorithms-v2--deleted-viability.md:44` transcribes `>=` — off-by-one** | `viability_min_size = 0x3FF` (`cr_mftwalk.h:80`), arms `ratio ≥ 1.0 → Overwritten; > 0 → PartialRisk; else Excellent; no bitmap → PartialRisk; ≤ 0x3FF → Excellent fast-path; torn → PartialRisk` (`cr_mftwalk.cpp:815-865`) ✓ (strict `>` :832) |
| disk-health 4-status | caution counts 5/197/198 = 1/1/1; SSD-life caution ≤ 10 (%); NVMe life >10 Good / ==10 Caution / <10 Bad; temp 50/60 display-only | DiskHealth corpus (core-decoded, `cr_grade.h:8-35`) | `cr_grade.h:94-97,109-114` ✓ |

---

## MATH DIVERGENCE CANDIDATES (og vs core; falsifiable claims)

1. **score_ratio count==0** — og: raw==0 → numerator 0.0 → dSuggestted 0.0 (DRWUI:842244-245,
   :842269-272; log-proven: `CalcPartition.log:199` candidate 6 prints `0.000000`).
   Core: `num = count > 1 ? log(count) : 1.0` → count==0 gives 1.0 (`cr_candidates.cpp:660`).
   Falsifiable: a scan whose every candidate has count 0 → og all-0.0 scores; core yields
   1.0/ln(max) (or score 1000 permille if max≤1). **HIGH confidence — needs lead
   verification only for whether count==0 is reachable via the core's fs_bonus=0 families.**
2. **+500 table-match inside the denominator** — og's max loop has NO bonus walk
   (DRWUI:842210-842230): a table-matched leader prints `ln(raw+500)/ln(raw)` > 1 (e.g.
   1.000295 for raw=168818); core folds +500 into `max_count` (`cr_candidates.cpp:1060-1065`)
   → leader is exactly 1.0 (1000 permille). My log fit (den=ln(168818), residual ≤7e-6 vs
   1.1e-4 for den=ln(169318)) supports the og reading; no QA-log row exercises a match, so
   the core's "verified to 6 decimals" claim never tested this leg. **HIGH — needs lead
   verification via a matched-leader replay.**
3. **per-FS base (+0x68) values** — og: analyser-set, variable (log fits: ≈0-2 in round
   23:12; ≈26 ghost / ≈60 leader in round 00:43); ctor default 1 (DRWUI:942638). Core: fixed
   NTFS 60/30, FAT 20, else 0 (`cr_candidates.cpp:781-794`, OQ-S2M-2 "observed values").
   Falsifiable: replaying `CalcPartition.log` round 23:12 with the core's constants gives
   ghost scores ≈0.509 vs og's 0.503579 (Δ≈0.006, 100× the fit residual). **MEDIUM — the og's
   +0x68 writer rule is undecoded.**
4. **NTFS viability gate transcription** — artifact `ntfs-engine-algorithms-v2--deleted-viability.md:44`
   says `size >= 0x3FF`; decompile says `0x3ff < lVar18` (NTFSSearchDLL:21208); core is
   strict `>` (`cr_mftwalk.cpp:832`, `cr_mftwalk.h:80`). Falsifiable: a 1023-byte deleted
   non-resident file takes the fast-path (no bitmap probe) — artifact doc must be corrected.
   **HIGH.**
5. **Prior artifact grading sketch errors** — `DRWUI-subsystems--assistant-grading-preview.md:40-47`:
   (a) branch polarity inverted (generic path runs for FS **NOT** in {3,10,0xB}, DRWUI:605736-737);
   (b) "type 0xC = ZFS special-case" is wrong — 0xC is the email category (core
   `cr_grading.h:16-18`); (c) "device.kind ∈ {1,4} → offset −2" is an address offset for
   twin keys, not a grade adjustment, and {1,4} = FAT16 + EXT2/3-family (M-133,
   `cr_grading.cpp:137-151`); (d) the {3,10,0xB} "deep validation path" is actually the
   insert hook (A7) + device walk. **HIGH — artifact corrections-register item.**
6. **kDeviceUnit** — core `1ull << 40` stand-in vs the og's undecoded `local_b8[0]` twin-key
   unit (`cr_grading.h:88-92`). Any og unit value < 2^40 that collides across devices would
   change twin pairing. **needs lead verification** (decode the og's vt-derived unit).
7. **Small-result population** — og: `total = gauge base + 5820 + 5830 + 5840` (the gauge
   population, DRWUI:340840-842) with the `+0x10` flag gate (:340843); core: `out.total =
   total_file_count`, unconditional (`cr_grading.cpp:557,594-596`). Falsifiable: a 150-file
   scan with <100 gauge-population members → og forces matched=0 (gauge hidden), core shows
   the gauge. **MEDIUM — needs lead verification of the og base-walk population.**
8. **Second 5818 tick site / 0xa8 insert rule** — og ticks DAT_141be5818 and promotes grade 2
   at the insert path (DRWUI:585040-585051, `vt+0x100 != 0xa8 && grade != 2`); the core
   models only the completion-pass device-index rule (`cr_grading.cpp:363-372`) and
   deliberately skips the 0xa8 arms (A5). Falsifiable: nodes graded by the og's insert rule
   but not by the core's rules (device index ≥ 9, no 0xa8) get og-2 / core-1.
   **MEDIUM — the 0xa8 semantics are undecoded (OQ-S42-3).**
9. **.rsf save progress curve** — og: 40→65 % window with the 0x2d (45) +1/node ramp and
   `40 + 50·processed/(2·remaining)` writer / `40 + 50·done/(remaining+done)` reader
   (DRWUI:186486/186575/186810/186965); core: linear `pm = done*1000/total`
   (`src/ui/cr_ui.cpp:2821`). Falsifiable: observable save-progress curve differs (core hits
   100 % at tree end; og never exceeds ~65 % before finalize). Also the landed artifact
   `rsf-record4--tree-node-grammar.md:98` says `/(total*2)` — should read `/(2·remaining)`.
   **HIGH.**
10. **Gauge mechanism equivalence** — og assembles matched/total from global accumulators
    (three matched-side / three total-side globals, one of which — 5838 — never ticks;
    DRWUI:340837-842) over a base walk; core re-derives by a member-set walk
    (`cr_grading.cpp:577-592`). The 5818/5828 ticks count ALL promoted nodes (not
    member-set-gated), so og matched can exceed the member-set count until the ≤ clamp.
    **needs lead verification** (decode `FUN_14036e900`'s base population + the :339906 tick).
11. **rawCount population** (N-205 landed, reference): og +0x1a0 = validated NTFS probe count
    ANDed with the 0x77fffff arm (DRWUI:331272-291 + :330974-999); core `raw_count` = Carved
    census with `raw_tail_zero` carrying the og population (`cr_grading.h:126-144`). Registered
    divergence — no new finding.
12. **Settings[+0x70] size-rule divisor** — og's non-0x23 arm divides size by an undecoded
    settings field (DRWUI:330989-993); **NO CORE COUNTERPART** (the core only implements the
    0x23/0x77fffff arm, `cr_grading.cpp:404-432`). needs lead verification.
13. **Non-adopted numeric arms (register)** — the og's 0.1 / 0.3 dSugg-difference
    runner-up rules and the FAT multi-table drop (B2) have **NO CORE COUNTERPART**
    (DRWUI:842756-768, :842812-821); the core's containment-drop + 16 MiB splice covers the
    same observed log behavior. needs lead verification whether the QA log's "result" lines
    require them.
14. **Progress/ETA and gauge-score math: NO divergence** — the piecewise remap
    (2.0/0.2+20/0.8/2.7+72/99.99), ETA EMA (2r+8p)/10 and 0.6 budget scale, the verdict
    score `int((m/t+0.05)·10)`, date/size buckets, and the deep-read 32-byte/0xFF/16-byte
    signatures all match the og verbatim (`cr_query.cpp:570-677`, `cr_grading.cpp:232-296,
    597-604`). The core's own note that its earlier test table mis-transcribed four remap
    outputs (raw 20→21.8 etc., `cr_query.cpp:562-569`) is resolved — corpus constants stand.
15. **Mission-premise note** — there is no og "Excellent/Good/Average/Poor/Very Bad"
    5-tier recoverability scale; the og ships a 3-grade (0/1/2) model plus a 4-status
    disk-health model. The core's 5-tier `Confidence` (cr_node.h:57-63) and 3-grade
    `Grade` (cr_node.h:71) split mirrors this correctly; any parity test expecting 5 og
    recoverability tiers is testing a phantom.
