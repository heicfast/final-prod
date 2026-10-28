# INFORMATIONAL I-034 — The never-cited corpus-artifact register: 23 artifacts with zero trail citations dispositioned; the two product-frame completeness censuses recorded (og-file census: no undispositioned product surface)

**Severity:** INFORMATIONAL
**Module:** gate docs (the corpus-navigation registers) / cross-cutting
**Parameters affected:** P68 (documentation accuracy — the artifact corpus's navigability), P69 (the stub/never-cited surface sweep)
**Corpus references:** the 23 never-cited artifacts (full list in `evidence/decompiled-quotes/s41-registry-key-residue.md` §5); the og-codebase file census (750 entries, top+1 deep)
**Core references:** audited `c7fa527c`. The gate registers read in full: `diff-to-fix/SURFACE-INVENTORY.md`, `docs/parity/PARITY-OG-CORE-INVENTORY.md` (via diff-to-fix's citations), `docs/parity/PARITY-ARCH.md:42`, `docs/parity/PARITY-DELTAS-draft.md:580` (D-26); the active findings N-134, S-050; `diff-to-fix/findings/INFORMATIONAL/I-027-og-codebase-data-asset-register.md`
**Status:** READ-ONLY FINDING — no fix applied (an accurate register, filed for the record and the next lanes' navigation)

## Summary

A systematic census of the corpus's 283 architecture/ + re-workspace artifact documents found **23 with zero citations** anywhere in the audit trail (diff-to-fix/, docs/, worklog). The three RAWSearchDLL family artifacts (brance/rawspeed/xjq) drove this session's M-112/N-142 filings (the never-filed DBX analyser + the DXF-999 key); the remaining 20 are dispositioned by existing registers (D-26's OOS families, N-134's product frame, S-050's repair line, the OOS-UI convention) — this register records the map so no future lane re-derives it. A second census — every og-codebase file (750 entries) against every register — found **no undispositioned product surface**: the never-mentioned set is runtime dependencies (api-ms-* CRT shims, Qt5, OpenSSL, ImageMagick CORE_RL/IM_MOD_RL), third-party license texts, and one Qt resource bundle (drw_light.rcc, the light-theme sibling of the registered ResourceDark).

## What the reference does

The 23 never-cited artifacts, dispositioned one by one (each verified this session by reading the artifact):

| Artifact | Disposition |
|---|---|
| SearchDLL/rawsearchdll--family-{brance,rawspeed,xjq}.md | the analyser-family rosters — drove **M-112** (Dbx) and **N-142** (DXF-999); every other roster member landed / actively-filed / M-107 residue |
| PartitionRecovery/eudcpepm-euedkepm--hook-framework-urgent-cache.md | the Todo-Backup sibling product's driver family (hook framework, VolSnap attach, urgent COW cache, registry-configured identity) — NOT the DRW recovery path; the DRW-relevant trio (eprdtdrv/ebrntdrv/epmdkdrv) is the driver-contract territory (I-029, OQ-S10-3). No core-parity gap: the COW/hook machinery is backup-product functionality, out of the recovery scope. |
| PartitionRecovery/burn--imapi2-disc-writer.md | the emergency-media burner (IMAPI2) — covered by N-134's media-builder family (Boot.dll/BootDriver.dll/peformat.dll) + PARITY-ARCH.md:42's DELIBERATE-DELTA row (roadmap v0.3) + D-26. The real-time streaming burn mode is a detail within that registered exclusion. |
| UIUX/32-repair-item-rows-decode.md | the repair product line's UI spine — S-050's territory; the sharpening evidence (row contracts, the task-queue serialization, the TimeCriticalPriority worker fleet, the Advanced Repair escalation, the m_bNeedViedoRepair probe at scan completion) rides S-050's addendum this session. |
| UIUX/16-shell-internals-ai-layer.md | the 2024+ AI features (AI Recommend/Dedup/Recovery Insight/assistant) — D-26's registered AI omission; monetization chrome ditto. |
| UIUX/14-cdetailpage-slots-viewmodes.md, 33-objectname-loop-closure.md, 34-navigatebar-bottomarea-chrome.md | Qt UI internals — the OOS-UI convention. |
| UIUX/22-rtti-class-census.md | the RTTI census behind the S10-F class walk — methodology artifact; its content is superseded by the S10-F filings' own census. |
| UIUX/23-euinapp-ie-emulation-bootstrapper.md + Licensing/euinapp--ie-bootstrapper.md | the in-app-purchase IE-emulation bootstrapper — Licensing/monetization OOS (D-26). |
| UIUX/24-dynamic-ground-truth-ingestion.md + dynamic-ground-truth--splash-captured.md + dynamic-ground-truth--window-census-verification.md | corpus-methodology artifacts (how the dynamic ground-truth was captured) — no parity surface. |
| Languages/language-packs--family-census.md | the translation family — multi-language OOS (D-26). |
| runtime-families/{qt5-core-stack,qt5-quick-qml-stack,network-libraries,d3d-graphics-shims,misc-windows-shims,imagemagick-modules}.md | the dependency-stack families — the og-file census's never-mentioned set maps exactly here; ImageMagick's per-module detail is N-129-item-2's count-defect territory (24 CORE_RL vs the register's 19). |

## What our core does

The registers already cover the surfaces these artifacts document — the gap was navigability (the artifacts were uncited, so a future lane could not know they existed or whether they had been considered). This register closes that.

## Gap

None (informational). Falsifiability of the census: `rg <basename>` over diff-to-fix/ + docs/ + worklog for each of the 283 artifacts — 260 cited, 23 not (script logic recorded in evidence §5).

## Impact

Navigation only. The M-112/N-142 outcome demonstrates the value of the census method: the never-cited family artifacts were the entry point to the trail's only never-filed analyser gap.

## Reproduction / verification

Executed this session: the basename-grep census (script: `scripts/s41_never_cited_census.sh` logic recorded in evidence §5) + the og-file census (`scripts/s41_ogfile_census.py`, 750 entries vs every register file). Every disposition row above verified by reading the named artifact and the named register row.

## Fix plan (for the engineering team)

None required (register-only). Optional hygiene: add the 23 basenames to the next SURFACE-INVENTORY sweep record so the corpus-navigation index is complete.

## Tests to add

None (informational).

## Related findings

**M-112 / N-142** (the filings this census produced) · **I-027** (the data-asset register this extends) · **N-134** (the product-frame register) · **S-050** (the repair-line finding the UIUX/32 artifact sharpens) · **N-129** (the register-count defect class — the ImageMagick 24-vs-19 row)

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED (the navigation register confirmed; the optional SURFACE-INVENTORY hygiene applied)
**Fixed in commit:** this session's I-034 landing (session-42; cite-by-title: "fix(trail): I-034 - the never-cited artifact register acknowledged + the SURFACE-INVENTORY navigation index completed")
**Verified by:** the disposition table's claims spot-checked this session: the three rawsearchdll family rosters re-read (the M-117/N-152 filings they drove are on the board — M-117 active, N-152 active); the eudcpepm artifact re-read first-hand (the I-029/N-155 decode this session — its 15-code census gap now a CORPUS-ERRATA entry); the og-file census's no-undispositioned-surface verdict stands (the runtime-dependency remainder re-confirmed by category).
**Resolution notes:** The register is confirmed accurate — the 23 never-cited artifacts all dispositioned, the two product-frame completeness censuses recorded. **One correction adopted from the parallel S42 lane's I-036:** the og-file census's "no undispositioned product surface" line was wrong by four config-layer surfaces (shellexe.ini, TreeType, themdark.ini, DRWLog.log — all OOS-UI/product-frame, both TreeType readers decoded in I-036's evidence); the corrected statement is carried in this session's SURFACE-INVENTORY addendum. The fix plan's optional hygiene applied: the 23 basenames + their dispositions added to SURFACE-INVENTORY.md as the session-42 navigation-hygiene addendum, so the corpus-navigation index is complete and the next census starts from the index instead of re-deriving the set. The M-117/N-152 filings the census drove remain the board's active carve work. No code change.
