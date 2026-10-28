# OQ-S10-2 — VideoViewer/sfc: a 211,614-byte opaque data file (" cfs-" magic, recurring "FAN." motif) with no disposition, no consumer string, and no corpus characterization — tracked-open

**Tracked-open question (UNVERIFIED — could not be resolved in this session from the trees; filed from the session-10 completeness sweep)**

**Module / Surface:** og-tree disposition register / VideoViewer payload
**Parameters affected:** P54-adjacent (module boundary), register completeness
**Corpus references:** `og-codebase/VideoViewer/sfc` (211,614 B; `file` → "data"; no PE header; first bytes `20 63 66 73 2d` = " cfs-"; recurring 4-byte motif `46 41 4E 2E` = "FAN."; zero meaningful ASCII or UTF-16 strings); `re-workspace/inventory.csv:217` (`VideoViewer/sfc,211614,external,data,,,,0,0,` — the corpus itself leaves it uncharacterized: class "external", format "data", no imports/exports); no binary in the og tree references it by name (unins000's `sfc.dll` imports are Windows' System-File-Checker, unrelated)
**Core references:** audited commit `84a1832` — no disposition row anywhere (the sweep's second and last undispositioned file, beside N-130's VideoFileRepairer.exe); `rg -i "\bsfc\b"` over core docs → only an unrelated "SFC-safe" note in M-078
**Status:** READ-ONLY — tracked-open
**Renumber note (push-time reconciliation):** filed as OQ-S10-1 in this lane's working tree; renumbered **OQ-S10-2** at push — the parallel S-045 resolution lane had concurrently spun its codec-bound question out as OQ-S10-1 (thumbnail-codec-bound-formats); the earlier remote numbering wins.

## The precise question

**What consumes `VideoViewer/sfc`, and does any engine-side behavior attach to it?**

Candidate hypotheses (to be eliminated or confirmed by the next corpus-decode pass):
1. A data asset of the video-repair line (VideoFileRepairer.exe's sample/codec database — the " cfs-" magic and the .FAN chunking suggest a container format, plausibly the "fix analysis" database the -r repair mode consults; the repair family is otherwise OOS, which would make this a registration row, not a capability).
2. A Qt resource container or codec payload for EaseUS_VideoViewer/EaseUS_MediaGraph (the VideoViewer UI family — OOS by the UI exclusion).
3. Dead payload (shipped but unused — the PE_Tools/dlconfig class of installer-chain leftovers).

## Why it could not be resolved here

- No strings: the file is opaque binary with no importable semantics (first-hand `strings`/`strings -el`: nothing beyond the magic and motif).
- No name references: no binary in the tree embeds the path or basename (grep across the reversed set's string tables).
- No corpus artifact: `per-binary/VideoFileRepairer.md` covers the exe, not the data file; `inventory.csv` records it as uncharacterized "external data".
- A full Ghidra pass on VideoFileRepairer.exe + EaseUS_MediaGraph.dll looking for the " cfs-" magic constant (`2d 73 66 63 20` little-endian compare) or file-open of "sfc" is the decisive probe — beyond this session's scope (the file-resource open path would confirm hypothesis 1 or 2).

## The corpus artifact to examine next

`re-workspace/reversed/ghidra-eufilerepair/` or a fresh Ghidra project on `VideoViewer/VideoFileRepairer.exe` + `VideoViewer/EaseUS_MediaGraph.dll`: search for the byte sequence `20 63 66 73 2d` (" cfs-") and for CreateFileW/fopen call sites reaching into the VideoViewer directory. `per-binary/VideoFileRepairer.md`'s 7,019-function census is the entry map.

## Impact if unresolved

None on the core (no core behavior attaches). Register completeness only — the row should read UNKNOWN/DEAD pending the probe, so future sweeps stop re-discovering it.

## Related

**N-130** (the same sweep's VideoFileRepairer.exe — the likely consumer, hypothesis 1) · **N-129** (the sweep's numeric defects) · the repair-family exclusion (I-010/D-26) that most plausibly owns the outcome.
