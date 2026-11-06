# D-ENUM-01 — only one of the user's three disks appears

**Reproduced on:** the shipped v0.8.4 release (forensic run
[42255492260](https://github.com/heicfast/final-prod/actions/runs/42255492260))
— with an honest environmental caveat the README records precisely.

## Expected

The landing page shows every physical device the OS exposes, grouped
"Internal drives (N)" / "External drives (%1)" (reference landing
baseline; SD cards additionally first-class on the dedicated page).

## Observed

- **Code-level reproduction (deterministic):** the pre-fix Home page
  rendered `kind === 'internal'` rows ONLY
  (`home-page.tsx:80-83` at the v0.8.4 commit) while the bridge
  degrades every removable-bus disk (USB/SD/MMC) to `kind: 'usb'`
  (`devices.rs:215`, BF-02) — and the `?media` view that rendered them
  was unreachable (no navigation to it exists). On the user's machine
  (2 internal + 1 SD) every non-internal-bus device was invisible; the
  count they saw depends on the bus classification of each drive.
- **Runner evidence (`home-dom-extract.json`):** the probe's Home DOM
  lists ALL FOUR runner disks (Windows(C:), Temporary Storage(D:),
  SeedNTFS(X:), SeedEXFAT(Y:)) — because Azure runner disks are all
  "Virtual Disk" internal-bus, they all pass the internal-only filter.
  The runner therefore demonstrates the ENUMERATION works; it cannot
  reproduce the user's USB/SD-bus mix — the FILTER is the defect, and
  it is proven by the unit regression pins
  (`tests/home-states.test.tsx` — RED on the old filter, GREEN on the
  fix: both groups render, the SD card renders with no `?media`
  param).
- Engine-side companion finding (lane B's register): both enumeration
  stacks silently DROP a present disk whose open fails with anything
  other than ACCESS_DENIED/SHARING_VIOLATION, or whose geometry query
  fails (`cr_enumerate.cpp:98-127`) — the never-drop fix is lane B's
  E-03/D3-engine scope.

## Break point

Two stacked: the Home filter (UI) + the bus→kind degradation (bridge).
The engine merge dedup never collapses identical-model disks (it keys
on disk number + PnP path, `cr_enumerate.h:228-255`).

## Fix (landed, UI layer)

The landing page renders BOTH groups — "Internal drives (N)" AND
"External drives (%1)" — exactly the reference layout; the `?media`
toggle is gone; the group label renamed to the reference string
("External drives" / 外接驱动器). Commit c6ff7c10.
