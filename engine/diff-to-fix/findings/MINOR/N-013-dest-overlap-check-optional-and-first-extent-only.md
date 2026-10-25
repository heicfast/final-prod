# MINOR N-013 — Destination overlap check is OPTIONAL (gated on drive-letter only) and first-extent-only

**Severity:** MINOR
**Module:** writer, win/transport
**Parameters affected:** F62 (destination overlap refusal), B8 (multi-volume/cross-partition — spanned volumes), R58 (layering — junction resolution)
**Corpus references:** `architecture/04-recovery-workflow.md §7.5` ("destination validation: `PathIsNetworkPathW`? → local: opens `\\.\X:` raw volume of destination (writability + same-volume detection), falls back to `PathFileExistsW`" — the og ALWAYS opens `\\.\X:` for drive-letter destinations, no opt-in gate); `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:60381-60444` (og uses `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` 0x2d1080 + `IOCTL_DISK_GET_DRIVE_LAYOUT_EX` 0x560000 + `FSCTL_LOCK_VOLUME` + `FSCTL_DISMOUNT_VOLUME` to validate and lock the destination)
**Core references:** `src/core/recovery/cr_safewriter.cpp:114` (`if (!is_dir && have_dest_ && dest_bytes_ > 0)` — overlap check gated on `have_dest_`); `src/ui/cr_ui.cpp:2057-2063` (`set_destination()` called only when ALL of: `utf8_path[1] == ':'`, `r->dev->disk_known`, `transport != "image"`); `src/app/rrecov.cpp:869-873` (same conditional in CLI); `src/win/cr_transport.cpp:701-712` (`volume_extent()` returns only `ext->Extents[0]`; the `VOLUME_DISK_EXTENTS ext[8]` buffer has room for 8 extents but only the first is read out); `src/core/recovery/cr_safewriter.cpp:16-18` (`intersects()` is byte-granular — offset+len, NOT cluster-aligned; does not resolve reparse points/junctions on the destination path) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's destination overlap check is gated on three conditions
(drive-letter path, disk-known source, non-image transport); when any
condition fails, the check is skipped entirely. Even when the check
runs, it only inspects the FIRST extent of a spanned/RAID destination
volume (`ext->Extents[0]`) — the buffer has room for 8 extents but
only the first is read out. The og always opens `\\.\X:` for
drive-letter destinations and uses the full `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`
result (multi-extent-aware) plus `FSCTL_LOCK_VOLUME`/`FSCTL_DISMOUNT_VOLUME`
to validate and lock the destination.

## What the reference does

The og's destination validation (per `04-recovery-workflow.md §7.5`):
1. `PathIsNetworkPathW`? → network paths rejected.
2. Local path → open `\\.\X:` raw volume (writability + same-volume
   detection via `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` +
   `IOCTL_DISK_GET_DRIVE_LAYOUT_EX`).
3. `FSCTL_LOCK_VOLUME` + `FSCTL_DISMOUNT_VOLUME` (the S-010 finding's
   lock sequence).
4. Fall back to `PathFileExistsW` if any of the above fails.

The og's `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` call
(`RecoveryModule_decompiled.c:60381-60444`) returns a
`VOLUME_DISK_EXTENTS` struct with `NumberOfDiskExtents` and an array
of `DISK_EXTENT` entries. The og iterates ALL extents to detect
overlap with the source volume.

## What our core does

```cpp
// src/core/recovery/cr_safewriter.cpp:114 (audited 5d6da5f)
if (!is_dir && have_dest_ && dest_bytes_ > 0) {
    // overlap check
}

// src/ui/cr_ui.cpp:2057-2063 (audited 5d6da5f)
if (utf8_path.size() >= 2 && utf8_path[1] == ':' && r->dev->disk_known && transport != "image") {
    set_destination(utf8_path, ...);   // sets have_dest_ = true
}

// src/win/cr_transport.cpp:701-712 (audited 5d6da5f)
u64 volume_extent(char drive_letter) {
    // ... open \\.\X: ...
    VOLUME_DISK_EXTENTS ext[8];        // room for 8 extents
    DWORD bytes;
    DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &ext, sizeof(ext), &bytes, NULL);
    if (ext[0].NumberOfDiskExtents > 0) {
        return ext[0].Extents[0].StartingOffset;   // <-- ONLY FIRST EXTENT
    }
    return 0;
}
```

The check is skipped for: image-file sources, UNC paths, relative paths,
and any source whose `disk_known` is false (image-file sources,
partition slices). When it does run, only the first extent is
inspected — spanned/RAID volumes with `NumberOfDiskExtents > 1` have
their second+ extents silently ignored.

## Gap

Three concrete gaps:

1. **Opt-in gate:** the og ALWAYS opens `\\.\X:` for drive-letter
   destinations. The core only does so when `have_dest_` is true (which
   requires drive-letter path + disk-known source + non-image
   transport). Image-source-to-disk-destination overlap is missed.
