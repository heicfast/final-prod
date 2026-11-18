# it-gate evidence — the green run (T15/T16 validation artifacts)

**Source**: bridge-integration run 44244567592, HEAD be3b8256 (the first run with
all five integration gates green), workflow job "Build engine + link bridge +
run integration tests", windows-2022. The fixture: the runner disk + a 6 GiB
seeded NTFS VHDX (120 live + 10 categorized live incl. REAL JPEGs and a genuine
43-entry zip; a deleted set of 80 del-*.bin + one 3 GiB bulk, all freed MFT
records intact by the write-before-delete ordering) + a 512 MiB raw VHDX.

## The files

- **scan-timeline.csv** — the scan_progress_axes_move gate's telemetry
  timeline. The terminal sample pins the D-P6 phase model ON REAL WINDOWS:
  `overall_pm=750` (the global quick|deep cap — the exact "75%" behind the
  user's report), `phase=1, phase_permille=1000` (the advanced phase's OWN
  completion at 100%), quick+deep both `completed`, `bytes_found=3,241,140,501`
  (~3.02 GiB — the deleted bulk found), 305 files, 0 read errors.
- **deleted-tree.txt** — the results_filters_and_deleted_tree gate's captured
  tree: **81/81 deleted rows** (80 del-*.bin + the 3 GiB bulk), every one
  under its RECONSTRUCTED original parent `Basic data partition/./gone/` —
  the "reconstruct them as they were before deletion" standard, proven. The
  same gate also pinned the category filters (5 .jpg / 3 .pdf / 2 .zip live
  rows), the size filter (>=18 rows over the 250 KiB floor), and the
  manifest total (>=100).
- **preview.txt** — the preview_surfaces gate. NOTE one cosmetic defect in
  the evidence FORMAT STRING only: it labels the archive node's kind as
  "image kind" (the `kind` binding was reused); the assertions themselves
  both held — the image-category node's metadata kind was "image" (a REAL
  JPEG: the engine classifies content, and the fixture's real-JPEG fix was
  required to get there), and the archive node's kind "archive" with the
  genuine zip's **43-entry** inventory (40 .bin + 3 .pdf — exactly the docs
  folder that was compressed).
- **health.json** — the health_read_per_disk_is_coherent gate: per-disk
  outcomes on all 4 visible disks (runner + seed + raw + system): the
  truthful no-source refusals (`rc_size_probe 0x50070001`, canary intact,
  transport "" on the runner's NVMe — hosted runners expose no SMART
  passthrough; the refusal IS the correct behavior, the physical-class
  validation remains deferred per BEHAVIOR-FINDINGS).
- **enumerate.json / scan-events.json** — the disk-enumeration shape (3+
  disks, the seeded scan target) and the engine event stream counts.
- **abi-layouts.json** — the generated ABI layout dump the it-crate pins
  (366 field paths, incl. the D-P6 phase axes and the D-P4 SMART fields).

## Provenance chain

The lane's seed script (`.github/workflows/bridge-integration.yml`, step
[4/5]) builds the fixture on every run; the tests write these files via
`write_evidence`; the workflow uploads the `cr-it-evidence` artifact. Every
number above is a captured measurement, not a claim.
