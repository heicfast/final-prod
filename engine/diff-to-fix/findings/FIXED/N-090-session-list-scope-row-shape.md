# MINOR N-090 — Session-list scope and row shape drift from the og's summary record (no `auto_` filter; 1-second save time; label[32]; no device-type field)

**Severity:** MINOR
**Module:** session / ABI / CLI
**Parameters affected:** D12 (session persistence semantics — the summary fields), B10 (filename handling — the list's enumeration scope), P42 (row shape)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:445-446` (§5.5: the home list enumerates `auto_*` + `.rsf` in the SaveScan folder); `DRWUI_decompiled.c:432326-432333` (first-hand: name must **contain** `auto_` and `.rsf`); `rsf-format:167-176` (record-2: `volumeName[100]`, `deviceType` u32 with 2→"Desktop"/3→"Recycle Bin" resolution, SYSTEMTIME scanTime local+ms, stat1 elapsed-ms + stat2 tick)
**Core references:** audited `35adcb9`; re-verified on the rebased tree (post-`dd225da`, the CI-REL lane's empty-vs-missing fix — orthogonal to this finding's scope claim; the listing pattern is still `*.crsf` with no `auto_` filter). Listing matches any `*.crsf` with no `auto_` filter (`src/ui/cr_ui.cpp:387-448` at the rebased tree — the `*.crsf` pattern + the tail-5 case-insensitive check only; read first-hand). Row shape `include/cr/cr_ui.h:881-892`: `label[32]` (vs og 100), `save_unix` u64 seconds-UTC (vs og local SYSTEMTIME with ms), no device-type field; `elapsed_ms` only (og stat2 tick counter undecoded corpus-side).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Four small shape divergences on the W1a session-list landing, verified against the og's decompiled list and record-2 grammar: (a) the og's home list shows only `auto_*`-named saves — the core lists every `.crsf` in the directory (a user's manual save appears in core's list, not the og's view); (b) the scan time carries second-granularity UTC unix (the og: local SYSTEMTIME with ms); (c) the row label truncates at 31 chars (og: 99); (d) the og's record-2 `deviceType` (with its Desktop/Recycle-Bin resolution) has no row counterpart.

## What the reference does

The og's list enumerates the SaveScan folder keeping only names containing both `auto_` and `.rsf` (decompile :432326-432333 — the auto-save naming contract's other half); the banner fields come from record 2: volumeName[100], deviceType (resolved to display names), local SYSTEMTIME with millisecond precision, elapsed-ms and a tick counter (rsf-format:163-183).

## What our core does

`list_crsf_files` accepts every `.crsf` (cr_ui.cpp:413-446); the row carries `label[32]`, `save_unix` (u64 seconds), `files_found/deleted/carved`, `fs_family`, `elapsed_ms` — no device-type; the v5 tail stamps unix-seconds at save (`cr_scan.cpp:1955`), rendered local by the CLI.

## Gap

Four falsifiable deltas: (1) a `manual.crsf` in the dir yields a core row, none in the og's list; (2) sub-second save times display identically in core (1 s floor) where the og distinguishes ms; (3) a 40-char volume label lists truncated in core; (4) no row field can ever carry the og's device-type resolution.

## Impact

Presentation-layer only — the list is a host-side summary surface. Field (a) changes *which* files surface (scope), (b)-(d) change fidelity of what's shown. No recovery effect.

## Reproduction / verification

Corpus-derived (the decompile sites + rsf-format grammar, both read first-hand this session); core side read first-hand (the four citations above). Cross-verified against the W1b/W1c verification lane's parameter table (its D-1/D-2/D-3 rows).

## Fix plan (for the engineering team)

1. Decide the scope rule and document it as a delta either way: either filter to `auto_*` for og fidelity or register the every-file listing as a deliberate superset in PARITY-DELTAS (the current ABI-CONTRACT §7f text implies but does not state the difference).
2. Widen `label[32]`→`label[100]` in `cr_ui_session_row` (append-only struct change: the field is caller-allocated by count, so widening the array changes row stride — version-gate the struct or add `label2[68]`; pick the ABI-CONTRACT §9-compliant form).
3. Optionally add `save_unix_ms` (u32 centiseconds append) or document the 1-second resolution as the contract.
4. `device_type` (u32) append if hosts ask; the og's 2/3 resolution table goes in the contract text.
5. Tests: `SessionList.AutoFilterRule` (whichever rule lands), `SessionList.LongLabel`, `SessionList.SecondPrecisionPinned`.

## Tests to add

- `SessionList.ScopeRulePinned` (auto_-only or registered-superset).
- `SessionList.Label40Chars`.
- `SessionList.SaveTimeGranularity`.

## Related findings

- M-079 (this session — the label the row would carry is the one the CLI fails to fill).
- W1a landing `78a6552`/`eb16335` (the surface this calibrates).

## Open questions

The og's stat2 tick-counter semantics are undecoded (rsf-format §8 flags it "soft label") — nothing to mirror until decoded.


## SA-10 cite-refresh note (session 10 — line drift only; all four deltas re-verified STANDING at the reconciliation HEAD)

The triage wave re-verified this finding against HEAD after the W1c/dd225da/095a329 landings:
the empty-vs-missing discriminator (`cr_ui.cpp:398-415`, the directory-exists probe) is
ORTHOGONAL to this finding's four deltas (the `auto_` scope filter, the row-shape fields, the
label, the summary surface) — all four stand. Current cites: `list_crsf_files` spans
`cr_ui.cpp:387-470` (the POSIX branch grew with the discriminator); the `\*.crsf` pattern at
`:391-393`; `label[32]`/`save_unix` at `cr_ui.h:889-890`. The N-116 ordering sibling was
resolved mid-flight (`9445627` — the save-time sort landed); the scope/row-shape deltas here
are untouched by it.


