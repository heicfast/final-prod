# s31 — OQ-S3-JPEG resolved: the legacy fallback tier ACCEPTS no-EOI JPEGs at its scan limit (S-018 re-filed)

> Session DFX31, corpus `03ad46c` unmoved. All quotes read first-hand from
> `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`.

## 1. The dispatcher: three sub-analysers by APPn identity

`FUN_18004b170` (`:57100-57210`, the `FileJPGAnalyser` vtable slot):

```c
  puVar4 = FUN_180042930((longlong *)&local_70,0);
  if (puVar4 != (undefined *)0x0) {
    builtin_strncpy(local_50,"JFIF",5);
    builtin_strncpy(local_48,"Exif",5);
    ...
    iVar2 = _strnicmp(local_50,puVar4 + 6,4);
    if (iVar2 == 0) {
      FUN_18004bbd0(param_1,param_2,(longlong *)&local_70);      /* JFIF  */
    }
    else {
      iVar2 = _strnicmp(local_48,puVar4 + 6,4);
      if (iVar2 == 0) {
        FUN_18004b3b0(param_1,param_2,(longlong *)&local_70);    /* Exif  */
      }
      else {
        FUN_18004c1e0(param_1,param_2,(longlong *)&local_70);    /* generic */
      }
    }
  }
```

## 2. The JFIF sub-analyser: SOS-gated header walk + dimension capture

`FUN_18004bbd0` (`:57567-57821`). Entry limit: `uVar15 =
*(ulonglong *)(FUN_180001350() + 0x70) / 10` — the global cap
(`:9048`-region init: `*(undefined8 *)(puVar3 + 0x70) = 200000000;`)
divided by 10 = **20,000,000**. The marker walk (`:57740-57806`):

```c
        switch(pcVar4[1]) {
        case -0x40:                       /* 0xC0 SOF0 */
          uVar12 = *(ushort *)(pcVar4 + 2) << 8 | ...;        /* segment len */
          pcVar4 = FUN_180042930(param_3,uVar11 + 4);
          if ((pcVar4 == (char *)0x0) || (*pcVar4 != '\b')) goto LAB_18004c154; /* precision != 8 → REJECT */
          uVar17 = ...BE16(pcVar4 + 1)...;                    /* height */
          uVar16 = ...BE16(pcVar4 + 3)...;                    /* width  */
          local_5e7 = '\x01';                                 /* SOF seen */
          break;
        ...
        case -0x3e:                       /* 0xC2 SOF2 */
          if (uVar17 * uVar16 == 0) { ...same dim read...; local_5e7 = 1; }
          break;                           /* fill-if-empty only */
        case -0x26:                       /* 0xDA SOS */
          if (local_5e7 == '\0') {        /* no prior SOF */
            FUN_18004c480(local_5e0,local_5c8,param_3,&local_5d8,(uint)uVar15); /* /10 limit */
            *(ulonglong *)(lVar1 + 0x10) = local_5d8;
            goto LAB_18004c154;
          }
          ... local_5e6 = '\x01';          /* SOS seen → walk exits */
          break;
        case -1:                          /* 0xFF */
          uVar12 = 0; break;
        }
        uVar11 = uVar11 + (ulonglong)uVar12 + 2;              /* advance */
        pcVar4 = FUN_180042930(param_3,uVar11);
        if (pcVar4 == (char *)0x0) goto LAB_18004c154;        /* buffer end → REJECT */
      } while (local_5e6 == '\0');                            /* until SOS */
      uVar17 = uVar17 * uVar16 * 3;                           /* h*w*3 */
      ...
        uVar20 = (uint)uVar15;                                /* default: 20,000,000 */
        if (uVar17 != 0) { uVar20 = uVar17; }                 /* dims known: h*w*3 */
        FUN_18004c480(local_5e0,local_5c8,param_3,&local_5d8,uVar20);
        uVar11 = local_5d8;
LAB_18004c13f:
        *(undefined4 *)(local_5c8 + 0x18) = 0x23;             /* cell format := JPEG */
        *(ulonglong *)(local_5c8 + 0x10) = uVar11;            /* cell size */
```

## 3. The post-SOS walker: accept-at-limit (THE ANSWER)

`FUN_18004c480` (`:57942-57991`) — scans for FF D9 (`-0x2601` = 0xD9FF
little-endian) in sector-block steps, and **accepts when the limit is
reached or the buffer getter returns NULL, with no EOI found**:

