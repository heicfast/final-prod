# SLEUTH KIT — Feature Context Sheet
> Agent E (Task 2-E), Phase-0. READ-ONLY extraction at HEAD 3dcb43668d811548206fbeaf5e0c9753202f672f, IPL 1.0 core — behavioral extraction only. Date 2026-11-19. Every claim cites file:line.
> Repo root: `/home/z/my-project/repos/sleuthkit` (github.com/sleuthkit/sleuthkit). No git commands run; static reading only.

## 0. Identity (URL, SHA, layout, LOC estimate, license map)

- **URL**: sleuthkit/sleuthkit (README.md present at repo root).
- **HEAD**: 3dcb43668d811548206fbeaf5e0c9753202f672f (given by task; not verified via git — git commands not allowed).
- **Layout** (from LS of repo root):
  - `tsk/` — C/C++ core library.
    - `base/`: errors, unicode conversion, md5/sha1, CRC, locks, stacks/lists, printf helpers, endian guessing, XGetopt.
    - `img/`: image formats (raw, ewf, aff, vmdk, vhd, logical, external).
    - `vs/`: volume/partition systems. **NOTE: the task brief said `tsk/mm/`; the actual directory is `tsk/vs/`** (files keep the `mm_*.c` prefix, e.g. `mm_open.c`).
    - `fs/`: filesystem modules (ntfs, fat/exfat, ext2/3/4, ffs/ufs, hfs, iso9660, yaffs2, apfs, rawfs, swapfs, logical) + shared tool libs (`fls_lib.c`, `ils_lib.c`, `icat_lib.c`, `dls_lib.c`, `dcalc_lib.c`, `dstat_lib.c`, `ifind_lib.c`, `ffind_lib.c`, `usnjls_lib.c`).
    - `pool/`: storage pools — APFS container + Linux LVM (`tsk_pool.h/.hpp`, `apfs_pool*.cpp`, `lvm_pool*.cpp`).
    - `hashdb/`: hash database formats + indexes.
    - `auto/`: auto-analysis pipeline + SQLite case DB (bundles `sqlite3.c`).
    - `util/`: Bitlocker parser (`util/Bitlocker/`), encryption detection (`detect_encryption.c`), crypto helpers.
    - `sorter/`: perl `.sort` extension classification configs (linux/windows/freebsd/solaris/openbsd/images/default).
    - `docs/`: doxygen pages (fs.dox, vs.dox, img.dox, hashdb.dox, auto.dox, pool.dox).
  - `tools/` — CLI front-ends.
    - `fstools/`: fls, icat, ils, ifind, istat, ffind, blkls, blkcat, blkcalc, blkstat, fsstat, fscheck, jcat, jls, usnjls, fcat.
    - `vstools/`: mmls, mmstat, mmcat.
    - `imgtools/`: img_stat, img_cat.
    - `hashtools/`: hfind (+ bundled md5.c, sha1.c).
    - `autotools/`: tsk_recover, tsk_loaddb, tsk_gettimes, tsk_comparedir, tsk_imageinfo.
    - `srchtools/`: srch_strings, sigfind.
    - `fiwalk/`: fiwalk + per-format plugins (jpeg/docx/odf/word extract, clamscan).
    - `timeline/`: mactime (perl, `mactime.base`).
    - `sorter/`: sorter (perl, `sorter.base`).
    - `pooltools/`: pstat.
    - `logicalimager/`: rule-based triage tool with a mini registry parser (RegParser.cpp/h) and JSON config.
  - `rejistry++/` — standalone Windows registry (REGF) parser library.
  - `bindings/java/` — Java datamodel + JNI (`SleuthkitCase`, `AbstractFile`, timeline, blackboard artifacts, etc.).
  - `tests/` (C++ API regression), `unit_tests/base/` (cppunit), `test/tools/autotools/test_loaddb.sh`,
    `samples/` (posix-style, callback-style + C++ variants), `man/` (25 man pages), `win32/` (MSVC projects),
    `m4/`, `debian/`, `licenses/`, `db_diff/` (tskdbdiff.py).
