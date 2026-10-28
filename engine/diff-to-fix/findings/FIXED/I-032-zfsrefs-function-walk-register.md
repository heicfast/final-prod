# INFORMATIONAL I-032 — the ZFS/ReFS function-level walk register: OQ-T6 resolved first-hand (the og DOES resolve multi-vdev DVAs — first-registry-entry fallback), the ReFS grading byte decoded, the unnamed-row no-fallback decoded, ZFS timestamps close as MATCH, and both engines' volume-label question closed

**Severity:** INFORMATIONAL (a resolutions-and-decode register: two prior open notes close as MATCH, two prior unknowns decode (one converts to S-053, one to N-141), and the volume-label completion set closes — the walk found no new SEVERE/MAJOR beyond S-053)
**Module:** zfs / refs (the two engine trees the S10-F class pass's read-agents did not cover)
**Parameters affected:** B8 (multi-volume — OQ-T6/B8 close as MATCH), B9 (timestamps — zfs.md's precision note closes as MATCH), B4 (grading — the og's ReFS grade shape decoded), B5 (orphan/naming — the no-fallback decoded), D11-adjacent (the grade byte's node offset)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ZfsSearchDLL_decompiled.c:19544-19556` (`FUN_1800168b0`: the DVA vdev-id registry search — `(DVA.w0>>32)&0xFFFFFF` at `:18027`; on search exhaustion the walker falls back to `*(param_1+0x18)`, the FIRST registry entry, under LOCK — **no-match → vdev 0's extent, never fails**); the ZPL time family `:24398/:24520/:24642/:24764` (ATIME/MTIME/CTIME/CRTIME all through `FUN_180010ef0`, hard-capped at 8 bytes — `:14937 … < 9` — no nanosecond persistence anywhere); `FUN_180006110 :4839-5051` (the .trashinfo CONTENT parse — ZfsDataRunlist materializes the sidecar and parses "Path=", building {stripped-name→orig-parent} maps at `:5020-5023`, no in-DLL consumer); `:5671-5673` (the fixed 1000 ms device poll, UNBOUNDED); `:17996` (the fixed +4 MiB DVA translate); `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — the grade byte at node+0x70 (2 bits; 2=deleted; set from record+0x59 via `:8476`; counted `:7137-7149`; the host bulk re-grade hook `FUN_18000a2c0 :7157-7159`); `FUN_1800041f0 :2141-2163` (the ReFS label branch is DEAD — no writer of ctx+0x1b8; the volume node is the constant "REFS"/"ReFS"); the ZFS FS-name builder `:40011` (cases 0-10 only, NO case 13 — no "ZFS" pool-name string; binary-verified)
**Core references:** audited `46b81573` — the multi-vdev handling landed under M-019 (FIXED — convergent with the og decode); `src/core/zfs/cr_zfs_engine.cpp` (the 8-byte time cap matches); `src/core/refs/cr_refs_engine.cpp` (the finer grading is the registered superset); N-133's completion set (neither ReFS nor ZFS decodes an on-disk volume label — nothing owed for parity on these two engines)
**Status:** READ-ONLY REGISTER — informational; the two defect-class outcomes are filed separately (S-053, N-141)

## Summary

The S11 function-level walk of ZfsSearchDLL (981 functions; 61 engine bodies
read in full) and ReFSSearchDLL (629 functions; ~35 read in full) — the two
engine trees no prior lane had function-walked. Every prior-filed verdict
held. The register records: OQ-T6 (multi-vdev) resolves first-hand in the
og's favor of M-019's landing; the ZFS timestamp note closes as MATCH (both
sides cap at 8 bytes); the ReFS grading machinery decodes (a 2-bit grade
byte — the core's finer grading stays the registered superset); the og has
NO unnamed-row fallback (the core's drop becomes S-053); and the S10-F
volume-label question closes for both engines (neither decodes an on-disk
label — the dead ReFS branch and the missing ZFS case-13 are the
instruction-decisive evidence).

## The register entries

1. **OQ-T6 / zfs.md B8 — RESOLVED (MATCH)**: the og's DVA translate resolves
   multi-vdev pools through the first-registry-entry fallback (mirror OK;
   stripe reads the local half; raidz beyond checksum repair drops). The
   core's M-019 landing is convergent. The "og extent unknown" note in
   zfs.md retires.
2. **zfs.md B9 — closes as MATCH**: the og persists ZPL times at 8-byte
   precision (no nsec fields anywhere in the persistence path); the core's
   one-second `unix_to_filetime` matches the og's shape. The "precision
   note" retires.
3. **refs.md B4 — the grading decoded**: 2-bit grade at node+0x70 (2=deleted),
   set from record+0x59, with a host-side bulk re-grade hook. No tiers in
   the DLL — the core's finer grading stays the registered superset; the
   "boundaries not decoded" note retires with this decode.
4. **refs.md B5 — the naming decoded**: the og emits empty-name rows with no
   fallback name (the core's drop is the S-053 divergence; the "fallback
   parity unknown" note retires).
5. **Volume labels (N-133's completion set)**: ReFS's label branch is dead
   code (no ctx+0x1b8 writer); ZFS's FS-name builder has no pool-name case.
   Neither og engine surfaces an on-disk label — N-133's NTFS/ext4/FAT legs
   are unaffected; these two engines owe nothing.
6. **The .trashinfo content parse** (UNVERIFIED consumer): the og's ZFS
   engine materializes .trashinfo sidecar CONTENT and parses "Path=" into
   {stripped-name→orig-parent} maps with no in-DLL consumer — a
   half-implemented trash-restore path or a DRWUI-side consumer. The core
   reads no sidecar content (identical visible emission). Registered as the
   walk's one open decode.
7. **Decode refinements** (CORPUS-ERRATA material): the og device poll is a
   fixed 1000 ms UNBOUNDED (refines N-056/N-072's "backoff" wording — the
   core's bounded 3×250/500/1000 stays the safer subset); the DVA translate
   is a fixed +4 MiB (the core's nvlist-driven vdev offset is identical on
   whole-disk pools — the registered superset).

## Impact

None as defects; closes three module-doc notes and one tracked-question
lineage, and hands S-053/N-141 their decode evidence.

## Reproduction / verification

Decompiled (strength 2): every cite re-read first-hand by the lead (the
fallback chain, the 8-byte cap, the grade byte, the dead label branch); the
full walk with both per-function disposition tables at
`evidence/decompiled-quotes/s11-zfsrefs-function-walk.md` (§4 carries the
open-question resolutions with quoted snippets).

## Fix plan (for the engineering team)

Documentation-only: zfs.md's OQ-T6/B8/B9 rows and refs.md's B4/B5 rows flip
to the closed verdicts citing this register; the CORPUS-ERRATA refinements
(#7) go to the next errata batch.

## Tests to add

None (the two behavioral outcomes carry their own tests in S-053/N-141).

## Related findings

- S-053 / N-141 (the walk's two defect filings)
- N-133 (the volume-label completion set this closes for ReFS/ZFS)
- M-019 (the multi-vdev landing this confirms)
- I-031 (the RAWSearchDLL counterpart register)

## Open questions

The .trashinfo content-parse consumer (entry 6): a DRWUI-side walk of the
ZFS result consumers would resolve whether the og's UI offers a
trash-restore affordance the maps feed; no core behavior attaches today.

## Resolution

**Status:** FIXED-ALREADY (the finding's documentation-only fix plan was executed by the filing/reconciliation lanes; this session re-verified every item on the current tree)
**Fixed in commit:** (the S11/S11-R/S40 lanes' own landings; re-verified at session-42 — no new commit required)
**Verified by:** reproduction on the current tree: modules/zfs.md's OQ-T6 row reads "RESOLVED (session-11, I-032)" with the first-registry-entry-fallback decode + the MATCH verdict; B8 reads "MATCH (OQ-T6.2 resolved with OQ-T6 — I-032)"; B9 reads "closed MATCH, session-11 — I-032" with the 8-byte-cap decode and the refuted nanosecond note; modules/refs.md's B4 row carries the 2-bit grade-byte decode (node+0x70, 2=deleted, record+0x59, the host re-grade hook) and B5 carries the S-053 resolution + the N-141 companion; diff-to-fix/CORPUS-ERRATA.md §items 6-7 of the S11 batch carry the two decode refinements (the fixed 1000 ms unbounded device poll refuting N-056/N-072's "backoff" wording; the fixed +4 MiB DVA translate). The two defect-class outcomes carry their own landings: S-053 + N-141 (session-40 main lane, `e10ec381`) and the M-019 multi-vdev convergence the register confirms.
**Resolution notes:** Every fix-plan item verified present on the current tree — the module-doc verdict flips cite this register exactly as the plan specified, and the CORPUS-ERRATA refinements landed in the S11 batch. The walk's one open decode (the .trashinfo content-parse consumer, entry 6) remains registered as the open note with no core behavior attaching — the DRWUI-side consumer walk is the recorded next step if ever prioritized. No code change; no new divergence found on re-verification (the grade byte, the no-fallback naming, the 8-byte time cap, and both engines' dead/absent label paths all re-checked against the register's cites).
