# s3-ntfs-indx-second-source — og INDX/$INDEX_ROOT directory-entry harvest (first-hand decode, S3-NTFS2)

All og quotes: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` @ corpus `03ad46c`.
Core: `src/core/ntfs/` + `include/cr/ntfs/` @ `34b3441`.
This decode resolves "UNVERIFIED-7" recorded in `include/cr/ntfs/cr_ntfs_wire.h:192-195`
("og INNX walking itself is UNVERIFIED-7 in the corpus") and refutes session-1's A26
og-side premise ("no INDX walk in og either").

## 1. The og's INDX-block wire family (validators/fixup/entry chain)

`FUN_180045510 :54704` — INDX-buffer validator ('INDX' = 0x58444e49):

```c
:54709  if (*param_1 != 0x58444e49) { return 0; }                 // magic
:54713    if (param_2 < (uint)param_1[8]) { return 0; }            // size < entries-alloc(+0x20)
:54716    if (... size>>9 < *(ushort*)(param_1+6) - 1) return 0;   // sectors < usa_count-1
:54721  if ((uint)param_1[8] < (uint)param_1[7]) { uVar1 = 0; }    // alloc(+0x20) < in-use(+0x1c)
:54724  else if (*(ushort *)((longlong)param_1 + 6) < 0x11) {      // usa_count < 0x11 → ≤ ~8 KiB blocks
:54725    if (*(ushort*)(param_1+6) - 1 == (param_1[8]+param_1[6])/0x200 & 0xffff) {
:54727      if (*(ushort *)(param_1 + 1) < 0x28) return 0;          // fixup-offset < 0x28
:54730      else if (*(ushort *)(param_1 + 1) < 0x101) return 1;    // < 0x101 → VALID
```

`FUN_1800457f0 :54823` (USA verify) / `FUN_1800458a0 :54847` (USA apply) — same fixup
sliding-window scheme as MFT records, on INDX blocks. `FUN_180045990 :54871` — entry-chain
validator: stops at last-entry flag (`(*(ushort *)(lVar6 + 0xc) & 2) != 0`), rejects
entry_len 0, checks each entry's embedded $FILE_NAME sanity (`FUN_180045b50 :54922` =
real_size(entry+0x40) ≤ alloc_size(entry+0x38)) and that all entries share the first
entry's parent ref (:54905-07). `FUN_180045690 :54768` = entries offset (+0x18 + 0x18),
`FUN_1800456c0 :54777` = in-use size (+0x1c), `FUN_1800456f0 :54786` = is-INDX.

## 2. Raw-cluster scanner (the og "orphaned-record sweep")

`FUN_18000c890 :9452` — walks a volume range cluster-by-cluster with a claim-map gate:

```c
:9507  (**(code **)(*plVar21 + 8))(device, *(int*)(param_1+0x120)+*(param_1+0xc0), param_2, param_3);  // read chunk
:9528  uVar7 = lVar18 / (longlong)uVar17;                          // cluster#
:9529  if ((param_4 == 0) || (*param_4 == 0)) { goto LAB_18000ca0c; }  // claim/filter gate
:9685  bVar4 = FUN_18002c2e0(param_4,uVar7);                       // claim-map probe (S-012's map)
:9543  cVar5 = FUN_18003f6d0(plVar21);                             // per-cluster prefilter
:9547  iVar6 = FUN_18000b4c0(param_1,uVar7,(int *)plVar21,uVar15); // → INDX-block handler
:9550  iVar6 = FUN_1800469e0(local_b8,(int *)plVar21);             // MFT-record recognizer (small win)
:9553  uVar14 = FUN_180046980(local_b8,(int *)plVar21);            // MFT-record recognizer (big win)
:9557-69 ... if (uVar7 - *(lVar16-0x10) == 8) { *(lVar16-0x18) += 8; *(lVar16-0x10) = uVar7; }
        // coalesce records found 8 sectors apart into RUNS (engine+0x7f8/+0x800, 0x28-byte entries,
        // appended at :9611 `*(param_1+0x800) += 0x28`)
:9580-82  local_58 = 0x54004e; ... if ((int)*plVar21 != 0x52545352)  // "RSTR" $LogFile restart page
:9584-90  psVar9 = plVar21+0x1fe; ... *psVar9 != *(short*)(plVar21+0x1e)  // 55AA sector-tail overlay check
:9574  p_Var12 = FUN_18000c390(param_1,uVar7,plVar21,uVar15);      // boot/record recognizer → emits
:9651  FUN_18000abd0(param_1,plVar21,uVar15,uVar7);                // always-on recognizer (name tables)
:9653  pwVar19 = L"$STANDARD_INFORMATION";  // 13-entry attr-name table walk ($STANDARD_INFORMATION..
:9623  if (*(ulonglong *)(lVar16 + 0x18) < 500000)                 // 500K entry cap
```

Coalesced MFT-record runs (> 8 sectors) are consumed by `FUN_18000f3f0 :11459`:
`:11489 if (8 < (longlong)psVar12[2])` → `:11499 uVar13 = (psVar12[2]+8)*bps` read of the
run, then per-record parse; directory records get the $INDEX_ROOT harvest (see §4) via
the `:9951-52` site: `if ((param_7 != '\0') && (FUN_180031960(node) != 0))
FUN_18000f760(param_1, local_78, param_4);` (props built at :9915 by the standard
`FUN_180006db0`; `:9945 (flags & 0x400) → -100` grading marker).

## 3. INDX-block handler (raw space → directory registry → entry harvest)

`FUN_18000b4c0 :8586` (called only from the raw scanner :9547):

```c
:8629  bVar3 = FUN_1800456f0(param_3);                     // is-INDX?
:8638-45  // split-block reassembly: 8-sector window carry via DAT_180086d70/DAT_180086d68
:8650  bVar3 = FUN_180045d60(piVar13,uVar5,1);             // full validate (magic+USA+chain)
:8652  FUN_18000e780(param_1,(longlong)param_3,param_4,param_2);  // ENTRY HARVEST
:8653  uVar6 = FUN_180045cb0((longlong)piVar13);           // first entry's record#
:8654  if (uVar6 != 5) {                                   // (skip root's own)
:8662-75  node = {vbn, fileref(+seq), vcn..., size};        // 0x30-byte INDX-block node
:8676+    b-tree insert into map at param_1+0x150 keyed by record#
```

`FUN_18000e780 :10923` — walks the INDX entries region and builds props per entry:

```c
:10943  uVar10 = *(int*)(param_2+0x18) + 0x18;             // entries abs offset
:10944  uVar4 = *(uint *)(param_2 + 0x1c);                 // IN-USE size (+0x1c)  ← NOT slack
:10945    if (param_3 - uVar10 <= *(uint*)(param_2+0x1c)) uVar4 = param_3 - uVar10;
:10951  while ((longlong)puVar9 + 0x2aU < puVar8) {        // entry walk bounded by in-use end
:10952    uVar1 = (ushort)puVar9[1];                        // entry length (u16@entry+8)
:10954    local_68[0] = 5;
:10955    plVar5 = FUN_18000e970(uVar1,puVar9,uVar1,local_68);   // entry → CNTFSFileProperties
:10963    if ((bVar3 >> 4 & 1) == 0) {                      // NOT a directory (flags@+0x23 bit 0x10)
:10964      FUN_180022760(param_1 + 0x888,plVar5,local_68[0]);   // → cross-source collection
          } else {
:10968      pvVar7 = name; ... cVar2 = FUN_1800445e0(name);      // "$..."-name filter
:10977      plVar6 = FUN_180031070(); plVar6[6]=props; plVar6[9]=state; plVar6[10]=rec; plVar6[0xb]=cluster#;
:10982      FUN_18000f2b0(param_1,plVar6,'\x01','\x01','\0');    // EMIT directory node
```

**No slack read**: the walk ends at `entries_off + in_use_size` (:10943-45); the
validator's chain walk also stops at the last-entry flag (§1). Deleted entries that
remain in INDX slack (between in-use end and allocated end) are NOT harvested.

## 4. Entry → props builder (both INDX and $INDEX_ROOT paths)

`FUN_18000e970 :11002` — INDEX_ENTRY → CNTFSFileProperties from the embedded $FILE_NAME:

```c
:11025  if (param_3 < 0x54) return 0;                       // entry ≥ 0x54 bytes
:11028  uVar3 = param_2[9];                                 // entry+0x48 = $FILE_NAME flags+reparse
:11029  if ((uVar3 & 0x10000020) == 0) return 0;            // require DIRECTORY(0x10000000) or ARCHIVE(0x20)
:11032  if (((byte)uVar3 & 6) == 6) return 0;               // reject HIDDEN|SYSTEM both set
:11035  bVar1 = (byte)param_2[10];                          // entry+0x50 = name length (wchars)
:11036  if (param_3 < bVar1*2 + 0x52) return 0;
:11040  local_b8 = *param_2 >> 0x30;  local_c0 = *param_2 & 0xffffffffffff;  // fileref seq/record#
:11044  local_c8 = (param_2[8] << 0x10) >> 0x10;            // entry+0x40 = real size (48-bit)
:11046  if ((uVar3 >> 0x1c & 1) != 0) local_d4 = 0x10;      // FOLDER attribute bit
:11049  name at entry+0x52 (UTF-16)
```

Times come from entry+0x18/+0x20/+0x30 (created/modified/accessed of the embedded
$FILE_NAME), parent ref from entry+0x10.

The SAME builder is fed from **$INDEX_ROOT of MFT records** — `FUN_18000f760 :11609`:

```c
:11631  lVar5 = FUN_1800430b0(param_2,0x90,0,NULL,NULL);    // find $INDEX_ROOT
:11632  if ((lVar5 != 0) && (lVar6 = FUN_1800460e0(lVar5), lVar6 != 0)) {
:11633    ... *(char *)(lVar6 + 0x1c) != '\x01'              // large-index flag must be 0 (resident entries only)
:11633    ... local_64 = *(uint *)(lVar6 + 0x14); local_64 < 0x321   // entries ≤ 0x320 bytes
:11636    uVar7 = FUN_180046060(lVar5,&local_68,&local_64);  // entries offset
:11642-83  // IDENTICAL entry walk: FUN_18000e970 → files: FUN_180022760(+0x888) / dirs: FUN_18000f2b0 emit
```

## 5. Register/emit plumbing

- `FUN_180022760 :27420` — inserts {record ref, state, size(props+0x1b), times, name}
  into the map at engine+0x888+0x28 (FUN_180024ea0/FUN_180024f60), keyed by record ref.
- `FUN_18000f2b0 :11410` — dedup via `FUN_180023350(engine+0x888,node,state)` (:11426),
  cluster-window state bit on props+0x72 (:11419-25, window = engine+0x838 set by the raw
  scanner at :9570-71), final insert `FUN_1800224d0(engine+0x888,...)` (:11451).
- The +0x888 collection is aliased to the engine's +0x140 map (`:7067
  *(longlong *)(param_1 + 0x888) = param_1 + 0x140;`), consulted by the standalone
  post-walk merger `FUN_180010c00` (`:12411 FUN_180022aa0(param_1+0x888, lVar18)`), which
  builds CNTFSFileProperties per collected entry (name/times/record#, `:12595-12624`) and
  wraps them into nodes via FUN_180031070 — the same wrapper shape the USN-harvest
  consumer uses (see s3-ntfs-usn-gate.md §4). Drain: `:7778-79
  FUN_180022130(param_1+0x888); FUN_180022910(param_1+0x888, param_1+0x770, ...)`.

## 6. Corpus + core cross-references

- Corpus doc (strength 3): `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md:108`
  — full-scan strategy = "full walk + $Bitmap volume bitmap build … + USN journal
  cross-check … + **orphaned-record sweep** + attribute-stream assembly". The INDX
  machinery above is that sweep's directory-entry half; the corpus never decoded it.
- Strings probe (strength 4): the og binary's attribute-name table
  (`re-workspace/reversed/NTFSSearchDLL.dll/strings_utf16.txt:10-22`) includes
  $INDEX_ROOT/$INDEX_ALLOCATION/$EA_INFORMATION/$LOGGED_UTILITY_STREAM (13 names).
- Core (audited `34b3441`): NO INDX/INDEX_ROOT decode anywhere —
  `include/cr/ntfs/cr_ntfs_wire.h:192-195`: "no INDX-block walker exists and none is
  planned. The speculative INDX wire family (magic, IndexHeader, IndexEntryHeader,
  IndxBlockHeader) was deleted (plan ruling D-N, AUDIT-NTFS §4 row 9: 0 refs; og INDX
  walking itself is UNVERIFIED-7 in the corpus)."
  `src/core/ntfs/cr_mftwalk.cpp` build_props switch handles only
  ATTRIBUTE_LIST/FILE_NAME/STANDARD_INFORMATION/DATA/INDEX_ALLOCATION-extents; the
  remnant path (B6) parses $DATA runlists only (post-7797bd6); no $INDEX_ROOT entry
  harvest. `rg -i indx src/ include/` → only the wire-header comment above.

## 7. What is NOT claimed

- INDX **slack** (deleted-entry remnants between in-use end and allocated end) is NOT
  harvested by the og (walk bounded by in-use size, §3; chain stops at last-entry flag, §1).
- The exact volume ranges/vt-slot wiring that invoke FUN_18000c890 (and whether the
  claim-map gate skips claimed clusters entirely) is not decoded — affects impact
  quantification only, not the existence of the harvest.
- The final result-tree placement of +0x888 entries (lost-tree vs merge-into-structured)
  is evidenced at the FUN_180010c00 consumer shape level, not fully traced to the emit
  callback.
