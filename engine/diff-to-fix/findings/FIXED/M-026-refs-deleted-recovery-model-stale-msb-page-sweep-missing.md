# MAJOR M-026 — ReFS deleted-file model: og harvests stale MSB pages from the object-ID area; core harvests row flags + ID2 evidence only

**Severity:** MAJOR (candidate for SEVERE escalation — see Impact; lead may assign an S-tier ID)
**Module:** refs
**Parameters affected:** B4 (deleted-file classification), B5 (orphan reconstruction), S35 (error recovery / damaged-volume tolerance), A25 (engine algorithm steps), A26 (tree walk logic)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — FUN_180005330 :2819–3054 (stale-page sweep, "MSB+" 0x2b42534d at :2904, cache-skip :2910–2931, forced deleted marking :2939, record dispatch :2942–2965), FUN_180003f40 :2015 (unconditional sweep call), FUN_18000ba90 :8476 (deleted bit from record+0x59), FUN_1800089f0 call sites :2385/:8469 (used-space marking of live files)
**Core references:** `src/core/refs/cr_refs_engine.cpp` @ `34b3441` — deleted harvest via row flag :805–810 (`ENTRY_DELETED = 0x4` :45), ID2 evidence rows :848–875, allocator grading :653–691, orphan tables :902–920; no stale-page sweep anywhere in the file
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ReFS deleted-file recovery is a **stale-metadata-page sweep**: after walking the live
checkpoint chain, it scans the object-ID-table area for "MSB+" pages that are *not* reachable
from the live chain (not in the page cache), parses their records, force-marks every record
deleted, and emits them through the normal pipeline. Live files' runs are additionally marked
in a used-space/read-error map. Our core's deleted model is row-flag-based (`flags & 0x4`) plus
ID2 name-evidence rows and allocator-bitmap grading — a different on-disk premise. ReFS is
copy-on-write: deleted objects' metadata persists as stale page versions, and the og's model
matches that; a row-flag harvest only yields results if live rows carry an explicit deleted
flag.

## What the reference does

`FUN_180005330` (ReFSSearchDLL_decompiled.c:2819–3054), called **unconditionally** after the
live walks by the main scan driver (`FUN_180003f40:2015`):

1. Sets the harvest flag in the record-processor context (:2859 — this is what makes
   `FUN_18000ba90:8476` set the deleted bit in file properties).
2. Reads the object-ID-table allocated ranges from the device (`FUN_180011f20` on device+0x108,
   :2860) — the area whose size came from the DBR (`container size / cluster`,
   `FUN_180012390:13560`, stored at ctx+0x148 via `FUN_180004980:2463`).
3. Scans it in 0x80-cluster windows (:2863–2909) for pages whose first DWORD is
   **"MSB+" (0x2b42534d)** (:2904) and that are **not already in the page cache**
   (:2910–2931) — i.e., older metadata-page versions the live chain no longer references.
4. Parses each such page's records (`FUN_18000ae70` :2932–2934) and runs the *same* live
   dispatch (type 0x10 → FUN_18000cbd0, 0x30/1 → FUN_18000ba90, 0x30/2 → FUN_18000c920,
   :2942–2965) with **every record force-marked deleted** (record+0x59 := 1, :2939) — so stale
   pages yield deleted-file results with full names/timestamps/runs through the normal emit
   path (deleted bit at `FUN_18000ba90:8476`; keep-one dedup and flush unchanged).
5. Separately, every live file's runs are marked in the used-space/read-error map
   (`FUN_1800089f0` at `FUN_18000ba90:8451–8473` and `FUN_180004820:2385`) — the og's
   cluster-liveness model for grading.

## What our core does

`cr_refs_engine.cpp:805–810`: a directory row is deleted iff `e2.flags & 0x4`
(`ENTRY_DELETED`, :45); deleted rows go to the deleted root with allocator-bitmap grading
(:766–779, :653–691 — "reused clusters ⇒ Overwritten else PartialRisk"). ID2 rows
(0x80000020, :36) are harvested as metadata-only name evidence (:848–875). Orphan *tables*
(unreachable in the ObjectID map) go to the lost tree (:902–920). **No stale-page sweep
exists**: pages not reachable from the chosen checkpoint are never read, and the object-ID
area is never scanned by signature.

