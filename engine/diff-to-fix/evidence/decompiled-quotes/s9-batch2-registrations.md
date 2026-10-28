# Evidence: the batch-2 format registrations + gates (S-042)

**Corpus:** `easeus-drw-research` @ `be0cfe9`
**Primary sources:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`
(henceforth DC), `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c`
(henceforth RC), the census artifact
`re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md`.
**Method:** the B-agent extracted per-format keys/gates with line cites; the
lead re-verified the load-bearing registrations first-hand (CHM, CRW, MRW,
ONE, SWF, MPEG-TS, MMOR/IIRS/FUJI) and the product absence mechanically
(the registry inventory, see `evidence/executions/s9-carve-registry-inventory.md`).

## Registration quotes (verified subset)

### ONE (OneNote) — u32 key 0x7B5C52E4, id 0x31 (DC:2160-2177)

```c
    *(undefined ***)puVar4 = RawSpeed::OneAnalyser::vftable;
    puVar4[2] = 0x31;
    local_res18[0][2] = 0x31;
    *(undefined ***)local_res18[0] = FileONEAnalyser::vftable;
  local_res10[0] = 0x7b5c52e4;              // bytes E4 52 5C 7B
  plVar3 = FUN_180006c80(param_1,local_res10);
```

Bounds 20 B..500 MB per the census row; the full OneNote GUID
{7B5C52E4-D8D8-4DA7-AEB1-5378D02996D3} is the documented signature.
(The census's earlier "LZH" attribution for this key is an erratum — LZH is
structural, see below.)

### SWF — list layer, id 0x3E (DC:3067-3068)

```c
    ppuVar14 = FileSWFAnalyser::vftable;
    *(undefined ***)local_a0 = FileSWFAnalyser::vftable;
    local_a0[8] = 0x3e;                     // id
```

Gate (B-sheet, DC cites): "FWS"/"CWS" only (ZWS rejected); size = u32@+4;
version in [3,20]; RECT parse. No u32 registry key — dispatched from the
list layer.

### MPEG-TS — BruteForce::CMTSAnalyser + CM2TSAnalyser (DC:3143-3175)

```c
    *(undefined ***)local_a8 = BruteForce::CMTSAnalyser::vftable;
    ppuVar14 = BruteForce::CMTSAnalyser::vftable;
    *(undefined ***)(local_a8 + 0x10) = BruteForce::CMTSAnalyser::vftable;
    ...
    *(undefined ***)local_a8 = BruteForce::CM2TSAnalyser::vftable;
