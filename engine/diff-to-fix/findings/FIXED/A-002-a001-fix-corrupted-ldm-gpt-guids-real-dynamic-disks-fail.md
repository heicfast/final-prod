# ACUTE A-002 — The A-001 fix corrupted both LDM GPT type-GUIDs: real Windows GPT dynamic disks now get zero LDM recovery (fix-induced regression)

**Severity:** ACUTE
**Module:** table, ldm
**Parameters affected:** C19 (magic bytes and signatures), B1 (detection correctness), B8 (multi-volume/cross-partition), Q68 (documentation accuracy — the "canonical" values cited in A-001 and in the header comment are fabricated)
**Corpus references:** `re-workspace/reversed/ghidra-mo/LdmManager_decompiled.c :59393-59472` (FUN_1800724f0 — GPT-LDM gate, GUID compare against DAT_1800c4960 at :59445); `re-workspace/reversed/ghidra-mo/Partition_decompiled.c :1284-1340` (FUN_180002bd0 — GPT-entry LDM scan against DAT_180036640/DAT_180036630 at :1322-1329); og binary data sections: `og-codebase/PartitionRecovery/LdmManager.mo` DAT_0x1800c4960 = `aac808588f7ee04285d2e1e90434cfb3` (extracted, probe script + dump in evidence); `og-codebase/PartitionRecovery/Partition.mo` DAT_0x180036630 = same metadata bytes, DAT_0x180036640 = `a0609baf3114624fbc683311714a69ad` (data). External ground truth: Microsoft Learn `PARTITION_INFORMATION_GPT` (`learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-partition_information_gpt`) — **PARTITION_LDM_METADATA_GUID `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3`, PARTITION_LDM_DATA_GUID `af9b60a0-1431-4f62-bc68-3311714a69ad`** (same values in ntdoc.m417z.com, MagnumDB, phnt ntbcd.h, the Linux kernel's efi.h GUID set, and Wikipedia's GPT type-GUID table).
**Core references:** `include/cr/table/cr_table_wire.h:87-90` (the two corrupted constants, fixed-in `1cb6a43`), consumed at `src/core/table/cr_ldm.cpp:1020` (GPT-LDM gate in `find_privhead`) and `src/core/table/cr_table_parse.cpp:273,282` (entry classification); self-referential regression tests at `tests/unit/test_crc32_gpt.cpp:369-381` and `tests/unit/test_ldm.cpp:519-593` — audited core commits `67193b6` (pre-fix, values correct) and `2545b83`+`1cb6a43` (post-fix, values corrupted)
**Status:** READ-ONLY FINDING — no fix applied. **This is a regression introduced by the A-001 fix itself; the LDM half of A-001 was a false positive** (see "Gap").

## Summary

The A-001 remediation (`1cb6a43`) replaced the two LDM GPT type-GUID constants in `cr_table_wire.h` with values that **do not exist in any authoritative source, in any og binary, or on any real disk**. The pre-fix values were byte-for-byte the real Microsoft `PARTITION_LDM_METADATA_GUID`/`PARTITION_LDM_DATA_GUID` — identical to what the og's own LdmManager.mo and Partition.mo carry at runtime. After the fix, the core's GPT-LDM gate (`find_privhead`) can no longer match the type GUID of a real Windows GPT dynamic disk, so `parse_ldm_database` fails with `ERR_BAD_MAGIC` — **every real GPT dynamic disk gets zero dynamic-volume recovery**, the exact failure mode A-001 was filed to fix.

## What the reference does

The og's LdmManager decides "this GPT disk is dynamic" by scanning GPT partition entries for a type-GUID match against a 16-byte constant it stores at `DAT_1800c4960` (`LdmManager_decompiled.c :59443-59452`, function FUN_1800724f0, quoted in `evidence/decompiled-quotes/s2-ldm-gpt-gate.md`):

```c
for (local_44 = 0; local_44 < *(uint *)(plVar1 + 10); local_44 = local_44 + 1) {
  iVar2 = memcmp(local_30,&DAT_1800c4960,0x10);
  if (iVar2 == 0) {
    *(undefined4 *)(param_1 + 0x100) = 1;
    *(undefined8 *)(param_1 + 0x68) = *(undefined8 *)((longlong)local_30 + 0x28);
    break;
  }
  local_30 = (void *)((longlong)local_30 + 0x80);
}
```

On a match it sets GPT mode and reads the PRIVHEAD candidate at the partition's **last LBA** (entry+0x28 — the documented backup-PRIVHEAD position at the metadata-partition tail; the core's `find_privhead` does the same, `cr_ldm.cpp:1020-1024`). The constant's actual bytes, extracted from the binary (`evidence/executions/s2-ldm-guid-constants.md`):

