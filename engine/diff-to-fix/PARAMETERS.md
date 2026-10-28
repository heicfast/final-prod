# PARAMETERS.md — the parity-audit parameter taxonomy

This is the parameter taxonomy every module is scored against. A module may be
`NOT-APPLICABLE` for a parameter (with a one-line reason). Verdicts: `MATCH`,
`DIVERGENCE-ACUTE`, `DIVERGENCE-SEVERE`, `DIVERGENCE-MAJOR`, `DIVERGENCE-MINOR`,
`DIVERGENCE-INFORMATIONAL`, `UNVERIFIED` (see `SEVERITY-SCALE.md`).

Every applicable parameter that is not `MATCH`/`DIVERGENCE-INFORMATIONAL`
generates a finding file under `findings/`.

**Audited core commit:** `67193b6` (baseline includes concurrent fixes through
`9fc82b0`). **Corpus commit:** `03ad46c`. **Session-9 currency note
(2026-10-28):** corpus pin advanced to `be0cfe9` (CORRECTIONS.md #9 — the
EUUserData/GetRawUserInfoMap corrections, already absorbed as CORPUS-ERRATA
E-A/E-B + D-104; the taxonomy below is unchanged by that commit). The
session-9 findings (S-040..S-042, M-071/M-072, N-090..N-092) score against
B1/A28/C19/P29 (carve) and P47/P-inst-15/16 + the doc-truth parameters
(transport/driver-install) — all existing taxonomy rows; no new parameters
were invented.

## Group B — Behavioral parameters

| # | Parameter | Scope notes |
|---|---|---|
| B1 | Detection correctness | Engine detects what the reference detects, and only that (boot/SB magics, offsets, tolerances, fallback probes). |
| B2 | Recovery correctness | Recovered file bytes match reference on the same fixture (hash-level). |
| B3 | False-positive rate | Wrong things reported recoverable (carve validators, inferred chains, remnant grading). |
| B4 | Deleted-file classification | Deleted-state detection predicate and grading tiers match reference (Excellent/Good/Poor boundaries). |
| B5 | Orphan reconstruction behavior | Lost/orphan directory reconstruction ("File Path Lost"/"Reconstructed" branches). |
| B6 | Post-format / remnant scan behavior | Post-quick-format remnant scan ($MFT remnant blocks, FAT remnants). |
| B7 | Hard-link, ADS, sparse handling | Alternate streams, hard links, sparse/inline extents surfaced the same way. |
| B8 | Multi-volume / cross-partition boundaries | Multi-volume disks, LDM spans/RAID, partition-boundary handling. |
| B9 | Time and timestamp handling | Same precision (ns vs s), same epoch conversion, same UTC/local semantics. |
| B10 | Filename, path, Unicode handling | Normalization, fallback (8.3, 0x05→0xE5 KANJI, U+FFFD), UTF-8 decode for byte-name FSes, path building. |

## Group D — Data model parameters

| # | Parameter | Scope notes |
|---|---|---|
| D11 | Node fields | Result-node shape: names, semantics, per-FS payload fields. |
| D12 | Session persistence format | .rsf (reference) vs .crsf (core): field-level equivalence, chunk framing, record grammar, resume semantics. |
| D13 | Report schema | Recovery-report fields, semantics, ordering. |
| D14 | Error-map shape | ReadErrorMap / error reporting contents and semantics. |
| D15 | Confidence grading model | Grade categories and decision boundaries (CRecoverability parity). |
| D16 | Filter model | Category set, extension membership, predicate semantics (FileFilter.xml parity). |
| D17 | Sort model | Sort keys, stability, defaults. |
| D18 | Paging model | Cursor/offset semantics of the query surface. |

## Group C — Constant and format parameters

| # | Parameter | Scope notes |
|---|---|---|
| C19 | Magic bytes and signatures | Every on-disk magic the reference checks. |
| C20 | On-disk struct offsets and sizes | Struct field offsets/sizes vs decompiled reads. |
| C21 | Enum values | Numeric values where interoperability matters (FS codes, type-registry IDs, category IDs). |
| C22 | IOCTL codes and ABI numbers | Driver contract numbers (RDA client surface). |
| C23 | Error codes | Classes and mapping (0x5002xxxx families, og error semantics). |
| C24 | CLI exit codes | Exit-code meaning of the CLI surface. |

