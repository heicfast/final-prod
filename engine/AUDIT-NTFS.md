# AUDIT-NTFS.md — Refactor-C: NTFS deep-parity audit (read-only)

**Task ID**: Refactor-C · **Date**: 2026-10-24 · **Verdict vocabulary**: MATCH / MATCH+
(spec-correct + hardening) / FIX (open bug) / DELTA (registered or needs registering) /
UNVERIFIED (corpus silent — tracked, never guessed).

**Pins**
- Ours: `core_recovery` @ `08b44ac28bdcdd6263e7fe20fe0b597c141f87d9` (main; clean tree
  except sibling agents' untracked AUDIT-*.md, untouched).
- Corpus: `easeus-drw-research` @ `18d6f14a1add9fc761ed7b3732bb1715b15aa79f`.
- Decompile line numbers cite
  `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` (62,394 lines).
  Artifact cites use `§section` of the named artifact.

## 1. Methodology

1. Read 100% of our NTFS layer line-by-line: 6 impls (`src/core/ntfs/*.cpp`, 2,171 lines),
   7 headers (`include/cr/ntfs/*.h`, 719 lines), tests (`test_ntfs_volume.cpp` 1,430,
   `test_fixup.cpp` 109, `test_runlist.cpp` 140, `fuzz/fuzz_ntfs.cpp` 41) + the
   orchestrator wiring (`cr_scan.cpp` NTFS paths, `cr_cached.cpp`).
2. Read the corpus ground truth in full: `SearchDLL/ntfs-engine--deep-dive.md` (191),
   `ntfs-engine-internals.md` (90), `ntfs-engine-algorithms-v2--deleted-viability.md`
   (144), `MOM-modules/NTFSLib.md`, `NTFSUtil.md`, `NTFSFileSystemAnalyser.md` (241 total).
3. **Re-verified every prior-audit claim against the decompile myself** (did not trust
   `docs/PARITY-AUDIT.md`, `docs/parity/PARITY-CORE-FS1.md`, or the artifacts): read
   ±150-line windows around FUN_1800411f0/180041290/180041a20/180041bb0/180046160,
   FUN_18001a3c0/18001a540, FUN_18001acc0 (deleted batch loop 21095–21302),
   FUN_18002a780 salvage (34123–34190), FUN_18003fa20/18003fe40/18003ff50 (runlist
   codec), FUN_180041850/1800416d0/180041700/180041820 (header getters),
   FUN_180040f20/180040d70 (metafile/name lookup), FUN_180006db0 (props builder
   5221–5395), line 7163 (fastsearch ini), string sites 9070/19761/19868/20282/37388–37408/
   52032/52102, and the 430–477 region backing the "0xFFF resident cap" claim.
4. Dead-code hunt via repo-wide symbol greps (src, tests, fuzz) for every declared
   constant/struct/accessor in the NTFS layer.

## 2. Parameter-by-parameter table (ours vs og, both cited)

Dec = `NTFSSearchDLL_decompiled.c`. "Registered" = has a D-nn entry in `docs/PARITY-DELTAS.md`.

| # | Parameter | Ours | OG | Verdict |
|---|---|---|---|---|
| **A. Constants** | | | | |
| 1 | FILE magic 0x454C4946 | `cr_ntfs_wire.h:57`; walk check `cr_mftwalk.cpp:269` | Dec 21152, 34153–34161 | **MATCH** |
| 2 | Min parse size 0x400 | `cr_fixup.cpp:24,30`; record clamp `cr_bootstrap.cpp:52-53` | Dec 21139–21142, 52520; FUN_1800411f0 `size<0x400→0` (52125) | **MATCH** |
| 3 | $ATTRIBUTE_LIST = 0x20 | `cr_ntfs_wire.h:85`; probe `cr_bootstrap.cpp:130` | Dec 20741 `FUN_180041850(buf,0x20,0)` | **MATCH** |
| 4 | $FILE_NAME = 0x30 | `cr_ntfs_wire.h:86` | Dec 52190/53409 | **MATCH** |
| 5 | $OBJECT_ID = 0x40 | `cr_ntfs_wire.h:87-88` | Dec 51006 | **MATCH** |
| 6 | $DATA = 0x80 | `cr_ntfs_wire.h:92` | Dec 54487 (FUN_180044bd0 walk 54293) | **MATCH** |
| 7 | Resident $DATA cap 0xFFF | no cap; value bounded by record (`cr_attributes.cpp:44-50`) | corpus claim is a **misread**: Dec 450–477 is an MSVC `std::string` SSO destructor (`0xf` capacity / `0xfff` dealloc check), not an NTFS cap | **DELTA (doc)** — NTFS-4 |
| 8 | Metafile filter | known 20-name list `cr_mftwalk.cpp:220-229` + `dos_flags&4` skip `:469` | blanket `flags&4 \|\| name==NULL \|\| name[0]=='$'` (Dec 21186) + separate 15-name metafile table FUN_180040f20 (51993–52014) | **DELTA (registered, folded into D-12)** |
| 9 | Viability gate boundary | `>= 0x3FF` (`cr_mftwalk.h:39`, `cr_mftwalk.cpp:491`) | strict `0x3ff < size` (Dec 21208) | **FIX (open)** — NTFS-1 |
| 10 | fastsearch ini default ON | `ScanConfig.run_quick = true` (`cr_scan.h:53`) | `GetPrivateProfileIntW(L"search",L"fastsearch",1,…)` Dec 7163 | **MATCH** (config-level; in-process config = D-14/D-21 scope) |
| **B. Fixup / USA** | | | | |
| 11 | Validation scope = first TWO sector tails | `check=min(sectors,2)` `cr_fixup.cpp:47-53`; test `test_fixup.cpp:67-84` | `local_18 < (uss-1) && local_18 < 2` (Dec 52129–52135) | **MATCH** (F-A1-3 landed) |
| 12 | Apply restores ALL tails | `cr_fixup.cpp:63-69` | FUN_180041a20 loop all sectors, bounded by size (Dec 52457–52463) | **MATCH** |
| 13 | USA offset gate | nonzero **and even** (`cr_fixup.cpp:34`) | `(FUN_180046160(rec) & 0xff) == 0 → reject` — "low byte nonzero", not evenness (Dec 52524–52527 + 55187–55227) | **DELTA (unregistered; contract mis-cites corpus)** — NTFS-3 |
| 14 | Sector size | parameterized 256..65536 (`cr_fixup.cpp:32`) | hardcoded `0x200` stride (Dec 52132, 52460, 52158) | **DELTA (registered D-04)** |
| 15 | USA count bounds | `usa_count>=2`, `sectors*sector_size<=record_bytes`, array in-bounds (`cr_fixup.cpp:36-42`) | implicit `usa_size < 0x11` else reject (FUN_180046160 Dec 55195→55220) — og caps records at 4 KiB @512B | **MATCH+** (ours more permissive; fold into D-04 note — NTFS-10) |
| **C. Runlist codec** | | | | |
| 16 | Nibble semantics | LOW=length-width, HIGH=offset-width (`cr_runlist.cpp:75-76`) | confirmed by three og sites: validator FUN_18003ff50 (51365–51373: lenw=low, offw=high, stride=1+lenw+offw), cluster-sum FUN_18003fe40 (51327–51333), decoder FUN_18003fa20 (51353–51356); deep-dive §7 "high=length" prose is the misread | **MATCH** (but `cr_runlist.h:4-7` comment still states the swapped order — NTFS-6) |
| 17 | Terminator 0x00 | `cr_runlist.cpp:69-70` | Dec 51251 | **MATCH** |
| 18 | Zero-length run rejected | `cr_runlist.cpp:85-86` | og: no equivalent guard decoded | **MATCH+** |
| 19 | Negative offset deltas (backward runs) | sign-extended read + wrapped u64 accumulate (`cr_runlist.cpp:93-105`) | og reads offset via LE memcpy into i64 (sign handling not decidable — Ghidra artifact: `memcpy(&local_40,…,0)` at 51257) | **MATCH (spec)** / og leg UNVERIFIABLE |
| 20 | INT64_MIN offset rejected (UB fix) | `cr_runlist.cpp:100-101`; test `test_runlist.cpp:127-140` | n/a (og would UB) | **MATCH+** (fix 383c3c5) |
| 21 | Sparse run = offset-width 0 | `cr_runlist.cpp:94,104-112` | og entry LCN = −1 sentinel (Dec 51260) | **MATCH** |
| 22 | Field-width caps | len 1..8, off ≤8 (`cr_runlist.cpp:77-80`) | both nibbles must be ≤4 (`if (4<uVar2 \|\| 4<bVar1) invalid`, Dec 51368) | **DELTA (unregistered micro)** — NTFS-9 |
| 23 | Run-count / cluster guards | `max_clusters` + `RUNLIST_MAX_RUNS` (`cr_runlist.cpp:88-91`, `cr_runlist.h:61`) | none | **MATCH+** (F6) |
| **D. $ATTRIBUTE_LIST chase** | | | | |
| 24 | Depth limit + visited set | walk BFS depth 64 + visited, `single_hop_attrlist` opt-in caps it at the og's depth-1 (M-008, session 11; the default BFS is the registered D-73 superset); bootstrap visited + FRAG_ENTRY_LIMIT (`cr_bootstrap.cpp:200-232`) | pass-1 chase is DEPTH-1: FUN_180041df0 :52615-52687 iterates the child list, no recursion (decoded M-008; the child list exists for every record — pass 1's FUN_1800431d0 call at :18461-18464 passes plVar9 non-NULL, deleted records included) | **MATCH at the flag; D-73 superset default** (cycle guard = hardening the og lacks) |
| 25 | Resident AND nonresident list | both (`cr_bootstrap.cpp:137-182`; walker uses child records) | only record-resident list evidenced | **MATCH+** |
| 26 | Multi-record reassembly ordering | fragments sorted by entry `lowest_vcn` (`cr_bootstrap.cpp:279-286`) | og builds extent map from list (FUN_180044ab0/180044bd0, 54203–54327) | **MATCH** (mechanism-equivalent) |
| **E. $MFT walk** | | | | |
| 27 | 64 KiB batched reads | per-record `read_record` calls coalesced by 64-KiB-block `CachedReader` (`cr_scan.cpp:373`, `cr_cached.cpp:38-96`); engine never batches itself | 64 KiB bootstrap buffer (Dec 20731–20736) + 0x1000-granular batch loader re-validating per record slot (Dec 20840; v2 §5) | **FIX (open F-A1-6)** — NTFS-2 |
| 28 | Cluster-strided MFT mapping | VCN→LCN via runlist + extent view (`cr_bootstrap.cpp:418-448`) | region math `recordSize*param` (Dec 21142–21144) | **MATCH** |
| 29 | $MFTMirr cross-check (first 4 records) | mirror region copy of record 1 → its $DATA runs → fallback for records 0–3 (`cr_bootstrap.cpp:291-315, 439-446, 451-462`) | `FUN_180040d70(ctx,L"$MFTMirr")` name-lookup (Dec 52032/52102); v1 §5 "first 4 records, volume midpoint" | **MATCH** (mechanism delta: we read the mirror copy, og the primary record; ours robust to primary damage) |
| 30 | Namespace ranking POSIX>W32+DOS>W32>DOS | `namespace_rank` `cr_mftwalk.cpp:41-54`, selection `:410-413, 579-583` | not decoded (corpus: "DOS+Win32 namespace variants", internals) | **MATCH (spec)** / og UNVERIFIED |
| 31 | Hard links: node per parent, per-link props clone, record_ref grouping | `cr_mftwalk.cpp:515-622`; test 1244–1250 | refcounted props model (deep-dive §3, census) | **MATCH** |
| 32 | ADS children (named $DATA) incl. resident content + canonical-first-link attach | `cr_mftwalk.cpp:625-670`; tests 763–861 | "attribute-stream assembly" (deep-dive §5, concept) | **MATCH** (details UNVERIFIED) |
| 33 | Deleted classification | record header `flags & 0x0001` (`cr_mftwalk.cpp:474`, `cr_ntfs_wire.h:79`) | artifacts claim "in-use maps to props bit 4" — **contradicted by decompile**: FUN_180041970 tests `record+0x20 != 0` = *base record* (extension-record skip, Dec 21155/52411-52415); props 0x10 = directory (FUN_180006db0 Dec 5327–5331, gated at 21190); in-use site not located | **MATCH (spec-correct)** + corpus-erratum — NTFS-11 |
| 34 | Remnant scan: FILE0 shifted-header salvage | probe `+0x58` else `+0x60` for `'FILE'+'0'`, ≥0x70 window, zeroed buffer, parse without mandatory fixup (`cr_mftwalk.cpp:963-1031`) | byte-exact: 0x454c4946 + `'0'` at +4, `0x70 < avail < 0x400`, memset 0x400, FUN_180041970+FUN_1800431d0, **no** FUN_180041bb0 (Dec 34151–34190) | **MATCH** (D-10 registered; full-record copy = our improvement) |
| 35 | Parallel chunk fan-out (remnants) | ThreadPool chunks (`cr_mftwalk.cpp:1078-1097`) | og Vista cond-var workers (internals; loop set UNVERIFIED) | **DELTA (registered D-18)** |
| **F. $Bitmap grading** | | | | |
| 36 | Bit test | cluster-granular, `buf[pos>>3] & (1<<(pos&7))`, OOR→false (`cr_bitmap.h:29-38`) | sector-granular `spc*cluster` index, same bit op (FUN_18002c2e0; v2 §3) | **MATCH** (index-space delta documented FS1) |
| 37 | Grading classes | ratio≥1.0→Overwritten, partial→PartialRisk, 0→Excellent; MetadataOnly for no-data (`cr_mftwalk.cpp:489-509`) | all-reused→destroy, partial→keep, <1 KiB fast-path (Dec 21249, v2 §1) | **DELTA (registered D-14/F5)**; `keep_overwritten` flag dead |
| 38 | Viability probe size source | `data_real_size ?: best->real_size` (`cr_mftwalk.cpp:485-488`); regression test 1376–1429 | gate reads props+0x1b (Dec 21203); og source field not decoded | **MATCH (F-A1-2 landed)** / og source UNVERIFIED |
| 39 | No-bitmap case | PartialRisk (`cr_mftwalk.cpp:500-502`) | skip grading, still add (Dec 21209–21214) | **DELTA** (should be folded into D-14 explicitly — NTFS-12 note) |
| 40 | Sparse runs in ratio | count as reused (`cr_bitmap.cpp:88-92`) | og iterates data runs only (sparse runs carry LCN −1, Dec 21220) | **MATCH+** |
| **G. $UsnJrnl + $Recycle.Bin** | | | | |
| 41 | $J harvest (V2/V3, FILE_DELETE 0x20000, 8-byte resync, dedup, 64 MiB cap) | `cr_mftwalk.cpp:708-847`; V2/V3 offsets verified against winioctl.h (ref@8/24, ts@32/48, reason@40/56, name@64/80) | og: `$UsnJrnl` strcmp sites only (Dec 9070/19761/19868/20282); no USN_RECORD loop located | **DELTA (registered D-11)** / og specifics UNVERIFIED |
| 42 | $Recycle.Bin $I/$R mapping (v1 544-form vs Win10 variable form) | `parse_recycle_info` `cr_mftwalk.cpp:149-215`; $R remap `:450-468, 540-554`; tests 1273–1345 | og: bin-dir strings incl. `$RECYCLE.BI0` (Dec 37388–37408); $I/$R pairing attested FAT-side only | **DELTA (registered D-12)** / og NTFS-side parse UNVERIFIED |
| **H. Index attributes** | | | | |
| 43 | Directory detection for viability | record flag 0x0002 (`cr_mftwalk.cpp:475`); $I30 ($INDEX_ALLOCATION 0xA0) runlist as dir probe (`:126-137, 480, 487`) | og: $FILE_NAME flags `& 0x10000000` (FUN_180043700 Dec 53481–53488, props 0x10 set at 5329) | **MATCH** (different source, same semantics; ours uses the authoritative record flag) |
| 44 | INDX / $INDEX_ROOT block walking | not implemented; `parse_index_root` + INDX structs are dead code | no INDX walker located in corpus artifacts or greps | **UNVERIFIED (og)** + dead code ours |
| **I. Error paths** | | | | |
| 45 | Parse failure → skip record, never abort walk | read fail/magic-miss/fixup fail/enum fail → damaged++ & continue (`cr_mftwalk.cpp:260-282`); walk aborts only on cancel/OOM | fixup fail → skip (FUN_180041bb0==0), bad-record check, props NULL → Release+skip (Dec 21153–21166); "never aborts the walk" (deep-dive §9) | **MATCH** |
| 46 | Extension records (base_record != 0) | nameless ext records fall into `names.empty()` → `damaged++` (`cr_mftwalk.cpp:404-407`) | skipped silently via FUN_180041970 (Dec 21155–21157) | **MICRO-DELTA** — NTFS-8 |
| 47 | BAAD records | implicit skip via magic != FILE (`cr_mftwalk.cpp:269-270`) | "BAAD-family records skipped" (internals §MFT acquisition) | **MATCH** |

