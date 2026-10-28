# OQ-S10-2 — The driver-repo-resident fixes pushed forward (M-086, M-087, M-089, N-107, N-127's driver halves)

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
