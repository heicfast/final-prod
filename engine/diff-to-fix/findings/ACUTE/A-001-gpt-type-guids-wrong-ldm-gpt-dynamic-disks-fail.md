# ACUTE A-001 — Six GPT type-GUID constants are wrong: GPT dynamic disks get zero LDM recovery; ESP/MSR/Linux misclassified

**Severity:** ACUTE
**Module:** table, ldm
**Parameters affected:** C19 (magic bytes and signatures), B1 (detection correctness), B8 (multi-volume/cross-partition), A30 (lost-partition candidate inputs), Q68 (documentation accuracy — the fix comment is false)
**Corpus references:** og binaries themselves (ground truth): `og-codebase/PartitionRecovery/Partition.mo`, `LdmManager.mo`, `MainModule.dll`, `RecoveryModule.dll`, `UILogic.dll`, `DeviceManager.mo` contain the **canonical** ESP GUID `c12a7328-f81f-11d2-ba4b-00a0c93ec93b` and MSR GUID `e3c9e316-0b5c-4db8-817d-f92df00215ae` byte-for-byte (binary scan this session, `evidence/executions/gpt-guid-probe.md`); canonical LDM GUIDs per Microsoft `winioctl.h` `PARTITION_LDM_METADATA_GUID` {5808c8aa-8e9f-4c87-aea2-af0aa3c41687} / `PARTITION_LDM_DATA_GUID` {af9b60a0-384e-4f7d-8fa5-b1c3d6a9e2b0}
**Core references:** `include/cr/table/cr_table_wire.h:58–75` (six constants), consumed at `src/core/table/cr_table_parse.cpp:269–279` (classification) and `src/core/table/cr_ldm.cpp:1020` (LDM metadata partition finder) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Six GPT type-GUID constants in `cr_table_wire.h` do not match the canonical values: BIOS
boot, Microsoft Reserved (which currently holds the **EFI System Partition** GUID — a
duplicate), Linux filesystem, LDM metadata, and LDM data. The LDM metadata GUID failure
means **every GPT dynamic disk fails `parse_ldm_database` with `ERR_BAD_MAGIC`** — zero
dynamic-volume recovery for GPT dynamic disks (MBR/0x42 dynamic disks still work). ESP
partitions classify as `Reserved` (via the duplicated GUID), real MSR partitions as
`Other`, Linux data partitions as `Other`. The header comment claims the values were
"corrected against Microsoft PARTITION_LDM_*_GUID (winioctl.h), the Linux kernel
block/partitions/efi.h and libldm — byte-for-byte identical in all three": **that claim is
false**, which is its own hazard (future readers will trust it).

## What the reference does

The og binaries embed the canonical GUIDs (binary scan found ESP and MSR byte-for-byte in
six PartitionRecovery binaries; `evidence/executions/gpt-guid-probe.md` lists offsets).
Canonical values (mixed-endian disk form, as they appear in a GPT entry):

| Type | Canonical GUID | Canonical disk bytes |
|---|---|---|
| EFI System | c12a7328-f81f-11d2-ba4b-00a0c93ec93b | `c12a7328f81f11d2ba4b00a0c93ec93b` |
| Microsoft Reserved | e3c9e316-0b5c-4db8-817d-f92df00215ae | `16e3c9e35c0bdb4817df92df00215ae` |
| BIOS boot | 21686148-6449-6e6f-744e-656564454649 | `4861682164496e6f744e656564454649` ("Hah!IdontNeedEFI") |
| Linux filesystem | 0fc63daf-8483-4772-8e79-3d69d8477de4 | `0fc63daf848347728e793d69d8477de4` |
| LDM metadata | 5808c8aa-8e9f-4c87-aea2-af0aa3c41687 | `aac808589f8e874caea2af0aa3c41687` |
| LDM data | af9b60a0-384e-4f7d-8fa5-b1c3d6a9e2b0 | `a0609baf4e387d4f8fa5b1c3d6a9e2b0` |

## What our core does

`include/cr/table/cr_table_wire.h:58–75` (audited `67193b6`):

```cpp
constexpr u8 GPT_TYPE_BIOS_BOOT[16] = {0x48,0x61,0x68,0x21,0x49,0x64,0x65,0x6f,
                                       0x4e,0x65,0x65,0x64,0x45,0x46,0x49,0x00};   // "Hah!IdeoNeedEFI\0"
constexpr u8 GPT_TYPE_MICROSOFT_RESERVED[16] = {0xC1,0x2A,0x73,0x28,...};            // == ESP GUID
constexpr u8 GPT_TYPE_LINUX_FS[16] = {0x06,0x57,0xFD,0x6D,...};                      // mangled hybrid
constexpr u8 GPT_TYPE_LDM_METADATA[16] = {0xAA,0xC8,0x08,0x58,0x8F,0x7E,0xE0,0x42,
                                          0x85,0xD2,0xE1,0xE9,0x04,0x34,0xCF,0xB3};  // first dword only
constexpr u8 GPT_TYPE_LDM_DATA[16] = {0xA0,0x60,0x9B,0xAF,0x31,0x14,0x62,0x4F,
                                      0xBC,0x68,0x33,0x11,0x71,0x4A,0x69,0xAD};       // first dword only
```

