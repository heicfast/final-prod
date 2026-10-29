# MAJOR M-145 — Core-only 0x55AA signature gate at NTFS engine open: the og's NTFS path never checks the DBR signature (the FAT/exFAT og paths do); a torn-signature NTFS volume scans in the og and is refused by the core

**Severity:** MAJOR
**Module:** ntfs (engine open gates)
**Parameters affected:** B1 (detection correctness), A25 (engine algorithm steps), B3 (false-positive rate — the gate's FP value is absorbed upstream by content proof)
**Corpus references:** `re-workspace/reversed/ghidra-mo/NTFSFileSystemAnalyser_decompiled.c` FUN_180004900 (:3568-3577 — the complete gate list: OEM "NTFS    ", bps & 0x1ff == 0, +0x0E..0x0F == 0, spc(+0x0D) != 0 any value, +0x10..0x14/+0x16/+0x17 == 0, +0x20..0x23 == 0 — no signature term; re-read first-hand); `ghidra-mo/NTFSLib_decompiled.c:19292-19310` (LoadNtfsVolume: OEM → bps mask → mft-product gate; no +0x1FE read)
**Core references:** audited `34d6cf10` — `src/core/ntfs/cr_bootstrap.cpp:385-388` (`boot_signature_ok` → `ERR_BAD_MAGIC` before `validate_boot_sector`) and `:395-396` (the same requirement on the backup fallback); contrast `src/core/orchestrator/cr_scan.cpp:769-811` (probe_family's NTFS branch deliberately does NOT check 55AA — the divergence is observable as "recognized by dispatch, refused by open")
**Status:** READ-ONLY FINDING — no fix applied

## Summary

No NTFS-side og layer reads the DBR signature at +0x1FE — the analyser's gate list and NTFSLib's open ladder both stop at OEM/BPB/mft-product checks (the og's FAT and exFAT paths DO check 0x55AA; this omission is NTFS-specific). The core's `NtfsVolume::open` hard-gates on the signature before anything else. A torn 2-byte signature on an otherwise-valid NTFS DBR (with a torn/absent volume-end backup) is scanned fully by the og and refused by the core.

## What the reference does

The analyser's complete gate conjunction (FUN_180004900 :3568-3577, quoted in the M-140 evidence and re-read first-hand): OEM string, bps 512-multiple, the reserved-zero sweeps, spc != 0 (any value), the +0x20..0x23 zero sweep — no signature term. NTFSLib's LoadNtfsVolume (:19292-19310): `local_1055 == 0x5346544e` (OEM "NTFS") → `(local_104d & 0x1ff) == 0` (bps) → `(local_104b & 0xff) * local_1028 < local_1030` (the mft product gate, M-141's subject) — again no +0x1FE access anywhere in the function (the region re-read in full this session). The og's NTFS acceptance is content-based (the analyser's follow-on record-0 probe with FILE magic + USA bounds), not signature-based.

## What our core does

`cr_bootstrap.cpp:385-388`:

```cpp
bool from_backup = false;
u32 rc = OK;
if (!boot_signature_ok(reinterpret_cast<const u8*>(&bs)))
    rc = ERR_BAD_MAGIC;
```

before `validate_boot_sector` runs, with the identical requirement on the D-99 backup fallback (:395-396). Meanwhile the core's own probe_family NTFS branch (`cr_scan.cpp:769-811`) deliberately omits 55AA (og parity at dispatch) and content-proves via record 0 + USA bounds — so a torn-signature volume is dispatched to the NTFS engine and then refused at open: "recognized but unopenable", the worst observable shape.

## Gap

Falsifiable: an NTFS DBR with bytes +0x1FE/+0x1FF destroyed (a classic partial-overwrite signature) and a torn volume-end backup: the og's analyser passes (OEM/BPB/record-prove all intact) and the engine scans the full volume; the core's dispatch passes and open fails with ERR_BAD_MAGIC — zero structured files, carve only.

## Impact

Damaged-volume detection loss on the torn-signature population; inconsistent with the og's documented salvage-over-purity posture (fixup failure → record skipped, never abort the walk). The core already accepts the principle at dispatch (probe_family has no 55AA) — the open gate contradicts the core's own dispatch layering.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand: the analyser gate list and the LoadNtfsVolume region read in full this session (no +0x1FE access in either); the FAT/exFAT og contrast (FUN_18002f970's `*(char *)(lVar2 + 0x1fe) == 'U' && *(char *)(lVar2 + 0x1ff) == -0x56` at :38056-38057 — the og DOES check there, proving the omission is a deliberate NTFS-side posture, not a decompile gap). Core side: the two gate sites quoted at `34d6cf10`.

## Fix plan (for the engineering team)

1. Drop the 0x55AA requirement from `NtfsVolume::open`'s primary path (keep it on the FAT/exFAT paths where the og has it).
2. For the D-99 backup fallback: accept a torn-signature backup when its content gates pass (the og has no backup-open path at all — the core's fallback is already a superset; keep its signature requirement only if the content gates are the fallback's real proof, which they are).
3. Alternatively, if the gate is kept as hardening, REGISTER it in D-52's kept list with the analyser/LoadNtfsVolume evidence acknowledged and a rationale stronger than "the og checks it" (which is false).
4. Files: `src/core/ntfs/cr_bootstrap.cpp`; `docs/PARITY-DELTAS.md` if registered.
5. Acceptance criteria: torn-signature fixture (no backup) → full tree; probe_family→open consistency (a dispatched family never fails on a gate the dispatch layer already chose not to apply).

## Tests to add

- `NtfsVolume.TornSignatureScansWithoutBackup` — RED today.
- `NtfsVolume.TornSignatureWithIntactBackupStillFallsBack` (the D-99 interplay pinned).
- `Fat.BootSignatureStillRequired` (the og-parity control — FAT keeps the gate).

## Related findings

- M-140 (the probe_family gate-family mismatch — the same layering question from the dispatch side).
- M-141 (the mft_lcn/mftmirr bounds — the third acceptance-band delta in the same open ladder).
- D-99 (the registered backup fallback this finding complements).

## Open questions

None — the corpus decode is complete (the FAT-side contrast proves the omission is intentional og behavior, not missing evidence).

## Resolution

**Status:** FIXED
**Fixed in commit:** `<this push>` (the S59 MAJOR-first fix lane, 2026-10-29)
**Verified by:** fail-before/pass-after on `NtfsVolume.TornPrimaryBackupBootScans`
(RED pre-fix: ERR_BAD_MAGIC on the torn-signature primary; GREEN post: the
primary opens directly, no backup consulted, full tree) and
`NtfsVolume.TornSignatureWithIntactBackupStillFallsBack` (the D-99 interplay:
a content-torn primary + a torn-signature but content-valid backup opens
FROM the backup — the backup's BPB gates are its proof now). The FAT control
(the og-parity 55AA gate there) is pinned by the existing FAT suite
(`Fat.*` signature tests + `Orchestrator.ProbeFamily`'s signature-destroyed
→ Unknown leg). Full suite 1465/1465.
**Resolution notes:** CONFIRMED-DIVERGENCE, corpus re-verified first-hand
(the analyser conjunction FUN_180004900 :3568-3577 and LoadNtfsVolume
:19292-19310 read in full — no +0x1FE access in either; the only 0x1fe
touches in NTFSLib are the USA fixup walkers at :14000/:14071/:14433; the
FAT-side contrast FUN_18002f970 :38056-38057 proves the omission is
NTFS-specific). The fix: the `boot_signature_ok` requirement is removed
from BOTH the primary path and the D-99 backup fallback (the fallback is
core-only either way; its content gates are the real proof). The M-043
backup fallback now triggers on CONTENT failure only — and a new
MFT-load-failure retry rung (landed with M-141) keeps the torn-geometry
class covered. `BackupBadFallsBackToRefusal` re-pinned from a
signature-tear to an OEM-tear (the refusal trigger is content now). The
FAT/exFAT signature gates are untouched (og parity there).
