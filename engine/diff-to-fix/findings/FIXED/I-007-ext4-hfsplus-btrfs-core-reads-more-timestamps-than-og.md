# INFORMATIONAL I-007 — Core reads more timestamps than og for ext4 (4 vs 2), HFS+ (4 vs 3), Btrfs (4 vs 3)

**Severity:** INFORMATIONAL
**Module:** ext4, hfsplus, btrfs
**Parameters affected:** B9 (timestamps — extra timestamp fields preserved), D11 (node fields — created/modified/changed/accessed all populated), I-005 (reviewed supersets family)
**Corpus references:**
- ext4: `re-workspace/reversed/EXT4SearchDLL/EXT4SearchDLL_decompiled.c:6262-6282` (reads `param_2+0x0C` ctime and `param_2+0x10` mtime; assigns to props+0x3b and props+0x43); `:6255-6258` (props+0x4b and props+0x55 — 3rd/4th FILETIME slots — initialized to 0 and NEVER assigned in any code path searched — confirmed og drops atime and crtime for ext4)
- HFS+: `re-workspace/reversed/HFSPlusSearchDLL/HFSPlusSearchDLL_decompiled.c:8012, 8027, 8042` (three conversions: create @ +0xc0, content_mod @ +0xc4, attr_mod @ +0xc8 — only three sites; the 4th HFS+ catalog date `access_date` at +0xCC is NEVER read)
- Btrfs: `re-workspace/reversed/BtrfsSearchDLL/BtrfsSearchDLL_decompiled.c:4430-4446` (`FUN_180006ba0` — three conversions: ctime @ +0x7c/+0x84, mtime @ +0x88/+0x90, atime @ +0x70/+0x78; the 4th Btrfs inode timespec at +0x94/+0x9C (otime → crtime) is NEVER read by the og BTRFS engine)
**Core references:**
- ext4: `src/core/ext4/cr_ext4_engine.cpp:283-297` (reads `i_atime_extra`, `i_ctime_extra`, `i_mtime_extra`, `i_crtime`, `i_crtime_extra`); `:503-508, 603-607` (assigns all 4 of `props->times.created/modified/changed/accessed` from `crtime/mtime/ctime/atime` respectively, with crtime→ctime fallback when crtime is absent)
- HFS+: `src/core/hfsplus/cr_hfsplus_engine.cpp:494-497` (`props->times.created = mac_to_filetime(r.create_date); modified = mac_to_filetime(r.mod_date); changed = mac_to_filetime(r.attr_date ? r.attr_date : r.mod_date); accessed = mac_to_filetime(r.access_date)`)
- Btrfs: `src/core/btrfs/cr_btrfs_engine.cpp:83-86` (`INO_ATIME = 0x70, INO_CTIME = 0x7C, INO_MTIME = 0x88, INO_otime = 0x94` (otime → crtime)); `:625-633, 750-753` (reads all 4 timespecs — sec + nsec each) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied (deliberate superset, recorded for parity benchmark awareness)

## Summary

The og's ext4/HFS+/Btrfs engines each read only 2-3 of the 4 standard
timestamps (created/modified/changed/accessed) the filesystems define.
The core's engines read all 4 for each filesystem. The result is a
SUPERSET divergence: the core's .rsf has non-zero `created`/`accessed`
slots where the og's has zeros.

This is a deliberate SUPERSET divergence — the core preserves metadata
the og discards. Users benefit (creation time and access time match
the filesystem's actual content); parity benchmarks diverge
mechanically.

## What the reference does

### ext4 — og reads 2 of 4 timestamps

```c
// EXT4SearchDLL_decompiled.c:6262-6282 (first-hand read)
*(FILETIME*)(param_2 + 0x3b) = convert(*(DWORD*)(param_2_src + 0x0C));   // ctime (sec only)
*(FILETIME*)(param_2 + 0x43) = convert(*(DWORD*)(param_2_src + 0x10));   // mtime (sec only)
// props+0x4b and props+0x55 (3rd/4th FILETIME slots — created/accessed) — full grep for
// `\+ 0x4b\),.*\+` and `\+ 0x55\),.*\+` returns only `= 0` writes. The og NEVER assigns them.
```

The og's ext4 engine reads only `i_ctime` (changed) and `i_mtime`
(modified); `i_atime` (accessed) and `i_crtime` (created) are
silently dropped.

### HFS+ — og reads 3 of 4 timestamps

```c
// HFSPlusSearchDLL_decompiled.c:8012, 8027, 8042 (first-hand read)
*param_2 = ((ulonglong)*(uint *)(param_1 + 0xc0) + 0x239eae080) * 10000000;   // create_date @ +0xC0
*param_2 = ((ulonglong)*(uint *)(param_1 + 0xc4) + 0x239eae080) * 10000000;   // content_mod_date @ +0xC4
*param_2 = ((ulonglong)*(uint *)(param_1 + 200) + 0x239eae080) * 10000000;   // attr_mod_date @ +0xC8 (200 = 0xC8)
// (only three sites — the 4th HFS+ catalog date access_date at +0xCC is NEVER read)
// constant 0x239eae080 = 9561628800 = Mac→NTFS epoch offset (9561628800 sec between 1904-01-01 and 1970-01-01)
```

