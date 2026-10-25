# MINOR N-012 — NTFS compressed/sparse flag propagation: $SI vs $FILE_NAME source mismatch

**Severity:** MINOR
**Module:** ntfs
**Parameters affected:** B7 (hard-link/ADS/sparse), B9 (timestamps — same source-mismatch shape), D11 (node fields — `attributes` field), D15 (confidence grading — sparse flag informs grading)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:5163` (`props+0x12 |= 0x800` for sparse `$DATA`); `:5167` (`props+0x12 |= 0x40` for compressed `$DATA`); `:5153-5154` (sparse+compressed converted to compressed-only at props level); `:5351` (`uVar4 = FUN_180044f60(lVar10)` reads `$SI.dos_flags`); `:5329` (`if (uVar4 != 0) { uVar14 = 0x10; local_86 = 0x10; }` — directory bit from `$FILE_NAME`)
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:129-134` (internal `data_compressed_out`/`data_cunit_out` build-time locals, used by `expand_compressed_data` at `:282-381`, then discarded — not stored in props); `:735` (`props->attributes = u16(best->dos_flags & 0xFF) | (is_dir ? 0x10 : 0)` — uses `$FILE_NAME`'s `dos_flags`, masks to low 8 bits, drops 0x400 OFFLINE) — audited commit `5d6da5f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's `NTFSSearchDLL` sets two parallel flag sources: (a) an
internal `+0x12` byte on the props object carrying sparse (0x800) and
compressed (0x40) flags from `$DATA`, AND (b) the serialized `+0x23`
flags byte sourced from `$STANDARD_INFORMATION.dos_flags` (the
canonical attributes). The core's `cr_mftwalk.cpp` only sets the
serialized `props->attributes` from `$FILE_NAME.dos_flags` (not `$SI`'s)
and masks to low 8 bits, dropping the OFFLINE bit (0x400). For files
where `$SI` and `$FILE_NAME` `dos_flags` disagree (rare; happens when
attributes were changed after the last name update), the og's serialized
props flags reflect `$SI`; the core's reflect `$FILE_NAME`. Also, the
og's internal `+0x12` carries compressed/sparse flags that downstream
code could query; the core has no equivalent persistent internal flag.

## What the reference does

```c
// NTFSSearchDLL_decompiled.c:5153-5167 (FUN_1800069e0 — $DATA processor)
// At :5153-5154: if both sparse AND compressed set on $DATA, treat as compressed-only.
if ((sparse_flag) && (compressed_flag)) {
    sparse_flag = 0;           // convert to compressed-only at props level
    compressed_flag = 1;
}
// At :5163:
if (sparse_flag)     props->internal_flags_0x12 |= 0x800;     // sparse
// At :5167:
if (compressed_flag)  props->internal_flags_0x12 |= 0x40;      // compressed

// :5351 (FUN_180006db0 — props builder)
uVar4 = FUN_180044f60(lVar10);        // read $SI.dos_flags
local_86 = (u8)uVar4 | 0x01 | 0x02 | 0x04 | 0x20 | 0x400;     // serialized flags from $SI
// (with various bit-set gates based on $SI's flags)
```

The og's serialized `+0x23` byte carries `$SI.dos_flags` (the canonical
attributes — read-only, hidden, system, archive, offline, etc.).
Compressed bit 0x800 is NOT propagated from `$SI` to `+0x23`
(compression is set per-`$DATA`, not per-record; the `+0x12` internal
flag is where it lives).

## What our core does

```cpp
// src/core/ntfs/cr_mftwalk.cpp:129-134 (audited 5d6da5f)
bool data_compressed_out = false;
u32 data_cunit_out = 0;
// (build-time locals — used by expand_compressed_data at :282-381, then discarded)

// src/core/ntfs/cr_mftwalk.cpp:735 (audited 5d6da5f)
props->attributes = u16(best->dos_flags & 0xFF) | (is_dir ? 0x10 : 0);
//   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//   - `best` is the namespace-best $FILE_NAME attribute (NOT $SI)
//   - `dos_flags` is $FILE_NAME's flags field
//   - `& 0xFF` masks to low 8 bits (drops 0x400 OFFLINE, 0x800 COMPRESSED)
//   - `| 0x10` adds the directory bit if is_dir
```

## Gap

Two parallel gaps:

1. **Source mismatch:** the og serializes `$SI.dos_flags`; the core
   serializes `$FILE_NAME.dos_flags`. They usually agree (when a file
   is renamed, both attributes are updated atomically), but they can
   disagree if attributes were changed after the last name update.
2. **Bit mask:** the og preserves bits 0x400 (OFFLINE) and others in
   the high byte; the core masks to `& 0xFF` (low 8 bits only),
   dropping 0x400 OFFLINE and any high-bit flags.

## Impact

- **Result-set correctness:** the file's content is correctly recovered
  regardless; only the `attributes` field on the result node differs.
- **UI display:** the og's UI shows the canonical attributes (read-only,
  hidden, system, archive, offline, etc.); the core's UI may miss
  OFFLINE-flagged files (e.g. HSM-migrated files on tiered storage).