The comment directly above the LDM pair claims verification against winioctl.h, the Linux
kernel, and libldm — the values match **none** of them past the first 4 bytes (LDM) or at
all (BIOS boot, MSR, Linux).

**Executed proof** (this session, harness at `scratch` preserved into
`evidence/executions/gpt-guid-probe.md`): a GPT image built with canonical GUIDs →

```
parse_disk_table rc=0 partitions=4
  part start=2048 sectors=1024 fs=12(Other)   <- LDM metadata partition
  part start=3072 sectors=991  fs=12(Other)   <- LDM data partition
  part start=4096 sectors=4096 fs=10(Reserved) <- ESP (classified Reserved via dup GUID)
  part start=8192 sectors=1024 fs=12(Other)   <- MSR
parse_ldm_database rc=0x50030001 (ERR_BAD_MAGIC)
VERDICT: GPT-dynamic-disk LDM discovery FAILS
```

## Gap

Falsifiable (and executed): on a GPT disk whose LDM metadata partition carries the
canonical GUID, `parse_ldm_database` returns `ERR_BAD_MAGIC` and no LDM volume is
assembled. On the same disk, `parse_disk_table` classifies ESP as `Reserved`, MSR as
`Other`, Linux partitions as `Other`.

## Impact

- **GPT dynamic disks (spanned/striped/mirrored/RAID-5 volumes): zero recovery.** Windows
  converts a dynamic disk to GPT layout when the underlying disk is GPT — a large fraction
  of the dynamic-disk population. Silent failure: the user simply does not see the volumes.
- ESP/MSR/Linux misclassification: wrong `fs` reporting downstream (lost-partition probe
  inputs, report fields); ESP content recovery unaffected in practice only because the
  orchestrator re-probes partitions by content — but any consumer of `PartitionInfo.fs`
  sees wrong values.
- The false verification comment invites future sessions to trust the constants — this is
  the second time these GUIDs have been "fixed" wrongly (the comment itself says the data
  GUID was previously a fabricated 3B8F0848-... value).

## Reproduction / verification

1. **Executed comparison** (strength 1): the harness above — GPT fixture with canonical
   GUIDs, core parser run, `ERR_BAD_MAGIC` + misclassifications observed. Log preserved in
   `evidence/executions/gpt-guid-probe.md` with the harness source.
2. **Og-binary evidence** (strength 4): ESP/MSR canonical bytes found in six og
   PartitionRecovery binaries (scan log in the same evidence file).
3. **Documented-contract evidence** (strength 3): winioctl.h GUIDs (Microsoft-documented
   stable ABI contract).

## Fix plan

1. Replace the six constants in `include/cr/table/cr_table_wire.h` with the canonical
   disk-byte values from the table above (all six, in one commit — do not partial-fix).
2. Fix `GPT_TYPE_MICROSOFT_RESERVED` to the real MSR GUID (`16e3c9e35c0bdb4817df92df00215ae`)
   — this also un-duplicates it from ESP, restoring correct ESP classification.
3. Rewrite the comment to cite THIS audit + the executed proof instead of the false
   three-source claim.
4. Add a compile-time regression test that round-trips each constant through
   `ldm_parse_guid_text`-style text↔bytes conversion against the canonical text GUIDs
   (the test-suite already has GUID text parsing at `tests/unit/test_ldm.cpp:79`).
5. Files: `include/cr/table/cr_table_wire.h`; tests in `tests/unit/test_crc32_gpt.cpp`
   (GPT family) + `tests/unit/test_ldm.cpp`.
6. Acceptance: the executed probe (kept as a test) classifies
   ESP→Reserved? No — ESP must classify as its own kind (add `FsKind::EfiSystem` or map
   ESP→Reserved deliberately and document); MSR→Reserved; Linux→Linux;
   LDM metadata+data found; `parse_ldm_database` proceeds past the GPT scan on the
   canonical fixture.

## Tests to add

- `test_crc32_gpt.cpp::Canonical_Guid_Table` — static_assert-style comparison of each
  constant against canonical bytes (fails before, passes after).
- `test_ldm.cpp::Gpt_Dynamic_Disk_Canonical_Guids_Located` — the executed probe as a unit
  test (LDM metadata partition found, `parse_ldm_database` ≠ ERR_BAD_MAGIC on a
  PRIVHEAD-bearing fixture).
- `test_crc32_gpt.cpp::Esp_Msr_Linux_Classification` — canonical ESP/MSR/Linux entries
  classify as Reserved/Reserved/Linux respectively.

## Related findings

- M-002 (EBR salvage — same module family).
- OQ-T7 (LDM RAID-5 rotation — the next thing that will bite after this fix lands).

## Open questions

ESP classification intent: the current code has no ESP-specific kind; after the fix,
decide whether ESP maps to `Reserved` (skip-scan posture) or gets its own kind surfaced in
reports. The og treats ESP as a scannable FAT volume (DRW lists it as a volume); if the
core intends og parity there, ESP should NOT be lumped under Reserved. Resolve during the
fix (needs a one-line product decision, recorded here).
