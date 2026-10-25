# Evidence — OQ-S4-1 RESOLVED: the CRecoverability 169-entry extension table dumped

> Session 5 (2026-10-26). Audited core `7cf6754`; corpus `03ad46c`.
> Extraction: session-5 lead, direct PE parse of `og-codebase/DRWUI.exe`
> (script: read-only Python, section-table RVA→file-offset translation).
> Corrects two details in the OQ: the table lives in **`.data`**, not
> `.rdata`, and it is **not pre-sorted** — the 2000-iteration loader loop
> (`DRWUI_decompiled.c:605297-605304`) inserts every entry into a runtime
> `std::map`-like container (`DAT_1425e7980` via `FUN_1401c02d0`), which
> provides the sorted lookup; the static array order is the build order.

## Extraction parameters

- Image base 0x140000000; target VA 0x141bb0500 → RVA 0x1bb0500 → section
  **`.data`** → file offset 0x1baef00.
- Entry format: **9-byte stride, null-padded ASCII, ≤8 chars** — matching the
  loader loop `FUN_1400563c0(&DAT_141bb0500 + i*9)` (string construction from
  a char* at 9-byte stride, DRWUI:605298-605304).
- Boundary: 32 bytes before and 48 bytes after the 169th entry are all zero —
  the table ends cleanly at entry 168. Entries counted by the printable-
  ASCII/1-8-char rule: **exactly 169** (0..168), first key **"Raw"**, last
  key **"COMMON"** (a sentinel, not a real extension).

## The 169 entries (extraction order = static array order)

```
  0 Raw      1 3DS      2 3GP      3 ADP      4 ANI      5 AI       6 ASF
  7 WMA      8 WMV      9 AU      10 AVI     11 BMP     12 CAB     13 CDR
 14 CHM     15 CLK     16 CMX     17 CPT     18 CRW     19 CSL     20 CUR
 21 DBX     22 DLL     23 DOC     24 DWG     25 DXF     26 EMF     27 EPS
 28 EXE     29 FH11    30 GIF     31 GZ      32 HLP     33 HTML    34 ICO
 35 JPG     36 LZH     37 MAX     38 MID     39 MDB     40 MNY     41 MOF
 42 MOV     43 MP3     44 MP4     45 M4A     46 M4V     47 MPP     48 OLE
 49 ONE     50 PDF     51 PNG     52 PPT     53 PUB     54 PSD     55 PSP
 56 PST     57 QPW     58 RAR     59 RIFF    60 RTF     61 SHW     62 SWF
 63 TAR     64 TIF     65 TTF     66 VSD     67 WAV     68 WPD     69 WPG
 70 WMF     71 XLS     72 ZIP     73 DOCX    74 XLSX    75 PPTX    76 ACCDB
 77 NEF     78 NRW     79 AAC     80 AIFF    81 APE     82 OGG     83 CR2
 84 SR2     85 ARW     86 RAW     87 RW2     88 DNG     89 RAF     90 FLV
 91 SVG     92 SIT     93 SITX    94 RM      95 RMVB    96 MKV     97 DCR
 98 KDC     99 3FR    100 ERF    101 MEF    102 MRW    103 MDC    104 PEF
105 PTX    106 SRW    107 ORF    108 X3F    109 MPG    110 PAGES   111 KEY
112 NUMBERS 113 DMG   114 EPUB   115 FCPEVENT 116 M2TS  117 MUSE   118 MTS
119 CR3    120 MXF    121 HWP    122 HWPX   123 M2T    124 HEIC   125 HEIF
126 WEBP   127 M1V    128 JP2    129 JPF    130 JPM    131 STL    132 RVT
133 MOBI   134 XMIND  135 MMAP   136 VCF    137 M4P   138 WEBM   139 POS
140 EDDX   141 WFP    142 VPJ    143 OSP    144 HFP    145 DRP    146 HIK
147 ODP    148 ODS    149 ODT    150 7Z     151 GPX   152 INSV   153 INSP
154 GPR    155 CRM    156 AEP    157 FFF    158 DPX   159 MUSX   160 XIB
161 BRAW   162 NEV    163 R3D    164 PIX    165 MAT   166 SLN    167 VCPROJ
168 COMMON
```

## Interpretation for the S-023 fix plan (step 3)

- The table is the **deep-validation extension gate**: files whose extension
  (case-insensitive, presumably — both "Raw" and "RAW" appear at 0 and 86,
  suggesting semantic families rather than case variants: "Raw" = generic raw
  camera-family, "RAW" = the .raw extension) is in this set get the
  content-probe deep validation; everything else takes the generic path.
- Notable memberships: document formats (DOC/DOCX/XLS/XLSX/PPT/PPTX/PDF/
  ACCDB/MDB/ONE/OLE/EPUB/HWP/HWPX/PAGES/KEY/NUMBERS/ODP/ODS/ODT/MOBI), photo
  raws (NEF/NRW/CR2/CR3/SR2/ARW/RW2/DNG/RAF/DCR/KDC/3FR/ERF/MEF/MRW/MDC/PEF/
  PTX/SRW/ORF/X3F/FFF/GPR/BRAW/R3D), video (AVI/MP4/M4V/MOV/MKV/MTS/M2TS/
  M2T/MXF/FLV/RM/RMVB/WEBM/M1V/MPG/3GP/HEIC?/HEIF?/WEBP?/CR3?), audio
  (MP3/M4A/M4P/WMA/AAC/AIFF/APE/OGG/MID/AU/WAV), archives (ZIP/RAR/7Z/GZ/
  LZH/SIT/SITX/TAR/CAB/DMG), graphics (BMP/GIF/PNG/JPG/TIF/EMF/WMF/ICO/CUR/
  SVG/PSD/PSP/AI/EPS/CMX/CPT/DRW/DXF/JP2/JPF/JPM), and a tail of project/
  niche formats (FCPEVENT/MUSE/EDDX/WFP/VPJ/OSP/HFP/DRP/HIK/XMIND/MMAP/VCF/
  POS/AEP/MUSX/XIB/NEV/PIX/MAT/SLN/VCPROJ/RVT/STL/INSV/INSP/GPX/CRM) plus
  the "COMMON" sentinel and the "RIFF" container family.
- Cross-check vs `og-codebase/FileFilter.xml` (the OQ's suggested next
  artifact): this set is a strict subset of the 1,481-extension filter
  catalog — consistent with "the deep-validatable formats".

## Reproduction

```
python3 <this session's script>   # scripts/s5-oq-s4-1-ext-table.py (session-5 scratch, not committed)
# parse PE headers of og-codebase/DRWUI.exe, RVA 0x1bb0500 -> .data file off 0x1baef00,
# read 9-byte-stride entries until non-printable/all-zero -> 169 entries printed
```

Independent corroboration: the loader loop at DRWUI:605297-605304 caps at
2000 iterations × 9-byte stride = 18000 bytes of address space; the 169
non-empty keys end at 0x141bb0500 + 169×9 = 0x141bb0ae9, followed by zero
padding (verified in the dump) — the 2000 cap is a static upper bound, the
effective set is the 169 non-empty keys above.
