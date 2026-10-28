# MAJOR M-069 — the staged-scenarios lane has never been green: the scenario jobs' array splat bound `-VhdPath` POSITIONALLY (the literal flag string into VhdPath, the vhdx path into the Int32 SizeMB — every run dies at the stage step) + the teardown's fixed-name `rda.sys` removal misses the v0.2.4 versioned image and swallows every failure

**Severity:** MAJOR
**Module / Surface:** CI / the staged-scenarios truth-matrix lane (the prebuilt acceptance proof)
**Parameters affected:** K71-adjacent (the lane's executability), the M-061/M-064 teardown contract
**Corpus references:** none (a core-side defect); the lane is the TRUTH MATRIX (docs/CI-SCENARIOS.md) — the acceptance-proof surface
**Core references:** `.github/workflows/staged-scenarios.yml:155-158` (`$args = @('-VhdPath', "$PWD\disk.vhdx", '-SizeMB', '256', ...)` + `& stage-disk.ps1 @args` — an ARRAY splat binds positionally: `VhdPath` receives the literal `'-VhdPath'`, `SizeMB` (Int32) receives the vhdx path → the conversion error) and `:194-196` (the raw `sc.exe stop/delete` with `2>$null | Out-Null` failure-swallowing + `Remove-Item ...\rda.sys` — the FIXED plain name; the v0.2.4 versioned image is `rda10.sys` — silently missed)
**Execution evidence:** runs 40672083772 + 40673135164 (the SA-6 drills): the install green (`rda10.sys` installed + started + the idempotent start skipped) — then `stage-disk.ps1: Cannot process argument transformation on parameter 'SizeMB'. Cannot convert value "D:\...\disk.vhdx" to type "System.Int32"` → exit 1 → the evidence upload finds nothing. All 5 lane runs failed (3 pre-rewrite + 2 drills); the lane has NEVER produced a green scenario.
**Status:** filed and fixed in the same landing (this session's M-069 commit)

## Summary

The lane that exists to prove the product end-to-end has never run a single
scenario: its stage step passes named parameters through an ARRAY splat
(positional binding — the flag strings become argument values), and its
teardown hard-codes the plain driver name with swallowed failures. The
SA-6 drills surfaced the install-path double-start (M-068) and then this —
the lane's own two defects.

## Fix plan (executed)

1. The hashtable splat (`$p = @{ VhdPath = ...; SizeMB = 256; ... }` +
   `@p`) — named binding, the QuickFormat switch as `$p.QuickFormat = $true`.
2. The teardown = the release's own uninstall.ps1 (the M-061 contract, the
   M-064 versioned-aware sweep), exactly like driver-install.yml.

## Tests to add

- The lane's own green run (the acceptance proof is the test).

## Related findings

- **M-068** (FIXED) — the install-path double-start the first drill surfaced.
- **M-061/M-064** (FIXED) — the teardown contract this lane's step violated.

## Open questions

None.
