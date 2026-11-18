# DIAGNOSIS — sector scan / detection / streaming: exact break points, quantified

Session: sector-scan-rebuild (2026-11-18). Tree at diagnosis: `976bced8` (+ concurrent lanes).
Sources: the five Phase 0 sheets in `worklog/session-sector-scan-rebuild/` (PRODUCT-PIPELINE,
REFERENCE-SCAN, REFERENCE-ALGORITHMS, MS-DOCS, REPRODUCTION), each claim cited there with
file:line / artifact:section / run IDs. This file quantifies WHERE the pipeline breaks, per
the Phase 1 instrumentation questions (does the reader read? do records reach the detector?
does the detector emit? do nodes reach the streamer? does the UI render?).

## Stage-by-stage verdict

| # | Stage | Verdict | Evidence (quantified) |
|---|---|---|---|
| 1 | Sector reader reads every sector | **WORKS** (post AsyncRing repair) | AsyncRing 8×4 MiB IOCP window + companion sync handle for out-of-window reads (cr_transport.cpp:318-746, :467-481); every sector reachable; bad regions zero-filled with ReadErrorMap truth (cr_retry.cpp:56-142). The ERR_NO_MEMORY miss-path (benchmark run 44275941738: api-recovery 102× over cap, bench-rand-1m 9427 ms vs 2192 ms) was root-caused and repaired by the concurrent installer-api lane — **Windows re-run not yet green** (see OPEN-1). |
| 2 | Geometry / total sector count | **WORKS** | StorageAccessAlignmentProperty physical/logical sector + [256,65536] sanity (cr_transport.cpp:153-175); sector_count CEIL form; DISK_GEOMETRY_EX + GET_LENGTH_INFO probes (cr_transport.cpp:140-175, cr_enumerate.cpp:32). Matches MS-DOCS rules 1-5. |
| 3 | Parsed records reach the detector | **WORKS** | Quick-stage measured 26-41k files/s (72.1 s on a real 274 GB volume); deleted-tree detection 81/81 nodes on the 6 GiB it-gate fixture (305 files, 3.02 GiB). |
| 4 | Detector emits nodes | **WORKS, but validation-throttled** | 154 grammar rows + registry gates emit correctly (cr_registry.cpp:1720-1760, cr_carve.cpp:672-789); the CARVE VALIDATION WALL is 8.45 MiB/s effective vs 544 MiB/s device (BF-30) — this is the dominant cost of every deep scan: 76-124 s/GiB of free space. |
| 5 | Nodes reach the streamer | **BROKEN — the drop stage** | **0 of 12 CR_EVT_\* types carry a result row** (cr_ui.h:151-297 — counters only). Rows reach the UI ONLY via snapshot pull, invalidated at 10% buckets (ui/src/queries/scan.ts:41-47) with a ≥1.5 s bridge poll (state.rs:628-677). During a scan the user sees aggregate counters, never results. This is the single root cause of "produces nothing a user would recognize as a real recovery result" DURING the scan. |
| 6 | UI renders what it receives | **WORKS (starved)** | results-explorer + scan-progress render correctly on snapshot pulls; found-count frozen at "197 items" and sector ticker EMPTY throughout the v0.8.4 forensic run (42255492260) because nothing fed them (stage 5), not because rendering failed. |
| 7 | Result tree correctness (phantom/system dominance) | **UNMEASURED — gate missing** | The seed manifest (seed-disks.ps1:242) is uploaded by forensic-repro but **never consumed by any gate**: no detection-rate, no phantom-rate, no classification-accuracy capture exists (REPRODUCTION §5). Bridge re-implements filtering in NodeMeta (results.rs:11-17) bypassing the engine's query contract — two filters, one truth. Grading covers only index==0 non-twin nodes (cr_grading.cpp) — most carved rows carry no recovery chance. |
| 8 | Progress percent | **BROKEN — phase axis decouples from I/O** | Weights quick(40)/deep(35)/carve(25) (cr_scan.h:72-76) vs stage order quick→carve→deep (cr_scan.cpp:1561, N-181). On structured volumes quick+deep finish in SECONDS (metadata passes), so the UI opens at exactly **75.0%** then crawls while the byte-denominated carve/remnant sweep grinds: **75.0→76.1% over 891 s** (run 42255492260; BF-22). The reference has NO phase-% weights (REFERENCE-SCAN: sector-linear model with X/Y ticker is the only evidenced form) — 40/35/25 is product lineage, and it is the wrong axis: percent must derive from bytes/sectors actually read. |
| 9 | Sector counter ("Reading sector X/Y") | **BROKEN — inconsistent axes** | Carve clamps `base+done` to Y; deep DIRECT sub-passes run UNCLAMPED (P-DEEP-01, cr_scan.cpp:545-565); events coalesce on permille-step OR 16 MiB (cr_scan.cpp:697-728). Result: the ticker can exceed Y, sit frozen, or sit empty while bytes flow (the v0.8.4 capture showed it empty for the whole run). |
| 10 | Cancellation / drain | **PARTIAL** | Cancel token observed only at intra-window checkpoints (D4c, ce71143e) — bounded but the drain window after a healthy exFAT scan stop was 180+ s (BF-25, run 42176600928); the 655 s remnant-sweep freeze class was fixed (62fb7670) and is gate-pinned (D-SCAN-01: no 180 s frozen stretch). |

