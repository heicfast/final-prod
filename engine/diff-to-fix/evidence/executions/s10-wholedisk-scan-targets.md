# Executed evidence — s10-wholedisk-scan-targets (the whole-disk scan flow: the og reads the partition list; our core silently yields nothing without --whole-disk, and anonymous Candidate roots with it)

**Lane:** session-10 continuation audit (wave 2 — the og-codebase deep-dive)
· **Date:** 2026-10-29 · **Audited tree:** core `84a1832` (rebuilt local; the
product-code lineage `b92ebef` + docs commits) · **Corpus:** `be0cfe9` ·
**Class:** executed comparison (protocol §6.1 — our own engine run on a
controlled fixture, plus the og's own persisted real sessions re-parsed
first-hand this lane).

## The question

The og's product flow for a DISK-level target selection
(architecture/04-recovery-workflow.md §2: "A partition/disk →
PartitionStartWnd → CDoScanPartition → CGenericRecoveryController"; §3.4:
the controller "reads the partition list, inserts lost partitions as
additional scan targets"). The real og sessions (the corpus's three
SaveScan .rsf payloads) are MULTI-VOLUME whole-disk sessions. What does OUR
core do when the scan target is a whole disk?

## The fixture (reproducible builder committed beside this file)

`s10-wholedisk-fixture-builder.py` builds an 18 MiB raw disk: a valid MBR
with TWO type-0x0E FAT16 partitions (LBA 2048 / 16483 secs, LBA 18531 /
16483 secs — the table hexdump-verified `00 20 21 00 0e 20 21 00 … 55 aa`),
each volume carrying one live file (DOC.TXT, 11 bytes) and one deleted
file (DELFILE.TXT, 0xE5-first-byte, 9 bytes). This is the minimal healthy
equivalent of the real G session's disk (two known in-table volumes).

## The executed run (verbatim output)

Probe: `s10-wholedisk-probe.cpp` (committed beside this file) — links the
core library, runs `ScanController::run` over the fixture as a `MemDevice`,
dumping rc / stats / the full tree. Build: gcc 14.2, `-std=c++20 -O1`,
against the locally built `libcr_core.a` (cmake Release, BUILD flags per
the repo presets). Run date 2026-10-29:

```
$ python3 s10-wholedisk-fixture-builder.py /tmp/s10disk.img
disk: /tmp/s10disk.img 18 MiB; P1 FAT16 @2048 (16483 secs), P2 FAT16 @18531 (16483 secs);
      each: DOC.TXT live + DELFILE.TXT deleted

$ ./s10probe /tmp/s10disk.img
image: /tmp/s10disk.img (18975744 bytes, 37062 sectors)

RUN 1 (the CLI `scan --disk N --whole-disk` shape: Unknown hint + whole-disk):
  rc=0 (0x00000000)
  files_found=4 deleted_found=2 carved=0 lost_partitions=2
  [run1] roots:
    - "Found" (children: none)
    - "Deleted" (children: none)
    - "Lost Files" (children: none)
    - "Other Deleted Files (Carved)" (children: none)
    - "Candidate @ 2048" (children: yes)
    - "Candidate @ 18531" (children: yes)
  [run1] tree nodes: 10, DOC.TXT: 2, DELFILE.TXT: 0

RUN 2 (plain `scan --disk N`: Unknown hint, no discovery):
  rc=0 (0x00000000)
  files_found=0 deleted_found=0 carved=0
  [run2] roots:
    - "Found" (children: none)
    - "Deleted" (children: none)
    - "Lost Files" (children: none)
    - "Other Deleted Files (Carved)" (children: none)
  [run2] tree nodes: 4, DOC.TXT: 0, DELFILE.TXT: 0

RUN 3 (control: fs_hint FORCED to Fat on the whole-disk slice):
  rc=1342373891 (0x50030003)   <- ERR_OPEN_FAILED, honest
  files_found=0 deleted_found=0 carved=0
```

Full-tree dump (RUN 1, node detail):

