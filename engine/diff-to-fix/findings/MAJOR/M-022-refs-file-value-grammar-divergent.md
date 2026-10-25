# MAJOR M-022 — ReFS file-value walk: og uses a system-dir-count-gated dual grammar reading type at key+0; core uses a single 0x80-at-key+8 grammar and stops at the first data row

**Severity:** MAJOR
**Module:** refs
**Parameters affected:** A25/A26 (engine algorithm steps / tree walk), B1/B2 (detection/recovery correctness on ReFS 3.x-era volumes), B9-adjacent, X52 (scalability of the walk)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — :8096 (`FUN_18000b820` = `*(u16*)key`, returns 0 when keylen < 4; :8113 `FUN_18000b840` = `*(u16*)(key+2)` subtype), :1469-1494 (directory-table row dispatch: type 0x10 / 0x30+subtype {1→FUN_18000ba90 file, 2→FUN_18000c920 dir-link}), :8233+ (FUN_18000ba90 file handler), :8290-8355 (the file-value attribute loop: `uVar16 = *(u16*)key`; `0x178` → `local_c0 |= 2`; `0x108` → `local_c0 |= 1`; `0xf8` → `local_c0 |= 3`; size-0x28 key + `param_1[10] >= 8` + `uVar16 >= 0xb1` → modern path; size-0x10 + `uVar16 == 100` + name starts `"$I"` → `FUN_18000ea30` recycle handler; size-0x10 + `local_f8 == 1` → `FUN_18000e490`), :8761-8830 (FUN_18000c560 the default value handler: old layout requires type == 0x178; modern requires keylen == 0x28 and type >= 0x29; iterates the value's entries, `(*(byte*)(entry+10) & 0x18) == 0x18` → FUN_18000c790 else RefsNodeRecord + FUN_18000cf30 recursion), :7890-7960 (the `param_1[10]` counter: a `std::list` append loop with `"list<T> too long"` guards — **param_1[10] is the system-directory list size**, appended 4-at-a-time from the DBR's offset table at `boot + *(u16*)(boot+10)`; the ctor `FUN_180001500` zeroes it and sets sector size 0x200 at +0x58)
**Core references:** audited commit `34b3441` — `src/core/refs/cr_refs_engine.cpp:33-39` (row markers `ROW_FILE = 0x00010030` etc. — consistent with the og's key+0 type/key+2 subtype packing), :282-284 (`attr_type_of` = `le16(e.key + 8)`), :39 (`ATTR_DATA = 0x80`), :738-760 (the attribute loop: first `ATTR_DATA` row wins, `break` at :759), no dual-grammar gate, no `$I` recycle handler, no `param_1[10]`-equivalent system-dir-count switch (grep for recycle/$I in `src/core/refs/`: zero hits)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Inside a ReFS file row's embedded table, the og and the core read the row-type field from **different offsets with different ID spaces**, and the og switches grammars on a structural version signal the core does not have. The og reads `*(u16*)key` and dispatches `{0xf8 → property-flag 3, 0x108 → flag 1, 0x178 → the data-run value handler (old layout), modern layout: key-size 0x28 + type >= 0x29/0xb1, 100 + name "$I…" → recycle-bin special handler, size-0x10 specials}`, iterating **every** value row. The core reads `le16(key+8)`, matches only `0x80`, and **breaks at the first match**. Additionally, the og's "modern vs old" switch is gated by `param_1[10] >= 8` — decoded this session as the **count of system directories appended from the DBR's offset table** (a structural ReFS-generation discriminator, e.g. 3.x-era volumes carry more system-identity directories).

## What the reference does

Quotes and cites above. The og's file pipeline: directory-table row (type@key+0 = 0x30, subtype@key+2 = 1) → `FUN_18000ba90` → builds `RefsFileProperties` → iterates **all** value rows of the embedded table, dispatching on `*(u16*)key`:

- `0x178` (old layout): sets property-flag bit 2 and (via `FUN_18000c560`, which in old mode requires exactly type 0x178) walks the value's entries (`(flags & 0x18) == 0x18` → `FUN_18000c790`; else `RefsNodeRecord` + `FUN_18000cf30` recursion) — this is the **data-run materialization path**;
- `0x108` / `0xf8`: set property-flag bits 1 / 3 (timestamp/attribute-class markers);
- modern layout (`param_1[10] >= 8` AND key size == 0x28 AND type >= 0xb1 at the outer site / >= 0x29 in the handler): the same data-run walk under the newer on-disk grammar;
- type 100 (0x64) + key size 0x10 + name beginning `'$','I'`: `FUN_18000ea30` — **the recycle-bin `$I…/$R…` special handler**;
- size 0x10 + `local_f8 == 1`: `FUN_18000e490` special;
- everything else: `FUN_18000c560` (which also carries the old/modern split).

The version gate `param_1[10] >= 8` is the size of the system-directory `std::list` built from the DBR offset table — a structural generation check, not a version-number read.

## What our core does

`emit_file` parses the file row's embedded root node and iterates its entries looking for `attr_type_of(ae) == ATTR_DATA (0x80)` where the type is read at **key+8**; the first match's value is parsed as the run table; `break`. There is no dual grammar, no system-dir-count gate, no `$I` handler, no iteration of the remaining rows (timestamps come only from the embedded root's general-info block, `parse_general_info`).

## Gap

Falsifiable claim: the og and the core do not agree on where the file-value row type lives (key+0 vs key+8) or which IDs are data rows (0x178/modern-≥0x29 vs 0x80). At most one of the two grammars can be correct for a given on-disk layout; either (a) they are reading different row families and the core's 0x80-at-key+8 rows are a subset the og handles through another path, or (b) the core's grammar is tuned to the fixtures it was built against (synthetic images from the thesis tables) and diverges from the og's on at least one ReFS generation. Additionally the og's `$I` recycle handling and its property-flag rows (0xf8/0x108) have no core counterpart, and the core ignores value rows after the first data row.

## Impact

Recovery correctness on real ReFS volumes (parameter B1/B2): if (b) holds, data-run extraction can be wrong or empty on the affected generation — files listed with wrong sizes or unrestorable content — and the `$I/$R` recycle-bin files lose the og's special pairing/grading. If (a) holds, the impact collapses to the missing `$I` handling and the first-row-break narrowing. Either way the walk is not yet evidence-verified equivalent, and ReFS is a first-class mandate filesystem.

## Reproduction / verification

Decompiled evidence, first-hand this session (all cites above; the `param_1[10]` list-counter decode via the `"list<T> too long"` guard pattern at :7918-7957 is unambiguous). Core cites at `34b3441`. NOT execution-verified: no ReFS 1.x/2.x/3.x real-volume fixtures exist in either tree's test suite (the core's ReFS tests are synthetic; the og's QA logs cover no ReFS volumes — corpus SaveScan artifact: "no EXT4/HFS+/ReFS volumes were scanned by the vendor QA"). The decisive empirical test needs real ReFS images of at least two generations — see fix plan.

## Fix plan (for the engineering team)

1. **Reconcile the key layouts first (cheap, decisive)**: dump the embedded-table keys from a real ReFS volume (any Windows 10+ ReFS disk; also generate a ReFS 1.2-era image if possible) and check where the type u16 physically sits and what value data rows carry (`0x178`? `>=0x29`? `0x80` at +8?). This settles (a) vs (b) in one afternoon with `ntfs-3g`-style hex inspection (ReFS has no Linux reader — use the og itself or Windows).
2. **Port the og's grammar switch**: add a `system_dir_count >= 8` generation gate to the core's engine (the count is computable from the boot's offset table at `boot + u16@+10`, exactly as the og's append loop reads it) and the dual type dispatch (old: 0x178 data rows; modern: key-size 0x28 + type >= 0x29), keeping the existing 0x80 path only if step 1 proves it correct for some generation.
3. **Iterate all value rows** (drop the `break` at :759): apply the og's per-type handling — data rows contribute runs; 0xf8/0x108 rows set the property flags the og sets; the file's emission happens once after the loop (the og's shape).
4. **Add the `$I` recycle handler**: type-100 + key-size-0x10 + name `"$I…"` rows take the og's `FUN_18000ea30` path (decode its body — likely the $I/$R pairing that grades recycled files; the NTFS engine's equivalent is the `Recycle` origin in our tree) and the `local_f8==1` special (`FUN_18000e490`).
5. **Tests**: `Refs.FileValueTypeSpace` (fixture built from the step-1 dump: old-grammar 0x178 data row → extents extracted); `Refs.ModernGrammarGate` (boot offset table with >=8 system dirs → modern type row accepted); `Refs.RecycleBinDollarI` (a `$I…` row → the recycle grading/pairing, not the generic path); `Refs.AllValueRowsIterated` (a file with rows [0x108-flag, 0x178-data, 0x108-flag] → flags AND runs both applied).
6. **Acceptance**: byte-for-byte extent equality with the og's .rsf on the same real-volume fixtures (the bench harness's compare tooling already diffs .rsf).

## Tests to add

See fix plan §5 (four named tests + the real-fixture bench comparison).

## Related findings

I-012 (ADS parity — same decode session), N-007/I-004 (detection strictness / version superset — sessions 1-2), S-006 (lost-partition ReFS probes missing — discovery side).

## Open questions

(1) The exact semantics of `FUN_18000ea30` ($I recycle: pairing vs grading vs both) and `FUN_18000e490` — both are located and small; decode before implementing step 4. (2) Whether the core's key+8/0x80 grammar matches ANY shipped ReFS generation or is synthetic-fixture-only — resolvable only by step 1's real-volume dump. (3) The quick-searcher's dot-extension prefilter (`memchr(name, '.')` at :8271-8287 — files whose names lack a `.` take a different path in `RefsQuickSearchFiles`) was noted but not decoded to a verdict; low priority (quick-search surface only).

---

## Session-3 parallel-leg addendum (2026-10-26 — the special-handler bodies decoded: RESIDENT-CONTENT materialization)

The parallel S3-REFS leg read the bodies of the two special handlers this finding
calls out but does not descend into — and the answer resolves this finding's fix-plan
step 4 ("decode its body"):

- **`FUN_18000e490` (:10410-10462, the `size-0x10 + local_f8 == 1` special):** its
  body constructs a **`CRefsResidentDataRunList`** — `operator_new(0x50)` with
  `IDataRunList::vftable` swapped to `CRefsResidentDataRunList::vftable`, a
  one-entry runlist `{offset, length}` pushed via `FUN_180015b60`, cluster-granularity
  sanity gate (`runlength < 4` clusters → resident path), `*(int*)(obj+9) = val_len` —
  i.e. it **materializes resident/inline content as a synthetic data run** so the
  normal pipeline can recover it.
- **`FUN_18000ea30` (:10723-10768, the type-100 `"$I…"` recycle handler):** the same
  `CRefsResidentDataRunList` construction — consistent with `$I` files being small
  resident metadata records.

**Consequence:** the og's resident/inline files (and its `$I` recycle records) are
recovered WITH content through this path; the core never populates its `resident_data`
member (grep-proven) and its `val_len ≥ 0x28` gate drops resident-sized rows —
inline files surface metadata-only. That recovery-correctness gap is filed as
**S-020** (SEVERE) with this finding cross-referenced; implementing S-020's fix
requires this finding's grammar work (the resident rows are only reachable once the
dual-grammar/all-rows walk lands). The two findings share the real-volume fixture
prerequisite noted below (no ReFS 1.x/2.x/3.x images exist in either tree).