## Group A — Algorithm parameters

| # | Parameter | Scope notes |
|---|---|---|
| A25 | Engine algorithm steps | Sequence and decisions of each engine's scan. |
| A26 | Tree walk logic | Traversal, pruning, bounds (MFT walk, B-tree walks, inode sweeps). |
| A27 | Fragment handling | Runlist/extent/chain reconstruction strategy (incl. deleted-chain heuristics). |
| A28 | Carving validator logic | Grammar checks, accept/reject criteria, size inference, padding rules. |
| A29 | Merge and dedup strategy | Grouping and precedence across engines/branches. |
| A30 | Candidate scoring (lost partition) | Features, thresholds, overlap penalties. |

## Group S — State machine parameters

| # | Parameter | Scope notes |
|---|---|---|
| S31 | Scan lifecycle | States and transitions (quick → advanced, stage model). |
| S32 | Cancellation | Responsiveness, partial-result behavior, save-on-cancel. |
| S33 | Pause and resume | Semantics and constraints. |
| S34 | Checkpoint | Frequency and recovery behavior. |
| S35 | Error recovery | Retry, fallback, failure propagation. |
| S36 | Session lifecycle | Save/load/resume semantics. |

## Group O — Orchestration parameters

| # | Parameter | Scope notes |
|---|---|---|
| O37 | Stage model | Stages, order, mergeability. |
| O38 | Progress semantics | Weighting, monotonicity, events (incl. the reference 40→65% formula). |
| O39 | ETA inputs | Inputs and calculation. |
| O40 | Threading model | Concurrency, locking discipline, guarantees. |
| O41 | Callback threading | Thread-calling contract of callbacks. |

## Group P — API parameters

| # | Parameter | Scope notes |
|---|---|---|
| P42 | Public surface shape | Operations and grouping (plugin contract, DLL exports, C ABI). |
| P43 | Ownership and lifetimes | Refcount/arena/callback ownership semantics. |
| P44 | Async model | Cancellation model, backpressure. |
| P45 | ABI stability and versioning | Version policy of cr_ui ABI. |
| P46 | Documentation of the contract | Complete, truthful, verified. |

## Group X — Performance parameters

| # | Parameter | Scope notes |
|---|---|---|
| X47 | Throughput | bytes/s, files/s vs reference on same fixtures/hardware (where measurable). |
| X48 | Latency | Time-to-first-result, per-stage, to-complete. |
| X49 | Memory | Peak and steady-state. |
| X50 | CPU | Utilization and efficiency. |
| X51 | I/O behavior | Read sizes, alignment (64 KB Windows direct-I/O multiple), buffering. |
| X52 | Scalability | Behavior at large N (nodes, files, size). |
| X53 | Cancellation responsiveness | Request → stop time. |

## Group R — Architectural parameters

| # | Parameter | Scope notes |
|---|---|---|
| R54 | Module boundaries | Separation of concerns parity. |
| R55 | Dependency direction | Allowed/forbidden edges. |
| R56 | Interface shapes | Abstractions and boundaries. |
| R57 | Wiring | Constructor/owner/callback direction, lifecycle. |
| R58 | Layering | No bypass of a reference layer. |

## Group F — Safety parameters

| # | Parameter | Scope notes |
|---|---|---|
| F59 | Driver-absent behavior | Refuse or documented fallback (RDA vs direct). |
| F60 | Bad-sector handling | Policy and reporting (retry, zero-fill, map). |
| F61 | BitLocker handling | Legitimate unlock only, no bypass. |
| F62 | Destination overlap refusal | Hard refusal semantics. |
| F63 | RawFixer consent | Gating of repair operations. |
| F64 | SDDL / access control | Same or stronger. |

## Group Q — Correctness and quality parameters

| # | Parameter | Scope notes |
|---|---|---|
| Q65 | Test coverage | Every parameter above has a test. |
| Q66 | Fuzz coverage | Structure-aware harness per parser. |
| Q67 | Sanitizer cleanliness | ASan/UBSan/TSan clean. |
| Q68 | Documentation accuracy | Docs reflect current behavior. |
| Q69 | Stub/mock/TODO scan | None in shipped paths. |
| Q70 | Win7-inclusive posture | Import allowlist and OS-version gates intact. |