```
rc=0 files=4 deleted=2 lost_parts=2
Found [size=0 del=0 conf=0 origin=0]
Deleted [size=0 del=0 conf=0 origin=0]
Lost Files [size=0 del=0 conf=0 origin=0]
Other Deleted Files (Carved) [size=0 del=0 conf=0 origin=0]
Candidate @ 2048 [size=0 del=0 conf=0 origin=0]
  DOC.TXT [size=11 del=0 conf=0 origin=0]
  _ELFILE.TXT [size=9 del=1 conf=0 origin=0]
Candidate @ 18531 [size=0 del=0 conf=0 origin=0]
  DOC.TXT [size=11 del=0 conf=0 origin=0]
  _ELFILE.TXT [size=9 del=1 conf=0 origin=0]
```

## What the run proves (our side, at `84a1832`)

1. **RUN 2 — the silent empty verdict.** The plain disk target (the CLI's
   `scan --disk N` without `--whole-disk`, the ABI's `cr_ui_open_disk` +
   `cr_ui_scan` with default config): `probe_family` reads LBA 0, finds the
   MBR (no FS signature), returns `FsFamily::Unknown`; the engine dispatch's
   default case is `rc = OK; /* unknown FS: quick yields nothing */`
   (cr_scan.cpp:1424-1427); `run()` never consults the partition table for
   scan targets (the `disk_layout_` parse at run() start feeds only the
   session records and the resume revalidation). Result: rc=0, zero files,
   an empty tree — on a healthy two-volume disk. The `probe_unreadable`
   warning does not fire (the sector was readable; it just is not an FS).
