# PARITY-DELTAS — divergences from the reference product, with rationale

The reference product's installed layout and install behavior are the
target WHERE OBSERVABLE (corpus: easeus-drw-research). Where the reference
is idiomatic we match; where its choice was idiosyncratic we deviate
deliberately and record it here. No silent divergence.

## Installer/layout deltas (from INTEGRATION-PLAN §7.6, shipped)

| ID | Reference behavior (corpus evidence) | Ours | Rationale |
|---|---|---|---|
| D-I1 | Inno Setup 5.5.x installer + unins000 trio | NSIS (Tauri-native) | installer tech is not user-visible behavior; the mandate names NSIS |
| D-I2 | ALL writable data inside the install dir (log\, SaveScan\, DRWCfg.bin) | Program Files (RO) + %ProgramData% sessions/logs + localStorage per-user | industry standard; deliberate |
| D-I3 | requireAdministrator whole chain (both exes) | Same: app manifest requireAdministrator | both transport legs are admin-only on stock Windows (driver DACL + raw handles); reference parity is the honest choice — recorded because the original decision text had flagged it |
| D-I4 | Inno `{AppId}_is1` uninstall key | NSIS standard uninstall key | mechanical consequence of D-I1 |
| D-I5 | DRWService "Read FVE Server" AUTO_START LocalSystem | no service; BitLocker in-process via cr_ui | our engine has the capability in-process; no key-broker needed |
| D-I6 | eprdtdrv installed on demand at first raw scan by setupdrvx64.exe | driver installed by a SURFACED installer step (demand start, Boot File System group, System32\drivers\<SN>.sys — same SCM semantics) | closer to the surfaced-elevation mandate; same wire contract |
| D-I7 | unins000 shared-file refcount prompts | NSIS standard /REBOOTOK in-use handling | mechanical |
| D-I8 | CVersionCtrl/AutoUpdate network update chain | no auto-updater; releases via GitHub, manual download | deliberate v1 scope cut; documented |
| D-I9 | Sessions `.rsf` in `<installdir>\SaveScan\` | `.crsf` (CRSFMT01) in %ProgramData%\...\sessions\ | the format IS the product; auto-naming parity preserved (N-061) |
| D-I10 | SetupUE 16-OS gate table + install telemetry | NSIS Win10-gated (WebView2 floor; engine/driver keep the Win7 contract — OQ-6 recorded) + no telemetry | privacy + simplicity |
| D-I11 | Driver SDDL absent (PID token only) | admin/SYSTEM SDDL | deliberate divergence already carried in the driver repo |

## This integration's additions

| ID | Divergence | Rationale |
|---|---|---|
| D-B1 | Engine DLLs installed BESIDE the app exe (reference: engines LoadLibrary'd from the exe dir too — matched, but we document it because a subfolder layout would be more "tidy") | static-import loader truth: resolution happens from the exe directory at process start; an `engine\` subfolder 0xc0000135s |
| D-B2 | Session auto-names use UTC (`auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf`) vs the reference's local-date naming | std gives no TZ offset; the bridge stays dependency-free and deterministic — recorded rather than silently differing |
| D-B3 | Results filtering is bridge-side for BOTH live scans and sessions (reference: client-side filtering over its trees; our plan originally said engine query for sessions) | the engine's query cannot express the contract's 'existing' atom (no deleted_never knob); ONE path = identical semantics mid-scan and post-scan (G-7 records the migration) |
| D-B4 | `rrecov.exe` ships beside the app | the G-1 interim: standalone repair undo is CLI-only until the ABI additions land; shipping the tool makes the documented path real |
| D-B5 | RESOLVED — pre-scan repair now prompts for consent (matching the reference) | follow-up (a) landed: the scan-page consent prompt + `consent_respond` + the `consent-request:<scanId>` event ship (G-1, contracts/event-mapping.md); the interim NULL-consent default-deny posture this row recorded is retired |
| **D-FALL** | **The driver is MANDATORY — every driver-absent fallback removed** (user directive, 2026-10-27). The reference's Device.mo duality — EPRDTDRV grammar first, silent `\\.\PhysicalDriveN`/`HarddiskVolumeN` direct fallback when the probe fails (`Device_decompiled.c:352-359, 7816-7823, 50653-50660, 52437-52444`) — is DELIBERATELY NOT REPRODUCED. Our engine rides the RDA grammar exclusively; a missing driver is the distinct `ERR_DRIVER_REQUIRED` (0x50020007) refusal naming the remedy. The CLI's `--no-rda` switch is removed; the WinPE direct-open gate is removed (the driver installs on PE media via `driver/scripts/winpe`); the direct-handle transports (`open_physical_drive`/`open_harddisk_volume`/`DirectDevice`/`open_path`) are DELETED. | The user directive supersedes the corpus-observed duality: "driver is the only way, its not optional, its mandatory, so remove any fallbacks for it." Consequences are embraced: no degraded mode, no per-user install, silent-direct scans never exist. What stays on the OS storage stack (unchanged, same class as the og's own list logic): enumeration geometry IOCTLs, drive-letter→extent mapping, SMART/NVMe/CSMI health passthroughs, BitLocker status probes, `IOCTL_DISK_UPDATE_PROPERTIES` — metadata/policy IOCTLs, not sector reads; the disk LIST works without the driver (with `table_ok=false`), the SECTORS never do. Engine-internal corroboration: `engine/docs/DRIVER-UTILIZATION.md` §1.9 (superseded marker), `engine/AUDIT-FALLBACKS.md` rows FB-T01/T02/K04 (class: fallback; disposition changed by this directive). |
| **D-DSIGN** | The driver + engine kit + installer ship **development test-signed** (ephemeral self-signed certificate per build; the installer imports it into Root + TrustedPublisher; uninstall removes it). The reference ships production-signed binaries. | The user directive ("test development signing on both driver and installer"). Production signing (EV/OSSign + Hardware Dev Center attestation) remains pending — every release note states it. The certificate is a development identity: it proves build provenance and enables load-time verification on machines that trust it; it is NOT a publisher identity the ecosystem trusts. Design: `docs/DEV-SIGNING.md` (policy matrix + primary sources). |