---

## Session-11B cite-refresh addendum (2026-10-29, re-triage at `743d7b8d`)

drift map: save_unix stamp now cr_scan.cpp:1960 (was 1955); cr_ui_session_row now cr_ui.h:910-921 (label[32] at :919); the 740d9b56 CWD fix touched only the path-composition lines — all four legs stand


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-090 landing, S56 lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `CrUi.SessionListVolumeLabelFullWidthRoundTrips` (RED pre-fix as the ABI-append compile failure — `label_full` absent from both structs at the pre-fix tree, the house stashed-build precedent; GREEN post-fix: a 60-char host label round-trips COMPLETE through the v6 record tail into BOTH `cr_ui_session_info.label_full` and `cr_ui_session_row.label_full`; the legacy `label[32]` keeps its 31-char prefix view with NUL at [31]; a short label's `label_full == label` — the uniform degrade) + `CrUi.SessionListScopeSupersetPinsAutoAndManual` (the scope pin: BOTH an `auto_*`-named save and a `manual-save.crsf` list, both valid, same `save_unix`) + the layout round-trips updated to the v6 truth (`Crsf.VolumeInfoIdentityCardRoundTrip` gains the 60-char `label_full` leg + the +100 size pin; `Session.SaveUnixVTailRoundTrip` gains the v5-era degrade leg — the v6 tail cut reads zeros). Suite 1276/1276 green (1234 pass / 42 win-only skip / 0 fail, linux-release) + the touched suites clean under the ASan preset (Crsf/Session/CrUi.SessionList + the full cr_ui_tests 70/70).
**Resolution notes:** All four deltas dispositioned per the fix plan, the deliberate-superset option taken at every choice point (registered as PARITY-DELTAS D-129 — renumbered from its born-collided D-135 seat at the N-191/N-207 repair: the N-163 row keeps D-135): (a) the SCOPE rule — the every-`*.crsf` listing registered as the deliberate superset over the og's `auto_`+`.rsf` home-list filter (DRWUI:432326-432333 re-read first-hand this session; the core serves hosts that resume manual saves; a host wanting the og's view filters client-side); (b) the LABEL WIDTH — CLOSED, not registered: the og's record-2 `volumeName[100]`/strncpy(…,99) width now round-trips end-to-end — `ScanConfig.volume_label` widened to 100 (the host `volume_label_utf8` channel; the engine-decoded on-disk labels are inherently ≤32 bytes by the per-family decoders' own caps — FAT/exFAT 11, ext4 16, NTFS 32 ASCII-substituted — so the width serves the host channel exactly), the v6 record tail `label_full[100]` (the len-gated append, the v2..v5 house pattern; zeros on v1..v5 = the legacy card is the whole truth), the ABI tail appends `label_full[100]` on BOTH `cr_ui_session_row` and `cr_ui_session_info` (no version bump per §9 — the append-only pattern; `label[32]` preserved as the legacy prefix view so old callers see exactly what they always saw), the resume adopt prefers the full label with the legacy copy bounded by the SOURCE field (the 32-byte read never walks the widened destination); (c) the SAVE-TIME resolution — the 1-second contract documented (D-129b): the newest-first comparator needs second resolution only (the og's own auto-save file names are second-granular), equal-second ties break by path bytes (N-116's deterministic rule); (d) the DEVICE-TYPE — the not-carried registration (D-129c): the og's record-2 `deviceType` u32 with its 2→"Desktop"/3→"Recycle Bin" resolution table is a UI display classification with no core consumer; the table lives in ABI-CONTRACT §7f's fields bullet for the host that eventually asks. Trail: ABI-CONTRACT §7f (the four registrations) + docs/crsf-format.md (the v6 tail row) + PARITY-DELTAS D-129 + session.md's row + the ledger row + this entry.
