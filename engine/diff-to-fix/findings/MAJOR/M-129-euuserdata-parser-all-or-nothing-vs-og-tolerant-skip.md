# MAJOR M-129 — EUUserData parser strictness: the og tolerates unknown members, case-insensitive keys, and malformed signs (skip-and-continue); the core is all-or-nothing — og-loadable files are wholesale-rejected

**Severity:** MAJOR
**Module:** carve (user-signature loading)
**Parameters affected:** error semantics (23 adjacent), detection completeness (1), forward compatibility
**Corpus references:** `DRWUI_decompiled.c:1005818` (the member loop acting only on `_stricmp "signs"` — unknown members ignored, "mime" itself never read), `:1005986` (malformed signs silently skipped), `:1003452` (odd-length hex loads but never matches — guard `(hexlen>>1)*2 != hexlen`), atoi-based offset decode (mis-parses gracefully, "0x10," → 0)
**Core references:** `src/core/carve/cr_usersigns.cpp:174-221` (the strict reader: unknown key → reject :206-207, duplicate mime → reject :204, odd hex → reject :110, malformed sign → reject :194-195, trailing garbage → reject :220-221, empty signs → reject :201-202, exact-case "signs" :174); CLI abort path `src/app/rrecov.cpp:1285-1289`; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's EUUserData loader is tolerant: unknown members in an extension object are
ignored (the parser only acts on "signs", case-insensitively), malformed sign strings are
silently skipped, odd-length hex loads but simply never matches. The core's reader is
all-or-nothing: any unknown key, duplicate "mime", odd hex, malformed token, or trailing
garbage rejects the WHOLE file with ERR_BAD_PARAMETER. A hand-edited or future-versioned
EUUserData that the og loads (partially) is rejected entirely by the core — the CLI then
runs with zero user signs (silent degrade) or aborts an explicit `--user-signs` load.

## What the reference does

`DRWUI:1005818` — the member loop dispatches on `_stricmp(name, "signs")` only; every
other member (including "mime" — never read by any og consumer) falls through silently.
`:1005986` — a sign string that doesn't split into exactly 2 comma tokens is skipped,
parsing continues. `:1003452` — odd-length hex is accepted at load; the matcher's length
guard makes it permanently unmatchable (dead entry, not a load failure). Offsets decode
via `atoi` (decimal; garbage → 0).

## What our core does

`cr_usersigns.cpp` — the Reader fails the entire parse on: unknown object key
(:206-207), duplicate "mime" (:204), odd-length hex (:110), token-count≠2 (:194-195),
empty signs array (:201-202), trailing garbage (:220-221); the "signs" key itself is
exact-case (:174). On ERR_BAD_PARAMETER: default-load → zero signs (silent);
`--user-signs` → the CLI aborts (rrecov.cpp:1285-1289).

## Gap

For any EUUserData file that is valid-for-the-og (tolerated members/case/signs) but
violates any core strictness rule: og loads (possibly partially), core rejects entirely.
Falsifiable: add one future member `"size": 1234` to a valid file → og: unchanged
behavior; core: ERR_BAD_PARAMETER, zero signs loaded.

## Impact

Forward-compatibility and operational availability: users who hand-edit the DB (the only
writer — no og or core writer exists) or carry a newer-format file lose the ENTIRE
user-sign channel in the core, silently (default load) or loudly (explicit load). The
honest-rejection doctrine is a deliberate core posture (S-039 lineage) — but the og's
tolerance semantics are the reference behavior for this file format, and the divergence
is unregistered.

## Reproduction / verification

Corpus-derived (lane C first-hand: the member loop, the skip site, the odd-hex guard —
cites above). Core-derived (the reject-site list read directly). Constructed: a valid
file + one extra member → core ERR_BAD_PARAMETER (mechanically verifiable from the
Reader's dispatch). PARITY-DELTAS: S-039/M-067 registered the CAPS (kMaxEntries etc.),
not the strictness semantics.

## Fix plan (for the engineering team)

1. Align the loader with the og's tolerance where it is safe, and keep honest rejection
   where the og has no analog:
   - unknown members → SKIP (og parity), do not reject;
   - member names case-insensitive (`_stricmp` semantics);
   - malformed signs (token count ≠ 2) → skip the sign, keep the file;
   - odd-length hex → skip that sign (the og keeps it as permanently-dead; skipping is
     equivalent-in-effect and simpler — register the difference in one line);
   - KEEP rejection for: unterminated strings/structures (a torn file is not
     og-loadable either — the og's JSON layer would fail it).
2. Files: `src/core/carve/cr_usersigns.cpp` (the Reader's member dispatch + sign loop).
3. Tests: below.
4. Registration: one PARITY-DELTAS line noting the skip-vs-dead-entry nuance.
5. Acceptance criteria: the og-shaped corpus quirks file (the shipped EUUserData verbatim
   — 112 blocks, duplicate asf, 12 lowercase signs, the 8-char "manifest" key) loads
   with zero errors; a future-member file loads with the member ignored.

## Tests to add

- `UserSigns.UnknownMemberIgnored` (fails today)
- `UserSigns.MemberNamesCaseInsensitive` (fails today)
- `UserSigns.MalformedSignSkippedNotFatal` (fails today)
- `UserSigns.OddHexSkipped` (fails today)
- `UserSigns.TornFileStillRejected` (regression: the honest-rejection floor)
- `UserSigns.ShippedOgFileVerbatimLoads` (the 112-block/191-sign census, N-169 interplay)

## Related findings

- N-169 (the duplicate-asf merge semantics — the same file's quirks)
- M-128 (the case semantics of the SIGNS themselves — distinct from member-name case)
- S-039 lineage (the caps doctrine this must not violate)

## Open questions

None.
