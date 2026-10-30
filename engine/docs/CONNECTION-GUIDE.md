# Connection Guide — binding, driving, and connecting devices

How a host connects to core_recovery: building, linking, the binding
lifecycle, device connections (driver / image / encrypted),
threading rules, and the consent model. For the complete surface
inventory see [API.md](API.md); for tunables
[PARAMETERS.md](PARAMETERS.md).

---

## 1. Build and link

- **Windows (production)**: `cmake --preset win-x64-release` (vcpkg
  manifest: zlib, lz4, zstd). Outputs `rrecov.exe`, `cr_ui.dll` +
  `cr_ui.lib` (import library), and the test binaries. Toolchain: MSVC
  `/W4 /WX /permissive-`; the loadability gate keeps every shipped PE
  **Win7 SP1-inclusive** (SDK macros gate declarations; PE OS version
  stamped and import-allowlist-checked in CI).
- **Linux (development)**: `cmake --preset linux-release` builds the
  pure core + tests (the win layer is parse-checked by the
  `win_syntax_check` gate instead of compiled). gcc
  `-Wall -Wextra -Wpedantic -Werror -Wshadow`.
- **Sanitizers**: `linux-ubsan` preset + CI TSan lane; ASan/UBSan/TSan
  all green with zero suppressions.
- The decompressors (zlib, lz4, zstd) are REQUIRED in every build — a
  build that silently compiled them out would turn compressed pools into
  zero-filled salvage (the fallback-honesty rule, AUDIT-FALLBACKS).

Embedding choices:

| You are building… | Bind via |
|---|---|
| a C/C++ host, wizard-style UI, service | `cr_ui.dll` / `libcr_ui.so` — the flat-C ABI (G-01) |
| a one-shot tool / scripting | the `rrecov` CLI (stable grammar, stable exit codes) |
| a host that wants engine-level control | the C++ headers under `include/cr/` (pin a commit; not ABI-stable) |

## 2. The cr_ui binding lifecycle

```c
#include <cr_ui.h>

if (cr_ui_abi_version() != CR_UI_ABI_VERSION) refuse();   /* version gate first */

cr_ui_disk d[8]; uint32_t n = 0;
cr_ui_enumerate_disks(d, 8, &n, NULL);                    /* census (≤ 200 disks) */

cr_ui_device* dev = NULL; uint32_t err = 0;
cr_ui_open_disk(0, &dev, &err);                           /* or _open_volume /
                                                             _open_image */
cr_ui_scan* s = NULL;
cr_ui_scan_config cfg = {0};                              /* zero-init is the documented default set */
cfg.whole_disk = 1;
cr_ui_scan_create(&s);
cr_ui_scan_attach_device(s, dev);
cr_ui_scan_set_event_fn(s, my_event_sink, my_user);       /* optional; thread-safe sink */
cr_ui_scan_run(s);                                        /* blocks; cancel/pause from other threads */
cr_ui_scan_stats st; cr_ui_scan_get_stats(s, &st);
cr_ui_scan_save_session(s, u"run.crsf", NULL);
cr_ui_scan_close(s); cr_ui_device_close(dev);
```

Rules that matter:

- **Zero-init every config struct** — the documented defaults are the
  zero fields; the ABI only reads what you set.
- **Strings the ABI returns** are heap buffers: free with
  `cr_ui_string_free`.
- **Snapshots** (`cr_ui_scan_snapshot` mid-scan, `cr_ui_session_*`
  post-scan) are immutable views — hand them to UI threads freely.
- **Pause/cancel** are cooperative and fast (the cancel token is polled
  inside every engine loop — the cancellable-IRP discipline).

## 3. Device connections

| Connection | Path | Notes |
|---|---|---|
| RDA driver (the ONLY live-disk path) | `\\.\RDA\<token>\<disk>\<part>` (grammar + token, D-37) | the repo's own kernel transport ([rda-driver](https://github.com/heicfast/rda-driver)); direct reads ride the 64 KiB envelope. The driver-only architecture (PLAN-DRIVER-ONLY.md, enforced by the CI architecture lint) REMOVED the direct `PhysicalDrive`/volume fallbacks: `cr_ui_open_disk` REFUSES with the classified `ERR_DRIVER_*` code when the driver is absent/not-started/unauthorized/version-incompatible — no silent direct-handle leg. M-030's quiesce chain (LOCK → DeleteVolumeMountPoint → flush → DISMOUNT → OFFLINE) lives on in the fix/apply volume path below |
| Disk images | `cr_ui_open_image` | flat, VMDK (descriptor + monolithic sparse), VHD/VHDX (fixed + dynamic) — the same engine stack as physical devices |
| Test/edge transports | implement `cr::IStorageDevice` | everything above the storage seam is transport-agnostic |

