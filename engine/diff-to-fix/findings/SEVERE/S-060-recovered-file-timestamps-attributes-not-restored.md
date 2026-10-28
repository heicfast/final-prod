# SEVERE S-060 — recovered files lose their timestamps and DOS attributes: the og restores both after every item; the core has no surface at all

**Severity:** SEVERE
**Module:** writer / recovery (timestamp focus battery)
**Parameters affected:** #9 Time and timestamp handling; #2 Recovery correctness (metadata); #13 Report schema
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:874673-874720` (the per-item post-copy restoration block: `SetFileAttributesW` + attribute-mask-gated `SetFileTime`); `:874686-874690` ("Failed to set file attribute" trace); `:874697-874718` (the vtable+0x80 mask, the +0x50/+0x58/+0x60 time getters, `SetFileTime`, "Failed to set file time"); `re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md:29-31` (the wire time getters the export reads)
**Core references:** `include/cr/recovery/cr_safewriter.h:39-63` (the `DestIo` interface — create/mkdir/write/commit/close/free/exists only, no time or attribute surface); `include/cr/win/cr_destio.h:17-46` (the Win32 backend, same); `src/core/recovery/cr_report.h:26-88` (the report carries no per-file times); `src/core/recovery/cr_safewriter.cpp:323-562` (the item body — no restoration step). Verified by grep: **zero** `SetFileTime`/`SetFileAttributesW`/`utimensat`/`set_times` hits in `src/` + `include/` at audited core `b26017a7`. **Trail row refuted:** `diff-to-fix/modules/writer.md:67-75` (the N-009d row) claims "og never calls SetFileTime — timestamps not restored on recovered files, both sides" — FALSE, see below.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

After every successfully copied item, the og restores the recovered file's DOS attributes and its creation/access/write timestamps from the scanned metadata — gated per-field by the source file's attribute mask. The core has no equivalent at any layer: the destination IO interface has no time or attribute operation, the writer never calls one, the report records none, and a grep across the whole tree finds no `SetFileTime`/`SetFileAttributesW`/`utimensat`. Every file the core recovers is written with "now" timestamps and default attributes. The trail's own writer module row asserted the og also skips this — that row is refuted by the decompile read this session.

## What the reference does

In the per-item export body (`FUN_1405414e0` region), after the content copy succeeds (or the file is empty), for every item with a resolved destination path:

1. **Attributes** (`:874686-874690`): `SetFileAttributesW(path, attrs & 0xfffd)` — the props' attribute word (`vtable+0x38` getter) with the in-use bit (0x2) cleared; failure logs `"Failed to set file attribute"` via `ReleaseTrace` and continues (non-fatal).
2. **Times** (`:874691-874718`): open the destination with `CreateFileW(GENERIC_READ|GENERIC_WRITE …)`; read the attribute mask via `vtable+0x80`; then per-bit gating — mask bit **0x10 → last-access** via the `vtable+0x58` getter; bit **0x20 → last-write** via `+0x60`; bit **0x8 → creation** via `+0x50`; pass the three `FILETIME*` (NULL for the un-set ones) to `SetFileTime(hFile, creation, access, write)`; failure logs `"Failed to set file time"` and `CloseHandle` (non-fatal; the file keeps whatever the OS assigned).

The getters are the IFileProperties wire slots documented in the SaveScan census (`part1 :29-31`: creation qword@+0x25, last-write@+0x2d, last-access@+0x35 with the vtable getter family) — i.e. the og restores exactly the timestamps the engines decoded and the session persisted, on every recovered file, as part of the export loop proper (not a UI post-pass).

## What our core does

Nothing at any layer, verified first-hand this session:

- `DestIo` (`cr_safewriter.h:39-63`) exposes only `create_temp`/`mkdir`/`write`/`commit`/`close`/`free_bytes`/`exists` — there is no operation through which a time or attribute could reach the destination.
- The Win32 backend (`include/cr/win/cr_destio.h:17-46`) implements exactly that surface.
- The item body (`cr_safewriter.cpp:323-562`) goes content → hash → commit, with no post-commit metadata step.
- `grep -rn "SetFileTime|SetFileAttributes|utimensat|utimes|set_times|fchmod" src/ include/` → **zero hits**.
- The JSON report (`cr_report.h:26-88`) carries outcomes and sizes; no per-file timestamps — so the loss is also invisible downstream.
- The core's `FileProperties` carries the data needed (`cr_types.h:43-64` `FileTimes` 4×u64 100ns-1601, `cr_node.h:83` + the attribute word on the props) — the restoration is a missing *consumer*, not missing data.

## Gap

Stated falsifiably: recover any file with the og and `GetFileTime`/`GetFileAttributes` on the output returns the scanned source's values (per the mask gates); recover the same file with the core and the output has the recovery moment's timestamp and default attributes. The trail's N-009d row ("og never calls SetFileTime") is disproven by the single `SetFileTime` call site at `:874714` (the only one in the 2.38M-line decompile, inside the export body).

## Impact

User-observable on **every recovered file** — sorted-by-date browsing of recovered output, backup/differential tooling that consumes file times, and forensic workflows all see wrong metadata; the attribute loss additionally flips read-only/hidden/system bits (e.g. restored files that were read-only lose that). This sits in the session's focus battery (timestamps); a verified divergence there is filed severe per the directive. The report's silence about it means the CLI cannot even warn.

## Reproduction / verification

- **Decompiled evidence (first-hand, this session):** the `SetFileAttributesW` site at `:874686-687` (grep-unique in the export family; the other `SetFileAttributesW` hits in the binary are viewer/config paths at :110055/:449586/:679296/:689013 etc.); the mask-gated `SetFileTime` at `:874697-718` with the exact vtable slots; the failure traces. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-reconstruction-export-chain.md`.
- **Core evidence (first-hand):** the DestIo surface read at `cr_safewriter.h:39-63`; the grep proof over `src/`+`include/`.
- **Execution status:** the og side is code-proven; the core side is code-proven (absence). An end-to-end `dir /T` comparison needs a Windows host — the fix lane should capture it as the acceptance proof (UNVERIFIED-BY-EXECUTION for the pair run; each side individually is verified).

