# s31 — OQ-RS resolved: the ReFS residual probes (RS-1..RS-8)

> Session DFX31, corpus `03ad46c` unmoved. Quotes first-hand from
> `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` and
> `re-workspace/reversed/ghidra-mo/RefsFileSystemAnalyser_decompiled.c`.

## RS-5 — the .mo ReFS analyser is structural-only (no deleted harvest)

`RefsFileSystemAnalyser_decompiled.c` (42,013 lines): grep for
`IsDeleted|deleted|Deleted|DEL` → **0 hits** (re-run this session). The
RefsProc class family is the structural analysis layer —
`CRefsChildMsbDupProc::ModifyMsbProc` (FUN_18001a090, :16705-16738):

```c
  if ((char)param_1[10] == '\0') {
    if ((param_1[0x1f] == 0) ||
       (((local_18 = (**(code **)(*(longlong *)param_1[0x1f] + 0x118))(param_1[0x1f],param_2),
         local_18 != '\0' && (param_1[4] == 0)) && ((char)param_1[10] == '\0')))) {
      ... FUN_18002e550(param_2,param_1,param_3,param_4); ...
    }
    if (local_18 == '\0') {
      FUN_1800119e0(L"[CRefsChildMsbDupProc] %s fail!",
                    L"RefsProc::CRefsChildMsbDupProc::ModifyMsbProc",param_3,param_4);
```

— an MSB-page MODIFICATION chain (the fix/analyse pipeline), with the
sibling `CRefsBasicMsbProc::ModifyMsbProc` at :16680-16704. The class
census (RefsFileSystemAnalyser.md): LogFileProc/MetaDirectory/FileProc/
ChildMsbProc/VolumeInfoProc/Meta*DataRun/DirItem/SymbolicDirItem/
FileItemEntry/VolumeAnalyser/BitmapProc/ContainerTable — the structural
object model. **Verdict: the .mo layer (IFSAnalyser role) has no
deleted-file harvest; the deleted domain belongs to ReFSSearchDLL alone.
The core's deleted surfacing is NOT double-counted against a
partition-side og harvest — parity-safe (I-009's registration stands).**

## RS-6 — CRefsAppDataRunItem is a data-run item, not ADS recovery

The hierarchy (`RefsFileSystemAnalyser_decompiled.c:13913-13998`):

```c
FUN_180015b80: *param_1 = RefsProc::CRefsDataRunBasicItem::vftable; ... param_1[3] = param_2;
FUN_180015bd0: FUN_180015b80(param_1,param_2);
               *param_1 = RefsProc::CRefsFileDataRunItem::vftable;     /* file data runs */
FUN_180015b40: *param_1 = RefsProc::CRefsAppDataRunItem::vftable;
               FUN_18000d890(param_1 + 1);                              /* run-list subobject */
```

CRefsAppDataRunItem is a sibling of CRefsFileDataRunItem in the
data-run item family (a run-list subobject, not a stream object); its
only references in the module are its ctor/dtor (:13917/:13996) — zero
behavioral uses. The run machinery it belongs to is
`[CalcFileDataRuns] ... CRefsFileDataRunItem::NotifyDataRunItem`
(:14512-13). **Verdict: NO og component recovers ReFS ADS through this
class — I-012's resolved-shared-limitation registration (neither side
recovers ReFS ADS) stands.**

## RS-7 — the og's system-table set is {0x500, 0x600, 0x701}; 0x520 is nowhere

The handler registrations in the RefsQuickSearchFilesImpl ctor
(`ReFSSearchDLL_decompiled.c:600-620`):

```c
  local_118._0_4_ = 0x500;  local_110 = &DAT_180002d30; ... FUN_180006d70(...);
  local_118._0_4_ = 0x600;  local_110 = &DAT_1800030d0; ... FUN_180006d70(...);
  local_118 = CONCAT44(local_118._4_4_,0x701);  local_110 = &DAT_1800032e0; ... FUN_180006d70(...);
```

and the id NORMALIZER in the quick-search table walk (`:1150-1154`):

```c
          if (puVar11 == (ulonglong *)0x600) {
            FUN_1800041f0(param_1);                 /* the dedicated root path */
          }
          else if ((ulonglong *)0x700 < puVar11) {
            puVar11 = (ulonglong *)0x701;           /* every id > 0x700 -> the 0x701 family */
          }
```

`0x520` has ZERO occurrences in ReFSSearchDLL_decompiled.c (grep). The
core's `FSMETADATA_TABLE = 0x520` exclusion (introduced uncited in the
T10-era engine, 77124c3) is a fabricated constant: the og's non-root
system families are **0x500 and everything above 0x700**. Fixed this
session (cr_refs_engine.cpp: the fallback root-detection and the orphan
sweep now exclude {0x500, >0x700} per the og; fail-before test
`Refs.FallbackRootSkipsOgSystemFamilies`).

## RS-8 — the og checks only the MSB+ magic per page; no volume-GUID cross-check

Both per-page validation sites in the page walks:

```c
:2904   piVar11 = (int *)((longlong)(uint)((int)uVar14 * iVar13) + (longlong)local_88);
        ...
        bVar19 = *piVar11 == 0x2b42534d;            /* MSB+ magic ONLY */
:12625  bVar19 = false;
        if (local_98 != (int *)0x0) {
          bVar19 = *local_98 == 0x2b42534d;         /* MSB+ magic ONLY */
        }
```

Exactly 2 occurrences of 0x2b42534d in the engine (grep); neither walk
compares the page's volume-GUID field against the boot sector's. **The
core's dual validation (page signature + volume signature on every
page, cr_refs_engine.cpp:111-119) is a deliberate superset — registered
INFO (a stricter bounds check that rejects cross-volume page bleed the
og would tolerate).**

## RS-2 / RS-3 — superseded by the landed decodes

- **RS-2** (the resident-row key shape, FUN_18000e490's trigger): landed
  in S-020 — the core materializes resident/inline content with the
  key-size-0x10 + subtype-1 trigger (cr_refs_engine.cpp:374-376,
  `Refs.ResidentInlineFileRecovered`).
- **RS-3** (the container-map key-list construction): landed in N-036 —
  FUN_180002a60 :1302-1342 decoded (0xa0/0xe0-length keys,
  {base@+0x90, span@+0x98}, the span==DBR+0x34 acceptance gate), the map
  built during the ObjectID walk + the bounded system-table sweep; the
  alternative key-relative reading is N-036's own recorded residual
  (real-multi-container-volume-gated).

## RS-1 / RS-4 — fixture-gated (pushed forward, precisely)

- **RS-1** (scan a real Server-2022 ReFS 3.7 image, body layout class
  9): the cited run id 40079065019 is the NTFS corrupt-MFT bench
  scenario, not a ReFS image; the corpus has NO real ReFS volume (the
  refs module's standing UNVERIFIED note). The mechanism RS-1 gated
  (the class-9 attribute-key layout) is covered by M-022's landed
  grammar decode + tests. Next step: the Windows bench stage with a
  real 3.7/3.11 volume image (mkrefs/format on Server-2022), CR_REFS_DEBUG=1,
  assert stats.files > 0 + spot-check extents.
- **RS-4** (do named-stream rows reach the stream handler on a real
  ADS-bearing volume): same fixture class. Next step: the same real
  volume with an ADS-bearing file, dump the file-table rows (types at
  key[0]) and compare the fold behavior against S-013's .mo-side answer
  (RS-6 above: no .mo ADS recovery).
