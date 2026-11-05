# MINOR N-194 — The og's two byte-size display formatters (binary /1024 tiers — log-side KB/MB/GB with no bytes/TB tier; UI-side B..TB full set) have no core counterpart: hosts cannot render og-faithful sizes from any core surface

**Severity:** MINOR
**Module:** query / display math (orchestration-adjacent surface)
**Parameters affected:** B10 (surfacing fidelity), D13/D11 adjacency (the report/UI schema's size axis), O38-adjacent (the M-125 progress-display family precedent)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:264525-264583` (FUN_140163580 wide-char + FUN_140163650 ascii — the LOG-side formatter: `bytes < 0x100001 → "%.2f KB"` (÷1024), `< 0x40000001 → "%.2f MB"` (÷1048576), else `"%.2f GB"` (÷1073741824) — binary units under decimal labels, NO bytes tier, NO TB tier, tier flips only above 1 MiB+1 B / 1 GiB+1 B); `:1556295-1556400` (the tr-family UI formatter: `"%1 B"` < 0x401, KB < 0x100001, MB < 0x40000001, GB < 0x10000000001, TB ÷ 1099511627776.0 — full tier set, `'f'` format with caller precision, separator-strip); runtime anchor `artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md:95,134-135` ("Files Found: 1139157 (143.74 GB)" — GiB under a GB label)
**Core references:** audited `34d6cf10` — grep over `src/` + `include/` for a tiered size formatter: zero hits; the CLI's one fixed site `src/app/rrecov.cpp:1915-1917` (`%.1f MiB`); the ABI carries raw u64 bytes; the landed precedent family `include/cr/query/cr_query.h:109-119` (`progress_display_*`) has no size sibling
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og renders human-readable sizes through two binary-tier formatters (the log side: KB/MB/GB only, "%.2f", tier boundaries at 1 MiB+1 / 1 GiB+1 — exactly 1 MiB renders "1024.00 KB"; the UI side: the full B..TB set with caller precision). The core has no equivalent: every surface emits raw bytes, and the one CLI site is a fixed MiB form. A host (or the core's own future UI) cannot reproduce og-faithful size strings from any core function — at every tier boundary the naive rendering diverges (1 MiB → og "1024.00 KB" vs naive "1.00 MB").

## What the reference does

Both formatters decoded with their exact tier constants and flip points (quoted in the corpus references; the constants verified this session — 0x40000001 = 1 GiB+1, 0x10000000001 = 1 TiB+1). The log side's quirk set: binary math under decimal labels, the missing B/TB tiers, and the +1 flip boundaries.

## What our core does

Raw u64 bytes everywhere; one fixed `%.1f MiB` CLI site; the M-125 `progress_display_*` family (the landed precedent for exactly this class of display-math helper) has no size sibling.

## Gap

Falsifiable: at `34d6cf10`, no core function maps a byte count to the og's display string (grep-verified); two og formatters with different tier sets mean even a correct host-side reimplementation needs both.

## Impact

Display fidelity only (no data path); every future UI/report host faces the boundary divergence. MINOR per the M-125 precedent's calibration.

## Reproduction / verification

Corpus-decompile evidence: both formatter bodies decoded this session (the agent's sheet carries the full constant tables; the "%.2f MB/GB" strings verified at DRWUI:264544-264583 by direct grep this session). Core side: the grep census at `34d6cf10`.

## Fix plan (for the engineering team)

1. Add `progress_display_size`-family helpers beside the M-125 set in `include/cr/query/cr_query.h`: one og-log-faithful form (KB/MB/GB, "%.2f", binary divisors, the +1 flip boundaries) and one UI-faithful form (B..TB, caller precision) — named for their sources (e.g. `display_size_log`, `display_size_ui`).
2. Wire the CLI's size prints through the log-faithful form where the og's CLI/logs render sizes (parity of the observable text).
3. Files: `include/cr/query/cr_query.h`, `src/core/query/cr_query.cpp`, `src/app/rrecov.cpp`.
4. Acceptance criteria: the boundary table pinned by tests (1 MiB → "1024.00 KB"; 1 GiB → "1024.00 MB"; 2 TiB → "2048.00 GB" on the log side; the UI side's tier set separately).

## Tests to add

- `Display.SizeLogFormatterBoundaries` (the three flip points + exact strings).
- `Display.SizeUiFormatterTiers` (B..TB + precision + the zero-bytes forced-0 quirk).
- `Display.SizeZeroHandled` (`bytes < 1 → 0` on the log side).

## Related findings

- M-126 (the ETA model — the same "decode the og's display math" family; this finding's addendum this session records the ETA seed).
- M-125 (retired; the progress-display precedent this fix follows).

## Open questions

None — both formatters are fully decoded.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S66 lane — renumbered at the push-rebase: the S64/S65 racing seats taken per SA-9)
**Verified by:** `Display.SizeLogFormatterBoundaries` (the three tiers, the +1 flip boundaries at 0x100001/0x40000001, the no-B/no-TB shape, the <1-writes-nothing gate, the 143.74 GB runtime anchor) + `Display.SizeUiFormatterTiers` (the B..TB set, the 0x401/0x100001/0x40000001/0x10000000001 boundaries, the zero-bytes forced-0 precision quirk) + `Display.SizeUiFormatterPrecisionAndStrip` (caller precision, the all-zero full-precision fraction strip, the negative-precision clamp). RED proof: the implementation stashed (declarations kept) — link failure, all three tests Not-Run (the absence WAS the finding); GREEN post-fix, Query/Display/Error/CrUi families 110/110, win_syntax_check green (the CLI wiring parse-gated).
**Resolution notes:** Corpus re-verified first-hand at the fix: FUN_140163580/FUN_140163650 (DRWUI:264525-264583 — the log-side tier structure, `%.2f` forms, 1024-tier divisors, the `param_1 < 1` nothing-written gate) and the tr-family UI formatter (DRWUI:1556295-1556400 — the 0x401/0x100001/0x40000001/0x10000000001 ladder, the `local_100 = 0` zero-bytes precision quirk, the `".%1".arg('0', prec)` replace-with-empty strip tail). Landed exactly per the fix plan: `query::display_size_log` + `query::display_size_ui` beside the M-125/M-126 display-math families (cr_query.h/cr_query.cpp); the CLI's four size prints (image info, disk listing, volume listing, unreadable-bytes diagnostic) wired through the log-faithful form — the throughput rates (MiB/s) stay rate-formatted, not size-formatted. The UI-side strip decoded first-hand from the call chain (`final.replace("." + zeros(prec), "")`): a fraction that is all zeros across the FULL precision strips with its point ("5 KB"); a partial fraction ("1.50 KB") keeps. Residual register-opaque tail noted: FUN_14064e240 (the og's locale-helper char in QString::number's format slot) — the port renders C-locale via snprintf, where no group separator exists to strip; the observable string for a C-locale host matches. The CLI's previous `%llu MiB` forms are superseded by og-parity text (no CI/contract surface pinned them — verified: no workflow or test greps the old forms).
