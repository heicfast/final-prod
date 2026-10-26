# MAJOR M-025 — Carve scan geometry/read model decoded: og = 8 MiB ping-pong double-buffer, sector-granular dispatch, bitmap cluster-skip, streaming pending objects; core = 1 MiB windows, byte-granular, stateless validators

**Severity:** MAJOR
**Module:** carve
**Parameters affected:** X51 (I/O behavior: read sizes/buffering), X47 (throughput), S32/S33-adjacent (per-position streaming state), A25 (scan-loop structure)
**Corpus references:** `RAWSearchDLL_decompiled.c` — read-size setup: `FUN_18000c430` :9269-9275 (`blocks = 0x400000 / sector_size`) and init `FUN_18000beb0` :9184-9209 (`read_size = blocks × sector × 2` stored at searcher+0xd8 = **8 MiB** for 512-B sectors; buffer = `read_size + 0x1000` at +0xe0, :9207-9209); chunk read fills the back half (`buffer + read_size/2`, :9434-9438); post-chunk `memmove(buffer, buffer+half, half)` (:9620-9622) — ping-pong; candidate = `buffer + half + sector_index × sector_size` (:9496) — **sector-aligned only**; bitmap cluster skip :9451-9459 (`FUN_180058df0` + `local_1e0 += cluster−1`); per-sector zero pre-filters :9498-9510; pending-object model: `CRawFileInfo` created via analyser slot 7 (:9541-9550), moved to the pending list (searcher+0xf8) when size unknown (:9564-9605), re-fed via `FUN_18000d670` → analyser slot 4 `ContinueScan(header, sector, pos, ctx)` (:10114-10116) every sector until done, then finalized (:10117-10144).
**Core references:** `src/core/carve/cr_carve.cpp:28-30` (WINDOW = 1 MiB, OVERLAP = 4096), `:77-78` (byte-granular `for (u64 off = 0; …; ++off)` dispatch), `:306-329` (scan_window_index: single 1 MiB+4 KiB read, no carry-over context), validators re-read extents via `read_at` probe blocks (`cr_analysers.cpp:101,127-163` kProbeBlock = 1 MiB), hashing re-reads ≤8 MiB (`cr_carve.cpp:160-170, 279-289`) — audited commit `34b3441`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's carve scan is now fully decoded: 4 MiB device reads into the back half of
an 8 MiB ping-pong buffer (+4 KiB tail), one dispatch per sector-aligned position
(cheap O(1) map lookups), allocated clusters skipped whole when a bitmap is
supplied, and open-ended formats tracked as pending CRawFileInfo objects extended
across chunk boundaries by per-sector Continue calls. The core scans 1 MiB windows
(+4 KiB overlap) at **byte** granularity with stateless single-shot validators that
re-read file extents from the device (plus an extra ≤8 MiB re-read for hashing).
Results are equivalent-or-better for the core (byte granularity finds mid-sector
starts — I-009), but the structural cost profile differs substantially and the og's
streaming state has no core analog.

## What the reference does

(All citations in the header; mechanics summarized.)
- **Reads:** 8 MiB heap buffer per searcher; each call reads `sector_count` sectors
  (4 MiB worth) into the back half; the front half retains the previous chunk as
  left-context for boundary-spanning validation; memmove rotates after each chunk.
- **Dispatch cost:** one u32 map lookup (lower_bound) + one u16 equal_range per
  SECTOR position; two cheap zero pre-filters per sector; no memcmp per byte.
- **Bitmap:** allocated clusters skipped at cluster granularity (the M-005 gate,
  here confirmed at loop level with the exact skip arithmetic).
- **Streaming state:** formats whose end is not yet known live in the pending list
  and are re-fed every sector until the analyser's Continue returns done — O(1)
  per-sector work per open file, no re-reads of already-scanned bytes.

## What our core does

1 MiB window reads (+4 KiB overlap re-scanned), byte-granular first-byte dispatch
with a 34-entry trial loop per matching byte, validators that re-read their extents
through the device in 1 MiB probe blocks (e.g. JPEG entropy scan, ZIP EOCD scan,
PDF %%EOF scan — each a fresh set of device reads per candidate), and a hashing
re-read of up to 8 MiB per emitted small file. No per-candidate state survives the
window loop; no bitmap consultation (M-005).

## Gap

Structural (not results): for the same coverage the core issues many more device
reads per candidate (validator probe re-reads + hash re-reads vs the og's
already-in-buffer data), and its byte-granular dispatch does ~512× the per-byte
dispatch work of the og's sector-granular model — while in exchange finding
mid-sector starts the og never sees (I-009 superset) and not needing the pending
machinery. No throughput numbers were measured in this session (no comparable og
binary execution possible); the divergence is cited as decoded structure, not as a
measured X47 delta.

## Impact

