# MINOR N-061 — BitLocker/Office password-dictionary unlock aid absent: the og ships Dict (228 common passwords) wired to manage-bde -Password and encrypted-document repair

**Severity:** MINOR
**Module:** health→bitlocker family (unlock aid) + repair (out-of-scope side registered)
**Parameters affected:** F61 (BitLocker handling — legitimate unlock only: a dictionary aid is user-consented unlock, not a bypass), B1-adjacent (capability), Q46 (docs)
**Corpus references:** `re-workspace/artifacts/config-files/config-data-reference.md:20` ("### Dict (2,742 B, 228 entries — byte-verified) — password brute-force dictionary"), `:86` ("Dict ties to BitLockerLib (manage-bde -Password) + EuOfficeRepairFile (encrypted HWPX)"), `og-codebase/Dict/` (the file itself — 228 newline-separated common passwords, read this session: 12345678, password, qwertyuiop, iloveyou, …)
**Core references:** `src/win/cr_bitlocker.cpp:23-28` (the core's BitLocker surface reads FVEAutoUnlock keys only — no password-attempt path), negative grep for any dictionary/password-attempt surface — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og ships a 228-entry password dictionary used as a legitimate unlock aid: BitLocker
volumes via `manage-bde -Password` attempts and encrypted Office/HWPX documents in the
repair chain. The core's BitLocker unlock is key-based only (FVEAutoUnlock live + offline
hive, per the S-014 fix) — no password-attempt path exists. The Office-repair side is
out-of-scope (repair chain, I-010) and is registered here only for completeness.

## What the reference does

`config-data-reference.md` byte-verifies Dict and wires it to BitLockerLib's
manage-bde -Password flow and EuOfficeRepairFile. The dictionary is common-passwords
(228 entries; contents quoted in Corpus references).

## What our core does

Key-based unlock only. No dictionary, no password attempts, no user-password prompt path
in the engine (the CLI/UI layer has none either).

## Gap

Falsifiable: a BitLocker volume whose password is a common password (e.g. "password123"
family) unlocks in the og's flow after dictionary attempts; the core cannot unlock it by
password at all (key-based paths only).

## Impact

Convenience capability gap on a narrow population (users who set trivially-common
passwords). Any port MUST keep the consent gate: dictionary attempts only inside the
user-initiated unlock flow (parameter F61 — legitimate unlock only; the og's flow is
user-consented). MINOR.

## Reproduction / verification

Corpus file read + config-data-reference lines read first-hand this session; core
negative-space verified.

## Fix plan

1. If pursued: add a user-consented password-attempt unlock path (explicit UI/CLI
   "unlock with password" action) that tries the shipped dictionary then user-provided
   candidates via the existing manage-bde/PowerShell layer (S-014's plumbing).
2. Keep every attempt inside the consented flow; log attempts to the run-log (N-058);
   never auto-attempt on scan.
3. Register the Office/HWPX side as out-of-scope (repair chain, I-010).

## Tests to add

- `Bitlocker.DictionaryUnlockConsentGated` (no attempts without the explicit action)
- `Bitlocker.DictionaryAttemptUsesManageBde` (plumbing test, mocked)

## Related findings

- S-014 FIXED (the key-based unlock this extends)
- I-019 (scope registers — the Office side)

## Open questions

Whether the og's dictionary attempts are rate-limited or ordered (the artifact does not
say) — a fix leg should document its own policy.
