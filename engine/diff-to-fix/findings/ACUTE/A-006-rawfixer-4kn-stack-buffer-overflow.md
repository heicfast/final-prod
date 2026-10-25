# ACUTE A-006 (renumbered from A-005 — ID collision with the hfsplus record-span finding, both filed in the same window) — rawfixer analyze(): 4Kn stack-buffer overflow (512-byte buffers receive 4096-byte reads on 4Kn devices)

**Severity:** ACUTE
**Module:** rawfixer
**Parameters affected:** F60 (bad-sector handling path), Q67 (sanitizer cleanliness — the overflow is not reachable in tests), Q65 (test coverage — the 4Kn path is untested because tests pin 512), A25 (algorithm steps — the FIX-3 4Kn offset fix changed read LENGTHS without resizing buffers)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/rawfixer-v2--method-inventory.md` (the og's analyze executes on the device's real sector size throughout; every boot-sector read is sector-sized, never 512-fixed); the og's 4Kn device classes anchored by `architecture/01-build-tree-intelligence.md` §Native
**Core references:** `src/core/repair/cr_rawfixer.cpp:553` (sector from geometry), `:557-558` (primary), `:572-575` (exFAT backup probe), `:589-590` (NTFS volume-end backup), `:638-639` (exFAT destroyed-primary backup) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`analyze()` derives `sector` from the device geometry (4096 on 4Kn drives — exactly the case
FIX-3/F-A4-25 was built for) but kept the four boot-sector stack buffers at `u8 x[512]` while
changing the read length to `sector`. On a 4Kn device every one of those `read_at` calls writes
4096 bytes into a 512-byte stack buffer — a 3,584-byte stack overflow per read, corrupting
adjacent locals and the return path. This is a crash / memory-corruption defect on the hardware
class the FIX-3 wave explicitly targeted.

## What the reference does

The og's RawFixer analysis chain operates sector-size-parametrically end to end: the corpus
method inventory (`rawfixer-v2--method-inventory.md`, CheckDBR/FixFromDBR/FixByScanSector
methods) reads boot sectors through the device layer with the device's own sector size, which is
why the og repairs 4Kn volumes correctly (the corpus's 512e/4Kn device handling is documented in
`01-build-tree-intelligence.md`). No fixed 512-byte boot-sector image exists anywhere in the
og's repair chain.

## What our core does

`cr_rawfixer.cpp:553`:

```cpp
const u32 sector = slice->geometry().logical_sector ? slice->geometry().logical_sector : 512;
```

then:

```cpp
u8 primary[512];                                    // :557
if (slice->read_at(0, primary, sector) != OK)       // :558 — reads `sector` bytes
```

and the same pattern at `:573-574` (`u8 bak[512]` + `read_at(12*sector, bak, sector)`),
`:589-590` (`u8 backup[512]` + `read_at(total - sector, backup, sector)`), and `:638-639`
(`u8 backup[512]` + `read_at(u64(12)*sector, backup, sector)`). The RW transport genuinely
reports 4096 on 4Kn (`src/win/cr_transport.cpp:960` `sector_bytes()` returns
`geom_.logical_sector`), so `sector == 4096` on real 4Kn hardware and each read overflows its
buffer by 3,584 bytes. The test suite cannot catch this: `tests/unit/test_rawfixer.cpp:24`
pins `RamDevice` geometry to `logical_sector = 512`, so CI never exercises the 4Kn path.

## Gap

Falsifiable claim: calling `analyze()` on any 4Kn slice (geometry.logical_sector == 4096)
with ≥ 24 sectors of size causes a 3,584-byte stack write past `primary`/`bak`/`backup` at
lines 558, 574, 590, and 639 — undefined behavior (in practice smashed stack / crash, or
silent corruption of `plan` state).

## Impact

RawFixer is the pre-scan repair stage: on 4Kn disks (increasingly common NVMe default and the
explicit target of the FIX-3 4Kn offset work) the repair planning path corrupts memory before
a plan is even produced. Severity ACUTE per the scale: crash / memory corruption on a real
hardware class. Not reachable by the current tests (512-pinned fixtures) — the shipped binary
is exposed even though CI is green.

## Reproduction / verification

- Core-side verified by direct source read this session (all four sites quoted above; the
  geometry source `cr_transport.cpp:960` and the 512-pinned test at `test_rawfixer.cpp:24`
  both read first-hand).
- Executed proof is a one-liner away from the existing suite: instantiate the rawfixer tests'
  `RamDevice` with `logical_sector = 4096` (fixture bytes re-laid at 4096-byte LBAs) — under
  ASan the very first `analyze()` reports a stack-buffer-overflow at `:558`. Not executed this
  session (read-only audit; recorded as the fail-before test for the fix leg).
- Corpus-side: the og's sector-parametric repair chain (method inventory) — the defect is a
  core-side regression introduced by the FIX-3 commit itself (F-A4-25 fixed the OFFSETS
  `12*512 → 12*sector` but left the buffers 512 and, in the process, made the LENGTHS
  sector-sized).

## Fix plan (for the engineering team)

1. Resize all four stack buffers to the runtime sector: `std::array<u8, 4096>` (max 4Kn
   logical sector) or, better, `std::vector<u8> buf(sector)` once at function entry — no
   algorithm change needed; the downstream consumers (`memcmp(primary+3, ...)`,
   `exfat_boot_sane`, `ntfs_boot_sane`, `a.after.assign(...)`) already take lengths or read
   only the fixed boot-sector fields inside the first 512 bytes.
2. In the `exfat_via_backup` branch (`:638-651`) make the restore plan sector-correct too
   (see M-046 for the co-located 512-hardcode defects): `a.length = sector`,
   `a.after.assign(backup, backup + sector)`, and `plan_exfat_checksum_after(slice, plan,
   sector, backup)`.
3. Add a 4Kn regression fixture to `test_rawfixer.cpp`: a `RamDevice` variant with
   `logical_sector = 4096` + the existing boot-sector fixtures re-laid at 4096-byte LBAs;
   run the full analyze/execute matrix under ASan (fail-before: stack-buffer-overflow).
4. De-pin the geometry: parametrize the rawfixer tests over {512, 4096} so the 4Kn path is
   permanently in CI.

## Tests to add

- `RawFixer.NtfsPrimaryDamaged4Kn` (volume-end backup restore on 4096-byte sectors)
- `RawFixer.ExfatViaBackup4Kn` (backup probe at 12*4096; sector-sized restore plan)
- `RawFixer.AnalyzeAllBranches4KnAsan` (drive every analyze branch on a 4Kn RamDevice — the
  overflow sites are all branch entries)

## Related findings

- M-046 (the co-located 4Kn plan-truncation defects in the same function — the overflow fix
  and the plan fix should land together)
- M-030 addendum (LDM resolver path — same "4Kn geometry plumbed unevenly" family)

## Open questions

None. (Whether `execute()`'s write path would also reject the 512-byte plan on 4Kn is
M-046's subject, not this finding's.)
