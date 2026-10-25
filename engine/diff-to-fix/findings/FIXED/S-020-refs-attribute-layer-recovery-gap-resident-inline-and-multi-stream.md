# SEVERE S-020 — ReFS attribute-layer recovery: og recovers resident/inline + indirect + all data-bearing attributes; core recovers only the first unnamed $DATA attribute

**Severity:** SEVERE
**Module:** refs
**Parameters affected:** B2 (recovery correctness), B7 (attribute/stream handling — the "ADS gap" filed properly), A27 (fragment/runlist reconstruction), B3 (partial), C20 (attribute record grammar)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — FUN_18000ba90 :8233–8530 (attribute loop + dispatch), FUN_18000c560 :8761–8849 (attribute-value walker + layout-gen gate), FUN_18000c790 :8854–8934 (run-entry grammar + container translation), FUN_18000cf30 :9294–9505 (indirect/paged attribute chase), FUN_18000e490 :10304–10524 (resident inline content → CRefsResidentDataRunList), FUN_18000ea30 :10615–10856 ($I resident variant), FUN_18000e020 :10084–10286 (CReFSDataRunList build + granularity validation); full map in `diff-to-fix/evidence/decompiled-quotes/s3-refs-engine-inventory.md` §6
**Core references:** `src/core/refs/cr_refs_engine.cpp` @ `34b3441` — emit_file :715–783 (attribute loop :739–760, first-$DATA-only `break` :759, `val_len < 0x28` gate :741), parse_data_runs :293–310 (embedded node only; rejects inner nodes :295–296), constants :39–40 (ATTR_DATA 0x80), header doc `include/cr/refs/cr_refs_engine.h:30–33` (0xb0 $NAMED_DATA, 0xc0 $REPARSE_POINT documented, never consumed), `include/cr/tree/cr_node.h:69–74` (resident_data field exists, never populated by refs)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ReFS engine recovers, per object, the union of its *attribute layer*: all data-bearing
attribute records (grammar switched by ReFS minor version), attribute records continued in
their own metadata pages (indirect attributes), and **resident/inline file content**
materialized as a `CRefsResidentDataRunList` — with a final runlist-granularity sanity check.
Our core parses exactly one attribute — the first row whose key type is `0x80` ($DATA) with a
≥0x28-byte value — takes its embedded data-run table, and ignores every other attribute row
(`break`), all resident content, and all paged attribute continuations. ReFS has no NTFS-style
named data streams, so this is not "enumerate ADS as separate results" on either side — the og
folds attributes into the object's single stream; the gap is that the core's fold covers only a
narrow subset of the attribute layer, dropping recoverable content the og recovers.

## What the reference does

(Citations are ReFSSearchDLL_decompiled.c unless noted; the inventory file §6 has the full
function map.)

1. **One stream slot per object, filled from all attributes.** A `refs::RefsFileProperties`
   carries one runlist pointer (props+0x62 → heap +0x73, `FUN_18000e020:10185`,
   `FUN_18000d970:9798–9801`; result contract `SaveScan/ifileproperties-part2--ext4-hfs-refs.md`
   §3 "u32 hasRun → DataRunList @+0x73").
2. **Attribute loop with layout-generation grammar switch** (`FUN_18000ba90:8354–8450`):
   for gen < 8 (ReFS 3.4–3.10) data-bearing attributes are keyed `0x178`; for gen ≥ 8 they are
   ≥0xb1-keyed with 0x28-byte values (`FUN_18000c560:8784–8800`,
   `FUN_18000d770:9661–9673`; gen tag derived at `FUN_180004980:2466–2477`). **Every** such
   attribute child is walked and its run entries merged into the same object list:
   `FUN_18000c790:8880–8931` — run entry `{block u64@+0, flags u16@+8 (0x40 = has block
   reference, 0x20 = physical/no-translate), ref-size u16@+0xa == 0x18, offset u32@+0xc, length
   u32@+0x14}`, container-translated when flag 0x20 is clear (:8893–8895), appended with
   adjacent-run coalescing (:8916–8931).
3. **Indirect (paged) attributes chased.** Records carrying their own LCN-quad block reference
   (the `FUN_18000b860` shape — value size 0x30 with a name and typed header
   {type ∈ {1,2}, size ∈ {4,8}}; the LCN quad is stored by `FUN_18000b210:7907–7960`) are
   followed: `FUN_18000cf30:9333–9334` reads the referenced pages, re-parses them as
   "MSB+ReFSFSRS" pages (:9363–9365), and merges their run entries into the same object
   (:9439–9442), recursing for further indirect sub-records (:9444–9463). Large/fragmented
   files whose run tables overflow the embedded node are therefore fully recovered.
