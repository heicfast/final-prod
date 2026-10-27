# DRIVER-UTILIZATION — the reference's driver usage vs. the core's, per interaction

> Session: core-api (2026-10-26). Baselines: core `e6d5b01`, driver `7e88e38`,
> corpus `03ad46c`. Authority hierarchy (mandate §Final): the **driver repo is
> the authority for the wire contract** (`src/shared/inc/rda_ioctl.h`,
> `rda_compat.h`, frozen by `C_ASSERT`s + 56/57 instruction-level checks vs.
> the three decompiled reference drivers); the **corpus is the authority for
> semantics** (how the og actually drives its drivers). Where they conflict,
> the reconciliation is documented here.

## 0. The reference's driver topology (corpus ground truth)

The og's recovery stack talks to exactly one driver family in production:
**EPMNtDrv (`eprdtdrv.sys`)**, opened ONLY by `PartitionRecovery/Device.mo`
(5 CreateFile sites). The other drivers are out of the recovery path:

| Driver | Role | Used by DRW recovery? |
|---|---|---|
| `eprdtdrv.sys` (EPMNtDrv) | raw disk/volume access | **YES — the recovery transport** |
| `ebrntdrv.sys` / `epmdkdrv.sys` | WinPE spare instances | PE failover only |
| `EUDCPEPM.sys` | disk-class UpperFilter, copy engine (session protocol) | NO — Todo-Backup copy chain, zero in-tree DRW IOCTL clients |
| `EUEDKEPM.sys` (EuDskAcs) | whole-disk raw access | NO — dormant deployment, **zero in-tree openers** (exhaustive search) |
| `EuMrx.sys` | NAS redirector | out of scope (remote recovery excluded by decision) |

Evidence: corpus `architecture/02-kernel-driver-architecture.md` §1-§6;
`eudcpepm-euedkepm--hook-drivers.md` §"Verified role + consumer analysis"
(zero `\\.\EUDKEPM` opens across DRWUI/PartitionRecovery/.mo/core/misc);
`epmntdrv-trio--naming-pid-handshake.md` §1-§3.

**Session-engine reconciliation (mandate §3):** the mandate directs "use the
driver's session engine for large sequential recovery reads the way the
reference does." The corpus answer is definitive: **the reference's recovery
path never uses a session engine** — EPMNtDrv is stateless (per-handle 16-byte
FsContext2, no sessions; `eprdtdrv-sys--driver-internals.md` §2-§4), and the
only session engine in the lineage (EUDCPEPM) belongs to the WinPE disk-copy
chain with no DRW client. The RDA driver's session engine (18-code protocol,
rebuilt from EUDCPEPM) is therefore a **copy/imaging surface, not a recovery
transport**. The core matches the reference by driving plain
grammar-open + ReadFile for scans; the session client (deleted by N-041 -
wire-exact vs `rda_ioctl.h` with static_asserts) is retained as the
imaging/verification surface — production-dark, same as the og's non-use.
This is the documented reconciliation.

## 1. Per-interaction audit

### 1.1 Open and addressing — **MATCH**

