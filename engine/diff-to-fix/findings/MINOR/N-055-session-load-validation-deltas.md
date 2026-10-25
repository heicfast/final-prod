# MINOR N-055 — Session load validation deltas: forward parent_id silently re-parented; marker-not-in-{0,1} treated as light; fail&&complete returns OK; u16 name-length wrap — against the og's per-node marker/name/parent checks (with the core's CRC/resync/bomb-cap as a registered superset)

**Severity:** MINOR
**Module:** session
**Parameters affected:** A35 (error recovery on load), D12 (session format — validation semantics), F35 (failure propagation)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-record4--tree-node-grammar.md` (record-4 load validation: marker ∈ {0,1}, name-length bounds, parent-reference checks — the og rejects/repairs malformed nodes per-rule)
**Core references:** `src/core/session/cr_records.cpp` (`deserialize_tree`: forward `parent_id` silently re-parents to a synthetic root; `marker` outside {0,1} falls into the light-node branch; the final `c.fail && created == count` guard returns OK; name length read as u16 without a wrap/bounds re-check) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og validates each node record on load (marker, name length, parent reference) with
per-rule handling. The core's loader has four soft spots: a forward parent reference
silently re-parents the node under a synthetic root instead of failing or deferring; a
marker byte outside {0,1} is treated as a light node rather than corruption; a stream that
fails after emitting every expected node returns OK; and the u16 name length is trusted
against the remaining buffer without an explicit wrap check. In the other direction, the
core's record-level CRC32 + F10 resync + bomb-cap are protections the og lacks — registered
as a deliberate superset.

## What the reference does

Per rsf-record4 (first read this session): the loader checks the marker byte against the
node-kind set, validates the name length against the record bounds, and checks the parent
reference — malformed nodes take the artifact's per-rule paths (reject/repair), not silent
normalization.

## What our core does

The four deltas quoted in Core references. Each is a lenient path: forward parents
re-parent silently; bad markers degrade to light nodes; a tail failure after a full count
is OK; the name length trusts the u16.

## Gap

Falsifiable: a session with a node whose parent_id points forward (to a node not yet
loaded) attaches that node under the synthetic root in the core; the og's parent check
handles the reference per its rule set. A session with marker=7 loads as a light node in
the core.

## Impact

Corrupted/hostile sessions normalize silently instead of surfacing corruption — a
diagnosability gap, not a data-loss path (the CRC layer catches most corruption first,
which is why this is MINOR and not higher).

## Reproduction / verification

Loader read first-hand this session; rsf-record4 validation rules read first-hand. No
executed fuzz this session (the fix leg's harness items below).

## Fix plan

1. Defer forward parent references (two-pass load or an unresolved-parent map) instead of
   silent re-parenting; surface a `reparented` count in the load result.
2. Treat marker outside {0,1} as corruption (fail the record, let the CRC/resync layer
   recover).
3. On stream failure, require `created == count && !c.fail` (or surface short-count).
4. Re-check the u16 name length against the remaining record length before allocating.
5. Keep the CRC32/F10/bomb-cap superset; register it in PARITY-DELTAS (I-020 family).

## Tests to add

- `Session.ForwardParentDeferredNotReparented`
- `Session.BadMarkerRejected`
- `Session.ShortCountSurfaced`
- `Session.NameLengthWrapRejected`

## Related findings

- M-041/M-044 (same loader — land as one session-hardening wave)
- I-020 (superset registers)

## Open questions

The og's exact per-rule repair behavior for each malformed class is census-level in
rsf-record4 — the fix leg should mirror "reject the node, keep the session" unless a deeper
decode lands.
