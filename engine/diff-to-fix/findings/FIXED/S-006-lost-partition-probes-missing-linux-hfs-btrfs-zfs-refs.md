# SEVERE S-006 — Lost-partition probes cover only NTFS/FAT/exFAT/GPT; the og's deep scan also finds Linux/FATX partitions

**Severity:** SEVERE
**Module:** lost-partition
**Parameters affected:** B1 (detection correctness), B8 (multi-volume), A30 (candidate scoring — probe set), B3
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/mo-module-map.md` — PartitionRecoveryD.mo deep-scan block parsers: **CBlockParseWin32, CBlockParserLinux, CExFatBlockParser, CExtXBlockParser, CFatxBlockParser, BitlockerParser** (agent-B verified present in `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` :661/:3528/:6463); `re-workspace/artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md` §3 (ExFatCalcPartition 54 refs / Fat32CalcPartition 39 refs — per-FS geometry calculators)
**Core references:** `src/core/lost/cr_candidates.cpp:229–235` (dispatch: NTFS / EXFAT / EFI PART / else-FAT — four families only) — audited commit `67193b6` + concurrent `03ddcd2` (probe-family set unchanged by the GAP-S6-6 fix)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The lost-partition scanner probes for exactly four boot-signature families: NTFS, exFAT,
GPT headers, and a FAT catch-all. There are **no probes for ext2/3/4, HFS+, Btrfs, ZFS, or
ReFS** superblocks/volume headers. The og's PartitionRecoveryD deep-scan carries dedicated
Linux-family block parsers (`CBlockParserLinux`, `CExtXBlockParser`) and a FATX parser. A
deleted Linux, Mac, or ReFS partition is invisible to our lost-partition pass (and ReFS's
own detector — a 1 MiB page scan — is not wired into candidate discovery either).

## What the reference does

PartitionRecoveryD.mo (28,463 lines decompiled) contains the deep-scan block-parser family
listed above — verified class presence with decompile line citations. The workflow
(mo-module-map §1.2) runs these parsers over unallocated space to find lost partitions
independently of the partition table. `CExtXBlockParser` covers the ext2/3/4 superblock
family (magic 0xEF53 at SB+0x38, offset 1024 — the same anchor the core's ext4 engine
uses); `CBlockParserLinux`/`CFatxBlockParser` cover additional Linux/Xbox-family layouts.

## What our core does

`src/core/lost/cr_candidates.cpp:229–235`:

```cpp
if (std::strncmp(oem, "NTFS    ", 8) == 0) { ... pts = probe_ntfs(...); }
else if (std::strncmp(oem, "EXFAT   ", 8) == 0) { ... probe_exfat(...); }
else if (std::memcmp(sec, "EFI PART", 8) == 0) { ... probe_gpt_header(...); }
else { c.fs = CandidateFs::Fat; pts = probe_fat(...); }
```

No 0xEF53 ext probe, no "H+"/"HX" HFS+ probe, no "_BHRfS_M" Btrfs probe, no ZFS
uberblock/label probe, no ReFS page-signature probe (`refs::detect` exists as a 1 MiB scan
in `cr_refs_detect.cpp` but is not called from candidate discovery).

## Gap

Falsifiable claim: a disk with a deleted ext4 partition (boot-signature-bearing superblock
remnant intact at the partition start) yields a lost-partition candidate in the og's deep
scan and none in ours (only content-level carving finds files, without volume structure or
names).

## Impact

- Users who deleted a Linux/Mac/ReFS partition see only raw carved files (no names, no
  paths, no structure) where the og offers structured volume recovery.
- The dual-partition delete bench shape (delete_partition) is NTFS-only today; extending
  benches to Linux FS would show the gap mechanically.
- ReFS is first-class per the audit mandate: lost ReFS partitions are undetectable.

## Reproduction / verification

- Corpus-derived: mo-module-map parser census (decompile-verified class names).
- Core-derived: dispatch code quote; `refs::detect` not referenced in `cr_candidates.cpp`
  (grep).
- Executed: construct a fixture disk with a 0x55AA+OEM-styled ext4 superblock remnant at a
  non-table LBA → `discover()` returns no candidate (deterministic from the dispatch).

## Fix plan

1. Add probe families to `cr_candidates.cpp`, reusing the engines' existing validators:
   - `probe_ext` — read 1024+512 window, `ext4::validate_superblock` (SB@+0x400, magic
     0xEF53), evidence points like the NTFS probe (OEM +15, valid SB +25, inode-table
     liveness probe +30).
   - `probe_hfsplus` — VH signature H+/HX at offset 1024, `hfsplus` volume-header
     validation.
   - `probe_btrfs` — superblock magic at 0x10000 **relative to the candidate start**, plus
     the mirror set (the candidate start acts as the disk base for the probe).
   - `probe_zfs` — vdev label/uberblock magic 0x00BAB10C at the four label positions
     relative to the candidate region.
   - `probe_refs` — wire the existing `refs::detect` (1 MiB page scan) with a bounded
     window (e.g. 8 MiB from candidate start) as the ReFS probe.
2. Extend `CandidateFs` and the dispatch; keep the FAT catch-all LAST (it is the noisiest).
3. Wire the new candidates through `materialize` and the orchestrator's engine dispatch
   (the engines already exist for all five filesystems — this is discovery wiring, not
   engine work).
4. Tests: fixture per family (deleted-partition position, intact SB/VH) —
   `Lost_Ext4_Partition_Discovered`, `Lost_HfsPlus_...`, `Lost_Btrfs_...`,
   `Lost_Zfs_...`, `Lost_Refs_...`; plus a noise case per family (wrong magic → no
   candidate).
5. Bench: add an ext4 delete_partition scenario to comparative-bench.yml once wired.

## Tests to add

(fix plan step 4 — all fail before, pass after)

## Related findings

- S-007 (carve format coverage — the fallback layer that currently carries these users).

## Open questions

The og's *scoring* internals (CalcPartition classes, AdjustPartitionInfoOrder ranking)
remain corpus-undecoded (tracked-open OQ-T9); this finding covers the probe SET, not the
scores.

## Session-2 update (2026-10-25)

The scoring half of the lost-partition story is now fully decoded by this leg: the og ranks
candidates by content-driven ln(count)/ln(max) with per-FS weights, a search-DLL bonus, and
+500 for a partition-table match — see **M-017** (this leg; resolves OQ-T9 with a
187-log-row-verified formula) and `../evidence/decompiled-quotes/ldm-raid5-and-scoring.md`.
Implement M-017 together with the probe-family wiring this finding carries, so candidate
content probes feed the og formula directly. The probe-family enumeration itself (Check* /
CExtXBlockParser accept criteria, deep-scan grid constants) is still artifact-derived —
unchanged from session 1's evidence.

## Resolution

**Status:** FIXED (with a scoped remainder: the og's FATX parser, and M-017's scoring formula as its own finding)
**Fixed in commit:** (this session's S-006 fix commit — see `git log` for "fix: S-006")
**Verified by:** fail-before/pass-after executed this session: with only `cr_candidates.cpp` reverted, 5 of 7 new tests FAIL (`Ext4SuperblockAnchorsDeletedPartition`, `HfsPlusVolumeHeaderAnchorsDeletedPartition`, `BtrfsSuperblockAnchorsDeletedPartition`, `ZfsLabelAnchorsDeletedPartition`, `RefsBootAnchorsDeletedPartition` — no candidates surfaced); the 2 negative guards (`Ext4BlockGroupCopyNotACandidateStart`, `SuperblockNoiseRejected`) pass both sides by design. Full suite 332/332 Linux ASan (325 baseline + 7 new). Corpus verification first-hand this session: the parser-tree wiring decoded from `PartitionRecoveryD_decompiled.c` (CBlockParseWin32 ctor FUN_180001c50 wires CFatxBlockParser + CNTFSBlockParser + CExFatBlockParser + BitlockerParser; CBlockParserLinux ctor FUN_180001b40 wires CExtXBlockParser as its ONLY sub-parser); the CExtX accept criteria decoded in full (validator FUN_180002880: magic 0xEF53 @SB+0x38, nonzero inodes/blocks/bpg/ipg, s_first_ino>=3, group-count cross-check `(blocks-first_data_block-1)/bpg+1 == (inodes-1)/ipg+1`, first_data_block==(block_size==1024); parse FUN_180002b90: s_block_group_nr==0 — the partition-start anchor).
**Resolution notes:** classification CONFIRMED-DIVERGENCE for the ext family (the finding's falsifiable claim — a deleted ext4 partition yields a lost-partition candidate in the og and none in ours — is now closed: `probe_ext` implements the og's exact accept gates on top of the core's `ext4::validate_superblock`, plus a GDT liveness probe; SB-derived real size, no device-end fallback for intact superblocks). **Finding corrected against the corpus (corpus wins):** the og's PartitionRecoveryD deep scan carries NO HFS+/Btrfs/ZFS/ReFS block parsers (the ctor wiring above is exhaustive) — the finding's impact paragraph over-claimed og coverage for "a deleted Linux, Mac, or ReFS partition" yielding candidates in the og; only the ext family is og lost-scan coverage. The HFS+/Btrfs/ZFS/ReFS probes are therefore implemented and documented as DELIBERATE SUPERSETS (docs/PARITY-DELTAS.md D-50): the core's engines exist for all four (the materialize->probe_family->engine dispatch was already wired — this fix adds only the discovery anchors), ReFS is first-class per the audit mandate, and the superset family is consistent with the acknowledged I-003 (Btrfs deep-scan superset). The ReFS DBR (OEM "ReFS" + 55AA) previously fell into the FAT catch-all and was dropped as noise — it now dispatches to `probe_refs` before the catch-all, which runs the existing `refs::detect_refs` page-signature probe over a bounded 8 MiB window from the candidate start. ZFS anchors on the label-0 uberblock ring head (LE magic 0x00BAB10C at +128 KiB, txg liveness) — the same anchor `probe_family` uses; Btrfs anchors on the primary superblock at +64 KiB (`btrfs::validate_superblock`). To bound scan cost, the two out-of-window anchors (btrfs +64 KiB, ZFS +128 KiB) are gated on 1 MiB grid alignment (the standard partition-start alignment; ~2 bounded reads per MiB) — non-aligned lost Linux-partition starts below 1 MiB granularity are not surfaced by these two superset probes (documented in D-50; ext/HFS+/FAT/NTFS/exFAT/ReFS remain stride-1). ZFS/Btrfs/ReFS candidate sizes carry the flagged `size_from_device_end` fallback (the label/MSB geometry is engine-side work; the GAP-S6-6 dedup rule keeps fallback-sized candidates from claiming containment). `CandidateFs` appended Ext2..Refs (6..10; session-serialized positions preserved; the resume-side validation bound extended with old sessions staying valid). **Scoped remainder:** the og's `CFatxBlockParser` (Xbox-original FATX, a legacy EaseUS surface per mo-module-map insight 7) is NOT implemented — its accept criteria are not decoded in the corpus at parser depth (the chain FUN_180006250 -> FUN_180004070/FUN_180004610 was entered but not resolved this session); per the no-hallucinated-parity rule the FATX probe is tracked as this finding's remainder rather than guessed (next step: decode the CFatxBlockParser vtable parse in PartitionRecoveryD_decompiled.c). **M-017 split:** the finding's session-2 update asked for the scoring formula to land together; this session lands the probe families (M-017 remains open in its own right — its ln(count)/ln(max) formula operates on exactly the content counts these probes now surface, and implementing it under its own ID keeps one-finding-one-fix). The finding's test names are realized under the suite's `LostDiscovery` prefix.
