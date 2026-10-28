# SEVERE S-049 — The og's universal carved-file naming contract (`<PREFIX>_%06d.EXT` with content-derived prefixes) is absent: core's `File%06u.ext` default fires for every format except MP4/MOV, and N-063's recorded remainder is now decoded

**Severity:** SEVERE
**Module:** carve (naming layer)
**Parameters affected:** B10 (filename handling for nameless recoveries), A28-adjacent (analyser detail: the per-format prefix extraction), B4-adjacent (recovered-file presentation), D12/D13 (session tree content), Q68-adjacent (documentation: the PARITY-DELTAS "rule-less formats" registration is now falsifiable)
**Corpus references:** EXECUTED — the three real og sessions: the dominant carved name across EVERY format is `EXT_%06d.EXT` (e.g. `GZ_000001.GZ` ×53,891, `PNG_N.PNG` ×42,412, `SVG_N.SVG` ×43,545, `FCPEVENT_000349.FCPEVENT`); content-derived prefixes observed: `JPG_192x144_000001.JPG` (SOF dimensions, 405 rows), `GT-S7562_000123.JPG`/`SM-G610F_N.JPG` (EXIF Model, 200+ rows), `Adobe Photoshop 7.0_N.JPG`/`GIMP 2.10.14_N.JPG`/`ACD Systems Digital Imaging_N.JPG` (EXIF Software, 320+ rows), `AREE DEEWANO MUJHE PECHANO_000123.MP3` (ID3 title), `2020-05-23 23-23-23_N.PDF` ×91 + `2006-07-25 02-16-00_N.DOCX` ×24 (creation-timestamp names), `META-INF__N.ZIP`/`__MACOSX__N.ZIP`/`LICENSE_OFL_N.ZIP` (ZIP central-directory first entries) — verbatim table in `evidence/executions/s10b-real-rsf-session-mining.md`; decompile: DRWUI `FUN_140c44770` @ `DRWUI_decompiled.c:1878330-1878381` — `L"%s_%06d"` (`:1878379`) with the `>99999 → L"%s_%d"` fallback and the prefix-less `L"%06d"` (`:1878369`) branch, `snprintf(buf, 0xff, ...)`
**Core references:** audited `62074bf` — `src/core/carve/cr_carve.cpp:907-921` (`default_namer` = `File%06u.%s`; its comment: "the og has NO FileNNNN scheme anywhere... OUR deliberate default for formats whose og naming rule is not decoded (registered in docs/PARITY-DELTAS.md)"), `:429-432` (the embedded_name > namer > default chain), `src/core/carve/cr_analysers.cpp:727`/`:736` (the ONLY two embedded_name producers — `mp4_og_name` for MP4/MOV per N-063); `docs/parity/PARITY-DELTAS.md` D-76 (the File%06u deliberate-default registration)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og names every carved file `<PREFIX>_<6-digit-counter>.<EXT>`, where the prefix defaults to the uppercase EXT and is replaced by content-derived strings per format (SOF dimensions, EXIF Model/Software for JPEG; ID3 title for MP3; creation timestamps for PDF/DOCX; central-directory entry names for ZIP). Our core names carved files `File%06u.<ext>` for every format except MP4/MOV (N-063's landing), and the code comment claims the og's naming rule is "not decoded" for those formats — the real sessions decode it. This is N-063's explicitly recorded remainder, now resolved with executed evidence.

## What the reference does

Two-layer contract, both pinned:
1. **The naming function** (DRWUI `FUN_140c44770`, decompile-cited): name = `prefix_%06d` when the analyser supplied a prefix string, bare `%06d` when it did not; both switch to unpadded `%d`/`%s_%d` past counter 99999; buffer 0xff.
2. **The prefix sources** (real-session-decoded): the default prefix is the format's own uppercase EXT (`JPG_000001.JPG`, `PNG_000001.PNG` — every observed format follows this); per-format analysers override the prefix with content-derived strings — the JPEG analyser reads SOF dimensions (`JPG_192x144_000001.JPG`) and EXIF Model/Software (`GT-S7562_NNNNNN.JPG`, `Adobe Photoshop 7.0_N.JPG`); MP3 gets ID3 titles; PDF/DOCX get creation-timestamp strings (`2020-05-23 23-23-23_N.PDF`); ZIP gets first central-directory entry names (`META-INF__N.ZIP`). The names land in the persisted sessions — this is the shipped product behavior on real user data, not a UI-side rendering choice.

## What our core does

`default_namer` (`cr_carve.cpp:907`) formats `File%06u.<ext>` — e.g. `File000123.JPG` where the og emits `JPG_000123.JPG`. The embedded_name carrier (N-063) is populated only by `mp4_og_name` (MP4/MOV) — `cr_analysers.cpp:727/:736` are the only two assignment sites. The comment on `default_namer` asserts the og's rule is undecoded for all other formats and registers `File%06u` as a deliberate default in PARITY-DELTAS (D-76). That registration's premise ("formats whose og naming rule is not decoded") is now falsifiable: the rule IS decoded — universally — from the real sessions plus the DRWUI function.

## Gap

Falsifiable: carve a JPEG carrying EXIF Model `GT-S7562` from a fixture. The og's persisted session carries `GT-S7562_000123.JPG` (or `JPG_192x144_000123.JPG` by dimension); ours carries `File000123.jpg`. Same content, same confidence — different name on every carved file of every format (except MP4/MOV where N-063's analyser template applies, and even there the real sessions show the `EXT_%06d` form, see below).

