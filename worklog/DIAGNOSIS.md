# DIAGNOSIS — session 2026-11-18 (result pipeline: streaming, detection, persistence)

> **Phase-0 complete before this diagnosis**: five context sheets committed
> and pushed (06987a83) before any code change — `worklog/PRODUCT-PIPELINE.md`
> (agent A), `worklog/REFERENCE-PIPELINE.md` (agent B),
> `worklog/REFERENCE-ALGORITHMS.md` (agent C), `worklog/REFERENCE-MODULES.md`
> (agent D), `worklog/PIPELINE-MATRIX.md` (agent E synthesis). Every claim
> below cites those sheets; they cite the corpus and product file:line. The
> PRIOR session's diagnosis (performance, 2026-11-18) is preserved verbatim
> at the bottom of this file — its bottleneck trains are landed and are NOT
> re-litigated here; this session's axis is the RESULT pipeline only.

## 0. The drop stage, quantified: the engine→UI event surface carries zero result rows

Stage-by-stage instrumentation of the product's result pipeline (PRODUCT-PIPELINE.md §1–6):

| # | Stage | Product evidence | Verdict |
|---|---|---|---|
| 1 | Sector read | `build_read_pipe` — RetryingDevice Skip-mode + 8 MiB LRU CachedReader (cr_scan.cpp:191); measured 544 MiB/s transport provision, 8.45 MiB/s deep-carve wall (prior diagnosis) | WORKS |
| 2 | FS recognition | `probe_family` content-verified (cr_scan.cpp:963) | WORKS |
| 3 | FS parse → detector | 8 FS engines parse MFT/FAT/exFAT/ext4/HFS+… records live during scan | WORKS |
| 4 | Detector emission | engines emit into shared `tree::ResultTree::add_file` (cr_node.cpp:188); carve stage adds 3-layer detection nodes live | WORKS — nodes exist during scan |
| 5 | Local persistence | CRSF chunked-deflate, atomic `.tmp`+rename checkpoints mid-scan (cr_crsf.cpp, cr_records.cpp) | WORKS — writes during scan |
| 6 | Engine→UI events | **12 `CR_EVT_*` types are aggregate counters only (permille/files/bytes/error ranges/lost candidates); 0 of them carry a result row.** `currentPath` hard null (events.rs:218) | ❌ **THE DROP STAGE** |
| 7 | UI result list | rows reach the UI only via `cr_ui_scan_snapshot` pull → bridge NodeMeta cache, invalidated at **10% progress buckets** (scan.ts:41) | STARVED |

**Conclusion — results are lost at stage 6, not in detection or persistence.**
Detection emits nodes live (stage 4) and the session store checkpoints them mid-scan
(stage 5), but the event channel transports **no result rows**; the UI compensates with a
full-snapshot pull refreshed only at 10%-bucket boundaries. Any volume whose scan completes
within one bucket (or whose permille stalls in a long carve phase — the 8.45 MiB/s wall)
shows an EMPTY result list for the entire scan: exactly the reported symptom "sector reading
produces nothing in the results".

Reference ground truth (corpus): the og pushes rows LIVE through its MOM→Qt bridge —
engine callbacks `(int,void*,int&)` → `CMessageProxy` → queued Qt bus → 15 subscribers'
`onReceiveData`; two-phase emission (quick-pass rows first, then advanced/carve rows into
separate branches, runtime-proven "Reconstructed" 1→16 live).
Citations: re-workspace/artifacts/MOM-modules/mom-modules--ui-bridge-and-dependency-matrix.md §1;
re-workspace/artifacts/UIUX/25-connect-wiring-graph.md §2/§3.1;
re-workspace/artifacts/UIUX/dynamic-ground-truth--scan-phase-captured.md §3/§7.

## 1. Secondary weaknesses (measured, same session)

1. **Grading coverage holes** (PRODUCT-PIPELINE.md §5): extension-in-table nodes skip twin
   detection entirely (S-058 inverted gate); deep-read grade runs only for index==0 non-twin
   nodes → recovery chance is unevenly computed across the tree.
2. **Tail-walk bound**: carver tail-paired formats fall back to header-only emission past
   8 MiB; the og's BruteForce walks to last-valid EOF with a 200 MiB bound
   (REFERENCE-ALGORITHMS.md §2/§5).
3. **Duplicated filter semantics**: the Rust bridge re-implements filter/sort/paging over its
   own NodeMeta cache instead of engine `cr_ui_query_run` (results.rs:11–17) — two divergent
   implementations of the same contract, only one engine-tested.

## 2. Fix order (root-cause, not symptom)

1. **Phase 3 first** (the drop stage): add a row-carrying event surface — engine emits result
   rows during scan; bridge forwards as `scan-result:{id}` Tauri events; UI appends
   incrementally with bounded queue + backpressure + clean cancellation.
2. **Phase 2** (detection strength): close the grading coverage holes (S-058 gate, deep-read
   coverage); raise the tail-walk bound to the og's 200 MiB with chunked re-read.
3. **Phase 4**: CRSF persistence already matches the og's .rsf role and ADDS mid-scan
   checkpointing the og lacks (REFERENCE-PIPELINE.md §12–13) — polish only, no rewrite.

---

# DIAGNOSIS — session 2026-11-18 (performance: the I/O pipeline, threading model, queue depth, and CPU/memory limits)

