# Decompiled evidence — session 4: the og's CRecoverability grading engine + RecoveryAssistant (S-023)

Source: `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (corpus `03ad46c`).
All quotes verbatim; addresses = Ghidra line numbers in that file. Verified by the
session-4 independent-verification leg (two-agent convergence: corpus-artifact leg +
decompile leg; the seed artifact `artifacts/DRWUI-subsystems--assistant-grading-preview.md`
was read FIRST and its claims re-checked — two of its glosses came back corrected, see
§Corrections).

## 1. Engine-side grade default = 1 (OQ-S2M-1 resolution)

`NTFSSearchDLL_decompiled.c` — the ONLY grade-field writes in any engine (grade bits =
bits 5-7 of node+0x73):

```c
39242:  *(undefined1 *)((longlong)puVar1 + 0x73) = 0x20;      // CFileNode ctor: grade = 1
39779:  *(undefined1 *)((longlong)param_1 + 0x73) = 0x20;     // CFileSystemNode ctor: grade = 1
```

Zero grade/quality/confidence strings in any engine DLL (rg case-insensitive, all 9
searchdll trees). **Every user-facing 0/2 verdict is computed DRWUI-side.**

## 2. The grade setter/getter — bits 5-7 of node+0x73, monotonic-max, grade-2 sticky

```c
// FUN_140089630 (:134188-134204) — setter
bVar1 = *(byte *)(param_1 + 0x73);
if (param_2 == 0) {
    if ((bVar1 & 0xe0) != 0x40) { *(byte *)(param_1 + 0x73) = bVar1 & 0x1f; return; }
    // clear grade — but a grade-2 node (0x40) is never cleared: STICKY
}
else if ((int)(uint)(bVar1 >> 5) < param_2) {
    *(byte *)(param_1 + 0x73) = bVar1 & 0x1f | (char)param_2 << 5;   // grade = max(cur,new)
}

