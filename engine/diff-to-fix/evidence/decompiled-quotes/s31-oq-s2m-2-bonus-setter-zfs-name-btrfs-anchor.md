# s31 — OQ-S2M-2 resolution quotes: the +0x68 bonus setter family, the ZFS "pool name", the Btrfs conversion anchor

> Session DFX31 (tracked-open HEAD lane), corpus `03ad46c` unmoved, all quotes
> read first-hand this session from `re-workspace/reversed/ghidra-searchdll/`
> and `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`.

## 1. The +0x68 bonus setter — fully located (the OQ's "not yet located" is closed)

The field is set by **scan-path composers in each SearchDLL**, not by the
serializer (the serializer member fn at `NTFSSearchDLL_decompiled.c:45692-45709`
copies `param_1 + 0x68,8` verbatim; the ctor `FUN_180039360`/`FUN_180027ed0`
family defaults it to `param_1[0xd] = 1`).

### 1a. FAT — the two-factor ladder (the "FAT32 20" decoded)

`FATSearchDLL_decompiled.c:6946-6969` (in `FUN_180009cb0`; identical second
site `:7142-7167`):

```c
        if ((uVar19 & 0x140) == 0x140) {
          lVar26 = 0x32;                      /* 50: both boot bits */
        }
        else {
          lVar26 = 0x2d;                      /* 45: one boot bit */
          if ((uVar19 & 0x140) == 0) {
            lVar26 = 10;                      /* 10: no boot bits */
          }
        }
        if (*(int *)(ppppppppppppplVar3 + 1) == 2) {   /* +8 == 2 → FAT32 */
          uVar19 = uVar19 & 0xc;
          if (uVar19 != 0xc) { lVar18 = 0x2d; goto LAB_180009f1f; }  /* 45 */
          lVar18 = 0x32;                      /* 50: both fs bits */
        }
        else {
          uVar19 = uVar19 & 3;
          lVar18 = 0x32;                      /* 50: both fs bits */
LAB_180009f1f:
          if (uVar19 == 0) { lVar18 = 10; }   /* 10: no fs bits */
        }
        ppppppppppppplVar3[0xd] = (longlong ************)(lVar18 + lVar26);
        /* ^ +0x68 = boot_quality + fs_quality, each ∈ {10, 45, 50} */
```

`uVar19` is `*(uint *)(ppppppppppppplVar3 + 0x14)` — the evidence-flags u32 at
**+0xa0** (13-star pointer type → 0x14×8 byte offset; ctor zeroes +0x9c..+0xa3).
`ppppppppppppplVar3[5]` (+0x28) is the file counter (`:5984` `+0x28 += 1` when a
file is found, `:5983` `+0xa0 |= 0x10` "has files"); the early-out at `:6944`
skips the ladder for file-less no-bit candidates.

**The flag producers (first-hand):**

- `FATSearchDLL_decompiled.c:6715-6719` (`FUN_180009960`, the FAT-candidate
  creator, flavor `param_3[1]`: 0=FAT12/1=FAT16/2=FAT32):

```c
    lVar7 = *param_3;
    ...
    local_res8 = 8;
    local_res10 = (uint)param_4 * 4 + 4;   /* FAT32: bit2 always, bit3 iff param_4 */
```
  with the FAT12/FAT16 arms at `:6693` `local_res10 = (uint)(param_4 == 0);`
  (bit 0) and `:6704-6707` `local_res10 = 2; if (param_4 != 0) local_res10 = 0;`
  (bit 1); the bits land in the flags via `:6771`
  `*(uint *)(puVar8 + 0x14) |= local_res10;`.

- `param_4` is the **FAT32 backup-boot-sector validation verdict** —
  `FATSearchDLL_decompiled.c:8004-8052` (the boot-sector classifier):
  `bVar6 = 0` initially; for `local_30 == 2` (FAT32) only, it reads the sector
  at `param_3 + 6` (the BPB +0x1C backup-boot offset), re-parses its BPB
  (`FUN_18002f970(&local_28)`), and checks the FAT32 media-descriptor family
  (`0xfffffff8` / `0xfffffff0` / FSInfo-signature shapes
  `DAT_1800704a0/a4/a8`); only if those pass → `bVar6 = 1`, then
  `FUN_180008d50(param_1, lVar5, &local_38, bVar6)` → `FUN_180009960(...,bVar6)`.

