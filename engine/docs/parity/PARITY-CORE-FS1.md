# PARITY-CORE-FS1 — NTFS / FAT(12/16/32) / exFAT engine audit (Task 2-a1)

**Agent**: parity auditor A1 (read-only; no repo modifications)
**Date**: 2026-10-15
**Pins**:
- Ours: `core_recovery` HEAD `d42bb003260a6ddcd9b47d46c2da8096b2a73c95` (`2473c03` + `d42bb00`)
- Corpus: `easeus-drw-research` HEAD `18d6f14a1add9fc761ed7b3732bb1715b15aa79f`

**Coverage**: 100% of in-scope engine code read line-by-line — NTFS 7 headers + 6 impls
(`include/cr/ntfs/*.h`, `src/core/ntfs/*.cpp`, 1,101 lines), FAT 2 headers + 3 impls
(925), exFAT 2 headers + 2 impls (863); all three volume tests (2,224 lines) read for
asserted behavior. Corpus: 14 artifacts read in full (all NTFS/FAT/exFAT SearchDLL
deep-dives + v2 algorithm files, MOM-module census docs, SaveScan ifileproperties-part1,
rawfixer-v2, engine-matrix) + targeted decompiled-code verification (~700 lines across
`ghidra-searchdll/{NTFS,FAT,exFAT}SearchDLL_decompiled.c`). All 3×8 audit-axis cells
filled. Not deep-verified (stated in UNVERIFIED): FAT LFN slot-assembly byte loop,
exFAT set-checksum/boot-checksum byte loops, USN record-parse loop — artifacts level only.

**Method note**: decompiled line numbers below are into
`re-workspace/reversed/ghidra-searchdll/<name>_decompiled.c`; artifact cites use
`§section`. "OG" = original (EaseUS DRW) per corpus.

---

## Verdict summary (audit grid)

| Axis | NTFS | FAT 12/16/32 | exFAT |
|---|---|---|---|
| Behavior | **MISMATCH** (F-3 fixup scope; F-2 gate field) | **MISMATCH** (F-1 FAT12 EOC/BAD) | **MISMATCH** (F-7/-8 gates) |
| Constants & offsets | MATCH (magic, types, 0x400, 0x3FF≈, nibble order, 0x200×512 caveat) | MATCH (FAT16/32 constants, 0xE5, (c-2)·spc) | MATCH (types, 0x80 InUse; contract-doc mislabel F-9) |
| Data model | MATCH w/ delta (props-flags bit meanings F-5) | MATCH (32-bit canonical space except FAT12 F-1) | MATCH (u64 volume_length, dentry structs) |
| State machine | MATCH (walk order, filters, attrlist chase, salvage) | MATCH (chain walk, cycle cut, dir recursion) | MATCH (set walk, backward-triple concept) |
| Error surface | MATCH (0x5002xxxx family, skip-not-abort) | MATCH | MATCH |
| Threading | PARTIAL (threaded remnant scan; walk single-thread; OG worker model UNVERIFIED) | MISSING (single-threaded; OG CFATQuickSearchWorker threaded) | MISSING (single-threaded; OG same family) |
| Deleted classification & confidence | **MISMATCH** (F-2 gate bypass; deltas F-16/-17) | **MISMATCH** (F-1 caps FAT12 at PartialRisk; F-12 naming) | MATCH (bitmap grading, InUse semantics) |
| Capability claims | **MISMATCH** (F-6 64KiB batch dead; USN/recycle UNVERIFIED) | MATCH (backup boot, mirror, LFN, reconstruction) | MATCH w/ UNVERIFIED (checksums) |

Bottom line: the engines implement the corpus algorithms faithfully at the structural
level (every must-check constant verified byte-exact where the corpus pins it), with **3
P1 behavioral bugs** (FAT12 chain termination, NTFS viability-gate field, fixup
validation scope), **1 P0-adjacent data-safety concern folded into F-2** (over-grading
deleted files as Excellent), and a set of P2 contract/doc drifts — most notably a
contract-doc error inherited from a corpus artifact mislabel (F-9) and a dead 64 KiB
batch-read claim (F-6).

---

## Findings

