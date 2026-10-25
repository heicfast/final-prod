# OQ-S4-1 — The CRecoverability 169-entry extension table (blob at 0x141bb0500) was located but not dumped

**Status:** OPEN (session 4) · **Module:** drwui/grading · **Parameters:** P15 (confidence grading model — deep-validation extension gate)
**Audited commit:** `63a9167` · corpus `03ad46c`

## The precise question

S-023's decision tree gates the generic-path deep validation on the file's extension
being present in a **169-entry static table whose first key is the literal "Raw"**
(lookup at `DRWUI_decompiled.c:605297-605304`; table @ `.rdata` 0x141bb0500). The
table's remaining 168 entries were NOT dumped this session — the S-023 fix plan's
step 3 requires the exact entry set before the gate can be byte-matched.

## Corpus artifacts to examine next

1. `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` around 0x141bb0500 — dump
   the 169 entries (Ghidra: the table is a sorted string-map blob; `strings` + the
   decompile's DAT_ references will enumerate it).
2. Cross-check against `og-codebase/FileFilter.xml`'s 1,481 distinct extensions —
   the grading table is plausibly a subset (the "deep-validatable" formats).

## Acceptance

RESOLVED when the 169 entries are enumerated with extraction evidence; fold into
S-023's fix plan step 3 and re-pin its extension-gate test.

---

## Session-3 parallel-leg note (2026-10-26)

This leg's FileFilter decode answers part of this question and extends it:
the 170-entry x9-byte table `DAT_18005a090` (exFATSearchDLL :30824) is
consumed by the **lost-partition content-consistency scorer**
`FUN_180025390` (:30973-31156, reject at `considered/2 <= failed && ratio <=
0.3 && failed != 0`, wired at 5 call sites) — see finding M-035 and evidence
`evidence/decompiled-quotes/s3-filefilter-health.md` §A2. The
blast-radius probe (is the table engine-wide or exFAT-only?) is now recorded
as OQ-FLT-1 in `tracked-open/OQ-Z-E-FLT-session3.md`.

## Resolution

**Status:** RESOLVED (close-all-tiers session, 2026-10-26)
**Verified by:** first-hand extraction from the corpus binary
`og-codebase/DRWUI.exe` (.data section, VA 0x141bb0500 → file offset
0x1baef00; 169 × 9-byte entries: 8-char NUL-padded key + flag byte).
**Resolution notes:** All 169 entries enumerated; every flag byte is 0 (the
table is a pure membership gate). First key "Raw" (a pseudo-extension the
og's own raw-sign format family), then 168 real extensions in category-
grouped order: 3DS 3GP ADP ANI AI ASF WMA WMV AU AVI BMP CAB CDR CHM CLK
CMX CPT CRW CSL CUR DBX DLL DOC DWG DXF EMF EPS EXE FH11 GIF GZ HLP HTML
ICO JPG LZH MAX MID MDB MNY MOF MOV MP3 MP4 M4A M4V MPP OLE ONE PDF PNG
PPT PUB PSD PSP PST QPW RAR RIFF RTF SHW SWF TAR TIF TTF VSD WAV WPD WPG
WMF XLS ZIP DOCX XLSX PPTX ACCDB NEF NRW AAC AIFF APE OGG CR2 SR2 ARW RAW
RW2 DNG RAF FLV SVG SIT SITX RM RMVB MKV DCR KDC 3FR ERF MEF MRW MDC PEF
PTX SRW ORF X3F MPG PAGES KEY NUMBERS DMG EPUB FCPEVENT M2TS MUSE MTS CR3
MXF HWP HWPX M2T HEIC HEIF WEBP M1V JP2 JPF JPM STL RVT MOBI XMIND MMAP
VCF M4P WEBM POS EDDX WFP VPJ OSP HFP DRP HIK ODP ODS ODT 7Z GPX INSV
INSP GPR CRM AEP FFF DPX MUSX XIB BRAW NEV R3D PIX MAT SLN VCPROJ COMMON.
The extraction script (PE section walk + 9-byte entry scan) ran against
corpus 03ad46c; the terminator is the 2000-entry loop bound at the lookup
site (DRWUI_decompiled.c:605299-605301) with the table's zero entry.
Folded into S-023's fix (the deep-validation gate's membership set).
