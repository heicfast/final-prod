# MINOR N-072 — Btrfs re-scan metadata cache (BtrfsCache.txt) not persisted; ZFS delay-execution import polling absent

**Severity:** MINOR
**Module:** btrfs, zfs
**Parameters affected:** P47/P48 (throughput/latency — re-scan speed), S36 (session lifecycle — cross-scan caching), A25 (algorithm steps — ZFS polling)
**Corpus references:** `re-workspace/artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md:66-67` ("**`BtrfsCache.txt`** (relative path string) — the engine persists a metadata cache file in the DLL's directory to speed re-scans of the same volume (subvolume/chunk trees cached)"), `:86` (ZFS: "Delay-execution import (polling during long walks)")
**Core references:** `src/core/btrfs/cr_btrfs_engine.cpp` (no cache-file write; grep BtrfsCache/cache-file: 0 hits), `src/core/zfs/cr_zfs_engine.cpp` (labels/uberblocks probed once; no delayed re-import/poll loop)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two engine-side performance behaviors the og has and the core lacks: (1) the Btrfs engine persists a metadata cache (subvolume/chunk trees) to `BtrfsCache.txt` beside the engine, speeding re-scans of the same volume; (2) the ZFS engine uses delayed re-import polling during long walks (re-probing labels/uberblocks when the first pass fails or the pool is mid-rewrite). The core re-derives Btrfs trees every scan and probes ZFS labels exactly once.

## What the reference does

remaining-engines (first-cited for these facts this session — the artifact was cited for other engines before): the Btrfs engine writes a relative-path `BtrfsCache.txt` cache of subvolume/chunk trees keyed to the volume, reloaded on re-scan (:66-67). The ZFS engine's import path is polled with delay-execution during long walks (:86) — a retry/backoff loop around label/uberblock discovery.

## What our core does

The Btrfs engine parses chunk/root trees fresh every scan (no persistence; the session file caches the RESULT tree, not the metadata maps). The ZFS engine probes the 8 label slots once per scan; a torn or mid-rewrite pool finds no uberblock and the engine gives up (M-019's multi-vdev fallback is the adjacent failure path).

## Gap

Falsifiable claims: (1) a second scan of the same Btrfs volume is metadata-cache-accelerated in the og and cold in the core; (2) a ZFS pool whose labels are transiently unreadable (hot-unplug/replug, mid-transaction) is retried with backoff by the og and failed immediately by the core.

## Impact

Re-scan latency (Btrfs) and torn-pool recovery success (ZFS) — both narrow, both performance/robustness rather than correctness of what IS recovered. The og's cache-file-beside-the-DLL placement is also a multi-user/readonly-media hazard (worth NOT copying verbatim — see fix plan).

## Reproduction / verification

Corpus-derived (artifact quoted with lines; the strings/class census is the evidence class — string + capa-verified). Core side grep-verified at the audited commit. UNVERIFIED-BY-EXECUTION for the actual speedup factor (no og Btrfs re-scan bench exists — the Windows bench lane's territory).

## Fix plan (for the engineering team)

1. Btrfs cache: if implemented, do NOT write beside the DLL (the og's placement breaks under Program Files/readonly/media-only contexts). Use an explicit caller-supplied cache directory (the session/checkpoint directory pattern) with a content-keyed filename (volume uuid + generation) and a strict validity check (generation match, else discard). Size-bounded, single file, optional flag.
2. ZFS polling: add a bounded retry loop (e.g. 3 attempts, 250 ms/500 ms/1 s backoff, cancel-aware) around the label-scan phase when the first pass finds no valid uberblock — only when the device is a live handle (image files skip the retry: a static image will not heal).
3. Tests: `Btrfs.CacheRoundTripAndInvalidation` (stale-generation cache discarded), `Btrfs.CacheDisabledByDefault` (no file written unless the flag is set), `Zfs.LabelRetryBackoff` (first probe fails, second succeeds via a fault-injecting device).
4. Acceptance: the re-scan of a cached Btrfs volume skips tree re-parse (telemetry counter); the torn-label fixture recovers on retry.

## Tests to add

- `Btrfs.CacheRoundTripAndInvalidation`
- `Btrfs.CacheDisabledByDefault`
- `Zfs.LabelRetryBackoff`

## Related findings

M-019 (ZFS multi-vdev fallback — same engine's robustness family); I-003 (Btrfs core-exceeds-og supersets — the cache would join that ledger as parity work); S-027-lineage (re-scan architecture family).

## Open questions

The og cache's exact format and invalidation rule are not decoded (string evidence only). If the team implements a cache, the corpus offers no grammar to match — register the format as our own (PARITY-DELTAS) rather than claiming og parity.
