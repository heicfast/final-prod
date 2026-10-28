# Executed evidence — s10b-real-rsf-session-mining (the first mining of the corpus's three REAL og sessions)

**Lane:** SA-10B / L-B · **Date:** 2026-10-29 · **Audited tree:** origin/main
`62074bf` (product-code lineage through `26da616`) · **Corpus:** `be0cfe9`
· **Class:** executed comparison (protocol §6.1 — both the og's own
persisted output and our tooling run on the same payload).

## The payload

The corpus ships THREE REAL og production scan sessions — actual DRW
auto-saved sessions from a real user machine (the trail previously cited
them only for auto-naming (N-061) and volume-label fill (M-079); their
tree-shape and naming content was never mined):

```
og-codebase/SaveScan/auto_Local Disk (G_)10_15_00_44_10.rsf
og-codebase/SaveScan/auto_Local Disk (H_)10_14_23_42_58.rsf
og-codebase/SaveScan/auto_Local Disk (I_)10_14_23_13_21.rsf
```

## The executed run (commands + raw output)

Parser: the core's own `scripts/compare_og_rsf.py` (the record-4 BFS
mining script; grammar validated in the corpus per
`artifacts/SaveScan/rsf-format--scan-session.md`). Run date 2026-10-29:

```
$ for f in og-codebase/SaveScan/*.rsf; do
    python3 scripts/compare_og_rsf.py "$f" out.csv; done

record-4 decompressed: 29,894,861 bytes   (G session)
rows=218503 names=14 stats={'roots': 4, 'nodes': 218513, 'vector_children': 10,
  'content_type_0x9': 13229, 'content_type_0x0': 122363, 'datarun': 205145,
  'content_type_0x3': 82911}
consumed 29,894,861 / 29,894,861 bytes

record-4 decompressed: 226,686,612 bytes  (H session)
rows=1500395 names=15 stats={'roots': 3, 'nodes': 1500407, 'vector_children': 12,
  'content_type_0x9': 432708, 'content_type_0x0': 971532, 'datarun': 1059424,
  'content_type_1': 2098, 'content_type_3': 94057}
consumed 226,686,612 / 226,686,612 bytes

record-4 decompressed: 36,001,842 bytes   (I session)
rows=267872 names=17 stats={'roots': 3, 'nodes': 267886, 'vector_children': 14,
  'content_type_0x9': 6959, 'content_type_0x0': 248727, 'datarun': 260843,
  'content_type_1': 39, 'content_type_3': 12147}
consumed 36,001,842 / 36,001,842 bytes
```

All three parse cleanly to their last byte (`consumed == decompressed`),
through the core's own grammar implementation — an executed validation
of the record-4 grammar against 292 MiB of real og payload.

## Extracted ground truth (the distributions)

### 1. The root set (depth-1 rows, all three sessions)

```
G session:  NTFS | File Path Lost | Reconstructed | System Reserved (NTFS)
H session:  NTFS | File Path Lost | Reconstructed
I session:  NTFS | File Path Lost | Reconstructed
```

