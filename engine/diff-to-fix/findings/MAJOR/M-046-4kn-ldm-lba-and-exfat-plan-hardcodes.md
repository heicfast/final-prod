# MAJOR M-046 — 4Kn wrong-offset / truncated-plan defects: LDM GPT path hardcodes 512-byte LBAs; exFAT repair plans write 512 of 4096 bytes

**Severity:** MAJOR
**Module:** ldm + rawfixer
**Parameters affected:** C20 (on-disk struct offsets — LBA→byte math must use device sector), B2 (recovery correctness — LDM metadata invisible on 4Kn GPT disks), A25 (algorithm steps — repair plan must be sector-sized), F60 (bad-sector/repair path correctness)
**Corpus references:** `re-workspace/artifacts/MOM-modules/LdmManager.md` (LDM metadata discovery via the GPT partition entry — LBA units are the device's); `re-workspace/artifacts/PartitionRecovery/rawfixer-v2--method-inventory.md` (sector-parametric repair chain); `architecture/01-build-tree-intelligence.md` §Native (512e/4Kn first-class)
**Core references:** `src/core/table/cr_ldm.cpp:1007` (`read_at(u64(1) * 512, ...)`), `:1014` (`gh.entries_start_lba * 512`), `:1021` (`e->last_lba * 512`); contrast `src/core/table/cr_table_parse.cpp:19-22` (`u64 off = lba * g.logical_sector` — the main GPT parser is geometry-correct); `src/core/repair/cr_rawfixer.cpp:644-649` (`a.length = 512`, `a.after.assign(backup, backup + 512)`, `plan_exfat_checksum_after(slice, plan, 512, backup)`), `:702-709` (`u8 backup[512]` + `read_at(12 * bps, backup, 512)` + `a.length = 512`) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two co-located 4Kn geometry defects survive the FIX-3 wave: (1) the LDM-over-GPT discovery
path multiplies LBAs by a hardcoded 512 while the main GPT parser correctly uses
`geometry.logical_sector` — on 4Kn disks the LDM metadata partition is looked up at the wrong
byte offsets and dynamic-disk recovery fails; (2) the exFAT repair branches plan 512-byte
restores and a 512-byte checksum span even when the boot sector is 4096 bytes — the execute
path would write one-eighth of the sector and checksum the wrong span.

## What the reference does

The og's LdmManager locates the LDM metadata partition through the GPT entry's LBA fields in
device-LBA units (corpus `LdmManager.md`), i.e. byte offsets scale with the device sector
size; its repair chain is sector-parametric throughout (`rawfixer-v2--method-inventory.md`).
Neither component contains a 512-fixed LBA or a 512-fixed boot-sector image.

## What our core does

**LDM GPT path** (`cr_ldm.cpp:1006-1023`):

```cpp
if (dev->read_at(u64(1) * 512, &gh, sizeof(gh)) != OK)          // :1007
...
if (dev->read_at(gh.entries_start_lba * 512, entries.data(), ...)// :1014
...
ph_byte_off = e->last_lba * 512;                                 // :1021
```

On a 4Kn disk, GPT header LBA 1 lives at byte 4096, not 512; the entries array and the LDM
metadata partition offset are likewise 8x off. The main parser in the same tree is correct
(`cr_table_parse.cpp:19-22`: `u64 off = lba * g.logical_sector;`), so this is an internal
inconsistency, not a project-wide assumption.

**exFAT repair plans** (`cr_rawfixer.cpp`): in the destroyed-primary branch
(`:638-651`) and the damaged-primary branch (`:701-711`), the restore action is
`a.length = 512` / `a.after.assign(backup, backup + 512)` and the checksum re-plan passes
`512` (`:649`) — even though `sector` (geometry) and `bps` (boot-declared) are both 4096 on a
4Kn exFAT volume, and the damaged-primary backup read at `:703` fetches only 512 of the
4096-byte backup sector (`read_at(12 * bps, backup, 512)`).

## Gap

Falsifiable claims: (a) a 4Kn GPT disk with an LDM metadata partition: `cr_ldm`'s GPT-path
lookup reads the GPT header at byte 512 (mid-sector-0) → `ERR_BAD_MAGIC`-family failure → no
dynamic-disk recovery, while `cr_table_parse` on the same disk succeeds; (b) a 4Kn exFAT
volume with a damaged primary boot: the planned restore writes 512 bytes at offset 0 of a
4096-byte sector and the checksum plan covers 512 bytes of an 11×4096-byte boot region.

## Impact

LDM on 4Kn GPT disks (ReFS/dynamic-disk scenarios — LDM is first-class per the mandate) is
unrecoverable through this path. exFAT repair on 4Kn produces wrong plans — a partial-sector
write on execution (or an envelope rejection) and a checksum computed over the wrong span.
Neither is caught by CI because the rawfixer/LDM fixtures pin 512-byte geometry
(`test_rawfixer.cpp:24`; LDM fixtures likewise).

## Reproduction / verification

- Core: all cited lines read first-hand this session, including the contrast pair
  (cr_ldm.cpp 512-hardcodes vs cr_table_parse.cpp geometry-correct) and the exFAT plan
  literals.
- Corpus: sector-parametric reference behavior per the method inventory + LdmManager LBA
  semantics.
- Executed comparison deferred to the fix leg (fail-before tests listed below); the same
  class was proven executable in the A-005 harness plan.

## Fix plan (for the engineering team)

1. `cr_ldm.cpp`: thread the device geometry into the GPT-path lookup — replace the three
   `* 512` sites with `* dev->geometry().logical_sector` (guarding the zero case exactly like
   `cr_table_parse.cpp:19` does).
2. `cr_rawfixer.cpp` exFAT branches: use `sector` (geometry) in the destroyed-primary branch
   and `bps` (boot-declared) in the damaged-primary branch for `a.length`,
   `a.after.assign`, and the `plan_exfat_checksum_after` span; fix the `:703` read length to
   the same value (this also removes the truncated backup image that `exfat_boot_sane`
   currently validates).
3. Land together with A-005's buffer resize (same functions, same fixtures).
4. Tests: 4Kn variants of the LDM GPT-path fixture and the exFAT repair matrix
   (`RamDevice` with `logical_sector = 4096`, fixtures re-laid at 4096-byte LBAs).

## Tests to add

- `Ldm.GptPathFindsMetadata4Kn` (4Kn GPT disk + LDM metadata entry → path resolves)
- `RawFixer.ExfatRestorePlanIsSectorSized4Kn` (plan length == 4096, checksum span == 11
  sectors × 4096)
- `RawFixer.ExfatBackupReadFull4Kn` (backup image validated from all 4096 bytes)

## Related findings

- A-005 (the buffer overflow at the same sites — the joint fix is one review unit)
- M-030 addendum (transport/driver 4Kn-adjacent family)
- A-001/A-002 lineage (LDM GUID correctness — same module's prior ACUTEs, both FIXED)

## Open questions

None.
