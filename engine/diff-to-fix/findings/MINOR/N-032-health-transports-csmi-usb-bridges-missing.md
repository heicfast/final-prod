# MINOR N-032 — health transports: og companion adds CSMI (Intel RST) + USB-bridge SAT + Realtek-NVMe accessors; core has ATA-passthrough + legacy + NVMe only

**Severity:** MINOR
**Module:** health
**Parameters affected:** B1-adjacent (health availability, not recovery), X47-adjacent, F60-adjacent (advisory quality)
**Corpus references:** `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md` (first-hand read this session): DiskHealth.exe = CrystalDiskInfo-lineage companion (93,316-line decompile in ghidra-wave2); `CAtaSmart` engine with per-transport accessors `GetSmartAttributeSat -1A/-2/-2A` (SAT/ATA passthrough variants), **`GetSmartAttributeCsmi`** (CSMI for Intel RST arrays), **`GetSmartAttributeNVMeRealtek`** (Realtek NVMe bridge); USB-bridge database (`AddDisk - ASM1352R`, `AddDisk - Si`, `AddDisk - USB0A…USB20`); `DiskInfoMgr`/`RefreshThread`/`CTrayIcon`/`AttrModel` classes; boot-USB deployment (auto.bat Phase E) + notification integration (`Click_Open_DiskHealth_By_Notification`)
**Core references:** audited commit `34b3441` — `src/win/cr_health.cpp` (326 lines: `ata_passthrough` + `smart_rcv_legacy` SMART_RCV_DRIVE_DATA 0x7c088 fallback, 30×12-byte attribute table parse, TRIM via fsutil), `src/core/health/cr_nvme.cpp` (42 lines: NVMe health-log page parser), `include/cr/health/cr_nvme.h`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og ships SMART telemetry as a **separate companion app** (DiskHealth.exe, a re-skinned CrystalDiskInfo-lineage Qt/QML binary deployed from the boot USB), whose engine polls SMART over a **multi-transport accessor set**: SAT passthrough variants, CSMI (Intel RST arrays), a Realtek NVMe bridge path, and a USB-bridge database for external disks. The core's in-process health module covers ATA passthrough + the legacy pre-Vista IOCTL + NVMe health pages. On Intel RST volumes and USB-attached disks the core's `read_smart` returns `ERR_HEALTH_UNAVAILABLE` where the og's companion succeeds. Recovery itself is unaffected (health is advisory).

## What the reference does

The companion's `CAtaSmart` engine (corpus artifact, vftable census) carries the transport matrix above plus `DiskInfoMgr` (disk list + refresh cycles), `AttrModel` (QML dashboard), tray/notification integration, and virtual-disk naming (`Parallels Virtual NVMe Disk`). DRWUI itself only surfaces the recovery-affecting facts (SSD/TRIM warnings on the home page); the deep telemetry is the companion's job.

## What our core does

`read_smart` (cr_health.cpp:162): IDENTIFY via `ata_passthrough`, falling back to `SMART_RCV_DRIVE_DATA` (0x7c088, pre-Vista filter drivers); SMART READ DATA (0xB0/D0); the 362-byte attribute table parsed per ATA-ATAPI-7 with failing-flag semantics; curated fields (reallocated/pending/uncorrectable/temperature/SSD wear); `query_trim_state` via fsutil; NVMe health-log page parser (cr_nvme.cpp). No CSMI path, no USB-bridge SAT variants, no bridge database.

## Gap

Falsifiable: on a disk behind an Intel RST array (CSMI-only access) or a USB bridge requiring vendor SAT translation, the core returns `ERR_HEALTH_UNAVAILABLE`; the og's companion reads SMART successfully (its accessor set exists precisely for those stacks).

## Impact

Health advisories only (the F-parameter family's advisory side): missing temperature/wear/failure warnings on RST/USB-attached disks. No recovery-correctness impact; the og isolates the same capability in a side app that is itself out of the recovery-engine scope.

## Reproduction / verification

Corpus-derived (artifact + class/accessor census, cited); core-derived (source read at `34b3441`). Execution on RST/USB hardware not possible in this environment — the finding's hardware dependency is inherent (UNVERIFIED-BY-EXECUTION on the RST/USB failure mode; the code paths' absence is verified).

## Fix plan (for the engineering team)

1. Add a CSMI transport (`IOCTL_SCSI_MINIPORT`/`IOCTL_SCSI_GET_ADDRESS`-based CSMI passthrough, the CrystalDiskInfo `GetSmartAttributeCsmi` pattern) behind the existing transport-fallback chain in `read_smart`.
2. Add USB-bridge SAT variants only if the product targets external-disk health (the og's bridge DB is a maintenance burden — 20+ vendor entries; consider the top-5 bridges).
3. Tests: a mocked `IStorageDevice`-style handle injecting CSMI responses (`Health.CsmiTransportFallsBack`, `Health.CsmiReadsAttributes`); keep `ERR_HEALTH_UNAVAILABLE` as the final fallback so the failure mode stays clean.

## Tests to add

`Health.CsmiTransportFallsBack`, `Health.CsmiReadsAttributes` (both mock-based; fail-before trivially — the transport does not exist).

## Related findings

None open in health (N-009a covers the report-field side; this is the transport side). The THIN matrix verdict for health is resolved by this finding + the verified parity of the ATA/NVMe paths.

## Open questions

None for parity. (Product question, not audit: whether the companion-app model itself is in scope for the core project — recorded as a scope note, not a divergence.)
