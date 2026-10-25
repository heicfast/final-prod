# Evidence — session 6: MOM/WinPE/config/UIUX wiring layer (16 never-cited artifacts, first consumption)

**Session:** 6 deep-continuation (2026-10-26) · **Audited core commit:** `022c81d`-lineage · **Corpus:** `03ad46c`
**Cited by:** N-055, N-058, N-059, I-019, I-020, OQ-S6-1, the M-030 addendum. Key quotes verbatim; the full 16-artifact decode lives in the session context sheet (scratch, not committed).

## 1. PartitionRecoveryD — generator sets + .dat persistence

`re-workspace/artifacts/MOM-modules/PartitionRecoveryD.md:23-24`:

> - `CGeneratorSetCompleteSearch` ×2
> - `CGeneratorSetRegSearch` ×2

`:37-38` (Notable string evidence):

> - `lostpart.dat`
> - `partinfo.dat`

## 2. WinPE — the driver-selection facts

`re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md:111-121` (summary quote; see artifact for the regdrv.reg chain):

> PE raw disk access does NOT use eprdtdrv (absent from the payload): the boot environment demand-loads ebrntdrv/epmdkdrv via NtLoadDriver at first open, with regdrv.reg wiring the UpperFilter and winpeshl's autobat chain launching the recovery shell.

Supporting byte-diff: `PartitionRecovery/epmntdrv-trio--byte-diff-verification.md` — eprdtdrv_win7older.sys == BUILDPE ebrntdrv.sys (identical MD5 73ddde6e…), i.e. the PE lane runs the OLDER driver build.

## 3. Enumeration breadth

`re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:57-59` (enumeration fan-in): WMI (MSFT_*), SetupAPI, VDS (`CDiskEnumerator : Vds loadservice`, gated `dwMajorVersion >= 6`, Device_decompiled.c:51709), Storage Spaces, Function Discovery (`CMyFDHelper`, CLSID c72be2ec-…, failure tolerated as HRESULT).

Core side: `src/win/cr_enumerate.cpp:15-60` — `\\.\PhysicalDrive%u` CreateFileW loop; grep SetupDi/WMI/VDS/StorageSpaces: 0 hits.

## 4. Event-bus shape

`re-workspace/artifacts/UIUX/25-connect-wiring-graph.md:41-62`:

> `doDispatchMessage(int, void*, int&)` — **15 UI classes subscribe to it**, always wiring to `onReceiveData(int,void*,int&)` … the double bridge at 1369866/1369872 (`dispatch` → `doDispatchMessage`) is the module-side publisher feeding the UI-side bus

`:125-144` (the results-page hub): `sigIsBusy(bool&) ×14`, `sigIsStopping(bool&) ×8`, `sigGetRightIndex(int&,int&) ×4` — "reference-out-param query signals".

`:175-192`:

> **Progress = poll, events = push**: scan/finish events push (signals); progress percentages pull (38 timeout-consumers). **The only progress-push signal is `sigPercentChange` (CUISink's)**

`UIUX/20-moc-surface-sink-architecture.md:145` — typed `IUIProgressInfo` recovery-progress interface.

## 5. DRWCfg persistent config

`re-workspace/artifacts/config-files/drwcfg-bin--aes-config-cache.md:32-54` — DRWCfg.bin: AES-256-CBC hardcoded-key config cache (per-scan state/MRU/flags). `UIUX/13-ui-core-wiring-state.md:28` — remembered scan mode re-applied at scan start.

## 6. SMART table contract

`re-workspace/artifacts/UIUX/19-diskhealth-quick-ui.md:26-27`:

> - **Health states**: "Good" / "Caution!" / "Normal" / …
> - **S.M.A.R.T. table**: ID / Attribute / Current / Worst→(ST) / Threshold / RawValue / Status / "Temprature" [sic] — the classic SMART attribute grid

Core side: `src/win/cr_health.cpp:167-187` — the issued command ladder is IDENTIFY (0xEC) + SMART READ DATA (0xB0 feature 0xD0) (+ legacy fallback); 0xD1/0xD8 never issued (contradicting `include/cr/win/cr_health.h:8`'s DELTA comment — N-074 sub-item 5).

## 7. MATCH register (verified equivalents, no findings)

- TRIM probe: fsutil behavior query disabledeletenotify (24:113 ↔ cr_health.cpp:282-286 — fsutil parse matches).
- Offline-hive reading: RegLib ↔ cr/reghive (S-014's landed fix).
- Abort-event separation (UIUX13:42 ↔ cr_ui.h:335-338 cancel vs close).
- MOM Error/Thread/Common/Config module internals: host-side orchestration equivalents registered under D-21/D-24/D-30x (the in-scope semantics — error taxonomy D-15, threading D-30x — were already registered; the module internals add no new engine contract).
- vendor-logs QA forensics: provenance only, no scan contract (OUT-OF-SCOPE notes recorded).