4. **Resident (inline) content recovered.** Attribute records with 0x10-byte values whose
   object type is 1 (`FUN_18000ba90:8422–8427`) and "$I…"-named 0x64-keyed records
   (:8374–8421, the $Recycle.Bin metadata files) are materialized as a
   **`CRefsResidentDataRunList`** — construction `FUN_18000e490:10410–10436` (vtable swap
   :10430, resident byte offset stored at +0x48 :10436, stored into the props slot :10454) and
   `FUN_18000ea30:10723–10768`. The corpus deep-dive's "CRefsResidentDataRunList (resident —
   ReFS small files inline)" (`SearchDLL/refs-engine--deep-dive.md` §1) is thereby confirmed at
   construction sites. A non-resident runlist, built after the attribute loop
   (`FUN_18000ba90:8478`), supersedes a resident one — but when only resident content exists,
   it is what the user gets.
5. **Runlist sanity gate.** After building the non-resident runlist the og validates that the
   extents' granularity is consistent with the volume cluster size and **discards the whole
   runlist** on mismatch (`FUN_18000e020:10201–10281`) — a false-positive guard against
   garbage runs from damaged records.

## What our core does

`emit_file` (`src/core/refs/cr_refs_engine.cpp:715–783`):

```cpp
/* attribute rows: $DATA (0x80) -> embedded data-run table */
for (u32 i = 0; i < nv.ki_count; ++i) {
    const EntryView ae = entry_at(nv, i);
    if (!ae.ok || attr_type_of(ae) != ATTR_DATA || ae.val_len < 0x28)   // :741
        continue;
    std::vector<DataRun> runs;
    if (!parse_data_runs(ae.val, ae.val_len, runs))
        break;
    ...
    break; /* default (unnamed) data stream */                          // :759
}
```

- Only rows with key type `0x80` and `val_len >= 0x28` are considered; the **first** one wins
  and the loop `break`s — any further $DATA-bearing attributes (checksummed/integrity
  continuation attributes, secondary data attributes) are dropped.
- `parse_data_runs` (:293–310) reads only the *embedded* node and **returns false for inner
  nodes** (:295–296) — an attribute whose run table overflowed into pages yields **zero runs**
  (the og chases pages per §2.3 above).
- Resident content: no code path. The 0x10-byte-value attributes the og materializes are
  skipped by the `val_len < 0x28` gate; `0xb0 $NAMED_DATA`/`0xc0 $REPARSE_POINT` are documented
  in `cr_refs_engine.h:30–33` but consumed nowhere; `tree::FileProperties::resident_data`
  (`include/cr/tree/cr_node.h:69–74`, used by the NTFS/ZFS/Btrfs engines) is never populated
  by the refs engine. Inline files are emitted with zero extents → the safewriter classifies
  them "nothing to recover" (`include/cr/recovery/cr_safewriter.h:112`, GAP-S6-7 counter) and
  the report marks them metadata-only (`include/cr/recovery/cr_report.h:39`).
- No runlist granularity validation exists (runs are bounds-checked only, :746–757).
- No layout-generation grammar switch: one grammar for all 1.x–3.x (superset per I-004, but
  the gen<8 `0x178`-keyed datarun records the og decodes are not searched for).

## Gap

Falsifiable: on a ReFS volume containing (a) any resident/inline file, or (b) a file with more
than one data-bearing attribute row, or (c) a file whose data-run table is an inner (paged)
node — the og emits that file with recoverable content; our core emits it with no extents
(a/c) or truncated extents (b). Conversely there is no class of attribute content the core
recovers that the og drops.

## Impact

