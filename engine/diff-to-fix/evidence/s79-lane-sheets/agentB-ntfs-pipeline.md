# Agent B — NTFSSearchDLL function-level pipeline census (parity ground truth)

Scope: exhaustive function-level map of og `NTFSSearchDLL.dll` (decompiled 62,394 lines,
1,931 functions; cites = `NTFSSearchDLL FUN_XXXX :lineno` in
`re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`), cross-read against
`core_recovery/src/core/ntfs/*` + `include/cr/ntfs/*` (cites = `file:line`).
Reading method: full sequential read of lines 1–7,950, 10,344–10,553, 17,843–17,962,
19,422–20,021, 20,700–21,379, 22,856–23,415, 34,079–34,153, 35,400–35,890, 41,171–42,231,
45,725–45,954, 50,963–51,739, 51,900–52,084, 51,900–52,739, 52,079–53,358, 52,739–53,900,
53,355–53,410, 53,397–53,900, 54,006–54,860, 54,081–54,800, 54,795–54,860, 55,187–55,241,
plus Grep sweeps for every API site and magic constant; the remainder (CRT/std template
instantiations, list/vector plumbing at 7,950–10,344 / 12,700–17,800 / 23,400–34,000 /
36,000–41,000 / 42,300–45,700 / 46,000–51,000 / 55,300–62,394) verified as boilerplate via
the functions.txt address map. Known-covered items (M-141/M-145/M-146/M-147/N-196/N-197
BPB family, S-063 mirror bootstrap, N-192 hard-link fan-out, fixup-mechanics findings)
are NOT re-filed.

---

## 1. PIPELINE MAP (ordered stages)

### A. Exports & host contract

| Stage | og function | Lines | Notes |
|---|---|---|---|
| Export 1 | `EUCreateNTFSSearcher` | :33–47 | `operator new(0x940)` (2,368 B) → ctor `FUN_180007ba0` |
| Export 2 | `EUCreateNTFSQuickSearcher` | :52–61 | factory `FUN_1800014a0` :339–396; `new(0x4e0)` (1,248 B) |
| Complete ctor | `FUN_180007ba0` | :5914–6089 | vtables: `[0]=CNTFSSearchController`, `[0x37]=IBuildVolumeBitmap`, `[0x38]=CNTFSBuildVolumeBitmap` (:5992–5995) — MI: the controller IS the bitmap builder. `InitializeCriticalSection(+0x7a8)` :6015; default `*(u32)(+0xf3)=8` :6011, `*(u16)(+0x83)=0x100` :6000 |
| Quick ctor | `FUN_1800014a0` | :339–396 | `IQuickFileSystemSearchController` :354 → `CNTFSQuickSearch` :360–361; `*(u8)(+0x2e1)=1` :392 |
| Quick config gate | `GetPrivateProfileIntW(L"search",L"fastsearch",1,ini)` | :7163 | **defaults ON**; result → `*(u32)(ctrl+0x124)` :7164 |
| Complete dtor | `FUN_1800083c0` | :6273–6567 | Frees node lists/bitmaps/critical section (`DeleteCriticalSection` :6551); base dtor chain `FUN_1800079d0` :5799–5813 |

Only two exports (verified vs imports/exports). Host drives via the interface vtables
(`vt[0x18]`=device size, `vt[0x28]`=bytes/sector, `vt[8]`=read(buf,off,len,flags),
`vt[0x10]`=cancel check on the cancel object). Core counterpart: none as a DLL surface —
core is a static library with `WalkerFlags` (cr_mftwalk.h:39–111); **no quick/complete
split and no ini gate** (coverage note, not a bug).

### B. Mount/bootstrap

```
FUN_1800020f0 :937–1074  (quick-scan open)
  *param_4[0] = total_sectors*bps …  :972–980
  if (dbr.cpr==0) cpr=0x400          :976  (record-size default 1024)
  if (dbr.spc==0) spc=8              :1026–1029 (spc default 8)
  if FS==0xe || cluster_bytes==0 || spc==0:      :985–987
      read sector0 → FUN_18003f8d0 (BPB validate) :992
      lVar10-- (total sectors -1)    :994–999
  MFT load: FUN_18001a3c0(mft_lcn…)  :1046
  MFTMirr load: FUN_18001a3c0(mirr_lcn…) :1065 (mirror read failure = non-fatal, walk continues)
```
- BPB validation family (KNOWN-covered M-141/M-145/M-146/M-147/N-196/N-197 — the
  `_strnicmp(boot+3,"NTFS    ",8)` + bps-mask + spc pow2 band + zero-sweeps at
  `FUN_18003f480 :50986–51033` / `FUN_18003f6d0 :51087–51122`) — not re-filed.
- `FUN_18001a3c0 :20718–20762` — **$MFT bootstrap**: 64 KB stack buffer
  (`aiStack_10038[16388]` :20731, memset 0x10000 :20735), read via `dev.vt[8]`,
  fixup `FUN_180041bb0(buf, bps*cpr)` :20739, magic `FUN_1800411d0` :20740
  (= "record's $FILE_NAME matches L"$MFT"" via `FUN_180040d70 :51895–51940`),
  **$ATTRIBUTE_LIST probe** `FUN_180041850(buf,0x20,0)` :20741:
  - absent → `FUN_180041cf0` wrap + `FUN_180041c50` validate + `FUN_180007550`
    (CMFTInfo install: MFT record-size/spc out, parent ref, $DATA runlist
    `FUN_180044bd0`, attr-list chase `FUN_1800425d0` in `FUN_1800431d0` :53302–53332).
  - present → `LogInfo("MFT file record has 0x20 attribute, MFT start sector is %I64d")`
    :20755 → fragmented-MFT handler `FUN_18001a540 :20769–20870`
    (builds extent map from the attr-list records `FUN_180044ab0`/`FUN_180042a80`,
    iterates entries, per-child read+fixup+`FUN_180044ab0` merge loop until fixpoint
    :20824–20865).
