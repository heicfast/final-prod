# Lane E — DETECTION ACCURACY & CALCULATION MATHEMATICS (deep-dive)

**Auditor:** lane E (read-only). **Date:** 2026-10-29.
**Baselines:** core `/home/z/audit/core_recovery` @ 8d0d88b8 · corpus `/home/z/audit/easeus-drw-research` @ be0cfe9.
**Scope guard:** carve-validator grammars (A4/S-007/M-106/M-107/M-112/N-146/N-152 stand-ins), NTFS 0x3FF
bitmap rule + base-record skip + two-pass/destroy (S-012/D-51/Q2), SCAN-ENGINE-DELTA rows Q1–Q11/A1–A6/
M1–M5/F1–F7 — all COVERED, not re-filed. Every claim below was read first-hand at the cited lines.

Cite shorthand: `NTFS:` = `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` (same style
for `FAT:`, `exFAT:`, `EXT4:`, `HFS:`, `Btrfs:`, `ZFS:`, `ReFS:`); `core:` = core_recovery tree.

---

## 1. CONTEXT SHEET

### 1.1 Filesystem-level detection accuracy (beyond the filed findings)

| # | Predicate / threshold | Og (cite) | Core (cite) | Verdict |
|---|---|---|---|---|
| 1.1.1 | **FAT deleted-entry marker** `first byte == 0xE5` (signed `-0x1b`), 6 sites; entry KEPT, 64-KiB chain guard `< 0x10000` clusters | FAT:11741-11742 `*pcVar21 == -0x1b`, :32155 `*pcVar13 == -0x1b` → `vt+0x70(props,1)` SetDeleted; corpus fat-engine--deep-dive.md:52-55 (sites 11742, 14634, 14825, 29978, 30178, 32155) | cr_fat_wire.h:80 `DENTRY_DELETED=0xE5`; cr_fat_quick.cpp:178 `deleted = (first == DENTRY_DELETED)`; chain cap 0x10000 cr_fat_quick.cpp:32,263 | **MATCH** (covered: fat.md B4; guard covered Q3) |
| 1.1.2 | **FAT deleted-name first-char convention: og substitutes `#` (0x23)** for the 0xE5 byte in the 8.3 builder | FAT:44166-44172 `uVar8=0x23; cVar2=name[0]; if (cVar2==-0x1b) cVar2='#';` inside FUN_180036350 (the 8.3 builder, called for every LFN-less entry at :32099) | cr_fat_quick.cpp:193-203: checksum-bijection recovery of the ORIGINAL byte, else `short_name[0]='_'` | **DIVERGENT → LE-1** |
| 1.1.3 | **FAT deleted-first-char recovery from LFN checksum oracle** — og has NO bijection/recovery machinery (the 8.3 builder unconditionally substitutes `#`; no 256-candidate search anywhere in the DLL) | FAT:44166-44172 (substitution is the only 0xE5 treatment on the name path) | cr_lfn.cpp:76-111 `lfn_deleted_checksum_recover` (GAP-S6-1, landed 9fc82b0) | **DIVERGENT (core superset) → LE-1** |
| 1.1.4 | **FAT LFN acceptance**: LFN wins when assembled; an LFN containing any of `? * \| \ / : " < >` is DISCARDED → 8.3 fallback; illegal-char set is a 9-byte strncpy | FAT:32084-32099 (`strncpy(local_538,"?*|\\/:\"<>",9)` + scan over the LFN; hit → LAB_180028885 → FUN_180036350 8.3 path) | cr_fat_quick.cpp:204-216: `lfn_assemble` (checksum-validated) else 8.3; **no illegal-char gate on the assembled LFN** | **DIVERGENT → LE-6** |
| 1.1.5 | **FAT 8.3 NTRes case folding**: NTRes byte (+0x0C) {0x00→UP/UP, 0x08→lower/UP, 0x10→UP/lower, 0x18→lower/lower} via `_strupr/_strlwr` before `%s.%s` join | FAT:44191-44216 (`cVar2==0`→strupr/strupr; `'\b'`(0x08)→strlwr/strupr; `0x10`→strupr/strlwr; `0x18`→strlwr/strlwr; else raw) | `nt_reserved` field exists (cr_fat_wire.h:89) but is **never read**; `short_name_to_u16` returns raw bytes (cr_lfn.cpp:144-164) | **DIVERGENT → LE-5** |
| 1.1.6 | **FAT dentry shape-validator family** (unconditional, FUN_1800365a0): (a) LFN gate attr==0x0F ∧ first_cluster(+0x1A)==0 → valid; (b) volume-label attr&8 → reject (attr∈{8,0x28} special-cased); (c) ext-tail byte > `'?'`(0x3F) → reject; (d) file: `(chain≠0) == (size(+0x1C)==0)` → reject (size/chain XOR consistency, param_2=1 call at :42948); (e) dir: `size≠0 ∧ first_cluster_high(+0x14)≠0` → reject; (f) **byte-histogram corruption gate**: any byte value appearing >9 times in the 21-byte window +0x0B..+0x1F of the paired entry → reject; a failed validation STOPS the walk at several call sites (break = end-of-dir) | FAT:44263-44375 (gates a-f at :44286-89, :44301, :44295-96, :44344-48, :44350, :44356-69; break-on-fail at :8792-8793) | Only the date-sanity sub-gate (month>12 ∨ day==0, config-gated, default OFF) was ported: cr_fat_quick.cpp:228-245 (`strict_date_sanity`, N-010c/D-51 delta 2). **No histogram/dir-gate/size-XOR/ext-byte gates anywhere** | **DIVERGENT → LE-7** |
| 1.1.7 | FAT date-sanity dentry gate (config-gated singleton+0x40): reject create(+0x10)/write(+0x18)/access(+0x12) date with month∉[1,12] ∨ day==0, whole-zero date passes; **~N numeric-tail rescue**: bad access-date dentries with `~` at name+6 and tail digits parsing 1..5 still accepted | FAT:44305-44339 (`(uVar6&0x1e0)-1 > 0x17f` = month raw-field 384 → month 13+; `(uVar6&0x1f)==0` day 0; `~`-rescue :44320-44339, `4 < iVar7-1U` reject) | cr_fat_quick.cpp:228-245 (predicate MATCH); the ~N rescue not ported (gate default OFF both sides) | **COVERED-BY-N-010c** (rescue nuance noted; the og's own gate is config-gated — same default-off posture) |
| 1.1.8 | **exFAT deleted dentry validation**: type dispatch accepts BOTH the deleted (bit-7 cleared) and in-use forms — `case 5: case 0x85:` (File), 0x81/0x82 (bitmap/upcase), 0x83 (label); deleted sets additionally gated by the dual-form SetChecksum (M-013) and the M-014 structural gates; raw+0x0D InUse==0 → `vt+0x70(props,1)` SetDeleted | exFAT:25078-79 `case 5: case 0x85:`; :25117-18 `case 0x81: case 0x82:`; :25063 `case 0x83:`; :45199-45201 `if (*(char*)(param_2+0xd)=='\0') vt+0x70(1)` | cr_exfat_wire.h:49-58 (`ENTRY_IN_USE=0x80`, `TYPE_FILE=0x05` etc., `entry_type()` masks 0x80); M-013 parked/oracle + M-014 gates landed | **COVERED-BY-M-013/M-014/exfat.md-B4** (constants re-verified MATCH — see §1.6) |
| 1.1.9 | **exFAT `._`-prefix sink filter**: the props builder refuses to build (returns 0, file dropped) when the name starts `"._"` — the macOS AppleDouble sidecar filter | exFAT:45151 `strncmp((char*)(param_2+0x29),"._",2)`; `iVar1==0` → skip build → `return 0` (:45222). Single site in the whole DLL; **no corpus artifact, no core counterpart** | cr_exfat_quick.cpp:555-800 — no name-prefix filter anywhere in the walk | **DIVERGENT → LE-4** |
| 1.1.10 | **exFAT dentry timestamp validity gates** (M-014): secondary_count∈[2,18]; day≠0 ∧ month∈[1,12] ∧ hour≤23 ∧ min≤59 ∧ 2s≤29 — an all-zero stamp FAILS; failing head skipped without parking | exFAT (FUN_180002f90, per exfat.md M-014 decode) | cr_exfat_quick.cpp:567-571 `file_entry_structural_ok` (landed s35) | **COVERED-BY-M-014** |
| 1.1.11 | **NTFS deleted-viability rules** (0x3FF strict `<`, `intact==total → destroy`, base-record skip, two-pass, INDX sweep, USN gate, rescue chain) | NTFS:2997/:3018 (`0x3ff < size`); corpus ntfs-engine-algorithms-v2:47-57 | cr_mftwalk.cpp:787/803 (`probe_size > flags.viability_min_size`, `viability_min_size=0x3FF` cr_mftwalk.h); two-pass S-012 | **COVERED** (S-012/D-51/S-022/N-039/OQ-S4 — per lane instructions, not re-filed) |
| 1.1.12 | **ext4 orphan/deleted detection (dtime semantics)**: the og NEVER reads `i_dtime` (+0x14) or `i_links_count` (+0x1A) — its deleted surface = journal remnants + stale merges + inode-image sizes; the core's `dtime≠0 ∨ links==0` predicate + dtime sweep is the registered superset | EXT4: no `+0x14`/`+0x1a` inode-field reads anywhere (props builder reads only +0x0C/+0x10 times, +0x04/+0x6C size: :6261-6272, :5464) | cr_ext4_engine.cpp:1005-1006 (`child.dtime != 0 ∥ child.links == 0` → Deleted), :1073 (sweep skip) | **COVERED-BY-I-014** (absent-reads evidence now first-hand) |
| 1.1.13 | **HFS+ deleted records**: journal differential harvest (GetDeleteRecord) is the og's primary deleted surface; core's freed-node sweep = registered superset; catalog link-pointer mechanics undecoded in corpus (B OQ-3 resolved as journal+superset) | HFS: journal decode per hfsplus.md S-021 | cr_hfsplus_engine.cpp `journal_harvest_pass` (S-021 FIXED) | **COVERED-BY-S-021/D-93/I-014** |
| 1.1.14 | **Btrfs deleted items in tree roots**: newest-generation tree only (og rule); core deep-walk superset | Btrfs (D-06 closed) | cr_btrfs_engine.cpp (I-003) | **COVERED-BY-D-06/I-003** |
| 1.1.15 | **ReFS metadata staleness**: stale-`MSB+`-page sweep (pages unreachable from the live checkpoint chain) force-marked deleted + row-flag 0x4 + allocator grading; grade = 2-bit byte at node+0x70 (2=deleted) from record+0x59 | ReFS: FUN_180005330 (:2819-3054); grade decode ReFS:8476/:7137-7159 | cr_refs_engine.cpp phase 8b (M-026 FIXED); grading I-032 | **COVERED-BY-M-026/I-032** |
| 1.1.16 | **ZFS deleted**: every ring-txg state walked newest-first, identity dedup | ZFS (S-034 decode) | cr_zfs_engine.cpp (S-034 FIXED) | **COVERED-BY-S-034** |

### 1.2 Timestamp calculation mathematics

| # | Item | Og (cite) | Core (cite) | Verdict |
|---|---|---|---|---|
| 1.2.1 | **NTFS FILETIME**: raw 100ns-1601 qwords copied verbatim from $SI/$FILE_NAME into props slots; NO conversion, NO rounding; $SI overrides $FILE_NAME when valid; og persists 3 times (created/modified/changed — no accessed) | NTFS:5338-5350 ($FN times then $SI override via FUN_180045090/5040/4ff0); 3-slot persistence per ntfs.md B9 | cr_mftwalk.cpp:734 (`props->times = has_si ? si_times : best->times`); raw qwords in cr_ntfs_wire.h:157-188; FileTimes u64 (cr_types.h:45-50) | **MATCH** (B9 covered; the 4-vs-3 slot note registered) |
| 1.2.2 | **FAT date/time decode**: `DosDateTimeToFileTime(date,time,&ft)`; failure → props slot zeroed (= 1601-01-01). Mapping: props+0x25=create(CrtDate@+0x10,CrtTime@+0x0E), +0x2d=modified(WrtDate@+0x18,WrtTime@+0x16), +0x35=accessed(LstAccDate@+0x12, time=0). `DosDateTimeToFileTime` validates month 1-12, day 1-31, hour≤23, min≤59, sec≤59 (SYSTEMTIME ranges) | FAT:32131-32154 (three calls + `if (BVar5==0) *slot=0` ×3) | cr_fat_quick.cpp:347-354 `fat_to_filetime`; `fat_date_to_unix`→`fat_datetime_to_unix_seconds` validates ONLY month∈[1,12] ∧ day∈[1,31] (cr_fat_wire.h:123-142) — **no hour/minute/second range validation**; zero-on-invalid ✓ (N-021) | **DIVERGENT (time-word components) → LE-2**; date-word + zero-fallback = COVERED-BY-N-021/N-010c |
| 1.2.3 | **FAT create 10ms tenths (+0x0D)**: og NEVER reads it (DosDateTimeToFileTime takes date+time only) | FAT:32131-32154 (no +0x0D read on the time path) | `fat_date_to_unix(..., tenths)` discards tenths (`(void)tenths`, cr_lfn.cpp:166-175) | **MATCH** (fat.md B9 "tenths discarded" re-verified) |
| 1.2.4 | **FAT 2107 wrap (year field 126/127)**: og converts fully (SYSTEMTIME year ≤ 30827) → correct 2106/2107 FILETIMEs | FAT:32135+ (DosDateTimeToFileTime — no 2038/2106 ceiling) | cr_lfn.cpp:173-174 `secs > 0xFFFFFFFFULL ? 0` — **FAT dates after 2106-02-07 06:28:15 collapse to the zero (invalid) FileTime**. exFAT path has NO such clamp (cr_exfat_quick.cpp:190-200) — core-internal inconsistency | **DIVERGENT → LE-3** |
| 1.2.5 | **exFAT timestamps**: raw date/time words only; **NEITHER the 7-byte-extra 10ms fields NOR the tz bytes are read** (six DosDateTimeToFileTime call sites, (date,time,&ft) only) | exFAT:45174-45194 (create/mod/access triple) and :5215-5235 (twin builder; raw ts @+0x10/+0x14/+0x18 = time-lo/date-hi) | `exfat_time_to_unix100(ts)` = shared FAT-family core, no 10ms, no tz (cr_exfat_quick.cpp:174-200; S-011/D-49) | **MATCH** (tz = COVERED-BY-D-49; 10ms non-use newly verified first-hand) |
| 1.2.6 | **exFAT invalid stamp**: same zero-on-failure (`BVar8==0 → slot=0`) | exFAT:45177-45194 ×3 | exfat_time_to_unix100 returns 0 on `date==0` or invalid (cr_exfat_quick.cpp:193-198) | **MATCH** |
| 1.2.7 | **ext4 seconds**: og reads ONLY ctime(+0x0C)/mtime(+0x10), `_gmtime32_s` + `wMilliseconds=0` (second granularity); **post-2038 seconds (u32 > 0x7FFFFFFF) make `_gmtime32_s` fail → uninitialized SYSTEMTIME → garbage/zero times** (og defect class) | EXT4:6261-6282 | cr_ext4_engine.cpp:122-127 `decode_extra` (kernel-exact: epoch bits `sec += epoch<<32`, nsec `(extra&~3)>>2`), 4 timestamps, i64 seconds — correct through 2106 | **COVERED-BY-I-006/I-007** (core superset); the post-2038 og failure = og defect not adopted (bench caveat, no core action) |
| 1.2.8 | **HFS+ (1904 epoch)**: `(mac_sec + 0x239eae080) * 10000000` — local-time-carried-as-is (zero TZ imports; the only time API is `GetSystemTimeAsFileTime` for the scan stamp) | HFS:8012/:8027/:8042 (`0x239eae080` = 9561628800) | cr_hfsplus_engine.cpp:549-551 `mac_to_filetime` — identical formula; zero date → 1904-01-01 on BOTH sides | **MATCH** (I-007 breadth note; one nuance: core `changed = attr_date ? attr_date : mod_date` at :1165 vs og's raw attr_mod → 1904-01-01 when attr_date==0 — cosmetic, noted) |
| 1.2.9 | **Btrfs timespec**: `(sec + 0x2b6109100) * 10000000 + nsec/100` — nsec PRESERVED at 100ns granularity (ctime/mtime/atime; otime dropped) | Btrfs:4430-4446 (FUN_180006ba0; `*(uint*)(p+0x84)/100` etc.; 0x2b6109100 = 11644473600) | cr_btrfs_engine.cpp:874-880 via `unix_to_filetime_ns(sec, nsec)` = `(sec+11644473600)*1e7 + nsec/100` (cr_types.h:56-61) — IDENTICAL arithmetic | **MATCH** (I-007 otime note) |
| 1.2.10 | **ZFS**: ZPL times persisted at 8-byte sec-only precision (no nsec anywhere in the og) | ZFS: FUN_180010ef0 family (`< 9` byte cap) | cr_zfs_engine.cpp:1520-1624 seconds via `unix_to_filetime` | **COVERED-BY-I-032** (closed MATCH) |
| 1.2.11 | **ReFS**: raw 100ns-1601 FILETIMEs stored verbatim | ReFS (refs.md B9 M) | cr_refs_engine.cpp (B9 M) | **COVERED** (refs.md B9) |
| 1.2.12 | **Timezone semantics**: og stores raw in session; converts to local ONLY at DRWUI display (`FileTimeToLocalFileTime`, DRWUI:412990-413010) | DRWUI cites per D-49 | Core stores raw 100ns in CRSF session + query surfaces (FileTimes u64 — full precision, same raw units the og persists) | **MATCH / COVERED-BY-D-49** |

### 1.3 Size / calculation mathematics (beyond the filed stand-ins)

| # | Item | Og (cite) | Core (cite) | Verdict |
|---|---|---|---|---|
| 1.3.1 | **FAT size field**: full u32 at dentry+0x1C (NO 28-bit mask) — the "28-bit size" hypothesis is disproven | FAT:32129 `props+0x1b = (u64)*(u32*)(dentry+0x1c)` | cr_fat_wire.h:98 `u32 file_size`; cr_fat_quick.cpp:338 `logical_size = e.file_size` | **MATCH** (mission hypothesis disproven — evidence recorded) |
| 1.3.2 | **FAT size-vs-chain precedence**: dentry size wins in metadata; recovery writes min(entry size, chain/extent coverage) | corpus fat-exfat-engine-internals.md:82 "size = min(entry size, chain walk)" | cr_fat_quick.cpp:338 (dentry size in props); cr_safewriter.cpp:405-423 (`total = logical_size`, loop stops when extents exhausted → effective min()) | **MATCH** |
| 1.3.3 | **FAT dir size**: og sets size from the dentry for both files and dirs (FAT dir dentries carry 0); the "File overflows partition!" guard releases chains whose cumulative size exceeds the volume (FAT:10901-10909) | FAT:32129 (unconditional); :10869-10909 | cr_fat_quick.cpp:338 `is_dir ? chain_bytes : file_size` (dir = chain-derived) — diverges only when the chain fails (no-FAT dir → 1 cluster vs og's 0) | **MATCH (benign dir-edge note)** |
| 1.3.4 | **NTFS $DATA sizes**: real size (@+0x30) is authoritative; $FILE_NAME real_size only as fallback; allocated carried separately | NTFS session-2 MATCH (ntfs.md:269) | cr_mftwalk.cpp:129-130 (`data_real_size = a.actual`), :866 (`logical = data_real_size ?: best->real_size`), :867 (`allocated = best->allocated_size`) | **COVERED (session-2 MATCH re-cited)** |
| 1.3.5 | **ext4 i_size**: og reads the FULL 64-bit size **unconditionally** (`i_size_lo@+4 ∥ i_size_high@+0x6C << 32`, FUN_180003590) — for directories +0x6C is legacy `i_dir_acl`, so a legacy-ACL dir shows a giant size in the og (og defect); core applies the high word to regular files only (kernel rule) | EXT4:2372-2379 (FUN_180003590); :5464 (size∥0x6C use in FUN_180007510) | cr_ext4_engine.cpp:404-407 (`if (ifmt(mode)==0x8000) size |= high<<32`) | **MATCH-correct (og defect NOT adopted — register alongside D-58/D-59)** |
| 1.3.6 | **ext4 inline-data flag**: neither side decodes inline-data content (no EXT4_INLINE_DATA_FL handling in the og's reads; core likewise) — shared limitation | EXT4 (absent) | cr_ext4_engine.cpp (absent) | **NOT-APPLICABLE** (shared limitation, MATCH-by-omission) |
| 1.3.7 | **ext4 size-vs-blocks damaged flag**: og sets props FLAGS bit 0x200 when `i_blocks_lo@+0x1C < size>>9` (file whose size exceeds its allocated blocks; serialized in .rsf) | EXT4:5455-5467 (FUN_180007510) + :6300-6303 (`props+0x39 |= 0x200`, files only) | **No counterpart** — the core never compares size against block count (grep-proven; flags field carries only bit 4 internal, cr_node.h:85) | **DIVERGENT → LE-8** |
| 1.3.8 | **exFAT size**: stream dentry `data_length` for files (raw+0x20 → node+0x58); no-FAT chains resolved contiguous | exFAT:45197 (`node+0x58 = param_2[4]` = raw+0x20) | cr_exfat_quick.cpp:789-790 (`logical_size = is_dir ? chain_bytes : data_len`) | **MATCH** (dir-edge note as 1.3.3) |
| 1.3.9 | **FAT/exFAT/NTFS min-at-recovery**: verified both sides (1.3.2; NTFS: writer stops at extent exhaustion same model) | corpus :82 | cr_safewriter.cpp | **MATCH** |

### 1.4 Grading / classification mathematics

| # | Item | Og (cite) | Core (cite) | Verdict |
|---|---|---|---|---|
| 1.4.1 | **NTFS `intact==total → destroy`** (and its `total>0`, run-START probe, sparse exclusion) | corpus ntfs-engine-algorithms-v2:47-57 | cr_mftwalk.cpp (S-012) | **COVERED (S-012/D-51 — not re-filed)** |
| 1.4.2 | **FAT/exFAT engine condition byte** — NEW DECODE: a per-node "condition" at `CFileNode+0x71`, persisted in the session stream (deserializer read at FAT:33131); values observed {0, 3, 4}; the adjustment rule on the reconstruction/inferred path: **if condition == 3 → bump node+0x73 band to 0x40 (when `(+0x73 & 0xe0) < 0x40`); else condition = 4** — applied when the chain-resolution out-flag is set AND the run list has > 1 run (fragmented). Same pattern in both engines | FAT:11717-11725, :14965-14973; exFAT:45207-45215; persistence FAT:33126-33131 (stream read into +0x71); node ctor leaves +0x71 uninitialized (FAT:32832-32862) → initial seed set elsewhere (undecoded) | `tree::Confidence` {Excellent, PartialRisk, Overwritten}: `grade_deleted_chain` (cr_fat_quick.cpp:92-102): empty→Overwritten; cycle∥broken→PartialRisk; `len ≥ expect`→Excellent; `len*2 ≥ expect`→PartialRisk; else Overwritten; + honesty caps (first-cluster FREE ∥ inferred → never Excellent, :420-428). Registered superset (D15); DRWUI-side grading parity landed (S-023) | **COVERED-BY-S-023/D15** for the surface; the +0x71 decode is NEW EVIDENCE for open question OQ-S2M-1 (see §3.1) — the og's exact initial-condition inequality remains undecoded |
| 1.4.3 | **DRWUI CRecoverability grading** (size<1→0; ZFS→2; device.kind {1,4}→−2; grades 0/2; monotonic-max sticky) | C5 row (SCAN-ENGINE-DELTA) | S-023 landed (cr_grading.cpp) | **COVERED-BY-S-023** |
| 1.4.4 | **Carve grading** (JPEG PartialRisk for salvaged no-EOI, S-018/D-79) | — | — | **COVERED-BY-D-79** |

### 1.5 Multi-volume / cross-partition math

| # | Item | Evidence | Verdict |
|---|---|---|---|
| 1.5.1 | Engine-level cross-volume runs | Og engines are handed a partition-scoped `CPartitionVirtualVolumeDevice` (SCAN-ENGINE-DELTA M2); all run/cluster coordinates (LCN, cluster#, block#) are volume-local by construction — a run cannot address another volume. Zero cross-volume arithmetic exists in any search DLL (read pass over the walk functions shows volume-relative math only, e.g. FAT:10846-10920 chain math against `param_1+0x48` volume extent). Core: engines run on `IStorageDevice` slices (cr_scan.cpp:155-177 `build_read_pipe(slice)`); cross-volume math exists ONLY in the LDM span layer (cr_safewriter.cpp:23-75 `LdmBasisTable::spans`, S-052 FIXED) | **NOT-APPLICABLE at the engine layer (evidence above)**; the LDM-span surface = COVERED-BY-S-052 |
| 1.5.2 | ZFS multi-vdev DVA resolution | OQ-T6 resolved MATCH (I-032/M-019) | **COVERED-BY-I-032** |

### 1.6 Numeric-literal sweep (both sides, one by one)

| Constant | Og | Core | Verdict |
|---|---|---|---|
| NTFS viability `0x3FF` strict `>` | NTFS:2997/:3018 | cr_mftwalk.h viability_min_size / cr_mftwalk.cpp:787 | **COVERED (Q2 — not re-filed)** |
| FAT12 EOC/BAD `≥0xFF8` / `==0xFF7` (raw 4087 BAD) | corpus deep-dive §3 (:8035-8060) | cr_chain.cpp:47-50 | **MATCH** (I-002 registered raw-BAD delta) |
| FAT16 EOC/BAD `≥0xFFF8` / `==0xFFF7` | same | cr_chain.cpp:58-61 | **MATCH** |
| FAT32 EOC/BAD `≥0x0FFFFFF8` / `==0x0FFFFFF7` (+ mask-24 alt family `(raw&0x07FFFFF8)==0x07FFFFF8`) | same | cr_chain.cpp:70-84 | **MATCH** (N-002 FIXED + single-entry approximation registered) |
| FAT32 cluster-count reject `≥0xFFFFFF0`; bps 32768 ceiling; numFATs {1,2}; SPC domain bitmask `0x800000008000808b` (spc∈{1,2,4,8,16,32,64,128}); FAT16 root ≥224 ∧ (root×32)%bps==0; spf≠0 | FAT:38048-38089 (FUN_18002f970) | cr_chain.cpp:183-231 (M-015 media 0xFFFFFFF8 exact), cr_fat_engine.h gates; `SpcDomainMatchesOgSet` pin | **COVERED-BY-N-010b/N-034/M-045/M-011/M-015** (re-verified at FUN_18002f970) |
| FAT media-word exact pair `0xFFFFFFF8` (FAT16) | FAT (M-015 decode) | cr_chain.cpp:195-201 | **COVERED-BY-M-015** |
| exFAT EOC family `≥0xFFFFFFF8`, BAD `==0xFFFFFFF7`, FREE `==0`; first-cluster gate `==1 ∥ ≥0xFFFFFFF7` invalid | exFAT:1857-59 (per exfat.md B1) | cr_exfat_quick.cpp:18-20, :714-720 | **MATCH** |
| exFAT dentry types 0x81/0x82/0x83/0x85/0xC0/0xC1 (InUse 0x80; deleted forms 5/0x40/0x41 accepted) | exFAT:25063, 25078-79, 25117-18 | cr_exfat_wire.h:49-58 + `entry_type()` mask | **MATCH** |
| exFAT cluster-count max 0xFFFFFFF5; boot region 24 sectors; backup LBA 12 | spec/corpus (exfat.md B1) | cr_exfat_wire.h:30, 161-162 | **MATCH** |
| NTFS attr flags COMPRESSED 0x0001 / ENCRYPTED 0x4000 / SPARSE 0x8000; record flags IN_USE 0x0001 / DIR 0x0002; attr types 0x10/0x20/0x30/0x80/0x90/0xA0 | NTFS (ntfs.md C19) | cr_ntfs_wire.h:78-79, 87-108 | **MATCH** |
| NTFS record magic 0x454C4946, OEM "NTFS    ", 0xAA55 | NTFS:21152 | cr_ntfs_wire.h:25-26, 57 | **MATCH** |
| NTFS $DATA size fields: real@+0x30, initialized@+0x38, allocated@+0x28 | NTFS (session-2) | cr_ntfs_wire.h:130-146 (NTFS-7 field order) | **MATCH** |
| ext4 SB magic 0xEF53@+0x38; block size `0x400 << (log&0x1F)`; GDT 32/64B | EXT4:3223/:3304 | cr_ext4_engine.cpp:135-158 | **COVERED (Q5/ext4.md B1)** |
| HFS+ sigs "H+"/"HX", blockSize&0x1FF==0, version<0x11, epoch 0x239eae080 | HFS:10005-10010, :8012-8042 | cr_hfsplus_engine.h/cpp; cr_hfsplus_engine.cpp:549-551 | **MATCH** (N-005 registered bounds superset) |
| Btrfs SB magic 0x4D5F53665248425F@+0x40; epoch-delta 0x2b6109100; nsec/100 | Btrfs (btrfs.md C19; :4430-4446) | cr_btrfs_engine.cpp; cr_types.h:56-61 | **MATCH** |
| ZFS uberblock magic 0x00BAB10C, txg@+0x10 | ZFS (zfs.md C19) | cr_zfs_engine.cpp | **COVERED (zfs.md B1)** |
| ReFS page sigs "SUPBMSB+ReFSFSRS" etc., version gates 1.x/3.4+ | ReFS:11689-11720 | cr_refs_detect.cpp / cr_refs_engine.cpp:152 | **COVERED (N-007/N-026/I-004)** |
| **FAT deleted 8.3 substitution char `#` (0x23)** | FAT:44166-44172 | (absent — core uses '_' or recovered byte) | **DIVERGENT → LE-1** |
| **FAT illegal-name char set `?*|\/:"<>`** | FAT:32084 | (absent) | **DIVERGENT → LE-6** |
| **FAT NTRes case-fold values {0x00,0x08,0x10,0x18}** | FAT:44191-44216 | (absent) | **DIVERGENT → LE-5** |
| **FAT dentry histogram gate `>9` repeats / 21-byte window @+0x0B** | FAT:44356-44369 | (absent) | **DIVERGENT → LE-7** |
| **ext4 damaged flag bit 0x200 (`i_blocks < size>>9`)** | EXT4:5455-5467, 6300-6303 | (absent) | **DIVERGENT → LE-8** |

---

## 2. DIVERGENCE CANDIDATES

### LE-1 — FAT deleted-entry name convention: og `#` substitution vs core `_` placeholder + checksum-bijection recovery
**Severity: SEVERE** (user-observable output divergence on the whole deleted-FAT-without-usable-LFN population; names differ in results, sessions, and restored files)
- **Og:** the 8.3 name builder FUN_180036350 substitutes `'#'` for the 0xE5 deleted first byte —
  FAT:44166-44172 (`cVar2 = (char)name[0]; if (cVar2 == -0x1b) cVar2 = '#';`). This builder is the name
  source for every LFN-less entry (called at FAT:32099). There is NO recovery of the destroyed byte anywhere
  in the DLL (the substitution is the only 0xE5 name treatment).
- **Core:** cr_fat_quick.cpp:184-203 — when surviving LFN slots exist, `lfn_deleted_checksum_recover`
  (cr_lfn.cpp:76-111) brute-forces the destroyed byte through the stored checksum and RESTORES the original
  first character (and validates the LFN against it); otherwise `short_name[0] = '_'`.
- **Concrete differing input:** deleted FAT16 entry, name bytes `E5 45 41 44 4D 45 20 20 44 41 54`
  ("?EADME.DAT"), no LFN slots → og result name `#EADME.DAT`; core result name `_EADME.DAT`. With intact
  LFN slots + checksum oracle → og `#EADME.DAT` (8.3 fallback; see open question Q1), core `readme.txt`
  (original LFN recovered).
- **Note:** closes OQ-T2's "cosmetic first-char question" (fat.md B4): the og's convention is now DECODED.
  The core's bijection recovery is a quality superset but a parity divergence; benches comparing name sets
  must normalize or the recovery must be flag-gated.

### LE-2 — FAT time-word component validation: core accepts hour≥24 / min≥60 / 2s-field≥30 and emits shifted timestamps; og zeroes the slot
**Severity: ACUTE** (wrong timestamps in OUTPUT data; input class = anomalous/torn dentries, common on the recovery target population) — same fix family as N-021 (one-line guard).
- **Og:** `DosDateTimeToFileTime` (FAT:32135/:32144/:32151, exFAT:45174-45194) unpacks to SYSTEMTIME and
  FAILS on hour>23, minute>59, second>59 → the caller zeroes the props slot (`if (BVar5==0) *slot=0`,
  FAT:32137-38/:32146-47/:32152-53) → zero FileTime (1601-01-01).
- **Core:** `fat_datetime_to_unix_seconds` (cr_fat_wire.h:123-142) validates ONLY month/day;
  `hour = (time>>11)&0x1F` (0..31), `minute = (time>>5)&0x3F` (0..63), `second = (time&0x1F)*2` (0..62)
  flow unvalidated into the seconds math. FAT path affected for all three stamps (created/modified; accessed
  passes time=0 so only its date matters). The exFAT path is protected upstream by the M-014 dentry gate
  (hour≤23 ∧ min≤59 ∧ 2s≤29 rejects the whole set), so this is FAT-specific.
- **Concrete differing input:** creation_time word `0xC000` (hour bits = 24, min 0, sec 0), valid date →
  og: created = 0 (1601-01-01, honest invalid); core: created = (date+1 day) 00:00 — a plausible, WRONG
  timestamp. Same class: minute field 60-63, 2s-field 30-31 (60-62 s).

### LE-3 — FAT 2106-02-07+ / 2107 dates collapse to the zero FileTime in the core (u32 clamp)
**Severity: MINOR** (edge-year class; legal FAT dates)
- **Og:** DosDateTimeToFileTime handles year ≤ 2107 (field 127) correctly — full FILETIME output.
- **Core:** cr_lfn.cpp:173-174 `return secs > 0xFFFFFFFFULL ? 0 : u32(secs);` — unix seconds past
  0xFFFFFFFF (2106-02-07 06:28:15) return 0 → the caller's zero-path emits the invalid (1601-01-01)
  FileTime. The clamp's "u32 props contract" comment is stale (FileTimes is u64, cr_types.h:45-50) and the
  sibling exFAT decoder has no clamp (cr_exfat_quick.cpp:190-200) — core-internal inconsistency.
- **Concrete differing input:** creation_date year field 127 (2107-12-31: date word 0xFFBF) → og:
  2107-12-31 FILETIME; core: 0 (invalid).
- **Fix shape:** drop the clamp (return u64) or raise the ceiling; pin with a 2107 boundary test.

### LE-4 — exFAT `._`-prefix sink filter absent in core (og drops macOS AppleDouble files)
**Severity: MINOR** (core superset; N-028 precedent class)
- **Og:** the exFAT props builder FUN_180036660 refuses names starting `"._"`
  (exFAT:45151 `strncmp(name,"._",2)`; match → no props build → `return 0` :45222 — the entry never
  surfaces). Single site in the DLL; never decoded in any corpus artifact.
- **Core:** cr_exfat_quick.cpp:555-800 — no name-prefix gate; `._*` files are emitted.
- **Concrete differing input:** an exFAT volume written by macOS carries `._photo.jpg` AppleDouble
  sidecars → og results: absent; core results: present (recoverable, metadata-only content).
- **Note:** mirror of the HFS+ sink filter N-028 (dotfiles + "HFS+ Private Data") — same family, exFAT side.

### LE-5 — FAT 8.3 NTRes case-folding absent in core (uppercase-only short names)
**Severity: MINOR** (case-only name difference; systematic for NTRes-folded 8.3 files)
- **Og:** FUN_180036350 applies the NTRes byte (+0x0C) case flags before joining `%s.%s`:
  0x00→`_strupr(base),_strupr(ext)`; 0x08→lower/UPPER; 0x10→UPPER/lower; 0x18→lower/lower
  (FAT:44191-44216).
- **Core:** `nt_reserved` is never read (grep over cr_fat_quick.cpp/cr_lfn.cpp/cr_fat_wire.h: field declared
  at cr_fat_wire.h:89 only); `short_name_to_u16` emits the raw on-disk bytes (usually uppercase).
- **Concrete differing input:** FAT dentry "FILE001 DAT" + NTRes 0x18 + no LFN (Windows stores
  "file001.dat" this way) → og name `file001.dat`; core name `FILE001.DAT`.

### LE-6 — FAT LFN illegal-character fallback absent in core
**Severity: MINOR** (corrupt-LFN class)
- **Og:** an assembled LFN containing any of `? * | \ / : " < >` is discarded; the 8.3 alias (with LE-1's
  `#` substitution) is used instead (FAT:32084-32099).
- **Core:** `lfn_assemble` validates only the checksum (cr_fat_quick.cpp:204-215); an LFN with illegal
  characters (corruption remnants) is emitted as-is.
- **Concrete differing input:** deleted file whose surviving LFN slots decode to `re/adme.txt` (corrupt
  fragment) → og: `#EADME.TXT` (8.3 fallback); core: `re/adme.txt`.

### LE-7 — FAT dentry shape-validator family (histogram, dir-size, size/chain-XOR, ext-byte gates) not ported
**Severity: MAJOR** (false-positive suppression on corrupt volumes — the core emits garbage dentries the og rejects; also the og uses validator failure as end-of-walk at several sites)
- **Og:** FUN_1800365a0 (FAT:44263-44375), called at 9 walk sites, unconditionally enforces:
  (a) LFN slot gate (covered); (b) volume-label reject; (c) ext-tail byte > `'?'` reject (:44295-96);
  (d) file size/chain XOR consistency — `(chain≠0) == (size@+0x1C==0)` → reject (:44344-48);
  (e) directory `size≠0 ∧ first_cluster_high≠0` → reject (:44350);
  (f) **byte-histogram corruption gate** — any byte value appearing more than 9 times in the 21-byte
  metadata window (+0x0B..+0x1F) of the paired entry → reject (:44356-44369).
  At :8792-8793 a failed validation `break`s the directory/orphan-registry walk (end-of-directory semantics).
- **Core:** only the date-sanity sub-gate exists (config-gated, default OFF — N-010c); none of (c)-(f).
- **Concrete differing input:** a torn FAT root sector leaving a garbage dentry with e.g. 12×`0xFF` bytes in
  +0x0B..+0x1F (typical erased-tail pattern) → og: dentry rejected (and walk stops); core: dentry emitted as
  a file result (bogus name/size/chain).
- **Note:** (f) is the og's main garbage-dentry FP guard; porting it needs the exact call-site semantics
  (reject vs walk-stop) decoded per site (9 sites listed in the sheet).

### LE-8 — ext4 "size exceeds blocks" damaged flag (props FLAGS bit 0x200) not computed
**Severity: MINOR** (metadata marker the og persists in .rsf; no core carrier)
- **Og:** FUN_180007510 (EXT4:5455-5467) computes `i_blocks_lo < size>>9` (with a +0x1F sign-extension
  quirk on i_size_high) and FUN_1800068b0's caller sets `props FLAGS (+0x39) |= 0x200` for files
  (EXT4:6300-6303). Serialized in the .rsf (ifileproperties-part2 §1: FLAGS at +0x39).
- **Core:** no size-vs-blocks comparison anywhere in cr_ext4_engine.cpp; `FileProperties::flags` carries
  only the internal bit-4 (cr_node.h:85).
- **Concrete differing input:** a truncated ext4 file (i_size 100 KiB, i_blocks covering 4 KiB after
  damage) → og: node carries the 0x200 damaged marker downstream; core: no marker (grading differs only
  through the S-023 superset inputs).

---

## 3. OPEN QUESTIONS

1. **Q1 — the og's LFN checksum validation site for FAT** (LE-1's LFN half): the corpus claims
   "reassembled checksum-verified against their 8.3 alias" (fat-engine--deep-dive.md:56-60) but the compare
   loop was not located in FUN_1800285d0/FUN_180028cc0 this session. Decisive for whether a deleted file
   with intact LFN slots shows its LFN in the og (checksum computed over the 0xE5- or `#`-led alias can
   never match the stored value → 8.3 fallback) or the raw LFN. One targeted read of FUN_180028cc0's tail
   settles it.
2. **Q2 — the og's initial FAT/exFAT condition assignment (+0x71)**: values {0,3,4} and the
   fragmented/inferred adjustment (→4, or the +0x73 0x40-band when already 3) are decoded; the ctor leaves
   +0x71 uninitialized (FAT:32832-32862), so the seed comes from an emitter call not yet identified. Feeds
   OQ-S2M-1 (grading locus) — likely in the quick-search controller, not the walk.
3. **Q3 — exFAT FUN_180036660's two callers' live/deleted discrimination** (:42058 param_5='\x01' vs
   :42092 '\0') and whether the `"._"` filter also applies in the second props builder (exFAT:5195+ — no
   `._` check seen there this session; if absent, the filter may be live-path-only, narrowing LE-4).
4. **Q4 — the `attr=='!'` gate in the FAT name path** (FAT:32096, with FUN_180036b40): attr 0x21
   (READONLY|ARCHIVE?) plus a name predicate forcing the 8.3 fallback — undecoded (Ghidra type-propagation
   noise around `lVar12`); low impact.
5. **Q5 — the ~N numeric-tail rescue of bad access dates** (FAT:44320-44339): the strlen over a
   zero-initialized local reads like a decompiler artifact; the real predicate (name[6]=='~' ∧
   atoi(name[7]) ∈ [1,5]) rescues date-invalid dentries the core's strict gate would reject. Only relevant
   if `strict_date_sanity` ever defaults ON.
6. **Q6 — ext4 `_gmtime32_s` post-2038 failure mode** (og defect): confirm the uninitialized-SYSTEMTIME
   behavior on a 2040-dated inode (bench caveat for parity comparisons; no core action — the core is
   correct through i64+epoch-bits).

---

## 4. VERDICT ROLL-UP

- Examined predicates/thresholds/constants: **62** rows across the 6 areas (1.1.1-1.16, 1.2.1-12,
  1.3.1-9, 1.4.1-4, 1.5.1-2, §1.6 table ×22).
- **MATCH:** 24 (incl. newly first-hand verified: FAT full-u32 size field, FAT create-tenths non-use,
  exFAT 10ms/tz non-use, HFS+ epoch formula, Btrfs nsec formula, exFAT dentry-type constants, FAT
  EOC/BAD families, exFAT EOC/BAD, NTFS attr flags).
- **COVERED-BY-<finding>/REGISTERED:** 26 (S-012, D-51, D-49, D-79, N-021, N-010b/c, N-034, M-011, M-013,
  M-014, M-015, M-026, M-045, S-021, D-93, S-023, D-15, I-006, I-007, I-014, I-032, I-032/M-019, S-034,
  S-052, S-022, I-002 et al.).
- **NOT-APPLICABLE:** 2 (engine-level cross-volume runs — evidence-based; ext4 inline-data — shared
  limitation).
- **DIVERGENT (new candidates):** 8 — LE-1 (SEVERE), LE-2 (ACUTE), LE-3 (MINOR), LE-4 (MINOR),
  LE-5 (MINOR), LE-6 (MINOR), LE-7 (MAJOR), LE-8 (MINOR).
- Og defects decoded but correctly NOT adopted by the core (register alongside D-58/D-59): ext4
  unconditional 64-bit size (i_dir_acl pollution of dir sizes, EXT4:2372-2379), ext4 post-2038
  `_gmtime32_s` failure (EXT4:6261-6282).
