# SEVERE S-050 — The og's file-repair line (photo/video/office/archive/HWP/raw repair — 9 product binaries, 8 corpus artifacts, product-flagged) has no core implementation AND no truthful registration: the SURFACE-INVENTORY's OOS citation points at a CAPABILITY-MATRIX §3 list that does not contain it

**Severity:** SEVERE
**Module:** mom-capabilities / product-frame (the repair line)
**Parameters affected:** A42 (public surface shape — no repair operation exists), B4-adjacent (recovery quality surface: damaged files recover-but-do-not-open), Q68 (documentation truth: the registration cites a list that lacks the row), I-010-adjacent (scope register integrity)
**Corpus references:** `re-workspace/artifacts/FileRepair/` — 8 artifacts, all never-cited by any finding: `repair-architecture.md` (the Fixo product line), `image-repair--format-grammar.md`, `image-repair--state-machines.md`, `image-repair--gif-bmp-svg-tiff-walks.md`, `archive-repair--7z-engine.md` (CArchive7zInfo/C7zNextHeaderInfo, password-aware repair pairing with the shipped `Dict`), `x3f-repair--section-checkers-walk.md`, `deepening-batch2--pdf-office-repair.md`, `hwp-pathchecker--module-decode.md`; og-codebase binaries: `EUImgRepair.exe`, `EUVideoRepair.dll`, `EUArchiveRepair.exe`, `EUAdobeRepair.exe`, `EURawImg.exe`, `EuOfficeRepairWin32.exe`, `EuOfficeTools.exe`, `EUHwpViewer.exe` + `EUHwpFilePathCheckerModule.dll`, `EUUnZip.exe`, `EUImageEnhance.dll` (post-repair enhancement, PARITY-OG-CORE-INVENTORY:80); the product-flag evidence: `og-codebase/remote_config_data` (`enable_imgrepair`, `enable_docrepair` — I-027's decode); DRWUI's repair integration: the `ln`/CHK chain (`CHKRepairFileSign` — CORRECTIONS.md #9; SCAN-ENGINE-DELTA F2: MatchedFile/NotMatchedFile) and `CPDFRepairGetPixmapThread` (PARITY-OG-CORE-INVENTORY:100)
**Core references:** audited `62074bf` — `src/core/repair/` contains ONLY `cr_rawfixer.cpp` (transactional BOOT-SECTOR/filesystem-structure repair — a different surface, the RawFixer.dll contract); `src/core/check/` is the damage-audit phase (advisory, never writes); grep-verified: no image/video/office/archive file-repair capability, no repair ABI op, no CLI verb beyond the RawFixer pair; `diff-to-fix/SURFACE-INVENTORY.md` §5 ("File repair line ... OUT-OF-SCOPE (not implemented; CAPABILITY-MATRIX §3)") vs `docs/CAPABILITY-MATRIX.md:77-87` (§3 lists ONLY: Cloud/NAS/remote recovery; BitLocker FVE-key derivation; signing; `.rsf` import — no repair row)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og bundles a complete file-repair product line — photo (JPEG/GIF/BMP/SVG/TIFF walks + state machines), video, office documents, HWP, raw photos (X3F section checkers), archives (a from-scratch 7z next-header engine, password-aware, paired with the shipped `Dict` dictionary), and post-repair image enhancement — 9+ binaries, remote-config-flagged, wired into DRWUI's repair chain and the results UX. Our core has none of it (its repair module is the different RawFixer disk-repair contract), and the only trail disposition is a SURFACE-INVENTORY row citing a CAPABILITY-MATRIX §3 exclusion list that does not actually contain the repair line. Per this session's instruction ("mark all remaining features, implementations, functions, as finding too") the surface is filed: it is the largest unregistered capability gap between the og product and the merged product.

## What the reference does

The corpus's FileRepair artifacts document the line end-to-end: the Fixo-source provenance (`D:\Fixo\01_Project\00_Source\_main\...`), the image-repair format grammars and per-format state machines, the archive engine (CArchive7zInfo analysis/save passes with progress callbacks, CheckPassWord/CheckPassWordByData pairing the shipped Dict dictionary, C7zNextHeaderInfo header-rebuild), the X3F section checkers, the PDF/office repair deep-dive, and the HWP path-checker module. The product frame carries it as a first-class feature: remote config gates it (`enable_imgrepair`/`enable_docrepair` in the fetched Firebase cache — I-027 decoded the flags), DRWUI integrates repair into the recovered-file flow (the CHK/`ln` chain classifies candidate files via `CHKRepairFileSign` — which reads the EUUserData signature DB per CORRECTIONS #9), and the viewer payloads embed repair threads (`CPDFRepairGetPixmapThread` in the PDF path).

## What our core does

Nothing on this surface. `src/core/repair/cr_rawfixer.cpp` implements the RawFixer.dll boot-sector/filesystem-structure contract (CheckDBR/FixFromDBR/CheckMFT/FAT mirror restores — transactional, consent-gated, well-audited in modules/rawfixer.md); `src/core/check/cr_fscheck.cpp` is the damage-audit phase. Neither repairs file CONTENT. No ABI operation, CLI verb, session record, or event carries a file-repair concept (grep-verified at the audited tree). The only registration is the SURFACE-INVENTORY §5 row whose citation is false against the current CAPABILITY-MATRIX §3.

## Gap

Falsifiable: recover a truncated JPEG or a header-damaged 7z with the og → the product offers Repair (the Fixo line) and produces a repaired copy. The merged product recovers the same file and offers nothing — the file lands as a non-opening artifact. And the exclusion trail does not hold: CAPABILITY-MATRIX §3 (the standing-exclusions list, `docs/CAPABILITY-MATRIX.md:77-87`) does not mention the repair line, so the surface is neither implemented nor truthfully excluded.

## Impact

The og's Repair features are headline, flag-gated product capabilities adjacent to recovery (users repair what they recover — a natural part of the value chain the og ships). The merged product cannot express them at the engine, ABI, CLI, or session layer. The registration drift is its own hazard: the roadmap claims the exclusion is registered when it is not — a future session could read the matrix and conclude the surface was deliberately and formally excluded when no such decision exists on record.

## Reproduction / verification

Corpus documentation evidence (protocol §6.3): the 8 FileRepair artifacts read at their heads; the binary list verified against `og-codebase/` (the 9 repair-line binaries exist as shipped files); the remote-config flags decoded first-hand at session-8 (I-027 §3) and re-verified in the cache blob this session; the DRWUI chain cited per CORRECTIONS #9 + SCAN-ENGINE-DELTA F2. Core side: the repair/check sources read; grep zero-hits for any file-content-repair concept. Registration drift: SURFACE-INVENTORY §5's row vs CAPABILITY-MATRIX §3 read side-by-side (the quoted line ranges above).

## Fix plan (for the engineering team)

Decision-first, then build or re-register:
1. **Product decision (the roadmap's call):** either (a) implement the repair-line engine surface, or (b) register it formally as a product-line exclusion in CAPABILITY-MATRIX §3 with the Fixo-provenance rationale and the UI-layer handoff note. Either way the false SURFACE-INVENTORY citation must be corrected THIS fix wave (it is a 2-line doc repair: point the row at the new §3 entry, or at this finding while the decision pends).
2. If (a), the corpus gives buildable contracts, in value order: image repair first (the grammar + state-machine artifacts are the most complete: per-format walks with accept thresholds), then the 7z archive engine (CArchive7zInfo/C7zNextHeaderInfo class contracts with the progress callback seam), then office/PDF (the deepening-batch2 decode), then X3F/HWP. The natural seams: `cr/repair` (a new file-repair family beside rawfixer), a `cr_ui` ABI pair (analyze + repair-with-consent mirroring the RawFixer S1 consent pattern), a CLI verb pair (`--repair-file`/`--repair-file-apply`), and repair events on the existing event channel.
3. The Dict dictionary (N-064's shipped asset) pairs with the archive password path (CheckPassWord semantics per the artifact).
4. Tests per surface (unit: grammar accept/reject on spec-derived fixtures per the S-042 fixture lesson; integration: repair-in-the-recovery-flow through the ABI pair).
5. Acceptance: the SURFACE-INVENTORY row cites a truthful registration; if (a), `RepairImage.RoundTrip`, `Repair7z.NextHeaderRebuild`, `RepairOffice.DocumentOpen` green; if (b), the §3 row carries the Fixo-provenance rationale.

## Tests to add

- Only if (a): the per-surface suites named above. If (b): a docs test/CI check that every SURFACE-INVENTORY OOS row's cited registration exists and contains the row (this drift class is systematic — the check would have caught this one).

## Related findings

- S-038 (the app-installer surface — the SEVERE precedent for product-frame capability findings)
- I-027 (the data-asset register — the Dict/remote-config decode rows feed this finding)
- M-100 (browse-inside: the archive EXTRACTION surface adjacent to EUUnZip's repair-extract path)
- SCAN-ENGINE-DELTA F2 (the DRWUI CHK/ln chain — the repair classification integration)

## Open questions

The exact product boundary of "Fixo line inside DRW" vs "separately-sold product" (the artifacts show the Fixo tree as a distinct source line bundled by the og installer) — the decision in fix-plan step 1 is the product owner's; the audit records the capability gap either way.


---

## Session-11B addendum (2026-10-29, re-triage at `743d7b8d`): PARTIAL — the false-citation leg was fixed at filing; the capability leg stands

Leg 2 (the false registration) was repaired by `ea2f3ea7` in the finding's own session: the SURFACE-INVENTORY row now truthfully reads "AUDITED this session (SA-10B): S-050 — unimplemented AND the prior row's OUT-OF-SCOPE (CAPABILITY-MATRIX §3) citation was FALSE". The title's "unregistered" clause is half-stale: the FALSE registration is gone, but the FORMAL disposition (implement vs a truthful §3 exclusion row) still pends — CAPABILITY-MATRIX §3 (:77-92) still lists only Cloud/NAS/remote, BitLocker FVE-key derivation, signing, .rsf import. Leg 1 (the capability gap) verified still-active: src/core/repair/ holds only cr_rawfixer.cpp; zero imgrepair/docrepair/videorepair symbols repo-wide; no CLI verb (rrecov.cpp:3127-3148), no ABI op.
## Session-11-lead addendum (2026-10-29 — the wiring decode extends the scope note)

The Lane A infrastructure walk decoded the og's repair-binding WIRING in full and
found a facet beyond this finding's registered scope: besides the per-candidate
hook (the finalized candidate's GetVideoInfo → interface `vtable+0x48`, polled at
RAWSearchDLL_decompiled.c:10130-10136 — the seam this finding's plan targets),
the og ALSO feeds **every scanned chunk's raw buffer** to the repair interface
(`vtable+0x40` at :9623-9627, with the init call `vtable+0x30` receiving the
device + size at :9226-9236). The og's repair module therefore sees ALL scanned
bytes — the per-chunk feed is a second, coarser channel beside the per-candidate
one. Core's CarvePostProcessor (include/cr/carve/cr_carve.h:96-160, the M-021
seam) is per-candidate only. When this finding's fix lands the repair line, the
post-processor contract should decide the per-chunk channel explicitly (a
`on_chunk` optional callback or its registered decline) — the og's repair engine
consumes both. Evidence: `evidence/s11-lead-deep-lanes.md` §Lane-A row 13 + the
addendum list.

---

## S41 addendum (2026-10-29 — the repair line's UI-side surface decoded: the never-cited UIUX/32 artifact)

The corpus artifact `re-workspace/artifacts/UIUX/32-repair-item-rows-decode.md` (zero
trail citations before this session — see I-034's register) decodes the repair product
line's UI spine, sharpening this finding's scope-registration evidence (complementary to
the per-chunk-feed addendum above — that one is the engine-side channel, this one is the
presentation/queue side):

- **Three row-controller families** (template-sibling pattern): the generic FileRepairListItem/
  FileRepairScanItem (10 moc methods), CRepairVideoItem and CRepairPDFItem (**byte-identical
  22-string moc blobs** — one row controller specialized per line).
- **The row contract** (stringdata-validated): 8 signals + 10 slots — the **Advanced Repair
  escalation** is sample-file-guided reconstruction (video: CRepairVideoGetSampleWnd; PDF: the
  decrypt path); `sigSaveActivateSuccess` fires on licensed-advanced-repair activation.
- **The task queue** (FileRepairDataMgr): sigDoNextRepairTask dequeues, sigUpdateRepairProgress
  fans out, sigFinishRepairTasks enables batch save — the serialized repair queue.
- **The thread fleet at TimeCriticalPriority**: FileRepairAddThread/FileRepairThread/
  FileRepairSaveThread/PdfRepairThread/ArchiveRepairThread all `QThread::start(t, 7)`.
- **The scan-completion probe**: `m_bNeedViedoRepair=0, m_pVideoRepair=1` — the app probes
  video-repair availability exactly when results finalize.

Fix-plan impact: step 1's scope registration should name the row-contract surface (the
actions: Repair/Preview/Save/Stop/Advanced/Feedback/Change) and the queue model as the
og's behavioral shape. No verdict change (extend-don't-refile).
---

## Addendum (session-12 continuation lane, 2026-10-29 — audited `c7fa527c`, corpus `be0cfe9`): the fix plan's build order omits the VIDEO engine — the corpus's deepest-decoded repair algorithm and the only repair surface with an engine-side seam

**Fix-plan amendment (step 2's build order + step 3):** the value-ordered build list ("image repair first … then the 7z archive engine … then office/PDF … then X3F/HWP") names no video surface. Per `architecture/06-repair-chain.md` §3 (125 trace points decoded — "the deepest mapped") and §5 ("Video repair uniquely bridges both: it can consume **filesystem dataruns from the scan engines** (engine-recovered extents) to reconstruct video even when mdat was overwritten"), the video line is:

1. the most completely documented engine in the corpus (weighted ISO-BMFF atom grammar with the 13-top-level-4CC float-weight registry; the two-scan architecture — locate atoms → block-bound calc → verify; moov triage with **sample-file moov substitution** (the GetSample UX contract); self-built fragment sector bitmaps; **datarun selection — original filesystem datarun vs regrouped mdat carve** (`useOrgFsDatarun:%d usergedDatarun:%d`); per-format regroup for MP4/MOV (stco/stsc/stsz/stsd), AVI (idx1 rebuild), MTS/M2TS (device-offset raw stream finding), MXF (KLV regroup));
2. the ONLY repair surface whose algorithm integrates with THIS core's existing outputs — the datarun-selection half consumes engine-recovered extents, i.e. `tree::FileNode::extents`, the exact surface the core already produces. The natural seam: `cr/repair` (a video family beside rawfixer) reading node extents, plus the sample-file input contract (the consent/UX pattern mirrors RawFixer's S1 consent + the M-108-style eligibility whitelist).
3. also unnamed in the plan: the repair-eligibility signature seam (the DRWUI CHK/`CHKRepairFileSign` chain — CORRECTIONS.md #9 — which classifies recovered files Matched/NotMatched for repair hints; a core-side equivalent would gate which recovered rows carry the repair hint) and the output-location contract for repaired files (distinct from the recovery destination). The cloud-assisted sub-paths (EUImgRepair's net/client.cpp, EUAdobeRepair's cloud client) ride the standing Cloud OOS registration and need only a note in the per-surface build docs.

**Corpus-reference completion:** the finding's corpus list should carry `architecture/06-repair-chain.md` §3/§4/§5 (the video algorithm decode + the per-format engine table + the repair-flow integration) and `re-workspace/artifacts/per-binary/EUVideoRepair.md` (the 7,154-function census + the box-class walk: TkhdBox/UuidBox et al.) — both first-hand re-read this session.

**Impact of the omission:** if the engineering team executes the plan as written, the shipped repair line would lack its best-documented engine and the only one that exercises the core's own scan results — a scope decision the plan never surfaces. The amendment's acceptance: either (a) the build order gains the video family (with the datarun-selection seam named as the integration point and the GetSample consent contract as the input model), or (b) a PARITY-DELTAS row registers the video line's exclusion with the rationale. The eligibility-seam and output-location contracts land as named steps either way.

---

## Addendum (session-48 lane, 2026-10-29 — the I-035 fold-in; all four cites re-read first-hand this session): the CHK repair-chain constants — precise inputs for the eligibility-seam step

The I-035 register's decode, folded here per its fix plan so the S-050 owner consumes it
without re-decoding (every constant verified on-disk this session):

1. **The rename formats** — identified fragments are RENAMED to their detected extension:
   `"%s.%s"` (name + identified ext) and, on the +0x98 conflict path, `"%s_%s.%s"`
   (name + discriminator + ext), both through the 0x1000-byte (4 KiB) stack buffer at
   `DRWUI:726699-706`.
2. **The hex-prefix adapter registry** (the repair dispatch, `DRWUI:1008918-1009407`) —
   keyed by the file's leading hex bytes as ASCII: **"47"=TS** (MPEG-TS), **"4D5A"=PE**,
   **"504B0304"=Office-zip** (`CHKOffice504B0304`), **"D0CF11E0A1B11AE1"=Office-OLE**
   (`CHKOfficeD0CF11E0A1B11AE1`), plus the TIFF keys — five entries total.
3. **The match read buffer** — 36 KiB (`0x9000`) with a 2-MiB (`0x200000`) cap
   (`DRWUI:772543-553`, the `FUN_1404f8e60` read).
4. **The "tif" exclusion** — `_stricmp(_Str1,"tif")` at `DRWUI:726583`: TIFF fragments
   are explicitly excluded from the rename flow.

These constants feed the fix plan's step-3 eligibility-seam contract (the
`CHKRepairFileSign`-equivalent gating which recovered rows carry the repair hint) and
the rename semantics any fragment-rename surface must reproduce. No verdict change
(extend-don't-refile; the register retires as I-035 ACKNOWLEDGED).
