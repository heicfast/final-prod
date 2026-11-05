# Agent A — RAWSearchDLL raw-carving pipeline census (function-level)

> Read-only parity audit lane A. Ground truth: the og's `RAWSearchDLL.dll` (Ghidra decompile,
> 80,614 lines / 1,921 functions), cited as `RAWSearchDLL FUN_XXXX :lineno`.
> Counterpart: `core_recovery` carve sources, cited as `cr_carve.cpp:lineno` etc.
> Scope excludes sector/LBA arithmetic (lane D owns that) and the KNOWN COVERED list
> (N-204 buffer geometry, M-140 CSL/CD01 latch decline, N-209 EXIF 39-char cap, S-042 WMF,
> M-081 CRW/MRW attribution, N-093 MID/GZ caps, A-010 ASF walk, S-037 FH11/CPT, A-010/S-037/N-086
> 131-class census, S-039 user-sign DB caps, DRWUI byte-size formatters).
> Reading posture: the full decompile was walked in ordered chunks; STL/CRT filler and the
> ~190 per-analyser grammar bodies (already census'd by the corpus artifacts + the
> known-covered findings) were skimmed, while every pipeline-bearing function was read
> line-by-line and is quoted below.

---

## 1. Pipeline map (ordered stages)

### Stage 0 — DLL export surface (host contract)

**FUN-export block RAWSearchDLL 72122–72221** — 6 exports, all `__fastcall`-family (Ghidra
`undefined8*` x64 ABI):

| Export | Line | Behavior |
|---|---|---|
| `EUCreateRAWSearcher` | 72122 | `new(0x160)` → `FUN_18000bba0` = **CRAWSearchController ctor** (352-byte object; no FS metadata state). Fails → `*param_1 = 0`. |
| `EUCreateRAWSearcherForCellTest` | 72142 | Static once-init controller into `DAT_180096240` (`_Init_thread_header/footer` + atexit), then `FUN_18000c650` binds the `CRawFileCellTestStorageDevice` synthetic device and feeds it sector ranges — an in-DLL unit-test harness for carvers. |
| `EUGetAnalyserList` | 72170 | `FUN_180006a80()` (registry singleton) → `FUN_180006b20(reg, 1)` = list view at singleton+0x30. |
| `EUGetAnalyserMap` | 72184 | Same singleton → `FUN_180006b00(reg, 1)` = u32-keyed map view (+0x10). |
| `EUGetAnalyserMultimap` | 72198 | Same singleton → `FUN_180006b10(reg, 1)` = u16-keyed multimap view (+0x20). |
| `GetRawUserInfoMap` | 72213 | `FUN_180001350()` + 0x90 — the per-extension user-preference table (slot array built 1..0xAA at :79–89). |

The host (DRWUI) enumerates carvers through the three projections to build the
"which formats can we carve" UI, then drives the controller through its
`ICompleteFileSystemSearchController` vtable (slots observed from call sites:
+0x58 reset, +0x98 `Search`, +0x110 `Init/SetRange`).

### Stage 1 — registry construction (DLL load time, static init)

- **FUN_180006a80 :4317–4336** — thread-safe singleton getter; one-time builder at :4240–4248:
  ```c
  DAT_1801181f0 = FUN_180007710();
  puVar1 = FUN_180010a40();          /* user-info / raw_signs singleton */
  FUN_180010b10(puVar1, ...);        /* loads config/RawConfig.ini [Filter] */
  FUN_180002a50(&DAT_1801181e0, ...);/* u32 4CC registry  → singleton+0x10 */
  FUN_180004320(&DAT_1801181f0, ...);/* u16 magic registry → singleton+0x20 */
  FUN_180004fb0(&DAT_1801181d0, 1, ...); /* all-analyser list → singleton+0x00 */
  ```
- **FUN_180002a50 :1436–2516** — u32 registry builder: per-format blocks
  `new(0x18|0x20)` node → factory-construct analyser (e.g. `FUN_180033620` = JpegAnalyzer,
  `FUN_180035e00` = PDF, `FUN_180037b50` = TIFF family) → RB-tree descent on
  `node[8] < key` → `FUN_18000acf0` (node make) + `FUN_18000ad50` (insert/rebalance) →
  analyser stored at `node+0x28` (`*(uint **)(puVar7 + 10) = puVar2`, :1572).
  First keys: `0xe0ffd8ff`/`0xe1ffd8ff` JPEG (:1458/:1488), `0x46445025` %PDF (:1578),
  `0x2a004d4d`/`0x2a4949`/`0x554949`/`0x4f524949` TIFF family (:1608–1698),
  `0x31685a42` BZh1→DMG chain (:1781), `0x62564f46` FOVb→X3F chain (:1803).
