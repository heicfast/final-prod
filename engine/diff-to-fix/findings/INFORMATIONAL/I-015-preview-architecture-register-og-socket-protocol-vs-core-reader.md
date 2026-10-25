# INFORMATIONAL I-015 — Preview architecture register: the og previews through a UI-layer media-agent socket protocol; the core's in-process bounded reader is the deliberate engine-side equivalent

**Severity:** INFORMATIONAL (deliberate architectural difference — recorded so the reference protocol constants and caps are never lost, and so parity tests target the right layer)
**Module:** preview (new module row — modules/preview.md)
**Parameters affected:** P15-adjacent (grade re-run on preview — see S-023), P46 (contract documentation), P54/P56 (module boundaries — where preview lives), P65-adjacent (test coverage)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — VAPreviewListener `FUN_1403dbab0` (:654520-654571) + `FUN_1403dbd10` (:654593-654616): 602-byte (0x25A) MediaInfo header, payload = size(+0x4) − 0x25A bounded (0, 0x100000], GlobalAlloc(0x42, (size+0xDA5) & ~0xFFF) HGLOBAL handoff, 100×100 ms = 10 s bounded wait, error codes 0xFFFFFFFF/0xE/0xFD, id correlation at +0xC; ZeroMQ REP control channel + `tcp://localhost:` ports 20171/20172 (:654442-654453), child process `VideoViewer\EaseUS_VideoViewer.exe`; listener thread `_LaunchPad` (:655949); UI-level caps: file cap 100 MB ("Files larger than 100MB are not available for preview."), free-tier video "15s preview only" (UIUX artifacts); seed doc `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md` §4 (verified this session — GlobalAlloc flags corrected to 0x42)
**Core references:** `include/cr/preview/cr_preview.h` + `src/core/preview/cr_preview.cpp` (in-process PreviewReader over the extent list + bounded metadata extraction; kMaxRead 4 MiB; kMaxDecode 256 MiB); `src/ui/cr_ui.cpp` preview ABI entries; audited commit `63a9167`
**Status:** READ-ONLY REGISTER — deliberate difference; no fix requested

## Summary

The og's preview is a UI-layer subsystem: DRWUI speaks a local-socket/thumbnail protocol to a media-agent child process (602-byte header + ≤1 MiB BMP payload, 10 s bounded wait, HGLOBAL handoff, 100 MB file-eligibility cap, 15 s free-tier video cap). The core implements preview as an in-process bounded reader over the node's extent list — the correct engine-side equivalent for a library with no UI process. This register preserves the og protocol constants and caps so (a) parity tests target the right layer, (b) a future host building the UI layer has the reference contract, and (c) the core's own caps (M-028) are not confused with the og's.

## What the reference does

Protocol (fully decoded, verbatim quotes in
`evidence/decompiled-quotes/s4-crecoverability-grading-engine.md` §context and the
session-4 context sheets):

- Request `SyncWait4DataBack`: socket connect wait 100 ms → `send(request)` → poll
  dataReady 100×100 ms (10 s) → response id must match request id → payload =
  `resp.size(+0x4) − 0x25A` must be in (0, 0x100000] → `GlobalAlloc(GMEM_MOVEABLE|
  GMEM_ZEROINIT, (size+0xDA5) & ~0xFFF)` → copy payload → HGLOBAL out.
- Receive `RecvThumbnailBmpData`: fixed `recv(fd, buf, 0x25A)` header first; short →
  0xFFFFFFFF; total from header.size, bounded (0, 0x100000]; streaming recv loop;
  incomplete → 0xFFFFFFFD; bad total → 0xFFFFFFFE.
- Transport: TCP 127.0.0.1:20171/20172 + ZeroMQ REP control channel (40-byte frames,
  SO_RCVTIMEO 1000) + `EaseUS_VideoViewer.exe` child process.
- UI-layer eligibility caps: files > 100 MB not previewable; free-tier videos 15 s.
- Per-selection re-grading fires from the preview controller (S-023's re-grade hook).

## What our core does

In-process `PreviewReader` (bounded read_at over resident+extents+sparse, zero-fill +
unreadable_spans, device-read diagnostics) + `extract_metadata` (10 format families,
EXIF orientation/datetime, moov-at-end tail window, raw-deflate docx text snippet) + a
`cr_ui` preview ABI. No socket, no child process, no HGLOBAL (correct for a
cross-platform library). Caps are the core's own engineering (4 MiB read / 256 MiB
decode — see M-028 for the bypass + provenance corrections).

## Gap

None as a defect. Recorded differences: protocol layer (socket RPC vs in-process),
payload cap (og 1 MiB BMP vs core 4 MiB read window — different layers, both
deliberate), eligibility caps (og UI 100 MB/15 s — a host concern; a core-side
equivalent would belong in the query/preview eligibility surface if ever needed).

## Impact

Parity tests must not attempt byte-comparison of preview payloads against og
thumbnails (different rendering layers). Hosts building the UI get the reference
constants here. M-028's cap provenance corrections anchor here.

## Reproduction / verification

Decompiled evidence (method 2): all protocol constants quoted with file:line this
session (two-agent convergence; GlobalAlloc flags corrected to 0x42 by the decompile
leg — CORPUS-ERRATA row filed).

## Fix plan (for the engineering team)

None required. Optional: link this register from CAPABILITY-MATRIX's preview row and
from any future host-side preview implementation doc.

## Tests to add

None (register).

## Related findings

- **M-028** — the core-side cap contract defects + provenance corrections.
- **S-023** — the re-grade-on-selection hook that fires from the og's preview
  controller.

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** kMaxRead 4 MiB / kMaxDecode 256 MiB confirmed in cr_preview.h this session; the og's socket-protocol constants (602-byte MediaInfo, 10 s bounded wait, ports 20171/20172, 100 MB UI cap) are recorded in the finding so parity tests target the right layer.
**Resolution notes:** Deliberate architectural difference, preserved: the og previews through a UI-layer ZeroMQ media-agent child process; the core's in-process bounded reader is the engine-side equivalent. The reference constants stay registered for the day a UI layer exists. No core change.

**Status:** ACKNOWLEDGED (deliberate architectural difference — register preserved)
**Verified by:** this session: core preview bounds re-confirmed
(`cr_preview.cpp:24` kMaxRead clamp at read time; `:59` kMaxDecode bound
in metadata extraction). The og protocol constants (602-byte 0x25A
header, payload (0, 0x100000], GlobalAlloc 0x42, ports 20171/20172,
100×100 ms) remain recorded in this finding and its evidence file.
**Resolution notes:** no code change. Registered as PARITY-DELTAS
**D-59h**. Parity tests must not byte-compare preview payloads across
the two layers; hosts building a UI get the reference contract here.