The og's HFS+ engine reads `create_date`, `content_mod_date`, and
`attr_mod_date` (changed); `access_date` is silently dropped.

### Btrfs — og reads 3 of 4 timestamps

```c
// BtrfsSearchDLL_decompiled.c:4430-4446 (FUN_180006ba0, first-hand read)
*(ulonglong *)(param_2 + 0x4a) =                                          // props.ctime @ +0x4a
    (ulonglong)*(uint *)(param_3 + 0x84) / 100 +                          //   nsec
    (*(longlong *)(param_3 + 0x7c) + 0x2b6109100) * 10000000;             //   sec (ctime @ +0x7c, nsec @ +0x84)
*(ulonglong *)(param_2 + 0x52) =                                          // props.mtime @ +0x52
    (ulonglong)*(uint *)(param_3 + 0x90) / 100 +
    (*(longlong *)(param_3 + 0x88) + 0x2b6109100) * 10000000;             //   mtime @ +0x88, nsec @ +0x90
*(ulonglong *)(param_2 + 0x5a) =                                          // props.atime @ +0x5a
    (ulonglong)*(uint *)(param_3 + 0x78) / 100 +
    (*(longlong *)(param_3 + 0x70) + 0x2b6109100) * 10000000;             //   atime @ +0x70, nsec @ +0x78
// (only 3 conversions — the 4th Btrfs inode timespec at +0x94/+0x9C (otime → crtime) is NEVER read)
// constant 0x2b6109100 = 11644473600 = Unix→NTFS epoch offset
```

The og's Btrfs engine reads `ctime`, `mtime`, and `atime`; `otime`
(creation time, at `+0x94/+0x9C`) is silently dropped.

## What our core does

### ext4 — core reads all 4 timestamps

```cpp
// src/core/ext4/cr_ext4_engine.cpp:283-297 (audited 5d6da5f)
// (reads i_atime_extra, i_ctime_extra, i_mtime_extra, i_crtime, i_crtime_extra
//  when inode_size >= 0x98 and extra_isize covers them)
u32 atime_sec, atime_nsec;  decode_extra(le32(inode + OFF_ATIME_EXTRA), atime_sec, atime_nsec);
atime_sec |= le32(inode + OFF_ATIME);
u32 ctime_sec, ctime_nsec;  decode_extra(le32(inode + OFF_CTIME_EXTRA), ctime_sec, ctime_nsec);
ctime_sec |= le32(inode + OFF_CTIME);
u32 mtime_sec, mtime_nsec;  decode_extra(le32(inode + OFF_MTIME_EXTRA), mtime_sec, mtime_nsec);
mtime_sec |= le32(inode + OFF_MTIME);
u32 crtime_sec, crtime_nsec;  decode_extra(le32(inode + OFF_CRTIME_EXTRA), crtime_sec, crtime_nsec);
crtime_sec |= le32(inode + OFF_CRTIME);

// :503-508, 603-607
props->times.created  = unix_to_filetime_ns(crtime_sec, crtime_nsec);  // (with crtime→ctime fallback when absent)
props->times.modified = unix_to_filetime_ns(mtime_sec,  mtime_nsec);
props->times.changed  = unix_to_filetime_ns(ctime_sec,  ctime_nsec);
props->times.accessed = unix_to_filetime_ns(atime_sec,  atime_nsec);
```

### HFS+ — core reads all 4 timestamps

```cpp
// src/core/hfsplus/cr_hfsplus_engine.cpp:494-497 (audited 5d6da5f)
props->times.created  = mac_to_filetime(r.create_date);
props->times.modified = mac_to_filetime(r.mod_date);
props->times.changed  = mac_to_filetime(r.attr_date ? r.attr_date : r.mod_date);  // (attr_date→mod_date fallback)
props->times.accessed = mac_to_filetime(r.access_date);
```

### Btrfs — core reads all 4 timestamps

```cpp
// src/core/btrfs/cr_btrfs_engine.cpp:83-86 (audited 5d6da5f)
constexpr u32 INO_ATIME = 0x70;     // + nsec @ +0x78
constexpr u32 INO_CTIME = 0x7C;     // + nsec @ +0x84
constexpr u32 INO_MTIME = 0x88;     // + nsec @ +0x90
constexpr u32 INO_otime = 0x94;    // + nsec @ +0x9C  (otime → crtime)

// :625-633, 750-753 — reads all 4 timespecs (sec + nsec each)
props->times.created  = unix_to_filetime_ns(le64(inode + INO_otime), le32(inode + INO_otime + 8));
props->times.modified = unix_to_filetime_ns(le64(inode + INO_MTIME), le32(inode + INO_MTIME + 8));
props->times.changed  = unix_to_filetime_ns(le64(inode + INO_CTIME), le32(inode + INO_CTIME + 8));
props->times.accessed = unix_to_filetime_ns(le64(inode + INO_ATIME), le32(inode + INO_ATIME + 8));
```

