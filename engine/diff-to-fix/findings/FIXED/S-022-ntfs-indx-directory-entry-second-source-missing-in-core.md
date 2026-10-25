# SEVERE S-022 — NTFS directory-entry second source: og harvests INDX-block + $INDEX_ROOT entries into results; core has no INDX decode at all (D-N ruling premised on "og UNVERIFIED-7", now resolved)

**Severity:** SEVERE (missing second-source recovery subsystem the og ships — result-set membership og ⊄ core on damaged/reused-MFT volumes)
**Module:** ntfs
**Parameters affected:** B1 (result-set membership), B2 (recovery correctness), B5 (orphan reconstruction), B6 (post-format/remnant — the record-half of the same sweep), A26 (tree walk — session-1 og-premise refuted), Q69 (stubs/scope refusals — D-N ruling re-opened)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` — INDX wire family: `FUN_180045510 :54704` ('INDX'=0x58444e49 validator, usa_count<0x11 cap, fixup-offset 0x28..0x100), `FUN_1800457f0 :54823`/`FUN_1800458a0 :54847` (USA verify/apply), `FUN_180045990 :54871` (entry chain, last-entry-flag stop, per-entry $FILE_NAME sanity), `FUN_180045690/:54768`, `FUN_1800456c0 :54777`, `FUN_1800456f0 :54786`, `FUN_180046530 :55342` (4 KiB-stride INDX collector); raw scanner `FUN_18000c890 :9452` (claim-map gate :9529/:9685, INDX dispatch :9547, MFT-record recognizers :9550-53, run coalescing :9557-69/:9611, "RSTR" $LogFile :9582, 55AA-tail check :9584-90, 500K cap :9623, attr-name table walk :9653-69); INDX handler `FUN_18000b4c0 :8586` (split-block reassembly :8638-45, entry harvest :8652, record-keyed INDX registry :8653-8700); entry harvest `FUN_18000e780 :10923` (in-use-bounded walk :10943-51, props :10955, files→collection :10964, dirs→emit :10977-82); entry→props `FUN_18000e970 :11002` (gates :11025-49); $INDEX_ROOT variant `FUN_18000f760 :11609` (0x90 lookup :11631, large-index==0 :11633, ≤0x320 bytes, same walk :11642-83, fired from the remnant-record path :9951-52); emit/dedup `FUN_18000f2b0 :11410`, collection `FUN_180022760 :27420`, alias `:7067`, merger consult `:12411`, drain `:7778-79`; run consumer `FUN_18000f3f0 :11459` (runs > 8 sectors :11489, read :11499). Consolidated quotes: `diff-to-fix/evidence/decompiled-quotes/s3-ntfs-indx-second-source.md`. Corpus doc: `SearchDLL/ntfs-engine--deep-dive.md:108` ("orphaned-record sweep"). Strings: `reversed/NTFSSearchDLL.dll/strings_utf16.txt:10-22`.
**Core references:** `include/cr/ntfs/cr_ntfs_wire.h:192-195` ("no INDX-block walker exists and none is planned … plan ruling D-N, AUDIT-NTFS §4 row 9: 0 refs; og INDX walking itself is UNVERIFIED-7 in the corpus"); `src/core/ntfs/cr_mftwalk.cpp` build_props switch (ATTRIBUTE_LIST/FILE_NAME/STANDARD_INFORMATION/DATA/INDEX_ALLOCATION only; no 0x90/$INDEX_ROOT/INDX entry decode); remnant path (B6) parses $DATA runlists only; `rg -i indx src/ include/` → only the wire-header comment — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's full scan includes an "orphaned-record sweep" the corpus never decoded: a raw-cluster
walk (claim-map gated) that recognizes INDX blocks and MFT records in volume space outside the
live $MFT, plus a $INDEX_ROOT entry harvest for directory records found there. Directory
entries' embedded $FILE_NAME content (record ref + parent + times + real size + name) is turned
into real CNTFSFileProperties per entry — files go into a cross-source orphan collection the
post-walk merger consumes, directories are emitted directly (dedup'd). This recovers names and
metadata for files whose MFT records no longer exist but whose parent's index blocks survive —
the classic second source. The core has NO INDX/INDEX_ROOT decode at all; its D-N scope refusal
was explicitly premised on "og INDX walking itself is UNVERIFIED-7", which this finding resolves:
the og DOES walk INDX (in-use entries; not slack). Session-1's A26 og-side premise ("no INDX
walk in og either") is refuted.

## What the reference does

Four-layer evidence (all quotes in the evidence file):

1. **INDX wire family** — magic 0x58444e49 validator with USA fixup verify/apply and an
   entry-chain walker (`FUN_180045510/…510/7f0/8a0/990`, :54704-54945). usa_count capped at
   0x10 (≤ ~8 KiB blocks); per-entry sanity: real_size ≤ alloc_size, all entries share the
   first entry's parent ref; stops at the last-entry flag.
2. **Raw-cluster scanner** `FUN_18000c890 :9452` — reads volume ranges, walks clusters,
   probes the claim map (`FUN_18002c2e0`, the S-012 map), dispatches: INDX blocks →
   `FUN_18000b4c0` (:9547); MFT records → recognizers + **run coalescing** (records 8 sectors
   apart merged into runs, engine+0x7f8 list, consumed by `FUN_18000f3f0` for runs > 8
   sectors); "$LogFile RSTR" pages; boot-like blocks → `FUN_18000c390` (which emits via
   `FUN_18000be00` → `FUN_18000f2b0`).
3. **Entry harvest** — `FUN_18000b4c0 :8652` → `FUN_18000e780 :10923`: walks the entries
   region **bounded by the in-use size (+0x1c)** — NOT slack — and calls
   `FUN_18000e970 :11002` per entry: gates = entry ≥ 0x54, `$FILE_NAME.flags` must have
   DIRECTORY (0x10000000) or ARCHIVE (0x20), HIDDEN|SYSTEM both-set rejected, name-length
   bounds; builds props {record#+seq, parent ref, created/modified/accessed times, real
   size (48-bit), FOLDER bit from flags bit 0x1C, UTF-16 name at entry+0x52}. Files →
   `FUN_180022760(engine+0x888, props, state)` (record-keyed map); directories → name
   filtered (`FUN_1800445e0`, "$…"-names dropped) → `FUN_18000f2b0` emit (dedup
   `FUN_180023350`).
4. **$INDEX_ROOT variant** — `FUN_18000f760 :11609`: for directory records processed by the
   remnant/run path (`:9951-52`), if $INDEX_ROOT exists with large-index==0 and ≤ 0x320
   bytes of entries, the SAME entry walk/harvest runs on the resident entries.
5. **Plumbing** — the +0x888 collection is aliased to the engine's +0x140 map (:7067),
   consulted by the post-walk merger `FUN_180010c90`-family consumer (`:12411`), which
   materializes CNTFSFileProperties per entry and wraps them into nodes (FUN_180031070 —
   the same wrapper the USN-harvest consumer uses); drained into +0x770 at :7778-79.

The corpus deep-dive names this subsystem only as "orphaned-record sweep" (§:108).

## What our core does

Nothing on this path: `cr_ntfs_wire.h:192-195` documents the deliberate deletion of the INDX
wire family ("0 refs; og INDX walking itself is UNVERIFIED-7 in the corpus"); `cr_mftwalk.cpp`
build_props handles only 0x10/0x20/0x30/0x80/0xA0(extents); the remnant scan (B6) extracts
$FILE_NAME props and (post-7797bd6) $DATA runlists but never $INDEX_ROOT entries; no INDX-block
recognition exists anywhere in `src/core` or `include/`.

## Gap

Falsifiable claims:
1. A volume where a directory's MFT record was destroyed/reused but its $INDEX_ALLOCATION
   INDX blocks survive in unclaimed space: og result set contains the directory's in-use
   children (names, refs, sizes, times — as collection entries/lost nodes); the core's result
   set cannot contain them (no code path reads INDX blocks).
2. A remnant/recovered MFT record for a small directory (resident $INDEX_ROOT, large-index==0,
   ≤ 0x320 bytes of entries): og harvests the directory's children as props; the core emits the
   directory node only (no children from $INDEX_ROOT).
3. NOT a gap (both sides): INDX slack deleted-entry remnants — the og's walk is bounded by the
   in-use size and the chain stops at the last-entry flag; neither side harvests slack.

## Impact

- **Recovery correctness:** second-source names/metadata for files whose MFT records are gone
  — the same "deep-scan advantage" class as S-012's pass 2. On damaged-MFT/reused-record
  volumes the og surfaces entries the core cannot produce at all.
- **Result-set parity:** og ⊄ core on any volume with orphaned INDX blocks or recovered
  directory records. Inventory comparisons fail.
- **Scope ruling:** the core's D-N refusal (wire family deleted) was premised on the og being
  UNVERIFIED-7 — that premise is now resolved and the ruling must be re-decided on its merits
  (cost of an INDX validator vs. recovered-name yield).
- **A26 (session-1 module verdict "no INDX walk in og either") is refuted** — module row to be
  corrected.

## Reproduction / verification

- Decompiled evidence (strength 2): every link quoted file:line in
  `evidence/decompiled-quotes/s3-ntfs-indx-second-source.md` — validators, scanner dispatch,
  INDX handler + split-block reassembly, in-use-bounded entry walk, entry→props gates,
  $INDEX_ROOT variant, emit/dedup/collection/merger consult/drain. Two independent harvest
  paths share the same builder and collection.
- Corpus doc (strength 3): deep-dive :108 "orphaned-record sweep".
- Strings probe (strength 4): attribute-name table in the og binary lists $INDEX_ROOT/$INDEX_ALLOCATION.
- Core side: wire-header quote + negative grep (no INDX symbols in src/include).
- Executed comparison: NOT run (would require building the og; the og binary is x86 PE and out
  of scope for this environment).

## Fix plan (for the engineering team)

1. **Re-open D-N with the og evidence:** the og's cost model is one validator (~60 lines:
   magic, usa_count≤0x10, fixup-offset window, in-use≤alloc) + one entry→props builder
   (~80 lines: the FUN_18000e970 gates) + two feeders.
2. **Feeder A (remnant records — smallest lift):** in the remnant/run path
   (`cr_mftwalk.cpp` remnant reader), for directory records parse $INDEX_ROOT when
   large-index==0 and entries ≤ 0x320 bytes; apply the og's entry gates (≥0x54, flags &
   (0x10000000|0x20), reject hidden+system, real≤alloc); emit child nodes
   Origin::Lost/Confidence::MetadataOnly with dedup vs structured results (mirror the
   og's FUN_1800445e0 "$" name drop for directories).
3. **Feeder B (raw-space INDX blocks):** extend the remnant sweep to recognize 'INDX' magic
   at index-block stride (respect the record-size ladder's clusters-per-index-block) in
   unclaimed regions; validate + fixup (reuse the existing USA codec); walk in-use entries
   with the same builder; emit under root_lost. Cap per directory (og: 0x1f5 entries/block).
4. **Do NOT harvest slack** — the og doesn't; keep parity and avoid the false-positive risk.
5. **Docs:** update `cr_ntfs_wire.h:192-195` comment (UNVERIFIED-7 → resolved: og walks INDX
   in-use entries), PARITY-DELTAS D-N row, and modules/ntfs.md A26.
6. **Bench:** add a fixture with an orphaned INDX block (directory record zeroed, INDX blocks
   intact in free space) — recovered-children count must go 0 → N.

## Tests to add

- `NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren` — recovered directory record with
  resident $INDEX_ROOT: children emitted as Lost/MetadataOnly. FAILS today.
- `NtfsVolume.OrphanIndxBlockHarvestsEntries` — INDX block in unclaimed space, parent record
  destroyed: in-use entries emitted under root_lost with names/sizes/times. FAILS today.
- `NtfsVolume.IndxSlackNeverHarvested` — deleted entry in slack beyond in-use end: NOT
  emitted (parity with og; guards the deliberate scope edge). PASSES today (nothing emitted).
- `NtfsVolume.IndxHiddenSystemEntryRejected` — entry with flags HIDDEN|SYSTEM: dropped.

## Related findings

- S-012 (same membership family: og-only records the core cannot produce; the +0x888
  collection consumer shape is shared with the USN-harvest merger) · N-039 (USN gate — the
  same post-walk merger consults this collection at :12411) · B6/remnant module row (the
  record-half of the sweep; FUN_18002a780 shifted salvage is the per-block parser, this is
  the cluster-walker + entry harvest) · A26 correction (module row) · cr_ntfs_wire.h D-N
  scope note (Q69).

## Open questions

- Which volume ranges/vt slot invoke `FUN_18000c890` (free-clusters-only vs whole-volume;
  which engine mode) — affects yield estimates, not existence. Next artifact: the vtable
  slot wiring around the engine ctor (`FUN_180015eb0`) and any `FUN_18000c890` reference in
  `NTFSSearchDLL_blocks.txt`.
- Final placement of +0x888 entries in the result tree (lost tree vs merge-into-structured):
  the `FUN_180010c00` consumer (:12595-12624) builds node-shaped props; the exact tree
  insertion is the next decode (same artifact as the USN-finding open question).
- Whether the run-coalescer (`FUN_18000c890`) and the B6 remnant scanner (`FUN_18002a780`)
  are alternate entry points of one sweep or two modes (e.g. deep-scan vs post-format).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see git log for S-022)
**Verified by:** `NtfsVolume.OrphanIndxBlockHarvestsEntries`, `NtfsVolume.IndxHiddenSystemEntryRejected`, `NtfsVolume.IndxSlackNeverHarvested`, `NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren` — 3/4 fail-before proven on the implementation-reverted build (the 4th is the negative pin that passes both sides, exactly as the finding's test plan predicted); full suite 358/358 Linux default + 358/358 Linux ASan (354 + 4 net-new).
**Resolution notes:** The D-N scope ruling is re-decided on the og evidence and the harvest is implemented in the remnant path: (1) `cr_ntfs_wire.h`'s "no INDX walker exists and none is planned / UNVERIFIED-7" comment replaced with the og-decoded INDX/INDEX_ENTRY layout (the resolved evidence); (2) feeder B — `scan_remnant_buffer` now probes 'INDX' at the same 512-stride as 'FILE' and runs the og's full pipeline: validator FUN_180045510 (magic, size ≥ alloc, sectors ≥ usa_count−1, alloc ≥ in-use, usa_count < 0x11, the (alloc+entries_off)/512 == usa_count−1 cross-check, usa_offset ∈ [0x28,0x101)), USA fixup, chain validation FUN_180045990 (entry_len ≠ 0, first-entry parent consistency — a violation voids the block, real ≤ alloc per entry, last-entry-flag stop), then the in-use-bounded entry harvest with the FUN_18000e970 gates (≥ 0x54, flags require DIRECTORY 0x10000000 or ARCHIVE 0x20, HIDDEN|SYSTEM both-set rejected, name-length bound, 48-bit real size, FOLDER from bit 0x1C) and the 0x1f5-entries-per-block cap; slack is never read; (3) feeder A — remnant DIRECTORY records harvest resident $INDEX_ROOT (large-index==0, in-use ≤ 0x320 — FUN_18000f760) through the same builder; (4) remnant_range_scan's inter-block overlap raised to 8 KiB (usa_count < 0x11 ⇒ ≤ 16-sector blocks) so blocks straddling 4 MiB window edges are not torn — the og's 8-sector carry equivalent. Emission shape: entries become root_lost children, Origin::Lost / DeletedState::Damaged / Confidence::MetadataOnly (the og's +0x888 collection is consumed into nodes by its post-walk merger — same node shape the USN harvest emits); directory entries carry the "$..."-name filter (FUN_1800445e0); dedup by full file ref + (parent, name) against structured results (find_by_record), the same discipline the remnant record path uses. One deliberate wiring delta, documented: the og's raw scanner gates INDX dispatch on the pass-1 claim map; the core's remnant stage runs post-walk without that map, so the claim gate's semantics (skip live directories' blocks) are enforced by the structured dedup instead — result-set equivalent, O(harvest) rather than O(1) per block. The pre-existing chunk-boundary loss class (records/blocks split across parallel chunk edges) is unchanged and noted.
