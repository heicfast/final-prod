# INFORMATIONAL I-046 — the og's lostpart.dat/partinfo.dat lifecycle decoded: NOT a cross-session knowledge base — a per-process-generation cache with a first-search delete latch, per-hit mid-scan appends, interrupt progress-markers, and a marker-restore resume path

**Severity:** INFORMATIONAL (deliberate-divergence register — the core's session-checkpoint persistence is the registered M-042 architecture; this finding registers the og's own lifecycle semantics, several surfaces decoded for the first time)
**Module:** lost-partition / session (persistence architecture)
**Parameters affected:** #36 Session lifecycle (save/load/resume semantics), #34 Checkpoint (frequency and recovery behavior), #58 Wiring (the persistence owner)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:23189-23211` (W1: the first-search-per-process latch — DeleteFileW BOTH .dat files, rebuild partinfo from the live table, set the latch), `:20154-20238` + `:20230` (W2: the per-found-candidate mid-scan append via FUN_180020a60), `:20079-20082` → `:23088-23121` (W3: the clone/search-interrupt progress-marker append via FUN_180025c60 — header-only, payload zeroed), `:20748-20770` (the marker-restore path: progress counter max-merge at +0xe4, scan-position LBA restore at +0xe8, candidate phase counter at +0xe0), `:23220-23271` / `:23276-23327` (the two append-at-EOF writers), `:20998-21039` (the layout comparator), `:21241-21284` / `:21289-21332` (the two readers)
**Core references:** `src/core/orchestrator/cr_scan.cpp:2468-2475` (the M-042 comment: "the core has no module-local .dat"), `:2666-2675` (the REC_DISK_LAYOUT emission — the core's partinfo-equivalent, riding the session file), `:2646-2665` (the REC_PARTITION_CONTEXT emission — the core's lostpart-equivalent), `:1085-1090` (the resume revalidation) — at audited core `9633d94f`
**Status:** READ-ONLY FINDING — registered divergence, no fix proposed

## Summary

First-hand decode of the og's on-disk lost-partition database lifecycle (the user's "local database as og-codebase" directive). The og's lostpart.dat/partinfo.dat pair is **not a cross-session accumulating knowledge base**: the first search of each process DELETES both files and rebuilds partinfo from the live partition table (a generation latch at pool+0xf0). Within a process lifetime, found candidates append per-hit (mid-scan), and a clone/search interrupt appends a header-only progress-marker record whose replay restores the progress counter and the scan-position LBA across process restarts. The core's architecture (M-042's landing) persists the same information classes through the session checkpoint instead — a deliberate, registered divergence; this finding registers the og-side lifecycle semantics that were never previously decoded (the W1 delete latch, the W3 marker shape, and the marker-restore path), so future parity tables stop describing the og's files as a persistent database.

## What the reference does

**W1 — the generation latch** (PRD:23189-23211): at search start (and phase-1 completion, PRD:20745/:20885), if the pool's first-search flag is clear: `DeleteFileW(lostpart.dat)`, `DeleteFileW(partinfo.dat)`, rebuild partinfo.dat from the CURRENT live table (FUN_1800264e0 → FUN_180025f90 per entry), set the latch. The shipped tree's two .dat files are therefore one process-generation old — the artifacts of the vendor's last QA run, not an accumulating history.

**W2 — the per-hit append** (PRD:20154-20238, the append call at :20230): each lost partition FOUND mid-scan appends one 0x448 candidate record (header + the CSearchedPartitionInfo image) at EOF — the checkpoint-on-discovery cadence.

**W3 — the interrupt marker** (PRD:20079-20082 → FUN_180025c60 at :23088-23121): on clone/search interrupt (cancelled OR state==4), a header-only record appends: `{magic, progress counter (pool+0xb4), disk number, min(cur,end) scan-position LBA, kind flag 0}` — payload zeroed. The kind flag 0 is what distinguishes markers from candidates at replay.

**The marker-restore path** (PRD:20748-20770): at load, marker records (flag 0) max-merge their counter into the pool's progress counter (+0xe4) and restore the scan-position LBA (+0xe8); candidate records (flag 1) re-enter the assembly through the {start, end, disk} dedup key (PRD:20825-20831). This is the og's intra-scan resume: cross-PROCESS restart resumes the lost-partition SEARCH at the persisted scan position, at sector granularity — finer than the core's stage-granular resume.

## What our core does

The M-042 architecture (verified at HEAD): no module-local .dat files; the session checkpoint carries REC_PARTITION_CONTEXT per candidate (with forced checkpoint on discovery — the W2 cadence equivalent), REC_DISK_LAYOUT for the whole-disk snapshot (the partinfo comparator equivalent, cr_scan.cpp:2666-2675 with the layout-changed flag), and resume revalidation through `revalidate_candidate` (cr_scan.cpp:1085-1090). The core's resume is stage-granular (the session's stage bits), not scan-position-granular; there is no first-search delete latch because there is no module-local state to reset.

## Gap

Deliberate and registered (M-042's own notes; the "sink-less residue" was registered with that landing). The NEW registrations this finding adds: (1) the og's database is per-process-generation (W1) — any future documentation calling it a persistent knowledge base is wrong; (2) the og's interrupt marker + restore path is a scan-position-granular resume capability the core's stage-granular model deliberately does not replicate; (3) the W3 marker's exact record shape (header-only, min(cur,end) LBA, kind flag 0) — the decode input for any future resume-fidelity comparison.

## Impact

None on the core's behavior (the divergence is registered and deliberate). The value is audit-trail truth: the corpus's own artifacts described the .dat pair only through string evidence (PartitionRecoveryD.md:37-38) — the lifecycle decode now exists with citations, and the resume-granularity delta is on the record as a known, accepted difference rather than an invisible one.

## Reproduction / verification

All four lifecycle sites read first-hand this session (the W1 latch with both DeleteFileW calls and the rebuild loop; the W2 append call in the found-handler context; the W3 marker builder with the min(cur,end) clamp and the zeroed payload; the marker-restore max-merge and LBA restore). The shipped samples hex-verified: partinfo.dat = one 0x18 live-table record; lostpart.dat = one 0x448 candidate record whose end-LBA validates arithmetically against the partinfo entry (2048 + 122,277,824 − 1 = 0x0749d7bf). Worksheets: `evidence/s55-lane-sheets/S55-D1-localdb.md` §a/§b (the full offset tables) + `evidence/s55-liveness.md`.

## Fix plan (for the engineering team)

None required (registered divergence). Optional follow-ups for the record: (1) a PARITY-DELTAS D-row referencing this finding for the resume-granularity delta; (2) if a future session wants scan-position-granular lost-partition resume, the W3 marker shape and the restore path are the decode inputs — cite this finding.

## Tests to add

None (register-only).

## Related findings

M-042 (FIXED — the session-checkpoint adoption architecture this registers against), N-203 (active — the resume round-trip's data-area drop, found in the same decode), M-137 (FIXED — the v3 presentation tail), N-131 (active — the parent-anchor persistence gap, same record family).

## Open questions

None.