> **Phase-0 complete before this diagnosis**: five context sheets committed
> and pushed before any code change — `worklog/PRODUCT-STATE.md` (agent A),
> `worklog/REFERENCE-ARCHITECTURE.md` (agent B), `worklog/REFERENCE-RAW.md`
> (agent C), `worklog/PLATFORM-PERF.md` (agent D), `worklog/BASELINE.md`
> (agent E, with new component microbenchmarks). Every claim below cites
> those sheets or the source tree; they cite the corpus and the platform
> docs. The PRIOR session's diagnosis (SMART/scan, 2026-11-17) is preserved
> verbatim at the bottom of this file; its functional fix trains are landed
> and are NOT re-litigated here — this session's axis is performance only.

## 0. The bottleneck, quantified: four layers, ranked by measured impact

**Layer 1 — the walls are validation-CPU-bound (the dominant end-to-end
number).** The deep-carve wall measured **8.45 MiB/s effective** (8 GiB in
1,014,235 ms, run 41140212296) while the transport's device rate at the
consumer's block size is **544 MiB/s** — the wall runs **~64× below the
device provision**. CPU/GiB = 303.8 CPU-s (4 threads) ≈ 76 s wall-equivalent
per GiB; the deepcarve wall is ~124 s/GiB — consistent with CPU-bound work
plus pipeline overheads (PERF-LEVERS §0, BASELINE §2.4). Every
transport-side micro-second saved is invisible against this wall UNLESS it
overlaps the validation CPU with the next reads (which is exactly what
read-ahead buys) or the CPU work itself shrinks.

**Layer 2 — one global mutex serializes every read through the pipe
(measured on ANY hardware, worst on the fan-out phases).**
`CachedReader::read_at` takes `std::mutex` for the WHOLE loop including the
transport fill (cr_cached.cpp:31, :61 — fill under the lock: one reader's
device round trip, ~10⁴–10⁶ ns, blocks all others). Consumers that queue on
this one lock:
- the scan engines during the sequential pass (cr_scan.cpp:1387 — one
  CachedReader 8 MiB/256 KiB/32 blocks for the whole pipe),
- the remnant/lost/carve fan-out workers — ThreadPool hw-1 clamped to 8
  (cr_scan.cpp:58-68; cr_carve.cpp:1631),
- the retry pipe's zero-fill reads and the carve windows (N-045 register).
**MEASURED serialization factor** (container microbench, page-cache-warm,
BASELINE §4.2): aggregate throughput **0.51× at N=2, 0.23× at N=4, 0.20× at
N=8 threads** — concurrency does not just fail to scale, it actively
degrades 2–5×. The og's contrast: its CRITICAL_SECTION guards only ring
bookkeeping; fills happen on the dedicated prefetch thread while consumers
memcpy under short critical sections (DRWUI_decompiled.c:962269/:962670/
:962776 — REFERENCE-RAW §12). On the CI runner (2 vCPU → fan-out resolves
to 1 worker, cr_scan.cpp:58-68) this cost is invisible; on every real
multi-core machine it is the first thing every worker hits.

**Layer 3 — the read path is QD1 with no read-ahead (the real-NVMe/HDD
exposure).** All data-path handles open WITHOUT `FILE_FLAG_OVERLAPPED`
(cr_transport.cpp:291-293 et al.); `overlapped_read`'s OVERLAPPED is an
offset holder and `GetOverlappedResult(...,TRUE)` blocks (cr_transport.cpp:53-80)
— **synchronous reads, queue depth 1 by construction, no IOCP anywhere in
production** (PRODUCT-STATE §1.1; QD>1 exists only in the bench harness's
separate overlapped ring, run-alignbench.ps1:115-126). Read sizes at the
pipe: 256 KiB cache blocks (cr_scan.cpp:1387); MFT records are fetched
per-record 1 KiB (cr_bootstrap.cpp:599-635 — sequential MFT rides the cache
at ~1 miss/256 KiB, but each miss is a full, un-overlapped device round
trip); carve windows 1 MiB+4 KiB (cr_carve.cpp:45-46); writer 1 MiB
(cr_safewriter.h:158). There is no prefetch/read-ahead engine (D-72/N-045:
reactive true-LRU only).
The og is ALSO synchronous QD1 (ReadFile with NULL OVERLAPPED,
Device_decompiled.c:55964-55965; zero IOCP machinery corpus-wide,
REFERENCE-RAW §1-3) — **its latency-hiding is chunk size × read-ahead**:
4 MiB device chunks (Device:56545), 7 MiB volume chunks (:56912), 2 MiB MFT
batches (NTFSSearchDLL:5649-5678), and the prefetch thread's 10-run
look-ahead ring of 4 MiB runs (DRWUI:919088-919142) — **~40 MiB of
in-flight read-ahead, effective QD~10**. Our engine: 1× 256 KiB in flight.
Per PLATFORM-PERF: QD1-sync on an Azure-capped volume (~240 MiB/s,
QD-flat — PERFORMANCE-BASELINE §6) shows no gap; on real NVMe
(latency ~50–100 µs) the ceiling of 256 KiB-per-round-trip with per-read
lock+syscall+copy overhead lands far under the device's ~3 GB/s class, and
on HDD (5–15 ms rotational latency) QD1-no-prefetch is 2–4× under a
4 MiB-run prefetcher. **The runner cannot discriminate this — the exact
reason PERF-LEVERS deferred F1/F2. This session opens them with the
sliding-window implementation plus the honest measurement-debt register
(§5).**

**Layer 4 — buffer churn (measured, real but secondary).** Per-miss cache
block allocation: `std::vector<u8>::resize` 256 KiB ≈ **4.6 µs = 0.8× the
block-copy cost itself** (BASELINE §4.4 — every miss allocates fresh); the
transport's unaligned-request bounce is a per-thread resize (cr_transport.cpp:206);
carve windows and writer chunks allocate per window/chunk; **no buffer pool
exists anywhere in the read path** (PRODUCT-STATE §3). LRU lookup cost at
the production 32-block capacity: +85 ns/read — negligible (BASELINE §4.3).