// FUN_140089670 (:134209-134213) — getter = *(byte*)(node+0x73) >> 5
// FUN_1400895b0 (:134149-134166) — sets bit 2 of +0x73 (overlap flag) AND forces grade >= 2
```

Grade domain = {0, 1, 2}; display strings "low"/"ave"/"high" at DRWUI:1162489-1162530
(QSS states; "Waiting..." transient). Grade-2 consumers: `:231145` ("Preview"/
"Preview_Fail_High" analytics), sort comparator `:535155-535161` (grade-2 first),
`:317982-317984` logs `"Level=%d"` to `Rb2.log`.

## 3. The decision tree (FUN_14038ac90, :605686; re-grader FUN_1401d7110 :340852-340873)

```c
605726:  lVar3 = FUN_140089170(param_2);           // size (node+0x30 obj, vtable +0x18)
605727:  if (lVar3 < 1) { FUN_140089630(param_2,0); }        // size < 1 → grade 0
605731:  iVar2 = FUN_140088f00(param_2);           // *(byte*)(node+0x70) >> 2  [CATEGORY byte]
605732:  if ((iVar2 == 0xc) && (grade != 2)) { FUN_140089630(param_2,2); }   // category 0xC → 2
605736:  local_c0 = FUN_140088770(param_2);        // virtual GetType (node+0x30, vt+0xa0; null→0xd)
605737:  if ((local_c0 != 3) && (local_c0 != 10 && (local_c0 != 0xb))) {
605738:      FUN_1403895c0(local_30,param_2);      // GENERIC path (extension extraction)
605739:      FUN_140389400(local_30);              //   + overlap tree insert
```

Deep-validation pass (separate job; the entry `FUN_14038aad0` :605634-605679 logs
"Recoverability analyze begin." and queues TWO std::thread jobs: `FUN_14038b0e0`
device walk + `LAB_14038b590` assistant half):

```c
605879:  if ((*(char*)(local_b8+0x160)!='\0') && (FS != 3)) skip;   // device walk gates on NTFS
605897:  if ((-1 < local_a8) && (local_a8 < 9)) { FUN_140089630(local_b0,2); }  // NTFS dev idx ∈[0,9) → 2
605902:  if ((vt+0xb8 != 0) || (vt+0x100 == 0xa8)) { FUN_140089630(local_b0,0); }
605915:  if (lVar5 < 0x400) { FUN_140089630(local_b0,2); }   // size < 1 KiB → 2
605919:  else              { FUN_140089630(local_b0,0); }   // else → 0          [not in the seed artifact]
```

Generic-path overlap grading (:605750-605785): extent-end key
`local_88 = base + len + local_a8 * unit` inserted into the tree at this+0x20; same-category
twin found → `FUN_140089630(node,2)` (:605781). `FUN_14038ba60` (:606023-606058) sets both
nodes' bit 2 + upgrades the new node to 2, and bumps the progress counters DAT_141be5838/28/40.

**Extension-table gate** (generic path): deep validation only for extensions in the
**169-entry static table starting with the key "Raw"** (table @0x141bb0500; lookup at
:605297-605304). Other extensions skip the deep path entirely.

**The −2 is an extent OFFSET, not a grade adjustment** (seed artifact's gloss corrected):

```c
605752:  local_a8 = (**(code **)(*local_b0 + 0x30))(local_b0);   // device-derived index
605753:  local_bc = (**(code **)(*local_b0 + 0xa0))(local_b0);   // device kind
605754:  if ((local_bc == 1) || (local_bc == 4)) { local_a8 = local_a8 + -2; }  // OFFSET
605768:  local_88 = local_80 + local_98 + local_a8 * local_b8[0];  // extent-end key
```

**Existing-file fast grade** (FUN_14037a8a0 family, :339898-339908): bit-6-of-+0x72 nodes get
grade 2 at insert if size ≥ 1 (else 0) — no deep validation.

**Per-selection re-grade** (:340852-340873): grade-1 nodes re-run the full tree when the
user selects/previews them (grade ∉ {0,2} gate) — the "re-grading telemetry" family.

**Small-result heuristic** (:340843-340845): if total < 100 (gate clear) → matched forced
to 0 — the "recoverable" gauge never shows for tiny results. Counter composition
(:340824-340847): matched = min(quickMatched,total) + DAT_141be5838+5828+5818; total =
quickTotal + 5820+5830+5840; log lines `"Quick scan matched=%d, total=%d"` /
`"Recovery ability matched=%d, total=%d"`.

## 4. RecoveryAssistant (FUN_1401c6f10, :330758-330960) — VERIFIED 100%

```c
330837:  FUN_1401631d0(L"RecoveryAssistant.log","RecoveryAssistant analyze begin.\r\n");
330838:  local_60 = GetTickCount64();
330908:  local_58 = GetTickCount64() - local_60;  param_1[0x38] = local_58;
330912:  FUN_1401631d0(L"RecoveryAssistant.log","RecoveryAssistant analyze finish.\r\n");
330913-330928: "analyze TotalFileCount = %d", "rawCount", "lostPartitionExistFileCount",
   "lostPartitionDeleteFileCount", "QuickScanExistFileCount", "QuickScanDeleteFileCount",
   "DeepScanFileCount", "QuickScanDeleteExtend"
330930-330952: per-category via FUN_1401c8940(param_1+8,&id):
   5→Graphics  6→Document  7→Archive  8→Video  9→Audio  0xB→Other
```

Runtime corroboration: `og-codebase/log/DRWUI.log` contains the live telemetry family.

## 5. Corrections to the seed artifact (fed to CORPUS-ERRATA)

1. "node.type == 0xC → grade 2 # ZFS special-case" — **REFUTED**: `FUN_140088f00` reads the
   CATEGORY byte (`*(byte*)(node+0x70)>>2`); 0xC is a derived category value (the category
   mapper `FUN_14039aa40` :614863-614904 rewrites type 6→0xC on bit 35 of node+0x89). ZFS is
   FS code 0xD (string map :955212-955227: "btrfs"→0xB, "ext4"→9, "zfs"→0xD).
2. "grade adjustment: device.kind ∈ {1,4} → offset −2" — the −2 applies to the
   extent-address index, never the grade (§3 above).
3. The {3,10,0xB} deep-path label "NTFS / ReFS / Btrfs" is consistent with the FS-code map
   (3=NTFS, 10=ReFS, 0xB=Btrfs) — accepted; but note the deep device-walk job itself gates
   on FS==3 (NTFS) only (:605879).

Two-agent convergence note: the corpus-artifact leg (context sheet A) and the decompile
leg (context sheet E) reached these corrections independently; the quotes above are the
decompile leg's verbatim extractions.
