# S42 evidence — the continuation-audit lane's first-hand decode pack

- **Task ID:** S42-B (session-42 continuation audit lane, the deep-comparison waves; "B" per the merge with the parallel S42-main window — this lane's filings renumbered at the push reconciliation: N-154→N-158, N-155→N-159, N-156→N-160, I-035→I-037; the DBX finding stayed M-117 per SA-9 [the parallel window's fix-under-M-117 pushed first])
- **S42-B merge note:** the parallel S42-main window (487949a4/4f8fa661 lineage) landed S-048/M-117-dbx/N-125/N-126/N-135/N-148/N-152 + the INFO-tier restoration AFTER this lane's 869da779 pin — the liveness battery's STILL verdicts are pinned-truth (the retirements ride the remote's own lanes, the S41 precedent). Their W5 bad-sector registration is convergent with this lane's I-037 item (b) — the 0x28000-sector toast and their "80 MiB gate" are the same threshold in different units.
- **Date:** 2026-10-29
- **Core HEAD:** `869da779` · **Corpus HEAD:** `be0cfe9`
- **Method:** every quote below was read first-hand this session (`sed -n` over the
  line-addressed decompiles; the binary label resolutions via a PE section-table
  parse). The lane's parallel agents (A: config deep-diff; B: folder census;
  C: DRWUI never-walked hunt; D: core currency; E: perf evidence) produced the
  candidate lists; the load-bearing claims were re-verified first-hand before any
  finding was filed — the SOP.

## §1 The OfficeTagFromXML five-branch ZIP typing walk (N-154's evidence)

`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`, the per-entry
typing walk (read `sed -n '53025,53110p'`):

```c
piVar18 = _Str1;
iVar3 = FUN_180045110((longlong)param_1,(longlong)local_10c0,param_5,_Str1,(longlong)param_1);
if (iVar3 != 0) {
    bVar2 = false;
    *(undefined4 *)param_5 = 0x86;                                   // XMIND
    puVar7 = (ushort *)FUN_18000f4b0((longlong)&local_1088,L"XMIND");
    FUN_18000ed00((longlong)puVar6,puVar7);
}
...
FUN_1800071e0((longlong)&local_1088,"xsd/MindManagerApplication.xsd",0x1e);
...  memcmp pair ...
if (local_10c8 != 0) {
    bVar2 = false;
    *(undefined4 *)param_5 = 0x87;                                   // MMAP
    puVar7 = (ushort *)FUN_18000f4b0((longlong)&local_1088,L"MMAP");
    FUN_18000ed00((longlong)puVar6,puVar7);
}
iVar3 = strcmp((char *)_Str1,"rels/_rels.xml");
if (iVar3 == 0) {
LAB_180045ef8:
    bVar2 = false;
    *(undefined4 *)param_5 = 0x8c;                                   // EDDX
    puVar7 = (ushort *)FUN_18000f4b0((longlong)&local_1088,L"EDDX");
    FUN_18000ed00((longlong)puVar6,puVar7);
}
else { iVar3 = strcmp((char *)_Str1,"rels/page1_rels.xml"); if (iVar3 == 0) goto LAB_180045ef8; }

iVar3 = strcmp((char *)_Str1,"ProjectFolder/Anon/");
if (iVar3 == 0) {
LAB_180045f4c:
    bVar2 = false;
    *(undefined4 *)param_5 = 0x8d;                                   // WFP
    puVar7 = (ushort *)FUN_18000f4b0((longlong)&local_1088,&DAT_18006d940);
    FUN_18000ed00((longlong)puVar6,puVar7);
}
else { iVar3 = strcmp((char *)_Str1,"ProjectFolder/project_info.json"); if (iVar3 == 0) goto LAB_180045f4c; }

iVar3 = strcmp((char *)_Str1,"MediaPool/Master/MpFolder.xml");
if (iVar3 == 0) {
    *(undefined4 *)param_5 = 0x91;                                   // DRP
    puVar7 = (ushort *)FUN_18000f4b0((longlong)&local_1088,&DAT_180067c88);
    FUN_18000ed00((longlong)puVar6,puVar7);
}
```

**The label constants, binary-resolved this session** (PE parse of
`og-codebase/RAWSearchDLL.dll`: image base 0x180000000, .rdata VA 0x66000, file
offset 0x64600):

```
DAT_18006d940  RVA 0x6d940  bytes 57 00 46 00 50 00 00 00  utf16-le: 'WFP'
DAT_180067c88  RVA 0x67c88  bytes 44 00 52 00 50 00 00 00  utf16-le: 'DRP'
```

**Core absence (grep executed this session):**
`grep -ri 'xmind|mindmanager|projectfolder|mediapool|page1_rels' src/core/carve/ include/cr/carve/`
→ 0 hits. **The toggle coupling:** `og-codebase/config/RawConfig.ini` —
`XMIND=1` (:12), `MMAP=1` (:15), `EDDX=1` (:17), `WFP=1` (:18), `DRP=1` (:22).

## §2 The failed-files HTML report (N-155's evidence)