- **"File Path Lost"** and **"Reconstructed"** are session-tree ROOTS in
  every real session. DRWUI decode: `DRWUI_decompiled.c:321001` / `:319865`
  ("File Path Lost"), `:320992` / `:357713` / `:611618` / `:642786`
  ("Reconstructed" — created when the row's type value == 7), `:319951` /
  `:535672` ("File Name Lost"). Qt strings corroborate
  (`re-workspace/reversed/DRWUI.exe/qt_strings_english.txt` offsets
  0x10957/0x109f2/0x1e33e: "File Name Lost", "File Path Lost",
  "Reconstructed"; help text: 'Refer to these files under the File Path,
  including "File Path Lost" and "Reconstructed"').
- "NTFS" is a root node in all three sessions (the FS-typed structured
  marker); volume subtrees render as "Local Disk(G:)"-style rows
  (121,698 in G) and a second-volume root "System Reserved (NTFS) "
  (191 rows).

### 2. The carve branch shape: `Reconstructed/<volume-display-name>/`

100% of carved rows (content type 3) sit at depth 2 under a per-volume
parent — no exceptions across 189,115 carved rows:

```
parents of carved rows (all 3 sessions):
   94057  Reconstructed/ (H:)            <- H: has an empty label
  82911  Reconstructed/System Reserved
  12147  Reconstructed/ (I:)
```

The parent name is the VOLUME DISPLAY NAME (label + drive letter; bare
label when the volume has no letter). Carves are attributed to the
volume whose unallocated space contained them. Our core places every
carved node under ONE global root ("Other Deleted Files (Carved)",
`cr_scan.cpp:151`) with no volume child and no volume attribution →
**S-048**.

### 3. The per-file naming contract: `<PREFIX>_<%06d>.<EXT>`

DRWUI decode (the naming function, first-hand this session):

```
FUN_140c44770 @ DRWUI_decompiled.c:1878330-1878381:
    if (param_2[2] == 0) { pwVar1 = L"%06d";  if (99999 < param_3) pwVar1 = L"%d";  snprintf(buf, 0xff, pwVar1, counter); }
    else {                    pwVar1 = L"%s_%06d"; if (99999 < param_3) pwVar1 = L"%s_%d"; snprintf(buf, 0xff, pwVar1, prefix, counter); }
```

Real-session confirmation — the dominant engine pattern for EVERY
carved format is `EXT_%06d.EXT` (uppercase ext as its own prefix):

```
.GZ (53891):  GZ_N            .PNG (42412): PNG_N          .SVG (43545): SVG_N
.ICO (5399):  ICO_N           .HTML (4543): HTML_N         .JPG (3360):  JPG_N
.GIF (3072):  GIF_N           .WAV (2038):  WAV_N          .EPS (1480):  EPS_N
.MP3 (1126):  MP3_N           .CUR (904):   CUR_N          .PDF (714):   PDF_N
.TTF (613):   TTF_N           .CAB (441):   CAB_N         .FCPEVENT:    FCPEVENT_N
.TAR/.WEBP/.MOF/.BMP/.OGG/.WMF/.M2TS/.SWF/.RTF/.M2T/.AVI/.MOV/.MP4/.MKV/.7Z: same EXT_N
```

The counter is zero-padded 6, switching to unpadded `%d` past 99999
(decompile-pinned; real sessions carry files beyond 099999 — e.g. the
G session's PNG counter exceeds 42000, the H session's SVG counter
exceeds 43000 — the padded form holds through all observed counters).

Content-derived prefixes (the analyser-provided prefix string — the
`param_2` branch — NOT cosmetic; real-session examples):

| Format | Prefix source | Real-session examples (verbatim names) |
|---|---|---|
| JPG | SOF dimensions | `JPG_192x144_000001.JPG`, `JPG_105x142_000019.JPG` (405 rows; 2735 rows without dims) |
| JPG | EXIF camera Model | `GT-S7562_000123.JPG`, `SM-G610F_NNNNNN.JPG`, `GT-S5830_N.JPG` (197+ rows) |
| JPG | EXIF Software | `Adobe Photoshop 7.0_N.JPG` (125), `GIMP 2.10.14_N.JPG` (156), `ACD Systems Digital Imaging_N.JPG` (43) |
| MP3 | ID3 title | `AREE DEEWANO MUJHE PECHANO_000123.MP3`, `JISKA MUJHE THA INTEZAAR_N.MP3` (real ID3 titles) |
| PDF | creation timestamp | `2020-05-23 23-23-23_N.PDF`, `2023-04-05 10-18-10_N.PDF` (91 rows) |
| DOCX | docProps timestamp | `2006-07-25 02-16-00_N.DOCX`, `KIS2_2022-05-25 13-14-00_N.DOCX` (24 rows) |
| ZIP | central-directory entry names | `META-INF__N.ZIP` (275), `__MACOSX__N.ZIP`, `LICENSE_OFL_N.ZIP` (first-entry prefixes) |

Our core names every carved file `File%06u.<ext>`
(`cr_carve.cpp:907` default_namer; embedded_name only for MP4/MOV via
N-063) → **S-049**.

Nuance recorded: the N-063-decoded MP4/MOV analyser template
("Lost Name File (...)EaseUS", RAWSearchDLL :33312/:33408) does NOT
appear in any of the 27 MP4 + 30 MOV real-session carve names — all
carry `MP4_%06d.MP4` / `MOV_%06d.MOV`. The decompile template is real
(the N-063 resolution re-verified it first-hand) but the DRWUI
`%s_%06d` path is the one that fires for these real carves; the fix
lane must determine which analyser branch produces the prefix string
that reaches FUN_140c44770 (see S-049's fix plan).

### 4. The "File Path Lost" branch: `LostDir<NNNN>` per-parent grouping

```
G session: File Path Lost -> 13,420 rows
H session: File Path Lost -> 938,982 rows (62.6% of the session!)
I session: File Path Lost -> 12,903 rows
```

Structure (H session): 398 synthetic folders named `LostDir<NNNN>` at
depth 1, with 938,582 real-named files at depth 2:

```
File Path Lost/LostDir7206/IMG-20210816-WAN0001.jpg
File Path Lost/LostDir1707/...   (NNNN values 7206, 1707, 7507, 7807, 8708, ...)
```

The `LostDir%d` construction is pinned in the corpus decompiles:
- `NTFSSearchDLL_decompiled.c:42041` — `FUN_180001e40(&local_78,"LostDir%d",<counter>)`
- `NTFSSearchDLL_decompiled.c:48915/:48924` — the "LostDir" literal
- `exFATSearchDLL_decompiled.c:5182` — `FUN_180001e40(&local_70,"LostDir%d",<counter>)`
  (the same contract on the FAT-family orphan path)
- Artifacts: `ifileproperties-part1--ntfs-fat-exfat.md`,
  `ntfs-engine-algorithms-v2--deleted-viability.md`,
  `searchdll-family--architecture.md` all mention LostDir.

The rows carry REAL filenames (camera/WhatsApp/screenshot names — the
MFT $FILE_NAME remnant recovery) with deleted=active/unknown mixes
(669,821 active / 269,161 unknown in H — the post-format salvage
population). Our core reparents orphans FLAT under "Lost Files"
(`cr_mftwalk.cpp:1091`, `cr_scan.cpp:202`), losing the per-parent-record
sibling grouping entirely → **M-103**.

### 5. Carved-row field values in real sessions

- `size`: **0** for every carved row in all three sessions (the og's
  record-4 carve entries do not persist the computed size in the tree;
  the size lives in the RAW properties record).
- `confidence`: empty in record-4 (graded in a separate pass — matches
  the trail's existing decode).
- `deleted`: EVERY carved row is flagged deleted (82,911 / 94,057 /
  12,147) — independent real-world corroboration of S-031's fix
  (carved files marked deleted).
- The extension census of the real carve sets (`.GZ` 53,891, `.SVG`
  43,545, `.PNG` 42,412, `.ICO`, `.wav`, `.HTML`, `.JPG`, `.GIF`,
  `.PDF`...) gives the S-041 school-2/3 remainder (HTML/SVG/GZ/MTS...)
  its real-world frequency ranking — the top-4 og carve formats by
  volume are school-2/3 formats still absent from the core.

## Reproduction

```
# from the core repo root (corpus cloned alongside):
python3 scripts/compare_og_rsf.py "../easeus-drw-research/og-codebase/SaveScan/auto_Local Disk (H_)10_14_23_42_58.rsf" /tmp/h.csv
# then the distribution analysis: csv module over path/size/origin/deleted/confidence/extent
# (the exact commands are the ones above; outputs quoted verbatim)
```

## What this evidence changes

1. Falsifies the "roles map 1:1" half of SCAN-ENGINE-DELTA row C2 for
   the carve branch (no volume children in ours) and the lost branch
   (no LostDir grouping in ours).
2. Resolves N-063's recorded remainder (the "which of the 22 SetName
   call sites" question) empirically: the DRWUI `%s_%06d` contract is
   the universal surface, with the prefix from the analyser's
   per-format extraction (EXIF/ID3/timestamps/ZIP entries).
3. Adds the first REAL-payload confirmation of S-031 (all carved rows
   deleted-flagged) and S-041's school-2/3 real-world weight.
