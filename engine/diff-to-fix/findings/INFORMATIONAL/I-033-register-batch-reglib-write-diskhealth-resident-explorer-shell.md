# INFORMATIONAL I-033 — Register batch: three og surfaces with no core counterpart and no prior registration (the RegLib hive-WRITE half, DiskHealth's resident monitoring shell, the Explorer "Recover any deleted data" context-menu integration) + one corpus erratum

**Severity:** INFORMATIONAL
**Module:** reghive, health (product-frame), host/integration (shell), corpus errata
**Parameters affected:** R54/R56 (module boundaries — capabilities outside the engine contract, registered), Q68 (corpus artifact accuracy — the erratum), F59-class (product-frame posture registration)
**Corpus references:** (1) `re-workspace/artifacts/MOM-modules/module-internals-wave2--ldm-reglib-ctbfile.md:73-78` ("The library also **writes/repairs hives**: the 'hbin' (0x6E696268) block constructor stamps `_time64()` timestamps, computes 0x1000-granular bin sizes (`((size >> 12) + 2) * 0x1000` with edge cases), and zeroes new bins — an hbin-allocator for hive modification, not just parsing. Consumers: CheckVersion module (offline Windows-installation discovery) and the BitLocker chain"); (2) `re-workspace/artifacts/DiskHealth/` family + the DiskHealth.exe product tree (`og-codebase/diskhealth/` — RefreshThread periodic refresh, tray-icon guarding, `Click_Open_DiskHealth_By_Notification`, addtask/deltask/querytask.bat); (3) `og-codebase/language/shellexe.ini`-family strings ("Recover any deleted data" context-menu verb, 25 locales); (4) erratum candidate: `re-workspace/artifacts/DiskHealth/` artifact line ~21 attributing auto.bat's `trayicon` argument to DiskHealth (it is changeres.exe's per `artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md:87`)
**Core references:** audited `c7fa527c` — (1) `src/core/reghive/cr_reghive.cpp` (read-only by design: "read-only regf parser + FVEAutoUnlock harvest (S-014)" — the file's own header; no hbin allocator, no write API); (2) `src/core/health/` (scan-time SMART/surface read surface — no resident monitor, no tray/notification IPC, by design for an engine library); (3) no shell-extension surface anywhere in `src/` (the core is a library + CLI; context-menu registration is installer/host territory — `docs/PACKAGING.md`'s domain); (4) `diff-to-fix/CORPUS-ERRATA.md` (the errata register — no row for the DiskHealth trayicon misattribution)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Three og-product capabilities fall outside the core engine's contract and were never registered anywhere on the board, plus one corpus-artifact erratum. This register records them with their scope posture so the board's "every surface dispositioned" claim stays truthful: the RegLib hive-WRITE half (the core's reghive is deliberately read-only; the og's write consumers are CheckVersion's offline discovery and the BitLocker chain — neither maps to a core feature the board has scoped), DiskHealth's resident monitoring shell (a separate product process — the core's health module is the scan-time surface), and the Explorer context-menu verb (installer/host integration). None is a defect in shipped core code; each is a capability the og ships that a product owner should consciously accept-or-scope.

## What the reference does / What our core does

1. **RegLib hive-WRITE half.** Og: an hbin allocator (timestamped, 0x1000-granular, zeroed new bins) for hive modification, per the artifact. Core: `cr_reghive.cpp` is a read-only parser whose landing contract (S-014) is FVEAutoUnlock harvest; the boot-repair family's BCD writes (`cr/bcdfix`) are in-place cell rewrites over an injected writer, not hive growth. Posture: deliberate read-only scope; the write half's named consumers have no core counterpart. Registered here.
2. **DiskHealth resident monitoring.** Og: a Qt product process with a periodic-refresh thread, tray-icon lifecycle, notification-IPC click-through, and task .bat scripts. Core: `src/core/health/` is the scan-time SMART/attribute surface the board has audited (modules/health.md). Posture: product-frame exclusion in the S-038 class (a separate product binary, not an engine capability); the notification/tray machinery is host territory. Registered here.
3. **Explorer context-menu verb.** Og: a shell extension advertising "Recover any deleted data" in 25 locales. Core: no shell surface (correct for a library+CLI); the installer/packaging docs own host integration. Posture: host/installer scope. Registered here.
4. **Corpus erratum.** The DiskHealth artifact's trayicon attribution contradicts boot-config's auto.bat decode (changeres.exe). Filed for the next CORPUS-ERRATA batch so future lanes don't re-derive the misattribution.

## Gap

Not a behavioral gap — a registration gap: prior to this row, none of the four had a disposition anywhere on the board (grep-verified: no "hbin"/"write" row in any reghive context; modules/health.md has no resident-monitoring row; no shell-integration register; CORPUS-ERRATA lacks the trayicon row).

## Impact

None on recovery behavior. The cost of NOT registering: future "mark all remaining" sessions re-discover these surfaces and re-file them as gaps (the exact duplication this register prevents), and the corpus erratum propagates.

## Reproduction / verification

Corpus-derived (this session, first-hand): the four corpus cites above read at their lines; the core-side greps (read-only header, health module census, shell-surface absence, CORPUS-ERRATA census) executed at HEAD. The scratch sheets (`scratch-corpus-rewalk.md` family, promoted to evidence) carry the full claim census.

## Fix plan (for the engineering team)

1. Transcribe this register into the standing docs: a `docs/PARITY-DELTAS.md` D-row for the RegLib read-only scope (the og's write half + its consumers + the rationale), a modules/health.md register note for the resident-monitoring product-frame exclusion, a `docs/PACKAGING.md`-adjacent note for the shell verb (host scope), and the CORPUS-ERRATA row for the trayicon attribution.
2. No code changes. If a future product decision wants resident monitoring or shell integration, the decision lands against these rows with the og's corpus evidence already attached.

## Tests to add

None (registration-only). Optionally: the S-050-proposed docs-CI check (every OOS row's cited registration exists) gains these rows as fixtures.

## Related findings

- S-038 (the app-installer/product-frame precedent), S-050 (the repair-line product-frame finding whose registration pattern this follows), I-027 (the data-asset register this extends), N-143 (the board-truth batch — this row is its registration-half companion), CORPUS-ERRATA (the erratum's home)

## Open questions

None — the postures recorded here are the reviewable claims; acceptance-or-rescope is the product owner's standing call.
