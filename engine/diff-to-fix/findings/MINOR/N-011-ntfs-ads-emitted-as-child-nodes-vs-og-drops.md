# MINOR N-011 — NTFS ADS: og ignores named $DATA, core emits ADS as child nodes (superset)

**Severity:** MINOR
**Module:** ntfs
**Parameters affected:** B1 (detection correctness — result-set membership), B7 (hard-link/ADS/sparse), D11 (node fields), A29 (merge/dedup)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:5029-5160` (`FUN_1800069e0` iterates `$DATA` attributes by index 0, 1, 2, ...; at `:5159-5160` checks `bVar2 = FUN_180045440((longlong)puVar9)` (is_named($DATA)?) and `if ((int)CONCAT71(extraout_var,bVar2) == 0) { /* process unnamed $DATA */ }` — fall-through for named `$DATA` (ADS) silently skipped)
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:854-899` (iterates ALL `$DATA` attributes; emits ADS (named, non-`$`-prefixed) as child nodes under the canonical first-link node; resident ADS content captured via `resident_span` into `sp->resident_data` at `:866-875`; nonresident ADS extents via `parse_data_runs + rl.extents()` at `:877-885`; name composition `sp->name = props->name + u":" + sname` at `:890`); `tests/unit/test_ntfs_volume.cpp:894-933` (`ResidentAdsCarriesContent` expects `ads.txt:sec` with 16-byte resident payload); `tests/unit/test_ntfs_volume.cpp:1315` (`EndToEndWalk` asserts `stats.ads == 1u`) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's `NTFSSearchDLL` silently drops named `$DATA` (ADS — Alternate
Data Streams). The core emits ADS as child nodes under the canonical
first-link node, capturing resident ADS content and nonresident ADS
extents. The core's behavior is MORE useful (users recover ADS content
the og discards), but every NTFS volume with ADS files has more nodes
in the core's result tree than in the og's.

Session-1's NTFS B7 verdict "ADS as child nodes with resident capture"
was correct on the core side but the note "Og-side result shape for ADS
not fully decoded in corpus — no contradiction found" was incomplete.
There IS a contradiction: the og's `FUN_1800069e0` at `:5159-5160`
explicitly checks `is_named($DATA)` and skips when true.

## What the reference does

```c
// NTFSSearchDLL_decompiled.c:5029-5160 (FUN_1800069e0 — $DATA iterator)
//   Iterates $DATA attributes by index 0, 1, 2, ...
//   At :5159-5160:
bVar2 = FUN_180045440((longlong)puVar9);                  // is_named($DATA)?
if ((int)CONCAT71(extraout_var,bVar2) == 0) {
    // process unnamed $DATA (sparse/compressed/resident/nonresident)
    //   ... build runlist, expand compressed, capture resident content ...
}
// FALL THROUGH — named $DATA (ADS) silently skipped.
//   No node emitted. No content captured. No error logged.
```

The og's props runlist (`param_2 + 0x37`, set at `:5125`) only carries
unnamed `$DATA` fragments. Named `$DATA` (ADS) is dropped.

## What our core does

```cpp
// src/core/ntfs/cr_mftwalk.cpp:854-899 (audited 5d6da5f)
for (const auto& attr : attrs) {
    if (attr.type != 0x80 /* $DATA */) continue;
    if (attr.name.empty() || attr.name[0] == u'$') continue;   // skip unnamed or $-prefixed
    // Named $DATA (ADS) — emit as child node under the canonical first-link node.
    auto sp = std::make_shared<FileProperties>(props->clone());
    sp->name = props->name + u":" + attr.name;                  // e.g. "file.txt:stream"
    sp->parent_record = props->record_number;
    if (attr.resident) {
        // Resident ADS — capture content.
        std::string_view resident_span(attr.resident_data, attr.resident_size);
        sp->resident_data.assign(resident_span.begin(), resident_span.end());
    } else {
        // Nonresident ADS — extents.
        auto rl = parse_data_runs(attr.runlist);
        for (const auto& ext : rl.extents()) {
            sp->extents.push_back(ext);
        }
    }
    tree.add_child(canonical_node, std::move(sp));
    ++stats.ads;
}
```

The core emits one child node per ADS, with the ADS name appended after
`:`. Resident ADS content is captured into `resident_data`; nonresident
ADS extents are added to the child's `extents` vector.

## Gap

