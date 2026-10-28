# MAJOR M-079 — CLI auto-saves never carry the volume label: the `auto_<vol>_` naming half is unreachable from the only shipped driver

**Severity:** MAJOR
**Module:** session / CLI / orchestrator
**Parameters affected:** D12 (session persistence — the auto-save name shape), B10 (filename/path handling), Q68 (contract text — the ABI field exists but the CLI cannot fill it)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md:167` (record-2 `volumeName[100]` — "`GetVolumeName()`" of the scanned volume, always present in the corpus samples); `rsf-format:443-451` (§5.5: the auto-save naming `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf`, volume-name sanitizer `/ * : ? " < > |` → `_`, folder derived from the module dir); the live samples `og-codebase/log/DRWUI.log` (`SaveResultPath = …\SaveScan\auto_Local Disk (I_)10_14_23_13_21.rsf` ×3 — every auto-save labeled); `re-workspace/artifacts/UIUX/30-interaction-driver-ground-truth.md:66-67`
**Core references:** audited `35adcb9`. The naming: `src/core/session/cr_crsf.cpp:363-404` (`auto_session_name` — `auto_` + sanitized label + `%02d` stamp, byte-exact vs the shipped samples; test-pinned `test_session.cpp:1012`). The config field: `include/cr/orchestrator/cr_scan.h:239` `char volume_label[32] = {0};` — caller-supplied; ABI in-card `cr_ui.cpp:1163` copies from `in->volume_label_utf8`. **No fill site derives it**: `rg -n "volume_label"` → `cr_scan.h:239` (decl), `cr_ui.cpp:1163` (ABI copy), `cr_ui.cpp:2008` + `rrecov.cpp:1702` (the two naming reads — `auto_session_name(... cfg.volume_label)`). `fill_identity_card` (cr_ui.cpp:1829-1844) fills model/serial/firmware/wwn/interface but **not** the label. The CLI never sets it.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

W1c landed auto-save-on-completion with og-exact naming (`auto_<vol>_<M>_<D>_<H>_<m>_<s>`) — but the `<vol>` half depends on `ScanConfig.volume_label`, which is caller-supplied and has no fill site: the CLI (the only shipped driver of the feature) never populates it, and the ABI's `fill_identity_card` fills every other identity field except the label. CLI auto-saves are always `auto_<timestamp>` with an empty volume half; the og's `auto_Local Disk (I_)10_14_23_13_21` shape is unreachable from the CLI surface.

## What the reference does

The og derives the volume name from the scanned volume itself (`GetVolumeName()`, rsf-format:167 — every corpus auto-save sample carries a real label), sanitizes it (`/ * : ? " < > |` → `_`), and composes `auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` unconditionally on completion into the module-adjacent SaveScan folder (rsf-format:443-451).

## What our core does

The naming helper is corpus-exact when a label is present (verified: `cr_crsf.cpp:363-393`, empty-label → the `auto_<ts>` form). But the label only enters via `ScanConfig.volume_label` (cr_scan.h:239) or the ABI card's `volume_label_utf8` — and nothing fills either from the volume: `fill_identity_card` (cr_ui.cpp:1829-1844) populates model/serial/firmware/wwn/interface only; the CLI has no `--label` flag and no auto-derivation. Verified first-hand: the only two reads are the naming sites.

## Gap

`rrecov scan --image X --autosave-dir D` (or any CLI scan with autosave) produces `auto_10_28_...crsf` — the volume half is empty. Falsifiable: no CLI invocation can produce an auto-save filename containing a volume label, while every og auto-save sample does.

## Impact

Session-list fidelity (the W1a list shows the label-less names), and the product behavior diverges from the corpus's uniform samples. ABI hosts CAN supply `volume_label_utf8` (the seam exists), so this is specifically the CLI-driver gap plus a missing convenience derivation.

## Reproduction / verification

- Corpus-derived (rsf-format:167/:443-451; the three labeled DRWUI.log samples).
- Core first-hand: the volume_label grep census (decl + ABI copy + two naming reads, no fill site); `fill_identity_card` read at cr_ui.cpp:1829-1844 (label absent from the fill set); `rrecov.cpp` scan-flag census (no label flag).

## Fix plan (for the engineering team)

