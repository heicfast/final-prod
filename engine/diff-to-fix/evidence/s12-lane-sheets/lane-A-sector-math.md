# LANE A — SECTOR NUMBERS / LBA MATH — deep-dive audit (read-only)

Auditor: LA (read-only lane). Core tree: `/home/z/audit/core_recovery` @ content `8d0d88b8`. Corpus:
`/home/z/audit/easeus-drw-research` (be0cfe9). Session directive: **"Special focus on sector numbers."**

Method: every claim below was read first-hand on both sides this session (decompile line + core
file:line). Already-filed findings (S-040…S-050, M-073…M-117, N-090…N-152, I-026…I-034) and
registered deltas in `docs/PARITY-DELTAS.md` are cited as COVERED/REGISTERED, not re-filed.
Corpus decompile paths abbreviated: `P/` = `re-workspace/reversed/ghidra-mo/Partition_decompiled.c`,
`FAT/` = `re-workspace/reversed/ghidra-searchdll/FATSearchDLL_decompiled.c`,
`NTFS/` = `…/NTFSSearchDLL_decompiled.c`, `exFAT/` = `…/exFATSearchDLL_decompiled.c`,
`ZFS/` = `…/ZfsSearchDLL_decompiled.c`, `DRWUI/` = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`.
Core paths relative to `/home/z/audit/core_recovery/`.

---

## 1. CONTEXT SHEET — every sector-math surface examined

### Area 1 — partition handling (MBR / GPT / EBR)

| # | Surface | og (cite) | core (cite) | Verdict |
|---|---|---|---|---|
| 1.1 | MBR entry LBA fields (u32 lba_start/sector_count at +8/+0xC) | read at `P:990` (`*(char*)(e+4)` type), `P:993` (`*(uint*)(e+8)` rel LBA) | `cr_table_wire.h:30-38` (u32 wire fields), `cr_table_parse.cpp:385-387` | **MATCH** (u32 wire, u64 math both sides) |
| 1.2 | GPT header verification strictness | `FUN_1800078d0` `P:4805-4856`: sig + revision 0x10000 + **header_size==0x5C exactly** (`:4814`) + **reserved==0** (`:4814`) + current_lba==1 (primary) / ==sector_count−1 (backup) (`:4815-4820`) + backup_lba==sector_count−1 (primary) / ==1 (backup) (`:4822-4827`) + first_usable<sector_count ∧ first_usable<last_usable ∧ last_usable<sector_count (`:4828-4830`) + entries_lba OUTSIDE [first_usable,last_usable] (`:4831-4832`) + entry_size==0x80 (`:4833`) + CRC over fixed 0x5C bytes (`:4837`) | `load_gpt_side` `src/core/table/cr_table_parse.cpp:29-70`: sig + entry_size==128 (`:38`) + entry_count∈[1, 8192] cap 1 MiB (`:40`) + **header_size∈[92,512]** (`:42`, CRC over header_size bytes `:49`) + header/entries CRCs — **NO reserved/position/usable-range checks** | **DIVERGENT — LA-1** |
| 1.3 | GPT backup-header probe position | analyzer always reads backup at **device sector_count−1** (`FUN_180006e20` `P:4567`: `param_1[1] + -1`), both when primary verified OK and when salvage succeeded | primary OK → probe at **prim.backup_lba** (`cr_table_parse.cpp:323`); primary dead/salvage-failed → dev_last (`:343`, `:353`) | **DIVERGENT — LA-2** |
| 1.4 | Dead/unreadable LBA 1 | `IsGPTDisk` (CGPTDiskAnalyzer) probes **last LBA** when LBA1 is dead or non-GPT (`P:4208-4218`) — but the analyzer `FUN_180006e20` **gives up** when the LBA1 read fails ("Failed to read GPT HEADER." → free+return, `P:4517-4524`); backup entries are then never emitted on that path | LBA1 unreadable → `lba1_gpt=false` → primary skipped → salvage skipped → **backup at dev_last probed and emitted** (`cr_table_parse.cpp:339-348`) | **DIVERGENT — LA-2** (core superset) |
| 1.5 | GPT primary-dead salvage (entries from untrusted primary header) | `FUN_180006e20` `P:4530-4560`: alloc entry_count×0x80 (64-bit saturating), read entry_size×entry_count bytes at entries_lba×device-sector-size (`:4552`), verify entries CRC (`FUN_180007b50` `P:4861-4878`); success ⇒ try_backup=0 | `salvage_primary_entries` `cr_table_parse.cpp:82-112` (same shape; adds entries_start_lba bounds check `:100`) | **MATCH** (core stricter on the array-bounds; the two-step order — salvage before backup — matches, cite `:4556-4560` ⇔ `:326-337`) |
| 1.6 | GPT entry→partition span | entries copied raw 128 B (`FUN_1800085b0` `P:5273-5286`), consumers walk stride **0x80** (`FUN_180007640` `P:4703`); og span consumption (first_lba/last_lba→volume start/size) **not located** (OQ-1) | `emit_gpt_entries` `cr_table_parse.cpp:273-274`: `start=first_lba; sector_count=last_lba−first_lba+1` — **no first≤last check** (underflow passes the `!=0` gate `:297`) | **DIVERGENT — LA-3** (core-side defect candidate; og side OQ-1) |
| 1.7 | GPT entry_size != 128 / entry-count cap | og **hardcodes 0x80 stride** in alloc/consumers but reads entry_size×entry_count bytes (`P:4541` int multiply) — mis-walks non-128 arrays; no count cap (alloc-bound) | core **rejects** entry_size != 128 (`cr_table_parse.cpp:38`) and caps entries at 1 MiB (`:15,:40`); in-code comment documents "Stricter than the og" (`:76-81`) but **NOT registered in PARITY-DELTAS** | **DIVERGENT — LA-7** (unregistered deliberate-strictness) |
| 1.8 | GPT entries-array read offset | `entries_lba × device sector size` (`P:4552`, vtable-provided) | `read_sectors`: `lba × geometry.logical_sector` (`cr_table_parse.cpp:21`) — parameterized, byte-bounds checked (`:23`) | **MATCH** (D-04 byte-anchored geometry both sides; note `lba×ls` is u64-wrap-able for hostile lba≥2^55 — theoretical, CRC-gated) |
| 1.9 | EBR chain link math | `FUN_180002190` `P:983-1014`: `next = base + e.lba_start` (`:993`), first link also advances `base += e.lba_start` (`:994-997`); starts by reading LBA 0 (MBR) so base = MBR extended entry LBA | `parse_ebr_chain` `cr_table_parse.cpp:155-219`: `base = first_ebr_lba`, `next_ebr = base + e.lba_start` (`:192,:197`); MBR extended entry → `ebr_base` (`:374`) | **MATCH** (identical base/rel semantics) |
| 1.10 | EBR link TYPE SET | og links on **{0x05, 0x0F, 0x85}** (`P:991` `'\x05' || '\x0f'`; `P:1006` `-0x7b` = 0x85 Linux-extended → same link path); 0x42 'B' exits to the LDM path (`P:1001-1004`) | `mbr_type_to_kind`: only **{0x05, 0x0F}** → Extended (`cr_table_parse.cpp:118-120`); 0x85 → default `FsKind::Other` (`:144-146`) → emitted as a *volume* at `walk+rel` (`:203-211`) and the chain **stops** | **DIVERGENT — LA-6** |
| 1.11 | EBR cycle / depth guards | og: self-loop check only (`while (prev != next)` `P:983`) — a 2-cycle A→B→A loops forever; no depth cap | core: visited-set cycle guard + `EBR_DEPTH_LIMIT 1024` (`cr_table_parse.cpp:158-165`, `cr_table_parse.h:99`) | **COVERED-BY-D-14** (F7 hardening family, registered) |
| 1.12 | EBR torn-link salvage | og: read-fail / no-link → break, walked set kept (`P:986,:1013`) | core: `break_lba` + `EbrChainStatus` prefix retention (M-002) (`cr_table_parse.cpp:166-175,:403-421`) | **COVERED-BY-M-002 (FIXED)** |
| 1.13 | EBR "entry 0 links next" tolerance | not decoded on og side (OQ-3) | core heuristic: extended entry in slot 0 before any volume → treated as next-link (`cr_table_parse.cpp:189-200`) | **OPEN** (core-only tolerance; no og contradiction found) |
| 1.14 | Partition-start rebasing (all offsets derived from partition start) | volume read context: absolute sector = `LCN × SectorPerCluster + StartSector + runIdx` (`DRWUI:875657` `*plVar6 * local_230 + local_248 + lVar4`; bounds vs device sector count `:875660-875664`) | `PartitionSlice::create(slice, start_lba×ls, sector_count×ls)` (`cr_scan.cpp:1680`; bounds `cr_slice.cpp:15-31`) — engines emit slice-relative byte offsets | **MATCH** (equivalent formula, u64 both sides; core byte-anchored per D-04) |
| 1.15 | GPT type-GUID constants (LDM meta/data) | `P` DAT_180036630/40 (provenance per A-002) | `cr_table_wire.h:81-94` | **MATCH** (A-002 FIXED) |

### Area 2 — cluster→LBA conversions

| # | Surface | og (cite) | core (cite) | Verdict |
|---|---|---|---|---|
| 2.1 | FAT cluster→sector multiply WIDTH | `(c−2)*spc + start` with **32-bit multiply** at read/geometry sites: `FAT:11239` `(ulonglong)((uVar7−2)*(int)param_1[4]) + param_1[8]`, `FAT:13194` (explicit `(uint)` cast), `FAT:28172`, `FAT:39068-39080` (`FUN_180030d90`, the CalcPartition geometry helper; decoded in `evidence/decompiled-quotes/s3-calcpartition-scoring.md:49-55` — width unremarked there). (64-bit sites also exist, e.g. the runlist factory `FAT:28045` `(longlong)(run+(c−2))×spc`.) | `FatTable::cluster_offset` `(u64(c)−2)*cluster_bytes + first_data_sector×bps` — **full u64** (`src/core/fat/cr_chain.cpp:24-28`; header contract `cr_fat_engine.h:9`) | **DIVERGENT — LA-4** (og wraps >2 TiB data areas; core correct) |
| 2.2 | FAT flavor brackets / EOC / BAD sentinels | `FUN_180007e50 :5219-5231`; mask-24 EOC family `:8030-8048` | `cr_chain.cpp:12-22` (N-010a), `:30-88` (F-A1-1, N-002) | **COVERED** (N-010a / F-A1-1 / N-002 — FIXED) |
| 2.3 | FAT first_data_sector / boot geometry | BPB ladder (fat-exfat-engine-algorithms-v2.md:40,53) | `validate_boot` `cr_chain.cpp:534-619` (first_data u64 P1-15, spc og-set gate, ≥0xFFFFFF0 reject) | **MATCH** (PARITY-CORE-FS1.md:194; M-045/N-010b/N-034 FIXED) |
| 2.4 | FAT run-end vs volume-limit refusal | runlist factories refuse + destroy the runlist when `(run_len+(c−2))×spc > volume sector limit` ("File overflows partition!" `FAT:10903`, `FAT:28070`; check `FAT:10873-10876`, `:28045-28048`, 64-bit) | no per-run guard; extents emitted (`cr_fat_quick.cpp:370-386`), out-of-slice reads fail at recovery (Salvaged path) | **DIVERGENT — LA-8** (nuance: og drops at scan, core defers failure to read time) |
| 2.5 | NTFS runlist nibble order + lengths | decoder `FUN_1800347d0` `NTFS:41853-41970`: off_width=hdr>>4, len_width=hdr&0xF (`:41883-41884`); length = LE bytes (`:41895`) | `cr_runlist.cpp:71-84` (same order; corpus-prose misread corrected) | **MATCH** (B2, ntfs.md) |
| 2.6 | NTFS runlist delta sign-extension | 1..8-byte cases: (char)/(short)/bit23−0x200000/bit39−0x2000000000/bit47/bit55 (`NTFS:41898-41927`) | `read_signed_le` `cr_runlist.cpp:15-27` (identical extension points) | **MATCH** |
| 2.7 | NTFS LCN accumulation / sparse | u64 accumulator + signed delta (`NTFS:41937-41939,:41962`); sparse = off_width 0, stored LCN −1 marker (`:41937-41941`) | u64 wrap-add (`cr_runlist.cpp:104-105`); sparse flag (`:94,:110`); INT64_MIN rejected (`:100`, UBSan) | **MATCH** (two's-complement-equivalent; INT64_MIN rejection = core hardening) |
| 2.8 | NTFS runlist malformed-input tolerance | len_width>8 → **break, return success** (keep runs so far, `NTFS:41891`); off_width>8 → return success (`:41992-41994`); zero-length runs accepted; buffer-end → return success (`:41875,:41888`); pre-validator `FUN_18001d5f0` **rejects** first-byte nibble==0 (sparse-first / zero-len-width) `NTFS:23184` | len_width==0 or >8 → `ERR_BAD_PARAMETER`; run_len==0 → `ERR_BAD_PARAMETER`; truncation → `ERR_TRUNCATED` (`cr_runlist.cpp:77-86`); decode failure drops the fragment's runs (`cr_mftwalk.cpp:561-567` — `has_data` not set) | **DIVERGENT — LA-5** |
| 2.9 | NTFS $MFT LCN→byte; DBR decode | MFT start sector via $DATA of record 0 (family doc §5, "MFT start sector is %I64d"); engine LCN×spc probe (D-56 quote :21220-22) | `mft_lcn × cluster_bytes` u64 (`cr_bootstrap.cpp:101`); signed-SPC `1<<(256−spc)` decode (`:47-51`, N-011b ⇔ `P`-side LoadNtfsVolume :19308-19312) | **MATCH / COVERED** (N-011b+M-001 FIXED; D-56 REGISTERED for the pass-2 claim-map spc×LCN inconsistency) |
| 2.10 | exFAT cluster→sector | **64-bit** `(u64)(c−2)×(u64)spc + base` (`exFAT:12999-13001`, `:13091-13094`, `:13115-13117`) | `(u64(c)−2)*cluster_bytes + cluster_heap_offset×bps` u64 (`cr_exfat_quick.cpp:146-147`) | **MATCH** (contrast: og FAT sites are 32-bit, og exFAT is 64-bit — width inconsistency is og-internal) |
| 2.11 | exFAT allocation-bitmap bit mapping | (M-054 decode) | `Bitmap::test` bit `c−2` (M-054 fix) | **COVERED-BY-M-054 (FIXED)** |
| 2.12 | ext4 block-group / block math | first_data_block×log_block_size kernel rules (ext4-hfs-engine-algorithms-v2.md:18-31); reads strided `(block+param_2)*geometry` (deep-dive :28) | `cr_ext4_engine.cpp:150-167` (block_size `0x400<<log`, groups math, 64-bit blocks_hi), `:272,:389` | **COVERED** (module M verdicts; og rebase defect NOT adopted = **REGISTERED D-59**) |
| 2.13 | btrfs logical→physical chunk/stripe math | RAID10 geometry gate :2162-2168; 2-copy rotating mirror :2534-2540 (module decode) | `cr_btrfs_engine.cpp:158-232` (find_chunk/logical_to_physical; RAID0 `idx=stripe_nr%n`, RAID10 sets; mirror rotation) | **COVERED** (M-020 FIXED → MATCH post-fix; **REGISTERED D-79→D-83** renumber noted in PARITY-DELTAS header) |
| 2.14 | ZFS DVA / ashift math | `<<9` / `×0x200` (`ZFS:2726`, `:2740`, `:18002-18012`); +4 MiB vdev skip (`(x+0x2000)×0x200` `ZFS:2735`) | `dva_asize/dva_offset` `<<9` (`cr_zfs_engine.cpp:143-147`); psize/lsize `((p&0xFFFF)+1)<<9` (`:120,:126`) | **MATCH** (spec-fixed 512 units both sides; ashift 10-13 probe = **REGISTERED D-04**; multi-vdev = M-019/I-032 COVERED) |
| 2.15 | ReFS cluster translation / dataruns | `FUN_180011e90 :13223-13267` (container map) | `translate_cluster` identity/container map (`cr_refs_engine.cpp:92-121`); datarun 0x40-flag grammar (`:380-405`) | **COVERED-BY-N-036** (half-fixed per modules/refs.md:202-204; residual = container map decode, filed) |
| 2.16 | LDM cluster/LBA grid | LDM LBAs are 512-byte logical blocks (kernel semantics; PRIVHEAD BE decode `P:1209-1229` ⇔ `rd64be` sites) | `cr_ldm.cpp:1060-1131` (dev_512 bounds, M-046), `probe_disk_lds` `:1278-1303` | **COVERED-BY-M-046 (FIXED)** |

### Area 3 — carve offset math

| # | Surface | og (cite) | core (cite) | Verdict |
|---|---|---|---|---|
| 3.1 | Signature-hit offset → absolute location | analyser gets batch-relative offset; absolute start sector = batchByteOffset/stride + batchBase, stride = vtable+0x28 = 512 default = SECTOR size (SCAN-ENGINE-DELTA:241-248, "Consequence 2" decode; +0x40 = record's absolute start sector, 0 on deep paths) | `cand_abs = window_base + cand_rel` (bytes), extent = `cand_abs + ext_base` (disk-absolute, GAP-S6-8) (`cr_carve.cpp:446`, `:594-596`) | **MATCH** (equivalent; core byte-granular) |
| 3.2 | Carve provenance fields (checksum/anchors/note/flag/format-id) | CRAWFileProperties: +0x56 checksum (0xFFFFFFFF invalid), +0x8C header anchor, +0x94 tail anchor, +0x5A note, +0xF4 flag, +0xF8 type-registry id (ifileproperties-part3 §3) | `carve_checksum` CRC32(first 512 B), `carve_anchor_b` tail, `carve_note`, `carve_flag`, `carve_format` (`cr_carve.cpp:584-589`, `:968-976`; persisted `cr_records.cpp:227-243`) | **COVERED-BY-M-051 / M-093 / S-049 / N-089 (FIXED)** ("Anchor A = the extent start (already carried)" — M-051) |
| 3.3 | Session persistence of carved location | .rsf record-4 CRAW carries a **DataRunList** (`[u32 len][NTFS-runlist bytes][u16]`, runlen 6..31,797 B in sample) — cluster-unit run encoding; **unit/rounding semantics undecoded** (OQ-2) | .crsf RESULT_TREE per-node `u32 extent_count + {u64 offset, u64 length}*` — byte offsets (`cr_records.cpp:213-225`; `docs/crsf-format.md` RESULT_TREE grammar) | **MATCH-in-function / OPEN on og units** (core byte-precise; if the og runlist is cluster-quantized, core is a strict superset in offset precision — OQ-2) |
| 3.4 | Carve free-space gating (cluster-granular) | RAW pass carves unallocated clusters via FS-engine volume bitmap (family doc §6.4; controller default cluster/sector config 0x100) | free_map gate (M-005), occupancy gate (M-039), zero-stride skip (N-031), byte↔cluster conversion `k=(pos−base)/cluster_bytes` (`cr_carve.cpp:238-260`) | **COVERED-BY-M-005 / M-039 / N-031** |
| 3.5 | Carve size formulas' numeric hygiene | MRW `BE32(u32@+4)+8+ceil(u8@0x20×BE16@0x1a×BE16@0x18/8)` — og has a wrapping signed multiply; core u64 (SCAN-ENGINE-DELTA:136-140, "registered numeric-hygiene divergence, the D-68 precedent") | `validate_mrw` u64 product | **REGISTERED** (SCAN-5 log; the D-68-precedent class) — LA-4 is a NEW member of this class (FAT cluster math), not covered by it |

### Area 4 — LDM span math (S-052 verification)

| # | Surface | og (cite) | core (cite) | Verdict |
|---|---|---|---|---|
| 4.1 | PRIVHEAD/VDBLK offsets | config start from PRIVHEAD@0x12B (`:59748`); TOCBLOCK candidates config+{2,0,1} (`FUN_180072ad0 :59573-59632`); PRIVHEAD BE-field swap (`P:1209-1229`) | `find_privhead` MBR LBA6×512 / GPT `e->last_lba×ls` (`cr_ldm.cpp:1000-1042`); `toc_offs[4]={1,2,2045,2046}` (`:1115`) | **COVERED-BY-M-046/N-025 (FIXED)**; core's extra TOCBLOCK candidates {2045,2046} = recovery-stance superset (INFO note, not sector-math defect) |
| 4.2 | Config-region gate / interleave default | config ≥0x801 sectors → reject (`:59748-59759` "ConfigSize larger than 1m ?."); interleave 0 → 0x10000 B = 0x80 sectors (`:47665`, `:68631-68641`) | `ph.config_size >= 0x801 → ERR_BAD_CHECKSUM` (`cr_ldm.cpp:1099-1100`); `chunk_sectors = chunk ?: 128` (`:918-922`, `:937`) | **COVERED-BY-N-025 (FIXED)** — both sub-items landed |
| 4.3 | Spanned member math | SpanVolumeStorage (LdmManager.mo family; span = vol_offset-sorted members) | `spanned_find` binary search by vol_offset, `translate_spanned` `member_lba = disk_lba + (s − vol_offset)` (`cr_ldm.cpp:1234-1263`) | **COVERED-BY-S-052 (FIXED)** — scan-side A27 M, recovery-side closed (findings/FIXED/S-052 + ledger :441 f4e500de) |
| 4.4 | Stripe / RAID-5 rotation | read path `FUN_180086a90 :68822-58`: `r=(off+S)/S−1; g=r/(N−1); c=off%S; col=((N−1)−(g%N)+1+(r%(N−1)))%N` (left-symmetric, hardwired layout 0 `:68924-62`); member offset `g*S+c` | `cr_ldm.cpp:1390-1409`: `ci=s/chunk; in=s%chunk; row=ci/(cols−1); parity=(cols−1)−(row%cols); col=(parity+1+i)%cols; member LBA row*chunk+in` — **algebraically identical** (evidence/decompiled-quotes/ldm-raid5-and-scoring.md:8-34) | **MATCH** (OQ-T7 RESOLVED) |
| 4.5 | RAID-5 reconstruct-on-failure | XOR across other members (`FUN_180085c30`) | `xor_into` reconstruction (`cr_ldm.cpp:1416-1433`); unresolvable second column → ERR (never zeroed) | **COVERED** (A27 M; recovery-side missing-member policy = **REGISTERED D-106**; overlap mapping families = **REGISTERED D-107**) |
| 4.6 | Member re-anchoring (per-disk logical_disk_start) | each member disk's own PRIVHEAD lds | `probe_disk_lds` (LBA6 + end-of-disk candidates, ×512 LDM grid) + `m.disk_lba = lds + part_start` (`cr_ldm.cpp:1278-1327`) | **COVERED-BY-S-052** (cross-disk resolver wiring, cr_ui.cpp:1881-1917) |
| 4.7 | LDM layout session persistence | (rsf record-3 raw partition structs — no LDM layout record decoded) | `REC_LDM_LAYOUTS`: kind/vol_obj_id/total_sectors/chunk_sectors/columns + per-copy per-member `disk_lba/part_start/sectors/vol_offset` u64 (`docs/crsf-format.md` LDM_LAYOUTS; `cr_records.cpp:292`) | **COVERED-BY-S-052** (core superset: the og's rsf has no equivalent decoded record — noted, not a defect) |

**Residual LDM sector-math check:** none found beyond the registered/covered items. The
512-byte LDM grid is applied consistently (probe_disk_lds `sec*512` `cr_ldm.cpp:1292`,
read paths `(…)*512` `:1346-1347`, `:1407-1409`); the GPT-metadata-partition path uses device
geometry (`e->last_lba * ls` `:1037`) per M-046. No divergence.

### Area 5 — sector-number SURFACES (UI / ABI / session)

| # | Surface | og (cite) | core (cite) | Verdict |
|---|---|---|---|---|
| 5.1 | Lost-partition insertion (start/total sectors) | "Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d" (`DRWUI:904076`, `:904198`, `:904404`, `:905106`, `:905938`; "Add lost partition: start_sector=%I64d, end_sector=%I64d" `:406499`) | `cr_ui_apply_candidate {u64 start_lba; u64 sector_count}` (`include/cr/cr_ui.h:757-763`); candidate LBA logging (`cr_scan.cpp:521,:527`) | **MATCH** (sectors, u64, both sides) |
| 5.2 | Scan ranges (quick/deep StartSector/TotalSector) | `DRWUI:567911` / `:567925` | stage telemetry + permille progress (`cr_ui.h:125-130`); single-walk quick/deep | **COVERED-BY-D-36 (REGISTERED)** |
| 5.3 | Volume read context (sector math in UI read path) | `StartSector + LCN×SPC + runIdx`, bounds-checked u64 (`DRWUI:875657-875689`) | slice + byte extents (`cr_slice.cpp`, engines) | **MATCH** (equivalent) |
| 5.4 | "Once partition" byte→sector conversion | parse offset %I64X, require `%sector_size==0`, `start = offset/sector_size` (`DRWUI:845942-845959`) | LBA-native candidates (no byte-parse analog needed) | **MATCH-in-effect** |
| 5.5 | Disk size in sectors / geometry | "TotalSectors[%I64u]" (`DRWUI:836459`); "device_sectors"/"sector_size" JSON keys (`:207414-207435`) | `cr_ui_geometry {total_bytes, logical_sector, physical_sector, sector_count u64}` (`cr_ui.h:431-435`) | **MATCH** (u64 sectors; sector-size parameterized per D-04 vs og 512-default in the lost path — SCAN-ENGINE-DELTA A3 row, registered) |
| 5.6 | Volume offsets | rsf record-3 partitionStruct +0x04 LBA/byte pair (rsf-format §5.3, semantics partially decoded); JSON "device_offset"/"partition_start"/"area_start" | `cr_ui_volume_info.offset_on_disk` **bytes** (`cr_ui.h:470-471`) | **MATCH-in-effect** (units differ between ABIs but are internally consistent; no cross-ABI contract exists) |
| 5.7 | Bad-sector reporting by sector number | "bad sector:%I64d, count:%d" (`DRWUI:179979`, `:180024`, `:180682`…) | SMART counters (`cr_ui.h:1387` reallocated/pending/uncorrectable) + `probe_unreadable` (`:690`); no per-sector bad-list ABI | **INFO** (surface-shape difference; the core's bad-sector skip policy is covered by FB-C01-class fixes, not a sector-math defect) |
| 5.8 | Scan progress by sector | og progress = scan of [StartSector, TotalSector) ranges | permille + bytes (`cr_ui.h:125-130`) | **COVERED-BY-D-36** |

---

## 2. DIVERGENCE CANDIDATES (new; not filed, not registered)

### LA-1 — GPT header validation: core accepts headers the og's analyzer rejects (no position/reserved/header-size/usable-range gates)
- **Proposed severity: MAJOR** (edge-case/scale correctness; direction: core laxer)
- **og:** `FUN_1800078d0` (P:4805-4856) requires header_size == 0x5C **exactly**, reserved == 0,
  current_lba == 1 (primary) / == sector_count−1 (backup), backup_lba == sector_count−1 (primary)
  / == 1 (backup), first_usable < last_usable < device sector count, and entries_lba outside
  [first_usable, last_usable]. Any violation → header untrusted → salvage/backup ladder.
- **core:** `load_gpt_side` (cr_table_parse.cpp:29-70) checks signature, entry_size==128,
  entry-count cap, header_size∈[92,512] and the two CRCs only.
- **Concrete example:** a GPT header written by a tool with header_size=96 and a matching CRC
  (CRC covers the 96 bytes): og → primary verify fails ("Verify GPT HEADER sector failed.",
  P:4851-4855) → salvage/backup path; core → primary trusted, entries emitted, `gpt_primary_ok=true`.
  Truncated-image case (device smaller than declared): a 4 TB GPT disk imaged to a 2 TB file —
  the og rejects the primary (last_usable ≥ sector_count, backup_lba ≠ sector_count−1);
  the core accepts it and reports a healthy primary.
- **PARITY-DELTAS:** not covered (no GPT-header-validation delta exists; D-04 is geometry-only).
- **Impact:** the flags are currently internal-only in the core (grep: no consumer outside
  cr_table_parse.cpp), but the acceptance decision also governs WHICH copy's entries are
  emitted when both are readable and disagree, and the truncated-image "healthy GPT" report.

### LA-2 — GPT backup-header probe position + dead-LBA1 asymmetry
- **Proposed severity: MAJOR** (classic damage class: primary header wiped/unreadable)
- **og:** the analyzer ALWAYS probes the backup at **device sector_count−1** (P:4567) — never at
  the primary's declared backup_lba. When the **LBA1 read itself fails** the analyzer returns
  early with no backup probe at all (P:4517-4524); only the separate detection layer
  `CGPTDiskAnalyzer::IsGPTDisk` (P:4166-4234) falls back to the last LBA (P:4208-4218) to
  *classify* the disk as GPT.
- **core:** primary trusted → probe at **prim.backup_lba** (cr_table_parse.cpp:323); primary
  dead OR **LBA1 unreadable** → probe at dev_last (:339-348) and emit its entries.
- **Concrete example:** bad sector at LBA1, intact backup at the last LBA: og analyzer yields
  NO GPT partitions (detection says GPT; analysis bails); core emits the partitions from the
  backup. Conversely a healthy primary with a nonstandard-but-valid backup_lba (backup really
  at that LBA): core sets gpt_backup_ok=true; og probes the device last sector, misses it.
- **PARITY-DELTAS:** not covered.

### LA-3 — GPT entry span underflow: `sector_count = last_lba − first_lba + 1` with no first≤last gate
- **Proposed severity: MINOR** (phantom/skipped volume; contained by slice bounds)
- **core:** cr_table_parse.cpp:274; the underflowed count passes the `sector_count != 0` gate
  (:297). first_lba=5, last_lba=0 → count = 0xFFFFFFFFFFFFFFFC.
- **og:** span consumption not decoded (OQ-1) — if the og computes the same expression without
  a check, this is MATCH (shared defect); if it validates, DIVERGENT.
- **Downstream (verified):** `PartitionSlice::create(slice, start_lba×ls, count×ls)`
  (cr_scan.cpp:1680) — the wrapped byte length either zeroes (rejected) or produces a slice
  whose reads all fail → "table_volumes_skipped" (cr_scan.cpp:1681-1683) / no identifiable
  filesystem (:1690-1698). No crash, no wrong bytes — but no explicit `first_lba <= last_lba`
  guard anywhere.
- **PARITY-DELTAS:** not covered.

### LA-4 — FAT cluster→sector 32-bit multiply wrap on >2 TiB FAT32 (og wraps, core u64-correct)
- **Proposed severity: MAJOR** (scale correctness; silent wrong-sector reads in the og)
- **og:** `(cluster−2) × spc` computed as a **32-bit** product at the content-read and geometry
  sites — FAT:11239, FAT:13194 (`(ulonglong)(((uint)uVar7 − 2) * (int)param_1[4]) + param_1[8]`),
  FAT:28172, FAT:39068-39080 (`FUN_180030d90`) — then zero-extended onto the 64-bit volume
  base. (Other `(c−2)×spc` sites at FAT:8483/8570/12618/12766/15278/24330/30048/30244/30776/
  31355/39469/39582/39915/41815 were located but their operand widths were NOT verified this
  session; the runlist-factory sites :10873/:28045 are 64-bit.)
- **core:** full u64 product (cr_chain.cpp:24-28).
- **Concrete arithmetic (512-byte sectors):** wrap begins when the data area exceeds 2^32
  sectors — exactly the 2.00 TiB mark for every cluster size:
  spc=16 → wraps from cluster 268,435,458; spc=32 → 134,217,730; spc=64 → 67,108,866;
  spc=128 → 33,554,434. At the FAT32 spec-max cluster 0x0FFFFFF4 with spc=32 (16 KiB clusters,
  a 4 TiB GPT volume mkfs.fat will build): og sector = 4,294,966,848 (0xFFFFFE40) vs correct
  8,589,934,144 — a 4,294,967,296-sector (2 TiB) displacement: the og reads/walks the wrong
  location or fails its own device bound; the core addresses it correctly.
- **Class:** the same numeric-hygiene family as D-56 (NTFS pass-2, registered) and the MRW
  carve formula (SCAN-ENGINE-DELTA:140, "D-68 precedent") — but the FAT cluster-math member is
  **not registered anywhere** (PARITY-CORE-FS1.md:194 records the formula as MATCH without
  flagging the width; the s3 evidence quote s3-calcpartition-scoring.md:49-55 shows the 32-bit
  product unremarked).
- **Direction:** core superset (correct where the og wraps). Needs registration or an og-defect
  notation, not a core fix.

### LA-5 — NTFS runlist malformed-input tolerance: og truncates-and-keeps, core rejects the fragment
- **Proposed severity: MINOR** (corrupt-input salvage posture; possibly MAJOR under the
  product's own salvage-over-purity doctrine)
- **og:** decoder `FUN_1800347d0` (NTFS:41853-41970): len_width > 8 → **break the walk and
  return success** (the runs decoded so far survive) (:41891); off_width > 8 → return success
  (:41992-41994); zero-length runs accepted (memcpy of 0 bytes, run appended); buffer
  exhaustion → return success (:41875, :41888). Separately, the pre-validator `FUN_18001d5f0`
  (NTFS:23175-23210) REJECTS a runlist whose first header byte has either nibble zero
  (:23184) — i.e. sparse-first $DATA is rejected at the five attribute-parse call sites
  (:23412, :23506, :23533, :24337, :24643).
- **core:** `RunList::decode` (cr_runlist.cpp:77-86) returns ERR_BAD_PARAMETER for
  len_width==0/>8, run_len==0, ERR_TRUNCATED at buffer end; a failed fragment contributes no
  runs (cr_mftwalk.cpp:561-567). No sparse-first rejection anywhere.
- **Concrete example:** a $DATA runlist torn mid-entry (`31 40 …` followed by garbage width
  0x9x): og keeps runs 1..k and recovers the prefix content; core keeps zero runs of that
  fragment → the file surfaces with no extents and fails at read time. Conversely a
  sparse-FIRST $DATA (fully-sparse file): og's validator rejects the attribute at those five
  sites; the core accepts and represents it.
- **PARITY-DELTAS:** not covered (B2's "runlist codec MATCH" verdict did not compare the
  malformed-input branches).

### LA-6 — EBR link type 0x85 (Linux extended) not recognized as a container by the core
- **Proposed severity: MAJOR** (a whole extended chain's logical volumes vanish on such disks)
- **og:** the chain walk links on {0x05, 0x0F, **0x85**} (P:991, P:1006 `-0x7b`) with
  `next = base + rel` (P:993).
- **core:** `mbr_type_to_kind` maps only 0x05/0x0F to Extended (cr_table_parse.cpp:118-120);
  0x85 → FsKind::Other (default, :144-146). In an EBR the link entry is then emitted as a
  VOLUME at `walk + rel` (cr_table_parse.cpp:203-211) — the wrong basis (links are
  base-relative, volumes EBR-relative) — and the chain stops; in the MBR the 0x85 container is
  emitted as a single opaque volume spanning the whole container.
- **Concrete example:** MBR extended base 2048; EBR#1 at 2048 with link (type 0x85, rel 8192)
  and volume (type 0x07, rel 63): og → next EBR at 2048+8192=10240, chain continues through
  all logical volumes; core → emits a bogus "Other" volume at 2048+8192=10240 with the link
  entry's sector_count, never reads EBR#2, and every later logical volume is invisible to the
  table pass (only findable via lost-partition content scans).
- **0x85 in the wild:** Linux fdisk/dual-boot disks historically stamp extended entries 0x85.
- **PARITY-DELTAS / findings:** not covered (grep for 0x85/"Linux extended" across
  diff-to-fix + PARITY-DELTAS: zero hits).

### LA-7 — GPT entry_size==128 requirement + 1 MiB entries cap: deliberate strictness documented in code but absent from the PARITY-DELTAS registry
- **Proposed severity: MINOR** (registry-discipline defect; the registry's own rule is
  "a delta that is not in this registry is a bug, not a delta")
- **core:** cr_table_parse.cpp:38 (`entry_size != sizeof(GptEntry) → ERR_BAD_PARAMETER`) and
  :15/:40 (1 MiB read cap ≈ 8192 entries); the in-code comment (:76-81) documents
  "Stricter than the og on one point… the og hardcodes the 0x80 stride, which mis-sizes the
  array for corrupt non-128 headers".
- **og:** allocation `entry_count × 0x80` with 64-bit saturation (P:4530-4541), read length
  = `entry_size × entry_count` as a 32-bit int product (P:4541), consumers stride 0x80
  (P:4703) — a non-128 entry_size produces an og mis-walk; the core refuses instead.
- **Also un-registered in the same block:** the core's salvage-only entries_start_lba bounds
  check (:100) which the og lacks.
- **Action:** register in PARITY-DELTAS (or fold into a table-module delta entry), not a code fix.

### LA-8 — FAT "File overflows partition!" run-end refusal absent in the core
- **Proposed severity: MINOR**
- **og:** both runlist factories refuse and DESTROY the runlist when
  `(run_len + (cluster−2)) × spc > volume sector limit` (FAT:10873-10876 + ReleaseTrace
  :10903; FAT:28045-28048 + :28070) — the file gets no data runs at scan time.
- **core:** extents are emitted unguarded (cr_fat_quick.cpp:370-386); an out-of-slice extent
  fails only at recovery-read time (Salvaged/unreadable). The chain walker's cluster bound
  (`max_cluster_ = cluster_count+1`, cr_chain.cpp:92) uses the BPB's cluster count, which
  validate_boot deliberately allows to exceed the device (truncated-tail volumes,
  cr_chain.cpp:613-616).
- **Concrete example:** BPB claims 1,000,000 clusters on a volume truncated to 600,000
  clusters' worth of bytes: og → files whose chains run past the limit are dropped at scan;
  core → they are listed and fail per-read at recovery (hash-verify flags them).
- **Direction:** core superset-with-deferred-failure. Not registered.

---

## 3. OPEN QUESTIONS (with the exact corpus artifact that would answer each)

1. **OQ-1 — og GPT entry→volume span math (first_lba/last_lba consumption; any first≤last
   validation?).** Artifact: `ghidra-mo/Partition_decompiled.c` — the CPartitionEntryGPT
   consumer chain: entries are copied raw at `FUN_1800085b0` (P:5273-5286) into the list built
   by `FUN_180008ff0` (P:5791+); the volume-materialization consumer (CFileSystemPartition /
   CRangeDevice, RTTI census in MOM-modules/Partition.md) reads the 128-B entry's +0x20/+0x28.
   Decides LA-3's og side (shared defect vs divergence).
2. **OQ-2 — og carved-file DataRunList units (cluster size + rounding of byte-granular carve
   hits).** Artifacts: DRWUI `IRawDirectoryTreeBuilder` vtable walk (RTTI map
   `re-workspace/reversed/ui-resources/drwwui_vtable_map.json:36948`; census
   `rtti_class_census.json:8169`) — the AddRawFile path that converts the analyser's offset
   into CRAWFileProperties' runlist; plus the shipped samples
   `og-codebase/SaveScan/auto_*.rsf` (decode the run bytes against sample I_'s known FAT32
   cluster size and the carve anchors +0x8C/+0x94). Decides whether the og quantizes carve
   locations to clusters where the core stores byte-exact extents (Area 3.3).
3. **OQ-3 — og EBR volume-entry emission basis (relative-to-EBR confirmation) and the
   entry-0-as-link shape.** Artifact: `ghidra-mo/Partition_decompiled.c` CMBR::Parse
   (`FUN_180009d40`, P:6330-6360) + the CExtendMBR class functions (P:6387-6500, vftable
   sites P:6057-6107) and their MOM wrappers (P:30233-30269). The link math is verified
   (P:983-1014); the volume-entry side is inferred from the standard only.
4. **OQ-4 — og GPT dead-LBA1 full call chain: does any caller re-route a backup-detected disk
   into an analyzer that reads the backup as primary?** Artifacts: the two callers of the
   analyzer `FUN_180006e20` (P:5896 `FUN_180009150`, P:32675) and the CDiskDeviceType /
   CGPTDiskAnalyzer dispatch above `IsGPTDisk` (P:4166-4234). Decides the strength of LA-2's
   dead-LBA1 leg.
5. **OQ-5 — widths of the remaining og FAT `(c−2)×spc` sites** (FAT:8483, 8570, 12618, 12766,
   15278, 24330, 30048, 30244, 30776, 31355, 39469, 39582, 39915, 41815). Artifact: the
   decompile itself (per-site operand declarations). Only quantifies LA-4's blast radius; the
   four verified 32-bit sites already establish the divergence.

---

## 4. Summary counts

- **Surfaces examined:** 45 (Area 1: 15, Area 2: 16, Area 3: 5, Area 4: 7, Area 5: 8) —
  every row carries a first-hand cite from BOTH sides unless marked OQ.
- **New divergence candidates:** 8 (LA-1…LA-8) — none covered by PARITY-DELTAS or the active
  findings list (checked: S-040…S-050, M-073…M-117, N-090…N-152, I-026…I-034, and the
  PARITY-DELTAS registry incl. D-04/D-14/D-36/D-56/D-59/D-68/M-020-family/D-106/D-107).
- **Covered / registered surfaces:** 22 (D-04, D-14/F7, D-36, D-56, D-59, D-79/83, D-106,
  D-107, M-002, M-005, M-009-class, M-019/I-032, M-020, M-039, M-040-class, M-043, M-046,
  M-051, M-054, M-093, N-002, N-010a/b, N-011b/M-001, N-025, N-031, N-036, S-049, S-052,
  plus the MRW numeric-hygiene precedent).
- **S-052 residual check (Area 4):** no residual sector-math divergence found — spanned/
  stripe/RAID-5 math is og-identical (algebra verified against the evidence-quote decode),
  the 512-LBA grid and 4Kn re-basing are consistent, and the recovery-side policies are
  registered (D-106/D-107).
