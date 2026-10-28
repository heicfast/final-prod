# MAJOR M-132 — GPT backup-header probe position + dead-LBA1 asymmetry: the og always probes the backup at device-last and bails entirely when LBA1 is unreadable; the core probes the primary's declared backup_lba and recovers from device-last on dead LBA1

**Severity:** MAJOR
**Module:** table (GPT detection/ladder)
**Parameters affected:** detection correctness (1), recovery completeness on damaged GPT, error recovery (35)
**Corpus references:** `ghidra-mo/Partition_decompiled.c:4567` (`FUN_180006e20` — the analyzer ALWAYS reads the backup at `param_1[1] - 1` = device sector_count−1), `:4517-4524` (the LBA1-read failure → free + early return — no backup probe on that path), `:4166-4234` + `:4208-4218` (`CGPTDiskAnalyzer::IsGPTDisk` — the separate detection layer falls back to the last LBA to CLASSIFY the disk)
**Core references:** `src/core/table/cr_table_parse.cpp:323` (primary OK → probe at `prim.backup_lba`), `:339-348` (LBA1 unreadable or primary dead → probe at dev_last); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two asymmetries in the GPT ladder: (1) the og probes the backup header at the FIXED
device-last LBA regardless of what the primary declares; the core probes the primary's
declared `backup_lba` when the primary verifies — so a healthy primary with a
nonstandard-but-valid backup location (or a corrupt backup_lba field) yields
og: backup found; core: backup validation fails (falls to salvage). (2) When the LBA1
READ ITSELF fails, the og's analyzer returns early and emits NO GPT partitions (only the
detection layer classifies the disk as GPT); the core proceeds to probe the backup at
device-last and emits its entries — a core superset on the classic "primary wiped"
damage class. Neither direction is registered.

## What the reference does

`FUN_180006e20` (Partition_decompiled.c, ghidra-mo): after primary verification or
salvage, the backup read is at `param_1[1] - 1` (:4567) — device sector_count−1, never
the primary's declared field. When the LBA1 read fails: "Failed to read GPT HEADER." →
free + return (:4517-4524) — no backup probe, no entries. The separate detection layer
`IsGPTDisk` (:4166-4234) probes the last LBA (:4208-4218) only to CLASSIFY the disk as
GPT for UI purposes.

## What our core does

`cr_table_parse.cpp`: primary verified → probe backup at `prim.backup_lba` (:323); LBA1
unreadable (`lba1_gpt=false`) or primary dead/salvage-failed → probe backup at dev_last
(:339-348) and emit its entries.

## Gap

Case A (primary OK, backup_lba points somewhere valid-but-nonstandard or is corrupt):
og finds the backup at device-last; core fails the probe at the declared LBA (backup_ok
false, salvage path). Case B (LBA1 unreadable): og emits nothing; core emits the
backup's partitions. Both directions verified in code; neither registered.

## Impact

- Case A: the core under-detects relative to the og on the corrupt-backup_lba class
  (detection accuracy).
- Case B: the core OVER-detects relative to the og on the dead-LBA1 class (a recovery
  stance improvement — but unregistered, so parity benches count divergence the registry
  doesn't explain).
- The asymmetry also affects which copy's entries win when both are readable and
  disagree (the M-123 laxity compounds: a laxly-trusted primary + a missed backup).

## Reproduction / verification

Corpus-derived (lead auditor + lane A first-hand: the fixed-position backup read at
:4567 and the early-return at :4517-4524 read directly; the detection-layer fallback at
:4208-4218). Core-derived (the :323/:339-348 branches read directly). No PARITY-DELTAS
row covers either direction (grep GPT across the registry: geometry-only D-04).

## Fix plan (for the engineering team)

1. Align the probe order with the og while KEEPING the core's dead-LBA1 recovery as a
   registered superset: (a) when the primary verifies, probe the backup at device-last
   FIRST (og parity), falling back to the declared `backup_lba` only if device-last
   fails verification (core's extra reach, registered); (b) the dead-LBA1 →
   backup-at-dev-last recovery stays, registered as a deliberate superset (the og
   analyzer's early return is an og weakness on the exact damage class recovery tools
   exist for).
2. Files: `src/core/table/cr_table_parse.cpp` (the :323 branch).
3. Registration: one PARITY-DELTAS row (two sub-items: probe-position parity + the
   dead-LBA1 superset) — doc edit belongs to the fix lane.
4. Tests: below.
5. Acceptance criteria: Case A fixture (primary OK + backup at nonstandard LBA) →
   backup_ok true via device-first probe; Case B fixture (dead LBA1 + intact backup) →
   partitions emitted (unchanged, now registered); the standard fixture set unchanged.

## Tests to add

- `Gpt.BackupProbedAtDeviceLastWhenPrimaryOk` (nonstandard-but-valid backup location —
  fails today)
- `Gpt.CorruptBackupLbaFieldStillFindsBackup` (the og-parity leg)
- `Gpt.DeadLba1BackupRecoveryRegistered` (pin the superset behavior so it is never
  "fixed away" as a parity bug)

## Related findings

- M-123 (the header-validation laxity feeding the same ladder)
- N-161 (the entry-span underflow downstream)

## Open questions

One (from the lane, does not change the verdict): whether any og CALLER of the analyzer
re-routes a backup-detected disk into a path that reads the backup as primary (the two
callers at P:5896/:32675) — would only strengthen Case B's superset reading.