1. Derive the label at scan setup: in `fill_identity_card` (cr_ui.cpp:1829-1844) — where the CLI's identity card is already built — query the volume label (Win32 `GetVolumeInformationW` on the volume handle, or the storage layer's equivalent) and copy it into `cfg.volume_label` when the caller left it empty. Image targets: derive from the image filename (the og has no image-scan label semantics — document the choice).
2. CLI: optional `scan --label S` override flag (three-site lock + `-h`) for hosts/scripts that want explicit control.
3. Tests: `AutoSaveCarriesVolumeLabel` (scan a fixture volume → the auto file name contains the sanitized label); `AutoSaveLabelOverride`; `AutoSaveLabelFallsBackToTimestampWhenUnknown` (the current behavior pinned as fallback, not primary).

## Tests to add

- `AutoSave.VolumeLabelDerivedFromVolume` (the fill-site test — fails before, passes after).
- `AutoSave.LabelOverrideFlag`.
- `AutoSave.EmptyLabelFallbackShape` (the `auto_<ts>` form, now the documented fallback only).

## Related findings

- N-060 (FIXED — the explicit-save machinery W1c built on).
- N-090 (this session — the session-row shape limits where the label surfaces; related surface).
- W1c landing `c95335f` (the feature whose CLI leg this finding completes).

## Open questions

None.

---

## Session-10 addendum (2026-10-28, re-verification sweep at HEAD `84a1832`) — a NEW false comment sits on top of the gap

Re-verified STILL-ACTIVE (line re-pins: the ABI copy is now cr_ui.cpp:1181;
the naming reads cr_ui.cpp:2026 + rrecov.cpp:1714; the session-record copy
cr_scan.cpp:1933-1934). New discovery this sweep: `include/cr/orchestrator/
cr_scan.h:236-239` — the field's comment now claims "Supplied by the caller
(**the CLI/win layer reads it from the volume entry**; engines never own
it)". No code does: `win::enumerate_disks` reads the volume label via
GetVolumeInformationW into the volume-listing row (cr_enumerate.cpp:245)
but nothing plumbs it into `ScanConfig.volume_label` — the CLI never sets
the field and `fill_identity_card` (cr_ui.cpp:1847-1862) fills
model/serial/firmware/wwn/interface but not the label. The comment is a
fresh false claim layered on the unfixed gap (the doc says the wiring
exists; the wiring does not). Fix plan gains a step 0: delete or truth the
cr_scan.h:236-239 sentence before the fill site lands (the M-018
comment-stale class, now on an ACTIVE gap).

## Resolution

**Status:** FIXED
**Fixed in commit:** <this landing>
**Verified by:** `CrUi.CliScanFillsAutoSaveVolumeLabel` — the source-truth pin (the N-074/N-071 pattern for MSVC-only surfaces; executed fail-before: 4 assertions RED at the pre-fix tree — no fill site, no `--label`, no letter capture, the false comment — GREEN after), the win_syntax_check parse gate over rrecov.cpp (the fill code compiles structurally), and the naming half already pinned corpus-exact by `test_session.cpp:1012` (auto_session_name with a label — the composition this fix finally feeds); the full suite **1081/1081** linux-release. The live GetVolumeInformationW behavior executes on the Windows CI lane (the same posture as every app-layer landing).
**Resolution notes:** The fill landed exactly at the finding's recommended seam: `fill_volume_label(target, cfg)` at scan setup (beside fill_identity_card, the identity-card's sibling), with the derivation chain `--label S` (the explicit override, narrow-copied) → the `--volume X:` target's GetVolumeInformationW label (the og's GetVolumeName, rsf-format:167; OpenResult now carries the letter) → the `--image` filename stem (the documented choice — the og has no image-scan naming semantics; the S-030 divergence's naming half, now documented at the fill site) → empty for whole-disk scans (the og has no whole-disk scan to mirror; the timestamp-only `auto_<ts>` shape is the documented fallback, demoted from primary to fallback per the finding's test plan). The session-10 addendum's step 0 (the false cr_scan.h:236-239 comment claiming "the CLI/win layer reads it from the volume entry" with no wiring) is closed: the wiring now exists AND the comment names the real chain — the sentence is no longer false, it is the documented contract, and the pin keeps both the new truth in and the old false-claim sentence out. The usage text gains `[--label s]`. No core changes needed: `auto_session_name` and the session-record copy were already corpus-exact — this fix supplies the input they were starved of.
