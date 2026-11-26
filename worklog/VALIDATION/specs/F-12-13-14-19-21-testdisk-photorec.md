# W3/W4 Implementation Specs — TestDisk Partition Layer + PhotoRec Carving

> Wave 3 rows F-12/F-13/F-14, Wave 4 rows F-19/F-21. Source: testdisk @
> `02941e1b` (GPLv2+). Policy: behavioral re-implementation ONLY — no code,
> no verbatim tables/threshold strings; on-disk format facts are
> uncopyrightable. Extracted 2026-11-26 with file:line citations against
> the pinned reference clone. Companion sheet:
> `worklog/TESTDISK-FEATURES.md`.

## F-12 — Backup boot-sector position lattice

**Where in the reference**: probe schedule built in `src/next.c`
(`search_location_init`, lines 112-213), consumed by the scan loop
`search_part()` (`src/godmode.c:535-977`). The schedule is a set of
arithmetic progressions `{offset, inc}` (max 256); the scan visits the
union of all lattices in sorted order; duplicate/subsumed progressions
are merged.

**The lattices** (512-byte sectors; 1 MiB unit = 2048·512):
- **FAT32 backup (sector 7)**: progression offset sector-7 with inc 1 MiB
  (`src/next.c:147-169`); i386 adds CHS families H=0/1/2, S=7 stepping
  per cylinder. Gate (`src/godmode.c:707-713`): sector==7 && (head<=2 ||
  fast_mode>1), or `offset % (2048*512) == (7-1)*512` (GPT/other arches
  via `location_boundary`). Handler reads one sector, requires 0xAA55,
  then validates as FAT with backup=1; on success `sb_offset = 6*512` and
  the partition start is BACK-PROJECTED (`part_offset -= sb_offset`).
- **exFAT backup (sector 13)**: rides the main grids; gate sector==13 &&
  (head<=2 || fast_mode>1) or `offset % (2048*512) == (13-1)*512`
  (`src/godmode.c:716-724`). Backup recognized only when
  `start_sector*sector_size + (12 << blocksize_bits) == part_offset`;
  back-project by `12 << blocksize_bits`.
- **NTFS/HFS backup**: progression offset = last-sector-of-first-track
  (CHS(1,0,-1)) inc per-cylinder (i386), non-i386 offset
  `location_boundary-512` inc `location_boundary`; PLUS offset
  `(2048-1)*512` inc 1 MiB (`src/next.c:170-185`). Gate
  (`src/godmode.c:726-746`): sector==sectors_per_head && (head ==
  heads_per_cylinder-1 || fast_mode>1) or 1 MiB congruence
  `(2048-1)*512`. NTFS: `part_size = (sectors_nbr+1)*sector_size`,
  reject if `part_offset + sector_size < part_size`, then `sb_offset =
  part_size - sector_size` and back-project. HFS/HFS+: `sb_offset =
  part_size - 0x400` (backup at total_blocks-2).
- **ext2/3/4 group-3 backup superblocks** (`src/next.c:186-211`,
  `src/godmode.c:749-781`): `hd_offset = 3 * B * 8 * B + (s_log_block_size==0
  ? 2*512 : 0)` where `B = 1024 << s_log_block_size`, s_log_block_size in
  0..2 (1K/2K/4K). From e2fsprogs: `s_first_data_block =
  s_log_block_size ? 0 : 1`, `s_blocks_per_group = 8*block_size`;
  sparse-superblock puts backups in groups 0, 1, powers of 3, 5, 7 —
  group 3 is checked. Acceptance: `s_magic==0xEF53`, `s_block_group_nr>0`,
  structural validation; back-project by `s_first_data_block +
  s_block_group_nr*s_blocks_per_group` blocks.
- **NTFS back-projection pass** (`src/godmode.c:489-531`): after the main
  scan (fast_mode>0), for each NTFS partition located via backup, probe
  **i = 32 down to 1** sectors before the found start for a valid primary
  boot sector; insert only if within `[min_location, search_location_max]`.

**FAT32 identity compare ignoring 0x3E8-0x3F0** (boot-sector repair UI,
`src/fat32.c:122-182`): primary and backup sectors are identical iff
bytes [0,0x3E8) and [0x3F0,0x600) compare equal — the gap 0x3E8..0x3EF
holds filesystem-state fields (free-count/next-free) that legitimately
differ at runtime.

**Error handling**: read failure propagates as read-error status; marker
mismatch or failed validation → advance to the next candidate; partitions
ending beyond the disk go to a "bad" list with a warning.

## F-13 — Quick-scan lattice + hint list