- **User-observable recovery-yield loss**: resident/inline small files (the common case for
  small files on ReFS, per the corpus's own "small files inline" characterization) are listed
  but not recoverable by our product — the single largest yield delta in the ReFS engine.
- $Recycle.Bin `$I` metadata (original-path/size evidence the og reads resident,
  FUN_18000ea30) is not read — degrades any future recycle-bin rename feature.
- Fragmented/huge files with paged run tables lose all content (inner-node rejection).
- Multi-attribute objects (integrity/checksummed streams) may recover only part of their
  content.
- This is the formal filing of the "project GAP: ADS" row that session-2's handoff left
  un-filed (modules/refs.md B7 note): properly scoped, it is an attribute-layer coverage gap,
  not a named-stream enumeration gap.

## Reproduction / verification

Decompiled-evidence comparison (strength 2; full map in
`evidence/decompiled-quotes/s3-refs-engine-inventory.md` §6/§7):
- Og resident construction: FUN_18000e490:10410–10454, FUN_18000ea30:10723–10768.
- Og attribute merge/indirect chase: FUN_18000ba90:8354–8450, FUN_18000c560:8784–8849,
  FUN_18000cf30:9294–9505, FUN_18000c790:8880–8931.
- Core first-only + resident-absent: cr_refs_engine.cpp:739–760, :293–310,
  cr_refs_engine.h:30–33, cr_node.h:69–74.
- Executed comparison NOT possible this session (no ReFS fixture in corpus — vendor QA scanned
  none, `SaveScan/ifileproperties-part2--ext4-hfs-refs.md` §3 note; core fixtures are
  synthetic). Next probe listed in scratch report.

## Fix plan (for the engineering team)

1. **Resident content** (`src/core/refs/cr_refs_engine.cpp`): in `emit_file`, handle attribute
   rows whose value is a resident descriptor (0x10-byte values per the og grammar; and/or type
   `0xb0` per the header doc): locate the inline bytes (value fields + node page offsets, og
   FUN_18000e490:10365–10375: `loc = value{+8,+0xc} + node{+0x5c,+0x60}`; block = loc /
   cluster; block < 4 → += node MSB base), read them from the cached page, and populate
   `props->resident_data` (the model already exists — cr_node.h:69–74). Non-resident runs, when
   present, take precedence (og order: resident first, overwritten by FUN_18000e020 after the
   loop).
2. **Attribute-loop breadth**: do not `break` after the first 0x80 row — collect runs from
   every data-bearing row (og: all ≥0xb1-keyed 0x28-valued rows for gen ≥ 8; 0x178-keyed rows
   for gen < 8), merging per og coalescing semantics. Add the gen<8 `0x178` grammar keyed on
   the checkpoint-body minor (already decoded for the descriptor-list gate at :567).
3. **Indirect attributes**: when an attribute entry carries an LCN-quad block reference
   (og shape: value size 0x30, name, typed header {1|2}/{4|8}), read the referenced pages via
   `read_page` and merge their run entries (bounded by MAX_TABLE_DEPTH / a hop budget, mirroring
   og recursion FUN_18000cf30). Remove the inner-node rejection in `parse_data_runs` and follow
   child page references instead.
4. **Runlist granularity validation** (og FUN_18000e020:10201–10281): after building the merged
   runlist, verify the power-of-two run-length band equals the volume cluster size; discard the
   runlist (downgrade to metadata-only) on mismatch.
5. **$I recycle metadata**: read resident `$I*` content (og FUN_18000ea30) and stash the
   original-path/size record on the node for the writer/report (coordinate with M-027's
   recycle-bin routing).
6. Update `docs/CAPABILITY-MATRIX.md` row "ReFS containers / security / ADS" and the
   CORE-GAP-LIST ADS row to cite this finding on closure.
7. Acceptance: synthetic fixture with (i) a resident 0x10-value attribute (content bytes in the
   metadata page) — recovered bytes must equal the inline content; (ii) two 0x80 rows — runs
   merged; (iii) an inner-node run table — runs recovered from the child page; (iv) a runlist
   whose run lengths are cluster-inconsistent — file downgraded, no garbage extents.

## Tests to add

- `Refs_Resident_Inline_File_Recovered` (resident 0x10-value attribute; extents empty but
  resident_data == inline bytes; safewriter writes them).
- `Refs_Resident_Dollar_I_Metadata_Read` ($I-named 0x64-keyed resident record → original-path
  evidence captured).
- `Refs_Multiple_Data_Attributes_Merged` (two 0x80 rows → union of runs, coalesced).
- `Refs_Paged_Run_Table_Followed` (inner-node $DATA value → child-page runs recovered).
- `Refs_Gen7_0x178_Datarun_Attribute` (gen<8 grammar: 0x178-keyed attribute yields runs).
- `Refs_Runlist_Granularity_Mismatch_Discarded` (inconsistent run lengths → metadata-only, no
  extents).

## Related findings

- N-036 (run-entry flag-bit and container-translation model — the run-entry grammar half of
  this layer).
- N-037 is folded into fix plan step 4 of this finding (granularity validation) — filed
  separately to keep the MINOR cosmetic scope visible.
- I-004 (version superset — the gen-gate in fix step 2 keys on the body minor, consistent with
  I-004's preserved superset).
- N-011 (NTFS ADS emitted as child nodes — the NTFS-side analogue; different FS, same B7
  parameter).
- modules/refs.md B7 "(project GAP: ADS)" row — superseded by this filing.

## Open questions

- Exact resident-descriptor field semantics for ReFS 1.x (og major==1 path sets page clusters
  = 0x4000/bps, FUN_180012390:13547–13550) — no 1.x fixture exists; implement from the 3.x
  grammar and mark 1.x resident decoding UNVERIFIED pending a fixture.
- Whether real volumes ever present multiple 0x80 rows per object vs the gen-gated ≥0xb1
  attributes (both are implemented by the og; both are in scope of fix step 2).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit)
**Verified by:** Refs.ResidentInlineFileRecovered (fail-before: FAILED on the pre-fix engine — resident_files 0, extents 0; pass-after: extent {page_base+0x3F00, 2 clusters} covering the inline bytes), Refs.ResidentDollarIMetadataRead (fail-before FAILED; pass-after: dollar_i_rows 1 + resident extent), Refs.MultipleDataAttributesMerged (fail-before FAILED — first-match break kept 1 of 3 rows; pass-after: 2 extents with adjacent coalescing), Refs.PagedRunTableFollowed (fail-before FAILED — inner-node rejection lost all content; pass-after: child-page runs recovered, paged_chases 1), Refs.Gen7OldGrammarDatarunAttribute (fail-before FAILED — 0x178 rows unknown; pass-after: run recovered), Refs.RunlistGranularityMismatchDiscarded (fail-before FAILED — garbage runs accepted; pass-after: discarded, metadata-only). All 6 fail-before proven on the engine-reverted build (stash/rebuild/run/pop); suite 402/402 Linux ASan (396 + 6 net-new).
**Resolution notes:** Implemented the og attribute layer end to end, with every mechanic independently re-verified first-hand in the corpus this session before coding: (1) RESIDENT/INLINE materialization — the FUN_18000e490 decode re-read (value length == type gate, cluster <= 64 KiB, size <= 64 KiB, run < 4 clusters, value{+8,+0xc} offset pair + record-context position, <4-cluster MSB-base rebase, container translate) is implemented in pointer form: content_abs = page_byte_base + (attr_value - page_buf) + o1 + o2 (the og's context-chain arithmetic and the pointer form cover the same bytes; the exact base of the +8/+0xc pair stays UNVERIFIED pending the OQ-2 real-volume dump — the synthetic fixture pins the value-relative reading), emitted as ONE synthetic extent (the og's CRefsResidentDataRunList placement, deferred-read form); "$I"-named type-100 records (FUN_18000ea30, the session-4-decoded recycle handler) take the same path with dollar_i_rows counted. (2) ALL value rows iterated — the first-$DATA break removed; every 0x80-keyed row contributes runs; adjacent runs coalesce (og c790/e020 adjacency). (3) DUAL GRAMMAR — the og's key+0 type dispatch added alongside the thesis key+8/0x80 grammar: 0x178 old data rows, >=0xb1-with-0x28-key modern rows gated on the generation tag (FUN_180004980 :2466-77 re-read: major 3 -> minor<5: gen 3, 5..10: 7, 11..12: 8, 13+: 9; modern iff gen >= 8 — this resolves the M-022 param_1[10] field-identity ambiguity in favor of the derived-generation reading), 0x108/0xf8 property-flag rows tracked. (4) PAGED/INDIRECT run tables — inner-node $DATA values are followed to their child pages (og FUN_18000cf30) instead of rejected; hop- and depth-bounded. (5) GRANULARITY VALIDATION — pow2-floor(avg run-cluster sector granularity) must equal spc, computed against the ALLOCATED size (the og reads props+0; the finding's "logical size" reading corrected on the :10213 re-read — the alloc-vs-run-count invariant); mismatch discards the whole runlist (the N-037 guard; that finding is closed by this implementation). (6) og run-entry recognizer — flags 0x40 + ref-size u16@+0xa == 0x18 accepted alongside the thesis 0x10 flag (N-036's recognizer half); flag 0x20 = physical (no translate); block 0 skipped. Residuals, honestly scoped: the container-MAP population (N-036's other half — multi-container translation) and the OQ-2 real-volume dump that settles the key+8-vs-key+0 layout question per generation both remain open (tracked in N-036 and M-022's OQ-2); the $I content is materialized but the $I/$R recycle pairing/renaming remains M-027's scope.
