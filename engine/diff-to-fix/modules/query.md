

---

## Session-11 fix row (2026-10-26, DFX11 — three M-029 sub-items closed)

- **Type sort CLOSED**: `SortKey::Type` (ABI code 7, append-only) — the
  og's results-view Type column: category in the FileFilter.xml
  iconIndex order, then extension, then name (pinned with the full
  8-row expected sequence).
- **modified-unit doc lie FIXED**: ground-truth-contracts now says
  FILETIME 100-ns since 1601 (the engines' native unit; the old "Unix
  seconds" line was false — 369-year mis-parse for trusting consumers);
  pinned by Query.ModifiedUnitIsFiletimeEverywhere. The stale Category
  enum row (Email/Bookmarks) fixed in the same pass.
- **Paging documented as the deliberate difference**: no og page-size
  constant exists (UI-layer virtualization, ~1000-item batches); the
  core's offset+limit is host-side convenience, no parity claimed.
- REMAINING (decode-blocked, next steps in the finding): the Date/Size
  bucket-ID translation (the og's buckets select by node+0x89 date TAGS,
  not ranges — a range translation without the tag decode would invent
  constants) and the general-raw naming bucket.