```

Gate: 0x47 sync @+0 or +4, second sync @+188/+192 (188/192-byte packet
periods), PID != 0x1FFF. NOTE: the "numeric key 284" the S-007 addendum
deferred is FreeHand 11 (CFH11, id 0x1D) — NOT an MTS key; the product's
existing FMT_FH11 rows are correct and MTS/M2TS is list-dispatched.

### CHM — u32 "ITSF" 0x46535449, id 0x0E (DC:2488-2499; gate :17703-17740)

Full instruction-level gate decode in `s9-chm-gate-asm.md`. ITSF + hdr_len
0x60 + both 16-byte GUIDs {7C01FD10-...}/{7C01FD11-...} @+0x18/+0x28 +
HS0 0x1FE@+0x60 + zeros + size = u64@+0x68.

### MNY — u16 "MS" 0x534D, id 0x28 (B-sheet; RC cites)

Gate: "MSISAM Database" 15-byte memcmp @+4; fixed 1 MB size. Open nuance
(tracked in the finding): whether the key sits at +0 with the signature at
+4 (the MS Money header layout has a 4-byte prologue).

### EMF — list-only, id 0x1A (B-sheet)

Gate: " EMF"@+0x28 + sReserved@+0x3A == 0 + nBytes@+0x30 != 0 ->
size = nBytes. **No keyed registration; no 01 00 00 00 key; no
record-type-1 check** (the S-007 addendum's "record-type-1 header" phrasing
is superseded by this decode).

### 3DS — u16 "MM" 0x4D4D, id 0x01 (B-sheet)

Gate: "MM"@0 + "=="@+16 (3D Studio chunk grammar) OR "NCSD"@+0x100 +
0x200000@+0x104 (Nintendo 3DS ROM image); max 200 MB.

### JP2 — list, ext-gated "JP2", ids 0x80/0x81/0x82 (B-sheet)

Gate: "jP  " + 0x0D0A870A + "ftyp", brand in {"jp2 ", "jpx ", "jpm "} ->
id by brand.

### MOBI — list, ext-gated "MOBI", id 0x85 (B-sheet)

Gate: BE "BOOK"@+0x3C + "MOBI"@+0x40 (the PalmDOC header positions).

### LZH — list/structural, id 0x24 (B-sheet)

Gate: u32@+2 in {-lh0, -lh5, -lh7, -lhd} + '-'@+6; max 200 MB. NOT the
0x7B5C52E4 key (that is OneNote).

### HTML — u16 keys <h/<H/<!/UTF-16-BOM, id 0x21 (B-sheet; reg DC:2844-2879)

Gate: memicmp "<HTML"/"<!DOCTYPE HTML" or UTF-16 BOM + '<' + L"<HTML>"/'!';
1 MB cap.

### SVG/XML — u32 keys <?xm / BOM< / <svg / 0x1CD, id 0x5B (B-sheet; reg DC:1806-1837)

Gate: "<?xml version\"" + "<svg..." strstr in 2 KB; extent via "\n</svg>"
tail scan. (The 0x1CD numeric key's semantics remain undecoded — tracked
open in the finding.)

### The text school (VCF/GPX/HIK/OSP/POS/AEP/HFP) — list, ext-gated (B-sheet)

| Format | id | Gate |
|---|---|---|
| VCF | 0x88 | "BEGIN:VCARD" |
| GPX | 0x97 | "<?xml version\"" (+0 or BOM+3) + "<gpx" scan after +0x16 |
| HIK | 0x92 | Hikvision DAV = MPEG-PS pack BA (+ optional system hdr BC) + private stream BD; size from BE u16 PES len |
| OSP | 0x8F | JSON `{\r\n "id": ` |
| POS | 0x8B | JSON `{"diagram":` |
| AEP | 0x9C | "RIFX" u32 |
| HFP | 0x90 | "<?xml version\"" + "<BiffProject"@+0x28 |

No shared "text analyser" class exists — seven independent ext-gated
analysers; only GPX+HFP are actually XML (OSP/POS are JSON, HIK is
MPEG-PS, AEP is RIFX, VCF is vCard text).

### OfficeTagFromXML — the OOXML rels classifier (B-sheet; DC:53155-53195)

```c
    iVar1 = strcmp((char *)_Str1,"word/_rels/document.xml.rels");
    if ((iVar1 != 0) || (param_3 == '\0')) {
      iVar1 = strcmp((char *)_Str1,"xl/_rels/workbook.xml.rels");
      if ((iVar1 == 0) && (param_3 != '\0')) { *param_4 = 0x4a; goto ...; }
      iVar1 = strcmp((char *)_Str1,"ppt/_rels/presentation.xml.rels");
      if ((iVar1 == 0) && (param_3 != '\0')) { *param_4 = 0x4b; goto ...; }
      if ((*(ushort *)(param_1 + 0x1c) < 5) || ((*_Str1 != 0x64726f77 || (param_3 == '\0'))))
        *param_4 = 0x49;
```

word/ -> 0x49, xl/ -> 0x4A, ppt/ -> 0x4B — three distinct format ids inside
ZIP carving. A global singleton with a 0x2800 scan buffer (ctor DC:53202-53224).

## Product absence (all rows)

See `evidence/executions/s9-carve-registry-inventory.md` — the mechanical
inventory of all 101 FMT_* ids in `src/core/carve/cr_registry.cpp`: no
CHM/MNY/EMF/ONE/TS/3DS/SWF/JP2/MOBI/LZH/HTML/SVG/VCF/GPX/HIK/OSP/POS/AEP/
HFP/CRW rows exist; FMT_DOCX exists but with a single id (no DOCX/XLSX/
PPTX split).
