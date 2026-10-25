# s3-ntfs-usn-gate — og $UsnJrnl harvest reason-gate + bounds (first-hand decompile read, S3-NTFS2)

All quotes from `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
(corpus `03ad46c`). Function index: `NTFSSearchDLL_functions.txt` (FUN names = Ghidra
auto-names; addresses in the 18001xxxx range are file-offset-stable per prior sessions'
citations). Core side from `src/core/ntfs/` @ `34b3441`.

## 1. The og's per-record harvest gate — `reason & 0x200` + a dead harvest-all flag

`FUN_1800196d0` (USN record → evidence entry), `:20056`:

```c
:20076  local_res18 = (undefined8 *)FUN_18002be40(0x38);      // evidence struct, 7 qwords
:20077  pWVar1 = (LPCWSTR)(param_2 + 0x3c);                  // USN_RECORD_V2.FileName
:20079  *(undefined4 *)(local_res18 + 4) = *(undefined4 *)(param_2 + 0x34);  // FileAttributes
:20080  *local_res18 = *(undefined8 *)(param_2 + 8);          // FileReferenceNumber
:20081  local_res18[1] = *(undefined8 *)(param_2 + 0x10);     // ParentFileReferenceNumber
:20082  *(undefined4 *)(local_res18 + 2) = *(undefined4 *)(param_2 + 0x28); // Reason (u32)
:20083  local_res18[3] = *(undefined8 *)(param_2 + 0x20);     // TimeStamp
:20088  pCVar4 = FUN_1800066f0(pWVar1,(ulonglong)(*(ushort *)(param_2 + 0x38) >> 1),plVar6);
        //                                        ^ FileNameLength >> 1, UTF-16 → ANSI convert
