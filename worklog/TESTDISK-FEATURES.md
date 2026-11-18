# TESTDISK/PHOTOREC — Feature Context Sheet
> Agent D (Task 2-D), Phase-0. READ-ONLY extraction at HEAD 02941e1b0fce14a33932724de85cbe8385bb70d0, GPLv2+ (COPYING) — behavioral extraction only, no code copying. Date 2026-11-19. Every claim cites file:line.

## 0. Identity (URL, SHA, layout, LOC estimate, license)

- Upstream: cgsecurity/testdisk (https://github.com/cgsecurity/testdisk).
- Local checkout: `/home/z/my-project/repos/testdisk`, HEAD `02941e1b0fce14a33932724de85cbe8385bb70d0`, 793 tracked files.
- License: GPLv2+ — `COPYING` lines 1-4 ("GNU GENERAL PUBLIC LICENSE Version 2, June 1991"); every source file carries the "version 2 of the License, or (at your option) any later version" grant (e.g. src/photorec.c:7-9).
  - **Constraint: no source may be copied; only the behaviors described here may be re-implemented.**
- Layout: flat `src/` directory; ~514 `.c/.cpp` files + headers; **~126,334 LOC** (`wc -l src/*.c src/*.cpp src/*.h`).
- Three user-facing tools in one codebase:
  - **TestDisk** (partition-table recovery): testdisk.c, tdiskop.c, tanalyse.c, tpartwr.c, godmode.c (1541 LOC scan engine), analyse.c, intrface.c, adv.c, geometry.c, tntfs.c/texfat.c/thfs.c, ntfs_fix.c, fat_adv.c (2892 LOC), parti386.c (1860 LOC), partgpt.c/partgptw.c, partmac/partsun/partxbox/parthumax/partnone/partauto.c, savehdr.c, tdelete.c, edit.c, io_redir.c, dirpart.c.
  - **PhotoRec** (file carver): phmain.c, pdiskseln.c, ppartseln.c, phrecn.c (pass machine), psearchn.c (main loop), photorec.c (file lifecycle), phbs.c (blocksize detect), phbf.c (brute force), phnc.c, phcli.c, phcfg.c, sessionp.c, pfree_whole.c, pblocksize.c, filegen.c (registry engine), file_list.c (registry table, 2172 LOC) + **353 `file_*.c` signature modules**.
  - **fidentify** (src/fidentify.c) — non-interactive signature identifier; **qphotorec** (src/qphotorec.cpp, 1005 LOC, + qphbs.cpp/qpsearch.cpp) — Qt GUI.
- Shared filesystem readers: fat*.c, ntfs*.c, ext2*.c, rfs*.c, hfs*.c/hfsp.c, btrfs.c, xfs.c, ufs.c, jfs.c, gfs2.c, zfs.c, luks.c, lvm.c, md.c, etc.
- Partition-arch abstraction: `arch_fnct_t` vtable — read_part / write_part / init_part_order / get_geometry_from_mbr / check_part / write_MBR_code / set_prev_status / set_next_status / test_structure / get_part_type / set_part_type / init_structure / erase_list_part / get_partition_typename / is_part_known (parti386.c:359-378).
- In-tree docs: README_dev_photorec.txt (carver architecture), documentation.html, man/ (Makefile.am), NEWS, ChangeLog.

## 1. TestDisk: partition detection & recovery (MBR/GPT/backup/rebuild/deeper search/candidate validation & scoring/MFT mirror repair)

### 1.1 Partition table architectures (pluggable)
- One `arch_fnct_t` instance per architecture: `arch_i386` (parti386.c:359-378), `arch_gpt` (partgpt.c:187-207), arch_mac, arch_sun, arch_xbox, arch_humax, arch_none (partnone.c); `partauto.c` auto-detects the table type at disk selection.
- Per-architecture conventions documented in godmode.c:445-472:
  - Intel: display CHS, geometry-sensitive, align to config, MinPartOffset 512.
  - Mac: display sectors, 4K align (optional), MinPartOffset 512, geometry not critical.
  - None: no alignment, MinPartOffset 0.
  - Sun: cylinder-aligned, partitions need H=0/S=1, geometry required.
  - XBox: no alignment, MinPartOffset 0x800.
- `get_location_boundary` (godmode.c:85-92): alignment unit = 4096 (mac) / one cylinder (sun) / sector_size (others).

### 1.2 MBR (Intel/i386) read & validation
- On-disk entry `struct partition_dos` (parti386.c:82-93): boot_ind (0x80=active), CHS start (head/sector/cyl), sys_ind, CHS end, 32-bit LE start4/size4.
- CHS encoding macros s_cyl/s_sect/e_cyl/e_sect (parti386.c:96-99) — 10-bit cylinder split across sector byte bits 6-7.
- `read_part_i386` (parti386.c:529-593):
  1. Read sector 0; require 0x55AA at 0x1FE/0x1FF (parti386.c:457-460) else "Bad MBR" message.
  2. Geometry inference from the entries (see §1.8).
  3. Iterate the 4 slots; sys_ind 0x05 (P_EXTENDED) / 0x0F (P_EXTENDX) / 0x85 (P_LINUXEXTENDX) → STATUS_EXT, else STATUS_PRIM (parti386.c:559-569).
  4. Each entry becomes a `partition_t` via `i386_entry2partition` (parti386.c:1222) which also probes the filesystem at the computed offset (check_part).
  5. `test_MBR_data` (parti386.c:595-648): warnings — >1 DOS FAT primary, >1 extended, 0 bootable, >1 bootable.
  6. `test_MBR_over` (parti386.c:832-846): adjacent partitions overlapping → msg_SAME_SPACE.
  7. `get_ext_data_i386` walks the EBR chain (below).
- **Extended partition chain** `get_ext_data_i386` (parti386.c:661-803):
  - Follows EBR links (typically entry 1 = logical partition, entry 2 = next EBR); **hard cap 32 logical partitions** (parti386.c:671).
  - Each EBR must carry 0x55AA (parti386.c:682-686).
  - Per-EBR sanity messages (parti386.c:713-722): logical partition must not be hidden; multiboot flag only on primary; at most one link to another logical; exactly one data partition per EBR; logical must not be bootable.
  - Bounds: logical partitions and next EBRs must lie inside the main extended partition (parti386.c:741-766 and 781-787).
- `recover_i386_logical` (parti386.c:805-830): during deep scan, recognizes a logical-partition slot hosting a TrueCrypt volume — accepts only FAT/NTFS sys_ind values (812-824).
- Type-name table `i386_sys_types` (parti386.c:262-357): ~90 entries incl. hidden FAT/NTFS variants, extended/extended LBA/Linux extended, 0xEE "EFI GPT" (protective), 0xEF "EFI (FAT-12/16/32)", Linux swap/LVM/RAID, BSD labels, VMFS, Solaris, BeFS.
- **MBR write** `write_part_i386` (parti386.c:848-855) = `write_mbr_i386` + `write_all_log_i386` (writes each EBR) + `disk->sync`.
  - Ordering: `init_part_order_i386` (parti386.c:503-526) numbers primaries/extended 1-4 and logicals 5+; user-editable via `ask_mbr_order_i386` (godmode.c:996-1107, keys 1-4 to assign MBR slot order).
  - `write_MBR_code_i386` (parti386.c:1115-1132) writes a stock boot code (embedded in tmbrcode.c) while preserving the table bytes.

### 1.3 GPT: read, validation, backup header, write/repair
- `read_part_gpt` (partgpt.c:378-389):
  1. Try the **primary header at LBA 1**.
  2. If that returns NULL (any validation failure), print "Trying alternate GPT" and read the **backup header at last LBA = (disk_size-1)/sector_size** — the backup is used to *list partitions* whenever the primary is unusable.
- `read_part_gpt_aux` (partgpt.c:214-376) validation chain, in order:
  - signature "EFI PART" (partgpt.c:230);
  - header size in [92, sector_size] (253);
  - CRC32 of the header (with hdr_crc_self zeroed) must match (259-272);
  - hdr_lba_self must equal the LBA it was read from (273-278);
  - usable range hdr_lba_start < hdr_lba_end (279-284);
  - revision != 1.0 and __reserved != 0 → warnings only (285-292);
  - hdr_entries in [1, 4096] (293);
  - hdr_entsz % 8 == 0 and in [128, 4096] (301);
  - entries array size >= 16384 bytes (309);
  - primary: entries must sit between the header and FirstUsableLBA (316-325);
  - CRC32 of the whole entries array must match hdr_crc_table (334-344).
  - Entries with non-UNUSED type GUID and lba_start < lba_end become partitions; order = index+1; status PRIM; UTF-16 name converted (349-368).
- GUID→name table `gpt_sys_types` (partgpt.c:135-185): EFI System, Extended Boot, MBR, FreeBSD set, MS Reserved/Basic Data/LDM/Recovery/Storage Spaces, Linux RAID/Swap/LVM/Reserved/home/src/data, HPUX, Mac APFS/HFS/UFS/RAID/Boot/Label/TV Recovery/Core Storage, full Solaris set, BeFS.
- `check_part_gpt` (partgpt.c:527-593) — content probe by type GUID:
  - MS Basic Data/Reserved → check_FAT → check_exFAT → check_NTFS → check_ReFS → check_linux (else "No FAT, NTFS, ext2, JFS, Reiser, cramfs or XFS marker").
  - Linux RAID → check_MD; Linux LVM → check_LVM/check_LVM2; Mac HFS → check_HFS/check_HFSP; Mac APFS → check_APFS; BeFS → check_BeFS.
  - Failures are logged and (with /saveheader) the header sector is dumped to a file.
- `test_structure_gpt` (partgpt.c:480-488): overlap check only (is_part_overlapping on a sorted copy).
- `init_structure_gpt` (partgpt.c:495-525): mutually overlapping found candidates are both marked DELETED.
- **GPT write/repair** `write_part_gpt` (partgptw.c:218-312):
  - 128 entries × 128 bytes (hdr_entries=128).
  - Disk GUID: preserved from the existing primary header if readable, else freshly generated `efi_generate_uuid` (243-246). (phmain.c:202: a weak `srand(time(NULL))` is explicitly considered acceptable for GPT GUID generation.)
  - FirstUsableLBA = 1 + entries_size/sector_size + 1 (254); LastUsableLBA = (disk_size-1-entries)/sector_size - 1 (255).
  - Computes entries CRC32, then header CRC32 with crc_self zeroed (256-261).
  - Writes entries at LBA 2 and header at LBA 1 (268-283).
  - Then writes the **backup**: entries at the end-entries LBA and header at last LBA with hdr_lba_self=last, hdr_lba_alt=1, recomputed CRC (284-304).
  - Finally `write_part_gpt_i386` (partgptw.c:118) writes a **protective MBR** (type 0xEE), then `sync` (309-310).

### 1.4 EFI/MBR interplay
- MBR type 0xEE is listed as "EFI GPT" in the i386 type table (parti386.c:346).
- Deep-scan hint for the GPT-on-Intel mistake: when arch is i386, offset `2*sector_size+16384` is always probed because "sometimes users choose Intel instead of GPT" (godmode.c:391-393); same hint for arch_mac ("i386 Mac", godmode.c:440-442).
- `get_min_location` (godmode.c:473-487) — minimum plausible partition start per arch: GPT 2*sector+16384; i386/humax sector; mac 4096; sun one cylinder; xbox 0x800; none 0.

### 1.5 Quick Search vs Deeper Search (partition scan)
- Driver `interface_recovery` (godmode.c:1478-1540), looping on fast_mode:
  1. `search_part(disk, list_part_org, verbose, dump_ind, fast_mode, current_cmd)`.
  2. `warning_geometry` (geometry check, §1.8).
  3. `align_structure` (round partition ends to 1 MiB → cylinder → track → sector; i386 path avoids overlap with next partition; godmode.c:94-152).
  4. `arch->init_structure` (mark overlaps deleted, fix statuses).
  5. `ask_structure` — interactive result editing (§3.1).
  6. If `arch->test_structure(list_part)==0` → `ask_write_partition_table` → 'W' confirm → `write_part` + `use_backup()` (auto-opens boot-sector repair for partitions found via backup, godmode.c:1446).
  7. Menu 'S' "Deeper Search" (tpartwr.c:50) increments fast_mode and loops (godmode.c:1462-1465).
- fast_mode semantics:
  - 0 = Quick Search: no backup-boot-sector probes; after an accepted candidate, **scan jumps to that partition's end** (godmode.c:904-909) — assumes found partitions are correct.
  - 1 = Deeper Search: adds backup-sector and ext2-backup-superblock probe positions; scans inside found partitions too.
  - 2 = "every sector" variant of the i386 head/sector lattice (godmode.c:655-657 `start.sector==1 && fast_mode>1`).
- `search_part` (godmode.c:535-993) — the scan loop:
  - search_location_max = max(cylinder-rounded disk size, disk_real_size) (godmode.c:555-558).
  - Seed hints: offsets of all current partitions (563-570).
  - `search_add_hints` (godmode.c:387-443): 2*sector+16384 (GPT-as-Intel), 2048*512 (Vista), 32/63 sectors (0/1/1), 16/17, 240/241, 255/256 × 63 spt heads (and same with the disk's actual spt), NTFS-backup at (C-1/H-1/S) for last and second-to-last cylinder, and `(disk_size-sector)/(1MiB)*(1MiB)-sector`; GPT-specific NTFS backup estimate from the entries size (godmode.c:430-437).
  - Probe gating (godmode.c:643-659): i386 — sector==1 and head<=2, or offset multiple of 2048*512; other arches — offset multiple of location_boundary.
  - Test ladder per candidate offset (test_nbr 0..14, godmode.c:669-941):
    1. Linux MD RAID superblock (v0/v1; partition offset corrected by super_offset, godmode.c:676-703).
    2. **FAT backup boot sector**: at C/H/S sector 7 when head<=2, or (7-1)*512 into each 1 MiB unit (godmode.c:705-715) → `search_FAT_backup` (analyse.c:129-141) → recover_FAT(..., backup=1).
    3. **exFAT backup**: sector 13 positions (godmode.c:716-725).
    4. **NTFS backup boot sector**: last sector of each head/track, and (2048-1)*512 per MiB (godmode.c:726-737).
    5. **HFS/HFS+ backup volume header**: same positions as NTFS backup (godmode.c:738-748).
    6. **ext2/3/4 backup superblock**: sparse-superblock group 3 for block sizes 1024/2048/4096 — offset `3*bs*8*bs (+2*512 when bs==1024)` — accepted only when `s_block_group_nr>0` (godmode.c:749-781).
    7. Generic dispatch `search_type_2/1/0/8/16/64/128/2048` (godmode.c:782-836) — see §1.6.
  - On a hit (godmode.c:853-938): log it, display it, and run the acceptance tests of §1.7. Also inserts RAID follow-up hints (offsets computed from MD geometry, godmode.c:866-881) and next-partition hints (partition end + head-rounded, godmode.c:893-903).
  - Stop conditions: user Stop/Skip/plus keys during scan (godmode.c:619-635, 943-960); read errors past disk end (842-852).
- `search_NTFS_from_backup` (godmode.c:489-531, invoked when fast_mode>0 at 976-977): for each NTFS partition found via its backup sector, probes 32..1 sectors *before* the found offset to locate the true partition start.
- Sector-stepping engine (next.c):
  - `search_location_init` (112-213) registers (offset, increment) lines: i386 — C/0/1, C/1/1, C/2/1 with 1-cylinder increment + offset 0 with 2048-sector (1 MiB) increment; fast modes add sector-7 (backup FAT), last-sector-of-track and (2048-1)*512 (backup NTFS/HFS) lines and the ext2 group-3 backup offsets for bs 1K/2K/4K.
  - `update_location` (57-110) merges/refines lines (a line covering another with equal stride is absorbed).
  - `search_location_update` (215-227) returns the minimum next candidate > current — this is why Quick Search only visits head-0/1/2 sector-1 and MiB-aligned offsets.

### 1.6 Filesystem signature dispatch (deep scan) — analyse.c
`search_type_N` functions probe a fixed offset from the candidate partition start:
- `search_type_0` (analyse.c:143-236, offset 0):
  - APFS (nx_magic 0x4253584e), Linux SWAP ("SWAP" at 0 or at 4K), LVM1 (LVM_ID), FAT (0xAA55 → recover_FAT), exFAT (0xAA55), HPFS (0xAA55), OS/2 Boot Manager (0xAA55 + "FAT     "), NTFS (0xAA55 + "NTFS"), NetWare ("Nw_PaRtItIoN"), XFS (XFS_SB_MAGIC), FATX ("FATX"), LUKS ("LUKS\xba\xbe"), ReFS (0x52654653), MD 1.1 (offset corrected), WBFS, cramfs, and `recover_i386_logical` (TrueCrypt host).
- `search_type_1` (analyse.c:238-277, offset +0x200): BSD disklabel (DISKMAGIC twice), BeFS, cramfs, SysV4 (0xfd187e20), LVM2 label, Sun disklabel-i386.
- `search_type_2` (analyse.c:279-309, offset +0x400): **ext2/3/4** (EXT2_SUPER_MAGIC 0xEF53), HFS (drSigWord), HFS+ (version 4 or 5), f2fs.
- `search_type_8` (analyse.c:311-332, offset +4096): MD 1.2 (part_offset corrected by super_offset*512-4096).
- `search_type_16` (analyse.c:334-359, offset +16*512): UFS/UFS2, ZFS vdev boot header.
- `search_type_64` (analyse.c:361-381, offset +63*512, struct at +0x200): JFS ("JFS1").
- `search_type_128` (analyse.c:383-420, offset +126*512, struct at +0x400 = 64 KiB): ReiserFS/Reiser4 ("ReIs"/"ReIsEr4" style magics), UFS2, **btrfs** (8-byte BTRFS_MAGIC), GFS2.
- `search_type_2048` (analyse.c:422-440, offset +2048*512): VMFS (0xc001d00d).
- Each `recover_*` derives part_size from the superblock and sets the type/label, e.g.:
  - recover_NTFS: part_size=(sectors_nbr+1)*sector_size (ntfs.c:103); backup mode shifts part_offset back (§1.9).
  - recover_EXT2: sborg_offset=0x400, part_size=blocks*blocksize; s_block_group_nr>0 marks a backup superblock (ext2.c:111-141).
  - recover_btrfs: flags "Backup superblock" when bytenr != primary position (btrfs.c:48-51); sborg_offset=65536 (btrfs.c:103).
- `check_linux` (analyse.c:442-458): chains check_JFS / check_rfs / check_EXT2 / check_cramfs / check_xfs / check_LUKS / check_btrfs / check_f2fs / check_gfs2 / check_ZFS.

### 1.7 Candidate validation & scoring (what makes a candidate acceptable)
From `search_part` (godmode.c:853-938) — a candidate recovered by any `recover_*` is inserted iff ALL of:
1. `disk->arch->is_part_known(partition)` is true (i386: sys_ind != 0; gpt: type GUID != UNUSED) — godmode.c:861.
2. `partition->part_size > 1` — godmode.c:862.
3. `partition->part_offset >= min_location` — godmode.c:863.
4. end `pos_fin <= search_location_max` — godmode.c:883.
   - Violation → candidate goes to `list_part_bad`; user sees "The hard disk (X) seems too small! (< Y)" with the unrecoverable list (godmode.c:166-298) and a Windows LBA48 hint (197-203).
- There is **no numeric scoring**: first structurally-valid recovery wins at a given offset. The per-FS `test_*` functions are the "magic + sanity" gate, e.g. `test_NTFS` (ntfs.c:144-205):
  - marker 0xAA55; reserved==0; fats==0; dir_entries==0; sectors==0; fat_length==0; total_sect==0; system_id "NTFS"; sectors_nbr>0; sectors_per_cluster ∈ {1,2,4,8,16,32,64,128}.
  - Boot-sector size must not exceed the partition size (188-198) — hard reject.
  - Geometry mismatches (heads/secs_track/sector_size vs disk) are **warnings only** (166-186).
- FAT subtype selection by cluster count: <4085 → FAT12; <65525 → FAT16; else FAT32 (fat.c:149-174).

### 1.8 Geometry (CHS) inference & repair
- `get_geometry_from_i386mbr` (parti386.c:448-501): max cylinders/end_head/end_sect over the 4 entries; the result is **trusted only if spt==32, or spt==63 with heads ∈ {16,32,64,128,240,255}**; anything else → geometry zeroed ("Don't trust the geometry", 495-499).
- Post-analysis `warning_geometry` (godmode.c:1336-1353): compares the current heads_per_cylinder against `get_geometry_from_list_part(disk, list_part, verbose)` (geometry.c) and warns "the current number of heads per cylinder is N but the correct value may be M" — with an explicit hint that wrong geometry can hide partitions or make the table unwritable (godmode.c:344-369).
- Geometry menu/CLI (geometry.c:74-173): `C,` cylinders, `H,` heads (≤ MAX_HEADS 255, geometry.c:41), `S,` sectors/track, `N,` sector size (allowed: 1, 256, 512, 1024, 1536, 2048, 4096, 8192 — geometry.c:55-64); cylinders recomputed from size (set_cylinders_from_size_up, 43-48).
- `align_structure` (godmode.c:94-152): partition ends rounded up to the largest of 1 MiB / cylinder / track / sector size that divides the start; i386-specific handling avoids overlap-induced misalignment (108-133).

### 1.9 Boot-sector backups: usage & repair menus
- **NTFS** `ntfs_boot_sector` (tntfs.c:203-303):
  - `ntfs_boot_sector_scan` (tntfs.c:119-201) reads the boot sector at part_offset and the **backup at `part_offset + part_size - sector_size`** (150); each gets "Status: OK/Bad" from test_NTFS; then "Sectors are identical / not identical".
  - Options (menu tntfs.c:208-220; offered subset adapts to validity — "DOBRL"/"DRML"/"DBRL"/"DORL"/"DR", 180-200):
    - **O** "Org. BS" — copy boot sector over backup sector (254-266).
    - **B** "Backup BS" — copy backup boot sector over boot sector (267-281; resets sb_offset).
    - **L** "List" — list files; when only the backup is valid, an IO redirection serves the backup as the boot sector (282-291).
    - **M** "Repair MFT" (§1.10); **R** "Rebuild BS" (rebuild_NTFS_BS, ntfs_adv.c); **D** "Dump" side-by-side hexdump of both sectors.
- **FAT32** `fat32_boot_sector` (fat32.c:80-310):
  - Backup read at **+6 sectors** (fat32.c:145) — the standard FAT32 backup boot sector position.
  - "Identical" comparison **skips bytes 0x3E8..0x3F0** (non-essential fields: formatted as FAT32 flags etc.), fat32.c:168.
  - Extra options: **C** "Repair FAT" ("Very Dangerous! Expert only"), **R** "Rebuild BS"; FAT1x variant adds **I** "Init Root" (fat1x.c:86-91).
- **FAT Rebuild BS heuristics** (fat_adv.c:1955-2219):
  - `fat_find_info` locates `reserved` and `fat_length` by scanning for the FAT tables; a candidate list with a **score display** ("FAT - sector - score", fat_adv.c:1821-1835) lets experts pick.
  - `find_sectors_per_cluster` infers cluster size from data-region structure; `no_of_cluster2part_type` re-derives FAT12/16/32.
  - FAT1x: `dir_entries` from an actual root-directory scan (`find_dir_entries`, fat_adv.c:700+; default 512 if not found, fat_adv.c:2064-2067).
  - FAT32: default reserved=32 (+1 when start_data is odd, 2079-2084); `fat32_free_info` (free-cluster count) and `fat32_find_root_cluster` (root cluster discovery, expert-overridable, 2186-2204).
  - Finally `create_fat_boot_sector` assembles and writes the new BS.
- **HFS/HFS+** (thfs.c): backup MDB / backup volume header near the partition end (search_HFS_backup, analyse.c:86-110).
- After a partition-table write, `use_backup` (godmode.c:1302-1334) automatically opens the right boot-sector screen for every found partition with `sb_offset != 0` (i.e. located via its backup) so the user repairs the boot sector immediately after the table fix.

### 1.10 MFT mirror repair (NTFS)
`repair_MFT` (ntfs_fix.c:53-320):
- Preconditions:
  - Valid boot sector via `check_NTFS` else "Boot sector not valid, can't repair MFT." (68-72).
  - Refuses while the backup-boot-sector redirection is active: "Please quit TestDisk and reboot your computer before trying to fix the MFT." (143-151).
- Geometry:
  - mft_pos / mftmirr_pos from the boot sector: `(reserved + mft_lcn|mirr_lcn * sectors_per_cluster) * sector_size` (80-81).
  - mft_record_size = clusters_per_mft_record>0 ? clusters×spc×sector_size : `1 << (-clusters_per_mft_record)` (82-85); must be ≥ 42 (87-93).
  - **mftmirr_size_bytes = max(cluster_size, 4 × mft_record_size)** (95-97).
- Compare the first mftmirr_size_bytes of the MFT with the MFT mirror:
  - Identical → "MFT and MFT mirror match perfectly." and stop (133-142).
- Adjudication via the **IO redirector trick** (159-291):
  - Redirect reads of the mirror to the MFT buffer and attempt an NTFS directory listing (libntfs); then the reverse; each side gets res = listing result (+1 bonus when >2 entries listed).
  - No libntfs → "Can't determine which MFT is correct, ntfslib is missing." (171-180).
- Outcomes:
  - MFT wins → Y/N "Fix MFT mirror using MFT?" → write the first mftmirr_size_bytes of the MFT over the MFT mirror (304-314).
  - Mirror wins → Y/N with explicit danger text: "Fix MFT using its mirror ? (Y/N) - DANGEROUS NON REVERSIBLE OPERATION. Use it ONLY IF TestDisk and Windows failed to access this filesystem." → write the mirror over the MFT (215-234, 292-303).
  - Tie → "Both MFT seems ok but they don't match, use chkdsk." or "MFT and MFT mirror are bad. Failed to repair them."; expert mode offers a manual B/M/Q menu (250-289).

## 2. PhotoRec: carving engine (registry + count, scan loop, header/footer matching, validators, fragmentation policy, output naming, options)

### 2.1 Signature registry
- `struct file_hint_t` (filegen.h:51-59): `{ extension, description, max_filesize, recover, enable_by_default, register_header_check() }`.
  - `recover=0` types are detected for logging only (no output file created — photorec_check_header.h:138-153 gates fopen on recover==1).
- `struct file_enable_t` (filegen.h:70-74): `{ file_hint_t*, enable }`.
- Registry table `array_file_enable[]` (file_list.c:1089-2149):
  - **353 rows + NULL terminator** (353 `extern const file_hint_t file_hint_*` declarations at file_list.c:31-1087; 353 `.file_hint=&file_hint_*` entries; one extra `{ .enable=0, .file_hint=NULL }` terminator at 2148).
  - All rows ship with `.enable=0` in source; runtime defaults come from `reset_array_file_enable` (phcfg.c:46-51) copying each `enable_by_default`.
  - User config overlay: `$HOME/.photorec.cfg` (Windows: %USERPROFILE%\photorec.cfg; fallback photorec.cfg in CWD) with "ext,enable" / "ext,disable" lines (phcfg.c:53-65, 180-244); saved from the File Opt screen 'b' key (phrecn.c:702-711).
- **Counts (mechanically counted, how counted)**:
  - 353 signature modules: `ls src/file_*.c | wc -l` = 353.
  - 285 of them have `enable_by_default=1`: `rg -l 'enable_by_default=1' file_*.c | wc -l` = 285.
  - 423 `static int header_check_*` functions and **930 `register_header_check()` calls** across file_*.c (rg counts) — a type may register several signatures (e.g. rar v1.5 + v5.0, file_rar.c:register section; zip PK\x03\x04 + WinZIPv8 "PK00PK\x03\x04", file_zip.c).
  - file_tiff_be.c / file_tiff_le.c are shared checkers without their own registry rows; file_sig.c/file_found.c are auxiliary/pseudo entries — hence registry = 353, matching the module count.
- Registration mechanism:
  - Each module's `register_header_check_X(file_stat)` calls `register_header_check(offset, value, length, header_check_fn, file_stat)` (filegen.c:192-223), inserting a `file_check_t` (filegen.h:108-117) into a list sorted by (offset, memcmp(value), longer-first) (file_check_cmp, filegen.c:114-151).
  - `init_file_stats` (filegen.c:518-571) iterates the enabled rows, calls each register function, then `index_header_check` (266-288) builds the **two-level index**: `file_check_list_t` nodes keyed by signature *offset*, each holding **256 buckets indexed by the signature's first byte at that offset** (filegen.c:159-185; file_check_list_t filegen.h:140-145). Log line: "%u first-level signatures enabled" (filegen.c:568).
- Hard limits (filegen.h:34-43): max signature offset 65535; max signature length 4095; block size ≤ 32 MiB; absolute max file size 2 TiB (1<<41); DJGPP builds cap at 2 GiB.

### 2.2 Pass structure & block-size detection
- `photorec()` (phrecn.c:230-502): `for(params->pass=0; params->status!=STATUS_QUIT; params->pass++)` over statuses (photorec.h:31):
  - STATUS_FIND_OFFSET (blocksize detection) → STATUS_EXT2_ON or STATUS_EXT2_OFF (carving) → (paranoid>1) _BF (brute force) → (keep_corrupted_file) _SAVE_EVERYTHING → STATUS_QUIT (status_inc, photorec.c:998-1042).
  - STATUS_UNFORMAT (FAT unformat, expert Y/N prompt, phrecn.c:278-282) is an alternative entry.
  - Each pass logs "Pass N (blocksize=B) STATUS" (phrecn.c:307-308) and saves the session (367).
- **Blocksize detection pass** (phrecn.c:332-356):
  - `photorec_find_blocksize` (phbs.c:76-229) scans with `safe_header_only=1` (header checks may accept on header alone, phbs.c:133) and **stops after 10 headers found** (phbs.c:170-173), recording found-file start offsets into the search-space list (file_found.c).
  - `find_blocksize` (photorec.c:515-560): starts at 128×512 = 64 KiB and halves until every recorded start is congruent modulo blocksize; FIXME notes NTFS 2 MiB clusters (Win10 Creators) are not handled (photorec.c:517-519).
  - Expert users may override via `menu_choose_blocksize` (phrecn.c:347-350).
  - `update_blocksize` (photorec.c:562-621) aligns all search-space ranges to the chosen (blocksize, offset) grid.
  - If the user supplied a blocksize, the detection pass is skipped (phrecn.c:336-341).

### 2.3 Main carving loop (`photorec_aux`, psearchn.c:80-383)
- Buffers: `buffer_size = blocksize + READ_SIZE`, READ_SIZE = 512 KiB (psearchn.c:77, 92); `read_size = max(blocksize, 65536)` (94). Reads are 512 KiB chunks; the previous blocksize is kept in front of the current buffer so `data_check` always sees a **2×blocksize sliding window (buffer_olddata = previous block, buffer = current)** (115-116, 211-212, 317-328).
- Per iteration at offset O in the current search-space element:
  1. **Header check** `photorec_check_header` (photorec_check_header.h:171-218):
     - Special case: while recovering a **tar**, do not run header checks (is_valid_tar_header on the previous block; 181-194) — prevents false stops inside tar streams.
     - Walk `file_check_list` nodes: bucket = `pos->file_checks[buffer[pos->offset]]`; for each entry, if `length==0` (offset-only check) or `memcmp(buffer+file_check->offset, value, length)==0`, call `header_check(buffer, read_size, 0, current, &new)`; **first non-zero return wins** and returns immediately (198-215).
  2. On a hit, `photorec_header_found` (photorec_check_header.h:101-155):
     - Finish the previous file (`file_finish2`); if it ended truncated, also stop the new one.
     - Copy `file_recovery_new` (hooks: data_check, file_check, file_rename; extension; min_filesize; calculated_file_size).
     - `set_filename`; `fopen(filename, "w+b")` if `file_hint->recover==1` — Windows uses `fopen_with_retry` (1s/2s retries for live-antivirus interference, photorec_check_header.h:35-66).
  3. **ext2 indirect-block skipping**: in ext2-mode, when `file_size >= 12*blocksize` and `ind_block(buffer, blocksize)!=0`, the block is appended with data=0 (metadata, not file data) (psearchn.c:166-183).
  4. Otherwise: fwrite the block; `file_block_append` moves the block from the search space into the file's allocation list (photorec.c:1174-1178); run `data_check(buffer_olddata, 2*blocksize, &file_recovery)` if set; file_size += blocksize (psearchn.c:186-224).
  5. **End-of-file triggers**:
     - data_check returns DC_STOP, or DC_ERROR (DC_ERROR → file_size=0 → reject) (psearchn.c:241-245);
     - `file_size >= file_hint->max_filesize` (225-232);
     - FAT partitions cap at PHOTOREC_MAX_SIZE_32 (4 GiB-1) when `is_fat(partition)` (233-240);
     - end of the search-space list (304-316).
  6. **Backtracking**: after a file finishes OK_TRUNCATED (or a successful recovery skipped a pending header), PhotoRec steps *backwards* up to 5 times to the start of the last unrecovered header (`get_prev_file_header` — limited to 3 fragments / 200 MB, photorec.c:338-371; back counter psearchn.c:263-303) so a header found mid-file still gets its own recovery attempt.
  7. Progress/log every second; session checkpoint every 5 minutes (`next_checkpoint=start_time+5*60`, psearchn.c:121, 368-369; regular_session_save sessionp.c:374).
- **File completion** `file_finish2` → `file_finish_aux` (photorec.c:655-802):
  - Run the type's `file_check` post-validator when paranoid>0 (660-667).
  - Clamp file_size to disk size (669-672).
  - Reject when `file_size < min_filesize` ("File too small (X < Y), reject it", 673-683).
  - Size 0 → **unlink the output file** (unless paranoid==2/save-everything context; 684-694).
  - `ftruncate` to the computed size (695-701); apply mtime via `set_date` (704-705); run `file_rename` (707-712).
  - Rotate the output directory every MAX_FILES_PER_DIR=500 files (713-716); increment the type's recovered counter.
  - PFSTATUS_OK_TRUNCATED marks files cut by a following header; `file_block_truncate` (photorec.c:1308-1340) returns excess blocks to the search space.
- Per-pass/final statistics: `write_stats_log` prints "ext: n/m recovered" sorted by count (photorec.c:450-489); `info_list_search_space` prints "N sectors contain unknown data, M invalid files found but saved|and rejected" (photorec.c:804-837).

### 2.4 Header/footer matching details
- Header: signature match at the block start (offset+length as registered; the index only dispatches by first byte — the full memcmp runs in the loop, photorec_check_header.h:207-208).
- Footer: type finalizers search **backwards in 4 KiB steps** from the current end for a footer string — `file_search_footer` / `file_rsearch` (filegen.c:380-449) — e.g. used for formats without size fields.
- Size-field driven: types set `calculated_file_size` in data_check; `data_check_size` (filegen.c:451-464) returns DC_STOP when the window passes it; `file_check_size/_min/_max` (466-490) clamp or zero the file at finalization.
- Next-header termination: any new recognized header ends the current file (photorec_check_header.h:106-116).
- Max-size termination: per-type max_filesize plus the 2 TiB absolute cap and the FAT 4 GiB cap (psearchn.c:225-240).

### 2.5 Brute-force pass for fragmented files (phbf.c)
- `photorec_bf` (phbf.c:124-320) walks the remaining search space **backwards** (list tail → head) over elements still tagged with a file_stat (headers that never produced a valid file).
- For each: the same check/append loop; when `file_finish_bf` fails (rejected with offset_error != 0), call `photorec_bf_aux` → `photorec_bf_frag`.
- `photorec_bf_frag` (phbf.c:540-725):
  - Walks candidate split points backwards by blocksize while within 6 sectors / 2 blocks of the error offset.
  - For each split, tries **skipping −2 … 5000 blocks** (blocs_to_skip loop, 616-623); negative skips jump by *next file header* (get_next_header, pnext.h:37-48) rather than by sector (665-682).
  - Outer phases 0/1/2 apply different effort limits (skip<16 / <100 / <10 respectively, phbf.c:619-621).
  - `photorec_bf_pad` (322-446) appends up to `offset_error + 1000*blocksize` (with data_check) or `+100*blocksize` (without), re-running file_check until it validates (398-415).
  - Recursion depth: **frag > 5 → give up** (519-521, 687-689).
  - `photorec_bf_frag_fast` (448-538) uses the type's `extra` hint (gap between offset_ok and offset_error) to try only plausible gap sizes first.
- **Fragmentation policy (explicit)**:
  - Normal passes: a file **stops at the first gap** — a new header, a failed data_check, or a non-contiguous region ends it (possibly truncated).
  - Only the optional brute-force pass (Paranoid = "Yes (Brute force enabled)") attempts multi-fragment reassembly, with the bounded search above (skip ≤ 5000 blocks, ≤ 5 fragments, 2 phases + fast path).

### 2.6 Whole/free-space, options
- **Whole vs Free**: after partition selection, PhotoRec asks (pfree_whole.c:38-151): ext2-mode ("E ext2/ext3" vs "O Other"), then Free/Whole with per-FS wording ("Scan for files from FAT32 unallocated space only" etc.).
  - Free-only implementation `remove_used_space` (photorec.c:401-420): FAT12/16/32 (cluster walk), exFAT, NTFS (libntfs), ext2/3/4 (libext2fs) — each only when the library is compiled in; ext2 mode can also init by group/inode (ext2grp.c; INIT_SPACE_EXT2_GROUP/INODE handled in phcli.c:170-179).
  - Free-only misses files in allocated-but-unused clusters and (obviously) overwrites nothing — it only shrinks the search space.
- Options menu (phrecn.c:505-572):
  - **P** Paranoid: No / Yes (Brute force disabled) / Yes (Brute force enabled).
  - **K** Keep corrupted files (Yes/No).
  - **S** ext2/ext3 (indirect block) mode.
  - **E** Expert mode (extra prompts: blocksize, unformat, root cluster, image_remaining.dd).
  - **L** Low memory.
- `paranoid` semantics: 0 = never run file_check validators; 1 = run validators and reject invalid files; 2 = additionally run the brute-force passes (status_inc photorec.c:1011-1013, 1024-1031).
- `keep_corrupted_file=1` adds a final STATUS_*_SAVE_EVERYTHING pass that re-recovers the rejected leftovers with the 'b' filename prefix, without validation (set_filename photorec.c:855-856; file_finish_aux skips file_check in that status, photorec.c:660-663).
- Low memory: `forget()` (photorec.c:373-399) frees search-space history older than 10000 elements from the current position (invoked psearchn.c:246-247 and 314-315).

### 2.7 Output naming and directories
- `set_filename` (photorec.c:853-871): `<recup_dir>.<dir_num>/<f|b><7-digit start sector>[.<ext>]`
  - e.g. `recup_dir.1/f0123456.jpg`.
  - The number is the file's **start sector relative to the partition** (location.start - part_offset) / sector_size.
  - 'b' prefix in the save-everything ("broken") pass; extension from `file_recovery->extension`.
- Directory scheme: `photorec_mkdir` (photorec.c:290-332) creates `recup_dir.1` (mode 0775) then increments (`recup_dir.2`, ...) every **500 files** (MAX_FILES_PER_DIR, photorec.h:25; rotation photorec.c:713-716). Default base "recup_dir" (photorec.h:26); `-d path/` appends "recup_dir" (phmain.c:275-291). DOS build guards against >999 directories (photorec.c:322-330).
- Post-recovery rename: `file_rename` (filegen.c:687-849) appends the embedded original filename — non-printable/path chars → '_'; the suffix is dropped when bad characters outnumber good ones (741-798); types like zip/doc use it to recover docx/pptx/etc. names.
- Expert extra: `image_remaining.dd` dump of all remaining unknown sectors at the end (phrecn.c:91, 195-227, 448-485).

### 2.8 Representative signature modules (sampled 15 across categories)
All follow the pattern: `file_hint_t` + `register_header_check_*` + `header_check_*` (+ optional `data_check_*` streaming validator + `file_check_*` finalizer + `file_rename_*`).
- **Images**:
  - file_jpg.c: sig `FF D8 FF` at 0 (2773-2777); hint ext "jpg", max 50 MiB, default on (87-94). header_check_jpg (882-1042): walks markers to SOS (0xDA); requires APP0="JFIF/JFXX" (1023-1029) and APP1="Exif" (1030-1036); validates DHT; suppresses false positives — thumbnails inside JPG (file_size<=16384 & small SOF dimensions, 961-982), JPGs embedded in indd/doc(albm/prt)/AVI/MOV/RW2 (946-1020). file_check_jpg searches the FFD9 EOI and parses MPO multi-picture entries (file_check_mpo_be, 168+).
  - file_png.c: 8-byte magics at 0 for PNG (89 'PNG' \r\n\x1a\n), MNG (0x8a), JNG (0x8b).
  - file_tiff.c: `II*\0` / `MM\0*` at 0, 1 GiB cap; per-endian helpers file_tiff_be.c/file_tiff_le.c walk IFDs to compute the exact size.
- **Documents**:
  - file_doc.c: OLE2 `D0 CF 11 E0 A1 B1 1A E1` (8 bytes at 0); distinguishes doc/xls/ppt/vsd/msi... by directory entries.
  - file_pdf.c: `%PDF-1` (6 bytes); file_check_pdf searches `%%EOF` / `startxref` tails.
- **Archives**:
  - file_zip.c: `PK\x03\x04` + WinZIPv8 `PK00PK\x03\x04`; header_check_zip sniffs the first entry name to rename to docx/odt/jar/epub...; file_check_zip (1055+) walks local headers + end-of-central-directory to compute size and reject bogus streams.
  - file_7z.c: `7z\xbc\xaf\x27\x1c`.
  - file_rar.c: `Rar!\x1a\x07\x00` (v1.5) and `Rar!\x1a\x07\x01\x00` (v5.0).
  - file_tar.c: `ustar\0` (GNU) and `ustar  \0` (POSIX) at **offset 0x101**; `is_valid_tar_header` reused by the scan loop (photorec_check_header.h:181).
- **Video**:
  - file_mpg.c: `00 00 01 B3 / B5 / BA / BB` at 0; `data_check_mpg` advances by parsed MPEG packet sizes, DC_STOP on a broken packet (153-176); file_check_size.
  - file_mov.c: 'moov'/'mdat'/'ftyp' at offset 4 (+ dedicated hint registering "mdat" at offset 4).
- **Audio**:
  - file_mp3.c: frame syncs `FF FA / FF FB / FF F2 / FF F3` (+ ID3 handling).
  - file_ogg.c: "OggS".
  - file_gz.c: `1F 8B 08`; file_check_gz uses ISIZE / footer search; renames to .tar.gz etc.
- **Heuristic/text**: file_txt.c — many literal heads (`<?xml`, UTF-8/UTF-16 BOM forms, ...) plus a general printable-text heuristic with per-format extension selection; newline tolerance via `file_allow_nl` (filegen.c:340-378).

### 2.9 Session persistence, DFXML, JSON logs
- `photorec.ses` (SESSION_FILENAME, sessionp.c:57): saved after every pass (phrecn.c:367) and every 5 minutes (psearchn.c:368-369); stores params + remaining search space; loaded for `/cmd <dev> resume` (phmain.c:324-327; session_load sessionp.c:87); **deleted on successful completion** (phrecn.c:425-426).
- Optional DFXML output (photorec.xml; per-file xml_log_file_recovered; ENABLE_DFXML, phrecn.c:294-298).
- JSON-lines progress log (`/logjson file`, json_log.c; json_log_progress every second, psearchn.c:355).

## 3. UX behaviors (menus, logs, CLI modes)

### 3.1 TestDisk (ncurses)
- Flow: disk selection → partition-table type selection → main menu.
- Main menu (tdiskop.c:104-115): **A** Analyse, **T** Advanced (Filesystem Utils), **G** Geometry, **O** Options, **C** MBR Code, **D** Delete, **Q** Quit, **E** Editor. arch_none disks go straight to Advanced (tdiskop.c:116-120).
- Analyse screen (tanalyse.c:41-105): shows the current structure; options **Q** Quick Search / **B** Backup (writes `backup.log` via partition_save, savehdr.c; intrface.c:380-386).
- Structure display (intrface.c:108-417): colorized valid/invalid (msg_STRUCT_OK / msg_STRUCT_BAD from `arch->test_structure`); keys: **A** add partition, **T** change type, **P** list files, **L** load backup.log, **B** save backup, ←/→ change status (Primary bootable / Primary / Logical / Extended / Deleted for i386; legend string per arch, e.g. parti386.c:362), Enter continue.
- Write screen (tpartwr.c:40-129): Enter "Locate more", **W** Write (Y confirmation; then "You will have to reboot for the change to take effect.", godmode.c:1433-1449), **S** Deeper Search (only when can_search_deeper), **E** extended min/max toggle (godmode.c:1466-1469).
- Advanced menu (adv.c:450-633): options depend on the FS (adv.c:284-319): **t** Type, **b** Boot (dispatch to fat32/fat1x/ntfs/exfat/HFS boot-sector screens, adv.c:322-363), **s** Superblock ("Locate ext2/ext3/ext4 backup superblock" for linux parts; HFS backup volume header), **l** List (io_redir serves a backup boot sector as the primary when sb_offset != 0, adv.c:396-434), **u** Undelete (NTFS undelete via libntfs), **c** Image Creation (`image.dd` of the partition, adv.c:365-392), **q** Quit.
- Logs: `testdisk.log` when `/log` (testdisk.c:283-289); full partition dumps and warnings; `backup.log` for partition-list backups.
- CLI (testdisk.c:103-339): `testdisk [/log] [/debug] [/dump] [/logname f] [/nolog] [/all] [/backup] [/direct] [/list|-l] [-lu] [/nosetlocale] [/safe] [/saveheader] [/cmd device "commands"] [image.dd|file.e01|device]`.
  - `/list` prints the partition table non-interactively (interface_list, intrface.c:54-78).
  - `/cmd` drives the ncurses state machine from a comma-separated command string; words parsed by adv_string_to_command (adv.c:207-259): type / addpart / boot / copy / list / undelete / superblock / <partition order number>.

### 3.2 PhotoRec (ncurses + CLI)
- Disk selection (pdiskseln.c:92-100): P/N navigate, **O** Proceed, **S** Sudo (restart as root via sudo when built with SUDO_BIN, phmain.c:496-507), **Q** Quit.
- Partition menu (ppartseln.c:82-90): **S** Search, **O** Options, **F** File Opt, **G** Geometry, **Q** Quit. A pseudo "Whole disk" partition is always appended (new_whole_disk photorec.c:505-513; init_list_part photorec.c:1044-1059).
- File Opt (phrecn.c:578-722): checkbox list of all 353 types ("[X] jpg  JPG picture"); **s** toggles default/all-off; **b** saves settings to ~/.photorec.cfg; arrows/PgUp/PgDn navigate.
- Options (phrecn.c:505-572): as §2.6.
- Progress screen with a "Stop" button (phrecn.c:311-322); SIGINT/SIGHUP/SIGTERM set need_to_stop → graceful stop, session save, Y/N "really Quit" prompt (phmain.c:101-126; phrecn.c:405-422).
- End screen (phrecn.c:98-155): "N files saved in <dir>"; status: Recovery completed / aborted by the user / cannot create file (EACCES) / no space left; ENOSPC asks for a new destination (phrecn.c:372-399).
- CLI: `photorec [/log] [/logjson f] [/debug] [/d recup_dir] [/cmd device "commands"] [file.dd|file.e01|device]` (phmain.c:128-139).
  - `/cmd` language: `search`; `status=<unformat|find_offset|ext2_on|ext2_on_bf|ext2_on_save_everything|ext2_off|...>` (phrecn.c:242-274); `options,<paranoid|keep_corrupted_file|...>`; per-format `<ext>,enable|disable` and `everything,enable|disable` (phcli.c:73-144); numeric start sector (set_search_start photorec.c:912-928); `resume`.
  - Defaults (phmain.c:182-190): paranoid=1, keep_corrupted_file=0, mode_ext2=0, expert=0, lowmem=0, verbose=0.
- fidentify (src/fidentify.c): non-interactive "which signature is this file" helper; libFuzzer harness `fuzzerfidentify` (src/Makefile.am:37, 450-452). qphotorec (qphotorec.cpp + qphbs.cpp + qpsearch.cpp): Qt re-implementation of the same pipeline.

## 4. Tests
- **No test suite directory** — no `tests/`, no check_PROGRAMS, no .github CI in tree. NOT FOUND: unit tests, golden-image regression tests.
- Testing infrastructure that does exist:
  1. **SINGLE_FORMAT standalone builds**: every file_*.c is compilable standalone (`#if !defined(SINGLE_FORMAT) || defined(SINGLE_FORMAT_xxx)` guards, file_list.c:31-1087) and **16 modules embed a `MAIN_*` main() harness** (e.g. file_jpg.c:2801-2803 `#if defined(MAIN_jpg) #define BLOCKSIZE 65536u int main()...`) that runs the carver against a sample file — a per-format smoke test.
  2. `fuzzerfidentify.cpp` (EXTRA_PROGRAMS, src/Makefile.am:37): fuzzes signature identification.
  3. **Frama-C contracts** throughout (`/*@ requires ... */`, e.g. filegen.h:119-237) for static analysis; sonar-project.properties present.
  4. documentation.html / README_dev_photorec.txt — documentation, not tests.
- Coverage implication: behavioral claims here come from code reading, not from a test oracle; the MAIN_* harnesses indicate which formats have at least a compile-and-run smoke path.

## 5. Coverage log (read fully / sampled / enumerated-only — precise and honest)
- **Read fully (skipping only Frama-C annotation blocks)**:
  - PhotoRec engine: photorec.c (1418 lines), phrecn.c (723), phbs.c (229), phbf.c (756), psearchn.c (383), photorec_check_header.h, filegen.c (1204), filegen.h, photorec.h, phcfg.c (245), phmain.c (relevant 88-517), pfree_whole.c, pnext.h, phcli.c (file_select_cli + menu sections), sessionp.c (grep-verified: 57, 87, 200, 374).
  - Registry: file_list.c structure + full mechanical counts (353 externs; 353 entries; all `.enable=0`; NULL terminator).
  - TestDisk engine: godmode.c (all 1541 lines), analyse.c (459), next.c (228), tanalyse.c, intrface.c, partgpt.c (623), partgptw.c (write path 218-312), ntfs.c (431), tntfs.c (303), ntfs_fix.c (320), btrfs.c, ext2.c (recover_EXT2 section), geometry.c (173), adv.c (633), testdisk.c (CLI + help), tdiskop.c (menu), fat32.c (menu/scan section), fat1x.c (menu).
  - parti386.c: read 82-600 and 650-860 and 1177-1405 fully (structs, type table, geometry, read, EBR chain, overlap, write entry, entry conversion, can_be_ext, test_structure); remaining write helpers sampled.
  - fat.c: set_FAT_info / log helpers / check_FAT / recover_FAT (990-1073) / OS2MB read fully; cluster-walk helpers sampled.
  - fat_adv.c: rebuild-BS section 1955-2219 and menus 922-935, 1821-1835 read; the rest (repair FAT internals) sampled via grep.
- **Sampled (representative signature modules — 15 across categories, exceeding the required 12)**: file_jpg.c (deep read of hint/register/header_check/MPO/thumbnail logic), file_png.c, file_tiff.c, file_doc.c, file_pdf.c, file_zip.c, file_7z.c, file_rar.c, file_tar.c, file_mpg.c, file_mov.c, file_mp3.c, file_ogg.c, file_gz.c, file_txt.c — each read for file_hint_t, register_header_check body, and the main header_check logic.
- **Enumerated mechanically (not read line-by-line)**: all 353 file_*.c counted; 285 default-enabled counted via rg; 423 header_check_* functions and 930 register_header_check() calls counted; file_tiff_be/le confirmed as helper modules (no own file_hint rows); file_sig.c / file_found.c confirmed auxiliary.
- **Not read (gaps)**: qphotorec.cpp UI internals (architecture only), dir.c/dirn.c file-copy browser, fat_dir.c/exfat_dir.c/ntfs_dir.c/ext2_dir.c directory parsers, hdaccess.c/hdwin32.c OS disk layers, ewf.c (E01), io_redir.c internals (behavior taken from call sites in tntfs.c/adv.c/ntfs_fix.c), rfs/hfs/hfsp/xfs/ufs/zfs/jfs/gfs2 recover functions beyond their analyse.c registrations, man pages, po/ translations. No claim in this sheet relies on these files.

## 6. Harvest notes (strongest behaviors, licensing constraints per feature)
**Top harvestable behaviors** (re-implementable from §1-§2; formats/offsets are on-disk facts, but TestDisk's thresholds and structure choices are creative expression — re-derive, don't translate):
1. **Two-level signature index** (offset-node → 256 first-byte buckets; sorted, first-match-wins) — filegen.c:159-288. O(1) dispatch per block.
2. **Auto block-size detection**: quick safe-header-only pre-pass, stop at 10 headers, halve 64 KiB until all starts are congruent — phbs.c:76-229 + photorec.c:515-560.
3. **Progressive pass machine** (find-offset → carve → optional brute-force → optional save-everything) driven by the paranoid/keep-corrupted matrix — photorec.c:998-1042, phrecn.c:230-502.
4. **Search-space interval algebra**: subtraction of carved ranges, block-granular allocation tracking with data/metadata flags, truncation returning blocks — photorec.c:110-244, 1061-1340.
5. **Backup boot-sector workflows**: NTFS backup at partition end-1 sector; FAT32 backup at sector 6 (identity check ignoring 0x3E8-0x3F0); io_redir to *use* a backup as primary while listing — tntfs.c, fat32.c:145-172, adv.c:396-434.
6. **MFT mirror adjudication** (size = max(cluster, 4×record); redirect-and-list to pick the good copy; mirror-over-MFT flagged DANGEROUS NON REVERSIBLE) — ntfs_fix.c:53-320.
7. **Deep-scan sector lattice**: head 0/1/2 sector-1 + 1 MiB alignment + FS-specific backup offsets (FAT sector 7, exFAT sector 13, NTFS/HFS last-of-track, ext2 group-3 backups for 1K/2K/4K blocks) + hint list (partition ends, NTFS-backup back-projection, RAID follow-ups) + jump-to-partition-end in quick mode — godmode.c:387-993, next.c:112-227.
8. **GPT dual-header read + dual write**: primary failover to last-LBA header; write re-creates both entries arrays + both headers + protective MBR, preserving the disk GUID — partgpt.c:378-389, partgptw.c:218-312.
9. **MBR/EBR chain validation** (32-logical cap; one data + one link per EBR; bounds containment) and extended-partition re-synthesis (min/max container; P_EXTENDX when end cylinder > 1023) — parti386.c:661-803, godmode.c:1130-1300.
10. **Candidate acceptance rules**: is_part_known + part_size>1 + offset>=min_location + end<=search_location_max; per-FS sanity tests as the only "scoring"; geometry mismatches are warnings — godmode.c:853-938, ntfs.c:144-205.
11. **Output naming & rotation**: recup_dir.N, 'f'/'b' + 7-digit sector id, 500 files/dir, embedded-filename rename with printable-char scoring — photorec.c:290-332, 853-871; filegen.c:687-849.
12. **Session/resume**: photorec.ses every pass + 5 min, resume command, delete on completion — sessionp.c, phrecn.c:367-426.
13. **FAT BS rebuild heuristics**: FAT-location score table, cluster-size inference, dir_entries scan, root-cluster discovery — fat_adv.c:1821-2219.

**Licensing constraints (per feature area)**:
- GPLv2+ applies to all source. Magic constants and structural offsets of on-disk formats (0xAA55, "NTFS", "EFI PART" + CRC32 fields, ext2 0xEF53 at +1024, btrfs magic at +64 KiB, FAT32 backup at sector 6, MFTMirr semantics, JPEG/TIFF/ZIP signatures...) are uncopyrightable facts — free to re-use.
- TestDisk/PhotoRec's *creative choices* must be re-expressed independently: thresholds (10-header stop, 500 files/dir, 3-fragment/200 MB backtrack, 5000-skip brute-force bound, frag≤5 recursion, 32-logical cap, 10000-element forget), the registry table's descriptions, code organization, and the exact log/UX strings. Cite this sheet, not their code.

**Anomalies / caveats**:
(a) file_list.c ships all `.enable=0`; defaults are applied at runtime — reading the source table alone understates the default-on set (285/353).
(b) file_tiff_be.c/file_tiff_le.c are shared checkers, not registry rows — registry size is 353, matching module count.
(c) Fragmented-file recovery exists only in the paranoid=2 brute-force path and is a bounded search (skip ≤ 5000 blocks, ≤ 5 fragments), not general fragmented carving.
(d) `find_blocksize` FIXME: NTFS 2 MiB clusters (Windows 10 Creators) not yet handled (photorec.c:517-519).
(e) DOS build: photorec_mkdir loop caps at 999 retries (photorec.c:322-330).
(f) GPT GUID generation uses a weak srand(time) by design (phmain.c:201-202) — acceptable for GUID uniqueness but not cryptographic.
(g) No unit tests in-repo (see §4) — validation of any re-implementation must be built fresh.
