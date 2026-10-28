# MINOR N-174 — exFAT `._`-prefix sink filter absent: the og drops macOS AppleDouble sidecar files; the core emits them (the N-028 family's exFAT twin, never decoded before)

**Severity:** MINOR
**Module:** exfat (entry filtering)
**Parameters affected:** detection correctness (1) — sink/exclusion set, false-positive shaping
**Corpus references:** `exFATSearchDLL_decompiled.c:45151` (`strncmp((char*)(param_2 + 0x29), "._", 2)` — names starting `._` skip the props build) + `:45222` (`return 0` — the entry never surfaces); single site in the DLL, first-hand verified by the lead auditor
**Core references:** `src/core/fat/../exfat/cr_exfat_quick.cpp:555-800` (the entry walk — no name-prefix gate); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's exFAT engine silently drops entries whose name starts `._` (macOS AppleDouble
sidecars written by Finder on exFAT volumes). The core emits them. This is the exFAT
mirror of the HFS+ sink filter N-028 registered earlier (dotfiles + "HFS+ Private
Data") — the exFAT side was never decoded until this session.

## What the reference does

The strncmp gate + the return-0 (cites above — verified first-hand).

## What our core does

No prefix gate; `._photo.jpg` surfaces as a recoverable entry.

## Gap

An exFAT volume written by macOS: og results omit `._*`; core results include them.

## Impact

Result-set shape on mac-written exFAT media (very common: camera cards, USB sticks
formatted exFAT and touched by macOS): the core carries metadata-only sidecars the og
sinks — count inflation in parity benches, and noise for users. Recoverable content is
never lost (the core is a superset).

## Reproduction / verification

Corpus-derived (lead auditor read the strncmp + return-0 directly). Core-derived (the
walk's gate census — no name-prefix check). Registry: N-028 covers the HFS+ family only.

## Fix plan

1. Add the `._` prefix gate to the exFAT entry walk (one check beside the name decode),
   matching the og's silent drop; register in PARITY-DELTAS beside N-028's family if
   the team prefers keeping them (not recommended — the og's sink is deliberate noise
   suppression).
2. Files: `src/core/exfat/cr_exfat_quick.cpp`.
3. Tests: the sidecar fixture below.
4. Acceptance: `._foo` entries absent from results; regular dotfiles unaffected (the og
   gate is prefix-specific).

## Tests to add

- `Exfat.AppleDoublePrefixSunk` (fails today)
- `Exfat.RegularDotFilesNotSunk` (no over-blocking)

## Related findings

- N-028 lineage (the HFS+ sink family this mirrors)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-174 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** `ExfatEngine.AppleDoublePrefixSunk` (RED pre-fix: the `._photo.jpg` sidecar surfaced and `apple_doubles_sunk` stayed 0; GREEN post-fix: the set is skipped at the walk, the counter reads 1, and the described `photo.jpg` still surfaces) + `ExfatEngine.RegularDotFilesNotSunk` (the over-blocking pin: `.hidden` and `_notes.txt` both surface, counter 0 — the og's gate is prefix-specific) + the full exFAT battery 36/36 (1 env-skip). Full suite green (linux-release).
**Resolution notes:** The gate landed in the entry walk immediately after name assembly, before the recycle-bin pairing and the props build — the same position the og's `FUN_180036660` holds it (the strncmp at exFATSearchDLL :45151 refuses the props build; the `return 0` at :45222 means the entry never surfaces; both cites re-read first-hand this session against the corpus tree at `be0cfe9`). A `apple_doubles_sunk` counter was added to `ExfatScanStats` so the silent sink stays observable (the og is fully silent; the counter is a deliberate observability superset, consistent with the core's stats-truth posture). The M-013 parked-set oracle re-walks through the same `walk_entries` path, so reconstructed sets cannot bypass the gate. No safety hardening was near the change.