- The `& 0x140` boot pair: bit 6 (0x40) is set on one creation path
  (`:9243-9251`, the path that stamps `*(undefined4 *)(puVar10 + 1) = 2` —
  FAT32); bit 8 (0x100) has **no `| 0x100` store anywhere in the 64,658-line
  decompile** (grep-verified this session: the only 0x100 hits are memset
  sizes/geometry constants) — the 50-tier of boot_quality is unreachable in
  this build.

**The observed "FAT32 20" (CalcPartition log) = `10 + 10` — both ladders at
their minimum tier**, for candidates whose creation path set no ladder bits
(e.g. the `:5322-5324` creator which only sets `| 0x10`, a bit in neither
mask). There is no "20" constant anywhere (grep `+ 0x14)` / `= 0x14`
accumulator patterns: zero hits — session-4's negative re-confirmed and now
explained).

### 1b. exFAT — the same ladder, different bit positions

`exFATSearchDLL_decompiled.c:39504-39526`:

```c
    uVar13 = *(uint *)(plVar2 + 0x1c) & 0xc0000000;    /* bits 30/31: boot */
    if (uVar13 == 0xc0000000) { lVar18 = 0x32; }
    else { lVar18 = 0x2d; if (uVar13 == 0) { lVar18 = 10; } }
    uVar13 = *(uint *)(plVar2 + 0x1c) & 3;             /* bits 0/1: fs */
    if (uVar13 == 3) { lVar8 = 0x32; }
    else { lVar8 = 0x2d; if (uVar13 == 0) { lVar8 = 10; } }
    plVar2[0xd] = lVar8 + lVar18;                      /* +0x68 */
```

### 1c. NTFS — the composite (base flags score + boot validation + stitching)

