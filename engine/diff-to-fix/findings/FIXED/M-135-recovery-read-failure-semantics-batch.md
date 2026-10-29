# MAJOR M-135 — recovery read-failure semantics: the og truncates + numbers + logs each failed file and keeps the partial output; the core zero-fills silently and deletes failed items wholesale

**Severity:** MAJOR
**Module:** writer / recovery (file-reconstruction focus battery)
**Parameters affected:** #35 Error recovery; #2 Recovery correctness (partial files); #13 Report schema (failure causes)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:875684-875696` (read failure: `LogError("Failed to ReadSector! ")` + the three-parameter dump + progress skip + error 9 + `break` — the file stays partial on disk); `:874031-874241` (`FUN_140541070`, the per-item result recorder: the failure collection, the failed-counters, the node mark `+0x73 |= 1`, the `LogInfo("Failed to recover file: %s")`, the std::filesystem existence/size probe); `:874072-874076` (the settings-gated error-9 accounting skip); `:875666-875668` (the beyond-EOF clamp); `re-workspace/reversed/ui-resources/translations/english_qm.json:448,538,598,640,664,682` (the six numbered user-facing failure causes)
**Core references:** `src/core/recovery/cr_safewriter.cpp:498-503` (read failure → zero-fill + `had_unreadable` + continue), `:532-547` (commit + `Outcome::HashMismatch`), `:616-644` (per-item failure collection), `:440-444` + `include/cr/win/cr_destio.h:36-43` (failed items leave no file — `close()` deletes the temp on every unrenamed exit); `include/cr/recovery/cr_report.h:26-88` (numeric outcomes, no per-file cause taxonomy) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

When a recovered file's read fails mid-copy, the og's runlist path logs a three-parameter error line, skips progress forward, records error 9, **breaks** — and leaves the partial file on disk (up to the failing run), counted in the failed list with a numbered user-facing cause taxonomy; the tree node is marked failed at runtime. The core instead zero-fills the failed span, continues recovering the rest of the file, commits it as `Salvaged`, and reports numeric outcomes only; for the cases it does fail (e.g. `ERR_TRUNCATED`), its atomic temp+rename posture deletes the partial output entirely. The og's per-file failure marks and the six-cause user surface have no core analog.

## What the reference does

Read-failure, runlist path (verified first-hand `:875684-875696` this session):

```c
875684  LogError("Failed to ReadSector! ");
875686  LogError("m_s64StartSector:%I64d,\tpDataRuns64LCN:%I64d,\tm_nSectorPerCluster:%I64d", …);
875689  LogError("StartSector:%I64d,\tReadCount:%I64d,\tSectorTotalSize:%I64d", …);
875691  (**(code **)(*(longlong *)*plVar14 + 0x18))((longlong *)*plVar14, param_7, 0);  /* progress skip */
875692  local_2a8 = 9;                                   /* error 9 */
        …break;                                          /* stop THIS file, keep the partial */
```

The partial file remains on disk (no delete in the path); the per-item recorder `FUN_140541070` (`:874031-874241`) then: pushes the error into the failure collection (`FUN_140302570(obj+0x80, &err)`; counters at +0x58/+0x60), **marks the tree node failed — `*(byte*)(node+0x73) |= 1`** (`:874087`), fires the observer callback, logs `"Failed to recover file: %s"` (`:874129`), and probes the destination file's existence/size via `std::filesystem` (`:874131-874205`) with a missing-file observer. A settings gate can suppress error-9's accounting (`:874072-874076`). The user-facing surface is the numbered taxonomy (english_qm.json:448..682): *1. Insufficient storage space. 2. The recovery path is too long. 3. The file name contains garbage characters. 4. The file name is most likely lost. 5. The correct storage sector location cannot be found. 6. …exceeded the free quota.* — the recovery-result dialog's "%1 files / %2 failed" list. (Note the og is internally split: its *chunked* path (`:875045-875049`) ignores read returns and writes zeros — the two paths disagree with each other; the runlist path is the one carrying the explicit failure semantics.)

## What our core does

