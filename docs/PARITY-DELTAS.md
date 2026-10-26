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
| D-B5 | Pre-scan repair analysis runs with the engine's NULL-consent default-deny (reference: prompts for consent pre-scan) | no consent surface exists in the UI to drive the callback; the engine's documented default-deny keeps the analysis read-only (G-1 records the follow-up: prompt + consent_respond) |
