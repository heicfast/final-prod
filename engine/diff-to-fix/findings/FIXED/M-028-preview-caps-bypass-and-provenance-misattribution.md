# MAJOR M-028 — Preview module: Caps.max_read bypass on the zip/docx path (executed proof), 4 MiB/IInWindowStream provenance misattributions, compressed-extent over-read

**Severity:** MAJOR
**Module:** preview (new module row — modules/preview.md)
**Parameters affected:** P46 (documentation of the contract — falsified), P65 (test coverage — the bypass is unpinned), P47/P49 (performance — bounded amplification on untrusted input), P28-adjacent (parser grammar correctness)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` — preview payload cap **1 MiB** (`0x100000`) at both gates (:654557, :654601); 602-byte MediaInfo header (:654593); **4 MiB = the DEVICE read window** (`0x400000/sectorSize`, `ghidra-mo/Device_decompiled.c:56545`), not a preview cap; **IInWindowStream = the 7-Zip/LZMA SDK input interface** (vftable :800729-800735, adjacent CLZInWindow/IMatchFinder; zero occurrences in any other reversed tree) — NOT a preview contract
**Core references:** `src/core/preview/cr_preview.cpp:572-676` (docx_sniff — direct `w.pr->read_at` calls at :583/:613/:639/:647 with `w.pulled +=` accounting but no `caps.max_read` check), `:109-110` ("All parsers obey Caps" — false), `include/cr/preview/cr_preview.h:10-11` ("mirrors the corpus 4 MiB preview window ... IInWindowStream" — both misattributed), `:12-13` ("exactly `len` bytes are assembled per call"), `:22` ("Untrusted input never makes the extractor read more than the caps allow" — falsified by execution), `:63-64` (diagnostics comment contradicted by the compressed-extent path), `src/core/preview/cr_preview.cpp:54-89` (compressed extent reads whole `psize`), audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The preview module's bounded-read contract is violated in three places, one proven by execution this session: (1) `docx_sniff()` reads directly through the PreviewReader — up to ~130 KiB (64 KiB tail + ≤64 KiB central directory + 30 + 2048) — while never checking `caps.max_read`, and reports `truncated=false`; an executed harness pulled 65,568 device bytes against a 4,096-byte cap. (2) The header's claimed corpus provenance is doubly misattributed: the og's preview payload cap is 1 MiB (not 4 MiB — 4 MiB is the og device layer's read-batching window), and `IInWindowStream` is the 7-Zip LZ decoder's input interface, not a preview streaming contract. (3) Compressed extents read and decode the whole `psize` block (≤256 MiB) regardless of the requested window, contradicting the "never more than requested" diagnostics comment.

## What the reference does

- **Preview payload cap = 1 MiB**: the only thumbnail/preview size bound anywhere in the
  og is `payload = resp.size(+0x4) − 0x25A` bounded to `(0, 0x100000]` at both the
  request-validation gate (`FUN_1403dbab0:654557`) and the receive gate
  (`FUN_1403dbd10:654601`).
- **The 4 MiB constant is the device read window** — `Device_decompiled.c:56545`:
  `maxSectorsPerRead = 0x400000 / sectorSize`, consumed by the sector-aligned
  synchronous ReadFile loop (`FUN_180062600:57010-57028`). It applies to ALL device
  reads, not preview. A "corpus 4 MiB preview window" does not exist.
- **IInWindowStream exists only in DRWUI.exe and is the 7-Zip/LZMA SDK's
  `IInWindowStream`** (the LZ77 match-window input: `HRESULT Read(void*, UInt32,
  UInt32*)` + `Seek`), sitting in the embedded 7-Zip stack between `CLZInWindow` and
  `IMatchFinder` vtables (:800704-800775). Zero occurrences in any other reversed tree.
- The og preview pipeline is a UI-layer socket protocol (602-byte MediaInfo header +
  ≤1 MiB BMP payload, 10 s bounded wait, HGLOBAL handoff — see I-015), with a UI-level
  100 MB file-eligibility cap; no in-engine "preview reader" exists to cap. The
  bounded-read idea is the core's own (good) design — but its claimed corpus anchoring
  is false.

## What our core does

- `cr_preview.h:10-11`: "Bounded by construction (aligned with the RE corpus preview
  contract IInWindowStream: range reads under explicit caps)" — **both anchors are
  misattributed** (IInWindowStream is 7-Zip's; the og's preview cap is 1 MiB, set at the
  UI socket layer, not 4 MiB).
- `cr_preview.h:41-42`: `kMaxRead = 4u << 20` "mirrors the corpus 4 MiB preview window" —
  the cited corpus fact does not exist; the closest corpus constant with that value is
  the device read-batching window.
- `cr_preview.cpp:572-676` `docx_sniff()`: the EOCD tail window (≤64 KiB, :580-587),
  central directory (≤64 KiB, :611-613), local file header (:638-641) and deflate window
  (:646-651) all call `w.pr->read_at(...)` **directly** — bypassing `Window::fetch_any`'s
  cap check (:152 `if (pulled + n > caps.max_read)`). Bytes are *accounted*
  (`w.pulled += ...`) but never *enforced*; `truncated` is never set on this path.
- `cr_preview.cpp:54-89` compressed extents: `std::vector<u8> packed(size_t(psize))` +
  `source_->read_at(e.offset, packed.data(), u32(psize))` — the whole packed block is
  read and decoded for any request size (bounded by `kMaxDecode` = 256 MiB).

## Gap

Falsifiable claims:
(a) **EXECUTED**: with `caps.max_read = 4096` and a 220,158-byte zip on one extent,
`extract_metadata` issues 2 device reads totaling **65,568 bytes** with `truncated=false`
(16× the cap; worst case ≈130 KiB ≈ 32× a minimal budget). The module's documented
invariant "Untrusted input never makes the extractor read more than the caps allow"
(cr_preview.h:22) is false.
(b) The header's two corpus citations (4 MiB preview window; IInWindowStream preview
contract) are misattributions per the decompile evidence above.
(c) A 4 KiB preview request over a compressed extent of psize P causes P device bytes
to be read (P up to 256 MiB), contradicting the diagnostics contract at cr_preview.h:63-64.

## Impact

- **Fuzz/API-consumer safety contract**: any consumer (fuzz harness, constrained UI,
  sandboxed preview service) that sets a small `max_read` budget silently gets up to
  130 KiB pulled per zip-sniff call and up to `psize` per compressed extent, with
  `truncated == false` — the caller's accounting is wrong and its resource guarantees
  are broken. The reads remain bounded (no unbounded read, no memory-safety fault), so
  this is a contract/robustness defect, not a crash.
- **Documentation truthfulness**: the module's header justifies its central design
  constant with corpus facts that do not hold — a future maintainer will "verify"
  against the wrong reference.
- **Diagnostics**: `device_bytes_` will exceed the documented model on both paths.

## Reproduction / verification

- **Executed comparison (method 1)**: scratch harness `harness_preview_caps.cpp`
  (outside the repo) links the built core (`build-s4/src/core/libcr_core.a`), constructs
  a synthetic device + one-extent FileProperties carrying a crafted zip (LFH +
  `word/document.xml` + 220,026 B stored-deflate payload + CD + EOCD = 220,158 B),
  sets `caps.max_read = 4096`, calls `extract_metadata`. Verbatim output + full command
  lines: `evidence/executions/s4-preview-caps-bypass.md`. Result: 65,568 bytes pulled,
  `truncated=false`, VERDICT: CAPS BYPASSED.
- **Decompiled evidence (method 2)** for the provenance misattributions:
  `evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` §3 (4 MiB =
  device window) and the IInWindowStream decode (7-Zip family, vftable adjacency,
  `:800704-800775`).
- **Code-cited** for the compressed-extent over-read: `cr_preview.cpp:58-70` (whole
  `psize` read) vs `cr_preview.h:12-13` and `:63-64`.

## Fix plan (for the engineering team)

1. No architectural change — the `Window` cursor already implements the right
   enforcement; the docx path simply bypasses it.
2. **Files to modify**: `src/core/preview/cr_preview.cpp` —
   a. `docx_sniff()`: route the tail/CD/LFH/deflate reads through a second `Window`
      (or add a `checked_read(off, buf, len)` helper on `Window` that enforces
      `pulled + n <= caps.max_read` and sets `truncated`); a zip whose EOCD/CD falls
      outside the budget must degrade to the honest `archive/zip` label with
      `truncated=true`.
   b. Compressed extents: clamp `psize` fetch to the remaining budget when
      `in_ext_off > 0` is impossible to serve otherwise — or (simpler, honest) set
      `truncated=true` when `psize` alone exceeds `caps.max_read`, and document that
      compressed-extent previews require `max_read >= psize`.
   c. Fix the header comments: replace the "4 MiB corpus preview window" /
      "IInWindowStream" provenance with the true anchors (og preview payload cap is
      1 MiB at the UI socket layer — see I-015; the core's 4 MiB is its own
      engineering choice; IInWindowStream is 7-Zip's LZ input interface, unrelated).
      Either re-anchor `kMaxRead` to 1 MiB for og parity or keep 4 MiB as a documented
      deliberate superset (recommend keep + document — the core previews up to 4 MiB
      where the og's UI previews a 1 MiB thumbnail payload; different layers).
   d. Update `cr_preview.h:63-64`'s diagnostics comment to disclose the
      compressed-extent whole-block behavior.
3. **Tests to add** (fail before / pass after):
   `Preview.DocxCapsMaxReadHonored` (the executed scenario: 220 KB zip, max_read=4096 →
   device_bytes ≤ 4096 OR truncated=true), `Preview.DocxTailBeyondBudgetDegradesToZip`,
   `Preview.CompressedExtentReportsTruncatedUnderBudget`,
   `Preview.CompressedExtentWholeBlockReadDisclosed` (device_bytes == psize, documented),
   `Preview.MetadataTruncatedFlagTruthfulAllPaths`.
4. **Contract/doc updates**: the Caps contract paragraph in `cr_preview.h` (and any doc
   quoting it) gains the compressed-extent disclosure; CAPABILITY-MATRIX preview row
   corrected.
5. **Acceptance criteria**: the executed harness scenario (evidence file above) yields
   `device_bytes <= caps.max_read` or `truncated == true`; fuzz corpus for
   zip-structured previews runs under the configured budget; 306/306 + new tests green.

## Tests to add

`Preview.DocxCapsMaxReadHonored`, `Preview.DocxTailBeyondBudgetDegradesToZip`,
`Preview.CompressedExtentReportsTruncatedUnderBudget`,
`Preview.CompressedExtentWholeBlockReadDisclosed`,
`Preview.MetadataTruncatedFlagTruthfulAllPaths`.

## Related findings

- **I-015** — the og preview subsystem's actual architecture (socket protocol, 1 MiB
  payload cap, 100 MB eligibility, 15 s free-tier video) — the reference facts this
  finding's provenance corrections point to.
- **M-023 / M-029** — the docx sniff feeds `text_snippet` for Document-category rows;
  category model divergence is tracked there.
- **N-040** — same parameter class (documentation truthfulness) on the ABI layer.

## Open questions

1. Whether the 4 MiB `kMaxRead` should be re-anchored to the og's 1 MiB payload cap for
   strict parity or kept as a documented superset — recorded as a deliberate-choice
   question for the engineering team (recommend keep + document; see fix plan 2c).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — `git log` "fix(M-028)")
**Verified by:** PreviewMeta.DocxCapsMaxReadHonored, PreviewMeta.DocxTailBeyondBudgetDegradesToZip, PreviewMeta.CompressedExtentReportsTruncatedUnderBudget, PreviewMeta.CompressedExtentWholeBlockReadDisclosed, PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths — all five fail-before against the executed scenario reproduced first-hand (the audit's 220 KB zip geometry, max_read=4096: pre-fix pulled 65,568+ device bytes with truncated=false; post-fix device_bytes ≤ 4096 with truncated=true and the honest archive/zip degrade). Suite 662/662 linux-release. Fuzz: fuzz_preview gained an input-derived tight-budget trap (budgets 1..65,536 explore the 64 KiB tail/CD window boundaries); g++ -fsyntax-only clean, the libFuzzer campaign runs in the CI Linux lane.
**Resolution notes:** CONFIRMED-DIVERGENCE (all three sub-claims verified: the executed caps bypass, the provenance misattributions, the compressed-extent whole-block cost). The fix is architectural, matching the finding's plan: (1) `PreviewReader::read_cost(off, len)` — the pre-read DEVICE cost of any read_at call, mirroring read_at's arithmetic exactly (resident/sparse cost 0; plain extents cost their overlapped window; compressed extents cost the whole packed block; malformed compressed caps cost 0); (2) `Window::fetch_any` gates on the COST (not the byte count) before touching the device — a fetch over a compressed extent is charged psize up front, so the 64 KiB-block-under-a-4 KiB-budget case is refused before any byte moves; (3) `Window::checked_read` — the cap-enforcing direct-read helper; all four docx_sniff reads (tail window, central directory, LFH, deflate window) now route through it, degrading to the honest archive/zip label with truncated=true when the budget refuses (the pre-fix code accounted `w.pulled +=` but never enforced); (4) the over-budget head-fetch early-return now propagates truncated (pre-fix dropped the flag on exactly the compressed-head path); (5) a post-extraction belt marks truncated=true if the per-call device-byte delta exceeds caps.max_read — a future parser cannot reintroduce the bypass silently; (6) the header's provenance is corrected: the og's preview payload cap is 1 MiB at the UI socket layer (I-015, DRWUI :654557/:654601), IInWindowStream is the 7-Zip/LZMA SDK input interface (vftable :800729-800735) — the core's 4 MiB kMaxRead stays as a documented deliberate superset (different layers), and the compressed-extent whole-block cost (psize device bytes for any window into the block, per logical read) is disclosed in both the header and the WholeBlockReadDisclosed test. The disclosure choice from the finding's open question 1: keep 4 MiB + document (the recommended option).