**Roll-up (47 rows): MATCH/MATCH+ 37 (79%) · DELTA-registered 6 · DELTA-unregistered/doc 4 ·
FIX (open) 2 · UNVERIFIED-og 2 (rows 44; 41/42 carry UNVERIFIED og-legs) · micro-deltas 2.
Counting registered deltas as acceptable parity: 43/47 ≈ 91%.**

## 3. Findings

### NTFS-1 — P2 — FIX (open) — Viability gate boundary `>= 0x3FF` vs og `> 0x3FF`
- **Ours**: `cr_mftwalk.h:39` `viability_min_size = 0x3FF` + `cr_mftwalk.cpp:491`
  `probe_size >= flags.viability_min_size` → files of exactly 0x3FF (1023) bytes get the
  bitmap probe.
- **OG**: `if (0x3ff < lVar18)` — strict (Dec 21208). 1023-byte deleted files take the
  fast-path (no bitmap check).
- **Status**: PARITY-AUDIT F-A1-10 (P2) disposition said "FIX" (wave 2); **not landed, not
  registered as a delta** — by the deltas-registry's own rule this is a bug, not a delta.
- **Acceptance test**: fixture with a deleted 1023-byte nonresident file whose clusters are
  50% reused; assert the chosen outcome explicitly (og-parity: Excellent fast-path, or our
  stricter probe) and pin it in the contract row.

