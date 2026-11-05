# s10-refs-population-site.md — first-hand read: the ReFS population site contradicts part2's [INFERRED] anchor reading (erratum E69)

**Session 10 (2026-10-29). Corpus `be0cfe9`. Source read verbatim:
`re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c:2090-2140`.**

## Context

`artifacts/SaveScan/ifileproperties-part2--ext4-hfs-refs.md:99-106` (the ReFS serialized
grammar table, [CODE]-backed) lists the per-record fields as: name (u16 len), size qword
(+0x11), **u32 +0x39 "objectId / parent anchor [INFERRED from field position + ReFS
semantics]"**, created/modified/accessed qwords (+0x19/+0x21/+0x29), byte marker (+0x3D),
hasRun+DataRunList. The table has **no FLAGS u16 row** — yet part2 §4 (:114-116) claims
the persistent field set is "invariant" across all six classes *including* `FLAGS(u16)`.
The corpus contradicts itself (table vs synthesis), and the core persists `flags`
unconditionally (`cr_records.cpp:192`), so the resolution matters for future parity work.

## The population site (read this session)

`ReFSSearchDLL_decompiled.c:2105-2135` — the `refs::RefsFileProperties` construction
(vftable assigned at :2105):

```c
*plVar3 = (longlong)refs::RefsFileProperties::vftable;
...
*(undefined8 *)((longlong)plVar3 + 0x11) = local_b8;        /* size        */
*(undefined8 *)((longlong)plVar3 + 0x19) = local_b0;        /* created     */
*(undefined8 *)((longlong)plVar3 + 0x21) = local_a8;        /* modified    */
*(undefined8 *)((longlong)plVar3 + 0x29) = local_a0;        /* accessed    */
*(undefined8 *)((longlong)plVar3 + 0x31) = local_98;        /* (unnamed 4th qword) */
uVar7 = (uint)local_90 | 0x10;
*(uint *)((longlong)plVar3 + 0x39) = uVar7;                 /* FLAG-BUNDLE dword, | 0x10 */
*(undefined2 *)((longlong)plVar3 + 0x3d) = local_90._4_2_;  /* u16 marker from same bundle */
*(undefined1 *)((longlong)plVar3 + 0x3f) = local_90._6_1_;
*(undefined1 *)(plVar3 + 8) = local_90._7_1_;
```

Three observations:

1. **The +0x39 u32 is flag-bundle-derived**, not an objectId: it is `local_90 | 0x10`
   where the SAME `local_90` bundle feeds the marker at +0x3d (u16), +0x3f, and +0x40.
   An objectId would come from the parse result, not from a flags local OR'ed with the
   has-children bit (0x10). part2's "[INFERRED] objectId/parent anchor" reading of +0x39
   is therefore **doubtful**; the neighboring FAT class carries its FLAGS-analog in
   exactly this position family.
2. **The marker at +0x3d is a u16, not a byte** (`local_90._4_2_` — a 2-byte store;
   the getter/setter pair at :14402/:14411 is `*(undefined2 *)(param_1 + 0x3d)`).
   part2's table says "byte +0x3D".
3. **A +0x31 qword exists in-memory** (local_98) that part2's table does not list at all
   (may be unserialized; the serializer subset is not established by this read alone).

## What this does and does not settle

- **Does NOT settle:** whether the SERIALIZED grammar includes a FLAGS u16 for ReFS (the
  serializer is the vtable +0x98 method; tracing it past the wrapper is the remaining
  work — the reader `FUN_1405f52e0` cited by part2 is the DRWUI-side consumer, not the
  engine serializer).
- **DOES establish:** the in-memory +0x39 is flag-like (weakening the [INFERRED] anchor
  reading), the marker is u16, and part2 §4's "invariant incl. FLAGS" synthesis
  over-generalizes beyond its own [CODE] table.

## Disposition

Filed as **CORPUS-ERRATA E69** (corpus-internal contradiction + the refuted-adjacent
[INFERRED] reading). No core defect: the core persists `flags` unconditionally
(cr_records.cpp:192) which is a superset under either reading, and `record_ref`
(cr_records.cpp:183) carries the ReFS object id (refs_engine.cpp:1111) — so the core is
safe under both resolutions. The open question (the ReFS serializer trace) is recorded in
the erratum for the next corpus-decode pass.