### F-A1-1 — P1 — FAT12 BAD/EOC values never canonicalized → every FAT12 chain counted broken, deleted FAT12 files capped at PartialRisk
- **Axis**: Behavior / Constants / Deleted-file grading. **Verdict**: MISMATCH (vs OG and vs our own contract §4).
- **OUR evidence**: `src/core/fat/cr_chain.cpp:30-37` — the `Flavor::Fat12` branch of `FatTable::entry_at` returns the raw 12-bit entry (`(cluster & 1) ? (v >> 4) : (v & 0xFFF)`) with **no** mapping of `0xFF7 → CHAIN_BAD` / `>= 0xFF8 → CHAIN_EOC_MIN`. The FAT16 (lines 38-48) and FAT32 (49-60) branches *do* canonicalize. Downstream, `follow_chain` (`cr_chain.cpp:186-195`) only recognizes the canonical values: a FAT12 EOC (0xFF8-0xFFF = 4088-4095) falls into `n < 2 || n > t.max_cluster()` → `r.broken = true`; a FAT12 BAD (0xFF7) likewise.
- **OG evidence**: `fat-exfat-engine-algorithms-v2.md §1` (FUN_180041c80): *"The engine normalizes every FAT flavor to a 32-bit space: BAD = 0xFFFFFF7, EOC = ≥0xFFFFFF8 — all downstream chain logic tests against these two constants"*; verified `FATSearchDLL_decompiled.c:54125-54143` (FAT16 `Next`: `0xFFF7→0xFFFFFF7`, `<0xFFF8→entry`, else `0xFFFFFF8`). Our own contract `docs/ground-truth-contracts.md §4` states the same canonical rule without a flavor carve-out.
- **Effect**: on FAT12 volumes every cleanly-terminated chain is flagged `broken` (`stats.chains_broken` inflated), and `grade_deleted_chain` (`cr_fat_quick.cpp:73-83`) returns `PartialRisk` for *all* deleted FAT12 files regardless of actual intactness. **No FAT12 test exists** (`tests/unit/test_fat_volume.cpp` covers FAT16/FAT32 only), which is why this survives.
- **Fix**: in the Fat12 branch, after extracting the 12-bit value `v12`: `if (v12 == 0xFF7) return CHAIN_BAD; if (v12 >= 0xFF8) return CHAIN_EOC_MIN;`. Add a FAT12 end-to-end test (fixture with 0xFFF EOC and one 0xFF7 bad cluster).

### F-A1-2 — P1 — NTFS viability gate probes `$FILE_NAME.real_size`, not the authoritative `$DATA` size → gate bypassed for freshly-written deleted files
- **Axis**: Deleted-file classification & confidence grading. **Verdict**: MISMATCH (internal inconsistency + vs OG gate field).
- **OUR evidence**: `src/core/ntfs/cr_mftwalk.cpp:481-483`: `probe_size = … (has_data ? best->real_size : 0)` — uses the **$FILE_NAME** size. But `cr_mftwalk.cpp:117-121` (build_props) and `:531` establish `data_real_size` from the **$DATA nonresident tail @+0x30** as authoritative, with the in-code comment: *"the $DATA tail's file-size-in-bytes (@+0x30) is the authoritative size: $FILE_NAME's size field is 0 on real volumes for freshly-written files"*.
- **OG evidence**: `NTFSSearchDLL_decompiled.c:21203-21208` — gate reads `props+0x1b` (size) and tests `0x3ff < size`. The props-size *source* inside FUN_180006db0 was not decoded (see UNVERIFIED), but our own runner evidence (cited in our code) says $FILE_NAME sizes are 0 on real volumes — so gating on `best->real_size` systematically yields `probe_size = 0 < 0x3FF` → the `Excellent` fast-path at `cr_mftwalk.cpp:498-501` fires for **large nonresident deleted files**, skipping the $Bitmap reuse check entirely and over-grading them as Excellent.
- **Effect**: deleted files whose clusters may be partially reused present as Excellent (data-safety: user confidence misinformed; recoverability not affected). The unit test masks this because the fixture sets $FILE_NAME size = $DATA size (`test_ntfs_volume.cpp:509-510`).
- **Fix**: `probe_size` for non-dir files should be `data_real_size ? data_real_size : best->real_size` (mirroring `logical_size` at :531). Add a fixture with $FILE_NAME size 0 + nonresident $DATA 100 KiB.

### F-A1-3 — P1 — NTFS fixup: OG validates the USN on only the first 2 sector tails; we validate all → we reject records the OG salvages (2 KiB/4 KiB records)
- **Axis**: Behavior / Error surface. **Verdict**: MISMATCH.
- **OUR evidence**: `src/core/ntfs/cr_fixup.cpp:39-43` — `fixup_validate` loops **all** `usa_count-1` sectors comparing each tail u16 against USA slot 0; any mismatch → `ERR_FIXUP_FAILED` → walker counts the record damaged and skips it (`cr_mftwalk.cpp:272-276`).
- **OG evidence**: the actual USN validator `FUN_1800411f0` (`NTFSSearchDLL_decompiled.c:52129-52135`): `for (i = 0; i < uss-1 && i < 2; i++)` — **at most the first two sector tails** (0x1FE, 0x3FE). The apply half `FUN_180041a20` (:52457-52463) then *replaces* every sector tail with its USA slot, so a torn 3rd+ tail is silently repaired rather than rejected. `FUN_180041bb0` (:52513-52543) = validate-only wrapper (apply → attribute-walk check → un-apply via FUN_180041290), matching our validate/apply API split (see MATCH list).
- **Effect**: on volumes with ≥2 KiB MFT records, records with a torn tail in sector ≥2 are recovered by OG and dropped by us. (For the ubiquitous 1 KiB record / 512 B sectors, `uss-1 == 2` and the two behaviors coincide — which is why the test, which uses 1024-byte records, passes.)
- **Fix**: limit the strict per-sector compare to the first two tails (keep the full-apply), or at minimum record this as a documented strictness delta. Corpus principle cited by v1 artifact §9: "Fixup failure → record skipped (never aborts the whole walk — a recovery product maximizes salvage over purity)".

