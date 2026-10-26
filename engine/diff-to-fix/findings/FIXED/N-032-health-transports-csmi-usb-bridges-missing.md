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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-032 commit — `git log` "fix(N-032)")
**Verified by:** Health.CsmiRequestMatchesOgWireFormat, Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic, Health.CsmiBuilderRejectsUndersizedBuffer, Health.CsmiResponseDataExtractedAtOgOffset, Health.CsmiTransportUnavailableClassifiesTheOgErrorSet, Health.CsmiTransportFallsBack, Health.CsmiReadsAttributes, Health.SmartChainEmptyIsBadParameter, Health.SmartAttrNameTableCoversTheCuratedIds (9 new; suite 752/752 linux-release, was 743/743 at the session baseline); win_syntax_check green over the refactored win layer; EXECUTED fail-before: the pre-fix two-transport chain shape [ata-passthrough, smart-rcv-drive-data] returns ERR_HEALTH_UNAVAILABLE (0x50070001) on the RST-shaped mock — the CSMI leg is what turns it green.

**Resolution notes:** CORPUS DEEPENED BEFORE FIXING: the finding's citation
was the companion-artifact census only; this session decoded the actual
transport wire in re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c
first-hand — GetSmartAttributeCsmi = FUN_00425450: IOCTL 0x4D008
(IOCTL_SCSI_MINIPORT), SRB_IO_CONTROL {len 0x1C, "CSMISAS", timeout 60,
ControlCode 0x19, Length = data+0x8C} + vendor fixed area {phy@+0x1C,
port@+0x1D, SAS address@+0x20, FIS 0x8027@+0x2C, command@+0x2E,
features@+0x2F, LBA/device dword@+0x30 (SMART magic 0xA0C24F00 = cyl
4F/C2 + device A0; IDENTIFY 0xA0000000), sector count@+0x38,
vendor field 0x11@+0x40, data length 512@+0x44}, total 0x2A8, response
data at +0xA4; the transport-unavailable GetLastError set {1, 0x32,
0x37}; the og's own IOCTL_SCSI_GET_ADDRESS address probe (FUN_00417750
:12254, 0x41018); and the og's legacy IDEREGS for SMART commands
(SendAtaCommandPd :14166-14176: dword 0x4F0101D1 + 0xC2 = features D1,
count 1, sector 1, cyl 4F, cyl high C2, command B0). LANDED: (1) the
pure wire module include/cr/health/cr_csmi.h + src/core/health/cr_csmi.cpp
(og-exact builder/parser, byte-pinned by tests); (2) the pure transport
chain seam include/cr/health/cr_smart_ata.h + cr_smart_ata.cpp — the
whole read flow (IDENTIFY → first-answering channel serves READ DATA
0xD0 + READ THRESHOLDS 0xD1 → 30x12 parse → model string → S-024
grading) moved out of the win layer, matching the og's CAtaSmart
accessor model (one accessor family serves the whole command set);
(3) the win layer now supplies three transports: ata-passthrough →
smart-rcv-drive-data (legacy IDEREGS now carry the og's count-1/
sector-1/cyl-4F/C2 SMART stamping) → CsmiMiniportChannel (SCSI address
→ \\.\Scsi%u: → CC-0x19 request). ERR_HEALTH_UNAVAILABLE stays the
clean final fallback. REGISTERED DELTAS (PARITY-DELTAS D-78): the og's
RAID-census routing (phy/port/SAS from GET_RAID_INFO/CONFIG/PHY_INFO
walks incl. AMD rcraid), the LSILOGIC sibling transport, and the og's
SMART auto-enable write (0xB0/D8 — a device write; this module is
report-only) are deliberately not ported, with rationale. The USB-bridge
SAT half of the finding stays as the fix plan's own "only if the product
targets external-disk health" option — not pursued (the bridge DB is a
20+-entry vendor maintenance burden the plan flags; health is advisory
and the ATA-passthrough route already serves most USB bridges through
SAT translation in the standard stack). Real-hardware validation on an
Intel RST host remains pending (the finding's inherent
UNVERIFIED-BY-EXECUTION limit — no RST array in any CI fleet).
