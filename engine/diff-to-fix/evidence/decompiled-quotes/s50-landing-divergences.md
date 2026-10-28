# S50 evidence — the three landing divergences, both sides quoted (S-062 / M-138 / M-139)

**Lane:** S50 continuation audit lane. Every quote below was read first-hand
by the lead (Ghidra decompile + core tree at `93461483`, code-identical to
`0d7b1d9d`); the retdec cross-check is noted where it was used.

---

## 1. S-062 — the ONE chunk-chain walk's size-gate exit values

### The og (RAWSearchDLL_decompiled.c FUN_180044e30, :52316-52358, read first-hand)

```c
uVar1 = *(longlong *)(puVar2 + 0x70) * 2;          /* uVar1 = 2 x device size */
while (((((uVar5 = uVar4, uVar4 <= uVar1 &&        /* (A) uVar5 = cursor        */
          (puVar2 = FUN_180042930(...), puVar2 != (undefined *)0x0)) &&
         (*(longlong *)(puVar2 + 4) == 0x7853b1ae4da7d88c)) &&   /* (C) chunk GUID */
        ((*(int *)(puVar2 + 0xc) == -0x2c69d630 &&
         ((uVar3 = *(ulonglong *)(puVar2 + 0xc4), uVar5 = uVar1, uVar3 != 0 ||
                                                    ^^^^^^^^^^^^^ (E) uVar5 = 2x-device
          (uVar3 = *(ulonglong *)(puVar2 + 100), uVar3 != 0)))) &&  /* (F) fallback size */
        (uVar3 <= uVar1))) {                          /* (G) size <= 2x-device */
  uVar4 = uVar4 + uVar3;
}
*(ulonglong *)(param_2 + 0x10) = uVar5;              /* the published size */
```

