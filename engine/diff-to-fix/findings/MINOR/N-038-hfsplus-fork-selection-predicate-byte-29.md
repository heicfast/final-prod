# MINOR N-038 — HFS+ fork-selection predicate: og gates on file-record byte +0x29 bit 0x20 (resource fork + decmpfs mark); core gates on empty-data-fork substitution

**Severity:** MINOR
**Module:** hfsplus
**Parameters affected:** B2 (recovery correctness — which fork's bytes are recovered), B7 (fork/ADS handling), A25 (per-record decision), B10-adjacent
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c :7663-7684` (`FUN_180009940` file-record decode: fileID@+0x08, dates@+0x0C/+0x10/+0x18, dataFork@+0x58, resourceFork@+0xA8), the fork-selection branch quoted verbatim: `:7665` (`(*(byte*)(param_6 + 0x29) & 0x20) == 0` → data fork; else `:7677` `node+0xce |= 4` mark + `:7678` resource fork), `:7669` (data-fork size fallback `totalBlocks << 12`)
**Core references:** `src/core/hfsplus/cr_hfsplus_engine.cpp:693-733` (data fork always chosen; decmpfs content substituted when the data fork is empty), audited commit `d4e5f5b` (core `34b3441` lineage)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og chooses a file's emitted fork by a single bit in the catalog record — byte
`+0x29 & 0x20` — and when set, emits the **resource fork** as the node's content and
marks the node (`+0xce |= 4`) for the decmpfs completion path. The core always takes
the data fork and substitutes decmpfs content only when the data fork is empty. The
two predicates disagree on two populations: (a) files with the bit set that are not
decmpfs-compressed (og emits rsrc-fork bytes; core emits data-fork bytes), and (b)
compressed files with a non-empty data fork (og takes the marked/rsrc path; core
takes the plain data fork). The mechanical predicate is decompile-quoted; which
documented field the byte corresponds to is UNVERIFIED (see Open questions).

## What the reference does

The quoted branch (`FUN_180009940`):

```c
if ((*(byte*)((longlong)param_6 + 0x29) & 0x20) == 0) {   // :7665
    fork = dataFork; size = logicalSize;                  // :7669
    if (size == 0) size = totalBlocks << 12;
} else {
    node+0xce |= 4;                                       // :7677 mark
    fork = resourceFork;                                  // :7678
}
```

The og emits exactly ONE fork per file. Bit 0x20 of the record byte at +0x29 selects
the resource fork and sets the decmpfs-lookup mark. Decmpfs mode-4 payloads genuinely
live in the resource fork (64-KiB-chunked zlib — TSK/mac_apt layout), so for ordinary
compressed files the marked/rsrc path is the "compressed" path; the completion site
(`:7840-7843`) then sets `node->size = *(u64*)(value+8)` from the decmpfs attribute's
uncompressed size.

## What our core does

`cr_hfsplus_engine.cpp:693-733`: the data fork is always the node content;
when the data fork is empty the engine looks up `com.apple.decmpfs` and substitutes
the decompressed content (modes 3/4, per the S-002 fix). There is no resource-fork
emission for marked files and no +0x29 predicate.

## Gap

Falsifiable: for a catalog file record with byte +0x29 bit 0x20 set and a non-empty
data fork, the og recovers the resource fork's bytes and the core recovers the data
fork's bytes — different content for the same file. For a compressed (decmpfs) file
with a non-empty data fork (rare; compressed files normally have empty data forks per
TSK), the og takes the rsrc/decmpfs path and the core takes the plain data fork.

## Impact

Bounded: population (a) — files carrying that flag without decmpfs — is narrow on
modern volumes, and population (b) is rare. Where it fires, the recovered BYTES
differ (wrong-fork content), which is a per-file correctness issue, not a volume-wide
one. Fork visibility policy (rsrc as `..namedfork/rsrc` child nodes) is the separate
N-015b superset item.

## Reproduction / verification

Decompiled (strength 2): the branch is quoted verbatim with line cites; the
completion-site interplay is cited. Core side by code read. NOT execution-verified:
needs one real HFS+ volume with a decmpfs-compressed file (or a synthetic catalog
record with +0x29 bit 5 set) to demonstrate the byte divergence — see tests.

## Fix plan (for the engineering team)

1. In `emit_file` (the FUN_180009940 counterpart), read the same byte (`record+0x29`)
   and, when bit 0x20 is set: prefer the resource fork as the node's content fork and
   mark the node for the decmpfs completion path (size from the attribute).
2. Keep the empty-data-fork substitution as the fallback predicate (it covers
   compressed files whose record bit is clear) — i.e. accept either signal for the
   decmpfs path, but let the record bit win for fork selection when set.
3. Register the divergence in `docs/parity/PARITY-DELTAS.md` next to N-015b (rsrc
   child-node superset) so bench comparisons filter marked files.
4. Tests: `HfsPlus.RecordBit29SelectsResourceFork` (synthetic record with the bit set
   + non-empty data fork → rsrc bytes emitted); `HfsPlus.MarkedNodeGetsDecmpfsSize`
   (bit set + decmpfs attribute → size from uncompressed-size field).

## Tests to add

See fix plan §4.

## Related findings

- N-028 (N-015b: the rsrc-child-node superset — different layer: emission shape vs
  content choice; this finding refines N-015b's "resource fork referenced solely
  inside the UF_COMPRESSED branch" note with the exact predicate and its two
  disagreement populations).
- S-002 (FIXED — the decmpfs modes 3/4 content recovery; this finding is the
  fork-selection gate that routes into it).
- S-021 (the journal harvester's emit path reuses FUN_180009940 — the same predicate
  applies to harvested deleted records).

## Open questions

The byte at +0x29's documented NAME is unresolved: this leg's decode labels it
`ownerFlags` (classic-Mac kHFSInvisibleBit at bit 5) with `UF_COMPRESSED` in a flags
u16 at +0x2A, but TN1150's permissions-block layout does not unambiguously confirm
the byte offsets, and macOS decmpfs literature (TSK, mac_apt, decmpfs.c) documents
UF_COMPRESSED as a BSD `st_flags` bit without pinning its HFS+ catalog byte. The
mechanical predicate (+0x29 bit 0x20) is decompile-quoted regardless; one real
compressed volume (istat/`hfsdebug` dump of a decmpfs file's catalog record) settles
which field it is and whether the og's read is a bug or the intended location.
