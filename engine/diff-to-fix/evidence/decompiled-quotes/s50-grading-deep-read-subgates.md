# S50 evidence — the grading deep-read sub-gates decoded (OQ-S42-3 closed)

**Lane:** S50 continuation audit lane. **Corpus pin:** `be0cfe9`
(`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`). **Core pin:**
`93461483`. Quotes read first-hand by the lead; the parallel probe agent's
FUN_14038c470 decode was re-verified line-by-line before this sheet.

## The function

`FUN_14038c470` (DRWUI_decompiled.c:606291-606421) — the "deep read" that
finalizes a file's recoverability grade. Signature shape:
`FUN_14038c470(file_object, start_offset, mode_flag, extra_flag)`.
The grade lives in **bits 5-7 of file+0x73** (0=Low, 1=Middle, 2=High).

## The sub-gate list (each condition -> effect, with the decompile lines)

**G0 — early-outs (no read):**
```c
/* :606318 */ if ((param_1 == 0) ||
                  (iVar2 = FUN_1401c5da0(param_1), iVar2 != 0)) goto EXIT;
```
`FUN_1401c5da0` (:330210-330214) = bit 2 of +0x73 — the session-restored
"already graded" flag (set during propbag deserialize :133407-133409).
A device handle must also resolve (`FUN_1401b54f0` chain :606319-606330),
else the file returns ungraded.

**G1 — the no-read grade-0 gate (mode 0 only):**
```c
/* :606356-606360 */
cVar1 = FUN_1401bc5a0(local_1078, param_1, param_2);   /* offset -> global key-set contains() */
if ((cVar1 == '\0') || (cVar1 = FUN_1401b3550(param_1), cVar1 == '\0')  /* bit7 of +0x72 */
 || (cVar1 = FUN_1403895a0(param_1), cVar1 != '\0'))                     /* bit2 of +0x74 */
    goto LAB_14038c622;                                  /* skip to the commit at grade 1 */
FUN_140089630(param_1, 0);                                /* grade 0 directly, no read */
```
Key-set membership via `FUN_1404ec7d0` (:820130-820143, vtable+0x20
`contains(key)`); the offset key resolves through `FUN_14035abe0`
(:575161-575178).

**G2 — the prev-sector tail gate (only when start_offset != 0):**
```c
/* :606396-606409 */
iVar2 = (*local_1070)(local_1090, local_1018, param_2 + -1, 1);   /* read 1 sector BEFORE the file */
if (iVar2 != 0) {
    local_1084 = local_1088 - 0x10;              /* local_1088 = device sector size (vtable+0x28, :606353) */
    iVar2 = memcmp(local_1018 + local_1084, local_1060, 0x10);   /* last 16 bytes of the prev sector */
    if (iVar2 == 0) {                            /* all-zero 16-byte tail */
        local_1098 = 1;                          /* (dead store in this decompile) */
        if (param_3 == '\0') { local_1094 = 2; } /* tentative grade 2 (mode 0 only) */
        goto LAB_14038c6d7;                       /* -> continue to G3 with a zeroed buffer */
    }
    if (param_3 == '\0') { local_1094 = 1; }      /* non-zero tail -> tentative 1 */
}
```

**G3 — the first-sector zeroed gate:**
```c
/* :606377-606383 */
iVar2 = (*local_1068)(local_1090, local_1018, param_2, 1);   /* read the file's FIRST sector */
if (iVar2 != 0) {
    iVar2 = memcmp(local_1018, local_1060, 0x20);             /* first 0x20 bytes vs 32-byte zero template */
    if (iVar2 == 0) { local_1094 = 0; FUN_140089630(param_1, 0); }   /* -> grade 0 (erased/wiped) */
```
(local_1060 pre-filled with 0x00 at :606333-606337; the 4096-byte stack
buffer local_1018 is zeroed before each read :606348-606352/:606370-606374.)

**G4 — the first-sector 0xFF gate:**
```c
/* :606385-606389 */
iVar2 = memcmp(local_1018, local_1040, 4);      /* first 4 bytes vs 0xFF template (:606338-606347) */
if (iVar2 == 0) { local_1094 = 0; FUN_140089630(param_1, 0); }
```

