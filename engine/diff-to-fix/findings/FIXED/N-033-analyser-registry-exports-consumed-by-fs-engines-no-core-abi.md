# MINOR N-033 — EUGetAnalyserList/Map/Multimap are consumed by the FS engines (registry clone); the core exposes no carve-capability ABI

**Severity:** MINOR
**Module:** carve (+ abi)
**Parameters affected:** P42 (public surface shape — capability enumeration), P46 (documentation of the contract)
**Corpus references:** exports `EUGetAnalyserList/Map/Multimap` (RAWSearchDLL_decompiled.c:72170-72208; `rabin2_exports.txt` entries 3-5; singleton master containers at +0x30/+0x40/+0x50, built by `FUN_180006730` :4216-4248); **consumer**: `FATSearchDLL_decompiled.c:13872-13977` — `LoadLibraryA("RAWSearchDLL.dll")`, `GetProcAddress("EUGetAnalyserList"/"EUGetAnalyserMap"/"EUGetAnalyserMultimap")` (:13881, :13897, :13942), then **std::map/multimap/list clone** of each returned container into the FAT engine's own members (+0xb8 list, +0xd0 map, +0xe0 multimap; tree-copy with leftmost/rightmost fixups :13903-13930, :13946-13975); second copy of the same sequence at :29290-29361 (per-scan re-fetch); deep-dive artifact §1 ("the host enumerates carvers through these to build 'which formats can we carve' UI")
**Core references:** `src/core/carve/cr_registry.cpp:300-305` (`registry()`/`registry_size()` — internal C++ accessors, no ABI surface), `docs/ABI-CONTRACT.md` (no carve-registry or capability query — grep: only stage/progress fields), the og-consumer role in the core would be the orchestrator/UI, both of which link the core statically — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og exports its analyser registry in three projections, and the FS SearchDLLs
themselves import RAWSearchDLL.dll at runtime to clone the master registry into
their own state — the engines keep a snapshot of "what can the raw layer carve" for
capability enumeration (and per-scan refresh). The core's registry is a static
table with internal accessors only; no host-facing API can enumerate carve
formats. This is the decoded answer to the "who consumes EUGetAnalyserMultimap"
question (task item 6 / parameters A42-A45 in the prompt → taxonomy P42).

## What the reference does

(Quoted in the corpus header.) The three exports return the master (unfiltered)
containers; the working copies used by the scan are separate members rebuilt per
search with the host filter (M-024). The FAT engine clones all three — including
the multimap, which exists precisely because several analysers share keys (e.g.
u16 0x2125 '%!' → [PdfAnalyzer, CEPSAnalyser]; u16 0xfeff → [MP3Analyzer,
HTMLAnalyser]) — a consumer that needs the FULL chain per key must get the
multimap, not the map.

## What our core does

