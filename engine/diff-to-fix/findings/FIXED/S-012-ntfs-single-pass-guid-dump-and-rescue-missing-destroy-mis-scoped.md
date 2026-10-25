# SEVERE S-012 — NTFS is single-pass: the og's GUID-dump pass 2 (viability/destroy) and size-0 rescue are missing, and the S-001 destroy rule landed in the wrong pass

**Severity:** SEVERE (result-set membership divergence in both directions; missing subsystem)
**Module:** ntfs
**Parameters affected:** B1 (detection/result-set membership), B2 (recovery correctness), B4 (deleted classification), B5 (orphan reconstruction), D15 (confidence grading — og has no main-walk grade)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c` — pass 1: `FUN_180017cf0 :18875+` (walk gates :18952-18955), `FUN_180017390 :18424-18636` (per-record processor; size-0 rescue gate :18493-18499; unconditional emit :18630); pass 2: GUID-file collection `:18584-18625` (template memcmp `"{3808876b-c176-4e48-b7ae-04046e6cc752}"` :18607-09), `FUN_18001a980` walk `:17983-17984`, `FUN_18001acc0 :21095+` (record parse from dump buffers; viability per-run-entry :21220-21229; destroy :21249→LAB_18001b06f :21257; 0x3ff gate :21208; metafile blanket-`$` filter :21186); rescue chain `FUN_18001e6c0 :24022-24168` (+ `FUN_18001e120 :23722` MFT-copy resident extraction, `FUN_18001d690 :23215` same-name merge, `FUN_18001d960 :23391` runlist adoption, re-emit `FUN_180016440 :17715-17785` code 0xfe0001)
**Core references:** `src/core/ntfs/cr_mftwalk.cpp:652-708` (single walk: grading + destroy at :704), `include/cr/ntfs/cr_mftwalk.h:54` (`destroy_fully_overwritten = true` default), `src/core/ntfs/cr_bitmap.cpp` (per-cluster `reused_ratio`) — audited commit `e93decf`
**Status:** READ-ONLY FINDING — no fix applied. Supersedes the scope premise of S-001 (whose fix landed as specified; see `resolved/findings/S-001-*.md` postscript).

## Summary

The og's NTFS scan is **two-pass**: pass 1 walks the live MFT and emits every record (deleted included) **with no viability grading and no destroy**; pass 2 collects *live* GUID-named files (on-disk MFT-record dumps matching `{3808876b-c176-4e48-b7ae-04046e6cc752}`), reads their `$DATA` as raw MFT-record buffers, and only THERE applies the viability accounting, the `size ≤ 0x3ff` destroy, and the blanket-`$` metafile filter. The core has a single walk: it grades every deleted file against the volume `$Bitmap` per-cluster, and — since the S-001 fix, by default — **destroys** fully-overwritten ones there. Result-set membership now diverges in both directions: the core drops live-MFT deleted files the og shows (flag ON), and lacks the dump-recovered records only pass 2 can find (no pass 2 at all). The og also has a **size-0 deleted rescue** (resident `$DATA` harvested from MFT copies + same-name candidate merges) the core lacks.

## What the reference does

**Pass 1 (live MFT)** — `FUN_180017cf0`: per record, gates are fixup (`:18954`), base-record (`:18955`), metafile name-table+hidden+system (`FUN_180034b20`, `:18952-53`); then `FUN_180017390` claims live/dir extents into a map (engine[0x4a], built by `FUN_180017190`), collects GUID-named live files into a list (`:18584-18625`), and **emits unconditionally**:

```
:18630  (**(code **)(*param_1 + 0x10))(param_1,param_4,plVar10,param_2,...);   // emit
```

Deleted records are merely marked (item code 1 at `:18562-66`); there is **no viability/destroy path in pass 1**. Deleted size==0 non-dir records take a *rescue* detour, not a grading one:

```
:18494 if (((lVar11 == 0) && (FUN_180041940(...)==0 /*not dir*/)) &&
           (props->vt[0x68]()&1 /*deleted*/)) {
:18498    FUN_18001e6c0(param_1 + 0x3b, ...);   // RESCUE: resident $DATA from MFT
                                               // copies + same-name merge + runlist adoption
:18499    *(undefined2 *)(lVar1 + 0x2a) = 2;     // item marked "rescued"
```

`FUN_18001e6c0` never probes the claim map, never grades, never destroys (props+0x72 bits 1/2 are set, `:24160-61`); merged props are re-emitted later with code `0xfe0001` (`FUN_180016440 :17778-85`).

**Pass 2 (GUID-dump walk)** — after pass 1, the engine walks the GUID-file list's runlists in ≤0x8000-sector chunks and calls `FUN_18001acc0` per chunk, which **parses the file contents as MFT record buffers**: 'FILE' magic (`:21152`), fixup, metafile filter `(flags&4) || !name || name[0]=='$'` → skip (`:21186`), props claim/dedup by record#, then:

```
:21208      if (0x3ff < size) { ...viability... } else /* falls into destroy */
:21220-29   per RUNLIST ENTRY: if (*entry != -1 /*not sparse*/) { total++;
              sector = spc * lcn;  if (FUN_18002c2e0(map, sector)) intact++; }
            // ONE probe at the run's START sector; map = pass-1 claimed extents
:21249      if ((0 < iVar21) && (iVar20 == iVar21)) goto LAB_18001b06f;  // destroy
:21257      LAB_18001b06f: props->vt[0x138](props,1);   // Release — dropped
```

The claim map contains pass-1 **live + dir + deleted-size>0** extents (claim condition `:18525`; deleted-with-size records ARE claimed, `FUN_180017190` skipping state-bit0 records only when engine+0x258 is set, `:18343-52` — default off).

## What our core does

One walk: `walk_mft` iterates the live MFT; every deleted non-dir record is graded per-cluster against the volume `$Bitmap` (`cr_bitmap.cpp` `reused_ratio`, 64 MiB probe cap) and, when `WalkerFlags::destroy_fully_overwritten` (**default true**, `cr_mftwalk.h:54`) and the grade is `Overwritten`, dropped before node creation (`cr_mftwalk.cpp:704-708`). There is no GUID-dump pass, no size-0 rescue, no same-name merge, no MFT-copy resident extraction.

## Gap

Falsifiable claims:
1. A fully-overwritten deleted file in the **live MFT**: og result set contains it (emitted ungraded at `:18630`); core (default flags) **omits** it. Membership: core ⊂ og.
2. A file whose MFT record was **reused after deletion** (recoverable only from a GUID-named MFT dump): og result set contains it (pass-2 emit, `LAB_18001afe6` list insert); core has no code path that can find it. Membership: og ⊄ core.
3. A deleted size-0 record with resident `$DATA` still present in an MFT copy: og rescues content/size (e6c0 chain) and re-emits; core emits a size-0 node.
4. A dump-recovered record whose run STARTs are all claimed by pass-1 extents but whose interiors are free: og destroys (per-entry accounting, one probe per run); core — for the analogous live-MFT record — grades `PartialRisk` and keeps (per-cluster ratio).

## Impact

- **Recovery correctness:** dump-only records (pass 2) are exactly the records users lose MFT damage/reuse to — the og's deep-scan advantage on fragmented histories; the core cannot produce them at all.
- **Result-set parity:** with the S-001 fix ON the core now also *loses* records the og shows (healthy-volume deleted files that are fully overwritten) — a regression introduced by a fix aimed at the wrong pass. Every og-comparison inventory diverges in both directions.
- **Grading model (D15):** the og applies no grade in pass 1; grading machinery (Recovery Chances) lives downstream (see OQ-S2M-1). The core's in-walk grading is a superset feature — fine only if membership parity is restored first.

## Reproduction / verification

- Decompiled evidence (strength 2): all quotes above, file+line cited; consolidated in `evidence/decompiled-quotes/ntfs-pass2-guid-dump.md`. Two independent session-2 agents (S2-A2, S2-G1) plus a first-hand re-read of :18488-18500, :18625-18633, :21246-21264 agree.
- Core side quoted at `e93decf` (single-walk + destroy default ON verified first-hand).

## Fix plan (for the engineering team)

1. **Membership first:** flip the default of `WalkerFlags::destroy_fully_overwritten` to `false` (or remove its application from the live-MFT walk) so the main walk restores og pass-1 membership (emit all deleted records). Keep the `destroyed_overwritten` counter for the pass-2 analogue.
2. **Add pass 2 (the real og feature):** during the main walk, collect live files whose name is 0x4C chars, starts `{`, ends with the GUID template (`memcmp` against `{3808876b-c176-4e48-b7ae-04046e6cc752}` — quote exact template from `:18607-09`); after the main walk, read each collected file's `$DATA` in ≤0x8000-sector chunks and parse as MFT records with: 'FILE' magic + fixup + base-record gates; skip `(flags&4) || !name || name[0]=='$'`; dedup against pass-1 records by record#; **viability = per-runlist-entry probe of the run START sector against a pass-1 claim map** (live + dir + deleted-size>0 extents — build the map alongside the walk, not from `$Bitmap`); destroy when `total > 0 && intact == total` **or** `size ≤ 0x3ff`; folders (`$FILE_NAME` flags 0x10000000) bypass viability entirely.
3. **Add the size-0 rescue in the main walk:** for deleted non-dir size-0 records, harvest resident `$DATA` from MFT-copy sector lists (validate fixup + record# match), merge same-name candidates (runlist + size steal, skip compressed 0x800-flagged), adopt sanity-checked runlists, and re-emit with a "rescued" marker.
4. **Document the model** in `docs/parity/PARITY-DELTAS.md`: two-pass architecture, claim-map semantics (NOT `$Bitmap`), per-run-entry accounting, the 0x3ff dump-pass rule, folders-never-destroyed, no main-walk grading (grading is downstream).
5. Bench: re-run `permanent_delete` and add a GUID-dump fixture (a volume containing a `{3808876b-…}`-named file wrapping stale MFT records) — the latter must go from 0 to >0 dump-recovered records.

## Tests to add

- `NtfsVolume.MainWalkKeepsOverwrittenDeleted` — fully-overwritten deleted live-MFT file: node present under default flags (fails today with the S-001 default).
- `NtfsVolume.GuidDumpPassRecoversReusedRecord` — stale record inside a GUID-named dump whose live-MFT slot was reused: recovered (fails before, passes after).
- `NtfsVolume.GuidDumpDestroyRules` — dump record with all run-STARTs claimed → dropped; run-start free + interior claimed → kept; `size ≤ 0x3ff` → dropped; folder records never dropped.
- `NtfsVolume.SizeZeroRescueFromMftCopy` — deleted size-0 record with resident `$DATA` in an MFT copy: size/content recovered, "rescued" state set.

## Related findings

- Supersedes the scope of S-001 (resolved/) · refines I-001 (blanket-`$` is a pass-2 filter; pass 1 uses the metafile name table) · interacts with OQ-S2M-1 (grading locus) · N-024 (boot/stride/error-code minors from the same re-read)

## Open questions

- Engine config bytes +0x258/+0x259 (claim-skip / metafile-gate overrides) defaults are not fully decoded — tracked in OQ-S2M-1.
- The og's emit callback `vt[0x10]` receives items even when props claim failed (`plVar10==0` path at `:18485-90`) — consumer-side handling unknown (SaveScan/DRWUI side).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see git log for S-012)
**Verified by:** `NtfsVolume.MainWalkKeepsOverwrittenDeleted` + updated `NtfsVolume.EndToEndWalk` (membership: fully-overwritten deleted file emitted, graded Overwritten/PartiallyOverwritten, `destroyed_overwritten == 0` under default flags; the pre-fix tree destroyed it — the historical assertion `destroyed_overwritten == 1` is the fail-before witness); `NtfsVolume.GuidDumpPassRecoversReusedRecord`, `NtfsVolume.GuidDumpDestroyRules` (all-run-starts-claimed → destroyed; size ≤ 0x3ff → destroyed; folders never destroyed; unclaimed → emitted), `NtfsVolume.SizeZeroRescueFromMftCopy`, `NtfsVolume.SizeZeroRescueSameNameMerge` — 4/5 fail-before proven on the implementation-reverted build (stash/rebuild/run/pop), full suite 339/339 Linux default + 339/339 Linux ASan (334 + 5 net-new).
**Resolution notes:** Implemented the og's two-pass model end to end, re-verified first-hand against the decompile before coding: pass-1 membership restored (`destroy_fully_overwritten` default flipped to false — the S-001 scope error corrected; the flag remains as the opt-in honest-recovery inversion); during the walk a cluster-granular claim map accumulates every emitted record's extents (files claim $DATA runs, dirs $I30 runs — og FUN_180017190 at :18525, live AND deleted, engine+0x258 default off) and live `{*}{3808876b-c176-4e48-b7ae-04046e6cc752}`-named files (exactly 0x4C chars, '{' at [0], '}' at [37], magic GUID at [38..75] — the :18607-09 memcmp re-read) are collected. Pass 2 (`guid_dump_pass`, default on) walks each dump's $DATA runs in ≤0x8000-sector chunks with the og's stride discipline (0x400 after a successful parse, 512 after a hole — magic/fixup/og-15-table+hidden+system metafile gate/base-record-extension), dedups by the pseudo record number (absolute sector index), and applies the dump-pass destroy rules exactly: (dos_flags&4) || no name || '$'-prefix → destroyed; folders bypass; size ≤ 0x3ff → destroyed; else per-RUN viability — every non-sparse run's START cluster probed against the pass-1 claim map, all-claimed → destroyed; no runlist (resident/empty) → survives (og `puVar12 == 0` path). The size-0 rescue (`size_zero_rescue`, default on) harvests resident $DATA from the $MFTMirr copy at walk time (mirror-first `read_record_mirror`, fixup + record-number validated — og FUN_18001e120/FUN_18001dd80), skips compressed (0x800) records (og FUN_18001d690 entry gate), and retries still-empty records post-walk via the relaxed same-name merge (first same-name record at ≥ record# with non-zero size; extents/resident + size adopted; 5000-candidate bound — og FUN_18001d690 param_4=1). New stats: `guid_dump_files`, `guid_dump_records`, `guid_dump_destroyed`, `size_zero_rescued`. In-walk grading stays as the documented D15 superset (the og grades downstream — OQ-S2M-1). Two-pass model registered in docs/PARITY-DELTAS.md (D-51). Deliberate-difference notes: the claim map is cluster-granular (the og's is sector-granular — probe outcomes identical since run starts are cluster-aligned); dump records resolve parents against pass-1 emissions and land in the lost root when unresolved; the og's in-walk strict merge (record-ref/parent-matched candidates) is not replicated — the relaxed post-walk pass is the dominant og path (its in-walk map is empty at rescue time by construction).

### Session-4 convergence addendum (parallel fix, independently verified)

A second agent landed the same two-pass fix independently (convergent: membership flip +
GUID-dump pass 2 + claim map); the parallel implementation ALSO carries the size-0
rescue (mirror-first + relaxed same-name merge) and is the version that stands on main
(`5f1a212` + leak follow-ups `e7deda8`/`75050bf`). The duplicate implementation was
dropped at merge. First-hand corpus findings from the converging leg that the landed
code was re-verified against:

1. **No cross-pass dedup (premise correction):** the og's pass-2 emit path
   (LAB_18001afe6 :21264-21292) performs NO registry lookup against pass-1 — dump
   record numbers are synthetic (chunk sector base + offset/record_size, :21137-38),
   so a file present BOTH as a deleted live-MFT record AND inside a dump emits TWICE
   og-side. The landed code's dedup by pseudo record number dedups WITHIN pass 2 only
   (correct); duplicates across passes are kept (og parity).
2. **$Boot exclusion decoded:** the claim gate's 6-byte DAT_18004cd70 compare at
   :18514 is "$Boot" (raw DLL read at file offset 0x4b570) — already covered by the
   metafile name table; the landed claim-point placement (post-gate) is equivalent.
3. **Stride table:** gates fail -> record_size (0x200 fallback); parse-fail/destroy ->
   max(0x400, record_size); emit -> record_size (:21137 uVar19 init). The landed
   0x400/512 discipline matches on the standard record size.
4. **Two og defects deliberately not replicated (verified in the decompile, preserved
   as deltas):** the chunk scan bound `record_size x chunk_sectors` runs past the
   filled buffer for record_size > sector size (uninitialized-heap reads); the claim
   builder's engine[0x2c] count-vs-probe engine[0x28] indexing is inconsistent on
   multi-sector clusters. The landed cluster-granular map + read-sized buffer are the
   documented correct-side divergences (see PARITY-DELTAS registration).
