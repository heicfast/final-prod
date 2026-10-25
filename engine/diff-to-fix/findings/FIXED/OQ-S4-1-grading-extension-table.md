# OQ-S4-1 — RESOLVED (session 5): the 169-entry grading extension table dumped byte-exact

**Status:** RESOLVED (session 5, 2026-10-26) · **Module:** drwui/grading · **Parameters:** P15
**Audited commit:** `7cf6754` · corpus `03ad46c` · **Resolution evidence:** `../evidence/decompiled-quotes/s5-grading-extension-table-dump.md`

## Resolution

The table (VA 0x141bb0500, in `.data` not `.rdata` — corrected) is a `char[9]`-strided
array of 169 extension keys + sentinel, inserted by ctor FUN_140389740 into a global
`std::set<std::string>` under a plain `i < 2000` loop (the set gains a spurious `""` from
zero padding — a faithful port reproduces it). No grades live in the table; the sole
consumer FUN_14038ac90 takes the substring after the LAST dot, ASCII-upercases it, and
does an exact `set::find`. In-table → deep validation (twins → grade 2, preview flag bit 3
of +0x73); not-in-table → grade stays 1. Quirks: entry 0 "Raw" is mixed-case DEAD (the
functional key is "RAW" at idx 86); a separate 20-string lowercase set in the same ctor
is a different member family — do not conflate.

The full 169-entry dump is in the evidence file. S-023's fix-plan step 3 is actionable;
the addendum in the S-023 finding file carries the same summary.

---

## Convergence record (session-5 audit leg, 2026-10-26)

The close-all-tiers fix leg resolved this OQ INDEPENDENTLY in the same window
(first-hand extraction from the same binary/VA; 169 × 9-byte entries in `.data`;
every flag byte 0 — a pure membership gate). Two independent decodes agree on
structure, count, and location. Their dump enumerates the extension list:
first key "Raw" (pseudo-extension for the raw-sign family), then 168 real
extensions in category-grouped order: 3DS 3GP ADP ANI AI ASF WMA WMV AU AVI
BMP CAB CDR CHM CLK CMX CPT CRW CSL CUR DBX DLL DOC DWG DXF EMF EPS EXE FH11
GIF GZ HLP HTML ICO JPG LZH MAX MID MDB MNY MOF MOV MP3 MP4 M4A M4V MPP OLE
ONE PDF PNG PPT PUB PSD PSP PST QPW RAR RIFF RTF SHW SWF TAR TIF TTF VSD WAV
WPD WPG WMF XLS ZIP DOCX XLSX PPTX ACCDB NEF NRW AAC AIFF APE OGG CR2 SR2
ARW RAW RW2 DNG RAF FLV SVG SIT SITX RM RMVB MKV DCR KDC 3FR ERF MEF MRW
MDC PEF PTX SRW ORF X3F MPG PAGES KEY NUMBERS DMG EPUB FCPEVENT M2TS MUSE
MTS CR3 MXF HWP HWPX M2T HEIC HEIF WEBP M1V JP2 JPF JPM STL RVT MOBI XMIND
MMAP VCF M4P WEBM POS EDDX WFP VPJ OSP HFP DRP HIK ODP ODS ODT 7Z GPX INSV
INSP GPR CRM AEP FFF DPX MUSX XIB BRAW NEV R3D PIX MAT SLN VCPROJ COMMON —
with the 2000-entry loop bound as terminator (their extraction) and the
`std::set` + last-dot/uppercase/exact-find consumption + `""`-membership +
dead-"Raw" quirks (this leg's decode, above). Both halves are folded into
S-023's addendum; the fix plan's step 3 is actionable from either.