:20098  if ((*(char *)(param_1 + 0x80) != '\0') || ((*(uint *)(param_2 + 0x28) & 0x200) != 0)) {
:20104    if ((*(byte *)(local_res18 + 4) & 0x10) != 0) {      // FILE_ATTRIBUTE_DIRECTORY
:20108        ... FUN_1800151c0((undefined8 *)(param_1 + 0x68), ... ) // 2nd map: dir refs
:20111    pvVar3 = *(void **)(param_1 + 0x48);                 // results vector push
:20116      *puVar7 = puVar8;                                  // entry appended
:20123  free((void *)local_res18[5]);
:20125  free(local_res18);                                     // gate failed → entry dropped
```

Offsets used (+0x08 fileref, +0x10 parent, +0x20 ts, **+0x28 Reason**, +0x34 attrs,
+0x38 namelen, +0x3c name) are exactly the `USN_RECORD_V2` layout, so `param_2+0x28`
IS the Reason dword. **The og harvest gate is `reason & 0x200`.**

## 2. The harvest-all byte (+0x80) is write-once-zero (dead code)

Ctor `FUN_180018570` (harvester object init), `:19349`:

```c
:19371  *(undefined1 *)(param_1 + 0x10) = 0;   // param_1 is undefined8* → byte at object+0x80 = 0
:19375  *(undefined4 *)(param_1 + 2) = 8;      // +0x10 cluster size default 8
:19376  uVar2 = FUN_18002be40(0x100000);       // +0x58: 1 MiB chunk-carry buffer
```

`rg '\+ 0x80\) = '` over the whole decompile finds only three writes — `:459`
(`undefined8`, =0xf), `:31504`/`:31802` (`undefined4`) — all to other classes, none to
the harvester. `rg 'char \*\)\([a-zA-Z0-9_]+ \+ 0x80\)'` finds exactly ONE hit: the
gate read at `:20098`. The embedded-path initializer `FUN_180018740` (:19422, called at
engine site `:757`) writes `*param_1`, `param_1[1]` (+0x08 device), `param_1[0xf]`
(+0x78) and `+0x10` cluster size — never +0x80. The standalone initializer
`FUN_180019a20` (:20217, called at `:12589`) writes +0x08/+0x78/+0x10 — never +0x80.
**No setter exists; the harvest-all flag is permanently 0.**

## 3. The og's record iteration — no version check, 512-byte record cap, 4-byte zero skip

`FUN_180019570` (chunk → records), `:19993`:

```c
:20023      if (0x200 < *puVar1) break;          // RecordLength > 0x200 → ABANDON rest of chunk
:20024      if (*puVar1 == 0) { sVar4 = sVar4 + 4; iVar6 = (int)sVar4; }   // zero word: skip 4
:20029        FUN_1800196d0(param_1,(longlong)puVar1);
:20030        sVar4 = *puVar1 + sVar4;           // advance by RecordLength
:20036    memset(*(void **)(param_1 + 0x58),0,0x100000);
:20040      *(size_t *)(param_1 + 0x60) = sVar4; // trailing partial → 1 MiB carry buffer
```

- **No MajorVersion/MinorVersion validation** — V2 layout offsets applied to anything.
- **`RecordLength > 0x200` (512) breaks the scan** for the remainder of the chunk. A
  maximal NTFS name (255 chars) yields a V2 record of `0x3C + 2*255 = 570 > 512` —
  names ≥ 227 chars stall the og's chunk parse. (Split-at-chunk-tail records are
  completed via the carry path, which allows up to `0x100000` bytes, `:20013`.)
- Chunk size: `FUN_1800193c0 :19938` `_Dst = FUN_18002be40(0xa00064)` and
  `:19941 lVar10 = 0xa00000 / bytes_per_sector` — 10 MiB chunks, whole-$J runlist walk,
  **no total-size cap**.

## 4. Harvest wiring (both engine paths)

- Locator `FUN_1800190d0` (:19805): builds props for an $Extend-area record via
  `FUN_180006db0`, requires `strcmp(name,"$UsnJrnl")==0` (`:19868`), iterates `$DATA`
  attrs 0..9, requires the named stream `L'$' (0x24) + L'J' (0x4a)` (`:19879-80`),
  stores props at harvester+0x18 (`:19895`).
- Embedded path: engine ctor `FUN_180015eb0 :17452` embeds the harvester
  (`FUN_180018570(param_1 + 0x52)`); scan site `:757-761` inits it
  (`FUN_180018740(param_1+0x318, ...)`) and runs the walk when `+0x330` (= harvester+0x18
  props) non-NULL: `FUN_1800193c0(param_1 + 0x318)`.
- Standalone path: `FUN_180010c00 :12588` builds a LOCAL harvester
  (`FUN_180018570(local_d8)`), `:12589` `FUN_180019a20(local_d8, device, ..., +0xc, +0x10)`,
  `:12593` `FUN_1800193c0(local_d8)`, then `:12595+` turns each harvested entry into a
  `CNTFSFileProperties` (name via vt+8 from the ANSI string `puVar4[5]`, times from
  `puVar4[3]`, record# from `*puVar4 & 0xffffffffffff`, `:12603-20`) and inserts it into
  a record-keyed collection (`FUN_180031070` wrapper, `:12622-24`).

## 5. Core side @ 34b3441

- `include/cr/ntfs/cr_mftwalk.h:61-65`:
  `constexpr u32 USN_REASON_FILE_DELETE = 0x00020000;` (comment: "winioctl.h USN_REASON_*").
- `src/core/ntfs/cr_mftwalk.cpp:1048` gate: `if ((reason & USN_REASON_FILE_DELETE) != 0 && namelen > 0 && namelen <= 512 && nameoff >= name_off && nameoff + namelen <= rlen && (namelen & 1) == 0)` — no 512-byte rlen cap, V2+V3 validated (`:1020-1023`, `:1032-1038`), `$`-names skipped (`:1054`), UTF-16 preserved, dedup vs structured results (`:1060-1081`), emitted under root_lost as MetadataOnly/Deleted (`:1091-1096`).
- Bounds: `USN_HARVEST_CAP = 64 MiB` (`:24`, applied `:980-981`) — first 64 MiB of the
  stream in VCN order; `USN_RESYNC_LIMIT = 1<<20` (`:25`), 8-byte resync (`:1026-1029`).
- Fixture: `tests/unit/test_ntfs_volume.cpp:30-33` keeps
  `USN_REASON_FILE_CREATE = 0x00000100` **fixture-only**; `:1074-1075` feeds
  `0x100 | 0x20000 = 0x20100` for the harvested record.

## 6. winioctl.h semantics (interpretation, not corpus evidence)

No Windows SDK copy exists in this environment; values recalled from the public SDK
table (stable since Win2K): `USN_REASON_HARD_LINK_CHANGE=0x00000100`,
`USN_REASON_COMPRESSION_CHANGE=0x00000200`, `USN_REASON_FILE_CREATE=0x00010000`,
`USN_REASON_FILE_DELETE=0x00020000`, `USN_REASON_CLOSE=0x80000000`. Under this table the
og's mask 0x200 is COMPRESSION_CHANGE (not FILE_DELETE), and a real Windows deletion
record carries `0x80020000` (FILE_DELETE|CLOSE) — which the og's gate does NOT match.
The core's 0x20000 IS FILE_DELETE. The dead agent's probe
(`evidence/executions/s3-ntfs-usn-reason/`) inverted this premise (its "realdelete"
record 0x80000200 is CLOSE|COMPRESSION_CHANGE; its "fixture" record 0x20100 contains
the true FILE_DELETE bit) — the probe's executed output remains valid evidence of CORE
behavior, with the interpretation corrected by this file.
