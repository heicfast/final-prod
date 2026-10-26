# MAJOR M-042 — Session-resume of lost partitions is blind: the og re-validates each persisted candidate against the disk and drops stale records; the core restores without re-reading

**Severity:** MAJOR
**Module:** session (lost-partition persistence semantics)
**Parameters affected:** P12 (session persistence format — candidate records), P31/P36 (lifecycle — resume semantics), P34 (checkpoint — frequency/gating), P35 (error recovery — stale-record handling), P13 (report schema — richer per-record fields)
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` — incremental per-hit append `FUN_180020a60` (call site scan loop `FUN_180021080` at :20409, lead-verified); clone-interrupt checkpoint `FUN_180025c60` (:23088-23121, call sites :20079-20082); per-engine reset `FUN_180025e30` (:23171-23215: DeleteFileW both files :23196-23198 + rebuild partinfo from live device `FUN_1800264e0` :23453-23552); readers `FUN_180023120` (:21241-21284, 0x18-byte partinfo records) / `FUN_180023280` (:21289-21332, 0x448-byte lostpart records); resume load `FUN_180021e90` (:20742-20744); **layout-change detection** `FUN_1800229c0` (:20998-21039: persisted partinfo vs fresh enumeration); **per-record re-validation** (:20748-20842: applied-flag `+0x14 != 0` → re-read the disk at the record's start LBA `+0x28` via the device vtable :20778, re-validate through `FUN_180021080`, **drop records whose {start `+0x28`, end `+0x30`, type `+8`} no longer match** — lead-verified at :20820-20835); record layouts (partinfo 24 B: start/size/MBR-type; lostpart 0x448 B: part-number, disk number, applied-flag, CSearchedPartitionInfo payload with start/end/bps/SPC/hidden/CHS-align; setters :502-559; extended-type end-LBA quirk `FUN_180002760` :1035-1045); the shipped artifacts `og-codebase/PartitionRecovery/lostpart.dat` (1,096 B) + `partinfo.dat` (24 B) validate the layout (start 0x800, size 0x0749cfc0, end 0x0749d7bf, type 7, SPC 8)
**Core references:** audited `7cf6754` — `src/core/orchestrator/cr_scan.cpp:1293-1310` (restore is **blind**: deserialize → push to `stats.lost_candidates`, no boot re-read, no drop of stale records; gate only `kind==1 && fs_kind<=BitLocker` G-23); save path `:1227-1238` (one REC_PARTITION_CONTEXT per candidate: start_lba/sector_count/fs_kind/score/kind — codec `cr_records.cpp:368-386`); mid-scan checkpoint `emit_checkpoint_` (:428-452) is **config-gated** (`cfg_.checkpoint`); candidate `evidence[]`/`size_from_device_end` not persisted (`include/cr/lost/cr_lostpart.h:56-66`); closest layout-identity artifact = single-volume identity card (`cr_records.cpp:307-310`), no whole-disk layout record
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og persists lost-partition findings to `lostpart.dat` (incrementally, once per found partition during the scan, plus an interrupt checkpoint) alongside a whole-disk layout snapshot `partinfo.dat`; on the next search it compares the snapshot against the freshly enumerated layout to detect "disk changed", then re-reads each persisted candidate's boot sector and **drops records that no longer match the disk**. The core persists candidates through its session format and restores them blindly — a resumed session on a disk whose layout changed surfaces stale candidates the og would have rejected, and the core's per-record payload is thinner (no bps/SPC/geometry evidence) than the og's.

## What the reference does

See the citations block; the full decode — record layouts, write/read
timing, reset semantics, re-validation identity triple {start, end, type},
and the shipped-file cross-validation — is in
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §D. Two design
points matter for parity: (1) the per-hit append is unconditional (crash
safety — a killed scan still leaves its found partitions on disk); (2) the
resume path re-validates against the CURRENT disk before surfacing anything,
so stale records never reach the UI.

## What our core does

The core's session round-trips candidates (G-23 gate, kind/fs_kind sanity)
but performs no disk re-validation on load, has no whole-disk layout
snapshot to compare against, checkpoints are config-gated rather than
unconditional, and the persisted record omits `evidence[]`,
`size_from_device_end`, and per-hit geometry (bps/SPC).

## Gap

Three falsifiable deltas: (1) resume after the disk layout changed → the
core surfaces stale candidates (og drops them — PRD:20825-20831 vs
cr_scan.cpp:1303-1308, no boot re-read); (2) a crash mid-scan with
checkpoints disabled leaves zero persisted candidates (og's per-hit append
persists every find — PRD:20409 unconditional vs cr_scan.cpp:429
config-gated); (3) a reloaded candidate lacks the geometry evidence the og
carries (SPC/bps/hidden sectors — needed to re-materialize without
re-probing).

## Impact

User-observable on resume: stale candidates point at sectors that may now
hold different data — materializing them yields wrong-volume scans (the
boot re-read would catch it). Crash-mid-scan loses found partitions in the
core where the og keeps them. No wrong FILE bytes in the normal path (the
materialize step reads the current disk), but a stale candidate wastes a
scan and can mislead. MAJOR, not SEVERE: the failure requires a layout
change between save and resume, and materialize() re-reads the disk anyway
(the damage is wasted work + wrong candidate listing, not silent wrong
recovery).

## Reproduction / verification

Corpus-derived with shipped-file cross-validation (the on-disk lostpart.dat
/ partinfo.dat bytes match the decoded field layout exactly — start 0x800,
size 0x0749cfc0, end = start+size−1 = 0x0749d7bf, MBR type 7, SPC 8).
Core side read first-hand (cr_scan.cpp:1293-1310). Execution: the resume
flow is testable in-suite — the fail-before test below is executable on
Linux. Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §D.

## Fix plan (for the engineering team)

1. On session load (cr_scan.cpp REC_PARTITION_CONTEXT case): for each
   restored candidate with a live target, re-read the boot sector at
   `start_lba` and re-run the probe family's classifier; drop records that
   no longer classify as the persisted `fs_kind` (mirror of the og's
   {start, end, type} identity — our equivalent is {start, fs_kind} since
   we persist start/count, and the count check rides the device size).
   Only for sessions whose target identity card matches the opened device.
2. Extend REC_PARTITION_CONTEXT append-only (bump the record version):
   add `bytes_per_sector`, `sectors_per_cluster`, `hidden_sectors`,
   `flags` (bit 0 = size_from_device_end) so materialize can rebuild
   geometry without re-probing; keep old sessions loadable.
3. Add a whole-disk layout record (REC_DISK_LAYOUT: the live table's
   entries at save time) emitted at end-of-scan and at checkpoints; on
   load, compare against the current table to set a `layout_changed`
   summary flag (the og's partinfo semantics; surfaced, not enforced).
4. Make the candidate checkpoint unconditional-per-hit OR document the
   gating as a deliberate delta (the og appends per find; recommend:
   piggyback on `emit_checkpoint_` but force a checkpoint when a new
   candidate is added).
5. Tests: `ResumeDropsStaleCandidateAfterRelayout` (save session, mutate
   fixture device, load, assert candidate gone), `ResumeKeepsValidCandidate`,
   `DiskLayoutRecordRoundTrip`, `LayoutChangedFlagged`, `CandidateGeometryRoundTrip`.

## Tests to add

The five named tests; `ResumeDropsStaleCandidateAfterRelayout` is the
fail-before proof (core today restores the stale record).

## Related findings

S-029 (apply — the og's applied-flag `+0x14` semantics tie into the apply
decision), N-049 (record-field richness — the informational residue of
this finding's step 2), M-042 pairs with G-23 (the existing restore gate).

## Open questions

The lostpart.dat `+0x440` field (0x100 on the shipped file) is not written
by the decoded writer — possibly a newer-build field; harmless to parity
since we do not interoperate with the og's files. The og's re-validation
read length (4 sectors per the call shape at PRD:20980) could not be pinned
exactly.

## Resolution

**Status:** FIXED
**Fixed in commit:** <this push — DFX20 session, M-042 landing>
**Verified by:** 6 tests in tests/unit/test_resume_revalidation.cpp: ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout (the finding's own fail-before proof — EXECUTED: with the re-validation gates temporarily disabled the test goes RED on both surfaces, restored -> green), ResumeKeepsValidCandidate, DiskLayoutRecordRoundTrip, LayoutChangedFlagged, CandidateGeometryRoundTrip, ArbiterStartOffDiskDropsAndUnknownKeeps; suite 685/685 linux-gcc (RelWithDebInfo + ASan).
**Resolution notes:** CONFIRMED-DIVERGENCE (all three falsifiable deltas reproduced by code read against the audited tree), fixed per the finding's 5-step plan. (1) Resume re-validation: `lost::revalidate_candidate` (cr_candidates.cpp — dispatches the SAME per-family probe the discovery uses, at the record's start LBA, through the CURRENT disk; drop on classification mismatch, on start-past-disk, and on persisted-BPB geometry mismatch when the record carries geometry; Unknown-family records kept — no probe to contradict them, the og's applied-flag skip shape). Execution points mirror the og's at-search-start timing: (a) `load_session_summary`/`load_session` gained a nullable `revalidate_disk` — the device-bearing listing surfaces (rrecov `cmd_recover` now opens the source disk BEFORE the load; `cmd_apply` opens once and re-validates the candidate listing, reusing the same view for plan_apply); (b) `run()` start — the resume flows (CLI --resume via `resume(..., revalidate_disk)`, the cr_ui scan-resume whose device attaches after load), guarded to whole-disk-basis resumes (volume-basis candidates would read against the wrong base; the og has no cross-basis case). (2) REC_PARTITION_CONTEXT v2 (append-only, length-detected like SessionState v2): bytes_per_sector/sectors_per_cluster/hidden_sectors/flags (bit 0 = size_from_device_end, bit 1 = geometry valid) — captured by probe_ntfs/probe_fat/probe_exfat at discovery (the BPB families; non-BPB formats stay zero like the og's), round-tripped through save/load; v1 sessions (25 B) load with extras zeroed and stay classification-only re-validatable. (3) REC_DISK_LAYOUT (type 8): the live whole-disk table captured at run() start, emitted at every save/checkpoint; on resume the freshly enumerated table is compared and `stats.layout_changed` is surfaced (CLI prints a WARNING; never enforced — the og's partinfo semantics, PRD:20998-21039); old sessions (no record) never flag. (4) The per-hit persistence delta: a checkpoint is now FORCED the moment discovery produces candidates (ahead of any stage boundary), still gated on a configured sink — the core has no module-local .dat side-channel, so a session-less run has nowhere to write: that residue is the honest architecture difference, registered in the Resolution and PARITY-DELTAS (session record, same push) rather than papered over. (5) All five planned tests landed under behavior-named IDs plus the arbiter-edge test. Safety hardenings preserved: nothing here touches S-029's consent-gated apply (the apply listing's re-validation only filters what is SHOWN — the same og behavior); G-23's kind/fs gate kept; N-058's load-validation deltas untouched. Docs: crsf-format.md (PARTITION_CONTEXT v2 + DISK_LAYOUT sections), the CLI resume warnings.
