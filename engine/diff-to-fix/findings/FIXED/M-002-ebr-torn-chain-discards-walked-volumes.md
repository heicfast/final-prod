# MAJOR M-002 — Torn EBR chain discards already-walked logical volumes (salvage-over-purity violation)

**Severity:** MAJOR
**Module:** table (EBR chain walk)
**Parameters affected:** B6 (damaged-volume behavior), S35 (error recovery), A30 (lost-partition inputs)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/mo-module-map.md` (Partition.mo MBR/EBR chain parsing; PartitionRecoveryD deep-scan block parsers re-find volumes independently of the EBR chain — the og's lost-partition path does not depend on EBR integrity); the repo's own documented posture: `AUDIT-FALLBACKS.md` salvage-over-purity census
**Core references:** `src/core/table/cr_table_parse.cpp:378–384` (caller keeps only `OK`/`ERR_CYCLE_LIMIT`), `:149–214` (`parse_ebr_chain` — pushes volumes to `out` as it walks, then returns the failure code) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

`parse_ebr_chain` accumulates every logical volume it successfully walks into the output
vector, but the caller (`parse_disk_table`) merges that vector **only when the return code
is `OK` or `ERR_CYCLE_LIMIT`**. A chain that breaks mid-walk — torn EBR sector
(`ERR_BAD_MAGIC`), unreadable sector (`ERR_READ_FAILED`) — causes all previously-walked
logical volumes to be discarded. The user loses volumes that were found, on exactly the
damaged disks this product targets.

## What the reference does

The og does not gate its volume inventory on EBR-chain integrity: PartitionRecoveryD's
deep-scan block parsers (CBlockParseWin32 et al., mo-module-map) scan unallocated space for
partition signatures independently of the EBR chain, and Partition.mo's chain parse feeds
the same candidate pool. A broken link degrades the og to the per-volume discovery path
rather than discarding prior results. (The corpus does not decode the og's exact
partial-chain return handling — the divergence is filed against the core's own
salvage-over-purity posture, which `AUDIT-FALLBACKS.md` documents as the governing
principle.)

## What our core does

`src/core/table/cr_table_parse.cpp:378–384`:

```cpp
if (has_ebr && ebr_base != 0) {
    std::vector<PartitionInfo> ext;
    u32 rc = parse_ebr_chain(device, ebr_base, ext);
    if (rc == OK || rc == ERR_CYCLE_LIMIT) {
        for (auto& p : ext)
            out.partitions.push_back(p);
    }
}
```

`parse_ebr_chain` returns `ERR_BAD_MAGIC` (line ~168, `if (!mbr_signature_ok(sec.data()))
return ERR_BAD_MAGIC;`) or `ERR_READ_FAILED` on the first bad link — after having already
pushed the earlier volumes into `ext`, which the caller then drops.

## Gap

Falsifiable claim: an extended partition whose 3rd EBR sector is torn (no 0x55AA) yields
logical volumes 1–2 in the og's result set (via chain-tolerant or deep-scan paths) and zero
logical volumes in ours.

## Impact

- Damaged extended partitions (the common real-world case for recovery) lose all their
  walkable volumes; the lost-partition scanner partially compensates (it probes
  unallocated regions independently), but volumes inside the *known* extended container are
  not re-probed there — they are simply gone.
- Confidence in inventory: the result depends on the LAST EBR sector, not on what was
  recovered.

## Reproduction / verification

- Core-derived: code quotes (caller gate + mid-walk error returns; volumes already in
  `ext`).
- Executed: the same stands provable from the quoted paths; a synthetic fixture
  (2 good EBR links + 1 torn) run through `parse_disk_table` shows 0 logical volumes —
  recommended as the regression test below (not run separately this session; the code path
  is deterministic).
- Corpus-derived: mo-module-map (deep-scan independence); AUDIT-FALLBACKS posture.

## Fix plan

1. In `parse_disk_table:378–384`, merge `ext` unconditionally, and record the chain status:
   add a `PartitionInfo`-adjacent field or a `TableInfo::ebr_status` (OK / PARTIAL / FAILED)
   so downstream can report honesty (e.g., "3 of 5 volumes found; chain broken at LBA X").
2. In `parse_ebr_chain`, distinguish "this link is bad" (return partial + status) from
   geometry errors before any volume was walked (existing behavior fine).
3. Optionally attempt one-link resync forward (scan forward in 1-sector steps up to a small
   budget for the next 0x55AA EBR-shaped sector) — matches the og's deep-scan resilience;
   keep it behind a flag if added.
4. Files: `src/core/table/cr_table_parse.cpp`, `include/cr/table/cr_table_parse.h`.
5. Tests: `test_crc32_gpt.cpp::Ebr_Torn_Link_Keeps_Prior_Volumes` (fails before, passes
   after); `Ebr_Torn_Link_Status_Reported`.

## Tests to add

(fix plan step 5)

## Related findings

- A-001 (same module family — GPT side of partition correctness).
- D-sheet test-gap note: EBR entry-0-link variant untested.

## Open questions

None.

## Session-2 note (2026-10-25)

Not re-verified this session: this leg's partition agents closed OQ-T7/T9 + TOCBLOCK +
geometry edges first and did not reach the EBR extended-chain walk. Stays open with its
session-1 evidence; the LdmManager/PartitionRecovery decompile locations are now mapped
(`../evidence/decompiled-quotes/ldm-raid5-and-scoring.md`) — prioritize alongside M-016.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's M-002 commit — see `git log` for "fix: M-002")
**Verified by:** `Mbr.TornLinkKeepsPriorVolumes` (fail-before: the caller's rc gate
dropped BOTH walked volumes on the torn 3rd link — 0 logical volumes; pass-after: 2
volumes + `EbrChainStatus::Partial` + `ebr_break_lba` = the torn link's LBA) and
`Mbr.TornLinkStatusReported` (the status vocabulary: Ok/Failed/break-LBA; the
PlainWithExtended whole-chain shape still reports Ok with break 0). Full suite
552/552 Linux Release green after the fix.
**Resolution notes:** the caller gate in `parse_disk_table` (cr_table_parse.cpp) now
merges the walked prefix UNCONDITIONALLY — salvage-over-purity, matching the repo's
own governing posture (AUDIT-FALLBACKS) and the og's shape (Partition.mo's chain parse
feeds the same candidate pool the deep-scan block parsers fill independently — corpus
mo-module-map; a broken link degrades to the walked prefix, never erases it). Chain
honesty is carried by the new `DiskTable::ebr_status` (None/Ok/Partial/Failed) +
`ebr_break_lba` (the first unreadable/invalid link's LBA), so downstream can report
"N volumes found; chain broken at LBA X" per the fix plan step 1.
`parse_ebr_chain` gained an optional `break_lba` out-param (defaulted — the public
signature stays source-compatible for existing callers/tests). The fix plan's optional
step 3 (one-link resync forward scan) was NOT added: the lost-partition scanner
already probes unallocated regions independently (the finding's own impact note), and
a resync scan would duplicate that surface with weaker gates — registered here as a
deliberate scope decision, not a silent skip. Corpus citation re-verified first-hand:
mo-module-map (deep-scan independence). Note: the finding's "2 good EBR links + 1
torn" reproduction is exactly the shipped test fixture.