| Aspect | Reference (corpus) | Core (verified) | Verdict |
|---|---|---|---|
| Device path grammar | `\\.\{SYM}\<PID-token>\<disk>\<part>` (3-segment); `\\.\{SYM}\<PID-token>\Volume\<N>`; `\\.\{SYM}\<PID-token>\<disk>` (2-segment) — `epmntdrv-trio §3.3`; client sites `Device_decompiled.c:329, 7793, 50628, 52414, 54621` | `open_rda_partition`: `\\.\%s\%u\%u\%u` (cr_transport.cpp:617-624); `open_rda_volume`: `\Volume\%u` (:642-664); 2-segment via the grammar bridges (the RdaClient bridges were deleted by N-041) | **MATCH** |
| PID token | `b0 \| (b0+b1)<<8 \| (b1+b2)<<16 \| (b2+b3)<<24` over caller PID LE bytes, 8-bit wrapping adds; driver re-derives at IRP_MJ_CREATE, mismatch → `STATUS_INVALID_PARAMETER` | `compute_pid_token` — byte-identical transform (cr_transport.cpp:285-289 vs driver `rda_compat.h:49-57`) | **MATCH** |
| Service-name-derived device | driver builds `\Device\` + UPPERCASE(last registry-path component); client hardcodes the service name (`EPRDTDRV`) | core uses service name `RDA` (driver installed as `rda` → `\\.\RDA`); parameterized in every open function | **MATCH** (name differs by deployment; mechanism identical) |
| Access/share | volume: GENERIC_READ share 0; disk functor: share 0; clone default 0xC0000000 share 3 | GENERIC_READ, share R\|W\|D (cr_transport.cpp:622-624) | **PARTIAL** — core's share set is a documented superset (coexistence with OS handles on the same disk while scanning; the driver device is non-exclusive). No contract effect: the driver applies no share checks |
| Handshake on open | none (token only; no version/magic exchange) | none — production path matches; the structured client surface was deleted by N-041 (production-dark, zero callers) | **MATCH** |
| Geometry on open | passthrough IOCTLs (0x70000 geometry, 0x74004 partition-info-ex, 0x7405c length) through the EPRDTDRV handle; sector default 512 | `probe_geometry`: IOCTL_DISK_GET_LENGTH_INFO + GEOMETRY_EX + STORAGE_QUERY_PROPERTY (access-alignment, corrected +0x10/+0x14) (cr_transport.cpp:63-122) | **MATCH+** (exact-length + 512e/4Kn correction superset) |

### 1.2 Request shape — **MATCH** (one gap: read granularity, §1.7)

| Aspect | Reference | Core | Verdict |
|---|---|---|---|
| Read units | bytes at absolute byte offsets (driver passes `Read.Length` raw; sector math client-side) | bytes at absolute offsets (SetFilePointer-equivalent via OVERLAPPED) | **MATCH** |
| Reported-count contract (F11) | driver reports `Information = Length × 512`; client guard: `if (chunk < got) got /= 512` (`Device_decompiled.c:55964-55968`) | `overlapped_read` accepts `got == len` or `got == len×512` when rda_scaled (cr_transport.cpp:43-59) | **MATCH** — the core implements the same compensation pair the og does |
| Write path | twin, same ×512 scale | `RwDevice::write_at` accepts both counts (cr_transport.cpp:817-854) | **MATCH** |
| Control IOCTLs | 0x60002400/04/08 through the handle (GetProcName/GetFileObjName/GetDiskContextMode) + storage passthrough | wire structs documented in `cr_rda.h` (client deleted by N-041); storage passthrough achieved by issuing standard IOCTLs on the grammar handle (`probe_geometry`) — the same passthrough the og exploits | **MATCH** |

### 1.3 Completion model — **MATCH**

Reference: strictly synchronous — `LPOVERLAPPED` NULL at every raw-path site
(`Device_decompiled.c:57010-57028, 55940-55984, 56143-56189`); kernel-side the
driver builds async FSD IRPs and waits (invisible to user mode). Core:
handles are NOT opened FILE_FLAG_OVERLAPPED; the stack `OVERLAPPED` carries
the 64-bit offset; completion via `GetOverlappedResult(..., TRUE)`
(cr_transport.cpp:43-59). Effective semantics: identical synchronous model.
The mandate's rule "never issue a synchronous round-trip where the reference
issues an overlapped/asynchronous one" is satisfied trivially — the reference
issues none. (The one pending path in the whole system, session GETNEXTBLOCK,
is unused by recovery on both sides.)

### 1.4 Error mapping — **MATCH** (self-healing reopen landed)

Reference (`Device.mo` class codes, decompiled): `0x57` invalid parameter;
`5` device not open; `0x284` out-of-bounds sector range; `6` invalid handle →
**self-healing reopen** (CloseHandle → re-CreateFile → retry seek,
`Device_decompiled.c:55947-55959`); partial reads tolerated in a loop; no
backoff. Driver statuses pass through as GetLastError.

Core (core-api session): read failures collapse to `ERR_READ_FAILED`
(deterministic), opens to `ERR_OPEN_FAILED`, bounds to
`ERR_RANGE_OUT_OF_BOUNDS`; the retry layer (3 attempts, bisect to sector
granularity, Skip-mode zero-fill) sits above. **The error-6 self-healing
reopen is now implemented on every device class** (DirectDevice,
RdaTargetDevice — grammar re-CreateFile re-binds at IRP_MJ_CREATE;
structured devices re-issue OPEN_TARGET after reopen; RwDevice on both read
and write paths, `Device_decompiled.c:56182-56183` write-loop guard parity):
the failing GLE is captured inside the transport read/write helper, and one
heal-and-retry rides a fresh handle under a per-device mutex.

Granularity note: the driver can return a wide NTSTATUS set
(rda_ioctl.c/rda_readwrite.c: INVALID_PARAMETER 87, BUFFER_TOO_SMALL 122,
INSUFFICIENT_RESOURCES 1450, NO_MEMORY 8, INVALID_HANDLE 6, SHARING_VIOLATION
32, CANCELLED 995, literal 0xC0000120 on pump-drain, plus verbatim
lower-stack statuses). The reference maps none of these granularly either —
its user-visible error space is the `0x5002xxxx` HRESULT family. The core's
deterministic coarse mapping is reference-accurate; the per-status table is
documented in the driver repo (`docs/ground-truth-contracts.md`) and
referenced from `docs/ABI-CONTRACT.md` error annex rather than duplicated.

### 1.5 Session lifecycle — **MATCH (both sides: no sessions in recovery)**

See §0 reconciliation. Core session client exists, wire-exact, dark — mirroring
the og's zero use. The imaging surface (future wizard) will use
flush/StartBackup/GETNEXTBLOCK/GET_PROGRESS/CLOSE per the driver's client
checklist (driver repo read, §5 of the driver report); the one driver-side
trap (GETNEXTBLOCK before StartBackup → literal `0xC0000120`) is already
handled truthfully (FB-T07 fix, cr_transport.cpp:529-560).

### 1.6 Access control — **DELIBERATE DIVERGENCE (keep)**

Reference weakness (corpus-verified mechanics): plain `IoCreateDevice`, no
SDDL, non-exclusive (`EUEDKEPM_decompiled.c:577`; same for EPMNtDrv) — "No ACL
on the device" (`eprdtdrv-sys--driver-internals.md` §6); the PID token is
self-derivable (anti-hijack, not anti-unprivileged) — so any process can open
`\\.\EUDKEPM\<own-token>\<disk>` and read whole disks raw.
Our driver: `IoCreateDeviceSecure` with `D:P(A;;GA;;;BA)(A;;GA;;;SY)`
(rda_p.h:62) — admins/SYSTEM only. The core honors it (no bypass, no
weakened path, no invented access). Registered hardening in
`docs/PARITY-DELTAS.md` (D-F2/F3 lineage). **Not replicated by policy.**

### 1.7 Read granularity and queue depth — **MATCHED to the og's class** (tuned this session)

Reference (decompiled, first-hand by the corpus leg): device-layer chunks
**4 MiB** per ReadFile (CWindowsStorageDevice, `Device_decompiled.c:56545`);
direct volume path **7 MiB** (:56912); engine-level MFT batches 4096 sectors
(2 MiB @512B, `NTFSSearchDLL_decompiled.c:5649-5678`); $UsnJrnl 1 MiB; boot
region 64 KiB. All sizes are 64 KiB multiples **by construction**.
Concurrency: one outstanding IRP per handle per thread; parallelism from
engine worker threads.

Core after this session (verified): engine reads coalesce to **256 KiB cache
blocks** (CachedReader, 8 MiB LRU = 32 blocks — was 64 KiB); carve 1 MiB
windows; writer 1 MiB chunks; preview ≤4 MiB; probes ride the unified 64 KiB
envelope. Sequential transitions per GiB: 64 KiB blocks ≈ 16,384 →
256 KiB blocks ≈ 4,096 (the og's device cadence is 256-512/GiB). The
`alignbench` sweep (`bench/ours/run-alignbench.ps1`, workflow
`api-benchmark.yml`) measures the size/QD plateau on Windows CI with the
driver loaded; further tuning rides its data. CachedReader still serializes
engine reads under its mutex (P2-5 contract) — the queue-depth sweep records
what QD>1 would buy before any architectural change.

### 1.8 Push vs pull — **MATCH**

Reference driver path: pure pull (synchronous request/response; no events, no
inverted call on EPMNtDrv — `eprdtdrv_decompiled.c:671-744`). The one
inverted-call mechanism in the lineage (EUDCPEPM GETNEXTBLOCK) is unused by
recovery. Core: no driver polling, no driver events; progress/events are
engine-side pushes to the host. Nothing to add — matching the reference means
not inventing driver events.

### 1.9 Driver-absent behavior — **REMOVED (deliberate divergence, driver-only session)**

Reference: every EPRDTDRV open first probes (`FUN_1800577e0`); on probe
failure the SAME code path opens `\\.\HarddiskVolume%d` /
`\\.\PhysicalDrive%d` directly — silent fallback, "the 'no-admin' degraded
mode" (`epmntdrv-trio §6`; sites `Device_decompiled.c:352-359, 7816-7823,
50653-50660, 52437-52444, 54667-54673`). Missing service → probe returns 0,
no refusal (`PartitionRecovery_decompiled.c:2606-2611`).

Core (driver-only session): **the fallback is gone**. The data path is the
RDA grammar path ONLY; a non-Ready driver state is a classified refusal
(`ERR_DRIVER_*` family 0x500B: absent / not-started / unauthorized /
version-mismatch / errored) returned by every driver-requiring open and
surfaced by the CLI as exit 4 with per-condition stderr (docs/DRIVER-REQUIRED.md).
`rda_available` (the boolean probe) is deleted — superseded by
`query_driver_status` + the pure `driver_state` classifier
(cr/win/cr_driver_status.h; pinned on the Linux lane by
tests/unit/test_driver_status.cpp). The WinPE gate is removed with it (the
driver is required in PE too; rda-driver ships scripts/winpe/regdrv-rda.reg).
This is a registered divergence (PARITY-DELTAS): the product does not market
a non-driver path, and silent degradation was the direct cause of the
"product installed, nothing works" failure class.

### 1.10 Buffering — **MATCH**

Reference: driver path DO_DIRECT_IO (kernel MDL, no user-side buffering
flags); direct path opens with FILE_ATTRIBUTE_NORMAL (no NO_BUFFERING
documented anywhere in the client). Core: same — FILE_ATTRIBUTE_NORMAL on
every handle; the RDA path is the driver's MDL direct path; the direct path
compensates with the 64 KiB envelope (below).

## 2. The 64 KiB direct-read alignment (mandate §3, non-negotiable) — **UNIFIED, LANDED**

Constraint: every direct read the core issues — **through the driver or
through the direct fallback** — must have lengths rounded up to a 64 KiB
multiple, clamped at the device end. Reads whose length is a multiple of
4 KiB but not 64 KiB take the documented ~3.5× slow path (~830 vs ~2900 MB/s
on NVMe).

Landed state (core-api session, verified in code):
- **BOTH paths use the one `io::direct_envelope`** (cr_transport.cpp
  `aligned_read_at`): offset rounded down / length up to 64 KiB multiples,
  clamped at the device end, zero-copy fast path when already aligned. The
  pre-unification RDA sector-only envelope is gone; 64 KiB subsumes sector
  alignment for every real Windows logical sector size (512/4096/65536), so
  the bench-proven sector-rejection property (run 40073093997) is preserved.
- Direct path, RwDevice (repair), RDA grammar path: one envelope, one code
  path. The F11 ×512 count compensation is unaffected (it keys on the ISSUED
  length).
- The og satisfies the same property by construction (all chunk sizes are
  64 KiB multiples — §1.7), so the unified envelope is ALSO an exact match
  to the reference's observable I/O shape, not just a hardening.

Verification: `test_directalign.cpp` (9 tests incl. the exhaustive
`UnifiedEnvelopeExhaustivePropertyBothDevices` sweep — every (offset,length)
combination over aligned and ragged devices: issued offset always 64
KiB-aligned, issued length always a 64 KiB multiple except the tail clamp,
request always fully covered) + the `alignbench` Windows CI job (the
4-KiB-vs-64-KiB slow-path demonstration + the size/QD sweep, numbers recorded
to artifacts).

## 3. Performance notes (measured numbers: see PERFORMANCE-BASELINE.md)

- Driver transport microbench (driver repo, run 39807929475): our driver at
  parity or better on every axis vs. the original eprdtdrv (seq 64 KiB
  532.9 vs 391.0 MiB/s; +52% hostile dispatch; 64/64 SHA-identical ranges).
- The core's per-call transport overhead is dominated by transition count:
  today 64 KiB blocks → ~16k transitions/GB on the sequential path vs. the
  og's ~256-1024/GB. The read-size sweep quantifies and fixes this.
- API call overhead: no measurement exists for cheap cr_ui ops — added this
  session to the Windows CI benchmark job.

## 4. Interaction checklist status

| Interaction | Status |
|---|---|
| Open/addressing (grammar + token) | MATCH — verified |
| Handshake | MATCH (none, like the og) |
| Request shape (units, F11 scale) | MATCH — verified |
| Completion model (synchronous) | MATCH — verified |
| Error mapping incl. error-6 self-heal | **MATCH — landed this session** |
| Session lifecycle | MATCH (no sessions in recovery, both sides) — reconciled §0 |
| Access control (SDDL) | DELIBERATE DIVERGENCE — keep, documented |
| Read granularity | **MATCHED to og class** — 256 KiB blocks + unified envelope; sweep measures the plateau |
| Push/pull events | MATCH — verified (pull on both sides) |
| Driver-absent behavior | **REMOVED (deliberate divergence)** — driver-only, classified refusals (docs/DRIVER-REQUIRED.md) |
| Buffering per class | MATCH — verified |
| 64 KiB envelope on EVERY direct read | **MATCH — unified on both transports, exhaustively unit-proven, CI-benched** |


## Driver release pinning (2026-10-26 addendum)

The bench lane's driver pin moved with the driver repo's releases:
`api-benchmark.yml` `RDA_RELEASE_TAG` is **v0.2.2** (the sc.exe
installer-script fix + docs truth pass) as of the v0.4.1-parity tree
(previously v0.2.1). Functional floor for ANY consumed release is
**>= v0.2.1**: v0.2.0 fails every bare `\\.\RDA` open with error 87 (the
classifier ordering bug fixed in the driver's `7e88e38`), which silently
disables the core's whole RDA transport via `rda_available()` probing.
The runtime wire contract is version-decoupled (driver PE/wire version
1.0.0.0 by design); the bench harness verifies the service install and
device open per run (`driver-info.json` in the run artifacts).