## Gap

Falsifiable: on a ReFS volume where files were deleted (their metadata persists only as stale
MSB pages in the object-ID area, with no deleted-flagged rows in the live tables — the normal
copy-on-write outcome), the og's sweep emits those files as deleted-recoverable results; our
core emits none of them (and its `harvest_deleted` yield is limited to any 0x4-flagged rows
and ID2 rows that happen to exist).

## Impact

- Deleted-file recovery on ReFS is likely **near-zero** in our product where the og yields
  deleted files with names, timestamps and extents — a user-observable recovery-yield gap of
  the same class the SEVERE scale names ("an engine the reference ships … is absent"). Filed
  MAJOR because one premise is not decompile-provable: whether real ReFS volumes also carry
  0x4-flagged rows (the core's premise). No real-volume fixture exists in the corpus (vendor
  QA scanned no ReFS volumes — `SaveScan/ifileproperties-part2--ext4-hfs-refs.md` §3 note).
  If the bench stage later shows zero deleted-ReFS yield against the og on a real volume,
  escalate to SEVERE.
- The og's used-space marking (live-run cluster map) feeds its grading; our allocator-bitmap
  grading approximates the same outcome by a different source — parity of *grading outcomes*
  is plausible, but the *candidate set* being graded differs (see Gap).

## Reproduction / verification

Decompiled evidence (strength 2): all citations above; inventory file §6.5.
Core side read first-hand at HEAD `34b3441` (lines cited). Executed comparison impossible
(no ReFS volume fixture; og is Windows-only). Next probes in scratch report: (a) ReFS thesis
(Prade 2019) cross-check whether index-entry flag 0x4 is ever set by ReFS on delete;
(b) construct a synthetic double-checkpoint fixture where a file exists only in the older
page version → run `scan_refs` → assert zero deleted results (deterministic from the code:
no sweep exists), vs the og's documented sweep behavior.

## Fix plan (for the engineering team)

1. Add a stale-page sweep phase to `scan_refs` (after step 8, before/alongside step 9):
   - Locate the object-ID area: the og derives its size from the DBR container-size field
     (+0x40 / cluster) and its position from the device/DBR object-ID field; for our engine,
     the ObjectID-table root from the checkpoint (ptr 0) already bounds the same region —
     scan the clusters spanned by the ObjectID table's own pages plus the DBR-derived extent.
   - Read 0x80-cluster windows; accept pages whose first DWORD is `SIG_MSB` and that were
     never fetched during the live walk (track visited first-LCNs — the `visited` sets plus
     the page cache already provide this).
   - Parse each page with the existing node parser and run the row dispatch with a
     force-deleted flag into the deleted root (mirror og `FUN_180005330:2939`).
   - Bound the sweep (og chunking 0x80 clusters; add a cancel gate per window — S32 parity).
2. Keep the existing row-flag and ID2 harvest (both are supersets vs the og, which handles
   neither 0x4-flagged rows nor 0x80000020 rows at the page dispatch — see inventory §7).
3. Wire deleted grading for sweep-found files through the existing allocator-bitmap
   `Confidence` model (unchanged).
4. Update `docs/CAPABILITY-MATRIX.md` (ReFS deleted row) and the module file B4/B5 rows.
5. Acceptance: double-checkpoint synthetic fixture — file present only in the stale page:
   og-equivalent behavior = one deleted result with extents; ours today = none.

## Tests to add

- `Refs_Stale_Msb_Page_Deleted_File_Recovered` (older checkpoint-era page in the OID area →
  deleted result with name/timestamps/runs).
- `Refs_Stale_Sweep_Skips_Live_Pages` (page also referenced by the live chain → emitted once,
  as live).
- `Refs_Stale_Sweep_Cancel_Gated` (cancel token stops between windows; partial results kept).
- `Refs_Deleted_Row_Flag_Still_Harvested` (0x4-flagged live row → deleted root, grading
  unchanged — superset preserved).