### F-A1-4 — P2 — Fixup offset gate: code requires "nonzero and even"; contract and OG require "not a multiple of 256"
- **Axis**: Constants & offsets / Error surface. **Verdict**: MISMATCH (code vs contract; code vs OG).
- **OUR evidence**: `src/core/ntfs/cr_fixup.cpp:27-28` — `if (usa_offset == 0 || usa_offset & 1) return ERR_BAD_PARAMETER; /* corpus: nonzero */` (comment understates what is implemented).
- **Contract**: `docs/ground-truth-contracts.md §3` — "Fixup min size gate | 0x400; offset must not be 0 mod 256".
- **OG evidence**: `NTFSSearchDLL_decompiled.c:52524-52527` — `uVar1 = FUN_180046160(rec); if ((uVar1 & 0xff) == 0) fail`.
- **Effect**: malformed-header handling only: odd offsets (OG accepts, we reject), 256-multiple offsets (OG rejects, we accept). All sane values (0x30 etc.) pass both.
- **Fix**: implement `(usa_offset & 0xFF) == 0 → ERR_BAD_PARAMETER` and drop the evenness test, or amend the contract to document the deliberate hardening.

### F-A1-5 — P2 — `PFLAG_IN_USE = 0x0010` "corpus mapping: bit4 = in-use" contradicts the corpus props-FLAGS census (0x10 = has-children/folder, 0x20 = deleted)
- **Axis**: Data model / contract drift. **Verdict**: MISMATCH (claimed mapping).
- **OUR evidence**: `include/cr/ntfs/cr_mftwalk.h:30-32` (`PFLAG_IN_USE = 0x0010` "corpus mapping: bit4 = in-use"; `PFLAG_IS_DIR = 0x0002`); same bit used in FAT at `src/core/fat/cr_fat_quick.cpp:196-199` ("corpus PFLAG_IN_USE bit4").
- **OG evidence**: `SaveScan/ifileproperties-part1--ntfs-fat-exfat.md §2` FLAGS census: bit **0x20 = deleted**, bit **0x10 = has-children (folder)**, 0x01 folder-ish/namespace, 0x02/0x04 attribute flags. In the OG walker the record-in-use test is `FUN_180041970` (`decompiled 21155`), and the `0x10` test at :21190 gates *"not a folder → has data runs"* (`ntfs-engine-algorithms-v2 §1`). The v1 deep-dive's "in-use maps to props bit 4" reading (:21186) was superseded by v2.
- **Effect**: internal representation only (our deleted state lives in `DeletedState`), but the citation is wrong and will mislead session-serialization parity work and future audits.
- **Fix**: correct the comments (or re-map bits to the OG scheme if .crsf ever mirrors .rsf semantics); note in the deltas registry if kept.

### F-A1-6 — P2 — "MFT batch read 64 KiB" claimed in contract, not implemented (dead constant)
- **Axis**: Capability claims / Behavior (IO pattern). **Verdict**: MISMATCH (code vs contract).
- **OUR evidence**: `include/cr/ntfs/cr_bootstrap.h:28` — `MFT_FIRST_READ_BYTES = 64 * 1024` is **never referenced** in `src/` (grep: zero uses). `load_mft_stream` reads exactly one record (`cr_bootstrap.cpp:71-73`); `read_record` is per-record; `walk_mft` reads one record per iteration (`cr_mftwalk.cpp:260`).
- **Contract**: `docs/ground-truth-contracts.md §2` — "MFT reads are cluster-strided, batched (reference: 64 KB buffers) — never per-record"; §3 "MFT batch | 64 KB read buffer".
- **OG evidence**: `NTFSSearchDLL_decompiled.c:20731-20736` — `int aiStack_10038[16388]` + `memset(aiStack_10038,0,0x10000)` (64 KiB bootstrap buffer in FUN_18001a3c0); v2 §5 notes 0x1000-granular batch reads re-validated per record slot.
- **Effect**: performance/IO-pattern parity only (our CachedReader mitigates); the contract claim is currently false.
- **Fix**: either implement 64 KiB batched record reads in the walker (preferred: contract §2 is a locked IO discipline) or amend the contract to "cache-backed per-record reads" as a documented delta.

### F-A1-7 — P2 — exFAT: missing stream-dentry first-cluster validity gate (`u32@+0x14 == 1 or >= 0xFFFFFF7`)
- **Axis**: Behavior / Error surface. **Verdict**: MISSING.
- **OUR evidence**: `src/core/exfat/cr_exfat_quick.cpp:317, 322-336` — `se->first_cluster` used without any validity test; only `first_cluster >= 2` gates chain resolution.
- **OG evidence**: `exFATSearchDLL_decompiled.c:1857-1859` (FUN_180003090): `uVar2 = *(uint*)(entry+0x14); if ((uVar2 != 1) && (uVar2 < 0xfffffff7)) goto invalid;` — repeated at :24405, :24553, :26704. Corpus table: `fat-exfat-engine-algorithms-v2.md §3` row 3.
- **Effect**: corrupt stream entries with garbage first-cluster values are accepted by us (chain resolution then fails or follows nonsense); OG rejects the set.
- **Fix**: add the gate when parsing stream extensions (count as `sets_damaged`, mirroring our reserved-field handling).

