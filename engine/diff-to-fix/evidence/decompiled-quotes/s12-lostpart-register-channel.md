# Evidence — M-118: the three-source lost-partition candidate assembly (mbr / filesystem / register)

Session-12 continuation lane, 2026-10-29. Corpus `be0cfe9`. Every quote below was
read first-hand this session.

## 1. The real-run observation — `og-codebase/log/DRWUI.log:637-643`

```
01:21:27:080 add lost partition from mbr start=2048, total=122277823
01:21:44:469 i64CountofSectors - i64RealReadSector = 10175
01:21:44:492 add lost partition from filesystem start=2048, total=122277823
01:21:44:661 add lost partition from register start=2048, total=122277823
01:21:44:676 add lost partition from register start=2048, total=122277823
01:21:44:692 add lost partition from register start=2048, total=122277823
01:21:44:705 Insert lost partition e:\drw\_drw_main\drwintelligentscan\recoverycontroller\genericrecoverycontroller.cpp, 162, StartSector=2048, TotalSector=122277823
01:21:44:723 Found lost partition(StartSector:2048, TotalSector:122277823, FileSystem:NTFS)
```

Three sources, one partition; the register source contributes three records
(historical mount records) that the downstream dedup collapses. (The
`i64CountofSectors - i64RealReadSector` line interleaved is the M-119 clamp
diagnostic — see `s12-partition-read-clamp.md`.)

## 2. The assembly function — DRWUI `FUN_140588820`
(`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:921820-921890`)

```c
void FUN_140588820(longlong *param_1,undefined8 param_2)
{
  ...
  if ((*(char *)(*param_1 + 0x6ec) != '\0') || (*(char *)((longlong)param_1 + 0x29) != '\0')) {
    lVar1 = FUN_140589070();                       /* source 1: the table walker   */
    if (lVar1 != 0) { FUN_140589070(param_1,local_108,lVar1); }
    lVar1 = FUN_140286ca0(local_108);
    lVar2 = FUN_140286c80(local_108);
    for (; lVar1 != lVar2; lVar1 = lVar1 + 0xa8) { /* candidates at 0xa8 stride   */
      FUN_140281e80(local_d8,lVar1);
      FUN_140162e00("add lost partition from mbr start=%I64d, total=%I64d\r\n",
                    local_d0,local_c8);
      FUN_140286cc0(param_2,local_d8);             /* append to the container      */
      FUN_1402753f0(local_d8);
    }
  }
  if (*param_1 != 0) { FUN_140588ba0(param_1,local_108); }   /* source 2: the FS probe */
  ...
  for (; lVar1 != lVar2; lVar1 = lVar1 + 0xa8) {
    FUN_140281e80(local_d8,lVar1);
    FUN_140162e00("add lost partition from filesystem start=%I64d, total=%I64d\r\n",
                  local_d0, local_c8);
    FUN_1402753f0(local_d8);
  }
  FUN_140588a50(&local_f0,param_2);
  FUN_1405895d0(param_1,local_108);                 /* source 3: the register walk  */
  ...
  for (; lVar1 != lVar2; lVar1 = lVar1 + 0xa8) {
    FUN_140281e80(local_d8,lVar1);
    FUN_140162e00("add lost partition from register start=%I64d, total=%I64d\r\n",
                  local_d0,local_c8);
    FUN_1402753f0(local_d8);
  }
  ...
}
```

Gates: the mbr wave requires either flag at `*param_1+0x6ec` or `param_1+0x29`
(scan-option bits); the filesystem wave requires `*param_1 != 0`; the register
wave runs unconditionally after them.

## 3. The register walk, engine-side — PartitionRecoveryD
(`re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:8930-8965`)

```c
LVar3 = RegOpenKeyA((HKEY)0xffffffff80000001,      /* HKEY_CURRENT_USER            */
        "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2\\CPC\\Volume"
        ,&local_180);
if ((LVar3 == 0) && (local_180 != (HKEY)0x0)) {
  for (local_188 = 0; local_188 < 1000; local_188 = local_188 + 1) {   /* ≤1000 keys */
    memset(local_128,0,0x104);
    LVar2 = RegEnumKeyA(local_180,(DWORD)puVar9,local_128,0x104);
    if (LVar2 != 0) break;
    local_178 = FUN_18000f5b0(param_1,param_2,local_180,local_128);   /* per-record  */
    if (local_178 != 0) {
      ...  /* insertion into the candidate container (FUN_1800108f0 + dedup splice) */
    }
  }
  RegCloseKey(local_180);
}
```

(The same key + enumeration shape exists DRWUI-side at
`DRWUI_decompiled.c:922335+` (FUN_1405895d0) — the walk is implemented in BOTH
the engine module and the UI layer.)

## 4. The per-record decode — PartitionRecoveryD `FUN_18000f5b0`

```c
void FUN_18000f5b0(undefined8 param_1,int param_2,HKEY param_3,LPCSTR param_4)
{
  ...
  local_d8 = (LPBYTE)thunk_FUN_180026e64(0x8000);          /* 32 KiB buffer          */
  LVar1 = RegOpenKeyA(param_3,param_4,&local_c8);           /* the {GUID} subkey      */
  if ((LVar1 == 0) && (local_c8 != (HKEY)0x0)) {
    local_cc = 0; local_d0 = 0x8000;
    LVar1 = RegQueryValueExA(local_c8,"Data",(LPDWORD)0x0,&local_cc,local_d8,&local_d0);
    if ((((LVar1 == 0) && ((local_cc == 3 && (0xe7 < local_d0)))) &&   /* REG_BINARY, >231 B */
         (local_d8[0xc4] == '#')) && (local_d8[0xe6] == '#')) {        /* field delimiters     */
      memcpy(local_88,local_d8 + 0xc6,0x20);                /* 32 ASCII-hex chars     */
      ...
      FUN_180010b70(local_88,L"%I64X",local_c0,lpType);     /* hex -> u64 byte start  */
      local_a0 = (ulonglong)param_2;                        /* bytes per sector       */
      if (local_c0[0] % local_a0 == 0) {
        local_c0[2] = local_c0[0] / local_a0;               /* sector conversion      */
      }
    }
    RegCloseKey(local_c8);
  }
  ...
}
```

The `Data` value under each `MountPoints2\CPC\Volume\{GUID}` subkey is the shell's
volume cache blob; the '#' pair at offsets 0xc4/0xe6 delimits a 32-char hex field
at +0xc6 encoding the volume's byte placement; the walk sector-aligns it and feeds
the candidate container. The sector-TOTAL derivation rides the same parse (the
fix-time probe in M-118's plan pins the exact second output of FUN_180010b70).

## 5. The core side (absence, grep-verified at `c7fa527c`)

- `grep -rn "MountPoints\|CPC" src/ include/` — zero hits.
- `src/core/lost/cr_candidates.cpp` — the discovery implements the windowed
  byte-scan + probes (the filesystem channel) only.
- `src/core/reghive/` — offline-hive readers (FVEAutoUnlock harvest, offline-OS
  profile); no live-HKCU walk, no MountPoints2 target.
- S-051's fix plan step 1 builds whole-disk targets "from the partition table"
  (channel 1) with the discovery as the extra (channel 2) — channel 3 unnamed.
