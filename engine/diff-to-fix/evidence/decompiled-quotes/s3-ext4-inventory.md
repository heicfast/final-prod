# s3-ext4-inventory — EXT4SearchDLL function/feature inventory (S3-EHB, session 3)

Reference: `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c`
(18,071 lines; 341 functions, all `FUN_*` — no symbols; class names only at
vftable sites). Core: `src/core/ext4/cr_ext4_engine.cpp` (880 LOC) @ HEAD `34b3441`.
All `:NNNN` cites below are EXT4SearchDLL_decompiled.c line numbers unless prefixed.

## 0. Call graph (decoded this session)

| Function | Role | Evidence |
|---|---|---|
| `FUN_180004710` | superblock validator (magic+field set) | :3214-3247 |
| `FUN_1800047c0` | SB read (0x200 bytes @+0x400) + validate + copy SB→ctx+0x98 + read GDT | :3254-3437 |
| `FUN_180003de0` | **the scan driver** (Quick & Complete share it) | :2722-2802 |
| `FUN_18000e5b0` | journal pre-pass: jsb gate + 1 MiB windows + inode-image walk | :12159-12260 |
| `FUN_1800068b0` | live-tree walk from inode 2 (dir read + recurse + entry processing) | :4898-5172 |
| `FUN_180005820` | **directory-block parser** (per-entry loop) | :4085-4283 |
| `FUN_1800085f0` | file-entry processing (dtime predicate + emit) | :6392-6317? → :6392-6641 |
| `FUN_180005340` | extent-tree resolver (0xF30A) | :3867-3920 |
| `FUN_180005190` | indirect-block resolver (12+3 ladder) | :3788-3862 |
| `FUN_180005020` | inode locator (group = (ino-1)/ipg, stride = s_inode_size) | :3723-3762 |
| `FUN_180004c70`/`FUN_180004e40` | group-descriptor resolvers (**META_BG path**) | :3500-3606 / :3611-3700 |
| `FUN_18000e890` | journal every-block salvage | :12265-12368 |
| `FUN_1800043f0` | journal-dir-block gate ("." + ".." head) + salvage insert | :3049-3196 |
| `FUN_1800041e0` | journal-orphan emit ("iNode%I64d") | :2961-3044 |
| `FUN_180005e50` | journal-block-as-inode-table-image walk | :4340-4613 |
| `FUN_180007720` | hard-link same-name (case-insensitive) dedup | :5544-5730 |
| `FUN_180007510` | sparse test: `i_blocks(+0x1C) < ceil64(size)/512` → attribute 0x200 | :5455-5467 |
| `FUN_180009c10` (in HFS+ DLL) | — (not ext4; listed to avoid confusion) | — |

Driver order (:2722-2802): SB/GDT (`FUN_1800047c0`) → 10 MiB VirtualAlloc (+0x48,
:2766) → journal pre-pass `FUN_18000e5b0(ctx, journal_inum(+0x6c), 0x1000)` (:2773)
→ **live walk `FUN_1800068b0(ctx, inode 2, parent 0, name=+0x88)` (:2776)**
→ journal every-block `FUN_18000e890` (:2778) → free GDT buffers (:2779) →
journal-orphan emit `FUN_1800041e0` (:2780). **No on-disk inode-table sweep
anywhere** (no loop bounded by the group count +0x68 exists — only bound checks
at :3738/:4963/:4990).

## 1. Superblock detection — gate (FUN_180004710 :3222-3247, re-read first-hand)

