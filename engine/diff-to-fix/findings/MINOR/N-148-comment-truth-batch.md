# MINOR N-148 — the comment-truth batch: cr_ldm.h still claims the RAID-5 rotation is "UNDECODED" after OQ-T7 resolved it as a hardwired MATCH; cr_zfs_engine.h's vdev comment contradicts D-09's corrected wording; cr_ui.cpp:3754's og cite is 9 lines loose; the EPS/AI stand-in caps drifted to 16 MiB against the registry's 64 MiB; six in-code Resolution pointers dangle into purged findings; ReFS og_prop_flags decoded-but-unconsumed

**Severity:** MINOR (comment/doc drift in shipped headers and the audit trail's in-code citations — the N-127 class: no behavioral defect, but each stale claim misleads a future parity session or a C consumer)
**Module:** ldm / zfs / preview-abi / carve (the comment-truth sweep)
**Parameters affected:** D46 (documentation accuracy — in-code contract comments), P69-adjacent (the stand-in register's shape)
**Corpus references:** (per item) the OQ-T7 resolution: `diff-to-fix/evidence/decompiled-quotes/ldm-raid5-and-scoring.md:8-34` ("RESOLVED: left-symmetric, hardwired … Core: cr_ldm.cpp:1346-59 … IDENTICAL"); D-09's closing wording demand: `docs/PARITY-DELTAS.md:275-279`; the og's TxtViewFiles/promotion cites for item 3: `ghidra-drwui/DRWUI_decompiled.c:422507-514` (the guard's true location — M-112's decode)
**Core references:** audited `461825d8`-lineage — (1) `include/cr/table/cr_ldm.h:30-32` ("the og's RAID-5 parity rotation is UNDECODED — our raid5_ls layout is the libldm verdict, not an og-parity claim"); (2) `include/cr/zfs/cr_zfs_engine.h:58` (`unsupported_vdevs /* gang/indirect/mirror/raid multi-vdev */`) vs `src/core/zfs/cr_zfs_engine.cpp:1369` ("mirror/raidz: report only") and the gang assembly at `:594-621`; (3) `src/ui/cr_ui.cpp:3754` (cites ":422498-518"; the guard is at :422507-514); (4) `src/core/carve/cr_analysers.cpp:4091` and `:4112` (the EPS/AI validators' `16 << 20`) vs `src/core/carve/cr_registry.cpp:916-926` (the rows' `64ull << 20`) and M-107's "all eleven at 64 MiB"; (5) the dangling Resolution pointers: `cr_analysers.cpp:749`, `src/core/lost/cr_candidates.cpp:248`, `src/core/tree/cr_grading.cpp:9-10`, `include/cr/health/cr_grade.h:139`, `src/core/ntfs/cr_lfn.cpp:119-120`, `src/core/exfat/cr_exfat_quick.cpp:483` — each cites a purged findings/FIXED/ path; (6) `src/core/refs/cr_refs_engine.cpp:1222-1225` (`og_prop_flags` decoded-then-tracked, "(not yet) consumed by the core's grading model")
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** the Lane E stand-in sweep's Section 4/6 items C-3..C-7 + the Lane C cite nit; the N-127 precedent (driver doc-vs-code truthfulness) extended to the engine headers; no prior finding owns any item (N-129's class is ledger-truth, not in-code comments; N-127's scope is the driver docs).

## Summary

Seven stale or dangling truth claims in shipped code and headers: a header that denies a resolved MATCH (LDM RAID-5), a header comment that contradicts the registered ZFS posture (D-09's own demanded wording never landed), a 9-lines-loose og cite in the promotion seam M-112 builds on, the EPS/AI stand-in caps drifting 4× below their registry rows, six in-code citations pointing into purged finding files, and a decoded-but-unconsumed ReFS datum with an unresolved consumer question.

## What the reference does

n/a per item — the references are core's own resolved audit trail: OQ-T7's
resolution record, D-09's registered posture, M-112's decode of the guard's true
location, M-107's filed description, the purge convention's history-not-tree
reality, and the ReFS property-flags decode.

## What our core does

The seven items above — each a claim in shipped code that no longer matches the
trail's own truth.

## Gap

Falsifiable per item: (1) `cr_ldm.h:30-32` says UNDECODED; the evidence file says
RESOLVED-IDENTICAL; (2) `cr_zfs_engine.h:58`'s comment names gang/indirect in the
unsupported set; the code assembles gang blocks and only reports mirror/raidz;
(3) the cite at `cr_ui.cpp:3754` names :422498-518; the guard is :422507-514;
(4) the EPS/AI validators cap at 16 MiB; the registry rows and M-107 say 64 MiB;
(5) six citations name `findings/FIXED/<slug>.md` paths that no longer exist in
the tree; (6) `og_prop_flags` is populated by the decode and read by nothing.

## Impact

Future-session truth cost, not behavior: a parity session reading cr_ldm.h would
re-litigate a resolved question; a ZFS reader gets the pre-D-09-corrected
wording; the M-112 fix lane starts from a loose cite; the EPS/AI drift means
M-107's acceptance criteria ("grep clean or owned") are met in the registry but
not the validators; the dangling pointers force git-history digs for every
resolution record.

## Reproduction / verification

Code-read + trail-read (every item's both sides cited above; the Lane E sweep's
greps re-verified by the lead at filing). Full record:
`evidence/s11-lead-deep-lanes.md` §Lane-E items C-3..C-7 + §Lane-C's citation
nit.

## Fix plan (for the engineering team)

1. `cr_ldm.h:30-32`: replace the UNDECODED hedge with the resolved truth (cite
   the OQ-T7 evidence file's conclusion; keep the libldm-provenance note).
2. `cr_zfs_engine.h:58`: adopt D-09's demanded wording — "mirror/raidz:
   report-only; gang blocks are assembled (cr_zfs_engine.cpp:594-621);
   indirect vdevs unsupported".
3. `cr_ui.cpp:3754`: ":422498-518" → ":422507-514" (rides M-112's fix or
   standalone).
4. The EPS/AI caps: decide — either restore 64 MiB in the validators to match
   the rows (recommended: the rows are the M-107-verified shape) or re-pin the
   rows at 16 MiB with the reason; do not leave them divergent.
5. The dangling pointers: rewrite the six in-code citations to name the finding
   ID + the ledger ("see the FIXED-LEDGER's <ID> row; the Resolution is in
   history at the row's commit") — a mechanical sweep, one commit.
6. `og_prop_flags`: either wire it into the grading model (if the og's grading
   consumers read it — decode first) or add the one-line "passive datum" note
   to the registered grading non-adoption row (`modules/orchestration.md:122-123`).
7. Acceptance: `grep -rn "UNDECODED" include/cr/table/cr_ldm.h` → 0; the six
   dangling paths → 0; the EPS/AI validators and rows agree; each item's test
   or grep is clean.

## Tests to add

- `Carve.EpsAiCapsMatchRegistryRows` (the item-4 agreement pin).
- (Items 1-3, 5, 6 are comment/doc truths — the doc-truth gate pattern where a
  pin test exists for the surface, else grep-clean acceptance.)

## Related findings

N-127 (the comment-truth class precedent); OQ-T7 (resolved — item 1's
resolution); D-09 (item 2's registered posture); M-112 (item 3's owner);
M-107 (item 4's owner); N-129 (item 5's purge-convention root cause); S-053
(the same registry file's cap-truth family).

## Open questions

Item 6's consumer question: whether the og's grading model reads the ReFS
property flags (a decode task; if unresolved, register as passive-datum).
