# MAJOR M-116 — the archive browse inventory (M-066's engine) is unreachable from every shipped consumer surface: the C ABI's cr_ui_metadata has no entries field and preview_metadata_impl silently discards Metadata.entries — M-100's retirement residue claim ("reachable today through cr_ui_preview_metadata") is FALSE and the exposure half is re-opened

**Severity:** MAJOR
**Module / Surface:** ABI / preview (modules/abi.md, modules/preview.md)
**Parameters affected:** A42 (public surface shape — the engine feature exists, the ABI drops it), A46 (the contract under-documents: ABI-CONTRACT does not state the entries are unreachable), P68-adjacent (the retirement record's truth)
**Corpus references:** the og's archive-browse product surface (the ZIP/7z/RAR inside-view): `re-workspace/artifacts/UIUX/` preview/browse artifacts (the browse-inside capability M-066/N-088 landed against); `og-codebase/archive.dll` (the og's own archive layer, register-row'd)
**Core references:** audited commit `b6e0a9ef` — `include/cr/preview/cr_preview.h:134-140` (ArchiveEntry) + `:191` (`std::vector<ArchiveEntry> entries` — the engine half, landed by 6d0f16c with the 512-entry hostile bound at :141-145); `src/core/preview/cr_preview.cpp` (the central-directory walk, the damaged-zip partial-CD salvage); **`include/cr/cr_ui.h:1214-1224` (cr_ui_metadata — kind/format/dims/orientation/datetime/duration/codec/snippet/truncated — NO entries field)**; **`src/ui/cr_ui.cpp:3613-3631` (preview_metadata_impl — copies kind/format/orientation/datetime/codec/snippet/dims/duration/truncated and NEVER touches `m.entries` — verified by reading the function in full and grepping `entries` in it: zero)**; `diff-to-fix/evidence/s10-verification-pass-b.md:36-38` (the false residue claim: "reachable today through `cr_ui_preview_metadata`"); no CLI verb exposes it either (the verb census: 11 verbs, none archive-listing)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (S11-R, 2026-10-29):** drafted as M-111 by this lane (the session-11 continuation lane, core baseline `8365956e`); renumbered at the push-time reconciliation — the parallel S10-F wave-2 lane's explorer-browse **M-111 pushed first and keeps the ID** (the SA-9 rule). The lane's other wave-3 filings were absorbed: the browse-surface finding itself duplicated their M-111 (dropped — their trace is deeper: the ShellExecuteA mounted-path branch, the enumeration impl :8699-8790, the IFSInterface vtable slots); the RAWSearchDLL infra minors' progress-shape item is registered by their I-031, the EUVideoRepair per-chunk-feed by their S-050 addendum. This finding — the M-100 re-open — is the lane's unique headline product (no remote lane caught it; verified by the findings-tree grep for `m.entries|Metadata.entries|cr_ui_metadata`: zero).

**Prior audit check:** M-100 was retired by the session-10 lane-B ("closed-as-covered — the archive browse-inside capability landed in 6d0f16c (M-066+N-088) BEFORE the finding's own baseline; the filing's central 'absent' grep was false"). The retirement's own residue note claimed the ABI path was "reachable today through cr_ui_preview_metadata" — **that claim is false** (verified first-hand this session at two trees: `8365956e` and `b6e0a9ef`: the impl discards the entries). The session-10 re-verification waves did not catch it because the claim was never executed against the impl. The parallel session-11 lanes' board-truth passes (f166440b et al.) verified the M-102/M-083 thumbnail landings but not M-100's residue text.

## Summary

The engine's archive inventory (the CD walk, the 512-entry hostile bound, the damaged-zip salvage — M-066's product) exists and is tested, but the only C-ABI metadata surface silently drops it: `cr_ui_metadata` has no entries field, and `preview_metadata_impl` copies every other Metadata member while `m.entries` goes untouched. A C host cannot list an archive's contents at any layer — the same "engine exists, ABI half missing" shape M-102 was filed for (and fixed with `cr_ui_thumbnail_generate`, cr_ui.h:1283-1290). The M-100 retirement record contains a false claim that hid this gap from the active board.

## What the reference does

The og ships browse-inside as a product surface (the results-page archive view backed by archive.dll and the ZIP-entry classification the OfficeTagFromXML analyser feeds). The engine-side inventory is consumable by the host through the product's own UI machinery — the capability the corpus documents and M-066/N-088 landed the engine half against.

## What our core does

The engine half is real and green (cr_preview.h:134-191; the CD walk; the 512 bound; the salvage-truncated flagging). The ABI half: `cr_ui_metadata` (cr_ui.h:1214-1224) carries ten fixed fields, none an entry list; `preview_metadata_impl` (cr_ui.cpp:3613-3631) bridges every scalar and string member — `m.entries` is not referenced anywhere in the function. The CLI exposes nothing either. Result: the feature is reachable only from C++ tests.

## Gap

Falsifiable: read `cr_ui.h:1214-1224` (no entries field) and `cr_ui.cpp:3613-3631` (no `entries` reference) — the engine's `Metadata.entries` cannot cross the ABI; and `s10-verification-pass-b.md:36-38` asserts it can. Both cannot be true; the code wins. Re-verified at `b6e0a9ef` (the cites shifted +5/+18 with the concurrent M-108/M-109 landings; the gap is unchanged).

## Impact