### F-A1-8 — P2 — exFAT stream reserved-field gate: ours requires all reserved bytes zero; OG requires `+2 == 0` and merely *one* zero byte of each later reserved field
- **Axis**: Behavior / Error surface. **Verdict**: MISMATCH (strictness).
- **OUR evidence**: `src/core/exfat/cr_exfat_quick.cpp:278-279` — `stream_reserved_bad = in_use && (reserved1 != 0 || reserved2 != 0 || reserved3 != 0)` (strict AND across all bytes; live entries only).
- **OG evidence**: `exFATSearchDLL_decompiled.c:1854-1856` — `entry[2]==0 && (entry[6]==0 || entry[7]==0) && (entry[0x10]==0 || (entry[0x11]==0 || entry[0x12]==0) || entry[0x13]==0)`; the type test `(t-0x40)&0x7F` catches live **and** deleted entries alike.
- **Effect**: we mark more sets damaged than OG (e.g. `reserved2 = 0x0100` passes OG, fails us) — `sets_damaged` stat and (for live sets) confidence downgrades diverge. Both engines keep the entry.
- **Fix**: decide policy — replicate the OG's permissive OR-form (parity) or keep strict AND as documented hardening (deltas registry). Also apply to deleted entries if parity is chosen.

### F-A1-9 — P2 — Contract §5 inherits a corpus artifact mislabel: "Allocation bitmap dentry 0x05/0x85" — 0x05/0x85 is the FILE dentry; the bitmap dentry is 0x01/0x81 (our code is correct)
- **Axis**: Constants & offsets / contract drift. **Verdict**: MISMATCH (doc vs code; doc vs decompiled truth).
- **OUR evidence (code, correct)**: `include/cr/exfat/cr_exfat_wire.h:54-61` — `TYPE_BITMAP = 0x01` (0x81), `TYPE_FILE = 0x05` (0x85), `TYPE_STREAM = 0x40` (0xC0), `TYPE_NAME = 0x41` (0xC1).
- **OUR evidence (doc, wrong)**: `docs/ground-truth-contracts.md §5` — "Allocation bitmap dentry | 0x05/0x85".
- **OG evidence**: `exFATSearchDLL_decompiled.c:1848` — the `(t-5)&0x7F` branch calls `FUN_180002f90(entry+1)`, which (decoding :1779-1798) validates the **file dentry's own fields**: secondary_count ∈ [2,18] (`*p - 2 < 0x11` on byte +1), valid create/modify timestamps (FAT date validators FUN_180002f40 on +8/+12), `reserved1 u16@+6 == 0`, reserved2 bytes +0x19..+0x1F zero. The backward triple reconstruction `FUN_180003120` (:1905-1929) sorts `{0x85, 0xC0, 0xC1}` into three parallel lists — i.e. 0x85 is unambiguously the file dentry of the triple. The corpus artifact `fat-exfat-engine-algorithms-v2.md §3` row 2 mislabeled this ("allocation-bitmap dentry … checks the bitmap descriptor fields"), and our contract copied it verbatim.
- **Fix**: correct contract §5 (bitmap = 0x01/0x81 per code + spec); optionally feed the mislabel back to the corpus repo's artifact errata.

### F-A1-10 — P2 — Viability gate boundary: ours `size >= 0x3FF`, OG `size > 0x3FF` (i.e. ≥ 0x400)
- **Axis**: Constants / Behavior. **Verdict**: MISMATCH (one-file-of-1023-bytes boundary).
- **OUR evidence**: `include/cr/ntfs/cr_mftwalk.h:39` (`viability_min_size = 0x3FF`) + `cr_mftwalk.cpp:486` (`probe_size >= flags.viability_min_size`).
- **OG evidence**: `NTFSSearchDLL_decompiled.c:21208` — `if (0x3ff < lVar18)`. The artifact prose (`ntfs-engine-algorithms-v2 §1`: "if size >= 0x3FF") mis-transcribes the strict compare; our contract §3 copied the prose.
- **Fix**: change the gate to `probe_size > 0x3FF` (or set `viability_min_size = 0x400` with `>=`) and align contract wording to the decompiled strictness.

### F-A1-11 — P2 — FAT engine walk has no responsive cancellation (contract §2 claims cancel in every loop head)
- **Axis**: Threading / Error surface / contract drift. **Verdict**: MISMATCH (code vs contract).
- **OUR evidence**: `src/core/fat/cr_fat_quick.cpp:265-266` — `cancel` checked once after the boot read; `walk_dir_entries` / `emit_entry` / `recurse_dir` (:110-252) never check again.
- **Contract**: `docs/ground-truth-contracts.md §2` — "Cancel callback consulted inside every loop head (responsive cancellation)."
- **OG evidence**: `FATSearchDLL_decompiled.c:11236-11238` — cancel via `vt+0x88` inside the chain-follow loop head (corpus v2 §2 [CODE]).
- **Fix**: thread the cancel check into `walk_dir_entries`' entry loop and `recurse_dir` (NTFS checks per record at `cr_mftwalk.cpp:254`; exFAT at `cr_exfat_quick.cpp:211` — FAT is the outlier).