| Constant | Bytes (mixed-endian disk form) | GUID |
|---|---|---|
| `LdmManager.mo` DAT_1800c4960 | `aac808588f7ee04285d2e1e90434cfb3` | `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3` |
| `Partition.mo` DAT_180036630 | `aac808588f7ee04285d2e1e90434cfb3` | `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3` |
| `Partition.mo` DAT_180036640 | `a0609baf3114624fbc683311714a69ad` | `af9b60a0-1431-4f62-bc68-3311714a69ad` |

These are exactly Microsoft's documented `PARTITION_LDM_METADATA_GUID` / `PARTITION_LDM_DATA_GUID` (Microsoft Learn, `PARTITION_INFORMATION_GPT` page: "**PARTITION_LDM_METADATA_GUID** 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3 … The partition is a Logical Disk Manager (LDM) metadata partition on a dynamic disk", "**PARTITION_LDM_DATA_GUID** af9b60a0-1431-4f62-bc68-3311714a69ad"), and exactly what the Linux kernel's EFI partition code matches for LDM on GPT, and what real Windows-written GPT dynamic disks carry in their partition entries. A whole-corpus binary scan finds these byte sequences in the og binaries and finds the values the A-001 fix installed **nowhere at all**.

## What our core does

Pre-`1cb6a43` the core carried (decoded here as mixed-endian GUIDs, as a GPT entry stores them):

```c
GPT_TYPE_LDM_METADATA[16] = {0xAA,0xC8,0x08,0x58,0x8F,0x7E,0xE0,0x42,0x85,0xD2,0xE1,0xE9,0x04,0x34,0xCF,0xB3};  // = 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3  (correct)
GPT_TYPE_LDM_DATA[16]      = {0xA0,0x60,0x9B,0xAF,0x31,0x14,0x62,0x4F,0xBC,0x68,0x33,0x11,0x71,0x4A,0x69,0xAD};  // = af9b60a0-1431-4f62-bc68-3311714a69ad  (correct)
```

The A-001 fix (`1cb6a43`) replaced them with `{0xAA,0xC8,0x08,0x58,0x9F,0x8E,0x87,0x4C,0xAE,0xA2,0xAF,0x0A,0xA3,0xC4,0x16,0x87}` (= `5808c8aa-8e9f-4c87-aea2-af0aa3c41687`) and `{0xA0,0x60,0x9B,0xAF,0x4E,0x38,0x7D,0x4F,0x8F,0xA5,0xB1,0xC3,0xD6,0xA9,0xE2,0xB0}` (= `af9b60a0-384e-4f7d-8fa5-b1c3d6a9e2b0`). Neither value appears in winioctl.h, Microsoft Learn, the Linux kernel, any og binary, or any real disk image — they trace back to session 1's A-001 "canonical table", whose LDM rows were fabricated. The header comment at `cr_table_wire.h:66-67` and the A-001 finding's corrected table both present these fabricated values as `PARTITION_LDM_*_GUID` — false provenance.

## Gap

Falsifiable claim: with the constants as they exist at the audited HEAD, a GPT disk image whose LDM metadata partition entry carries the real `PARTITION_LDM_METADATA_GUID` bytes fails `parse_ldm_database` with `ERR_BAD_MAGIC`; with the pre-`1cb6a43` constants it parses end-to-end. **Proven by execution** (below). The og accepts such disks; the core rejects them. Ergo, on every real Windows GPT dynamic disk, the core produces zero dynamic-volume recovery while the og recovers the volume graph.

## Impact