`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (read `sed -n
'642655,642700p'` + `sed -n '642755,642825p'`):

```c
undefined8 * FUN_1403c6da0(undefined8 *param_1)          // CSaveFailedFilesWithHtml ctor
{
  FUN_1401f5fe0(param_1);
  *param_1 = CSaveFailedFilesWithHtml::vftable;
  FUN_14006da40(param_1 + 1);
  FUN_14006da40(param_1 + 5);
  *(undefined4 *)(param_1 + 9) = 1;
  param_1[0xb] = 0xffffffffffffffff;
  *(undefined4 *)(param_1 + 0xc) = 1;
  uVar1 = thunk_FUN_140053400(2000);                     // the 2000-byte content buffer
  param_1[10] = uVar1;
  return param_1;
}

void FUN_1403c6fc0(...)                                  // the per-failed-file HTML row
{
  ...
  local_40 = FUN_14008a650(param_3);                     // custom raw-category name
  local_54 = FUN_1400d3ee0(local_48);                    // the node's group
  local_58 = FUN_14008b040(local_48);                    // the node's kind
  if (local_40 == 0) {
    if (local_54 == 1) {
      ...
      FUN_140081bf0(local_30,&DAT_1414a5804);
      FUN_140081bf0(local_30,"Lost Files");
      FUN_140081bf0(local_30,&DAT_1414a5a5c);
      if (local_58 == 7) {
        FUN_140081bf0(local_30,"Reconstructed");
        FUN_140081bf0(local_30,&DAT_1414a5a44);
      }
      else if (local_58 == 6) {
        FUN_140081bf0(local_30,"Tags Extracted");
        FUN_140081bf0(local_30,&DAT_1414a59f4);
      }
    }
    else if (local_54 == 0) {
      ...
      if (local_58 == 4) {
        FUN_140081bf0(local_30,&DAT_1414a5a0c);
        FUN_140081bf0(local_30,"Existing Files");
        FUN_140081bf0(local_30,&DAT_1414a59cc);
      }
      else if (local_58 == 0) {
        FUN_140081bf0(local_30,&DAT_1414a5984);
        FUN_140081bf0(local_30,"Lost Files");
        FUN_140081bf0(local_30,&DAT_1414a5914);
        FUN_140081bf0(local_30,"Deleted");
        FUN_140081bf0(local_30,&DAT_1414a58c4);
      }
    }
  }
  else {
    FUN_140081bf0(local_30,local_40);                    // the custom raw-category name
    FUN_140081bf0(local_30,&DAT_1414a5814);
  }
  FUN_140056390(param_2,local_30);
  ...
}
```

The classification mapping — group 1 → "Lost Files" (+ "Reconstructed" at kind 7 /
"Tags Extracted" at kind 6); group 0 → "Existing Files" at kind 4 / "Lost Files" +
"Deleted" at kind 0; else the custom raw-category name — is the per-file
tree-classification axis N-155 files. The register claims refuted:
`docs/REFERENCE-API-GAP.md:104` (R-58: "Per-run **text log** …") and
`diff-to-fix/API-DELTA.md` row 21 ("og format NOT IN CORPUS") — the generator and
its row grammar ARE corpus-decodable. Vtables (from
`reversed/ui-resources/drwwui_vtable_map.json`): ISaveFailedFiles 0x141886800,
CSaveFailedFilesWithHtml 0x14188a078, CloudRecover::CloudFaildFilesWithHtml
0x141886930; RTTI names in `re-workspace/rtti_classes.json`.

## §3 SearchResult.txt — the QA-gated full-result-list dump (I-035 item a)

`DRWUI_decompiled.c` (read `sed -n '388960,389045p'`):

```c
FUN_140235a40(local_218,"%s/Config.dat",uVar4);
UVar2 = GetPrivateProfileIntA("register","SearchResult",0,local_218);   // QA gate, default 0
if (UVar2 != 0) {
    FUN_140072740(local_218,"%s/log/SearchResult.txt",uVar4);
    local_368 = fopen(local_218,"wb");
    ...
    local_2e0 = param_1 + 0x40;                       // the existing-files map
    FUN_1401d8500(local_2e0,&local_360);
    while( true ) {
        ...
        fwrite(local_2c8,1,local_2d0,local_368);       // the full path string
        ...
        cVar1 = FUN_140088fd0(local_358);
        local_320 = cVar1 ? ",folder:1" : ",folder:0";
        fwrite(local_320,1,local_348,local_368);
        if ((*(ulonglong *)(local_358 + 0x89) >> 1 & 1) == 0) {
            local_310 = ",deleted:0\r\n";
        } else {
            local_310 = ",deleted:1\r\n";              // node+0x89 bit 1
        }
        fwrite(local_310,1,local_340,local_368);
    }
    local_2b0 = param_1 + 0x58;                       // the lost-files map (second pass)
    ...
```

## §4 The DRWUI user-mode bad-sector skip policy (I-035 item b)

`DRWUI_decompiled.c` (reads `sed -n '179880,179900p'` + the constant greps over
`179912,180075`):

```c
// the trigger, in FUN_1400cb580 (the \\.\PhysicalDrive read wrapper):
LAB_1400cb773:
  *(undefined1 *)(param_1 + 0xd8) = 1;
  if ((iVar5 == 2) && (0xb < *(int *)(param_1 + 0xa8))) {      // 12+ bad reads
    iVar5 = FUN_1400cb7e0(param_1,param_2,uVar9,param_7);      // engage the skip routine
    ...
```

```c
// FUN_1400cb7e0 — the constants:
LogInfo("Start skip bad sectors");
uVar10 = 0x200000;                                        // initial chunk 2 MiB
uVar9  = 0x200000 / (ulonglong)(uint)param_1[0xc];
local_88 = (void *)FUN_140053400(0x4000000, ...);         // 64 MiB buffer
LogInfo("Skip bad sector step:%d");
if (local_94 == 0x163) { ... CancelIo(local_78); LogInfo("bad sector:%I64d, count:%d"); }
if (0x28000 < param_1[0x14]) { LogInfo("Send bad sector message."); ... }   // 163,840-sector toast
(uVar3 = (uint)uVar10 * 2, uVar10 = (ulonglong)uVar3, 0x4000000 < uVar3) { uVar10 = 0x4000000; }  // doubling, 64 MiB cap
if (0x40000000 < local_68) { LogInfo("Break skip bad sectors, ReadSize = %I64d"); }  // 1 GiB cap
LogInfo("Skip bad sectors Finish.");
```

The cached bad-range short-circuit (`FUN_1400cc800` :180578-180600) walks the
`DAT_1425e71b0` known-bad list and returns 0x163 **without issuing the read**.
The sector map is a file-backed 1-bit-per-sector bitmap (`FUN_1404ec520`
:819998-820059: `CreateFileW` + `DeviceIoControl 0x900c4` + `SetEndOfFile` +
`CreateFileMappingW` + `MapViewOfFile`). The direct-open sites:
`\\\\.\\PHYSICALDRIVE%d` at :176835/:176879/:177099/:177642/:178061/:178805.

## §5 The register-truth verification transcripts (N-156's evidence)

```
$ find . -name 's41_ogfile_census.py' -o -name 's41_never_cited_census.sh'   → 0 hits
$ grep -n '750' diff-to-fix/findings/INFORMATIONAL/I-034-never-cited-artifact-register.md
  :6  "the og-codebase file census (750 entries, top+1 deep)"
  :46 "Executed this session: ... (script: `scripts/s41_never_cited_census.sh` ...)
        + the og-file census (`scripts/s41_ogfile_census.py`, 750 entries ...)"
$ grep -n 'OQ-S10-1' diff-to-fix/findings/FIXED/N-129-...md
  :15 "the 2 exceptions are N-130/OQ-S10-1"      # the sfc exception was OQ-S10-2
$ git log --format='%h %ci %s' -1 0f5d57fb   → 2026-10-28 19:17:34 +0000  (suppress_overlap → M-117)
$ git log --format='%h %ci %s' -1 9cc6a03e    → 2026-10-28 19:20:03 +0000  (S41: "M-117 ... confirmed free")
$ grep -n 'M-117' diff-to-fix/FIXED-LEDGER.md → :436 (the retired suppress_overlap row keeps the ID)
```

The og-codebase recursive census (the S42 folder-census agent's walk, the
load-bearing counts re-executed in the lane's own shell): 1,538 entries total
(68 dirs + 1,470 files); depth ≤1 = 555; **files at depth ≤2 = 750**; ~720 files
at depth ≥3 (the diskhealth QML tree + BUILDPE nested payload) rest on the
wholesale family lines — every one re-verified dispositioned by the S42 walk.

## §6 The doc-drift verification transcripts (the N-150 addendum's evidence)

```
$ grep -n 'CR_UI_ERR_FVE_LOCKED' include/cr/cr_ui.h
  :328/:407/:418/:421  (comments only — never #defined; the real name is ERR_FVE_LOCKED, cr_error.h:72)
$ grep -n '32 MiB' docs/ABI-CONTRACT.md      → :616  "max_read 32 MiB (M-080)"   # M-109 re-pinned 200 MB
$ grep -n 'path_substring\|state_mask' include/cr/cr_ui.h   → :1026/:1028        # M-072 landed
$ grep -n 'ERR_DEST_PATH_TOO_LONG' include/cr/cr_error.h    → :67               # M-074 landed
$ grep -n '1,014' docs/testing.md            → :4                                    # actual: 1,199 TEST/TEST_F
$ grep -n '81 export\|78 export' docs/API.md docs/CORE-API-CURRENT.md
  API.md:92 "81 exported functions"; CORE-API-CURRENT.md:12 "78 exports" vs :38 "83 exports"
$ sed -n '1395,1425p' src/app/rrecov.cpp     → --label parsed at :1398 AND dead-duplicated at :1411
$ grep -c 'CR_UI_API' include/cr/cr_ui.h     → 86 (−3 #defines = 83 exports; ABI v3 at :81)
```
