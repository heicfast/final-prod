# MINOR N-040 — ABI/doc truthfulness defects: nonexistent SQLite session-index documented as shipped, disk-map state mismatch, event-thread contract contradiction, unrepresentable ABI sub-versions, frozen-contract `modified` unit lie

**Severity:** MINOR
**Module:** abi (cr_ui) / docs
**Parameters affected:** P46 (documentation of the contract — complete, truthful, verified), P68 (documentation accuracy), P13-adjacent (report/session sidecar claim), P42-adjacent (surface truthfulness)
**Corpus references:** N/A (internal truthfulness — core docs vs core code; the og has no counterpart surface to diverge from. The frozen `tree --json` contract is registered against the core's own wire, `docs/ground-truth-contracts.md:148-152`.)
**Core references:** `docs/ABI-CONTRACT.md:264-284` + `:114` (SQLite sidecar claims), `src/ui/cr_ui.cpp:1468-1488` (`scan_save_session_impl` writes only the .crsf; zero sqlite/crsidx code repo-wide — only the unrelated carve constant `FMT_SQLITE` at `src/core/carve/cr_carve.cpp:38`); `docs/ABI-CONTRACT.md:236` ("2 free" disk-map state) vs `include/cr/cr_ui.h:527-529` (0/1/3 only) and diff-to-fix abi.md:48-51 (S-009 remainder); `docs/ABI-CONTRACT.md:183` ("Fired on the thread running run() only") vs `include/cr/cr_ui.h:129-131` + trampoline `src/ui/cr_ui.cpp:662-685` (engine worker threads, no marshaling); `include/cr/cr_ui.h:69` (`CR_UI_ABI_VERSION 1u`) vs "ABI-since 1.1/1.2" annotations (`:166-168, :310-311, :361-365, :657-659`) and `docs/ABI-CONTRACT.md:288-294`; `docs/ground-truth-contracts.md:152` ("Unix epoch SECONDS") vs `include/cr/query/cr_query.h:46-47` (100-ns FileTimes) and the engines' FILETIME store (`src/core/fat/cr_fat_quick.cpp:256-259` `unix*10000000+116444736000000000`); audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Five documentation-vs-implementation falsehoods in the ABI layer, one of them shipped in a FROZEN wire contract: (1) ABI-CONTRACT §8 describes a SQLite session-index sidecar (WAL, busy_timeout, migrations, "cr_ui_query_run may answer from SQL") that does not exist anywhere in the code — §5 even claims `scan_save_session` writes it; (2) the disk-map doc documents a "2 free" state the header does not define; (3) the event-thread contract claim ("run() thread only") contradicts the header's honest "engine worker threads" and the unmarshaled trampoline; (4) fields annotated "ABI-since 1.1/1.2" cannot be detected — the version macro is still 1 and the doc's own append policy says no bump; (5) the frozen `tree --json` contract says `modified` is Unix epoch seconds while the engines store FILETIME (100-ns since 1601) — a consumer trusting the frozen doc mis-dates every row by 369 years.

## What the reference does

Not applicable — this is an internal truthfulness finding (the core's own docs are the
"reference" being violated). The frozen-contract registry (`ground-truth-contracts.md`)
exists precisely so consumers can trust doc claims byte-for-byte; a false unit line in a
frozen contract is the worst doc defect class the repo defines.

## What our core does

- **D1 (highest-impact)**: ABI-CONTRACT.md:264-284 normatively specifies the SQLite
  sidecar (WAL/busy_timeout 15000/synchronous NORMAL/cache −8000, `user_version`
  migrations, "cr_ui_query_run may answer from SQL", checksummed session_info), and
  :114 claims the save flow writes it. The implementation writes only the `.crsf`
  (cr_ui.cpp:1468-1488). `rg -l "sqlite|crsidx" src/ include/` → no hits (the only
  "SQLITE" string is the carve format constant).
- **D2**: ABI-CONTRACT.md:236 documents allocation state "2 free" for disk-map cells;
  `cr_ui.h:527-529` defines 0 unknown / 1 used / 3 bad only — the "2 free" state is
  unreachable per the header, and the bad-state conflation is the S-009 remainder.
- **D3**: ABI-CONTRACT.md:183 claims events fire "on the thread running run() only";
  `cr_ui.h:129-131` states events may fire from ENGINE WORKER THREADS and the
  trampoline (cr_ui.cpp:662-685) performs direct calls with no marshaling. The header
  is the truth; the doc is false as written.
- **D4**: "ABI-since 1.1"/"ABI-since 1.2" annotations exist on multiple fields while
  `CR_UI_ABI_VERSION` is 1u and ABI-CONTRACT.md:288-294 states appends require no
  version bump — consumers cannot programmatically detect the sub-version features.
- **D5**: ground-truth-contracts.md:152 (FROZEN contract, G-12 projection) says
  `modified` = "Unix epoch SECONDS (u64; 0 = unknown)". The engines store FILETIME
  (verified in the FAT engine conversion, and `cr_query.h:46-47` documents 100-ns
  units). Every `tree --json` consumer following the frozen doc mis-parses timestamps.

## Gap

Falsifiable claims: each of the five doc claims above is contradicted by the cited code
line. All five verified by direct read this session (D1 additionally by repo-wide grep).

## Impact

- D1 misleads integrators into relying on a query-acceleration feature that does not
  exist ("may answer from SQL") — silent behavioral surprise at scale.
- D5 is a data-correctness trap for every JSON-tree consumer (369-year date shift).
- D2/D3/D4 degrade contract trust generally; D3 specifically can cause host-side
  threading bugs (a consumer serializing events per the doc's promise).

## Reproduction / verification

- Executed greps + direct reads this session (all citations in §Core references):
  `rg -l "sqlite|crsidx" src/ include/` → 0; the five doc lines read side-by-side
  with their contradicting code lines; the FILETIME store verified at
  `cr_fat_quick.cpp:256-259` and the query header's unit comment at
  `cr_query.h:46-47`.
- Cross-checked against the session-2 abi.md module file (D5's unit conflict was
  surfaced by the session-4 core-reading leg; the doc line is the defect, not the
  wire).

## Fix plan (for the engineering team)

1. No architectural change — documentation corrections + one policy decision.
2. **Files to modify**: `docs/ABI-CONTRACT.md` (delete or mark §8 as NOT-SHIPPED/
   ROADMAP with a tracker ID; fix :114 to describe only the .crsf write; fix :236 to
   the header's state set; fix :183 to the header's thread contract; add a sub-version
   policy: either bump to 2 with a feature-presence probe or drop the "since 1.1/1.2"
   annotations in favor of a capability-flags export), `docs/ground-truth-contracts.md`
   (fix the `modified` unit line to "FILETIME 100-ns since 1601 (u64; 0 = unknown)" —
   and re-freeze with a change note).
3. **Tests to add**: a doc-contract lint is overkill; instead pin the wire truth:
   `TreeJson.ModifiedFieldIsFiletime` (round-trip a known FAT timestamp through
   `tree --json` and assert the 1601-epoch value), `Abi.EventThreadContractPinned`
   (event delivered on a worker thread is asserted-allowed — pins the header truth),
   `Session.SaveWritesCrsfOnly` (no sidecar files appear next to the .crsf).
4. **Contract/doc updates**: this IS the contract/doc update.
5. **Acceptance criteria**: every ABI-CONTRACT and ground-truth-contracts claim has a
   matching code cite in a CI-checked doc lint (optional stretch), or at minimum the
   five lines above are corrected; the round-trip test pins the unit forever.

## Tests to add

`TreeJson.ModifiedFieldIsFiletime`, `Abi.EventThreadContractPinned`,
`Session.SaveWritesCrsfOnly`.

## Related findings

- **M-029** — the `modified` unit fix is canonically planned there (query model);
  this finding registers the doc-truthfulness bundle.
- **S-009 remainder / N-020** — the disk-map state conflation (D2) is that finding's
  open half.
- **M-028** — same parameter class (P46) on the preview module.

## Open questions

None.