## Session-4 addendum (2026-10-26, commit `63a9167`) — OQ-1 and OQ-3 RESOLVED by decompile

**OQ-1 RESOLVED — `FUN_18000ea30` decoded (the `$I` recycle handler body):**
`ReFSSearchDLL_decompiled.c:10615-10850`, invoked iff the entry name starts with
`"$I"` (caller `:8389-8419`; the sibling `FUN_18000e490` at `:8424` handles the
`$R`-side special: name length 0x10 with flag 1). Semantics, verbatim-quoted in the
session-4 evidence sheets: for a small (≤64 KiB) `$I` recycle entry whose resident
record length matches its declared size, it computes the logical cluster (record
length + two metadata offsets ctx+0x5c/ctx+0x60 + caller adjustment), translates it
through the volume's block/extent map (mask+OR translation, red-black-tree walk at
:lVar2+0xf8), and builds a **`CRefsResidentDataRunList`** (IDataRunList) of at most
3 runs, attached to the file-properties node at **+0x62** — i.e. it materializes
recoverable data runs for the recycle-bin metadata stream so the paired `$R` file can
be recovered. Additional gates decoded: clusters 0-3 reserved → abort; cluster size
must be ≤64 KiB; resident gate file ≤64 KiB; `[len][data]` record check at +0xC.
**Fix plan step 4 is now fully specified** — the handler is a run-list builder (not a
grading pass); port it as such. The `local_f8==1`/`FUN_18000e490` half (the `$R`-side
pairing decision) remains the only undecoded piece.