- Read failure mid-file: `cr_safewriter.cpp:498-503` — the chunk is zero-filled, `it.had_unreadable = true`, the loop **continues**; the item commits as `Outcome::Salvaged` (`:532-547`, `cr_report.h:34-37`). The core recovers the post-failure spans the og loses — but never produces the og's per-file failure record or partial-file truth; the zeroed spans are only visible in the report's numeric accounting.
- Whole-item failures (`ERR_TRUNCATED` etc.): `:440-444` + `cr_destio.h:36-43` — `close()` deletes the temp on every unrenamed exit, so failed items leave **no** destination file (the og's truncation keeps what was written).
- The report (`cr_report.h:26-88`) carries `Outcome` enums + counts; there is no per-file cause string, no numbered-cause mapping, no `Failed to ReadSector` log family, and no tree-node failure mark (the session reload cannot see prior failures).

## Gap

Stated falsifiably: a 1 MiB file whose second half is unreadable recovers in the og as a ~512 KiB partial file + a failed-list entry with cause 5 + a `Failed to ReadSector!` log trio + a runtime tree mark; in the core it recovers as a full 1 MiB file with a zeroed second half, committed Salvaged, no per-file log family, no numbered cause, and no node mark. A file the core refuses (metadata-inconsistent) leaves no file, where the og's truncated output would still exist.

## Impact

Users comparing recovery results against the og see different file sets and sizes under media errors (the classic bad-sector case): partial-but-present vs present-but-zeroed vs absent. The "which files failed and why" answer — the og's numbered list — has no core equivalent, so a CLI user cannot tell a zeroed span from real data without reading the JSON outcome vocabulary. MAJOR: behavioral difference under failure, not on the happy path; interacts with M-136 (the retry bypass makes transient errors look like permanent ones) and S-059 (the refusal path).

## Reproduction / verification

- **Decompiled evidence (first-hand):** the log trio + error 9 + break at `:875684-875696`; the recorder's collection/mark/log/probe at `:874072-874129` + `:874131-874241`; the numbered taxonomy in english_qm.json (six sites listed above). Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-reconstruction-export-chain.md`.
- **Core evidence (first-hand):** the zero-fill-and-continue at `:498-503`; the Salvaged commit at `:532-547`; the delete-on-fail at `cr_destio.h:36-43`; the report's outcome-only shape at `cr_report.h:26-88`.
- **Execution status:** UNVERIFIED-BY-EXECUTION for the og side (needs a Windows host with a damaged image); the core side is code-proven; the og's own real-run logs (vendor QA logs) were not mined for the `Failed to ReadSector` line this session — the fix lane should grep `og-codebase/log/` for it as corroboration.

## Fix plan (for the engineering team)

1. Decide the partial-file policy deliberately (recommended: keep the og's truncate-and-keep for **hard** read failures after retries, while keeping zero-fill for spans the og's chunked path itself zeroes — i.e. make the policy per failure class and document it): on a failed read after `M-136`'s retry pass, stop the item, commit what was written (rename the temp with a `.partial` suffix or a report flag rather than deleting), and record `Outcome::ReadError` with the span.
2. Add the per-file failure record to the report: `{path, outcome, cause_id, cause_text, bytes_written, first_failed_offset}` with a `cause_id` mapping the og's numbered list (the six strings) — engine-side enum + the CLI rendering the same numbering.
3. Add the `Failed to ReadSector!` log family (StartSector/LCN/SPC + StartSector/ReadCount/SectorTotalSize parameter trio) to the writer's log channel, matching the og's format strings.
4. Optionally persist a per-node failure bit in the session (the og's is runtime-only — `+0x73` bit 1 is not persisted per the rsf grammar; parity means runtime-only is sufficient — register the decision).
5. Tests (below). Acceptance: a fixture with a deliberately unreadable extent produces a partial committed file + a report row with cause 5 + the log trio; the og-shaped dialog data is derivable from the report.

## Tests to add

- `Writer.HardReadFailureTruncatesAndKeeps` (fails today: zero-fill, full size, Salvaged).
- `Writer.FailureCarriesNumberedCause` (fails today: outcome-only).
- `Writer.ReadSectorLogTriFormat` (new log family).
- `Writer.ReportPartialBytesWritten`

## Related findings

- **S-059** — the hole/refusal export semantics (the other side of the partial-content policy).
- **M-136** — the retry bypass (decides whether a failure is transient before this policy applies).
- **S-050** (active) — the repair line (the og's MOV/MP4 repair-during-recovery gate sits in the same export body; this finding's decode adds the extension-gate cites `:874502-874574` to S-050's evidence pool).

## Open questions

Which of the og's six numbered causes maps to error 9's internal code (the mapping lives in `FUN_1400ed760`, not decoded this session) — the fix lane needs it for the cause-id table; one read of that function resolves it.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — the S72 MAJOR-first lane's second landing)
**Verified by:** `Writer.HardReadFailureTruncatesAndKeeps` (RED: link-failure with the implementation stashed — the API is new; the pre-fix behavior was the zero-fill the finding names), `Writer.ReadErrorLogTriFormat` (the verbatim three-line trio + "Failed to recover file: %s" captured through the log sink), `Writer.BeyondVolumeReadRecordsCauseFourAndClampsStraddle` (the :875666-68 straddle clamp + cause 4), `Writer.ShortExtentMetadataCommitsPartial` (the ERR_TRUNCATED refusal replaced by the og's partial-keep, cause 7), `Writer.CompressedUndecodableBlockStillSalvages` (the per-class split: undecodable compressed blocks keep zero-fill + Salvaged), `Writer.FailureCauseTableMatchesOgRuntimeStrings` (the corrected SEVEN-entry table), `Report.ReadErrorOutcomeMarksTruncatedPartials` (the FB-R02 re-pin: outcome read_error, cause_id 5, first_failed_offset 2048, bytes_written 2048, failed_bytes 3072, JSON fields emitted), `Writer.BareDeviceTruncatesWhatThePipeRetries` (the M-136 negative control re-pinned to the new contrast); the two re-pinned tests' RED proven first-hand at the intermediate build (both FAILED asserting the old Salvaged-on-read-failure before the re-pin); full suite **1515/1515**.
**Resolution notes:** CONFIRMED-DIVERGENCE. The corpus re-verified and CORRECTED first-hand this session: (1) the cause list is SEVEN entries, not six — the compiled-in defaults at DRWUI strings_ascii.txt :30580-30596 (RecoveryFailedInfo1..7); the finding's qm citation missed cause 7 ("Other unknown reasons") and misread cause 4 — the qm's "4.The file name is most likely lost" is a search-help string, the runtime list's cause 4 is "The sector reading location exceeds total sector number of the source partition"; (2) the finding's open question RESOLVED-AS-REFUTED: FUN_1400ed760 is a %d string formatter (read in full), NOT the code→cause mapper — the og's internal mapping function remains corpus-undecoded, so the core maps by the cause strings' semantics (registered as PARITY-DELTAS D-149); (3) the runlist export body re-read in full (:875620-875840): the straddling-read count clamp (:875666-68), the read-failure trio + error 9 + break (:875684-875696), the outer-loop error exit, the "Failed to write file!" write-failure family, and the beyond-EOF behavior compose into: clamp the straddle, fail the beyond-start read, truncate at the failure, keep the partial. The fix lands the og's semantics: a HARD read failure (the pipe's post-retry error-map hit for basis-0, the raw non-OK read for bases) truncates at the FIRST UNREADABLE BYTE (the bisection knows the exact boundary — D-149's registered refinement over the og's read-geometry-granular truncation), COMMITS the partial (never deletes — the old ERR_TRUNCATED refusal is gone too: the og's runlist simply ends the file where the runs end), records Outcome::ReadError + cause_id + first_failed_offset + bytes_written in the report (additive-within-v1, schema doc updated), fires the verbatim log trio through cr::log, keeps the pass running (WRITER-1), and the CLI renders the og's numbered cause strings. The per-failure-class split the finding's fix plan prescribed: read failures truncate (the runlist path); undecodable compressed blocks keep zero-fill + Salvaged (the og's chunked-path behavior); the short-extents case commits with cause 7 (the og is silent — the core's report is the truthful superset). The tree-node failure mark stays RUNTIME-ONLY (the QueueItem's cause row — the og's +0x73 bit 1 is runtime-only too; the session never persists it; decision registered). Safety hardenings untouched: F12/N-013 arming, the destination-side raw IO, S-059 holes, S-060 metadata tail (the partial keeps it). D-149 registers the three deliberate refinements.
