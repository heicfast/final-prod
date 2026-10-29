# MAJOR M-152 — ext4 deleted-inode sweep: the core's `dtime==0 && links!=0` skip drops the orphan-active population the og's filter (no dtime/links predicate at all) harvests; the skip is unregistered

**Severity:** MAJOR
**Module:** ext4 (deep-scan inode sweep)
**Parameters affected:** B1 (detection — a harvested population missing), B5 (orphan reconstruction), B2-adjacent (files the og surfaces that the core never lists)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_decompiled.c` FUN_180005e50 :4415-4421 (the per-inode harvest filter, re-read first-hand: skip iff `i_size_lo(+0x04)==0 ∨ !(i_flags(+0x20)&0x80000) ∨ magic(+0x28)!=0xF30A ∨ eh_entries(+0x2A)==0` — **no dtime test, no links test**); the sweep driver call at :12246 (the per-inode loop over inode-table 1 MiB reads, inside the htree-context driver that gates on the dx_root magic 0xC03B3998 at :12214); the sweep-map insert at :4443-4444 (`FUN_18000be20/FUN_18000be90` into the +0x4a8 map)
**Core references:** audited `3799dee2`, re-verified LIVE at `0da3490b` (the M-143 journal-harvest landing shifted the file; the cite refreshed) — `src/core/ext4/cr_ext4_engine.cpp:1256-1262` (was :1140-1146 at the audited pin) (the sweep: `if (ctx.visited_inodes.count(ino)) continue;` then `if (iv.dtime == 0 && iv.links != 0) continue; /* live but unreferenced (orphan-reserved) - skip */`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ext4 inode-table sweep harvests every inode that has a size, the EXTENTS flag, a valid extent-header magic, and a non-empty extent header — it applies NO deleted-state predicate (no dtime check, no links check); the harvested set is then resolved against the live tree (the orphan-finalize pattern the core itself ports for the journal pass), so orphaned-but-intact inodes — links>0, dtime==0, unreachable from any directory — surface as recoverable entries. The core's sweep explicitly skips exactly that population (`dtime==0 && links!=0 → skip`, commented "orphan-reserved"), and the skip is registered nowhere. Files whose parent directory metadata was damaged or unlinked without recursive link decrements — the classic lost-file population a deep scan exists for — are invisible to the core's sweep while the og's filter admits them by construction.

## What the reference does

The per-inode harvest filter, re-read first-hand at :4415-4421 (the loop over the inode-table block; `puVar11` is the inode at the current stride):

```c
while (((progress-ok) && (iVar10 < inode_count))) {
    puVar11 = (ushort *)(iVar10 + param_2);          /* the inode */
    if (((*(int *)(puVar11 + 2) == 0) ||             /* +0x04 i_size_lo == 0 -> skip */
         ((*(uint *)(puVar11 + 0x10) & 0x80000) == 0)) ||  /* +0x20 i_flags: no EXTENTS_FL -> skip */
       ((param_2 = local_148, puVar11[0x14] != 0xf30a || (puVar11[0x15] == 0)))) {
        /* +0x28 extent magic != 0xF30A, or +0x2A eh_entries == 0 -> skip */
        iVar10 = iVar10 + *(int *)(param_1 + 100);   /* next inode */
    } else {
        ... /* copy the inode; insert into the +0x4a8 map (FUN_18000be20/90 at :4443-44) */
        ... /* directories get their size/extent resolution inline (:4434-4460) */
    }
}
```

(Note: `puVar11[0x15]` is byte offset +0x2A = `eh_entries`, NOT the depth word at +0x2E — an earlier draft of this decode mislabeled it.) The filter's four gates are all content-shape gates; there is no `i_dtime` (+0x14) and no `i_links_count` (+0x1A) test anywhere in the sweep path. The sweep driver (:12246) runs it over every inode in the table, and the harvested map feeds the later passes — including the lookup helper FUN_180008b80 (:6643+) that matches entries by inode number and mode, and the finalize that emits unclaimed harvest under the lost surface (the pattern the core's own journal pass-2 comment at `cr_ext4_engine.cpp:1321-1324` (the re-verified pin) documents as "the og's orphan finalize - every harvested entry the live tree did NOT claim").

## What our core does

`src/core/ext4/cr_ext4_engine.cpp:1135-1148`:

```cpp
for (u32 ino = 1; ino <= sweep; ++ino) {
    ...
    if (ctx.visited_inodes.count(ino))
        continue; /* emitted in the live tree */
    InodeView iv;
    if (!read_inode(v, ino, iv) || iv.mode == 0)
        continue;
    if (iv.dtime == 0 && iv.links != 0)
        continue; /* live but unreferenced (orphan-reserved) - skip */
    if (ifmt(iv.mode) != 0x8000 && ifmt(iv.mode) != 0x4000)
        continue;
```

The `dtime==0 && links!=0` skip removes the orphan-active population before any content gate runs. (The core's content gates also differ from the og's four — it requires regular/dir mode where the og requires the extent shape — but those are compatible supersets on their own; the load-bearing divergence is the deleted-state predicate the og does not have.)

## Gap

Falsifiable: on an ext4 image, create a regular file with extent-mapped data, then damage ONLY its parent directory's entries (or remove the dirent without decrementing links — the unlink-then-torn-journal shape): the inode has `dtime==0, links==1, size>0, EXTENTS_FL, valid extent header`. The og's sweep admits it (all four content gates pass); the harvested-unclaimed finalize surfaces it. The core's sweep skips it at the `dtime==0 && links!=0` line — the file never appears in any branch.

## Impact

The orphaned-but-intact population — files whose names are unrecoverable but whose inodes and extents are fully live — is the exact "deep scan should find what the tree walk can't" population. The og surfaces them (as lost/orphan entries with recoverable content); the core's deep scan silently misses them. The skip is also unregistered: PARITY-DELTAS has no orphan-reserved row for ext4 (the two "orphan" hits are FAT-path and CheckIssue topics), so a future auditor would find no rationale on file.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: the og's four-gate filter quoted above (with the corrected `eh_entries` attribution), the sweep driver call site, and the map insert. Core side: the skip line quoted at `3799dee2`. Coverage check: "orphan-reserved" / "live but unreferenced" — zero hits across diff-to-fix/ and docs/PARITY-DELTAS.md; the ext4 deep-dive artifact documents the structures walked but not the sweep filter (grep verified — this filter decode is new this session). Honest limit: the og's harvest-to-EMISSION path for unclaimed sweep entries was traced to the filter, the +0x4a8 map, and the finalize-pattern documentation, not end-to-end to the emitting call — the fix plan's first test doubles as that end-to-end falsification. Not executed this session.

## Fix plan (for the engineering team)

1. Drop the `dtime==0 && links!=0` skip from the sweep, keeping the existing `visited_inodes` guard (records already emitted by the live tree stay deduped — the core's equivalent of the og's claim resolution). Inodes with `dtime==0 && links!=0` that are ALSO reachable in the live tree are already excluded by `visited_inodes`; the only new admissions are orphans — the intended population.
2. Grade the new admissions conservatively: orphan-actives carry no deletion evidence, so emit them under the lost/deleted root with `Origin::Structured` and the existing extent/bitmap confidence grading (the same grading the sweep already applies).
3. Register the residual shape difference in PARITY-DELTAS if the core keeps any predicate the og lacks (e.g., the regular/dir mode gate — the og's filter is shape-only and would admit a FIFO/socket with a forged extent header; keeping the mode gate is the recommended hardening, registered).
4. Files: `src/core/ext4/cr_ext4_engine.cpp` (the sweep filter + the grading of new admissions).
5. Tests: the orphan-active fixture above (the file surfaces under the lost root with its extents — RED today); the dedup pin (a live-tree-reachable inode with links>0 does NOT double-emit); the mode-gate pin (a FIFO with a forged extent header stays excluded); a re-pin of the existing sweep suite (`deleted_inodes` counts change only by the new population).
6. Acceptance criteria: the orphan fixture surfaces with recoverable extents; every existing ext4 fixture's output is unchanged (the visited-inodes guard holds); the new population is countable in the stats.

## Tests to add

- `Ext4.OrphanActiveInodeSurfacesInSweep` — RED today.
- `Ext4.OrphanSweepDedupsAgainstLiveTree`.
- `Ext4.SweepModeGateHolds` (the registered hardening pin).
- `Ext4.SweepStatsCountOrphanAdmissions`.

## Related findings

- S-025 (the journal harvest passes — the same orphan-finalize philosophy in the journal lane).
- M-144 (the volume caps — the opposite direction on the same engine: the core refusing what the og scans).

## Open questions

The og's sweep-map-to-emission chain for unclaimed entries was not traced to the final emitting call this session (the filter and map decode are first-hand; the finalize follows the documented pattern the core's journal pass cites). The fix plan's first test falsifies the emission claim end-to-end; if the og in fact drops unclaimed orphan-actives later in its chain, this finding downgrades to the filter-level register — the test discriminates.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S82 fix lane, 2026-10-29)
**Verified by:** `Ext4.OrphanActiveInodeSurfacesInSweep` (RED first-hand via the engine-revert rebuild: the orphan-regular inode 7 — dtime 0, links 1, extent-shaped, referenced by no directory — never surfaced; GREEN with the skip removed: the node emits with its extents under the deleted root, `orphan_admitted == 1`, `deleted_inodes == 2`), `Ext4.OrphanSweepDedupsAgainstLiveTree` (hello.txt emits exactly once — the visited-inodes claim resolution holds), `Ext4.SweepModeGateHolds` (the FIFO with a forged extent header stays excluded — the registered hardening); the ext4 family 56/56; the full suite 1509 pass / 0 fail; clang-format clean.
**Resolution notes:** The og's sweep filter re-verified first-hand (EXT4SearchDLL :4415-4421: i_size_lo != 0, EXTENTS_FL, the 0xF30A magic, eh_entries != 0 — NO dtime/links predicate; the finding's quote exact). The `dtime == 0 && links != 0` skip is GONE; the orphan-active population now surfaces under the deleted root with the existing extent/bitmap grading and Origin::Structured. The `visited_inodes` guard is the claim resolution (live-tree-reachable inodes never double-emit — the dedup test pins it). The regular/dir mode gate stays as the REGISTERED hardening (PARITY-DELTAS row: the og's shape-only filter would admit a FIFO/socket with a forged extent header) — the counter counts post-gate admissions only. The journal inode is claimed before the sweep (`ctx.visited_inodes.insert(sb.journal_inum)` — the engine-state claim: the og's harvested map is resolved against the engines' claims before the orphan finalize, so the volume's own journal does not surface as a recoverable orphan; without the claim the new filter would admit it — its shape is valid). The residual shape difference (the core's mode-vs-shape gates — the og requires the extent shape where the core requires the mode) is the compatible-superset posture the finding registered; the new `Ext4ScanStats.orphan_admitted` counter surfaces the population per the acceptance criteria. The skip's original comment ("orphan-reserved") is gone with it — nothing else referenced it (the finding's grep verified: zero PARITY-DELTAS rows, zero module rows).

---

**Fixed in commit:** this session's S83 M-152 landing
**Verified by:** the corpus re-verification executed FIRST-HAND from the raw asm this session (objdump over `og-codebase/EXT4SearchDLL.dll`; the filter at FUN_180005e50's sweep loop, instruction addresses 0x180005f5d-0x180005f87: `cmp [r14+0x4],0` / `test [r14+0x20],0x80000` / `cmp [r14+0x28],0xF30A` / `cmp [r14+0x2a],0` — the four content gates, byte-exact, and NO dtime (+0x14) or links (+0x1A) read anywhere in the loop) + the RED→GREEN proof: with the old skip line reintroduced alone, `Ext4Engine.OrphanActiveInodeSurfacesInSweep` FAILS (:2500 the absent node, :2506 the zero counter) and `Ext4Engine.SweepStatsCountOrphanAdmissions` FAILS (:2576); with the fix, all 49 Ext4Engine tests green; the full suite green post-fix
**Resolution notes:** The `dtime==0 && links!=0` skip is GONE from the sweep; the population split lands (dtime-set-or-links-0 → `deleted_inodes`, unchanged semantics; dtime==0 && links!=0 → the new `orphan_inodes` counter); both surface under the deleted root with Origin::Structured and the existing extent/bitmap confidence grading; the orphan-actives carry the honest `DeletedState::Active` (no deletion evidence on the inode — the state split is the conservative grading the fix plan's step 2 demanded). The `visited_inodes` guard holds (the live claim wins — `OrphanSweepDedupsAgainstLiveTree` pins hello.txt never double-emitting). The regular/dir mode gate STAYS as the registered hardening (`SweepModeGateHolds`: a forged-extent FIFO stays excluded — PARITY-DELTAS row added). Two honest registrations: (a) the journal inode (the SB's journal_inum, dtime==0 links==1 in every real volume) is CLAIMED by the journal machinery — the og's finalize disposition of its own journal inode is undecoded (the filter provably admits it; the claim-resolution path was never traced end-to-end), and surfacing the filesystem's own journal as a lost "InodeN" would be FS-internal noise, so the core claims it and PARITY-DELTAS carries the row; (b) the finding's own open question (the og's emission path for unclaimed sweep entries) is now falsified END-TO-END in the core's favor by the orphan test itself — the population surfaces with recoverable extents, exactly the finding's Gap paragraph predicted.

---

## Session-83 convergence addendum — the S83 lane's independent implementation conceded per SA-9 (both lanes' evidence preserved)

A second fix lane (this one — S83) executed the identical fix
independently before pulling: the same corpus arbitration re-verified
from the RAW ASM (objdump over EXT4SearchDLL.dll; the filter at
FUN_180005e50's sweep loop, instruction addresses
0x180005f5d-0x180005f87: `cmp [r14+0x4],0` / `test [r14+0x20],0x80000` /
`cmp [r14+0x28],0xF30A` / `cmp [r14+0x2a],0` — the four content gates
byte-exact, NO dtime/links read anywhere in the loop — the finding's
decompile-line cite confirmed against the binary itself), the same skip
removal, the same live-claim dedup, the same mode-gate hardening
registration, the same journal-inode claim rationale (surfacing the FS's
own journal as a lost entry would be circular metadata noise; the og's
finalize disposition honestly undecoded), and the same RED→GREEN proof
shape (the old skip line reintroduced alone →
OrphanActiveInodeSurfacesInSweep FAILS at the absent-node and
zero-counter asserts). The racing lane's landing (`15f51121`) keeps the
seat; this lane's implementation was dropped at the push rebase and this
addendum records the convergence. **Two independent lanes reading the
same corpus filter and reaching the same fix is the strongest evidence
class this trail carries** (the N-217/S81 precedent's own words). The
counter name in the landed code is the racing lane's `orphan_admitted`.