- **Base score** `NTFSSearchDLL_decompiled.c:10538-10573` (`FUN_18000dc70`,
  the candidate processor): flag bits at `+0x9c` —
  `bit0 → +0x1e(30)`, `bit1 → +10`, `bit2 → +0x1e(30)`, `bit3 → +10`,
  `bit4 → +10`, `bit6 → +10`, `bit7 → +10`; plus three clamped-10 terms from
  `puVar3[0x18]` (`(v+0xff)>>8*10`, clamp 10), `puVar3[0x16]` (`/10000*10`,
  clamp 10), `puVar3[0x17]` (`/5*10`, clamp 10) — the overlap-map products
  computed at `:10491-:10537` (`FUN_18002bc40` against the known-partition
  interval maps at `param_1+0xe8/+0xf0`); then
  `puVar3[0xd] = lVar11 + lVar9 + lVar24 + lVar10;` (overwrites the ctor's 1).
- **Boot-sector validation** `:7229-7233` (`FUN_1800097d0`, called at
  `:10574` right after the base score): reads 1 sector at the candidate's
  `+0x10` offset, validates the NTFS BPB (`FUN_18003f6d0` — the 0x10-0x15
  zero-bytes + BPB checks):

```c
  if (*(longlong *)(param_2 + 0x10) < 0) {
    *(undefined8 *)(param_2 + 0x68) = 0xfffffffffffffff6;   /* -10: unreadable */
  }
  ...
  if (*(longlong *)(param_2 + 0x10) < 0) {
    *(longlong *)(param_2 + 0x68) = *(longlong *)(param_2 + 0x68) + 0x32;  /* +50 */
```

  i.e. a **negative relative offset** (the backup-boot anchor): unreadable →
  the score is hard-overwritten to −10; validated → **+50**.
- **MFT-run stitching bonuses** (`FUN_180023cb0`): `:28944-28945`,
  `:28972-28973`, `:29002-29003` — `+0x68 += 0x1e` (30) per successfully
  merged run-list fragment (each paired with the `FUN_180024a40` extent-merge
  at `+0x130/+0x138`); `:28995-28997` the same +30 through the generic adder
  `FUN_180024990(pvVar16, pTVar17, plVar29[6], 0x1e)`; `:28803-28806` the
  one-shot `+= 0x32` (50, latch at `+0x178`) when the chain walk's
  miss-counter `local_res20 > 3` — stitching that survived a >3-link gap;
  `:29262-29264` the generic adder body
  `*(longlong *)(param_1 + 0x68) = *(longlong *)(param_1 + 0x68) + (longlong)param_4;`.

**Consequence for M-017 (landed, verified):** the core's `fs_bonus()` — NTFS
60/30 by probe strength, FAT 20 flat, others 0 — reproduces the observed
CalcPartition rows (187 rows, 6-decimal verification). The decode above now
grounds those numbers: 60/30 are generator outputs of the NTFS composite;
FAT 20 is the two-factor ladder's minimum tier. The core's single-probe
architecture maps to the 20-tier path; a tiered port would require the og's
internal creation-path identity, which the IPC surface does not carry — the
landed flat model is the honest parity (changing it would break the verified
log-row parity). No code change.

## 2. The ZFS "pool display name" — there is none; FS 13 renders as "Unknow"

- `ZfsSearchDLL_decompiled.c`: `"name"` — **0 hits**; `"pool"` — **0 hits**
  (re-verified this session; 45,364 lines). The engine reads
  `"vdev_tree"`/`"id"`/`"root_dataset"` only (`:19934`/`:19983` — ZAP lookup
  keys, quoted in OQ-T6).
- `DRWUI_decompiled.c:842335-842396` (`FUN_140510080`, the FS-code → name
  builder): cases 0=FAT12, 1=FAT16, 2=FAT32, 3=NTFS, 4=EXT2, 5=EXT3, 6=RAW,
  7=EXFAT, 8=HFSPLUS, 9=EXT4, 10=REFS, 0xe="Unknow" — **no case 13 (ZFS)** →
  the default stands:

```c
  FUN_140056880(param_1,"Unknow",6,param_4,0,0xfffffffffffffffe);
  switch(param_2) { case 0: ... pcVar1 = "FAT12"; ... case 10: pcVar1 = "REFS"; break;
    default: goto switchD_1405100e4_caseD_b;      /* ZFS 13 → "Unknow" */
    case 0xe: uVar2 = 6; pcVar1 = "Unknow"; }
```

- Consumers (first-hand): `:207358` the XML report's `<filesystem>` field
  (`FUN_14006da00(local_338,L"filesystem")` right after the
  `FUN_140510080` result is stored) and `:845776` the CalcPartition log row
  (`FUN_14050fa20("result %p FileSystem (%d) StartSector:%I64d ...")`).
- `DRWUI.exe/strings_ascii.txt:39717` `"ZFS partition"` is the **GPT
  type-GUID table name** (GUID `516E7CB8-6ECF-11D6-8FF8-00022D09712B`,
  between "Vinum volume manager partition" and the HFS+ GUID row) — a
  partition-type display, not a volume/pool label.
- **Verdict:** the og never reads, transports, or displays a ZFS pool name.
  The ZFS volume's FS-type name renders as "Unknow"; the pool label nvlist
  `name` is dead on disk as far as this product is concerned. The core's
  label-`name` read for the pool display (`cr_zfs_engine.cpp:1014`) stays a
  documented superset (more information than the og shows its user — never a
  divergence in the wrong direction). Registered as deliberate-superset note
  in `modules/zfs.md` (this session).

## 3. The Btrfs name-conversion anchor — the DRWUI render layer, CP_UTF8, dwFlags=0

- `BtrfsSearchDLL_imports.txt`: `MultiByteToWideChar` / `WideCharToMultiByte`
  / `mbstowcs` — **0 hits** (re-verified this session). The engine is
  byte-oriented end-to-end; names cross the IPC seam as raw UTF-8 and land
  raw in the `.rsf` session (`rsf-format--scan-session.md:326`
  `char name[name_len]`).
- The conversion happens **at the DRWUI layer**, CP_UTF8 with **dwFlags=0**:
  - `DRWUI_decompiled.c:264664-264689` (`FUN_140163af0`, std::string →
    std::wstring): `MultiByteToWideChar(0xfde9,0,param_2,local_58,...)` —
    the two-call size-then-convert pattern; invoked on the restore path at
    `:357529` (the same site N-029/N-016a's landing already cites —
    convergent evidence, re-read first-hand this session).
  - `DRWUI_decompiled.c:190981-190997` (`FUN_1400d9ef0`): the
    length-explicit `MultiByteToWideChar(0xfde9,0,param_1,(int)param_2,...)`
    heap-buffer converter; ~10 call sites (`:191027`, `:731316`, `:868291`,
    `:874388`, `:894090/:894132/:894173` with mode args 1/3/5/9 — the
    per-field name-conversion family).
- dwFlags=0 → Windows' default replacement policy (U+FFFD per maximal
  subpart) — the exact semantics the core's `text::utf8_to_u16` implements
  post-N-029, and the same conversion the og's ZFS engine does in-engine
  (`ZfsSearchDLL_decompiled.c:24985/:25014`, also re-verified this session).
- **Verdict:** S-005 parity holds for Btrfs (byte-oriented engine →
  render-time CP_UTF8/flags-0 conversion); the predicted residuals N-016a
  (replacement-count policy) and N-016b (raw-bytes persistence) are already
  FIXED (N-029 + M-010). No code change.
