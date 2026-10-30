# DESIGN-CONSTRAINTS — the gate document for the driver→direct replacement

> Phase-0 synthesis sheet (driver-replacement session, Task T2-e / Agent E). Consolidates
> the four context sheets — `worklog/ACCESS-LAYER.md` (A), `worklog/CONSUMERS.md` (B),
> `worklog/DRIVER-CONTRACT.md` (C), `worklog/REFERENCE-ACCESS.md` (D) — and cross-checks
> their load-bearing claims against the tree at HEAD `b6418420` (every code cite below was
> re-read for this document). **This is the contract the rest of the session builds against**:
> what must be preserved exactly, what must change, and the measured bar. Where the sheets
> and the code disagree, the code wins and the disagreement is noted.
>
> Companion authority: `worklog/session-driver-replacement-prompt.md` (the mandate).
> Sheet cites use the form `(A §n)`, `(B §n)`, `(C §n)`, `(D §n)`.

---

## §0 The mandate and the phase order

**The mandate, one paragraph.** Replace the kernel-mode RDA driver entirely with direct
user-mode raw I/O (`\\.\PhysicalDriveN`, volume-GUID/letter handles) under administrator
privilege, as a surgical layer swap beneath the engines: every capability, error class,
threading/lifetime guarantee, geometry/capability query, output, format, JSON schema,
exit code, and user-visible semantic is preserved **identically** — the only user-visible
changes are (i) the absence of the driver and (ii) the honest admin-privilege model that
follows from direct access. The driver path is not gated, deprecated, or left as a
fallback — it is deleted from code, build, installer, tests, workflows, and docs
(mandate §2, §5; session-driver-replacement-prompt.md:8-14, 80-87, 222-231). Performance
is the primary objective after correctness: every consumer's throughput, latency, CPU,
memory, and cancellation responsiveness must meet or beat its driver baseline on real
Windows hardware, measured per consumer (mandate §3).

**Phase order** (mandate §4 + prompt §10; correctness before performance, non-negotiable):

1. **Phase 0** — the five context sheets + THIS gate document, pushed before any code change.
2. **Phase 1** — baseline capture on real hardware for every consumer → `worklog/PERFORMANCE.md`
   (prompt T4; the run-ID numbers in §7 below are the captured driver-era baselines).
3. **Phase 2** — design the direct layer as a drop-in contract → `worklog/DESIGN.md` (T5).
4. **Phase 3** — implement + wire, correctness first: primitives (open/geometry/read/cancel/
   close/error map), edge states, privilege model, then wire engine→orchestrator→session/
   writer/health/repair/visualization→ABI/CLI (T6-T12). Full test suites green at each step.
5. **Phase 4** — removal: source/headers/build/tests/benches (T13), installer (T14), docs (T15).
6. **Phase 5** — end-to-end scenario matrix, byte-for-byte against the pre-replacement baseline
   on the same seeded disks (T16).
7. **Phase 6** — performance measurement/tuning; every result captured; tuning never trades
   correctness (T17).
8. **Phase 7** — full CI sweep green for the right reason; final verification against every
   §8 gate of the mandate (T18-T19).
9. **Phase 8** — handoff (`worklog.md` complete: SHAs, run links, measurements, deferred items
   with precise scope) (T20).

---

## §1 The operation inventory — every transport operation today → its direct replacement

The transport's public surface is `include/cr/win/cr_transport.h:53-152`. Every row below
was verified in `src/win/cr_transport.cpp` at HEAD.

