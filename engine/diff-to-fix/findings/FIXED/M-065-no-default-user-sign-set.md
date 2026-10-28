# MAJOR M-065 — no shipped default user-signature set: the og consults 111 install-tree EUUserData defaults (79 extensions absent from every core stock source) and the core ships none

**Severity:** MAJOR
**Module / Surface:** carve / user-extensible signature DB (modules/carve.md; the M-048 surface's data half)
**Parameters affected:** B1-adjacent (detection coverage), C19-adjacent (the default sign vocabulary), the packaging default-data question (K-adjacent: what ships in the box)
**Corpus references:** `easeus-drw-research/og-codebase/EUUserData` (the shipped default set: 111 entries — measured this session; 16,891 bytes; also present pristine in `og-codebase/BUILDPE/EaseUS-x64/EUUserData`); `re-workspace/artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` (the grammar + the DB's role); `re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md:61` ("secondary typing" — the consumption-mode ambiguity, quoted below)
**Core references:** audited commit `dd82db9` — `src/core/carve/cr_usersigns.cpp` (the parse/install surface exists, M-048); **no default set ships**: `rg -i "euuserdata" src/ include/ docs/ tools/` → zero; the release zip's asset list (`docs/PREBUILT-ARTIFACTS.md` §3) carries no signature-DB asset; of the og's 111 default extensions, **79 are absent from every core stock source** (the generated raw-signs table + the grammar registry — the intersection analysis in this session's wave sheet: tar/gz/xz/bz2/swf/ogg/mxf/3gp/mpeg/xls/ppt/eml among them)
**Status:** FIXED — see Resolution below
**Prior audit check:** expanded on M-048 (FIXED — which landed the parse/install surface, not a default set); the default-set absence and the 79-extension delta are newly quantified by this session's grammar-artifact read + first measurement of the og's DB

## Summary

The og ships and consults a 111-entry default signature DB (EUUserData) in every install — sharper MXF/DOC signatures plus a legacy long tail — and 79 of its extensions exist in none of the core's stock sources (raw-signs table, grammar registry). The core's user-signature surface exists but ships empty: there is no default set, no asset in the release zip, and no doc claim that one should exist. Whether this is a recall gap or merely a typing-refinement gap depends on the og's consumption mode (the corpus is ambiguous — see Open questions), which is exactly what the tracked probe resolves.

## What the reference does

Every og install carries `EUUserData` (111 entries, measured: signs up to 44 bytes, the `zip` offset-526 probe, sharper MXF/DOC signatures than raw_signs.json carries, plus legacy extensions); the engine's user-info map is consulted at scan setup (the deep-dive's :61 "secondary typing" phrasing describes the JSON signs' role for formats without C++ analysers — but the same artifact elsewhere describes EUUserData as consulted alongside raw_signs; the exact capability weight is the tracked ambiguity). The PE payload ships a pristine copy — the DB is product frame, not user state.

## What our implementation does

The parse/install/install-count surface exists (M-048, `cr_usersigns.cpp`), reachable via `--user-signs <file>` and the cr_ui ABI — but the product never ships a DB: no asset in the release zip, no generated default set in the tree, and 79 of the og's 111 default extensions have no stock source at all (so even a user who hand-built a DB from the core's own tables could not reproduce the og's coverage).

## Gap

Falsifiable claim: no core release asset, generated source, or doc ships a default user-signature set; 79 of the og's 111 default extensions appear in no core stock signature source (verified by extension-intersection this session).

## Impact

- **Bounded recall risk**: if EUUserData signs create or materially sharpen detection for any of the 79 extensions (the probe's question), the core's stock coverage is thinner than the og's for exactly those formats; if they are secondary typing only, the impact is labeling precision. Either way the core cannot express them today.
- **Product parity**: the og's out-of-box experience includes this DB; the core's does not — a shipped-data gap, not a code gap.

## Reproduction / verification

Corpus evidence (strength-1/3 mix): the measured 111-entry DB (this session's census) + the grammar artifact; the extension-intersection analysis (this session's wave sheet, reproducible: intersect the DB's keys against `cr_rawsigns_data.cpp`'s table and the grammar registry's format names). Core absence (strength-3): the greps quoted in Core references.

## Fix plan (for the engineering team)

1. Resolve the consumption-mode probe first (OQ-SA-3's RAWSearchDLL user-info-map consumer decode): if the signs are capability-creating or materially sharpening, the default set is a recall item (severity would rise); if secondary typing, it is a labeling item.
2. Ship a default set either way: generate `userdata-defaults.json` from the og's measured grammar (the caps fix in S-039 must land first or the shipped file will not parse), add it to the release zip's asset list + `MANIFEST.json`/`SHA256SUMS` conventions (PREBUILT-ARTIFACTS §3), and load it at engine init when present (falling back to empty with a log line when absent).
3. For the 79 absent extensions: register them in the generated set verbatim from the og's DB (the corpus file is the ground truth; byte-measured this session).
4. Tests: an engine-init load of the shipped defaults (count = 111); the 79-extension presence check (a fixture enumerating the absent-from-stock names must now find them); the absent-file fallback logs and proceeds.
5. Acceptance criteria: the release zip carries the DB; a stock scan on a fixture containing one 79-list format (e.g. `.ogg` raw) detects/labels it per the probe's resolved semantics.

## Tests to add

- `UserSignsDefaultSetShipsAndLoads`.
- `UserSignsOgExtensionsPresent` (the 79-name enumeration).

## Related findings

- **S-039** (this session) — the caps fix that must land first (the shipped DB cannot parse today).
- **M-048** (FIXED) — the surface this data half completes.

## Open questions

The og's consumption mode for EUUserData signs — "secondary typing" (deep-dive:61) vs. consulted-alongside-raw_signs (the same artifact's other phrasing): resolved by decoding RAWSearchDLL's user-info-map consumers (the `GetRawUserInfoMap` chain at `FUN_180001350()+0x90`, the same function M-024's trail already decoded — the probe is one decompile read away and is registered as this finding's blocking question for severity calibration, not for the gap's existence).

## Resolution (FIXED — the shipped default set + the engine-init load; OQ-SA-3's probe resolved en route)

**The consumption-mode probe (OQ-SA-3, resolved first-hand this session):**
RAWSearchDLL's `GetRawUserInfoMap` is an EXPORT consumed by DRWUI
(DRWUI_decompiled.c:227538 — `GetProcAddress(hRaw, "GetRawUserInfoMap")`),
and the consumer at :227545-227585 walks the map assembling each entry
(name + the numeric setting via `"%lld"` of entry+0x20) into a config tree
whose consumers are keyed `"Trigger_Raw"` / `"FileSystem"` — the entries
feed the RAW searcher's setup/trigger configuration at scan-setup time
(the `filter_generalraw` family). Verdict: **consulted at scan setup —
capability-configuring for the RAW filter surface**, not result-time-only
labeling; shipping the default set is a recall-adjacent parity item, and
M-065's MAJOR severity stands.

**The fix:**
1. `data/userdata-defaults.json` — the og's own 111-entry / 190-sign DB,
   canonicalized (sorted keys, signs/mime verbatim, 16,156 bytes pretty
   JSON), generated from the measured corpus file; it parses clean under
   the post-S-039 caps (the whole-file validation is the
   `UserSigns.DefaultSetShipsAndLoads` test: count 190, distinct exts
   111).
2. **Engine-init load** (`src/app/rrecov.cpp`): with no explicit
   `--user-signs`, rrecov loads `userdata-defaults.json` from the
   executable's directory when present (the og's install-tree posture);
   precedence: an explicit `--user-signs` file REPLACES the default
   (install_set's replace semantics); a corrupt shipped default logs and
   proceeds empty (the og's bad-config-as-absent degrade); an absent
   default logs one line and proceeds — never an error.
3. **The release asset**: `release.yml` stages the file BOTH beside
   `rrecov.exe` (the load path) and under `data/` (the documented home);
   PREBUILT-ARTIFACTS §3's zip row documents it.
4. **Tests**: `UserSigns.DefaultSetShipsAndLoads` (the ship/load
   contract: 111 exts / 190 signs) + `UserSigns.OgAbsentFromStock-
   ExtensionsCarried` (the 47-name spot enumeration across the
   families — the finding's 79-name census minus 'cpt', which S-037
   added, minus case variants; every listed absent-from-stock extension
   must ship). Full suite: 920 tests — 916 passed / 4 platform-skips /
   0 failed; the Win7 parse gate green (a GetModuleFileNameW stub joined
   the win-stub SDK).

**Landed:** commit "fix(carve): M-065 - the shipped default user-sign set
(the og's 111-entry EUUserData) + the engine-init load + the release asset".

## Resolution addendum (2026-10-28, Agent 2 scan-engine lane — the parity-jus­tification corrected; the landed fix stands as a registered superset)

The Resolution above and the probe note inside it justify the ship with
"the og consults the DB at scan setup … capability-configuring" (the FIX-6
reading of the DRWUI :227538 consumer). That reading is superseded by the
OQ-SA-3 re-resolution (see the tracked-open file + CORPUS-ERRATA E-A/E-B +
PARITY-DELTAS D-104): `GetRawUserInfoMap` returns the per-scan carve
statistics map (insert-or-increment from the analysers' detection paths,
cleared at every searcher init) — scan OUTPUT, not scan INPUT; and
EUUserData's sole consumer is DRWUI's `CHKRepairFileSign` (the CHK repair
chain), never the RAW engine (zero engine references).

**What this changes:**
- The og-parity claim "the og's scan consults EUUserData" is withdrawn. The
  og's scan engine has no user-sign channel; the 79-extension "recall risk"
  premise is void for the scan engine.
- The landed behavior (shipped default set + engine-init load) is retained
  as a **deliberate product superset** (D-104): the core's user-sign surface
  exceeds the og's engine by design, and feeding it the og's own shipped
  data is a product enhancement with a clean opt-out (explicit
  `--user-signs` replaces the default entirely). No code change.
- The finding's original severity question resolves to: not a scan gap at
  all; the true og consumer (CHK repair file-signature checking) belongs to
  the file-repair lane, which is out of the core's product scope —
  handoff: file-repair lane owns any future CHKRepairFileSign-equivalent
  surface and its data asset (the same JSON file can serve both channels).
- The tests and release mechanics landed with this fix remain valid as the
  superset's contract tests.

**Evidence chain:** DRWUI_decompiled.c:1005387 (:1005497+ JSON parse,
:1005818 "signs", :1002677 CHKRepairFileSign vtable, :721520-721680 the
MatchedFile/NotMatchedFile checker, :227538-:227625 the stats report
reader); RAWSearchDLL_decompiled.c: 0 EUUserData hits over 80,614 lines,
:11376-11525 FUN_18000ed00 (map insert-or-increment), :9044-9051 searcher
init (map clear + naming-callback registration), :23836-23850 raw_signs.json
load, :13133 RawConfig.ini load, :916931-917010 the per-searcher start
sequence (no sign handoff); RTTI `.?AVCHKRepair@@/.?AVICHKRepairObject@@/
.?AVCHKRepairFileSign@@/.?AVCHKRepairAdapterMgr@@`.
