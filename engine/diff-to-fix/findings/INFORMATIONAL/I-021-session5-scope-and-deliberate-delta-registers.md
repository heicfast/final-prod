# INFORMATIONAL I-021 — Session-5 scope confirmations and deliberate-delta registers

**Severity:** INFORMATIONAL (grouped registrations)
**Modules:** cross-cutting (win/elevation, transport/WinPE, writer, query, bitlocker)
**Parameters affected:** B1-adjacent, F59/F61/F62, Q46, Q68
**Corpus references:** og PE manifests (verified this session by direct binary parse: DRW.exe + DRWUI.exe = `requireAdministrator`, DRWService.exe = `asInvoker`); `re-workspace/artifacts/WinPE/eusendmessage--wm-copydata-messenger.md` (EuSendMessage = WM_COPYDATA{dwData=999} to window "DRW_Installer" — installer-side only); `re-workspace/artifacts/UIUX/11-recovery-phase-funnel.md` (destination "Enable file repair" toggle); `re-workspace/artifacts/UIUX/09-results-phase-deep-dive.md` (CRecoveryFilter recovery-time filter); `re-workspace/artifacts/WinPE/boot-config--winpeshl-autobat-regdrv.md` (PENetwork SMB destinations); `re-workspace/artifacts/config-files/drwcfg-bin--aes-config-cache.md` (DRWCfg.bin = UI-layer config cache)
**Core references:** `src/win/cr_elevate.cpp` (probe + optional elevation), `src/win/cr_destio.cpp` (destination I/O), query filter model, bitlocker layer — audited commit `7cf6754`
**Status:** READ-ONLY REGISTRATION — no fix applied

## Summary

Session-5's never-cited-artifact sweep surfaced five surfaces whose correct disposition is
REGISTER, not fix. Each is a deliberate difference or a scope confirmation, recorded here so
future sessions do not re-litigate them.

## Registrations

1. **The og ALWAYS runs elevated; the core's optional elevation is a superset.** Direct PE
   manifest parse this session: DRW.exe and DRWUI.exe both demand `requireAdministrator`;
   DRWService.exe runs `asInvoker`. The og has no non-elevated code path at all. The core's
   `cr_elevate` probe + graceful non-elevated degradation (read-only scans without raw
   access) is a deliberate portability superset — register, do not "fix" by demanding
   elevation.
2. **EuSendMessage (WinPE WM_COPYDATA messenger) is installer-only.** dwData=999 to the
   "DRW_Installer" window; zero engine-contract content. Confirms the WinPE scope line —
   the ENGINE-relevant WinPE surfaces remain M-030's environment gate + driver set.
3. **Destination "Enable file repair" toggle (og) vs core's pre-scan-only repair (S-008).**
   The og's recovery funnel can enable repair at destination time; the core's repair hook
   is pre-scan only. Deliberate (S-008's scope); the toggle itself is UI-layer.
4. **CRecoveryFilter (recovery-time filter contract) is client-side in the core.** The og
   pushes filter state into the recovery (write) phase; the core filters at the query layer
   and the writer takes the filtered set. Equivalent semantics, different layer — register
   the layering difference (the og's filter re-evaluation during recovery has no core
   counterpart event, folded into N-043/M-029's addenda).
5. **PENetwork SMB destinations untested.** The og's WinPE build mounts SMB destinations
   via PENetwork; the core's writer handles UNC paths (N-056's test gap). Register: SMB
   destination recovery is a WinPE-lane validation item, not an engine gap.
6. **DRWCfg.bin (AES-encrypted config cache) is UI-layer** — confirmed out of scope (the
   scan engine reads RawConfig/FileFilter/raw_signs/EUUserData, not DRWCfg).

## Action

Register rows in `docs/parity/PARITY-DELTAS.md`; no engine changes.

## Related findings

- M-030 (WinPE gate — the engine-relevant remainder), N-056 (UNC test gap), S-008 (repair
  gating), I-010 (prior scope register)

## Open questions

None.