## Related findings

- S-020 (attribute layer — the sweep reuses the same record processors; fix order: S-020 first
  so sweep-found files recover resident/paged content too).
- M-027 (object-identity keep-one — sweep-found stale records participate in the same dedup).
- S-006 (lost-partition probe set — the *discovery* side; this finding is the engine side).
- modules/refs.md B4 note ("og's ReFS deleted-grading boundaries not decoded in corpus") —
  now decoded: the deleted premise is the stale-page sweep, not a grading tier boundary.

## Open questions

- Whether ReFS ever sets index-entry flag 0x4 on delete (our harvest's premise). Next
  artifact: Prade 2019 thesis table 4.38 flag semantics vs a real-volume dump (none in
  corpus — UNVERIFIED).
- The og's read-error/used-space map (`FUN_1800089f0`, not fully decoded this session) —
  whether it feeds grading beyond the live-run marking seen at the call sites (UNVERIFIED;
  next artifact: FUN_1800089f0 body + DRWUI consumer).

---

## Session-3 parallel-leg cross-reference (2026-10-26)

The *storage/census parallel leg*'s ReFS decode independently verified the
opposite-direction fact this finding should be read with: our flag-4/ID2/orphan
harvest + allocator grading (`cr_refs_engine.cpp:806-875`) has **no counterpart
in the shipped SearchDLL** — the deleted-marker setter `FUN_180013320` :14528
has zero callers. Directionality: this finding (M-026) = the og's stale-MSB-page
sweep we LACK; the parallel leg's record = the deleted-row harvest we HAVE that
the og's engine does not produce. Parity benches must net both directions.

## Resolution

**Status:** FIXED
**Fixed in commit:** `7feb79b`
**Verified by:** Refs.StaleMsbPageDeletedFileRecovered (fail-before FAILED on the sweep-less engine — the stale-only file never surfaced; pass-after: deleted result with name + extent at the planted run), Refs.StaleSweepSkipsLivePages (regression GUARD for the live-page exclusion — trivially green pre-fix by the sweep's absence; pins that the landed sweep never double-harvests live pages), plus the S-020 tests still green (the sweep reuses the attribute walk). Suite 429/429 Linux ASan (402 + 4 net-new M-026/M-027 tests).
**Resolution notes:** The stale-MSB-page sweep landed in scan_refs (phase 8b), mirroring FUN_180005330: after the live walk, the MSB/OID area is scanned in 0x80-cluster windows for "MSB+"-signed, volume-sig-validated pages NOT reachable from the live chain; their ROW_FILE records dispatch through the normal emit_file (names/timestamps/full attribute walk incl. resident + paged content from the S-020 landing) with the force-deleted flag (og record+0x59 := 1) into the deleted root, gated on the new stale_sweep flag (default on = the og's unconditional call). Live-reachability is tracked by an UNBOUNDED live_pages set recorded in read_page (the og's page-cache skip :2910-31; the core's bounded 256-entry cache would evict on large volumes and re-harvest live pages - the unbounded set is the correct form). Area bound: min(volume_clusters, DBR container-bytes/cluster) — the og reads the device's OID allocated ranges (ctx+0x148 = container/cluster, FUN_180012390:13560); the DBR-derived bound covers the same region on standard layouts. Bounded simplification, documented: only ROW_FILE rows dispatch from stale pages — stale dir-links reference tables outside the live table map (the og resolves them through its own machinery), and the orphan-table walk already covers unreachable live tables; the loss is stale EMPTY-directory names, not file content. The stale rows flow through the M-027 identity map (live rows win over stale copies of the same object). The existing row-flag + ID2 harvest stays (superset vs the og - the session-3 parallel leg verified the og has no flag-4/ID2 counterpart). The open question "whether ReFS sets flag 0x4" stays open but is now moot for yield: both deleted models are active. Escalation note: with the sweep landed the finding's SEVERE-escalation condition (near-zero deleted-ReFS yield) no longer holds.
