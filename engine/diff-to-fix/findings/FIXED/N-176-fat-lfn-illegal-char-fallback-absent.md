# MINOR N-176 — FAT LFN illegal-character fallback absent: the og discards an assembled LFN containing `? * | \ / : " < >` and falls back to the 8.3 alias; the core emits the corrupt LFN as-is

**Severity:** MINOR
**Module:** fat (LFN validation)
**Parameters affected:** filename handling (10), false-positive shaping
**Corpus references:** `FATSearchDLL_decompiled.c:32084-32099` — an assembled LFN containing any of `? * | \ / : " < >` is discarded; the 8.3 alias (with the `'#'` substitution) is used instead
**Core references:** `src/core/fat/cr_fat_quick.cpp:204-215` — `lfn_assemble` validates only the checksum; no character-set gate; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Corrupt-but-checksum-valid LFN remnants (illegal characters that can never appear in a
real Windows name) are rejected by the og in favor of the 8.3 fallback; the core emits
them raw. A surviving corrupt fragment decoding to `re/adme.txt` becomes a core result
name the filesystem could never have contained.

## What the reference does

The illegal-char set gate at the LFN consumption site (cites above).

## What our core does

Checksum-only validation; the name flows through with its illegal characters.

## Gap

LFN decoding to a name with any of the 9 illegal characters: og → 8.3 fallback;
core → the corrupt name emitted.

## Impact

Result-name hygiene on corrupt volumes (names that cannot be restored to disk verbatim
on Windows — the OS rejects them at write time, so recovery of such a name fails or
mangles downstream); bench name-set noise.

## Reproduction / verification

Corpus-derived (the gate + character set decoded with cites). Core-derived (the
checksum-only assembly read directly).

## Fix plan

1. Add the 9-character gate to `lfn_assemble`'s validation (alongside the checksum):
   on failure, fall back to the 8.3 name exactly as the checksum-failure path already
   does.
2. Files: `src/core/fat/cr_fat_quick.cpp` (or cr_lzn.cpp where assembly lives).
3. Tests: one per character class + the fallback assertion.
4. Acceptance: the corrupt-LFN fixture yields the 8.3 alias; valid LFNs unchanged.

## Tests to add

- `Fat.LfnIllegalCharFallsBackTo83` (parametrized over `? * | \ / : " < >`; fails today)
- `Fat.LfnValidNameUnchanged` (regression)

## Related findings

- N-172 / N-175 (the same name family)
- M-130 (the dentry validator family upstream)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-176 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `FatEngine.LfnIllegalCharFallsBackTo83` (RED pre-fix: the checksum-valid corrupt LFN surfaced with its illegal character in the result name for each of the 9 chars; GREEN post-fix: the 8.3 alias serves and no result name carries the char — all 9 iteration sub-cases) + `FatEngine.LfnValidNameUnchanged` (the regression pin: the fixture's own valid 23-char LFN still rides the LFN path). Suite 1218/1218 (linux-release).
**Resolution notes:** The nine-character gate landed inside `lfn_assemble` beside the checksum validation, exactly per the fix plan — a match returns false and the caller takes the same 8.3-fallback path a checksum failure already takes (the corpus's diversion target is the same fallback builder FUN_180036350, FATSearchDLL :32084-32099 re-read first-hand: the strncpy of the 9-byte set `?*|\/:"<>` + the nested compare loop; the matching-branch goto feeds the fallback call). One implementation subtlety preserved: the end-of-name NUL terminator keeps its early-return semantics (an intermediate draft's `break` would have continued to further slots — caught in self-review before commit). The `#` deleted-placeholder path composes (the placeholder is substituted before the builder runs and '#' is not in the illegal set). No safety hardening near the change.