Same scan engine with `fast_mode` 0 (first "Search!" pass) / 1 ('S' loop)
/ 2 ("Deeper Search"). i386 quick lattice: head 0/1/2 sector 1 every
cylinder, plus every 1 MiB (`src/next.c:124-137`; gate
`src/godmode.c:654-657`). Hints added during the scan
(`search_add_hints`, `src/godmode.c:387-443`): starts of known partitions;
after each hit the end and the end rounded up to head/cylinder boundaries;
`2·sector+16384` (GPT-as-Intel), `2048·512` (Vista), 32/63 sectors, CHS
1/[01]/1 for 16/240/255-head geometries; NTFS-backup hints at
last-cylinder CHS and disk-end-aligned 1 MiB grid; GPT-arch NTFS-backup
hint computed from the backup header LBA. RAID follow-ups: on each
non-md find, hints at `part_offset + MD_NEW_SIZE_SECTORS(...)·512` for
disk_factor 6..1 and help_factor 0..(4 MiB/64 KiB + 3).

## F-14 — GPT repair (dual write)

`src/partgptw.c:218-312`: read the original 128 entries (128·128 bytes at
LBA 2) to **preserve partition GUIDs** (match on `ent_lba_start`,
rejecting duplicate UUIDs); **preserve the disk GUID** by reading the old
header at LBA 1 (fresh UUID only if unreadable). Write primary entries at
LBA 2 and header at LBA 1; flip `hdr_lba_self/hdr_lba_alt/hdr_lba_table`
to the disk-end equivalents and write the backup header/entries;
recompute `hdr_crc_table` (CRC32 of entries ^0xFFFFFFFF) and
`hdr_crc_self` (CRC32 of the 92-byte header with the field zeroed) for
each header. Finally write a protective MBR with a type-0xEE entry
spanning the disk.

## F-19 — Bounded fragmentation reassembly (PhotoRec brute force)

Entry `photorec_bf` (`src/phbf.c:124-320`): two phases over the remaining
free space, walking the search-space list **backwards**. Standard
header/data_check loop; when finishing fails (file_size==0 &&
offset_error!=0), truncate to `offset_error/blocksize*blocksize` and
enter `photorec_bf_frag` (`src/phbf.c:540-725`) at frag=0. If a file was
recovered and the last freed block ends before the next known header,
direction flips forward.

`photorec_bf_frag` control flow:
1. Fast path (`src/phbf.c:448-538`) when extra>0 and
   `offset_error/blocksize > offset_ok/blocksize`: with `blocs_to_skip =
   extra/blocksize`, for i in 0..blocs_to_skip−1: truncate to
   original_offset_ok, skip i blocks, append blocks to
   original_offset_error, skip the remaining blocs_to_skip−i, re-pad.
2. Split-point search: `file_offset` walks **backwards** from
   original_offset_error rounded down to blocksize, while
   `original_offset_error <= file_offset + 6*512 || original_offset_error
   < file_offset + 2*blocksize` (split points within ±6 sectors / 2
   blocks of the error offset).
3. For each split point, inner loop `blocs_to_skip` from **−2 to 4999**.
   Negative → jump to the next search-space element (next candidate
   header) `2+blocs_to_skip` times; non-negative → advance blocksize at a
   time, abort at list end. Continue while `offset_error==0` or
   phase-dependent bounds: phase 0 while `offset_error >= file_offset ||
   blocs_to_skip<16`; phase 1 while `offset_error+blocksize >=
   file_offset || blocs_to_skip<100`; phase 2 while `blocs_to_skip<10`.
4. Each iteration pads: append up to **+1000 blocks** past offset_error
   (**+100** when no data_check), skipping blocks tagged with another
   file's header, then re-validate; repeat while the error offset
   improved by more than `nbr/2*blocksize`. Outcomes: `offset_error==0` →
   recovered; error ≥2 blocks past the split → next fragment; else prune.
5. On fragment-found, recurse with `frag+1`, **only while frag<=5**
   (≤5 extra fragments). New error before the split point → prune. User
   stop checked once per second.

## F-21 — Auto block-size detection

State machine in `src/phrecn.c:230-430`; first pass STATUS_FIND_OFFSET
with `safe_header_only=1` (`src/phbs.c:76-229`): walk the space
blocksize-stride (blocksize defaults to sector size), run ONLY the header
checks (never write files), split the search-space at each header hit and
tag it, **stop as soon as 10 headers are found** (`src/phbs.c:170-173`).
Then `find_blocksize` (`src/photorec.c:515-560`): start at **64 KiB**;
seed `*offset = first_entry->start % blocksize`; if any tagged element's
`start % blocksize != *offset` and `blocksize > sector_size`, **halve**
blocksize, reseed, restart. Terminate when all header offsets are
congruent mod blocksize or blocksize reaches sector size. Then re-align
every range to the discovered grid (ceil-align starts, merge/delete
degenerate ranges). Rationale: all files on a volume share the cluster
size, so header positions are congruent modulo it.

## License note

All formulas above are behavioral descriptions re-derived for
re-implementation. The GPLv2+ reference is cited for behavior only; no
code, tables, or threshold strings are copied into the product. Format
facts (sector positions, magic markers, structure offsets) are
uncopyrightable facts.