- **MFT size extension by probing** `FUN_180007670 :5617–5687`: reads
  `sectorSize<<12` (2 MB) past each MFT run end in 0x1000-sector steps, counts
  fixup-valid 0x400-strided records, and **grows the run length by the probed clusters**
  (`*(run+0x10) += (validBytes/spc)/bps` :5671–5673). Salvage for truncated $MFT $DATA.
- $MFTMirr cross-reference sites: `FUN_180003ed0 :3012–3014` validates a read window's
  record 0 is $MFT-named and record at +0x100 is $MFTMirr-named;
  `FUN_1800097d0 :7249–7258` (complete-scan partition probe) loads the mirror record via
  `FUN_1800411a0` ($MFTMirr name check) + `FUN_180007460` CMFTInfo.
- Core counterpart: `cr_bootstrap.cpp:19–121` (validate), `:123–315` (load_mft_stream with
  resident-$DATA MFT, nonresident attrlist 1 MiB cap, fragment merge by lowest_vcn),
  `:385–517` (open with backup-DBR fallback M-043 + S-063 mirror bootstrap),
  `:519–572` (read_record + mirror capacity N-001).

### C. The MFT walk

- **Quick scan MFT enumerator** `FUN_180016750 :17843–17962+`: buffer =
  `bytes_per_sector << 0xf` (16 MB), MFT runs read in **0x8000-sector chunks**
  (`uVar3 = min(remaining, 0x8000)` :17937–17940), a per-sector offset map is built
  (:17952–17958) for progress; per-chunk record processing.
- **Dump/second-source walk** `FUN_18001a980 :20957–21090`: buffer 16 MB
  (`sectorSize << 0xf` :20982), per-run chunked reads ≤ 0x8000 sectors
  (:21018–21026) calling **`FUN_18001acc0 :21095–21302`** per chunk.
- `FUN_18001acc0` per-record gates (the deleted-record batch loop of
  algorithms-v2 §1 — verified line-by-line):