2. **RUN 1 — the anonymous-candidate shape.** With `whole_disk` on, the
   lost-partition discovery returns BOTH in-table partitions as candidates
   (the `start_lba == p.start_lba` table-match exemption from the M-017(a)
   containment drop + the +500 "Found once partition" bonus,
   cr_candidates.cpp:928-962), and `scan_candidate` scans them — so the
   FILES are found (no silent loss) — but under synthetic
   `"Candidate @ <start-LBA>"` roots (cr_scan.cpp:1629-1636) with no volume
   display name, no FS-type marker, and no distinction from genuinely lost
   partitions. The main `Found`/`Deleted` roots stay EMPTY (the main pass
   still dispatches Unknown). Deleted files live under the candidate root
   (`_ELFILE.TXT del=1` — the FAT engine's 0xE5 first-char reconstruction).
3. **RUN 3 — the control.** Forcing `fs_hint=Fat` on the whole-disk slice
   fails honestly (ERR_OPEN_FAILED) — the only path that finds the volumes
   without `--whole-disk` is the discovery rescue of RUN 1.

## The reference side (corpus + the og's own persisted sessions)

1. **architecture/04-recovery-workflow.md §2** (the routing table): a
   disk-level selection routes to `CDoScanPartition` →
   `CGenericRecoveryController`; §3.4: the controller "builds the FS→searcher
   map, reads the partition list, defaults sector size 512, then logs
   'Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d' per
   lost partition — lost partitions are inserted as scan targets directly."
   The partition list is READ at scan setup: the table's volumes are the
   primary targets; lost partitions are ADDITIONAL targets, each carrying a
   NAME (`%s` — decompile `DRWUI_decompiled.c:904076` / `:904198`, the
   insertion loop over the 0xa8-stride partition-info objects).
2. **The real G session, re-parsed first-hand this lane** (the core's own
   `scripts/compare_og_rsf.py`; verbatim parser output: `rows=218503
   names=14 stats={'roots': 4, 'nodes': 218513, …}`):

```
=== depth-1 roots (G session, 218,503 rows) ===
  'Local Disk(G:)': 121775        <- volume 1's whole tree (live + deleted, per-row flags)
  'Reconstructed': 83106          <- the carve root (S-048's branch)
  'File Path Lost': 13430         <- the orphan root (M-103's branch)
  'System Reserved (NTFS)': 190   <- volume 2's whole tree (bootmgr, Boot, …)
  'NTFS': 1                       <- the FS-type marker row
  'System Reserved (NTFS)\x00': 1 <- the og's own NUL-suffixed twin marker (data quirk)
```

   Every volume — known-in-table or discovered — surfaces as a DISPLAY-NAME
   root (`Local Disk(G:)`, `System Reserved (NTFS)`), with the live and
   deleted files in ONE tree (deleted = per-row flag), not split into
   Found/Deleted roots.

3. **UIUX/07 + the landing-page decode:** the device tree presents disks
   with their volume children (`Internal drives (2)`, `Windows(C:)`) —
   volume display names are the og's first-class target vocabulary.

## What this evidence changes

- Filed **S-050**: the plain whole-disk target flow (no partition-list
  targets) — the silent empty verdict of RUN 2.
- Filed **M-104**: the anonymous `Candidate @ <LBA>` presentation of
  table-matched volumes (RUN 1) — no display-name volume roots, no FS-type
  markers, empty Found/Deleted roots; the og's real-session shape is
  unreachable through the whole-disk flow.

## Reproduction

```
python3 diff-to-fix/evidence/executions/s10-wholedisk-fixture-builder.py /tmp/s10disk.img
# build the probe against the core (gcc 14.2, c++20):
g++ -std=c++20 -O1 -I include -I tests/win-stub \
    diff-to-fix/evidence/executions/s10-wholedisk-probe.cpp \
    <build>/src/core/libcr_core.a -lz -llz4 -lzstd -lpthread -o /tmp/s10probe
/tmp/s10probe /tmp/s10disk.img          # RUN 1/2/3 verbatim above
# the real-session side:
python3 scripts/compare_og_rsf.py \
    "../easeus-drw-research/og-codebase/SaveScan/auto_Local Disk (G_)10_15_00_44_10.rsf" /tmp/g.csv
```

## Addendum — the record-level census (the multi-volume session's persistence contract, first-hand this lane)

Record census over all three real sessions (chain-walk per the validated
grammar; payloads de-chunked + zlib-decompressed; the record-2 layout
verified against rsf-format--scan-session.md §5.1/§5.2 — the artifact's
147-byte record-2 confirmed exactly):

```
=== record chain (all three sessions) ===
G: rec0 state   25B compressed -> 45B decompressed
   rec1 volinfo 65B -> 147B   label "Local Disk (G:)"  deviceType=6
   rec2 partctx 588B
   rec3 tree    7,365,723B
H: rec0 state   13B -> 12B (all zero)      rec1 147B "Local Disk (H:)"
   rec2 590B    rec3 52,743,051B
I: rec0 state   13B -> 12B (all zero)      rec1 147B "Local Disk (I:)"
   rec2 588B    rec3 8,995,756B
```

**One record-2 (volume info) per session — even in the two-volume G
session** — and it names the PRIMARY volume ("Local Disk (G:)"). The
multi-volume identity is NOT a second record-2.

**The multi-volume registry is record-1's special-partition list** (the
artifact §5.1: `entryCount` + entries copied from the scan device's
0xA8-stride partition-info objects — the SAME objects the scan-setup
insertion loop iterates at DRWUI_decompiled.c:904198). The G session's
45-byte state decodes as: `entryCount=1`, one entry
`fieldA=2048` (start LBA — System Reserved, the non-primary volume),
`fieldB=0x31FFF=204,799` (extent end / size-1 form), `fieldC=3`,
`nameLen=0`, then `trailerLen=0` + the four flag bytes (all 0 — the
is-whole-disk boolean among them reads 0 in this sample). The
single-volume H/I sessions carry `entryCount=0` (12 zero bytes).

Parity note for the fix plans: our crsf persists the table SNAPSHOT
(`DiskLayoutEntry[]` — every table row) where the og persists the
session's SELECTED additional partitions (record-1's list) — adjacent
but not equivalent; the primary-volume identity (record-2) maps to our
singular `VolumeSessionInfo`. S-050/M-104's fixes need no session-model
change for the primary volume; the per-volume tree identity (M-104)
should ride the existing record fields (the tree node names), not a new
record type.