## 1. The composed answer to "why is final-prod slow vs the reference"

On the only surface that measures walls end to end today (GH windows-2022
runner, 8 GiB 512e VHDX), the product is NOT slower on raw transport —
our 256 KiB leg beats the og's 4/7 MiB legs on the runner (241.64 vs
197.32/196.15 MiB/s, BASELINE §3 — the og pays for chunk size in syscall
count against a capped virtualized volume). The user-perceived slowness
composes from the layers above, in this order:

1. **Deep/carve walls are CPU-bound** (Layer 1) and the CPU work is not
   overlapped with I/O (Layer 3): the validators chew while NOTHING reads
   ahead — the og's 10-run ring keeps feeding them.
2. **Fan-out phases serialize on the cache mutex** (Layer 2): 8 workers,
   one lock, one fill at a time — measured 0.2–0.5× aggregate scaling;
   the og's workers hit a short bookkeeping critical section while a
   dedicated thread fills.
3. **On real hardware** (not measurable here — §5 debt): QD1-sync ×
   256 KiB leaves the device latency-exposed exactly where the og's
   read-ahead runway hides it; per-transport classes (NVMe QD4–32,
   HDD NCQ 32, USB BOT QD1) have no tuning surface at all — every read
   rides one hardcoded shape (REFERENCE-RAW §17: the og likewise tunes
   nothing per bus — parity today; the mandate requires the sweep).