Quiesce model (volume writes: fix/apply): LOCK → DeleteVolumeMountPoint
→ flush → DISMOUNT → OFFLINE terminal step (the og's full chain, M-030)
→ restore ONLINE + mount point on release. An OFFLINE failure degrades
to dismount+lock and is logged — never silent.

## 4. BitLocker volumes

Locked volumes are NEVER bypassed (`ERR_FVE_LOCKED`). The legitimate
unlock surfaces:

1. **`--bitlocker-fvek hex` / `cr_ui_open_volume_with_key`** — a
   user-supplied FVEK flows through the in-stack XTS-AES
   `DecryptingDevice` (M-033); the engines stay BitLocker-agnostic.
2. **`--bitlocker-dict [file]` / `cr_ui_unlock_volume_with_dictionary`**
   — the consented password-dictionary aid (N-064, canonical D-80): the
   228-entry shipped table first, then user candidates, through the same
   PowerShell `Unlock-BitLocker -Password` / `manage-bde -Password`
   plumbing as a typed password. Windows applies the key; the core
   performs no crypto and no attempts without the explicit consent.

Auto-unlock (S-014): live-registry + offline-hive legitimate unlock only
— reading the og's stored unlock material, never cracking.

## 5. Threading contract

- **Event sink**: may fire from engine worker threads; the core does NOT
  serialize deliveries — the sink must be thread-safe, non-blocking
  (documented at `cr_scan.h` :110-113; the shipped test sinks lock).
- **State queries** (`busy`/`stopping`/telemetry/snapshots): any thread,
  any time.
- **One scan handle per device**; sessions/snapshots are read-only and
  share freely.
- `threads = 1` gives a fully single-threaded scan (deterministic
  testing).

## 6. The consent model

Every write-side or unlock-side operation is an explicit operator action:

| Operation | Consent surface | Refusal behavior |
|---|---|---|
| RawFixer apply | `--repair-apply` / `RepairConfig::consent` callback | NULL callback = consent denied, zero writes, not even a volume lock |
| Lost-partition apply | `--apply` / `cr_ui_apply_lost_partition` consent | plan-only; refusals listed (overlap / no-slot / GPT-v1 / capacity / 32-bit-range) |
| Boot-chain repair (BCD/NVRAM) | plan/execute/undo, default-deny | no write without the explicit execute |
| Dictionary unlock | `--bitlocker-dict` / the ABI action | locked stays `ERR_FVE_LOCKED` |
| FVEK open | `--bitlocker-fvek` / the ABI open | same — no bypass path exists |

The writer also refuses a destination inside the source volume
(destination-overlap preflight). These are documented deliberate
supersets over the reference (see the safety rows in
[PARITY-DELTAS.md](PARITY-DELTAS.md)).

## 7. Sessions and resume

`--out run.crsf` (or `cr_ui_scan_save_session`) writes the durable
container ([FORMATS.md](FORMATS.md#the-crsf-session-container)) with
auto-checkpoints at stage boundaries. `--resume run.crsf` revalidates
the device identity (boot-sector hash + the disk-layout snapshot) before
continuing; a changed layout is surfaced (`layout_changed`), stale
candidates dropped with counts — never silently ignored. Cancelled scans
exit 3 and still save the session.


## 8. Known host projects (binding status, 2026-10-26)

- **recovery-wizard-ui-new** (the product's UI): consumes **nothing yet**.
  The app runs on its deterministic `MockConnector` behind a typed TS
  connector contract that mirrors the `cr_ui` C ABI family-for-family
  (`contract.ts` domains map onto the `cr_ui_*` export groups); the
  Tauri bridge is a loud throwing stub pending the engine-integration
  session. The two contracts are kept in lockstep from the UI side —
  post-ADR-008 the connector surface is `requestPreview(scanId, nodeId)`
  (connector-inferred kind), `startRecovery(selection)`, filters without
  `confidence` but with `hideEmptyFiles`, and
  `pickDirectory({excludeDeviceId})` — so when the binding session lands,
  the UI's contract documents and this repo's [ABI-CONTRACT.md](ABI-CONTRACT.md)
  must not contradict each other.
- **rda-driver** (the kernel transport): not a linker consumer — the
  core opens `\\.\RDA` at runtime with the service name `"RDA"` (§3); the
  bench lane pins its release assets (see
  [DRIVER-UTILIZATION.md](DRIVER-UTILIZATION.md) pinning addendum).
