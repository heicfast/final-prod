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
| BitLocker placement | S61 | **DIVERGENCE-MAJOR** — M-017: og in-stack XTS-AES decrypt; ours pre-unlock gate (no decrypting device) |
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
| M-017 | MAJOR | BitLocker: og in-stack decrypt vs our pre-unlock gate |
| M-018 | MAJOR | No NAS/remote device layer (CRemoteDevice/EuMrx) |
| N-025 | MINOR | Cache budget 8 vs 4 MiB |
| N-026 | MINOR | No geometry refresh path |
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
| N-045 addendum | — | Algorithm divergence added: og CStorageDeviceWithCache is a prefetch read-ahead engine (prefetch thread, 10-run window, 1000-sector runs) vs core reactive true-LRU; cite refreshed to cr_scan.cpp:647 / 256 KiB blocks |
| N-055 | MINOR | Enumeration breadth: og spans WMI+SetupAPI+VDS+StorageSpaces+FunctionDiscovery; core is the PhysicalDrive CreateFileW loop (cr_enumerate.cpp:15-60) |

MATCH/verified registers from the never-cited-files read: cr_elevate's probe is a byte-shape
twin of DRWUI's elevation check (elevation ACTION is host-side — deliberate, no finding);
cr_decomp's NTFS LZNT1 parity re-verified (N-058 fixes the stale CAPABILITY-MATRIX claim);
cr_sha256 is a deliberate core addition (carve ids + writer verify — D-19 side registered).
