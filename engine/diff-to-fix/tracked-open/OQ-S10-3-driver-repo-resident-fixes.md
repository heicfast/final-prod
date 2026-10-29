# OQ-S10-3 — The driver-repo-resident fixes pushed forward (renumbered from the OQ-S10-2 draft at the S11-lead reconciliation: the videoviewer OQ-S10-2 (added 0e279e43, 17:32:28Z) pushed 28 minutes before this file (0a08cb3c, 18:00:27Z) and keeps the ID) (M-086, M-087, M-089, N-107, N-127's driver halves)

**Status:** OPEN (session-10, this lane) — spun out of the two-repo grant boundary
**Module:** transport / driver install lifecycle / driver wire contract
**Parameters affected:** K76 (versioned coexistence), K78 (WinPE UpperFilters), C22 (IOCTL probe set), Q68 (driver doc truth)

## The scope boundary

This session's grant covers exactly two repositories: core_recovery (modified) + easeus-drw-research (read-only corpus). Five findings' ENTIRE fix surfaces live in heicfast/rda-driver (a third repository, explicitly out of the grant this session — session-9's grant covered it; this session's does not):

1. **M-086** — rda_version.h's 1.0.0.0 pin + the unstamped release workflow + install.ps1's FileVersion-derived tag (every v0.2.x release collides at exit 2).
2. **M-087** — scripts/winpe/regdrv-rda.reg's invalid hex(7) UpperFilters (the WinPE filter never attaches); the one-line byte fix + the .reg parse gate.
3. **M-089** — rdabench.c's phantom-zero 0x6000244x probe list (the live extended IOCTL handlers get zero fuzz coverage); derive the probes from the header + the static dispatch-coverage gate.
4. **N-107** — the 14 stale "15/18-command" prose sites vs the code's 19.
5. **N-127** — the six doc-vs-code truthfulness defects (the admin-gate claim with no code, the stale sector prose, the unreachable default, the S12 56/57 checks claim).

## The next concrete step (the driver-repo lane)

One driver-repo lane takes all five (they share the read): M-087 first (one line + the gate), then M-089 (probe derivation + the gate), then N-107/N-127 (the prose sweeps), then M-086 (the version-stamping decision + the two-version coexistence drill on a windows runner). Each with the fail-before/pass-after proof per the house SOPs.

## Core-side verification done this session

