# INFORMATIONAL I-031 — the RAWSearchDLL infrastructure register: the function-level walk's deliberate-divergence set and new trail data (pending-16 cap with oldest-first eviction, Stop() flushes partial files, engine emits no progress, 512-multiple sector refusal, the full controller vtable, the singleton layout)

**Severity:** INFORMATIONAL (registered deliberate divergences and new decode data — no defect; the walk found no ACUTE/SEVERE/MAJOR residue in the infrastructure)
**Module:** carve (the og engine's infrastructure layer)
**Parameters affected:** A31-35-adjacent (scan lifecycle/cancel/checkpoint boundaries), A38-adjacent (progress semantics), A51-adjacent (I/O behavior), A54-58 (module boundaries — the engine/host contract shape)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` — `FUN_18000c820 :9573-9591` (the pending-list 16-entry cap: `if ((count != 0) && (0x10 < count + 1))` → force-finalize + enqueue + host-notify + evict the OLDEST); `FUN_18000ce30 :9658-9693` (Stop: +0x150 flag; when the device is idle, every pending FileInfo is force-finalized `vt+0x10(node, 0, -1)`, the host sink is notified per file, then clear + free); `:9439-9441` (the per-chunk continue-check — the engine's ONLY observer interaction; no progress emission exists in the DLL); `FUN_18000c430 :9265-9268` (setup refuses sector sizes not a multiple of 512); the controller vtable at RVA 0x66e38 (36 slots, all resolved — previously only +0x48/+0x68 were on record); `FUN_180001350 :65-100` (the user-info singleton's full layout: {1, 0, 200000000, vector ids 1..169 @+0x78, stats-map @+0x90, byte @+0xa0, string @+0xb0, 0xff @+0xc0}); `FUN_18000b7a0 :8594-8621` (the 9-byte-stride id→name table at DAT_18008f070)
**Core references:** audited `46b81573` — the counterparts are registered deliberate divergences: D-101 (the stateless single-shot validator architecture — no pending objects), N-017 (FIXED; the cancel watcher), D-78 (the push-events progress model — the core's per-window callback is the superset), I-009 (the geometry superset — byte-granular, no sector refusal)
**Status:** READ-ONLY REGISTER — informational, no fix sought

## Summary

The S11 RAWSearchDLL infrastructure function-level walk (the trail's named
wave-2 candidate, never previously executed) read all 64 infrastructure
functions of the og's carve engine. The load-bearing machinery is fully
dispositioned by the existing trail (M-005/M-021/M-024/M-025/N-017/N-031,
D-78/D-100/D-101, s3, S-049). This register records the four boundary
details that were never documented, plus three new trail-data items — none
is a defect; each belongs in its existing register's boundary text.

## The register entries

1. **Pending-16 cap, oldest-first eviction** (D-101 addendum): the og tracks
   at most 16 size-unknown candidates concurrently; the 17th force-closes the
   OLDEST at its best-known size and emits it. Binds any future
   streaming-analyser port behind the M-021 seam (the cap bounds memory AND
   truncates the least-recent file on overflow).
2. **Stop() flushes partial files** (D-101 addendum): on Stop with an idle
   device, in-flight pending files are force-finalized and EMITTED (partial
   content reaches the user); the core's cancel discards un-emitted
   candidates — inherent to the registered stateless architecture, one
   sentence for the boundary note.
3. **Engine emits no progress** (D-78 carve-layer note): the og engine's
   only observer interaction is the per-chunk continue-check; DRWUI computes
   progress host-side. The core's per-1-MiB-window callback is the
   registered superset.
4. **512-multiple sector refusal** (I-009 clause): the og's setup returns
   failure for sector sizes not a multiple of 512 (no carve at all on
   520-byte-sector devices); the core is byte-granular — one clause for
   I-009's register text.

## New trail data

- The complete CRAWSearchController vtable (36 slots resolved, RTTI-verified
  via the COL at 0x757d8) — previously only slots +0x48/+0x68 were on record.
- The user-info singleton's full layout (the 200 MB cap at +0x70 has 12+
  readers; +0x60=1/+0x68=0 are write-only resets with NO in-DLL reader —
  semantics UNVERIFIED, carried as the walk's one open note).
- The 9-byte-stride id→name table at DAT_18008f070 (S-049's supporting
  mechanism, now pinned).

## Impact

None as defects; the value is completeness of the boundary registers (D-78/
D-101/I-009) and the vtable map for any future engine-side work.

## Reproduction / verification

Decompiled (strength 2): every cite re-read first-hand; the full walk with
the per-function disposition table (64 infra functions, every one
dispositioned COVERED-BY/MATCH/candidate) at
`evidence/decompiled-quotes/s11-rawsearchdll-infra-walk.md`.

## Fix plan (for the engineering team)

Documentation-only: three one-line boundary addenda (D-78, D-101 ×2, I-009)
in docs/PARITY-DELTAS.md carrying the register entries above; no code change.

## Tests to add

None.

## Related findings

- N-139 / N-140 (the walk's two MINOR filings — the only non-INFO residue)
- D-78 / D-101 / I-009 (the registers these entries extend)
- M-021 / M-024 / M-025 / M-005 / N-017 / N-031 (the covered machinery the
  walk re-verified)

## Open questions

The singleton's +0x60/+0x68 reset fields have no in-DLL reader (semantics
UNVERIFIED — host-side read channel or vestigial; carried in the evidence
file's §4).


---

## S11-R addendum (2026-10-29, the reconciliation lane; re-verified at `b6e0a9ef`)

**Register extension (this lane's walk, two items the register does not
carry):**
1. **The dormant `only_common` vendor mode** — when config/config.ini's key
   is set (the shipped og config does NOT set it), the registration walk
   (`FUN_180004fb0 :3013-3056`) registers ONLY `RawFormat_xjq::
   CommonRawAnalyser` (id 0xa8=168) and the dispatch's map/multimap fast
   paths are disabled (`FUN_18000cf60 :9842-9893` falls to the list layer).
   Core analog: none (repo grep `only_common` → 0 hits) — **register as
   deliberate divergence** (a vendor QA mode; the core's compiled-in signs
   table supersedes its purpose). One PARITY-DELTAS D-row.
2. **SCAN-ENGINE-DELTA A4's phrasing correction** — "sector-size query via
   vtable slot 5" is the **DEVICE** vtable's slot 5 (`:9192/:9203`,
   IStorageDevice: +8 Read, [3]/[4] GetSize, [5] GetSectorSize), not the
   searcher's. One-row edit in the trail's own gate doc.
Both verified first-hand in the decompile; the lane's full infrastructure
census (the 18-row itemization, the 14 MATCH confirmations incl. the
ping-pong slide at :9620-9622, the zero-threading/zero-error-propagation
negatives) is preserved in the backup branch evidence and corroborates this
register's rows.
## Resolution

**Status:** ACKNOWLEDGED (the register's deliberate-divergence set confirmed; the fix plan's documentation-only addenda applied)
**Fixed in commit:** this session's I-031 landing (session-42; cite-by-title: "fix(docs): I-031 - the RAWSearchDLL infra register's boundary addenda land in PARITY-DELTAS + the A4 phrasing repair")
**Verified by:** the four register entries' corpus cites re-read first-hand this session (FUN_18000c820 :9573-9591 the pending-16 cap; FUN_18000ce30 :9658-9693 the Stop flush; :9439-9441 the per-chunk continue-check; FUN_18000c430 :9265-9268 the 512-multiple refusal; the S11-R pair :3013-3056 / :9842-9893 for only_common) — all present and as decoded; the core counterparts confirmed still the registered divergences (stateless single-shot D-101, push progress D-78, byte-granular D-59c, no only_common analog by grep).
**Resolution notes:** The documentation-only fix plan applied in full: (1) D-101 carries both boundary addenda — the pending-16 cap with oldest-first eviction (the memory bound AND the least-recent-truncation semantics any future streaming port must decide explicitly) and the Stop()-emits-partials boundary (the core's cancel discards un-emitted candidates — the registered stateless consequence, now on-file so the host layer does not paper over the cancel-during-carve difference); (2) D-78 carries the engine-emits-no-progress carve-layer note (the og engine's only observer interaction is the per-chunk continue-check; the core's per-1-MiB-window callback is the superset); (3) D-59c carries the 512-multiple sector-refusal clause (the og refuses non-512-multiple sector sizes outright; the core carves regardless); (4) the S11-R addendum's two items landed — the dormant `only_common` vendor QA mode registered as D-114 (deliberately not ported, rationale on-file) and SCAN-ENGINE-DELTA A4's "vtable slot 5" phrasing corrected to the DEVICE vtable's slot 5. The walk's one open note (the singleton's +0x60/+0x68 write-only reset fields, no in-DLL reader) stays open in the evidence file — no core behavior attaches. No code change.