```
uVar8 = ctx[0x20].vt[0x28]()  … if <1 → 0x200          :21134–21137  (stride unit)
uVar16 = max(0x400, uVar8)                                :21139–21142  (parse bound)
for (off = 0; off < uVar8*param_3; off += uVar8):
    cancel: ctx[0x38].vt[0x10]() == 0 → break            :21148–21150
    rec = buf+off
    *(u32*)rec == 0x454c4946 ('FILE')                    :21152
    FUN_180041bb0(rec, uVar16) != 0  (fixup)             :21153
    FUN_180034b20(rec, uVar16) == 0  (metafile gate:     :21154
        FUN_180040ea0 = og-15 name table FUN_180040f20
        :51993–52014 AND $SI flags &2&4 FUN_180040d70)
    FUN_180041970(rec) != 0 → skip   (BASE-RECORD != 0 — extension records) :21155–21157
    recnum = off/uVar8 + param_4                          :21159
    FUN_1800431d0(parse ctx, copy+fixup+attrlist map)     :21161
    FUN_180043030(ctx) (ctx->rec != NULL)                 :21163
    FUN_180006db0(recnum, ctx, spc*recsize, &parent, 0x200) :21165
    filters: (flags&4) || name==NULL || name[0]=='$' → release+skip :21186
    if ((flags & 0x10) == 0)   /* NOT a directory */      :21190
        if (0x3ff < size)                                 :21208
            bitmap = ctx[0x40] ?: ctx[0x48]               :21209–21212
            per run (FUN_1800340b0/FUN_1800341c0 iterator):
                if (run.lcn != -1):                       :21220
                    total++                                :21221
                    sector = spc * run.lcn                 :21222   (ONE probe per RUN)
                    if FUN_18002c2e0(bitmap,sector) || bitmap[0xc].vt[0x20](sector): intact++ :21223–21228
            if (total>0 && intact==total) → LAB_18001b06f destroy :21249
    ADD (LAB_18001afe6):                                   :21265
        vt[0xe](props, props->+0x3d | 1)  (lost-marker)   :21269–21275
        append props to ctx[0x10] list                     :21277–21288
```
  Stride semantics (per the core's re-verified annotation, cr_mftwalk.cpp:1399–1416):
  `uVar8` is the **record size** (ctx+0x20 vt[0x28], 0x200 fallback), `uVar16` the parse
  bound; on hole/emit paths the stride is `uVar8`, on props-failure/skip/destroy paths
  `uVar16` (identical when record size ≥ 0x400). Note: the artifact v2 §1 claim
  "FUN_180041970 = in-use" is **wrong** — FUN_180041970 is the base-record-ref check
  (:52411–52415); the in-use bit is `FUN_180043060 → FUN_1800415a0` (flags&1,
  :52244–52248/:53240–53258) consumed inside FUN_180006db0 (:5374–5380, sets
  props+0x3d deleted). The walk itself passes BOTH live and deleted records.
- **Record classification**: deleted = !(flags&1) → props+0x3d (u8) + bit 8 at +0x72
  (:5374–5380); directory = $FILE_NAME flags & 0x10000000 → props flags |= 0x10
  (FUN_180043700 :53481–53488; FUN_180006db0 :5327–5331); orphan = not found in the
  by-record tree → lost list (FUN_1800099f0 :7286–7597 second pass, entries whose claim
  bit is 0 are appended to the lost vector :6787–6836 in FUN_180008c40).
- **In-use ($Bitmap) consultation**: the og never consults the $Bitmap FILE for
  classification; the claim/volume map at ctrl+0x40/+0x48 is **built by walking**
  (CNTFSBuildVolumeBitmap job: `FUN_18000a040 :7602–7781` constructs the walk context —
  spc default 8 :7651–7654, +0xa0 volume-in-clusters :7661–7668, +0x90+4=0x100 — and runs
  `FUN_180003ed0` index walks; `FUN_180008c40 :6677–6857` builds a 2-bits-per-node claim
  map under the +0x7a8 critical section, marking node i when
  `(node+0x9c != 0x20 || (ctrl+0x918==0 && node+0x68 > 99))` :6736–6738, then sweeps
  unclaimed nodes into the lost list :6787–6836).
- Hard-link fan-out: KNOWN (N-192 landed) — og emits ONE node per record
  (FUN_180006db0 runs once; FUN_1800448b0 :54124–54162 fetches at most TWO $FILE_NAME
  entries (index 0/1) and picks one by namespace rules: second==DOS(1) → first;
  both POSIX(0) → longer name; first==Win32&DOS(2) && second==POSIX → second).
- Core counterpart: `cr_mftwalk.cpp:415–1706` (walk_mft), stride = record_size, one
  record per iteration via `read_record`, fixup mandatory (:509–513), base-record skip
  (:506), attrlist chase depth-1/64 (:546–650), phase B parent resolution
  (:1145–1199).

### D. Attribute decode

- **Attribute walk** (in-record): `FUN_180041370 :52170–52201` (validation) and
  `FUN_180041850/180041730/1800414c0` (find nth of type) — offsets from
  `rec+0x14` (attrs offset, FUN_1800416d0 :52307), **cap 0x3f7** (:52182 etc.),
  end = type 0xFFFFFFFF or attr length 0. Per-attr header validation
  `FUN_1800454a0 :54664–54699`: len ≥ 0x10, len ≤ remaining; resident:
  contentOffset+len ≤ totalLen (`FUN_1800451b0` :54500–54504); nonresident type 0x80:
  runlist nibble validation (`FUN_180045130` :54472–54495).
- **$FILE_NAME (0x30)**: layout relative to attr header — parent ref u64 @+0x18
  (48-bit mask `FUN_1800438b0` :53547; parent seq >>0x30 `FUN_180043660` :53463),
  create @+0x20, modify @+0x28, access @+0x38, alloc size @+0x40 & 48-bit
  (`FUN_180043610` :53454), real size @+0x48 & 48-bit (`FUN_1800436b0` :53472),
  flags u32 @+0x50 (`FUN_180043730`), **is-directory = flags & 0x10000000**
  (`FUN_180043700`), name len u8 @+0x58, name @+0x5A, namespace u8 (clamped to 4,
  `FUN_180043330` :53338–53350). **Name validity** `FUN_1800433a0 :53355–53381`:
  rejects `" * / : < > ? \ |` and NUL — an invalid name fails the whole RECORD's
  fixup-validation (`FUN_1800434f0` → `FUN_180041370` return 0 :52190–52192).
- **$STANDARD_INFORMATION (0x10)**: SI validation `FUN_180044fb0 :54409–54413`
  (`contentOffset + 0x28 ≤ totalLen`); create @+0x18, modify @+0x20, access @+0x30,
  **flags u32 @+0x38** (`FUN_180044f60` :54400). In FUN_180006db0, SI OVERRIDES the
  $FILE_NAME times and merges dos-attribute bits: &1→RO, &2→hidden, &4→system,
  &0x20→archive, &0x400→offline (:5343–5372).
- **$DATA (0x80) walk** `FUN_1800069e0 :5028–5215`: enumerates ALL 0x80 attrs by index
  (`FUN_1800430b0`), per attr: compressed flag 0x800 set when `FUN_1800453b0` (attr
  flags&1 … actually `FUN_180045380` = flags&0x4000 → 0x40; `FUN_1800453b0` = flags&1)
  (:5159–5168); resident → CResidentNTFSDataRunList w/ content+length
  (`FUN_180045400/350/320` :5170–5198); nonresident → `FUN_180044e60` runlist decode;
  **compression-unit** = `FUN_1800067e0 :4911–4963` (pow2 map 1→1,2→2,3-4→4,…,0x801-0x1000→0x1000,
  >0x1000→0) applied to `ceil(realSize / totalRunClusters / sectorSize)`
  (`FUN_18002e2b0 :37240–37262`, called from `FUN_180044da0 :54332–54359`);
  size = max($DATA real size u64@+0x30 `FUN_180045200`, existing) :5201–5209;
  `(flags&0x840)==0x840 → (flags&~0x800)|0x40` fix-up :5153–5155.
- **Runlist decode** `FUN_18003fa20 :51235–51269` → list nodes {lcn(-1 sparse), prevLen
  (VCN), len}; header nibble validation `FUN_18003ff50 :51351–51376` (both nibbles ≤ 4);
  sum-of-lengths `FUN_18003fe40 :51312–51336`; VCN→LCN `FUN_180040570 :51556–51591`.
  Iterator (chunked deque): `FUN_1800340b0` :41481–41510 (begin), `FUN_180034150`
  :41515–41534 (next), `FUN_1800341c0` :41539–41544 (end).
- **MFT $DATA validation** `FUN_180044bd0 :54269–54327`: all 0x80 attrs unnamed
  nonresident; **sum-of-run-lengths == highestVCN−lowestVCN+1** (:54308–54312) — the
  runlist must fully cover the VCN span, else the MFT stream is rejected.
- **$ATTRIBUTE_LIST following**: detector `FUN_1800419e0 :52430–52437`; in-record find
  `FUN_180042020 :52692–52713`; **cross-record chase** `FUN_180041df0 :52615–52687`
  (iterates child-record list, loads each via `FUN_1800427a0 :52961–52994` (read+fixup+
  copy), counts attrs of type per record `FUN_1800414c0`, finds the nth). **Depth-1**
  (no recursion) — M-008 landed in core as `single_hop_attrlist`.
- **ADS enumeration**: the 0x80 loop above enumerates named streams too; named-stream
  skip filter is `FUN_180045440` (name length != 0) in the MFT validator; the og's
  $UsnJrnl:$J ADS finder matches name "$J" (0x24,0x4a) explicitly
  (`FUN_1800190d0 :19873–19888`, ≤ 10 attrs).
- Core counterpart: `cr_attributes.cpp:12–77` (enum_attributes, MAX_ATTRS=64),
  `:131–161` (parse_file_name), `:163–181` (SI), `:183–191` (parse_data_runs);
  `cr_runlist.cpp:57–151` (decode, low-nibble=length, N-163 torn-keep),
  `:189–247` (vcn_to_lcn/extents).

### E. Deleted-file viability heuristics (algorithms-v2 numeric thresholds)

1. record parse bound `max(0x400, recordSize)` — FUN_18001acc0 :21139–21142.
2. destroy gate `0x3ff < size` (STRICT >) — :21208.
3. probe = **run START cluster only**, `sector = spc × lcn` — :21222; sparse runs
   (`lcn == -1`) excluded from both counters — :21220.
4. destroy iff `total > 0 && intact == total` — :21249 (partially-overwritten kept).
5. folders bypass (`(flags & 0x10) == 0` gate) — :21190; live files own clusters.
6. bitmap test `FUN_18002c2e0 :35643–35657`: `pos = bm[10] + idx; pos > bm[2] || bm[0]==0
   → 0; else bm[0][pos>>3] & (1<<(pos&7))`; set-range twin `FUN_18002c1e0 :35580–35602`;
   secondary check `bm[0xc]->vt[0x20](sector)` :21225–21227.
7. deleted size rescue: size<0 → recompute `spc × Σrunclusters × 0x200`
   (`FUN_180007270 :5442–5483`).
8. size-0 rescue chain (mirror resident $DATA, same-name merge ≤ 5000 tree entries)
   — FUN_18001d690 :23215–23386 (cap `5000 < tree+0x68` :23270), compressed (0x800)
   candidates excluded (:23239).
9. name filters: `(flags&4) || !name || name[0]=='$'` :21186; metafile gate = og-15
   names + $SI hidden+system (FUN_180040ea0 :51960–51988).
10. Core counterpart: `cr_mftwalk.cpp:815–882` (grading + S-001/S-012 destroy flag,
    default off = og pass-1), `:777–813` (size-0 rescue), `cr_bitmap.cpp:86–106`
    (reused_ratio).

### F. Index/directory walk

- `FUN_180003ed0 :2943–3150` — directory index walk (complete scan): reads
  `spc-sector` blocks at (run base + i), validates via `FUN_1800411d0` ($MFT name) at
  buffer start and `FUN_1800411a0` ($MFTMirr name) at +0x100 (:3012–3014) — the
  mirror-pair window — else `FUN_180004c00(param_5=3/0)` finds the INDX/attr record
  (`FUN_180004fc0` :3686+), `FUN_180007460` builds CMFTInfo, children appended to a
  list (+0x158/:0x168), then `FUN_180005bc0 :4216` recursion.
- `FUN_180004c00 :3499–3590` — find-nth-attr in an INDX buffer: record search dispatch
  `FUN_1800059f0 :4121–4176` (type 3=$MFTMirr, 0=$LogFile, 1=$Bitmap, 2=$Volume;
  0x400-strided), `FUN_1800431d0` parse, `FUN_180006db0` props.
- INDX validation `FUN_180045510 :54704–54745` ("INDX" magic; usa_count < 0x11;
  `alloc(+0x20) ≥ actual(+0x1C)`; `(alloc + entriesOff(+0x18))/0x200 == usaCount−1`;
  USA offset ∈ [0x28, 0x100]); INDX USN check `FUN_1800457f0 :54823–54842` and fixup
  apply `FUN_180045720 :54795–54818` (**all** sectors, unlike FILE's 2-tail check).
- **No visited set / depth cap observed** in `FUN_180003ed0`'s recursion — bounds come
  from the runlist lengths and the 0x400-stride entry walks; the entry walk cap is
  0x1f5 (per core's S-022 decode of FUN_18000b4c0/FUN_18000e780).
- Core counterpart: `cr_mftwalk.cpp:1893–2032` (indx_block_validate / harvest_indx_block
  / harvest_index_root — remnant-mode only) + `:2147–2148` directory-records harvest.
  The main walk builds the tree purely from $FILE_NAME parent refs (:1145–1199).

### G. Result emission

- Props construction `FUN_180006db0 :5220–5437`: **metafile name filters**
  `$Tops` (:5295–5298), `$TxfLog` (:5300–5304), `$RmMetadata`/`$Repair`
  (:5306–5324) → NULL props; name → **UTF-8 via WideCharToMultiByte(0xFDE9)**
  fallback CP 0 default-char '_' (:5333–5336); times create/+0x25, modify/+0x2d,
  access/+0x35; size +0x1b (48-bit from $FILE_NAME real size, overridden by $DATA);
  flags +0x23 (dir 0x10, SI dos bits); deleted +0x3d; record# +0x58 from
  `FUN_180042ff0` (rec+0x2C u32); seq +0xd; runlist {u16 kind, char* hex} +0x48
  (`FUN_180003250 :2207–2268` — kind 1/0xE = resident); prop size 0x73 bytes
  (FUN_1800033b0 :2304–2333).
- Serializer `FUN_180003440 :2338–2407` — field map `{+0x19 u16 len, +0x11 str,
  +0x1b u64, +0x23 u16, +0x25/+0x2d/+0x35 u64, +0x3d u8, +0x40 u64, +0x64/+0x60 u32,
  runlist (hex nibble-length walk :2377–2383), runlist-map list}`; deserializer
  `FUN_180003690 :2412–2504`.
- Lost marker: `vt[0xe](props, props->+0x3d | 1)` :21269–21275; lost-list insert
  `FUN_18001b150 :21307–21320`; list length cap `0x7fffffffffffffe` (:21281).
- Stat counters: total/intact runs (:21215–21216), list size ctrl+0x18, node count
  ctrl+0x148; FS-code priority weights (`FUN_18000dc70 :10534–10553`: bit0→0x1e,
  bit1→10, bit2→0x1e, bit3→10, bit4→10, bit6→10).
- Progress ticks: per 0x8000-sector chunk (FUN_180016750 :17937–17958 builds a
  per-sector map; the quick scan's progress = sectors-read/total via the
  FUN_1800020f0 param_4[0]/[1] fields); cancel checks per record (:21148) and per
  32 KB chunk (:20986/:21014).
- Error paths: fixup failure → record skipped, walk continues (salvage-over-purity);
  read failure ends the current run's dump (chunk loop `break` :21023); allocation
  failure → `RaiseException(0xe0000001)` after GlobalMemoryStatusEx logging
  (`FUN_18002be40 :35422–35447`).
- Core counterpart: emission `cr_mftwalk.cpp:884–1143` (props, extents, ADS,
  LostDir<rec> M-103), stats WalkStats (cr_mftwalk.h:119–151), progress every
  0x40 records (:484).

### H. Teardown

- `FUN_1800083c0 :6273–6567` — controller dtor: frees MFT stream helpers (+0x1d..+0x1f),
  result lists, orphan lists, bitmap temp file (`FUN_18002c250 :35607–35638`:
  UnmapViewOfFile/CloseHandle/**DeleteFileW** of the mapped bitmap), critical section
  (:6551), returns to base `FUN_1800079d0`.
- Quick dtor: `FUN_1800017c0 :482–552` (node list teardown, vector of 0x38-byte nodes).
- Core: RAII (unique_ptr/vectors); no temp-file lifecycle (no file-backed bitmap).

---

## 2. CONSTANTS TABLE

| Constant | Value | og cite | core cite | status |
|---|---|---|---|---|
| Controller object size | 0x940 (2,368 B) | FUN_180007ba0 caller :39 | n/a | shape |
| Quick object size | 0x4e0 (1,248 B) | FUN_1800014a0 :346 | n/a | shape |
| FILE magic | 0x454C4946 | :21152, :55194 | cr_ntfs_wire.h:57 | parity |
| Record parse bound | max(0x400, recSize) | :21139–21142 | cr_fixup.cpp:24 (0x400) | parity (via boot clamp) |
| Record stride fallback | 0x200 when vt[0x28]<1 | :21136 | none (boot clamp ≥0x400) | unreachable in core |
| Fixup validate tails | 2 (first two) | FUN_1800411f0 :52129 | cr_fixup.cpp:25 | parity (landed) |
| Fixup extra gates | alloc%0x400==0, ≤0x1000, usaCount≤0x10, USA off ∈[0x2A,0x100] | FUN_180046160 :55195–55212 | cr_fixup.cpp:34–42 (weaker) | **D-7** |
| Attr offset cap | 0x3f7 | :52182, :52338 | none (64-attr cap cr_attributes.cpp:23) | **D-8** |
| Viability size gate | `0x3ff < size` strict | :21208 | cr_mftwalk.h:80 (0x3FF) | parity (NTFS-1) |
| Viability probe | run START cluster ×spc | :21222 | cr_bitmap.cpp:99–102 per-cluster | **D-1** |
| Destroy rule | total>0 && intact==total | :21249 | cr_mftwalk.cpp:839–847, 1567 | parity (S-001/S-012) |
| Chunk size | 0x8000 sectors (16 MB@512) | :21018, :17938 | cr_mftwalk.cpp:1432 | parity (dump pass) |
| Dump-walk buffer | sectorSize<<0xf (16 MB) | :20982, :17894 | 4 MiB REMNANT_BLOCK cr_mftwalk.cpp:231 | shape |
| $LogFile load cap | 0x8000001 (128 MB+1) | :23033 | none | **D-4** |
| USN chunk | 0xa00000/sector (10 MB), buf 0xa00064 | :19938–19941 | USN_HARVEST_CAP 64 MB :25 | **D-5** |
| USN match tree cap | 5000 | :23270 | cr_mftwalk.cpp:1682 (5000) | parity |
| Bitmap in-mem cap | 0x5f400000 (1.6 GB) | FUN_18002c330 :35676, :35556 | none (always in-mem) | coverage |
| Bitmap temp-file IOCTL | 0x900c4 (EPMNtDrv) | :35528 | none | coverage |
| spc default | 8 | :1026–1029, :7651–7654 | none (spc=0 rejected) | hardening delta |
| record-size default | 0x400 | :976, FUN_18003f630 :51062 | cr_ntfs_wire.h:242 | parity (M-147) |
| INDX usa band | offset∈[0x28,0x100], count<0x11 | FUN_180045510 :54724–54731 | cr_mftwalk.cpp:1893–1914 | parity |
| INDX entry walk cap | 0x1f5 entries | (S-022 decode FUN_18000b4c0) | cr_mftwalk.cpp:1951, :1973 | parity |
| $INDEX_ROOT in-use cap | 0x320 | (S-022 decode FUN_18000f760) | cr_mftwalk.cpp:2009 | parity |
| Index entry min size | 0x54 | (FUN_18000e970) | cr_mftwalk.cpp:1806 | parity |
| Index entry flags gate | DIRECTORY(0x10000000) or ARCHIVE(0x20); reject HIDDEN\|SYSTEM | (FUN_18000e970) | cr_mftwalk.cpp:1810–1813 | parity |
| MFT $DATA span check | Σrunlen == lastVCN−firstVCN+1 | FUN_180044bd0 :54310 | none | **D-18** |
| Compression-unit map | pow2 1..0x1000, >0x1000→0 | FUN_1800067e0 :4911–4963 | cunit read from attr+0x22 | **D-9** |
| Name invalid chars | `"*/:<>?\|` + NUL | FUN_1800433a0 :53364–53373 | none (name kept) | **D-8** |
| LostDir naming | "LostDir%d" | (M-103, :48915–48924) | cr_mftwalk.cpp:1187 | parity |
| USN reason | FILE_DELETE 0x20000 | (winioctl) | cr_mftwalk.h:117 | parity |
| attrs-per-record cap | none (offset-driven) | FUN_180041370 | 64 | **D-8** |
| recnum source | rec header +0x2C (u32) | FUN_180042ff0 :53214–53226 | walk index | shape |
| progress tick | per 0x8000-sector chunk | :17937–17958 | per 0x40 records cr_mftwalk.cpp:484 | **D-6** |

---

## 3. DIVERGENCE CANDIDATES (numbered, falsifiable)

1. **Viability probe granularity (main walk).** og: ONE bitmap probe per RUN at the run's
   start cluster (`sector = spc × lcn`, FUN_18001acc0 :21220–21228; total++ per run).
   core: `VolumeBitmap::reused_ratio` probes EVERY cluster of every run
   (cr_bitmap.cpp:94–104, 64 MiB/run cap). Falsify: deleted file, run of 100 clusters,
   first free / rest allocated → og: intact=0/total=1 → kept un-destroyed, grading
   intact-partial; core: ratio 0.99 → PartialRisk, near-Overwritten. (The core's dump
   pass IS per-run-start — cr_mftwalk.cpp:1558–1569 — so only the main walk diverges.)
2. **Viability input provenance.** og ctrl+0x40/+0x48 map is WALK-BUILT (claim map:
   FUN_18000a040 :7602–7781 sets up the walk, FUN_180008c40 :6705–6836 marks node
   claims); core main-walk grades against the live $Bitmap FILE (cr_bitmap.cpp:19
   record 6). Falsify: cluster allocated ONLY to metafiles / to deleted records →
   $Bitmap says allocated, claim map says unclaimed → different Overwritten grading.
   The deep-dive artifact §4's "$Bitmap objects" reading conflicts with the core's
   S-012 claim-map reading — **needs lead verification** (trace FUN_18000fae0/
   FUN_18000fe10 to see what is installed at +0x40/+0x48 and whether $Bitmap file
   bytes are ever read into it).
3. **USN-journal evidence loses content in core.** og FUN_18001d690 :23215–23386
   matches a journal-deleted name against the result tree (≤5000 entries) and ADOPTS
   the matched node's runlist + size (`FUN_180003250` copies the {kind,hex-runlist}
   pair and size, :2340/:2348); core's harvest emits metadata-only nodes
   (logical_size = 0, no extents, cr_mftwalk.cpp:1375–1391). Falsify: journal-delete
   whose MFT record was reused but a same-name live record exists → og surfaces
   recoverable extents; core surfaces a size-0 Lost node.
4. **No $LogFile replay in core (coverage).** og loads ≤128 MB of $LogFile
   (FUN_18001cf20 :23033 cap 0x8000001), walks restart pages ("RCDR" 0x44524352 at
   +0x2000, FUN_18001d460 :23141) and replays log records (FUN_180029dd0) to recover
   very-recent deletions; core has zero $LogFile machinery. Falsify: volume where a
   deletion is only in $LogFile (MFT record already reused AND $J rotated) → og finds
   it, core does not.
5. **USN harvest bounds.** og walks the ENTIRE $J in 10 MB chunks with carry-over
   record reassembly (FUN_1800193c0 :19913–19988, buffer 0xa00064, reassembly state at
   ctx+0x58/+0x60 in FUN_180019570 :19993–20021); core mines a 64 MB TAIL window
   (USN_HARVEST_CAP, cr_mftwalk.cpp:25, :1264–1267) with 1 Mi resync budget. Falsify:
   a >64 MB non-wrapped journal whose deletions are older than the last 64 MB → og
   surfaces them, core misses them. (Conversely the core's tail-window is the
   N-039 deliberate fix for WRAPPED journals — the og's head-to-tail walk re-parses
   stale head records; net effect depends on wrap state.)
6. **Progress tick units.** og ticks per 0x8000-sector read chunk with a per-sector
   offset map (FUN_180016750 :17937–17958; quick progress = sectors/total); core ticks
   every 64 records (cr_mftwalk.cpp:484). Observable via progress-callback cadence on
   a fragmented MFT (chunk boundaries ≠ record boundaries).
7. **Fixup gate strength.** og's fixup entry additionally demands
   `allocSize % 0x400 == 0 && allocSize ≤ 0x1000`, `usaCount ≤ 0x10`, and
   USA offset ∈ [0x2A, 0x100] (FUN_180046160 :55195–55212, called from
   FUN_180041bb0 :52524); core validates only offset nonzero/even + span bounds
   (cr_fixup.cpp:34–42). Falsify: record with USA offset 0x28 or 0x150 → og rejects
   (skip), core accepts (parses). Sub-cases go both directions (og accepts odd offsets
   ≥0x2A that core rejects — e.g. 0x2B).
8. **Record-level rejection on bad $FILE_NAME / attr-walk differences.** og: an
   attribute whose $FILE_NAME contains an invalid char fails the ENTIRE record
   (FUN_1800434f0 → FUN_180041370 returns 0, :52190–52192) and the 0x3f7 attr-offset
   cap bounds the walk with no attr-count cap; core: a bad name only drops that name
   (parse_file_name → skip), the record survives via other names, and the walk caps at
   64 attrs (cr_attributes.cpp:23) with no 0x3f7 rule. Falsify: record with name
   "a/b" + a valid second $FILE_NAME → og drops the whole record, core emits it.
9. **Compression-unit derivation.** og computes cunit from
   `pow2map(ceil($DATA realSize / ΣrunClusters / sectorSize))`
   (FUN_1800069e0 :5144 → FUN_18002e2b0 :37240–37262 → FUN_1800067e0 :4911–4963);
   core reads the cunit u16 directly from the nonresident header +0x22
   (cr_mftwalk.cpp:138–143). Falsify: torn record with cunit field 0 but consistent
   run/size geometry → og expands with computed unit, core refuses (failed=true, no
   extents) — or the reverse for a bogus stored cunit.
10. **Directory-tree construction source.** og complete scan walks
    $INDEX_ROOT/$INDEX_ALLOCATION nodes (FUN_180003ed0 :2943–3150 + FUN_180004c00)
    as a first-class tree source; core builds the tree ONLY from $FILE_NAME parent
    refs (cr_mftwalk.cpp:1145–1199) and harvests INDX solely in remnant mode
    (:1920–2032). Falsify: healthy volume with an index entry whose MFT record is
    missing → og complete scan surfaces it from the index walk; core only if the
    remnant/lost-partition pass runs.
11. **MFT read shape.** og reads the MFT in 16 MB chunks with in-buffer iteration
    (FUN_180016750/FUN_18001a980); core reads record-by-record through
    `read_record` (cr_bootstrap.cpp:519) — per-record VCN→LCN + dev read. Not
    output-visible, but IO-pattern and partial-read failure semantics differ (og: one
    failed chunk read aborts that run; core: per-record ERR → damaged count).
12. **$MFTMirr identification strategy.** og identifies mirror/MFT records by NAME
    ($MFTMirr/$MFT/$LogFile/$Volume/$Bitmap via FUN_180040d70/FUN_1800059f0 type
    dispatch) inside scanned windows and validates the MFT|mirror pair at
    window_start/+0x100 (FUN_180003ed0 :3012–3014); core uses fixed positions
    (record 1's copy at mftmirr_lcn+record_size, cr_bootstrap.cpp:323). Falsify:
    mirror region with permuted records → og still finds the mirror record by name,
    core's positional read hits the wrong record.
13. **Quick-vs-complete product split.** og: two exports, two controllers, ini gate
    `fastsearch` default-on (:7163); core: one walk + flags, no config surface.
    Behavioral only at product level (both core paths always run the "full" shape).
14. **Deleted size<0 recompute.** og: props size < 0 → recompute
    `spc × Σrunclusters × 0x200` (FUN_180007270 :5476); core keeps the parsed size
    (torn size → 0 → rescue path). Falsify: deleted record with size 0xFFFFFFFFFFFF
    and valid runs → og emits a plausible size, core emits 0/rescue.
15. **Metafile table + $SI condition.** og pass-2 gate = og-15 names AND $SI
    hidden+system (FUN_180040ea0); core = 20-name table, name-only (cr_mftwalk.cpp:220–229,
    N-040/I-001 registered superset). Reference item, already documented.
16. **$UsnJrnl ADS enumeration bound.** og scans ≤10 $DATA attrs for the "$J" name
    (FUN_1800190d0 :19888); core scans all (cr_mftwalk.cpp:677–687). Minor;
    diverges only on pathological records with >10 streams.
17. **$Extend discovery path.** og locates $Extend via the root directory's
    $INDEX_ROOT/$INDEX_ALLOCATION entry search (FUN_180018a80 :19568–19663), then
    $UsnJrnl via $Extend's index; core requires the $UsnJrnl MFT record to survive the
    walk (cr_mftwalk.cpp:669). Falsify: $UsnJrnl record wiped but $Extend index intact
    → og still harvests the journal, core's harvest silently never runs.
18. **$MFT $DATA span validation.** og rejects an MFT stream whose
    Σ(run lengths) ≠ VCN span and (in the unnamed-sparse check) refuses sparse runs
    for the MFT (FUN_180044bd0 :54294–54314); core accepts any decoded MFT runlist
    (cr_bootstrap.cpp:173–177) — holes fail vcn_to_lcn per record instead. Falsify:
    MFT $DATA with a 1-cluster hole mid-span → og refuses the volume, core opens and
    emits records around the hole.
19. **Stale core doc (internal).** cr_bitmap.h:41 comment says "Sparse runs count as
    reused" but cr_bitmap.cpp:96 skips them (S-001 landed behavior) — doc-only fix.
20. **$Tops/$TxfLog/$RmMetadata/$Repair name filters.** og returns NULL props for
    these names (FUN_180006db0 :5295–5324); core's is_metafile_name covers
    $RmMetadata but NOT `$Tops`, `$TxfLog`, `$Repair`, `$SDH`, `$SII`, `$Transact`,
    `$Config`… wait — core table (cr_mftwalk.cpp:221–224) DOES list $RmMetadata,
    $UsnJrnl, $SDH, $SII, $Transact, $Config but NOT `$Tops`/`$TxfLog`/`$Repair`/
    `$Quota`→(has it). Falsify: a live `$TxfLog` record → og suppresses, core emits.
    **needs lead verification** (confirm the og list is exhaustive for these four).

---

## 4. COVERAGE NOTES (no core counterpart)

- **$LogFile replay** (load ≤128 MB, RCDR restart pages, FUN_180029dd0 log walk) —
  nothing in core (D-4).
- **MFT growth-by-probing** (FUN_180007670 :5617–5687): extends the MFT's last run
  past its recorded length by counting fixup-valid 1 KB records in 2 MB probes —
  truncation salvage; core has no equivalent.
- **File-backed bitmap temp file** (CreateFileMappingW + MapViewOfFile + EPMNtDrv
  IOCTL 0x900c4 + DeleteFileW, 1.6 GB in-memory cap — FUN_18002bfd0/FUN_18002c250
  :35484–35638): core's bitmap is always in-memory.
- **Worker/threading infrastructure** (condition variables via GetProcAddress,
  InitializeCriticalSectionAndSpinCount, CStorageDeviceWithCache read cache) — core
  has a ThreadPool only for the remnant scan.
- **Volume label extraction** ($VOLUME_NAME attr 0x60 → codepage 0x50 string,
  FUN_18003f3a0 :50963–50981) — not in the scanned core NTFS files.
- **FS-code priority weights / partition scoring** (FUN_18000dc70 :10534–10553) —
  core has no cross-partition scoring in these files.
- **fastsearch ini config** — no core config surface.
- Covered in core (verified): two-pass S-012 GUID-dump walk, size-0 mirror rescue,
  same-name merge, USN $J harvest (bounded), remnant + INDX second-source harvest,
  LostDir grouping, recycle-bin $I/$R, $ATTRIBUTE_LIST chase (depth-1 parity flag),
  resident content, compression envelope expansion (GAP-P1-1 — og has NO decompression:
  it only records the compression flags/unit; the og never expands LZNT1).

## 5. Artifact-corrections discovered during the census

- algorithms-v2 §1: "FUN_180041970 = record_in_use (parsed+0x20)" is WRONG — +0x20 is
  the base-record reference (extension-record gate); the in-use bit is flags&1 via
  FUN_1800415a0/FUN_180043060. The artifact's "ONLY NOT-IN-USE records pass" claim is
  therefore wrong for FUN_18001acc0 (both live and deleted records are processed; the
  pass is the dump/second-source walk, not a deleted-only sweep).
- deep-dive §4: "$Bitmap cross-check" — the +0x40/+0x48 map is built by walking
  (CNTFSBuildVolumeBitmap), not read from the $Bitmap file, per FUN_18000a040/
  FUN_180008c40; competing reading flagged as D-2 for lead verification.
- deep-dive §3 step 3 "FUN_180034b20 == 0 // not a 'bad' record": confirmed — it is
  the og-15-name + $SI hidden&system metafile gate, not a corruption check.
