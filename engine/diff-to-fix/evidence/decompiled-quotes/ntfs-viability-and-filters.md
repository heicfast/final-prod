# evidence/decompiled-quotes/ntfs-viability-and-filters.md

Exact snippets quoted during the NTFS audit wave (T5). Source repository:
`easeus-drw-research` @ `03ad46c`. Quotes taken from the decompiled corpus by
agent B (session 2-B) and re-verified by the main auditor against the files.

## Q1 — NTFS deleted-record loop prologue: record-size clamp (never rejects)

File: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
Function: `FUN_18001acc0` (deleted-record batch loop)
Lines: ~21130–21137 (agent-B verified read)

```
record_size = device.vt[0x28]();  if < 1 → 0x200 fallback
buf_size = max(0x400, record_size)
```

Artifact cross-cite: `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1.
Used by: M-001.

## Q2 — NTFS deleted-record loop: filters and viability gate

File: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
Function: `FUN_18001acc0`
Lines: ~21143–21260 (agent-B verified read; line refs per B-ntfs.md §3.1)

```
cancel check: controller+0x38 → vt[0x10]                       [:21143]
magic == 0x454c4946 ("FILE")                                   [:21152]
FUN_180041bb0(rec, buf_size) != 0  (fixup)                     [:21153]
FUN_180034b20(rec, buf_size) == 0  (corruption signal)         [:21154]
FUN_180041970(rec) != 0 → skip   ← BASE-RECORD test (CORRECTIONS #2)
FUN_1800431d0(&props, rec, buf_size, rec#, device)             [:21164]
FUN_180043030(&props) == 0 (validate)
FUN_180006db0(rec#, props, sectors_per_rec*recsize, &out, 0x200)
filters (props flags u16@+0x23, name str@+0x11):
   (flags & 4) || name==NULL || name[0]=='$' → Release+skip     [:21186-21194]
if ((flags & 0x10) == 0):
   runs = props.get_runlist() (slot 0xF); size = props.size@+0x1b (slot 3)
   if (0x3ff < size):            ← STRICT (CORRECTIONS #7)      [:21208]
      bitmap = controller+0x40 (fallback +0x48)
      it = FUN_1800340b0(runs); total=0; intact=0
      per run: sector = sectorsPerCluster(controller+0x28) * cluster
               hit = FUN_18002c2e0(bitmap, sector)
                     || secondary: bitmap[0xC] → vt+0x20(sector)
               intact++ on hit                                  [:21216-21231]
      if (total > 0 && intact == total) → props.Release() — DESTROYED  [:21250]
add path (LAB_18001afe6): node flag byte@+0x3d |= 1; list insert at controller+0x18
```

Used by: S-001, I-001, OQ-T1, modules/ntfs.md (B4/A25/A26 verdicts).

## Q3 — Primary bitmap bit test

File: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
Function: `FUN_18002c2e0`
Lines: ~35643–35657 (agent-B verified read)

```
pos = bitmap[10]/*+0x50 base*/ + sector_index
if (bitmap[2]/*+0x10 limit*/ < pos || bitmap[0]/*+0x00 buffer*/ == NULL) return false
return buffer[pos >> 3] & (1 << (pos & 7))
```

Used by: OQ-T1, modules/ntfs.md.

## Q4 — Fixup chain gate (min size 0x400, two-pass validate/restore)

File: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
Function: `FUN_180041bb0`
Lines: ~52513–52535 (agent-B verified read)

```
if size < 0x400 → false
fixoff = FUN_180046160(rec); if (fixoff & 0xFF)==0 → false
ok1 = FUN_180041a20(rec,size)   // apply fixups (walk sectors)
ok2 = FUN_180041370(rec,size)   // USN match per sector
if ok2==0 → false; if ok1 → FUN_180041290(rec,size) (restore); return true
```

Cross-cite: core `cr_fixup.cpp:6–9` documents the og validate loop bound
(`local_18 < 2`, first-two-tails tolerance) which the core replicates
(VALIDATE_TAIL_COUNT=2).

## Q5 — Runlist byte-advance formula (nibble semantics)

File: `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`
Function: `FUN_180003440`
Lines: ~2378–2384 (agent-B verified read)

```
do { len += (run[i]>>4) + 1 + (run[i]&0xF); } while (run[len] != 0);
```

Reading: advance = offset_width(high nibble) + 1 + length_width(low nibble) —
i.e. **low nibble = length-field width, high nibble = offset-field width**.
The corpus prose in `ntfs-engine--deep-dive.md` §7 states the opposite order;
the decompile (and the NTFS spec, and Linux fs/ntfs3 which core cites at
`src/core/ntfs/cr_runlist.cpp:71–75`) agree with the decompile. Core matches
the decompile. Corpus erratum candidate #5 (feed back to corpus owners).

## Q6 — NTFSLib InitMFTMirror (mirror size from runlist)

File: `re-workspace/reversed/ghidra-mo/NTFSLib_decompiled.c`
Function: `InitMFTMirror`
Lines: ~17310–17430 (agent-B verified read; summary quote)

```
mirror_sector = clusters_per_record(+0x24) * mft_start(+0x18/+0x10) + volume_base(+0x38);
override param_3 != -1 replaces it;
__InitMftRunList fails → fallback reads MFT record 1 ($MFTMirr) via ReadFileRecord,
walks its attributes for unnamed non-resident $DATA (0x80),
"...$DATA is resident!" → abort;
GetDataRunList → runlist at this+0x60;
mirror size this+0x70 = (last_run.offset + last_run.length) * factor / sectors;
this+0xD8 = 1;
```

Used by: N-001.
