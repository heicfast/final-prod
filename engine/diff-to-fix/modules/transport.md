# modules/transport.md — Windows transport / driver-client layer audit verdicts (NEW in session 4)

Reference: og's user-mode device layer (`ghidra-mo/Device_decompiled.c` +
`ghidra-drivers/eprdtdrv_decompiled.c` + `ghidra-partition/RecoveryModule_decompiled.c`,
evidence: `evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md`):
- open grammar `\\.\EPRDTDRV\<mangled-pid>\<disk>\<part>` / `\\.\EPRDTDRV\<mangled-pid>\Volume\<n>`
  (PID cascading byte-sum token matches the driver's CREATE check);
- **WinPE environment gate** (`FUN_1800577e0`: GetSystemDirectory → X: drive probe →
  direct `\\.\PhysicalDriveN`/`\\.\HarddiskVolumeN` in PE; driver on live Windows);
- read chunking **4 MiB disk / 7 MiB volume** (`0x400000`/`0x700000` ÷ sectorSize),
  sector-granular, **synchronous** ReadFile loop, queue depth 1, NO 64 KiB rule;
- stale-handle ladder (ERROR_INVALID_HANDLE → close/reopen/retry once, :55942-55967);
- geometry triple-query 0x70000/0x74004/0x7405c (sector default 0x200);
- driver wire: IOCTLs 0x60002400/04/08 (GetProcName/GetFileObjName/GetDiskContextMode)
  + pass-through; READ completes Information = Length<<9 (512-byte sector units);
  CREATE grammar bound **disk# ≤ 199**;
- quiesce terminal step `IOCTL_VOLUME_OFFLINE (0x56C00C)` after DISMOUNT
  (RecoveryModule :60440);
- the og's recovery client never drives the copy-session protocol (grammar + sync
  ReadFile only; the EUDCPEPM copy-session family belongs to the backup line).

Core (audited `63a9167`): `src/win/cr_transport.cpp` (966 LOC) +
`include/cr/win/cr_transport.h` + `include/cr/win/cr_rda.h` — grammar-path open
functions (`\\.\{service}\<tok>\<disk>\<part>`, service default **"RDA"** — the core's
own driver; grammar SHAPE matches the og's incl. the PID-token pattern),
`open_disk(disk, prefer_rda, service)` with RDA-first + direct fallback,
`rda_available` probe, `volume_extent`, 64 KiB direct-I/O envelope on the DIRECT path
(GAP-S6-13; M-009 routed the repair read path through it), `RwDevice` repair path
(S-010's quiesce chain), `RdaClient` structured copy-session protocol (dead code),
`volume_letter_to_disk` (dead code). `cr_ui.cpp:1251-1253` hard-codes prefer_rda=true
with silent fallback; `enumerate_disks(32, ...)` bounds enumeration.

## Verdicts

- **P51 (I/O behavior)** — finding **M-030**: chunking strategy divergent (64 KiB
  direct-I/O envelope vs 4/7 MiB buffered-sync — deliberate Windows heuristic, corpus
  grounding claims corrected); read-shape parity otherwise (sector-granular RDA path).
- **P47 (throughput)** — finding **M-030** OQ: relative perf UNMEASURED (tracked-open
  OQ-S4-3).
- **P35 (error recovery)** — finding **M-030**: no stale-handle refresh ladder
  (ERROR_INVALID_HANDLE → reopen once).
- **P59 (driver-absent behavior)** — `M`-with-finding: the fallback ladder exists and
  works (probe → direct), but the **WinPE environment gate** is absent (M-030) and the
  RDA-open error is silently swallowed at the ABI level (M-030 §Gap (f)).
- **P60-adjacent (write-safety chain)** — finding **M-030**: quiesce chain lacks the
  terminal IOCTL_VOLUME_OFFLINE step (+ a stale corpus cite in the core comment).
- **P8-adjacent (multi-volume/scale)** — finding **M-030**: 32-disk enumeration cap vs
  the og's 200-disk grammar space.
- **P46 (doc truthfulness)** — finding **M-030** (stale RecoveryModule cite;
  M-009/GAP-S6-13 provenance rewording) + **N-040** family.
- **P69 (dead code)** — finding **N-041**: `RdaClient` copy-session protocol +
  `volume_letter_to_disk` have zero callers.
- **P62-adjacent (destination-overlap preflight)** — `M`-note: F12 arming uses
  `volume_extent` only (same data the og's preflight uses); `volume_letter_to_disk`
  was the intended richer preflight (N-041).
- **C22 (IOCTL/ABI numbers)** — `M`-equivalent: the core's RDA wire (identity IOCTLs
  0x60002400/04/08 mirrored, extended 0x60002440/44/48, session codes 0x800C2AC0+)
  is its own driver's contract — grammar shape + token pattern match the og pattern;
  numbers deliberately differ (own driver). Register **I-016**.

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-030 | MAJOR | WinPE gate, stale-handle ladder, terminal OFFLINE, chunking provenance, 32-disk cap, silent RDA fallback |
| N-041 | MINOR | Dead shipped code: RdaClient copy-session protocol + volume_letter_to_disk (zero callers) |
| I-016 | INFORMATIONAL (shared) | Driver wire-contract register + FS-code-space warning |