```c
  uVar6 = *param_4;
  do {
    while( true ) {
      if (((longlong)(ulonglong)param_5 <= (longlong)uVar6) ||
         (piVar4 = (int *)FUN_180042930(param_3,uVar6), piVar4 == (int *)0x0))
          goto LAB_18004c56b;                    /* LIMIT/DEVICE-END → ACCEPT */
      ...
      sVar1 = *(short *)((longlong)piVar4 + lVar5 + -2);
      if (sVar1 != 0) break;
      ...
    if (sVar1 != -0x2601) goto LAB_18004c501;    /* not FF D9 at boundary */
LAB_18004c511:
    if (sVar1 == -0x2601) goto LAB_18004c56b;    /* FF D9 → ACCEPT */
    psVar7 = FUN_18004b070((short *)piVar4,(int)lVar5);   /* in-block scan */
    if (psVar7 != (short *)0x0) {
      *param_4 = (longlong)psVar7 + *param_4 + (2 - (longlong)piVar4);  /* size = EOI end */
LAB_18004c56b:
      *(undefined4 *)(param_2 + 0x18) = 0x23;    /* the accept stamp, unconditional */
      return *param_4;                           /* size = the offset reached */
    }
    if (iVar2 == 0xd9) { *param_4 = *param_4 + 2; goto LAB_18004c56b; }
    *param_4 = *param_4 + *(longlong *)(param_1 + 0x50);   /* next sector block */
    uVar6 = *param_4;
  } while( true );
```

The offsets (`*param_4`) are **candidate-relative** (0 = the SOI — the
dispatcher's getter is created with offset 0 at the hit), so the limit
`param_5` = h*w*3 (or 20,000,000) is measured **from the candidate start**.
The getter `FUN_180042930` (`:57942`-region body) is a **sliding window**
(a global 0x81000 buffer, refilled on demand from the device with a
partial-read retry; NULL only on real read failure) — so the scan runs to
the limit or the device end, whichever comes first. **The Exif
(FUN_18004b3b0 :57520/:57544) and generic (FUN_18004c1e0 :57896/:57927)
sub-analysers call the same FUN_18004c480 with the same ladder** — all
three branches accept no-EOI files.

## 4. The two-generation chain (the fallback is live)

- The registry census (`artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md`
  §2-3, §6): "every format has a modern analyser … and — where one exists —
  a legacy fallback `File*Analyser`. When the modern analyser rejects a
  candidate (validation failure), the chain falls to the legacy
  implementation before giving up … one strong validator accepting is
  enough."
- The modern `BruteForce::JpegAnalyzer` ctor `FUN_180033620`
  (`:39277-39304`) **owns** a `FileJPGAnalyser` (`param_1[3] = puVar1`,
  a 0x80-byte object with `FileJPGAnalyser::vftable`); the four JPEG
  magics (0xe0ffd8ff/0xe1ffd8ff/0xdbffd8ff/0xeeffd8ff at `:1458-1572`)
  all register the pair.
- The dispatch loop (`:9843-9916`) walks the registry chain calling each
  generation; either accepting emits the cell.
- The modern analyser is EOI-gated (S-018's INVALID evidence stands for
  THAT tier: the five `-0x27` FF D9 sites and the `if (uVar21 == 0) goto
  epilogue` early-return in FUN_180033a90).

**Verdict: the og recovers tail-truncated JPEGs (SOI+APPn+SOF+SOS, tail
destroyed) through the legacy tier, sized at min(candidate_start + h*w*3 |
+ 20,000,000, device end), stamped as JPEG (0x23) with no reduced grade.**
S-018's salvage claim is REINSTATED with these citations; the core's
validator previously required EOI unconditionally and dropped exactly
these files.

## 5. Question 2 (the continuation model) — residual disposition

The modern analyser's pending-continuation machinery (the early return
when no EOI is in the current buffer; the 0x7800000 cap) governs the
MODERN tier only. With the legacy tier decoded, the salvage question no
longer depends on it: a no-EOI candidate emits in-window through the
fallback at the estimate limit. The remaining continuation nuance (which
size wins when the modern tier eventually finds an EOI beyond the window
AFTER the fallback already accepted at the estimate) is a same-file size
preference the og resolves by first-accept-wins in the dispatch
(:9869-9916: the chain stops at the first accepting generation); the
core's single-pass walk finds the EOI when it exists (the 900-tier
accept) and only falls to the salvage estimate when no EOI exists within
the probe limit — a superset of the og's first-accept preference for
complete files, exact parity for truncated ones.
