# MAJOR M-009 — RawFixer read path bypasses the 64 KiB direct-I/O envelope (perf regression on 4Kn NVMe)

**Severity:** MAJOR
**Module:** rawfixer, win/transport
**Parameters affected:** X47 (throughput), X48 (latency — repair phase), X51 (I/O behavior — read sizes/alignment), R58 (layering — bypass of the alignment layer)
**Corpus references:** GAP-S6-13 mandate (the 64 KiB envelope — implemented in `src/win/cr_transport.cpp:148-170` `aligned_read_at` `!rda_scaled` branch); `re-workspace/artifacts/PartitionRecovery/rawfixer-fs-repair.md` (documents the og's RawFixer 33-method pre-scan repair surface — does not specify the og's read alignment for RawFixer)
**Core references:** `src/win/cr_transport.cpp:743-749` (`RwDevice::read_at` calls `overlapped_read(h_, off, buf, len, via_rda_)` directly — bypasses `aligned_read_at`); compare `cr_transport.cpp:213-219` (`DirectDevice::read_at` routes through `aligned_read_at`); compare `cr_transport.cpp:263-269` (`RdaTargetDevice::read_at` routes through `aligned_read_at`); `src/core/repair/cr_rawfixer.cpp:440-538` (`execute()` consumes `IWriteDevice::read_at` for the RMW loop) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's RawFixer write device (`RwDevice` in `cr_transport.cpp`)
performs sector-aligned but NOT 64 KiB-aligned reads via direct
`overlapped_read` calls, bypassing the `aligned_read_at` envelope that
GAP-S6-13 mandates for every direct read path. The scan-path devices
(`DirectDevice`, `RdaTargetDevice`) both route through the envelope;
only `RwDevice` (the repair-path write device) skips it. On 4Kn NVMe
devices this triggers the documented ~3.5× slow path (~830 MB/s vs
~2900 MB/s); on 512e devices the impact is smaller but still nonzero
(extra interrupt per request).

The mandate is `cr_directalign.h:1-22` (GAP-S6-13): "Every direct read
path in core must round lengths UP to a 64 KiB multiple, clamped at the
device end." `RwDevice` is a direct read path on Windows raw disk handles
(`\\.\PhysicalDriveN` or the RDA grammar path) — it is in scope per the
mandate.

## What the reference does

