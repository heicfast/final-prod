# S3-REFS scratch report — ReFS deep pass + ADS gap filing (session 3)

Agent: S3-REFS · read-only · core HEAD `34b3441` · corpus HEAD `03ad46c`.
Deliverables written: 1 evidence file, 7 findings (S-015, M-015, M-016, N-029, N-030, N-031,
I-011), module append to `diff-to-fix/modules/refs.md`, this report.

## (a) What I read (coverage)

Corpus (`easeus-drw-research`):
- `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — deep pass. Line
  ranges actually read: 1–520, 521–1160, 1161–1820, 1821–2520, 2521–3080, 7059–7458,
  7740–8030, 8096–8555, 8556–9115, 9116–9735, 9736–10375, 10376–10985, 11440–11739,
  11740–12299, 12899–13608, 13609–13908, 15333–15472. Targeted reads + greps over the rest
  (vftable census, call-site census for every record processor, function index). Roughly
  75% of the 20,270 lines read directly; every capability-relevant function (imports,
  gates, walkers, record processors, runlist builders, emit path) read in full.
- `ReFSSearchDLL_functions.txt` (629 entries, full), `ReFSSearchDLL_imports.txt` (50, full).
- `re-workspace/artifacts/SearchDLL/refs-engine--deep-dive.md` (full),
  `refs-btrfs-zfs-engine-algorithms-v2.md` §1 (full), `remaining-engines--ext4-hfs-refs-btrfs-zfs.md`
  (ReFS + cross-links), `SaveScan/ifileproperties-part2--ext4-hfs-refs.md` (full),
  `re-workspace/artifacts/PartitionRecovery/mo-module-map.md` (RefsFileSystemAnalyser row).
- `architecture/05-scan-engine-architecture.md` (ReFS greps + context), CORPUS-ERRATA.md
  (ReFS greps — no ReFS-specific rows).

Core (`core_recovery` @ 34b3441):
- `src/core/refs/cr_refs_engine.cpp` (all 928 lines), `src/core/refs/cr_refs_detect.cpp`
  (all 84), `include/cr/refs/cr_refs_engine.h` (all 105), `include/cr/refs/cr_refs_detect.h`
  (referenced), `include/cr/tree/cr_node.h:44-74` (resident_data),
  `include/cr/recovery/cr_safewriter.h:80-112`, `include/cr/recovery/cr_report.h:39` (grep),
  `bench/seed/generate_seed.ps1:141,343` (grep — "ReFS has no named data streams" claim).
Audit context: `diff-to-fix/PROTOCOL.md`, `SEVERITY-SCALE.md`, `PARAMETERS.md`,
`modules/refs.md`, findings N-007/I-004, S-006 (avoided re-filing), M-007/N-011 (cross-refs),
`CROSS-REF-MATRIX.md` refs row (grep).

## (b) Findings filed (ID · title · 1-line gap · evidence type)

| ID | Title | Gap | Evidence |
|---|---|---|---|
| S-015 (SEVERE) | ReFS attribute-layer recovery gap | Core recovers only the first unnamed $DATA attribute (break at cr_refs_engine.cpp:759), drops resident/inline content (og CRefsResidentDataRunList FUN_18000e490:10410-10454 / FUN_18000ea30:10723-10768), drops indirect paged attributes (og FUN_18000cf30:9294-9505), drops non-first data attributes (og merge FUN_18000ba90:8354-8450) | Decompiled (strength 2) both sides |
| M-015 (MAJOR) | Deleted-recovery model divergence | Og deleted files come from a stale-MSB-page sweep of the OID-table area (FUN_180005330:2819-3054, records force-marked deleted :2939); core harvests row-flag 0x4 + ID2 only — sweep absent | Decompiled both sides |
| M-016 (MAJOR) | Object-identity keep-one + recycle-bin routing | Og dedups same-identity rows (keep-one, FUN_18000d970:9904-9966) and special-routes $R*/S-*/"System Volume Information" (+$I resident read); core emits every ROW_FILE row independently | Decompiled both sides |
| N-029 (MINOR) | Boot-geometry gate | Og bps {256..4096} + cluster pinned {4K,64K} (FUN_180012390:13544-46); core bps {512,4096} + any pow2 cluster (:369-374) | Decompiled both sides |
| N-030 (MINOR) | Run recognizer + container translation | Og flags 0x40 required/0x20 physical + container-map rewrite ((block/cluster>>1)-2, FUN_18000c790:8880-8895, FUN_180011e90:13244); core flag 0x10 only + never-populated top-nibble map (:42, :72-82) | Decompiled both sides |
| N-031 (MINOR) | Runlist granularity validation | Og discards cluster-inconsistent runlists (FUN_18000e020:10201-10281); core accepts any in-bounds runs | Decompiled both sides |
| I-011 (INFO) | Host-OS runtime gate | Og DLL refuses < Win8 host (FUN_1800010d0:37-49); core host-agnostic — deliberate, register in PARITY-DELTAS | Decompiled + import census |

Not filed (deliberately): report-only/detect-only concern — **resolved negative**: core
`scan_refs` is a full recovery engine (files+extents+deleted+orphans), no SEVERE
capability-mismatch on that axis; lost-partition probe wiring = S-006 (existing, other module);
core's ID2/row-flag harvest is a recovery-positive superset vs the og page dispatch (inventory
§7 note); "page+8" doc nit → inventory §1/§8 (log-only).

## (c) Parameters newly verified MATCH (fresh evidence this session)

- **C21** (fs code enum): og lost-partition probe sets `*param_4 = 10`
  (FUN_180010390:12125) = core `REFS_FS_KIND = 10` (cr_refs_engine.h:100) — new site beyond
  the factory-map claim.
- **C19** (engine-level): page-sig DWORDs + volume-signature check on every sig page
  (FUN_180011b50:13117-13146 ≙ cr_refs_engine.cpp:111-119).
- **A25** (chain): SB checkpoint pair at sb+0x28/+0x30 (FUN_180011770:12974-75), newer
  sequence wins (FUN_1800021e0:1029-1031) ≙ core :495-526; checkpoint body gates
  (FUN_180012650:13744-13805) ≙ core :546-602 (GAP-S6-3 re-confirmed at fresh lines).
- **A26**: record-type dispatch u16 key[0]/key[2] (FUN_1800032f0:1469-1487,
  FUN_18000b820/b840) ≙ core ROW markers 0x00010030/0x00020030/0x10 (grammar identity:
  0x30+subtype1 == 0x00010030 etc.).
- **X51** (I/O): og page cache keyed by first LCN (FUN_180011b50:13158,
  FUN_180003620:1616) ≙ core 256-entry cache (:98-126).
- **B10**: name decode UTF-16 key+4 → UTF-8 (FUN_18000c2b0:8673-8696, CP_UTF8 0xfde9) ≙ core
  :704-711.
- Prompt aliases scored: A1=B1, A2=B2, P47–P53=X47–X53 (see module append; X47-X50/X52
  UNVERIFIED this session, X51/X53 M).

## (d) UNVERIFIED residuals (precise question + next artifact)

1. **Complete-search open offset**: what does `FUN_180004bf0:2539` read at ctx+0x170
   (= `[+0xa0] − [+8]`, set at FUN_180002130:862)? Guesses: end-of-volume backup DBR.
   Next: trace the CSearchedPartitionInfo field layout via FUN_180003b30/FUN_1800012b0
   serializers + a truncated-volume fixture; cross-check DRWUI's partition-info writer.
2. **Container-map value semantics**: the og maps `((block/cluster)>>1)−2` → first-half OID
   of system-dir records (FUN_180002a60:1302-1342); how this composes on multi-container
   volumes is undecoded. Next: `ghidra-mo/RefsFileSystemAnalyser_decompiled.c`
   (CRefsAllocatorMgr / CRefsBlockRefTableProc / CRefsAppDataRunItem) — also the open
   question behind modules/refs.md B7's ".mo family covers ADS" claim.
3. **Row-flag 0x4 on real volumes**: does ReFS ever set index-entry flag 0x4 on delete (the
   core's deleted-harvest premise)? Next: Prade 2019 thesis table 4.38 semantics vs a
   real-volume dump (none in corpus — vendor QA scanned no ReFS volumes,
   ifileproperties-part2 §3); decides M-015 escalation to SEVERE.
4. **Run-flag polarity on real volumes**: do $DATA run entries carry 0x40+0x20 (physical)
   or the container-relative flavor? Next: real-volume comparative bench (extent hashes) —
   decides N-030's final shape (fix vs close-as-INFO + delete dead map).
5. **`FUN_1800089f0` (used-space/read-error map) internals**: marking call sites decoded;
   the map's consumer (grading? report?) is not. Next: FUN_1800089f0 body + DRWUI-side
   consumer in ghidra-core.
6. **Keep-one tie-break field names** (M-016): +0x43/+0x41/+0x2f/+0x42 semantics
   shape-decoded only. Next: FUN_18000ac40 body (:7513-7625) + two-row fixture.
7. **Perf X47/X48/X49/X50/X52**: no fixture → no executed throughput/latency/memory/CPU/
   scalability numbers for ReFS. Next: real ReFS volume bench lane (Windows runner) or a
   generated large synthetic ReFS image; compare scan wall-time + RSS vs og on the same host.

## (e) Verdicts appended (module files)

- `diff-to-fix/modules/refs.md` — appended "Session 3 deep pass — S3-REFS" section (B1/B2/B4/
  B5/B7/A25/A26/C19/C20/C21/X47-X53/Q70-adjacent rows, all marked **(S3)**) + session-3
  findings-index table (7 rows). No existing rows rewritten.
- No other module file touched (lost-partition side belongs to S-006's module; LDM untouched —
  no new LDM evidence surfaced in this DLL beyond what session 1/2 filed).

## Candidate divergences NOT filed (lead may assign follow-up IDs)

- `FUN_18000cbd0:9213-9247` special single-char root-name list + 0x600 tagging vs the core's
  fixed 0x600 root — functionally equivalent on single-root volumes; fold into M-015/M-016
  fix work if the bench shows root-identity issues.
- `FUN_180004ec0` already-scanned-volume dedup (complete-search overlap suppression) — core
  orchestrator-side concern (A29); check whether the orchestrator dedups overlapping
  candidates for ReFS before filing.
- Core `record_ref` = directory-table id vs og global file-id counter (D11 field semantics) —
  report/session consumers should be checked before filing.
