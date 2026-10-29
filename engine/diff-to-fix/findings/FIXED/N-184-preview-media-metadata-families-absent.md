# MINOR N-184 — preview metadata covers MP4/WAV only: the og's media metadata families (AVI/MKV/MOV/FLAC/MP3/…) have no core extractors

**Severity:** MINOR
**Module:** preview (the preview focus battery's residue)
**Parameters affected:** #46 Documentation of the contract (metadata families); #52 scalability of the extractors
**Corpus references:** `re-workspace/artifacts/UIUX/10-preview-subsystem.md` (the preview type map); `re-workspace/artifacts/DRWUI-subsystems--assistant-grading-preview.md:66-98` (the media-agent path — all media metadata via the agent); `re-workspace/artifacts/UIUX/18-preview-architecture.md` (the stub family)
**Core references:** `src/core/preview/cr_preview.cpp:985-1079` (`extract_metadata`'s magic families: jpeg/png/gif/bmp/tiff/webp/wav/mp4/pdf/zip-docx — the media families are **wav + mp4 only**; avi/mkv/mov/flac/mp3/m4a fall to `kind="unsupported"`); the registered OQ-S10-1 scope (thumbnails' codec-bound formats — this finding is the **metadata** siblings, not the thumbnail decoders) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's preview pipeline derives metadata for every media family its agent opens (ffmpeg-side: AVI, MKV, MOV, MP3, FLAC, M4A…); the core's `extract_metadata` implements exactly two media formats (MP4 via ftyp/moov, WAV via fmt/data) — the remaining media extensions return `unsupported` with no duration/codec/dims, so hosts cannot render the og's per-type preview affordances for them.

## What the reference does

All media metadata flows through the media-agent path (VAPreviewListener; the 602-byte MediaInfo header carries the fields the UI renders); the engine-side supply is the extent list the agent reads. The og's media surface is therefore as wide as its ffmpeg build — every common container/codec family.

## What our core does

`cr_preview.cpp:985-1079`: MP4 (brand + moov/mvhd duration, head/tail 64 KiB windows) and WAV (fmt/data walk → codec + duration) are the only media branches; the magic dispatch has no entries for the RIFF-AVI family (`RIFF…AVI `), Matroska (`0x1A45DFA3`), FLAC (`fLaC`), MP3 (`ID3`/frame sync), or M4A — those extensions return `kind="unsupported"` (the eligibility gate then hides them from preview surfaces that key on metadata).

## Gap

Stated falsifiably: a `.mov` or `.mkv` in the results yields `{duration, codec}` metadata in the og's preview and `unsupported` from `cr_ui_preview_metadata` in the core (a `.mov` is especially user-visible: it is MP4-family in the og's naming contract per S-049).

## Impact

Preview metadata gaps for the non-MP4/WAV media families; hosts degrade those rows. Bounded by the fact that thumbnails for those families are already tracked (OQ-S10-1) — this is the metadata leg. MINOR.

## Reproduction / verification

Core evidence first-hand (the magic-family table read this session; the gap families verified absent by grep over the dispatch at `cr_preview.cpp:985-1079` — `RIFF…AVI `, `0x1A45DFA3`, `fLaC`, `ID3`/frame-sync have no branch; M4A is not in the MP4 branch's brand test). Corpus evidence: the preview subsystem map (`UIUX/10` + `DRWUI-subsystems:66-98`). See `evidence/s42-liveness-sweep.md` §4 for the preview battery's covered/residue split.

## Fix plan (for the engineering team)

1. Add the four cheap extractors to the magic table: AVI (RIFF/`AVI `/`avih` → fps + stream count), Matroska (EBML Segment → Tracks/duration), FLAC (`fLaC` STREAMINFO → sample rate/channels/duration), MP3 (ID3v2 skip + frame-sync walk → duration estimate; M4A rides the MP4 branch — add the `M4A `/brand test).
2. Keep each extractor under the existing `Caps::max_decode_ms` budget (head-window walks only).
3. Tests per family with tiny fixtures; `unsupported` remains for everything else.

## Tests to add

- `Preview.AviMetadataExtracted`, `Preview.MkvMetadataExtracted`, `Preview.FlacMetadataExtracted`, `Preview.Mp3DurationEstimated`, `Preview.M4aTypedAsMp4Family` (all fail today)

## Related findings

OQ-S10-1 (thumbnail codecs — the render siblings), M-116 (archive entries ABI seam — the other metadata-family gap, already active).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-184 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `PreviewMeta.AviMetadataExtracted` (LIST/hdrl walk: avih 40ms × 100 frames → 4000 ms + the 2-strh stream count), `PreviewMeta.MkvMetadataExtracted` (the full EBML vint chain: Segment → Info/TimecodeScale/Duration float → 2500 ms + the 2-TrackEntry count), `PreviewMeta.FlacMetadataExtracted` (STREAMINFO: 44100 Hz, 2 channels, 441000 samples → 10000 ms), `PreviewMeta.Mp3DurationEstimated` (ID3v2 syncsafe skip + the 128 kbps frame → the size-based estimate), `PreviewMeta.M4aTypedAsMp4Family` (the M4A brand rides the ftyp branch — the pin that branch always carried it implicitly). RED pre-fix: 4 of 5 FAILED (kind "unsupported"); GREEN post-fix: 5/5. The two UTF-16-BOM text tests caught the MP3 frame-sync collision mid-RED (the BOM pair now excluded — the og's extension-driven agent never routes .txt to media). Suite 1341 ran / 1299 pass / 42 env-skips / 0 fail.
**Resolution notes:** Four head-window extractors landed (avi_info/mkv_info/flac_info/mp3_info), each bounded by the existing Window budget — no full-file walks. The Matroska walk implements the minimal EBML vint grammar (id/size readers in an n184 namespace) for Info's TimecodeScale+Duration and the TrackEntry count; two real bugs were caught and fixed during the RED pass by the pin (the 4-byte EBML float was memcpy'd into a double — zero every time; the strict 8-byte element fetch refused the final partial TrackEntry of a short file — the reader now uses fetch_any with the prefix-length validation). The BOM exclusion on the MP3 magic is the one deliberate narrowing (a file starting FF FE is a UTF-16 text on the magic-first dispatch; the og's extension-driven agent produces the same net result). M4A/MOV needed no code — they are ISO-BMFF containers riding the ftyp branch; the M4A pin proves the family typing the finding asked for. `unsupported` remains the default for everything else.
