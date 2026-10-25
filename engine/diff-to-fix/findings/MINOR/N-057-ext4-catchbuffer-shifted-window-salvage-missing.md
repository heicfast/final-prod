# MINOR N-057 — ext4 bad-sector salvage: og re-reads a 1 KiB-backtrack shifted window (CatchBuffer proxy); core zero-fills the failed extent

**Severity:** MINOR
**Module:** ext4
**Parameters affected:** A27 (fragment handling — partial salvage), S35 (error recovery — bad-sector strategy), B2 (recovery correctness at window edges)
**Corpus references:** `re-workspace/artifacts/SearchDLL/ext4-engine--deep-dive.md:25-33` ("`CEXT4CatchBufferDeviceProxy` … on read failure the proxy allocates a fresh 0xE00-byte buffer object and **retries with a shifted window** (lVar8-0x400 = 1KB-backtrack re-read) — the 'catch buffer' catches short/failed reads and salvages what it can instead of failing the whole group scan. This matches DRW's 'Program is trying to skip bad sectors to find data' UX copy — the EXT4 engine implements that skip-and-salvage behavior most literally.")
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:368-372` (content-read salvage: `if (v.dev->read_at(...) != OK) { /* salvage: keep zeros for this extent, continue (catch-buffer behavior - the transport retry layer already isolated the bad sectors) */ }`); no shifted-window/backtrack re-read anywhere in ext4 or transport (grep backtrack/0x400-shifted: 0 hits)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

On a failed/partial read, the og's ext4 engine re-reads the region through a 1 KiB-backtrack shifted window to salvage the readable bytes at the failure boundary; the core zeroes the failed span (after the transport layer's sector-granular retries) and continues. The salvage mechanism differs: shifted-window re-read vs. zero-fill-after-retry.

## What the reference does

ext4-engine--deep-dive.md:25-33 (first-cited for this mechanism this session — the artifact's §3): the CatchBuffer device proxy wraps the volume device; on read failure it allocates a fresh 0xE00-byte (3584-byte) buffer and retries at `offset - 0x400` (1 KiB backtrack), catching short/failed reads and salvaging partial data instead of failing the group scan.

## What our core does

The ext4 content path (cr_ext4_engine.cpp:368-372) reads each extent; on failure it leaves the already-zeroed buffer span and continues — the comment claims catch-buffer-equivalent behavior on the theory that the transport retry layer isolated the bad sectors. The transport retries at sector granularity but never re-reads a SHIFTED window (the backtrack mechanism does not exist in the core).

## Gap

Falsifiable claim: for a read that fails at a window boundary (e.g. the last 512 bytes of a 4 KiB read are unreadable), the og salvages up to the boundary + backtrack-window content; the core zeroes the whole affected extent span and recovers less content at the edges of bad regions.

## Impact

Partial-content salvage at bad-sector boundaries on ext4 volumes — a data-completeness delta (fewer recovered bytes in the boundary zones), not a correctness delta (nothing wrong is recovered). Concentrated on physically-damaged media where reads partially succeed.

## Reproduction / verification

Corpus-derived (deep-dive §3 quoted; the mechanism is decompile-derived with vftable evidence per the artifact's ledger). Core side read in-source; transport grep-verified for the backtrack absence. UNVERIFIED-BY-EXECUTION (needs a fault-injecting device that fails partial windows — the fix's test provides one).

## Fix plan (for the engineering team)

1. In the ext4 content-read path, on a failed `read_at` over an extent: attempt one shifted re-read — the 1 KiB-backtrack window (read `[start - 0x400, start - 0x400 + span)` when `start >= 0x400`, or clamp) — and use whichever bytes succeed, zero-filling only the truly unreadable remainder. Honor the cancel token between attempts.
2. Bound it: one backtrack attempt per extent (the og's loop shape is a single retry with a fresh buffer), no retry storms.
3. Consider whether the same salvage belongs in the other engines' content paths (the corpus documents it as ext4-unique — "unique to this engine" per the artifact title — so keep it ext4-scoped for parity).
4. Tests: `Ext4.CatchBufferBacktrackSalvage` (fault-injecting device fails the tail of a 4 KiB window; the backtrack read recovers the boundary bytes — assert nonzero content where the og would salvage), `Ext4.ZeroFillOnlyWhenBacktrackAlsoFails`.
5. Acceptance: the fault-injection fixture recovers the boundary bytes; the healthy-volume suite is unaffected.

## Tests to add

- `Ext4.CatchBufferBacktrackSalvage` — fails today (zero-fill)
- `Ext4.ZeroFillOnlyWhenBacktrackAlsoFails`

## Related findings

M-032 (slice tail-read clamp — the same storage-boundary family); S-003-family HFS+ journal replay is the parallel lane's active fix (no overlap); I-014 (ext4/HFS+ superset register — ext4-side robustness ledger).

## Open questions

The 0xE00 (3584) buffer size's exact role (3.5 KiB = 7×512) vs the 1 KiB backtrack is decompile-shape; the fix plan's per-extent single backtrack preserves the observable behavior without replicating the buffer arithmetic.