```
(short)param_1[0xe] == -0x10ad        // magic u16@SB+0x38 == 0xEF53
(byte)(param_1+0x18) & 1 == 0         // incompat@SB+0x60 bit0 (COMPRESSION) clear
inodes_count(+0x00) != 0, blocks_lo(+0x04) != 0, bpg(+0x20) != 0, ipg(+0x28) != 0
first_data_block(+0x14)==0 ⇒ log_bs(+0x18)!=0 ; ==1 ⇒ log_bs==0   // kernel rule
block_size = 0x400 << (log_bs & 0x1F), positive as int            // ≤ 2^30!
blocks64 = blocks_lo | blocks_hi(+0x150)<<32 != 0
(rev_level(+0x4C) == 0) || (inode_size(+0x58) != 0)
groups = blocks64 / bpg; 0 < groups ≤ blocks; (int)(~bs & groups*0x40+bs) > 0
```
Quick path reads **0x400** bytes @+1024 (:321), this path reads **0x200**
(:3303); both gates identical (:3304-3316). The og reads `s_inodes_count`,
`blocks_lo`, `bpg`, `ipg`, `fdb`, `log_bs`, `blocks_hi(+0x150)` ✓ core parity
(cr_ext4_engine.cpp:102-129). Core stricter: `log_bs > 6 → reject` (:114) vs
og allows `0x400 << (log & 0x1F)` up to 2^30; core `groups > 2^20 → reject`
(:128 MAX_GROUPS) vs og's int-overflow bound ≈ 33.5M groups. Corrupt-SB only.

## 2. Geometry, inode size, desc size (FUN_1800047c0 :3376-3414)

- `+0x58 = block_size` (:3376-3377); `+0x5c = inodes_per_group` (:3387);
  `+0x60 = blocks_per_group` (:3393); `+0x68 = groups` (:3394-3396);
  `+0x70 = blocks64` (:3383-3384).
- inode stride: `if (rev_level != 0) inode_size = u16@+0x58 else 128` (:3388-3391)
  — **no clamp**; core clamps to [128,1024]&~7→128 (cr_ext4_engine.cpp:139-140).
- desc_size: clamped band **[32,64]** unconditionally (:3392-3405: `<0x20→0x20`,
  `>0x40→0x40`); GDT buffer slot `+0x498` when 32, else `+0x4a0` (:3409-3414).
  Additionally `+0xdf8 = s_desc_size` **only under incompat&META_BG(0x10)** with
  0→1 (:3368-3374). Core: desc_size 64 forced when 64BIT, else 32
  (cr_ext4_engine.cpp:147-155) — kernel semantics (64BIT gates desc_size, no
  64BIT ⇒ 32 regardless of the SB field).
- **GDT position**: og reads `round_up(groups*desc_size, block_size)` bytes at
  **`base + block_size`** (:3406-3422) — correct for fdb=0 (block 1), **wrong
  for 1 KiB-block volumes** (fdb=1: GDT is block 2 = base+2048; og reads the SB
  block). Core: `gdt_block = first_data_block + 1` (cr_ext4_engine.cpp:438) —
  kernel-correct for both (executed pin: probe + test_ext4_volume.cpp:17-26).
- journal_inum copied to `+0x6c` **only when compat&HAS_JOURNAL(0x4)** (:3365-3366);
  journal pass skipped entirely when `journal_inum < 1` (:12179-12181). Core
  substitutes inode 8 when journal_inum==0 (cr_ext4_engine.cpp:144-145).

## 3. Inode resolution (FUN_180005020 :3723-3762 / FUN_1800068b0 :4959-4967)

`group = (ino-1)/ipg; offset = inode_size*((ino-1)%ipg)`; inode-table block =
`FUN_180004e40(g, off)` or, when that helper declines, direct GDT read
`GDT32[g]+8` / `GDT64[g]+8` (:3743-3753, :4972-4981). Reads a whole
`block_size` per inode-table block into the 2×block_size buffer `+0x50`
(:3380, :3755, :3778-3781). Core reads `inode_size` bytes per inode
(cr_ext4_engine.cpp:266) — CachedReader (8 MiB/64 KiB, cr_scan.cpp:610) makes
device traffic equivalent (X51).

### META_BG distributed descriptors (FUN_180004c70/FUN_180004e40)
Both bail unless `incompat & 0x10` (META_BG) — :3521-3523/:3632-3634
(`+0x2e4`/`+0x2e8` are never written anywhere in the DLL). Under META_BG they
resolve `g` via `rem = g % desc_size`, and for `g ≥ desc_size` read a
**per-group block** at `bpg*block_size*round_down(g,desc_size)+base`
(map-cached, :3574-3595) and take the descriptor at `rem*stride+8` within it
(:3541-3554). Core has **no META_BG path** — it reads only the primary GDT for
every group (cr_ext4_engine.cpp:175-191) → on `-O meta_bg` volumes, groups
beyond the first meta-block-group resolve garbage/zero inode tables and their
files vanish from the live tree. → **M-019**.

