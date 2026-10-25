# MAJOR M-001 — NTFS bootstrap rejects record sizes the reference clamps (torn-DBR availability gap)

**Severity:** MAJOR
**Module:** ntfs
**Parameters affected:** B1 (detection correctness), S35 (error recovery), C20 (on-disk struct offsets and sizes)
**Corpus references:** `re-workspace/artifacts/SearchDLL/ntfs-engine-algorithms-v2--deleted-viability.md` §1 (record-size acquisition, lines :21130–21137 agent-B verified); `re-workspace/artifacts/SearchDLL/ntfs-engine-internals.md` §"MFT acquisition" (record size via clusters-per-record or negative exponent)
**Core references:** `src/core/ntfs/cr_bootstrap.cpp:51–61` (`validate_boot_sector` record-size block), `include/cr/ntfs/cr_ntfs_wire.h:208–217` (`record_size_from_dbr`) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og engine never rejects an NTFS volume over its MFT record-size field: a value < 1 falls
back to 0x200 and the working buffer is clamped up to `max(0x400, record_size)`. Our bootstrap
**returns `ERR_BAD_PARAMETER`** when the decoded record size is 0 or not a power of two, and when
it does not divide the sector size. On torn/quick-format-damaged DBRs this refuses the whole
volume (and its remnant scan), where the og proceeds with a clamped record size.

## What the reference does

Decompiled `FUN_18001acc0` prologue (agent-B spot-verified quote, B-ntfs.md §3.1):

```
record_size = device.vt[0x28]();  if < 1 → 0x200 fallback        [:21130-21133]
buf_size = max(0x400, record_size)                               [:21135-21137]
```

No power-of-two check, no divisibility check, no rejection path. A garbage
clusters-per-record byte degrades the walk (records likely fail fixup) but the engine keeps
scanning — and the remnant/post-format path (FUN_18002a780 shifted-header salvage) still runs.

## What our core does

`src/core/ntfs/cr_bootstrap.cpp:51–61`:

```cpp
u64 record_size = record_size_from_dbr(bs, bs.bytes_per_sector);
if (record_size == 0 || !power_of_two(record_size))
    return ERR_BAD_PARAMETER;
if (record_size < 0x400)
    record_size = 0x400; /* corpus clamp */
if (record_size % bs.bytes_per_sector != 0)
    return ERR_BAD_PARAMETER;
```

`NtfsVolume::open()` propagates the error; the orchestrator moves on to other FS probes and
eventually raw carving. The NTFS remnant scan (`scan_mft_remnants`) — the path that exists
precisely for damaged volumes — is unreachable for that volume because it requires a
successfully opened `NtfsVolume`.

## Gap

Falsifiable claim: there exists a DBR with a non-power-of-two (or zero, or
non-sector-divisible) clusters-per-record encoding for which the og engine performs an NTFS
scan (with clamped record size, including its remnant pass) and our core performs none.

## Impact

- **Recovery availability** on the primary target scenario of the product (damaged disks,
  quick-format remnants): NTFS-structured and NTFS-remnant results are both lost; only raw
  carving remains.
- The core's own remnant scan implements the og's shifted-header salvage
  (`+0x58`/`+0x60` probes, `cr_mftwalk.cpp:1206–1212`) — that parity investment is bypassed
  whenever the DBR record-size field is torn.

## Reproduction / verification

- Corpus-derived: decompiled quote above (file + line range); artifact cross-cite.
- Core-derived: code quote with file:line.
- Executed: synthetic DBR fixture with `clusters_per_record = 3` (non-pow2) — predicted og
  behavior per decompile (clamp + scan), predicted core behavior `ERR_BAD_PARAMETER`. Not run
  against og in this environment (no Windows); the code paths are fully quoted.

## Fix plan

1. In `validate_boot_sector`, replace the rejection with the og's degradation ladder:
   - decoded 0 or non-pow2 or non-divisible → fall back to `record_size = 0x200`, then clamp
     `max(0x400, …)` as the og does (`buf_size = max(0x400, record_size)`);
   - keep the existing `record_size < 0x400 → 0x400` clamp (already og-parity).
2. Preserve the strict path behind a flag for callers that need spec-strict validation
   (e.g. the table/lost-partition classifier) — the og's *classifier* (RawFixer
   `clusters_per_record` handling, rawfixer-v2 §3) is separately documented as
   signed-at-+0x40 with `<0 → 1<<(-cpr)`; do not conflate the two decoders.
3. Files: `src/core/ntfs/cr_bootstrap.cpp`, `include/cr/ntfs/cr_ntfs_wire.h`
   (`record_size_from_dbr` gains a `fallback` out-param or a sibling
   `record_size_degraded()`).
4. Tests (below) — fail-before/pass-after.

## Tests to add

- `test_ntfs_volume.cpp::Boot_NonPow2_RecordSize_Clamps_And_Scans`: DBR with cpr=3 →
  `open()` returns OK with `record_size == 0x400`, walk runs (finds nothing on a clean volume,
  finds the synthetic file when records are 1 KiB and cpr was torn to 3 with a 1 KiB read
  granularity — assert no crash and sane walk stats).
- `test_ntfs_volume.cpp::Boot_Zero_RecordSize_Fallback_512`: cpr=0 → record_size 0x200 →
  clamped 0x400 → open OK.
- `test_ntfs_volume.cpp::Remnant_Reachable_Under_Torn_DBR`: torn DBR fixture + a shifted
  'FILE' remnant at +0x58 → remnant node emitted (fails before fix because open() refused).

## Related findings

- M-002 (lost-partition probe set) — the same class of refuse-vs-degrade posture on non-NTFS
  sides.
- E-2 evidence note (agent E): the salvage-over-purity posture is the repo's own documented
  principle (AUDIT-FALLBACKS.md census).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: M-001`)
**Verified by:** fail-before/pass-after — with the fix stashed, all five new
tests fail (`NtfsVolume.BootNonPow2RecordSizeClampsAndScans`,
`BootZeroRecordSizeFallbackScans`, `BootNegativeExponentOverflowFallsBack`,
`BootUsableLargeRecordSizeKept`, `RemnantReachableUnderTornDBR` — open()
returns ERR_BAD_PARAMETER pre-fix); restored, all pass. Suite green.
**Resolution notes:** implemented the degradation ladder per the fix plan:
unusable encodings (0, non-pow2, non-sector-divisible, negative-exponent
overflow) fall back to the og's 0x200 default; usable values pass through;
the existing 0x400 minimum-capacity clamp applies to both paths. No
strict-mode flag was added: the finding's suggested flag had exactly two
callers (NtfsVolume::open and lost::probe_ntfs) and BOTH benefit from the
ladder — the classifier's $MFT probe now scores torn-DBR candidates
instead of silently forfeiting 55 evidence points, which is also the og's
lost-partition posture (its classifier does not gate on record size; the
RawFixer CheckMFT decoder is a separate signed-cpr surface, untouched).
The Boot tests confirm the ladder happens to RECOVER the true geometry on
torn fields (cpr=3 on this 1024-byte-record fixture degrades to
0x200→0x400 = the real record size, so the full walk still yields its
normal 9+ nodes — a raw 1536-byte stride, the og's behavior, would read
nothing); this is a recovery-availability superset over the og on that
edge, registered in the same spirit as the I-001 superset. The
RemnantReachable test proves the end-to-end availability claim (torn DBR →
volume opens → remnant sweep runs → shifted-header remnant surfaces).