Every C consumer (the ABI's primary audience) is locked out of the archive inventory — the browse-inside UX stays blocked at the host layer exactly as the thumbnail UX was before `cr_ui_thumbnail_generate` (M-102's shape). Second-order: the board truth — a retired finding's Resolution contains a false claim that neither the filing lane's verification wave nor the parallel session-11 lanes executed; this finding repairs the record and re-opens the half.

## Reproduction / verification

Executed this session (read-level, twice — at `8365956e` and at `b6e0a9ef` after the 63-commit push window): the struct read in full; the impl read in full; the grep `entries` in the impl → zero; the retirement record's exact text re-read at s10-verification-pass-b.md:36-38. Engine-side cites re-read (cr_preview.h:134-191).

## Fix plan (for the engineering team)

1. The ABI half, the M-102 house pattern: `cr_ui_archive_entry_count(pv)` + `cr_ui_archive_entry(pv, index, cr_ui_archive_entry* out)` (two-call; entry = {name, size, compressed, CRC-present flag} — the ArchiveEntry fields at cr_preview.h:134-140) — OR an `entries`/`entries_count` append to `cr_ui_metadata` (struct-append is ABI-legal per the versioning rules; the two-call export is the cleaner superset).
2. `rrecov archive-list <ref>` CLI verb (the smoke surface).
3. ABI-CONTRACT §7 section + the export-count sites + API.md rows + the API-DELTA row-16 split (its "MATCH landed" claim is engine-true/ABI-false — N-150 owns the row annotation).
4. **Record repair:** correct s10-verification-pass-b.md's residue note (a dated addendum in the evidence file); annotate the FIXED-LEDGER M-100 row with the re-open pointer (the row stays — the engine half IS landed; the exposure half is this finding).
5. Acceptance: a plain-C consumer lists a ZIP's entries end-to-end (ui_abi_check.c block); the damaged-zip salvage crosses the ABI with truncated=1 honestly.

## Tests to add

- `CrUi.ArchiveEntryTwoCallPattern` — count probe + entry fill.
- `CrUi.ArchiveEntryDamagedZipSalvageDisclosed` — truncated crosses honestly.
- `CrUi.ArchiveEntryHostileBound` — the 512 cap at the ABI layer.
- `Rrecov.ArchiveListVerbSmoke`.

## Related findings

M-102 (the fixed twin — engine-real/ABI-absent, same repair shape; cr_ui.h:1283-1290); M-066/N-088 (the engine landing); M-111 (the explorer-browse capability — the other unreachable engine surface this window surfaced); N-150 (the doc-truth batch owning the stale counts this fix must not re-drift); N-137 (the thumbnail ABI header's own contradiction — the same shipped-header truth class).

## Open questions

None for the gap itself. Product question only: whether browse-inside's host surface (the og's results-page view) is in the merged product's UI scope — the ABI half is worth landing regardless (the engine work is otherwise dead surface).

## Resolution

**Status:** FIXED
**Fixed in commit:** <this push> (S42 lane, the M-first directive)

**Verified by:** `CrUi.ArchiveEntryTwoCallPattern` ·
`CrUi.ArchiveEntryDamagedZipSalvageAndAbstain` ·
`CrUi.ArchiveEntryHostileBound512` (3/3 GREEN; RED pre-fix by construction —
the exports did not exist, the tests cannot compile against the pre-fix
header); the plain-C consumer block in `tests/ui_abi_check.c` (compiles under
MSVC /W4 /WX on Windows CI; the count-0/error-path contract at the C surface);
`CrUi.CliHelpListsEveryDispatchedVerb` green over the new archive-list verb
(the three-site verb lock). Full suites: cr_ui_tests 67/67, cr_tests
1124 PASS / 0 FAIL / 42 env-skips (linux-release).

**Resolution notes:** The ABI half landed per the fix plan's option 1 — the
M-102 house two-call pattern: `cr_ui_archive_entry_count(pv, out)` +
`cr_ui_archive_entry_at(pv, index, out)` over `cr_ui_archive_entry`
{name[260], size, compressed, crc_present, name_truncated} (the ArchiveEntry
fields at cr_preview.h:134-140; the 260-byte name capacity with the honest
`name_truncated` flag — names are archive-native UTF-8 pass-through, never
re-encoded). The extraction runs ONCE at the first archive call on the handle
and is cached for the handle's lifetime (a stable listing — the og's
ArchivePreviewModel serves one model per browse view; a fresh handle
re-extracts); non-archive nodes report count 0, never an error; out-of-range
indices refuse with ERR_RANGE_OUT_OF_BOUNDS. The `rrecov archive-list --in
file.crsf (--disk N | --image file) --node ID [--json]` verb landed as the
smoke surface (the CLI goes through the core directly per the no-second-
singleton rule documented at rrecov.cpp:787; the driver gate matches the
other read paths). ABI-CONTRACT §5 gained the archive flow; API.md /
CORE-API-CURRENT.md counts re-truthed (85 exports / 12 verbs); the API-DELTA
row-16 MATCH claim now carries the ABI half's landing. **Record repairs:**
the s10-verification-pass-b.md false residue claim corrected by a dated
addendum; the FIXED-LEDGER M-100 row carries the closure annotation. Test
notes: the hostile-bound proof runs over an FS-origin node (the exact file
extent — the carve layer's extent arithmetic is its own audited surface);
the damaged-zip proof pins BOTH honest halves (the LFH-damaged salvage still
enumerates — the CD is self-contained; the CD-unreachable case abstains with
count 0 and no error). No safety hardening was touched.
