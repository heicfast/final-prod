# check-phase — the damage-audit check pass (M-041, session 35)

The og's FileSystemCheck.mo is a chkdsk-grade READ-ONLY audit that runs
before/alongside scanning. Session 35 (DFX35) landed the core's parity
module: `include/cr/check/cr_fscheck.h` + `src/core/check/cr_fscheck.cpp`.

## What landed (decompile-exact, every citation re-verified first-hand)

**FAT pass** (CCheckFAT::Check = FUN_1800015b0, phase order preserved):
| Phase | Behavior | Code |
|---|---|---|
| DBR | structural gates (FS strings, SPC pow2, numFATs 1-4) reject; per-flavor ceilings CLAMP the audited cluster count (spf×341.3333 / spf×0x100 / (bps×spf32)>>2 — the decompile clamps, it does not reject) | 0x100d |
| Table | entry-hole scan (data-region-to-reserved-family gap, FUN_180005fd0) + duplicate-reference scan (two entries naming one target, FUN_180005e90) | 0x1006 |
| Copies | pairwise FAT1/FAT2 compare, first mismatch reported and stopped | 0x1005 |
| Dir audit | `.`/`..` pair validation; invalid first-cluster; size-vs-chain (ceil(size/cluster) != chain clusters); directory cycles (path-local, before the cross-link test) | 0x1007 / 0x1004 |
| Chains | global visited array — two files claiming one cluster | 0x1002 |
| Orphans | post-walk sweep: unvisited, non-free, non-BAD {0xff7/0xfff7/0xffffff7} | 0x1003 |

**NTFS pass** (CCheckNTFSVolume::Check = FUN_1800045a0):
| Check | Behavior | Code |
|---|---|---|
| Size | the DBR total must not exceed the device | 0x1001 |
| $MFT | record-0 resident $FILE_NAME whose VALUE is "$MFT" | 0x1008 |
| $MFTMirr | the same proof on the mirror's record 0 | 0x1009 |
| Version | {1.2, 3.0, 3.1} from $Volume's $VOLUME_INFORMATION | 0x100a |
| $Bitmap | rejects $ATTRIBUTE_LIST / resident $DATA; stream must cover the volume | 0x100b |
| Cross-check | every allocated record's datarun LCNs: bitmap-free-but-allocated ("Bitmap mark it as free!!!") / fresh-map double-claim ("Please use 'Chkdsk' first!!!") | 0x1003 / 0x1002 |
| Internal | unexpected failures | 0x100e |

**Wiring**: a pre-scan stage in cr_scan.cpp after repair, before the
engines; default ON (og parity) with `run_fs_check` opt-out; the report
lands in `ScanStats.fs_check_*` + a log-seam Warn line; NEVER aborts the
scan. Reads through the RAW slice (not the retry pipe) — check reads
must never steal the error map's per-range engine attribution.

**Verdict table**: P1/P3/P15/P25/P35/P42 → MATCH (the audit semantics,
thresholds and error taxonomy are decompile-exact); the 8-event host
callback contract and the WinChkdsk child launcher are recorded out of
core scope (the UI layer's escalation endpoints).

**Proof**: 14 tests — FatCheck.* (9), NtfsCheck.* (4),
Orchestrator.CheckPhaseNeverAbortsScan; fail-before by construction
(the API did not exist). Suite 877/877.