The og's RawFixer read alignment is not explicitly documented in the
corpus (the `rawfixer-fs-repair.md` artifact documents the 33-method
repair surface but not the read alignment). The og's
`RecoveryModule.dll` repair path uses the same Win32 raw disk handle
pattern as the scan path (per `04-recovery-workflow.md §10` "locked
volumes via PartitionRecovery pending-op engine"), so the og likely
uses 64 KiB-aligned reads on the repair path too — but this is
inferred, not verified.

What IS verified: GAP-S6-13 (the core's own mandate) requires every
direct read path to round lengths UP to a 64 KiB multiple. The scan
path complies. The repair path does not.

## What our core does

```cpp
// src/win/cr_transport.cpp:743-749 (audited 5d6da5f)
class RwDevice final : public IWriteDevice {
public:
    u32 read_at(u64 off, void* buf, u32 len) const override {
        return overlapped_read(h_, off, buf, len, via_rda_);    // <-- bypass
    }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        return overlapped_write(h_, off, buf, len);
    }
    // ...
};

// Compare: scan-path devices route through the envelope:
// src/win/cr_transport.cpp:213-219 (DirectDevice::read_at)
u32 read_at(u64 off, void* buf, u32 len) const override {
    return aligned_read_at(h_, off, buf, len, geom_, false, size());
}

// src/win/cr_transport.cpp:263-269 (RdaTargetDevice::read_at)
u32 read_at(u64 off, void* buf, u32 len) const override {
    return aligned_read_at(h_, off, buf, len, geom_, true, size());
}

// src/win/cr_transport.cpp:148-170 (aligned_read_at — the envelope)
u32 aligned_read_at(HANDLE h, u64 off, void* buf, u32 len,
                    const DeviceGeometry& geom, bool rda_scaled, u64 dev_bytes) {
    if (!rda_scaled) {
        // Round length UP to 64 KiB multiple, clamp at device end.
        // ... io::direct_envelope(...) ...
    } else {
        // RDA path — sector-aligned (the F11 unscale).
    }
}
```

`RwDevice::read_at` calls `overlapped_read` directly, skipping the
envelope. The `via_rda_` flag is passed but unused (the function takes
a single `via_rda` bool, not the full geometry+scale context the
envelope needs).

`src/core/repair/cr_rawfixer.cpp:476-490` `execute()` consumes this
path for every RMW iteration:

```cpp
// cr_rawfixer.cpp:476-490 (audited 5d6da5f)
for (const auto& op : plan->operations) {
    if (cancel->checkpoint()) return err::ERR_CANCELLED;
    if (u32 rc = dev->read_at(op.offset, scratch.data(), op.bytes))    // <-- bypass
        return rc;
    undo->append(op.offset, scratch.data(), op.bytes);
    if (u32 rc = dev->write_at(op.offset, op.new_bytes.data(), op.bytes))
        return rc;
    if (u32 rc = dev->read_at(op.offset, verify.data(), op.bytes))     // <-- bypass
        return rc;
    if (std::memcmp(verify.data(), op.new_bytes.data(), op.bytes) != 0)
        return err::ERR_WRITE_VERIFY_FAILED;
}
```

Each operation does TWO `read_at` calls (original sector + verify) +
one `write_at`. The reads bypass the envelope.

## Gap

The mandate (`cr_directalign.h:1-22`) requires every direct read path
to round lengths UP to a 64 KiB multiple. `RwDevice::read_at` does
not. The scan-path devices do.

## Impact

- **Throughput:** on 4Kn NVMe devices, sector-aligned reads trigger
  the documented ~3.5× slow path (~830 MB/s vs ~2900 MB/s for
  64 KiB-aligned reads). RawFixer repair on a 4 TB 4Kn NVMe volume
  with 1000 sectors of repair work: ~5 ms × 1000 = 5 seconds (with
  envelope) vs ~17 seconds (without). Not catastrophic but a clear
  perf regression on the product's primary scenario.
- **CPU:** sector-aligned reads cause one interrupt per request
  instead of one per 64 KiB batch. Higher CPU utilization per byte
  transferred.
- **Latency:** repair-phase latency is bounded by the number of
  operations, not by the read alignment. But each operation's latency
  is ~3.5× higher than necessary.
- **Mandate compliance:** GAP-S6-13 is violated. Future readers
  auditing the envelope will assume all read paths comply; the
  `RwDevice` bypass is a surprise.
- **Correctness:** none — the bypass produces correct bytes, just
  slowly. This is a perf-only finding.

## Reproduction / verification

- **Core evidence:** first-hand read of `cr_transport.cpp:743-749`
  (`RwDevice::read_at` calls `overlapped_read` directly),
  `:213-219` (`DirectDevice::read_at` routes through envelope),
  `:263-269` (`RdaTargetDevice::read_at` routes through envelope),
  `:148-170` (`aligned_read_at` — the envelope). The bypass is
  explicit in the source.
- **Mandate evidence:** first-hand read of `include/cr/storage/cr_directalign.h:1-22`
  ("Every direct read path in core must round lengths UP to a 64 KiB
  multiple, clamped at the device end").
- **Consumer evidence:** first-hand read of
  `src/core/repair/cr_rawfixer.cpp:476-490` (the RMW loop that
  consumes `RwDevice::read_at`).
- **Executed comparison:** not run — would require Windows + 4Kn NVMe
  device + RawFixer repair scenario. The ~3.5× number is from the
  GAP-S6-13 mandate's documented measurements on the scan path
  (the same hardware would show the same ratio on the repair path).

## Fix plan (for the engineering team)

1. **Architectural change:** none — the envelope already exists. The
   fix is routing `RwDevice::read_at` through it.
2. **Files to modify:**
   - `src/win/cr_transport.cpp:743-749` — replace
     `overlapped_read(h_, off, buf, len, via_rda_)` with
     `aligned_read_at(h_, off, buf, len, geom_, via_rda_, size())`.
     The `via_rda_` flag already encodes whether the F11 unscale
     applies; `aligned_read_at` honours `rda_scaled` to pick
     sector-vs-64 KiB envelope. `RwDevice` needs to store the
     `DeviceGeometry` (already stored on `DirectDevice` and
     `RdaTargetDevice`) — add a `geom_` member and pass it in the
     constructor.
3. **Algorithm/constant:** none — the envelope logic is unchanged.
4. **Tests to add:**
   - `WinRepair.ReadsUse64KiBEnvelope` — mock device records
     `aligned_read_at` was called (not `overlapped_read` directly).
     Assert the read length was rounded UP to a 64 KiB multiple.
   - `WinRepair.ReadAtOn4KnDeviceUsesAlignedRead` — fixture: a 4Kn
     device, read at offset 0x1000 with length 0x200 (one sector).
     Assert the underlying `ReadFile` was called with length
     0x10000 (64 KiB) and the result was sliced to the requested
     0x200.
   - `WinRepair.RdaScaledReadStillSectorAligned` — the RDA path
     (sector-aligned) is preserved when `via_rda_` is true.
   - `RawFixer.RepairRmwUsesEnvelope` — integration test: the RMW
     loop in `execute()` calls `read_at` which routes through the
     envelope. Assert via mock.
5. **Contract/doc updates:** none — the mandate already covers this
   path; the fix brings the code into compliance.
6. **Acceptance criteria:**
   - The four tests above pass.
   - `grep overlapped_read src/win/cr_transport.cpp` returns no
     hits in `RwDevice::read_at` (the call goes through
     `aligned_read_at` instead).
   - A 4Kn NVMe repair scenario shows ~3.5× speedup vs the pre-fix
     baseline (bench harness measurement).

## Tests to add

- `WinRepair.ReadsUse64KiBEnvelope`
- `WinRepair.ReadAtOn4KnDeviceUsesAlignedRead`
- `WinRepair.RdaScaledReadStillSectorAligned`
- `RawFixer.RepairRmwUsesEnvelope`
- (Bench) `Bench.RawFixerRepair4KnNvme` — measures repair throughput
  on a 4Kn NVMe fixture; ~3.5× speedup expected.

## Related findings

- **S-010** — repair-path volume lock missing. Same path
  (`RwDevice`), same module (`cr_transport.cpp`), different concern
  (safety vs perf). Both findings touch `RwDevice`; the fixes
  compose (lock + envelope).
- **S-008** — RawFixer is boot-sector-only and not wired pre-scan.
  M-009 is a perf gap on the WRITE path that S-008 wires; even after
  S-008 is fixed, the repair writes are slow without M-009's fix.
- **GAP-S6-13** (already fixed) — the 64 KiB envelope was added to
  the scan path; M-009 closes the same gap on the repair path.

## Open questions

- **OQ-S2-D-3 (carried from S2-D):** does the og's RawFixer read path
  use a different alignment? The corpus doesn't say. Likely
  irrelevant: RawFixer runs once before scan, not on the hot path —
  performance impact is minor. The MAJOR finding stands regardless:
  core's RawFixer bypasses its OWN 64 KiB envelope mandate.