- **Filter:** the query layer's `Filter::origin_mask` doesn't depend on
  `attributes`; the divergence doesn't affect filtering. But a future
  filter on attributes (e.g. "show only hidden files") would diverge.
- **Parity benchmark:** any og-vs-core `attributes` field comparison
  diverges on files where `$SI` and `$FILE_NAME` disagree, OR on
  OFFLINE-flagged files.

## Reproduction / verification

- **Decompiled evidence:** first-hand read of
  `NTFSSearchDLL_decompiled.c:5153-5167` (the `+0x12` internal flags)
  and `:5351` (the `+0x23` serialized flags from `$SI`).
- **Core evidence:** first-hand read of `cr_mftwalk.cpp:129-134` (the
  build-time locals) and `:735` (the serialized `attributes` from
  `$FILE_NAME`).
- **Executed comparison:** not run — would require a fixture where
  `$SI.dos_flags` and `$FILE_NAME.dos_flags` deliberately disagree.

## Fix plan (for the engineering team)

1. **Source:** change `cr_mftwalk.cpp:735` to use `$SI.dos_flags` when
   `$SI` is available, falling back to `$FILE_NAME.dos_flags` only
   when `$SI` is absent (mirrors the time-precedence rule at `:734`).
2. **Bit mask:** change `& 0xFF` to `& 0x7FF` (or full u16) to preserve
   the OFFLINE (0x400) and other high-bit flags.
3. **Files to modify:** `src/core/ntfs/cr_mftwalk.cpp:735` only.
4. **Tests to add:**
   - `NtfsVolume.AttributesFromSIWhenAvailable` — fixture: `$SI.dos_flags`
     = 0x02 (hidden), `$FILE_NAME.dos_flags` = 0x00 (no flags). Assert
     `props->attributes & 0x02`. FAILS today.
   - `NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent` — fixture:
     no `$SI`. Assert `props->attributes` reflects `$FILE_NAME`. PASSES
     today.
   - `NtfsVolume.AttributesPreserveOfflineBit` — fixture: `$SI.dos_flags`
     = 0x400 (offline). Assert `props->attributes & 0x400`. FAILS today.
5. **Acceptance criteria:** the three tests above pass.

## Tests to add

- `NtfsVolume.AttributesFromSIWhenAvailable` (FAILS today)
- `NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent` (regression guard)
- `NtfsVolume.AttributesPreserveOfflineBit` (FAILS today)

## Related findings

- **S-001** (FIXED) — the destroy rule's sparse-run accounting sub-fix
  already touched `reused_ratio`'s sparse handling. N-012 is the
  parallel "the props layer doesn't carry the sparse/compressed flag
  persistently" gap. They compose (the destroy rule checks the bitmap;
  the props flag is informational).
- **M-007 / M-008 / N-011** — the parallel SUPERSET divergences. N-012
  is more subtle: it's not a SUPERSET (the core does LESS than the og
  on the high bits); it's a source-mismatch.

## Open questions

- Should the core add an internal `compressed` / `sparse` flag to
  `FileProperties` for downstream consumers? The og's `+0x12`
  internal flag is queryable by the writer/UI; the core has no
  equivalent. The query layer doesn't need it (no filter on
  compressed/sparse); the writer doesn't need it (it always expands
  compressed data via GAP-P1-1). Low priority — only file if a
  downstream consumer surfaces a need.