The comma semantics: at (A) `uVar5` = the current cursor on EVERY iteration;
at (E) `uVar5` = 2x-device the moment the primary size field is read.
Exit table:
- both size fields 0 (E's `!= 0` tests fail) -> **uVar5 = 2x-device**
- size > 2x-device (G fails) -> **uVar5 = 2x-device**
- window read miss (B) / GUID mismatch (C)/(D) / cursor > 2x-device (A's
  second test) -> **uVar5 = the cursor** (the last valid position)
- normal exhaustion -> uVar5 = the cursor

The retdec cross-check (ci-multitool/RAWSearchDLL/retdec_decompiled.c,
function_180044e30) independently shows the same shape: `v5 = v2;` (v2 =
2*device) immediately before both size-gate breaks.

The published size then rides the FileInfo ctor's 500,000,000 clamp
(FUN_180016080 :17203).

### The core (cr_analysers.cpp validate_one, current tree, read first-hand)

```cpp
u64 sz = u64_from_bytes_le(c + 0xc4);
if (sz == 0)
    sz = u64_from_bytes_le(c + 0x64); /* the fallback field */
if (sz == 0 || sz > dev2)
    break; /* the 0<size<=2x-device gate */        <- keeps `size` = last-valid end
pos += sz;
size = pos; /* the end of this valid chunk - the break keeps it */
```

The comment above the loop claims "The og's size is the cursor at the
chain's break point (uVar5 - the end of the last FULLY-VALID chunk)" — the
comma-decode above refutes it for the two size-gate exits. On a first-chunk
decline (both fields 0) the core publishes 0 (no carve) where the og
publishes min(2x-device, 500M).

---

## 2. M-138 — the HIK discriminator start-code byte

### The og (RAWSearchDLL_decompiled.c FUN_1800272a0, :30412-30484, read first-hand)

```c
if (*param_2 == -0x45ff0000) {                      /* 00 00 01 BA  - the pack */
  bVar7 = (*(byte *)((longlong)param_2 + 0xd) & 7) + 0xe;
  ...
  if (*(int *)(uVar6 + (longlong)param_2) == -0x44ff0000) {   /* 00 00 01 BB - system header */
    ...
    if (*(int *)((ulonglong)uVar5 + (longlong)param_2) == -0x43ff0000) {   /* <- the discriminator */
      ...                                             /*  -0x43ff0000 = 0xBC010000 = 00 00 01 BC */
      *(undefined4 *)(param_3 + 0x18) = 0x92;         /* the HIK format id */
      ... FUN_18000f4b0(..., &DAT_18006b428);         /* "HIK" (48 00 49 00 4B 00, byte-dumped) */
```

The else-branch (the direct path, :30452) uses the SAME constant
`-0x43ff0000`. Constant arithmetic: -0x43ff0000 as u32 = 0xBC010000, and
0xBC010000 in little-endian bytes = 00 00 01 **BC** — MPEG-PS
**private_stream_2**. The -0x42ff0000 (= 00 00 01 BD, private_stream_1)
constant appears NOWHERE in the decompile (grep-verified; retdec's
function_1800272a0 shows the same -0x43ff0000 twice).

### The core (cr_analysers.cpp validate_hik, current tree, read first-hand)

```cpp
u8 bd[6];
if (!rd(s, hit + p, bd, sizeof(bd)))
    return r;
if (bd[0] != 0 || bd[1] != 0 || bd[2] != 1 || bd[3] != 0xBD)
    return r; /* private_stream_1 required at the skip end */
```
and the direct path:
```cpp
} else if (sc[0] == 0 && sc[1] == 0 && sc[2] == 1 && sc[3] == 0xBD) {
    pos = pack_len; /* the direct private_stream_1 after the pack */
```

The block comment (:6000-6003) and the landed finding's own title
("HIK private_stream_1 discriminator") carry the transcription error.

---

## 3. M-139 — the CSL/CD01 per-candidate latch vs the core's end-tag fallback

### The og (RAWSearchDLL_decompiled.c FUN_180041950, :49749-49828, read first-hand; ctor FUN_180041880 :17291-17306)

```c
if (param_2 == (int *)0x0) goto EXIT;
if (*(int *)(param_1 + 0x120) == 0) {                 /* the per-candidate LATCH */
    if (*param_2 == DAT_180093918) {                  /* window starts with CD 01 00 00 */
        *(undefined4 *)(param_1 + 0x120) = 1;         /* latch ON - permanent for this candidate */
        goto LAB_1800419b2;                           /* -> the 0x44c branch, NOW */
    }
    /* ...the end-tag branch (FUN_180041b40) + the 0x110/0x118 size assignment... */
}
else {
LAB_1800419b2:  /* the 0x44c branch */
    uVar4 = 0;
    ...
    if ((uVar7 < 0x44c) && (0x44c < (ulonglong)((lVar5 + 1) * lVar6))) {
        ...
        uVar4 = (ulonglong)(*(int *)(lVar3 + (longlong)param_2) + 0x480);   /* dword@0x44c + 0x480 */
        if ((lVar6 < lVar5 + 0x450) ||
            (*(longlong *)(param_1 + 0x118) < (longlong)uVar4))             /* the 1 MiB cap */
            goto EXIT;                                /* declined - NO size assigned */
    }
    if (uVar4 == 0) goto EXIT;                        /* declined - NO size assigned */
    *(ulonglong *)(param_1 + 0x10) = uVar4;
}
EXIT: return;
```

The ctor sets the cap field: `puVar1[0x23] = 0x100000` (= +0x118, the 1 MiB
cap) and the id `*(undefined4 *)(puVar1 + 3) = 0x5b` (= +0x18), with the
size slot (+0x10) left unset by the base ctor. A latched candidate whose
0x44c read declines (the file too short to span 0x44c, the dword+0x480 out
of cap, or the zero result) NEVER reaches the end-tag branch — every
subsequent walker call enters at the latch test and jumps straight to the
0x44c branch — and the size slot stays unset (the og's pending model drops
unsized candidates). The block scan is sector-unit, so a real CD01
candidate's first walker window starts at the candidate and the latch fires
on the first call.

### The core (cr_analysers.cpp, the M-112 block, current tree, read first-hand)

```cpp
if (n >= 4 && u32_from_bytes_le(win) == 0x000001cdu) {
    const u64 avail = s->size() > hit ? s->size() - hit : 0;
    if (avail >= 0x450) {
        u8 f[4];
        if (rd(s, hit + 0x44c, f, sizeof(f))) {
            const u64 field_size = u64(u32_from_bytes_le(f)) + 0x480;
            if (field_size != 0 && field_size <= cap) {
                ... return r;                          /* the 0x44c branch - parity */
            }
        }
    }
}                                                     /* <- DECLINE FALLS THROUGH */
const u64 limit = probe_limit(s, hit, cap);
const u64 marker_end = scan_tag_two_case(s, hit, limit, "</svg>");   /* the end-tag walk */
...
if (marker_end != 0) { r.ok = true; r.end = marker_end; ... return r; }
r.ok = true; r.end = limit; r.confidence = 500;         /* the bounded fallback */
```

The landed comment ("Declination ... falls to the end-tag walk exactly as
the og's pending model retries other sizing") is the claim the decompile
refutes: the og has no end-tag retry for latched candidates.

---

## Cross-cutting note

All three divergences share one root cause: a MAJOR-tier stand-in repair
whose og decode was mistranscribed in exactly one place (a constant, an
exit-value family, a control-flow structure). None of the 22 landings in
the S42-C sweep had this defect — the window's newest code (c0efacc3,
c98eb7a7, 73a134eb) is where all three live, which is the regression watch
working as designed.
