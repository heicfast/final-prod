# S55 liveness battery — the fifth continuation lane's independent verification (convergent)

Session-55 continuation audit lane. Core pin `1fccb0a4` (HEAD at battery run),
corpus `be0cfe9`. Full per-finding tables: `s55-lane-sheets/S55-L1-liveness-tier1.md`
(tier 1: 9 SEVERE + 27 MAJOR) and `s55-lane-sheets/S55-L2-liveness-tier2.md`
(tier 2: 32 MINOR + 1 INFO + 6 tracked-open OQs).

**Method:** every active finding's "Core references" re-located at the pin by
symbol/pattern search (line drift up to ~+1700 lines absorbed), the cited
divergence re-read in the current code, verdict recorded with fresh file:line
cites. Absence findings re-grepped path-scoped. No corpus-side re-verification
(the findings' og sides were already evidence-pinned).

## Tier 1 (36 findings: 9 SEVERE + 27 MAJOR at the pin)

**36/36 STILL-ACTIVE · 0 FALSELY-ACTIVE · 0 PARTIAL.** No fix landed without
bookkeeping in the SEVERE/MAJOR tiers at the pin. Driver-repo trio caveat
(M-086/M-087/M-089): defect files live in heicfast/rda-driver, outside both
repos — "still-active by record"; the core-side leg (release.yml:291) unchanged.

## Tier 2 (33 findings + 6 OQs at the pin)

- **26 STILL-ACTIVE** (incl. N-107/N-127 by evidence-trail default — the
  driver repo is out of sandbox, flagged honestly).
- **4 PARTIAL** — N-154 (N-079 mis-cite live; salvage leg fixed D-116; module
  rows half-landed), N-170 (1 of 8 legs fixed: the N-179 ext-length), N-185
  (code + worklog erratum landed; N-157's attribution line still un-annotated),
  plus the manifest-listing-stale observation on I-035.
- **2 FALSELY-ACTIVE** — N-142-rsf (fixed by N-066's `4d3f0409` BEFORE its own
  filing; later renumbered N-186 and retired by the S50 lane with the same
  verdict — **convergent**); I-035 (retired WITH bookkeeping; only the active
  listing was stale — the S54/S50 windows absorbed).
- **OQs:** OQ-S42-1 RESOLVED-in-substance (pre-existing decode re-verified
  first-hand: $SI-wins-when-present at NTFSSearchDLL:5338-5350); OQ-S42-2
  RESOLVED by first-hand probe (Btrfs nsec FOLDED into FILETIME by the og —
  the standard full-precision conversion; core's `unix_to_filetime_ns`
  matches — MATCH verdict); OQ-S42-3 resolved this session by the D3 decode
  (see the OQ's S55 addendum); OQ-S10-1/S-10-3/S-4-3 remain open per their own
  recorded boundaries.

## Convergence record

The parallel S54 and S50 lanes ran their own batteries over the same window
(S54: "66 active → 55 LIVE / 5 PARTIAL / 6 UNCERTAIN / 1 falsely-active
N-142→N-186 scope-corrected"; S50: "74-finding battery — 63 LIVE / 6 PARTIAL /
N-186 retired false-active / zero phantom-fixed"). This lane's independent
battery at its own earlier pin **converges on every overlapping verdict** —
the falsely-active set is the same finding, the partials name the same legs,
and zero phantom-fixed findings exist in any tier. The hot window's
retirements (S-055..S-061, M-119, M-126..M-130, M-132..M-134, M-137, N-142→N-186,
N-154, N-170, N-171, I-035, OQ-S42-1/2) all landed AFTER this battery's pin —
the battery documents the pin state truthfully; the board moves live.

**Systemic observation (registered, not filed):** citation drift is board-wide
(+1700 lines in cr_analysers.cpp, +350 in rrecov.cpp at the pin vs the
findings' recorded cites). The findings' evidence remains symbol-locatable;
the doc_truth_gate's phantom-citation check (N-150's unexecuted invariant) is
the owning surface for any cite-refresh wave.