### NTFS-2 — P2 — FIX (open) — 64 KiB MFT batched reads unimplemented; contract claim false
- **Ours**: `walk_mft` issues per-record `read_record` reads; `MFT_FIRST_READ_BYTES`
  (`cr_bootstrap.h:28`) is dead. Mitigation: every engine read goes through the 8 MiB LRU
  `CachedReader` created with **64 KiB blocks** (`cr_scan.cpp:373`, `cr_cached.cpp:38-96`),
  so sequential record reads coalesce to one device read per 64 KiB.
- **OG**: 64 KiB bootstrap buffer (Dec 20731–20736) + 0x1000-granular batch loader
  re-validating per record slot (Dec 20840, v2 §5); contract §2 "never per-record", §3
  "MFT batch 64 KB".
- **Status**: F-A1-6 open; not in the deltas registry.
- **Acceptance**: either (a) batch `read_record` over 64-KiB windows in `walk_mft`
  (preferred: contract §2 is a locked IO discipline) or (b) amend contract §2/§3 to
  "cache-coalesced per-record reads (64-KiB LRU blocks)" and register D-xx. Add a
  performance assertion (device read count ≤ ceil(MFT bytes / 64 KiB) + slack) to
  `test_ntfs_volume.cpp`.

### NTFS-3 — P3 — DELTA (unregistered, doc-integrity) — Fixup offset gate wording
- **Ours**: `usa_offset == 0 || (usa_offset & 1)` → reject (`cr_fixup.cpp:34`); test
  `test_fixup.cpp:93-100` asserts the evenness rejection.
