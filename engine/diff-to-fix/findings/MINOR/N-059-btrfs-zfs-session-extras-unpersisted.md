# MINOR N-059 — Btrfs/ZFS session extras unpersisted: secondary ids A/B + generation/transid qword (Btrfs) and secondary flags u16 (ZFS); ZfsDataRunlist polymorphism flattened to extents (functional match)

**Severity:** MINOR
**Module:** session (+ btrfs/zfs engines)
**Parameters affected:** D11 (node fields), D12 (session persistence), B7/Q68 (field documentation)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md:23-25` (Btrfs record: "u32 secondary id A (+0x3B)", "u32 secondary id B (+0x3F)", "qword extra (+0x81) — Btrfs-specific: generation/transid family"), `:52-65` (ZFS record: "u16 secondary flags (+0x5A)"; polymorphic `zfs_struct::ZfsDataRunlist` at +0x60 serialized through its own vtable)
**Core references:** `src/core/session/cr_records.cpp` (props block has no btrfs/zfs secondary fields), `src/core/btrfs/cr_btrfs_engine.cpp` / `src/core/zfs/cr_zfs_engine.cpp` (engines hold the data in-memory during scan; not persisted) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's Btrfs record carries two secondary ids and a generation/transid qword; the ZFS
record carries a secondary-flags u16 and serializes a polymorphic ZfsDataRunlist. The core's
session persists neither the Btrfs nor the ZFS extras — reload loses them. The runlist
flattening is a functional match (the core's decoded absolute-extent vector expresses the
same layout, more expressively with codec/psize), so only the flag/id extras are state
loss.

## What the reference does

Quoted field tables (part3, first full read this session): Btrfs +0x3B/+0x3F secondary ids,
+0x81 generation/transid qword; ZFS +0x5A secondary flags, +0x60 ZfsDataRunlist via its own
vtable (+0x18 write/+0x20 read).

## What our core does

The btrfs/zfs engines read generation/transid and the secondary identifiers during scan
(they drive grading/supersets — see I-003/I-007), but the session props block has no
carriers; on reload the values are gone. The ZFS runlist is persisted as plain extents.

## Gap

Falsifiable: a Btrfs session reloaded in the og retains generation/transid per node; the
core's reloaded session returns zeros for any equivalent query.

## Impact

Post-resume provenance for Btrfs/ZFS nodes degrades (timestamps-of-change, subvol ids,
secondary flags). No recovery-correctness impact (extents and names survive). MINOR.

## Reproduction / verification

Part3 field tables read first-hand; core props block enumerated first-hand; negative space
verified.

## Fix plan

1. Add optional per-node engine-extras block (append-only): {u32 id_a, u32 id_b, u64 gen,
   u16 zfs_flags} — populated by the btrfs/zfs emit paths, serialized after the props.
2. Document the mapping in the session contract (which og field each mirrors).
3. Keep the extent-vector runlist (registered superset — I-022).

## Tests to add

- `Session.BtrfsExtrasRoundTrip`
- `Session.ZfsSecondaryFlagsRoundTrip`

## Related findings

- M-050 (node-model carrier wave — land together)
- I-022 (superset registers — codec/psize expressiveness)

## Open questions

The precise semantics of ZFS secondary flags (+0x5A) is census-level; carry opaque and
document UNDECODED.