2. **First-extent-only:** the og iterates all extents; the core only
   reads `Extents[0]`. Spanned/RAID destinations have their second+
   extents ignored.
3. **No reparse resolution:** neither side resolves junctions on the
   destination path (symmetric gap — `\\.\D:` resolves to D:'s volume,
   not the junction target).

## Impact

- **Image-source gap:** recovering from an image-file source to a
  destination on the same disk that the image was taken from — the
  overlap check is skipped entirely. A user could recover `disk0.img`
  (taken from disk 0) to `D:\recovered` (also on disk 0) and the writer
  would silently overwrite the source disk's sectors via the image's
  extents mapped to disk 0's bytes. (Mitigated: image-file sources have
  file-internal extents, not disk-absolute — but the gate is still
  documented as "cannot be evaluated", not enforced.)
- **Multi-extent gap:** a spanned volume across disks 0+1 with the
  destination extent on disk 1 (second extent) and the source on disk 0
  — `volume_extent('D')` returns only the first extent (disk 0),
  missing the disk-1 overlap.
- **Junction gap:** `D:\recovered` is a junction to `E:\foo` where E:
  is on the source disk. `volume_extent('D')` returns D:'s disk extent,
  not E:'s. The overlap is missed. (Symmetric with the og.)

## Reproduction / verification

- **Core evidence:** first-hand read of `cr_safewriter.cpp:114`,
  `cr_ui.cpp:2057-2063`, `rrecov.cpp:869-873`, `cr_transport.cpp:701-712`,
  `cr_safewriter.cpp:16-18`.
- **Corpus evidence:** first-hand read of
  `04-recovery-workflow.md §7.5` and `RecoveryModule_decompiled.c:60381-60444`.
- **Executed comparison:** not run — would require building both
  implementations and exercising the destination validation paths on
  multi-extent and image-source fixtures.

## Fix plan (for the engineering team)

1. **Always call `set_destination()` for any path that resolves to a
   volume.** Drop the `utf8_path[1] == ':'` gate; use `GetVolumePathNameW`
   to resolve any path (UNC, relative, junction) to its volume letter,
   then call `set_destination()`.
2. **Iterate ALL extents** in `volume_extent()`: change the return type
   to a `std::vector<DiskExtent>` and check each for overlap with the
   source. The buffer (`VOLUME_DISK_EXTENTS ext[8]`) already has room
   for 8 extents; just iterate `ext[0].NumberOfDiskExtents`.
3. **Resolve junctions** via `GetFinalPathNameByHandleW` (or
   `GetFileInformationByHandleEx(FileAttributeTagInfo, ...)`) before
   calling `volume_extent()`.
4. **Files to modify:**
   - `src/core/recovery/cr_safewriter.cpp:114` — drop the `have_dest_`
     gate (or compute it for every destination).
   - `src/ui/cr_ui.cpp:2057-2063` — drop the `utf8_path[1] == ':'` and
     `transport != "image"` gates; use `GetVolumePathNameW` to resolve
     any path.
   - `src/app/rrecov.cpp:869-873` — same change as `cr_ui.cpp`.
   - `src/win/cr_transport.cpp:701-712` — change `volume_extent()` to
     return `std::vector<DiskExtent>` and iterate all extents.
5. **Tests to add:**
   - `Writer.ImageSourceToSameDiskRefused` — fixture: image of disk 0,
     recover to `D:\recovered` on disk 0. Assert refusal. FAILS today.
   - `Writer.MultiExtentSpannedVolumeRefused` — fixture: spanned volume
     across disks 0+1, source on disk 0, destination extent on disk 1.
     Assert refusal. FAILS today.
   - `Writer.JunctionToSourceDiskRefused` — fixture: `D:\recovered` is
     a junction to `E:\foo` where E: is on the source disk. Assert
     refusal. FAILS today (both sides miss this — but the fix is to
     resolve junctions).
6. **Acceptance criteria:** the three tests above pass.

## Tests to add

- `Writer.ImageSourceToSameDiskRefused` (FAILS today)
- `Writer.MultiExtentSpannedVolumeRefused` (FAILS today)
- `Writer.JunctionToSourceDiskRefused` (FAILS today)

## Related findings

- **S-010** — repair-path volume lock missing. Same destination-volume
  concern; the `FSCTL_LOCK_VOLUME` fix on the writer's destination
  address S-010's safety gap AND S2-C-2's overlap-check gap.
- **S2-C-4** — writer does not pre-check destination free space. Same
  destination-volume code path; the `lock_volume()` fix site is the
  natural place to add the `GetDiskFreeSpaceEx` pre-check.

## Open questions

- **OQ-S2C-4:** is the image-source F62 gap exploitable in practice?
  Image-file sources have file-internal extents (offsets relative to
  the image file's start), and `set_destination` is skipped for image
  sources — so the intersection math would compare file-internal
  offsets against disk-absolute dest extents, which is a category
  error even if the check ran. The real risk is only when the image
  is of the SAME disk that the destination lives on. Worth a
  fail-before test (the fix plan above includes this).
