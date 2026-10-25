# OQ-RS (tracked-open) — ReFS open questions from the session-3 storage/census parallel leg

> **Reconciliation note (2026-10-26):** filed by the parallel S3 leg whose ReFS
> findings S-020/M-022/M-026/M-027/N-036/N-037 (S3 lead leg) superseded this
> leg's local A-004/S-012/S-013/M-011/M-013/I-009/N-023/N-024. OQ-RS-1 below
> (attribute dispatch) is owned by the lead leg's M-022 — kept here only as a
> pointer. The remaining probes are this leg's unique contributions.

**Status:** OPEN · **Module:** refs · **Filed:** session 3 (2026-10-26)
**Context findings:** A-004 (resident), S-012 (container), S-013 (ADS), M-011 (dispatch), I-009 (deleted superset), N-023 (v1 dead code)
**Evidence base:** `evidence/decompiled-quotes/s3-refs-engine-map.md`

## OQ-RS-1 — Attribute dispatch on real 3.7+/3.11+ layouts (gates M-011)

Run `scan_refs` (CR_REFS_DEBUG=1) on the corpus bench's Server-2022 ReFS 3.7
image (run 40079065019, body layout class 9); assert `stats.files > 0`,
`stats.data_runs > 0`, and spot-check extent contents. If $DATA rows are
missed, decode the class-9 attribute-key layout (consumers of `param_1[10]`,
Dec:8374/:8784) and fix the dispatch. If all rows are recovered, downgrade
M-011 to INFORMATIONAL (deliberate simplification) and pin with a test.

## OQ-RS-2 — Resident-row key shape (gates A-004's fixture)

Decode the exact on-disk trigger of `FUN_18000e490` (key length 0x10,
`local_f8 == 1`, Dec:10304-10352) — which key bytes compose the in-page data
offset (:10368-70). Build the fixture from a real tiny file on a ReFS volume
and confirm the og recovers content where we do not.

## OQ-RS-3 — Container map construction (gates S-012 step 2)

Find where the og populates the container map (partition object +0xf8; keys
from DBR +0x34/+0xa8): trace `FUN_180011e90` callers and the map inserts.
The formula (`physical = (lcn & (containerSize-1)) | containerBase`) is
verified; the map's key-list construction is not.

## OQ-RS-4 — Do named-stream rows reach the stream handler? (gates S-013's fold question)

On a real volume with an ADS-bearing file, dump the file-table rows (types at
key[0]) and determine whether named-stream runs get folded into the default
runlist (og correctness question — if folded, the og's default-stream content
contains foreign runs and we must NOT replicate).

## OQ-RS-5 — .mo side deleted evidence (gates I-009's re-grade)

Decode `CRefsChildMsbDupProc`/`CRefsChildMsbProc` in
ghidra-mo/RefsFileSystemAnalyser_decompiled.c (42K lines) — decides whether
our deleted superset is parity-safe or double-counted against a partition-side
og harvest.

## OQ-RS-6 — `CRefsAppDataRunItem` semantics (gates S-013's .mo question)

Decode ghidra-mo :13917/:13996 usage — settles whether ANY og component
recovers ADS (the class-hierarchy evidence suggests a data-run item family,
not stream recovery; unproven).

## OQ-RS-7 — System table identities (0x500/0x701 handlers)

Map the og's handlers (`&DAT_180002d30`/`&DAT_1800030d0` at Dec:593-616) vs
our 0x520 exclusion — affects orphan classification only (MINOR).

## OQ-RS-8 — Volume-signature cross-check on data pages

Ours validates page signatures + volume signature on every page
(engine :111-119); confirm whether the og cross-checks the volume GUID on
non-superblock pages (search FUN_18000fc80 consumers). If not, ours is a
superset (record as INFO).