**G5 — the param_4 hook:**
```c
/* :606413-606414 */ if ((local_1094 == 1) && (param_4 != '\0')) FUN_140389570(param_1, 1);
```
(sets bit 1 of +0x74 when the tentative grade is 1 — the caller-controlled
"needs more work" marker.)

**G6 — the ratchet commit + done-flag:**
```c
/* :606416-606417 */
FUN_140089630(param_1, local_1094);   /* commit the grade */
FUN_140389540(param_1, 1);            /* bit 3 of +0x73 = deep-read-done this run */
```
`FUN_140089630` (:134188-134204) is a **MAX-ratchet**: it refuses to
overwrite an existing grade 2 with a 0 —
```c
/* :134196-134204 */ if (param_2 == 0) { if ((bVar1 & 0xe0) != 0x40) { *(byte *)(param_1 + 0x73) = bVar1 & 0x1f; return; } }
```
i.e. a committed 2 cannot be erased by a later zero-write (the "grade-2
ratchet"). Mode 1 (param_3 != 0) can never reach 2 — the `local_1094 = 2`
store is mode-0-guarded, and mode 1 ends with the 0->1 upgrade only.

## The callers (read first-hand)

**Caller A — the full grading pass (`FUN_14038ac90`, :605726-605800):**
entry gates = size < 1 -> grade 0 (:605726-605728); FS-0xc with subtype != 2
-> grade 2 (:605732-605733); skip FS types 3/10/0xb (:605736-605737);
the extension-table gate (`FUN_1403895c0` + `FUN_1401bebe0`, :605738-605742);
twin-key dedup — start-offset key `local_88 = local_80 + local_98 +
local_a8 * local_b8[0]` with `local_a8 -= 2` for FS types 1/4
(:605753-605756/:605768) — on collision with the SAME device id the
PREVIOUS twin is upgraded to grade 2 (`FUN_140089630(local_90, 2)` at
:605773-605783) and the current file skips the deep read
(`local_c8 = '\x01'`); only non-twins reach `FUN_14038c470(file, off, 0, 0)`
(:605798). After return: `FUN_140389540(param_2, 1)` (:605800).

**Caller B — the ReFS preview/count pass (`FUN_1401c7d80`, :331272-331289):**
FS type 3 (ReFS) only, not-yet-graded, current grade == 1 ->
`FUN_14038c470(file, offset, 1, 0)` — mode 1's only possible effect is a
0->1 upgrade + the done-flag, then the counter tick at :331289.

## What this closes

- **OQ-S42-3** (the sub-gates inside the deep read): fully decoded — G0
  already-graded/done, G1 the no-read grade-0 key-set gate, G2 the
  prev-sector 16-zero-tail tentative-2, G3 the 32-zero first-sector grade-0,
  G4 the 4-byte-0xFF grade-0, G5 the flag hook, G6 the max-ratchet commit +
  done bit; read sizes = two single-sector device reads (vtable+8,
  count=1, 4096-byte buffer, sector size from vtable+0x28).
- **M-134's fix plan gains the decoded content**: the core's deep-read
  validation (absent — M-134 LIVE per the battery) can now be ported
  faithfully: first-sector 0x20-zero check -> Low; first-4 0xFF -> Low;
  prev-sector 0x10-zero-tail -> tentative High; max-ratchet semantics; the
  mode-1 ReFS preview pass; the twin-collision "previous twin -> 2" rule
  (which also feeds S-058/M-133's device-family decode: the twin-key index
  shifts by -2 for FS types 1/4 — FAT16 and EXT2-family — CONFIRMING
  M-133's og decode at :605753-605756).
- **OQ-S42-3's secondary question (the twin-key unit value)**: the callers
  show the unit is `local_b8[0]` (a per-scan table value), consistent with
  the M-133/M-137 filings' ranges; the sub-gate decode does not change
  M-133's open unit-value question (it stays inside M-133's own scope).
