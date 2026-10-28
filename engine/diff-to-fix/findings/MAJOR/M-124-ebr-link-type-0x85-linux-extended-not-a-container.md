# MAJOR M-124 — EBR link type 0x85 (Linux extended) is not a container in the core: a whole extended chain's logical volumes vanish from the table pass

**Severity:** MAJOR
**Module:** table (MBR/EBR parse)
**Parameters affected:** detection correctness (1), recovery completeness, tree walk logic (26)
**Corpus references:** `ghidra-mo/Partition_decompiled.c:983-1014` (the EBR chain walk: `'\x05' || '\x0f'` links at :991; `'B'` 0x42 exits to the LDM path at :1001-1004; **`-0x7b` = 0x85 → the SAME link path** at :1006)
**Core references:** `src/core/table/cr_table_parse.cpp:115-146` (`mbr_type_to_kind` — only 0x05/0x0F → Extended), `:203-211` (the non-extended emission path a 0x85 link takes instead); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's EBR chain walker treats link entries of type {0x05, 0x0F, **0x85**} identically
— follow `next = base + rel` and continue the chain. The core's `mbr_type_to_kind` maps
only 0x05/0x0F to Extended; a 0x85 link (the Linux-extended convention stamped by
fdisk and dual-boot installers) falls to `FsKind::Other`, is emitted as a bogus VOLUME at
`walk + rel` (the wrong basis — links are base-relative, volumes EBR-relative), and the
chain stops: every later logical volume in that container is invisible to the table pass.

## What the reference does

`Partition_decompiled.c` (ghidra-mo) :989-1010:

```c
989    for (local_120 = 0; local_120 < 4; local_120 = local_120 + 1) {
990      local_128 = *(char *)(local_118 + 4);              /* the type byte */
991      if ((local_128 == '\x05') || (local_128 == '\x0f')) {
992  LAB_180002348:
993          local_110 = local_100 + (ulonglong)*(uint *)(local_118 + 8);  /* next = base + rel */
994          if (local_134 != '\0') {
995              local_100 = local_100 + (ulonglong)*(uint *)(local_118 + 8);
996              local_134 = '\0';
997          }
998          local_137 = '\x01';
999      }
1000      else {
1001          if (local_128 == 'B') {                        /* 0x42: the LDM path */
1002              local_136 = '\x01'; local_138 = '\x01';
1003              goto LAB_1800023b6;
1004          }
1006          if (local_128 == -0x7b) goto LAB_180002348;    /* 0x85: SAME link path */
```

`-0x7b` is the decompiler's signed-char rendering of 0x85 — it jumps to the identical
link handling as 0x05/0x0F.

## What our core does

`cr_table_parse.cpp:115-146`:

```c
115  FsKind mbr_type_to_kind(u8 type) {
116      switch (type) {
117      case 0x05:
118      case 0x0F:
119          return FsKind::Extended;
...
144      /* default: */
145      return FsKind::Other;
```

0x85 hits the default → in the EBR walk the link entry is not recognized, so it is
emitted as a volume at `walk + rel` (:203-211) with the link's sector_count, and the
chain terminates at that EBR. In the MBR itself, a 0x85 extended container is emitted as
one opaque Other-volume spanning the whole container.

## Gap

A disk whose extended-partition entries are stamped 0x85: og → the full logical-volume
chain walks; core → the first EBR's volume entry surfaces (if any) plus one bogus
"Other" volume at the link target; all subsequent logical volumes are absent from the
table pass (only findable via lost-partition content scans).

## Impact

Whole-chain loss on a real-world population (Linux fdisk, many dual-boot disks): recovery
completeness for those containers drops to whatever the lost-partition heuristic finds —
slower, less graded, and mis-basis'd (the bogus volume at `walk+rel` can shadow the real
second-EBR region). Users on such disks see most of their logical volumes missing.

## Reproduction / verification

Verified first-hand this session (lead auditor): the og walk read directly at
Partition_decompiled.c(ghidra-mo):989-1010 (quote above; `-0x7b` → LAB_180002348 — the
same label 0x05/0x0F use); the core read directly at cr_table_parse.cpp:115-146. Grep
for 0x85/"Linux extended" across diff-to-fix + PARITY-DELTAS: zero hits (never covered).

## Fix plan (for the engineering team)

1. Add `case 0x85:` to the Extended arm of `mbr_type_to_kind` (cr_table_parse.cpp:117).
   Keep 0x42 on the LDM path (unchanged). Consider also 0x93/0x93-amoeba variants ONLY if
   a corpus cite exists — none does; 0x85 is the decoded set.
2. File: `src/core/table/cr_table_parse.cpp` (one case label).
3. Tests: below (MBR-level 0x85 container + EBR-level 0x85 link chain).
4. Doc: table module rows in CROSS-REF-MATRIX.
5. Acceptance criteria: a 3-EBR 0x85 chain yields all 3 logical volumes with correct
   bases; no regression on 0x05/0x0F chains (existing EBR tests green).
6. Benchmark target: none (correctness-only).

## Tests to add

- `Ebr.LinkType085WalksChain` (MBR extended 0x85 + two EBRs, links 0x85 — fails today)
- `Ebr.LinkType085SameBasisAs05` (volume bases identical for a 0x85- and 0x05-stamped
  copy of the same chain)
- `Mbr.Extended085NotEmittedAsVolume` (no bogus Other-volume at the link target)

## Related findings

- N-161 / M-123 (same file's GPT gates — independent rows)
- M-046/M-002 lineage (the EBR walk's landed fixes this rides on)

## Open questions

None on 0x85. Adjacent undecoded: whether the og treats 0x93 (Linux extended-mixed) the
same — no corpus evidence either way this session (no 0x93 in the walk); out of scope
until cited.
