# Evidence: CRW vs MRW key attribution (M-081, FIXED at 9e5dedc — the SA-9 lane's convergent instruction-decisive decode, merged as the FIXED file's addendum)

**Corpus:** `easeus-drw-research` @ `be0cfe9`
**Source:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
**Verified:** first-hand by the session-9 lead.

## 1. The II/MM 1A 00 keys are registered to the CRW chain

Registration block (lines 1746-1762):

```c
  local_res8 = (uint *)FUN_1800020c0(0x18);
  puVar2 = puVar8;
  if (local_res8 != (uint *)0x0) {
    puVar2 = (uint *)FUN_18003c9f0((undefined8 *)local_res8);   // <- CRW ctor
  }
  local_res10[0] = 0x1a4949;                                     // "II\x1A\x00"
  plVar3 = FUN_180006c80(param_1,local_res10);
  *plVar3 = (longlong)puVar2;
  local_res8 = (uint *)FUN_1800020c0(0x18);
  puVar2 = puVar8;
  if (local_res8 != (uint *)0x0) {
    puVar2 = (uint *)FUN_18003c9f0((undefined8 *)local_res8);   // <- CRW ctor
  }
  local_res10[0] = 0x1a4d4d;                                     // "MM\x1A\x00"
  plVar3 = FUN_180006c80(param_1,local_res10);
  *plVar3 = (longlong)puVar2;
```

The ctor `FUN_18003c9f0` (lines 46100-46121):

```c
undefined8 * FUN_18003c9f0(undefined8 *param_1)
{
  *param_1 = IRawFileAnalyser::vftable;
  *(undefined4 *)(param_1 + 1) = 0xffffffff;
  *param_1 = Brance::CRWAnalyser::vftable;      // id:
  *(undefined4 *)(param_1 + 1) = 0x12;          // 0x12
  puVar1 = (undefined8 *)FUN_1800020c0(0x418);
  if (puVar1 == (undefined8 *)0x0) { ... }
  else {
    *puVar1 = IRawFileAnalyser::vftable;
    *(undefined4 *)(puVar1 + 1) = 0xffffffff;
    *puVar1 = FileCRWAnalyser::vftable;         // fallback analyser
    *(undefined4 *)(puVar1 + 1) = 0x12;         // id 0x12
    memset(puVar1 + 2,0,0x400);
  }
  param_1[2] = puVar1;
  return param_1;
}
```

So: **II/MM 1A 00 -> Brance::CRWAnalyser (id 0x12) wrapping FileCRWAnalyser
(id 0x12)** — Canon CIFF (CRW).

## 2. FileMRWAnalyser rides `\0MRM` ONLY

Registration (lines 2500-2511):

```c
  if (local_res8 != (uint *)0x0) {
    *(undefined ***)local_res8 = IRawFileAnalyser::vftable;
    local_res8[2] = 0xffffffff;
    *(undefined ***)local_res8 = FileMRWAnalyser::vftable;
    local_res8[2] = 0x66;                       // id 0x66 = 102
    puVar8 = local_res8;
  }
  local_res10[0] = 0x4d524d00;                  // "\x00MRM"
  plVar3 = FUN_180006c80(param_1,local_res10);
  *plVar3 = (longlong)puVar8;
```

No other registration binds FileMRWAnalyser (grep of the full decompile:
vftable references at :2504 and :14222 only — the ctor/dtor pair).

## 3. The CRW validation gate (CIFF)

`FUN_18003cb10` (lines 46148-46185; DAT values extracted from the DLL .data
by the B-agent and re-checked against the validator's constant table at
FUN_180052000, lines 62290-62300):

```c
  if ((*(int *)(param_2 + 4) == DAT_180093804) && (*(short *)(param_2 + 8) == DAT_180093808)) {
    if (((*(int *)(param_2 + 10) == DAT_18009380a && (*(char *)(param_2 + 0xe) == DAT_18009380e))
       || ((*(int *)(param_2 + 10) == DAT_18009383a && (*(char *)(param_2 + 0xe) == DAT_18009383e))
       ))) goto LAB_18003cbd7;
    if (*(int *)(param_2 + 10) != DAT_180093882) { return 0; }
    ...
LAB_18003cbd7:
    *(undefined4 *)(param_3 + 0x18) = 0x12;     // format id 0x12
```

DAT table (from the binary): u32@+4 == 0x48450000 (bytes `00 00 'H' 'E'`),
u16@+8 == 0x5041 ("AP"), u32@+0xA in {"CCDR", "TIFP", "ARCH"},
u8@+0xE == 0x02. I.e. after the II/MM 1A 00 key: `\x00\x00` + `HEAP` +
block type in {CCDR (image data), TIFP, ARCH} + length byte 2 — the CIFF
heap structure of Canon CRW files (`II\x1A\x00\x00\x00HEAPCCDR...`).

The validator's expected-pattern table (FUN_180052000, :62290+) carries the
II/MM 1A + "HEAP" + type-block constants for all key/type combinations.

## 4. The MMOR/IIRS/FUJI keys are NOT CRW (agent-sheet correction, absorbed)

Lines 1727-1744 register `0x53524949` ("IIRS"), `0x494a5546` ("FUJI"), and
`0x524f4d4d` ("MMOR") with `FUN_180037b50` — which constructs
`BruteForce::TiffAnalyzer` (id 0x40) wrapping `FileTIFAnalyser` (verified:
ctor at FUN_180037b50, vtable lines shown in the decompile). These are
TIFF-family variants; the product already carries ORF/IIRS + ORF/MMOR rows
and the RAF "FUJI" key (cr_registry.cpp:281-313, :233-234).

## 5. Product state (core @ 35adcb9)

`src/core/carve/cr_registry.cpp:709-738`:

```cpp
    {FMT_MRW,     "mrw", {'I', 'I', 0x1A, 0x00}, 4, 0, 512, 64ull << 20, 650, validate_mrw, "Minolta MRW"},
    {FMT_MRW_MM,  "mrw", {'M', 'M', 0x1A, 0x00}, 4, 0, 512, 64ull << 20, 650, validate_mrw, "Minolta MRW (BE)"},
    {FMT_MRW_MRM, "mrw", {0x00, 'M', 'R', 'M'}, 4, 0, 512, 64ull << 20, 600, validate_mrw, "Minolta MRW (MRM)"},
```

`validate_mrw` (`cr_analysers.cpp:3057-3088`) gates the II/MM 1A rows on a
"PRD" block anchor at the u32@+4 offset — a Minolta MRW block structure that
Canon CRW files do not carry (they carry `\x00\x00HEAPCCDR`). Registry grep
for a CIFF/CRW analyser: none (format list: 101 FMT_* ids, no FMT_CRW).

Consequences (both verified):
1. Real Canon CRW files hit the II/MM 1A keys, run `validate_mrw`, fail the
   PRD gate -> **not carved**.
2. The FMT_MRW / FMT_MRW_MM rows can never accept a real file: real MRW
   files begin `\x00MRM` (they hit FMT_MRW_MRM, which works), and the II/MM
   1A byte patterns only occur in Canon CRW/CIFF data -> **two dead rows**
   occupying the og's CRW key space.