- **FUN_180004320 :2517–2918** — u16 registry builder (39 keys: `%!` 0x2125, `BM` 0x4D42,
  `MM` 0x4D4D, `FL` 0x4C46, `<h`/`<H`/`<!` HTML, `1F 8B` 0x8B1F, MPEG syncs 0xFFF1..0xFFFF
  and 0xFFE2..0xFFE7, `0x0100`, default/fallback `0x0000` — dead under the zero-header
  guard :9954). Install pair: `FUN_18000ac20` (find-or-create node) + `FUN_18000ac70`
  (append analyser to node's chain) :7916–7961.
- **FUN_180004fb0 :2919–4208** — the all-analyser list. Reads `only_common` from
  `config/config.ini` (hand-rolled INI parser `FUN_180007890` :5233, section `"only_common"`,
  key `DAT_180066bcc`) at :3013; **if set, the list contains ONLY
  `RawFormat_xjq::CommonRawAnalyser` (ID 0xA8)** :3017–3055. Otherwise ~106 classes are
  constructed in order (FileSWFAnalyser :3067 … Brance::LZH :3097, BruteForce ADTS/MP3
  :3143–3203, FileEMF :3203, RawSpeed TAR :3254, xjq HEIC/JP2/MOBI :3367–3608, VCF/POS/OSP
  :3423–3728, CFileAIFAnalyzer ID 0x50 :4141, CFileAACAnalyzer ID 0x4F :4168,
  FileFCPEVENTAnalyser ID 0x73 :4195), and **CommonRawAnalyser is appended LAST** as the
  raw-sign catch-all (`if (param_2 == 0)` :4076–4098).
- **FUN_180006b30 :4386–4434** — per-search registry rebuild with the host's
  `IAnalyserFilter` callback: rebuilds all three structures, then REMOVES list nodes whose
  analyser is neither exempt nor approved:
  ```c
  while ((_Memory != plVar1 &&
         (((uVar4 = *(int *)(_Memory[2] + 8) - 0x30, uVar4 < 0x19 &&
           ((0x1000801U >> (uVar4 & 0x1f) & 1) != 0)) ||
          (cVar3 = (**(code **)(*param_2 + 8))(param_2), cVar3 != '\0')))))
    _Memory = *_Memory;            /* keep */
  ... unlink + free + delete       /* else remove */
  ```
  Exempt IDs (mask 0x1000801 → bits 0/11/24 over id−0x30): **0x30 FileOLEAnalyser (:2337),
  0x3B Brance::RIFFAnalyser (:2304), 0x48 FileZIPAnalyser (:2243)**. (N-149 already
  re-verified this mask; recorded here for census completeness.)

### Stage 2 — controller init / config pass

- **CRAWSearchController ctor FUN_18000bba0 :8859–8894** — 0x160 bytes; default sector size
  `0x200` at :8881 (`*(undefined4 *)(param_1 + 0x1e) = 0x200`), pending-list head via
  `FUN_1800077d0` at +0xF8, `InitializeCriticalSection` at +0x128.
- **Init FUN_18000beb0 :9006–9240** (vtable setter `FUN_18000cf20 :9698` binds +0xE8 first):
  - User-info singleton defaults :9047–9051: `*(u64*)(ui+0x60) = 1` (enabled),
    `*(u64*)(ui+0x70) = 200000000` (200 MB default cap), `*(u64*)(ui+0x68) = 0`, and map
    reset `FUN_180001500(ui+0x90)`.
  - `filter_generalraw` config: builds the string at :9076–9084, parses an XML doc
    (`FUN_180057320`/`FUN_180057b80` = tinyxml2) :9093–9139, installs the per-key filter
    list into the user-info map at +0x18 (:9142–9148).
  - Registry singleton init :9170–9179; host filter object bound at :9180–9183
    (`FUN_180006b30(&DAT_1801181a0, ppuVar8, ...)`).
  - Device sector size resolved (CellTest `+0xc` vs vtable+5) :9184–9204; stored at +0xF0.
  - Double buffer allocation :9205–9209: `*(int*)(this+0xD8) = sectors_per_half *
    sector_size * 2` and `alloc(size + 0x1000)` → `+0xE0`.
  - **EUVideoRepair binding :9210–9237**: `LoadLibraryA("...\EUVideoRepair.dll")`,
    `GetProcAddress("GetIMovMp4Repair")` → `+0x158`; on success the repair interface is
    initialized with (device, 0, sector_size × device_size).
- **Session start / buffer sizing FUN_18000c430 :9245–9329** — the `Search` prelude:
  ```c
  if ((int)param_3 == 0) {
      iVar3 = SUB164((... ZEXT816(0x400000)) / SEXT816(param_2), 0);   /* 4 MiB / sector */
      *(int *)(param_1 + 200) = iVar3;            /* +0xC8 sectors per half  */
      iVar3 = iVar3 * *(int *)(param_1 + 0xf0) * 2;
      *(int *)(param_1 + 0xd8) = iVar3;           /* +0xD8 total buffer = 8 MiB */
  ```
  then alloc `(bufsize + 0x1000)` and, once, the **CNeedRawMgr** at +0xD0
  (`alloc(0x10048)` = 4096 × 0x10 buckets + header, :9297–9327) carrying the
  IIterator/ListIterator<CRawFileInfo*> machinery and two internal lists
  (+0x10000 found list, +0x10028/+0x10038 hash maps).
- **RawConfig.ini [Filter] loader FUN_180010b10 :13066–13220** — module-dir resolution
  (:13129–13131), then:
  ```c
  FUN_1800114f0(local_1448, &DAT_180066f88, &DAT_180096ad0, "config/RawConfig.ini");
  DVar3 = GetPrivateProfileSectionA("Filter", local_1038, 0x1000, lpFileName);
  ...
  if ((local_19f8 == 1) && (*(char *)pppcVar6 == '1')) { bVar2 = true; }
  ...
  FUN_180011a10(plVar1, ...);   /* key=1 → enabled map */
  ```
  key=1 → enabled map, else disabled map (:13180–13203). This is the M-024 surface.
- **raw_signs.json loader FUN_18001ea40 :23765–24000** — `FUN_1800098d0(local_2e8,
  "raw_signs.json")` :23836 (ifstream), seekg(end)/tellg size :23847–23850, whole-file
  `read` :23889, `FUN_180057b80` (neb::CJsonObject) parse :23891, per-extension walk
  building header/tail sign trees at CommonRaw+0x110/+0x118. Sign string parser
  **FUN_18001e510 :23522–23700**: splits on `,`, parses part[0] as a number
  (:23633–23648, `0x1999999999999999` overflow guard), consumes part[1] (hex body);
  part[2] (the JSON "confidence") is never read — the N-030 dead-data decode, re-verified.
- **User-sign/extension map FUN_180001350 :65–100** — the 1..0xAA (169) slot-array
  constructor + `atexit` teardown; exposed at +0x90 by `GetRawUserInfoMap`.

### Stage 3 — the block scan loop (per 4-MiB chunk)

**FUN_18000c820 :9385–9633** — vtable+0x98 `Search(startSector, sectorCount, pFilter)`
(host calls it once per chunk; `FUN_18000c650 :9337–9380` shows the CellTest driver issuing
`halfSectors`-sized calls):

1. **Chunk read** :9434–9438 — `device->Read(buffer + halfSize, startSector, sectorCount)`
   (device read wrapper `FUN_1800425c0 :50388–50405` divides byte args by sector size).
2. **Per-chunk liveness gate** :9439–9441 — global object `FUN_1800424b0()`
   (`DAT_1801183c0`, host-settable via `FUN_18000cf40 :9717`): scan proceeds when absent or
   its vtable+0x10 returns nonzero.
3. **Per-sector loop** `local_1e0` 0..sectorCount, `+1` per iteration (:9616–9618):
   - **Filter/bitmap/stop gate** :9446–9460:
     ```c
     if ((param_4 == (longlong *)0x0) || (*param_4 == 0)) { /* no bitmap → probe */ }
     else if (*(char *)((longlong)param_4 + 0x1d) == '\0') {
         uVar9 = *(uint *)(param_4 + 3);                      /* +0xC cluster size */
         bVar6 = FUN_180058df0(local_1c0, (param_2 + local_1e0) / uVar9);
         if (((bVar6 != 0) ||
              (local_1c0[0xc] != 0 && (cVar7 = stopCb->vt20(), cVar7 != 0)))
             && (bVar5 = true, (param_2 + local_1e0) % uVar9 == 0)) {
             local_1e0 = local_1e0 + (uVar9 - 1);             /* cluster jump */
         }
     }
     ```
     `FUN_180058df0 :67842–67856` is the raw bit test
     (`bits[(i+base)>>3] & 1<<((i+base)&7)`, OOB→0). The **stop callback lives at
     filter+0x60 (vtable+0x20) and is polled EVERY SECTOR**; a firing stop is folded into
     the same cluster-jump path (`bVar5`). NOTE (decompiler ambiguity): the
     `if (!bVar5) goto LAB_18000c99a` shape at :9447–9448 makes the skip-vs-still-dispatch
     outcome ambiguous in the decompile — flagged in §3 DC-A4.
   - **Candidate pointer** :9479–9481: `puVar1 = (uint*)(startSector + local_1e0)` — the
     ABSOLUTE SECTOR NUMBER (emission/dedup key), bounds-checked vs device size
     (`FUN_180042590`).
   - **First-u32 probe + trailing-zero metric** :9496–9510:
     ```c
     puVar19 = (uint *)(sector_size * local_1e0 + halfSize + buffer);  /* sector in 2nd half */
     uVar20 = 4; if (*puVar19 != 0) { uVar20 = 0; }        /* 4 ⇔ first u32 == 0 */
     iVar17 = 0; iVar8 = 0x1c; piVar15 = puVar19 + sector_size - 4;
     do { if (*piVar15 != 0) break; iVar17 += 4; piVar15--; iVar8 -= 4; } while (-1 < iVar8);
     ```
     uVar20 = zero-header flag; iVar17 = trailing zero dwords (max 8 dwords = 32 bytes at
     sector tail). Both are passed to the dispatch block.
   - **Look-back window computation** :9511–9527: `lookbackStart = startSector -
     (halfBuffer / sectorSize)`; dispatch block `{lookbackStart, endSector, 0, uVar20,
     iVar17}`; then **dispatch** `FUN_18000cf60(this, puVar19, FileInfo, absSector, &blk)`
     :9533.
   - **Hit path** :9541–9570 — `analyser->vt0x38()` creates the per-analyser
     `CRawFileInfo`; ID cache `local_150 = analyser+4` (:9543–9546); `info[1] = absSector`,
     `info[2] = name/size`, `info+0xD0 = analyser`, `info+0x1C = ID` (:9546–9550); inserted
     into the found list (`FUN_180009f00` into CNeedRawMgr+0x10000, count +0x10008,
     :9551–9563). If `info[2] != 0` (size known) → notify host callback `+0x108`
     (:9564–9567) and free. Else `analyser->vt8()` ("needs continuation?") → pending.
   - **Pending push + 16-entry cap** :9571–9605:
     ```c
     if (((pendingCount != 0) && (0x10 < pendingCount + 1U)) &&
        (oldest = firstPending, oldest != 0)) {
         oldest->analyser->vt0x10(oldest, puVar19, absSector);   /* force-finalize */
         FUN_18000d980(this, oldest[1], oldest->id);              /* record */
         hostCallback(+0x108, oldest);
         unlink; pendingCount--;
     }
     ... insert new pending node; pendingCount++;
     ```
   - **Continuation walk** `FUN_18000d670 :10068–10153` (reached via LAB_18000ccfc
     :9537/9606) — iterates the pending list; per pending analyser:
     `analyser+0x1D = device`, `FileInfo+0x18 = analyser`; `vt0x20(buffer, sectorSize,
     absSector, FileInfo)` (:10114–10115). Return 1 → if host callback `+0x108` bound:
     `FUN_18000d920` record + callback + **the EUVideoRepair poll**:
     ```c
     if ((this->repair != 0) &&
         (cVar3 = analyser->vt0x18(&q0,&q1,&q2,&q3), cVar3 != 0))
         repair->vt0x48(q0, q1, q2, q3);        /* :10130-10136 */
     ```
     then unlink the completed pending.
   - **Sector advance**: always `local_1e0++` (:9616) — **no jump past an emitted extent**;
     the only forward jumps are the bitmap/stop cluster jump (:9458).
4. **Chunk slide** :9620–9622 — `memmove(buffer, buffer + half, half)` (the ping-pong;
   look-back for the next chunk).
5. **Per-chunk EUVideoRepair stream feed** :9623–9628:
   ```c
   plVar10 = *(longlong **)(param_1 + 0x56);          /* +0x158 repair interface */
   if (plVar10 != 0)
       repair->vt0x40(sector_size * startSector, buffer, halfSize);
   ```
   EVERY scanned chunk (offset, buffer, 4 MiB) is streamed to the video-repair module —
   independent of the per-candidate poll above.

### Stage 4 — signature dispatch (per sector)

**FUN_18000cf60 :9730–9925** (reads `only_common` once via `FUN_180007890` :9766–9841):

1. **u32 registry path** :9842–9886 — `uVar2 = *sector`; skip when 0; RB-tree lower_bound
   on singleton+0x10; exact-match node's analyser chain:
   ```c
   plVar7 = (longlong *)puVar12[5];                  /* analyser            */
   lVar6 = (**(code **)(*plVar7 + 0x40))(plVar7);    /* GetSpecial()        */
   if (lVar6 == 0) {
       iVar3 = (**(code **)(*plVar7 + 0x10))(plVar7, param_2, param_3);  /* Analyse */
       if (iVar3 == 1) { FUN_18000d920(...); return; }                    /* accept */
   } else {
       iVar3 = special->vt0(sector, FileInfo, sector_size);              /* 2-step  */
       if (iVar3 != 0) { FUN_18000d920(...); return; }
       iVar3 = (**(code **)(*plVar7 + 0x10))(plVar7, param_2);           /* plain   */
       if (iVar3 != 0) { FUN_18000d920(param_1, param_4, 0, param_3); }
   }
   ```
   **First-accept-wins; the loop does not continue past an accepting analyser.**
2. **u16 registry path** `FUN_18000d3e0 :9933–10063` — zero-guard `*(int*)param_2 == 0 →
   return 0` (:9954); lower_bound + upper_bound walk over the equal-range (multimap
   semantics); per analyser the same two-step special/plain dispatch; on accept:
   ```c
   local_40 = *(int *)(param_3 + 0x18);        /* FileInfo format ID   */
   plVar8 = FUN_18000e980(mgr+0x10038, &key);  /* find-or-make node    */
   FUN_18000e9d0(mgr+0x10038, out, analyser, &key, node);  /* insert   */
   return plVar10;                              /* FIRST accept returns */
   ```
3. **Fallback: full analyser list** :9894–9921 — iterate every list node; same two-step
   dispatch (`plain` gets only `(buffer)` here, no FileInfo — a per-layer calling
   difference); `break` on the first accept with `FileInfo+0x10 != 0` gate.
   CommonRawAnalyser (raw-sign layer) is tried LAST here.

**Dispatch order/precedence**: u32 4CC map → u16 magic map (equal-range) → full list
(grammar analysers in list order, CommonRaw last). Within every layer:
first-accept-wins, position claimed by the first accepting analyser.

### Stage 5 — validator grammar mechanics (per analyser)

Analyser vtable (IRawFileAnalyser): `+0x08` needs-continuation?, `+0x10` Analyse
(buffer[, FileInfo]) → 0 decline / 1 accept / 2 need-more, `+0x18` GetRepairInfo,
`+0x20` Continue(buffer, sectorSize, absSector, FileInfo) → 1 complete, `+0x38`
CreateFileInfo, `+0x40` GetSpecial (→ ISpecialRawFileAnalyser with the 7-arg Analyse).
(N-143 already fixed the core's two-step special dispatch.)

**CRawFileInfo** (`FUN_18000b520 :8450–8517`, 0x108 bytes): `+0x08` absolute sector
position, `+0x10` size, `+0x18` format ID (analyser-stamped: TAR 0x3F :51234, RAF 0x40
:59555, PDF 0x32 :41234, WEBP 0x7E :49349, AI-EPS 5 :41246), `+0x1C` 0xFFFF default,
`+0x20/+0x40/+0x68/+0x88/+0xB0` strings, `+0xA8` 0xFFFFFFFF, `+0xD0` analyser,
`+0x100` 0xFFFFFFFF, `+0x104` flag.

Size-inference order (JPEG exemplar `FUN_180033a90 :39483–39710`):
1. Header fields: JFIF/EXIF parse (`Exif` 0x66697845 :39550, `JFIF` :39556 →
   `FUN_180034560`); SOF dimension pre-scan with `width,height < 20000` (:39608),
   `pixels = w*h` latched at +0x118/+0x128, size-latch /6 when 0x400 (:39613–39616);
2. SOF0 grammar gate: `bVar1 == 0xc0 && pbVar25[4] == 8` (precision 8, :39653) +
   `400 < width` (:39654) → `size += (pixels+rounding)>>3` (:39657–39659);
3. **End-tag walk (backward)** :39701–39709: scan backward from
   `buffer + (size − lookback)` for `FF D9` — the LAST EOI in the tail window;
4. Bounded fallback: cap/10-class salvage (the legacy tier; core S-018 mirrors it).
   JpegRawFileInfo defaults `FUN_180033920 :39415–39435`: min 0x400, max 0x7800000
   (120 MiB), ID 0x23. PdfRawFileInfo `FUN_180036090 :41256–41276`: min 0x5000, max
   200000000, ID 0x32; EPS/AI discrimination at :41223–41247.
   Raw-sign walker `FUN_1800205b0 :25020–25250`: returns **2 = "need more data"** while
   `span < maxKiB * 0x400` (:25192–25194), default max `0x7D000` KiB = 500 MiB when 0
   (:25189–25191), emitted size `max << 10` (:25196–25197).
   The GZ analyser embeds a full **zlib inflate** state machine
   (`FUN_180054c10 :64465+`, gzip header 0x8B1F, "incorrect header check"/"invalid window
   size"/"unknown compression method" strings :64537–64560) to validate and find the
   stream end (N-093 owns the cap facets).
   Continuation/latch state: the pending list (+0xF8/+0x100) with per-candidate
   FileInfo; decline simply returns 0 (no latch); "need more" (return 2) latches the
   candidate into the pending list until `vt0x20` completes or the 16-cap evicts.

### Stage 6 — result emission & dedup

- **FUN_18000d920 :10158–10178** — record: `local_20 = FileInfo+0x18 (ID) or analyser+8`;
  insert into CNeedRawMgr map at +0x10038 keyed by **the absolute sector position**:
  ```c
  local_28 = param_2;                                   /* key: abs sector      */
  plVar2 = FUN_18000e980(map, &local_28);               /* find-or-make node    */
  FUN_18000e9d0(map, out, param_3, plVar2 + 4, plVar2); /* insert-if-absent     */
  ```
  **FUN_18000e9d0 :11165–11235** is insert-if-absent: an existing key frees the duplicate
  value and returns the existing node (`free(param_5); *param_2 = plVar5; …=0`)
  — **first claim per position wins; no confidence/ID comparison at insert time.**
- Host notify: callback `+0x108` invoked per finalized candidate (:9565–9567, :9584–9585,
  :10124–10125); the tree/naming side is DRWUI's (S-049/S-048 own that).
- Counters: found-list count (+0x10008, :9557–9559), pending count (+0x100), the
  CNeedRawMgr map size; the host queries via the registry projections +
  `GetRawUserInfoMap` (N-205 owns the RawCount population semantics).
- Cancellation checkpoints: per-sector stop callback (filter+0x60 vt0x20, :9454–9457),
  per-chunk global object (:9439–9441), controller stop flag +0x150 (set in Stop).
- Progress ticks: host-driven (one Search call per chunk); no engine-internal progress
  callback exists.

### Stage 7 — completion / teardown

- **Stop FUN_18000ce30 :9658–9693** — sets `+0x150 = 1`; gated on
  `device+0x14 == 0 && (vt0x70 == default-stub || returns 0)`:
  force-cancels every pending analyser with the sentinel
  `vt0x10(analyser, 0, 0xFFFFFFFFFFFFFFFF)` (:9676), notifies the host callback per
  pending (:9677–9679), clears the pending list (`FUN_18000db40`), frees the 8-MiB buffer
  (:9685–9688), and zeroes the global stop object (`FUN_1800424b0()->ptr = 0`,
  :9689–9690).
- **Dtor FUN_18000bcb0/bdb0 :8899–8979** — frees buffer (+0xE0), +0x118 block, critical
  section, the found list, the +0xF8 list; base dtor `FUN_18000b9a0`.
- **Registry teardown FUN_1800068c0 :4265–4311** (atexit) — destroys the three structures
  and the singleton maps.

---

## 2. Constants table

### og (RAWSearchDLL)

| Constant | Meaning | Cite |
|---|---|---|
| 0x160 (352) | CRAWSearchController object size | FUN_18000bba0 :72128/:8859 |
| 0x200 (512) | default/CellTest sector size | ctor :8881; :9188/:9199/:9489 |
| 0x400000 (4 MiB) | per-half chunk size (÷ sector size) | FUN_18000c430 :9271/:9284 |
| ×2 | double buffer total (8 MiB @512B) | :9274/:9287; init :9205 |
| +0x1000 (4 KiB) | buffer over-allocation guard | :9207/:9293 |
| 0x10048 | CNeedRawMgr alloc (4096×0x10 + hdr) | :9298 |
| 0x10 (16) | **max pending/continuation candidates; 17th force-finalizes oldest** | FUN_18000c820 :9575–9592 |
| 4 | zero-header flag (first u32 == 0) | :9498–9501 |
| 8 dwords (0x20 bytes) | trailing zero-dword probe at sector tail | :9503–9510 |
| 200000000 | user-info default max (200 MB) | FUN_18000beb0 :9049 |
| 0xAA (169) | user-info slot loop bound | FUN_180001350 :79–89 |
| 0x1000801 (bits 0/11/24) | filter-rebuild exempt mask → IDs 0x30/0x3B/0x48 | FUN_180006b30 :4416–4417 |
| 0xA8 | CommonRawAnalyser format ID | FUN_180004fb0 :3036/:4093 |
| 0x7D000 KiB (500 MiB) | raw-sign default max when max==0 | FUN_1800205b0 :25190/:25235 |
| 0x400 (1024) | KiB→byte scale; also JPEG min size | :25193; FUN_180033920 :39424 |
| 0x7800000 (120 MiB) | JPEG max size | :39425 |
| 20000 | JPEG dimension sanity bound | :39608 |
| 8 | JPEG SOF0 precision gate | :39653 |
| 400 | JPEG SOF0 width floor | :39654 |
| 0x5000 (20 KiB) | PDF min size | FUN_180036090 :41267 |
| 200000000 | PDF max size | :41270 |
| 0x23/0x32/0x3F/0x40/0x7E/0x5 | JPEG/PDF/TAR/RAF/WEBP/AI-EPS format IDs | :39433/:41272/:51234/:59555/:49349/:41246 |
| 0x1999999999999999 | number-parse overflow guard (sign parser) | FUN_18001e510 :23638 |
| 0x555555555555553 | std::map "map/set<T> too long" bound | FUN_18000e6c0 :10979 |
| 0x18/0x20/0x10/0x38/0x40 | analyser/registry node alloc sizes | :1573/:1838/:1770 etc. |
| 0xFFFFFFFF | "unassigned" analyser-ID sentinel | :1767/:2253 etc. |
| 12 ("only_common"→DAT_180066bcc key len 4) | ini section/key lens | :2961/:2981 |

### core (core_recovery)

| Constant | Meaning | Cite |
|---|---|---|
| WINDOW = 1 MiB, OVERLAP = 4096 | window geometry (N-204 owns the delta) | cr_carve.cpp:45–46 |
| MAX_FILE_READ_FOR_HASH = 8 MiB | hash read bound | cr_carve.cpp:47 |
| kMaxTrackedRejections = 1<<20 | grammar-rejection set bound | cr_carve.cpp:54 |
| kMaxTrialSlots = 160 | trial-disable array | cr_carve.cpp:161 |
| 512 | raw-sign probe stride | cr_carve.cpp:918 |
| 47 | raw-sign header probe offset span | cr_rawsigns.cpp:36 |
| kMaxSignBytes = 40 | sign body cap | cr_rawsigns.h:68 |
| kRawSignTailScanLimit = 8 MiB | tail-walk bound (S-015) | cr_rawsigns.h:75 |
| 139 / 199 / 62 | raw-sign formats / header / tail signs | cr_rawsigns_data.cpp:14–16 |
| FMT_COUNT (=kTrialCount) | grammar registry rows | cr_registry.cpp:1642; cr_carve.cpp:59–146 |
| len_class: /65536, ≥1MiB→pow2+64 | extent dedup buckets | cr_carve.cpp:709–715 |
| 254 / 99999 | name prefix cap / counter padding switch | cr_carve.cpp:382/:406 |
| 600 (fixed) | user-sign carve_confidence | cr_carve.cpp:1049 |

---

## 3. DIVERGENCE CANDIDATES (new; known-covered items excluded)

Numbered, falsifiable, each with og cite vs core cite. "Uncertain" = needs lead
verification.

**DC-A1 — the pending/continuation streaming model and its 16-entry force-finalize cap
have NO core counterpart.**
- og: `FUN_18000c820 :9571–9605` — analyser returning "needs continuation" latches the
  candidate into the pending list (+0xF8, count +0x100); each subsequent sector is fed to
  every pending analyser (`FUN_18000d670 :10068–10153`, `vt0x20(buffer, sectorSize,
  absSector, FileInfo)`); completion (`vt0x20 == 1`) triggers record+notify+unlink. When
  `pendingCount ≥ 16` (`0x10 < count + 1U`, :9575–9576), the OLDEST pending is
  **force-finalized** (`vt0x10(oldest, sector, absSector)` :9579) and emitted
  (`FUN_18000d980` :9582) before the new candidate is pushed.
- core: `cr_carve.cpp:672` — validators are one-shot (`sig.validator(&wdev, cand_abs,
  sig.max_size)`); no in-flight candidate set, no per-sector continuation feed, no cap, no
  force-finalize eviction. Large-file semantics are bounded only by each row's max_size
  (cr_registry.cpp rows) and the S-015 raw-sign tail cap.
- why it matters: (a) a mid-scan `Stop` in the og force-cancels and STILL emits pending
  candidates (Stop :9671–9680 calls `vt0x10(analyser, 0, -1)` then notifies the host per
  pending), the core discards nothing-but-also-emits-nothing on ERR_CANCELLED — different
  partial-scan result sets; (b) with >16 overlapping long candidates the og emits a
  TRUNCATED oldest file (force-finalize at the current sector), the core emits full
  validators' results for all of them; (c) the og's "need more data" return-2 protocol
  (FUN_1800205b0 :25192–25194) re-scans growing spans — a different cost profile.
- M-140 covered only the CSL/CD01 latch-decline facet; the cap/eviction/continuation
  architecture is unregistered. *Uncertain: whether the og's force-finalize emission is
  byte-identical to a natural completion (the host callback receives the same shape).*

**DC-A2 — winner selection at a shared-magic position: og = first-accept-wins (registry
chain order) with insert-if-absent position→ID dedup; core = N-070 best-accumulated-score.**
- og: u32 path returns on the FIRST accepting analyser (`FUN_18000cf60 :9869–9874`
  `if (iVar3 == 1) { FUN_18000d920(...); goto LAB_18000d3c0; }`); the u16 path returns the
  first accept out of the equal-range (`FUN_18000d3e0 :10003–10016`); the fallback list
  breaks on the first accept (:9903–9908). The CNeedRawMgr insert is keyed by the
  absolute sector with **no confidence/ID comparison** — `FUN_18000e9d0 :11224–11233`:
  `if (plVar5[4] < *param_4) { insert } else { free(param_5); return existing; }`. The
  stored payload is the format ID (`local_40 = FileInfo+0x18`, :10005–10008;
  `FUN_18000d920 :10167–10170`).
- core: `cr_carve.cpp:729–738` — every first-byte-matching row is validated and the
  strictly-greater `base_confidence + cr.confidence` winner is emitted
  (N-070, landed as a fix); ties break by kTrialOrder (:56–146).
- why it matters: at a position where two grammars both accept (e.g. II\* CR2 vs DNG vs
  TIFF), the og's winner is fixed by registry chain order, the core's by score max
  (DNG 880 > CR2 850 > TIFF 800 — cr_registry.cpp:143–166). N-070's premise (og ranks by
  accumulated confidence) traces to the family artifact's claim that N-030's decode
  *refuted for the raw-sign layer* (the JSON confidence is dead data,
  `FUN_18001e510 :23522+` never reads part[2]). No confidence comparison is visible
  anywhere in the og's engine dispatch/dedup. *Uncertain — needs lead verification:*
  whether the og's host-side consumption of the CNeedRawMgr map (or the registry
  multimap ordering) re-ranks by the FileInfo confidence/ID later; if not, N-070's
  ranking is a deliberate improvement mislabeled as og parity and should be re-tagged.

**DC-A3 — the per-chunk EUVideoRepair buffer feed is undocumented and unported.**
- og: `FUN_18000c820 :9623–9628` — after the slide, every chunk is streamed to the repair
  interface: `repair->vt0x40(sector_size * startSector, buffer, halfSize)` (binding built
  in Init :9210–9237 via `LoadLibraryA("EUVideoRepair.dll")`/`GetIMovMp4Repair`).
- core: the M-021 `CarvePostProcessor` hook (cr_carve.h:152–196, fired at
  cr_carve.cpp:829–837/:1279–1287) models ONLY the per-candidate deferred poll
  (`FUN_18000d670 :10130–10136`, `vt0x18` GetVideoInfo + `vt0x48` check). No buffer-level
  stream feed exists anywhere in the core.
- why it matters: the og's repair module receives the raw disk stream *continuously*
  (one 4-MiB feed per chunk) independent of carve hits — an analysis channel the core
  does not offer. Repair-line scope is registered OOS (I-010/D-26/S-050), so this is
  INFO-tier at most, but the M-021 documentation presents itself as the og's binding
  shape and omits this second feed. *Uncertain: whether vt0x40 is pure telemetry/analysis
  or participates in repair availability (S-050's "availability when results finalize").*

**DC-A4 — cancellation checkpoint granularity and stop-drain shape.**
- og: stop callback at filter+0x60 polled EVERY SECTOR (:9454–9457) and folded into the
  bitmap skip path — a firing stop sets `bVar5` and (when cluster-aligned) jumps whole
  clusters (:9457–9458), i.e. stop ACCELERATES the drain of the remaining sector count;
  plus a per-chunk global object gate (:9439–9441) and the controller +0x150 flag (Stop
  :9667). The decompile's `if (!bVar5) goto LAB_18000c99a;` (:9447–9448) makes it
  ambiguous whether a "skipped" boundary sector still runs dispatch — both successors
  visually merge at :9462.
- core: `cr_carve.cpp:1475–1478` (serial) / `:1494–1497` (worker) — `cancel->checkpoint()`
  once per 1-MiB window, plus the per-byte `ctx.aborted` load (:576); on stop the engine
  returns ERR_CANCELLED at the window boundary (no drain, no pending flush — see DC-A1).
- why it matters: observable behavior after a stop request differs — og may keep finding
  files for up to a chunk (or drain without finding, per the ambiguity); core stops at the
  next window boundary with at most 1 MiB of latency. *Uncertain — needs lead
  verification against the disassembly (the bVar5 fall-through shape), and vs D-18
  (cancellation-first-class) which may already own the policy-level framing.*

**DC-A5 — host contract: host-driven chunk loop vs engine-internal windowing.**
- og: the host calls `Search(startSector, sectorCount, pFilter)` per ~4-MiB chunk
  (vtable+0x98; `FUN_18000c650 :9365–9376` shows the chunk-size calls;
  `FUN_1800425c0 :50388–50405` confirms sector-unit args). Progress, interleaving and
  pause are host-owned; the emission record's position field is an absolute SECTOR number
  (`info[1] = startSector + local_1e0`, :9548; dedup key likewise, `FUN_18000d920
  :10173`).
- core: one `carve_slice(...)` call for the whole slice (cr_carve.h:234–237,
  cr_carve.cpp:1399–1569); the engine windows internally (1 MiB + 4 KiB), reports
  progress via callback per window (:1477/:1506), emits byte-offset extents
  (cr_carve.cpp:808).
- why it matters: engine-boundary parity (who owns chunking/progress) and the emission
  units (sector vs byte). The byte-vs-sector probe superset is already filed (I-009/DC-2);
  the *contract* facet (no per-chunk entry point; no host interleaving between chunks) is
  not registered anywhere I can find. Likely INFO-tier (architecture), adjacent to
  D-18/D-21/D-45. *Uncertain — needs lead verification whether the API-shape register
  (D-45 cr_ui C-ABI) already subsumes this.*

**DC-A6 — user-sign emission size: og default cap 200,000,000 vs core to-device-end.**
- og: Init stamps the user-info singleton `+0x70 = 200000000` (FUN_18000beb0 :9049) next
  to `+0x60 = 1` (enabled); the user-sign machinery keys off this singleton
  (`FUN_180001350`; analysers consult `FUN_180010f50(ui, ext)` before accepting, e.g.
  WEBP :49322).
- core: user-sign hits emit `len = ctx.total - start_abs` — extent to the device end, no
  size cap (cr_carve.cpp:1013; cr_carve.h's S-055/S-056 comments describe user signs as
  header-only markers).
- why it matters: if `+0x70` is the user-sign max-size gate (its consumers include an
  analyser reading it ×6 at :59527), the og caps user-sign records at 200 MB while the
  core emits unbounded-to-EOF extents — a per-record size divergence on large devices.
  *Uncertain — needs lead verification: the +0x70 field's exact consumer set was not
  fully decoded (I read one consumer at :59520–59528 that multiplies it by 6); S-039
  covers the DB caps (entry counts/sign lengths), not this emission-size facet.*

**DC-A7 — og emits the LAST EOI in a bounded tail window (backward end-tag walk); core
accepts at the FIRST EOI of a forward marker walk.**
- og: `FUN_180033a90 :39701–39709` — after the SOF tier, the JPEG analyser scans
  BACKWARD from `buffer + (size − lookback)` for `FF D9` (`(*pcVar16 == -1) &&
  (pcVar16[1] == -0x27)`), taking the last EOI in the window as the end.
- core: `cr_analysers.cpp:744–752` — the forward marker walk returns at the first EOI
  (`mk == 0xD9 → r.end = pos + 2`); trailing-garbage JPEGs (multiple FFD9 or junk after
  EOI) carve smaller here than the og's last-EOI anchor when an EOI occurs inside the
  og's tail window.
- why it matters: per-file size deltas on damaged/truncated JPEGs (the og's tail-window
  lookback field `FileInfo+0x14` bounds the walk). *Uncertain — needs lead verification:
  FUN_180033a90's tier precedence (the backward walk's gate `8 < *(int*)(local_68+0x14)`
  at :39701) was only partially decoded this session; the JPEG tier order may already be
  owned by an S-018/S-049-family finding.*

---

## 4. Coverage notes (og stages with NO core counterpart)

1. **The pending/continuation streaming model as a whole** (Stage 5's return-2 protocol +
   Stage 3's per-sector continuation feed + the 16-cap eviction) — see DC-A1. The core's
   one-shot validator model covers the same formats but not the mechanics.
2. **EUCreateRAWSearcherForCellTest + CRawFileCellTestStorageDevice** (72142–72165) — the
   shipping product carries an in-DLL carver unit-test harness (112 vftable refs). No core
   counterpart (nor should there be; register-only).
3. **EUGetAnalyserList/Map/Multimap** (72170–72208) — three host-facing projections of
   one registry (list / u32 map / u16 multimap). The core exposes `registry()` +
   `validate_registry()` + `carve_format_listed` (cr_signatures.h:550–573) — a flat table
   with no multimap view; the signature-collision enumeration surface differs (the
   capability-enumeration consumer is host-side either way; D-45's C-ABI shim is the
   nearest core analog).
4. **GetRawUserInfoMap** (72213) — the 169-slot per-extension user preference table.
   The core's user-sign layer (cr_usersigns.cpp) is install-set-shaped, not a
   slot-array + preference map.
5. **only_common QA mode** (`config/config.ini`, FUN_180004fb0 :2952–3056) — swaps the
   whole engine to a CommonRaw-only fast scan. REGISTERED as D-114 (deliberately not
   ported); listed here for census completeness.
6. **The per-chunk EUVideoRepair stream feed** (FUN_18000c820 :9623–9628) — DC-A3.
7. **Five og config/data sources** vs the core's compiled tables + API:
   `config/config.ini` [only_common] (FUN_180007890), `config/RawConfig.ini` [Filter]
   (FUN_180010b10 :13133), `raw_signs.json` (FUN_18001ea40 :23836), EUUserData JSON
   (the user-sign DB), and the `filter_generalraw` XML (tinyxml2, Init :9093–9139).
   Core equivalents: disabled_formats list (M-024), compiled raw-sign table (139 formats
   — count parity with the og's JSON), usersigns::parse (M-048/M-129), nothing for the
   XML filter (host policy).
8. **The two-generation validator redundancy** (modern analyser + legacy `File*Analyser`
   fallback per chain — registry census §2) has no core twin (one validator per row);
   D-75 owns the registered chain/fallback divergence.
9. **LogInfo/PUBLOG + RaiseException log-then-die OOM policy** (imports :184; core
   converts to ERR_NO_MEMORY — D-115 registered; noted for completeness).

---

## 5. Verification ledger (spot re-verifications this session)

| Claim | Evidence |
|---|---|
| 6 exports + factory sizes | RAWSearchDLL :72122–72221 |
| 0x400000/2 buffer math + 0x1000 guard | :9205–9209, :9271–9296 |
| Per-sector stride +1, no extent jump | :9616–9618 (only jumps: :9458) |
| First-u32 probe + 32-byte trailing-zero metric | :9496–9510 |
| Look-back = half buffer in sectors | :9511–9527 |
| Dispatch order u32→u16→list; first-accept-wins | :9842–9921; :10003–10016 |
| CNeedRawMgr insert-if-absent keyed by abs sector | FUN_18000e9d0 :11224–11233; FUN_18000d920 :10167–10175 |
| Pending cap 16 + force-finalize eviction | :9571–9605 |
| Per-candidate repair poll vt0x18/vt0x48 | FUN_18000d670 :10130–10136 |
| Per-chunk repair feed vt0x40 | :9623–9628 |
| Stop = flag + pending force-cancel + buffer free + global zero | FUN_18000ce30 :9658–9693 |
| Exempt mask 0x1000801 → 0x30/0x3B/0x48 | FUN_180006b30 :4412–4418 (N-149 re-confirmed) |
| only_common → CommonRaw-only registry | FUN_180004fb0 :2952–3056; CommonRaw last :4076–4098 |
| raw_signs.json istream load + tokenizer | :23836–23891 (correction note in deep-dive holds) |
| Sign confidence = dead data (part[2] unread) | FUN_18001e510 :23522–23648 |
| RawConfig.ini [Filter] via GetPrivateProfileSectionA | FUN_180010b10 :13133–13203 |
| CommonRaw need-more=2 / 500 MiB default / max<<10 | FUN_1800205b0 :25189–25197 |
| JPEG SOF0 prec-8 gate, dims<20000, backward EOI | :39608/:39653/:39701–39709 |
| PDF min/max/ID; EPS/AI split | FUN_180036090 :41256–41276; :41223–41247 |
| WEBP RIFF+WEBP size+8, ID 0x7E, user-info gate | FUN_180040f10 :49322–49350 |
| zlib inflate embedded in GZ analyser | FUN_180054c10 :64465–64624 |
| Bitmap test raw bit, OOB→0 | FUN_180058df0 :67842–67856 |
| Device read converts bytes→sectors | FUN_1800425c0 :50388–50405 |
| Core: 1 MiB window, per-byte grammar loop, 512 raw-sign stride | cr_carve.cpp:45–46, :575, :918 |
| Core: N-070 score-accumulate winner | cr_carve.cpp:729–738 |
| Core: no continuation/pending model; one-shot validators | cr_carve.cpp:672 |
| Core: M-021 per-candidate post-processor only | cr_carve.cpp:829–837/:1279–1287 |

## 6. Cross-links

- Prior lanes: S55-D2-scanmath.md (DC-1 buffer geometry=N-204; DC-2 probe granularity=I-009;
  DC-3/DC-4 geometry — sector/LBA lane), S55-D3 grading, N-143 (ISpecial two-step),
  N-151/N-153 (extent consumption), N-205 (RawCount semantics).
- Corpus: rawsearchdll--analyser-registry-census.md (70 u32 keys), raw-dual-registry--
  u16-magic-keys.md (39 u16 keys), rawsearch-carving--deep-dive.md, the six family sheets.
- Core registers: PARITY-DELTAS D-13/D-63/D-64/D-75/D-95/D-114/D-115, findings N-149,
  S-015/S-017/S-018/S-049/S-055/S-056, M-005/M-021/M-024/M-117/M-140.
