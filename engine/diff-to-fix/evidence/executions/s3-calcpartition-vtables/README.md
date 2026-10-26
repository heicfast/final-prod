# S3-LOST execution evidence — ICalcPartition vtable extraction (RE probe)

Tool: `rtti_vtable_dump.py` (pure-stdlib MSVC x64 RTTI walker; reads PE sections,
locates the TypeDescriptor via the rabin2-dumped RTTI name VA, finds the
CompleteObjectLocator via pTypeDescriptor RVA + pSelf self-check, then the
vftable via the 8-byte pointer-to-COL, and resolves slots against Ghidra's
`*_functions.txt` index). Read-only against the og binaries in
`og-codebase/`.

## Commands

    python3 rtti_vtable_dump.py \
      /home/z/my-project/audit/easeus-drw-research/og-codebase/FATSearchDLL.dll \
      /home/z/my-project/audit/easeus-drw-research/re-workspace/reversed/ghidra-searchdll/FATSearchDLL_functions.txt \
      "Fat32CalcPartition:0x18006c530:14" "ICalcPartition:0x18006c560:10" \
      > fat_vtable_dump.txt

    python3 rtti_vtable_dump.py \
      /home/z/my-project/audit/easeus-drw-research/og-codebase/exFATSearchDLL.dll \
      /home/z/my-project/audit/easeus-drw-research/re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_functions.txt \
      "ExFatCalcPartition:0x18005efc8:16" "ICalcPartition:0x18005f088:10" \
      > exfat_vtable_dump.txt

TypeDescriptor VAs derive from the rabin2 string dumps (name at TD+0x10):
- FATSearchDLL.dll  `rabin2_strings_data.txt:545` `.?AVFat32CalcPartition@@` @ VA 0x18006c540 (TD 0x18006c530)
- FATSearchDLL.dll  `:546` `.?AVICalcPartition@@` @ VA 0x18006c570 (TD 0x18006c560)
- exFATSearchDLL.dll `rabin2_strings_data.txt:559` `.?AVExFatCalcPartition@@` @ VA 0x18005efd8 (TD 0x18005efc8)
- exFATSearchDLL.dll `:562` `.?AVICalcPartition@@` @ VA 0x18005f098 (TD 0x18005f088)

## Results (see fat_vtable_dump.txt / exfat_vtable_dump.txt)

Fat32CalcPartition vftable @ 0x180053680 (12 slots), ExFatCalcPartition vftable
@ 0x18004b9f0 (12+ slots); slot 4 (the "score" of session-2's note) resolves to
`FUN_180030550` / `FUN_180022670` — both are plain `return *(u32*)(this+0x18)`.
Slot 1 is the matching setter. Field +0x18 is proven to be sectors-per-cluster
by FUN_180030d90 (`(cluster-2)**(int*)(this+0x18) + *(u64*)(this+0x10)`), i.e.
the classes are cluster->sector geometry calculators, not confidence scorers.
