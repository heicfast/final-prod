# gpt-guid-probe — executed evidence (session, main auditor)

## Og-binary canonical-GUID scan (python, PartitionRecovery/* + DRWUI.exe)
```
ESP  c12a7328f81f11d2ba4b00a0c93ec93b -> UILogic.dll, MainModule.dll, LdmManager.mo, DeviceManager.mo, Partition.mo, RecoveryModule.dll
MSR  16e3c9e35c0bdb4817df92df00215ae -> UILogic.dll, MainModule.dll, LdmManager.mo, Partition.mo, RecoveryModule.dll, ToolsModule.dll
LDM_META / LDM_DATA / BIOSBOOT / LINUX_FS / BASIC: not present as contiguous bytes in any scanned og binary (og likely compares via immediates or identifies LDM by PRIVHEAD magic/MBR 0x42; canonical values taken from winioctl.h)
```

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
