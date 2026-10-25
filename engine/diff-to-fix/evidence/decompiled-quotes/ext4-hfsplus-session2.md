# Evidence — ext4/HFS+ session-2 quotes (journal, decmpfs, gates)

Source: `re-workspace/reversed/ghidra-searchdll/{EXT4SearchDLL,HFSPlusSearchDLL}_decompiled.c`
(corpus `03ad46c`). Reader: agent S2-G2 (scratch `S2-G2-context.md`). Core: `e93decf`.

## HFS+ decmpfs (S-002 corrected payload)

```
FUN_180009940 :7662-7700  catalog parse: byte41 & 0x20 = UF_COMPRESSED →
              :7677  *(byte*)(node+0xce) |= 4;  fork = param_6 + 0x54  (RESOURCE FORK);
              :7668-69 (uncompressed fallback size = dataFork.totalBlocks << 12)
              :7687-89  userInfo.fileType == 0x66647270 ('prdf') → folderized
FUN_180009c10 :7759-68   builds "com.apple.decmpfs"; :7771 keyLength = (17+7)*2 = 48
              :7774  search ATTRIBUTES fork (cnid 8)
              :7776-7821  miss → journal-harvested deleted-attr map (volume+0x78)
              :7825  if (*rec == 0x10)  (INLINE only)
              :7826-29  logicalSize@+12; memcpy data@+16
              :7833  recordType 0x20/0x30 → "unsupported attribute node format"
              :7840-42  node.size = *(u64*)(header + 8); free(payload)   // SIZE ONLY
imports: no zlib in HFSPlusSearchDLL — decompression impossible
FUN_1800057d0 :3714-18   result sink: name[0]=='.' || strstr("HFS+ Private Data") → destroy
```

## HFS+ journal (S-003 corrected payload)

```
ctor :13456  VH.attributes & 0x2000 (kHFSVolumeJournaledMask)
     :13466-70  JIB read at blockSize × journalInfoBlock; swap {flags, offset@0x24, size@0x2c}
     :13513  JIB.flags bit 0 required (journal inside FS)
     :13542  *(u32*)hdr == 0x4a4e4c78 ('JNLx' LE)   :13543 endian marker 0x12345678
     :13545  jhdr.size == JIB.size enforced
GetDeleteRecord FUN_180011300 :13637-14252
     :13707  walk from jhdr.start;  ring wrap modulo JIB.size (:13779-83)
     :13799  LEAF nodes only (BTNodeDescriptor.kind == -1)
     :13809-17  owning fork classified (catalog/extents/attributes/allocation/startup)
     :13958-62  catalog leaf: search LIVE tree; not-found OR different fileID → DELETE record
     :14096  attribute record: found-live → skip; else volume+0x78 map
     :13859  extents not-found → volume+0x68 map
consume FUN_18000c8f0 :10034-35 (harvest maps), :10069/:10083 (emit deleted, grade 5),
     :10122 (live catalog walked unchanged after)
```

## ext4 journal (S-004 refined + inode-image salvage)

```
main flow :2773  FUN_18000e5b0(...)   pass 1 — journal inode vol+0x6c, window 0x1000
         :2778  FUN_18000e890(...)   pass 2
         :2779-80  normal walk after — passes run in the MAIN scan (healthy volumes too)
pass 1 FUN_18000e5b0 :12206-07  bswap(jsb+0) == 0xC03B3998
         :12211  blocktype ∈ {3,4};  :12212-13 blocksize@jsb+0xC, start@jsb+0x14
         :12236-46  EVERY block from jsb.start, 1 MiB windows, cancel per block
per-block FUN_180005e50 :4388-4403  dir signature ('.'/'..' leading dirents) → register
         :4419-20  inode images: i_size_lo != 0, flags & 0x80000, eh_magic == 0xf30a,
                   eh_entries != 0  (inode-size strides :4422)
         :4446-52  S_IFDIR → register by first extent
         :4514-40  NON-DIR inode images copied (0x40 bytes) into salvage list   // NEW subsystem
pass 2 FUN_18000e890 :12265-12368  jsb.start..s_maxlen;  FUN_1800043f0 :3150/:3165/:3170
         journal dir-block parent binding + per-dirent name binding into orphan sweep
         content cap i_size < 0x1400001
```

## ext4/HFS+ gate matches (session-2 confirmations)

ext4 SB gate `:3223` `(short)param_1[0xe]==0xEF53 && (*(byte*)(param_1+0x18)&1)==0` ↔ core
`cr_ext4_engine.cpp:102-104`; primary-SB-only `:3303` (single read base+0x400); 64-bit GDT
`:4974-81`; HFS+ VH gate `:10005-06` ('H+' 0x482B / 'HX' 0x4858, `&0x1ff==0`, `<0x11`) ↔ core
`:330-346`; VH read `:9128` (0x400@1024 size 0x200) ↔ core `:369`; names via
`WideCharToMultiByte(0xFDE9,0,…,'_')` `:7491`; extents-overflow key {10, cnid, startBlock}
`:7931-35`; collation ':'↔'/' fold `:11858-63`, HFSX 0xCF table dispatch `:11895`.
