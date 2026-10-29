# Recent commits — Session-84 Phase 0 (baseline e8dcaba8)

Scope: `git log -60` from HEAD `e8dcaba8` (all 60 land on 2026-10-29, 14:07–16:54 UTC —
a single ~2.8-hour racing window). The last 50 (`e8dcaba8`..`59a431d3`) were file-mapped
via `git diff-tree`; commits 51–60 are summarized from subject + known context.
No merges in the window; working tree clean at audit start.

Authors across the last 50 (the racing lanes): `parity-fix-lane` 18, `Z User` 18,
`core-recovery-fix-agent` 10, `Z Fix Agent` 2, `parity-fix-agent` 1, `coherence-session` 1
— **6 identities pushing to main inside 3 hours**.

Subject-prefix census (last 50): fix 25 · repair 8 (+1 `converge+repair`) · trail 7 ·
converge 4 · rider 2 · docs 1 · worklog 1 · chore 1.
Keyword census in subjects: "racing" 18/50 · "converge/convergence" 17/50 · "trail" 30/50 ·
"rebase" 12/50 · "conceded" 12/50 · "RED->GREEN" 8/50 · "conflict" 6/50 · "red" 25/50.

## The last 60 commits

| sha | date (UTC) | category | purpose (one line) | subsystems |
|---|---|---|---|---|
| e8dcaba8 | 16:54 | session-open | Session-84 opened; baseline cac42582; 5-file live-marker sweep noted (N-210 class) | worklog |
| cac42582 | 16:47 | trail | M-154 convergence record — seat conceded to racing af5987d0 | findings/FIXED/M-154 |
| 497b1277 | 16:47 | repair(docs) | A-ABI-3 count drift recurred TWICE: doc counts 89 vs tree 91 (gate RED at origin/main); 5 sites refreshed | docs, worklog |
| b2bc1970 | 16:45 | fix+rider | S82 batch (I-048 ack D-155..D-157, M-111/M-114 closed D-158/D-159) + M-154 hardening rider (name gate, save_path basename gate) | session core, ABI(cr_ui.h), tests, trail |
| 0cf6142d | 16:44 | fix(carve) | M-107 CRW member: CIFF heap walk replaces 64 MiB stand-in (cap 0x3200000, tail map {a8:0x46,b2:0x3c,e4:0x32}) | carve core, tests, trail |
| 1d7d31a6 | 16:42 | converge+repair | S81 rebase landing: N-145 conceded; racing CLI syntax error repaired; M-151 red window #3 closed | worklog only (see red flags) |
| d7856484 | 16:42 | fix(apply) trail | N-145 landing record — but diff is trail-only (code lives in 5bdefa58) | trail docs |
| 9b4a9302 | 16:40 | trail | S-068 4th implementation conceded to ad1bcb9e; claims "no code change" yet renames fat hint member | fat core+ABI header, trail |
| 977812cd | 16:39 | docs(trail) | I-048 acknowledged — three registers verified, registered | trail |
| af5987d0 | 16:37 | fix(session) | M-154 auto-session lifecycle: retention-one, last_scan.info pointer, pointed-resume ABI export | session core, ABI(cr_ui.h, cr_lastscan.h), src/ui, CLI, build |
| 033c5d7c | 16:37 | fix(orch) | N-181 RAW-first start order (carve before deep extras) | orchestrator core, tests, trail |
| 5e1dca99 | 16:28 | converge | M-151 conceded to racing 27bda40e (4th independent impl this window) | trail |
| 67d2bf25 | 16:26 | repair(trail) | born-collided D-150 seat: THREE rows claimed D-150 → renumbered D-153/D-154 (gate caught it) | trail docs |
| 92bb8ead | 16:24 | trail | M-152 convergence record — "the third independent landing" | trail |
| f18ba226 | 16:23 | repair(ntfs) | N-219: 4th red-main conflict-marker repair (27bda40e); interleaved tests reconstructed | tests, finding N-219 |
| 18012c1a | 16:23 | fix(ext4) | M-152 orphan-reserved skip dropped; `orphan_inodes` counter (twin of 15f51121) | ext4 core+ABI, tests, trail |
| c7037bf3 | 16:20 | repair(tests) | M-151 rebase interleaved-test conflict resolved (A-016 + growth tests both kept) | tests only |
| 842a587e | 16:19 | fix(cli) | N-178 pause surface CLI legs (--pause-stdin + engine-notification registration) | CLI, tests, win-stub, trail |
| 27bda40e | 16:12 | fix(ntfs) | M-151 truncated-$MFT run-growth probe — pushed RED (committed conflict markers) | ntfs core+ABI, tests, trail |
| 15f51121 | 16:10 | fix(ext4) | M-152 racing twin landing — `orphan_admitted` counter naming | ext4 core+ABI, tests, trail |
| b051fe45 | 16:06 | rider | N-211 third site: rrecov.cpp stale comment re-truthed; doc-truth gate widened to CLI | CLI comment, tools/doc_truth_gate.py |
| 0eaded3d | 16:05 | fix(carve) | M-156 X3F: ASCII version gate REJECTED every real Sigma X3F (live bug); union acceptance + version-shape routing | carve core, tests, trail |
| 868d1e85 | 16:04 | fix(docs) | N-182 deep-range resolver closed by registration (D-36 wording corrected) | docs, trail |
| d1730da3 | 16:03 | trail | M-149 convergence record (seat conceded to racing 1ecd0875) | trail |
| e2ea4d50 | 16:02 | fix(verify) | A-016 root-dot folder-filter transparency + N-218 unsigned live-counter wrap — defects found IN the racing M-073 landing | tree/orch core, tests, trail |
| 11cbb13b | 16:01 | trail | S-068 arbitration addendum + FUN_180012c00 stride-scan decode | trail |
| d558e6a4 | 16:00 | converge | M-150 conceded to racing f7432fb4 (3rd impl; stronger 78-byte bound recorded as optional tightening) | trail |
| 5bdefa58 | 15:58 | fix(apply) | N-145 in-flight journal CRIFJ01 — the CODE lane (ABI cr_apply.h, CLI probe, tests) | apply core+ABI, CLI, tests, trail |
| f7432fb4 | 15:55 | fix(session) | M-150 count pre-allocation bomb gate (wire-impossible node count refused) | session core, tests, trail |
| 6b9f03ce | 15:55 | repair(trail) | N-216 rebase's nested worklog conflict resolved (marker swept post-push) | worklog only |
| 92ae836c | 15:54 | fix(orch) | N-216 MaxFileSize cap (filterfolder; set_max_file_size ABI, 1 TiB default) | orch core+ABI(cr_filterfolder.h), tests, trail |
| ce0f1101 | 15:52 | converge | S-068 conceded to racing ad1bcb9e (3rd impl; two open decode deltas registered) | trail |
| 1ecd0875 | 15:49 | fix(lost) | M-149 ghost-score two-pass form (+500 numerator-only, count==0 → 0.0 floor) | lost core, tests, trail |
| 638abdd3 | 15:46 | trail | S-068 convergence record (lane's impl conceded; corroboration recorded) | trail |
| c71099db | 15:42 | fix(ntfs)* | N-213 "lands" — but diff is worklog.md ONLY (code lives in twin e54762dd) | worklog only |
| ad1bcb9e | 15:40 | fix(fat) | S-068 deleted FAT32 high-16 re-base (last-good hint machinery) — SEVERE tier closes | fat core+ABI(cr_fat_engine.h), tests, trail |
| 793c9c4b | 15:39 | trail | M-073 seat concession (racing 0ba5f14e stands; lane re-planned) | trail |
| 023cee29 | 15:35 | repair(ci) | clang-format 18.1.8 sweep — racing lanes' accumulated formatting drift cleared | 23 files: core, ABI headers, ui, tests |
| e54762dd | 15:34 | fix(ntfs) | N-213 record-header pre-gate band, family-keyed (FILE 0x2A/INDX 0x28 floors) | ntfs core, tests, trail |
| 532c6811 | 15:29 | fix(reghive) | M-113 recovered-OS boot surgery: HiveEditor + surgery, 2 NEW ABI headers, R-69 PARTIAL | reghive core+ABI(2 headers), build, tests, trail |
| 2d614cf2 | 15:23 | fix(exfat) | S-067 correction by arbitration: clamp bound is og PRIMARY-path formula (fixes racing d37840c4's shape) | exfat core, tests, trail |
| 42228a11 | 15:19 | converge | X3F arbitration conceded AFTER first-hand dispatcher verification (census prose = erratum) | trail |
| 5291076b | 15:16 | repair(trail) | N-214 push's rebase-resolution remnant swept from FIXED-LEDGER (N-120 class, caught post-push) | ledger only |
| 55f6ecc1 | 15:15 | fix(lost) | N-214 ext probe journal-indicator consistency gate | lost core, tests, trail |
| dae8a65c | 15:12 | repair(trail) | stray conflict-separator line removed from FIXED-LEDGER (N-210 class, caught pre-push) | ledger only |
| 0ba5f14e | 15:12 | fix(orch) | M-073 scan-time folder filter: cr_folderfilter + ABI append cr_ui_scan_config{filter_folders} + CLI --filter-folder | orch core, 7 ABI files, CLI, build, tests, trail |
| d37840c4 | 15:11 | fix(exfat) | S-067 out-of-range FAT-entry clamp (entry >= cluster_count → EOC) | exfat core, tests, trail |
| 9ddc2bbe | 15:08 | chore | purge accidentally-committed build2/ tree (802 files) + gitignore it | .gitignore |
| 113a5627 | 15:08 | rider | S80 concession + validate_x3f 256 KiB window off worker stack — AND accidentally committed build2/ (802 files) | carve core, build2 (802 files), worklog |
| 59a431d3 | 15:07 | fix(docs) | N-215 R-20 'Unimportant' mechanism re-truthed (usefullostfiles.xml gate, not M-018) | docs, trail |
| 567694e7 | 15:05 | trail | rebase-lost blocks re-landed + board re-counted vs filesystem (racing block's 18M/11N/170FIXED corrected to 30 active/173 FIXED) | trail |
| 3dfb2a5a | 15:03 | fix(docs) | N-211 autosave_dir header contract re-truthed at both sites + retired-phrase gate (caught a 2nd drift site cr_ui.h:636) | include comments, tools, trail |
| a67e1f62 | 15:00 | repair(trail) | N-217 ed501caa conflict-marker recurrence recorded + X3F authority dispute settled by dispatcher decode | trail |
| 9508b90b | 14:59 | audit | M-155 convergence record — parallel corroboration, finding moved to FIXED | trail |
| 6d74ec2c | 14:50 | fix/repair(carve) | M-155 red-main repair: ed501caa's committed conflict markers resolved corpus-first | carve core, tests |
| 04fb5e88 | 14:17 | audit | S69 lane filings: 5 new findings (M-154/N-214/N-215/N-216/I-048) + 2 addenda + liveness battery | evidence, findings |
| 4298a48d | 14:16 | fix(preview) | N-144 TxtViewFiles text-snapshot thumbnails (ABI thumbnail routes + txt support) | preview core+ABI, tests, trail |
| ed501caa | 14:15 | fix(carve)* | M-107 X3F member — pushed WITH committed conflict markers in validate_x3f (red-main #1 of the window) | carve core, tests (broken at push) |
| 1f271af0 | 14:10 | fix(carve) | M-107 X3F racing twin (count-walk variant; superseded by arbitration) | carve core, tests |
| ff7e0b7c | 14:07 | worklog | S79 close-out handoff (coverage stats, first-20 fix queue, seeds) | worklog |

(*) = subject claims a code landing the diff does not contain — see Red flags.

## ABI-touching commits

12 of the last 50 touched `include/cr/**` or `src/ui/`:

- **0ba5f14e** (M-073) — the big one: 7 ABI surfaces (`cr_ui.h` append `cr_ui_scan_config{filter_folders_utf8,filter_folder_count}`, new `cr/orchestrator/cr_folderfilter.h`, `cr_scan.h` ScanConfig.filter_folders, `cr_crsf.h`/`cr_records.h` REC_SCAN_FILTER record, `cr_node.h` detach_child, `src/ui/cr_ui.cpp` wiring).
- **af5987d0** (M-154) — `cr_ui.h` append `cr_ui_session_last_autosave_path()`; new `include/cr/session/cr_lastscan.h`; `src/ui/cr_ui.cpp` retention loop.
- **b2bc1970** (M-154 rider) — `cr_ui.h` append `cr_ui_session_resume_last()` (the "newer append" that together with af5987d0's export left doc counts at 89 vs 91 → 497b1277 repair).
- **532c6811** (M-113) — 2 NEW headers: `include/cr/reghive/cr_reghive_edit.h`, `include/cr/reghive/cr_surgery.h`.
- **5bdefa58** (N-145) — `include/cr/apply/cr_apply.h` (ApplyJournalFn sink, inflight_probe).
- **92ae836c** (N-216) — `include/cr/orchestrator/cr_filterfolder.h` (max_file_size/set_max_file_size).
- **18012c1a + 15f51121** (M-152 twins) — `include/cr/ext4/cr_ext4_engine.h` BOTH add an orphan counter: `orphan_inodes` (18012c1a, survives at HEAD) vs `orphan_admitted` (15f51121, gone from code but alive in trail docs).
- **27bda40e** (M-151) — `include/cr/ntfs/cr_bootstrap.h` (run-growth probe surface) — the commit that pushed RED.
- **ad1bcb9e** (S-068) — `include/cr/fat/cr_fat_engine.h` (high16_rebased stat).
- **9b4a9302** (S-068 concession) — `cr_fat_engine.h` + `cr_fat_quick.cpp` comment churn + member rename `last_good_first_cluster`→`last_good_first` DESPITE "No code change rides this commit" in the message.
- **023cee29** (format sweep) — `cr_grading.h` + `src/ui/cr_ui.cpp` formatting only.
- (just outside the 50: 4298a48d N-144 — thumbnail ABI routes.)

Net ABI growth in the window: +91-export surface (docs now claim 91 exports / 45 error codes / 12 event types — verified derivable at HEAD), two new module headers (reghive), one new orchestrator header (folderfilter).

## CLI-touching commits

5 of the last 50 touched `src/app/rrecov.cpp`:

- **0ba5f14e** (M-073) — CLI scan `--filter-folder DIR` (repeatable, three-site lock).
- **5bdefa58** (N-145) — CLI writes `rrecov-apply.inflight` journal beside undo log + probes at EVERY startup (warning names disk ordinal); note: at HEAD the probe loop is `marks[m].identity` / `marks[m].begin_unix` (rrecov.cpp:3351–3360) — correct.
- **b051fe45** (N-211 rider) — comment-only re-truth of rrecov.cpp:2063 completion-window contract + `tools/doc_truth_gate.py` RETIRED_CLI_CONTRACT_PHRASES extended to the CLI surface.
- **842a587e** (N-178) — `--pause-stdin` leg + engine-notification registration.
- **af5987d0** (M-154) — the CLI twin of the autosave retention loop (delete-previous-auto before every write).

No `.github/workflows/` commits in the window (the apparent hits were `build2/` paths). Build files: `src/core/CMakeLists.txt` ×3 (af5987d0, 532c6811, 0ba5f14e), `tests/CMakeLists.txt` ×2 (532c6811, 0ba5f14e) — all new-module wiring, no gate changes. The only CI-adjacent commit is **023cee29** (clang-format sweep) and **9ddc2bbe/113a5627** (the build2 add/purge pair).

## Repair/convergence commits — the racing-lane pattern quantified

**9 explicit `repair` commits in the last 50** (13 subjects contain "repair"), plus 2 implicit
repair landings (e2ea4d50, 2d614cf2) and 1 more just outside the window (6d74ec2c):

| sha | damage class repaired |
|---|---|
| 497b1277 | doc count drift (A-ABI-3): gate RED at origin/main — exports 89 claimed vs 91 derived, after two racing ABI appends |
| 1d7d31a6 | red window #3: 27bda40e's committed conflict markers (tests + matrix) + racing N-145 CLI syntax error; diff is worklog-only |
| 67d2bf25 | born-collided D-150 seat: THREE rows claimed D-150; renumbered D-153/D-154 |
| f18ba226 | N-219: red window #4 — 27bda40e markers again; tests reconstructed to "semantic intent" |
| c7037bf3 | interleaved-test conflict from the M-151 rebase (both test generations kept complete) |
| 6b9f03ce | nested worklog conflict marker swept post-push (N-210 class) |
| 023cee29 | accumulated clang-format drift across racing landings (23 files) |
| 5291076b | `>>>>>>>`-only remnant in FIXED-LEDGER (N-120 class, caught post-push) |
| dae8a65c | stray conflict-separator line in FIXED-LEDGER (caught pre-push) |
| e2ea4d50* | two real defects found in the racing M-073 landing (A-016 root-dot filter, N-218 counter wrap) |
| 2d614cf2* | arbitration correction of racing d37840c4's S-067 clamp bound |

**Convergence/concession records: 12 commits** (4 `converge(` + 1 `converge+repair` + 7 trail
concessions): cac42582, 9b4a9302, 92bb8ead, d1730da3, 11cbb13b, 638abdd3, 793c9c4b,
5e1dca99, d558e6a4, ce0f1101, 42228a11, 1d7d31a6. Concessions cite SA-9 (drop your
independently-derived duplicate at the push rebase). Counting the messages, at least
**7 findings were independently implemented 2–4 times each within the window**: S-068 ×4,
M-151 ×4, M-152 ×3, M-150 ×3, N-145 ×2 (+1 conceded design), N-213 ×2, M-107-X3F ×2,
M-155 ×2, M-073 ×2, M-154 ×2 (+ rider).

**Red-main events in the 60-commit window: 4** — ed501caa (conflict markers in
validate_x3f + the finding file; "suite 1570/1570" claimed on a non-compiling tree →
repaired by 6d74ec2c/a67e1f62), 27bda40e (markers in test_ntfs_volume.cpp + matrix →
repaired by c7037bf3/f18ba226/1d7d31a6 — "win_syntax_check not run before push"), the
N-214 push (ledger remnant → 5291076b), and the A-ABI-3 count-RED at origin/main
(497b1277). Every one is the same root cause: racing lanes push without running the
local gates (`win_syntax_check`, `doc_truth_gate`, N-210 marker lint).

## Hot files

Top 15 by touch count in the last 50 commits (build2/'s 802-file noise excluded):

| # | file | touches | note |
|---|---|---|---|
| 1 | worklog.md | 37 | 74% of commits; carries a live marker block at :12069–12168 |
| 2 | diff-to-fix/CROSS-REF-MATRIX.md | 24 | live marker block at :39–46 |
| 3 | diff-to-fix/FIXED-LEDGER.md | 21 | live marker block at :656–669 + inline `>>>>>>> fd6d63d1` remnant at :635 |
| 4 | diff-to-fix/SUMMARY.md | 14 | NOT in the known-marker set — prime candidate for silent board-count drift |
| 5 | docs/PARITY-DELTAS.md | 12 | TWO live marker blocks (:4677–4682, :4707–4863) + orphan end at :4743 |
| 6 | tests/unit/test_ntfs_volume.cpp | 6 | twice conflict-torn (27bda40e) then reconstructed |
| 7 | tests/unit/test_cr_ui.cpp | 5 | ABI-consumer pins |
| 8 | src/app/rrecov.cpp | 5 | CLI surface (5 landings above) |
| 9 | diff-to-fix/findings/FIXED/S-068-…md | 5 | 4 implementations' addenda stacked |
| 10 | src/core/carve/cr_analysers.cpp | 4 | M-107/M-155/M-156 X3F+CRW wars |
| 11 | tests/unit/test_session.cpp | 3 | M-154/M-150 |
| 12 | tests/unit/test_lostpart.cpp | 3 | M-149/N-214 |
| 13 | tests/unit/test_exfat_volume.cpp | 3 | S-067 (landed, corrected, arbitration) |
| 14 | tests/unit/test_carve.cpp | 3 | X3F pins replaced twice (S78 vs S71 generations) |
| 15 | src/ui/cr_ui.cpp | 3 | ABI shim (tied: src/core/orchestrator/cr_scan.cpp 3) |

Trail-vs-code ratio: ~1,100 of ~1,130 non-build2 file touches are `diff-to-fix/`+`docs/`+`worklog`
bookkeeping; only ~48 touches are `src/`, 45 `tests/`, 19 `include/`.

## Red flags for the coherence audit

Each tied to the SHAs that predict it:

1. **D-152 is triple-claimed at HEAD** — `docs/PARITY-DELTAS.md` carries `### D-152` (N-145, :4218) AND `## D-152` (M-152 ext4 guards, :4656) AND `## D-152` (M-154 lastscan, :4683). Root cause: af5987d0 and d7856484/5bdefa58 both registered "D-152" although 67d2bf25 had already ruled the seat held. **The gate's regex `^(?:###|\*\*)\s*D-` (tools/doc_truth_gate.py:466) never sees `##` headings** — this N-191/N-207 recurrence is invisible to the gate that "caught" the D-150 triple.
2. **I-048's registers are double-registered**: the combined `### D-155` row (:4710) coexists with renumbered `## D-160/D-161/D-162` rows (:4745/:4760/:4779), and b2bc1970's message claims "PARITY-DELTAS D-155..D-159" while the tree holds D-155+D-158/159+D-160/161/162 (and D-160..D-162 appear BEFORE D-158/159 in file order). FIXED-LEDGER carries **two I-048 rows** (:657 "query/config/lost-partition" vs :666 "query/carve/lost-partition" — subsystem tags disagree). Origin: b2bc1970 vs its pre-rebase twin 09a18f9f (the side preserved inside the live markers).
3. **Commit-message/diff mismatches (the phantom-claim class)**: d7856484 describes the full N-145 implementation + "suite 1634/1634" but touches only trail docs (code is in 5bdefa58); c71099db says N-213 "lands" but changes worklog.md only (code is in e54762dd); 1d7d31a6 claims rrecov.cpp:3259/:3264 syntax repair + test/matrix repairs but its diff is worklog.md-only (the cited line numbers have since drifted to :3351–3360); 9b4a9302 says "No code change rides this commit" yet renames `last_good_first_cluster`→`last_good_first` in the FAT ABI header + engine. Any doc that trusts commit messages (worklog, ledger "verified by" fields) inherits these lies.
4. **Stale counter name in trail docs**: `orphan_admitted` (15f51121's M-152 counter) is gone from code — HEAD uses `orphan_inodes` (cr_ext4_engine.h:44, engine :1328) — but `findings/FIXED/M-152-…md` (4 hits), `modules/ext4.md` (1) and `FIXED-LEDGER.md` (2) still reference `orphan_admitted`. Twin landings 15f51121 vs 18012c1a are the cause.
5. **SUMMARY.md (14 touches) is not in the known-marker set** — 567694e7 already had to re-count the board once ("the racing block's 18 M / 11 N / 170 FIXED figures corrected per the filesystem-is-the-truth convention"); with 25 fix landings since, its ACTIVE/FIXED counts and per-finding statuses are the likeliest silent drift in the trail.
6. **The A-ABI-3 count class recurs per-append** (497b1277): two racing ABI appends (af5987d0 + b2bc1970) left five doc count claims at 89 vs 91. Counts verified consistent at HEAD (91 exports derivable, docs claim 91/45/12) — but any ABI append landed after 497b1277 without re-running the gate re-REDs it. Check API.md:95, CORE-API-CURRENT.md:14/15/43, API-SURFACE-AUDIT.md:32 against any post-497b1277 export.
7. **Marker asymmetry = hidden remnants**: PARITY-DELTAS has 2 `<<<<<<<` but 3 `>>>>>>>` (orphan end at :4743); FIXED-LEDGER has 1 start / 2 ends (inline `>>>>>>> fd6d63d1` at :635). The N-210 lint greps `^<<<<<<<` only — worklog.md:12063 documents this blind spot itself. Expect `=======`/`>>>>>>>`-only remnants in other high-churn trail files (SUMMARY.md, modules/*.md, findings/).
8. **Reconstructed tests may not match their findings**: f18ba226/1d7d31a6 rebuilt the M-151/A-016 test bodies "to semantic intent" after the 27bda40e tearing; c7037bf3 kept BOTH generations complete. Cross-check tests/unit/test_ntfs_volume.cpp pin names/asserts against findings/FIXED/M-151 and A-016 Resolution text.
9. **Stale impl comment the gates can't see**: cr_analysers.cpp:5144–5146 still says the CIFF heap walk "is the recorded follow-up; the DWG-pattern bounded extent stands in" directly above 0cf6142d's new comment declaring "The old blind 64 MiB stand-in and its comment are GONE" — the claimed comment removal never happened. doc_truth_gate scans headers + rrecov.cpp only; `src/core/**.cpp` comments are an ungated drift surface (same class as N-211's original finding).
10. **Racing-lane volume itself**: 6 author identities, 50 commits in ~1h50m at the densest stretch, 12 rebase mentions, 12 concessions — the highest-risk files are exactly the hot list above; anything touched by BOTH `parity-fix-lane` and `Z User`/`core-recovery-fix-agent` within minutes (M-152, N-213, N-145, S-068, M-151, M-107-X3F, M-154) carries twin-landing residue.
11. **Un-gated pushes**: every red-main event (ed501caa, 27bda40e, N-214 push, count-RED) came from a lane pushing without win_syntax_check/doc_truth_gate/N-210. The gates exist locally but nothing in `.github/workflows/` (untouched in the window) enforces them at push time — the recurrence mechanism is structural, not accidental.
12. **build2 hygiene**: 113a5627 accidentally committed 802 build files alongside a real rider; 9ddc2bbe purged them 27s later. No tree damage remains, but it demonstrates commits reaching main without any pre-push review — the same mechanism as the marker pushes.