## Group K — Packaging & installer parameters (added by the dev-sign session, corpus-derived)

> Derived from `architecture/02-kernel-driver-architecture.md` §5,
> `artifacts/Installer/setupue-infoforsetup--bootstrap-pair.md`, the
> unins000/SetupUE/InfoForSetup string tables,
> `artifacts/per-binary/deepening-batch1--japanese-winchkdsk-setupdrv-legacy.md`
> §3, `artifacts/DRWService-exe--bitlocker-service.md`, the BUILDPE tree
> (regdrv.reg / winpeshl.ini / auto.bat / software.reg / Unattend.xml),
> `artifacts/WinPE/*`, `artifacts/Licensing/licensing-activation-chain.md`,
> and `og-codebase/EuShare/{install,uninstall}.bat`. Full parameter source
> map: `SURFACE-INVENTORY.md` §5-§6.

| # | Parameter | Scope notes |
|---|---|---|
| K71 | Installer executability & error semantics | The documented install path runs; failures surface loudly (per-call SCM error strings in the reference; no warning-and-exit-0). |
| K72 | Service configuration parity | Group, Type/Start/ErrorControl, Parameters registry values, ImagePath form — the installed service matches the documented shape (the reference writes + verifies). |
| K73 | Uninstall lifecycle | Stop → verify stopped → delete → remove file → verify gone; the reference treats uninstall as a definite, verified (and instrumented) lifecycle step. |
| K74 | Silent / unattended surface | /SILENT /VERYSILENT /SUPPRESSMSGBOXES /NORESTART(+Applications) install; ARP `QuietUninstallString`; admin-required uninstaller; no visible console in the customer path. |
| K75 | Signing posture & environment matrix | Release artifact signature state; the dev/test sign mechanism; HVCI/SecureBoot/testsigning constraints; timestamp policy; per-OS validation evidence. |
| K76 | Versioned coexistence & upgrade | `%s%d` numbered driver layouts (eprdtdrv2.sys-style), unins%03d multi-uninstallers — old and new versions coexist deliberately. |
| K77 | Install-time registry & telemetry contract | `SOFTWARE\EASEUS\DRW\Version`-class keys, OS gate (16-entry table), install/uninstall telemetry verbs (/SendInfo /SendInfoEx /UninstallStart /UninstallEnd). |
| K78 | Boot-media deployment | winpeshl launch order; offline-hive service matrix + UpperFilters; auto.bat phases (OEM branch, companion deploy); the WAIK/DISM/IMAPI2 media builder; installer stamping of shipped config. |

## Application notes

- Engine modules (ntfs, fat, exfat, ext4, hfsplus, btrfs, zfs, refs) are scored
  against B/C/A/S/X/F/Q groups; D/O/P/R groups apply to the surfaces they feed.
- `table`+`ldm` are scored as one module row each in the matrix (partition
  parsing vs LDM volume assembly).
- `lost-partition` scores A30/B1/B3 plus the shared groups.
- `carve` scores A28/C19 plus shared groups; the reference is RAWSearchDLL +
  raw_signs.json.
- `session` scores D12/S36/C19/C20 plus shared groups; the reference is the
  .rsf grammar.
- `orchestrator` scores O37-O41/S31-S35/R54-R58 plus shared groups.
- `writer`+`report` score D13/F62/B2 plus shared groups.
- `health`, `rawfixer`, `visualization`(preview/query), `abi`, `cli` score their
  applicable subsets.
- **driver-install / packaging** (new module row) scores the K group
  (K71-K78) — the Phase-1 dev-sign findings (S-036, M-060..M-063, N-084,
  N-085, I-026) are K71-K75 verdicts; the app-installer/WinPE waves cover
  K74/K76-K78.
- **Session note (CORRECTIONS #5)**: the corpus errata corrected the
  RawConfig toggle count (37 → 34). M-024's mechanism is unaffected; the
  count references across the trail are being reconciled by the dev-sign
  session's propagation wave.
