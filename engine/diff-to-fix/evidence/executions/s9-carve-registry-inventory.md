# Evidence: the carve-registry inventory (S-042 product-absence proof)

**Core:** `core_recovery` product code @ `35adcb9` (session-9 audited baseline)
**Method:** mechanical enumeration of every FMT_* id in `src/core/carve/cr_registry.cpp`
**Command:** `grep -o 'FMT_[A-Z0-9_]*' src/core/carve/cr_registry.cpp | sort -u`

## The 101 rows (FMT_COUNT is the sentinel)

```
FMT_3GP FMT_7Z FMT_AAC FMT_AAC2 FMT_AI FMT_AIFF FMT_APE FMT_ASF FMT_AU FMT_AVI
FMT_BMP FMT_CAB FMT_CAB_LC FMT_CPT7 FMT_CPT8 FMT_CPT9 FMT_CR2 FMT_CUR FMT_DMG
FMT_DNG FMT_DOCX FMT_DWG FMT_DXF FMT_EPS FMT_FH11 FMT_FLV FMT_GIF FMT_GZ FMT_HEIC
FMT_HEIF FMT_HLP FMT_HLP2 FMT_HLP3 FMT_ICO FMT_ISO FMT_JPG FMT_M4A FMT_MDB FMT_MID
FMT_MKV FMT_MOV FMT_MOV_NAKED FMT_MP3 FMT_MP4 FMT_MPEGES FMT_MPEGPS FMT_MPGA_E2
FMT_MPGA_E3 FMT_MPGA_E4 FMT_MPGA_E5 FMT_MPGA_E6 FMT_MPGA_E7 FMT_MPGA_F2 FMT_MPGA_F3
FMT_MPGA_F4 FMT_MPGA_F5 FMT_MPGA_F6 FMT_MPGA_F7 FMT_MPGA_FA FMT_MPGA_FB FMT_MPGA_FC
FMT_MPGA_FD FMT_MPGA_FE FMT_MPGA_FF FMT_MRW FMT_MRW_MM FMT_MRW_MRM FMT_MXF FMT_OGG
FMT_OLE FMT_ORF FMT_ORF_IIRS FMT_ORF_MMOR FMT_PDF FMT_PNG FMT_PSD FMT_PSP FMT_PST
FMT_RAF FMT_RAR4 FMT_RAR5 FMT_RM FMT_RTF FMT_RW2 FMT_SIT FMT_SQLITE FMT_TAR FMT_TIFF
FMT_TIFF_BE FMT_TIFF_EP FMT_TTF FMT_WAV FMT_WEBP FMT_WMF FMT_WMF_PL FMT_WPD FMT_WPG
FMT_X3F FMT_ZIP FMT_ZIP_CD
```

## Absence greps (patterns vs cr_registry.cpp — all zero matches)

| Pattern (the og's magic/signature) | Matches in our registry |
|---|---|
| `ITSF` (CHM) | 0 |
| `MSISAM` (MNY) | 0 |
| ` EMF` (the EMF header's signature field) | 0 |
| `E4 52 5C 7B` / `0x7B5C52E4` (ONE) | 0 |
| `BOOKMOBI` (MOBI) | 0 |
| `FWS`/`CWS` (SWF) | 0 |
| `-lh` (LZH) | 0 |
| `<svg` (SVG) | 0 |
| `BEGIN:VCARD` (VCF) | 0 |
| `<gpx` (GPX) | 0 |
| `BiffProject` (HFP) | 0 |
| `RIFX` (AEP) | 0 |
| `jP  ` (JP2 signature box) | 0 |
| CIFF `HEAP`/`CCDR` (CRW) | 0 |
| MPEG-TS 0x47-sync analyser (CMTS/CM2TS class) | 0 (MPEGPS/MPEGES exist; no TS) |
| `"id":` (OSP), `{"diagram":` (POS), DAV pack header (HIK) | 0 |

## The og's registered batch-2 surface (from the corpus; see S-042's table)

| Format | Og key/id | Product row |
|---|---|---|
| CHM | u32 "ITSF" id 0x0E | ABSENT |
| MNY | u16 "MS" id 0x28 | ABSENT |
| EMF | list-only id 0x1A | ABSENT |
| ONE | u32 0x7B5C52E4 id 0x31 | ABSENT |
| MPEG-TS (MTS/M2TS) | list, CMTS id 0x76 / CM2TS id 0x74 | ABSENT |
| 3DS | u16 "MM" id 0x01 | ABSENT |
| SWF | list id 0x3E | ABSENT |
| JP2 | list, ext-gated, ids 0x80/0x81/0x82 | ABSENT |
| MOBI | list, ext-gated, id 0x85 | ABSENT |
| LZH | list/structural id 0x24 | ABSENT |
| HTML | u16 <h/<H/<!/BOM id 0x21 | ABSENT |
| SVG/XML | u32 <?xm/BOM</<svg/0x1CD id 0x5B | ABSENT |
| VCF | list, ext-gated id 0x88 | ABSENT |
| GPX | list, ext-gated id 0x97 | ABSENT |
| HIK (Hikvision DAV) | list, ext-gated id 0x92 | ABSENT |
| OSP | list, ext-gated id 0x8F | ABSENT |
| POS | list, ext-gated id 0x8B | ABSENT |
| AEP | list, ext-gated id 0x9C | ABSENT |
| HFP | list, ext-gated id 0x90 | ABSENT |
| CRW | u32 II/MM 1A 00 id 0x12 | ABSENT (see S-041 — the keys are occupied by dead MRW rows) |
| OfficeTagFromXML | word/xl/ppt rels names -> ids 0x49/0x4A/0x4B | PARTIAL — single FMT_DOCX (`[Content_Types].xml` first-entry gate, no DOCX/XLSX/PPTX id split) |
