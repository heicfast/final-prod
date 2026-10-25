# MINOR N-001 — $MFTMirr fallback hardcodes records 0..3 instead of mirror-runlist size

**Severity:** MINOR
**Module:** ntfs
**Parameters affected:** B1 (detection correctness — salvage coverage), C20 (on-disk struct offsets and sizes)
**Corpus references:** `re-workspace/artifacts/SearchDLL/zfs-vdev--mftmirr--calcpartition.md` §2 (NTFSLib `InitMFTMirror`, decompiled `re-workspace/reversed/ghidra-mo/NTFSLib_decompiled.c:17310–17430`, agent-B verified)
**Core references:** `src/core/ntfs/cr_bootstrap.cpp:250–305` (`load_mftmirr_stream`, `enum_mirr_attrs`), `:377–386` (`read_record` mirror fallback `record_no < 4`) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og derives the mirror's record coverage from the $MFTMirr $DATA runlist itself
(`this+0x70 = (last_run.offset + last_run.length) * factor / sectors`) and can therefore serve
mirror reads for however many records the mirror actually spans. Our core decodes the same
runlist into `mirr_run_` but then gates the fallback on a hardcoded `record_no < 4`, ignoring
the decoded size.

## What the reference does

`InitMFTMirror` (NTFSLib, ghidra-mo decompile :17310–17430): resolves the $MFTMirr record,
requires a **non-resident unnamed $DATA** (resident → hard abort with the logged string
"…$DATA is resident!"), decodes its runlist to `this+0x60`, and computes the mirror's size in
sectors at `this+0x70` from the last run's `offset + length`. Any record read within that span
can be served from the mirror when the primary copy is unreadable.

## What our core does

`cr_bootstrap.cpp:377–386`:

```cpp
if (!found || sparse) {
    if (record_no < 4 && mirr_valid_) {
        u64 mlcn = 0; bool msparse = false;
        if (mirr_run_.vcn_to_lcn(vcn, &mlcn, &msparse) && !msparse)
            return dev_->read_at(mlcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
    }
    return ERR_RANGE_OUT_OF_BOUNDS;
}
```

The runlist is decoded (`enum_mirr_attrs` sets `mirr_run_` + `mirr_valid_`) but its extent is
never used to bound the fallback — the constant 4 is hardcoded.

## Gap

Falsifiable claim: on a volume whose $MFTMirr spans more than 4 records (legacy NT 3.x/4.0
layouts with small record sizes; some third-party formatters), a primary-copy read failure for
record ≥ 4 is servable from the mirror in the og and returns `ERR_RANGE_OUT_OF_BOUNDS` in our
core.

## Impact

Bounded: modern Windows formats $MFTMirr with exactly the first 4 records, so the practical
exposure is legacy/exotic volumes — exactly the class a recovery product sees occasionally.
Cost when hit: damaged records 4+ that the og salvages are counted `damaged`/skipped by us.

## Reproduction / verification

- Corpus-derived: decompile citation above (agent-B verified lines).
- Core-derived: code quote with file:line.
- Executed: not run (fixture cost outweighs severity); the two code paths fully determine the
  behavior difference.

## Fix plan

1. In `load_mftmirr_stream`/`enum_mirr_attrs`, compute `mirr_record_capacity =
   mirr_run_.total_clusters() * p_.cluster_bytes / p_.record_size` after a successful decode
   (this is the og's `this+0x70` computation in record units).
2. Replace the `record_no < 4` gate in `read_record` with `record_no < mirr_record_capacity`
   (keep a floor of 0 and cap at, say, 1024 to defensible-guard absurd runlists).
3. Files: `src/core/ntfs/cr_bootstrap.cpp` (both functions), header field in
   `include/cr/ntfs/cr_bootstrap.h` (or the existing volume state struct).
4. Tests: extend the mirror test in `test_ntfs_volume.cpp` with a synthetic 8-record mirror
   runlist; assert record 5 is served from the mirror when the primary copy is unreadable
   (fails before, passes after).

## Tests to add

- `test_ntfs_volume.cpp::Mirror_Covers_Runlist_Size` (as above).
- `test_ntfs_volume.cpp::Mirror_Resident_Data_Rejected` — og-parity of the abort path
  (resident $DATA on the mirror record → mirror invalid, volume still opens; current code
  simply leaves `mirr_valid_` false — assert the behavior is unchanged by the fix).

## Related findings

None.

## Open questions

None.