- Scan-time cost on large volumes: M-005 already priced the whole-slice-vs-bitmap
  half (~2× on half-full volumes); this finding prices the read-model half
  (window size, per-byte dispatch, validator re-reads, hash re-reads). Combined
  they put the core's carve stage well behind the og on volume scans unless the
  bitmap gate (M-005 fix) lands.
- The pending/streaming model matters for anyone porting the og's BruteForce
  analysers (MovAnalyzer/MTS): they assume buffer-resident data + continuation; a
  port must either keep that model or rewrite against the core's stateless
  validator contract (S-017 fix plan step 3 will hit this).

## Reproduction / verification

Decompiled (strength 2), every constant cited: :9269-9275, :9195-9209, :9434-9438,
:9496-9510, :9620-9628, :9541-9605, :10114-10144. Core: code-cited above.
Evidence file §5.

## Fix plan (for the engineering team)

1. Measure before changing: extend the bench harness with a carve-stage timer and
   record per-stage throughput for the current model (feeds X47 with numbers).
2. Cheap wins that preserve behavior: (a) pass the window buffer into validators
   for the hit's first window (a `ReadCtx` with buffer+bounds) to eliminate the
   per-candidate probe re-reads; (b) hash from the validator's reads where the
   extent was already read (or hash the window slice) instead of re-reading ≤8 MiB.
3. Optional geometry change (needs bench evidence): larger windows (4-8 MiB) with
   the og's ping-pong front-half context; keep byte-granular dispatch (I-009
   superset is worth keeping).
4. Document the stateless-vs-pending model difference in PARITY-DELTAS once the
   S-017 fix decides whether any ported analyser needs continuation state.

## Tests to add

`Carve_Hash_No_Second_Read` (count read_at calls per emitted file with a counting
device), `Carve_Window_Buffer_Reused_By_Validator` (after fix 2a), bench-scenario
`carve_stage_throughput` (X47 tracking).

## Related findings

- M-005 (bitmap gate — the other half of the volume-scan cost).
- I-009 (byte-granularity superset — the deliberate core-side trade).
- S-017 (the porting note in Impact).

## Open questions

The og's progress callback granularity (per 4 MiB chunk vs core's per 1 MiB
window) is decoded structurally but its UI weighting (O38) was not re-scored this
session — next probe: the controller's chunk-driver virtual in the host
(RecoveryModule decompile) if O38 needs re-verification.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's M-025 landing (DFX33; sha in the worklog entry)
**Verified by:** `CarveEngine.WindowServingEliminatesPerCandidateRereads` (fail-before EXECUTED: 52 device reads pre-fix vs 4 post-fix on the 4 MiB / 4-plant fixture — RED on the pre-fix wiring, GREEN after) + `CarveEngine.WindowServingServesStraddlingReadsCorrectly` (straddle correctness: a plant crossing the window end validates with the exact extent/length/checksum of a direct read; holds both states) + the full carve suite 46/46 + full suite **799/799** linux-release at the fix commit (parallel-lane landings included in the count).
**Resolution notes:** Fix-plan items 2a+2b implemented as the WindowDevice read context: scan_one_window wraps the slice for the window's duration; reads fully inside [base, base+want) are served from the window buffer (memcpy, zero device traffic); reads straddling a window edge serve the in-window part from the buffer and read only the remainder from the device — byte-identical content by construction (the buffer IS the device's bytes for that range, read once at window fill). Wired: every grammar validator call, both checksum sites (512 B M-051 re-read), both hash sites (<=8 MiB re-read), and the raw-sign tail-scan reads. The M-021 post-carve hook keeps the raw slice (host contract clarity). MEASURED (fix-plan item 1, the counting-device execution the finding's own test list asks for): 4 MiB fixture, 4 small PNG plants (one per 1 MiB window), hashing on — pre-fix 52 device read_at calls (4 window fills + 48 per-candidate: validator probe re-reads + checksum + hash); post-fix 4 calls (window fills only) — **13x fewer device reads, zero per-candidate re-reads for in-window candidates**; straddling probes (the 1 MiB JPEG/MP3 block reads a 1 MiB window can never contain) degrade to the in-window-prefix + device-tail split, never more bytes than the pre-fix full read. Fix-plan item 3 (4-8 MiB ping-pong geometry) NOT taken: the finding's own rule requires bench evidence for geometry changes, and the executed-transport comparison is Windows-bench territory (the finding records "no comparable og binary execution possible" in this environment; same posture as M-009's Resolution) — the 1 MiB window + byte-granular dispatch stays, preserving the I-009 mid-sector superset. Item 4 documented: PARITY-DELTAS D-80 (stateless validators vs the og's pending CRawFileInfo/ContinueScan objects — the core's stateless contract is the porting boundary S-017 must respect). Safety hardenings preserved: G-05 fill-rate accounting, FB-C01 unreadable-window skip, M-005 bitmap gate, N-031 zero-stride skip, N-070 score-ordered emission, M-051 checksum/anchor-B, S-031 timestamps, S-032 inventories — all re-proven by the untouched 46-test carve suite.
