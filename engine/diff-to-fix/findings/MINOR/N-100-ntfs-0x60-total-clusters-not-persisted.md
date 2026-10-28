# MINOR N-100 — NTFS session-wire: the og persists +0x60 (total $DATA clusters, sparse-zeroed) beside +0x64; the core carries +0x64 only (derives the rest)

**Severity:** MINOR
**Module:** ntfs / session
**Parameters affected:** D11 (node fields), D12 (session persistence — field equivalence), C20-adjacent (struct semantics)
**Corpus references:** `NTFSSearchDLL_decompiled.c` — the field mapper `FUN_180003020` :1987-88 stamps BOTH `+0x64 <- +0x53` and `+0x60 <- +0x4f`; the stamp chain's first hop is +0x53/+0x4f on the internal struct (:5126-27) not +0x64 directly; the GUID-dump twin `FUN_18001c660` :22470-71 stamps both; `FUN_180003250` :2264-65 (the landing's "FUN_180033250" is a typo — no such symbol); sparse-anywhere zeroes the +0x60 total (`FUN_1800336f0` :41096-98); the resident path builds a synthetic run {value,1} and still stamps (:5192-94); persistence: `artifacts/SaveScan/rsf-format--scan-session.md:398` — record-4's CNTFS extra writes `u32(+0x64); u32(+0x60)`; the getter stubs `FUN_180002da0`/`FUN_180002db0` :1821-34
**Core references:** audited `ff879f9` — `include/cr/tree/cr_node.h:216` (`ntfs_origin_lcn` only), stamps `cr_mftwalk.cpp:901` (main walk: sparse→0xFFFFFFFF) and `:1494` (GUID-dump), resident stays 0; session persistence nflags bit-8 (cr_records.cpp:172,269,461) — +0x60 NOT carried (the landing's "derivable from allocated_size+extents" rationale); landed `eac8e1d` closing OQ-SA-1
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og persists both NTFS anchor dwords on the session wire: +0x64
(origin LCN, now carried by the core) and +0x60 (total $DATA clusters,
with sparse-anywhere ⇒ 0 semantics). The core carries only +0x64 and
derives cluster counts from allocated_size — which does NOT reproduce
the og's sparse-zeroing rule. The landing's one-hop stamp description
and its "FUN_180033250" cite are also corrected here.

## What the reference does

Both fields stamped at every site (:1987-88, :22470-71, :2264-65) and
both persisted in RSF record-4 (:398). +0x60's semantics: total $DATA
clusters, zeroed when any run is sparse (:41096-98) — not always equal
to allocated clusters.

## What our core does

Only `ntfs_origin_lcn` (+0x64) is stamped, cloned, and persisted
(bit-8). +0x60 is absent everywhere; the landing's rationale assumed
derivability, which the sparse-zeroing rule contradicts (a sparse file
has og +0x60 == 0 but nonzero allocated clusters).

## Gap

Falsifiable: a sparse $DATA file's og session row carries total=0;
the core's session row has no equivalent field, and any derived
cluster count is nonzero.

## Impact

Informational loss on the session wire for sparse files only; no
recovery-correctness impact (the field is a location anchor, not
content). The audit's field-equivalence rule (D12: "fields must be
equivalent") is technically unmet for +0x60.

## Reproduction / verification

Decompiled evidence: `evidence/decompiled-quotes/s8-autosave-and-stamps.md` §5
(the full chain with corrections); the rsf-format doc cite (:398) read
this session. Core: stamp sites + records read directly.

## Fix plan (for the engineering team)

1. Either carry +0x60 as nflags bit-9 (4-byte block, fires when
   nonzero — the bit-7/bit-8 precedent) with the sparse-zeroing rule
   implemented at the stamp sites, or register the deliberate
   non-carriage in PARITY-DELTAS with the sparse-zero caveat (the
   honest current state).
2. Correct the OQ-SA-1 closure record's chain description (one-hop
   +0x53/+0x4f stamps; FUN_180003250 not FUN_180033250) and the
   resident-semantics note (og stamps the synthetic-run value, core
   stamps 0 — a documented deviation, now decode-confirmed).
