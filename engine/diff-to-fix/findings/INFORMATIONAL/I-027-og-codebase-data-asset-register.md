# INFORMATIONAL I-027 — og-codebase data-asset ground-truth register (session-8 sweep): the free-edition quota contract, the raw-results shield, telemetry batching caps, the DiskHealth autostart, and the filter_generalraw value

**Severity:** INFORMATIONAL
**Module:** mom-capabilities / config assets / docs
**Parameters affected:** Q68 (documentation), B10-adjacent (the shield's extension list), I-010 lineage (scope/asset registers)
**Corpus references:** `og-codebase/recoveryFree.ini` (128 B binary) + `DRWUI_decompiled.c:550794-551204` (the registry decode); `og-codebase/remote_config_data` (the fetched Firebase cache) + `DRWUI_decompiled.c:499195-499228` (the json_raw consumer); `og-codebase/AliyunConfig.ini` (the caps block); `og-codebase/diskhealth/addtask.bat` / `deltask.bat` / `querytask.bat`; `og-codebase/remote_config_data` (filter_generalraw's 65-extension list)
**Core references:** audited `35adcb9`. All five verified ABSENT from core (correctly, per the registered exclusions — the greps and the exclusion citations are in the sweep record below); the register exists so the ground truth is on-file for future parity decisions.
**Status:** READ-ONLY FINDING — informational register; no defect in core

## Summary

The session-8 sweep of the never-audited og-codebase data assets decoded five product-behavior contracts that no artifact or finding had recorded. None opens a core code gap (each maps to a registered exclusion — licensing, telemetry, UI, host-side); each is now on-file as ground truth with its decoded constants.

## The five decoded contracts

**1. The free-edition quota contract** (`recoveryFree.ini` + `DRWUI_decompiled.c:550794-551204`): HKLM `SOFTWARE\DRWNewFree\Metadata` (sibling `…DRWNewTrial\Metadata`) QWORDs — `EnableTotalSize` default **0x80000000 = 2 GiB**, `EnableTotalFileCount` default 2^31, `RecoveredTotalSize`, `RecoveredTotalFileCount`, `Times`; a **500 MB (0x1f400000) clamp constant** caps a quota parameter; the 128-byte ini files are the on-disk mirrors of the same record family. The core has no edition gating (licensing OOS, row 104) — this is the spec if the downstream product ever implements gating.

**2. The raw-results shield** (`remote_config_data` `json_raw` = `{"switch":true,"shield":"TXT;HTML;LOG;H;CPP;MM;"}`; consumer `DRWUI_decompiled.c:499195-499228`): DRWUI splits the shield on `;` into the raw-tree extension-exclusion list. `filter_generalraw` was decoded by OQ-S4-2 (the display-name mapping); `json_raw` was not — this register decodes it. UI-layer posture (OOS); the value is the ground truth for any host-side results filter.

**3. Telemetry batching caps** (`AliyunConfig.ini`): `nMaxItemCount=1000`, `nMaxItemDays=7`, `nMaxIdleMinutes=5` (idle-flush), `nbLimitKeyvalueMaxLen=60000`, `nLimitPairsCount=20`, `bAllowSendInfo=0` (default off), the `AllowSendUrl` gate, machine-SID uid. REPORTING-OOS (row 51); the caps are the spec for any host-side telemetry parity.

**4. DiskHealth autostart** (`diskhealth/addtask.bat`): `schtasks.exe /create /f /sc ONLOGON /rl HIGHEST /tn %1 /tr %2` (+ delete/query pair) — the companion app installs a logon scheduled task at HIGHEST for itself. Host-concern datum (the core ships no autostart; the merged product's installer would own any equivalent decision).

**5. filter_generalraw's concrete value**: the 65-extension junk list (`loggz, crash, evtx, lrprev, tmb, peak, default, sqm, pf, icns, …, sqlite, m3u, amr, moff`) — the trail recorded the semantic (display-name mapping, OQ-S4-2); the value is now on-file.

## Sweep disposition summary (for the coverage record)

Every other asset examined maps to an existing register or exclusion: DataFile.ini (row 67, Aliyun cursor), Upgrade.ini (row 78), needcopy.txt (row 98), dlconfig.xml/PEtools.xml (rows 71/101, the WAIK media builder), DRWReport.ini (row 73), google-services.json (row 88), the runtime logs (row 75 + the vendor-logs artifact — EPMLOG corroborates N-071; DRWUI.log's auto-rsf names corroborate cr_crsf.cpp:363-404; CalcPartition.log corroborates M-017), Resource/ResourceDark (UI), VideoViewer (row 107), License/ (row 95), Qt plugins (line 30), icloud_logger/NasConfig (rows 97/110), unins000.* (S-038). auto.txt's misclassification is filed separately as **N-097**.

## Impact

None on core behavior. The register prevents re-derivation and anchors future host-side parity decisions to corpus-exact values.

## Reproduction / verification

Corpus first-hand: each file read in full; each consumer decode read at the cited decompile sites; each core absence grep-verified (the commands and zero-hit results are in the session-8 sweep context sheet, worklog-recorded).

## Fix plan (for the engineering team)

1. None required for core. Optionally: fold items 1-5 into the PARITY-OG-CORE-INVENTORY rows they extend (quota contract → row 104's note; shield → a new row-adjacent note; caps → row 51's note; autostart → row for DiskHealth; filter value → the carve.md session-5 addendum).

## Tests to add

- None (informational).

## Related findings

- N-097 (the auto.txt misclassification found by the same sweep).
- M-082/OQ-S8-1 (the decode candidates the same session closed or opened).

## Open questions

None.