## Impact

Every carved file's name diverges from the og's — user-observable quality at the most-viewed layer of the product (the recovered-file list), and information-bearing: the og's EXIF-model/ID3-title/timestamp/ZIP-entry prefixes surface real file identity that our `File%06u` discards entirely (a user scanning a gallery of 50,000 carved photos cannot tell camera-taken files apart; the og can). Session trees diverge content-wise from the og's (D12/D13). The N-063 "remainder" status in PARITY-DELTAS D-76 is stale and must be re-registered.

## Reproduction / verification

Executed comparison (§6.1): the three real og sessions parsed; the per-format name-pattern census extracted (the verbatim table: 34 formats' dominant `EXT_N` pattern plus the 5 content-derived prefix families — full distributions with counts in the evidence file). The naming function re-read at `DRWUI_decompiled.c:1878330-1878381` first-hand (the `%s_%06d`/`%06d`/`>99999` literals quoted). Core side: `default_namer` read at `cr_carve.cpp:907`; the embedded_name sites enumerated by grep (`cr_analysers.cpp:727/:736` only).

## Fix plan (for the engineering team)

1. Replace the `File%06u` default with the og contract: `<UPPERCASE-EXT>_%06d.<ext>`, with the `%d` fallback past 99999 (FUN_140c44770 both branches). Keep the caller-provided `namer` hook ahead of the default exactly as now.
2. Add per-analyser prefix extraction onto the existing `CarveResult::embedded_name` carrier (no new plumbing — N-063's precedence chain already prefers it):
   - JPEG: SOF dimensions prefix `EXT_<W>x<H>` (the dimensions are already parsed for the S-018 salvage inputs — reuse the SOF walk), then EXIF Model, then EXIF Software (in that precedence, per the real-name co-occurrence; pin with fixtures).
   - MP3: ID3v2 title (the tag walk exists in the analyser family).
   - PDF: the creation-timestamp string `YYYY-MM-DD HH-MM-SS` (the trailer/info parse the analyser already does for other fields).
   - DOCX: docProps created date (same format).
   - ZIP: first central-directory entry name (S-032's inventory walk is the source).
   - Formats without a decoded content prefix keep the plain `EXT_%06d` default.
3. Files: `src/core/carve/cr_carve.cpp` (`default_namer`), `src/core/carve/cr_analysers.cpp` (the per-analyser extractions), the PARITY-DELTAS D-76 row (re-register: the naming contract is decoded; the deliberate-default registration narrows to nothing), `docs/` carve contract notes.
4. MP4/MOV nuance (recorded for the fix lane): the N-063 analyser template ("Lost Name File (...)EaseUS") did not fire in any of the 57 real-session MP4/MOV carves — all are `MP4_%06d.MP4`/`MOV_%06d.MOV`. Before touching N-063's landed template, decode which analyser branch supplies DRWUI's prefix for carved MP4s (the mp4_og_name template may apply to a non-carve flow); pin with an executed fixture comparison if possible.
5. Tests (fixture-spec-derived, never implementation-mirrored — the S-042 class lesson):
   - `Carve.OgNamingUniversalExtCounter` — plain PNG → `PNG_000001.PNG`.
   - `Carve.OgNamingCounterPast99999` — `%d` fallback pinned.
   - `Carve.OgNamingJpgSofDimensions` — a spec JPEG with known SOF → `JPG_<W>x<H>_%06d.JPG`.
   - `Carve.OgNamingJpgExifModel` / `Carve.OgNamingJpgExifSoftware` — spec EXIF → the model/software prefix.
   - `Carve.OgNamingMp3Id3Title` — spec ID3v2 tag → title prefix.
   - `Carve.OgNamingPdfTimestamp` / `Carve.OgNamingDocxTimestamp`.
   - `Carve.OgNamingZipFirstEntry` — spec ZIP central directory → first-entry prefix.
   - `Carve.FileNnnnFallbackRemoved` — `File%06u` asserted ABSENT (the retired default).

## Tests to add

See fix plan step 5 — nine named scenarios, each derived from the real-session evidence table in the evidence file (the real names are the fixtures' expected outputs).

## Related findings

- S-048 (the branch these names live under — same fix wave)
- N-063 (FIXED; its recorded remainder is this finding — the Resolution's "next concrete step" executed)
- S-032 (the ZIP inventory walk feeds the ZIP-entry prefix)
- S-018 (the SOF dimension inputs already parsed for salvage)

## Open questions

The exact precedence when a JPEG carries BOTH EXIF Model and Software (real sessions show both families; no observed co-occurrence sample settles order) — the fix lane should pin with a fixture that mirrors the og's behaviour, decoded at the analyser if ambiguous. The MP4/MOV template-vs-EXT interplay (fix plan step 4).


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

cr_carve.cpp cites drifted: default_namer now :934-951 (was :907-921), the format literal at :944; the precedence chain now :451-459 (was :429-432); the only two embedded_name producers remain cr_analysers.cpp:727/:736 (MP4/MOV) — the M-093/M-096 landings ADDED formats (XML/SVG/MOBI) without prefix extractors, so this finding's scope GREW: every newly-carved format also defaults to File%06u