| # | Operation today (site) | Direct replacement | Semantic deltas to preserve |
|---|---|---|---|
| 1 | `open_rda_partition(svc, disk, part, err)` — grammar `\\.\SVC\<tok>\<d>\<p>`, GENERIC_READ, share R\|W\|D, probe_geometry, zero-size refuse (cr_transport.cpp:483-508) | `open_physical_drive`-shaped open: `CreateFileW(L"\\\\.\\PhysicalDrive%u")`, **same access/share/disposition/flags**, same `probe_geometry`, same zero-size refusal. The PID token, grammar string, and `part` (never used driver-side — C §1.2) all die. | Whole-IO `read_at` contract; per-handle single-flight; heal-on-GLE-6; `ERR_OPEN_FAILED` on CreateFile/zero-size; bounds → `ERR_RANGE_OUT_OF_BOUNDS` (cr_transport.cpp:333-346) |
| 2 | `open_rda_volume(svc, volume, err)` (cr_transport.cpp:510-534) — **zero production callers** (A §2.2) | **Delete, do not port.** No consumer. Volume views are built as `volume_extent(letter)` → whole-disk open → `PartitionSlice` (cr_ui.cpp:1489-1516; rrecov.cpp:772-786) — that shape stays. | none (dead grammar branch) |
| 3 | `open_disk(disk, service, err)` = `open_rda_partition(svc, disk, 0)` (cr_transport.cpp:574-586) | Same function, one leg: the direct `\\.\PhysicalDriveN` open (the surviving `open_path`/`DirectDevice` machinery, cr_transport.cpp:292-317/:258-290 — a RE-CALLER of proven code, not a new implementation; A §8.8) | Header contract rewrite: "driver REQUIRED" language → privilege language; failure = target error with GLE |
| 4 | `RdaTargetDevice::read_at` → `aligned_read_at(..., rda_scaled=true, ...)` (cr_transport.cpp:333-346) | `DirectDevice::read_at` → `aligned_read_at(..., rda_scaled=false→flag deleted, ...)` (cr_transport.cpp:266-279) | **F11 flip** (§4); heal-on-GLE-6 identical |
| 5 | `open_disk_rw` / `open_disk_apply_rw` → `open_rw_device` grammar RW open, GENERIC_READ\|WRITE share R\|W\|D (cr_transport.cpp:948-983) | Direct RW open `\\.\PhysicalDrive%u`, GENERIC_READ\|GENERIC_WRITE. **Share decision**: keep R\|W\|D on the *disk* handle (the historical direct tail used R\|W, e58bde10~1:893-897 — see A §8.5; disk handles do not carry mounted-volume share constraints; the og's own disk-path writability probe used share 3, D §2.1). `via_rda_` flag dies. | zero-size refuse; `RwDevice` = `apply::IApplyDevice` IS-A `repair::IWriteDevice` (one class, both roles); write staging rule goes LIVE (§6) |
| 6 | `RwDevice::read_at` (M-009: write path rides the same 64 KiB envelope) (cr_transport.cpp:706-723) | unchanged shape, `via_rda_` false-branch becomes the only branch | envelope semantics identical on both legs (already unified, test_directalign.cpp:104) |
| 7 | `RwDevice::write_at`/`write_impl_` — staged WriteFile, `put == len \|\| put == len*512` on RDA (cr_transport.cpp:725-741, :838-877) | direct-only: buffer staging via `_aligned_malloc(len, ssec)` when caller buffer not sector-aligned (cr_transport.cpp:843-849 — currently DEAD code, the replacement reactivates it; A §8.6), success = `put == len` exactly (cr_transport.cpp:874-875) | `ERR_NO_MEMORY` on staging alloc failure; `write_gle_` recorded for the heal decision; heal-and-retry-once on GLE 6 |
| 8 | S-029 chain `delete_drive_layout`/`set_drive_layout_mbr`/`set_partition_info_ex`/`update_properties`/`settle_refresh`/`legacy_os_path`/`generate_disk_signature` (cr_transport.cpp:752-830) — issued through the RDA bound handle's pass-through (cr_transport.h:112-120) | **Issue the identical IOCTLs (0x7C100 / 0x7C010 / 0x7C058 0x18-byte record / 0x70140) on the direct RW handle.** `refresh_disk_properties` (cr_transport.cpp:985-1010) is the in-tree proof that this IOCTL class works on a direct `\\.\PhysicalDriveN` handle; the driver's pass-through added exactly one IRP build + wait per IOCTL (C §5) | error convention: any DeviceIoControl failure → `ERR_WRITE_FAILED` (hard device error → apply rolls back, cr_transport.cpp:746-751); `Sleep(1000)` settle cadence; `legacy_os_path()` constant false (Win7+ envelope) |
| 9 | S-010 `lock_volume()` quiesce: per mounted volume on `disk_` — CreateFileW `\\.\X:` GENERIC_READ\|WRITE share R\|W → `FSCTL_LOCK_VOLUME` → capture GUID (`GetVolumeNameForVolumeMountPointW`) → `DeleteVolumeMountPointW` → `FlushFileBuffers` → `Sleep(2000)` → `FSCTL_IS_VOLUME_MOUNTED` → `FSCTL_DISMOUNT_VOLUME` → `IOCTL_VOLUME_OFFLINE` (terminal; failure = Warn seam, never fatal); handles held for device lifetime (cr_transport.cpp:633-698) | **Unchanged — already 100% direct volume handles, transport-independent** (A §2.5). Becomes MORE important: with direct disk writes it is the only quiesce (the driver never wrote through the volume stack either, but the offline step closes the third-party-handle window) | busy volume → `ERR_VOLUME_LOCK_FAILED`, zero writes, never race; idempotent via `locked_`; M-030 restore chain in `release_locked_volumes_()` (ONLINE + mount-point restore + settle, cr_transport.cpp:896-921); destructor runs the restore |
| 10 | `query_driver_status(svc, out)` (cr_transport.cpp:392-481) | **Delete** (§2 disposition). Privilege probe `is_elevated()` (src/win/cr_elevate.cpp:12-21) is the surviving honest precondition surface | the query-never-fails-on-condition property dies with the thing being queried |
| 11 | `open_physical_drive(disk, err)` control-path open (cr_transport.cpp:542-546) | **Unchanged** — becomes the data-path open's implementation basis (row 3); its "control-path only" classification comment is rewritten | enumeration callers keep working (cr_enumerate.cpp:148, :201) |
| 12 | `refresh_disk_properties(disk)` (cr_transport.cpp:985-1010) | **Unchanged** — already direct (RW open on `\\.\PhysicalDriveN`, 3× UPDATE_PROPERTIES + settle) | best-effort contract: refusal reports its code, never fails the apply (cr_transport.h:102-110) |
| 13 | `volume_extent(letter,…)` / `volume_extents(letter,out)` / `destination_volume_letter(path,letter)` (cr_transport.cpp:548-572, :1019-1082) | **Unchanged** — driver-independent by classification (DRIVER-REQUIRED.md §1 row b) | N-013: first-extent vs all-extents (≤8 cap, ERROR_MORE_DATA tolerated); access-mask-0 metadata open; reparse resolution via `GetFinalPathNameByHandleW`; network refusal |
| 14 | `image_file_io()` — WinImageFileIo (src/win/cr_image_win.cpp:20-116) | **Unchanged** (S-030 driver-independent) | position-independent OVERLAPPED reads behind a locked handle table |
| 15 | `rda::compute_pid_token` / `control_device_path` (cr_transport.cpp:372-386) | **Delete** (wire-compat artifact; explicitly "NOT a security boundary", C §6.2) | none |
| 16 | `disconnect::gle_is_disconnect` N-125 classification (cr_transport.h:154-183) | **Unchanged** — pure numeric predicate, consumed in `overlapped_read` (cr_transport.cpp:68/:74) | precision: 21/55/1110/1112/1619 → `ERR_DEVICE_DISCONNECTED`; 1117/23/87 stay `ERR_READ_FAILED` (pinned test_transport_logic.cpp:65-88) |

Layering invariant: the pure-core stack above the transport (SliceShim → RetryingDevice →
CachedReader, cr_scan.cpp:153-200) **does not change at all** — engines never know the
transport (cr_device.h:8-14; B §1).

---

## §2 The error contract — and the disposition of every driver-era surface

### 2.1 The GLE → ERR_* mapping that must survive verbatim

Consolidated from cr_transport.cpp (re-verified at HEAD) and (A §2.15):

| Site / Win32 condition | ERR_* today | Direct-layer requirement |
|---|---|---|
| any open: CreateFileW fails (device absent → 2/3; not elevated → 5; busy → 32) | `ERR_OPEN_FAILED` (cr_transport.cpp:491-494, :956-959) | identical; GLE still surfaced through the open-failure prints + `is_elevated()` guidance selection (rrecov.cpp:611-615) |
| any open: probe_geometry total_bytes == 0 | `ERR_OPEN_FAILED` (:497-501, :961-966) | identical |
| ReadFile/GetOverlappedResult fails, GLE ∈ {21,55,1110,1112,1619} | `ERR_DEVICE_DISCONNECTED` (:68, :74) | identical — direct handles surface the same GLE family from the storage stack |
| read fails, any other GLE (1117/23/87/…) | `ERR_READ_FAILED` + gle_out (:68, :74) | identical — **verbatim propagation, no masking, no zero-fill at the transport** (C §8.1) |
| read count ≠ len | `ERR_READ_FAILED` (:77-81) | identical with the F11 tolerance deleted (§4.3) |
| envelope out of bounds / bounce > 1 MiB+64 KiB / take≠len | `ERR_RANGE_OUT_OF_BOUNDS` / `ERR_BAD_PARAMETER` / `ERR_READ_FAILED` (:198-199, :205-206, :215-216) | identical |
| read/write fails with GLE 6 after a failure | one heal + retry, same code if still failing (:273-277, :340-344, :716-721, :733-739) | identical (reopen = fresh CreateFileW same path/access/share) |
| write fails / count mismatch | `ERR_WRITE_FAILED` with `write_gle_` recorded (:854-875) | identical |
| staging `_aligned_malloc` fails | `ERR_NO_MEMORY` (:844-846) | identical (goes live, §6) |
| `lock_volume` volume open or FSCTL_LOCK_VOLUME fails | `ERR_VOLUME_LOCK_FAILED` (:647-654) | identical — clean refusal, zero writes |
| S-029 DeviceIoControl failures | `ERR_WRITE_FAILED` (:755-757, :778-780, :795-797, :802-804) | identical, on the direct handle |
| `refresh_disk_properties` 3× failure | `ERR_WRITE_FAILED` (open: `ERR_OPEN_FAILED`) (:997-1009) | identical |
| volume_extent(s)/destination_volume_letter any failure | false (boolean surface) (:554-571, :1027-1045, :1049-1081) | identical |

### 2.2 THE DECISION: the ERR_DRIVER_* family, driver-status, require_driver, exit 4, the `driver` verb, cr_ui_driver_status

The mandate's rule (session-driver-replacement-prompt.md:131): a driver-related surface is
removed **by understanding what it was consumed for** and either providing the equivalent
through the new layer or removing the consumer's dependency because it was only ever a
driver concern. Applying that, surface by surface — each with its consumers (verified):

**(1) `ERR_DRIVER_ABSENT/NOT_STARTED/UNAUTHORIZED/VERSION_MISMATCH/ERRORED` (0x500B0001-05,
cr_error.h:112-120; err_text cr_error.cpp:94-105).**
*Consumed for*: fail-fast refusal when the data-path *prerequisite* was unmet — the only
member of that family with a surviving, driver-independent meaning is **UNAUTHORIZED**
(the elevation refusal; GLE 5 on the control device, cr_driver_status.h:96-98).
*Consumers*: cr_ui.h:314-331 (open contracts), ABI-CONTRACT.md:94-98 (normative error
table) + §7e, test_cr_ui.cpp:3998/:4044/:4050 (0x500B0001 return + doc pins),
checks.yml:356 (stderr match), CORE-API-CURRENT.md:446-488, hosts.
*Disposition*: **REMOVE the entire family and its five values.** Replace with **one** new
append-only code for the surviving semantics — the privilege refusal — proposed
`ERR_ELEVATION_REQUIRED` at a **fresh value** (recommend a new family slot, e.g.
`0x500C0001`; do NOT reuse 0x500B0001-05: hosts compiled against the old header would
silently decode a re-used value as its driver-era meaning, and the storage-family
append-only discipline is pinned by test_transport_logic.cpp:86-87). Five codes die
because absent/not-started/version-mismatch/errored described *driver service states*
that no longer exist; the direct layer's only precondition states are *elevated* /
 *not elevated* (probe: `is_elevated()`, cr_elevate.cpp:12-21 — FALSE for standard users
AND non-elevated admins, which is exactly the token class raw `\\.\PhysicalDriveN` access
needs, A §6). Target-level failures (busy/offline/protected/absent device) are already
honestly classified as `ERR_OPEN_FAILED` + GLE (§2.1 row 1) — they were never driver
refusals and must not become "elevation" refusals.

**(2) `require_driver()` + exit code 4 (rrecov.cpp:525-574; exit-code comment :66-69).**
*Consumed for*: the verb-level gate that runs BEFORE any other output — "the refusal is
the whole response" (DRIVER-REQUIRED.md:106-131) — in scan (:1338), recover (:2454),
archive-list (:2271), image (:2887), bench (:2982), apply (:3218), fix (:3608).
*Consumers*: every data-path verb; checks.yml:322-366 (the driverless refusal probes);
DRIVER-REQUIRED.md §4; usage text (rrecov.cpp:4021-4024).
*Disposition*: **KEEP the flow and KEEP exit code 4; rename and re-semantic the gate to
`require_privilege()`** — probe `is_elevated()`; when false, print the honest two-line
refusal (`administrator privilege is required for raw disk access …` + remediation +
`[<class> 0x…] - no operation was attempted`) and return 4. Exit 4 is a *number* whose
documented meaning is "the data-path prerequisite is not met" — preserving it preserves
the CLI contract for every script consumer while removing all driver language. Per the
mandate's JIT-elevation rule (§9 below), the gate first attempts the standard elevation
flow when interactive; the plain refusal stands when elevation is declined/unavailable.
`--image` targets keep skipping the gate (argv_has_image_target, rrecov.cpp:579-584).

**(3) The `driver` CLI verb (rrecov.cpp:3885-3942; dispatch :4067-4068; usage :48,
:3995-3998).** *Consumed for*: the diagnostic that REPORTS the prerequisite's facets.
*Consumers*: checks.yml:346-351 (exit-4/absent probe), test-benchmark.yml:264-278
(api-driver-status evidence), docs.
*Disposition*: **REMOVE the verb entirely.** The driver's six facets have no referent;
the direct layer's prerequisite is a single boolean that belongs in the refusal text
itself. No replacement verb: a `rrecov doctor` is new surface the mandate does not ask
for. The checks.yml probe becomes the privilege-refusal probe (exit 4 + exact stderr on
an unelevated run).

**(4) `cr_ui_driver_status` ABI export + `cr_ui_driver_status_t` +
`CR_UI_DRIVER_STATE_*` (cr_ui.h:343-396; impl cr_ui.cpp:1679-1722; Linux stub
:1758-1766).** *Consumed for*: the host's startup/pre-action gate (DRIVER-REQUIRED.md
§6.1) and the advisory parity with the open calls.
*Consumers*: test_cr_ui.cpp:3990-4002 (driver-attached/driverless branches),
tests/ui_abi_check.c:1161-1170 (stub → UNSUPPORTED / NULL → BAD_PARAMETER),
test-benchmark.yml `--driver-status` API probe, ABI-CONTRACT.md, the out-of-repo UI
(dependency tracked in DRIVER-REQUIRED.md §6.3 — which itself is deleted).
*Disposition*: **REMOVE the export, struct, and enum; bump the cr_ui ABI version** (the
removal is the one sanctioned breaking change; "no ABI/CLI surface change beyond the
driver removal itself"). A permanent `Ready` stub is explicitly banned by the mandate
(prompt:131). The **equivalent through the new layer** already exists: the open calls
return `ERR_ELEVATION_REQUIRED` (and `cr_ui_last_transport_error` mirrors it — §2.2(6)),
and the platform probe `cr::win::is_elevated()` is already public (cr_elevate.h:5-8).
Recommendation: do NOT add a new `cr_ui_is_elevated` export in this session — hosts
consume the refusal code from the open; if the UI repo later needs a pre-check, an
additive export is a deliberate follow-up. Flagged as open question OQ-3 (§11).

**(5) `query_driver_status` + `DriverStatus`/`DriverState`/`driver_state`/
`driver_state_error`/`driver_state_name` + `kDefaultDriverService` +
`kDriverWireMajorRequired` (cr_transport.cpp:392-481; cr_driver_status.h, whole file).**
*Consumers*: require_driver, cmd_driver, open_disk_view's TOCTOU layer (rrecov.cpp:592-602),
open_disk_impl/open_volume_impl/open_volume_with_key_impl/image-create impl
(cr_ui.cpp:1432-1443, :1495-1506, :1547+, :1782+), driver_status_impl, test_driver_status.cpp
(13 tests), win-stub SCM trio (tests/win-stub/windows.h:579-598).
*Disposition*: **DELETE all of it** (header included) with the driver. The open-path
TOCTOU layers collapse into the privilege gate: each `open_*_impl` returns
`ERR_ELEVATION_REQUIRED` when `!is_elevated()`, else the target error from the direct
open. tests/unit/test_driver_status.cpp is deleted; the SCM stubs go if no other
translation unit consumes them (verify at implementation — the BitLocker/health paths
were checked and do not).

**(6) `cr_ui_last_transport_error` (M-030 advisory; cr_ui.h:335-341; ABI-CONTRACT §7e).**
*Consumed for*: the transport-open diagnostic — today "the ONE leg there is"
(ABI-CONTRACT.md:875-897).
*Disposition*: **KEEP the export and advisory semantics unchanged** (it is not
driver-named and not driver-specific). Its documented value set changes: 0 on success /
`ERR_ELEVATION_REQUIRED` when the privilege gate refused / any other `cr_error` when the
TARGET refused. ABI-CONTRACT §7e and its doc-truth pin (test_cr_ui.cpp:4036-4060) are
rewritten in the same commit. Note cr_ui.cpp:1874-1880's own comment is already stale vs
§7e (A §10.8) — fix in passing.

**(7) The transport label `"rda"` (cr_ui.h:439-444; CLI `[transport] RDA driver path`
rrecov.cpp:604; run-ours.ps1:142-149 asserts it when the service runs).**
*Disposition*: the value becomes **`"direct"`** (the historical label of the direct leg,
and honest for the new architecture; `"image"` unchanged). Consumers updated in lockstep:
cr_ui.h contract text, test_cr_ui.cpp:4022-4034 (the pin currently *forbids* advertising
"direct" — a driver-era pin that must be inverted), run-ours.ps1's GAP-BENCH-5 assertion
(transport line must now be the direct path), CORE-API-CURRENT.md:217.

**Exit-code surface after the swap** (rrecov.cpp:55-69 verified): scan 0/1/3/5, list 1,
health 3, recover 2, apply/fix 2, bench 2, image 2/3 — **all byte-identical**; 4 =
privilege prerequisite not met (was: driver prerequisite). The ONE deliberate stderr
surface remains the prerequisite-refusal path (CORE-API-CURRENT.md:345).

---

## §3 Threading / lifetime / cancellation guarantees to preserve

1. **Per-handle single-flight** is the ABI contract (cr_transport.cpp:188-191); the
   engine path arrives serialized under the CachedReader mutex, but carve/remnant/candidate
   workers (≤8, B §3) can hit one shared handle concurrently — the `thread_local` bounce
   buffer in `aligned_read_at` (cr_transport.cpp:207-210) tolerates that. Both stay.
2. **Heal serialization**: `heal_mu_` per device; the winning reopen swaps the handle,
   losers' retries ride the fresh handle (cr_transport.cpp:221-253). One heal attempt per
   failing call; geometry is NOT re-probed (N-046 refresh lives on PartitionSlice,
   cr_slice.h:58-72). Stays.
3. **Cancellation**: engines poll `CancelToken::checkpoint()`; transports do NOT poll
   (F-B-16 scope, cr_device.h:146-149); the pool watcher aborts queued work within one
   50 ms tick (N-017, cr_parallel.h:106-127; pinned test_parallel.cpp:273). The driver
   invested F4/S1/S2 in cancellable forward-waits (C §3.2, §8.7) — a synchronous
   OVERLAPPED `ReadFile` on a raw handle gets the same property natively
   (`CancelIoEx`/handle close aborts a stuck IRP); the CTRL_BREAK hard-kill probe
   (run-behavior.ps1 probe 2) remains the behavioral measurement.
4. **Lifetime**: every device owns exactly its HANDLE; destructors CloseHandle;
   `RwDevice`'s destructor additionally runs `release_locked_volumes_()` so held volume
   locks release and the M-030 restore chain runs (cr_transport.cpp:603-610, :896-921).
   Devices are constructed per verb and live for the verb's duration; the ReadPipe
   outlives the writer (B §6 threading note).
5. **No transport-level cache, no `FILE_FLAG_NO_BUFFERING`** on core handles
   (cr_transport.cpp:370-372 comment; D §2.1 — the og's data path is plain buffered
   synchronous I/O; the only NO_BUFFERING|OVERLAPPED handle anywhere is DRWUI's
   IOCTL-only disk-info handle). The direct replacement keeps the same posture: the
   64 KiB envelope, not buffering flags, is the alignment mechanism (§4).
6. **Position-independent reads**: one OVERLAPPED per call with the absolute byte offset
   split into `ov.Offset/OffsetHigh` (cr_transport.cpp:56-58) — the shared synchronous
   HANDLE is safe under single-flight. Stays; no `SetFilePointer` (the og used
   SetFilePointer+ReadFile, D §2.2 — our OVERLAPPED form is equivalent and thread-safer).

---

## §4 Geometry / sector-size / alignment rules to preserve

1. **The probe ladder** (cr_transport.cpp:96-152): `IOCTL_DISK_GET_LENGTH_INFO` (exact,
   works on disk AND volume handles) → `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX` (BytesPerSector
   into logical AND physical) → `IOCTL_STORAGE_QUERY_PROPERTY` StorageAccessAlignment
   reading **BytesPerLogicalSector@+0x10 / BytesPerPhysicalSector@+0x14** — the 183dfcd3
   fix; the pre-fix code read the CACHE-LINE fields and every VHD reported logical 0 —
   sanity-gated [256, 65536]; `total_bytes` = exact when available else `dg.DiskSize`;
   **fallback** when geometry IOCTL is not forwarded (some volume stacks): 512/512 +
   exact length (cr_transport.cpp:140-148). All of this is already handle-generic and
   survives the driver unchanged; the fallback matters if any path ever opens a volume
   handle (we keep whole-disk opens, so it is defensive).
2. **N-206 CEIL**: `sector_count = (total_bytes + ss - 1) / ss` (cr_transport.cpp:134-139),
   pinned by `Geometry::valid()` (cr_device.h:49-60) and test_image.cpp:163. The final
   partial sector of an odd-sized source is ADDRESSABLE — through the driver it read
   back zero-padded; **a direct handle read past EOF fails or short-reads**. The direct
   layer MUST reproduce the padded-tail semantics itself: when a request's envelope
   extends past `total_bytes` but stays inside the ceil window (`sector_count × ss`),
   issue the read clamped to `total_bytes` and **zero-fill the tail bytes**
   `[total_bytes, sector_count×ss)` in the bounce path — exactly the semantics
   `FlatImageDevice` already implements for images (cr_image.cpp:156-185, "the
   driver-padded semantics the direct replacement must also honor", A §3). A short read
   (`got < requested`, buffer zeroed tail) must be treated as success within the window.
   Test needed: odd-sized raw device image of the TruncatedFlatImageCeilsSectorCount
   class, plus the lost-discovery tail sweep (cr_candidates.cpp:45-49 relies on the ceil
   bound, B §6).
3. **The 64 KiB envelope** (cr_transport.cpp:154-219; cr_directalign.h:48-73):
   offset-down/length-up to 64 KiB multiples, clamped at the device end (the ragged-tail
   exception); zero-copy fast path for aligned requests; bounce capped at 1 MiB + 64 KiB
   (the writer chunks at 1 MiB). **Already unified on BOTH transports** (M-009;
   test_directalign.cpp:104 exhaustive property) — it stays exactly as is. Its two
   raisons d'être are direct-path facts: (a) sector rejection — the Windows disk stack
   rejects non-sector-multiple ReadFile IRPs (run 40073093997: every size%512≠0 file was
   zero-filled "salvaged" pre-shim; cr_transport.cpp:160-166); 64 KiB is a multiple of
   every real logical sector size; (b) GAP-S6-13 — 4-KiB-but-not-64-KiB lengths take the
   ~3.5× slow path (~830 vs ~2900 MB/s on mid-range NVMe; runner ratio 1.03× recorded
   honestly, run 40227375329). The og satisfies the property by construction: every chunk
   it issues is a 64 KiB multiple (4 MiB device / 7 MiB volume / 2 MiB MFT / 64 KiB boot;
   D §6, cr_transport.cpp:167-177).
4. **The F11 flip — exactly where the unscale lives and what dies**:
   - READ: `overlapped_read(h, off, buf, len, bool rda_scaled, gle)` accepts
     `got == len * rda::kLegacyInformationScale` (cr_transport.cpp:53-82, the acceptance
     at :79-80). Callers: RdaTargetDevice passes true (:339), RwDevice passes
     `via_rda_` (:715), DirectDevice passes false (:272).
   - WRITE: `write_impl_` accepts `put == len*512` only when `via_rda_`
     (cr_transport.cpp:872-875).
   - **What flips**: the `rda_scaled`/`via_rda_` parameters are deleted; direct handles
     return true byte counts (proven by the historical DirectDevice, e58bde10~1:250-260,
     and by run 40227375329's direct leg where `want = chunk`, no unscale). The
     `rda::kLegacyInformationScale` constant, the whole `rda` namespace, and
     `include/cr/win/cr_rda.h` are deleted. run-alignbench.ps1's `rdaScaled` parameter
     and RDA round-trip leg (:14, :38, :104, :128, :156, :193, :330-340) are deleted —
     the two-pass checksum discipline on the remaining direct legs stays.
5. **512e/4Kn**: logical/physical distinction via the alignment query (row 1);
   `is_4kn()` feeds bench info and fixup/alignment math; the N-004 backtrack step is
   `max(0x400, logical_sector)` so shifted re-reads stay sector-aligned on 4Kn
   (cr_retry.cpp:83-99; cr_device.h:107-115); slice starts must be parent-sector-aligned
   with ragged ends allowed (cr_slice.h:26-29); DecryptingDevice tweaks use the absolute
   inner offset / logical_sector. All unchanged. The driver had NO 4Kn translation on the
   grammar path (C §0 D2 — byte transparency; Information=Length×512 on all media), so
   nothing is lost; direct handles are byte-transparent the same way, with stack-side
   alignment enforcement (which the envelope satisfies).
6. **Direct-write buffer alignment**: the `!via_rda_` staging rule
   (cr_transport.cpp:843-849) — `_aligned_malloc(len, ssec)` + memcpy when the caller's
   buffer is not sector-aligned; `ERR_NO_MEMORY` on failure — is the proven direct-write
   rule (A §8.6) and becomes the only branch.

---

## §5 Edge states — driver behavior → required direct behavior → test

| Edge state | Driver behavior (C §10; A) | Required direct behavior | Test needed |
|---|---|---|---|
| Device absent (disk # has no device) | grammar CREATE fails, `STATUS_OBJECT_NAME_NOT_FOUND` → GLE 2 → `ERR_OPEN_FAILED` | `CreateFileW(\\.\PhysicalDriveN)` fails GLE 2/3 → `ERR_OPEN_FAILED` + GLE print; LDM member simply does not resolve (rrecov.cpp:1757-1759 continues) | scan/fix on absent disk N+1: exit/open contract; LDM resolver skip |
| Disconnect mid-read | forward IRP status verbatim; no driver timeout; cancel escapable | GLE ∈ N-125 set → `ERR_DEVICE_DISCONNECTED` (exit 5 mid-scan, session checkpoint resume); GLE 6 → heal-and-retry-once; verbatim otherwise, no masking | yank VHD mid-scan on the runner: exit 5, resume-after-reattach equivalence (run-behavior.ps1 probe 3 shape) |
| Volume mounted (scan) | driver grammar opened the disk; reads coexist with the OS (no exclusivity) | share R\|W\|D disk open coexists with mounted volumes — same as today's `open_physical_drive`; NO lock during scan | scan on the running OS disk's VHDX sibling; expected: works, shares |
| Volume unmounted / lost | volume grammar top-of-stack (unused); volume views = extent→disk open→slice | unchanged shape: `volume_extent` needs the letter; unlettered volumes are whole-disk-scan targets (S-051) — no volume-handle data path exists to port | `scan --disk` on a disk with no letters (table-driven targets) |
| Volume in use (write path) | no driver arbitration; the core's own S-010 refuses | `FSCTL_LOCK_VOLUME` failure → `ERR_VOLUME_LOCK_FAILED`, ZERO writes, clean refusal — unchanged chain (cr_transport.cpp:647-654) | fix --apply against a busy volume: refusal, no bytes written |
| Bad sectors / media errors | verbatim NTSTATUS→GLE, Information=0; no remap/zero-fill/retry in the driver | GLE 1117/23 → `ERR_READ_FAILED` → RetryingDevice ×3 → N-004 backtrack → bisect → ReadErrorMap + Skip zero-fill; `bytes_unreadable` truth (D-DRV-6 read-truth gate) | seeded bad-sector VHD: error-map coverage + zero-fill + verdict fields |
| Unaligned reads | byte-transparent forwarding; stack rejects (87) | envelope rounds to 64 KiB; bounce for unaligned caller shapes; `ERR_BAD_PARAMETER` > 1 MiB+64 KiB | test_directalign class + on-hardware alignment bench (run-alignbench leg 1) |
| 512 / 512e / 4Kn | probed at open, cached; grammar IO byte-transparent; ×512 on all media | geometry ladder (§4.1); envelope subsumes; true byte counts (F11 flip) | bench info on 4Kn VHD (logical 4096); 512e physical-4096 report; slice create on VHD (the 183dfcd3 regression class) |
| Odd-sized media (final partial sector) | driver zero-padded the ceil-window tail | clamp-to-EOF + zero-fill in the tail window (§4.2) | odd-sized raw/VHD image: ceil addressability, TruncatedFlatImageCeilsSectorCount analogue |
| Win7 SP1 → Win11 / Server 2025 | driver PE contract 6.1/NATIVE; Win7 allowlist on imports | **no new user-mode imports needed** — every API the direct path uses is already imported for health/enumeration/refresh/quiesce (CreateFileW/ReadFile/WriteFile/GetOverlappedResult/DeviceIoControl/CloseHandle + the IOCTLs; the SCM trio is DELETED). The Win7 allowlist gate (tools/verify_pe_um.py, ref_imports_um.json, ci 3b203fe1/cc6312ca) must stay green | CI Win7 PE gate (already in checks.yml) + osgate matrix (installer/README.md:39) |
| WinPE | driver required via regdrv-rda.reg (DRIVER-REQUIRED.md:133-145) | **no driver needed in PE** — the og's PE edition runs the same product fully direct (D §7.1); PE runs as SYSTEM so `is_elevated()` is TRUE (no UAC); PE registration files/scripts deleted | PE smoke: list+scan on WinPE media (documented; runner PE leg if available) |
| Elevated / not | DACL on `\\.\RDA` → GLE 5 → Unauthorized → exit 4 | raw-device ACL → GLE 5 at open; the verb-level `require_privilege()` gate refuses first (exit 4, `ERR_ELEVATION_REQUIRED`), offering the standard elevation flow; refusal text names what needs elevation and what does not (sessions/tree/preview/--image keep working) | checks.yml lane: unelevated run exits 4 with exact stderr, NOTHING past the gate; elevated run proceeds |
| Write-protected media | n/a through driver (stack status) | GLE 19 (`ERROR_WRITE_PROTECT`) on the write path → `ERR_WRITE_FAILED` (write_gle_ recorded); the og's `IOCTL_DISK_IS_WRITABLE` short-circuit (D §5) is og parity we do NOT currently implement — do not add scope | read-only VHD: fix/apply write failure path |
| Wrong-PID / grammar-abuse classes | token mismatch → 87 at CREATE (C §1.3) | no equivalent — the class dies with the grammar (the token was never a security boundary) | none (removed surface) |
| Disk # out of bounds | DISK ≥ MaxDisks (200) → 87 | enumeration space stays 200 (`ENUMERATE_DISK_BOUND`, cr_winenv.h:45) — but as the ENUMERATION contract, re-pinned without the driver-grammar justification (test_transport_logic.cpp:56-63) | enumeration bound test re-based |

---

## §6 The write paths — preservation requirements

**S-010 quiesce (repair/apply, `lock_volume`)** — unchanged code, now load-bearing:
per MOUNTED volume with a letter on the target disk, in order: open `\\.\X:`
GENERIC_READ|WRITE share R|W → `FSCTL_LOCK_VOLUME` (busy = `ERR_VOLUME_LOCK_FAILED`,
zero writes, never race) → capture the volume GUID BEFORE the mount point goes away →
`DeleteVolumeMountPointW` → `FlushFileBuffers` → `Sleep(2000)` settle →
`FSCTL_IS_VOLUME_MOUNTED` (verify, ignored) → `FSCTL_DISMOUNT_VOLUME` →
`IOCTL_VOLUME_OFFLINE` (0x56c00c — the M-030 terminal step; failure = Warn through the
log seam, degraded to dismount+lock, never fatal). Handles held for the device lifetime;
restore chain (destructor/`unlock_volume`): reopen by GUID → `IOCTL_VOLUME_ONLINE`
(0x56c008) → `SetVolumeMountPointW` (saved letter) → `Sleep(1000)`; no-GUID degrades to
handle-close only (cr_transport.cpp:633-698, :896-921; constants pinned
cr_winenv.h:47-66 + test_transport_logic.cpp:40-54; corpus
RecoveryModule_decompiled.c:60401-60447, FUN_1800459b0 :34465-34502).
**Requirement**: the chain is already direct — it must be proven on the direct write path
(a driver-era write went through the disk stack bound handle; a direct write goes through
the same `\\.\PhysicalDriveN` RW handle — with the volumes offlined, third-party I/O
windows are closed the same way). Tests: quiesce matrix per §5 "volume in use" row +
restore verification (letter returns after fix).

**S-029 apply chain** — identical IOCTLs, new issuer: `IOCTL_DISK_DELETE_DRIVE_LAYOUT`
(0x7C100) → `IOCTL_DISK_SET_DRIVE_LAYOUT` (0x7C010, 0xA8-byte DLI: count+signature, entry
payload zero except RewriteBoot on the first — the sector-0 write carried the real bytes)
→ the 0x18-byte record via 0x7C058 {partition number, 16-byte identity, boot flag} →
`IOCTL_DISK_UPDATE_PROPERTIES` (0x70140) with the 3× `Sleep(1000)` settle tail
(cr_transport.cpp:752-808). The driver passed non-session IOCTLs through to the bound
target with verbatim status/Information (C §5); `refresh_disk_properties`
(cr_transport.cpp:985-1010) proves the identical class on a direct handle. All four are
METHOD_BUFFERED (or no-buffer) — nothing the driver's proxy added is semantically needed.
Error convention unchanged: any failure → `ERR_WRITE_FAILED` → apply rolls back.

**Staging**: the direct-write alignment rule (§4.6) goes live — sector-aligned staging
buffer for unaligned caller buffers; `put == len` exactly.

**Consent gates (unchanged, non-negotiable)**: repair is dry-run without `--apply`;
analyze is read-only; execute = coalesce-to-sector-span → snapshot span into CRUNDO01
BEFORE writing → write → read-back memcmp verify → reverse-order rollback on any failure;
apply: revalidate sector 0 (`ERR_APPLY_STALE`) → consent callback
(`ERR_APPLY_CONSENT_DENIED`, zero writes/locks) → lock → revalidate → optional legacy
layout chain → snapshot→write→verify transaction → refresh tail
(`ERR_APPLY_REFRESH_FAILED` = written+verified, table stays); undo replays CRUNDO01
byte-exact (B §4). No raw write outside these consented paths; source-write prohibition
and destination-overlap refusal (R7.2/F12, N-013) untouched.

---

## §7 The performance target table

Sources: (B §5.2/§5.3) with run IDs; (C §7). Measurement methods: the bench verb
(rrecov.cpp:3037-3178: `info`/`read`/`seq`/`rand`/`open`, `--json`), run-alignbench.ps1
legs (alignment demo, size×QD sweep, API latency), test-benchmark.yml 07-performance,
run-ours.ps1 scan timings + sampling, bench/scenarios matrix, run-behavior.ps1 probes.
**Every target is "meet or beat on real Windows hardware"; where the baseline was at
storage-stack parity, "match" is the target** (mandate §3).

| # | Consumer / metric | Baseline (run ID) | Target | Measured by |
|---|---|---|---|---|
| 1 | Sequential 64 KiB read (image create, lost discovery, envelope shape) | driver 532.9 MiB/s; og-equivalent 203.98-208.6 core-path (39807929475; 40315255368; 40227375329) | ≥ 532.9 MiB/s raw-transport class; core-path ≥ 203.98 | `bench --mode seq --block-kb 64`; image --create wall+SHA; run-alignbench sweep |
| 2 | Sequential 1 MiB (recovery writer, carve windows) | driver 500.6 MiB/s @1 MiB; core-pipe 241.64 @256 KiB; RDA 160.5 @1 MiB (39807929475; 40315255368; 40227375329) | ≥ 500.6 @1 MiB transport class; carve-class ≥ 160.5; pipe ≥ 241.64 | `bench --mode seq --block-kb 1024` / `--block-kb 256`; deepcarve wall ≤ 90.06 s on the smoke volume (40073093997) |
| 3 | Sequential 4 MiB (remnant sweep, ext4 GDT) | 202.8-242.4 MiB/s @4 MiB QD1 (40227375329) | ≥ 242.4 | `bench --mode seq --block-kb 4096`; deep-mode scan timing |
| 4 | Random 4 KiB (MFT-record class, ZFS blocks) | 7,932 IOPS / 126.1 µs mean (39807929475 R2) | ≥ 7,932 IOPS; ≤ 126.1 µs — **expected to EXCEED**: the driver's own documented −2.9% was cancel-safe+DACL machinery direct access sheds (C §7, §9) | `bench --mode rand --block-kb 4` |
| 5 | Device open (cold) / open churn | 8,217 ns / 8,532 ns mean (39807929475 R2) | ≤ both — direct open is one object-manager open vs the grammar walk + per-layer ObQueryNameString + geometry probe (C §9) | `bench --mode open` |
| 6 | Scan read pipe (256 KiB cache block, all engines) | 241.64 MiB/s, cpu 562.5 ms (40315255368) | ≥ 241.64 at ≤ baseline CPU; note runner direct-vs-driver @256 KiB was 151.5 vs 169.3 (40227375329) — the pipe shape must recover that gap | run-ours.ps1 quick/deep/deepcarve timings + process sampling |
| 7 | Scan wall-time per scenario/mode | quick 0.5 s / deep 2.0-8.0 s / deepcarve 42-90 s (40073093997) | ≤ same per cell | test-benchmark scenario matrix (byte-for-byte results AND timings) |
| 8 | Recovery throughput + quality | exFAT 99.94/99.16; runs 40073093997/40079065019 (B §5.2f) | identical recall/precision AND ≥ writer read throughput (≥ 224.4 MiB/s @1 MiB, 40227375329) | scenario matrix recover + report diff |
| 9 | Alignment ratio (envelope proof) | 1.03× virtualized / ~3.5× real NVMe (40227375329; GAP-S6-13) | ≥ 1.0 on virtualized; the 3.5× property on real NVMe (weekly dispatch) | run-alignbench leg 1 (128K vs 132K, NO_BUFFERING QD1) |
| 10 | Data correctness | 64/64 SHA-256 ranges byte-identical (39807929475) | byte-for-byte identical results vs pre-replacement baseline on the same seeded disks | test-benchmark + T16 comparison; scenario hashes |
| 11 | Cancellation responsiveness | ≤ one 50 ms watcher tick + current-item poll (N-017; test_parallel.cpp:273-330) | ≤ same | run-behavior.ps1 probe 2 (CTRL_BREAK, exit 3, session saved) |
| 12 | CPU / memory overhead | cpu_ratio og/core 1.026-1.083 at the pipe shapes (40315255368); WS sampled | ≤ baseline per consumer | process sampling in run-ours/test-benchmark |
| 13 | cr_ui cheap-op API latency | abi_version 10.7 ns / last_error 11.3 / carve_format_count 12.6 / info 166.3 ns (40228880804) | ≤ same (removal should not move these) | run-alignbench API latency leg |
| 14 | TTFR | no transcribed number yet (runs 40290109004/40290704073/40291877684/40783364141/40956286997 — in flight/stale; B §5.2i) | record baseline first, then match/beat | TtfrProbe `ttfr_ms=`/`first_files=` lines (Run-Benchmark.ps1) |

Measurement discipline (mandate §3): every number carries environment, hardware, disk,
read size, QD, pattern; every result lands in `worklog/PERFORMANCE.md` beside its
baseline; no claim without a captured measurement.

---

## §8 The removal inventory — every driver reference, enumerated

Verified by grep at HEAD (`rg -i 'rda|driver'`, path counts cited). Product surfaces
only — `worklog/` and `worklog.md` are the session's historical archive and keep their
history (the final sweep scope is src/include/tests/tools/bench/scripts/installer/
.github/docs/README).

**Source & headers (7 files)**
1. `include/cr/win/cr_rda.h` — DELETE (entire wire mirror; doc-only structs, D5/C).
2. `include/cr/win/cr_driver_status.h` — DELETE (facets/6-state classifier/mappings).
3. `src/win/cr_transport.cpp` (82 hits) — delete `rda` namespace (:372-386),
   `query_driver_status` (:392-481), `open_rda_partition`/`open_rda_volume` (:483-534),
   `RdaTargetDevice`; strip `via_rda_`/`rda_scaled`/F11 branches (:79-80, :872-873);
   rewrite the driver-era comment blocks (:154-192, :575-584, :940-946).
4. `include/cr/win/cr_transport.h` (44 hits) — delete grammar/status declarations; rewrite
   the header contract (:1-24, :42-66, :82-122).
5. `include/cr/cr_error.h:108-120` + `src/core/err/cr_error.cpp:94-105` — the 0x500B
   family + err_text lines (→ §2.2(1) replacement).
6. `src/app/rrecov.cpp` (122 hits) — `require_driver` (:525-574), `cmd_driver`
   (:3885-3942), dispatch arm (:4067-4068), usage/exit-code text (:46-69, :3995-3998,
   :4021-4024), `open_disk_view` status layer + `[transport] RDA driver path` print
   (:592-607), every `L"RDA"` service argument (scan/recover/bench/fix/apply/LDM/bench-open
   churn), bench transport label (:3045).
7. `src/ui/cr_ui.cpp` (91 hits) — `driver_status_impl` + stub (:1679-1722, :1758-1766),
   the status gates in open_disk/open_volume/open_volume_with_key/image-create impls,
   transport `"rda"` strings (:1450, :1515), the stale "surviving transport leg" comment
   (:1874-1880).
   Plus `include/cr/cr_ui.h` (driver-status block :343-396, open contracts :308-341,
   transport contract :439-444, :1602-1603) and the ABI version bump.
   Minor comment-only hits to sweep: cr_health.cpp:3, cr_candidates.cpp:1, cr_ldm.cpp:1,
   cr_ext4_engine.cpp:1, cr_mftwalk.cpp:1, cr_safewriter.cpp:2, cr_image.cpp:2,
   cr_query.cpp:1, cr_grading.cpp:1, cr_analysers.cpp:5, cr_carve/cr_registry:1 each,
   cr_offlineos.cpp:1, cr_btrfs/hfsplus/exfat:1-2 each, include/cr/query/cr_query.h:3,
   include/cr/health/cr_csmi.h:1, include/cr/win/cr_image_create.h:6,
   include/cr/win/cr_winenv.h:4 (PE-predicate note + disk-bound justification),
   src/win/CMakeLists.txt + src/ui/CMakeLists.txt (comment hits).

**Tests (19 files, 164 hits)**
8. `tests/unit/test_driver_status.cpp` — DELETE (13 tests).
9. `tests/unit/test_cr_ui.cpp:3990-4006` (driver-attached/driverless open branches),
   `:4036-4060` (ABI-CONTRACT driver-era pins), `:4062-4104` (wire-code + bench pins) —
   rewrite to the privilege-era truth.
10. `tests/unit/test_transport_logic.cpp` — re-base `EnumerateBoundMatchesDriverGrammarSpace`
    (:56-63) to the enumeration contract; decide the PE-predicate pin (:23-38, see OQ-6).
11. `tests/ui_abi_check.c:1161-1170` — driver-status probes deleted.
12. `tests/CMakeLists.txt` (2 hits — test_driver_status target), `tests/win-stub/windows.h`
    (:579-598 SCM trio + :216-229 CreateFileW family — delete the SCM stubs if orphaned),
    `test_fixtures.hpp:1`, `test_image_create_win.cpp:2`, `test_query.cpp:10`,
    `test_carve.cpp:13`, `test_lostpart.cpp:3`, `test_fat_volume.cpp:3`,
    `test_scan_truth.cpp:2`, `test_directalign.cpp:1`, `test_grading/test_image/
    test_btrfs/test_exfat:1 each` — comment/driver-era-name sweeps.

**Tools & CI (local + remote)**
13. `ci-local.sh` — N-041 dead-surface lint (:55-61, becomes moot) and the **driver-only
    architecture lint (:62-70)**: the blocklist `rda_available|win_pe_environment|
    open_harddisk_volume|prefer_rda` must be REPLACED by the inverse gate (no driver
    surface may return: `query_driver_status|cr_ui_driver_status|open_rda_partition|
    ERR_DRIVER_|kDefaultDriverService|cr_rda.h`).
14. `ci-local.ps1:54-57` — same lint, same inversion.
15. `.github/workflows/checks.yml` — lint job (:111-124), the **driverless
    refusal-contract probes** (:322-366 → privilege-refusal probes), header comments
    (:13-21, :39).
16. `.github/workflows/test-benchmark.yml` — the whole driver leg: release fetch
    (`rda.sys…rda-test-sign.cer` required assets, :170-173), Install-ReleaseDriver
    (:212-222), API driver-status probe (:264-278), teardown (:302-311), header
    comments (:1-48, :94, :134).
17. `.github/workflows/release.yml` — the `driver-tier` job (:266-396), fetch/sign steps,
    MANIFEST driver fields (:440-475, :517-534), name/header (:11-44).
18. `.github/workflows/installer.yml` — driver fetch (:114-118), test-sign (:121-137),
    header (:5-21).
19. `.github/scripts/` — `Install-ReleaseDriver.ps1` (DELETE), `fetch-release.ps1`
    (driver-repo consumer — keep only if still used for core artifacts),
    `Run-Benchmark.ps1`/`Teardown-Benchmark.ps1` (driver references),
    `api_harness.c` (driver-status probe).
20. `tools/doc_truth_gate.py` — the deleted-workflow allowlist (:62-63), the
    driver-repo citation allowlist (:82-85), the N-195 CTL-literal gate (:429-464, keyed
    on cr_rda.h — delete with the header).
21. `tools/verify_pe_um.py` (7 hits), `tools/ref_imports_um.json` (SCM/driver imports —
    verify), `tools/stamp_osversion_um.py:1`, `tools/test_installer_osgate.py:1`.

**Bench (10+ files)**
22. `bench/ours/verify-driver.ps1` — DELETE (22 hits; the driver round-trip proof).
23. `bench/ours/run-alignbench.ps1` (14 hits) — delete the RDA grammar round-trip leg +
    `rdaScaled`/`want = chunk*512` (:104, :125-131, :156, :193, :330-340).
24. `bench/ours/run-ours.ps1:142-149` — GAP-BENCH-5 transport assertion → assert the
    direct transport line.
25. `bench/ours/run-behavior.ps1` — probe 1 "nodriver" (:4-6, :31-61) is **already stale**
    (it expects the pre-e58bde10 fallback text "RDA not available … - direct path");
    rewrite to the privilege refusal or delete.
26. `bench/og/og-driver.ps1` + `run-og.ps1` (og-lane driver legs), `bench/tools/pe_info.py:1`,
    `bench/metrics/compare.py:1`/`merge_runs.py:1`, `bench/root-causes.json:1`,
    `bench/README.md:5`, `bench/baseline*.json` (transport labels in captured evidence —
    historical, keep; document).

**Installer (5 files)**
27. `installer/core-recovery.iss` — `#define DriverService "RDA"` (:53), the `driver`
    component (:93-101), the `driver\{…}` payload (:109-113), the elevated post-install
    deployer step whose failure fails the install (:194-206), the uninstall teardown
    (:217-229), header (:11). **What remains**: `PrivilegesRequired=admin` (:67 — the
    product's own elevation posture stays), the engine payload, ARP/version-key/OS-gate.
28. `installer/build-installer.ps1` — `-DriverDir` param, the 5-asset hard-failure check
    (:44-46), the payload driver/ staging (:53, :70-76).
29. `installer/run-gates.ps1` — `-ServiceName`/`-SkipDriver` params, gates 1c/1e driver
    assertions (:28-29, :55-57, :85-89, :130-147).
30. `installer/README.md` (11 hits) + `installer/osgate.ps1:47` (reword the x64-only
    rationale — it cites the driver's PE contract).

**Docs (46 files, 1154 hits) — the sweep**
31. DELETE outright: `docs/DRIVER-REQUIRED.md`, `docs/PLAN-DRIVER-ONLY.md`,
    `docs/DRIVER-CONTRACT.md`, `docs/DRIVER-RELEASE.md`, `docs/DRIVER-UTILIZATION.md`,
    `docs/REFERENCE-DRIVER-BEHAVIOR.md` (the mandate's "no driver mentions anywhere"
    applies to product docs; surviving og-evidence that still matters gets re-homed into
    the parity sheets with corpus citations that describe the OG, not our product).
32. Rewrite the load-bearing contracts: `docs/ABI-CONTRACT.md` (:94-98 error table,
    :692, §7e :875-897), `docs/CORE-API-CURRENT.md` (62 hits — the API/CLI truth sheet),
    `docs/API.md` (9), `docs/architecture.md` (4), `docs/CORE-DISK-ACCESS.md` (80 — the
    transport decision map), `docs/CAPABILITY-MATRIX.md` (6), `docs/PARITY-DELTAS.md`
    (89 — D-DRV-1…6 re-closed as "superseded by the direct-access session"), root
    `README.md` + `SECURITY.md` sweep.
33. Sweep the rest: CI-CURRENT (24), CI-LOCAL (4), CI-SCENARIOS (5), PLAN-CI-REBUILD
    (33), PACKAGING (12), RELEASE (15), PREBUILT-ARTIFACTS (34), PLAN-API-EXPANSION (12),
    UI-READINESS-PLAN (3), benchmarking (12), PERFORMANCE-BASELINE (21 — historical
    numbers keep their run-ID context), BENCHMARK-REPORT (5), testing (2), requirements
    (3), roadmap (3), ground-truth-contracts (7), error-fix-registry (2),
    CONNECTION-GUIDE (5), PARAMETERS (4), REFERENCE-API-GAP (18), API-SURFACE-AUDIT (3),
    PARITY-AUDIT (6), CORE-GAP-LIST (4), PARITY-REFACTOR-PLAN (5), PARITY-OG-CORE-
    INVENTORY (15), recovery-report-schema (1), PERFORMANCE-PROFILE (1), docs/parity/*
    (98 across 8 files), plus root AUDIT.md / AUDIT-FALLBACKS.md (24) / AUDIT-PIPELINE /
    AUDIT-LINUX-FS (verdict rows re-annotated, not deleted — they are the audit register).

**Total**: ~90 product files touched/deleted across code (7), tests (19), tools+CI (9
groups), bench (10+), installer (5), docs (46) — plus the .github/scripts set. Every
deletion lands with its consumers in the same commit (contract, implementation, docs,
pins move together — PLAN-DRIVER-ONLY's own rule, :212-213).

---

## §9 The privilege model requirements

1. **What changes**: today the access gate is the driver device's DACL
   (`D:P(A;;GA;;;BA)(A;;GA;;;SY)`, C §6.2) + an elevated process; user mode never opens
   `\\.\PhysicalDriveN` for data (A §6). Direct access moves the gate to the raw device
   object's own ACL (also admin-only) — an **elevated token** is required, which
   `is_elevated()` (TokenElevation; FALSE for standard users AND non-elevated admins,
   cr_elevate.cpp:12-21) probes correctly.
2. **Elevation timing (mandate §6)**: elevate at the moment raw disk access is needed —
   NOT at process start for UI browsing; no hidden helper, no silent runas, no UAC bypass.
   - CLI: `require_privilege()` is the first statement of every data-path verb (the
     require_driver placement, rrecov.cpp:1338 etc.). When unelevated, the standard
     Windows elevation flow (`ShellExecuteExW` runas re-launch with the identical command
     line) is offered; declined (ERROR_CANCELLED) or unavailable → the two-line refusal +
     `ERR_ELEVATION_REQUIRED` + exit 4. Implementation note: stdout capture/exit-code
     propagation across the elevated relaunch must be designed (OQ-1) — the
     minimal-guarantee fallback (plain refusal + remediation text) also satisfies the
     mandate if the relaunch flow is judged out of scope; that is a user-approved
     decision, never a silent one.
   - ABI/host: the host process owns its launch model (the og's whole model is one
     launch-time `requireAdministrator` manifest, D §4 — our UI repo may adopt the same);
     the core's contract is only the honest refusal code from the open calls.
3. **Refusal honesty**: the refusal names what requires elevation, what the user can do,
   and what still works without it (sessions, tree, previews of recovered content,
   report viewing, `--image` scans — mandate §6). One deliberate stderr surface stays
   (CORE-API-CURRENT.md:345).
4. **Manifest posture (verified correction)**: there is NO embedded `requireAdministrator`
   manifest on `rrecov.exe` in-tree (no .manifest, no mt.exe in any CMakeLists; the only
   UAC manifest is the installer's `PrivilegesRequired=admin`, core-recovery.iss:67 —
   ACCESS-LAYER §6's phrasing "the CLI/app embeds an admin manifest" is imprecise).
   Decision required: keep the JIT model (no exe manifest; runas relaunch/refusal) per
   the mandate, vs the og's launch-time manifest — the mandate's JIT rule governs; the
   exe stays manifestless unless the user approves otherwise (OQ-1).
5. **Scope of the gate**: every raw-device consumer needs elevation — data path AND
   enumeration (geometry probes open `\\.\PhysicalDriveN`) AND health (GENERIC_READ|WRITE
   opens, rrecov.cpp:2769-2772) AND destination arming (volume handles) AND the write
   paths. Operations that never touch raw disk (sessions/tree/query/preview/reports/
   `--image`) stay ungated. Health keeps its current posture (guidance on failure, exit 3
   when all sources unavailable) — it is not fail-fast-gated today and does not become so.
6. **Safety rules unchanged because privilege is higher, not lower** (mandate §6): no
   source writes outside consented repair; destination-overlap hard refusal; BitLocker
   legitimate unlock only; every RAW write consent-gated; the S-010 chain is mandatory
   before any write to a disk with mounted volumes.
7. **WinPE**: runs as SYSTEM (UAC absent) — `is_elevated()` is TRUE; no PE registration
   of anything; the PE docs say "runs as SYSTEM" honestly.

---

## §10 The non-negotiables checklist (mandate §7/§8 restated as checkable items)

- [ ] **Behavior identical** beyond the driver removal itself: every engine, output,
      error class (post-§2 remap), format, JSON schema, user-visible state — verified by
      byte-for-byte scenario comparison on the same seeded disks (gate 4).
- [ ] **No hidden kernel component**: no driver link, device open, IOCTL, presence
      dependency, or activation-when-present path (gate 6; final grep sweep).
- [ ] **No masked failure**: reads fail with the driver-era semantics — verbatim GLE
      propagation, N-125 disconnect precision, retry→Skip→error-map accounting, read-truth
      verdict (D-DRV-6) intact.
- [ ] **No performance regression**: every §7 row met or beaten on real Windows hardware,
      captured in worklog/PERFORMANCE.md with baselines (gate 5); TTFR baseline recorded.
- [ ] **No security regression**: genuine elevation at the right moment, honest refusal,
      no helper/bypass; all safety rules of §9.6 hold.
- [ ] **No dead code**: every driver file/header/target/test/fixture/workflow/doc/installer
      component removed — the §8 inventory closes to zero (grep-verified, gate 6 + gate 9).
- [ ] **No stubs**: no always-Ready driver-status shim, no "driver required" language, no
      fallback language anywhere in product surfaces (gate 9).
- [ ] **Edge-state equivalence**: every §5 row has a test on real hardware (gate 2).
- [ ] **Privilege model correct** (gate 7): JIT elevation or honest exit-4 refusal;
      unelevated runs refuse with nothing past the gate; elevated runs proceed.
- [ ] **CI green for the right reason** (gate 8): all workflows, no continue-on-error, no
      skipped tests; the inverted architecture lint (§8.13-14) enforces the removal.
- [ ] **Win7-SP1→Win11 x64 + PE matrix green**; Win7 import allowlist stays green with the
      SCM imports deleted and no new imports added.
- [ ] **The 64 KiB envelope, N-206 ceil+zero-fill tail, geometry ladder, heal-on-GLE-6,
      single-flight/thread_local-bounce, S-010/S-029 chains, consent gates**: all proven
      on the direct path by dedicated tests before any tuning (correctness first, §0).

---

## §11 Risks and open questions (ranked)

1. **R1 — The JIT elevation flow for the CLI** (OQ-1). The mandate requires elevation at
   the moment of need, not launch; the og's precedent is launch-time manifest (D §4); our
   exe is manifestless (§9.4). The runas-relaunch design (stdout redirection, exit-code
   propagation, `--json` consumers) is real engineering with UX consequences. Decision
   needed from the user: (a) runas relaunch, (b) plain refusal + "re-run from an elevated
   terminal" guidance (minimal, mandate-compliant), or (c) launch-time manifest (og
   parity, conflicts with the mandate's "not at process start" rule for UI hosts).
2. **R2 — The 256 KiB direct-vs-driver gap on the runner** (151.5 vs 169.3 MiB/s,
   run 40227375329): on that virtualized hardware the driver's direct-IRP forwarding beat
   a plain ReadFile at the same chunk. The scan pipe (241.64 MiB/s measured, 40315255368)
   suggests the gap closes through the pipe shape, but this is the single most likely
   place to miss gate 5. Mitigation: measure `bench seq` on the direct path FIRST
   (Phase 1), before mass rewiring.
3. **R3 — ABI break for hosts**: removing `cr_ui_driver_status` + the 0x500B codes breaks
   any compiled host (the out-of-repo UI dependency is tracked in the very doc being
   deleted, DRIVER-REQUIRED.md §6.3). The ABI version bump + a UI-repo coordination note
   must land in the same change-set. (OQ-3: is an additive `cr_ui_is_elevated` wanted?)
4. **R4 — Doc-truth pins read docs at test time** (test_cr_ui.cpp:4036-4104): the doc
   sweep and the test rewrite must be one commit, or CI fails confusingly mid-sweep. The
   46-file doc sweep is the largest single mechanical risk of the session.
5. **R5 — The padded-tail (N-206) edge on real odd-sized media**: the zero-fill rule (§4.2)
   is a semantic the driver provided implicitly; a miss shows up only on odd-sized sources
   at the last sector. Dedicated test required; interacts with the envelope's tail clamp.
6. **R6 — Direct-write posture on mounted disks**: the S-010 chain is unchanged and
   required, but the driver-era write path was only ever exercised through the grammar
   handle; the direct RW open's share mode (recommend R|W|D on the disk handle, §1 row 5)
   and the offline-step sufficiency need the fix/apply matrix on real mounted volumes.
7. **R7 — Stale bench probes**: run-behavior.ps1's nodriver probe is already stale
   (expects pre-e58bde10 text); run-ours.ps1 asserts the RDA transport line. Both must be
   rewritten with the replacement or the lanes lie.
8. **OQ-4 — ENUMERATE_DISK_BOUND (200)**: re-pin to the enumeration contract (the driver
   grammar limit is gone; 200 stays as the product's enumeration space) — test and
   comment re-based, no behavior change.
9. **OQ-5 — `bench --no-rda`**: already removed (rrecov.cpp:2978-2980); confirm no bench
   script still passes it.
10. **OQ-6 — `pe_environment_by_system_drive` (cr_winenv.h:36-38) and its pin**: pure
    corpus-evidence predicate with zero production callers since R12. Recommend DELETE
    with its test (the evidence lives in REFERENCE-ACCESS.md); keeping it violates no
    rule but is dead product code under the mandate's no-dead-code clause. Decide once.
11. **OQ-7 — win-stub SCM trio**: delete `OpenSCManagerW/OpenServiceW/QueryServiceStatus/
    CloseServiceHandle` stubs (tests/win-stub/windows.h:579-598) only after confirming no
    remaining consumer (BitLocker/health verified clean; re-grep at implementation).
12. **OQ-8 — session-protocol capabilities** (exclusive sector ranges, TRIM swallowing,
    C §2.4): documented driver features with zero core consumers (D5) — nothing migrates;
    confirm no bench depends on them (run-alignbench uses plain ReadFile only, A §10.11).

---

*One file written this task. No source modified, nothing committed, nothing pushed.*
