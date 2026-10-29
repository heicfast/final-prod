# SEVERE S-059 — sparse/hole files: the writer left-shifts content over holes or refuses the whole file (the og always writes full logical size)

**Severity:** SEVERE
**Module:** writer / recovery (file reconstruction — the user's focus battery)
**Parameters affected:** #2 Recovery correctness (byte-for-byte); #27 Fragment handling; #5 Orphan reconstruction behavior
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:874797-875088` (chunked copy: `FUN_1405421a0` normalizer + `FUN_140542590` writer); `:875503-875855` (`CFileRecoverWorker::ReadAndWriteDataRunList`); `:875661-875673` (hole → zeroed buffer written, read skipped); `:874877-874889` + `:874969-874970` (sparse chunk typing in the normalizer)
**Core references:** `src/core/ntfs/cr_runlist.cpp:187-197` (`RunList::extents()` drops sparse runs); `src/core/ntfs/cr_mftwalk.cpp:262-263` + `:271` + `:340-352` (the compressed-unit expander emits no extents for holes while its own comments claim "the writer zero-fills"); `include/cr/tree/cr_node.h:109-120` (`Extent` has no logical-offset field); `src/core/recovery/cr_safewriter.cpp:405-455` (the cumulative extent walk + `ERR_TRUNCATED`; lines re-pinned at the merged HEAD — :442-445); `:277-290` (the metadata-only enqueue skip) — all at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

For files containing sparse holes (NTFS sparse files with `sparse` runs, and NTFS compressed files whose all-zero compression units become holes), the og's export writes every logical byte in order — holes come out as zeros, reads are skipped for them, and the full logical size is always produced. The core's pipeline drops hole runs from the extent list entirely, and its writer places each surviving extent at a cumulative logical position with no per-extent logical offset: interior holes left-shift subsequent data over the hole, and when the extent sum falls short of the logical size the writer fails the whole file with `ERR_TRUNCATED` and deletes the temp — no output at all. A code comment in the walk asserts a zero-fill the writer cannot perform.

## What the reference does

Two export paths, both decoded first-hand this session:

**The runlist path** (`CFileRecoverWorker::ReadAndWriteDataRunList`, `:875503-875855`): per data run, a 1/10 MiB buffer is `memset` to zero **first** (`:875661`); a **sparse run (LCN == −1) skips the read entirely** — `if (*plVar6 != -1) { …read… }` (`:875662-875673`) — and the pre-zeroed buffer is then written anyway (`:875699-875706`): sparse = zeros at full run length, size preserved. The read range is clamped to the volume's total size when a run extends past the end (`:875663-875668`).

**The chunked path** (`FUN_1405421a0` → `FUN_140542590`, `:874797-875088`): the normalizer chunks the datarun list into 16-unit blocks; a run whose offset field is **−1 (sparse/hole)** becomes a type-1/type-2 chunk descriptor (`:874877-874889`, re-typing of the final chunk at `:874969-874970`). In the writer, a **type-2 (full sparse) chunk** takes a 1 MiB zero-filled buffer and is written with **no read at all** (`:875062-875075`); a type-0 (data) chunk reads and writes; a type-1 (partial sparse) goes through the nested runlist reader with a pre-zeroed buffer (`:875057-875060`, `:875111-875112`). Net: every logical byte of the file is written in order; holes are zero-filled; **the file always reaches its full logical size**.

## What our core does

- `RunList::extents()` (`cr_runlist.cpp:187-197`) filters sparse runs out of the extent list: `if (r.sparse) continue;` — a mid-file hole simply vanishes, and adjacent real runs are coalesced.
- The compressed-unit expander (`cr_mftwalk.cpp:262-263`, `:344-352`) emits **no extent** for a hole unit (`header == 0 → hole: the rest of the unit is sparse` / `remaining blocks are holes, which emit no extents`) while its own header comment claims "the writer zero-fills" (`:263`, `:271`) — a comment-vs-code defect: the writer has no hole knowledge to zero-fill.
- `FileProperties::Extent` (`cr_node.h:109-120`) carries only `{offset, length, codec, psize}` — **no logical offset** — so the writer (`cr_safewriter.cpp:405-455`) walks extents **cumulatively**: extent i is assumed to occupy logical `[Σ len[0..i-1], +len[i])`.
- Consequences, both verified in the writer body: (a) an **interior hole** left-shifts every later extent's data over the hole position; (b) when the extent sum falls short of `logical_size` with no resident data, the item fails with `ERR_TRUNCATED` (`:442-445`) and `close()` deletes the temp — **no output file at all**; (c) with resident data the tail is zero-filled but interior gaps still shift.
- No test covers this: `tests/unit/test_runlist.cpp:36-49` proves sparse runs are parsed and `vcn_to_lcn` reports them — the **export** side has no sparse-file test anywhere (no `test_safewriter.cpp` exists; zero sparse hits in writer tests).

## Gap

Stated falsifiably: for an NTFS file of logical size 3 MiB with an interior 1 MiB sparse run, the og recovers a 3 MiB file whose middle MiB is zeros and whose last MiB is the correct data; the core either (a) fails the file with `ERR_TRUNCATED` and writes nothing, or (b) if it had resident data, writes a shorter file with the tail data shifted 1 MiB early. Both are wrong recovery results for a real population: NTFS sparse files and NTFS compressed files with all-zero units.

## Impact

Recovery correctness and silent data loss: sparse-file populations (system files, databases, download-pre-allocated files) and compressed files with zero-run units either fail wholesale (user sees an error count, no file) or — worse — recover with content at wrong offsets when resident data is present, which is silent corruption of the recovered artifact. This is squarely the og-parity bar the project set for reconstruction correctness (file reconstruction is this session's focus battery; a verified divergence is filed severe).

## Reproduction / verification

- **Decompiled evidence (first-hand):** the zero-then-skip-read shape at `:875661-875673`; the beyond-EOF clamp at `:875663-875668`; the type-2 zero-chunk write at `:875062-875075`; the sparse chunk typing at `:874877-874889`. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-reconstruction-export-chain.md`.
- **Core evidence (first-hand):** the sparse filter at `cr_runlist.cpp:190-191`; the cumulative walk + `ERR_TRUNCATED` at `cr_safewriter.cpp:415-444`; the no-logical-offset `Extent` at `cr_node.h:109-120`; the hole-emits-no-extent code with its contradicting comments at `cr_mftwalk.cpp:262-271` + `:340-352`; the enqueue metadata-only gate at `cr_safewriter.cpp:277-290`.
- **Execution status:** UNVERIFIED-BY-EXECUTION for the end-to-end recover comparison (no NTFS image with a sparse run in the fixture set); the mechanism is fully code-proven on both sides. The prescribed reproduction for the fix lane: craft a runlist with `0x0x` hole entries (or an LZNT1 unit of zero headers), scan, `recover`, and diff against a `fsutil file setzerodata`-shaped original.

## Fix plan (for the engineering team)

1. **Carry logical position:** extend `FileProperties::Extent` with a `logical` start (u64), set at emit time (`RunList::extents()` knows the VCN; the compressed expander knows `unit_start + b*cluster` — it already computes it at `cr_mftwalk.cpp:340-342`). Bump the session record v2 codec if the wire changes (keep back-compat read for old sessions: `logical = cumulative` fallback).
2. **Emit hole extents** rather than dropping them: a hole becomes `Extent{offset=0, length=hole_len, codec=0, hole=true}` (or `logical` + a `sparse` flag) so the writer's accounting sees the full logical span. `RunList::extents()` must stop `continue`-ing sparse runs.
3. **Writer:** map `done` through the per-extent `logical` field; zero-fill `logical` gaps up to the next extent (the og's type-2 zero chunk); keep the atomic temp+rename posture. The `ERR_TRUNCATED` refusal then only fires for genuinely truncated metadata, matching the og's per-run clamp-and-continue instead of whole-file refusal where the og clamps at volume end (`:875663-875668`).
4. **Fix the misleading comments** at `cr_mftwalk.cpp:262-263`/`:271` once the writer really zero-fills.
5. **Tests:** (i) runlist decode + extents with an interior hole → extent list sums to logical size with a hole marker; (ii) writer round-trip: 3 MiB file, 1 MiB interior hole → output bytes 0, zeros, tail data at correct offsets, file size 3 MiB; (iii) compressed unit of zero headers → same; (iv) session round-trip of the new field; (v) a fail-before proof: today's `ERR_TRUNCATED` on the same fixture.

## Tests to add

- `Runlist.SparseHoleEmitsExtent` (fails today: hole dropped).
- `Writer.InteriorHoleZeroFilledFullSize` (fails today: `ERR_TRUNCATED`, no output).
- `Writer.CompressedZeroUnitIsHole` (fails today).
- `Session.ExtentLogicalRoundTrip` (new field).

## Related findings

- **M-135** — the read-failure semantics batch (the og's other partial-content policy on the same export path).
- **M-136** — the recovery read pipeline bypass (transient read errors become permanent holes — interacts with this fix's zero-fill policy).
- **M-119** (active) — the scan-side clamp-vs-refuse sibling of the og's beyond-EOF clamp at `:875663-875668` (the export-side clamp site was not cited there; this finding adds it).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-059 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `RunList.SparseHoleEmitsExtent` + `RunList.ContiguousCoalesceRequiresLogicalContiguityToo` (RED pre-fix [emission neutered]: the hole dropped, 2 extents; GREEN: 3 extents — the hole keeps its VCN, the tail run's logical position jumps past it, disk-contiguous-but-VCN-gapped runs never coalesce), `WriterDirs.InteriorHoleZeroFilledFullSize` (the finding's falsifiable case: 3 MiB file, 1 MiB interior hole → the FULL 3 MiB materializes, head 0xAA at [0,1MiB), the hole reads 0x00 — RED pre-fix [hole branch neutered]: "byte 0 is ee want 0", the bogus device-read class — the tail 0xBB at its TRUE [2MiB,3MiB) offset, never ERR_TRUNCATED), `WriterDirs.LegacyCumulativeExtentsUnchanged` (the back-compat pin: unstamped extents take the cumulative walk, byte-identical to the pre-S-059 writer), `Crsf.ExtentLogicalRoundTrip` (the logical + hole fields survive the session wire, nflags bit 2048), `NtfsVolume.CompressedDataRecoversRealContent` RE-PINNED (the all-zero-header unit now emits exactly ONE hole extent covering the unit's logical span — the old pin "a hole unit emits no extents" pinned the defect). Suite 1287/1287 + cr_ui 77/77 (linux-release).
**Resolution notes:** CONFIRMED-DIVERGENCE, corpus re-verified first-hand before fixing: the zero-then-skip-read shape at :875661-673 (the pre-zeroed buffer written in full, the sparse run's read skipped), the type-2 full-sparse chunk at :875062-75 (1 MiB zero buffer, NO read), the sparse chunk typing at :874877-889. The fix per the finding's plan: (1) `FileProperties::Extent` gained `logical` (u64) + `hole` (bool); (2) `RunList::extents()` EMITS sparse runs (lcn 0, sparse mark, the run's VCN) instead of dropping them, and the contiguous-coalesce now requires VCN contiguity beside LCN contiguity; (3) all four mftwalk emit sites stamp `logical = vcn * cluster_bytes` and the hole mark; the compressed-unit expander emits a hole extent for a zero header AND for a stored stream that ends mid-unit (the rest of the unit's logical span, clamped to real_size) — the stale "the writer zero-fills" comments re-truthed; (4) the writer's positioned mode (auto-detected: any hole or nonzero logical; the legacy shape keeps the cumulative walk — old sessions and unstamped engines are byte-identical by construction) zero-fills logical gaps and hole extents with NO device read (the og's type-2 chunk), so ERR_TRUNCATED now fires only for genuinely truncated metadata — the hole-explained gaps no longer reach it; (5) the session wire carries the new fields under nflags bit 2048 (old files read back logical==0/hole==false → the legacy walk). The beyond-volume clamp question the finding added to M-119 stays with that finding (scan-side); the export-side clamp site is now the positioned walk's bounds.