## Fix plan (for the engineering team)

1. **Extend `DestIo`** with a post-commit metadata call, e.g. `virtual u32 set_metadata(u64 item_id, const FileMeta&)` where `FileMeta{created, accessed, written (u64 100ns-1601), attributes (u32), per-field enable bits}`. WinDestIo maps it to `SetFileTime` + `SetFileAttributesW` (mask `& ~0x2` like the og); POSIX backends map to `utimensat` + `fchmod` (a deliberate cross-platform translation of the same contract — register as a delta).
2. **Wire the writer** (`recover_item_impl_`, after successful commit): build `FileMeta` from `pr->times` + the props' attribute word, honoring the og's per-field mask gates (bit 0x8 → creation, 0x10 → access, 0x20 → write — i.e. mirror which timestamps the source FS actually carries per the engine's emission), then call `set_metadata`; **non-fatal** on failure (log + a report field, never fail the item — the og continues past both failure traces).
3. **Report surface:** add per-file `times_restored: true/false` (or the failure reason) to the JSON outcome so the CLI can tell the truth when the destination filesystem refuses.
4. **Tests:** WinDestIo unit (mock handle layer) asserting the call sequence and mask; a writer integration test with a fixture file whose times differ from "now"; the failure path (setter returns error → item still `Done`, report notes).
5. **Acceptance:** recovering the tree JSON fixture produces output files whose mtime equals the scanned source's (within the FS's granularity), including a read-only source.

## Tests to add

- `Writer.RestoresTimestampsAndAttributes` (fails today: no call ever made).
- `Writer.MetadataFailureIsNonFatal` (fails today: no path).
- `WinDestIo.SetMetadataMapsVtableGates` (new API).
- A `tests/ui_abi_check.c` note that the ABI needs no change (engine-internal surface).

## Related findings

- **S-059** — the same export loop's hole handling (the og's `FUN_1405414e0` body is the shared evidence site).
- **M-135** — the same loop's failure-reporting semantics.
- **N-009d refutation** — `modules/writer.md:67-75` needs the correction addendum (the row's premise is the refuted "og never calls SetFileTime").
- **N-090** (active) — the session save-time semantics sibling on the timestamp axis.

## Open questions

None — the only one (which of the three times each per-FS family populates when the mask bits differ) is answered by the engine emission sites already pinned in the timestamp battery (`cr_types.h:43-64` and the per-FS decode table in the s42 evidence).
