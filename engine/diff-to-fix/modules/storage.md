# modules/storage.md — storage/slice/device layer audit verdicts (session 3)

Reference (og ground truth): `CPartitionVirtualVolumeDevice` + `IStorageDevice`
family, decoded from HFSPlusSearchDLL (`FUN_180009130/:7219-7264`) and DRWUI
(`FUN_140643760/:1044939-1044979`, identical), `rtti_classes.json`, vtable map
`drwwui_vtable_map.json:38427-38462`, `architecture/05-scan-engine-architecture.md`
§4.2/§8. Evidence: `evidence/decompiled-quotes/s3-virtual-volume-device.md`.

Core (audited commit `7fd00e0`): `include/cr/storage/` (cr_device.h, cr_slice.h,
cr_cached.h, cr_retry.h, cr_devmap.h), `src/core/storage/cr_slice.cpp`,
orchestrator pipe `src/core/orchestrator/cr_scan.cpp:595-614`.

## Parameter verdicts

| # | Parameter | Verdict |
|---|---|---|
| Role (bounded virtual volume for OS-invisible partitions) | A54 | **M** — clean-room equivalent, role identical |
| Read translation (offset rebasing to parent) | A26 | **M** — `parent->read_at(start_+offset)` ≡ og `parent->Read(off+base)` |
| Bounds enforcement (tail reads) | S35 | **DIVERGENCE-MAJOR** — M-016: og clamps+zero-fills+ succeeds; ours refuses (ours safer; og engines written against the zero-fill contract; og's tail memset has a sector-unit length defect) |
| Ctor validation | Q43 | **DIVERGENCE-INFO** — I-012: ours eager, og deferred |
| Construction points (lost candidates / volume gate / repair-analyze) | A57 | **M** (superset on repair-analyze) |
| BitLocker placement | S61 | **FIXED (DFX20 / M-033, this session)** — the in-stack XTS-AES decrypting device now exists: `DecryptingDevice : IStorageDevice` (cr_decrypt_device.h) wraps the volume slice with per-sector LE-index tweaks (the og's layer order, corpus arch doc §8); AES-128/256+XTS clean-room in cr_aes.cpp pinned byte-exact against pycryptodome AND OpenSSL EVP (the og's own cipher interface) + FIPS-197 KATs. Key surface = the og's GetFullVolumeEncryptionKey output shape (FVEK hex via `--bitlocker-fvek` / `cr_ui_open_volume_with_key`); locked-WITHOUT-key refusal preserved (S61 legitimate-unlock-only). The recovery-password/BEK→VMK→FVEK unwrap chain is out-of-corpus (ParserBitLocker datum parse behind opaque C++ graphs) — tracked-open OQ-S20-1. 13 tests in test_bitlocker_xts.cpp; suite 608/608. |
| Write handling | A59 | **M** — no write path on either side's device interface |
| Error model | S35 | **M+INFO** — I-012: per-call codes + ReadErrorMap superset; no sticky error |
| Units/granularity | C20 | **DIVERGENCE-INFO** — I-012: bytes+alignment (superset of og sectors) |
| I/O alignment | P51 | **M(ish)** — 64 KiB envelope (GAP-S6-13) vs og 512B-sector IOCTL; deliberate perf |
| Cache layer placement/budget | P49/P47 | **M** placement / **DIVERGENCE-MINOR** budget — N-025: 8 MiB vs og 4 MiB |
| Bad-sector resilience | S60 | **M-in-spirit** — RetryingDevice generalizes og's EXT4-only catch-buffer proxy |
| Sibling completeness (NAS/remote) | A54 | **DIVERGENCE-MAJOR** — M-018: CRemoteDevice/EuMrx absent (scope decision) |
| Geometry refresh | S35 | **DIVERGENCE-MINOR** — N-026 |

## Session 3 findings index

| ID | Severity | One-line |
|---|---|---|
| M-016 | MAJOR | Tail-read semantics: og clamp+zero-fill vs our strict refusal |
| M-017 (= global M-033) | MAJOR | BitLocker: og in-stack decrypt vs our pre-unlock gate — **FIXED (DFX20, findings/FIXED/M-033)** |
| M-018 | MAJOR | No NAS/remote device layer (CRemoteDevice/EuMrx) |
| N-025 | MINOR | Cache budget 8 vs 4 MiB |
| N-026 (=N-046) | MINOR — **FIXED (session 18, DFX18)** | No geometry refresh path — `PartitionSlice::refresh_geometry()` (og FUN_140643870 RefreshSectorSize-from-parent, DRWUI_decompiled.c:1044889-1044895) with revalidation hardening (misaligned/shrunken/zero-sector media changes refused, snapshot untouched); Slice.GeometryRefreshAfterMediaChange + GeometryRefreshRefusesInvalidatingChange |
| I-012 | INFO | Deliberate supersets ledger (units, eager validation, error map) |

## Notes

- The layer was never audited before session 3 (the session-1/2 audits
  covered engines and orchestration; the slice layer was only incidentally
  touched via A-003's rebase work).
- A-003 (FIXED) rebased repair plans onto the parent basis through this
  layer; the og has no repair-write path through the device family at all
  (repair is a separate product world — `architecture/06-repair-chain.md`),
  which our separate write-device design mirrors.

---

## Integration note (2026-10-26)

This module file is the *storage/census parallel leg's* first audit of the
volume-abstraction layer (CPartitionVirtualVolumeDevice + our PartitionSlice),
filed during the multi-leg reconciliation. It complements — and does not overlap —
`modules/transport.md` (the S4 leg's Windows transport/driver-client audit:
EPRDTDRV grammar, WinPE gate, chunking, handle ladder — the layer BELOW the
storage devices audited here). Findings: M-032/M-033/M-034/N-045/N-046/I-017
(renumbered from this leg's local M-016/M-017/M-018/N-025/N-026/I-012).

---

## Session-6 rows (deep continuation, 2026-10-26 — cr_cached.cpp / cr_enumerate.cpp first-cited)

| ID | Severity | One-line |
|---|---|---|
| N-045 addendum → **FIXED (session 18, DFX18)** | — | Algorithm divergence added: og CStorageDeviceWithCache is a prefetch read-ahead engine (prefetch thread, 10-run window, 1000-sector runs) vs core reactive true-LRU; cite refreshed to cr_scan.cpp:647 / 256 KiB blocks. **Resolution:** budget + algorithm REGISTERED as PARITY-DELTAS D-72 (the cite re-truthed at the N-207 repair: the pre-fix text carried the stale D-75 - the scan-cache row has been D-72 since the D-72-collision errata; corpus cites re-verified first-hand: DRWUI_decompiled.c:919080-:919092 `0x400000/sector` + fallback 0x2000; ctor :962250-:962597); divergence block in cr_cached.h; benchmarking.md §4 intent note; doc-truth pin `Cached.ScanCacheBudgetIsTheRegisteredDivergence` (fail-before RED on the pre-fix tree, suite 592/592 after). Finding file in FIXED/. |
| N-071 (was this leg's local N-055; renumbered at integration) | MINOR — **FIXED (s31 — DFX31)** | Enumeration breadth: og spans WMI+SetupAPI+VDS+StorageSpaces+FunctionDiscovery; core is the PhysicalDrive CreateFileW loop. Two-stack merge landed (cfgmgr32 GUID_DEVINTERFACE_DISK pass + pure merge_disk_enumeration); residual stacks registered as PARITY-DELTAS D-79 |

MATCH/verified registers from the never-cited-files read: cr_elevate's probe is a byte-shape
twin of DRWUI's elevation check (elevation ACTION is host-side — deliberate, no finding);
cr_decomp's NTFS LZNT1 parity re-verified (N-058 fixes the stale CAPABILITY-MATRIX claim);
cr_sha256 is a deliberate core addition (carve ids + writer verify — D-19 side registered).

---

## Session-11 fix row (2026-10-26, DFX11 — M-032 closed by decision)

**M-032 FIXED** (the fix plan's own decision path 1 — no behavior change
wanted): the slice tail-read refusal stays; the divergence from the og's
clamp+zero-fill virtual-volume contract (FUN_180009290 :7219-7264) is
registered D-74 and pinned (Slice.TailReadRefusalPinned + the updated
BoundsEnforced). Engine audit: no reliance on the og contract (the HFS+
journal ring reads are bounded within the ring; everything else is
explicitly bounded; refusals degrade via the damage counters). The
opt-in clamped-read primitive (path 2) is the documented next step WHEN
an engine needs it. I-017's register is unchanged; M-033 untouched.

## Session-30 fix row (2026-10-26 — N-071 closed by scope registration + the busy-disk hardening)

| Parameter | Verdict |
|---|---|
| B8 device discovery (enumeration breadth) | **MATCH-by-scope-registration (D-80)** — the engine's device truth stays the PhysicalDrive namespace + volume-letter companion; the og's five-stack fan-in is MOM UI machinery. The load-bearing observable (a busy disk stays listed) is ported: query_disk retries a 0-access metadata-only open on ERROR_ACCESS_DENIED/ERROR_SHARING_VIOLATION (busy=true on the row). Revisit trigger in D-80: a live miss-case (disk in Disk Management but absent from `rrecov list`) reopens the row |
| P42/A54 | the scope contract documented at the surface (cr_enumerate.h SCOPE note); the SetupAPI disk-interface pass named as the designed extension point |

Q65: Enumerate.{ScopeDeltaRegisteredWithRevisitTrigger, BusyDiskMetadataRetryShipsInQueryDisk, ScopeContractDocumentedInHeader} (source-truth pins; win-layer behavior parse-gated on Linux, exercised on the MSVC lane).

## Session-24 verdict update (M-034 scope-registered, DFX24, 2026-10-26)

**NAS/remote device layer — `M-034` resolved by scope registration.**
PARITY-DELTAS D-88: NAS scanning is excluded from the core's device
scope (the EuMrx redirector is host-layer surface; the corpus proves
the scan-path plumbing only — deleted-file yield over SMB is
undecoded and is the row's REVISIT TRIGGER). cr_device.h carries the
divergence block; `Storage.ParityDeltasListsNasExclusion` pins both
artifacts. The IStorageDevice contract itself is transport-agnostic
and remains open to an SMB-backed host device — the Q54/Q55/A8
verdicts: MATCH-by-registered-scope-exclusion.

---

## Session-12 continuation row (2026-10-29, audited `c7fa527c`, corpus `be0cfe9`)

**M-119 FIXED (S47 MAJOR-first fix lane, `033b9cdb` — the tolerant pair landed: create-clamps an over-long declared window [was_clamped/clamp_delta; the start-past-device refusal kept per N-046], the read ladder refuses only the parameter errors while the straddle clamps + FULL-tail zero-fills + succeeds [FUN_180009290 :7219-7264 confirmed first-hand at the fix-time probe]; D-74's read-half superseded with a dated PARITY-DELTAS addendum, its safety core + under-zeroed-tail non-adoption kept; CLI warns-with-delta, both cr_ui opens clamp+log; F60/P1/B6/B8/X51/E23 → MATCH; 3 RED-pre-fix tests, suite 1216/1216):** partition-boundary read semantics — the og's
CBasicPartitionDevice read wrapper (RTTI-confirmed, DRWUI_decompiled.c:183633-183666)
CLAMPS a past-end read to the in-bounds prefix, ZERO-FILLS the tail, and returns
success (the `"i64CountofSectors - i64RealReadSector = %I64d"` diagnostic fired in
the real-run log at DRWUI.log:638 — the probe continued and the partition was
found). The core's `PartitionSlice::read_at` (cr_slice.cpp:46-49) returns
ERR_RANGE_OUT_OF_BOUNDS, and `PartitionSlice::create` (:25-28) REFUSES the whole
slice on declared-length overrun — which the CLI partition open (rrecov.cpp:2973-2978,
raw table length → "cannot slice partition" abort) and both cr_ui volume opens
(:1372/:1456, ERR_OPEN_FAILED) pass raw. The discovery's own `materialize()`
(cr_candidates.cpp:493-501) already clamps ragged tails — the internal
inconsistency. F60/B6/E23/X51 verdict: **DIVERGENCE-MAJOR** pending the
create-clamp + read-clamp pass (read-side only; the write path keeps hard bounds —
the fix plan's fence test). Evidence:
`evidence/decompiled-quotes/s12-partition-read-clamp.md`.

---

## Session-42 S42 addendum (2026-10-29 — the bad-sector skip constants + the Ldq ladder; the DefaultLog real-run decode)

`evidence/decompiled-quotes/s42-defaultlog-ldq-decode.md` (the three classes DefaultLog.txt
carries that no prior lane had dispositioned — zero trail hits at this lane's open):

- **The Ldq read-parameter ladder** (DRWUI :180139-180160): three refuse classes
  (negative start/count; count ≥ 4 GiB-of-sectors; start ≥ device size) + the over-long
  CLAMP + the error-path zero-fill — **M-119's addendum** (its finding file carries the
  sharpened acceptance; the 393 real-run refusals corroborate that the og's scan layers
  over-read routinely and the transport's refuse-with-log is normal operation).
- **The bad-sector skip machinery's constants** (:179930-180080): 2 MiB initial span,
  read size doubling every 3 steps capped 64 MiB, inner per-granularity skip above
  4 MiB, ERROR_CRC (0x163) trigger, the 80 MiB-skipped UI-message gate (once-only),
  skip-range recording into the device's bad map, 1 GiB total-skip break. **REGISTER —
  mechanism-divergent (skip-forward-with-backoff) vs the core's bisection
  (`cr_retry.h`), outcome-equivalent** (sector-granularity isolation, zero-fill, the
  recorded map, the BadSectorTip state per FB-O03). No finding: the user-observable
  matches.
- **The recovery-write failure observable**: one real-run
  `Failed to write file! CFileRecoverWorker::ReadAndWriteDataRunList 1515` — the
  per-file failure log with function+line; the core's SafeWriter records per-item
  per-extent failures into the report (the S-052 model). Register datum (the N-048
  log-census family).

## S55 addendum (2026-10-29 — the sixth continuation lane: the device-geometry deep dive)

New on this module: **N-206** (the device sector-count rounding — the og's
authoritative geometry-EX path CEILS the last partial sector
((L−1+ss)/ss, Device_decompiled.c:56536-56538 with the 0x200 fallback), the
core FLOORS everywhere (cr_transport.cpp:134/:139, cr_image.cpp:427) — the
tail of a non-sector-multiple source is structurally invisible to the core
while the og addresses it zero-padded; the device-tail bound consumers
(disk->size()/ls sites) inherit the floor). Verified-MATCH with new cites:
the geometry cascade 0x70000→0x74004→0x7405C decoded first-hand; the 4 MiB
device chunk sizing (0x400000/ss at Device:56538-56545) re-pinned — the
device-layer figure OQ-S4-3/M-030 register, now separated from the
engine-layer 4 MiB of N-204. Evidence:
`../evidence/s55-lane-sheets/S55-D2-scanmath.md` §3.