## The three user-visible symptoms, mapped to root causes

1. **"Scan stalls"** = stage 8+4: the percent axis decouples from the byte-denominated work
   (75% plateau) × the 8.45 MiB/s carve validation wall. Movement fixes landed (D4a-d,
   ce71143e/62fb7670 — gates green on v0.9.1+); the THROUGHPUT wall itself is open (BF-30).
2. **"Shows only system files"** = stage 7's missing measurement + the classification surface:
   no manifest-vs-tree gate has ever run, so neither detection rate nor system-file dominance
   has a number. The classification model itself (FileFilter 8 categories, FilterFolder noise
   layer, $-metafile skip at walk time) exists engine-side and matches the reference model
   (REFERENCE-ALGORITHMS §6) — it is the EVIDENCE and the bridge-side filter split that are
   missing, not the category table.
3. **"Phantom non-existent files"** = stage 7: no phantom gate exists in the product's
   validation harness. The reference's phantom gates are documented (REFERENCE-ALGORITHMS §7:
   dual-generation validation, grammar-walk-with-end, size sanity, extent viability, name
   sanity, structured-wins merge); the product implements several (grammar rows, N-070
   best-score) but has NEVER MEASURED a phantom rate against the seed manifest, and grading
   coverage holes mean unvalidated rows can surface without a recovery-chance verdict.

## Fix plan (phases 2-4 entry points, from the sheets)

| Fix | Root cause addressed | Entry points |
|---|---|---|
| F-A Row-carrying event + during-scan streaming | stage 5 | cr_ui.h:151-297 (append-only), cr_ui.cpp sink dispatch, cr_node.cpp:188 add_file batch emitter, bridge abi.rs + events.rs:353 `scan-result:{id}`, contract.ts/tauri-connector.ts onScanResult, scan.ts incremental append. Bounded queue + backpressure in the bridge channel; cancel flushes partial rows. |
| F-B Byte-derived monotonic percent | stage 8 | cr_scan.cpp stage weighting → overall percent derived from bytes read vs device bytes (the reference's sector-linear model); keep stage tags for UI copy. Weights struct retired honestly (register the divergence). |
| F-C One sector axis | stage 9 | unify carve+deep on bytes-read-into-total-bytes; clamp at 100% only at true completion; coalesce on time (e.g. 200 ms) OR permille, not 16 MiB. |
| F-D Manifest-vs-results gate (the phantom/accuracy instrument) | stage 7 | new forensic step: dump results tree post-scan → compare vs seed-manifest.json (recall per category, deleted-flag recall, 0 unexplained rows = phantom rate). Ride forensic-repro S3. |
| F-E Bridge filter unification | stage 7 | delete NodeMeta re-filter (results.rs:11-17) → engine `cr_ui_query_run` one contract. |
| F-F Grading coverage | stage 7 | cr_grading.cpp: grade every surfaced node (not index==0 non-twin only). |
| F-G Carve throughput | stage 4 (wall) | BF-30 scope: profile the matcher; the tail-walk bound 8 MiB→og 200 MiB (cr_carve.cpp) rides with it. |

## OPEN items inherited at diagnosis time (not this session's breaks)

- OPEN-1: benchmark-src 44275941738's seven red gates — repaired by the concurrent
  installer-api lane (AsyncRing miss-path); Windows matrix re-run pending.
- OPEN-2: BF-32 emitter teardown race (two-phase teardown design sketched in register).
- OPEN-3: currentPath axis is sampled (M13) not a true path ticker (BF-29).

**No number in this file is invented**: every figure traces to a captured run ID, a
file:line, or a measured benchmark recorded in the five sheets. What this diagnosis cannot
yet prove — the manifest-accuracy numbers — is itself the finding (stage 7): the instrument
does not exist, and building it (F-D) is the first rebuild act of Phase 2/3.