- **LOC estimates** (wc -l over C/C++/H/hpp/perl):
  - `tsk/` ≈ **271,000** total — but this includes the ~250k-line bundled `tsk/auto/sqlite3.c`.
  - `tools/` ≈ **46,000** (C/C++/perl/.base scripts, includes fiwalk's bundled sha2/base64).
  - `rejistry++/src` ≈ **6,050**; `bindings/java` ≈ **78,700** lines of Java.
  - Hand-written C/C++ core (tsk/ minus sqlite3 + tools) ≈ **~65–70 kLOC**.
  - 959 tracked files (given by task).
- **License map** (licenses/README.md, checked per component):
  - Core `tsk/` + `tools/`: **IBM Public License 1.0** (inherited from TCT, Wietse Venema 1997-99; `licenses/IBM-LICENSE`), with many files dual-stamped **Common Public License 1.0** (`licenses/cpl1.0.txt`) — e.g. fs_dir.c header "Common Public License 1.0" (tsk/fs/fs_dir.c:8), tsk_vs.h:7-8.
  - `bindings/java/src/org/sleuthkit/datamodel/*` and `rejistry++/*`: **Apache 2.0** (licenses/README.md:47-61, 129-135).
  - `tools/fiwalk/*` + `tools/fiwalk/plugins/*`: **public domain** (NPS / Simson Garfinkel; licenses/README.md:249-263; fiwalk.cpp:19-31).
  - `tsk/auto/sqlite3.c/.h`: public domain (licenses/README.md:153-164).
  - `tsk/fs/lzvn.c`: Apple 3-clause BSD (licenses/README.md:233-239). `tsk/auto/guid.cpp`: MIT (75-81). `samples/*`: 3-clause BSD (137-143).
  - `tools/srchtools/srch_strings.c`: **GPLv2** (241-247). `tsk/base/md5c.c`: RSA terms; `sha1c.c`, `crc.c`, `XGetopt.c`: public domain; `tsk_unicode.c`: Unicode Inc. terms.
  - ⚠️ IPL 1.0 / CPL 1.0 are not GPL-compatible; **behavioral re-implementation only** is the safe path for core code; do not copy source, struct layouts verbatim, or comments.

## 1. Filesystem abstraction (object model + plug-in mechanism + allocated/deleted semantics)

### 1.1 Object model (tsk/fs/tsk_fs.h)
All public structs are tag-checked with magic constants (e.g. `TSK_FS_FILE_TAG 0x11212212` tsk_fs.h:687,
`TSK_FS_DIR_TAG 0x57531246` tsk_fs.h:633, `TSK_FS_INFO_TAG 0x10101010` tsk_fs.h:940).

**TSK_FS_INFO** (tsk_fs.h:952-1057) — open filesystem handle. State:
- `{img_info, offset}`: image pointer + byte offset where the FS starts (tsk_fs.h:955-958).
- Metadata geometry: `inum_count`, `root_inum`, `first_inum`, `last_inum` (tsk_fs.h:961-964).
- Content geometry: `block_count`, `first_block`, `last_block`, `last_block_act` (adjusted for truncated
  images, tsk_fs.h:969-970), `block_size`, `dev_bsize` (967-972).
- RAW-CD padding: `block_pre_size` / `block_post_size` bytes before/after each sector (tsk_fs.h:974-979).
- `journ_inum` (journal inode, 982); `ftype` (TSK_FS_TYPE_ENUM) and `duname` (block-unit name string, 984-985).
- `flags` ∈ {HAVE_SEQ, HAVE_NANOSEC, ENCRYPTED} (tsk_fs.h:919-925); `fs_id[32]` boot-sector ID (987-988); `endian` (990).
- Orphan machinery: `list_inum_named` (TSK_LIST of unalloc inodes pointed to by names, filled after a full
  name walk) + `orphan_dir` (cached $OrphanFiles TSK_FS_DIR), each under its own lock (tsk_fs.h:992-1007).
- `encryption_type` / `encryption_data` (tsk_fs.h:1001-1003), `impl` opaque impl pointer (1056).

**TSK_FS_FILE** (tsk_fs.h:695-702) — THE name↔meta join:
```
struct TSK_FS_FILE { int tag; TSK_FS_NAME *name; TSK_FS_META *meta; TSK_FS_INFO *fs_info; };
```
Header comment (tsk_fs.h:688-694): "When deleted files are being processed, this structure may have the
name defined but not metadata because it no longer exists. Or, if you are calling meta_walk … the name
will not be defined. always check these to make sure they are not null." — **name and meta allocation
flags are independent**; a deleted name can point to a reallocated (still-allocated) meta structure.

**TSK_FS_NAME** (tsk_fs.h:601-618) — directory entry:
- `name` / `shrt_name` (UTF-8, 8.3 short name), `meta_addr` + `meta_seq` (seq NTFS-only), `par_addr` +
  `par_seq`, `date_added` (APFS only, tsk_fs.h:614).
- `type`: TSK_FS_NAME_TYPE_ENUM — FIFO/CHR/DIR/BLK/REG/LNK/SOCK/SHAD(Solaris)/WHT(OpenBSD)/VIRT/VIRT_DIR
  (tsk_fs.h:571-584), with 1-char string table `tsk_fs_name_type_str` (tsk_fs.h:586-589).
- `flags` ∈ {ALLOC=0x01, UNALLOC=0x02} (tsk_fs.h:562-565).

**TSK_FS_META** (tsk_fs.h:456-517) — inode equivalent:
- `flags` ∈ {ALLOC=0x01, UNALLOC=0x02, USED=0x04, UNUSED=0x08, COMP=0x10, ORPHAN=0x20} (tsk_fs.h:373-380).
- `type`: TSK_FS_META_TYPE_ENUM incl. VIRT (virtual files TSK synthesizes for FS areas) and VIRT_DIR
  (virtual dirs like $OrphanFiles) (tsk_fs.h:395-408); `TSK_FS_IS_DIR_META()` treats DIR+VIRT_DIR as dirs (414).
- `mode` (Unix permission bits, 416-436), `nlink`, `size`, `uid`, `gid`.
- Four standard times **mtime/atime/ctime/crtime + `_nano` each** (tsk_fs.h:470-477).
- FS-specific `time2` union (tsk_fs.h:479-500): ext2 `dtime` (+nano), hfs `bkup_time` (+nano),
  ntfs `fn_crtime/fn_mtime/fn_atime/fn_ctime` (+nanos) + `fn_id` (attribute id the FN times came from).
- `content_ptr`/`content_len`/`content_type` — FS-specific content (ext direct blocks or extent tree or
  inline data: TSK_FS_META_CONTENT_TYPE_{DEFAULT, EXT4_EXTENTS, EXT4_INLINE}, tsk_fs.h:438-442, 502-505);
  optional `reset_content` cleanup callback (505); `seq` (NTFS sequence, incremented on realloc, 506).
- `attr` TSK_FS_ATTRLIST + `attr_state` ∈ {EMPTY, STUDIED, ERROR} (383-388, 508-513) — attributes are
  **lazily loaded** through the FS `load_attrs` hook.
- `name2` TSK_FS_META_NAME_LIST — names stored *with* the metadata ("currently used for NTFS and FAT
  file systems only", tsk_fs.h:351-364, 515); `link` symlink target (516).

**TSK_FS_ATTR / TSK_FS_ATTRLIST / TSK_FS_ATTR_RUN** (tsk_fs.h:146-336):
- `TSK_FS_ATTR`: `type` (TSK_FS_ATTR_TYPE_ENUM), `id` (NTFS instance), `name` (ADS name), `flags` ∈
  {INUSE, NONRES, RES, ENC, COMP, SPARSE, RECOVERY} (tsk_fs.h:179-188).
- Attribute types are NTFS-numbered (tsk_fs.h:224-260): 0x10 $SI, 0x20 $ATTRLIST, 0x30 $FNAME, 0x40 $OBJID,
  0x50 $SEC, 0x60 $VNAME, 0x70 $VINFO, 0x80 $DATA, 0x90 $IDXROOT, 0xA0 $IDXALLOC, 0xB0 $BITMAP, 0xC0
  $REPARSE, 0xD0 $EAINFO, 0xE0 $EA, 0x100 $LOG; HFS+ forks at 0x1100+ (DATA/RSRC/EXT_ATTR/COMP_REC);
  `TSK_FS_ATTR_TYPE_UNIX_INDIR=0x1001` and `UNIX_EXTENT=0x1002` for UFS indirect blocks / ext4 extents;
  APFS aliases the HFS+ values (255-259). `TSK_FS_ATTR_ID_DEFAULT 0` (262).
- Resident data: `rd.buf/buf_size/offset` (tsk_fs.h:312-316).
- Non-resident data: `nrd.run` linked list + `nrd.run_end` (299-301); size model
  `skiplen/allocsize/initsize/compsize` (302-305) with ASCII diagram (tsk_fs.h:277-282):
  allocsize ≥ size ≥ initsize; allocsize defines slack; initsize = bytes actually initialized (rest reads 0).
- `TSK_FS_ATTR_RUN` (tsk_fs.h:164-171): `{next, offset (file-block), addr (fs-block), len, crypto_id
  (XTS IV base), flags ∈ {FILLER, SPARSE, ENCRYPTED}}` (148-153); entries must be in file-offset order.
- Special files (compressed/encrypted) install custom `r` and `w` function pointers on the attribute
  (tsk_fs.h:318-322).

**TSK_FS_DIR** (tsk_fs.h:637-650): open directory = `fs_file` (dir itself) + `names[]` array
(`names_used`/`names_alloc`) + `addr` + `seq` (NTFS) + `fs_info`.

**TSK_FS_BLOCK** (tsk_fs.h:102-108): `{buf, addr, flags}`; block flags ∈ {ALLOC, UNALLOC, CONT, META, BAD,
RAW, SPARSE, COMP, RES} (tsk_fs.h:68-80) — RAW/SPARSE/COMP/RES are file_walk-level facts (62-66).

### 1.2 Plug-in mechanism (the function table)
A FS module embeds TSK_FS_INFO as the first member of its INFO struct (e.g. NTFS_INFO in tsk_ntfs.h),
fills geometry, and assigns ~20 function pointers (tsk_fs.h:1009-1056):
- `block_walk`, `block_getflags` (1009-1011)
- `inode_walk` (meta walk) (1013)
- `file_add_meta` — populate FS_FILE->meta for an inode address (1015)
- `get_default_attr_type`, `load_attrs` (lazy attribute loader) (1017-1019)
- `decrypt_block` (1021)
- `istat(fs, flags, hFile, inum, numblock, sec_skew)` (1035-1036)
- `dir_open_meta(fs, &fs_dir, inode, recursion_depth)` — populate a TSK_FS_DIR (1038)
- `jopen` / `jblk_walk` / `jentry_walk` — journal access (1040-1044)
- `fsstat` (1046), `name_cmp` (FS-specific name comparison, 1048), `fscheck` (1050), `close` (1052),
  `fread_owner_sid` (1054), `impl` (1056).
- Structures must be allocated by `tsk_fs_malloc` / freed by `tsk_fs_free`, which init/destroy the two
  locks (fs_open.c:331-376).

**Registration** = static array in `tsk_fs_open_img_decrypt()` (fs_open.c:132-150):
```
{ "NTFS", ntfs_open, TSK_FS_TYPE_NTFS_DETECT }, { "FAT", fatfs_open, TSK_FS_TYPE_FAT_DETECT },
{ "EXT2/3/4", ext2fs_open, TSK_FS_TYPE_EXT_DETECT }, { "UFS", ffs_open, TSK_FS_TYPE_FFS_DETECT },
{ "YAFFS2", yaffs2_open, ... }, [HFS if TSK_USE_HFS], { "ISO9660", iso9660_open, ... },
{ "APFS", apfs_open_auto_detect, TSK_FS_TYPE_APFS_DETECT }
```
- Auto-detect calls **every** opener; if ≥2 succeed it closes both and fails with `TSK_ERR_FS_MULTTYPE`
  ("%s or %s") (fs_open.c:190-210).
- BitLocker errors (`TSK_ERR_FS_BITLOCKER_ERROR`) are saved and re-raised after the loop so a later FAT
  open can still win (fs_open.c:213-235).
- Nothing matched: at offset 0, `detectUnsupportedImageType` names known unsupported formats
  (fs_open.c:242-252); then `detectVolumeEncryption` → `TSK_ERR_FS_ENCRYPTED` (signature),
  `TSK_ERR_FS_POSSIBLY_ENCRYPTED` (entropy), or `TSK_ERR_FS_UNKTYPE` (fs_open.c:254-275).
- Explicit-type dispatch: fs_open.c:279-312 (ntfs/fat/ffs/ext/hfs/iso9660/raw/swap/yaffs2/apfs).
- Logical images force the synthetic logical FS (fs_open.c:158-171).
- Entry points: `tsk_fs_open_img_decrypt` / `tsk_fs_open_vol_decrypt` (vol offset =
  `part_info->start * block_size + vs->offset`, fs_open.c:88-92) / `tsk_fs_open_pool_decrypt`
  (tsk_fs.h:1061-1072).
- Type-name table: fs_types.c:41-69 (ntfs, fat, ext, iso9660, hfs, yaffs2, apfs, logical, ufs, raw, swap,
  fat12/fat16/fat32/exfat, ext2/ext3/ext4, ufs1/ufs2, hfsp/hfsl) + legacy names (`linux-ext*`, `bsdi`,
  `freebsd`, `netbsd`, `openbsd`, `solaris`) at fs_types.c:71-82. Bit values in tsk_fs.h:794-826
  (e.g. EXFAT=0x0a inside FAT_DETECT=0x0e; EXT4=0x2000; APFS=0x10000; LOGICAL=0x20000).

### 1.3 Name+meta combination and allocated/deleted semantics
- `tsk_fs_dir_get()` (fs_dir.c:432-489): copies a TSK_FS_NAME into a fresh TSK_FS_FILE, then calls
  `fs->file_add_meta` **only if** `meta_addr != 0` OR the name flag is ALLOC (fs_dir.c:470-473);
  if `meta->seq != name->meta_seq` the meta is discarded (fs_dir.c:483-486) — NTFS sequence consistency.
- `tsk_fs_file_open_meta()` (fs_file.c:98-135) never sets `name`; `tsk_fs_file_open()` (fs_file.c:149-197)
  resolves path→inum via `tsk_fs_path2inum` then delegates and attaches the name (backfilling
  `fs_name->meta_seq = fs_file->meta->seq`, fs_file.c:184-191).
- `tsk_fs_dir_add()` (fs_dir.c:209-281): dedups by (meta_addr, exact name) — **skipped for FAT** because
  FAT dentries never share meta addresses (fs_dir.c:214-220); when both exist, an UNALLOC stored entry is
  replaced by an ALLOC one (fs_dir.c:235-244); caps at `MAX_DIR_SIZE_TO_PROCESS` with
  `TSK_ERR_FS_LARGE_DIR_ERROR` (fs_dir.c:256-262); sets `par_addr`/`par_seq` from the dir (274-278).
- **Orphan files** (deleted meta with no name): `TSK_FS_ORPHANDIR_INUM(fs) == fs->last_inum`
  (tsk_fs.h:525-530) reserves the last inode for the virtual dir.
  - `tsk_fs_dir_find_orphans()` (fs_dir.c:1354-1476): (1) build `list_inum_named` via a full
    UNALLOC|RECURSE|NOORPHAN dir walk (`tsk_fs_dir_load_inum_named`, fs_dir.c:1145-1177); (2) walk
    UNALLOC|USED meta (fs_dir.c:1407-1409); unnamed entries become `OrphanFile-<inum>` or reuse
    `meta->name2` (fs_dir.c:1271-1280) with `meta_seq = seq - 1` (unalloc MFT entries are incremented,
    fs_dir.c:1282-1286); (3) orphan directories are recursed to mark their contents reachable
    (fs_dir.c:1302-1318) with loop detection (1221-1227); (4) the top-level list is de-duped against that
    reachable set (fs_dir.c:1427-1447); (5) result cached in `fs->orphan_dir` under lock (1455-1466).
  - `$OrphanFiles` gets a synthetic VIRT_DIR name (fs_dir.c:1052-1063) and VIRT_DIR meta (1071-1107).
  - `TSK_FS_ORPHAN_STR "-ORPHAN_FILE-"` prefixes known-name/unknown-parent files (tsk_fs.h:521-523).
- Walk flag sets: BLOCK_WALK {ALLOC, UNALLOC, CONT, META, AONLY} (tsk_fs.h:87-95); META {ALLOC, UNALLOC,
  USED, UNUSED, COMP, ORPHAN} (373-380); DIR_WALK {ALLOC, UNALLOC, RECURSE, NOORPHAN} (656-662);
  FILE_WALK {SLACK, NOID, AONLY, NOSPARSE} (209-215).

## 2. Partition layer (mmls): every system, enumeration model

### 2.1 Abstraction (tsk/vs/tsk_vs.h)
- **TSK_VS_INFO** (tsk_vs.h:65-80): `{tag, img_info, vstype, is_backup, offset (bytes), block_size,
  endian, part_list, part_count, close}` — `is_backup`=1 when the table found was a backup (GPT secondary).
- **TSK_VS_PART_INFO** (tsk_vs.h:103-116): doubly-linked `{prev, next, vs, start (sector, VS-relative),
  len (sectors), desc (UTF-8), table_num, slot_num (each -1 when not from a table), addr (sequential
  number), flags}`.
- **Partition flags** (tsk_vs.h:93-98): ALLOC=0x01, UNALLOC=0x02, META=0x04 (table structures themselves;
  can combine with ALLOC/UNALLOC), ALL=0x07.
- **Supported volume systems** — complete list from `vs_open_table` (mm_types.c:23-33):
  | name | enum | source | description |
  |------|------|--------|-------------|
  | dos | TSK_VS_TYPE_DOS | tsk/vs/dos.c (1150 lines) | DOS Partition Table |
  | mac | TSK_VS_TYPE_MAC | tsk/vs/mac.c | MAC Partition Map |
  | bsd | TSK_VS_TYPE_BSD | tsk/vs/bsd.c | BSD Disk Label |
  | sun | TSK_VS_TYPE_SUN | tsk/vs/sun.c | Sun Volume Table of Contents (Solaris) |
  | gpt | TSK_VS_TYPE_GPT | tsk/vs/gpt.c | GUID Partition Table (EFI) |
- `TSK_VS_TYPE_APFS = 0x0020` exists (tsk_vs.h:56) but is **"Not supported yet"** — `tsk_vs_open`
  rejects it (mm_open.c:227-228); APFS/LVM containers are handled by the separate pool layer
  (`TSK_POOL_TYPE_APFS=0x0001, TSK_POOL_TYPE_LVM=0x0002`, tsk/pool/tsk_pool.h:25-29).
- **AIX volume system: NOT FOUND.** AIX appears only as DOS partition-type *descriptions*:
  "AIX Boot (0x08)" (dos.c:62), "AIX Data (0x09)" (dos.c:65). No AIX LVM VS module at this HEAD.
- `TSK_VS_TYPE_DBFILLER` (tsk_vs.h:57) = fake type for loaddb when an image has no volume system.
- A logical image cannot have a VS at all (mm_open.c:46-51).

### 2.2 Auto-detection and tie-breaking (mm_open.c:60-211)
Order: DOS → BSD → GPT → Sun → Mac, each error-reset on failure.
- **BSD silently overrides DOS** — BSD disklabels begin with the DOS magic, so BSD is preferred when both
  match (comment + commented-out conflict block, mm_open.c:72-90).
- **GPT vs DOS**:
  - If a DOS table was already found and the GPT hit is a *backup* table (`vs->is_backup`), keep DOS
    (mm_open.c:98-105).
  - If the DOS table contains a "GPT Safety" partition starting ≤ sector 63, drop DOS and use GPT
    (mm_open.c:109-130).
  - Otherwise GPT+DOS is a fatal `TSK_VS_MULTTYPE` ("GPT or %s") (mm_open.c:133-141).
- Sun or Mac conflicting with any earlier hit ⇒ MULTTYPE (mm_open.c:151-187).
- Nothing found ⇒ `detectDiskEncryption` (signature only; entropy is handled at fs layer) ⇒
  `TSK_ERR_VS_ENCRYPTED` else `TSK_ERR_VS_UNKTYPE` (mm_open.c:189-208).

### 2.3 Enumeration model
- `tsk_vs_part_add()` (mm_part.c:29-114) inserts **sorted by start sector** and renumbers the `addr` of
  all following entries — partition numbers are disk order, not table order.
- `tsk_vs_part_unused()` (mm_part.c:123-170) synthesizes gap entries described "Unallocated"
  (flag UNALLOC, table=-1, slot=-1) between META-ignoring partitions and after the last partition up to
  `img_info->size / block_size` — **this is how OFFSET/unallocated regions are presented**.
- `tsk_vs_part_walk()` (mm_part.c:240-283): flags==0 ⇒ ALL; iterates the sorted list, callback per
  intersecting entry.
- `tsk_vs_part_read[_block]` reads relative to the volume start (tsk_vs.h:145-148).
- **GPT specifics** (gpt.c): `GPT_HEAD_SIG` validation (gpt.c:125-132); PRIMARY adds META entries
  "Safety Table" (sector 0, 1 sector, gpt.c:135-146) and "GPT Header" (gpt.c:148-161); per-entry size must
  be ≥ sizeof(gpt_entry) (gpt.c:164-172); entry count capped at 8192 (gpt.c:186-187); SECONDARY_TABLE
  path does its own endian guess (gpt.c:113-123).
- **DOS specifics** (dos.c): ~100-entry type→description table in `dos_to_str()` — e.g. 0x07
  "NTFS / exFAT (0x07)" (dos.c:59), 0x0a "OS/2 Boot Manager" (dos.c:68), 0x63 "UNIX System V" (dos.c:266),
  0x82 "Linux Swap / Solaris x86" (dos.c:304), 0x83 "Linux (0x83)" (dos.c:306-308), 0x8e "Linux Logical
  Volume Manager (0x8e)" (dos.c:335-337), 0xee GPT safety; extended partitions walked recursively,
  producing META entries for each EBR.

### 2.4 mmls tool (tools/vstools/mmls.cpp)
- Flags (mmls.cpp:23-49): `-t vstype` (`-t list` prints the supported table), `-i imgtype`, `-b
  dev_sector_size`, `-o imgoffset` (in sectors), `-B` rounded length in bytes, `-r` recurse into
  embedded tables (**DOS only**, single level — mmls.cpp:39, 298-321 with the `recurse` flag cleared
  before recursion), `-a/-A/-m/-M` show allocated / unallocated / meta / hide-meta; default flags = ALL
  (mmls.cpp:250-258).
- Header (mmls.cpp:129-142): type description, `Offset Sector: %d`, `Units are in %d-byte sectors`,
  columns `Slot  Start  End  Length  [Size]  Description`.
- Row format (mmls.cpp:57-127): `%.3d:` partition addr; slot column = `Meta` (META flag), `-------`
  (no table/slot), `%.3d` (slot only), or `%.3d:%.3d` (table:slot); then start, end (start+len-1),
  length, all 10-width; description last; `-B` adds humanized size (B→K→M→G→T, mmls.cpp:80-104).
- Recursion candidates are collected during the walk (ALLOC partitions, cap 64) into `recurse_list`
  (mmls.cpp:19-20, 119-124) and re-opened with `tsk_vs_open(..., TSK_VS_TYPE_DETECT)` (mmls.cpp:304-315).

## 3. Tools and their exact behaviors (fls / icat / blkls / ils / mmls / hfind / fiwalk / mactime formats)

### 3.1 fls (tools/fstools/fls.cpp + tsk/fs/fls_lib.c)
- CLI flags (fls.cpp:29-73): `-a` include "." and ".." (TSK_FS_FLS_DOT), `-d` deleted only (clears
  DIR_WALK ALLOC, fls.cpp:142-144), `-u` undeleted only (clears UNALLOC, fls.cpp:229-231), `-D` dirs only
  (145-148), `-F` files only (161-164), `-l` long listing (177-179), `-m dir/` mactime body format +
  mount prefix (180-183), `-h` MD5 in body output (184-186), `-p` full paths (217-219), `-r` recurse
  (223-225), `-s seconds` skew, `-z ZONE` TZ env, `-b` sector size, `-o` imgoffset, `-f` fstype,
  `-i` imgtype, `-P pooltype -B pool_volume_block -S snap_id`, `-k password`.
- Default walk flags = ALLOC|UNALLOC (fls.cpp:90); dir+file both printed by default (fls.cpp:118).
- FULL auto-enabled when (recurse AND deleted-only) OR files-only (fls.cpp:264-271), and always for `-m`
  (fls.cpp:274-276). `-m` prefix gets a trailing `/` appended (fls.cpp:281-289).
- Final argv element parsed as `inum[-type[-id]]` (tsk_fs_parse_inum) selects the start dir, else root
  (fls.cpp:295-343).
- Rendering dispatch (`printit`, fls_lib.c:49-107): unless FULL, path depth renders as one `+` per `/`
  followed by a space (fls_lib.c:55-68); then `-m` → `tsk_fs_name_print_mac[_md5]` (hash when `-h`,
  else literal `0`), `-l` → `tsk_fs_name_print_long`, else `tsk_fs_name_print`.
- **NTFS special case** (fls_lib.c:129-195): iterates all attributes; one line per `$DATA` attribute —
  **every ADS is a separate fls line** (type-id shown); one line for IDXROOT (the directory itself);
  in `-m` mode one extra line for the FNAME attribute whose id == `meta->time2.ntfs.fn_id`
  (fls_lib.c:176-187) so FILE_NAME times enter the timeline; `..`-relative dir streams suppressed
  unless `-a` (148-159); a file with attributes but no $DATA still prints once (189-193).
- **Default (CS-style) line** (`tsk_fs_name_print`, fs_name.c:405-480):
  `<dentrytype>/<metatype>` from 1-char tables (`r`, `d`, `-`…); meta `NULL` prints `-` for the second
  char; NTFS $DATA on a directory prints `r` (fs_name.c:429-432); `* ` inserted when the **name** is
  UNALLOC (fs_name.c:447-448); inode number; `-<type>-<id>` when an attribute is in play (453-455);
  `(realloc)` marker when name UNALLOC but meta ALLOC (457-460); then (FULL: sanitized path) + name,
  with `:adsname` appended unless IDXROOT/$I30 (473-479). Examples of deleted-name presentations:
  deleted name, no meta → `r/- * 1234: file.txt`; deleted name, live meta → `r/r * 1234 (realloc) …`.
- **Long format** (`tsk_fs_name_print_long`, fs_name.c:492-550): default prefix + TAB-separated
  `mtime  atime  ctime  crtime  size  gid  uid` (order as coded, 516-548); time strings
  `YYYY-MM-DD HH:MM:SS (TZ)` and `0000-00-00 00:00:00 (UTC)` for zero (tsk_fs_time_to_str,
  fs_name.c:304-322); FAT atime printed day-only (FAT stores no access time) (fs_name.c:523-525, 375-389);
  skew subtracted only from non-zero times (517-539); size from ADS when present (541-545).
- **Body (mactime) format** (`tsk_fs_name_print_mac_md5`, fs_name.c:598-764):
  `md5|prefix+path+name[:ads][ ($FILE_NAME)][ -> symlink][ (deleted|-realloc)]|inum[-type-id]|type/ls-mode|uid|gid|size|atime|mtime|ctime|crtime`
  - hash field `0` when none (624-632); names sanitized of control chars (637-647).
  - FNAME rows print the `time2.ntfs.fn_*` times instead of the standard ones (706-736).
  - times printed as raw epoch ints, skew applied only to non-zero (738-762).
  - symlink targets appended after ` -> ` (654-658); `(deleted)` / `(deleted-realloc)` suffix (662-665).
- **Recursion engine** = `tsk_fs_dir_walk` (§1): callback fires when `(name->flags & a_flags) ==
  name->flags` (fs_dir.c:718); recursion into a subdir requires name type DIR-or-UNDEF **and** meta type
  DIR, RECURSE flag, name ALLOC **or** both name+meta UNALLOC, not `./..`, orphan dir only without
  NOORPHAN (fs_dir.c:786-798); infinite-loop protection via seen-inode stack (800-815) + MAX_DEPTH=128 /
  DIR_STRSZ=4096 (521-522, 826-843); `macro_recursion_depth` cap 128 for FS-internal re-entrant walks
  (969-977, added for FAT recursion overflow issue #1859).
- **Root-dir output prioritization** (fs_dir.c:582-646): Users / Documents and Settings / home /
  ProgramData = HIGH; Windows / $Orphan = LOW; pagefile / hiberfil = LAST; everything else MED — user
  content is enumerated first.
- "Redirect handling": no feature by that name; the equivalents are the NTFS ADS lines (above) and the
  symlink `->` target in body mode.

### 3.2 icat (tsk/fs/icat_lib.c + tools/fstools/icat.cpp)
- `tsk_fs_icat()` (icat_lib.c:55-98): `tsk_fs_file_open_meta`, then `tsk_fs_file_walk_type()` when a
  type was given (id omitted ⇒ `TSK_FS_FILE_WALK_FLAG_NOID` matches any id of that type, icat_lib.c:77-86),
  else `tsk_fs_file_walk()` on the default attribute; `icat_action` fwrites each chunk to stdout
  (binary mode on Win32, icat_lib.c:36-52, 62-71).
- Attribute addressing syntax `inum-type-id` parsed by `tsk_fs_parse_inum` (tsk_fs.h:1195-1196) — this is
  how ADS extraction (`icat 1234-128-2`) works.
- **Content assembly semantics** (fs_attr.c):
  - `tsk_fs_attr_walk` dispatch (fs_attr.c:1034-1087): COMP → attribute's own `w()` (1062-1070);
    RES → `tsk_fs_attr_walk_res` (single callback from `rd.buf`); NONRES → `tsk_fs_attr_walk_nonres`.
  - `tsk_fs_attr_walk_nonres` (fs_attr.c:840-1018): iterates runs block-by-block.
    - SLACK flag extends total from `size` to `nrd.allocsize` (862-865).
    - Sparse runs and FILLER runs produce zero-filled blocks (906-919).
    - Reads at/after `inrs.initsize` return zeros (uninitialized tail) unless SLACK (921-925, 947-952).
    - `skiplen` leading bytes skipped (956-1006).
    - NOSPARSE suppresses callbacks for sparse/FILLER/past-initsize data (974-985).
    - Callback gets RAW + real block address for live runs; SPARSE + addr 0 otherwise (986-993).
    - Address > last_block is an error — `TSK_ERR_FS_RECOVER` when the meta is UNALLOC (deleted),
      `TSK_ERR_FS_BLK_NUM` otherwise (888-900).
  - `tsk_fs_attr_read()` = read()-style twin ("0s are returned for missing runs", fs_attr.c:1091-1101+).

### 3.3 blkls (tsk/fs/dls_lib.c + tools/fstools/blkls.cpp)
Three modes in `tsk_fs_blkls()` (dls_lib.c:204-244):
1. **Slack** (`-s`, TSK_FS_BLKLS_SLACK): walks **allocated inodes only** (dls_lib.c:213-216); per file,
   walks content with TSK_FS_FILE_WALK_FLAG_SLACK while tracking remaining file length `flen`; full
   blocks decrement flen; on the last block the in-file bytes are zeroed before output
   (`slack_file_act`, dls_lib.c:112-146). **NTFS iterates every non-resident attribute separately**
   with `flen = attr->size` (dls_lib.c:173-197); other FS use the single default stream (161-171).
2. **List** (`-l`, TSK_FS_BLKLS_LIST): prints `class|host|image|first_time|unit` header, then
   `addr|alloc` rows with `a`/`f` (dls_lib.c:43-77), using BLOCK_WALK AONLY (no content read) (218-226).
3. **Default**: streams every block matching the walk flags (blkls.cpp default: UNALLOC) to stdout via
   `tsk_fs_block_walk` + `print_block` (dls_lib.c:82-99, 238-241).
- Per-FS logic lives in each module's `block_walk`/`block_getflags`: NTFS consults the $Bitmap
  (`is_clustalloc`, ntfs.c:476-482); FAT consults FAT-chain/cluster state; ext consults group block
  bitmaps; block flags also carry CONT vs META classification.

### 3.4 ils (tsk/fs/ils_lib.c + tools/fstools/ils.cpp)
- Non-mac header (ils_lib.c:52-84): `class|host|device|start_time`, row `ils|<host>||<epoch>`, then
  column header `st_ino|st_alloc|st_uid|st_gid|st_mtime|st_atime|st_ctime|st_crtime|st_mode|st_nlink|st_size`.
- Row (ils_lib.c:112-159): `inum|a-or-f|uid|gid|mtime|atime|ctime|crtime|mode(octal)|nlink|size`; the
  second field is `a` when meta ALLOC, else `f` (ils_lib.c:141-142); skew applied then undone around
  printing (133-152).
- Filtering modes (`tsk_fs_ils`, ils_lib.c:231-285):
  - LINK/UNLINK flags filter on `nlink == 0` (ils.cpp `-l`/`-u`); if neither set, both enabled (249-254).
  - OPEN (`-O`) forces UNALLOC-only + LINK-only (243-248).
  - ORPHAN flag auto-enables LINK|UNLINK (239-241).
  - Walk = FS `inode_walk` over istart..ilast with ALLOC/UNALLOC/USED/UNUSED flags.
- `-m` mac mode (ils_lib.c:166-214): header `md5|file|st_ino|st_ls|st_uid|st_gid|st_size|st_atime|st_mtime|st_ctime|st_crtime`
  (105-107); row `0|<imagename>-[name2-]alive|dead-inum|inum|-/<ls-mode>|uid|gid|size|atime|mtime|ctime|crtime`
  — name from `meta->name2`, alive/dead from ALLOC flag (182-204). This is how **orphan/deleted inodes
  with no directory entry** enter the timeline.

### 3.5 hfind (tools/hashtools/hfind.cpp)
- `-c` create a new SQLite KDB (path must end `.kdb`; `tsk_hdb_create`; prints "New database … created",
  hfind.cpp:145-167).
- `-i type` build an external index via `tsk_hdb_make_index` ("Index created"); supported index types
  string `nsrl-md5, nsrl-sha1, md5sum, encase, hk` (TSK_HDB_DBTYPE_SUPPORT_STR, tsk_hashdb.h:100;
  usage hfind.cpp:40-47); mutual-exclusion checks vs `-f/-q/-e` (hfind.cpp:183-205).
- `-a` add a hash entry (rejected when `!tsk_hdb_accepts_updates`, hfind.cpp:175-180).
- Lookup: hashes from argv or `-f file` or STDIN; `-q` quick existence; `-e` extended; only one hash
  allowed with `-q` (hfind.cpp:221-260).
- Hit output `hash\tname` (name or "File name not available", hfind.cpp:52-58); miss
  `hash\tHash Not Found` (64-68).

### 3.6 fiwalk (tools/fiwalk/src/fiwalk.cpp, fiwalk_tsk.cpp) — public domain
- Self-documented algorithm (fiwalk.cpp:9-14): (1) find partitions; (2) walk files per partition;
  (3) print per-file info; (4) walk inodes per partition; (5) print per-inode info — files **and**
  orphan inodes.
- Output devices: ARFF (`arff.cpp`), XML/DFXML (`dfxml.cpp`), text or body file (fiwalk.cpp:60-63).
- Defaults: MD5+SHA1 on, byte runs on, maxgig=2 (fiwalk.cpp:68-96); options: `-I` ignore NTFS system
  files, `-O` allocated only, `-g` no data, `-b` no byte runs, `-z` no hashes, `-G<n>` size cap, `-m`
  mactime body, `-c config.txt` plugin config, `-n pattern` name filters (125-150).
- Plugins (tools/fiwalk/plugins/): jpeg_extract (java+cpp), docx_extractor.py, odf_extractor.py,
  word_extract.java, Libextract_plugin.java, ficlam.sh (clamscan), clamconfig.txt, ficonfig.txt.
- Uses its own bundled sha2.c / base64.cpp; hash_t.h defines md5/sha1/sha256 wrappers.

### 3.7 mactime (tools/timeline/mactime.base — perl) and the body format
- CLI (mactime.base:60-77): `-b body_file` (else STDIN), `-d` CSV, `-h` session header, `-i day|hour
  idx_file` (summary index), `-y` ISO8601 dates, `-m` numeric months, `-z TIME_ZONE`, `-p passwd_file`,
  `-g group_file`, `[DATE]` or range `yyyy-mm-dd..yyyy-mm-dd` (optionally with `Thh:mm:ss`).
- Input = **body file**: pipe-delimited, ≥11 fields
  `md5|name|inode|mode_as_ls|uid|gid|size|atime|mtime|ctime|crtime` (parsed by `tm_split`,
  mactime.base:941-950; read_body 348-369).
  - comment/blank lines skipped; header lines rejected by sanity regexes on inode/uid/gid/size/times
    (mactime.base:360-379); all-zero-time rows skipped (382).
  - rows entirely before the requested window skipped (384-389).
- Every non-zero time is appended to `%timestr2macstr{"<time>,<inode>,<file>"}` with letter
  **m / a / c / b** (b = crtime, "born"), de-duplicated per key+letter and window-filtered
  (mactime.base:404-465); times zero-padded to 10 digits (`sprintf("%.10d")`) for string sorting (391-396).
- Output: chronologically sorted; `-d` comma-delimited; uid/gid mapped via passwd/group files
  (load_passwd_info/load_group_info, 905-935); the `macb` column shows which of the four times
  coincide on that row.
- Companion: `tsk_gettimes` (tools/autotools/tsk_gettimes.cpp:92-118) is an automated body-file producer —
  per FS it calls `tsk_fs_fls` with `MAC|DIR|FILE|FULL` (+`HASH`), walk flags ALLOC|UNALLOC|RECURSE,
  prefix `vol<N>/` (and `poolVol<N>/`), applying `-s` skew.

## 4. NTFS deep-dive (tsk/fs/ntfs.c, tsk_ntfs.h, ntfs_dent.cpp, usn_journal.c)

- **MFT record read + fixup** (`ntfs_dinode_lookup`, ntfs.c:143-468):
  - Bootstrap: while $MFT's own $Data is uncached, entry address = `root_mft_addr + inum *
    mft_rsize_b`, refused for inum > NTFS_LAST_DEFAULT_INO (ntfs.c:194-211).
  - After bootstrap: walk the cached `$Data` run list of $MFT (`ntfs->mft_data->nrd.run`) translating
    inum→byte offset, including the special case of a record **crossing a run boundary** (512-byte
    clusters, two-part read, ntfs.c:230-334).
  - Fixup application (ntfs.c:370-465): `upd_cnt`/`upd_off` bounds-checked (380-412); the signature at
    `upd->upd_val` must equal the last 2 bytes of every 512-byte stride (`NTFS_UPDATE_SEQ_STRIDE`);
    mismatch ⇒ `TSK_ERR_FS_INODE_COR` "Incorrect update sequence value in MFT entry" (437-451); on
    match the saved originals are copied back (453-464). Entry-magic checking is deliberately disabled
    (#if 0 block, 354-369 — Windows tolerates invalid magics).
- **Runlist parsing** (`ntfs_make_data_run`, ntfs.c:596-715+):
  - Variable-length (LEB-style) length and offset fields; each field size sanity-checked ≤ 8 bytes and
    within the runlist buffer (651-659, 693-701); run len > block_count rejected (672-680).
  - The address offset is **signed and sign-extended** (714-715) — runs are relative to the previous run.
  - Offset == 0 ⇒ sparse run (comment 620-624); loop ends at a zero-length header byte.
- **Attribute processing**: `ntfs_proc_attrseq` (ntfs.c:1844+) walks the MFT record's attribute sequence;
  resident data → `tsk_fs_attr_set_str`; non-resident → runlist + `tsk_fs_attr_set_run`;
  `ntfs_proc_attrlist` (ntfs.c:2580+) follows $ATTRIBUTE_LIST records in external MFT entries;
  compressed attributes install `ntfs_attr_walk_special` / `ntfs_file_read_special` (ntfs.c:1288, 1594)
  with compression-unit state (`ntfs_uncompress_*`, 858-1173).
- **Deleted-entry semantics**:
  - `ntfs_dinode_copy` (ntfs.c:2886-3001): meta ALLOC iff MFT flags & NTFS_MFT_INUSE else UNALLOC
    (2964-2968); USED iff the attribute list is non-empty/INUSE (2991-2998); `seq` from `mft->seq` (2930).
  - Deleted directory **names** (`ntfs_proc_idxentry`, ntfs_dent.cpp:583-646): a name is UNALLOC if
    the parent dir is deleted (`a_is_del`), or `strlen == 0` (only that field is zeroed on delete), or
    the entry's end passes `endaddr_alloc` — i.e. **entries living in index-allocation slack are
    reported as deleted** (583-598). Advancing past a deleted entry guesses
    `idxlen = align4(16 + 66 + 2*nlen)` when strlen==0 (623-646) — the deleted-dentry carve step.
  - Deleted directory ⇒ its children get `meta_seq = dir->seq - 1` (ntfs_dent.cpp:919-928).
  - DOS 8.3 aliases are paired with WIN32 names via a WINDOS-namespace stash (560-621); a parent map
    keyed by (parent inum, parent seq) resolves parents without re-walking (70-102, 139-181).
- **$MFT runlist**: cached as `ntfs->mft_data` after bootstrap (ntfs.c:189-300); istat prints the
  `$LogFile Sequence Number` from $STANDARD_INFORMATION (ntfs.c:4660).
- **MFT slack**: no dedicated MFT-slack scanner at this HEAD — NOT FOUND. Equivalent behaviors:
  deleted index-entry recovery inside $INDEX_ALLOCATION slack (above), and inode_walk visiting UNALLOC
  MFT entries with partially intact attributes (recovery-mode error `TSK_ERR_FS_RECOVER` when runs are
  invalid: ntfs.c:3011-3030, fs_attr.c:888-892).
- **$UsnJrnl** (usn_journal.c + usnjls_lib.c): `ntfs_usnjentry_open` opens $UsnJrnl:$J as a file and
  iterates in fs-block-size buffers (usn_journal.c:232-254); record headers are scanned
  (`search_record`); **V2 records parsed; V3/V4 recognized but "not supported yet"** (147-183).
  usnjls output: default row `refnum-seq  parrefnum-seq  time.sec.time.nsec  REASONS  name`
  (usnjls_lib.c:214-228); `-l` long multi-line dump (version/length/USN/time/reason/source/security/
  attributes/name, 231-261); `-m` mactime-ish pipe row (264-280).
- **$LogFile**: only consumed as the LSN field (ntfs.c:4660); **no $LogFile transaction parser —
  NOT FOUND**. jls/jcat journal machinery (`jopen/jblk_walk/jentry_walk`, tsk_fs.h:1040-1044) is
  implemented for ext*/HFS; NTFS's implementations are pass-through stubs (ntfs.c:5118-5148).
- Other NTFS specifics: `$Secure` $SDS/$SII parsing for owner SIDs (`ntfs_load_secure`, `ntfs_get_sds`,
  `ntfs_file_get_sidstr`, ntfs.c:3389-3730); `$Bitmap` for cluster allocation; sequence-number-aware
  `ntfs_inode_lookup` (3040+); `ntfs_istat` prints attributes, runlists, FN times, and flags (4620+).

## 5. Other FS deleted-entry semantics (FAT/exFAT/ext)

- **FAT12/16/32** (fatfs.c, fatxxfs.c, fatfs_meta.c, fatxxfs_dent.c, tsk_fatxxfs.h):
  - Names and metadata come from the same 32-byte dentry, so TSK synthesizes both TSK_FS_NAME and
    TSK_FS_META from one entry.
  - A name is deleted when its first name byte is **0xE5** (`FATXXFS_SLOT_DELETED`, tsk_fatxxfs.h:33-35);
    an **Android exception subtype** treats `00 00` as deleted instead (tsk_fatxxfs.h:37-43).
  - Final alloc decision **combines that byte with the cluster's allocation state**: cluster allocated +
    0xE5 ⇒ UNALLOC name; cluster unallocated ⇒ UNALLOC name; cluster allocated + live first byte ⇒ ALLOC
    (fatxxfs_dent.c:410-424) — deleting a FAT directory often leaves per-entry flags intact, so the
    cluster state is the tiebreaker.
  - Meta-level allocation derives from the FAT chain: `fatfs_make_data_runs` (fatfs_meta.c:394+) takes
    the stashed first cluster from `content_ptr` and walks the FAT cluster-by-cluster building runs;
    file size is rounded up to a cluster multiple for allocsize (450-453); an out-of-range first cluster
    yields `TSK_ERR_FS_RECOVER` when the file is UNALLOC (434-448). Deleted FAT files report whatever
    chain remains (the chain head is usually zeroed, so recovery degrades gracefully).
  - FAT12/16 root directory is a fixed contiguous run outside the FAT (455-502); virtual files for the
    boot sector and each FAT copy are exposed as VIRT inodes (503-523).
  - Parent directories are reconstructed by scanning (expensive, cached via `fatfs_dir_buf_add`,
    fs_dir.c:1231-1242, 1293-1300).
- **exFAT** (exfatfs.c, exfatfs_meta.c, exfatfs_dent.c):
  - Directory entries are typed with an alloc bit in `entry_type`; a **file entry set** (file + stream +
    name-segment entries) is assembled into one TSK file.
  - Meta flags: ALLOC|USED iff the containing cluster is allocated **and** the entry-type alloc bit is
    set (exfatfs_meta.c:1299-1305, 1834+); otherwise UNALLOC — a deleted set degrades to a partial
    file with the entries it still has (1225-1227).
  - Cluster allocation is read from the exFAT **allocation bitmap** (`exfatfs_is_cluster_alloc`,
    exfatfs_meta.c:44-90); contiguous-flagged streams become a single run without consulting the FAT
    (`exfatfs_make_contiguous_data_run`, 1221-1224).
- **ext2/3/4** (ext2fs.c, ext2fs_dent.c):
  - Inode alloc state from the **group inode bitmap** (`ext2fs_imap_load` + `isset`,
    ext2fs.c:1218-1247); USED inferred from `i_ctime != 0` (1265-1266); ORPHAN = unalloc inode not
    referenced by any (even deleted) name (1271-1278, via `tsk_fs_dir_find_inum_named`).
  - Linux **deletion time `i_dtime`** preserved into `meta->time2.ext2.dtime` (ext2fs.c:829) and shown
    by istat (2400).
  - ext4 adds `*_extra` nanosecond fields and `i_crtime` (830-840), extent trees (`EXT2_IN_EXTENTS` ⇒
    `TSK_FS_META_CONTENT_TYPE_EXT4_EXTENTS`, 861-869) and inline data (870-880).
  - **Deleted dentry carving** (ext2fs_dent.c:116-195): the parser advances by the *minimum* record
    length instead of the recorded `rec_len` "so we can view the deleted entries" (136-140), tracking
    `dellen` — the gap created when a deleted entry's rec_len was merged into its predecessor; an entry
    is UNALLOC if `dellen > 0` (consume dellen by minreclen), `inode == 0`, or the whole dir block is
    unallocated (186-195). This recovers deleted names still present in directory slack.

## 6. Image formats layer (tsk/img)

- **TSK_IMG_INFO** (tsk_img.h:93-114): `{tag, itype, size, num_img, sector_size, page_size (NAND, 2048
  default), spare_size (64 default), images[], cache_lock, cache[32][65536], cache_off[32], cache_age[32],
  cache_len[32], read, close, imgstat}` — a **32-entry × 64 KiB LRU read cache** with lock
  (TSK_IMG_INFO_CACHE_NUM/LEN, tsk_img.h:84-85).
- **Plug-in surface is only three functions**: `read(img, off, buf, len)`, `close(img)`,
  `imgstat(img, FILE*)` (tsk_img.h:111-113). A new format implements these inside a struct whose first
  member is TSK_IMG_INFO.
- **External format injection**: `TSK_IMG_TYPE_EXTERNAL` (tsk_img.h:77, "used by pytsk");
  `tsk_img_open_external(ext_img_info, size, sector_size, read, close, imgstat)` validates sector size
  (≥512, multiple of 512), stamps the tag, and inits the cache lock (img_open.cpp:410-479).
- **Types** (tsk_img.h:62-82): RAW (single + split share one code path), AFF_AFF/AFF_AFD/AFF_AFM/
  AFF_ANY (AFFLIB), EWF_EWF (libewf), VMDK_VMDK (libvmdk), VHD_VHD (libvhdi), EXTERNAL, POOL, LOGICAL.
- **Auto-detection order** (img_open.cpp:117-232): AFF → EWF → VMDK → VHD (each `#if HAVE_LIB…`),
  then **raw as fallback**:
  - Two simultaneous non-raw matches are fatal ambiguity (`TSK_ERR_IMG_UNKTYPE` "EWF or %s" etc.,
    img_open.cpp:155-213).
  - AFF password errors abort the whole detection (148-151).
  - Raw open failure with errno set aborts; otherwise `TSK_ERR_IMG_UNKTYPE` (222-231).
  - Sector-size argument must be ≥512 and %512==0 (95-109).
- **raw.c (816 lines)**: single/split files; split segment names generated by `getSegmentName`
  (mult_files.c:41-149): `.dmg` → `.NNN.dmgpart`; `.001`/`_001` (1-based, unlimited digits — FTK style);
  `.000`/`_000` (0-based); `.01`/`.00` (2-digit); `.aaa`/`.aa` (alphabetic base-26); `.bin` →
  `name(N).bin`; segments probed until open fails; concatenation is transparent via the cache.
- **ewf.cpp (783 lines)**: libewf adapter (`libewf_handle_open[_wide]`, ewf.cpp:346-350; legacy
  `libewf_open` path at 459); multi-file E01 sets supported; verification API `tsk_img_verify` with
  result enum TSK_IMG_VERIFY_{UNSUPPORTED, ERROR, PASS, FAIL} + progress callback (tsk_img.h:117-160).
- **vmdk.c (278) / vhd.c (283)**: thin adapters over libvmdk / libvhdi
  (`libvmdk_handle_read_buffer_at_offset`, vmdk.c:64-65); sparse-grain logic lives inside the libs;
  `imgstat` prints type/size/sector_size (vmdk.c:87-97).
- **aff.c (381)**: AFFLIB adapter incl. encrypted-image password path (TSK_ERR_IMG_PASSWD).
- **logical_img.c (302)**: TSK_IMG_TYPE_LOGICAL — a local directory is treated as an image;
  `tsk_fs_open_img` then forces `logicalfs_open` (fs_open.c:158-171), surfacing local files as TSK
  files (used by Autopsy "local files" data sources).
- **Unsupported-type + encryption awareness**:
  - `unsupported_types.c` (`detectUnsupportedImageType`) names formats TSK cannot open — used by the
    fs-open failure path (fs_open.c:242-252).
  - `detect_encryption.c` signatures: LUKS `"LUKS\xba\xbe"` (0), BitLocker `"-FVE-FS-"` (offsets 0-16),
    FileVault `"encrdsa"`, Check Point `"Protect"` (80-100), McAfee Safeboot `"Safeboot"` (0-32, two
    capitalizations), Guardian Edge `"PCGM"`, Sophos `"SGM400"` (detect_encryption.c:30-90).
  - A full **BitLocker** parser lives in tsk/util/Bitlocker/ (BitlockerParser.cpp, MetadataValue* etc.),
    feeding `TSK_FS_ENCRYPTION_TYPE_BITLOCKER` (tsk_fs.h:934-938) and the `-k password` decrypt paths
    (`tsk_fs_open_*_decrypt`, tsk_fs.h:1067-1072).

## 7. Hash databases + timelines

### 7.1 tsk/hashdb
- **Formats** (TSK_HDB_DBTYPE_ENUM, tsk_hashdb.h:78-87):
  - NSRL (nsrl.c; md5 or sha1 index), md5sum (md5sum.c), HashKeeper (hashkeeper.c), EnCase (encase.c),
    idxonly (open a previously built TSK index without the original DB, idxonly.c), SQLite
    (sqlite_hdb.cpp — the modern updatable format created by `tsk_hdb_create` / hfind `-c`).
- **Hash algorithms**: MD5, SHA1, SHA2-256 (tsk_hashdb.h:41-47; string names + lengths 49-57).
- **TSK_HDB_INFO is a vtable** (tsk_hashdb.h:115-137): get_db_path, get_display_name,
  uses_external_indexes, get_index_path, has_index, make_index, open_index, lookup_str, lookup_raw,
  lookup_verbose_str, accepts_updates, add_entry, begin/commit/rollback_transaction, close_db.
  Two implementations: TSK_HDB_BINSRCH_INFO (text DBs) and the SQLite one.
- **Binary-search index** (TSK_HDB_BINSRCH_INFO, tsk_hashdb.h:142-158):
  - Sorted fixed-line-length text index: `idx_off` (header offset), `idx_llen` (line length), `idx_size`.
  - **Index-of-index**: `idx_offsets[]` maps the first three bytes of a hash to a file offset
    (tsk_hashdb.h:157) — constant-time bucket seek, then binary search inside.
  - `get_entry` is a per-format callback that seeks into the *original* text DB to fetch the full entry
    (tsk_hashdb.h:145); lazy index open under `lock` (119).
- **Lookup API**: `tsk_hdb_lookup_str/raw` return -1/0/1 and invoke `TSK_HDB_LOOKUP_FN(hdb, hash, name,
  ptr)` per hit unless TSK_HDB_FLAG_QUICK (tsk_hashdb.h:180-184); TSK_HDB_FLAG_EXT for extra detail (32-36).
- **Integration with file tools**: fls `-h` computes MD5 via `tsk_fs_file_hash_calc`
  (tsk_fs.h:747-753, TSK_FS_HASH_RESULTS carries md5+sha1 digests) and embeds it in the body row;
  fiwalk computes md5/sha1 by default; the Java layer (HashDbTest, HashUtility) wraps the same C calls.

### 7.2 Timelines (fls/ils -m + mactime)
- **Producers**:
  - `fls -m dir/ [-h]` — one body row per name (per NTFS attribute, incl. ADS) plus the extra
    `($FILE_NAME)` row (§3.1).
  - `ils -m` — one body row per inode incl. orphans (§3.4).
  - `tsk_gettimes` — automated `fls -m` across all volumes of an image (§3.7).
  - `fiwalk -m` — body output from the fiwalk walk.
- **Body format** = `md5|name|inode|mode|uid|gid|size|atime|mtime|ctime|crtime` (§3.7).
- **Time semantics per FS** (tsk_fs.h:470-500 + module code):
  - mtime: last content modification (ext `i_mtime`; NTFS $SI modified).
  - atime: last access — **FAT stores only the day** (rendered day-only, fs_name.c:523-525); often 0 on FAT16.
  - ctime: last metadata/inode status change (ext `i_ctime`; NTFS $SI changed).
  - crtime: creation (ext4 `i_crtime` (ext2fs.c:830-840); NTFS $SI created; HFS create).
  - extras: ext `dtime` deletion time (istat-only), HFS `bkup_time`, NTFS `fn_*` set of four from
    $FILE_NAME — surfaced as the extra body row so both SI and FN timelines are visible.
- **Consumer**: mactime (§3.7) sorts body rows chronologically, tagging each with `macb` letters.

## 8. Tests layout

- `tests/` (Makefile.am:12-17): four C++ programs + two shell scripts:
  - `read_apis` — cross-checks `file_walk` callback data against `tsk_fs_file_read`, `tsk_fs_read`, and
    `tsk_img_read` on real images (skips sparse/compressed cases) (read_apis.cpp:13-19, 30-80).
  - `fs_fname_apis` — name-level API regression (fs_fname_apis.cpp).
  - `fs_attrlist_apis` — attribute-list API regression (fs_attrlist_apis.cpp).
  - `fs_thread_test` (+ tsk_thread.cpp/h) — runs the same traversal with N threads and diffs logs.
  - `runtests.sh` — thread regression over ext2/ufs/hfs/ntfs/fat images; **"not currently being used
    anywhere"** because the images are private (runtests.sh:3-7); per-FS 1-thread baseline vs N-thread
    diff (51-98).
  - `test_libraries.sh` — downloads three images (.vhd/.vmdk/.E01) from Google Drive and asserts `mmls`
    opens each, gated on compiled-in libs (test_libraries.sh:19-79).
- `unit_tests/base/`: cppunit suite for the error subsystem (errors_test.cpp).
- `test/tools/autotools/test_loaddb.sh`: end-to-end tsk_loaddb check.
- `tools/fiwalk/src/test_fiwalk.sh`: fiwalk self-test.
- `bindings/java/test/`: the *de-facto* golden-output regression suite — DataModelTestSuite +
  CrossCompare / CPPtoJavaCompare traversals (sequential, top-down, bottom-up) with gold `.txt` outputs
  for images `ntfs1-gen2` and `nps-2009-canon2-gen6` (bindings/java/test/output/gold/), plus JUnit
  suites for HashDb, Timeline, OsAccount, Communications, etc.
- Coverage gaps at this HEAD: no C unit tests for tsk/vs, tsk/img, or hashdb internals (NOT FOUND
  beyond the java suite and the shell scripts above).

## 9. Coverage log (read fully / skimmed / enumerated — honest)

- **Read fully (or near-fully)**: tsk/fs/tsk_fs.h (all struct/flag sections; C++ wrapper classes
  skimmed), fs_open.c, fs_dir.c, fls_lib.c, fls.cpp, icat_lib.c, ils_lib.c, dls_lib.c, fs_name.c
  (print section), fs_attr.c (walk_nonres / walk dispatch / read), tsk/vs/tsk_vs.h, mm_open.c,
  mm_types.c, mm_part.c, mmls.cpp, tsk/img/tsk_img.h, img_open.cpp, mult_files.c,
  tsk/hashdb/tsk_hashdb.h, hfind.cpp (main flow), mactime.base (arg parsing + read_body), tsk/auto/
  auto.cpp (findFilesIn* family), tsk_gettimes.cpp (filterFs), usn_journal.c, usnjls_lib.c,
  tests/Makefile.am + runtests.sh + test_libraries.sh, licenses/README.md, fs_types.c.
- **Read in targeted depth (function-level)**: ntfs.c (dinode_lookup, make_data_run, dinode_copy,
  load_attrs, inode_lookup, istat/journal refs), ntfs_dent.cpp (deleted-entry logic, parent map,
  dir_open_meta), ext2fs.c (dinode_copy, inode_walk), ext2fs_dent.c (dellen carving), fatfs_meta.c
  (make_data_runs), fatxxfs_dent.c + tsk_fatxxfs.h (deleted flag), exfatfs_meta.c (alloc bitmap,
  entry-set flags), fs_file.c (open_meta/open/attr_check), gpt.c (load_table), dos.c (type-description
  table), vmdk.c (read/close/imgstat), detect_encryption.c, pool/tsk_pool.h (types), fiwalk.cpp
  (header/options), tsk_db_sqlite.h (API surface).
- **Enumerated only**: tsk/fs (hfs*.c ~15k lines, apfs*, iso9660*, ffs*, yaffs*, logical_fs, rawfs,
  swapfs, decmpfs/lzvn), tsk/base/*, tsk/util/Bitlocker/*, tsk/auto/{auto_db,db_sqlite,case_db}.cpp +
  bundled sqlite3.c, tsk/pool/*.cpp, ewf.cpp/aff.c/vhd.c/raw.c/logical_img.c internals, hashdb format
  parsers (nsrl/md5sum/hashkeeper/encase/idxonly/binsrch_index/sqlite_hdb), rejistry++/, tools/{sorter,
  srchtools, logicalimager, pooltools, imgtools, vstools/{mmstat,mmcat}, fstools/{jcat,jls,istat,ifind,
  ffind,fcat,fsstat,fscheck,blkcat,blkcalc,blkstat}}, fiwalk plugins, win32/, man/, bindings/java
  (file list only), samples/ (names only).
- **Not verified**: exact HEAD SHA (git prohibited); any runtime behavior (no builds/executions).

## 10. Harvest notes (strongest abstractions/behaviors to harvest, licensing constraints per component)

**Strongest abstractions/behaviors to re-implement (behavioral only):**
1. **Layered object model with independent name/meta/attr allocation flags** (§1.1) — a "file" is
   (name entry, meta entry, attribute list) with independent alloc states, joined lazily, with a
   `(realloc)` presentation when a deleted name points to live meta.
2. **Function-table plug-in contract** (~20 FS pointers; 3 img pointers) + try-all-openers auto-detect
   with explicit multi-match failure, BitLocker-error preservation, and signature/entropy fallbacks
   (fs_open.c:176-277; img_open.cpp:117-232).
3. **Sorted-partition enumeration with synthesized "Unallocated" gaps** (mm_part.c:123-170) and the
   mmls slot/start/end/length/description row format (mmls.cpp:57-127).
4. **Orphan-file pipeline** (§1.3): UNALLOC name walk → named-inode set → UNALLOC meta walk minus set →
   `$OrphanFiles` virtual dir, cached + locked, with nested-dir de-dup.
5. **Run-list content model** (§3.2): runs {offset, addr, len, flags}, resident buffer,
   skiplen/allocsize/initsize, zero-fill for sparse/FILLER/past-initsize, SLACK switching size→allocsize.
6. **Per-FS deleted-dentry carving idioms** (§4, §5): NTFS index slack + strlen==0 + guessed idxlen;
   ext min-reclen advance + dellen; FAT 0xE5 combined with cluster allocation; exFAT entry-set alloc bit
   + bitmap.
7. **Body-file/mactime timeline contract** (§3.1, §3.7): 11-field pipe format, macb letters, NTFS
   double-row (SI + FILE_NAME), epoch-zero suppression, skew only on non-zero times.
8. **Hash DB vtable + 3-byte index-of-index binary search** (§7.1) and the hfind
   create/index/lookup/add CLI surface.
9. **Split-raw segment naming matrix** (mult_files.c:41-149) — the most complete open-source list of
   split-image naming conventions.
10. **Encryption-aware failure UX**: LUKS/BitLocker/FileVault/Safeboot/… signature detection feeding
    distinct error codes instead of "unknown format" (detect_encryption.c:30-90).

**Licensing constraints (per component):**
- Core `tsk/` + `tools/` (everything in §§1-8 except fiwalk): **IPL 1.0 / CPL 1.0** — copy nothing,
  not even verbatim struct field layouts or comments; reproduce interface facts (CLI syntax, output
  field order) from prose descriptions like this sheet.
- `tools/fiwalk/*`: public domain — safe to study closely; still credit NPS/Garfinkel.
- `tsk/auto/sqlite3.c`: public domain. `tsk/fs/lzvn.c`: BSD-3. `tsk/auto/guid.cpp`: MIT.
- `rejistry++/`, java datamodel: Apache-2.0 (notice-preserving reuse OK).
- Do NOT use as copy sources: `tools/srchtools/srch_strings.c` (GPLv2), `tsk/base/md5c.c` (RSA terms),
  `tsk/base/tsk_unicode.c` (Unicode Inc. terms).

**Anomalies / caveats for downstream agents:**
- The task brief's "tsk/mm" is actually **`tsk/vs`** (files keep mm_ prefixes). There is **no AIX VS
  module** (AIX exists only as DOS type descriptions 0x08/0x09, dos.c:62-65) and no standalone EFI VS —
  GPT *is* the EFI table. `TSK_VS_TYPE_APFS` is reserved-but-unimplemented (mm_open.c:227-228); APFS
  containers go through the pool layer instead.
- NTFS `$LogFile` is parsed only as an LSN field — no transaction replay (NOT FOUND). USN journal V2
  only (V3/V4 skipped, usn_journal.c:167-180).
- fls root-directory output order is **prioritized** (Users/home first, pagefile last, fs_dir.c:582-646)
  — golden outputs will reflect this ordering.
- tests/runtests.sh is dead (private images); the real regression suite is the Java bindings' gold files.
- Hard limits worth replicating defensively: `MAX_DIR_SIZE_TO_PROCESS` (fs_dir.c:257), dir-walk depth
  128 / path 4096 / macro-recursion 128 (fs_dir.c:521-522, 972), GPT entry cap 8192 (gpt.c:186-187),
  mmls recurse list cap 64 (mmls.cpp:20).
