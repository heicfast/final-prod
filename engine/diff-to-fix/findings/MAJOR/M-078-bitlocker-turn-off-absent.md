# MAJOR M-078 — BitLocker turn-off absent: the og's CBitlockerTurnOffOperation step cannot be performed from the core

**Severity:** MAJOR
**Module:** health (BitLocker) / win layer / ABI / CLI
**Parameters affected:** F61 (BitLocker handling — the legitimate-only stance must extend to a consented destructive op), P42 (new export), C24 (CLI exit codes)
**Corpus references:** `re-workspace/artifacts/DRWService-exe--bitlocker-service.md:83-88` (§4 downstream consumers: BitLockerLib.dll — PowerShell `manage-bde -unlock … -RecoveryPassword` / `Unlock-BitLocker -MountPoint … -Password $SecureString` + registry fallbacks; DRWUI classes `BitlockerDevice`, `CCheckBitlockerPartition`, `CBitlockerStartWnd`, `BitlockerUnlockDialog`, `BitlockerUnlockThread`); `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md:78` ("Operation pattern: `CBaseOperation` → … `CBitlockerTurnOffOperation(+Pending)`"); `re-workspace/artifacts/PartitionRecovery/pending-operation-engine.md:62-63` ("**BitLocker interplay**: CBitlockerTurnOffOperation + bitlockerhelp.mo + BitLockerLib.dll (pause/decrypt flows around recovery of encrypted volumes)"); `re-workspace/artifacts/per-binary/MainModule.md:28-29` (the `CBitlockerTurnOffOperation` ctor exports)
**Core references:** audited `35adcb9`. `src/win/cr_bitlocker.cpp` (509 lines): `is_fve_volume` (:112), `status` (:116), `unlock` (:138-149, PowerShell primary + manage-bde fallback), `unlock_with_dictionary` (:158-190, N-064), FVE auto-unlock harvest (live :444-459, offline :460-494, `try_auto_unlock_offline` :496), command builder `detail::build_unlock_command` (:320-339). **No turn-off/decrypt function.** `rg -n "turn_off|TurnOff"` over `src/`+`include/` → 0 code hits. `cr_ui_turn_off_bitlocker` absent from the 81-export census. Board row: `docs/REFERENCE-API-GAP.md:331` (the gap itself named).
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og coordinates BitLocker state around recovery with a dedicated transactional operation — `CBitlockerTurnOffOperation` in MainModule's pending-op queue (with bitlockerhelp.mo and BitLockerLib.dll performing pause/decrypt flows). The core's BitLocker surface is detect/unlock/auto-unlock only: no export or win-layer function disables (turns off / decrypts) BitLocker on a volume, so the tail of the unlock chain is unreachable from the core.

## What the reference does

The og's recovery-time BitLocker interplay is a pending-operation-engine transaction: `CBitlockerTurnOffOperation` (+Pending) rides the `CBaseOperation` queue (mom-module-system:78), coordinated with bitlockerhelp.mo and BitLockerLib.dll (pending-operation-engine:62-63 — "pause/decrypt flows around recovery of encrypted volumes"). The unlock chain itself uses PowerShell `Unlock-BitLocker -MountPoint` / `manage-bde -unlock` with recovery password or secure-string password, plus registry fallbacks (DRWService:83-88). The exact verb the turn-off operation issues (manage-bde `-off` vs an FVE API call) is not decoded in the corpus — its existence, transactional framing, and plumbing are.

## What our core does

`cr_bitlocker.cpp` implements detection, status, dual-path unlock (PowerShell primary, manage-bde fallback — the og's exact command duality, N-064's dictionary unlock), and both live and offline-hive FVE auto-unlock harvesting (S-014). No function turns BitLocker off. The unlock command builder (:320-339) has no `-off` mode. The ABI has no export; the CLI has no flag.

## Gap

No export or win-layer function disables BitLocker on a volume: `cr_ui_turn_off_bitlocker` is absent and `cr_bitlocker.cpp` implements only detect/unlock/auto-unlock. Falsifiable: a locked volume that the og would pause/decrypt around recovery can only be unlocked (not turned off) via the core.

## Impact

A recovery-workflow step the reference coordinates is unavailable — users on TRIM-enabled/encrypted volumes lose the og's decrypt-before-recovery option and must manage BitLocker out-of-band. The core's legitimate-unlock-only stance (D-16) is intact and this finding does not touch it: turn-off is a consented, user-initiated destructive operation, not a bypass.

## Reproduction / verification

- Corpus-derived (the four citations above; the operation's existence + coordination decoded, the wire verb not).
- Core absence first-hand: the cr_bitlocker.cpp function census read; greps zero; export census.

## Fix plan (for the engineering team)

1. `src/win/cr_bitlocker.cpp`: add `turn_off(wchar_t letter)` — `manage-bde -off <vol>:` with the same PowerShell/manage-bde duality as `unlock` (extend `detail::build_unlock_command` with an `-off` mode; SFC-safe fixed-format command, no shell-injection surface — reuse `run_command` :53-77). Driver class (b) (no driver check, per the plan).
2. ABI: `cr_ui_turn_off_bitlocker(char letter, cr_ui_progress_fn progress, void* user)` — explicit opt-in only, progress surfaced, refusal on failure, never silent (the destructive-op contract).
3. CLI: `unlock --turn-off C:` (absorbed by the existing verb family — three-site lock on the flag).
4. Tests: command-construction + rc-mapping unit (the seam pattern from the unlock tests); a Windows scenario is optional with the PARITY-DELTAS honest caveat (BitLocker provisioning out-of-reach on runners).
5. Docs: ABI-CONTRACT §7, PARITY-DELTAS row for the new posture, CORE-API-CURRENT.

## Tests to add

- `Bitlocker.TurnOffCommandConstruction` (letter → the exact manage-bde -off line, PowerShell dual form).
- `Bitlocker.TurnOffRefusesOnFailure` (no silent success).
- `Bitlocker.TurnOffNeverSilent` (progress/refusal surfacing).

## Related findings

- S-014 (FIXED — the FVE harvest chain this operation coordinates with).
- N-064 (FIXED — the dictionary unlock; the same builder this plan extends).
- D-16 (the legitimate-unlock-only register — unaffected, cited to preempt misreading).

## Open questions

The og's exact turn-off verb (manage-bde `-off` vs FVE API) is undecoded — the fix plan uses the unlock chain's own duality as the evidence class; if a corpus decode recovers the verb, re-pin.
