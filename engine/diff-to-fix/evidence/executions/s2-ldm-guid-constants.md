# s2-ldm-guid-constants — extracted LDM GUID constants from og binaries + core comparison

**Session 2 (T7 deep pass) · audited core `2545b83`→`4f0c841` vs corpus `03ad46c`**

## 1. Og binary constant extraction (python PE section walker)

Script: `s2-ldm-real-guid-proof/probe_ldm_manager_guid.py` (also scans Partition.mo).

Output (verbatim):

```
LdmManager.mo DAT_0x1800c4960 (RVA 0xc4960): aac808588f7ee04285d2e1e90434cfb3
   mixed-endian GUID: 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3
Partition.mo DAT_0x180036630 (RVA 0x36630): aac808588f7ee04285d2e1e90434cfb3
   mixed-endian GUID: 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3
Partition.mo DAT_0x180036640 (RVA 0x36640): a0609baf3114624fbc683311714a69ad
   mixed-endian GUID: af9b60a0-1431-4f62-bc68-3311714a69ad

LdmManager.mo contains canon-meta(=og-meta real): 1 hits
Partition.mo contains og-meta(real): 2 hits; og-data(real): 2 hits
LdmManager.mo contains real LDM data GUID: 0 (it gates on metadata only)
Neither binary contains the post-1cb6a43 core values (aac808589f8e874c... / a0609baf4e387d4f...): 0 hits anywhere in the 1,470-file corpus scan
```

- `LdmManager.mo` DAT_0x1800c4960 is the GPT-entry compare operand in the og's GPT-dynamic
  gate (FUN_1800724f0, `LdmManager_decompiled.c :59445`).
- `Partition.mo` DAT_0x180036630/0x36640 are the compare operands of the GPT-entry scan in
  Partition.mo's is-GPT-dynamic/label check (FUN_180002bd0, `Partition_decompiled.c :1322-1329`).

## 2. Core constant comparison (post-`1cb6a43` tree, executable probe)

Script: `s2-ldm-real-guid-proof/real_guid_probe.cpp`. Output (verbatim):

```
core GPT_TYPE_LDM_METADATA bytes: aac808589f8e874caea2af0aa3c41687
real canonical metadata bytes:    aac808588f7ee04285d2e1e90434cfb3
core metadata == real canonical? NO  <-- REAL GPT DYNAMIC DISKS FAIL
core GPT_TYPE_LDM_DATA bytes:     a0609baf4e387d4f8fa5b1c3d6a9e2b0
real canonical data bytes:        a0609baf3114624fbc683311714a69ad
core data == real canonical? NO  <-- LDM data partitions misclassify
parse_ldm_database with REAL canonical GUIDs: rc=1342373889 (FAILS — real GPT dynamic disks get zero LDM recovery)
```

With the pre-`1cb6a43` constants restored (scratch include copy, no repo change):

```
core GPT_TYPE_LDM_METADATA bytes: aac808588f7ee04285d2e1e90434cfb3
real canonical metadata bytes:    aac808588f7ee04285d2e1e90434cfb3
core metadata == real canonical? YES
core GPT_TYPE_LDM_DATA bytes:     a0609baf3114624fbc683311714a69ad
real canonical data bytes:        a0609baf3114624fbc683311714a69ad
core data == real canonical? YES
```

## 3. External ground truth (Microsoft)

Microsoft Learn, `PARTITION_INFORMATION_GPT`
(learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-partition_information_gpt),
quoted verbatim:

> | **PARTITION_LDM_METADATA_GUID** 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3 | The partition is a
> Logical Disk Manager (LDM) metadata partition on a dynamic disk. This value can be set only
> for dynamic disks. |
> | **PARTITION_LDM_DATA_GUID** af9b60a0-1431-4f62-bc68-3311714a69ad | The partition is an LDM
> data partition on a dynamic disk. This value can be set only for dynamic disks. |

Same values in: ntdoc.m417z.com `PARTITION_LDM_METADATA_GUID`, MagnumDB, phnt
(winsiderss) ntbcd.h, the Linux kernel's EFI partition GUID set (Wikipedia's GPT
partition-type table), and Wikipedia's GPT page. The values the A-001 fix installed
(`5808c8aa-8e9f-4c87-aea2-af0aa3c41687` / `af9b60a0-384e-4f7d-8fa5-b1c3d6a9e2b0`) appear in
**none** of these sources and in **no** og binary.

## 4. Session-1 evidence-chain correction

Session-1's `evidence/executions/gpt-guid-probe.md` scanned for its (fabricated) "canonical"
needles, found nothing in the og binaries, and concluded "LDM_META / LDM_DATA / BIOSBOOT /
LINUX_FS: not present as contiguous bytes in any scanned og binary". The real metadata/data
GUID bytes ARE present in the og binaries (LdmManager.mo ×1, Partition.mo ×2 each); the
session-1 scan missed them because it scanned for the wrong needle values. Session-1's
probe harness (`gpt-guid-probe-harness.cpp :88-91`) wrote the fabricated values into its
fixture; the pre-fix core (which carried the real values) correctly rejected that fixture —
recorded as "VERDICT: GPT-dynamic-disk LDM discovery FAILS", the root of the false positive.
