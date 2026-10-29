# S54 liveness battery — the reconciliation record (pin drift documented)

**Lane:** session-54 continuation audit (the fifth continuation lane of the user's
second-session mandate). **Batteries run:** three parallel agents (SEVERE+OQ,
MAJOR, MINOR) over the then-active board, every verdict evidence-cited; the three
agent sheets are beside this file (`s54-liveness-battery-{severe,major,minor}.md`).
**Pin drift:** the batteries pinned `2794da8b`; the fix lanes' hot window moved the
board DURING verification (to `34d6cf10` at filing time) — the movement itself is
the battery's most useful output. All findings the window retired were absorbed;
no verdict below contradicts a retirement.

## Battery results at the battery pin (2794da8b — 66 active)

- **SEVERE (9):** 8 LIVE + 1 PARTIAL (S-050: the capability leg live, the
  false-citation leg already repaired at filing). 0 falsely-active.
- **MAJOR (26):** 23 LIVE + 3 UNCERTAIN (M-086/M-087/M-089 — the fix surfaces live
  in heicfast/rda-driver, out of the two-repo grant per OQ-S10-3; M-089's core-side
  residue is filed as **N-195** this session). 0 falsely-active. The battery also
  flagged the FIXED/M-133 ID-collision hazard (a different retired finding reused
  the M-133 ID — automated ID-matched retirement would wrongly retire the active
  twin-key M-133; recorded here for the fix lanes).
- **MINOR (31):** 24 LIVE + 4 PARTIAL (N-150, N-154, N-170, N-185 — each with the
  exact remaining-legs list in the agent sheet) + 1 FALSELY-ACTIVE (**N-142** —
  see below) + 2 UNCERTAIN (N-107/N-127, the rda-driver boundary).
- **Tracked-open (6):** all still `Status: OPEN`; OQ-S4-3's transport half is
  MEASURED (alignbench 40315255368), the TTFR half remains.

## The N-142/N-186 reconciliation (the one falsely-active, resolved as a scope correction)

The battery flagged N-142 (the RSF variant device-descriptor fields) as
FALSELY-ACTIVE: the N-066 v4 tail (`firmware[32]`, `wwn[32]`, `interface_desc[32]`,
`boot_snapshot[512]`) landed in `VolumeInfo` (`include/cr/session/cr_records.h`)
BEFORE the finding's own filing pin — the finding read `VolumeSessionInfo` (the
restored-summary struct, which does NOT mirror the v4 tail) instead of the
persisted record. Mid-verification, the S56 lane renumbered the finding
**N-142 → N-186** (the remote's CrPreview N-142 retired first). The lead's
resolution: a **scope-correction addendum on N-186** (not a full retirement — the
restored-summary mirror gap is live, though narrow) + the modules/session.md
parity-table row correction queued. See the finding file's addendum.

## The window's retirements absorbed between the pins (2794da8b → 34d6cf10)

SEVERE −5 (S-055, S-056, S-057, S-058, S-061 — the S51 severe lane landed them);
MAJOR −8 (M-118, M-126..M-136 band, M-137 — the S52 engine-math lane); MINOR −13
(N-142→N-186 renumber excluded; the S46/S53/S56 minors lanes); INFO 2→0. Board at
the filing pin: **4 SEVERE · 18 MAJOR · 18 MINOR · 0 INFO = 40 active + 104 FIXED
+ 6 tracked-open** — the batteries' LIVE verdicts on the retired set were correct
at their pin and are superseded by the retirements (convergent, the S41 precedent).

## The battery's standing hazard notes (for the fix lanes)

1. The FIXED/M-133 ID collision (above).
2. The bench phantom-zero IOCTL residue (now N-195).
3. The line-drift map for the long-lived findings (cr_carve.cpp +146,
   cr_ui.cpp cancel path +44, rrecov.cpp verb dispatch :3458-3492, cr_ui.h export
   census 81→89 — the SEVERE sheet carries the full list; re-pin before fixing).
4. M-089's driver-side work stays OQ-S10-3; the core-side half is N-195.