## 4. Extents / indirect ladder (A27)

- Extent resolver `FUN_180005340` (:3867-3920): magic 0xF30A, leaf walk
  `ee_block/ee_len/ee_start(48-bit concat)`; interior descent via child read
  `FUN_180005120`; per-node cancel. Core: same decode incl. `raw_len > 0x8000`
  unwritten semantics (cr_ext4_engine.cpp:215-231) — og reads ee_len plain
  (:3885-3887) with no unwritten-bit decode (core superset, B7 sparse).
- Indirect resolver `FUN_180005190` (:3788-3862): direct[12]@inode+0x28,
  single@+0x58, double@+0x5C, triple@+0x60, 32-bit entries — parity with the
  core ladder (cr_ext4_engine.cpp:307-348).

## 5. Directory walk (FUN_180005820 :4130-4268) — the load-bearing deltas

Per entry: `inode(+0)!=0 else STOP whole block` (:4154-4155); `inode <=
s_inodes_count` (:4168); type byte@+7 must be 1..7 (0 or >7 → skip+resync by 4)
(:4164-4165); `rec_len@+4` must be 4-aligned and ≥ align4(8+name_len)
(:4166-4167); **advance = `align4(name_len)+8` (computed)** (:4160-4162,
:4173-4174). Name at +8 (FILETYPE layout assumed unconditionally — no
incompat&0x2 gate anywhere in the DLL; only incompat bits tested: bit0
compression :3223/:3304, bit0x10 META_BG :3368/:3522/:3633).
Core: `off += rec_len` (cr_ext4_engine.cpp:391, :410), skips inode-0 entries
(:396), majority-valid gate (:412), FILETYPE-gated type byte (:447).

Consequences (see M-020/N-036/I-013): og re-emits delete-merged stale entries
by name (core skips the merged space — executed: s3-ehb-ext4-dirwalk); og stops
at inode-0 (core continues — executed, same); og requires dot-dot heads for
journal-dir salvage (FUN_1800043f0 :3077-3109: entry0 name must be "."/"..",
entry@+0x12 likewise) while core's majority gate accepts more journal remnants.

## 6. File/entry processing (FUN_1800085f0 :6392-6641)

- 64-bit size = `size_lo(+0x04) | size_high(+0x6C)<<32` (:6436, :5000).
- **Dir content cap 20 MiB**: `size < 0x1400001` or the whole directory is
  skipped (:5001); buffer = static 10 MiB `+0x48` (:2766) or a fresh
  `VirtualAlloc` for 10–20 MiB dirs (:5013-5019). Core cap 64 MiB
  (MAX_DIR_BYTES, cr_ext4_engine.cpp:53) with truncation instead of subtree
  loss.
- **Deleted predicate (live-walk entries)**: `deleted = dtime(+0x14)!=0`;
  `if (dtime==0 && size==0) deleted = (i_block[1](+0x2A)==0)` (:6438-6442) —
  i.e. **all empty files classify deleted**; journal-cache inodes always
  deleted (:6447-6448); `param_8` forces deleted (:6444-6445). Zero-size +
  dtime!=0 dirs get their size re-fetched from the journal inode-image map
  (+0x4a8, :6450-6451, built by FUN_180005e50 :4419-4445 from journal block
  images walked as inode tables — key = u32@inode+8, value = journal-block tag).
  Core: `dtime!=0 || links==0` (cr_ext4_engine.cpp:516) — no empty-file
  misclassification, no journal size-resurrection.
- Sparse flag: `FUN_180007510` `i_blocks(+0x1C) < size>>9` → attribute 0x200
  (FILE_ATTRIBUTE_SPARSE_FILE) (:5464-5466, applied :6300-6302). Core sets only
  0x10 for dirs (cr_ext4_engine.cpp:502).
