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

**Status:** FIXED (steps 1, 2, 4 — the two-pass architecture; step-3 rescue chain split precisely to `tracked-open/OQ-S4-ntfs-size0-rescue-chain.md` with this session's decoded corpus evidence)
**Fixed in commit:** (this commit)
**Verified by:** NtfsVolume.MainWalkKeepsOverwrittenDeletedDefaultFlags (fail-before: FAILED with the S-001 default destroy=true — the node vanished; pass-after: present, PartiallyOverwritten/Overwritten, opt-in flag still destroys 1), NtfsVolume.GuidDumpPassRecoversAndDestroysByOgRules (fail-before: FAILED with pass 2 disabled — 0 dump files, no stale records; pass-after: 5 dump records → 2 destroyed by the og rules [size ≤ 0x3ff; all run-STARTs claimed], 3 emitted [unclaimed start; intact<total; directory bypass]), updated NtfsVolume.EndToEndWalk (overwrit.dat now present under default flags — membership core⊃og restored); suite 353/353 Linux ASan (351 before + 2 new).
**Resolution notes:** Independently re-verified first-hand in the corpus before the fix: pass-1 unconditional emit (:18630 re-read), GUID collector gates (:18560-18625 — live-only state bit0, name len 0x4C, '{' head, trailing-0x26 memcmp against the `{3808876b-c176-4e48-b7ae-04046e6cc752}` template, '}' at [len-39]), pass-2 gates/destroy (FUN_18001acc0 :21152-21264 re-read line-by-line), claim-map build (FUN_180017190 :18323-410 — size-clipped loop, sparse runs skipped; claim gate :18523 with the $Boot DAT_18004cd70 raw-binary exclusion now read at file offset 0x4b570), and the stride table (uVar19 init = record_size at :21137; destroy/parse-fail = max(0x400, record_size); emit = record_size). ONE audit claim corrected: pass-2 record numbers are SYNTHETIC (chunk sector base + offset/record_size, :21137-38) and there is NO cross-pass dedup by record# (no registry lookup exists in the emit path LAB_18001afe6 — first-hand verified); the finding's "dedup against pass-1 records by record#" premise was wrong and is NOT implemented (the og emits duplicates when a record is in both the live MFT and a dump; the core now does the same). Implemented: (1) `destroy_fully_overwritten` default false — pass-1 membership parity, the flag stays as the honest-recovery opt-in; (2) the full pass-2 GUID-dump walk (`guid_dump_pass` flag, default on): live GUID-template file collection during the walk, cluster-interval claim map built from every emitted record's primary runlist (size-clipped, og FUN_180017190 semantics), chunked ≤0x8000-sector reads per dump run, og gate order (magic → fixup → metafile name-table → base-record → no-name → system/'$' → dir-bypass → `0x3ff < size` viability with per-run-START probes → destroy), synthetic record refs, phase-B parent resolution; (4) documented in docs/parity/PARITY-DELTAS.md. Two og defects deliberately NOT replicated (safety hardenings, see PARITY-DELTAS): the chunk-buffer scan overrun (`record_size × chunk_sectors` reads uninitialized heap for record_size > sector size) and the >512b-sector claim-map indexing ambiguity (engine+0x2c field: cluster count claimed as sector bits). The claim map uses cluster-granular intervals — exactly equivalent at the run-start probe point on 512b-sector geometries where the audit's behavioral claims live.