### F-A1-12 — P2 — Deleted FAT entry naming: we restore `'_'` as first char; OG persists the raw 0xE5-marked bytes
- **Axis**: Behavior / user-visible results. **Verdict**: DELIBERATE-DELTA (undocumented).
- **OUR evidence**: `src/core/fat/cr_fat_quick.cpp:142-143` — `if (deleted) short_name[0] = '_';` ("undelete convention"); tests assert `"_EL.TXT"` / `"_ONE.TXT"` (`test_fat_volume.cpp:209-211`).
- **OG evidence**: `ifileproperties-part1 §3` [CENSUS]: FAT names "observed mangled control-byte names (`\x05\x01\x04\x01…`) = recovered deleted dir entries whose first char(s) were 0xE5/0x05-marked" — the OG emits the raw short-name bytes including the deletion marker.
- **Effect**: cosmetic result-name divergence (ours arguably better UX); should be registered as a deliberate delta rather than silently differing.
- **Fix**: deltas-registry entry; no code change required.

### F-A1-13 — P3 — Dead/incorrect helper: `is_dot_entry` never called and checks the wrong byte (nt_reserved +0xC) vs OG's full 11-byte '.' signature
- **Axis**: Constants & offsets / hygiene. **Verdict**: MISMATCH (dead code) + doc drift.
- **OUR evidence**: `include/cr/fat/cr_fat_wire.h:144-149` — requires `name[1..7]==' '` **and `nt_reserved == ' '`** (offset +0xC); zero call sites in src/tests (grep verified). The runtime path uses a looser inline check (`cr_fat_quick.cpp:132-135`).
- **OG evidence**: `FATSearchDLL_decompiled.c:11283-11285` — `*(u64*)buf == 0x202020202020202e && (short)(buf+8) == 0x2020 && buf[10]==' ' && (attr & 0x10)`; the '..' form at :11295-11297. OG uses the signature to *validate directory identity* (mismatch → directory rejected); ours merely filters dot entries from results.
- **Fix**: delete `is_dot_entry` or fix it to the OG signature and use it where the inline check stands; keep the identity-validation semantics gap in mind (ours accepts, OG rejects on mismatch — currently a silent divergence).

### F-A1-14 — P3 — NTFS metafile filter: known-name list vs OG blanket `name[0]=='$'` (deliberate, documented, tested)
- **Axis**: Behavior / State machine. **Verdict**: DELIBERATE-DELTA.
- **OUR evidence**: `src/core/ntfs/cr_mftwalk.cpp:220-229` (`is_metafile_name`, 20 names) with in-code rationale (blanket '$' skip swallowed `$Recycle.Bin`/`$Extend` subtree content); test asserts `stats.meta_skipped == 7` (`test_ntfs_volume.cpp:1179`).
- **OG evidence**: `NTFSSearchDLL_decompiled.c:21186` — `(flags & 4) != 0 || name == NULL || name[0] == '$'` → Release+skip; also recycle-dir name checks `Recycler`/`Recycled`/`$Recycle.Bin`/`$RECYCLE.BI0` at :37388-37408 (case-insensitive) showing the OG does special-case bin dirs despite the blanket filter.
- **Fix**: already documented in code; add to the deltas registry (user files named `$…` surface in ours, not in OG).

### F-A1-15 — P3 — OG fixup uses a hardcoded 0x200 (512 B) sector stride; ours is sector-size-parametrized (4Kn-aware) — deliberate delta (fix F1), registry entry needed
- **OG evidence**: `NTFSSearchDLL_decompiled.c:52458-52462` (apply: `(i+1)*0x200`), `:52132` (validate), `:52156-52160` (un-apply). The v1 artifact's "each 512-byte sector's slot at +0x1FE" matches.
- **OUR evidence**: `src/core/ntfs/cr_fixup.cpp:25, 40, 55` — `sector_size` parameter, validated 256..65536.
- **Note**: on true 4Kn volumes the OG's fixed stride under-fixes (an OG quirk); ours is more correct. Keep as documented delta.

### F-A1-16 — P3 — No-bitmap viability case: ours grades `PartialRisk`; OG keeps the record ungraded
- **OUR**: `cr_mftwalk.cpp:495-497`. **OG**: `decompiled 21209-21214` — bitmap object null → viability skipped, record still added (LAB_18001afe6).
- Minor confidence-display divergence; ours arguably more honest. Delta-registry candidate.

