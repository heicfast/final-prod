# s6-health-grading-decode — the DiskHealth.exe grading engine, decoded first-hand (S-024 upgrade)

Session-6 fix leg (the "fix ALL findings" continuation). Artifact:
`re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c` @ corpus
`03ad46c` (93316 lines). Every line number below was read directly from
that file during this session; this decode SUPERSEDES the analog-based
statements in the first S-024 landing (see the Resolution addendum in
`findings/FIXED/S-024-health-grading-layer-missing.md`).

## 1. The two-level flow

- **Disk grade**: `FUN_0040cdf0(this, disk_index)` :5755-6130 returns
  0/1/2/3 (Unknown/Good/Caution/Bad); the caller stores it at
  disk+0x1800 (`:26960-26962`, `:27143-27144`) after the SMART refresh
  (case 0xe / case 0x16 paths of the update dispatcher).
- **Per-attr display grade**: the rule chain at `:34148-34167` (also
  `:37400-37420`, the JSON-export variant): `FUN_00458350` → (only if
  attr ∉ {194, 184}) `FUN_004584b0` → `FUN_004580f0` → `FUN_004582c0`
  → `FUN_00458010` → default `status = 1` (Good).

## 2. Status codes

`{"Unknown", "Good", "Caution", "Bad"}` QList built at :37087-37100;
grade index stored at disk+0x1800 selects the string.

## 3. Per-disk defaults (:37004-37019, repeated at :38243-38246)

- `flag@0x1724 == 0 → temp-caution 0x32 (50)` else `0x3c (60)` —
  **but 0x1808 (the temp-caution field) is NEVER READ by the grader**;
  grep shows only writers (:37004/:37007, :38243/:38246) and
  copy-constructors. Temperature (derived at 0x17bc, `:9714-9745`)
  feeds DISPLAY only (`:35903` C→F conversion for the tray). **The disk
  grader has no temperature branch; temp attrs display Good (:33596,
  `case -0x3e/-0x48 → *param_1 = 1`).**
- caution counts `0x182a/0x182c/0x182e = 1` (attrs 5/197/198),
  SSD-life caution `0x1830 = 10`.

## 4. is-SSD flag (0x1724) — writer decoded (:6300-6344)

Model string (CStringT at disk+0x1884): prefix `OCZ` / `SPCC` /
`PATRIOT` / `PHOTOFAST` / `STT_FTM` / `Super Talent` (Find==0), or
contains `Solid` / `SSD` / `SiliconHardDisk` (Find≥0) → `0x1724 = 1`.
Consequences: temp default 60 instead of 50; the 5/197/198 rule and
its caution counts apply only when `0x1724 == 0` (HDD). Notably
`"KINGSTON SV300S37A"` matches nothing → HDD-class grading even though
the drive is an SSD (og-faithful).

## 5. Vendor family (0x181c) — FUN_0040dad0 cascade :6300-7000

Family stores with their og parser names (all read at the store site):

