# MAJOR M-119 — Partition-boundary read semantics: the og's partition device CLAMPS out-of-bounds reads and zero-fills the tail (read succeeds); the core's PartitionSlice hard-errors, and the CLI/UI partition-open paths REFUSE the whole volume when the table's length runs past the device end

**Severity:** MAJOR
**Module:** storage (PartitionSlice), writer/recovery read paths, cli/ui partition open
**Parameters affected:** F60/P1 (bad-media / boundary handling policy), B6/B8 (multi-volume edge geometry), X51 (I/O behavior at boundaries), E23 (error codes — the same condition maps to SUCCESS+zeros in the og and ERR_RANGE_OUT_OF_BOUNDS in the core)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:183633-183666` (FUN_1400d0510 — the RTTI-confirmed `CBasicPartitionDevice` read wrapper: a read whose `(offset, count)` runs past the partition's sector count is clamped to the in-bounds prefix via the parent device, the unread tail is `memset` to ZERO, the read returns success, and the diagnostic `"i64CountofSectors - i64RealReadSector = %I64d"` logs the tail size; the `__RTDynamicCast(..., &CBasicPartitionDevice::RTTI_Type_Descriptor, ...)` inside the function proves the class); `og-codebase/log/DRWUI.log:638` (the real-run observation: `i64CountofSectors - i64RealReadSector = 10175` fired during the deep-scan lost-partition probing — the og's scan really does issue past-end reads and proceeds on the zero-filled result); `architecture/05-scan-engine-architecture.md` §4.2/:118-125 (the CPartitionVirtualVolumeDevice device family — the engine-side counterpart whose per-read policy is the fix-time probe below)
**Core references:** audited `c7fa527c` — `src/core/storage/cr_slice.cpp:46-49` (`PartitionSlice::read_at`: `if (offset > length_ || u64(length) > length_ - offset) return ERR_RANGE_OUT_OF_BOUNDS;` — hard error, no partial read, no zero-fill); `src/core/storage/cr_slice.cpp:15-31` (`PartitionSlice::create`: `if (start_byte + length_bytes > parent->size()) { *err = ERR_RANGE_OUT_OF_BOUNDS; return nullptr; }` — the whole slice is rejected when the declared length overruns the device); `src/app/rrecov.cpp:2973-2978` (the CLI partition open: `PartitionSlice::create(view.get(), part_base, u64(p.sector_count) * ls, &prc)` with the table's RAW sector count — on overrun the partition is refused entirely: `"cannot slice partition %d (rc=%08x)"`, scan aborts); `src/ui/cr_ui.cpp:1372-1375` and `:1456-1459` (the ABI volume open: same raw-extent `PartitionSlice::create`, same `ERR_OPEN_FAILED "slice creation failed for %c"` refusal); `src/core/lost/cr_candidates.cpp:493-501` (`materialize()` — the discovery path DOES clamp: `if (bytes > max) bytes = max;` — the internal inconsistency: the discovery tolerates ragged tails, the CLI/UI opens do not)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Where a read or a declared extent runs past a partition's end, the og's device layer is tolerant: the per-read wrapper clamps the read to the in-bounds prefix, zero-fills the remainder, and reports success (RTTI-confirmed at DRWUI_decompiled.c:183633-183666, observed firing in a real scan log). The core's `PartitionSlice` is strict: per-read out-of-bounds returns `ERR_RANGE_OUT_OF_BOUNDS`, and creation-time overrun refuses the whole slice. The practical consequence sits at the CLI/UI partition-open paths, which pass the partition table's declared length raw: a corrupt/oversized table entry (or a disk image restored onto smaller media, or a table surviving a device shrink) makes the og's product SCAN the volume (real bytes to the device end, zeros beyond) while the core's product refuses the volume outright ("cannot slice partition" / "slice creation failed").

## What the reference does

FUN_1400d0510 (DRWUI_decompiled.c:183633-183666), the `CBasicPartitionDevice` read wrapper (the RTTI dynamic-cast inside the function targets the class by name):

```c
if ((((-1 < param_3) || (-1 < *(longlong *)(param_1 + 0x30) + param_3)) &&
    (param_3 < lVar3 + -1)) && (param_4 <= lVar3 + -1)) {
    lVar4 = param_4;
    if (lVar3 < param_3 + param_4) {
        lVar4 = lVar3 - param_3;          /* clamp: read only the in-bounds prefix */
    }
    uVar1 = (**(code **)(**(longlong **)(param_1 + 0x28) + 8))
              (..., *(longlong *)(param_1 + 0x30) + param_3, lVar4, ...);  /* parent read */
    _Size = param_4 - lVar4;              /* the unread tail, in sectors */
    ...
    if ((longlong)_Size < 1) {
        return uVar1;                     /* fully in-bounds: plain success */
    }
    FUN_140162e00("i64CountofSectors - i64RealReadSector = %I64d", _Size);  /* diagnostic */
    iVar2 = (**(code **)(**(longlong **)(param_1 + 0x28) + 0x28))();       /* sector size */
    memset((void *)(iVar2 * lVar4 + param_2), 0, _Size);  /* ZERO-FILL the tail */
    return uVar1;                         /* SUCCESS over the clamped+zeroed read */
}
```

The og's scan flows therefore never see a boundary error from an over-long request: they see real bytes up to the partition/device end and zeros beyond, and their parsers proceed (the real-run log line `i64CountofSectors - i64RealReadSector = 10175` at DRWUI.log:638 fired during lost-partition probing — the probe continued and the partition was found).

The engine-side `CPartitionVirtualVolumeDevice` (RTTI-present in HFSPlusSearchDLL and DRWUI; the reader plumbing at HFSPlusSearchDLL_decompiled.c:454-461) is the direct counterpart of the core's `PartitionSlice`; its own per-read policy was not located in this session (its read slot resolves through the tree reader chain) — that decode is the named fix-time probe, and the DRWUI-side decode plus the real-log observation already establish the og's product-level semantics for the scan flows.

## What our core does

```cpp
// src/core/storage/cr_slice.cpp:46-49 — per-read: hard error
u32 PartitionSlice::read_at(u64 offset, void* buffer, u32 length) {
    if (length == 0) return OK;
    if (offset > length_ || u64(length) > length_ - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;
    return parent_->read_at(start_ + offset, buffer, length);
}

// src/core/storage/cr_slice.cpp:25-28 — creation: whole-slice refusal on overrun
if (start_byte + length_bytes > parent->size()) {
    if (err) *err = ERR_RANGE_OUT_OF_BOUNDS;
    return nullptr;
}

// src/app/rrecov.cpp:2925-2931 — the CLI passes the table's raw length
part_slice = cr::PartitionSlice::create(
    view.get(), part_base, u64(p.sector_count) * view->geometry().logical_sector, &prc);
if (!part_slice) {
    wprintf(L"cannot slice partition %d (rc=%08x)\n", part, prc);
    return 2;                                  /* the whole scan aborts */
}

// src/ui/cr_ui.cpp:1349-1352 / :1433-1436 — the ABI volume open, same refusal
auto slice = PartitionSlice::create(whole.get(), start, len, nullptr);
if (!slice)
    return ret_(ERR_OPEN_FAILED, "slice creation failed for %c: (@%llu +%llu)", ...);
```

The discovery path already implements the tolerant posture for its own materialization (`cr_candidates.cpp:493-501` (line cites re-verified at the post-rebase HEAD): `if (bytes > max) bytes = max;` — ragged candidate tails are clamped, matching the og's tolerance), which makes the CLI/UI strictness an internal inconsistency as well as an og divergence.

## Gap

Falsifiable claims: (1) at `c7fa527c`, `PartitionSlice::read_at` returns an error for any request straddling the slice end, where the og's `CBasicPartitionDevice` read returns success with a zero-filled tail; (2) the CLI partition open (`rrecov.cpp:2973`) and both `cr_ui` volume opens (`:1349`, `:1433`) pass the partition's DECLARED length to `PartitionSlice::create` without clamping to the device end, so a table entry whose length overruns the device refuses the entire volume scan — the og scans such volumes clamped. Both verified by direct read this session.

## Impact

- **Whole-volume refusal on damaged tables (the target market):** a corrupt length field, a table from a larger disk restored onto smaller media, or a shrunken device yields "cannot slice partition" and scan abort in the core; the og scans the volume with a zero tail. This is exactly the class of disk a recovery product is bought for.
- **Mid-scan boundary reads:** any engine request straddling the volume end (a final cluster/MFT record/extent that runs long) surfaces as a read error in the core and as zeros in the og; the core's engines then apply their own error policies (skip/fail the file) where the og's parsers simply see zero bytes — divergence in recovered-content tails and in error-map rows (E23).
- **API contract:** hosts using `PartitionSlice` (a public `include/cr/storage/` surface) inherit the strict semantics with no tolerant alternative.

## Reproduction / verification

Corpus-derived + decompile-verified + code-verified (this session, first-hand):
1. The FUN_1400d0510 body quoted above, read at DRWUI_decompiled.c:183633-183666 (the clamp, the diagnostic, the memset — quoted in `evidence/decompiled-quotes/s12-partition-read-clamp.md`).
2. The real-run log line `og-codebase/log/DRWUI.log:638` — the clamp path firing in the field during lost-partition probing (quoted in the evidence file).
3. The core cites read at HEAD (`cr_slice.cpp:46-49` and `:25-28`; `rrecov.cpp:2925-2931`; `cr_ui.cpp:1349-1352`, `:1433-1436`; `cr_candidates.cpp:493-501` (line cites re-verified at the post-rebase HEAD)) — quoted above.
4. Executed comparison is possible at fix time (a fixture disk whose table declares a longer partition than the media — the og-side behavior is established by 1+2; the core-side refusal is directly reproducible on the existing CLI with a hand-built fixture — recorded as the fix plan's test (a)).

## Fix plan (for the engineering team)

1. Architectural: adopt the og's split posture — **creation tolerant, per-read tolerant**. `PartitionSlice::create` clamps `length_bytes` to the device end (recording the declared-vs-clamped delta on the slice for telemetry/diagnostics), and `read_at` clamps any straddling request to the in-bounds prefix and zero-fills the tail (returning OK). The strict variant remains available for callers that need hard bounds (the write path — `cr_apply`/SafeWriter — must NEVER silently extend; gate the tolerance on the read-only paths only).
2. Files: `src/core/storage/cr_slice.cpp` (create clamp + read clamp/zero-fill + a `declared_bytes`/`clamped` field), `include/cr/storage/cr_slice.h` (the field + doc), `src/app/rrecov.cpp` (the partition open: keep passing the declared length — the create-clamp now handles overrun; replace the abort with a warning line carrying the clamp delta), `src/ui/cr_ui.cpp:1349/:1433` (same — `ERR_OPEN_FAILED` only for genuinely unopenable devices, a clamped open succeeds with the delta surfaced).
3. Algorithm: per-read clamp is the og's exact shape (prefix read via parent + `memset` tail + success); no constants beyond the existing geometry. **Fix-time probe (one decompile read):** the engine-side `CPartitionVirtualVolumeDevice` read slot (start from its vtable in HFSPlusSearchDLL — RTTI gives the class; confirm its per-read policy matches the DRWUI-side clamp before shipping the per-read half, so the core's engine-facing semantics are pinned to the ENGINE device, not only the UI device).
4. Tests: (a) unit — slice create with declared length > device end succeeds clamped (field carries delta); read straddling the end returns OK with zero tail bytes (memcmp-pinned); read fully beyond returns OK all-zeros-or-error-per-probe (adopt the probe's verdict); (b) CLI integration — a fixture disk with an over-long table entry scans the volume instead of aborting (the og-parity pin; fails before, passes after); (c) write-path fence — `cr_apply`/SafeWriter refuse out-of-bounds writes unchanged (the tolerance must not leak into the write side); (d) error-map shape — a straddling read no longer emits an ErrorRange row (E23 parity).
5. Contract/doc: `include/cr/storage/cr_slice.h` doc block states the clamp+zero-fill read semantics and the write-side exception; `docs/CORE-API-CURRENT.md` boundary-read section; `modules/storage.md` row update.
6. Acceptance: the over-long-table fixture scans and finds files (the og's observable); the write-path fence test stays green; the per-read zero-tail bytes are byte-pinned.

## Tests to add

- `Slice.CreateClampsOverlongDeclaredLength` (fails before: nullptr + ERR_RANGE_OUT_OF_BOUNDS)
- `Slice.ReadStraddleZeroFillsTail` (fails before: ERR_RANGE_OUT_OF_BOUNDS)
- `Cli.ScanOverlongTableEntryScansVolume` (fails before: "cannot slice partition" abort)
- `CrUi.VolumeOpenClampedNotRefused` (the ABI twin)
- `Apply.WriteBoundsStillHardRefused` (the fence — must pass before AND after)

## Related findings

- S-051/M-104 (the whole-disk/partition-target family this composes with — their fixtures gain the over-long entry case), N-046 (the refresh-geometry refusal — the create-clamp must keep the honest-refusal posture for misaligned/shrunken starts, only the LENGTH clamps), E23 error-map shape (the boundary row disappears)

## Open questions

The engine-side `CPartitionVirtualVolumeDevice` per-read policy (the named fix-time probe — the DRWUI-side decode + real-log observation stand on their own for the product-level semantics).

---

## Post-rebase renumber note (2026-10-29, the hottest parallel window)

This finding was filed under the M-113 number at the pre-rebase HEAD `c7fa527c`;
the parallel continuation/fix lanes pushed first with their own M-113 filing
(their M-113 landed earlier and keeps the number per the N-129 collision
convention) — renumbered to M-119 at the reconciliation. The defect site was
re-verified LIVE at the post-rebase HEAD `cd883754` (grep + direct read, cites
re-pinned where lines drifted); the finding's substance is unchanged.

---

## Session-42 S42 addendum (2026-10-29 — the transport-entry decode + the real-run corroboration: 393 Ldq refusals in DefaultLog.txt)

The finding's site family decoded FIRST-HAND at the transport entry this session
(`evidence/decompiled-quotes/s42-defaultlog-ldq-decode.md` §1):

1. **The full parameter-validation ladder** (DRWUI :180139-180151, the Ldq entry —
   "Ldq" = logical-disk query): three REFUSE classes precede the CLAMP class —
   (a) negative start OR negative count, (b) **count ≥ 4 GiB-of-sectors**
   (`0x100000000 / sector_size <= count` — a 512-B device: ≥ 8,388,608 sectors),
   (c) **start ≥ device size**; each refusal logs the Ldq line and errors out. The
   CLAMP class (start + count > size but start < size) is the finding's core decode —
   `count = size − start`, then the read proceeds.
2. **The zero-fill branch** at :180157-180160: the read-error path logs
   "Read sector error in %d" and **zero-fills count × sector_size** — the
   zero-fill-succeed semantics sit at BOTH the boundary path and the error path.
3. **The real-run corroboration**: `og-codebase/log/DefaultLog.txt` carries **393
   `Ldq:Read sector parameter error` lines** from ONE captured run on a basic disk —
   the og's own scan layers issue out-of-parameter reads ROUTINELY and the transport's
   refuse-with-log is normal operation. The finding's fix plan should implement the
   full ladder (three refusals + the clamp + the error-path zero-fill), not the clamp
   alone.

**Sharpened acceptance for the fix:** the core's read path refuses negative and
out-of-range reads (MATCH), clamps over-long reads to the device (the finding's ask),
zero-fills on read errors where the scan opted into skip mode (the RetryingDevice
contract), and never hard-errors a whole volume open on a table-length overrun (the
CLI/UI leg of the finding).