- **Recovery correctness:** every real GPT dynamic disk (the product's primary dynamic-disk scenario; Windows writes these GUIDs via dmio) is refused at the LDM gate — no spanned/striped/RAID-5 dynamic volume can be opened, listed, or recovered. MBR (type 0x42) dynamic disks are unaffected (no GUID involved).
- **Silent:** no error surfaces to the user beyond the partition not being recognized as dynamic; the disk simply looks non-dynamic.
- **Quality process:** two regression tests (`Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes`, `Ldm.GptDynamicDiskCanonicalGuidsParse`) pin the fabricated bytes by writing the constants themselves into the fixtures — self-referential, they would pass for any constant value and validate nothing about reality.
- **Audit integrity:** the session-1 evidence chain (`evidence/executions/gpt-guid-probe.md` + harness) encoded the fabricated "canonical" bytes into its fixture; the pre-fix core (correctly) rejected that fixture, and the run was misrecorded as "VERDICT: GPT-dynamic-disk LDM discovery FAILS". The false positive and the "fix" then compounded each other.

## Reproduction / verification

Executed this session (scripts preserved in `diff-to-fix/evidence/executions/s2-ldm-real-guid-proof/`):

1. **Constant comparison:** probe binaries print `GPT_TYPE_LDM_METADATA` bytes `aac808589f8e874caea2af0aa3c41687` ≠ real `aac808588f7ee04285d2e1e90434cfb3`; data likewise (`evidence/executions/s2-ldm-guid-constants.md`).
2. **Executed test, current core:** `Ldm.GptDynamicDiskRealCanonicalGuidsParse` — byte-identical copy of the suite's `GptDynamicDiskCanonicalGuidsParse` fixture with ONLY the two entry GUIDs replaced by the real Microsoft bytes → `FAILED`, `parse_ldm_database rc=1342373889 (ERR_BAD_MAGIC)`.
3. **Executed test, pre-fix constants:** the same test compiled with the pre-`1cb6a43` constant values restored (scratch include dir, no repo modification) → `PASSED` (2 volume layouts resolved, `is_gpt=true`).
4. **og extraction:** python PE-section walker dumped `LdmManager.mo` DAT_1800c4960 and `Partition.mo` DAT_180036630/0x36640 — all three equal the real Microsoft values (probe script + output in evidence).
5. **External cross-check:** Microsoft Learn `PARTITION_INFORMATION_GPT` lists the same two GUIDs (quoted in evidence); the Linux kernel efi.h GUID set and Wikipedia's GPT table agree; a 1,470-file corpus scan finds the fabricated values in **zero** files and the real values in LdmManager.mo (×1) and Partition.mo (×2 metadata, ×2 data).

## Fix plan (for the engineering team)

1. **Restore the correct constants** in `include/cr/table/cr_table_wire.h:87-90` to the pre-`1cb6a43` values (the real Microsoft GUIDs):
   - `GPT_TYPE_LDM_METADATA = {0xAA,0xC8,0x08,0x58,0x8F,0x7E,0xE0,0x42,0x85,0xD2,0xE1,0xE9,0x04,0x34,0xCF,0xB3}` (= `5808c8aa-7e8f-42e0-85d2-e1e90434cfb3`)
   - `GPT_TYPE_LDM_DATA = {0xA0,0x60,0x9B,0xAF,0x31,0x14,0x62,0x4F,0xBC,0x68,0x33,0x11,0x71,0x4A,0x69,0xAD}` (= `af9b60a0-1431-4f62-bc68-3311714a69ad`)
2. **Fix the provenance comment** (`cr_table_wire.h:64-67`): cite Microsoft Learn `PARTITION_INFORMATION_GPT` (the true winioctl.h values) and the og binary offsets (LdmManager DAT_1800c4960, Partition DAT_180036630/0x36640), not the fabricated table. Note the Linux-kernel efi.h agreement as third source.
3. **Fix the regression tests to be non-self-referential:** in `test_crc32_gpt.cpp:369`, `expect_guid` must assert the real byte sequences (hard-code the Microsoft bytes in the test, not a copy of the constant); in `test_ldm.cpp:519`, the fixture must write the real Microsoft bytes into the GPT entries (hard-coded literals), so a future corruption of the constant fails the test instead of passing it.
4. **Correct the A-001 audit trail:** A-001's LDM rows (both the original and the "corrected" table), the Resolution text, and `evidence/executions/gpt-guid-probe.md` + its harness must be annotated: the LDM claims were a false positive (mixed-endian mis-decode), and `1cb6a43` regressed them. The harness's fabricated GUIDs should be replaced by the real bytes.
5. **Add a comment-level test guard:** a static test that decodes the mixed-endian constant and compares the GUID *text* against the string `"5808c8aa-7e8f-42e0-85d2-e1e90434cfb3"` — text comparison catches transposed-hex errors that byte-array diffs do not.
6. Acceptance criteria: `Ldm.GptDynamicDiskRealCanonicalGuidsParse` (add it, using this session's evidence fixture) passes with the constants restored; the full suite stays green; a fixture using the fabricated `8e9f-…`/`384e-…` values is *rejected* by `parse_ldm_database` (add as a negative test, documenting that those values correspond to nothing real).

## Tests to add

- `Ldm.GptDynamicDiskRealCanonicalGuidsParse` — the suite's fixture with real Microsoft entry GUIDs (fails today, passes after the constant restore).
- `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText` — decode the constants to GUID text and compare against `"5808c8aa-7e8f-42e0-85d2-e1e90434cfb3"` / `"af9b60a0-1431-4f62-bc68-3311714a69ad"` as string literals (immune to future byte-transposition edits).
- `Ldm.GptDynamicDiskFabricatedGuidsRejected` — negative test proving the `8e9f`/`384e` values parse nothing (guards against reintroducing the fabricated constants).

## Related findings

- **A-001** — reopened for its LDM half: false positive; the fix introduced this regression. A-002 supersedes A-001's LDM claims; A-001's ESP/MSR/BIOS-boot/Linux rows were true and stand.
- **I-008** — the same-session LdmManager deep pass that resolved OQ-T7 (RAID-5 rotation) and the GPT-LDM gating question (og gates on the same real metadata GUID, reads PRIVHEAD at the entry's last LBA, no CRC checks — all matching the core's non-GUID behavior).
- Session-1 `evidence/executions/gpt-guid-probe.md` — its "LDM_META / LDM_DATA … not present as contiguous bytes in any scanned og binary" conclusion was an artifact of scanning for the fabricated needles.

## Open questions

None on the GUID values themselves (four independent sources + execution agree). Residual, tracked in I-008/OQ-T7 notes: the og's GPT-LDM gate additionally checks header revision==0x10000 and reserved==0 where the core does not (core slightly more permissive — informational); the og's Partition.mo labels ("LDM Metadata Partition"/"LDM Data Partition") vs the core's Reserved/Ldm classification is a presentation nuance recorded in `modules/ldm.md`.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit; see `git log` for the A-002 fix on main)
**Verified by:** fail-before/pass-after executed proof — `Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes` (FAILED at fabricated constants, PASSED after restore), `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText` (new; FAILED before, PASSED after), `Ldm.GptDynamicDiskCanonicalGuidsParse` (de-self-referenced to hard-coded real Microsoft bytes; FAILED before — `parse_ldm_database` rc=ERR_BAD_MAGIC on the real-GUID fixture — PASSED after with 2 layouts, is_gpt=true, 2 privheads), `Ldm.GptDynamicDiskFabricatedGuidsRejected` (new negative guard; FAILED before — the fabricated-GUID disk parsed end-to-end — PASSED after with ERR_BAD_MAGIC), `Ldm.GptDynamicDiskTableClassifiesLdmEntries` (de-self-referenced; FAILED before — LDM entries classified Other — PASSED after). Full suite 303/303 Linux ASan green at the fix commit (301 baseline + 2 net-new tests).
**Resolution notes:** Restored `GPT_TYPE_LDM_METADATA`/`GPT_TYPE_LDM_DATA` in `include/cr/table/cr_table_wire.h` to the pre-`1cb6a43` bytes — the real Microsoft `PARTITION_LDM_METADATA_GUID` `{5808c8aa-7e8f-42e0-85d2-e1e90434cfb3}` and `PARTITION_LDM_DATA_GUID` `{af9b60a0-1431-4f62-bc68-3311714a69ad}` — and rewrote the header's provenance comment to the true two-source chain (winioctl.h/Microsoft Learn + first-hand PE extraction: LdmManager.mo DAT_1800c4960, Partition.mo DAT_180036630/0x36640, the og gate memcmp target in FUN_1800724f0 at `LdmManager_decompiled.c:59443-59452`; Linux kernel efi.h agreement). Session-3 first-hand re-verification (independent of the audit's evidence): own PE-section extraction script reproduced all three DAT bytes; whole-corpus 1,470-file scan found the real bytes in 8 og binaries (9+9 occurrences) and the fabricated `8e9f`/`384e` values in zero files; the corpus gate decompile re-read first-hand (header checks sig/rev 0x10000/size 0x5C/reserved 0; per-entry memcmp vs the real metadata GUID; PRIVHEAD candidate at entry+0x28 last LBA — matching `find_privhead`'s behavior once the constant is right). De-self-referenced the regression tests: both GPT-dynamic-disk fixtures now write hard-coded REAL Microsoft GUID literals (shared `make_gpt_dynamic_disk` builder in test_ldm.cpp) instead of copying the constants under test; `CanonicalTypeGuidConstantsMatchRealDiskBytes` now carries the real winioctl.h text forms; added the bytes→text guard `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText` and the negative guard `GptDynamicDiskFabricatedGuidsRejected` (the 1cb6a43 values parse nothing real — a future re-corruption fails both). Corrected the audit trail in the same commit: `gpt-guid-probe-harness.cpp`'s two LDM rows rewritten to the real bytes with a correction note, `gpt-guid-probe.md` annotated with the mis-recorded-verdict correction, `docs/parity/PARITY-CORE-REC.md` "GPT — LDM GUIDs" row rewritten to the A-002 history, A-001's session-2 addendum already carries the false-positive correction (verified in place, unchanged). No safety hardening was near this fix; no other behavior touched — both consumption sites (`find_privhead`, GPT entry classification) are byte-identical, only the constant values changed. Win7-inclusive import posture unaffected (constant data, no new imports).
