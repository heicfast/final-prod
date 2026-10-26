# Evidence — ReFSSearchDLL file pipeline decode (session 3)

All quotes from `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c`
(corpus `03ad46c`) unless noted; binary facts from `og-codebase/ReFSSearchDLL.dll`
(x64, imagebase 0x180000000). Read first-hand this session.

## 1. RTTI census (binary strings, first-hand extraction)

The `refs::` class family in the binary: `RefsPartitionInformation`,
`RefsQuickSearchFiles(Impl)`, `RefsSearchController`, `RefsNodeRecord(Impl)`,
`RefsNodeHeader(Impl)`, `RefsMSTreeHeaderDesc(Impl)`, `RefsFileProperties`,
`RefsFileValues(Impl)`, `RefsDirectoryValues(Impl)`, `RefsSupperBlock(Impl)`
(vendor typo), `RefsDbr(Impl)`, `RefsCheckPoint(Impl)`,
`RefsMetadataBlock{Header,Reference,RefrenceImpl}`, `RefsBaseDataImplManager<T>`
(four instantiations), `CReFSDataRunList`, `CRefsResidentDataRunList`,
`CNoneResidentNTFSDataRunList`, `IDataRunList`,
`IQuickFileSystemSearchController`. **No stream/named-stream class.**
Exports (rabin2): `EUCreateReFSQuickSearcher` @0x180001100,
`EUCreateReFSSearcher` @0x180001180.

## 2. Row-type discriminators

```c
/* :8096 */ undefined2 FUN_18000b820(longlong param_1) {
    puVar1 = *(undefined2 **)(*(longlong *)(param_1 + 8) + 0x18);   /* key */
    if (key_end - key < 4) return 0;
    return *puVar1;                          /* *(u16*)key  */
}
/* :8113 */ FUN_18000b840: return *(undefined2 *)(key + 2);         /* subtype */
```

Directory-table dispatch (:1469-1494): type `0x10` → dir descriptor
(`FUN_18000cbd0`); type `0x30` + subtype@key+2 `1` → file row
(`FUN_18000ba90`), `2` → dir link (`FUN_18000c920`). (The core's
`ROW_FILE = 0x00010030` / `ROW_DIR_LINK = 0x00020030` u32 markers are the
same {type@+0, subtype@+2} packing. ✓)

## 3. File-row handler FUN_18000ba90 (:8233)

One properties block (`local_e8`..`local_76`, field-mirroring
`RefsFileProperties`), one runlist (`local_7e`), one emission
(`FUN_18000d970` — after `FUN_18000e020` validation). The value-row loop
(:8290-8355):

```c
uVar16 = *(u16*)key;                       /* the row type, at key+0 */
if (uVar16 == 0x178)      local_c0 |= 2;   /* property-flag bit 2 */
else if (key_size == 0x28 && uVar16 >= 0xb1 && param_1[10] >= 8)
                          ;                 /* modern layout -> FUN_18000c560 */
else if (uVar16 == 0x108) local_c0 |= 1;
else if (uVar16 == 0xf8)  local_c0 |= 3;
/* size-0x10 + type 100 + name[0..1] == '$','I'  -> FUN_18000ea30 (recycle) */
/* size-0x10 + local_f8 == 1                      -> FUN_18000e490          */
else                      FUN_18000c560(...);  /* default value handler */
```

`FUN_18000c560` (:8761): **old layout** (`param_1[10] < 8`): requires type
== 0x178 exactly; **modern** (`param_1[10] >= 8`): requires keylen == 0x28
and type >= 0x29. Then iterates the value's entries
(`(flags@entry+10 & 0x18) == 0x18` → `FUN_18000c790`; else wrap
`RefsNodeRecord` → `FUN_18000cf30` recursion — the runlist build).

## 4. The `param_1[10] >= 8` gate is the system-directory list size

`param_1` = `RefsPartitionInformation` (vftable at `FUN_180001500` ctor :240;
ctor sets `param_1[0xb] = 0x200` = sector size and zeroes `param_1[10]`).
The DBR parse (:7890-7960) reads the offset table at
`boot + *(u16*)(boot+10)` and appends its entries to the `param_1[9]` list
**four per loop iteration**, each append doing
`param_1[10] = param_1[10] + 1` under the `std::_Xlength_error("list<T> too
long")` guard (:7918/:7931/:7944/:7957). So `param_1[10] >= 8` ⇔ the DBR's
offset table declared >= 8 system directories — a structural ReFS-generation
discriminator (3.x-era volumes carry more system-identity dirs), not a
version-number read.

## 5. .rsf ReFS file record (corpus artifact, 100% [CODE])

`SaveScan/ifileproperties-part2--ext4-hfs-refs.md` §3: RefsFileProperties
(type 6) persists `{u16 name-len + bytes, qword size, u32 objectId,
qword created, qword modified, qword accessed, byte marker,
u32 hasRun + DataRunList}` — one name, one size, ONE runlist; serializer
`FUN_1405f5190`, reader `FUN_1405f5190`-mirror with a 256-byte name cap.
No stream field exists in the session format.

## 6. Core-side citations (audited commit 34b3441)

`src/core/refs/cr_refs_engine.cpp` :282-284 (`attr_type_of = le16(key+8)`),
:39 (`ATTR_DATA = 0x80`), :738-760 (first-0x80-row-wins + `break` at :759),
:33-36 (row markers — og-consistent), no recycle/$I handler
(grep `recycle|\$I` over `src/core/refs/`: zero hits), no system-dir-count
gate.

## 7. Quick-search bootstrap (context)

`FUN_18000ba90`-adjacent bootstrap (in the quick path, :2050-2200): root
directory id `0x600` (`plVar4[10] = 0x600` — matches the core's
`ROOT_DIR_TABLE = 0x600` ✓), volume label capped at 400 chars, fs-kind 10
via vt+0x40. The quick searcher also has a dot-extension prefilter
(`memchr(name, '.')` loop :8271-8287) whose no-dot branch was not decoded to
a verdict (M-022 Open question 3).
