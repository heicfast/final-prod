# S11/L2 — ZFS/ReFS function-level walk (ZfsSearchDLL + ReFSSearchDLL)

**Task ID:** S11/L2 (ZFSREFS-FN) · **Date:** 2026-10-28 · **Lane:** read-only auditor
**Core @** `2498b9fbc7f274d7336166794a48dc59e22cfcd4` (2026-10-28 17:51) ·
**Corpus @** `be0cfe95ecf22ac67e0f3a823dd003163b196d38` (2026-10-28 09:39)
**Inputs:** `re-workspace/reversed/ghidra-searchdll/{ZfsSearchDLL,ReFSSearchDLL}_{functions,imports,_decompiled}.{txt,c}`
+ corpus artifacts (`SearchDLL/zfs-engine--deep-dive.md`, `refs-engine--deep-dive.md`,
`refs-btrfs-zfs-engine-algorithms-v2.md`, `remaining-engines--ext4-hfs-refs-btrfs-zfs.md`,
`zfs-vdev--mftmirr--calcpartition.md`, `per-engine-function-map.md`) + the full trail
(modules/zfs.md, modules/refs.md, evidence/decompiled-quotes/*, findings/).
All `:NNNN` cites = line numbers in the named `_decompiled.c`; `core:` cites = file:line at HEAD.

---

## §1 Census

### ZfsSearchDLL (45,364 decompile lines; 981 functions in map)

| Bucket | Count | Notes |
|---|---|---|
| Functions in map total | 981 | = 580 `FUN_*` + 401 named |
| — named exports (domain) | **2** | `EUCreateZFSSearcher` @0x180011660 (L15407), `EUCreateZFSQuickSearcher` @0x180011750 (L15453) |
| — CRT/STL-named noise | 399 | `_scrt_*`, `Unwind@/Catch_All@`, `std` streambuf/locale/facet thunks, `_alloca_probe`… |
| — unnamed `FUN_*` | 580 | **560 in domain range 0x180001080–0x18002cd00** (173,607 bytes; ≥500B: 118, 100–499B: 222, 30–99B: 131, <30B: 95) + 20 in CRT range |
| — all functions below CRT boundary | 566 | (560 FUN_ + 2 EU + 4 low CRT thunks like `_guard_check_icall`) |
| Functions with a decompiled body | 499 | 419 carry engine/domain behavior (≈21.2K lines); 80 are std-container instantiations (≈8.1K lines); ~81 map entries are referenced-only thunks (no body exported) |
| Domain-function split (disposition, §2) | 61 engine / ~180 domain-container plumbing (std::map/vector/string for domain types) / ~60 family-shared base (CFileNode, CFileSystemNode, props, IDataRunList) / ~40 ostream+`zfs_debug.txt` logging / ~50 stubs-dtors-accessors / ~128 small vtable/registry helpers | |
| Imports (119) | | `Sleep`, `MultiByteToWideChar`+`WideCharToMultiByte`, full MSVCP iostream/**fstream** family (`_Fiopen`, `fread/fwrite/fclose`) — the `zfs_debug.txt` logger (`:8252 std::_Fiopen("zfs_debug.txt",…)`) |

### ReFSSearchDLL (20,270 decompile lines; 629 functions in map)

| Bucket | Count | Notes |
|---|---|---|
| Functions in map total | 629 | = 402 `FUN_*` + 227 named |
| — named exports (domain) | **2** | `EUCreateReFSQuickSearcher` @0x180001100 (L54), `EUCreateReFSSearcher` @0x180001180 (L83) |
| — CRT/STL-named noise | 225 | (incl. 160+ `Unwind@/Catch_All@` EH thunks) |
| — unnamed `FUN_*` | 402 | **354 in domain range 0x180001000–0x180015780** (79,389 bytes; ≥500B: 53, 100–499B: 126, 30–99B: 93, <30B: 85) + 48 in CRT range |
| — all functions below CRT boundary | 357 | (354 FUN_ + 2 EU + 1 `_guard_check_icall`) |
| Functions with a decompiled body | 379 | ≈11.4K domain lines + ≈2.7K std-container lines |
| Imports (49) | | `GetVersion` (host-OS gate I-013), `WideCharToMultiByte` (names UTF-16→UTF-8; **no** MB2WC — ReFS names are never byte-lifted in) |

Census method: mechanical (awk) function-boundary split of both `_decompiled.c` (header-line →
closing `}` at col 0), cross-checked against the Ghidra function maps; every domain function
was then digested (signature + constants ≥0x100 + string literals + call targets) and the
substantive ones read in full (§2/§5).

---

## §2 Per-function disposition

### 2.1 ZfsSearchDLL — engine functions (read first-hand this session unless marked)

| Function (addr:line) | Role | Disposition vs core |
|---|---|---|
| `EUCreateZFSSearcher` (11660:L15407) | complete-controller factory (0xE8-byte obj; 34 vtable slots) | COVERED-BY-TRAIL (s30 OQ-Z5) |
| `EUCreateZFSQuickSearcher` (11750:L15453) | quick factory (0x120 obj, 16 slots) | COVERED-BY-TRAIL (s30) |
| FUN_180014360 (14360:L17898, 424 ln) | **block reader**: lsize/psize/cksum decode `:18002/07/12` (`(prop&0xffff)+1)*0x200`, embedded `(prop&0x1ffffff)+1`, `prop>>0x10&0xffff`), DVA vdev `:18027 ((w0>>0x20)&0xffffff)`, **fixed +4 MiB translate** `:17996 (w1+0x2000)*0x200`, vdev-registry lookup `:18025`, stream read `:18036-42`, **fletcher4 {7,0} gate** `:18076`, 32-B memcmp `:18117`, **next-DVA retry** `:18128-29 (idx<2→idx+1)`, drop `:18139-41`, BPE else-branch `:18179→FUN_180014bf0` | MATCH (M-019 landing verified first-hand; og fixed 4 MiB vs core nvlist `offset` = INFO-9) |
| FUN_1800168b0 (168b0:L19471) | **vdev registry search**: per-entry id via FUN_180017f40; match→entry; **no match→FIRST entry `:19544-56` — never fails** | MATCH — **OQ-T6 confirmed** (§4.1) |
| FUN_180017f40 (17f40:L20795) | registry entry id getter (RTTI type-compare vs 0x18003eb48) | MATCH (supports 168b0) |
| FUN_180014bf0 (14bf0:L18326) | BPE embedded payload assembly (words minus #6/#10) | MATCH (bpe_payload core:197-207) |
| FUN_180014200 (14200:L17838) | LZ4: comp==2 passthrough; {0xF,0} LZ4 with BE-u32 length `:17870-76` | COVERED-BY-TRAIL (session-2; core codec registry = registered superset) |
| FUN_180012850 (12850:L16353) | std::map<string,·> machinery (a940 range checks) | NOISE (container plumbing) |
| FUN_180016c50 (16c50:L19686, 395 ln) | **tree open** per uberblock slot: `root_dataset` lookups `:20007-09` (both strings init `"root"`, DAT_1800305d8) | MATCH (S-034 torn-state skip; core walk_state Failed) |
| FUN_180003150 (03150:L1967) | **ZfsDecider::addBlock(txg)**: registry add `:2002→FUN_180017310`; ring slot match by txg `:2070-71 (slot+0x10==txg, 0x400 stride)`; tree open `:2076` | MATCH (S-034/DFX37) |
| FUN_180017310 (17310:L20085) | per-txg UberData registry insert | COVERED-BY-TRAIL (S-034) |
| FUN_1800182c0 (182c0:L20999) | nvlist walk: **"vdev_tree" type-9** `:20999`, parse limit **0x1bffc** `:21000` | MATCH (session-2; core nv_parse superset incl. name/XDR) |
| FUN_180018730 (18730:L21225) | nvlist pair parser (0x40-byte entries) | COVERED-BY-TRAIL (zfs-vdev artifact §1) |
| FUN_180018930 (18930:L21323) | uberblock-ring slot reader (0x400 stride; `_Size<0x401` bound) | MATCH (fixed 1-KiB geometry; core shift-probe 10..13 = registered superset I-010) |
| FUN_1800243d0 (243d0:L32248) | backward uberblock walk `:32321 plVar8-0x80 / ==0xbab10c` | COVERED-BY-TRAIL (algorithms-v2 §3) |
| FUN_1800270d0 (270d0:L35207, 870 ln) | vdev-label/uberblock scan + txg compares `:35244` | COVERED-BY-TRAIL (deep-dive §2) |
| FUN_180029aa0 (29aa0:L37984) | staged-ub txg ordering `==0xbab10c && txg<` (newest-wins) | MATCH (core ring sort :1349-58) |
| FUN_180028320 (28320:L36570) | **txg max-heap** over 0x400-byte ub slots (child pref `:36592-93`, sift-up `:36666-702`); 0x800 = 2×slot only | MATCH — no 4Kn handling anywhere (fixed 1-KiB slots; core shift-probe stays superset) |
| FUN_180010f90 (10f90:L14996) | **complete-search hit probe**: 0x400-stride ub magic + **ZEC_MAGIC 0x0210da7ab10c7a11 @slot+0x1d8** `:15049/15070` (LE reads via FUN_180010ef0 reverse-accumulate) | COVERED-BY-TRAIL (s30 slot [19]) — first-hand confirmed |
| FUN_180011260 (11260:L15163) | per-hit emission: fs-code 13 record `:15223 (0xd)`, sector math `:15224-31` (blocksize default 0x200) | COVERED-BY-TRAIL (s30) |
| FUN_180006b90 (06b90:L5312, 607 ln) | quick run loop: root node **SetName("root")** `:5587`; **unbounded 1000-ms device poll** `:5671-73`; per-txg "\n\ntxg = " log `:5690+` | MATCH-with-note (S-034 ring walk verified; poll refinement → §3 C8) |
| FUN_180003660 (03660:L2331, 677 ln) | ZPL node emit: attribute bit clear `:2333 (+0x58&0xffef = dir bit)`, ZPL-flags bit0 clear `:2334 (+0x5a&0xfffe)`, "from_txg/birth_txg/id/parent_id" logs | MATCH (S-034 emit chain; flag-clear detail → §3 C7-note) |
| FUN_180005940 (05940:L4208, 454 ln) | file-visit emission: **global monotonic file id** `:4534-36 (ctx+0xf0)`, objid registry `:4571→FUN_180007db0`, tree add `:4580`, **trash hook** `:4600→FUN_180006110`, emit(tag 9) `:4601-04` gated on trash-handler return | MATCH (S-034); trash hook → §3 C4 |
| FUN_180004570 (04570:L3038, 307 ln) | directory tree-add: parent chain `:3166-82 (+0x1250/58)`, **"root" name match `:3148-61 (DAT_1800305d8)`→root bucket ctx+0xe8**, dir attr `:3250 (+0x58|=0x10)`, ZPL-flags bit0 clear `:3251`, emit(tag 9) `:3263`, bucket+0x20=node `:3266` | MATCH (registry/emit parity; "root" naming → §3 C5) |
| FUN_180006110 (06110:L4668, 551 ln) | **.trashinfo sidecar parser**: `/files` gate `:4748→FUN_180004ad0`, `/info` gate `:4758→FUN_180004f00`; **materializes sidecar content through ZfsDataRunlist** `:4831-39 (cast, runlist+0x58)`; "Path=" scan `:4843`, line end `:4876`, **last-'/'+parent-dir extraction** `:4929-63`; ".trashinfo" suffix strip `:5002`; map insert {stripped-name→orig-parent} `:5020-23` (bucket+0x48); $R-name map (bucket+0x38) `:5109-58` | **NEW** — §3 C4 (no in-DLL consumer found) |
| FUN_180004f00 (04f00:L3610) / FUN_180004ad0 (04ad0:L3349) | `.Trash-1000/info` / `.Trash-1000/files` subtree classifiers (L".Trash-1000/info" :3708 / L".Trash-1000/files" :3447) | MATCH (S-034 landing; binary-verified UTF-16 strings) |
| FUN_180001a90 (01a90:L607) | cross-state name compare: **skips/normalizes '\'(0x5c) and '/'(0x2f) separators** `:626-52` | MATCH (S-034; separator-normalization detail verified) |
| FUN_18001b6b0 (1b6b0:L23980, 299 ln) | ZPL object/SA decode (calls ZPL_PARENT `1bbb0`, SA registry `1a1f0`) | COVERED-BY-TRAIL (M-038/S-034) |
| FUN_18001a1f0 (1a1f0:L22816, 517 ln) | SA registry chain: "SA_ATTRS"/"LAYOUTS"/"REGISTRY" `:22816+` | COVERED-BY-TRAIL (M-038) |
| FUN_18001bbb0/1bda0/1bfc0/1c1e0/1c400/1dd70 (L24283/24398/24520/24642/24764/26408) | **ZPL_PARENT / ZPL_ATIME / ZPL_MTIME / ZPL_CTIME / ZPL_CRTIME / ZPL_SIZE** by-name resolution | MATCH (M-038; B9 — §4.2) |
| FUN_180010ef0 (10ef0:L14926) | BE/reverse-accumulate int reader, **≤8 bytes only** (`p2-p1<9` gate `:14937`) | MATCH (B9: seconds-only; core `le64` 8-B read) |
| FUN_18001c630 (1c630:L24895) | names → `MultiByteToWideChar(0xFDE9,0,…)` `:24895` | MATCH (S-005 FIXED) |
| FUN_1800165f0 (165f0:L19344) | ZAP dispatch: ZBT_MICRO `0x8000000000000003` → micro-zap (13590/13830); ZBT_HEADER `0x8000000000000001` → fat path | MATCH (core read_zap :936; micro/fat split) |
| FUN_180016330 (16330:L19232) | fat-ZAP continuation: **+0x430 skip; throws std::range_error when <0x430** `:19282-85` | MATCH-behavior / style note §3 C9 |
| FUN_180013590/13830/13280/13410 (L17095/17263/16928/17006) | micro-ZAP entry parse/convert (salt 0xffff check 13280:16928) | MATCH (core read_microzap :753) |
| FUN_180017690 (17690:L20313, 476 ln) | DSL hierarchy walk: **bonus caps 0x100/0x140** (`:20313+`) = 256/320 | MATCH (core `blen<256||blen>MAX_BONUS(320)` :1432) |
| FUN_180016a20 (16a20:L19577) | 4-KiB block copy via stream (0x1000 memcpy `:19617-19`) | MATCH (read path) |
| FUN_18001afb0 (1afb0:L23550) | node path getter (+0x1258 chain) | MATCH (walk ctx) |
| FUN_180012740 (12740:L16280) | registry state init (+0x202..0x205) | NOISE (state plumbing) |
| FUN_180013e10 (13e10:L17563, 256 ln) | SA layout map (attr nums `&0x3ff`, slot `>>7`) | COVERED-BY-TRAIL (M-038 layouts) |
| FUN_1800150f0/155f0/14ec0/15970/159a0/15b20/15c20/15d00/161e0/16330/165f0/18220 | ZfsDataRunlist/extent-plan cluster (recursive block reads via 14360; 512-B dnode reads 0b6d0 `:9479 (uVar1&0x1ff)||(uVar1==0)`, `_Dst+0xb=0x200`) | MATCH (A27/X51 verdicts stand; core plan_file_extents :1080) |
| FUN_18002b280/2b070/2aa40/2a3b0 (L39377/39284/39028/38661) | family CFileSystemNode/CFileNode ctors+merge (+0x1d4=0xE, u16@+0x288=0x2a00, +0x72/74/91 flag merges) | NOISE (family-shared base; same code in ReFS DLL :9250/2272) |
| FS-name builder (L40005, the `2c050` cluster) | family volume-name switch **cases 0–10 only: FAT12/16/32, NTFS(31660), EXT2/3, RAW, EXFAT, HFSPLUS, EXT4, ReFS(31680)** — **NO case 13 (ZFS)** | **NEW** §3 C6 / §4.5 |
| FUN_18000a1f0 (0a1f0:L8240) | `std::_Fiopen("zfs_debug.txt")` lazy logger open | INFO (diagnostic only; no core counterpart needed) |
| FUN_1800111c0/115e0 (L15112/15374) | lambda-deleter log (" deleter running") | NOISE (logging) |
| Remaining ~330 domain-range functions | std::map/vector/string instantiations for domain types, ostream/locale plumbing, dtors, vtable stubs, thunk chains | NOISE (counted, digested, no FS behavior) |

### 2.2 ReFSSearchDLL — engine functions

Backbone = the s3-refs-engine-inventory (≈75% read claim re-verified at its key cites). This
session's first-hand reads add/verify:

| Function (addr:line) | Role | Disposition |
|---|---|---|
| FUN_1800010d0 (L37-49) | host-OS gate `GetVersion(); major<6||minor<2 → refuse` | COVERED-BY-TRAIL (I-013) |
| FUN_1800019c0 (L515) | quick-impl ctor; handlers **{0x500, 0x600, 0x701}** `:593-616` | COVERED-BY-TRAIL (RS-7) |
| FUN_180002130 (L832) | quick open: fs code 10 `:847`, MSB default **0x4000** `:861`, sector default 0x200 | MATCH (N-026) |
| FUN_1800021e0 (L873) | checkpoint A/B read + **newer sequence wins** `:1029-31` | MATCH (core :523) |
| FUN_180002770 (L1099) | root-tree walk; 0x600 → volume node `:1151`; >0x700 → 0x701 `:1153-55` | MATCH (RS-7) |
| FUN_180002a60 (L1255) | system-dir/container map (span==+0x148 gate `:1314`, map@+0x188) | MATCH (N-036 landed half) |
| FUN_1800032f0 (L1386) | record dispatch (0x10 / 0x30+subtype; deleted-flag propagate :1463-66) | MATCH (M-022) |
| FUN_1800041f0 (L2044) | **volume/root node emission**: 0x290 obj, name "REFS" (DAT_1800192d0) `:2101`, attrs|=0x10 `:2102`, FsKind **10** `:2139`, **label copy ≤64 wchars, cap 400** `:2141-63` from ctx+0x1b8, tag 0x600 `:2169` | MATCH + **label branch DEAD** (§4.5) |
| FUN_1800046c0 (L2272) | quick file-node emit: **prefilter hook ctx+0x1e8 vtbl+0xa0** `:2315`, emit code 6 `:2319-20` | INFO (host filter hook; the record-level filter is in ba90 — §3 C2) |
| FUN_180004980 (L2410) | full detect (MSB=cluster×0x1e `:2460`, gen tag 3/7/8/9 `:2466-77`) | MATCH (M-022) |
| FUN_180004ec0 (L2634) | already-scanned dedup (global DAT_180022650 seen-list) | INFO (host dedup; no core counterpart needed) |
| FUN_180005330 (L2819) | stale-MSB sweep, force-deleted (record+0x59:=1 `:2939`), MSB+ 0x2b42534d `:2904` | MATCH (M-026) |
| FUN_180008620 (L5802) | page reader: "ReFS"@+3/0x53466552, FSRS/0x53525346@0x10, SUPB/CHKP DWORDs | MATCH (N-007) |
| FUN_18000a040 (L6995) | **family FS-name table** case 0..10 = FAT12/FAT16/FAT32/**NTFS/EXT2/EXT3/RAW/EXFAT/HFSPLUS/EXT4/ReFS** (binary .rdata dump verified) — volume node name+attrs | **NEW** §4.5 |
| FUN_18000a2c0 (L7059) | BFS aggregation: **grade byte node+0x70&3, ==2 deleted** `:7118/:7137-49`, attr counters 0x800/0x40 `:7150-55`, **bulk re-grade when param_2[10]≠0** `:7157-59` | **NEW detail** §3 C3 (B4) |
| FUN_18000b210/b820/b840/b860 (L7815/8096/8111/8126) | record grammar (key/type/offsets; LCN-quad indirect recognizer) | MATCH (M-022/S-020) |
| FUN_18000ba90 (L8233) | file-entry processor: **zero-size ∧ no-'.' prefilter** `:8317-37` (size=props[0]=FileValues@+0x30 via c060 `:8617`); $I path `:8374-421`; used-space marking for live `:8451-73`; **deleted → props bit 32** `:8476 (local_c0|=0x100000000)`; e020+d970 emit `:8478-81` | MATCH + **prefilter NEW** §3 C2 |
| FUN_18000c060 (L8535) | shape gate + FileValues fill `:8597-8631` (**props+0x00=FileValues@+0x30=size; +0x08/0x10/0x18=@0/+8/+0x18; u32@+0x28=@+0x20 flags; node id@+0x1d0; global file id DAT_180021038 `:8620-22`) | MATCH (sizes/flags map to core gi) |
| FUN_18000c2b0 (L8641) | **name builder**: only if key type 0x30 and **nameLen=(keySize−4)/2 > 0** `:8674-75`; else NO-OP (no fallback name); UTF-16→CP_UTF8 | **NEW** §3 C1 (B5) |
| FUN_18000c560/c790/c920/cbd0/cf30/d470/d770 (L8761/8854/8939/9106/9294/9510/9640) | attribute walk, run grammar (0x40/0x20/0x18), dir values, 0x10 timestamps (4×16 B `:9172-85`), indirect chase | MATCH (S-020/M-022) |
| FUN_18000d970 (L9727) | emit + identity keep-one + recycle routing ($R/S-/SVI, DAT_1800199a8 table binary-verified `{".","$R","S-","","","","System Volume Information"}`) | MATCH (M-027) |
| FUN_18000e020 (L10084) | runlist build + granularity bands `{0x200..0x1000}` `:10223-81` | MATCH (S-020) |
| FUN_18000e490/ea30 (L10304/10615) | resident/$I materialization | MATCH (S-020) |
| FUN_18000ef20 (L10861) | flush (0x600 special-name tagging `:10914-33`; recycle flush) | MATCH (M-027) |
| FUN_18000fc80/ff20/ff40 (L11659/11789/11801) | page sigs first-DWORD; SUPB 0x42505553 / CHKP 0x504b4843 | MATCH (N-007) |
| FUN_180011b50/11e90 (L13093/13223) | page read + container `((block/cluster)>>1)−2` translate | MATCH (N-036 half) |
| FUN_180012390 (L13511) | DBR gates (bps ladder, cluster {4K,64K}) | MATCH (N-026) |
| FUN_180012650 (L13663) | checkpoint body gates (0x58/0x44 by minor≥0xE) | MATCH (GAP-S6-3) |
| FUN_180010390/10630/10820/10a20/10c00/10da0 (L12016…) | lost-partition probe family (fs code 10 `:12125`) | COVERED-BY-TRAIL (S-006 side) |
| ~230 remaining domain functions | std containers, EH thunks, dtors, node/base-class plumbing (009250/0046c0 family ctors), map helpers | NOISE (counted, digested) |

---

## §3 New divergence candidates (not in any prior finding/module row)

### C1 (MINOR, recovery-negative) — ReFS unnamed $DATA-only rows: og emits, core drops (refs.md B5 resolved)
- **og:** a 0x30/subtype-1 record with `keySize ≤ 4` gets **no name** — FUN_18000c2b0 is a
  no-op (`:8674-75 if (0 < iVar4)` guards the whole builder) — but the record is **still
  processed and emitted** with an empty name (FUN_18000ba90 → FUN_18000d970; no name gate
  anywhere on that path). No synthetic/fallback name is generated.
- **core:** `cr_refs_engine.cpp:1101-1103` `const std::u16string name16 = name_from_key(...);
  if (name16.empty() || ev.val_len < 0x28) return;` — **the row is dropped entirely**.
- **Falsifiable gap:** a fixture with a 0x30 row whose key is `{0x00010030}` only (no name
  bytes) but a valid value (≥0x28) + data runs: og → one nameless recoverable file; core →
  nothing. Deleted files whose name row was reused/lost are exactly this shape.
- **Severity:** MINOR (small population, but deleted-name-stripped content is the recovery-
  interesting case). **Lead re-verify:** confirm `FUN_18000ba90` has no later name gate
  (search for reads of props+0x42 before d970; my read of :8478-81 shows none).

### C2 (MINOR, core-superset / parity-bench noise) — ReFS zero-size + extensionless row filter (M-022 OQ-3 decoded)
- **og:** FUN_18000ba90 `:8317-37` — when the file's size (`props+0x00` = FileValues u64@+0x30,
  written by c060 `:8617`) is **< 1**, the name is scanned for '.' (memchr loop `:8323-25`);
  a record with **no '.' in the name is skipped** (falls out of the processor). Files with
  size ≥ 1, or any '.' in the name, proceed. This is the "dot-extension prefilter" the
  M-022 fix plan left as OQ-3 — it is a **record-level anti-noise gate in the shared
  processor**, not a quick-only host hook (the separate host hook is 46c0 vtbl+0xa0).
- **core:** no equivalent — `emit_file` emits every parsed row.
- **Falsifiable gap:** fixture with a size-0 file named "README" (no dot): og → absent;
  core → present (empty file row). Parity benches must not count these.
- **Severity:** MINOR. **Lead re-verify:** the props[0]<1 field identity (size) — check
  FUN_18000c060:8614-17 mapping against a real-volume FileValues dump (same residual class
  as OQ-2).

### C3 (INFO) — ReFS grade byte + host bulk re-grade (refs.md B4 refined)
- **og:** grade = **node+0x70 & 3** (2 = deleted), set from record+0x59 via
  `:8476 local_c0 |= 0x100000000` (props+0x3d bit 0) and converted at emit; the counting
  pass FUN_18000a2c0 buckets grade==2 `:7137-49` and, when the caller passes
  `param_2[10] != 0`, **rewrites every node's grade low bits to `param_2[0xb]`** `:7157-59`.
  No Excellent/Good/Medium/Poor tiers exist in the DLL.
- **core:** internal DeletedState; no host re-grade hook. Core's medium-allocator grading
  stays a core-only superset (already registered). The 2-bit grade + override hook is an
  interface fact, not a recovery delta. INFO.

### C4 (INFO→UNVERIFIED consumer) — ZFS .trashinfo content parse builds never-consumed maps
- **og:** FUN_180006110 materializes sidecar **content** through ZfsDataRunlist
  (`:4831-39` — the og reads file bytes engine-side), parses "Path=" `:4843`, extracts the
  original parent dir `:4929-63`, strips ".trashinfo" `:5002`, and inserts
  {stripped-name→orig-parent-dir} into bucket map@+0x48 `:5020-23`; $R names go into
  bucket map@+0x38 `:5109-58`. **No in-DLL reader of either map was found** (all
  FUN_180007f50 bucket consumers read +0x20 only) — the restore/rename step is either
  host-side or unfinished in this build.
- **core:** suppresses info sidecars (S-034) and never reads their content — **same user-
  visible emission** (both surface $R files under .Trash-1000/files, unsuppressed).
- **Severity:** INFO (no observable emission delta in-DLL); the UNVERIFIED residual: a
  consumer in DRWUI. **Lead re-verify:** grep DRWUI decompile for the bucket map layout or
  "Path=" before deciding whether the og relocates trashed files at the UI layer.

### C5 (INFO) — ZFS pool-root display name: og "root" vs core decoded pool name / "pool"
- **og:** the pool root node is named the literal **"root"** — SetName(&DAT_1800305d8)
  `:5587` (FUN_180006b90), "root" init in FUN_180016c50 `:20007/:20009`, and the root-bucket
  keying on the name "root" in FUN_180004570 `:3148-61` (DAT_1800305d8 = `root\0`,
  binary-verified). No pool-name decode (s31 stands).
- **core:** `cr_zfs_engine.cpp:1464-1465/1473` — pool_display_ = decoded nvlist `name` or
  `"pool"` fallback.
- Registered s31 superset unchanged; the **fallback constant differs** ("pool" vs "root") —
  INFO-level naming note only.

### C6 (INFO, confirms registered superset) — engine-side FS-name table: no ZFS case
- The family volume-name builder exists in both DLLs (ZfsSearchDLL `:40005-40059`,
  ReFSSearchDLL FUN_18000a040 `:6995-7051`): switch on FsKindCode **0..10** →
  FAT12/FAT16/FAT32/NTFS/EXT2/EXT3/RAW/EXFAT/HFSPLUS/EXT4/ReFS (binary .rdata dumps
  verified: ReFS DLL 0x197e8-0x19810, ZFS DLL 0x30a48-0x30a98). **Neither has a case 13
  (ZFS)** and ZfsSearchDLL contains no "ZFS" name string. ZFS volumes therefore get no
  engine-side FS name (DRWUI "Unknow" — s31) and ReFS volumes get the constant "ReFS".
- Core: pool-name decode = registered display superset; no action.

### C7 (INFO) — ZFS props flag bookkeeping on re-emit
- og clears bit 0 of the u16 at props+0x5A (ZPL_FLAGS) and re-sets the directory bit at
  +0x58 on directory re-emit (FUN_180004570 `:3250-51`, FUN_180003660 `:2333-34`). The core
  carries `zfs_secondary_flags` (N-059) without the clear. No observable divergence found
  (the og's clear only affects its own dedup bookkeeping); recorded so N-059's census note
  gains the exact sites.

### C8 (INFO, refines N-056/N-072) — og device-ready poll is fixed 1 s, unbounded
- `:5671-73 while (poll vtbl+0x10 == 0) Sleep(1000);` (FUN_180006b90; same pattern at
  `:4268` in FUN_180005940). The module row said "retries with backoff" — actually a
  **fixed 1000-ms interval, no bound**. Core's bounded 3×(250/500/1000) live-handle retry
  (cr_zfs_engine.cpp:1330-42) remains the deliberate safer subset. No action.

### C9 (INFO) — ZFS fat-ZAP guard throws
- FUN_180016330 `:19282-85`: header < 0x430 → `std::__gsl_range_check` + `swi(3)` (a C++
  throw); the core returns false and skips the object. Error-path style only.

### C10 (INFO) — ZFS DVA translate: og fixed +4 MiB vs core nvlist-driven
- og: `:17996 (w1 + 0x2000) * 0x200` — every DVA read is offset+4 MiB, unconditionally.
- core: `vdev_offset` from the label nvlist leaf vdev's `offset` key
  (cr_zfs_engine.cpp:1250-63 pick into `tr` = the leaf's key map; :1365). Identical on
  standard whole-disk pools (leaf offset = 0x400000); the core is the spec-general form.
  Verified decode supporting M-019's registered superset posture.

---

## §4 Open-question resolutions

### 4.1 OQ-T6 (multi-vdev) — RESOLVED-CONFIRMED first-hand (matches the M-019 landing)
```
ZfsSearchDLL_decompiled.c
:18025-28  FUN_1800168b0(local_600,&local_340,
             (uint)((ulonglong)*(undefined8 *)(param_3 + lVar17*0x10) >> 0x20) & 0xffffff);
           // ^ vdev id = DVA word0 >> 32 & 0xFFFFFF (SPA_VDEVBITS)
:19502     if (param_3 == lVar6) { … break; }      // registry entry id match
:19544-56  if (puVar7 == *(undefined8 **)(param_1 + 0x20)) {   // NO match — loop exhausted
             puVar7 = *(undefined8 **)(param_1 + 0x18);         // ← FIRST entry (registry head)
           }
           *param_2 = *puVar7; param_2[1] = puVar7[1];          // never fails
```
**The og DOES handle multi-vdev pools** by never refusing a DVA: foreign vdev ids resolve to
the registry head (the scanned disk). Mirror members read correct bytes; stripe members
yield their local half (checksum-valid only for self-contained blocks); raidz parity blocks
fail the fletcher4 gate and drop. Full chain verified: checksum types {7,0} `:18076`,
interleaved fletcher4 `:18093-18111`, 32-byte memcmp vs blk_cksum `:18117`, retry next DVA
`:18128-29`, drop `:18139-41`, BPE else `:18179`. Core parity = M-019's landed
`read_raw`/`read_block` (cr_zfs_engine.cpp:525-592) incl. `foreign_vdev_reads`/
`checksum_drops` counters. No new divergence.

### 4.2 zfs.md B9 (timestamp precision) — MATCH stands, now with the full site map
- og decode family: ZPL_ATIME `:24398`, ZPL_MTIME `:24520`, ZPL_CTIME `:24642`,
  ZPL_CRTIME `:24764` (by-name registry resolution, M-038), value read via FUN_180010ef0
  which is **hard-capped at 8 bytes** (`:14937 (longlong)param_2 - (longlong)param_1 < 9`).
- Both sides read seconds only; **no nsec persistence found anywhere** (props carry the four
  filetimes only; the .rsf/SaveScan layer has no ZFS nsec field). Core: `le64` first-u64
  reads (cr_zfs_engine.cpp:1574-86) + `unix_to_filetime` (:1709-12). MATCH (the module's
  "not graded / revisit with OQ-T6" note can close as MATCH).

### 4.3 refs.md B4 (deleted-grading boundaries) — DECODED (see C3)
2-bit grade byte at node+0x70 (2=deleted), set from record+0x59 → props+0x3d bit0
(`:8476`), consumed by FUN_18000a2c0 (`:7137-49`), host bulk-override `:7157-59`. No tier
system in the DLL → the core's finer grading remains the registered core-only superset; no
og tier boundaries exist to mirror.

### 4.4 refs.md B5 (unnamed $DATA-only naming fallback) — DECODED (see C1)
The og has **no naming fallback at all** (empty name emitted as-is); the divergence is the
core's drop. Filed C1 MINOR.

### 4.5 Engine-side volume-label decode (S10-F completion set) — RESOLVED for both engines
- **ReFS:** FUN_1800041f0 `:2140-67` copies a label from ctx+0x1b8 (≤64 wchars, byte-capped
  400) onto the 0x600 volume node — but **no writer of that string exists in the DLL**
  (exactly 3 references: two dtor frees `:821/:18574` + this read) → the branch is dead;
  the volume node name is the constant **"REFS"** (DAT_1800192d0) / **"ReFS"**
  (family table case 10, DAT_180019808). **No on-disk label decode.**
- **ZFS:** no case 13 in the family FS-name builder (`:40011` switch) and no "ZFS" string —
  no engine-side name (DRWUI "Unknow", s31).
- Consequence for the S10-F/M-079/N-133 completion set: **no ReFS/ZFS label decode is owed**
  — the og itself surfaces none. The core's ZFS pool-name display stays the registered
  superset; nothing to mirror for ReFS.

### 4.6 M-022 OQ-3 (dot-extension prefilter) — DECODED (see C2)
Record-level gate in the shared processor (size==0 ∧ no '.' → skip), not a quick-only hook.

---

## §5 Coverage limits (honest accounting)

1. **Method:** every function in both decompiles was mechanically split and digested
   (signature, constants ≥0x100, string literals, call targets); the substantive engine
   functions (§2 tables — 61 ZFS + ~35 ReFS first-hand this session, on top of the trail's
   prior decodes) were read in full. The ~500 remaining domain-range functions per DLL are
   std-container instantiations for domain types, family-shared base classes, EH thunks and
   ostream plumbing — digested but not line-read; their dispositions rest on the digest
   (constants/calls) plus spot reads. Residual risk: a behavior hiding in a mid-size
   container-typed function with no distinctive constant — considered low (every function
   with an FS-relevant constant or engine call target IS in §2).
2. **Not decoded to verdict:** the og's ZFS fzap +0x430 constant's exact semantics (fun-
   internal layout; core uses the kernel layout — no divergence signal); FUN_180011260's
   ZEC-at-+0x1d8 geometry rationale (s30 already registered the core's shift-probe
   superset); the DRWUI-side consumer of the trash maps (C4 residual).
3. **Corpus gaps carried forward (unchanged from the trail):** no real ReFS volume image
   exists (RS-1/RS-4 fixtures); row-flag-0x4 reality, container-map values, run-flag
   polarity remain fixture-gated residuals (refs.md session-3 list).
4. **Out of scope this lane:** ZfsDataRunlist's .rsf serialization (SaveScan artifacts),
   the .mo analysers, DRWUI consumers (except the two targeted checks in C4/C6).
5. ZFS `FUN_180018930`'s `_Size < 0x401` ring bound and `FUN_180013280`'s 0xffff salt
   check were digested but not deep-read; neither contradicts the verified ring/zap
   geometry (fixed 1-KiB slots, salt-checked micro-ZAP).

---

## LEAD VERIFICATION LOG (S11, first-hand — appended on filing)

- **OQ-T6 resolution re-verified**: FUN_1800168b0's registry-exhaustion fallback
  re-read at :19544-19556 — when the vdev-id search pointer hits `param_1+0x20`
  (end), the walker falls back to `*(param_1+0x18)` (the FIRST registry entry),
  copies it out under LOCK — **no-match → vdev 0's extent, never fails**. The
  og DOES resolve multi-vdev DVAs (mirror OK; stripe reads the local half;
  raidz beyond checksum repair drops). Convergent with the M-019 landing's
  behavior; zfs.md's OQ-T6/B8 notes close as MATCH (module doc updated).
- **C1 (the ReFS unnamed-row drop)**: core cite re-pinned at `46b81573` —
  `cr_refs_engine.cpp:1102` (`if (name16.empty() || ev.val_len < 0x28) return;`).
  og cites re-read: the name builder FUN_18000c2b0 :8674-75 no-ops on
  keySize≤4 (empty name) and the emit path :8478-81 has NO name gate. **Filed
  as S-053 (SEVERE)** — the class is silent loss of recoverable content
  (name-destroyed $DATA-only records), the S-034 precedent.
- **C2 (the zero-size∧no-dot filter)**: og FUN_18000ba90 :8317-8337 re-read
  (the `local_e8 < 1` size gate + the memchr(0x2e) dot-scan). **Filed as
  N-141 (MINOR)** — a junk-suppression gate core lacks (core emits the rows
  the og filters; the reverse direction of S-053).
- **B4/B5/B9/volume-label closures** filed as I-032 + the refs.md/zfs.md row
  updates. The trashinfo content-parse (C4) rides I-032 as its one
  UNVERIFIED-consumer residual.
