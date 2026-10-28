# WORKLOG — api-fix session 2 (API-scope remediation)

Method: context-recovery protocol, API scope only. Three parallel read legs
(corpus / core / driver) -> `/home/z/my-project/api-remediation/context/
{research,core,driver}-api-inventory.md`; delta board in
`diff-to-fix/API-DELTA.md` (§4, session 2). Corpus `03ad46c`; core base
`c899998`; driver `f569836` (v0.2.3 deployer batch landed upstream).

## Done (this session)

1. Delta verification pass, first-hand, zero-invention:
   - Driver wire re-verified byte-exact vs `rda_ioctl.h`: identity
     0x60002400/04/08, extended 0x6002440/44/48, all 19 session codes,
     F11 Information x512. MATCH.
   - cr_ui recount: 82 CR_UI_API hits - 3 macro defines = 79 exports;
     API.md claims 79. MATCH.
   - Error-model recount: 39 ERR_* + OK. MATCH.
   - M-064 status re-check: upstream rda-driver v0.2.3 (6b50bb8) landed the
     Parameters write+verify half (install.ps1:91-103); -Uninstall verb /
     %s%d versioned layout / per-call SCM error strings still absent
     upstream -> remainder stays the driver-scripts lane, no core action.
2. NEW drift found + verified (fix items A-D, see API-DELTA §4):
   - A: docs/crsf-format.md omits REC_FS_CHECK type 9 (M-041) — list AND
     payload grammar section missing.
   - B: rrecov.cpp header comment lists 8 of 10 verbs (missing
     list-carve-formats, apply); scan line missing --image/--deep/
     --repair-undo/--user-signs; recover line missing --image; stray `*`
     artifact on the bitlocker continuation line.
   - C: docs/API.md §1 lists 8 of 10 verbs (missing list-carve-formats,
     apply); scan line missing --image/--deep/--repair-undo and carries a
     phantom `[plan]` value on --repair (bare flag in code); recover line
     missing --image; exit-code table has a duplicate row 3.
   - D: docs/CORE-API-CURRENT.md §c intro says 9 verbs (omits driver) with
     stale wmain cite; c.7/c.8/c.9 document the REMOVED --no-rda flag;
     c.9 claims the "direct" transport label is reported (driver-only
     "rda" is the only label, rrecov.cpp:1992); the c.9 doc-drift note
     cites API.md's already-fixed `info`-verb state as current; §c
     function-range cites stale throughout.
3. Fixes landed: A (crsf-format.md), B (rrecov.cpp comment), C (API.md),
   D (CORE-API-CURRENT.md §c truth pass). See API-DELTA §4 ledger.

## Next

- Items that remain after this session (none of them core API-surface
  code): S-038 steps 2-4 (packaging lane, Windows installer deliverable),
  M-064 upstream remainder (driver scripts lane), A-ABI-6 (owner policy
  decision), OQ-SA-1 / OQ-S4-3 (engine-scope probes, tracked-open).

## Current API item

None open in the API lane — the board is clean pending the next corpus
refresh. Watch list: any new verb/flag/export lands must update
docs/API.md + CORE-API-CURRENT §c + the rrecov.cpp header comment in the
same change (the three-site verb-list rule, pinned this session).
