# RECALL0-DIAGNOSIS — the scenario-matrix hash-recall 0.0 (both transport eras)

> Session-97, T5-R2 (the recall-0 diagnostic). Written from first-hand
> artifact + source analysis at HEAD `e9bbb721`; the fix lands in the same
> session. Every claim cites its source. This document exists because the
> finding changes the session's validation landscape: the recall-0 is NOT a
> direct-path regression and NOT a transport issue — it is a pre-existing
> destination-arming defect that refuses nearly all scenario-lane recovery.

---

## §1 The defect, in one paragraph

The recovery writer's destination-overlap refusal (F12/N-013, R7.2) is armed
by both recovery entry points with the destination volume's disk spans
**unconditionally**, without the same-disk precondition the check's own
comment documents ("under the same-disk convention the arming documents",
cr_safewriter.cpp:189-197). The per-item check then compares the item's
ABSOLUTE source-disk offsets (sessions store parent-disk-absolute extents,
rrecov.cpp:2452's M-042 note) against the destination volume's spans on a
DIFFERENT physical disk, numerically. On the benchmark runner the work drive
(D:, the only volume of disk 1, a GPT disk) spans ≈[135 MiB, 220 GiB], so
virtually every source extent above 135 MiB "intersects" it numerically and
every such item is refused with `ERR_DEST_OVERLAP` (0x50050001).

## §2 The evidence chain (both eras, run-ID'd)

| Fact | Source |
|---|---|
| S-DEL-NTFS deepcarve recovery: queued 10,405, **refused 10,248**, done 94, dirs 60, bytes 0 | v0.7.0 run 41140212296 `04-scenarios/S-DEL-NTFS/cli/report-deepcarve.json` totals |
| Every refusal carries `reason = 0x50050001` = `ERR_DEST_OVERLAP` | same report, items[] outcome×reason distribution (10,248 × refused/0x50050001) |
| **Identical in the driver era**: queued 10,403, refused 10,248, reason 0x50050001 | v0.6.1 run 41042267586 `S-DEL-NTFS/cli/report-deepcarve.json` |
| The "done" items are 0-byte: directory-skeleton names ("seed", "tiny", "small", "media") with `bytes: 0` | same reports, items[] done samples |
| The clean-volume API recovery: 82 done with hash-verify ON, 0 mismatches, 591 refused — the 82 = items whose extents sit entirely BELOW D:'s ≈135 MiB span start | run 41140212296 `07-performance/api-result.json` recovery block |
| The runner layout: D: = only volume of disk 1 (219.98 GB GPT); the source VHDs are disks 2/3/4 | run 41140212296 `00-environment/disk-list-before.txt`, `02-disk-setup/disk-state.json` |
| The CLI computed `on_source` (v.disk == disk) but used it ONLY to pick the warning text — arming happened either way | rrecov.cpp:2533-2556 (pre-fix) |
| The API path armed unconditionally (image sources excepted) | cr_ui.cpp:4505-4529 (pre-fix) |
| The writer's check has no same-disk knowledge — it compares raw numeric ranges | cr_safewriter.cpp:198-236 |
| The HARNESS's own intent: "dest is on the runner workspace disk, never the source VHD — destination-overlap refusal stays quiet by construction" | bench/ours/run-ours.ps1:128-129 comment (pre-fix) |

## §3 Why it stayed invisible

1. The old lane (pre-session-96) had no gate table: recall-0 rendered as a
   green job with failing scenarios nobody gated. The "87.16% recall
   (baseline-candidate)" in the matrix's gate note was measured on an
   earlier fixture/era (the number's provenance predates the current seed
   corpus; both current-era runs measure 0.0).
2. The refusal is per-item and non-fatal by contract ("a refusal never
   fails the item" — cr_safewriter.cpp:829): the recovery exits 0 with a
   report full of refusals; only the inventory matcher sees the void.
3. The clean-volume API benchmark's 82 hash-verified recoveries below
   D:'s span start kept the recovery WIRE looking alive — and it is; the
   wire was never the problem.

## §4 The fix (landed this session)

- `win::dest_overlap_spans(ve, source_disk_known, source_disk, out)` — the
  arming predicate: arms ONLY when a destination extent sits on the source
  disk (include/cr/win/cr_transport.h, src/win/cr_transport.cpp).
- The CLI (`rrecov.cpp` cmd_recover): arms through the predicate; image
  sources never arm; the cross-disk case prints the not-armed line.
- The API (`cr_ui.cpp` recovery destination): arms through the predicate
  using the device's own disk number (`CrDevice::disk`/`disk_known`, set by
  both open paths).
- New Windows-lane unit suite `tests/unit/test_transport_win.cpp`: the
  runner case verbatim (cross-disk never arms), same-disk arms with ALL
  spans (N-013 preserved), unknown-source never arms, degenerate inputs,
  zero-offset adjacency.
- The writer-level mechanics (the per-item numeric comparison, N-013
  multi-span arming, the LDM basis mapping) are UNCHANGED — the existing
  unit tests (test_report.cpp, test_orchestrator.cpp:2143,
  test_ldm_recovery.cpp:448) keep pinning them; same-disk refusal still
  fires exactly as before.

## §5 Classification and blast radius

- **Classification: product defect** (a false-positive refusal defeating
  the product's core function for cross-disk destinations — the common
  case on real hardware too: any destination volume whose absolute span
  numerically overlaps the source extents, e.g. a dest volume starting at
  135 MiB on any second disk). Transport-independent; present since the
  arming landed (S-054's restoration); NOT a driver-era vs direct-era
  delta — parity between eras holds (both refused identically).
- **Blast radius of the fix:** destinations on the source disk keep the
  full refusal (safety rule intact, now actually correct in its own
  coordinate space); cross-disk destinations stop refusing (the fix);
  image sources stop arming in the CLI too (the API already excluded
  them — the CLI now matches). No format, ABI, session, or output change;
  the report's refusal REASONS change only in that the false ones vanish.
- **The validation:** the benchmark scenario matrix (real Windows, cross-
  disk dest by construction) is the behavioral test — recall floors
  become meaningful again with this fix; the floors themselves stay as
  versioned (bench/baseline.json scenarios section).
