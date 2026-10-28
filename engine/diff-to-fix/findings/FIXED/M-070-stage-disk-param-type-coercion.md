# MAJOR M-070 — stage-disk.ps1's `$manifest = Get-Content $Manifest -Raw | ConvertFrom-Json` self-assigns the `[string]`-typed PARAM variable (case-insensitive collision): the parsed PSCustomObject is COERCED TO A STRING on assignment (the param's type constraint persists on the variable) — every scenario seeded 0 files, deleted 0 files, and wrote an empty truth

**Severity:** MAJOR
**Module / Surface:** CI / the staged-scenarios truth-matrix lane (M-069's root cause, the second blocker under it)
**Parameters affected:** the lane's executability (the truth-matrix acceptance proof)
**Corpus references:** none (a core-side defect — a PowerShell language landmine)
**Core references:** `.github/scripts/stage-disk.ps1:29` (`$manifest = Get-Content $Manifest -Raw | ConvertFrom-Json` — `$manifest` IS `$Manifest` case-insensitively; the `[string]` param's TYPE CONSTRAINT persists on the variable, so the assignment coerces the parsed object to its string form — `.files`/`.version` become null)
**Execution evidence:** staged-scenarios run 40673882330 (all 4 scenarios): `staged truth: 0 expected recoverable of 0 deleted`, the truth JSON's `expected_recoverable: null` + `seed_hashes: {}` + `manifest_version: null` (the object became a string). **Reproduced locally in pwsh 7.4.6 with the exact param form**: the param-typed variable coerces on assignment (`$x = @{a=1}` inside `param([string]$X)` → `$x.GetType().Name == 'String'`); the `-Command` form with a differently-named variable does not (which is why the parse "worked" in isolation and the bug hid).
**Status:** filed and fixed in the same landing (this session's M-070 commit)

## Summary

A one-character-case variable collision: the script's `$manifest` and the
param `$Manifest` are the SAME PowerShell variable, and the param's
`[string]` type constraint stays attached to it — so the line that parses
the seed manifest into `$manifest` actually coerces the parsed object to a
string and stores THAT. Every loop over `$manifest.files` iterates zero
times; the lane staged empty disks and wrote empty truths while exiting 0.

## Fix plan (executed)

1. Rename the script variable (`$manifest` → `$seedManifest`) so the
   assignment never touches the param-typed variable.
2. Sweep the other CI scripts for the same pattern (case-insensitive
   variable/param collisions): clean (stage-disk was the only one).

## Tests to add

- The lane's own green run (the M-070 commit's re-dispatch; the truth must
  show 11 seeded / 8 deleted / 8 expected).

## Related findings

- **M-069** (FIXED) — the first blocker under it (the array-splat); this is
  the second, found by the same drill.
- **M-068** (FIXED) — the install-path blocker the drill surfaced first.

## Open questions

None — the mechanism is proven (the local reproduction + the mechanism
isolation).
