# MAJOR M-128 — user-sign hex-case semantics: the og matches signs as UPPERCASE-hex text memcmp, so 12 of its own shipped signs are DEAD in the og but LIVE in the core (unregistered superset)

**Severity:** MAJOR
**Module:** carve (user-signature matching semantics)
**Parameters affected:** detection correctness (1), false-positive rate (3), constants/format semantics (19 adjacent)
**Corpus references:** `DRWUI_decompiled.c:1003452`/`:1003555` (the hex-text conversion + memcmp), `FUN_140604090` `:1003619` (the `%02X`-only conversion — zero `%02x` in DRWUI.exe, verified by string sweep at 0x165bfba), `:1005818` (the `_stricmp "signs"` member loop), `:1005986` (malformed-sign skip); the shipped `og-codebase/EUUserData` (12 lowercase-hex signs: mpeg ×11, img ×1 — e.g. `0,000001b321000100`)
**Core references:** `src/core/carve/cr_usersigns.cpp:77-91` (`hex_nibble` accepts both cases), `:257` (decoded-BYTE comparison); the default set `data/userdata-defaults.json` (shipped 111/190, D-104); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's EUUserData consumer converts candidate file bytes to UPPERCASE hexadecimal TEXT
(`%02X`) and memcmps against the stored sign string — any sign containing lowercase hex
can never match. The shipped EUUserData contains 12 such signs (11 mpeg, 1 mixed-case
img): dead configuration in the og. The core decodes hex case-insensitively and compares
BYTES, so all 12 signs match — with the default-on set, the core carves MPEG streams the
og's own signature infrastructure could never fire. The case-semantics divergence is
registered nowhere.

## What the reference does

`FUN_140604090` builds the uppercase hex text; the matcher memcmps it against the stored
sign (`DRWUI:1003452`, `:1003555`). The wide `%02X` format is the only case form present
in DRWUI.exe (zero `%02x` — string sweep). Therefore a stored sign like
`0,000001b321000100` (lowercase b) can never equal the generated `...B3...` text: the 12
lowercase-hex signs in the shipped file are inert in the og. (Their consumer is the CHK
repair identify flow; the og's scan engine has no user-sign channel at all — E-A/E-B.)

## What our core does

`cr_usersigns.cpp:77-91` — `hex_nibble` accepts `'0'-'9'`, `'a'-'f'`, `'A'-'F'`; `:257`
compares decoded bytes. All 190 shipped signs (including the 12 og-dead ones) are live
in the core's user-sign channel, which is default-on (D-104: the CLI loads
userdata-defaults.json at engine init when no `--user-signs` is given,
rrecov.cpp:1228-1264).

## Gap

12 default-set signs produce carve results in the core that the og's own signature DB
semantics can never produce (in any consumer). Falsifiable: an MPEG-PS stream starting
`00 00 01 B3 ...` at a free-space offset → core emits an mpeg user-sign record (tag
0xFFFE0000); the og's EUUserData machinery is structurally incapable of matching that
sign text.

## Impact

Default-scan output differs from the og's own DB semantics: extra MPEG/img carves
(false-positive-shaped for parity benches — every mpeg-sequence-header in free space
becomes a device-end user-sign file). Parity comparisons against og sessions on the same
image will show core-only records in exactly this class. The divergence also masks S-055's
channel-flip effects (mpeg signs that are dead in the og are live here).

## Reproduction / verification

Corpus-derived (lane C first-hand: the `%02X`-only sweep, the two memcmp sites, the
member loop, and the 12 lowercase signs located in the shipped EUUserData — file offset
cites in the lane sheet, evidence/s12-lane-sheets/lane-C-localdb.md). Core-derived (the
hex_nibble + byte-compare reads). PARITY-DELTAS: D-104/M-048 do not discuss case
semantics; no row covers it.

## Fix plan (for the engineering team)

1. Decide the semantics and REGISTER it — two defensible options:
   a. Og-faithful: uppercase-normalize stored signs at parse time (`a-f` → `A-F`);
      the 12 og-dead signs stay dead (a sign with lowercase hex is dead configuration by
      the og's own rules). Recommended for parity benches.
   b. Keep case-insensitive as a deliberate superset and register it in PARITY-DELTAS
      (D-104 addendum) with the 12-sign census.
   Either way the default set's EFFECTIVE sign count must be documented (178 live vs 190
   shipped under (a)).
2. Files: `src/core/carve/cr_usersigns.cpp` (option a: one normalization loop at load) or
   `docs/PARITY-DELTAS.md` (option b — note: doc edits belong to the fix lane, not this
   audit).
3. Tests: below.
4. Acceptance criteria: under (a), a lowercase-hex sign never fires (test pins the
   mpeg class); under (b), the registration cites the census and the parity bench
   normalizes.

## Tests to add

- `UserSigns.LowercaseHexSignDeadIfOgFaithful` (option a) or `...LiveAndRegistered`
  (option b) — one of the two, per the decision
- `UserSigns.UppercaseHexSignMatches` (regression, both options)
- `UserSigns.DefaultSetEffectiveCount` (178 or 190, per the decision)

## Related findings

- S-055 / S-056 (the same channel's ordering + gating defects)
- I-036 (the default-load posture)
- D-104 (the channel registration this rides on)

## Open questions

None — the census and the mechanics are fully decoded; the only open item is the
product decision (a) vs (b).

## Resolution

**Status:** FIXED (option (b) — the deliberate-superset registration; no code change)
**Fixed in commit:** `2b0184f3` (the S47 MAJOR-first fix lane)
**Verified by:** UserSigns.LowercaseHexSignLiveAndRegistered (the og-dead class pinned LIVE: the shipped mpeg sign 0,000001b321000100 decodes and fires) · UserSigns.UppercaseHexSignMatches (the og-live form unchanged, byte-identical to the lowercase twin) · UserSigns.DefaultSetEffectiveCount (190 live — the full default set; the og-dead mpeg-PS family present) — all green; suite green.
**Resolution notes:** The product decision per the fix plan's option (b): the core's case-insensitive byte-decode channel KEPT and registered as PARITY-DELTAS D-125 (the D-117 class — the og's %02X-only text compare [FUN_140604090 at DRWUI:1003619, zero %02x in DRWUI.exe, both memcmp sites :1003452/:1003555 re-read first-hand this session] is an og machinery defect, not a semantic contract: the vendor's own shipped DB carries signs its matcher can never fire). Folding case to reproduce the defect would break good code (the byte-compare channel is architecturally right and is itself part of the D-104-registered channel superset — the og's scan engine has no user-sign channel at all). **Census corrected per the corpus-wins rule:** the finding's "12 lowercase-hex signs (mpeg ×11, img ×1)" is 11 first-hand (10 pure-lowercase MPEG-PS `000001b3…`/`000001ba…` family + 1 mixed-case img sign `7E742C01…7e742c01` — re-counted at og-codebase/EUUserData this session); the core's default set carries the same 11 among its 190. The effective-count documentation the fix plan demanded lives in D-125 (190 live in the core; 179 would be live under og semantics). Parity benches normalize via D-104 + D-125.
