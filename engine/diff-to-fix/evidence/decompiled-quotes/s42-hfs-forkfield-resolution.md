# S42 — the HFS+ fork-selection field-name resolution (N-038's UNVERIFIED residual CLOSED) + the decmpfs completion residual CLOSED + a new decode-methodology datum (the ushort-units class)

**Lane:** S42 (session-42 continuation audit) · **Tree:** core `869da779`-lineage (lane tip `d4776278`) · **Corpus:** `be0cfe9`.
**Method:** first-hand decompile read — `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c`.

## 1. The resolution: +0x29 IS `ownerFlags` (TN1150 byte-exact)

The N-038 residual question ("field-name UNVERIFIED — TN1150 layout ambiguity vs decmpfs
literature; one real volume settles") is settled **by decode, no volume needed**.

### 1.1 The ushort-units discovery (why the layout ever looked ambiguous)

`FUN_180009940`'s record parameter is declared **`short *param_6`** — so every
`param_6 + N` read is in **USHORT units (×2)**, while every `(longlong)param_6 + N` read
(cast first) is in **BYTES**. The decompile mixes both forms in the same branch; reading
them all as byte offsets produced the apparent layout mismatch the trail recorded. With the
units separated, all ten reads map to the TN1150 `HFSPlusCatalogFile` layout exactly:

| og read (decompile form) | units | byte offset | TN1150 field | semantics |
|---|---|---|---|---|
| `*param_6` (u16) :7660 | ushort[0] | **+0x00** | `recordType` | == 2 → file record |
| `*(uint*)(param_6 + 4)` :7661 | ushort[4] | **+0x08** | `fileID` | the node's CNID (`param_1[10]`; −1 = failure per both callers) |
| `*(uint*)(param_6 + 6)` :7679 | ushort[6] | **+0x0C** | `createDate` | → node+0xC0 |
| `*(uint*)(param_6 + 8)` :7680 | ushort[8] | **+0x10** | `contentModifyDate` | → node+0xC4 |
| `*(uint*)(param_6 + 0xc)` :7681 | ushort[0xC] | **+0x18** | `accessDate` | → node+0xC8 |
| `*(int*)(param_6 + 0x18)` :7686 | ushort[0x18] | **+0x30** | `userInfo[0]` (FinderInfo fileType) | == 0x66647270 `'prdf'` → +0xce \|= 2 (the private-data folder mark) |
| `*(byte*)((longlong)param_6 + 0x29)` :7665 | **byte** | **+0x29** | `permissions.fileModes` byte 1 = **`ownerFlags`** | bit **0x20** → the fork-selection predicate |
| `*(u64*)(param_6 + 0x2c)` :7667 | ushort[0x2C] | **+0x58** | `dataFork.logicalSize` | the content size (bit clear path) |
| `*(uint*)(param_6 + 0x32)` :7669 | ushort[0x32] | **+0x64** | `dataFork.totalBlocks` | **<< 0xC (×4096)** — the zero-logical-size fallback |
| `*(u64*)(param_6 + 0x54)` :7677 | ushort[0x54] | **+0xA8** | `resourceFork.logicalSize` | the content size (bit set path) |

The permissions block (`HFSPlusPermissions`, 16 bytes at +0x20..+0x2F) is
`{ownerUserID u32 @+0x20; ownerGroupID u32 @+0x24; fileModes u32 @+0x28; special u32 @+0x2C}`
with `fileModes = adminFlags(1) @+0x28 | ownerFlags(1) @+0x29 | posixMode(2) @+0x2A`.
**+0x29 is the `ownerFlags` byte; the predicate bit is 0x20** — the TN1150 layout wins over
the "layout ambiguity" reading; the decmpfs literature's ownerFlags-bit marker and the
record layout now agree.

### 1.2 Independent corroboration inside the same binary

`FUN_18000f7b0` (the engine's debug-print helper, :12355-12395) decodes the SAME record
buffer with the og's own labels: `Type @ ushort[0]` (=byte 0), `FolderID/FileID @
ushort[4]` (=byte 8), `FileSize @ ushort[0x2C]` (=byte 0x58 = `dataFork.logicalSize`),
and for thread records (types 3/4): `ParentID @ ushort[2]` (=byte +4), name length
`ushort[4]` (=byte +8), name `ushort[5]` (=byte +0xA) — the TN1150 thread-record layout,
byte-exact. Two independent functions, one layout.

### 1.3 The record-source note (the "both record sources" claim verified)

The LIVE walk passes record copies built by `CHFSPlusCatalog::ReadKey`/`ReadData`
(the B-tree virtual getters at vtable+8/+0x18, called at :15620/:15625 — the copies are
allocated per record and freed after `FUN_180009770`). The JOURNAL walk
(`FUN_180011300` → `FUN_18000fe90`) copies the same shape: **8 × u64 = 0x40 bytes** from
the raw node slot at `keyStart + 0xC` onward (with the key's own 12-byte normalized
header from `FUN_18000fbe0`) — wait, see the erratum below.

**Erratum on the journal-path copy base (corrected this session):**
`FUN_18000fe90` (:12665) copies from `node[keyStart + 0xC .. +0x4C]`, but the record DATA
begins at `keyStart + 8 + 2×nameLen` — the +0xC base is only record-aligned for 2-character
names. The LIVE path's `ReadData` getter (the authoritative source) produces the
TN1150-aligned 0x40-byte struct the table above decodes. The journal path's fixed-base copy
is a **name-length-dependent misalignment in the og itself** (the og's journal-recovered
records for names ≠ 2 chars read fields shifted by `4 − 2×nameLen` bytes — an og-side
approximation/defect, NOT a core parity obligation; the core's journal walk reads the true
record layout). Recorded here as a decode datum; no core finding (the core must NOT
replicate an og bug — registered as deliberate non-parity, the D-row class).

## 2. The decmpfs completion residual CLOSED (core-superset CONFIRMED)

The module's open question ("§5's decmpfs completion site reads only the size
(`node->size = *(u64*)(value+8)` :7840-7843) with NO mode-3/4 branch — mode-3 inline
payloads may not be wired to content at this site... next artifact: the og's marked-node
content path re-read at :7880+") — **read this session, first-hand**:

```c
/* :7835-7845 (FUN_180009940's decmpfs completion tail) */
if (*local_258 == 0x10) {                    /* the attribute record's format gate */
    uVar8 = local_258[3];                    /* the payload length */
    _Dst = (void *)thunk_FUN_180018ba0(uVar15);
    memcpy(_Dst, local_258 + 4, (ulonglong)uVar8);
} else {
    fprintf(stderr, "unsupported attribute node format\n");   /* the only fallback */
    _Dst = (void *)0x0;
}
...
if (uVar15 != 0) {
    param_1[0x17] = *(longlong *)((longlong)_Dst + 8);   /* node->size = *(u64*)(value+8) */
    free(_Dst);
    goto LAB_180009f14;                                   /* RETURN — no content wiring */
}
```

The og's completion path copies the attribute payload, sets **only the node's size** from
the decmpfs header's uncompressed-size field (value+8), frees the payload, and returns.
There is **no mode-3 inline-content wiring and no mode-4 rsrc-offset branch anywhere in the
SearchDLL's node emission** — the og's recovered output for a compressed file is the RAW
resource-fork bytes (mode 4, via the fork-selection path) with the UNCOMPRESSED size, or a
size-with-no-extents file (mode 3, via the skip-resolver path). The core (S-002's fix)
wires actual decompressed content (kZlib codec extents, writer-side inflation). **Verdict:
core-superset, deliberate improvement — the residual is closed; no finding.**

## 3. The one NEW divergence the resolution surfaced

The bit-clear (normal-file) path's zero-size fallback:
`size = dataFork.totalBlocks << 0xC` (blocks × 4096, hardcoded — NOT the volume's
allocation-block size) when `dataFork.logicalSize == 0`. The core
(`cr_hfsplus_engine.cpp:1455` `props->logical_size = content_fork.logical_size`) applies
no fallback — a record with logicalSize==0 and totalBlocks>0 emits size 0 in the core,
blocks×4096 in the og. Filed as **N-155** (the og's fallback also mis-sizes on
non-4096-blocksize volumes — the core should implement the og's observable while noting
the block-size quirk in the test).

## 4. The I-014 allocation-bitmap residual (scoped, still open)

The freed-node grading question ("does the og grade journal-recovered nodes against the
allocation bitmap?") — the bitmap-test idiom sweep over the decompile found no
grade-consumer (the `>> 3` sites are pointer-array destruction and byte-span math; the
allocation file's consumers in the freed path remain the address-mapping use the S11 lane
cited). The residual stays **core-probable-superset, UNVERIFIED**, with the sharpened next
probe: walk every consumer of the volume's allocation-file object (the `param_1+0x160`
extent-map family, `FUN_180013de0`'s callers) for a bit-test idiom (`(byte[block >> 3] >>
(block & 7)) & 1`) — one pass settles it.
