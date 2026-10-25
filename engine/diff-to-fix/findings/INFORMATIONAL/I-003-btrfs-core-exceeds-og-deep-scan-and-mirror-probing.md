# INFORMATIONAL I-003 — Btrfs: core exceeds the og (working deep scan; unconditional mirror probing)

**Severity:** INFORMATIONAL (deliberate supersets — recovery improvements, recorded for parity-test truthfulness)
**Module:** btrfs
**Parameters affected:** B1 (detection — mirror rule), A25 (algorithm surface), Q68 (docs should register both)
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/BtrfsSearchDLL_decompiled.c` — `EUCreateBtrfsComplateSearcher` **is a NULL stub** (:11–19, agent-B verified: `*param_1 = 0; return;`); mirror-scan abort rule `if (lVar5 == 0) break` when the primary candidate's magic fails (:10320–10324, agent-B verified)
**Core references:** `src/core/btrfs/cr_btrfs_engine.cpp:352–369` (mirror-select loop — probes **all** offsets unconditionally, `continue` on failed read/magic, newest-generation wins), full deep engine at `src/core/btrfs/` (799 LOC, 7 test areas) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two places where our Btrfs engine is a deliberate superset of the og:

1. **The og ships no working complete/advanced Btrfs scan** — the exported
   `EUCreateBtrfsComplateSearcher` (with the vendor's own "Complate" typo) writes NULL and
   returns. Only the quick path exists in the reference build. Our core implements a full
   chunk-map → root-tree → per-subvolume walk.
2. **The og aborts the mirror scan if the primary superblock's magic fails** (first
   candidate fails → `break`, mirrors never probed). Our core probes every mirror offset
   and selects the newest generation — so we detect Btrfs on volumes where the primary SB
   is destroyed but a mirror survives; the og detects nothing.

## What the reference does

Decompile quotes (B-btrfs.md §1/§2, verified):

```c
void EUCreateBtrfsComplateSearcher(undefined8 *param_1)
{
  *param_1 = 0;
  return;
}
```

and in the SB scan: a failed magic check on the first (primary) candidate breaks the whole
loop — mirror offsets {0x4000000, 0x4000000000, 0x4000000000000} are unreachable when the
primary is gone.

## What our core does

`cr_btrfs_engine.cpp:352–369`: the loop reads every `kMirrorOffsets[]` entry (the offsets
themselves are ground-truth-extracted from the og's .rdata at VA 0x180018D60 and match the
kernel's `btrfs_sb_offset()`), `continue`s past failures, and keeps the
newest-generation valid SB. The deep engine implements subvolume trees, inline/compressed
extents, and (documented) no-deleted-recovery.

## Gap

Falsifiable claims: (a) a volume with primary SB destroyed + valid mirror yields a full
Btrfs result set in our core and "not Btrfs" in the og; (b) any structured Btrfs deep-scan
result in our core has no og counterpart to compare against (og complete scan = NULL).

## Impact

- Strictly better recovery availability for (a).
- For (b): parity benches and result-set comparisons must NOT assume og deep-Btrfs output;
  the og's Btrfs yield on damaged volumes comes from RAWSearch carving only.
- The core's no-deleted-recovery posture on Btrfs (CoW rationale, documented at
  `cr_btrfs_engine.cpp:347–348`) is audit-consistent with the og's *nonexistent* deep scan —
  there is no og deleted-Btrfs behavior to diverge from.

## Reproduction / verification

- Corpus-derived: NULL-stub decompile (:11–19) and abort-rule decompile (:10320–10324),
  both agent-B verified.
- Core-derived: mirror-loop code quote; engine LOC/test inventory (agent D).
- Executed: not run; both deltas are fully determined by the quoted code.

## Fix plan

No code change. Actions:

1. Register both supersets in `docs/parity/PARITY-DELTAS.md` (if not already rows — the
   mirror-probe rule at minimum) so future parity sessions treat them as accepted.
2. In bench comparisons, tag Btrfs structured results as core-only (no og leg).
3. Optional: add a comment at the mirror loop citing the og's abort rule (currently the
   comment documents the offsets' provenance but not the abort-rule deviation).

## Tests to add

- `test_btrfs_volume.cpp::Primary_Damaged_Mirror_Detected` (pins the superset as intended
  behavior — already true; guards against anyone "fixing" parity by adding the abort).

## Related findings

- None.

## Open questions

The og's `BtrfsCache.txt` re-scan cache (persisted next to the DLL) has no core
counterpart — perf-only, re-scan speed; not registered as a defect (an engine that writes
files next to its own DLL during a scan is an og behavior worth NOT replicating; recorded
here for completeness).


## Resolution

**Status:** ACKNOWLEDGED (deliberate supersets — preserved)
**Verified by:** the NULL-stub decompile (:11-19) and the mirror-abort
decompile (:10320-10324) re-read from the finding's evidence; both hold. The
requested pin ALREADY EXISTS: `Btrfs.MirrorSelectionNewestGeneration`
(primary SB smashed -> 64 MiB mirror recovers the volume) — no new test
needed, the guard the finding asked for is in place. Registered this
session: PARITY-DELTAS D-48a (first row for the mirror-probing + deep-scan
superset; benches must not assume og deep-Btrfs output). Future work must
PRESERVE both supersets.
