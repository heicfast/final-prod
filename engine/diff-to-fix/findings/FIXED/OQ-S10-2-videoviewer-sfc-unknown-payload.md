# OQ-S10-2 — VideoViewer/sfc: a 211,614-byte opaque data file (" cfs-" magic, recurring "FAN." motif) with no disposition, no consumer string, and no corpus characterization — tracked-open

**Tracked-open question (UNVERIFIED — could not be resolved in this session from the trees; filed from the session-10 completeness sweep)**

**Module / Surface:** og-tree disposition register / VideoViewer payload
**Parameters affected:** P54-adjacent (module boundary), register completeness
**Corpus references:** `og-codebase/VideoViewer/sfc` (211,614 B; `file` → "data"; no PE header; first bytes `20 63 66 73 2d` = " cfs-"; recurring 4-byte motif `46 41 4E 2E` = "FAN."; zero meaningful ASCII or UTF-16 strings); `re-workspace/inventory.csv:217` (`VideoViewer/sfc,211614,external,data,,,,0,0,` — the corpus itself leaves it uncharacterized: class "external", format "data", no imports/exports); no binary in the og tree references it by name (unins000's `sfc.dll` imports are Windows' System-File-Checker, unrelated)
**Core references:** audited commit `84a1832` — no disposition row anywhere (the sweep's second and last undispositioned file, beside N-130's VideoFileRepairer.exe); `rg -i "\bsfc\b"` over core docs → only an unrelated "SFC-safe" note in M-078
**Status:** CLOSED — RESOLVED (session-10, S10-F lane, 2026-10-29): the decisive probe ran — **NO consumer exists anywhere in the reversed corpus**; the file is dead payload (hypothesis 3). Registered via the N-134 register batch.
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


---

## RESOLUTION (session-10 S10-F, 2026-10-29)

**The decisive probe, executed this session** (the OQ's own "corpus artifact to
examine next" — the magic/constant search over the VideoViewer-family decompiles):

- `grep -l "sfc\|cfs"` over ghidra-wave2/VideoFileRepairer_decompiled.c,
  EaseUS_MediaGraph_decompiled.c, ghidra-repair/EUVideoRepair_decompiled.c →
  **ZERO hits** (per-token counts: cfs=0 sfc=0 FAN=0 in each).
- The WIDE sweep — `grep -l "sfc\|cfs"` over every `*/  *_decompiled.c` in
  reversed/ — returns only ghidra-misc/unins000_decompiled.c (Windows' own
  sfc.dll import — the OQ already recorded it as unrelated) and
  ghidra-repair/EuOfficeRepairFile_decompiled.c (the hit is the substring inside
  `"plcfsed not found"` at :4854 — an unrelated error string).
- String tables: zero `sfc`/`cfs-` references across the reversed set's string
  tables (sweep over `*strings*.txt` / `*ascii*.txt`).

**Hypotheses 1 and 2 are REFUTED** (no video-repair-line consumer, no
VideoViewer/MediaGraph consumer). **Hypothesis 3 stands: dead payload** —
shipped in og-codebase/VideoViewer/ among the media assets (PNGs, cursors,
the FFmpeg DLL set), never referenced by any reversed binary. First-hand
structure note: " cfs-" + 3 bytes + 30 "FAN." markers at scattered offsets
with high-entropy inter-marker data (compressed/encrypted; no clean u32
length framing) — an opaque container with no reader.

**Disposition:** registered as dead-shipped installer-chain payload (the
PE_Tools/dlconfig class) via the **N-134** register batch (the
product-frame register completion finding); no core action, no capability
question remains.

---

## REFUTATION ADDENDUM (session-11 S11 lane, 2026-10-29 — the resolution above is superseded)

The "dead payload" verdict is **refuted by decode**: the consumer exists and is
VideoFileRepairer.exe itself. The zero-hit grep could not find it because the
filename appears in the decompile only as the `DAT_0059d24c` symbol (the "sfc"
string lives in .data) and the magic only as the hex constant `0x73666320`
(`VideoFileRepairer_decompiled.c:248309`) — neither contains the ASCII
substring the probe searched for. The decoded truth: `" cfs"` magic + a
16-byte XOR key (`:249963-249966`, "FAN." = key bytes 4-7 — the "motif" was
the key showing through zero plaintext) + a clean u32 length framing
(827,489 uncompressed bytes — the "no clean framing" note is also refuted) +
a version byte dispatching a custom-LZ v2 parser; loaded by the repair-engine
ctor at startup and REBUILT on failure (`:245536`, `:246921-246931`). Live
product machinery of the OOS repair line — hypothesis 1, not hypothesis 3.
Full decode + methodology autopsy: `evidence/decompiled-quotes/s11-sfc-payload-decode.md`;
the false-resolution record itself is filed as **N-138**. The question stays
closed — with the corrected answer.
