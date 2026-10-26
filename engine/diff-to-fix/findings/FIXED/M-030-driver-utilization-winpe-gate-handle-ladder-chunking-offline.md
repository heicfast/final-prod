# MAJOR M-030 — Driver/transport utilization pattern divergences: no WinPE environment gate, no stale-handle ladder, no terminal OFFLINE step, different read-chunking strategy (64 KiB provenance correction), 32-disk enumeration cap, silent RDA-open fallback

**Severity:** MAJOR
**Module:** transport / win-layer (new module row — modules/transport.md)
**Parameters affected:** P51 (I/O behavior — read sizes/alignment/chunking), P47 (throughput — strategy-level), P35 (error recovery — handle-refresh ladder), P59 (driver-absent behavior — decision point), P62-adjacent (destination-overlap preflight uses volume_extent only), P8-adjacent (multi-volume: 32-disk cap), P46 (doc truthfulness — stale corpus cite in a core comment)
**Corpus references:** `re-workspace/reversed/ghidra-mo/Device_decompiled.c` — WinPE probe `FUN_1800577e0` (:50943-50992: GetSystemDirectory → X: drive → `\\.\X:` + IOCTL 0x560000; live Windows → EPRDTDRV path, WinPE → direct), driver grammar `"\\\\.\\EPRDTDRV\\%d\\%d"` / `"\\\\.\\EPRDTDRV\\%d\\Volume\\%d"` + PhysicalDrive/HarddiskVolume fallbacks (:50628-50659, :329-358), stale-handle ladder ERROR_INVALID_HANDLE → close/reopen/retry (:55942-55967), **read chunking 4 MiB disk / 7 MiB volume** (`0x400000/sectorSize` :56545; `0x700000/sectorSize` :56915-56916), synchronous ReadFile loop queue-depth-1 (:57010-57028), geometry triple-query 0x70000/0x74004/0x7405c (:56500-56558); `re-workspace/reversed/ghidra-drivers/eprdtdrv_decompiled.c` — **disk# ≤ 199** CREATE grammar bound (:184); `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:60440` — quiesce chain terminal `IOCTL_VOLUME_OFFLINE (0x56C00C)` after DISMOUNT; `re-workspace/reversed/ghidra-drivers/REPORT.md`
**Core references:** `src/win/cr_transport.cpp` — `open_disk(disk, prefer_rda, service)` (:718-728: RDA-first when `prefer_rda`, direct fallback; comment "reference duality - Device.mo"), grammar path `\\.\{service}\<tok>\<disk>\<part>` default service **"RDA"** (:455-465, :617-664), **no ERROR_INVALID_HANDLE refresh ladder** (rg → 0 hits), `RwDevice::lock_volume` stops at FSCTL_DISMOUNT + handle hold (S-010's fix; **no terminal IOCTL_VOLUME_OFFLINE**), `volume_letter_to_disk` dead (:919-962), `probe_geometry` = GET_LENGTH_INFO + extents (:63-110, no 0x70000/0x74004 triple-query); `src/ui/cr_ui.cpp:1251-1253` (`open_disk(pd.number, true, L"RDA", &oerr)` then silent direct fallback — `oerr` from the RDA leg is overwritten; ABI reports transport name only), `src/win/cr_enumerate.cpp` (`enumerate_disks(32, ...)` — 32-disk cap, called at cr_ui.cpp:1246); `src/win/cr_transport.cpp:135-190` 64 KiB direct-I/O envelope (GAP-S6-13/M-009); M-009's Resolution and GAP-S6-13's claim of corpus grounding for 64 KiB; `RwDevice` repair path comment citing "RecoveryModule_decompiled.c:44686-44720" (actual offlining site is :60380-60450 — stale cite); audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's user-mode device layer uses the kernel driver on live Windows and direct handles in WinPE (an environment probe decides), refreshes stale handles on ERROR_INVALID_HANDLE, reads in 4 MiB (disk) / 7 MiB (volume) sequential synchronous chunks with queue depth 1, accepts disks up to 199, and terminates volume quiesce with IOCTL_VOLUME_OFFLINE. The core always tries the RDA grammar first (service default "RDA"), has no environment gate, no handle-refresh ladder, no terminal OFFLINE step, chunks direct reads through a 64 KiB envelope (a Windows perf heuristic whose "corpus grounding" claims are wrong at the device layer — the og has no 64 KiB rule), enumerates at most 32 disks, and silently swallows the RDA-open error when falling back. Each delta is bounded but the pattern diverges on six axes at once.

## What the reference does

(Verbatim quotes in `evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md`.)

1. **WinPE gate**: `FUN_1800577e0` probes `GetSystemDirectoryA`; on drive `X:` it opens
   `\\.\X:` + IOCTL 0x560000 and the caller picks the DIRECT path; on live Windows the
   EPRDTDRV grammar path is used. The driver is preferred only where it is expected to
   be installed.
2. **Stale-handle ladder** (Device.mo :55942-55967): SetFilePointer failure with
   `GetLastError() == ERROR_INVALID_HANDLE (6)` → `CloseHandle` → re-`CreateFile` →
   retry once. Long scans survive handle invalidation (device removal/re-enumeration,
   standby, driver reload).
3. **Read chunking**: disk devices batch at `0x400000/sectorSize` (4 MiB), volume
   devices at `0x700000/sectorSize` (7 MiB); the loop is a **synchronous** ReadFile
   (`LPOVERLAPPED 0x0`) → queue depth 1; no OVERLAPPED/IOCP anywhere in Device.mo; the
   only 0x10000-ish constants are the >4 GiB total-read reject and unrelated fields.
   **There is no 64 KiB alignment/multiple rule at the og's device layer.**
4. **Disk-number bound**: the driver's CREATE grammar accepts disk# ≤ 199 (`:184`) —
   the product's enumeration space is 200 disks.
5. **Quiesce terminal step**: the repair/write chain ends with
   `IOCTL_VOLUME_OFFLINE (0x56C00C)` after DISMOUNT (RecoveryModule :60440) — the
   volume is offlined, not just locked.
6. **Geometry probing**: DISK_GET_DRIVE_GEOMETRY (0x70000) + GET_PARTITION_INFO_EX
   (0x74004) + GET_LENGTH_INFO (0x7405c), sector size defaulting to 0x200.

## What our core does

1. **No environment gate**: `open_disk()` unconditionally prefers RDA when
   `prefer_rda=true` (hard-coded at `cr_ui.cpp:1251`); zero WinPE/X:/GetSystemDirectory
   concepts in the win layer (rg → 0 hits). In a PE-like environment running the RDA
   service the core still goes driver-first where the og deliberately goes direct.
2. **No stale-handle ladder**: `rg ERROR_INVALID_HANDLE src/win/` → no hits; a stale
   handle surfaces as a read error into the retry pipe (sector bisection) rather than
   a reopen — the retry pipe will bisect a *healthy* device whose handle simply died.
3. **Chunking**: direct path reads through the **64 KiB direct-I/O envelope**
   (`cr_transport.cpp:135-190`, GAP-S6-13; M-009 routed the repair path through it).
   The envelope is a legitimate Windows direct-I/O strategy, but M-009's Resolution and
   the GAP's provenance language imply corpus grounding that **does not exist at the
   device layer** — the og chunks at 4/7 MiB buffered-synchronous. This is a
   deliberate-difference question (direct-I/O + 64 KiB is arguably better engineering)
   that the audit record must state honestly: WINDOWS-HEURISTIC, NOT OG-BEHAVIOR.
4. **32-disk enumeration cap**: `enumerate_disks(32, list)` (cr_ui.cpp:1246,
   rrecov.cpp:602-634 cross-disk LDM resolver) vs the og's 200-disk space — hosts with
   >32 disks (SAN/RAID boxes) lose disks 33+.
5. **No terminal OFFLINE**: `RwDevice::lock_volume` (S-010's verified fix) stops at
   FSCTL_DISMOUNT + handle hold; the og additionally offlines. A mounted-but-locked
   volume can still receive other components' I/O through pre-existing handles; the
   og's terminal step closes that window. (Also: the core's comment cites
   "RecoveryModule_decompiled.c:44686-44720" — the actual site is :60380-60450; stale
   cite.)
6. **Silent RDA fallback**: `cr_ui_open_disk` tries RDA, then direct, overwriting the
   RDA leg's error (`cr_ui.cpp:1251-1253`); the ABI surfaces the transport *name*,
   never the failure reason (`rda_available`'s `last_error` exists but is unused).
   Only the CLI prints `[transport]` evidence lines.
7. **Service name**: grammar default is `"RDA"` (our own driver) vs the og's
   `EPRDTDRV` — expected (different driver), but any cross-reference table must not
   equate the grammar constants beyond the SHAPE (`\<token>\<disk>\<part>` /
   `\<token>\Volume\<n>` — the shapes match, including the mangled-PID token pattern).

## Gap

Falsifiable claims:
(a) No code path in the core decides driver-vs-direct by environment — the og's WinPE
probe has no counterpart (rg-verified).
(b) No ERROR_INVALID_HANDLE close/reopen/retry exists — rg-verified.
(c) The core's direct-read chunking constant is 64 KiB; the og's is 4 MiB (disk) /
7 MiB (volume) — and no corpus artifact grounds a 64 KiB device-layer rule.
(d) The core enumerates ≤32 disks; the og's driver grammar accepts ≤199.
(e) The core's quiesce chain has no IOCTL_VOLUME_OFFLINE terminal step.
(f) `cr_ui_open_disk` drops the RDA-open error code on fallback (ABI cannot report it).

## Impact

- **WinPE/boot-media deployments** (the og ships a dedicated WinPE boot environment —
  corpus architecture/03): the core's driver-first posture can add failed-open latency
  or unexpected behavior where the reference goes direct by design.
- **Long scans on churny hardware**: without the handle-refresh ladder, a device
  re-enumeration mid-scan degrades into retry-bisection (slow, and it will report
  false bad-sector ranges for healthy sectors on the reopened-path failure mode).
- **>32-disk hosts**: disks beyond 32 are invisible to the core (LDM cross-disk
  resolution included — the og resolves up to 200).
- **Write-safety posture**: mounted-but-locked vs offlined — a real (if narrow) window
  where third-party writes can interleave with repair writes.
- **Diagnosability**: hosts cannot distinguish "driver absent" from "driver open
  failed with X" through the ABI.
- **Provenance honesty**: M-009/GAP-S6-13's "corpus-grounded 64 KiB envelope" narrative
  must be corrected — future engineers will otherwise "verify" a nonexistent og
  constant (this finding's errata feeds CORPUS-ERRATA.md).

## Reproduction / verification

- **Decompiled evidence (method 2)**: all six og-side behaviors quoted with file:line
  in `evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` (§1-§4) —
  WinPE probe, grammar + fallbacks, 4/7 MiB windows + sync loop + queue depth 1,
  disk#≤199, stale-handle ladder, OFFLINE terminal step, geometry triple-query.
- **Core-side verification (executed greps + reads, this session)**:
  `rg -i "winpe|X:\\\\|GetSystemDirectory" src/win/ include/cr/win/` → 0 hits;
  `rg ERROR_INVALID_HANDLE src/win/` → 0 hits; `cr_ui.cpp:1251-1253` read (fallback
  overwrite); `enumerate_disks(32, ...)` at cr_ui.cpp:1246; `cr_transport.cpp:718-728`
  open_disk read; 64 KiB envelope at `cr_transport.cpp:135-190`; stale cite comment
  located.
- **Og-execution comparison**: NOT performed (driver client requires Windows + the og
  driver; this environment is Linux). The chunking/I-O behavior claims are decompile-
  derived; the core side is code-verified. Marked UNVERIFIED-BY-EXECUTION for the
  relative-performance question (see Open questions).

## Fix plan (for the engineering team)

1. **Architecture**: keep the 64 KiB direct-I/O envelope as a deliberate improvement,
   but (a) add the environment gate (WinPE probe: GetSystemDirectory drive letter —
   direct-first in PE), (b) add the stale-handle ladder in the transport read path
   (ERROR_INVALID_HANDLE → reopen once → retry), (c) add the terminal
   IOCTL_VOLUME_OFFLINE to `RwDevice::lock_volume`'s repair chain (with an undo step
   on release — the og restores on next mount; verify the restore path), (d) raise the
   enumeration bound to 200 (or make it a parameter with default 200), (e) surface the
   RDA-open error through the ABI (`cr_ui_last_transport_error()` or a diagnostics
   field), (f) fix the stale RecoveryModule cite, (g) reword M-009/GAP-S6-13
   provenance: "Windows direct-I/O heuristic, deliberately divergent from the og's
   4/7 MiB buffered-sync chunking" + file the CORPUS-ERRATA row.
2. **Files to modify**: `src/win/cr_transport.cpp` (ladder + OFFLINE + cite fix +
   chunk-size comment), `src/win/cr_enumerate.cpp` + `include/cr/win/cr_enumerate.h`
   (bound 32 → 200 or parameter), `src/ui/cr_ui.cpp` + `include/cr/cr_ui.h`
   (environment gate at open_disk call site; transport-error surfacing),
   `docs/ABI-CONTRACT.md` (transport diagnostics section), `docs/CORE-GAP-LIST.md`
   (GAP-S6-13 provenance rewording).
3. **Constants**: disk bound 200 (og grammar ≤199 + 1); ERROR_INVALID_HANDLE = 6;
   IOCTL_VOLUME_OFFLINE = 0x56C00C; WinPE probe drive `X:`.
4. **Tests to add**: `Transport.StaleHandleReopensOnce` (mock device returning
   ERROR_INVALID_HANDLE after N reads → assert reopen + successful retry, no
   bad-sector ranges added), `Transport.WinPEDirectFirst` (probe fake → direct chosen,
   driver never opened), `Transport.OfflineTerminalStepInRepairChain` (sequence assert:
   DISMOUNT → OFFLINE → … → release restores), `Enumerate.Bound200` (mock 40 disks →
   all visible), `Abi.TransportErrorSurfaced` (RDA open fails with code X → ABI
   reports X).
5. **Contract/doc updates**: ABI-CONTRACT transport section (diagnostics + gate
   semantics); CORE-GAP-LIST provenance rewording; CAPABILITY-MATRIX transport row.
6. **Acceptance criteria**: the six falsifiable claims above each flip to MATCH or
   documented-deliberate; 306/306 + new tests green; the bench harness
   (bench/comparative-bench.yml) gains a `driver-absent + PE-gate` scenario.

## Tests to add

`Transport.StaleHandleReopensOnce`, `Transport.WinPEDirectFirst`,
`Transport.OfflineTerminalStepInRepairChain`, `Enumerate.Bound200`,
`Abi.TransportErrorSurfaced`.

## Related findings

- **M-009 / GAP-S6-13 (FIXED)** — the 64 KiB envelope routing is correct engineering;
  this finding corrects its *provenance narrative* only (no behavior change requested).
- **S-010 (FIXED)** — the quiesce fix's chain gains the terminal OFFLINE step here.
- **N-032 (open)** — health transports (CSMI/USB-bridge) are the health-face of the
  same device layer; fix together where the file overlaps.
- **S-014 (open)** — BitLocker offline-hive support is the other "environment-aware
  behavior" gap in the win layer.
- **I-016** — the factory-contract register records the driver's wire contract
  (IOCTLs 0x60002400/04/08 + passthrough) that the core's RDA path mirrors with its
  own codes.

## Open questions

1. **Relative performance of 64 KiB-direct vs 4/7 MiB-buffered-sync** on real Windows
   hardware — needs the Windows bench leg (bench/ours + a 4 MiB-chunk build variant;
   read-only mandate prevents this session from building the variant). The og side is
   UNVERIFIED-BY-EXECUTION for throughput numbers; tracked in
   `tracked-open/OQ-S4-3-chunking-relative-perf.md`.
2. Whether the og restores volume ONLINE state on release (the decompile shows the
   OFFLINE terminal step; the restore path was not decoded this session) — needed for
   the undo semantics of fix 1c.
3. `FUN_1800577e0`'s return polarity is decompiler-unrecovered (typed void) — the
   caller branch direction was read from context; a 10-minute Ghidra re-check during
   the fix leg should pin it before the gate is implemented.

---

## Session-4 addendum (2026-10-26, verified at `668c52f`/`a4053c4`) — PARTIAL FIX landed by the parallel core-api fix leg

The parallel fix leg's `a4053c4` (transport completion wave) landed while this finding
was being filed. First-hand verification against the claims above:

- **(b) stale-handle ladder — FIXED**: `reopen_handle()` + GLE-captured
  ERROR_INVALID_HANDLE heal-and-retry now in `src/win/cr_transport.cpp:202-260`
  (read loop :253-255; one heal per failing call; original open parameters kept — the
  og's clone-reopen parity, Device_decompiled.c:55947-55959 cited in their comment).
- **(c) chunking provenance — RESOLVED (doc-level)**: the envelope is now UNIFIED on
  both transports (RDA + direct) through `io::direct_envelope`; the fix leg's
  re-anchoring is honest and verifiable: the og's chunk sizes (4 MiB device
  `Device_decompiled.c:56545`, 7 MiB volume `:56912`, 2 MiB MFT batches
  NTFSSearchDLL:5649-5678, 1 MiB UsnJrnl, 64 KiB boot) are ALL 64 KiB multiples — so
  the core's 64 KiB envelope is I/O-shape-compatible with every documented og chunk
  size (compatibility anchor, not identity claim). M-030's provenance demand
  ("Windows heuristic, not og-behavior") is satisfied by this framing; the
  CORPUS-ERRATA row 34 (no 64 KiB device-layer rule) stands as written.
- **(d) disk-number bound — PARTIALLY FIXED**: `enumerate_disks(200, ...)` at
  `cr_ui.cpp:1044` and `:1232` — but the LDM cross-disk resolver still calls
  `enumerate_disks(32, ...)` at `cr_ui.cpp:1248` (inconsistent: LDM resolution is
  exactly the multi-disk path where >32 disks matters most).
- **Still open**: (a) WinPE environment gate — no GetSystemDirectory/X: probe anywhere
  in src/win (re-verified at `668c52f`); (e) terminal IOCTL_VOLUME_OFFLINE — zero
  hits repo-wide (re-verified); (f) silent RDA-open fallback at the ABI level — no
  transport-error surfacing found in cr_ui.h/cr_ui.cpp (re-verified); the stale
  RecoveryModule cite (:44686-44720 vs actual :60380-60450) — not yet re-checked by
  the fix leg.

**Disposition:** keep this finding OPEN (SEVERE→MAJOR scope now reduced to the four
open items: WinPE gate, terminal OFFLINE, the LDM-path 32-disk cap, silent fallback +
stale cite). The fix plan above remains valid for those items; items (b)/(c) can be
struck from the fix plan. OQ-S4-3 (relative perf measurement) is unaffected.

---

## Addendum (session 6 — WinPE driver-selection detail: eprdtdrv is ABSENT from PE; raw access rides ebrntdrv/epmdkdrv demand-loaded via NtLoadDriver, 2026-10-26)

The boot-config artifact (first-cited this session) sharpens this finding's WinPE-gate item with the driver-selection facts: in the WinPE payload, **eprdtdrv is not present** — PE raw disk access uses **ebrntdrv / epmdkdrv**, demand-loaded via **NtLoadDriver at first open** (WinPE/boot-config--winpeshl-autobat-regdrv.md:111-121; the regdrv.reg UpperFilter wiring and winpeshl autobat chain). Consequence for the core's WinPE gate design (this finding's fix plan step on the gate): the gate cannot simply branch on "driver present vs direct" — it must pick the SERVICE NAME (eprdtdrv on full Windows, ebrntdrv/epmdkdrv on PE) before deciding the transport route, or probe the PE pair first. The trio byte-diff (epmntdrv-trio--byte-diff-verification.md, first-cited this session) additionally proves ebrntdrv == eprdtdrv_win7older (identical MD5), i.e. the PE lane runs the OLDER driver build — capability differences (the completion-engine IOCTL set) should be checked against the win7older build when the gate lands.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-030 commit — `git log` "fix(M-030)")
**Verified by:** `Transport.PeGateDecisionMatchesOgProbe` + `Transport.QuiesceChainConstantsMatchCorpus` + `Transport.EnumerateBoundMatchesDriverGrammarSpace` (the corpus pins, Linux-lane; compile-fail pre-fix — cr_winenv.h and the export did not exist) + `CrUi.TransportErrorAdvisoryZeroOnCleanState` + the plain-C `ui_abi_check.c` exercise of `cr_ui_last_transport_error()` (the ABI consumer proof) + `win_syntax_check` green over all touched win-layer files (the stubs gained the SDK-faithful declarations: GetSystemDirectoryA, IOCTL_VOLUME_OFFLINE/ONLINE, SetVolumeMountPointW). The behavioral fail-befores for claims (a)/(e)/(f) are Windows-lane runs (the real DeviceIoControl chain): Transport.WinPEDirectFirst / Transport.OfflineTerminalStepInRepairChain / Abi.TransportErrorSurfaced are the Windows CI surface this fix wires. Full suite 852/852 linux-release + ASan exits 0; ctest 895/895.
**Resolution notes:** CONFIRMED-DIVERGENCE on all four open items, each re-verified against the corpus FIRST (the finding's three open questions resolved by this session's decode): **OQ-3 RESOLVED** — FUN_1800577e0's return polarity is decompiler-unrecovered (typed void) but the FOUR callers' branch patterns pin it: `== 0` (live Windows) -> the PID-token service grammar; nonzero (PE) -> `\\.\PhysicalDrive%d` (disk, :7816/:50653/:52437) / `\\.\HarddiskVolume%d` (volume, :352) DIRECT — the finding's polarity reading was right. **OQ-2 RESOLVED** — the restore path exists and is now decoded: FUN_1800459b0 (RecoveryModule_decompiled.c :34465-34502) opens the volume by identity, IOCTL_VOLUME_ONLINE (0x56C008 :34484), re-creates the mount point (vtable +0x78), Sleep(1000) :34499. (a) The WinPE gate landed: `win_pe_environment()` in src/win/cr_transport.cpp (GetSystemDirectoryA first byte 'X'/'x' — kernel32, the Win7-inclusive import gate intact; the corpus probe's inert \\.\X: open + IOCTL 0x560000 discards its result — the drive letter is the decision input), gating BOTH the `win::open_disk` RDA preference and the ABI's `open_disk_impl`; in PE the core goes direct like the og's callers, skipping the RDA-first latency (the core's own driver is absent from PE media — the session-6 addendum's ebrntdrv/epmdkdrv facts concern the og's OTHER components; Device.mo's storage factory itself goes PhysicalDrive-direct on PE, which is what the core mirrors). (d) The LDM-path `enumerate_disks(32,...)` at cr_ui.cpp raised to the og's full 200-disk space (`win::ENUMERATE_DISK_BOUND`, pinned to eprdtdrv_decompiled.c :184's disk#<=199 CREATE grammar). (e) The terminal OFFLINE step landed: `RwDevice::lock_volume` now issues IOCTL_VOLUME_OFFLINE (0x56C00C, RecoveryModule :60440 — the stale `:44686-44720` cite corrected to the real :60401-60447 chain; the comment's mislabeled FSCTL hexes fixed: DISMOUNT=0x90020, UNLOCK=0x9001c) after DISMOUNT, and `release_locked_volumes_` runs the corpus restore chain (GUID captured BEFORE DeleteVolumeMountPointW; reopen by \\?\Volume{...}\, ONLINE, SetVolumeMountPointW with the saved letter, Sleep(1000)) — closing the pre-fix letterless-after-repair hole: the core DELETED the mount point and never restored it. An OFFLINE failure degrades to the dismount+lock posture, surfaced through the N-048 log seam (never silent, never fatal — the og's own chain records GetLastError into its slot and proceeds). (f) The silent RDA fallback surfaced: `cr_ui_last_transport_error()` (ABI-since 2.2, additive) reports the surviving transport leg's error from the last cr_ui_open_disk; the RDA leg's error is preserved on the direct fallback. Prior-session items (b) the stale-handle ladder and (c) the chunking provenance were verified present and untouched. The full decoded quiesce chain (LOCK :60401 -> DeleteVolumeMountPoint :60412 -> Flush :60422 -> Sleep(2000) :60428 -> DISMOUNT :60431 -> UNLOCK :60436 -> OFFLINE :60440, restore ONLINE :34484 -> mount-point re-create -> Sleep(1000)) is pinned in include/cr/win/cr_winenv.h with the corpus line cites so both lanes compile the constants. OQ-S4-3 (the relative-perf measurement) remains tracked-open, unaffected by this fix (the 64 KiB direct-I/O envelope stays the registered deliberate improvement over the og's 4/7 MiB buffered-sync chunking).
