# MINOR N-064 — BitLocker/Office password-dictionary unlock aid absent: the og ships Dict (228 common passwords) wired to manage-bde -Password and encrypted-document repair

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
2. Keep every attempt inside the consented flow; log attempts to the run-log (N-061);
   never auto-attempt on scan.
3. Register the Office/HWPX side as out-of-scope (repair chain, I-010).

## Tests to add

- `Bitlocker.DictionaryUnlockConsentGated` (no attempts without the explicit action)
- `Bitlocker.DictionaryAttemptUsesManageBde` (plumbing test, mocked)

## Related findings

- S-014 FIXED (the key-based unlock this extends)
- I-021 (scope registers — the Office side)

## Open questions

Whether the og's dictionary attempts are rate-limited or ordered (the artifact does not
say) — a fix leg should document its own policy.


## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-064 commit — DFX31)
**Verified by:** 11 pure-core tests (tests/unit/test_dict.cpp: ShippedTableMatchesCorpusShape, ShippedTableKeepsCorpusDuplicates, ParseDictionaryGrammar, ParseToleratesLfAndEmptyLines, ParseRoundTripsTheCorpusGrammar, ParseRejectsBadGrammar, TryStopsAtFirstSuccessInShippedOrder, UserCandidatesFollowShippedDictionary, AllFailReportsEveryAttempt, NullAttemptFunctionIsANoOp, AttemptsLoggedThroughSeamWithoutPasswordMaterial) — fail-before EXECUTED on the loop-disabled tree (the 4 attempt-loop tests RED with attempted=0 / no order / zero log lines; implementation restored -> 11/11 GREEN) — plus 3 win-lane contract tests (tests/unit/test_bitlocker_win.cpp: DictionaryAttemptUsesManageBde, DictionaryUnlockConsentGated, DictionarySeamOrderStopAndWideConversion; Windows CI lane, win_syntax_check parse-clean). Suite 754/754 linux-release at the landing tree.
**Resolution notes:** Per the finding's own fix plan. (1) The dictionary: the corpus Dict file (og-codebase/Dict, 2,742 B, 228 comma-terminated CRLF lines, pure ASCII, lengths 2..20, seven duplicate lines kept byte-faithfully - password/clinton/administrator/admin/super/user x2, cola x3) converted to a generated compiled table by scripts/gen_dict.py (the repo's raw_signs.json conversion precedent; numeric-initializer pool, escape-proof) -> src/core/recovery/cr_dict_data.cpp. (2) The pure-core module cr::dict (include/cr/recovery/cr_dict.h): parse() implements the Dict file grammar (comma-terminated lines, CRLF/LF, final line unterminated, non-printable-ASCII rejection, 32-char/4096-candidate hostile-blob caps); try_passwords() is the consent-gated attempt loop - shipped set in corpus order first, then user candidates, stop at first OK, honest AttemptResult stats, every attempt reported through the N-048/N-061 cr::log seam as "dict unlock attempt %u/%u" Info lines carrying the INDEX ONLY (the og's qa-logs channel carries no password text either - verified; password material is never logged, pinned by AttemptsLoggedThroughSeamWithoutPasswordMaterial). (3) The attempt policy the finding's open question asked for, documented in the header: sequential, no parallelism, no retry, dictionary-then-user ordering. (4) The win layer (cr_bitlocker.h/.cpp): unlock_with_dictionary() - the EXPLICIT CONSENT ACTION riding the SAME PowerShell Unlock-BitLocker -Password primary / manage-bde -unlock -Password legacy fallback plumbing as a user-typed password (the command grammar extracted unchanged into detail::build_unlock_command for shape tests); nothing in the scan/open path calls it - open_volume_view refuses locked volumes with ERR_FVE_LOCKED until the user passes the new CLI --bitlocker-dict flag (cmd_scan pre-pass, valueless, the og's user-initiated dialog analog), and the guidance message now advertises the aid. (5) The cr_ui ABI: cr_ui_unlock_volume_with_dictionary(letter, extra_utf8, count) - additive export, no version bump (the M-033 precedent), the host's explicit consent surface; ASCII candidate refusal mirrors the win layer. (6) The Office/HWPX consumer stays out-of-scope per the finding's own registration (repair chain, I-010 - FIXED/ scope register). Safety hardening preserved: F61 legitimate-unlock-only is the design center - the consent gate is structural (separate entry point + CLI flag + ABI action), and the auto-unlock/scan paths attempt passwords never (pinned by DictionaryUnlockConsentGated on the win lane).
## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-064 commit — `git log` "fix(N-064)")
**Verified by:** 9 `Pwddict.*` tests (ParsesOgeDictFormatByteFaithfully, ParseAcceptsLfAndOptionalComma, ParseEnforcesCapsAndRefusesOversize, SeedDictionaryIsSane, TryPasswordsStopsAtFirstSuccess, TryPasswordsDedupsPreservingOrder, TryPasswordsExhaustsToLockedAndGuardsArgs, AttemptLogCarriesProgressAndLengthNeverSecrets, ExhaustLogsWarnOnce); executed fail-before RED (the test unit does not compile on the pre-fix tree — `fatal error: cr/pwddict/cr_pwddict.h: No such file or directory`, the capability absent); suite **752/752** linux-release incl. win_syntax_check (the gate itself caught the first win-layer draft's undeclared MultiByteToWideChar/CP_UTF8 — stub surface grown, the catch recorded as the gate working as designed).
**Resolution notes:** CONFIRMED-DIVERGENCE, re-verified first-hand against the corpus this session before any change: og-codebase/Dict byte-read (2,742 B, 228 entries, comma-terminated CRLF lines with the unterminated final entry, longest 20 chars, entries may contain spaces — "charlie brown"/"mickey mouse" verified directly), config-data-reference.md:20/:86 (Dict -> BitLockerLib manage-bde -Password + EuOfficeRepairFile), UIUX 15-scenario-flows:33 (the user-consented BitlockerUnlockDialog -> BitlockerUnlockThread flow). Fixed per the finding's own plan: (1) a pure module `cr/pwddict` (parse in the og's exact comma-terminated format + seed list + the consent-gated sequencer with order-preserving dedup, caps 4096/128/256KiB, and N-048-seam attempt logging that carries progress and candidate LENGTH only — password text never logged); (2) the win plumbing `bitlocker::try_password_unlock` riding the SAME unlock() path as a typed password (PowerShell Unlock-BitLocker -Password, manage-bde -Password fallback — Windows applies the key, the core performs no crypto), UTF-8 widen CP_UTF8 per the repo text policy; (3) the explicit host consent action — CLI `scan --bitlocker-pwddict <path|seed>` (the flag IS the consent; `seed` selects the shipped list, a path loads any dictionary in the og's format — the og's own Dict file works verbatim); nothing in the scan pipeline or orchestrator references the sequencer (never auto-attempt on scan, F61 preserved). REGISTERED DIVERGENCE **D-78** (docs/PARITY-DELTAS.md): the core ships an authored 20-entry generic seed, NOT the og's 228-entry asset (never copy proprietary data files) — the loader is byte-format-compatible so a host pointing at the og's file gets the exact behavior. The finding's open question (ordering/rate-limiting) resolved as the documented policy: sequential, first-success-wins, one attempt per candidate, duplicates attempted once. The Office/HWPX + archive-repair Dict consumers stay scope-excluded per I-010 (recorded in the finding and D-78). F61 consent posture and the S-014 key-based paths unchanged; locked volumes without user consent/key stay refused ERR_FVE_LOCKED.
