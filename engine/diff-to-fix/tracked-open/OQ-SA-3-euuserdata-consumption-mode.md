# OQ-SA-3 — the og's EUUserData consumption mode: capability-creating signs vs. secondary typing (the M-065 severity calibrator)

**Status:** UNVERIFIED (moved from wave 5 — the corpus carries both phrasings and the difference decides whether the missing default set is a recall gap or a labeling gap)
**Module / Surface:** carve / user-extensible signature DB (modules/carve.md)
**Parameters affected:** B1-adjacent (detection coverage weight of user signs)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md:61` — "secondary typing" phrasing for the JSON-signs layer; the same artifact family elsewhere describes EUUserData as consulted alongside raw_signs.json at scan setup; `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:72213-72221` — `GetRawUserInfoMap` = `FUN_180001350()+0x90` (the user-info map's fetch point, already partially decoded by M-024's trail)
**Core references:** audited commit `dd82db9` — `src/core/carve/cr_usersigns.cpp` (install_set installs into the same engine registry the grammar validators use — capability-creating by construction in the core)
**Prior audit check:** not in prior scope (the grammar artifact was never cited; M-048 landed the surface without the consumption-mode decode)

## The precise question

When the og's RAWSearch engine consults EUUserData's signs, does it (a) create/extend detection capability for the 79 stock-absent extensions (the signs can produce carve hits raw_signs.json and the C++ analysers cannot), or (b) only refine typing/labels for hits other layers produce? The deep-dive's ":61 secondary typing" phrasing suggests (b); the consult-alongside phrasing suggests (a); the two imply different severities for M-065 (recall gap vs. labeling gap) and different fix priorities.

## The probe that resolves it

Decode the consumers of the user-info map in `RAWSearchDLL_decompiled.c`: `GetRawUserInfoMap` (`FUN_180001350()+0x90`) returns the map; find its call sites (the searcher init M-024's trail decoded at :9180-9182 is one) and read whether the map's signs enter the SIGNATURE SCAN loop (the raw_signs scanning pass — capability) or only the result-typing path (post-hit classification — labeling). One decompile read, two call sites, resolved.

## Why it was not resolved this session

The wave's budget went to the grammar decode + the og-DB measurement (which produced S-039/M-065's filed facts); the consumption-mode decode is a bounded follow-up that does not block either finding (S-039's caps are wrong under either mode; M-065's gap exists under either mode — only its severity moves).

## Related findings

- **S-039, M-065** (this session) — the findings this calibrates.


## Probe resolution (2026-10-28, the M-065 fix session)

Decoded first-hand: `GetRawUserInfoMap` is a RAWSearchDLL EXPORT
(:72213) whose consumer lives in DRWUI (DRWUI_decompiled.c:227538 —
GetProcAddress at RAWSearchDLL load, then :227545-227585): the consumer
walks the map and builds a config tree keyed `"Trigger_Raw"` /
`"FileSystem"` — the entries (extension name + the numeric setting at
entry+0x20, formatted "%lld") feed the RAW searcher's scan-setup trigger
configuration (the filter_generalraw family). Verdict: the DB is
**consulted at scan setup and configures the RAW trigger surface** — the
"consulted-alongside" phrasing was the accurate one; the deep-dive:61
"secondary typing" line describes raw_signs.json's role for
analyser-less formats, not EUUserData's role. M-065's severity stands at
MAJOR (the shipped default set landed: see findings/FIXED/M-065).

## Probe RE-resolution (2026-10-28, Agent 2 scan-engine lane — supersedes the FIX-6 reading above)

The M-065 fix session's resolution above misread the DRWUI :227538 consumer's
data direction. First-hand insert-side decode, full chain:

1. **What `GetRawUserInfoMap` returns is a per-scan statistics map, not a
   configuration store.** The returned object is the RAWSearchDLL singleton's
   `+0x90` member: a `std::map<std::wstring, count>` whose insert-side is
   `FUN_18000ed00` (RAWSearchDLL_decompiled.c:11376-11525 — MSVC map
   lower_bound + insert-or-increment: first detection of an extension creates
   the entry with value 1 (`*(lVar14 + 0x20) = 1`), every repeat detection
   increments (`plVar10[8] = plVar10[8] + 1`)). The call sites are the ~70
   analyser detection paths (e.g. :19626, :20222, :20892, :21574, :29402,
   :30443) — each inserts the DETECTED file's extension after a successful
   identify. The searcher-init block (FUN_18000beb0 :9044-9051) CLEARS the
   map (`FUN_180001500` = the map/list clear) before every scan. A map that
   starts empty each scan and grows per detection is an output (statistics),
   not an input (configuration); the FIX-6 reading has the data flow inverted.
2. **The DRWUI :227538 consumer is a one-way post-scan REPORT reader.** It
   GetProcAddress's the export at RAWSearchDLL load, walks the map, formats
   each entry's +0x20 count as `%lld` keyed by the extension, and files the
   result under the `"Trigger_Raw"` / `"FileSystem"` report tree
   (:227545-:227625, ending in the report/telemetry calls FUN_1400ed3f0 /
   FUN_1400ed2a0). It never writes anything back into the engine and never
   reaches the searcher's setup path. "Trigger_Raw" is a report section name,
   not a trigger configuration; the filter_generalraw family is the naming
   callback (searcher vt+0x48 = FUN_140161fb0, registered in the init block
   at :9078+), which this function never touches.
3. **EUUserData's actual consumer is the CHK repair chain.** DRWUI reads
   `%s/EUUserData` (install dir; DRWUI_decompiled.c:1005387, `_wfopen_s` +
   fseek/ftell) → JSON-parses it (:1005497+) → per-extension "signs" arrays
   (`_stricmp "signs"` :1005818; "offset,HEX" split; `atoi` offset) → the
   `CHKRepairFileSign` manager (vtable :1002677; by-offset map +0x20 /
   by-ext map +0x30) → consumed by the MatchedFile/NotMatchedFile
   file-signature checker (:721520-721680; strings `*****MatchedFile:%s,
   FileSign:%s` / `xxxxxxxxxx NotMatchedFile:%s,FileSign:%s`; RTTI
   `.?AVCHKRepair@@`, `.?AVICHKRepairObject@@`, `.?AVCHKRepairFileSign@@`,
   `.?AVCHKRepairAdapterMgr@@`). Repair-chain surface.
4. **RAWSearchDLL contains ZERO references to EUUserData** (rg over the
   80,614-line decompile: no hits) and **DRWUI never writes raw_signs.json**
   (rg over DRWUI decompile: no hits). The engine's only runtime-loaded
   signature data is raw_signs.json (read-only ifstream, :23836-23850) and
   config/RawConfig.ini (:13133). The per-searcher start sequence
   (FUN_140581360 :916931-917010) passes config/auxiliary/path arguments only
   (vt+0xe8/+0x70/+0x30/+0x40) — no signature handoff exists.

**Verdict (supersedes the FIX-6 verdict):** the question's (a)/(b) framing was
wrong — EUUserData's signs are consumed by NEITHER the RAW scan loop NOR the
scan-result typing path. They feed the CHK repair chain's file-signature
checker (a subsystem outside the core's product scope). The og's scan engine
has no user-sign channel at all; core's `--user-signs` carve capability is a
deliberate product superset (registered D-104), and the landed default-set
load (37c48c6) extends that superset with the og's own shipped data —
legitimate, but NOT an og-parity behavior, and M-065's original MAJOR
"scan-recall" severity premise is void. M-065's true residue: the CHK-repair
data asset belongs to the file-repair lane (out of core scope), recorded as a
handoff. Corpus errata filed (CORPUS-ERRATA + the corpus's CORRECTIONS.md #9).
