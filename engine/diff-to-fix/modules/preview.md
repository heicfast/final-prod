# modules/preview.md — preview module audit verdicts (NEW in session 4)

Reference: og's preview is a **UI-layer subsystem** — DRWUI's VAPreviewListener speaks a
local-socket/thumbnail protocol to a media-agent child process (`tcp://localhost:`
20171/20172 + ZeroMQ REP control + `EaseUS_VideoViewer.exe`): 602-byte (0x25A) MediaInfo
header, payload = size−0x25A bounded (0, 0x100000] = **1 MiB BMP cap**, 10 s bounded
wait (100×100 ms), GlobalAlloc(0x42, (size+0xDA5)&~0xFFF) HGLOBAL handoff, id
correlation at +0xC, error codes 0xFFFFFFFF/0xE/0xFD; UI-level eligibility: files >
100 MB not previewable, free-tier videos 15 s. Evidence:
`evidence/decompiled-quotes/s4-crecoverability-grading-engine.md` (context) + the
session-4 context sheets; register finding **I-015**.
**IInWindowStream is NOT a preview contract** — it is DRWUI's embedded 7-Zip/LZMA SDK
input interface (vftable :800729-800735, adjacent CLZInWindow/IMatchFinder; zero
occurrences in any other reversed tree).

Core (audited `63a9167`): `src/core/preview/cr_preview.cpp` (772 LOC) +
`include/cr/preview/cr_preview.h` — in-process bounded PreviewReader (resident prefix
→ extents → sparse zero-fill; zero-fill + `unreadable_spans` failure policy; device-read
diagnostics; `kMaxRead` 4 MiB, `kMaxDecode` 256 MiB) + `extract_metadata` (10 format
families: jpeg/png/gif/bmp/tiff/webp/wav/mp4/pdf/zip→docx + text fallback; IFD0-only
EXIF tags 274/306; moov-at-end 64 KiB tail; raw-deflate docx text snippet) + the
`cr_ui` preview ABI entries. Tests: 14 unit tests + fuzz target. Prior sessions never
audited this module (zero matrix row, zero findings) — session 4 is the first pass.

## Verdicts

- **P1/P2 (detection/recovery correctness)** — `NA`: preview reads content the engines
  already emitted; no new detection surface.
- **P15-adjacent (grade re-run on preview)** — finding **S-023**: the og re-grades
  grade-1 nodes at preview/selection time; the core has no re-grade hook.
- **P28-adjacent (parser grammar)** — `M`-with-defect: parsers are bounded and honest
  (strict fetch, short-window guards) — the docx-path exception is CLOSED: **M-028
  FIXED (s21)** (all four docx reads through the cap-gating `checked_read`).
- **P46 (contract documentation)** — **M-028 FIXED (s21)**: the header now carries the
  true anchors (og preview payload cap = 1 MiB at the UI socket layer, I-015;
  IInWindowStream = the 7-Zip/LZMA input interface, vftable :800729-800735; the
  core's 4 MiB kMaxRead = deliberate documented superset) and the compressed-extent
  whole-block cost disclosure.
- **P47/P49 (throughput/memory on untrusted input)** — **M-028 FIXED (s21)**: the
  executed bypass (65,568 device bytes against a 4,096-byte cap, `truncated=false`)
  is closed by cost-gated budgets — `PreviewReader::read_cost` prices every fetch
  BEFORE the device is touched (compressed windows cost the whole psize block);
  a 4 KiB budget now yields `device_bytes ≤ 4096, truncated=true, archive/zip`.
- **P65 (test coverage)** — **M-028 FIXED (s21)**: the caps-bypass class is pinned
  (DocxCapsMaxReadHonored / DocxTailBeyondBudgetDegradesToZip /
  CompressedExtentReportsTruncatedUnderBudget /
  CompressedExtentWholeBlockReadDisclosed / MetadataTruncatedFlagTruthfulAllPaths,
  all fail-before against the executed scenario) + the fuzz harness gained the
  input-derived tight-budget trap; the prior 14 tests unchanged and green.
- **P54/P56 (module boundaries)** — `M`-equivalent (deliberate): in-process reader vs
  the og's socket protocol — register **I-015**.
- **P69 (dead code)** — `M`: no stubs; all parsers live.

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-028 | MAJOR | Caps.max_read bypass on the zip/docx path (executed proof) + 4 MiB/IInWindowStream provenance misattributions + compressed-extent whole-block reads — **FIXED (s21)** |
| I-015 | INFORMATIONAL | Preview architecture register: og socket protocol (1 MiB BMP cap, 602-byte header, 10 s wait, 100 MB eligibility) vs core in-process reader — deliberate |

## Not-audit-note

The "mov" format is documented in `cr_preview.h:88`'s comment but never produced by
any parser path (only "mp4" is; MP4 brand detection covers qt brands) — cosmetic doc
drift, folded into M-028's doc-fix step.

---

## Dev-sign session wave 4 (2026-10-28, audited `dd82db9` — UIUX/10-preview-subsystem.md's first read)

19 parameters from the never-cited preview artifact: 6 MATCH (mid-scan
preview via live snapshot, streaming range-read semantics, failure-surface
truthfulness, plus 3), 6 NOT-IMPLEMENTED as deliberate UI/host scope, 6
already covered (I-015 socket protocol + 100 MB/15 s caps, S-023
Recovery-Chances grading, M-021/I-010 repair bridge, M-028 IInWindowStream).
NEW:

- **M-066 (MAJOR)** — archive-content enumeration at preview time: the og's
  ArchivePreviewModel family browses damaged zips in preview; the core's
  zip branch hunts only `word/document.xml` (`cr_preview.cpp:685-715`), and
  S-032's children are carve-path/tree-level only — FS-origin archives
  preview opaque.
- **N-088 (MINOR)** — M-028's IInWindowStream correction didn't reach
  `cr_ui.h:1035` / `docs/API-SURFACE-AUDIT.md:94`; `cr_preview.h:116` lists
  a never-produced `mov` format.

## Fix-session delta (M-066 + N-088 closed — 2026-10-28)

- **M-066 → FIXED**: the archive-content inventory (Metadata.entries:
  name/size/compressed/crc_present; kMaxPreviewArchiveEntries = 512 +
  truncated) collected on the existing CD walk inside docx_sniff; the
  docx upgrade coexists; FS-origin archives now enumerate (the
  ArchivePreviewModel contract). Belt fix: extract_metadata's final
  truncated ASSIGNMENT clobbered the inventory flag -> OR. 4 tests.
- **N-088 → FIXED**: cr_ui.h's provenance claim, API-SURFACE-AUDIT:94's
  row name, REFERENCE-API-GAP R-33 corrected (IInWindowStream = the
  7-Zip/LZMA SDK input interface, M-028); the mov ghost removed from
  the format comment; the PreviewDocs grep guard holds the three
  contract sites.