4. **MFT walk record-granularity + no batching at the reader level**
   (Layer 3): the og issues 2 MiB batches; we issue 1 KiB calls that the
   cache coalesces only opportunistically (the sequential pass pays 4,096
   transitions/GiB vs the og's 256-512 — PERFORMANCE-BASELINE §5).

## 2. The parameter table — ours at HEAD vs the reference (decompiled)

| Parameter | final-prod (HEAD 6d2cefb9) | og (corpus) | Cite |
|---|---|---|---|
| Read model | sync (blocking GetOverlappedResult) | sync (NULL OVERLAPPED) | cr_transport.cpp:70 / Device:55964 |
| Queue depth | **1, no read-ahead** | 1 + **10-run×4 MiB prefetch ring (QD~10 eff.)** | PRODUCT-STATE §1 / DRWUI:919088-919142 |
| Handle flags | FILE_ATTRIBUTE_NORMAL (no OVERLAPPED/NO_BUFFERING) | same | cr_transport.cpp:291 / REFERENCE-RAW §4 |
| Device chunk | 256 KiB (cache block) | 4 MiB | cr_scan.cpp:1387 / Device:56545 |
| Volume chunk | 256 KiB | 7 MiB | cr_scan.cpp:1387 / Device:56912 |
| MFT batch | 1 KiB per record (cache-coalesced) | 2 MiB (0x1000 sectors/call) | cr_bootstrap.cpp:599 / NTFSSearchDLL:5649-5678 |
| Carve window | 1 MiB + 4 KiB | 4 MiB + 0x1000 tail, double buffer | cr_carve.cpp:45 / RAWSearchDLL:9271-9296 |
| Cache | 8 MiB, 32×256 KiB, reactive LRU, one global mutex, fill under lock | 4 MiB budget, CRITICAL_SECTION bookkeeping only, dedicated prefetch thread | cr_cached.cpp:31 / DRWUI:962269-962776 |
| Fan-out threads | hw-1 clamp 8, ThreadPool; scan on caller thread | per-worker FAT threads; engine threads core-pinned (SetThreadAffinityMask) | cr_scan.cpp:58-68 / DRWUI:388833-388836 |
| Buffers | per-miss/per-window/per-chunk allocs; no pool | plain heap, ring-owned | PRODUCT-STATE §3 / REFERENCE-RAW §18 |
| Alignment | 64 KiB envelope, both transports; physical sector probed (StorageAccessAlignmentProperty) | sector-aligned by construction (offset×sectorSize, truncation) | cr_directalign.h / cr_transport.cpp:105-127 / Device:55936-55968 |
| Per-transport tuning | none | none (bus type only a flag) | PRODUCT-STATE / REFERENCE-RAW §17 |

## 3. Ruled OUT by measurement (do not spend this session here)

- **The 64 KiB envelope is correct and mandated** — GAP-S6-13's slow-path
  property + sector-multiple subsumption (cr_transport.cpp:152-190); F4's
  sector-granular envelope was already dispositioned a behavior risk with
  nil gain.
- **The transport is not broken at QD1 large reads**: 544 MiB/s at 1 MiB on
  the runner VHDX; open latency 9-10 µs (−41% vs driver era); self-healing
  reopen is correctness-only (PRODUCT-STATE §1).
- **LRU lookup, checkpoint, cheap-op API**: +85 ns/read lookup; save_session
  9.3 ms; cr_ui cheap ops 10.7-166.3 ns (BASELINE §2/§4).
- **The result-tree global mutex (cr_node.h:656)**: the query path walks
  lock-free under the ABI single-thread + G-02 snapshot discipline; mutation
  serialization is a real register item on multi-core but the walls say the
  CPU spend is validation, not tree emission — it goes on the
  contention-profile register (§5), not the fix frontier, until profiled.
- **QueryPerf**: 1M-node filter+count 26 ms / page 64 ms — green (BASELINE §4).

## 4. Fix plan (Phase 2, acceptance measurement per fix)

1. **Overlapped read-ahead ring (the og's model, generalized)** — a
   PrefetchReader layer: `FILE_FLAG_OVERLAPPED` data-path handles, a
   sliding window of N outstanding 4-16 MiB overlapped reads completed by a
   dedicated completion thread (IOCP-shaped; CreateThreadpoolIo is
   Win7-safe per PLATFORM-PERF §2), pooled VirtualAlloc buffers served to
   the caller under short bookkeeping locks. ABI surface UNCHANGED:
   `read_at` stays a synchronous call per the contract (ABI-CONTRACT §3);
   the concurrency lives INSIDE the reader stack — the single-flight
   clause holds at the public boundary, with its in-tree rationale
   (unique-OVERLAPPED legality) documented in the amendment.
   Acceptance: full unit suite green + byte-identical scan output on the
   Linux lane and the CI VHDX fixtures; reader-level component bench ≥
   QD1-sync throughput on the container; engine-ci green; wall deltas
   (quick/deep/deepcarve + TTFR + cancel_to_stop_ms) captured per dispatch.
2. **CachedReader fill-outside-lock + per-block in-flight dedup** — misses
   fill concurrently (each under its own block state), hits stay
   short-critical-section; blocks pooled. Acceptance: the N-thread
   aggregate scaling factor measured ≥1.0× where today it is 0.2-0.5×
   (BASELINE §4.2 rerun); no output change (hit/miss counts may change —
   telemetry only).
3. **Read sizing at the pipe**: block size sweep {256K, 512K, 1M, 2M, 4M}
   × the new read-ahead window {4M, 8M, 16M} × QD {1,2,4,8} — runner
   numbers recorded (capped-surface truth) + component-bench numbers; the
   og-parity default (4 MiB runs) is the floor, the mandate's 16 MiB class
   the sweep top. MFT batching at the reader level rides the ring (the
   per-record 1 KiB calls become ring hits).
4. **The sweep harness + the missing bench lane**: extend
   run-alignbench.ps1's QD list (:316) to 16/32 and chunk list to 16 MiB;
   re-wire the bench dispatch workflow (test-benchmark.yml is absent from
   root .github/workflows at HEAD — BENCHMARK.md §1 records its removal;
   re-add as a dispatchable lane) — every future number lands in CI
   artifacts, not prose.
5. **CPU-parallel validation (attacks Layer 1)**: the carve/deep CPU spend
   (303.8 CPU-s/GiB) in the signature matcher + validators — batching,
   early-exit order, and per-record parallelism on the existing ThreadPool;
   output-identity gated by the exact-result contract tests (carve registry)
   + the 1 MiB-block SHA-256 hash-map ground truth. Acceptance: deepcarve
   wall reduction on the same 8 GiB fixture with byte-identical trees and
   recover-all reports.
6. **Alignment on the overlapped path**: pooled VirtualAlloc buffers
   (page-aligned ⇒ sector-aligned; physical-sector probed already at
   cr_transport.cpp:105-127) + NO_BUFFERING evaluation on the ring handles
   (the runner leg exists in alignbench — `nobuf` flag) with the
   cache-pollution rationale recorded either way.
7. **Cancellation on the overlapped path**: CancelIoEx per pending
   OVERLAPPED + the existing cooperative checkpoints (transports never
   poll — cr_device.h:144-148; measured stop 282.6 ms stands as the bar
   to meet or beat).

## 5. The measurement-debt register (precise scope, per BASELINE §5)

NOT MEASURED on any available surface: per-transport QD/read-size optima on
physical NVMe/SATA/HDD/USB/SD; the ~3.5× 64 KiB-multiple slow-path ratio on
real NVMe (Azure shows 1.03× — virtualized); QD>1 payoff on physical media;
cancel latency on slow media. The procedures are staged in BASELINE §5
(alignbench sweep on physical media; Run-Benchmark.ps1 matrix). Every claim
this session makes about those classes will say "debt" or cite the runner's
capped-surface truth — never project.

---
---

# PRIOR SESSION'S DIAGNOSIS (2026-11-17, SMART/scan-functional) — preserved verbatim below
# DIAGNOSIS — session 2026-11-17 (SMART rebuild, scan pipeline repair, reference parity)

> **This session's Phase-0 is complete before this diagnosis**: five context
> sheets, committed and pushed before any code change — `worklog/PRODUCT-STATE.md`
> (agent A), `worklog/REFERENCE-SMART.md` (agent B), `worklog/REFERENCE-SCAN.md`
> (agent C), `worklog/REFERENCE-FOOTPRINT.md` (agent D), `worklog/VERIFICATION.md`
> (agent E). Every claim below cites those sheets; they cite the corpus and the
> source tree. The PRIOR session's diagnosis (2026-11-06) is preserved verbatim
> at the bottom of this file — most of its fix trains have since landed on main
> (ABI re-pin ba2b9347, scan-truth 693d4d60, surface-truth eb4bb5f3) and its
> strata are carried forward into §0 below.

## 0. Three defect strata, not one

**Stratum 1 — the user's installed build (v0.9.1, tag cdf66ff8, 2026-11-06).**
Every symptom the user reported is explained by defects that are FIXED on main
but were NEVER RELEASED (no tag after v0.9.1; 26 commits unreleased):
D-NVMe-01 (mis-packed NVMe query → only the first SATA disk had health),
D-ENUM-02 (shifted bus table), F-HLT-02 (UI-invented `healthScore` — the exact
"fake green percentage"), BF-31 (kind 'sd' never fired → hard-coded fake SD
card), D4f (the 605 s carve freeze), P-DEEP-01/P-HB-01/P-TICK-01 (deep stage
had no byte axis; heartbeat swallowed). PRODUCT-STATE §1.2, §7.1, §7.2.

**Stratum 2 — the current tree's residual breakage.** The ABI drift
(BF-ABI-01) is fixed in-tree (ba2b9347 + the bridge-integration layout gate),
BUT the integration lane itself is RED at HEAD: `it/tests/bridge_integration.rs:461`
assigns integer `0` to the `f64 frozen_run_s` (E0308) — commit e119cc71 fixed
only the declaration at :454. `ui-ci` clippy-strict is RED on the lib surface
(run 43108277889). `engine-ci` last ran (and failed) at 3415c71b — the
deliberate RED Phase-1 reproduction of the prior session; it has not run since
because no engine/** path was touched. Main is not green at HEAD. PRODUCT-STATE §5.

**Stratum 3 — registered divergences this session REVERSES (the structural
"works nothing like the reference").** Two rows of docs/PARITY-DELTAS.md that
recorded deliberate divergences are now mandated to match the reference:
- **D-P4**: ours = in-app `/health` page showing ONE selected disk at a time;
  the reference = a SEPARATE disk-health surface (DiskHealth.exe, Qt 5.15.2 QML,
  840×590 frameless, "EaseUS DiskHealth", drive selector "Which disk do you care
  about?", per-disk 4-state grade, ID/Attribute/Current/Threshold/RawValue/ST
  table, temperature card, tray-resident, launched from the home "Disk Health"
  card with `diskHealthIndex=<n>`). REFERENCE-SMART §1, §10; VERIFICATION V3/V4.
- **D-P6**: ours = one global 40/35/25-weighted curve (engine
  cr_scan.h:72-76 — location corrected by VERIFICATION V2; NOT contract.ts);
  the reference = per-phase 0-100% restart ("Quick Scanning %1, Files Found:"
  → "Advanced Scanning%1, Files Found:", gauge restarts at the phase boundary).
  REFERENCE-SCAN §1-§5; VERIFICATION V5 (byte-exact templates).
Plus two missing transports (below) and the unreleased-fix release gap.

## 1. Every reported defect → evidenced root cause → stratum → phase

| ID | Symptom (user words) | Root cause (evidenced) | Stratum | Fixed by |
|---|---|---|---|---|
| R1 | "reports only one disk" | D-NVMe-01: every real NVMe controller refused the mis-packed query → health only for the first SATA disk (cr_health.cpp:277-288); D-ENUM-02 misgrouping; ABI drift row-collapse (fixed ba2b9347) | 1+2 | RELEASE the train (Phase 4); the fixes exist in-tree |
| R2 | "fake green percentage with no attributes" | v0.9.1 UI computed `healthScore = max(88, 100-wear)` (F-HLT-02, health-page.tsx:100-107 pre-fix); NVMe disks have no ATA table and pre-NG-6/9 rendered empty; ex2 threshold axis unbound pre-ba2b9347 | 1 | RELEASE; current tree computes NO percentage anywhere (engine grades 4-state only, REFERENCE-SMART §4 parity) |
| R3 | "sometimes shows for the SD card" | kind 'sd' never fired pre-ba2b9347 (BF-31); SD page rendered a hard-coded fake card; reference: SD = no transport, stays in list, grades Unknown (gray) — never fake-green | 1 | RELEASE + Phase 1 truthful Unknown state in the new surface |
| R4 | "works nothing like the reference" (SMART) | D-P4 structural divergence (one-disk in-app page vs separate all-disks surface) + missing transports: SAT via \\.\SCSI%d: ABSENT, WMI MSStorageDriver_FailurePredict{Data,Thresholds} fallback ABSENT (comment-only cr_health.h:21), USB VID/PID quirk ladder ABSENT, SMART_GET_VERSION never called | 3 | Phase 1: separate disk-health surface + SAT + WMI fallback + truthful not-supported |
| R5 | "jumps directly to 30–75% advanced scanning" | exact math of the global 40/35/25 curve: quick (40%) + deep (35%) complete in seconds, so the first sample lands at 75.0% (carve boundary); "advanced" phase label spans deep+carve (contract.ts ScanPhase) | 3 (D-P6) | Phase 2: per-phase 0-100% restart, quick phase rendered with its own gauge |
| R6 | "never shows a quick-scan phase" | quick is one metadata pass finishing in seconds; no distinct quick-phase presentation exists; reference shows "Quick Scanning %1, Files Found:" with its own 0-100% gauge (strings @0001d7d6) | 3 | Phase 2 |
| R7 | "stalls in sector reading" / "very slow" | D4f 605 s freeze (registry mutex 2×/512B stride × 134M strides; fixed 0e87689d UNRELEASED) + pre-693d4d60 no deep byte axis + swallowed heartbeat; residual wall: BF-30 carve matcher 8.45 MiB/s | 1 (+perf) | RELEASE + Phase 2 bounded perf work on BF-30 |
| R8 | "produces no real results" | scan never passed the frozen carve window on the user's build; results surface only after stages emit; read-truth gate (post-fix) refuses fully-blind scans | 1 | RELEASE; Phase 2 validation on seeded fixture |
| R9 | "filters do not work" / "images, documents, archives are not found" | HYPOTHESIS (high confidence, needs runtime evidence): filters are bridge-side over the NodeMeta cache (results.rs, D-B3); carve populates those categories; the stalled scan left the cache empty — the filter is not dead, it has no data | 1 | RELEASE + Phase 2 per-filter validation against the seeded manifest |
| R10 | "deleted directory structure is not reconstructed" | engine DOES reconstruct (MFT parent-chain emit_parents, cr_mftwalk.cpp:1033-1098) — the user's scan never completed; reference model = 3 roots (existing / File Path Lost / Reconstructed) + viability gate 0x3ff strict + orphans grouped | 1 | RELEASE + Phase 2 deleted-tree validation vs expected tree |
| R11 | "product ships far fewer files than og" | 8 files / 8.2 MiB vs reference live tree 1,276 files / 494.7 MiB — registered consolidation (everything claimed ships inside cr_ui.dll) + REAL functional gaps: preview stack, repair workbench (G-1(b)), partition-apply UI, diskhealth companion content; rest is out-of-scope (WinPE 175.6 MiB, cloud/telemetry, 37 language packs, 200 UCRT shims) | registered | Phase 3: close functional gaps, document consolidations in FOOTPRINT-DELTA.md |

## 2. Verification corrections folded into the plan (agent E)

- Weights 40/35/25 live in `engine/include/cr/orchestrator/cr_scan.h:72-76` (+ bridge comment scan.rs:6) — PRODUCT-STATE's contract.ts:177-184 location was PARTIAL; that area holds ScanStage/ScanPhase types. (V2)
- Status-bar byte-exact templates: `Quick Scanning%1, Files Found:` / `Advanced Scanning%1, Files Found:` (no space before comma), `Reading Sector:` (capital S), `Estimated time remaining: %1s` (not "Remaining time"). Phase 2 must render these byte-exact. (V5)
- The reference's WMI fallback is `MSStorageDriver_FailurePredict{Data,Thresholds}` from root\WMI (VERIFIED in DiskHealth.exe UTF-16 strings) — NOT MSFT_PhysicalDisk. Phase 1 implements the reference's route. (V10-resolution)
- USB quirk ladder: 23 distinct `AddDisk - USB*` strings (not 28); the vendor NVMe route NAMES are verified but their IOCTL payloads are UNVERIFIED — we implement the generic STORAGE_QUERY NVMe route (as today) + SAT, and do NOT fabricate vendor routes. (V9a/V9b)
- RAWSearchDLL does NO device I/O (no CreateFile/DeviceIoControl imports) — carving rides the caller's buffers; our architecture (engine reads, carve analyses windows) matches. (V5)
- 840×590 verified via extracted main.qml (`width: blur === 0?840:856`). (V3/V4)

## 3. The plan (this session's phases)

**Phase 0 — DONE**: five sheets pushed (this commit train). No code change
before them (mandate honored).

**Phase 0.5 — make main green again (precondition for every gate):**
fix `frozen_run_s = 0;` → `0.0` (E0308), fix the ui-ci clippy findings (fetch
the run log — no cargo in this sandbox, CI is the only Rust gate), push, all
lanes green. Evidence: workflow run links.

**Phase 1 — SMART rebuild (T5/T6):**
1. Engine: add SAT transport (ATA pass-through via IOCTL_SCSI_PASS_THROUGH on
   \\.\SCSI%d:, the reference's SAT route) for USB-bridged disks; add the WMI
   fallback (MSStorageDriver_FailurePredict{Data,Thresholds}, root\WMI) as the
   last-resort attribute source; SMART_GET_VERSION support check on the legacy
   path; truthful per-disk "not supported" → grade Unknown with the reason.
   NO vendor NVMe routes (payloads unverified — recorded as DEFERRED in
   PARITY-DELTAS with precise scope). NO fabricated values anywhere.
2. Surface: the D-P4 REVERSAL — a separate disk-health window (Tauri
   WebviewWindow) matching the reference model: all-disks selector, per-disk
   4-state grade (Unknown/Good/Caution/Bad, no percentage), SMART table
   ID/Attribute/Current/Worst/Threshold/Raw/Status, temperature card, refresh
   (manual + interval), no-SMART disks stay listed with Unknown. Tray-residency
   and schtasks autostart: evaluated, likely DELIBERATE-DIVERGENCE (recorded
   with rationale) — the guardian/autostart behaviors are companion-app
   packaging, not health reading.
3. worklog/SMART-MATRIX.md: platform capability × product behavior per OS and
   transport (Win7/8.1/10/11 rows; NVMe protocol-query is Win10+; Win7 NVMe =
   truthful Unknown unless the vendor driver exposes it).

**Phase 2 — scan pipeline (T7–T12):** per-phase 0-100% progress (D-P6
reversal) with byte-exact status grammar; quick phase visibly rendered with
its own gauge; automatic quick→advanced transition (already ADR-008, keep);
filters validated per-dimension against the seeded manifest during AND after
scan; deleted-tree validation vs the expected tree on the seeded VHD fixture
(bridge-integration lane already attaches 2 VHDs and seeds — extend its
assertions); bounded BF-30 perf work (profile first, land safe wins, document
the wall honestly if structural).

**Phase 3 — preview + footprint (T13/T14):** close the preview gaps per
REFERENCE-FOOTPRINT §3.2 (bundled decode set, bounded workers, no console);
FOOTPRINT-DELTA.md reconciling every module; PARITY-DELTAS rows updated
(CLOSED / DELIBERATE-DIVERGENCE / DEFERRED with precise scope).

**Phase 4 — validation + release (T15–T20):** full CI green for the right
reason; forensic-repro run on the release candidate; release tag (0.9.3 per
the prior session's train — 0.9.2 was announced but never tagged; bump to
keep the train honest); installer exercised on a clean CI Windows machine;
worklog.md handoff with every SHA, run link, artifact, deferred item.

## 4. What is NOT broken (re-verified, do not re-break)

- Engine enumeration core (D3 keep-present, exact-size volume retry).
- The 4-state grading model already mirrors the og decoder (V8 verified
  byte-level: NVMe life=100−pct_used, >10 Good / =10 Caution / <10 Bad;
  critical_warning→Bad; attrs 194/184 never graded).
- Deleted-tree reconstruction exists engine-side (MFT parent chains).
- Privilege posture end-to-end (requireAdministrator manifest + D1 byte-scan).
- Contracts lane, ui-e2e lane green at HEAD.

---

# PRIOR SESSION'S DIAGNOSIS (2026-11-06, preserved verbatim)

# DIAGNOSIS — the consolidated Phase 0 picture (this session, T3)

> Six refreshed context sheets feed this document (REPO-STATE, UI-MAP,
> CORE-API, WIRING-AUDIT, REFERENCE-BEHAVIOR, INSTALLER-AND-PRIVILEGE).
> Every claim below is evidenced in those sheets with file:line. This is
> the entry-point diagnosis for Phase 1 reproduction.

## 0. The headline finding — TWO defect strata, not one

The user's machine and the current main tree carry DIFFERENT bugs that
produce the SAME symptoms:

**Stratum 1 — the user's installed build (v0.9.1, released Nov 6).** Its
symptom triad is fully explained by three defects that are FIXED on main
but were NEVER RELEASED (no v0.9.2 tag exists; the newest tag is v0.9.1
at cdf66ff8; RELEASE-RUN.md ends at v0.9.1):

- D-ENUM-02: the bus-type text table was shifted by one from bus 11 up —
  every SATA disk decoded `removable` via the mislabeled bus and landed in
  the wrong home-page group ("not all of my disks are visible" — they are
  present but misclassified/misgrouped, and the SD page can never match).
- D-NVMe-01: the NVMe health query's `pack(1)` buffer placed protocol
  data at offset 9 instead of 8 (`offsetof(STORAGE_PROPERTY_QUERY,
  AdditionalParameters)`) — every real NVMe controller refused the query,
  so health data existed only for the first (SATA) disk. "as I switch to
  other it shows nothing" — the exact D-NVMe-01 signature.
- D4f: the carve's user-sign probe took a registry mutex twice per
  512-byte stride over ~134M zero strides — a 605-second all-axes freeze
  ("stuck in sector scanning"). Fixed lock-free at 0e87689d, validated
  engine-side, never shipped.

**Stratum 2 — the current tree (post-eb4bb5f3, unreleased 0.9.2 state).**
The engine-side fixes above ARE in-tree, but commit eb4bb5f3 appended
fields to two frozen-grammar C structs (`cr_ui_disk_info.bus_type`,
`cr_ui_smart_report.power_cycles_nvme`, `cr_ui_smart_attr.threshold`) and
THE RUST BRIDGE WAS NEVER RE-PINNED. The bridge's abi.rs still asserts
"v4 carries NO bus_type" (abi.rs:184-186) — a comment that documents the
world BEFORE the append. This creates memory-corruption-class defects
that are WORSE than the user's current symptoms:

- `CrUiDiskInfo` is 296 B in Rust vs 304 B in the engine → the
  two-call enumeration (util.rs:156-171) allocates a `Vec` with the Rust
  stride while the engine writes with the C stride: an 8×N-byte heap
  overflow AND every row ≥1 decodes shifted — all rows collapse toward
  garbage `disk-0` duplicates (React key collapse drops them).
- `CrUiSmartReport` is 224 B in Rust vs 240 B in the engine → the
  engine's `memset(rep, 0, sizeof(*rep))` (cr_ui.cpp:4892) overwrites 16
  bytes of the caller's stack on EVERY health read (health.rs:32 local).
- `bus_type` is never consumed → `kind` can never be `'sd'` → the SD
  card page's `kind==='sd'` filter can never match (devices.rs:215).
- `cr_ui_health_read_ex2` (93rd export, the threshold axis + NVMe power
  cycles) exists but is unbound → the SMART threshold column is nulled.
- The startup ABI gate (lib.rs:98-107) only LOGS a version mismatch —
  a stale kit serves scrambled data without refusal.

**The systemic root cause behind Stratum 2:** no CI gate links the
bridge against the real engine. ui-ci runs `cargo check` (no link step);
the engine lane never touches Rust. A C header field append is
structurally invisible to every gate on the repo. This is the exact
class of seam the session prompt's "the thousand small seams" names.

## 1. The user's five reported defects — diagnosis table

| ID | Symptom (user words) | Stratum-1 cause (their build) | Stratum-2 cause (current tree) | Verdict |
|---|---|---|---|---|
| R1 | "on opening its not requesting or opening as administrator" | pre-0.9.0 build class (v0.8.x asInvoker) | manifest IS embedded (app.manifest:25 → build.rs:45-48 → D1 gate) | User's build predates 0.9.0 OR stale-bundle installer hazard (build.ps1:211-213 picks bundle by LastWriteTime) |
| R2 | "popup shells are appearing, probably cmd or powershell" | v0.8.x `_popen(fsutil)` per health read | 0 spawn sites lack CREATE_NO_WINDOW (all 3 audited: cr_health.cpp:393, cr_fsck.cpp:50, cr_bitlocker.cpp:64) | Same stale-build class; re-verify on clean install |
| R3 | "only one disk is appearing" (2 internal + 1 SD) | D-ENUM-02 SATA-mislabeled-removable + SD-never-'sd' | ABI stride drift → disk-0 row collapse (heap overflow) | BOTH strata must be fixed; release cut must carry both |
| R4 | "jumps directly to 75% advanced scanning, files found 21 (0 B), no sector progress, all stuck" | D4f 605s freeze + D4b-unshipped live bytes + deep phase has NO byte axis by design | bytes_scanned is carve-true only (cr_scan.cpp:510-518); bridge swallows the 5s heartbeat (events.rs:337-340) and only refreshes counters on file events | 75% is exact math (weights 40/35/25; quick+deep mid = 75%); "Advanced Scanning" spans deep+carve (contract.ts:171-176) |
| R5 | "restart as administrator... weird popup colour and background" | v0.8.x amber ElevationBanner (removed 0.9.0) | no elevation UI exists; dead `errors.elevationRequired` string (en.ts:543) + stale installer text (nsi:331) | Stale-build class; dead vocabulary to clean |

## 2. Beyond the reported five — the session's extended defect register

Refreshed from the sheets, ranked by user-visible severity:

- **BF-ABI-01 (NEW, this session): the bridge struct drift** — Stratum 2
  above. THE blocker: nothing may ship until abi.rs is re-pinned and a
  layout gate exists.
- **BF-ABI-02 (NEW): no real-bridge CI lane** — the missing gate that let
  BF-ABI-01 land. Fix: an integration job that builds the engine, links
  the bridge, and runs bridge-level tests against the real DLL
  (multi-disk where possible).
- **S-SD-01: SD cards are not a first-class surface** — kind 'sd' never
  emitted; the SD page renders a hard-coded fake card (en.ts:131-134,
  "58 GB/Disk2"). Reference: SD is a first-class sidebar page (atlas:143).
- **H-EX2-01: threshold column dead** — ex2 unbound; the reference's
  table renders ID/Attribute/Current/Worst/Threshold/Raw/Status
  (UIUX/19:26-28).
- **P-DEEP-01: deep phase has no byte axis** — "Reading sector: 0/N"
  frozen through quick+deep (the D4e coverage counter exists engine-side
  for the remnant sweep but does not stamp bytes_scanned for deep).
- **P-TICK-01 (BF-29): the "Searching:" ticker is dead** — currentPath
  always null (events.rs:193); reference shows a live path ticker
  (scan-phase-captured.md:56-107).
- **P-HB-01: the 5s heartbeat is swallowed** — events.rs:337-340
  (emit=false); the UI refreshes counters only on file events → any
  cadence gap reads as a hang.
- **F-REPAIR-01: the repair page's dropzone/Repair-All is local-state
  theater** — fake paths, no engine call (repair-page.tsx:121-124).
- **F-HLT-02: the health page synthesizes values** — wear '2%', '· 512B',
  hard-coded 512 B divisor, gauge computed UI-side from partial fields
  (health-page.tsx:100-106).
- **F-ACT-01: activation paste inserts a fake license key** — theater.
- **BF-30: the carve throughput wall** — 8.45 MiB/s (76-124 s/GiB):
  multi-TB scans take hours-to-days even with every D4 fix. Performance
  defect, engine-side.
- **BF-32: scan-emitter teardown race** — latent UAF in events.rs/state.rs
  (recorded with a two-phase fix design; deliberately deferred by the
  prior session).
- **I-STALE-01: the installer lane's bundle pick is LastWriteTime-based**
  (build.ps1:211-213) — can wrap a stale app exe.
- **I-CLEAN-01: app.log survives uninstall** (nsi:537-541).
- **CI-STALE-01: forensic-repro.yml default tag is v0.8.4**;
  engine-ci.yml:60 carries a false workflow_call claim.

## 3. What is NOT broken (re-verified this session)

- The engine's enumeration core is now genuinely robust (D3 keep-present,
  N-071 share-blocked, E-01 exact-size volume retry — CORE-API.md §D).
- All 43 manifest commands are implemented; all 10 events emit↔listen with
  payload parity (W-01 held).
- The privilege posture as built is correct end to end (manifest → D1
  byte-scan gate → forensic gate → OS-refusal twin).
- The shipped connector is REAL (connector/index.ts:23-26 defaults to
  tauri; mock is dev/test-only).

## 4. The plan (Phase 1 → 5)

1. **Reproduce Stratum 2 on a real runner** (fail-before evidence for the
   ABI drift: bridge integration harness against the real DLL, multi-disk
   via VHD attach).
2. **Fix train A (correctness blockers):** re-pin abi.rs (bus_type,
   power_cycles_nvme, threshold), bind ex2, map bus_type→kind (sd!),
   refuse-on-ABI-mismatch, rebuild.
3. **Fix train B (scan truth):** deep-phase bytes_scanned, heartbeat
   emission, current-path ticker, live counter refresh on poll.
4. **Fix train C (surface truth):** SD page real, health page real (no
   synthesized wear/sector-size), thresholds, honest no-SMART state,
   repair/activation theater removed or made real, dead strings cleaned.
5. **The new gate:** engine+bridge integration CI lane (link + run
   against the real DLL; ABI layout manifest cross-check; multi-disk
   enumeration pin).
6. **BF-30 investigation** (bounded: profile the carve, land safe wins,
   document the wall honestly if structural).
7. **Release 0.9.3** (0.9.2 was announced but never tagged; the version
   files already carry 0.9.2 — bump to 0.9.3 to keep the train honest),
   full forensic validation, clean-install exercise, uninstall check.

The register is deliberately larger than the user's five: "everything
seems broken" is the standard, and the fake/theater surfaces are exactly
the "script kiddie slop" the user refuses.