| code | name | gate (decoded) |
|---|---|---|
| 0xb | SmartWdc | model prefix `"WDC "` :6347-6359 |
| 0xa | Seagate/IronWolf | FUN_004241f0 :24291+: IronWolf attr sig {1,5,9,12,100,0x66,0x67,0xAA,0xAB,0xAC,0xB3,0xB5,0xB7,0xB9,0xBB} (sub 4) :24312; Seagate model branches :24312+ |
| 2 | SmartMtron | `"MTRON"` prefix OR attr[0]==187 with attr count 1 :6363-6373 |
| 4 | SmartJMicron60x | attr sig {1,9,12,0xC2,0xE5,0xE8,0xE9} :6401-6410; JMicron61x at :6984; the 66x sig {1,2,3,5,7,8,9,10,12,0xA7,0xA8,0xA9,0xAA,0xAD,0xAF} + `"ADATA SU700"` :6396-6400 sets ONLY the parser name (no family) |
| 5 | SmartIntel | `"INTEL"` contains OR attr sigs :6438-6449 |
| 6 | SmartSamsung | FUN_00423bd0 :24090-24142: `"SAMSUNG"` / `"MZ-"`, sub 4 |
| 8 | SmartMicron | FUN_00423030 :23582+: MICRON M500/M510/M550/M600/MU01/1100/1300, BX100-500SSD |
| 0x12 | SmartMicronMU02 | FUN_00422cc0 :23406+: CRUCIAL/CT/C300-C600/M3--M6-/M300-M600/P300-P600 |
| 7 | SmartSandForce | `"SandForce"` contains OR attr sig {1,5,9,12,13,100,0xC0} :6470-6499 (also sets 0x16f0=1) |
| 9 | SmartOcz | uppercased model: OCZ prefix AND (OCZ-TRION prefix OR sig {1,3,4,5,9,12,0xE8,0xE9}) :6530-6557 |
| 0xe | SmartOczVector | OCZ-class sig {5,9,12,0xAB,0xAE,0xC3,0xC4,0xC5,0xC6} :6560-6605 |
| 0xc | SmartPlextor | sig {1,5,9,12,0xB1,0xB2,0xB5,0xB6} :6640-6651 |
| 0xd | SanDisk | FUN_00423e10 :24143-24244: SD5/X110/X300/X400/X600/SDSSDP/SDSSDRC/SSD U100/U110/G1001/iSSD P4/pSSD/2280; forms: G1001→0x17f4=1 (sub 4); X/SD5-class→0x17f0=1 (sub 4); U-class/SDSSD*→0x17ec=1 (sub 4); generic SanDisk→0x17ec=1 |
| 0x15 | SmartSKhynix | SHG/HFS/TND/MND/TNF/SC311/SC401 branches :6714-6772; HFS/TND/MND/TNF-class → sub 4 + 0x17e4=1; else-class → sub 4 + 0x17e8=1; SC311/SC401 → sub 1 (name anchored at :6895-6902) |
| 0x11 | (Kingston) | FUN_00422910 :23260-23341: KINGSTON/DC500/KC600/SA400/SEDC400/SKC310/SKC400/SM2280/SUV300/SUV400/SUV500/SHSS; 0x17e4=1, sub 3/4 (family code positionally anchored at :6907) |
| 0x16 | SmartKioxia | `"KIOXIA"` contains :6886-6893, sub 3 |
| 0x1b | SmartPhison | attr sigs {1,9,12,0xA8,0xAA,0xAD,0xC0,0xB5,0xDA,0xE7,0xF1} (+variant) :6788-6812, sub 2/4 by firmware `"S9"` at disk+0x187c |
| 0x19 | SmartApacer | :6856 (+helper window :23073: sub 1 + 0x17e4=1) |
| 0x1d | SmartMaxiotek | :6864 |
| 0x1c | SmartMarvell | :6872 |
| 0x1a | SmartSiliconMotion | :6881 |
| 0x1e | SmartYmtc | :6849 |
| 0x17 | SmartSsstc | :6921 |
| 0x18 | SmartIntelDc | :6960 |
| 3 | SmartIndilinx | :6969, also 0x16ec=1 (attr-grade skip flag) |
| 0x13 | SmartNVMe | NVMe enumeration ("NVM Express") :5645-5660 |
| 0 / 1 | Smart / SmartSsd | fallbacks :6826-6833, :6998 |

**Residual (not decoded this session):** the exact model-string
conditions inside FUN_00422b60/FUN_00422c30/FUN_00422470/FUN_00424960/
FUN_004224e0/FUN_00424430 (overlapping windows; families for the
Micron-tail/Kingston-adjacent/LiteOn/ADATA branches). Those vendors
currently fall to the generic 0/1 in our classifier; the ENGINE
consumes every family code exactly, so the residual is a pure
classifier-data gap (see S-024 Resolution addendum).

## 6. Disk grade semantics — FUN_0040cdf0 :5755-6130

1. family 0x13 (NVMe) path FIRST :5795-5810: model ==
   `"Parallels Virtual NVMe Disk"` → Good; **attr[0].raw[0] != 0 →
   Bad** (the NVMe critical_warning byte, converted at :26985+: page+0
   → attr0.raw[0]; temp = Kelvin−273 :26987-26990; life =
   100 − percentage_used :26999-27005 with clamp ≥ 0); life > 10 →
   Good; life == 10 → Caution; life < 10 → Bad. **Media errors are
   never read by the grader.**
2. Unknown gates :5843-5852: `0x16d8 == 0` → Unknown;
   `0x1724 == 0 && 0x16dc == 0` → Unknown; `0x16f8 != 0` → Unknown.
