# gpt-guid-probe — executed evidence (session, main auditor)

## Og-binary canonical-GUID scan (python, PartitionRecovery/* + DRWUI.exe)
```
ESP  c12a7328f81f11d2ba4b00a0c93ec93b -> UILogic.dll, MainModule.dll, LdmManager.mo, DeviceManager.mo, Partition.mo, RecoveryModule.dll
MSR  16e3c9e35c0bdb4817df92df00215ae -> UILogic.dll, MainModule.dll, LdmManager.mo, Partition.mo, RecoveryModule.dll, ToolsModule.dll
LDM_META / LDM_DATA / BIOSBOOT / LINUX_FS / BASIC: not present as contiguous bytes in any scanned og binary (og likely compares via immediates or identifies LDM by PRIVHEAD magic/MBR 0x42; canonical values taken from winioctl.h)
```

> **A-002 correction (fix session):** the LDM line above is WRONG and was the root
> of the A-001 false positive that became the A-002 regression. The scan searched
> for the session-1 "canonical" (in fact fabricated) needles; the REAL
> `PARTITION_LDM_METADATA_GUID` disk bytes (`aac808588f7ee04285d2e1e90434cfb3`)
> and `PARTITION_LDM_DATA_GUID` disk bytes (`a0609baf3114624fbc683311714a69ad`)
> ARE present, byte-for-byte, in `LdmManager.mo` (DAT_0x1800c4960 — the og
> GPT-dynamic gate's compare operand, FUN_1800724f0) and `Partition.mo`
> (DAT_0x180036630 / DAT_0x180036640, FUN_180002bd0's operands). The A-002 fix
> session's independent full-corpus scan (1,467 files) found the real metadata
> GUID in 8 files and the real data GUID in 7, and the fabricated values in zero.

## Core probe execution (g++ -std=c++20, libcr.a built from audited 67193b6)
```
parse_disk_table rc=0 partitions=4
  part start=2048 sectors=1024 fs=12 from_gpt=1
  part start=3072 sectors=991 fs=12 from_gpt=1
  part start=4096 sectors=4096 fs=10 from_gpt=1
  part start=8192 sectors=1024 fs=12 from_gpt=1
parse_ldm_database rc=1342373889 (0=OK, 0x50030001=ERR_BAD_MAGIC?)
VERDICT: GPT-dynamic-disk LDM discovery FAILS (canonical GUID not recognized)
```

Harness source: gpt-guid-probe-harness.cpp (same directory).

## Fix-session reproduction + correction (diff-to-fix session, A-001 fix)

Reran the og-binary scan over ALL 556 corpus binaries (`og-codebase/**`),
checking BOTH orders for every canonical GUID:

```
ESP   disk-order 28732ac11ff8d211ba4b00a0c93ec93b -> CloneModule.dll, DeviceManager.mo, LdmManager.mo, MainModule.dll, Partition.mo, RecoveryModule.dll, ToolsModule.dll, UILogic.dll
MSR   disk-order 16e3c9e35c0bb84d817df92df00215ae -> CloneModule.dll, LdmManager.mo, MainModule.dll, Partition.mo, RecoveryModule.dll, ToolsModule.dll, UILogic.dll
BASIC disk-order a2a0d0ebe5b9334487c068b6b72699c7 -> CloneModule.dll, MainModule.dll, Partition.mo, RecoveryModule.dll, ToolsModule.dll, UILogic.dll
      text-order (any of the three)              -> ZERO hits in every binary
```

Correction: the original scan section above transcribed the og-binary hits as
GUID-TEXT-order hex (the ESP/MSR lines); the bytes actually present in the og
binaries are the mixed-endian DISK-order forms now listed. The original
executed core probe (below) remains valid as-is — its LDM entries used the
correct disk-order bytes, which is why the pre-fix core failed them.

Pre-fix build rerun (audit baseline code, harness unchanged):
`parse_ldm_database rc=1342373889 (ERR_BAD_MAGIC) — VERDICT: FAILS` —
reproduced. Post-fix: the GUID gate passes; the harness's magic-only fake
PRIVHEAD then fails `parse_privhead` (the harness comment "PRIVHEAD magic
optional" is inaccurate — a valid PRIVHEAD is required by `parse_ldm_database`);
the permanent unit test `Ldm.GptDynamicDiskCanonicalGuidsParse` (real PRIVHEAD,
canonical-GUID GPT) proves the end-to-end fix: `parse_ldm_database == OK`.

> **A-002 CORRECTION (diff-fix session 3, 2026-10-26).** Both blocks above are
> now known to be artifacts of the fabricated 1cb6a43 "canonical" LDM values:
>
> 1. The og-binary scan searched for the **fabricated** needle bytes, so "LDM_META /
>    LDM_DATA not present" describes values that exist in no og binary and no real
>    disk. Re-extraction with the REAL Microsoft constants (session-2 A-002 probes +
>    session-3 first-hand re-run): `LdmManager.mo` DAT_1800c4960 and `Partition.mo`
>    DAT_180036630 both carry `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3` (metadata);
>    `Partition.mo` DAT_180036640 carries `af9b60a0-1431-4f62-bc68-3311714a69ad`
>    (data); 8 further corpus binaries carry the same bytes (9+9 file hits across
>    1,470 scanned files; fabricated values: 0 hits).
> 2. The core-probe VERDICT "LDM discovery FAILS" ran a harness whose GPT entries
>    encoded the same fabricated values (see the A-002 correction note in
>    `gpt-guid-probe-harness.cpp`); the audited `67193b6` core — which carried the
>    REAL constants — correctly rejected the bogus fixture. The fixture was wrong,
>    not the code. The A-001 remediation `1cb6a43` then "fixed" the core to match
>    the bogus fixture, regressing every real GPT dynamic disk (A-002, FIXED this
>    session: constants restored, harness + this log annotated).
