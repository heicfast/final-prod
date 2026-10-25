# s3-hfsplus-inventory — HFSPlusSearchDLL function/feature inventory (S3-EHB, session 3)

Reference: `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c`
(22,304 lines). The embedded hfspluslib source-tree is recoverable from
`__FILE__` trace strings (census: hfsplusbtree.cpp ×60, hfsplusvolume.cpp ×32,
hfsplusbtreeoption.cpp ×24, hfspluscatalog.cpp ×16, hfsplusattribute.cpp ×16,
hfsplusextents.cpp ×12, hfsplusjournal.cpp ×8, hfsplusrawfile.cpp ×4,
hfsplusbtnode.cpp ×4). Core: `src/core/hfsplus/cr_hfsplus_engine.cpp` (946 LOC)
@ `34b3441`. All cites are HFSPlusSearchDLL_decompiled.c line numbers.

## 0. Method-name map (from `\tFuction:` [sic] traces)

| Class::method | Site |
|---|---|
| `CHFSPlusVolume::CHFSPlusVolume` | :9561 |
| `CHFSPlusCatalog::ReadKey` / `ReadData` | :12063 / :12331 |
| `CHFSPlusExtents::ReadKey` / `ReadData` | :12640 / :12756 |
| `CHFSPlusAttribute::ReadKey` / `ReadData` | :11156 / :11470 |
| `CHFSPlusJournal::GetDeleteRecord` | :14235 (body :13637-14252) |
| `CHFSPlusBTNode::TestNodeBelongToFile` | :13036 |
| `CHFSPlusBTree::Search/SearchNode/Traverse/TraverseNode` | :14895-15885 |
| `CHFSPlusBTreeOptionBase::CreateNode/GetChild` | :14625 / :14772 |
| `CHFSPlusRawfile::Read` | :13306 |

## 1. Volume header (re-verified)

Gate :10005-10006 first-hand: `(sig==0x482B || sig==0x4858) && (blockSize &
0x1FF)==0 && version<0x11 && allocFileBlocks<=totalBlocks &&
extentsFileBlocks<=totalBlocks` — N-005 filed (core adds 512..65536 bounds).
All five VH forks are byte-swapped and parsed: allocation(+0x70), extents
(+0xC0), catalog(+0x110), attributes(+0x160), **startup(+0x1B0)**
(:10499-10503). Core parses four (cr_hfsplus_engine.cpp:544-547) — the startup
fork is used by the og only for journal-image tree identification (§3), not
read as a tree.

## 2. Catalog record decode (FUN_180009940 :7588-7708) — fork selection

File record (type 2) fields: fileID@+0x08 (:7663), createDate@+0x0C,
contentMod@+0x10, accessDate@+0x18 (:7654-7656, :7682-7684 — 3 dates,
attrMod never read here → I-007 already filed), dataFork@+0x58
(:7666 `param_6+0x2c` shorts), resourceFork@+0xA8 (:7678 `param_6+0x54`).

**The og emits exactly ONE fork per file, chosen by:**
```
if ((*(byte*)((longlong)param_6 + 0x29) & 0x20) == 0) {   // :7665
    fork = dataFork; size = logicalSize; if (size==0) size = totalBlocks << 12  // :7669
} else {
    node+0xce |= 4;                                        // :7677 mark
    fork = resourceFork;                                    // :7678
}
```
Record byte +0x29 = **HFSPlusPermissions.ownerFlags** (permissions owner
@0x20, group@0x24, adminFlags@0x28, ownerFlags@0x29, BSD flags u16 LE @0x2A,
special@0x2C). Bit 0x20 of ownerFlags = `kHFSInvisibleBit` (classic-Mac owner
flags bit 5); the documented decmpfs marker `UF_COMPRESSED` (0x20) lives in the
**flags u16 at +0x2A** — the og reads the byte one before it. Mechanical
consequence: files with ownerFlags bit 5 set get their **resource fork** as
node content (data fork dropped) and get marked for the decmpfs lookup;
ordinary compressed files (UF_COMPRESSED in flags@0x2A, ownerFlags typically
0) take the data fork and are never marked. The core picks the data fork
always, substituting decmpfs content when the data fork is empty
(cr_hfsplus_engine.cpp:693-733). → I-013 (detection-predicate + fork-surface
superset), with the ownerFlags/invisible population noted as the only
non-superset overlap.

