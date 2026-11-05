# s10-config-layer-absence-greps.md — the config-layer triplet claimed "landed" but absent (N-128 + M-108 evidence)

**Session 10 (2026-10-29). Core `84a1832`. Corpus `be0cfe9`.**

## The claim under test

`diff-to-fix/API-DELTA.md` §1.7 (data assets behind API surfaces):

- **Row 44:** "FilterFolder.ini noise blocklist + FatFilterName 10 locales + TxtViewFiles
  119 ext | **landed (cr_filterfolder_data.cpp)** | corpus-api-context §4"
- **Row 45:** "PriorityDisplayType.xml / usefullostfiles.xml / SumaryFileType groups |
  **landed (query/filefilter data layers)** | corpus-api-context §4"

## Greps run (all at core `84a1832`, whole-repo, read-only)

```
rg -i "A8S|ARENA|FOUNTAIN|MCMETA|PNPROJ|MDBACKUP" src/ include/ tests/   -> 0 hits
rg -i "txtview|text.whitelist|text_ext" src/ include/ tests/ scripts/   -> 0 hits
rg -i "alias|priority_display|display_order|PriorityDisplay" \
       src/core/query/ src/core/tree/ include/cr/query/                 -> 0 hits
rg -i "useful|valuable|lostfiles" src/ include/ tests/                  -> 2 unrelated comments
rg "m4p" src/ include/                                                  -> 1 hit: cr_filefilter_data.cpp:732
                                                                        {"m4p", Category::Audio}  (plain category row)
rg -i "family|unif" src/core/query/cr_filefilter_data.cpp \
                     src/core/tree/cr_grading.cpp                       -> FAT-family address math only
```

## Findings

1. **TxtViewFiles (119-ext text-preview whitelist): ABSENT.**
   `src/core/orchestrator/cr_filterfolder_data.cpp` (read in full, 65 lines) carries exactly
   three pools — `kFolderPool` (4 entries), `kFilePool` (7), `kFatNamePool` (14) — the
   junk-filter lists, which ARE byte-exact against `config/FilterFolder.ini`
   (FolderName/FileName/FatFilterName; independently re-verified this session, see below).
   **No TxtViewFiles pool exists anywhere in the tree.** Row 44's claim is FALSE for the
   TxtViewFiles half. The behavioral consequence (the core's content-sniff replacement and
   its BOM false-negatives) is M-108, with the executed proof at
   `evidence/executions/s10-preview-text-eligibility.md`.

2. **SumaryFileType (extension unification groups): ABSENT.**
   The core's summary counts are the plain 8-bucket per-category counter
   (`include/cr/tree/cr_grading.h:115` `u64 category_counts[8]`;
   `src/core/tree/cr_grading.cpp:247`). There is no family-unification layer
   (`m4p+m4a+m4v+mp4` counted as one "MP4 family"; `zip_docx_pptx_xlsx` as the
   office-in-zip family — `filtering-priority--config-layer.md:41,49`). Row 45's claim is
   FALSE for the SumaryFileType half.

3. **PriorityDisplayType.xml (5-cat/35-ext within-category presentation order): ABSENT and
   UNDISPOSITIONED.** No alias/sort-key surface exists in query/tree. Unlike
   `usefullostfiles.xml` (which has an explicit OOS-UI row at
   `docs/PARITY-OG-CORE-INVENTORY.md:65` — "sole consumer = DRWUI :320396, display-tier
   whitelist"), PriorityDisplayType has **no disposition row at all** — its only trail
   mentions are the R-31 gap row (`docs/REFERENCE-API-GAP.md:225`, which honestly lists it
   as missing) and the false API-DELTA row 45. The two gate docs contradict each other.

4. **Contradiction with the honest register:** `docs/REFERENCE-API-GAP.md` R-31
   ("**Missing:** RawConfig.ini per-format carve enable/disable toggles (34 in og);
   usefullostfiles.xml whitelist promotion; PriorityDisplayType display ordering") is
   truthful; API-DELTA rows 44-45 are not.

## Independent re-verification of what DID land (the honest half of the rows)

- The junk pools: `cr_filterfolder_data.cpp:17-57` decoded byte-by-byte against
  `og-codebase/config/FilterFolder.ini` — FolderName (Config.Msi, $Windows.~BT, $Windows.~WS,
  $Windows.~TMP), FileName (desktop.ini, ._.DS_Store, .DS_Store, $UsnJrnl,
  IndexerVolumeGuid, WPSettings.dat, MountPointManagerRemoteDatabase), FatFilterName
  (New Folder + 9 localized variants + System Volume Information + the 3 BitLocker
  artifacts) — **all 25 entries byte-exact, order preserved**. TRUE part of row 44.
- FileFilter.xml: `rg -c "{\"" src/core/query/cr_filefilter_data.cpp` → **1481** — matches
  the 1,481-distinct-extension claim (M-023). Not part of the contested rows.

## Disposition

Filed as **N-128** (the false "landed" claims + the two absent config models) and **M-108**
(the preview behavior divergence). Fix direction: either land the surfaces or correct the
rows + register the exclusions (PriorityDisplayType → OOS-UI row like usefullostfiles;
SumaryFileType → PARITY-DELTAS row if the plain 8-bucket counter is the deliberate choice).