### F-A1-17 — P3 — OG destroys fully-reallocated deleted records; ours keeps and grades `Overwritten`/`PartiallyOverwritten` (deliberate, fix F5 — but the `keep_overwritten` flag is dead)
- **OUR**: `cr_mftwalk.cpp:489-494` (grading), `include/cr/ntfs/cr_mftwalk.h:36` (`keep_overwritten` never read — grep: zero uses in src/).
- **OG**: `decompiled 21249-21257` — `if ((0 < total) && (intact == total)) goto destroy` (Release + skip). Rule confirmed exactly: all-clusters-reallocated → destroy, partial → keep.
- **Fix**: remove the dead flag or wire it (destroy mode for OG-exact behavior); keep the grading default as the documented delta.

### F-A1-18 — P3 — FAT flavor detection: cluster-count thresholds only; OG additionally `_strnicmp`s the OEM label ("FAT16   " at +0x36)
- **OUR**: `cr_chain.cpp:12-19` (4085/65525 thresholds, Microsoft whitepaper) + shape guess in `validate_boot` (:262-270). **OG**: `fat-engine--deep-dive.md §2` — `_strnicmp(boot+0x36,"FAT16   ",8)` @38048, FAT32 label at +0x52, type strings at 9860-9863/27904-27907/34341-34345.
- Ours is more tolerant of relabeled volumes. Delta-registry candidate.

### F-A1-19 — P3 — exFAT root opening triple (0x83 followed by 0x81 at +0x20 and 0x82 at +0x40) validated by OG, not by us
- **OG**: `exFATSearchDLL_decompiled.c:1847` (`entry[0x20] != 0x81 || entry[0x40] != 0x82` → invalid triple). **OUR**: `cr_exfat_quick.cpp:258-261` — volume-label/upcase entries skipped individually, no sequence validation. Also: OG's ChecksumDBR-equivalent boot validation is not mirrored for sector 23's fill pattern (we check sector 11 only, `cr_exfat_dentry.cpp:121-132`).

### F-A1-20 — P3 — Doc hygiene: `LfnEntry.sequence` comment claims "0x20 was deleted" — no such VFAT semantic (sequence = bits 0-5 number, bit 0x40 last-fragment); contract §3 cites metafile filter at "21170" (actual: 21186); `MFT_RESERVED_RECORDS` and `USN_REASON_FILE_CREATE` are declared-but-unused.
- **OUR**: `include/cr/fat/cr_fat_wire.h:109`; `docs/ground-truth-contracts.md §3`; `include/cr/ntfs/cr_bootstrap.h:27`; `include/cr/ntfs/cr_mftwalk.h:44`.

---

## Verified MATCHes (must-check items, evidence both sides)

