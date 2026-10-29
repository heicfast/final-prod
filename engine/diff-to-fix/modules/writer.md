# modules/writer.md — recovery writer + report audit verdicts

Reference: og ExportThread/recovery flow per `architecture/04-recovery-workflow.md` §7.3–7.4
(destination validation via `\\.\X:` raw open + same-volume detection, 0x5c→0x2f
normalization, per-file error collection "one unreadable file must not cancel the
remaining ten thousand" — WRITER-1 parity), ReportSchema contract.

Core (audited `67193b6` + concurrent GAP-S6-7 in tree): `src/core/recovery/` (779 LOC) +
`tests/unit/test_writer_dirs.cpp` + `test_report.cpp` + `tools/validate_report.py`.

## Verdicts

- **B2 recovery correctness** — `M`: resident-prefix + extents streaming; compressed
  extents decode with never-wrong-bytes policy (zero-fill + Salvaged flag); SHA-256
  always captured (G-09), verify against sha128 with mismatch-still-commit salvage.
- **F62 destination overlap refusal** — `M` (hard refusal on any intersecting extent,
  R7.2/F12; directories never refused G-08). Og equivalent: same-volume detection at
  destination validation.
- **D13 report schema** — `M`: schema_version 1, totals {queued, done, dirs, refused,
  errors, hash_mismatch, resumed, salvaged, pending, bytes} + per-item outcomes
  (done/refused/error/hash_mismatch/resumed/pending/salvaged) — the og has no JSON
  report (GUI funnel); the core's schema is its own contract, validated by CI
  (`validate_report.py`). GAP-S6-7 (concurrent) added the NothingToRecover skip policy
  (metadata-only items never zero-fill) + schema/ABI append — verified in tree.
- **Name sanitization** — `M`: `/ * : ? " < > |` → `_` (the og's volume-name sanitizer
  set — same rule family applied to recovered filenames).
- **Collision policy** — `M`-note: Rename suffix " (n)" before last dot; Skip/Overwrite
  idempotent. Og collision policy not decoded in corpus (UI-driven) — no contradiction.
- **O38 progress** — **N-009b** (error-item bytes counted as done).
- **F64 ACLs** — **N-009d** (not preserved; og preserves on copy paths).
- **S32 cancellation** — `M`: per-chunk checkpoints, ERR_CANCELLED aborts with temp
  closed; save-on-cancel at the CLI.
- **X49 memory** — `M`-note: whole-block compressed-extent decode spikes ≤ 256 MiB per
  extent (documented cap; a windowed decoder is a follow-up, not a parity gap — og has
  no compressed-extent writer at all).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| N-009b | MINOR | Progress counts error-item bytes as done |
| N-009d | MINOR | ACLs not preserved on recovery (og preserves on copy) |

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **F62 — N-013 (MINOR):** destination overlap check is OPTIONAL (gated on
  `have_dest_` which requires drive-letter path + disk-known source +
  non-image transport — `cr_ui.cpp:2057-2063`, `rrecov.cpp:869-873`) AND
  first-extent-only (`cr_transport.cpp:701-712` returns only
  `ext->Extents[0]` from `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`; the
  `VOLUME_DISK_EXTENTS ext[8]` buffer has room for 8 extents but only the
  first is read out). Image-source-to-same-disk and multi-extent
  spanned-volume destinations miss the check. The og ALWAYS opens `\\.\X:`
  for drive-letter destinations and uses the full multi-extent result
  (`RecoveryModule_decompiled.c:60381-60444`).
- **F60/O38/X48 — N-015 (MINOR):** writer does NOT pre-check destination
  free space. `WinDestIo::write()` (`cr_destio.cpp:24-42`) returns
  `ERR_WRITE_FAILED` on any `WriteFile` failure (including
  `ERROR_DISK_FULL = 112`); no ENOSPC special-case. The queue loop
  (`cr_safewriter.cpp:440-469`) continues to the next item after a disk-full
  failure — every subsequent item will ALSO fail with ENOSPC, pointlessly
  attempting `create_temp` + `write` + `delete` for each. The og's
  `RecoveryModule.dll` calls `GetDiskFreeSpaceExA` before writing
  (`RecoveryModule_decompiled.c:47033`).
- **N-009d refinement (REFUTED at the S-060 verification, 2026-10-29):** the
  session-2 claim above — "og ALSO resets attributes to FILE_ATTRIBUTE_NORMAL
  (`RecoveryModule_decompiled.c:79677`) and does NOT call SetFileTime" — was
  grep-scoped to the WRONG BINARY (RecoveryModule, the progress dialog's
  helper, not the export body). The export loop's own tail in DRWUI
  (:874686-718, the binary's only SetFileTime/SetFileAttributesW pair) restores
  the DOS attributes (word & ~0x2) and the mask-gated timestamp trio after
  EVERY item, both failures non-fatal. S-060 (FIXED session-51) landed the
  core's equivalent: `DestIo::set_metadata` + the writer's post-commit call
  with per-field enables mirroring the engine emission (zero slots pass as
  NULL); the Win32 backend implements the og's shape verbatim, the POSIX
  translation (utimensat + the readonly-bit chmod) is registered in
  PARITY-DELTAS. The ACL drop (N-009d proper) remains the only OTHER
  divergence on this axis.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-013 | MINOR | **FIXED (s7; FIXED/ move s11)** — all-extent arming + any-resolving path arming; image→same-volume sub-claim refuted with corpus evidence |
| N-015 | MINOR | **FIXED (s9 `915bce3`; FIXED/ move s11)** — ERR_DEST_FULL end-to-end: free_bytes pre-check + ENOSPC fast-fail, report still emitted |
| N-009d (refined) | MINOR | ACL drop is the only divergence (timestamps/attributes match og) |
## Session-2 update (2026-10-25, core `e93decf`)

- **N-016a filed:** U+FFFD replacement-count policy (core per-bad-lead + resync vs og's
  Windows maximal-subpart via MB2WC CP_UTF8 dwFlags=0 on the restore path,
  `DRWUI_decompiled.c:264664-94`/`:357529`) — restored filenames can differ in FFFD count;
  fix = maximal-subpart decoder + restore from raw bytes (with N-016b).
- Observation (owner note for N-009b/d): og restore-name sanitizer replaces `:?\|*><"`
  including backslash → `#` (`:357536-545`); core's `sanitize_name` replaces 8 chars with `_`
  and does NOT sanitize backslash — fold into the N-009 group when touched.
- N-014b: carved-name split — RESOLVED (s28; one File%06u family per scan, both carve layers).

---

## Session-5 audit (audited commit `7cf6754`)

- **N-056 (MINOR, NEW):** WinDestIo leaks the `.part-N` temp file when the commit-time
  rename fails (`commit()` nulls the handle before MoveFileExW; `close()`'s delete gate
  requires a live handle — cr_destio.cpp:44-71). UNC path construction untested. (An
  earlier scratch note claiming the ERROR_IO_PENDING branch is unreachable was withdrawn
  this session — overlapped handles make it reachable.)
## Session-6 rows (deep continuation, 2026-10-26)

| ID | Severity | One-line |
|---|---|---|
| N-048 | MINOR | Collision naming: og `name_N.ext` (`%s_%d` forms) cap 10,000 (DRWUI:357116-357131) vs core `name (N).ext` cap 64 + `_dup` fallback; mkdir: og accepts SHCreateDirectoryExW only on 0/0xB7 (DRWUI:870436-438) vs core exist-as-dir-beats-any-error — and D-19's "collision unevidenced in og" premise refuted (evidence: s6-writer-collision-and-mkdir.md) |

## Session-17 fix row (2026-10-26, DFX17 — N-056 closed)

**N-056 FIXED** (findings/FIXED/): the failed-rename temp leak closed at
both sites — commit() deletes the staged temp in the rename-failure path
(the og's funnel leaves no orphaned temps, UIUX 11), and close()'s
delete gate is now the RENAME STATE (`Open::temp_renamed`, set only
after a successful MoveFileExW) instead of handle liveness (the old
gate provably never fired after a failed rename: commit() nulls the
handle BEFORE MoveFileExW). Covers every un-committed exit (write-phase,
flush, failed rename with the close-side retry) and never fires
post-success. WinDestIo.FailedRenameCleansTemp (structural fail-before
pinned in-test) + WinDestIo.UncShapedPathsFailCleanly (the `\\?\UNC\`
family: exists/mkdir/create_temp contracts on an unresolvable share).
Windows/MSVC lane proves execution; Linux suite 684/684 unaffected.
F62/A35/Q65 → MATCH.


## Session-34 verification (DFX34) — N-065 closed FIXED-ALREADY

| Parameter | Verdict |
|---|---|
| A27 collision naming | **MATCH** — the og's two `%s_%d`/`%s_%d%s` branches (DRWUI :357116-357131), last-dot split, extensionless branch, counter from 1, cap 10,000, exhaustion fails the item (parallel-lane landing, verified green this session) |
| S35 mkdir acceptance | **MATCH (semantic form, REGISTERED)** — the SHCreateDirectoryExW 0/0xB7 contract as exist-as-directory success / file-in-the-way fail / else propagate; the literal gate broke drive-root chains (MSVC test #227 incident, in-source comment) |
| P46 D-19 doc truth | **MATCH** — the refuted "unevidenced" claim superseded by the decoded policy (PARITY-DELTAS:456+) |


## Session-8 section (2026-10-28, audited `35adcb9`)

**M-071** (**FIXED** — the M-018 regression: recover-all and the CLI tree print
lost their junk-filter gates in the N-064 rrecov.cpp reconstruction;
`69507cb` → `cfa4c75`; the stale cr_scan.cpp:1200 comment. Fixed by the
closing-fix session: both gates restored byte-exact from the M-018 form +
the cr_ui session/snapshot bulk walks gated the same way + the
explicit-selection semantic pinned by test — see the finding's Resolution).
**M-074**
(MAJOR — destination validation classification: path-too-long/invalid/network
collapse to ERR_DEST_UNWRITABLE vs the og's distinct outcomes
("Might be invalid path to save." / "The recovery path is too long." /
PathIsNetworkPathW branch); fix = three appended 0x5005-family codes + the
CreateFileW error classification).

## Session-42-B addendum (2026-10-29, audited `869da779`, corpus `be0cfe9` — the continuation lane B's report-surface filing; renumbered at the S42-B merge: N-155→N-159)

**N-159** (MINOR, renumbered from this lane's N-155 filing — the parallel window's HFS+ finding keeps the ID — the failed-files report's classification axis): the og's
"recovery log" is an HTML report (`CSaveFailedFilesWithHtml` — the 2000-byte
content buffer, `ShellExecuteW` open-the-report action), accumulated per failed
file during the export loop (failed-count + failed-bytes counters), whose per-file
rows carry a tree-classification column derived from the node (group 1 → "Lost
Files" + "Reconstructed"@kind7 / "Tags Extracted"@kind6; group 0 → "Existing
Files"@kind4 / "Lost Files"+"Deleted"@kind0; else the custom raw-category name) —
`DRWUI_decompiled.c:642669-642822`, first-hand. The core's
`cr_ui_recovery_report_json` (D13's deliberate structured superset) carries no
classification field and no failed-bytes split. The register half: R-58's
"per-run text log" (REFERENCE-API-GAP:104) and API-DELTA row 21's "og format NOT
IN CORPUS" are refuted — the generator + row grammar are corpus-decodable. Fix
shape: a per-item `tree_class` field + `failed_bytes` split (append-only schema
bump) + the three register-site truth repairs. Evidence: the S42 decode pack §2.

---

## Session-12 continuation row (2026-10-29, audited `c7fa527c`, corpus `be0cfe9`)

**S-054 (SEVERE — NEW, REGRESSION):** the N-013 multi-extent destination arming was
clobbered back to first-extent-only at both production call sites by the unrelated
M-036 NVMe commit `1ebd1bcc` (git-proven: the `3b5397a1` landing's spans loop +
junction resolution removed in that commit's rrecov.cpp/cr_ui.cpp diffs).
`volume_extents()` / `set_destination_spans()` / `destination_volume_letter()`
survive as dead code (zero production consumers, grep-verified) while
`cr_transport.h:124-126` still publishes "destination arming must use
volume_extents() (all spans)" and the ledger records N-013 FIXED. The SafeWriter's
refusal loop itself is intact and multi-span-aware (cr_safewriter.cpp:135-137) — it
is fed an incomplete span set. F62 verdict: **DIVERGENCE-SEVERE** (the refused-write
guarantee silently defeated for spanned/RAID second+ spans and junction-rooted
destinations — the write-proceeds-onto-source-disk window); Q65: the suite is green
over the regression (1117/1117) because no test pins the wiring. Fix: re-land the
`3b5397a1` shape + the wiring-fence tests. Evidence:
`evidence/decompiled-quotes/s12-destarming-regression.md`.

## S42 continuation lane (2026-10-29 — the file-reconstruction focus battery; core `b26017a7`)

Four new rows, all first-hand verified (worksheet:
`../evidence/decompiled-quotes/s42-reconstruction-export-chain.md`):

- **B2/F60 (recovery correctness under holes) — FIXED session-51 (S-059) → MATCH**: the og
  writes zero chunks for sparse runs and always produces the full logical size
  (DRWUI:875661-673 runlist path, :875062-75 chunked path); the core drops sparse
  runs (`cr_runlist.cpp:190`), has no per-extent logical offset
  (`cr_node.h:109-120`), left-shifts interior holes, and refuses with
  ERR_TRUNCATED (`cr_safewriter.cpp:440-444`) — NTFS sparse files and
  compressed-with-hole-unit files are lost or shifted; the mftwalk comments
  claim a zero-fill the writer cannot perform.
- **B9 (timestamp restoration) — FIXED session-51 (S-060) → MATCH**: the og restores
  DOS attributes + the mask-gated SetFileTime creation/access/write trio after every
  item (DRWUI:874686-718 — the binary's only SetFileTime; **the N-009d row above
  ("og never calls SetFileTime") is REFUTED**); the core's DestIo now carries the
  set_metadata surface, the writer restores post-commit with per-field enables
  mirroring the engine emission, non-fatal + reported per row (4 tests RED→GREEN).
- **E35 (read-failure policy) — DIVERGENCE-MAJOR, M-135**: the og truncates at the
  failing run, logs the StartSector/LCN/SPC + ReadCount trio, records error 9,
  keeps the partial file, and surfaces six numbered causes; the core
  zero-fills-and-continues (Salvaged) and deletes failed items wholesale; the
  per-node runtime failure mark (+0x73 bit 1) has no analog.
- **E35/R51 (the recovery read pipeline) — DIVERGENCE-MAJOR, M-136**: recovery
  hands the writer the raw device (`rrecov.cpp:2080-2082`, `cr_ui.cpp:4027`)
  while the scan's Retry(3×)+Cache pipe (`cr_scan.cpp:155-194`) goes unused —
  one transient read error becomes a permanent zero span.

## Session-46 addendum (2026-10-29 — the MINOR-first fix lane; audited `fa654673`-lineage, corpus `be0cfe9`)

**N-159 FIXED — the failed-files report's tree-classification axis.** The
register-truth half first (three sites repaired): R-58's "per-run text log"
→ the truth (an HTML report: `CSaveFailedFilesWithHtml`, the generator class
family decoded at DRWUI :642669-642892); API-DELTA row 21's "og format NOT
IN CORPUS" → "decodable; the generator + grammar are in the decompile";
D13's addendum below. The axis half: the og's per-row tree classification
(:642755-642822 — group@+0x1e8 / kind@+0x1f0 / custom-name@+0x1d8 → the
"Lost Files"/"Reconstructed"/"Tags Extracted"/"Existing Files"/"Deleted"/
custom-name labels) lands as the report's per-item `tree_class` wire field
(derived from the node's root role, the S-048/M-103 convention) + the
`failed_bytes` totals split (the og's +0x254/+0x268 failed-count/failed-bytes
accumulators). Both additive-within-schema-v1 per the schema doc's own
version contract (the finding's "version bump" plan corrected). Tests:
Report.FailedRowsCarryTreeClassification / FailedBytesSplitFromTotal /
RawCategoryNameClassification / TreeClassIsAdditiveWithinSchemaV1. P13/P11 →
MATCH; P46/P68 register-truth repaired.
