# s42 evidence — the lost-partition surfaced-shape batch (M-137's worksheet)

**Corpus:** `easeus-drw-research` @ `be0cfe9`; decompile = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (cites re-read first-hand this session).
**Core:** `core_recovery` @ `b26017a7`.

## §1 The naming template `FUN_1403b9330` (:634795-634843)

```c
:634817  if (flag_1c8 == 0) {
:634818      volname = FUN_14008a650(node);                    /* GetVolumeName   */
:634819      if (volname == 0) {
:634820-822      fs = FS-name-of(param_2);  counter = *(param_1+0x400);  *(param_1+0x400) += 1;
:634825          sprintf_s(buf, 100, L"%s %d(%s)", L"Lost Partition", counter, fs);
:634827      } else {
:634828          if (strstr(volname, ":)") == 0)                 /* no drive-letter label */
:634831              sprintf_s(buf, 100, L"%s (%s)", volname);
:634834          else  sprintf_s(buf, 100, L"%s", volname);     /* keep "Local Disk (I:)" */
```

The counter is per-scan (the controller's +0x400), incremented per unnamed partition; the FS name rides the third format slot.

## §2 The "Unallocated" relabel (:525588-525598)

The device-listing pass (`FUN_1403040d0` region) does `strstr(name, "Lost Partition")` → relabel to "Unallocated" (any partition still carrying the placeholder name at listing time; "USB Drive" default for empty names at :525578).

## §3 The insertion loop (:904185-904242) — cap, Quick registration, 512 fallback

```c
:904185  if (local_19b8 == 0) local_19b8 = 0x200;            /* sector-size fallback = 512 */
:904190  FUN_140588820(&local_19d0, &local_1948);            /* the 3-source assembly     */
:904195  for (p = list; p != end; p += 0xA8) {               /* 0xA8-stride candidates     */
:904198      FUN_140162e00("Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d\r\n",
                             "…genericrecoverycontroller.cpp", 0xa2, …);
:904215      target = FUN_14017b110(…);                       /* the 0x6F5-B partition struct */
:904223      FUN_14017fd40(param_1 + 0x42, &target);
:904229      uVar19 = param_1[0x58];  param_1[0x58] = uVar19 + 1;
:904231      if (9 < uVar19) { FUN_1402753f0(local_1928); break; }   /* INSERTION CAP = 10 */
:904239      FUN_14006e2f0(&local_1a38, L"Quick", 5);
:904240      FUN_140118650(uVar11, &local_1a38, local_1928);  /* register for a Quick search */
```

## §4 The found-partition presentation rows (UIUX/08 §2)

The `CPartitionScanFrame` "We found partition(s):" list: per-partition **Disk / Basic / Dynamic / MBR / GPT / File System** property rows, per-disk "Preview for disk %1", Recover/Stop/Home controls; the home screen's partition list comes from `CreateEnumPartitionObject` (PartitionRecovery.dll exports, rabin2 :54) via `EnumCollectPartition::EnumDevice` / `CEnumPartitionThread`.

## §5 Core-side pins (at b26017a7)

- `Candidate @ <LBA>` roots: `cr_scan.cpp:1933-1939`.
- The refusal on zero sector size: `cr_candidates.cpp:759-760` (`ERR_BAD_PARAMETER`).
- No cap / always-deep: `cr_candidates.cpp:915-1014` (pass 3), `cr_scan.cpp:1900-2010` (materialize + fan-out), the drift note `cr_scan.cpp:1307-1317`.
- The listing surfaces: `rrecov.cpp:2665-2672` (LBA/sectors/score/fs), `cr_ui.h:449-486` (OS-level enumeration only), `cr_ui_apply_candidate{start_lba, sector_count, fs}`.

## §6 The og's real-run corroboration

`og-codebase/log/DRWUI.log:630-648` (read via the s12 lane's record + this session's re-check): the smart partition scan runs first and separately (~2.3 s), then `CScanMgr::doStartLogic` / `ScanThread type=5`, the three-source assembly, then `"Insert lost partition … 162, StartSector=2048, TotalSector=122277823"` and `"Found lost partition(StartSector:2048, TotalSector:122277823, FileSystem:NTFS)"` — one candidate, under the cap; the vendor QA `CalcPartition.log` shows the surfaced-cap side (79/101/6 → 5).