| Item | OUR (file:line) | OG (file:line / §) | Verdict |
|---|---|---|---|
| 'FILE' magic 0x454C4946 | cr_ntfs_wire.h:57 | decompiled 21152, 34153-34161, 55194 | MATCH |
| Record-size clamp ≥ 0x400 | cr_bootstrap.cpp:52-53; cr_fixup.cpp:18 | decompiled 21139-21142; 52520 | MATCH |
| Fixup = validate-only wrapper (apply→check→restore), validate/apply split | cr_fixup.h:22-34 | FUN_180041bb0 @52513-52543 (FUN_180041290 un-apply @52144) | MATCH (scope caveat F-A1-3) |
| $ATTRIBUTE_LIST (0x20) probe + fragmented-MFT handler + trace-equivalent path | cr_bootstrap.cpp:130-187, 199-287; nested chase cr_mftwalk.cpp:305-402 | decompiled 20741/20755 (log string verbatim), FUN_18001a540; NTFSLib `mftwith0x20.cpp` trace (MOM-modules/NTFSLib.md:31) | MATCH |
| Attrlist cycle guard + depth limit | cr_bootstrap.cpp:200-232; cr_mftwalk.cpp:312-331 | corpus v2 §4 detector; chase mechanics INFERRED in corpus — our guards are hardening | MATCH+ |
| $FILE_NAME 0x30 / $OBJECT_ID 0x40 / $DATA 0x80 types | cr_ntfs_wire.h:86-92 | decompiled 51006/52190/53409/54487 (verified by sed) | MATCH |
| Deleted branch = record in-use bit (0x0001) | cr_ntfs_wire.h:79; cr_mftwalk.cpp:474 | FUN_180041970 gate @21155 (v2 §1) | MATCH |
| Viability: all-clusters-reallocated → destroy-equivalent, partial → keep | cr_mftwalk.cpp:486-494 | decompiled 21249 (rule), 21215-21248 (loop) | MATCH (graded-not-destroyed = delta F-A1-17) |
| $Bitmap bit test `buf[pos>>3] & (1<<(pos&7))` + base/limit | cr_bitmap.h:29-38 | FUN_18002c2e0 @35643-35657 | MATCH (index space: OG sector-granular `spc*cluster`, ours cluster-granular on-disk $Bitmap — semantically equivalent; build loop UNVERIFIED) |
| Runlist nibble order LOW=length / HIGH=offset | cr_runlist.cpp:71-76 | ifileproperties §2 nibble-walk [CODE]; decompiled 2378 sum is order-agnostic; corpus v1 §7 prose ("high=length") is the misread our contract already corrected | MATCH (documented correction) |
| Runlist signed offsets, sparse = off_width 0, terminator 0 | cr_runlist.cpp:84-109 | ifileproperties §2; spec | MATCH |
| MFTMirr cross-check (records 0..3, repair source) | cr_bootstrap.cpp:440-446, 451-462 | decompiled 52032/52102 (`L"$MFTMirr"`); v1 §5 | MATCH |
| Hard links: per-parent nodes, shared record_ref | cr_mftwalk.cpp:509-617; test 1244-1250 | concept-level only (refcounted props; census +0x40 parent bands) | MATCH (details UNVERIFIED) |
| ADS: named $DATA child nodes, resident content capture | cr_mftwalk.cpp:620-665 | v1 §5 "attribute-stream assembly" (complete scan) | MATCH (details UNVERIFIED) |
| Remnant shifted-header salvage: 'FILE'+'0' probe at +0x58 else +0x60, ≥0x70 window, zero-filled buffer, parse WITHOUT fixup | cr_mftwalk.cpp:958-1026 | FUN_18002a780 @34151-34186 — byte-exact (`0x454c4946` + `'0'` at +0x5C; `0x70 < size < 0x400`; memset 0x400; FUN_180041970+FUN_1800431d0, no FUN_180041bb0) | MATCH (we copy full record vs OG's <0x400 window — documented delta) |
| 64 KiB bootstrap buffer | (constant dead — F-A1-6) | decompiled 20731-20736 | see F-A1-6 |
| FAT canonical BAD 0xFFFFFF7 / EOC ≥0xFFFFFF8 (FAT16/32) | cr_fat_wire.h:126-135; cr_chain.cpp:38-60 | FUN_180041c80 @54135-54141 | MATCH (FAT12 broken — F-A1-1) |
| cluster→sector `(c-2)*spc + first_data_sector` | cr_chain.cpp:21-25 | decompiled 11239 | MATCH |
| Deleted marker 0xE5 (+ 64 KB chain guard) | cr_fat_wire.h:80; cr_fat_quick.cpp:137 | decompiled 14634/29978 (`-0x1b` + `< 0x10000`), 6 sites | MATCH |
| LFN: VFAT slots + checksum vs 8.3, corrupt → fallback | cr_lfn.cpp:14-51; test 245-262 | fat-engine--deep-dive §4 ("standard algorithm", 0x40 flag sites 11442/28446/33978/44355) | MATCH (byte loop UNVERIFIED) |
| Cycle guard (visited set / OG RB-tree ranges) | cr_chain.cpp:170-183, 214-221 | decompiled 11250-11266 (std::map range insert) | MATCH |
| Backup-boot fallback at sector 6 (4Kn-aware) | cr_fat_quick.cpp:271-287; test 339-382 | rawfixer-v2 §"FATFixer CheckBackupDBR" [CODE method] | MATCH |
| Mirrored FAT choose-healthy | cr_chain.cpp:75-87, 91-111 | CDoubleFATTable (fat-exfat-engine-internals §2; fallback switch INFERRED in corpus) | MATCH (inference) |
| Deleted-chain reconstruction + confidence caps | cr_chain.cpp:201-235; cr_fat_quick.cpp:73-83, 226-239 | fat-exfat v1 §5 "deleted-entry chain reconstruction + cross-FAT recovery" | MATCH (caps are our design; UNVERIFIED specifics) |
| Flavor thresholds 4085/65525 | cr_chain.cpp:12-19 | MS whitepaper (corpus dispatches by context+OEM — F-A1-18) | MATCH (spec) |
| exFAT InUse bit 0x80; deleted keeps type with 0x80 cleared | cr_exfat_wire.h:51-52, 137-142 | decompiled 1846-1863 (`(t-X)&0x7F` tests); v2 §3 | MATCH |
| exFAT dentry types 0x81/0x82/0x83/0x85/0xC0/0xC1 | cr_exfat_wire.h:54-61 | decompiled 1846-1864, 1905-1929 (triple lists) | MATCH (contract doc mislabel F-A1-9) |
| NoFatChain contiguous resolution | cr_exfat_quick.cpp:74-83, 324-329 | exfat deep-dive §3 | MATCH |
| Volume length u64 | cr_exfat_wire.h:28; cr_exfat_dentry.cpp:90-93 | spec (corpus silent) | MATCH (spec) |
| Allocation-bitmap grading of deleted exFAT files | cr_exfat_quick.cpp:373-391 | CQuickSearchAndBuildBitmap (exfat deep-dive §2) | MATCH (concept) |
| Boot-region checksum ROL31, skip 106/107/112, fill-repeated sector 11 | cr_exfat_dentry.cpp:12-37, 117-132 | rawfixer-v2 §4 ChecksumDBR (method [CODE], byte loop [INFERRED]) | MATCH (algorithm family); sector-23 pattern unchecked (F-A1-19) |
| FS type codes: FAT 0/1/2, NTFS 3, exFAT 7 | cr_mftwalk.cpp:529; cr_fat_quick.cpp:185; cr_exfat_quick.cpp:344 | engine-matrix §2 + ifileproperties §1.1 | MATCH |
| Error family 0x5002xxxx, success 0 | cr_error.h:18-27 | contract §1; ntfs deep-dive §9 | MATCH |
| Salvage-over-purity (skip, never abort walk) | cr_mftwalk.cpp:260-282 | ntfs deep-dive §9 | MATCH |
| Contract §3-§5 evidence pointers resolve | — | all spot-checked lines verified (21152, 34153-34161, 55194, 52190, 53409, 51006, 54487, 7163, 52102, FUN_180041bb0/18002c2e0/180041850) | MATCH (two citation drifts: F-A1-10 boundary, F-A1-20 "21170") |

---

## UNVERIFIED (corpus silent or not decoded — no verdict claimed)

1. **UsnJrnl harvest specifics**: `$UsnJrnl` cross-check sites exist (`decompiled 9070, 19761, 19868, 20282`; USN-indexed RB-trees at controller +0x8f0/+0x900), but no USN_RECORD_V2/V3 parse loop, FILE_DELETE (0x20000) reason filter, root_lost placement, or dedup semantics are evidenced in the artifacts read. Our implementation (`cr_mftwalk.cpp:703-842`) is winioctl.h-spec-derived.
2. **$Recycle.Bin $I/$R mapping in the NTFS engine**: recycle-dir recognition verified (`decompiled 37388-37408`, incl. the `$RECYCLE.BI0` deleted-form); the FAT engine's `IRecycleBinNode2UI` "$I…/$R… pairing by index" is attested (fat-exfat v1 §4), but the NTFS-side $I v1/v2 parse (our `parse_recycle_info`, `cr_mftwalk.cpp:160-215`) has no corpus algorithm-level counterpart.
3. **Hard-link node model** (per-link props cloning, canonical-first-link ADS attach) — census shows parent-reference bands only.
4. **exFAT entry-set 16-bit checksum recompute** during scan (our `entry_set_checksum`, spec-derived; no ROL15 loop located in the decompiled greps; capa's "FNV hash" note is a different mechanism).
5. **exFAT boot-region checksum exact byte loop** (rawfixer-v2 flags it [INFERRED]; our skip-106/107/112 loop is spec 3.1.14.1).
6. **CDoubleFATTable exact primary/mirror fallback switch** (corpus itself flags [INFERRED]).
7. **OG props size source** feeding the 0x3FF gate (FUN_180006db0 not decoded) — relevant to F-A1-2.
8. **Threading model specifics** (OG Vista condition-variable workers; which loops run parallel). Our remnant scan is threaded; walk_mft/scan_fat/scan_exfat are single-threaded.
9. **$J sparse-run materialization + 8-byte resync** (our design; corpus silent).
10. **NTFS `.` root reparenting / orphan→root_lost placement details** (concept attested: v1 internals "reparenting into lost tree").
11. **MFTMirr "volume midpoint" placement** — our code reads mirror geometry from record 1's copy at `mftmirr_lcn + record_size` (cr_bootstrap.cpp:291-315); corpus attests the cross-check but not the mirror-record discovery mechanism.

## Deliberate-delta candidates (for the deltas registry)

1. **Grade-don't-destroy** for fully-reallocated deleted NTFS records (fix F5; F-A1-17). Claimed justification: product UX "possibly damaged but listed".
2. **Known-metafile list** vs blanket `name[0]=='$'` skip (F-A1-14) — recovers `$Recycle.Bin`/`$Extend` subtree content and user `$`-named files.
3. **4Kn-aware fixup sector size** + backup-boot at `6*logical_sector` (fix F1; F-A1-15) vs OG hardcoded 0x200 stride.
4. **Shifted-header salvage copies the full record** (vs OG's `< 0x400` truncated window) — recovers record tails.
5. **FAT deleted-name `'_'` restoration** vs OG raw 0xE5-marked bytes (F-A1-12).
6. **Block-buffered (4 MiB) remnant scan + optional thread pool** vs OG per-sector probes — IO-pattern optimization, semantics preserved.
7. **No-bitmap → PartialRisk** vs OG ungraded-keep (F-A1-16).
8. **Strict fixup validation of all sector tails** (F-A1-3) — currently *undocumented* strictness; either fix or register.
9. **Strict exFAT reserved-field AND-gate** (F-A1-8) — same: fix or register.
10. **FAT flavor by cluster count only** (F-A1-18) — tolerant of OEM-label damage.

## Recommended fix-wave order

1. F-A1-1 (FAT12 canonicalization + test) — small, user-visible correctness.
2. F-A1-2 (viability gate field) — one-line fix, restores grading integrity.
3. F-A1-3 (fixup first-2-tails validation) — small, restores salvage parity on ≥2 KiB records.
4. F-A1-6 (64 KiB batch or contract amendment) — contract integrity.
5. F-A1-7/-8 (exFAT gates), F-A1-4 (fixup offset gate), F-A1-10 (0x3FF boundary), F-A1-11 (FAT cancel).
6. Doc wave: F-A1-5, F-A1-9, F-A1-20; dead-symbol cleanup F-A1-13/-17.