**No `..namedfork/rsrc` child emission exists anywhere in the og** — the
node keeps one runlist (FUN_180009f40 :7864+, HFSPlusFileDataRun) and there is
no second node per fork. The deep-dive §4 "resource forks named
`..namedfork/rsrc` conventionally in recovery products" is artifact
speculation, not decompile evidence. Core emits rsrc children by default
(cr_hfsplus_engine.h:50). → I-013 / module B7 correction.

Zero-length fallback: `if (logicalSize==0) size = totalBlocks << 0xC` (:7669)
— **hardcodes 4096** as the block size for empty-fork size estimation (legal
block sizes are 512..65536). Corrupt-record quirk; core uses the real
`logical_size` (cr_hfsplus_engine.cpp:677).

Special-folder magic: `"fdrp"` at record+0x30 (userInfo, :7687) marks the node
(bit 2 at +0xce, :7688). No core counterpart ( Finder metadata not decoded by
either side beyond this).

## 3. Journal — CHFSPlusJournal::GetDeleteRecord (:13637-14252) — NEW decode

The og's journal machinery has a pass S-003's addendum did not cover: a
**deleted-catalog-record harvester**. Mechanically verified:

1. Starts at the journal start offset (`*(u64*)(jinfo+0x10)`, trace
   "Journal Start Offset:0x" :13709-13713); allocates max(blocksz-ish,
   `jinfo+0x28`, `jinfo+0x20`) (:13698-13702).
2. Ring walk: reads a block; if `first u32 == (u32@(jinfo+0x20) >> 4) - 1`
   (:13737) treats it as a transaction block-list header, entry list at
   block+0x20 (`local_d0 = local_c8 + 0x10` :13748), entries `{?, image_size
   u32@+4, target_block u16@+8}`; per entry: allocate `image_size`, read it
   from the ring with wrap `% journal_size (volume+0x2c)` (:13779-13782) —
   same ring arithmetic S-003 recorded.
3. Each image passes the **B-tree node-kind gate** `FUN_180010720`
   (:13784) and is installed into a scratch btree via `FUN_180010110`
   (:13789, 4th arg 1).
4. **Only installed nodes with kind==kBTLeafNode (-1)** proceed
   (`*(char*)(image+8) == -1` :13799).
5. Target block → owning tree by address range, tried in order
   **catalog(VH+0x110) → extents(+0xC0) → attributes(+0x160) →
   allocation(+0x70) → startup(+0x1B0)** (:13809-13817, `FUN_18000cb70`
   :10133-10202 walks the 8 initial fork extents and chases extents-overflow
   records — key built as {10, cnid} :10184-10187).
6. **Catalog branch** (tree==4, :13939-14130): iterate the leaf's records
   (`FUN_18000ef30` data / `FUN_18000f2d0` key :13948-13952), parse, and
   insert {key → record} into the caller's map (+0x58) unless the record's
   own cnid equals the key's parentCNID (:13961-13962); HFSX vs HFS+ key
   collation switches on a `-0x31` byte (:13970-13978). **Extents branch**
   (tree==3, :13837-13937): same for extents-overflow records into map (+0x68).
   Progress ticks per record (:14020-14026).
7. **Emit driver FUN_18000c8f0** (:10017-10127): calls GetDeleteRecord, then
   for each harvested catalog {key,record} builds a `CHFSPlusFileNode`
   (`FUN_180009770`, 0xD0 bytes, :10066-10069) and inserts it into the result
   tree (controller vtable 0x28, kind 5 — the deleted branch, :10081-10089);
   progress total = `journal_count + 1 + folderCount(VH+0x24) +
   fileCount(VH+0x20)` (:10037-10041). Records whose node kind comes back -1
   are destroyed (:10077-10079).