`registry()`/`registry_size()` are namespace functions used by the F8 gate and the
scan loop; `include/cr/carve/` exposes no enumeration API, `docs/ABI-CONTRACT.md`
defines no carve-capability query. Since the core is one process/static lib the
cross-DLL clone pattern is unnecessary — but the *capability question* ("which
formats, which extensions, what confidence") has no answer surface at all.

## Gap

Falsifiable claim: the og host can enumerate every carve format with its key(s),
analyser chain and enabled state (three exports + GetRawUserInfoMap); the core
host cannot enumerate anything beyond hard-coding knowledge of the table.

## Impact

- UI/CLI cannot render "formats we can recover" lists or per-format filter UIs
  (ties to M-024's filter surface).
- Parity tooling (benches, docs generation) cannot self-describe the carve set;
  S-007 + s3-rawsearch-inventory coverage comparisons had to be done by hand against the source.

## Reproduction / verification

Decompiled (strength 2): export bodies :72170-72208; FAT consumer :13872-13977
(the LoadLibrary/GetProcAddress/clone sequence) + :29290-29361; evidence file §1,
§7. Core: code-cited.

## Fix plan (for the engineering team)

1. Add a small stable enumeration to the ABI: `size_t cr_carve_format_count(void)`,
   `const cr_carve_format_info* cr_carve_format(size_t i)` where the info struct
   carries {id, ext, display name, base_confidence, min/max size, enabled} —
   table order, append-only (P45 versioning: grow the struct by trailing fields).
2. Wire it into the CLI (`rrecov --list-carve-formats`) and the UI readiness plan;
   document in ABI-CONTRACT.
3. Tests: `ABI_Carve_Format_Enumeration_Matches_Registry` (count == FMT_COUNT,
   ids in order), `ABI_Carve_Format_Info_Stable` (golden struct layout check).

## Tests to add

(fix plan step 3)

## Related findings

- M-024 (the filter half of the same surface).
- N-031 (dispatch decode — why the multimap projection exists: shared keys; my session-3 chain-semantics decode is an addendum there).

## Open questions

Whether the FAT engine's cloned registry is consulted during ITS OWN scanning
(beyond capability enumeration) is undecoded — the clone call sites feed members
that this session did not trace into the FAT scan path. Next artifact:
`FATSearchDLL_decompiled.c` readers of members +0xb8/+0xd0/+0xe0.

---

## Addendum (session 6 — partial fix verified in-tree, 2026-10-26)

**Verified at core commit `022c81d`-lineage (landing commit `1f08c20`, plus the
`74b0854` test-boundary correction).** The core now exposes the carve-capability
registry at the ABI level — the main capability gap this finding registered:

- `src/ui/cr_ui.cpp:2074-2117` — `cr_ui_carve_format_count()` +
  `cr_ui_carve_format_info()` (added-export rule, ABI stays 1): read-only
  enumeration of BOTH carve layers in the og-registry-equivalent order —
  grammar-validator registry (FormatId order) then raw-sign prefilter table
  (kFormats order), stable append-only, per-row ext[24]/min/max/base_confidence/magic.
- The og-equivalent surface cited in this finding (EUGetAnalyserList/Map/Multimap +
  GetRawUserInfoMap) now has a first-class core counterpart; a host can build the
  same format-config UI from it.
- Suite evidence: 363/363 ASan at landing; the ABI-level enumeration test
  (CarveFormatRegistryEnumeration) moved to cr_ui_tests after the LNK2019
  DLL-boundary correction (`74b0854`).

**Still open from this finding's fix plan:** step 2 (CLI surface
`rrecov --list-carve-formats`) — re-verified ABSENT this session (grep over
`src/app/rrecov.cpp`: no carve-format listing), and the per-scan registry-refresh
semantics (the og FS engines re-clone the RAWSearchDLL registry per scan) remain
a doc-level divergence. Severity stays MINOR; ABI sub-item CLOSED, CLI sub-item
open.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-033 CLI-close commit — `git log` "fix(N-033)"; the ABI half landed earlier at `1f08c20` per the session-6 addendum)
**Verified by:** the ABI sub-item's existing proof (`CrUi.CarveFormatRegistryEnumeration` — count/order/determinism/error contract, incl. every print-safety invariant the CLI loop relies on: in-field NUL termination, lowercase/no-dot exts, confidence ≤ 1000, magic ≤ 16) + the win-layer parse gate `win_syntax_check` green on the new `print_carve_formats` + dispatch + usage text (extended with the `-D__declspec(x)=` parse-neutralization — ELF g++ rejects `__declspec`, which `cr/cr_ui.h`'s _WIN32 branch uses; the gate checks structure, the MSVC lane exercises the real macro). The CLI sub-command is Windows-runtime surface (wmain): the Windows CI smoke executes it; the Linux lane's proof is the parse gate + the tested ABI it reads. Fail-before: the CLI surface's absence re-verified first-hand this session (grep over rrecov.cpp — 0 hits, the standing addendum evidence); the gate extension's fail-before executed (cr_ui.h include → `__declspec` parse errors → the neutralization lands → green).
**Resolution notes:** The finding's two open items closed. (1) **CLI half:** `rrecov list-carve-formats` — a subcommand (matching the CLI's subcommand grammar: list/scan/tree/…, rather than the fix plan's `--list-carve-formats` option form; the finding's own impact section asks for a "formats we can recover" list, which a subcommand serves) printing one row per enumerable format (layer tag, ext, min/max bytes, base confidence, grammar magic hex), over BOTH carve layers in the registry's stable append-only order. Exit-code discipline: exit 0 only on a complete walk; an empty registry or a failed row is exit 1 with a diagnostic (the FB-K01 family — scripts must never mistake a broken table for "nothing to carve"). Usage text updated. (2) **Per-scan registry-refresh divergence registered as PARITY-DELTAS D-80:** the og's FS engines LoadLibrary/GetProcAddress + std::map-tree-clone the RAWSearchDLL master registry per scan (:13872-13977 + the per-scan re-fetch :29290-29361) because the og is multi-DLL; the core reads its compile-time immutable registry directly — same content by construction, one less torn-clone/version-skew failure surface. ABI-CONTRACT §7a documents both the CLI projection and the D-80 pointer. The finding's open question (whether the FAT engine's cloned registry feeds its own scan path beyond capability enumeration) remains decoded-open — next artifact unchanged (readers of FAT members +0xb8/+0xd0/+0xe0); it does not gate this close (the capability surface — the finding's registered gap — is now enumerable at both the ABI and the CLI).
