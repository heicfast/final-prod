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