The og emits 0 nodes for ADS. The core emits 1 node per ADS. For an
NTFS file with K ADS streams, the og's result tree has 1 node for the
file; the core's has 1+K nodes (1 for the file + K for the ADS).

## Impact

- **Result-set membership:** the core's result set is a strict SUPERSET
  of the og's for any NTFS volume with ADS files. Comparative
  inventories diverge by K per ADS-bearing file.
- **User experience:** the core is MORE useful — users recover ADS
  content the og discards. ADS is sometimes used for legitimate
  metadata (e.g. Zone.Identifier for downloaded files, thumbnail
  caches) that the og silently loses.
- **Recovery correctness:** no impact on the primary stream — the
  file's main `$DATA` is correctly recovered by both sides.
- **Parity benchmark:** any og-vs-core file count comparison fails on
  volumes with ADS files.
- **Session size:** the core's `.crsf` is larger than the og's `.rsf`
  for the same volume (1+K nodes vs 1 per ADS-bearing file).

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `NTFSSearchDLL_decompiled.c:5029-5160` (`FUN_1800069e0` — the named-
  `$DATA` skip at `:5159-5160`).
- **Core evidence:** first-hand read of `cr_mftwalk.cpp:854-899` (the
  ADS emission loop).
- **Test evidence:** `tests/unit/test_ntfs_volume.cpp:894-933`
  `ResidentAdsCarriesContent` expects `ads.txt:sec` with 16-byte
  resident payload. `:1315` `EndToEndWalk` asserts `stats.ads == 1u`.
  Both tests currently PASS on the core; they would FAIL on the og.
- **Executed comparison:** not run — would require building both
  implementations and comparing result-tree sizes on an ADS-rich
  fixture.

## Fix plan (for the engineering team)

The decision is: match the og's drop-ADS behavior (parity) or keep the
emit-ADS-as-child-nodes behavior (superset, more useful).
Recommendation: **superset by default, opt-in parity** — ADS content
recovery is a real user benefit; the og's drop is a limitation, not a
deliberate choice. Mirrors M-007's shape.

1. **Architectural change:** add `WalkerFlags::emit_ads` (default ON
   for superset, OFF for parity).
2. **Files to modify:**
   - `include/cr/ntfs/cr_mftwalk.h:30-58` — extend `WalkerFlags` with
     `emit_ads` (bit 0x100 or similar).
   - `src/core/ntfs/cr_mftwalk.cpp:854-899` — gate the ADS emission
     loop on the flag.
3. **Algorithm/constant:** none.
4. **Tests to add:**
   - `NtfsVolume.AdsEmittedAsChildNodes` — fixture: a file with one
     resident ADS. Assert 1 ADS child node. PASSES today.
   - `NtfsVolume.AdsDroppedInParityMode` — same fixture; assert 0 ADS
     child nodes when `emit_ads` is OFF. FAILS today.
   - `NtfsVolume.AdsResidentContentCarried` — assert the ADS child
     node carries the resident content. PASSES today.
5. **Contract/doc updates:** `docs/AUDIT-NTFS.md` ADS section — note
   the default superset mode and the opt-in parity.
6. **Acceptance criteria:** the three tests above pass.

## Tests to add

- `NtfsVolume.AdsEmittedAsChildNodes` (PASSES today — regression guard)
- `NtfsVolume.AdsDroppedInParityMode` (FAILS today)
- `NtfsVolume.AdsResidentContentCarried` (regression guard)

## Related findings

- **M-007** — hard-link fan-out. Both are SUPERSET divergences where
  the og's behavior is a limitation. Same fix shape (opt-in parity).
- **M-008** — `$ATTRIBUTE_LIST` multi-hop chase. Both are SUPERSET
  divergences. Same fix shape.

## Open questions

- **OQ-S2A-3:** does the og's downstream `NTFSUtil.mo` `Ntfs2FAT`
  converter (or the writer) handle ADS separately? `NTFSUtil.mo` has
  `CNtfsStreamInfo` class (`:2037` `.?AVCNtfsStreamInfo@Ntfs2FATProc@@`).
  `NTFSUtil` is the NTFS→FAT conversion module, not the scan engine —
  its ADS handling wouldn't affect the scan result set. But the
  writer/UI might surface ADS via a separate path. Needs RE of
  `NTFSLib`/`NTFSUtil` to confirm no scan-path ADS handling exists.
