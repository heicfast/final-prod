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
  (strict fetch, short-window guards) EXCEPT the docx path — finding **M-028**.
- **P46 (contract documentation)** — finding **M-028**: the header's corpus anchors are
  misattributed (the "4 MiB corpus preview window" is the og DEVICE read window
  `Device_decompiled.c:56545`; the og's preview cap is 1 MiB; IInWindowStream is
  7-Zip's), and the "All parsers obey Caps" claim is falsified by execution.
- **P47/P49 (throughput/memory on untrusted input)** — finding **M-028** (executed:
  65,568 device bytes pulled against a 4,096-byte cap, `truncated=false`; compressed
  extents read whole `psize` ≤256 MiB per request).
- **P65 (test coverage)** — finding **M-028**: the caps-bypass class is unpinned; TIFF/
  WEBP dims, moov-at-end, kMaxRead/kMaxDecode, Caps truncation partially pinned by the
  existing 14 tests.
- **P54/P56 (module boundaries)** — `M`-equivalent (deliberate): in-process reader vs
  the og's socket protocol — register **I-015**.
- **P69 (dead code)** — `M`: no stubs; all parsers live.

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-028 | MAJOR | Caps.max_read bypass on the zip/docx path (executed proof) + 4 MiB/IInWindowStream provenance misattributions + compressed-extent whole-block reads |
| I-015 | INFORMATIONAL | Preview architecture register: og socket protocol (1 MiB BMP cap, 602-byte header, 10 s wait, 100 MB eligibility) vs core in-process reader — deliberate |

## Not-audit-note

The "mov" format is documented in `cr_preview.h:88`'s comment but never produced by
any parser path (only "mp4" is; MP4 brand detection covers qt brands) — cosmetic doc
drift, folded into M-028's doc-fix step.