**Core has none of this** — `journal_info_block` is parsed and unused
(cr_hfsplus_engine.cpp:525; grep "journal" → 1 hit). The og's primary
deleted-file surface for HFS+ is this journal harvest (+ RAW carving);
the core has only the freed-node sweep (cr_hfsplus_engine.cpp:826-938).
→ **S-017**. It also answers module note "B OQ-3" (og cnid-sweep mechanics):
there is no on-disk freed-node sweep decode in the og; deleted records come
from journal images keyed by catalog key.

## 4. Journal replay (S-003 scope — cheap verification only)

S-003's addendum facts re-spotted in passing: info-block gate magic
0x4A4E4C78 (:13542-13554 area), ring wrap via volume+0x2c (:13780-13782,
:14194-14196), per-entry cancel (:13758-13766). No re-decode attempted.
GetDeleteRecord is a **separate consumer** of the same ring walk ( harvesting
rather than replay) — S-003's fix plan should share the walker with S-017's.

## 5. Attributes B-tree (S-002 scope — remaining modes/types)

- Inline gate `recordType == 0x10` (:7825); non-0x10 → trace
  "unsupported attribute node format" (:7833) — parity with the core's
  `++unsupported` (cr_hfsplus_engine.cpp:433-437) ✓.
- decmpfs completion (:7840-7843): gate `uVar15 = length(local_258[3]) != 0`,
  then `node->size = *(u64*)(value+8)` (uncompressedSize low dword; single
  native load). **No mode branch (3 vs 4) at this site** — the content runlist
  was already fixed to the fork chosen in §2 (resource fork for marked nodes);
  mode-3 payloads (inline in the attribute) are not wired to content here.
- Modes 1/2: neither side handles (core gates mode 3|4,
  cr_hfsplus_engine.cpp:698; og reads only the size) → MATCH-by-omission.
- "com.apple.decmpfs" name spelled from a char table at 0x18001f480
  (:7759-7769) — constant-string, no runtime compare loop for the name
  itself.

## 6. Names / Unicode (B10)

`FUN_180009630` (:7480-7510): `WideCharToMultiByte(CP_UTF8 0xFDE9, flags 0)`
with **'_' default-char fallback** (:7504-7507) — UTF-16BE→host→UTF-8; no
NFC/NFD normalization (NFD preserved). Core: raw `be16()` units
(cr_hfsplus_engine.cpp:233-234), no normalization. MATCH (both preserve NFD;
edge: unpaired surrogates → og '_', core passes the raw unit through — noted,
not filed). Name length capped 255 (:7630-7632 `0xff`).

## 7. Hard links, hot files, allocation file

- **Hard links**: no `0xF0000000` / cnid-15 handling anywhere — hard-link
  catalog records are ordinary records on both sides (module open note
  stands; MATCH-note).
- **Hot-file B-tree**: not walked. The only startup-file (CNID 7) use is the
  journal-image address-range mapping (:13817). Core: no hotfile code either.
  MATCH-by-omission.
- **Allocation file**: parsed (:10499) and used by GetDeleteRecord only for
  journal-image address mapping (:13815). **No allocation-bitmap grading of
  deleted records is visible anywhere in the og** — the core grades freed-node
  records against the allocation bitmap (cr_hfsplus_engine.cpp:846-934).
  Possible core superset; og-side grading cannot be excluded from unexecuted
  paths → scratch UNVERIFIED residual (low impact: grading affects confidence
  only).

## 8. Catalog leaf walk (A26 — re-verified M)

Live walk iterates leaf records with per-record key/data extraction
(FUN_18000fbe0/FUN_18000fe90 for the journal-scratch tree; catalog ReadKey/
ReadData traces), dedup map keyed by {parentCNID, cnid, name} (:2432-2450 —
byte+2, u32+4, u32 comparisons), record-kind dispatch by tree id 3/4/8
(:2404-2524). Sibling-chain/leaf-first/last geometry per S-002/session-1
verdicts — unchanged.