3. Tests if implemented: sparse file round-trips total=0; non-sparse
   round-trips the cluster count.

## Tests to add

(If carried) NtfsTotalClustersSparseZeroRoundTrip;
NtfsTotalClustersNonSparseRoundTrip.

## Related findings

OQ-SA-1 (closed — the +0x64 half), S-044 (the same landing family's
auto-save divergence).

## Open questions

The provenance of the resident path's synthetic-run value (local_60)
— likely the MFT-record-relative offset; needs one more decode pass if
resident-file +0x64 parity is ever wanted.


---

## Session-8 lead-lane reconciliation addendum (2026-10-28, the push-window merge)

The resident synthetic pair's first element, decoded end-to-end by the lead lane, is the
**(u32-truncated) walk-context record anchor** — local_60, the 5th output of FUN_1800430b0
(:5066) -> FUN_180042020 (:52709, `*param_6 = param_1[2]`) <- the record ctor FUN_1800431d0
(:53315) — which in raw-scan mode is the record's raw disk offset, i.e. **NONZERO**
(this finding's corpus references already carry the {value,1} shape; the provenance chain
is the addition). Comment consequence: `include/cr/tree/cr_node.h:208-209` justifies
core's resident=0 with "the og's zeroed raw block" — inaccurate; the og's resident wrapper
stamps the record anchor, and core's 0 is the honest deliberate no-runs value. The comment
should be rewritten to state the og model truthfully (the registered divergence stands;
the prose was wrong).

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-100 commit — the session-38 minor-first lane)
**Verified by:** `Session.NtfsTotalClustersRoundTrip` + `NtfsVolume.OriginLcnCarriedFromFirstDataRun`'s new total-clusters half (both new) — the round-trip test proves the non-sparse total (7) persists beside the anchor, the sparse-anywhere shape (total 0 beside a NONZERO allocated_size — the exact not-derivable pair the finding's falsifiable describes) round-trips truthfully, and the zero-total gate keeps pre-N-100 records shape-identical (bit-9 fires only when nonzero); the walk test proves the stamp semantics on the fixture (readme 2 / deleted 4 / holes 0-with-nonzero-allocation / dirdel 0 / ads 1). Fail-before = compile-RED on the reverted tree (the field itself is the fix — the M-066 precedent for field-additions). Full suite 1077/1077 (linux-release, gcc Werror); clang-format 18.1.8 clean.
**Resolution notes:** The finding's fix-plan option 1 landed (carry +0x60, not the register-the-omission option): `FileProperties::ntfs_total_clusters` (cr_node.h) with the sparse-anywhere zeroing rule at BOTH stamp sites (the main walk cr_mftwalk.cpp:902-917 and the GUID-dump twin :1511-1525 — the og stamps both fields at every site, FUN_180003020 :1987-1988 / FUN_18001c660 :22470-22471); persisted as the nflags bit-9 block (cr_records.cpp — the bit-7/bit-8 4-byte precedent, fires only when nonzero, pre-fix sessions shape-identical; the og's rsf record-4 persists the pair in the same order, rsf-format :398); the hard-link clone carries it (cr_node.cpp — the og's clone copies the whole props block). The addendum's comment correction ALSO landed: cr_node.h's resident-semantics prose no longer claims "the og's zeroed raw block" — it now states the decoded truth (the og's resident wrapper stamps the synthetic {record-anchor, 1} pair; core's 0 is the honest deliberate no-runs value, a REGISTERED divergence), and the refuted "+0x60 type-registry id" census reading plus the "derivable from allocated_size+extents" rationale (which the sparse-zeroing rule contradicts) are gone from the field's contract. The OQ-SA-1 closure-record corrections (one-hop +0x53/+0x4f stamps; FUN_180003250 not FUN_180033250) were already carried by the scan-engine lane's records (SCAN-ENGINE-DELTA :1972-2005 re-read this session); the resident provenance chain from the addendum is now in the field comment.