3. Per-attr loop with `bad_count` (local_1c), `caution` flag (bVar5),
   `graded` (bVar4, inverted sense):
   - duplicate attr id → return Unknown :5826-5833
   - SandForce attr-1 all-zero row skips the threshold block :5888-5896
   - attr 194 skips the threshold block entirely :5897
   - SSD (0x1724≠0): skip flag `0x16ec ≠ 0` → skip; else **any attr**
     with threshold ≠ 0 and value < threshold → bad_count++ :5899-5906
   - HDD: same rule but only for attrs in
     {1..13, 187..189, 191..193, 195..209, 211..212, 220..228, 230,
     231, 240, 250, 254} (the range test :5916-5935 inverts to exactly
     this set — attr id 0 excluded)
   - graded signal :5947: `is_ssd && threshold != 0`
   - attrs 5/197/198 :5949-5974: if raw[0..3] NOT all 0xFF and
     caution-count ≠ 0 and count ≤ raw16 and NOT is_ssd → caution
     flag; `!is_ssd` → graded
   - SSD-life family gates (attr → families, :5977-6084): 169 →
     {0x14, 0x1a, 0x11&sub==3}; 173 → {0xf, 0x16}; 177 → {6};
     187 → {2}; 202 → {8, 0x12, 0x18}; 209 → {3}; 231 → {7, 0x10,
     0x11, 0x15, 0x14, 0xd, 0x17, 0x19, 4, 0xa, 0x1b, 0x1d, 0x1e};
     232 → {0xc}; 233 → {5, 9, 0xe, 0x15} OR 0x17f4≠0
   - SSD-life value (LAB_0040d4d5 :6006-6023):
     `0x17e8≠0 → life = 100 − raw[0]`; else `0x17e4≠0 → life =
     raw[0]`; else `life = value`; clamp [0, 100] (≥101 → 100);
     `life == 0` → bad_count = 1 (:6100); else if
     (`0x17e4≠0 || threshold ≤ life`): `0x1830 < life` → healthy;
     else caution; else bad_count = 1
   - WDC/SanDisk attr-230 (:6084-6110): used = `0x17ec≠0 ?
     raw16/100 : raw[1]`; life = clamp(100 − used); 0 → bad_count = 1;
     `0x1830 < life` → healthy; else caution
4. Aggregation :5825-5831: bad_count > 0 → Bad; graded → (caution ?
   Caution : Good); else Unknown.

## 7. Per-attr rules

- `FUN_00458350` :33548-33583 (5/197/198, **HDD only**): threshold ≠ 0
  && value < threshold → Bad; caution-count ≠ 0 && count ≤ raw16 →
  Caution; else Good. No all-FF guard here (disk-side only) — the
  asymmetry is pinned by test.
- `FUN_004584b0` :33607-33632 (attr 230, family 0xb/0xd): life =
  100 − (0x17ec? raw16/100 : raw[1]); 0 → Bad; ≤ 0x1830 → Caution.
- `FUN_004580f0` :33469-33521 (SSD-wear family attrs, same gates as
  §6): `0x17e4≠0` → Good; else `value ≠ 0 && threshold ≤ value` →
  (0x1830 < value ? Good : Caution); else Bad.
- `FUN_004582c0` :33523-33545 (attr 1, family 7 only):
  `(value==0 && raw[0]==0 && raw[1]==0) || threshold==0 || threshold ≤
  value` → Good else Bad.
- `FUN_00458010` :33419-33446 (general): for HDDs (or SSDs with
  0x16ec) attrs in the §6 special set are NOT handled here; others:
  threshold == 0 → Good; threshold ≤ value → Good; else Bad.
- attrs 194/184 → Good (:33596 region).
- Chain gate :34129-34138: `0x16d8==0 || 0x16dc==0 || 0x16f8≠0` →
  per-attr Unknown.

## 8. 0xD1 READ_THRESHOLDS (:14108-14118)

`FUN_0041a160`: ATA task {0xB0 cmd, 0xD1 feature, 0x0E count} → 512B
at disk+0x1200, then `FUN_00414230` mirrors per-attr thresholds to
disk+0x1569+idx*0xc (byte 0 of a parallel 12-byte row table). The
public 0xD1 page layout (id, threshold@+1, reserved) is equivalent;
the landed core reads threshold at entry+1 — verified consistent.

## 9. NVMe conversion (:26985-27010)

page+1/+2 (Kelvin LE) − 273 → temp (0x17bc, display); page+5
(percentage_used) → life 0x17e0 = 100 − used, clamp ≥ 0; page+0 →
critical_warning byte (attr-0 raw analog; graded by §6.1).
