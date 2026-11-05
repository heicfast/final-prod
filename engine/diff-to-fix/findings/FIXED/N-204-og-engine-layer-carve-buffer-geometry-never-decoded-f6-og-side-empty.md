# MINOR N-204 — the og's engine-layer carve buffer geometry (4 MiB block / 8 MiB double buffer / half-buffer look-back) was never decoded into the parity table: SCAN-ENGINE-DELTA F6's og-side cell is empty of constants

**Severity:** MINOR
**Module:** carve (engine-layer read model)
**Parameters affected:** #51 I/O behavior (read sizes, buffering), #47 Throughput (the carve loop's read shape), #26 Tree walk logic (the analyser window derivation)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:9271-9275` (FUN_ region: `0x400000 / sector_size` → block sectors at obj+0xc8; `×2` double buffer at obj+0xd8 via the +0xf0 multiplier), `:9434-9438` (the block read issued into `buffer + half`), `:9505-9526` (the analyser base = `start − half_buffer/sector_size`, with the CRawFileCellTestStorageDevice 0x200 fallback), `:9616-9622` (the slide: `memmove(buf, buf + half, half)` per block advance), `:9494-9502` (the first-u32 registry-key probe at sector stride)
**Core references:** `src/core/carve/cr_carve.cpp:45-46` (kWindow / kOverlap constants — the 1 MiB window + 4 KiB overlap), `:918` (the raw-sign 512-stride loop), `:575` (the grammar layer's per-byte loop), `:1195` (the window fill); registered as SCAN-ENGINE-DELTA F6 ("1 MiB window + 4 KiB overlap, 8 MiB hash cap, 256 MiB max extent — all registered constants") — at audited core `9633d94f`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The core's carve window constants are registered as a deliberate divergence (SCAN-ENGINE-DELTA F6), but the table's og-side cell for F6 never carried the og's actual engine-layer constants — they were never decoded until this session. The og's RAWSearchDLL carves in **4 MiB read blocks inside an 8 MiB double buffer** with a **half-buffer (4 MiB) look-back**: each block advance slides the buffer by one half and re-runs the analyser family over the retained half plus the new half. The parity table's F6 row therefore registers "we differ from the og" without ever pinning what the og's side of the difference IS at the engine layer (the 4 MiB figure previously cited in the trail is the DEVICE-layer chunk from M-030/OQ-S4-3, a different surface).

## What the reference does

The block-scan loop's setup (RAWSearchDLL:9271-9275): the block size in sectors is `0x400000 / sector_size` (4 MiB worth of sectors, stored at obj+0xc8), and the buffer allocation is `block_sectors × *(int*)(obj+0xf0) × 2` (obj+0xd8) — a **double** buffer, two blocks' worth. Each iteration (the read at `:9434-9438`) issues the device read into `buffer + half` — the second half — preserving the previous half as look-behind context. The analyser dispatch base (`:9505-9526`) starts at `start − half/sector_size` — i.e. every analyser sees the retained half-block of history before its probe position, which is how formats whose headers sit behind the nominal probe grid (or whose validators need preceding context) stay findable across block boundaries. The advance (`:9616-9622`) is a `memmove(buf, buf+half, half)` slide — O(half) per block, amortized into the read cost. The probe positions themselves iterate at **sector stride** with a first-u32 registry-key compare (`:9494-9502`). With the shipped 512-byte geometry this is: 4 MiB reads, 8 MiB resident, 8192 sectors of look-back, one sector between probe positions.

## What our core does

`cr_carve.cpp:45-46` defines the 1 MiB window with a 4 KiB overlap; the window fill (`:1195`) reads through the slice, the grammar layer walks every byte (`:575`), the raw-sign layer probes at 512-byte strides (`:918`), and validators re-read through the per-window `WindowDevice` (the M-025 landing). The constants and their rationale are registered at SCAN-ENGINE-DELTA F6 — but F6's og-side cell says only "Carve params: window/overlap, hash threshold, free-map gate, max extents", with no decoded og constants to compare against.

## Gap

Stated falsifiably: SCAN-ENGINE-DELTA F6's og-side cell contains zero og constants; the og's engine-layer geometry (4 MiB block / 8 MiB double buffer / 4 MiB look-back / sector-stride probes) is now decoded (this finding's citations) but registered nowhere in `diff-to-fix/` or `docs/PARITY-DELTAS.md`. Any future tuning decision on the core's window size has no cited og baseline at the layer where carving actually reads.

## Impact

Audit-trail completeness, not behavior: the core's carve results are unaffected. But the parity table's F6 row is currently unfalsifiable ("we chose differently" — differently from WHAT, at which layer?), and the engine-layer 4 MiB was previously conflated with the device-layer 4 MiB (M-030's chunking) — two different surfaces that happen to share a number. This finding pins both, separated, with citations. MINOR (registration gap; the perf comparison itself is OQ-S4-3's territory).

## Reproduction / verification

All five og sites read first-hand this session (the 0x400000 division and ×2 buffer sizing at `:9271-9275`; the half-offset read target at `:9434-9438`; the `start − half/sector` analyser base with the 0x200 fallback at `:9505-9526`; the memmove slide at `:9616-9622`; the sector-stride u32 probe at `:9494-9502`). Core constants read at `cr_carve.cpp:45-46/:575/:918`. Worksheet: `evidence/s55-lane-sheets/S55-D2-scanmath.md` §1 (the decode) + the divergence-candidate DC-1.

## Fix plan (for the engineering team)

1. Documentation-only: extend SCAN-ENGINE-DELTA F6's og-side cell with the decoded geometry (4 MiB block / 8 MiB double buffer / half-buffer look-back / sector-stride first-u32 probes / 8192-sector analyser base at 512 bps — cites RAWSearchDLL:9271-9275/:9434-9438/:9505-9526/:9616-9622/:9494-9502).
2. Add the explicit note separating engine-layer 4 MiB (this finding) from device-layer 4 MiB (M-030/OQ-S4-3) so the two surfaces stop being conflated.
3. Cross-link OQ-S4-3's acceptance criteria: any comparative chunking bench must state which layer it varied.
4. No code change. No new tests (nothing behavioral).

## Tests to add

None (registration-only finding).

## Related findings

OQ-S4-3 (the unmeasured relative-perf question this decode feeds), M-030 (the device-layer chunking — the layer distinction this finding draws), N-181 (raw-first ordering — the same scan-loop family), D-59c (the byte-granular probe superset — the probe-stride contrast at the grammar layer).

## Open questions

None (the +0xf0 buffer multiplier's exact value — whether the og ever sizes the double buffer at other than 2× — has no writer in the decompile; the shipped path is ×2 and that is what this finding registers).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's landing, session-60 MINOR-first lane — the post-push SHA recorded in the worklog entry)
**Verified by:** The registration itself — SCAN-ENGINE-DELTA F6's og-side cell re-read at HEAD carries the full decoded constant set (4 MiB block / 8 MiB double buffer / half-buffer look-back / sector-stride first-u32 probes / 8192-sector analyser base at 512 bps / the CRawFileCellTestStorageDevice 0x200 divisor fallback) with all five RAWSearchDLL cites; the five corpus sites re-verified first-hand this session before the registration landed (`:9271-9275` the 0x400000/sector_size block computation and the ×2 double-buffer sizing; `:9434-9438` the read issued at `buffer + half`; `:9505-9526` the analyser base `start − half/sector_size` with the 0x200 fallback; `:9616-9622` the `memmove(buf, buf+half, half)` slide; `:9494-9502` the sector-stride first-u32 probe). No tests (registration-only finding, per the finding's own fix plan: "No code change. No new tests (nothing behavioral)").
**Resolution notes:** The fix plan's three documentation steps executed exactly: (1) F6's og-side cell extended with the decoded geometry and every cite; (2) the explicit engine-layer-vs-device-layer 4 MiB separation note added inside the cell (M-030/OQ-S4-3 named, the conflation called out); (3) the OQ-S4-3 acceptance-criterion cross-link embedded ("any comparative chunking bench must state which layer it varied"). The +0xf0 multiplier's no-writer observation kept verbatim in the cell (the shipped path is ×2, registered as such). The core's own constants (1 MiB window + 4 KiB overlap) unchanged — the finding is the registration gap, not a behavior divergence; the core's deliberate geometry divergence note (D-59c's probe-stride contrast) now sits beside a cited og baseline instead of an empty cell.
