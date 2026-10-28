# MINOR N-162 — FAT cluster→sector 32-bit multiply wrap in the og (4 verified sites): the core is u64-correct — an unregistered member of the D-56/D-68 numeric-hygiene class

**Severity:** MINOR
**Module:** fat (cluster→LBA math) / registry discipline
**Parameters affected:** calculation mathematics (session directive), scalability (52), constants/widths hygiene
**Corpus references:** `FATSearchDLL_decompiled.c:11239`, `:13194` (verified first-hand: `(ulonglong)(((uint)uVar7 - 2) * (int)param_1[4]) + param_1[8]` — 32-bit truncating product), `:28172`, `:39068-39080` (`FUN_180030d90`, the CalcPartition geometry helper); 64-bit sites also exist (`:28045` runlist factory) — the width inconsistency is og-internal
**Core references:** `src/core/fat/cr_chain.cpp:24-28` (`FatTable::cluster_offset` — full u64), contract `include/cr/fat/cr_fat_engine.h:9`; audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og computes `(cluster−2) × spc` as a 32-bit product at four verified read/geometry
sites, wrapping on data areas beyond 2^32 sectors (2 TiB at 512 B/sector — legal for
FAT32 with large clusters: spc=32 wraps from cluster 134,217,730; at the spec-max
cluster 0x0FFFFFF4 with spc=32 the og addresses 0xFFFFFE40 instead of 8,589,934,144 — a
2 TiB displacement). The core is full u64 and correct. This is a core SUPASET that is
registered nowhere — the numeric-hygiene family (D-56 NTFS pass-2, the D-68 MRW
precedent) lacks its FAT member.

## What the reference does

The four 32-bit sites (cites above; :13194 carries the explicit `(uint)` truncation of
the cluster operand — lead auditor verified). PARITY-CORE-FS1.md:194 recorded the
cluster-offset formula as MATCH without flagging the width; the s3 evidence quote
(s3-calcpartition-scoring.md:49-55) shows the 32-bit product unremarked.

## What our core does

u64 product everywhere (cr_chain.cpp:24-28).

## Gap

Registry gap: the deliberate non-adoption of an og arithmetic defect is unregistered —
per the registry's own rule, "a delta that is not in this registry is a bug, not a
delta".

## Impact

None behavioral (core correct). Risk is drift: a future parity session re-derives
"matching" FAT math from the decompile and "fixes" the core DOWN to the wrapping form.

## Reproduction / verification

Lead auditor verified :13194's cast structure directly (quote in the lane sheet,
evidence/s12-lane-sheets/lane-A-sector-math.md row 2.1); arithmetic checked with python
(the 2 TiB displacement example). 14 further `(c−2)×spc` sites located but widths
unverified (OQ-5 — blast-radius only).

## Fix plan

1. Register: one PARITY-DELTAS row (FAT cluster-math width; core u64 deliberate;
   the four og sites cited) — doc edit for the fix lane.
2. No code change.
3. Test (optional pin): `Fat.ClusterOffsetBeyond2TiB` — a synthetic >2 TiB geometry
   addressing test proving the core's u64 path (protects against the down-fix).
4. Acceptance: the registry row exists and cites the four sites.

## Tests to add

- `Fat.ClusterOffsetBeyond2TiB` (the anti-down-fix pin)

## Related findings

- I-040 (the og-defect non-adoption batch — this can ride that registration instead)
- M-127 (the same walkers' complexity class — independent)

## Open questions

The widths of the remaining 14 sites (OQ-5) — quantification only.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-162 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `FatEngine.ClusterOffsetBeyond2TiB` (the spec-max cluster 0x0FFFFFF4 with spc=32: the exact u64 address 4,398,046,332,928 — beyond the 32-bit wrap band — and explicitly NOT equal to the og's wrapping form 0xFFFFFE40+base; the down-fix guard). Suite 1264/1264 (linux-release).
**Resolution notes:** Corpus re-verified first-hand: :13194's cast structure re-read directly (`(ulonglong)(((uint)uVar7 - 2) * (int)param_1[4]) + param_1[8]` — the explicit (uint) truncation with the (int) spc operand) and :11239's promotion form (`(uVar7 - 2) * (int)param_1[4]` — 32-bit product without the cast); the core's cr_chain.cpp u64 product re-read live. The fix followed the finding's plan: PARITY-DELTAS D-119 registered (the D-56/D-68 numeric-hygiene family's FAT member) with the four og sites, the wrap arithmetic, the 64-bit-site inconsistency note, and the silent-corruption rationale; NO code change (the core is correct); the optional pin test landed as the down-fix guard. Fixture note: FatParams::cluster_bytes is the field the math reads (not derived from sectors_per_cluster) — the pin sets it explicitly.
