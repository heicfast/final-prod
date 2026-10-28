# Context — OQ-S4-1: the og's 169-entry grading extension table (VA 0x141bb0500)

**Status: RESOLVED (read-only RE; table fully dumped byte-exact).** Related finding:
`core_recovery/diff-to-fix/findings/SEVERE/S-023-no-post-scan-grading-engine-crecoverability-unimplemented.md`
(§Open questions #1). This file resolves that open question; no repository code was modified.

## 1. Artifact provenance

- Binary: `/home/z/my-project/audit/easeus-drw-research/og-codebase/DRWUI.exe` (og original;
  30,571,752 bytes = 0x1d27ce8; md5 `7150af8c6303c42ea7f2d89285f33e98`; PE32+ x86-64,
  ImageBase 0x140000000, 6 sections).
- **Correction to S-023 / the OQ text:** VA 0x141bb0500 is in **`.data`**, not `.rdata`
  (`.rdata` = 0x140dc6000..0x141b9bf3c; `.data` = 0x141b9c000..0x142644248). File offset
  0x1b9aa00 + (0x141bb0500-0x141b9c000) = **file offset 0x1baef00** (`.data` raw size 0x39200
  covers it: it is initialized data).
- Decompiled corpus: `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`
  (cited below by line number). All line numbers verified this session.

## 2. Table structure (decode)

- **Stride 9 bytes** — an array of `char[9]` C-strings: up to 8 significant chars + NUL,
  zero-padded to 9. Stride confirmed by the consumer decompile (`i * 9`,
  `DRWUI_decompiled.c:605299-605301`) and by byte-scan (every 9th boundary lands on a
  printable key; all padding bytes 0x00).
- **169 entries** (indices 0..168), followed by an all-zero **sentinel entry** at index 169
  (VA 0x141bb0af1..0x141bb0af9), followed by a zero run of 0x4056 bytes (to VA 0x141bb4b50).
  Longest key: `FCPEVENT` (8 chars, exactly fills char[9]).
- Total table span: 170*9 = 0x5fa bytes (0x141bb0500..0x141bb0af9); the 2000-entry read
  window (see §4) ends at 0x141bb4b50 — entirely inside the zero padding.

## 3. Full dump — all 169 entries, verbatim (byte-exact)

| idx | VA | 9 bytes (hex) | key |
|----:|:----|:----|:----|
|   0 | `0x141bb0500` | `52 61 77 00 00 00 00 00 00` | `Raw` |
|   1 | `0x141bb0509` | `33 44 53 00 00 00 00 00 00` | `3DS` |
|   2 | `0x141bb0512` | `33 47 50 00 00 00 00 00 00` | `3GP` |
|   3 | `0x141bb051b` | `41 44 50 00 00 00 00 00 00` | `ADP` |
|   4 | `0x141bb0524` | `41 4e 49 00 00 00 00 00 00` | `ANI` |
|   5 | `0x141bb052d` | `41 49 00 00 00 00 00 00 00` | `AI` |
|   6 | `0x141bb0536` | `41 53 46 00 00 00 00 00 00` | `ASF` |
|   7 | `0x141bb053f` | `57 4d 41 00 00 00 00 00 00` | `WMA` |
|   8 | `0x141bb0548` | `57 4d 56 00 00 00 00 00 00` | `WMV` |
|   9 | `0x141bb0551` | `41 55 00 00 00 00 00 00 00` | `AU` |
|  10 | `0x141bb055a` | `41 56 49 00 00 00 00 00 00` | `AVI` |
|  11 | `0x141bb0563` | `42 4d 50 00 00 00 00 00 00` | `BMP` |
|  12 | `0x141bb056c` | `43 41 42 00 00 00 00 00 00` | `CAB` |
|  13 | `0x141bb0575` | `43 44 52 00 00 00 00 00 00` | `CDR` |
|  14 | `0x141bb057e` | `43 48 4d 00 00 00 00 00 00` | `CHM` |
|  15 | `0x141bb0587` | `43 4c 4b 00 00 00 00 00 00` | `CLK` |
|  16 | `0x141bb0590` | `43 4d 58 00 00 00 00 00 00` | `CMX` |
|  17 | `0x141bb0599` | `43 50 54 00 00 00 00 00 00` | `CPT` |
|  18 | `0x141bb05a2` | `43 52 57 00 00 00 00 00 00` | `CRW` |
|  19 | `0x141bb05ab` | `43 53 4c 00 00 00 00 00 00` | `CSL` |
|  20 | `0x141bb05b4` | `43 55 52 00 00 00 00 00 00` | `CUR` |
|  21 | `0x141bb05bd` | `44 42 58 00 00 00 00 00 00` | `DBX` |
|  22 | `0x141bb05c6` | `44 4c 4c 00 00 00 00 00 00` | `DLL` |
|  23 | `0x141bb05cf` | `44 4f 43 00 00 00 00 00 00` | `DOC` |
|  24 | `0x141bb05d8` | `44 57 47 00 00 00 00 00 00` | `DWG` |
|  25 | `0x141bb05e1` | `44 58 46 00 00 00 00 00 00` | `DXF` |
|  26 | `0x141bb05ea` | `45 4d 46 00 00 00 00 00 00` | `EMF` |
|  27 | `0x141bb05f3` | `45 50 53 00 00 00 00 00 00` | `EPS` |
|  28 | `0x141bb05fc` | `45 58 45 00 00 00 00 00 00` | `EXE` |
|  29 | `0x141bb0605` | `46 48 31 31 00 00 00 00 00` | `FH11` |
|  30 | `0x141bb060e` | `47 49 46 00 00 00 00 00 00` | `GIF` |
|  31 | `0x141bb0617` | `47 5a 00 00 00 00 00 00 00` | `GZ` |
|  32 | `0x141bb0620` | `48 4c 50 00 00 00 00 00 00` | `HLP` |
|  33 | `0x141bb0629` | `48 54 4d 4c 00 00 00 00 00` | `HTML` |
|  34 | `0x141bb0632` | `49 43 4f 00 00 00 00 00 00` | `ICO` |
|  35 | `0x141bb063b` | `4a 50 47 00 00 00 00 00 00` | `JPG` |
|  36 | `0x141bb0644` | `4c 5a 48 00 00 00 00 00 00` | `LZH` |
|  37 | `0x141bb064d` | `4d 41 58 00 00 00 00 00 00` | `MAX` |
|  38 | `0x141bb0656` | `4d 49 44 00 00 00 00 00 00` | `MID` |
|  39 | `0x141bb065f` | `4d 44 42 00 00 00 00 00 00` | `MDB` |
|  40 | `0x141bb0668` | `4d 4e 59 00 00 00 00 00 00` | `MNY` |
|  41 | `0x141bb0671` | `4d 4f 46 00 00 00 00 00 00` | `MOF` |
|  42 | `0x141bb067a` | `4d 4f 56 00 00 00 00 00 00` | `MOV` |
|  43 | `0x141bb0683` | `4d 50 33 00 00 00 00 00 00` | `MP3` |
|  44 | `0x141bb068c` | `4d 50 34 00 00 00 00 00 00` | `MP4` |
|  45 | `0x141bb0695` | `4d 34 41 00 00 00 00 00 00` | `M4A` |
|  46 | `0x141bb069e` | `4d 34 56 00 00 00 00 00 00` | `M4V` |
|  47 | `0x141bb06a7` | `4d 50 50 00 00 00 00 00 00` | `MPP` |
|  48 | `0x141bb06b0` | `4f 4c 45 00 00 00 00 00 00` | `OLE` |
|  49 | `0x141bb06b9` | `4f 4e 45 00 00 00 00 00 00` | `ONE` |
|  50 | `0x141bb06c2` | `50 44 46 00 00 00 00 00 00` | `PDF` |
|  51 | `0x141bb06cb` | `50 4e 47 00 00 00 00 00 00` | `PNG` |
|  52 | `0x141bb06d4` | `50 50 54 00 00 00 00 00 00` | `PPT` |
|  53 | `0x141bb06dd` | `50 55 42 00 00 00 00 00 00` | `PUB` |
|  54 | `0x141bb06e6` | `50 53 44 00 00 00 00 00 00` | `PSD` |
|  55 | `0x141bb06ef` | `50 53 50 00 00 00 00 00 00` | `PSP` |
|  56 | `0x141bb06f8` | `50 53 54 00 00 00 00 00 00` | `PST` |
|  57 | `0x141bb0701` | `51 50 57 00 00 00 00 00 00` | `QPW` |
|  58 | `0x141bb070a` | `52 41 52 00 00 00 00 00 00` | `RAR` |
|  59 | `0x141bb0713` | `52 49 46 46 00 00 00 00 00` | `RIFF` |
|  60 | `0x141bb071c` | `52 54 46 00 00 00 00 00 00` | `RTF` |
|  61 | `0x141bb0725` | `53 48 57 00 00 00 00 00 00` | `SHW` |
|  62 | `0x141bb072e` | `53 57 46 00 00 00 00 00 00` | `SWF` |
|  63 | `0x141bb0737` | `54 41 52 00 00 00 00 00 00` | `TAR` |
|  64 | `0x141bb0740` | `54 49 46 00 00 00 00 00 00` | `TIF` |
|  65 | `0x141bb0749` | `54 54 46 00 00 00 00 00 00` | `TTF` |
|  66 | `0x141bb0752` | `56 53 44 00 00 00 00 00 00` | `VSD` |
|  67 | `0x141bb075b` | `57 41 56 00 00 00 00 00 00` | `WAV` |
|  68 | `0x141bb0764` | `57 50 44 00 00 00 00 00 00` | `WPD` |
|  69 | `0x141bb076d` | `57 50 47 00 00 00 00 00 00` | `WPG` |
|  70 | `0x141bb0776` | `57 4d 46 00 00 00 00 00 00` | `WMF` |
|  71 | `0x141bb077f` | `58 4c 53 00 00 00 00 00 00` | `XLS` |
|  72 | `0x141bb0788` | `5a 49 50 00 00 00 00 00 00` | `ZIP` |
|  73 | `0x141bb0791` | `44 4f 43 58 00 00 00 00 00` | `DOCX` |
|  74 | `0x141bb079a` | `58 4c 53 58 00 00 00 00 00` | `XLSX` |
|  75 | `0x141bb07a3` | `50 50 54 58 00 00 00 00 00` | `PPTX` |
|  76 | `0x141bb07ac` | `41 43 43 44 42 00 00 00 00` | `ACCDB` |
|  77 | `0x141bb07b5` | `4e 45 46 00 00 00 00 00 00` | `NEF` |
|  78 | `0x141bb07be` | `4e 52 57 00 00 00 00 00 00` | `NRW` |
|  79 | `0x141bb07c7` | `41 41 43 00 00 00 00 00 00` | `AAC` |
|  80 | `0x141bb07d0` | `41 49 46 46 00 00 00 00 00` | `AIFF` |
|  81 | `0x141bb07d9` | `41 50 45 00 00 00 00 00 00` | `APE` |
|  82 | `0x141bb07e2` | `4f 47 47 00 00 00 00 00 00` | `OGG` |
|  83 | `0x141bb07eb` | `43 52 32 00 00 00 00 00 00` | `CR2` |
|  84 | `0x141bb07f4` | `53 52 32 00 00 00 00 00 00` | `SR2` |
|  85 | `0x141bb07fd` | `41 52 57 00 00 00 00 00 00` | `ARW` |
|  86 | `0x141bb0806` | `52 41 57 00 00 00 00 00 00` | `RAW` |
|  87 | `0x141bb080f` | `52 57 32 00 00 00 00 00 00` | `RW2` |
|  88 | `0x141bb0818` | `44 4e 47 00 00 00 00 00 00` | `DNG` |
|  89 | `0x141bb0821` | `52 41 46 00 00 00 00 00 00` | `RAF` |
|  90 | `0x141bb082a` | `46 4c 56 00 00 00 00 00 00` | `FLV` |
|  91 | `0x141bb0833` | `53 56 47 00 00 00 00 00 00` | `SVG` |
|  92 | `0x141bb083c` | `53 49 54 00 00 00 00 00 00` | `SIT` |
|  93 | `0x141bb0845` | `53 49 54 58 00 00 00 00 00` | `SITX` |
|  94 | `0x141bb084e` | `52 4d 00 00 00 00 00 00 00` | `RM` |
|  95 | `0x141bb0857` | `52 4d 56 42 00 00 00 00 00` | `RMVB` |
|  96 | `0x141bb0860` | `4d 4b 56 00 00 00 00 00 00` | `MKV` |
|  97 | `0x141bb0869` | `44 43 52 00 00 00 00 00 00` | `DCR` |
|  98 | `0x141bb0872` | `4b 44 43 00 00 00 00 00 00` | `KDC` |
|  99 | `0x141bb087b` | `33 46 52 00 00 00 00 00 00` | `3FR` |
| 100 | `0x141bb0884` | `45 52 46 00 00 00 00 00 00` | `ERF` |
| 101 | `0x141bb088d` | `4d 45 46 00 00 00 00 00 00` | `MEF` |
| 102 | `0x141bb0896` | `4d 52 57 00 00 00 00 00 00` | `MRW` |
| 103 | `0x141bb089f` | `4d 44 43 00 00 00 00 00 00` | `MDC` |
| 104 | `0x141bb08a8` | `50 45 46 00 00 00 00 00 00` | `PEF` |
| 105 | `0x141bb08b1` | `50 54 58 00 00 00 00 00 00` | `PTX` |
| 106 | `0x141bb08ba` | `53 52 57 00 00 00 00 00 00` | `SRW` |
| 107 | `0x141bb08c3` | `4f 52 46 00 00 00 00 00 00` | `ORF` |
| 108 | `0x141bb08cc` | `58 33 46 00 00 00 00 00 00` | `X3F` |
| 109 | `0x141bb08d5` | `4d 50 47 00 00 00 00 00 00` | `MPG` |
| 110 | `0x141bb08de` | `50 41 47 45 53 00 00 00 00` | `PAGES` |
| 111 | `0x141bb08e7` | `4b 45 59 00 00 00 00 00 00` | `KEY` |
| 112 | `0x141bb08f0` | `4e 55 4d 42 45 52 53 00 00` | `NUMBERS` |
| 113 | `0x141bb08f9` | `44 4d 47 00 00 00 00 00 00` | `DMG` |
| 114 | `0x141bb0902` | `45 50 55 42 00 00 00 00 00` | `EPUB` |
| 115 | `0x141bb090b` | `46 43 50 45 56 45 4e 54 00` | `FCPEVENT` |
| 116 | `0x141bb0914` | `4d 32 54 53 00 00 00 00 00` | `M2TS` |
| 117 | `0x141bb091d` | `4d 55 53 45 00 00 00 00 00` | `MUSE` |
| 118 | `0x141bb0926` | `4d 54 53 00 00 00 00 00 00` | `MTS` |
| 119 | `0x141bb092f` | `43 52 33 00 00 00 00 00 00` | `CR3` |
| 120 | `0x141bb0938` | `4d 58 46 00 00 00 00 00 00` | `MXF` |
| 121 | `0x141bb0941` | `48 57 50 00 00 00 00 00 00` | `HWP` |
| 122 | `0x141bb094a` | `48 57 50 58 00 00 00 00 00` | `HWPX` |
| 123 | `0x141bb0953` | `4d 32 54 00 00 00 00 00 00` | `M2T` |
| 124 | `0x141bb095c` | `48 45 49 43 00 00 00 00 00` | `HEIC` |
| 125 | `0x141bb0965` | `48 45 49 46 00 00 00 00 00` | `HEIF` |
| 126 | `0x141bb096e` | `57 45 42 50 00 00 00 00 00` | `WEBP` |
| 127 | `0x141bb0977` | `4d 31 56 00 00 00 00 00 00` | `M1V` |
| 128 | `0x141bb0980` | `4a 50 32 00 00 00 00 00 00` | `JP2` |
| 129 | `0x141bb0989` | `4a 50 46 00 00 00 00 00 00` | `JPF` |
| 130 | `0x141bb0992` | `4a 50 4d 00 00 00 00 00 00` | `JPM` |
| 131 | `0x141bb099b` | `53 54 4c 00 00 00 00 00 00` | `STL` |
| 132 | `0x141bb09a4` | `52 56 54 00 00 00 00 00 00` | `RVT` |
| 133 | `0x141bb09ad` | `4d 4f 42 49 00 00 00 00 00` | `MOBI` |
| 134 | `0x141bb09b6` | `58 4d 49 4e 44 00 00 00 00` | `XMIND` |
| 135 | `0x141bb09bf` | `4d 4d 41 50 00 00 00 00 00` | `MMAP` |
| 136 | `0x141bb09c8` | `56 43 46 00 00 00 00 00 00` | `VCF` |
| 137 | `0x141bb09d1` | `4d 34 50 00 00 00 00 00 00` | `M4P` |
| 138 | `0x141bb09da` | `57 45 42 4d 00 00 00 00 00` | `WEBM` |
| 139 | `0x141bb09e3` | `50 4f 53 00 00 00 00 00 00` | `POS` |
| 140 | `0x141bb09ec` | `45 44 44 58 00 00 00 00 00` | `EDDX` |
| 141 | `0x141bb09f5` | `57 46 50 00 00 00 00 00 00` | `WFP` |
| 142 | `0x141bb09fe` | `56 50 4a 00 00 00 00 00 00` | `VPJ` |
| 143 | `0x141bb0a07` | `4f 53 50 00 00 00 00 00 00` | `OSP` |
| 144 | `0x141bb0a10` | `48 46 50 00 00 00 00 00 00` | `HFP` |
| 145 | `0x141bb0a19` | `44 52 50 00 00 00 00 00 00` | `DRP` |
| 146 | `0x141bb0a22` | `48 49 4b 00 00 00 00 00 00` | `HIK` |
| 147 | `0x141bb0a2b` | `4f 44 50 00 00 00 00 00 00` | `ODP` |
| 148 | `0x141bb0a34` | `4f 44 53 00 00 00 00 00 00` | `ODS` |
| 149 | `0x141bb0a3d` | `4f 44 54 00 00 00 00 00 00` | `ODT` |
| 150 | `0x141bb0a46` | `37 5a 00 00 00 00 00 00 00` | `7Z` |
| 151 | `0x141bb0a4f` | `47 50 58 00 00 00 00 00 00` | `GPX` |
| 152 | `0x141bb0a58` | `49 4e 53 56 00 00 00 00 00` | `INSV` |
| 153 | `0x141bb0a61` | `49 4e 53 50 00 00 00 00 00` | `INSP` |
| 154 | `0x141bb0a6a` | `47 50 52 00 00 00 00 00 00` | `GPR` |
| 155 | `0x141bb0a73` | `43 52 4d 00 00 00 00 00 00` | `CRM` |
| 156 | `0x141bb0a7c` | `41 45 50 00 00 00 00 00 00` | `AEP` |
| 157 | `0x141bb0a85` | `46 46 46 00 00 00 00 00 00` | `FFF` |
| 158 | `0x141bb0a8e` | `44 50 58 00 00 00 00 00 00` | `DPX` |
| 159 | `0x141bb0a97` | `4d 55 53 58 00 00 00 00 00` | `MUSX` |
| 160 | `0x141bb0aa0` | `58 49 42 00 00 00 00 00 00` | `XIB` |
| 161 | `0x141bb0aa9` | `42 52 41 57 00 00 00 00 00` | `BRAW` |
| 162 | `0x141bb0ab2` | `4e 45 56 00 00 00 00 00 00` | `NEV` |
| 163 | `0x141bb0abb` | `52 33 44 00 00 00 00 00 00` | `R3D` |
| 164 | `0x141bb0ac4` | `50 49 58 00 00 00 00 00 00` | `PIX` |
| 165 | `0x141bb0acd` | `4d 41 54 00 00 00 00 00 00` | `MAT` |
| 166 | `0x141bb0ad6` | `53 4c 4e 00 00 00 00 00 00` | `SLN` |
| 167 | `0x141bb0adf` | `56 43 50 52 4f 4a 00 00 00` | `VCPROJ` |
| 168 | `0x141bb0ae8` | `43 4f 4d 4d 4f 4e 00 00 00` | `COMMON` |

Sentinel (not a key): index 169 @ `0x141bb0af1` = `00 00 00 00 00 00 00 00 00`.

One-line key list (semicolon-separated, verbatim):
Raw; 3DS; 3GP; ADP; ANI; AI; ASF; WMA; WMV; AU; AVI; BMP; CAB; CDR; CHM; CLK; CMX; CPT; CRW; CSL; CUR; DBX; DLL; DOC; DWG; DXF; EMF; EPS; EXE; FH11; GIF; GZ; HLP; HTML; ICO; JPG; LZH; MAX; MID; MDB; MNY; MOF; MOV; MP3; MP4; M4A; M4V; MPP; OLE; ONE; PDF; PNG; PPT; PUB; PSD; PSP; PST; QPW; RAR; RIFF; RTF; SHW; SWF; TAR; TIF; TTF; VSD; WAV; WPD; WPG; WMF; XLS; ZIP; DOCX; XLSX; PPTX; ACCDB; NEF; NRW; AAC; AIFF; APE; OGG; CR2; SR2; ARW; RAW; RW2; DNG; RAF; FLV; SVG; SIT; SITX; RM; RMVB; MKV; DCR; KDC; 3FR; ERF; MEF; MRW; MDC; PEF; PTX; SRW; ORF; X3F; MPG; PAGES; KEY; NUMBERS; DMG; EPUB; FCPEVENT; M2TS; MUSE; MTS; CR3; MXF; HWP; HWPX; M2T; HEIC; HEIF; WEBP; M1V; JP2; JPF; JPM; STL; RVT; MOBI; XMIND; MMAP; VCF; M4P; WEBM; POS; EDDX; WFP; VPJ; OSP; HFP; DRP; HIK; ODP; ODS; ODT; 7Z; GPX; INSV; INSP; GPR; CRM; AEP; FFF; DPX; MUSX; XIB; BRAW; NEV; R3D; PIX; MAT; SLN; VCPROJ; COMMON

## 4. Who builds the table into a lookup structure

- `FUN_140389740` (`DRWUI_decompiled.c:605124-605308`, CRecoverability ctor/static init):
  first inserts 20 lowercase extension strings into a *member* set at `obj+0x188` from
  .rdata 0x1413ca6bc-0x1413ca824 (a **different** set: jpg, jpeg, nef, cr3, dng, arw, cr2,
  heic, raf, orf, mp4, mov, pdf, doc, docx, xls, xlsx, ppt, pptx, mp3 — not part of
  OQ-S4-1). Then, gated by `FUN_1401bc920(&DAT_1425e7980)` (empty-check = lazy init),
  it loops over the 0x141bb0500 array inserting each entry as `std::string` into the
  **global set `DAT_1425e7980`** (`:605297-605305`).
- **Loop bound, verified by disassembly** (objdump of 0x140389f60-0x14038a030; the loop is
  `0x140389f73`-`0x14038a004`): `movl $0; cmpl $0x7d0` → **plain `for (i=0; i<2000; ++i)`**
  with NO sentinel test. Ghidra's `(i*9 != -0x141bb0500)` (`:605299`) is a decompiler
  artifact of the always-true null-check of the computed pointer (`lea table; add i*9;
  test rax,rax` at 0x140389f91-0x140389faa). Consequence: entries 169..1999 are read from
  the zero padding → 1831 inserts of `""` → after std::set dedup the runtime set =
  **169 real keys + the empty string `""`**.
- Global-set lifecycle: constructed by dynamic initializer `FUN_140003240` (`:446-452`),
  destroyed by atexit `FUN_140dbe290` / `FUN_1401b42b0(&DAT_1425e7980)` (`:2372409`).

## 5. Consumption semantics (the matching rule)

Sole consumer: `FUN_14038ac90` — the post-scan grading decision tree
(`DRWUI_decompiled.c:605686-605815`), generic path only, i.e. after the fast paths
(size<1→grade 0 `:605727`; category 0xC→grade 2 `:605731-:605734`) and only when
FS ∉ {3 NTFS, 10 ReFS, 0xB Btrfs} (`:605736-605737`):

1. **Key derivation** — `FUN_1403895c0` (`:605070-605109`): node name via `FUN_140089050`;
   `FUN_14014f8f0(name, '.', npos)` = **rfind — LAST dot** (npos default is rfind's; high
   confidence, not byte-verified); hit → `FUN_14008b5d0(..., dot+1, npos)` = substring
   after the dot (**dot stripped, no leading '.'**); no dot → the empty string
   constant `DAT_140f8d99a` (`""`).
2. **Case normalization** — `FUN_140389400` (`:604974-605017`) = in-place
   `std::transform` (callee `FUN_14038ed20` `:608087-605115`) applying `toupper`
   (functor `FUN_140389500`, `:605022-605027`). **The lookup key is UPPERCASED**
   (`tolower` is never used on this path).
3. **Lookup** — `FUN_140256d00(&DAT_1425e7980, ..., key)` = `std::set::find`
   (`:605740`) vs `FUN_1401be4c0` = `set::end` (`:605741`); equality test
   `FUN_1401bebe0` (`:605742`). Exact, case-sensitive std::set compare against the
   stored (mostly uppercase) keys — **no dot prefix, no wildcards, no length limit
   beyond char[9]**; precedence: pure gate (in-table / not-in-table), nothing ordered.
4. **In-table (find != end)** → deep-validation branch (`:605743-:605803`):
   overlap-tree insert into `obj+0x20` with extent-end key `base+len+(idx-2 if
   device-kind∈{1,4})*unit`; twin found → existing twin node graded 2
   (`FUN_140089630(local_90,2)` `:605781`); node flagged bit 3 of +0x73
   (`FUN_140389540(param_2,1)` `:605800` = "needs validate/preview") and
   `FUN_14038c470` content validation queued (`:605798`). **Not-in-table** → else branch
   (`:605804-605807`): nothing — node keeps the engine-default grade 1 (Average).

## 6. Observed quirks (port-blocking notes for the fix leg)

1. **`Raw` (entry 0) is dead.** It is the only mixed-case key; the lookup key is
   uppercased, and the set compare is case-sensitive, so `"Raw"` can never match
   (files `*.raw` produce key `"RAW"`, which matches the *separate* entry `RAW` at
   index 86, VA 0x141bb0806). Entry 0 is unreachable legacy data; do not "fix" it by
   case-folding the set, or `RAW`-vs-`Raw` behavior changes.
2. **`""` is in the set.** Because the init loop has no sentinel test (§4), a file with
   **no dot** in its name (extension `""`) MATCHES the table and enters the
   deep-validation branch. A faithful port must reproduce this (either port the
   2000-iteration loop over a zero-padded array, or explicitly include `""` in the set
   and treat dot-less files as in-table).
3. **Section gloss corrected:** table is in `.data` (0x141bb0500), not `.rdata` as
   S-023/OQ text states. No other S-023 claim is affected.
4. The 20-string lowercase set in the same ctor (§4) is a different structure (member of
   the CRecoverability object, .rdata 0x1413ca6bc-824); don't conflate it with this table.

## 7. Citations

- Binary: `og-codebase/DRWUI.exe`, md5 7150af8c6303c42ea7f2d89285f33e98; table at
  VA 0x141bb0500 (`.data`, file offset 0x1baef00), 170*9 bytes incl. sentinel.
- Decompiled: `DRWUI_decompiled.c` — table ref `:605299-605302`; ctor `FUN_140389740`
  `:605124`; consumer `FUN_14038ac90` `:605686-605815` (lookup `:605738-605743`);
  key derivation `FUN_1403895c0` `:605070`; uppercasing `FUN_140389400` `:604974` →
  `FUN_14038ed20` `:608087` → toupper functor `:605022`; global-set init `:446-452`
  (`FUN_140003240`), teardown `:2372409`.
- Disassembly (this session, objdump): 0x140389f68-0x14038a004 — loop `i<0x7d0` (2000),
  `imul $0x9`, `lea 0x141bb0500`, unconditional body; confirms Ghidra's
  `!= -0x141bb0500` is an artifact.

**Bottom line:** OQ-S4-1 is RESOLVED. The extension-gate rule is now byte-matchable:
gate = { ext(name-after-last-dot, ASCII-uppercased) ∈ 169-key set ∪ {""} }, exact
case-sensitive match, and only on the generic FS path of `FUN_14038ac90`.

---

## ERRATUM (S-058 Resolution, 2026-10-29 — §5 item 4's polarity line corrected)

§5 item 4's "In-table (find != end) → deep-validation branch" is INVERTED. The
comparator chain re-read first-hand at the S-058 verification:
`FUN_1401bebe0` (:324923) returns `FUN_1401bec20(a,b) == 0` and `FUN_1401bec20`
(:324925) is `*a == *b` — so `FUN_1401bebe0(find, end)` is TRUE iff find != end
iff FOUND, and the gate `if (cVar1 == '\0')` at :605743 enters the
deep-validation block exactly when the lookup MISSED (NOT in the table). The
control case at :585885-905 (the found-in-set || name-in-consts idiom using the
EQUALITY comparator) calibrates both directions simultaneously. §6.2's note
survives with the corrected reading: the runtime set's "" quirk makes dot-less
files SKIP the branch (they are in-table), not enter it. The table dump itself
(§2-§3) remains byte-exact and valid.
