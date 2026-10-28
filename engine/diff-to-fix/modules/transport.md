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
  0x60002400/04/08 mirrored, extended 0x6002440/44/48, session codes 0x800C2AC0+)
  is its own driver's contract — grammar shape + token pattern match the og pattern;
  numbers deliberately differ (own driver). Register **I-016**.

## Findings index

| ID | Severity | One-line |
|---|---|---|
| M-030 | MAJOR | WinPE gate, stale-handle ladder, terminal OFFLINE, chunking provenance, 32-disk cap, silent RDA fallback |
| N-041 | MINOR | Dead shipped code: RdaClient copy-session protocol + volume_letter_to_disk (zero callers) |
| I-016 | INFORMATIONAL (shared) | Driver wire-contract register + FS-code-space warning |

---

## Session-5 audit (audited commit `7cf6754`)

- WinPE artifacts (eusendmessage + boot-config) read for the first time: EuSendMessage is
  installer-only (WM_COPYDATA dwData=999 → "DRW_Installer" window; no engine contract);
  the boot chain confirms M-030's environment-gate evidence (ebrntdrv/epmdkdrv
  demand-load, EUDCPEPM boot-time filter, PENetwork SMB destinations — registered I-021).

---

## Session-23 verdict (M-030 FIXED)

All four open items landed with the finding's three open questions
resolved by first-hand decode (the full map is in
findings/FIXED/M-030's Resolution):

- **P51/P47** — the 64 KiB direct-I/O envelope stays the registered
  deliberate improvement (OQ-S4-3's relative-perf measurement remains
  the tracked-open remainder); no behavior change from this fix.
- **P35** — the stale-handle ladder was the prior leg's landing,
  verified present (read_at + write_at ERROR_INVALID_HANDLE heal-and-
  retry, reopen_handle).
- **P59/P43 (the WinPE gate)** — FIXED: `win::win_pe_environment()`
  (GetSystemDirectoryA first byte 'X'/'x' - FUN_1800577e0 :50943-50992;
  OQ-3's polarity resolved by the four callers' branch patterns) gates
  BOTH `open_disk`'s RDA preference and the ABI's `open_disk_impl`:
  in PE the core opens `\\.\\PhysicalDrive%d` direct like the og's
  callers. The probe is kernel32-only (Win7-inclusive gate intact).
- **P60-adjacent (the quiesce chain)** — FIXED: the terminal
  IOCTL_VOLUME_OFFLINE (0x56C00C, RecoveryModule :60440) after DISMOUNT,
  plus the corpus RESTORE chain on release (OQ-2 decoded: reopen by
  GUID, IOCTL_VOLUME_ONLINE 0x56C008 :34484, SetVolumeMountPointW with
  the saved letter, Sleep(1000)) - closing the letterless-after-repair
  hole. The stale :44686-44720 cite corrected to :60401-60447; the
  mislabeled FSCTL hex comments fixed (DISMOUNT=0x90020,
  UNLOCK=0x9001c).
- **P8 (multi-disk)** — FIXED: the LDM resolver's enumeration raised
  32 -> 200 (`win::ENUMERATE_DISK_BOUND`, eprdtdrv :184 grammar space).
- **P62-adjacent (diagnosability)** — FIXED:
  `cr_ui_last_transport_error()` (ABI-since 2.2, additive) surfaces the
  surviving transport leg's error; the RDA leg's failure is preserved
  on the direct fallback (the silent swallow is gone).

The corpus constants + the probe decision are pinned in
`include/cr/win/cr_winenv.h` (platform-independent: both CI lanes
compile them; `Transport.*` pins in the Linux suite, the real-chain
behavior tests on the Windows lane).


## Session-8 section (2026-10-28, audited `35adcb9`)

The two NEVER-CITED driver docs read in full against the user-mode half
(session-8 E lane): identity IOCTLs, in/out gates, grammar, PID-token
(both-sides-exact), 200-disk cap, failure mapping — MATCH throughout. New:
**N-096** (identity-IOCTL non-issuance — the og client queries
GET_DISK_CONTEXT_MODE per open; core never issues any), **N-095** item 4
(this module's :6 grammar misquote — the og client's disk template is
2-segment `\\.\EPRDTDRV\%d\%d`). DRIVER-REPO-ITEM handoffs recorded in the
worklog for the rda-driver lane: the READ-length semantics nuance (og sectors
vs rda bytes; Information×512 invariant holds) and the disk.sys-comparison
field reading (DriverName vs ServiceKeyName, unreconciled).


## Session-8 lead-lane wave (audited `ff879f9`)
- **N-124**: the fallback-ladder and WinPE-gate rows are SUPERSEDED by D-DRV-1/D-DRV-2
  (cr_transport.cpp:563-565 — both REMOVED); annotate rather than re-audit.
## Session-9 addendum (2026-10-28, SA-9 lane — rda-driver @ `323e75a` v0.2.4, corpus `be0cfe9`)

**The never-audited EPMNtDrv surfaces resolved MATCH (no finding):**
- **v2 identity IOCTLs + async completion engine** — our driver carries
  0x60002400 GetProcName (in==4 u32 PID, out>0x207 image path), 0x60002404
  GetFileObjName (in==4 handle, out>0x208 NT path), 0x60002408
  GetDiskContextMode (out==4, bound handles only), method-aware pass-through
  — the corpus v2 artifact's contract, cross-verified against the ghidra
  driver decompiles and our rda_ioctl.c/rda_ident.c (the SA-9 C/E sheets).
- **Trio naming + PID handshake** — the byte-sum PID-token transform
  re-derived at CREATE (mismatch → STATUS_INVALID_PARAMETER),
  `\Device\<UPPERCASED service-name>` + EPMNTDRV fallback, DO_DIRECT_IO,
  the `%s%d` numbered-variant enabling — byte-exact, benchmark-proven.
- SURFACE-INVENTORY rows flipped to AUDITED (this lane).
- Corpus-internal note: the v1 artifact's §4 mislabels 0x60002400 as "sets
  the global target"; the decompile shows GetProcName (v2 is correct).

**Driver-repo doc-truth (the lane's unique finding):**
- **N-127 (MINOR)** — the falsified admin-gate claim at rda_ioctl.h:367 (no
  privilege check in code — the og-parity vs real-hardening decision), the
  stale 512-byte-sector prose, §2.2 pre-G2 prose, the RDA_DP_COMPONENT S12
  contradiction, the unreachable dispatch default, the undocumented 57th
  check. (The count prose is N-107's; the regdrv/installer items are
  M-086/M-087/M-088's.)
