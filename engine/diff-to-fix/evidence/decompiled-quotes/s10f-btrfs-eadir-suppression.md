# Evidence — S10-F/T3b: the og's Btrfs "@eaDir" subtree suppression (first-hand decompile)

> Session-10 S10-F lane (function-level completeness pass). Corpus `be0cfe9`;
> core product-code tree `44f043a5` (e56baac7 lineage). Every quote below read
> first-hand by the lead from
> `re-workspace/reversed/ghidra-searchdll/BtrfsSearchDLL_decompiled.c`.

## 1. The suppression site — FUN_180001cd0 (the per-directory descent), lines 655-760

The Btrfs quick-scan driver `FUN_180002120` (:843-928, the only og controller
per I-003 — the complete factory is a NULL stub) calls `FUN_180001cd0` per
entry (:523). The descent function:

1. Emits the current entry via `FUN_180001f90` (:683 — creates the
   `CBtrfsFileProerties` (0xB6) + `CFileNode` (0x93) pair and pushes it to
   the sink or the result list; read in full at :775-837).
2. Walks a pending-descent list. Each popped directory entry's name is
   compared against the 7-byte literal `"@eaDir"` — **a prefix match with NO
   length check** (`BtrfsSearchDLL_decompiled.c:704-711`):

```c
if (*(short *)((longlong)puVar1 + 0x66) == 0x10) {      /* entry is a directory */
    if (*(longlong *)((longlong)puVar1 + 0x42) == 0) {
LAB_180001e49:                                            /* normal descent */
        ... walk children, enqueue dirs, emit each child via
            FUN_180001f90 (:735) ...
    }
    else {
        lVar6 = 0;
        do {
            lVar7 = lVar6 + 1;
            if (*(char *)(*(longlong *)((longlong)puVar1 + 0x42) + lVar6)
                != "@eaDir"[lVar6])
            goto LAB_180001e49;                           /* mismatch -> descend */
            lVar6 = lVar7;
        } while (lVar7 != 7);                             /* 7/7 match -> SKIP */
    }
}
```

**Decoded semantics (instructive read, first-hand):**
- A directory whose name's first 7 bytes are exactly `@eaDir` is **never
  descended into** — its children are not enqueued and not emitted. The
  directory node itself WAS emitted (the unconditional child-emit at :735 /
  the entry-emit at :683 run before the pop-time name check), so the og's
  result tree shows the `@eaDir` directory **empty**.
- The match is a **prefix** match: `@eaDirBackup` also matches (the loop
  exits at 7/7 without comparing a terminator).
- The pending-list loop carries a cancel poll
  (`uVar13 + 0x328` → vt+0x10, :693-696) — the suppression sits inside the
  cancellable walk, not a post-filter.

## 2. What "@eaDir" is

Synology DSM's per-directory extended-attribute metadata directory (created
in every folder of Synology Btrfs volumes; holds the EA data for the parent
directory's files). The og's engine-side filter suppresses their enumeration
from scan results. The string census corroborates the literal:
`raw-evidence` `BtrfsSearchDLL rabin2_strings_all.txt:1911` carries `@eaDir`.

## 3. No corpus artifact documents this filter

`rg -i eadir` over `re-workspace/artifacts/` → zero hits. The
`btrfs-engine--deep-dive.md` and `refs-btrfs-zfs-engine-algorithms-v2.md`
walks do not mention it. This evidence file is the decode's first trail
record (the T3b read-agent surfaced it; the lead re-read the decompile
site and the surrounding control flow before filing).

## 4. Core state (verified first-hand at `44f043a5`)

```
$ grep -rn "eaDir" src/ include/ tests/
(no output — zero @eaDir handling anywhere)
```

- The core's Btrfs walk (`src/core/btrfs/cr_btrfs_engine.cpp`, `scan_btrfs`
  :480, deep walk + mirror probing) emits every directory and descends every
  child unconditionally.
- The M-018 junk-filter pass (`src/core/orchestrator/cr_filterfolder.cpp`)
  does NOT cover it: its pools (`kFolderPool`/`kFilePool`/`kFatNamePool` in
  `cr_filterfolder_data.cpp`) are the Windows-junk name sets (Config.Msi,
  $Windows.~BT, desktop.ini, System Volume Information, localized New
  Folder, BitLocker artifacts) — no `@eaDir` — and the pass **flags**
  nodes (`NODE_FLAG_JUNK_FILTERED`, `cr_filterfolder.cpp:103-113`) rather
  than suppressing descent, so it could not reproduce the og's
  empty-directory shape even if the name were added to a pool.
- The og's semantics are therefore NOT reachable from any core surface: a
  Synology Btrfs volume scanned by the core emits every `@eaDir` with its
  full subtree of EA metadata files.

## 5. Falsifiable comparison

Fixture: a Btrfs image containing `/photos/@eaDir/SYNO@EA@photos` (the
standard Synology EA layout). The og's result tree: `photos` → `@eaDir`
(present, EMPTY — no `SYNO@EA@photos`, no children). The core's result tree:
`photos` → `@eaDir` → `SYNO@EA@photos` and every sibling EA entry (full
subtree). Executed comparison not run in this environment (no Synology
fixture on hand); the decompile decode above is instruction-decisive for
the og side, the core side is grep- and read-verified.