**OQ-3 RESOLVED — the dot-prefilter is a name gate, not a grammar variant:**
`FUN_18000ba90` (`:8316-8336`): a `memchr(name, 0x2E, len)` scan — names containing a
dot take the full emit path (RefsNodeHeader tree, the 0x178/0x108/0xf8 type dispatch,
the `$I` handler, the emitters); **dot-less names are skipped entirely on the quick
pass**; entries with the node-kind flag (local_e8 ≥ 1) bypass the dot requirement.
Corroboration: length-0x28 names with first-u16 ≥ 0xB1 and Win8+ take another branch
(`:8373`). Verdict: the quick-searcher emits dot-containing names only — the core's
quick path has no equivalent gate (its quick search emits all names). Fold into fix
plan step 3's port as a quick-path-only prefilter (kind-flag bypass included).

OQ-2 (real-volume dump) remains the decisive open item; the session-4 decode does not
substitute for it.

*Convergence note (2026-10-26): the S3-REFS parallel leg and the session-4 leg decoded the same two handler bodies independently and agree (CRefsResidentDataRunList resident-content materialization; the session-4 decode adds the block-map cluster translation, the <4-run cap, the props+0x62 attach site, and the caller's '$I' name gate). OQ-1 is resolved by both; OQ-3 (dot-prefilter) is resolved by the session-4 leg alone. The S3-REFS leg's S-020 (resident-content gap) extends this finding's grammar work.*