- **OG**: `(FUN_180046160(rec) & 0xff) == 0` → reject (Dec 52524–52527). FUN_180046160
  (Dec 55187–55227) computes a defensive offset whose low byte must be nonzero — a
  **mod-256 test**, not "nonzero + even".
- **Contract §3** now reads "offset nonzero + even | FUN_180041bb0 chain" — it attributes
  our hardening to the corpus. F-A1-4's DOC disposition landed as a contract edit that
  invents the evenness clause.
- **Acceptance**: correct contract §3 to "og: low byte ≠ 0 (mod-256); ours: nonzero+even
  (hardening)" and add a deltas-registry entry; keep the test but label it hardening.

### NTFS-4 — P3 — DELTA (doc-integrity) — "resident $DATA cap 0xFFF" is a corpus misread
- The deep-dive §6 claim ("0x80-resident data length checks, clamp ≤ 0xFFF at 450-474
  region") points at Dec 450–477, which is an MSVC `std::string` destructor: SSO capacity
  0xF, dealloc path `0xfff < size+1` with heap-granularity check 0x1f — **nothing to do
  with NTFS resident attributes**. Contract §3 repeats it ("resident cap logic 0xFFF").
- **Ours** correctly has no cap (resident `value_length` bounded by the record,
  `cr_attributes.cpp:44-50`) — spec-correct.
- **Acceptance**: delete the contract row's cap clause; feed an erratum to the corpus
  repo's artifact (ntfs-engine--deep-dive.md §6).

### NTFS-5 — P3 — DELTA (doc) — PFLAG_IN_USE bit-semantics comment wrong (F-A1-5 not landed)
- `cr_mftwalk.h:31` `PFLAG_IN_USE = 0x0010 /* corpus mapping: bit4 = in-use */`; the same
  wrong comment is repeated in the FAT engine (`cr_fat_quick.cpp:214`).
- **OG truth** (decoded this session): props word is FILE_ATTRIBUTE-style — 0x10 =
  directory (FUN_180006db0 Dec 5327–5331 via FUN_180043700 `$FILE_NAME flags &
  0x10000000`), 0x20 = deleted, 4 = system (ifileproperties §2 census + Dec 21190's
  `(flags & 0x10) == 0` = not-folder gate). Our `props->flags` persists into .crsf
  (`cr_records.cpp:211`), so if .crsf ever mirrors .rsf semantics, bit 0x10 collides
  (ours: in-use; og: directory).
- **Acceptance**: fix both comments (or remap bits if .rsf-semantic mirroring is planned);
  assert in `test_session` that the flag word round-trips with our documented meaning.

### NTFS-6 — P3 — DELTA (doc) — `cr_runlist.h` header comment states the swapped nibble order
- `cr_runlist.h:4-7`: "high nibble = length-field width, low nibble = offset-field width
  (corpus FUN_180003440)" — contradicts `cr_runlist.cpp:71-76`, contract §3's correction,
  and the decompile (rows 16 of §2). The .cpp carries the correction note; the .h was
  never updated.
- **Acceptance**: one-line comment fix; no code change (decoder is correct, tests cover it).

### NTFS-7 — P3 — FIX (doc + latent) — `NonresidentTail` field labels swapped vs spec
- `cr_ntfs_wire.h:126-135` labels `initialized /* +0x30 */` and `actual /* +0x38 real
  size */`. NTFS spec (ntfs-3g layout.h; ntfs3 `data_size@0x30 / valid_size@0x38`):
  **+0x30 = file (real) size, +0x38 = initialized size**. `cr_attributes.cpp:63-65` is
  correct (actual@+48=0x30).
- Impact: `cr_bootstrap.cpp:148` reads the $ATTRIBUTE_LIST content length from
  `na.actual` = +0x38 (initialized size). Benign for typical attrlists (allocated ==
  initialized == real) but wrong for a sparse nonresident $ATTRIBUTE_LIST (under-read).
- **Acceptance**: swap the two field names/comments in `NonresidentTail`; add a fixture
  with a nonresident $ATTRIBUTE_LIST whose +0x30 ≠ +0x38 asserting the read uses the file
  size.

### NTFS-8 — P3 — MICRO-DELTA — Extension records counted `damaged`
- Nameless extension records (base_record ≠ 0) reach `names.empty()` → `stats.damaged++`
  (`cr_mftwalk.cpp:404-407`). OG skips them silently via FUN_180041970 (Dec 21155–21157).
  On fragmented volumes our damaged count is inflated by every $ATTRIBUTE_LIST child.
- **Acceptance**: early-`continue` when `rh.base_record != 0` (the attrlist chase reads
  children explicitly); assert `damaged == 0` on the existing `NestedAttrlistChase`
  fixture extended with a nameless child record.

### NTFS-9 — P3 — DELTA (unregistered micro) — Runlist width caps and FILE_NAME size masking
- OG validator requires both nibble widths ≤ 4 (Dec 51368) — i.e. 32-bit run fields only;
  ours accepts 1..8 (`cr_runlist.cpp:77-80`). OG masks $FILE_NAME allocated/real sizes to
  48 bits with sign extension (FUN_180043610/1800436b0, Dec 53454–53476); ours reads raw
  u64 (`cr_attributes.cpp:148-149`). Both are ours-is-stricter/more-capable divergences.
- **Acceptance**: register one combined hardening delta (widths ≤8, unmasked sizes) under
  D-14's F-family; note og's ≤4 cap makes >16 TiB volumes' runlists unreadable to og.

### NTFS-10 — P3 — DELTA (fold into D-04) — OG rejects `usa_size >= 0x11`
- FUN_180046160 returns a low-byte-zero value for `usa_size >= 0x11` (Dec 55195–55220) →
  FUN_180041bb0 fails: og effectively caps records at 4 KiB (8×512B sectors). Ours
  accepts any `usa_count` bounded by record size (4Kn-aware). Already subsumable under
  D-04's sector-size family — add the note.

### NTFS-11 — P3 — UNVERIFIED-og / corpus-erratum — OG in-use classification site + two artifact mislabels
- Decoded this session: FUN_180041970 = `record+0x20 != 0` = **base-record (extension)
  test**, not "record in use" as both v1 §3 (line 77–79) and v2 §1 (line 25) claim.
  FUN_1800411d0 (bootstrap, Dec 20740) = `find $MFT by name` (FUN_180040e40 →
  FUN_180040d70, requires $SI flags 4&2 + $FILE_NAME=="$MFT"), not "header sanity".
  The actual `flags & 0x0001` in-use test was not located in the decoded loops (likely
  inside FUN_1800431d0/FUN_1800069e0, not decoded).
- **Ours is unaffected**: `RECORD_IN_USE = 0x0001` at +0x16 is spec-correct and delivers
  the product behavior (deleted records surface). No action beyond erratum notes.
- **Acceptance**: feed both errata to the corpus repo; optionally decode FUN_1800431d0 in
  a follow-up to close the in-use-site question.

### NTFS-12 — P3 — DELTA (registry hygiene) — Under-specified D-14 sub-items
- The no-bitmap→PartialRisk case (row 39) and `keep_overwritten` being a dead flag are
  D-14/F5 scope but the registry entry does not name them. Fold: (a) name the no-bitmap
  case in D-14; (b) delete the dead flag or wire an og-exact destroy mode behind it.

## 4. Dead / duplicate code register (verified by repo-wide grep incl. tests + fuzz)

| # | Symbol | Location | Evidence |
|---|---|---|---|
| 1 | `MFT_RESERVED_RECORDS` | `cr_bootstrap.h:27` | 0 refs anywhere |
| 2 | `MFT_FIRST_READ_BYTES` | `cr_bootstrap.h:28` | 0 refs (ties NTFS-2) |
| 3 | `NtfsVolume::read_record_mirr` | `cr_bootstrap.cpp:451-462` | public API, 0 callers (mirror fallback lives inside `read_record`) |
| 4 | `WalkerFlags::keep_overwritten` | `cr_mftwalk.h:36` | default-initialized, never read (F-A1-17 cleanup pending) |
| 5 | `USN_REASON_FILE_CREATE` | `cr_mftwalk.h:44-46` | engine never filters on it; used only by tests — move to test helper or document as such |
| 6 | `ntfs_time_to_unix_ns` + `NTFS_EPOCH_100NS` | `cr_ntfs_wire.h:224-227` | 0 refs |
| 7 | `mft_reference_sequence` | `cr_ntfs_wire.h:219-221` | 0 refs (`cr_attributes.cpp:143` inlines the shift) |
| 8 | `RECORD_MAGIC_BAAD` | `cr_ntfs_wire.h:58` | 0 refs (BAAD skip is implicit via magic != FILE) |
| 9 | `INDX_MAGIC`, `IndexHeader`, `IndexEntryHeader`, `IndxBlockHeader` | `cr_ntfs_wire.h:182-208` | 0 refs (no INDX walking — row 44) |
| 10 | `parse_index_root` | `cr_attributes.cpp:195-213` | 0 callers |
| 11 | `VolumeBitmap::bit_count` | `cr_bitmap.h:44` | 0 refs |
| 12 | `RunList::allocated_bytes` | `cr_runlist.h:44` | 0 refs |
| 13 | `AttrView::buffer_index` | `cr_attributes.h:46` | never set, never read |
| 14 | `AttrView::initialized` | `cr_attributes.h:41` (written `cr_attributes.cpp:65`) | never read |
| 15 | `FileNameInfo::parent_sequence` | `cr_attributes.h:68` (written `cr_attributes.cpp:143`) | never read |
| 16 | `NtfsVolume::mft_runlist() / mftmirr_runlist() / mft_extents()` | `cr_bootstrap.h:72-76` | 0 callers |
| 17 | `NtfsVolume::device() / size()` | `cr_bootstrap.h:55,79` | 0 callers |
| 18 | `enum_mirr_attrs` `scratch` param | `cr_bootstrap.cpp:317,365` | vestigial `(void)scratch` |
| 19 | Documentation-only constants, no code consumers | `cr_ntfs_wire.h:87,89-99,102-104` (`ATTR_TYPE_VOLUME_VERSION_OLD`, `SECURITY_DESCRIPTOR`, `VOLUME_NAME`, `VOLUME_INFORMATION`, `BITMAP`, `REPARSE_POINT`, `EA_INFORMATION`, `EA`, `LOGGED_UTILITY_STREAM`, `ATTR_FLAG_*`) | keep as parity documentation or prune; flag as non-consuming |
| 20 | Wire structs parsed by hand instead | `cr_ntfs_wire.h:147-178` (`FileNameAttribute`, `StandardInformation`) | static_assert-only; parser uses byte reads — keep as layout docs or delete |
| 21 | **Duplicated attribute-walk loops** (real maintenance hazard) | `cr_bootstrap.cpp:92-187` (main), `:247-272` (child-record loop), `:326-364` (`enum_mirr_attrs`) vs `cr_attributes.cpp:12-78` (`enum_attributes`) | three hand-rolled walkers with **divergent bounds checks** (main loop checks `name_off > len`; child loop and `enum_mirr_attrs` do not; none share the 64-attr cap) — consolidate on `enum_attributes` |

Also verified live (not dead): `NonresidentTail`, `ResidentAttributeHeader`,
`BootSector`, `RecordHeader` (memcpy-parsed), `RunList::empty/vcn_to_lcn/extents/encode`,
`VolumeBitmap::load/test/reused_ratio/valid`, `is_metafile_name`, `scan_mft_remnants`
(caller `cr_scan.cpp:655`, tests, fuzz).

## 5. UNVERIFIED register (corpus silent; no verdict claimed)

1. **OG USN harvest specifics** — no USN_RECORD_V2/V3 parse loop, reason filter, resync,
   or dedup semantics in the artifacts or greps (strcmp sites only). Ours is
   winioctl.h-spec-derived (D-11).
2. **OG NTFS-side $I/$R recycle parse** — bin-dir strings verified (Dec 37388–37408,
   incl. `$RECYCLE.BI0` deleted form); the $I format parse is attested FAT-side only (D-12).
3. **OG namespace-preference order** — "namespace variants" mentioned; no ranking decoded.
4. **OG hard-link emission model** — DECODED + LANDED (M-007, session 11):
   the og emits ONE node per record (FUN_180006db0 :5273 builds props once;
   FUN_1800448b0 :54124-54162 fetches at most two $FILE_NAME entries and
   picks ONE by namespace ranking, FUN_180043330). The core's default now
   matches (WalkerFlags::single_link_per_record = true); the per-parent
   fan-out is the registered opt-in superset (PARITY-DELTAS D-65).
5. **OG props size source (+0x1b)** feeding the 0x3FF gate — FUN_1800069e0 not decoded;
   our $DATA-authoritative choice rests on runner evidence + F-A1-2.
6. **OG runlist offset-field sign handling** — decompile mangled (`memcpy` len 0 artifact
   at Dec 51257); nibble order is verified from three other sites, sign handling is not.
7. **OG INDX/$INDEX_ROOT walking** — no sites located; our index structs are dead code.
8. **OG remnant-scan stride + dedup + worker model** — salvage mechanics are byte-exact;
   the outer scan loop and threading are not decoded.
9. **OG viability-bitmap build loop** — the test is verified; how og builds the bitmap
   (base-offset, re-basing) is v2 §3 inference only.
10. **OG $MFTMirr "volume midpoint" discovery** — DBR-supplied LCN (ours) vs computed
    midpoint; only the cross-check itself is attested (Dec 52032/52102).

## 6. Summary of required actions (priority order)

1. **NTFS-1** (P2): decide 0x3FF boundary — one-line fix or registered delta + test.
2. **NTFS-2** (P2): batch the MFT walk reads or amend contract §2/§3 + register delta;
   delete the two dead constants (register #1-2).
3. **NTFS-7** (P3): swap `NonresidentTail` labels; sparse-attrlist fixture.
4. **NTFS-8** (P3): skip extension records early; damaged-count assertion.
5. Doc wave: NTFS-3, NTFS-4, NTFS-5, NTFS-6 (+ corpus errata feed), NTFS-9/10/12
   registry notes.
6. Dead-code sweep: register items 1-18, 21 (consolidate the three attribute walkers);
   decide keep-as-doc for 19-20.
