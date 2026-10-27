# MAJOR M-066 — preview-time archive-content enumeration exists only for the carve path's children: FS-origin archives preview as one opaque node where the og's ArchivePreviewModel browses inside any damaged zip

**Severity:** MAJOR
**Module / Surface:** preview / visualization (modules/preview.md)
**Parameters affected:** B1-adjacent (what preview reveals), D11-adjacent (preview Metadata contents for archive nodes), the preview contract's per-type behavior
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/UIUX/10-preview-subsystem.md:17` — the preview-type table: "| Archives | `ArchivePreviewWindow` (54) + `ArchivePreviewWindowChildItem`/`WorkItem`, `ArchivePreviewModel` (45)/`View` (96)/`Delegate` (23)/`Stub` (20) | full MVC | **browse-inside-archive of damaged zips** (pairs with EUArchiveRepair) |" (NEVER-CITED before this session — the prior preview audit, I-015, registered only the socket-protocol architecture via a different doc)
**Core references:** audited commit `dd82db9` — `src/core/preview/cr_preview.cpp:685-715` (the zip-family preview path: walks the central directory hunting **only** `word/document.xml` for the docx upgrade — no entry-list is produced for any other archive; `if (!found) return; /* xlsx/pptx/plain zip: no docx upgrade (label stays) */`); the carve path's inner-entry inventories (S-032, FIXED) attach children at the TREE level for carved archives only — an NTFS-deleted zip's node previews with label + size and no content enumeration
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** not in prior scope — I-015 (FIXED) registered the preview architecture divergence (socket protocol vs in-process reader) via a different artifact; S-032 (FIXED) landed carve-path inner entries; the preview-surface archive-browsing contract was never dispositioned (the UIUX/10 artifact was NEVER-CITED)

## Summary

The og's preview subsystem includes a full MVC archive-browsing family (ArchivePreviewModel, 45 methods; the window/view/delegate around it) whose documented purpose is browse-inside-archive **of damaged zips** at preview time — the user sees the contents of a corrupt archive before recovering it. The core's preview, for any archive, either upgrades a docx (hunting one entry name) or leaves an opaque label; content enumeration exists only on the carve path (S-032's tree-level children). A deleted-but-listed zip recovered from an NTFS scan — the common case — previews as one node with no content view.

## What the reference does

UIUX/10's preview-type table (quoted above) lists Archives among the og's preview types with a dedicated 5-class MVC family and the role "browse-inside-archive of damaged zips (pairs with EUArchiveRepair)" — preview is where the user verifies a damaged archive's recoverable contents before committing to recovery (and before deciding whether to route through the repair product).

## What our implementation does

`cr_preview.cpp:685-715`: the zip-family branch reads the central directory, hunts `word/document.xml` (the docx text-upgrade path), and `return`s without metadata when it is absent — "xlsx/pptx/plain zip: no docx upgrade (label stays)". No entry enumeration is emitted into the preview Metadata for any archive. The tree-level inner entries (S-032) cover carved archives only, and are a query-surface feature, not preview's.

## Gap

Falsifiable claim: for an FS-origin (non-carved) archive node, the core's preview Metadata carries no entry inventory, while the og's preview surfaces the archive's content list (the damaged-zip browsing family); the only enumeration the core has (S-032) is bound to the carve path.

## Impact

- **User-observable preview difference**: the recovery-decision workflow (preview → decide → recover) loses its content evidence for archives from filesystem scans — the exact population where a user most needs to know what is inside before recovering.
- Bounded: the tree children exist for carved archives; the docx upgrade exists; the archive bytes themselves are unaffected (no recovery-correctness impact).

## Reproduction / verification

Corpus doc evidence (strength-3): UIUX/10:17 quoted verbatim above. Core evidence (strength-3): the preview path quoted (the single-entry hunt + the explicit no-upgrade return). An executed comparison would preview the same damaged zip through both products (the og on Windows; the core via the preview CLI/ABI) — the static evidence already establishes the contract difference.

## Fix plan (for the engineering team)

1. Extend the zip-family preview branch: while walking the central directory (the walk already exists at `:685-715`), collect the entry list (name, uncompressed size, compressed size, per-entry CRC presence) into the preview Metadata (a bounded inventory — cap the entry count for hostile archives, e.g. 512 entries + a truncated flag, matching the preview budget model already in the file).
2. Reuse the S-032 entry-decode helpers where the formats overlap (the central-directory parser is already structurally similar) — the enumeration is the same walk the docx hunt performs, minus the single-name filter.
3. Tests: (a) a damaged-zip fixture (corrupt local header, intact central directory) → preview Metadata carries the entry list; (b) a docx still upgrades (the hunt continues alongside enumeration); (c) a hostile 100k-entry zip → the cap + truncated flag; (d) an FS-origin archive node (not a carve child) previews with the inventory — the case this finding names.
4. Acceptance criteria: the fixture tests green; the preview module row's archive cell moves to MATCH-with-bounded-cap.

## Tests to add

- `PreviewArchiveEnumeratesEntries` / `PreviewArchiveDamagedCdStillEnumerates`.
- `PreviewDocxUpgradeCoexistsWithEnumeration`.
- `PreviewArchiveEntryCapAndTruncatedFlag`.
- `PreviewFsOriginArchiveNodeEnumerated` (fails before).

## Related findings

- **S-032** (FIXED) — the carve-path inner entries (the shared walk; different surface).
- **I-015** (FIXED) — the preview architecture register (in-process reader vs socket protocol).

## Open questions

Whether the og's archive preview also drives the EUArchiveRepair routing decision (the artifact says "pairs with") — the core has no repair-bridge in preview either way (M-021/I-010 registered the repair bridge); not load-bearing for the enumeration gap.