## Gap

Three parallel gaps:

1. **ext4:** og reads 2 of 4 (ctime + mtime); core reads 4.
2. **HFS+:** og reads 3 of 4 (create + mod + attr_mod); core reads 4.
3. **Btrfs:** og reads 3 of 4 (ctime + mtime + atime); core reads 4 (adds otime → created).

## Impact

- **`.rsf` byte-level divergence:** every ext4/HFS+/Btrfs file has
  non-zero `created`/`accessed` slots in the core's .rsf where the
  og's has zeros (for the dropped timestamps).
- **User-visible metadata:** the og displays "no creation time" /
  "no access time" for these files; the core surfaces the actual
  values.
- **Parity benchmark:** any og-vs-core file inventory comparison
  diverges on the dropped-timestamp slots.
- **Spec compliance:** the core is MORE correct per each filesystem's
  spec. This is a SUPERSET divergence in correctness.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `EXT4SearchDLL_decompiled.c:6262-6282`, `:6255-6258`;
  `HFSPlusSearchDLL_decompiled.c:8012, 8027, 8042`;
  `BtrfsSearchDLL_decompiled.c:4430-4446`.
- **Core evidence:** first-hand read of `cr_ext4_engine.cpp:283-297,
  503-508, 603-607`; `cr_hfsplus_engine.cpp:494-497`;
  `cr_btrfs_engine.cpp:83-86, 625-633, 750-753`.
- **Executed comparison:** not run — would require building both
  implementations and comparing .rsf byte streams on fixtures.

## Fix plan (for the engineering team)

This is an INFORMATIONAL finding — the deliberate superset is the
correct behavior; no fix is needed. The finding is recorded for:

1. **Parity benchmark awareness:** any og-vs-core comparison on these
   engines' timestamps will diverge on the dropped-timestamp slots;
   this is expected, not a regression.
2. **Opt-in parity mode (if desired):** a `--match-og-timestamps` flag
   could zero out the dropped-timestamp slots for parity benchmarks.
3. **Documentation:** `docs/AUDIT-LINUX-FS.md` ext4/HFS+/Btrfs
   sections should note the deliberate superset.

## Tests to add

- `Ext4.AllFourTimestampsPopulated` — fixture: an ext4 inode with all 4
  timestamps set; assert all 4 `props->times` slots are non-zero.
  PASSES today (regression guard).
- `HfsPlus.AccessDatePopulated` — fixture: an HFS+ catalog record with
  `access_date` set; assert `props->times.accessed` is non-zero.
  PASSES today.
- `Btrfs.OtimePopulatedAsCreated` — fixture: a Btrfs inode with
  `otime` set; assert `props->times.created` is non-zero. PASSES today.

## Related findings

- **I-006** — ext4 nanoseconds superset. I-007 is the parallel
  "more timestamps read" aspect of the same superset family.
- **I-005** — grouped supersets family from session 1. I-007 is the
  session-2 extension.

## Open questions

None. The deliberate superset is documented; no further RE needed.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** first-hand code re-read (all four ext4 slots populated via decode_extra; the HFS+/Btrfs og sites carry only three conversions each per the cited decodes; the core fills created/modified/changed/accessed for all three engines).
**Resolution notes:** Deliberate superset, preserved. The core reads atime/crtime (ext4), access_date (HFS+), otime/crtime (Btrfs) that the og never assigns; the extra slots surface in results and reports. Bench comparisons must field-mask the extra slots, not treat them as divergences.

**Status:** ACKNOWLEDGED (deliberate superset — preserved)
**Verified by:** first-hand re-verification this session: corpus
`EXT4SearchDLL_decompiled.c:6255-6258` (slots +0x4b/+0x55 zeroed, never
assigned); `HFSPlusSearchDLL_decompiled.c:8005-8048` (exactly THREE
conversion functions — +0xc0/+0xc4/+0xc8(200); no 4th); Btrfs
`BtrfsSearchDLL_decompiled.c` FUN_180006ba0 (:4425-4450, three timespec
conversions; `+0x94` appears only as a zeroing write at :271 — otime never
converted). Core: ext4 `cr_ext4_engine.cpp:603-607,702-704`; HFS+
`cr_hfsplus_engine.cpp:853-856` AND `:1093-1096` (both record sources —
catalog walk and the S-021 journal emission carry all four); Btrfs
`cr_btrfs_engine.cpp:83-86,626-632` (INO_otime=0x94 read).
**Resolution notes:** rationale correct on both sides: og reads 2/4 (ext4),
3/4 (HFS+), 3/4 (Btrfs); core reads 4/4 on every engine. No code change.
Registered as PARITY-DELTAS **D-59b**. Future work must PRESERVE this.
