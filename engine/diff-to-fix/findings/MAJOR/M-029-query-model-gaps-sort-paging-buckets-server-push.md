
---

## Session-5 addendum (2026-10-26) — the filterbar family decode (artifact 31, first read) + the naming-hook correction (OQ-S4-2)

The never-cited UIUX/31-filterbar-family-decode.md was read in full this session; it
extends this finding's filter-model evidence without changing its scope:

- **7-dimension AdvanceFilterCondition layout confirmed** (0x48-byte struct:
  type/time/size/state/tag/path/search) — this finding's "7-group" row is the same object;
  the **tag and path dimensions have no core field at all** (the core's filter carries
  name-pattern, category, deleted, size, time — no tag, no path).
- **Per-mode persistence**: the og keeps 3 filter-slot memories (CAdvanceFileFilterMgr)
  with Path-mode and Type-mode bars persisting independently; re-assert action IDs
  0x1a/0x105/0x1006; the core has no filter-slot persistence.
- **CSetFilterThread**: the og refilters on a dedicated worker (SetFilter vtbl+0x10,
  classify +0xf0) — the core's query is synchronous; register as the async-query contract
  item (N-0xx family).
- **CCheckNodeThread re-grade wiring**: recoverability re-grading runs on a TimeCritical
  worker per selection — S-023's per-selection re-grade now has its threading evidence.
- **OQ-S4-2 naming correction (folds into this finding's doc-accuracy family)**: the og
  has NO "FileNNNN" carve-naming scheme (see N-063) — the corpus deep-dive artifact's
  "File0001.jpg lives in the UI" claim is wrong and is filed as a corpus-errata row; the
  vtable+0x48 hook is a one-shot per-scan "filter_generalraw" config fetch, i.e. it
  belongs to THIS finding's server-side filter-push section (the push mechanism's config
  source), not to naming.