- Hard links: `FUN_180007720` — for an inode already emitted, a **same-name
  (case-insensitive `_stricmp` :5684) re-emission is dropped**; different names
  both emit (:5582-5661). Core emits every dirent unconditionally.
- Root node name: driver passes `param_1+0x88` = **the volume label**
  (s_volume_name copied :3375) as the root's name (:2776). Core names it "/".
- Timestamps: ctime@+0x0C, mtime@+0x10, `_gmtime32_s`, `wMilliseconds=0`
  (:6261-6282) — I-006/I-007 already filed (superset core).
- Symlinks: type 7 processed like a file; no fast-symlink inline-target read
  anywhere (no i_block-as-content path). Core: same (neither reads targets).
- xattrs: **no i_file_acl reads anywhere** (no +0x64 inode-field access);
  no `inline_data` (incompat 0x8000) and no `EXT4_INLINE_DATA_FL` test
  (no 0x8000000 hits). Core: likewise none. MATCH-by-omission.
- BIGALLOC: **no incompat 0x400 test** — the og never refuses bigalloc; it
  just mis-scans (block-granular). Core refuses loudly
  (cr_ext4_engine.cpp:432-435, registered deliberate).

## 7. Journal (S-004 surface, refined)

- jsb gate magic 0xC03B3998 + blocktype {3,4} (:12206-12211) ✓ core parity.
- Range: **[s_first(jsb+0x14), s_maxlen(jsb+0x10))** — og treats s_maxlen as
  the end index (:12324-12326); core `[s_first, s_first+s_maxlen)` clamped to
  the journal inode's blocks (cr_ext4_engine.cpp:767-771) = kernel
  `j_last = j_first + j_maxlen` semantics — core covers the og's range plus
  the tail block after clamping.
- Fragmentation: og resolves only journal block s_first physically, then walks
  **physical+1 per logical block** (:12361) and per window (:12247 `puVar10+1`)
  — a fragmented journal inode walks the wrong blocks. Core reads through the
  extent map (read_extent_data, cr_ext4_engine.cpp:678-680) ✓ correct.
- Dedup: og skips physical blocks already seen (cache map :12329-12343);
  core dedups by {inode,name} (cr_ext4_engine.cpp:830).
- Pre-pass `FUN_18000e5b0` feeds every journal block to the inode-image walk
  `FUN_180005e50` (stride = inode_size, bound = block_size, :4417/:4422) which
  collects {size!=0, EXTENTS_FL, 0xF30A} images — the og's only mechanism that
  gives zero-size deleted dirs a size. Core has no counterpart (folded into
  S-017's HFS+ sibling discussion; ext4-side impact is the M-020/I-013 set).

## 8. Perf read patterns (X51)

SB: 0x200 (complete) / 0x400 (quick) reads; GDT: one bulk read
round_up(groups*desc_size, block); inodes: block_size read per inode-table
block through a block cache (FUN_1800062d0); dir content: whole-dir reads
(≤10 MiB static / ≤20 MiB alloc, 1 MiB journal windows :12236-12239
`0x100000`); result: og reads are block-granular and cached in-engine; core
reads are per-inode (inode_size) through the pipeline CachedReader
(8 MiB cache, 64 KiB blocks — cr_scan.cpp:610). Equivalent device traffic.

## 9. What the og does NOT have (core supersets — I-013)

- **No on-disk deleted-inode sweep** (no dtime scan of inode tables); its
  deleted surface = journal dir remnants ("iNode%I64d" :2989) + stale merged
  entries (§5) + journal inode-image sizes (§6). Core's default-on sweep
  (cr_ext4_engine.h:44-46, cr_ext4_engine.cpp:545-641) has no og counterpart.
- No BIGALLOC refusal, no metadata_csum (incompat 0x2000) test, no backup-SB
  probes (deep-dive §2's "8193, 32768…" remains corpus-doc overreach,
  CORRECTIONS #4), no crc32c, no inode-count sanity beyond `<= inodes_count`.