The core-side evidence for each finding re-verified at the current tree: release.yml's -FloorVersion v0.2.4 consumption (M-086's core half is sound), the corpus's correct regdrv.reg contrast (M-087), and the driver-contract doc's wire-code rows (M-089/N-107/N-127's core-side citations).
## Session-40 disposition note (2026-10-29)

Boundary re-affirmed: this session's grant covers exactly core_recovery (modified) +
easeus-drw-research (corpus) - the five findings' fix surfaces (M-086/M-087/M-089/
N-107/N-127) remain in heicfast/rda-driver and CANNOT be touched without violating
the grant (the same class as the OQ's original record). The core-side halves were
re-verified once more this session at the current tree: the S-054 restore re-touched
the destination-arming seam adjacent to M-087's WinPE attach question (no core-side
change owed), and the driver-contract rows (M-089/N-107's wire-code citations) remain
the core-side truth. The next concrete step stands: one driver-repo lane takes all
five in the recorded order (M-087 -> M-089 -> N-107/N-127 -> M-086).

## Session-48 disposition note (2026-10-29 — the boundary re-affirmed a fourth time; core-side halves re-verified at the S48 tree)

The S48 lane's grant carries the same two-repo boundary as every session since 10 ("Two
repositories are in scope and only two: core_recovery (modified) + easeus-drw-research
(read-only); do not clone, reference, or look at any other repository") — the five
findings' fix surfaces (M-086/M-087/M-089/N-107/N-127) remain in heicfast/rda-driver
and stay untouchable this session. Core-side halves re-verified at the current tree
(`2d9cbf71`-lineage): release.yml:291's `-FloorVersion v0.2.4` consumption of the
rda-driver release assets (M-086's core half); docs/DRIVER-CONTRACT.md §4.2's
extended-IOCTL wire rows 0x910-0x912 + the identity/global-handle gating (M-089/N-107's
core-side citations — the 0x600 device-type/CTL_CODE derivation chain intact); the
three MAJOR finding files correctly still active in findings/MAJOR/ (their resolutions
are driver-repo-resident). The next concrete step stands unchanged and precise: one
driver-repo lane takes all five in the recorded order (M-087's one-line hex(7) fix +
the .reg parse gate first, then M-089's probe derivation + the static
dispatch-coverage gate, then the N-107/N-127 prose sweeps, then M-086's
version-stamping decision + the two-version coexistence drill on a Windows runner),
each with fail-before/pass-after proof per the house SOPs. No core-side change owed by
any of the five; no silent leave-behind — the boundary is the grant's own text.

## Session-86 disposition note (2026-10-29 — the boundary re-affirmed a fifth time; both MINOR findings brought to the consistent PUSHED-FORWARD posture)

The S86 lane's grant carries the identical two-repo text ("Two repositories are in
scope and only two: core_recovery (modified) + easeus-drw-research (read-only)") —
the five findings' fix surfaces (M-086/M-087/M-089/N-107/N-127) remain in
heicfast/rda-driver and stay untouchable this session. Trail-consistency fix landed
this session: N-107/N-127's status lines — which still carried the audit lane's
"READ-ONLY FINDING — no fix applied" posture while the three MAJORs had been moved
to "PUSHED-FORWARD (driver-repo-resident…)" — are now the same PUSHED-FORWARD
posture with per-finding session disposition notes, so the board's five
driver-resident items read uniformly and no future lane re-derives the boundary.
Core-side halves re-verified at `b6801ade`: DRIVER-CONTRACT.md §4.4's "19
recognized codes" + §4.2's 0x600244x wire-truth rows (N-107/M-089's core-side
citations), the corpus's eprdtdrv-sys--driver-internals.md:75-90 no-ACL posture
(N-127's corpus ground), release.yml's -FloorVersion consumption (M-086's core
half, per the S48 note). The next concrete step stands unchanged and precise:
one driver-repo lane takes all five in the recorded order (M-087 → M-089 →
N-107/N-127 prose sweeps → M-086's version-stamping decision + the two-version
coexistence drill), each with fail-before/pass-after proof per the house SOPs.

## Session-86 disposition note (2026-10-30, the S86 fix lane — the boundary re-affirmed; core-side halves re-verified)

Boundary re-affirmed for the seventh recorded time: this session's grant
covers exactly core_recovery (modified) + easeus-drw-research (read-only
corpus) — the five findings' fix surfaces (M-086/M-087/M-089 + the
N-107/N-127 halves) remain in heicfast/rda-driver and stay untouchable
(heicfast/rda-driver is out of the two-repo grant; zero clone/reference/
look this session). The core-side halves re-verified first-hand at this
session's HEAD: release.yml's `-FloorVersion v0.2.4` consumption of the
rda-driver release assets (M-086's core half sound); the corpus's correct
regdrv.reg contrast (M-087's); docs/DRIVER-CONTRACT.md §4.2's
extended-IOCTL wire rows 0x910-0x912 + the identity/global-handle gating
(M-089/N-107's core-side citations); the driver-service-only content of
the install scripts (`.github/scripts/Install-ReleaseDriver.ps1` +
`installer/core-recovery.iss` — both RDA-only, re-grepped this session).
The next concrete step stands: one driver-repo lane takes all five in
the recorded order (M-087's one-line hex(7) fix + the .reg parse gate,
then M-089's probe derivation + the static dispatch-coverage gate, then
the N-107/N-127 prose sweeps, then M-086's version-stamping decision +
the two-version coexistence drill on a Windows runner), each with
fail-before/pass-after proof per the house SOPs. NOTE: the racing
window's board shows M-086/M-087/M-089 + N-107/N-127 still active in
findings/ — consistent with this boundary (no driver-repo lane has
landed them yet).
