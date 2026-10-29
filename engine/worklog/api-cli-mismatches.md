# API↔CLI mismatch sheet — Session-84 (T3/T4/T5)

The cross-check of the CLI surface, the ABI surface, their documented
contracts, and the research corpus's reference behavior, at Phase-0
baseline `e8dcaba8` with re-verification at each landing through
`d796f9df`. Every row carries file:line evidence. Status reflects the
tree at the last re-verification (see `recent-commits.md` for the racing
windows that landed mid-session).

## Method notes

- The CLI (`rrecov`) deliberately links **cr_core + cr_win only** — zero
  `cr_ui_*` call sites (the no-second-singleton rule; the comment at
  src/app/rrecov.cpp:860-868 states it: linking cr_ui.dll would give the
  process two engine copies and a --user-signs install would land in the
  copy the exe never reads). The API↔CLI cross-check therefore runs over
  (a) CLI→core C++ surface, (b) ABI(cr_ui)→core, (c) CLI docs vs CLI
  code, (d) ABI docs vs ABI code, (e) both vs the corpus formats.
- The corpus (easeus-drw-research) is consulted where the repo's own
  PARITY-DELTAS/CROSS-REF-MATRIX rows cite it; no undocumented divergence
  was found this session beyond those already registered (D-1xx family),
  which the gate and trail already track.

## FIXED this session (with evidence)

| # | Defect | Evidence (both sides) | Fix SHA |
|---|--------|----------------------|---------|
| M1 | `image --create` documented at 5 sites + usage header rrecov.cpp:43 + -h :3858 + Run-Benchmark.ps1:184 + CI-SCENARIOS.md:30 + DRIVER-REQUIRED.md:38 + PLAN-API-EXPANSION.md:64 + REFERENCE-API-GAP.md:336/:435, but `cmd_image` never parsed it — "unknown image option: --create" exit 1 (rrecov.cpp:2874 pre-fix); the flag never parsed in ANY tree including the S-040 landing 13fe7258 itself | code: cmd_image parse loop (only --disk/--out); docs: the 7 sites above; CI: test-benchmark lane's image-create probe cannot pass | `32441353` (has_flag acceptance + the CrUi.CliImageCreateFlagParses pin) |
| M2 | Run-Benchmark.ps1:152 `$profile` collides case-insensitively with the `[ValidateSet('smoke','full')] $Profile` param (:50) — CRBIG's `follow-run-profile` sentinel (benchmark-matrix.json) threw MetadataError; worked in run 6 only because the prior volume held the same value | log: worklog/ci-failures/test-benchmark-6.txt line 441; code: the seed loop | `8daf75dc` (rename to $seedProfile at :152 and :402) |
| M3 | Run-Benchmark.ps1 carve-parity probe: `list-carve-formats` emits the count line + one line per format (rrecov.cpp:876 + the loop after) = 240-line array; array-LHS `-match` filters and never populates `$Matches` → "Cannot index into a null array" → "carve-registry parity failed: CLI= API=239" | log: test-benchmark-6.txt lines 454 + the `##[error]` line; code: cmd_list_carve_formats output shape | `8daf75dc` (rode the same commit as M2 through a staging misfire — recorded in worklog.md; both fixes real, both evidenced) |
| M4 | checks.yml lint job RED: 10 clang-format violations (cr_ui.cpp:2427-2429/:3035-3036, cr_health.cpp:267-270, win-stub/windows.h:308) — the racing 8fe3542f sweep missed these 3 files | log: worklog/ci-failures/checks-281-lint-job.txt lines 128-138 | `9b921ef8` |
| M5 | The error-code census: doc_truth_gate's file-wide `0x5[0-9a-fA-F]{7}` regex counted the truncated prefixes (0x50022000/0x5002c000) of the two 13-digit og-corpus comment literals at cr_error.h:6 → derived 45 vs the enum's real 43 ERR_ members; CORE-API-CURRENT.md:18/:43 carried "45 + OK" | code: cr_error.h enum census (43 members, extras = the two comment prefixes — derivation script output in worklog); gate: tools/doc_truth_gate.py derive_abi_counts() | `7a6f6bb3` (enum-member-anchored census + both doc sites → 43+OK; gate green) |
| M6 | The N-125 exit-5 disconnect hint named a nonexistent verb: "rrecov resume %ls" (rrecov.cpp:2092) — no `resume` verb in the dispatch; the real path is `scan --resume` (parsed :1559, stated by the sibling message :1921) | code: dispatch table vs :2092 | `d796f9df` (one-line message correctness + CrUi.CliDisconnectHintNamesRealVerb pin) |

## FIXED by racing windows mid-session (verified here)

| # | Defect | Fixed by | Verified |
|---|--------|----------|----------|
| R1 | Phantom export `cr_ui_session_resume_last` — declared cr_ui.h:927 (full M-154 contract) with zero implementation in src/** at baseline; never implemented on any branch at Phase-0; DLL shipped 90 exports vs 91 declared | `a47419c7` (the M-154 pointed-resume ABI half) | impl at src/ui/cr_ui.cpp:3699; `extern "C" CR_UI_API` census in cr_ui.cpp = 91 (matches header 94−3); tests pin BAD_PARAMETER/ERR_OPEN_FAILED (0x50020001) dead-pointer classes at test_cr_ui.cpp:5180-5222 |
| R2 | Exit code 5 (scan disconnect, rrecov.cpp:2094) absent from API.md's exit table | `736f9a9e` | docs/API.md:67 documents 5 with the N-125 semantics |
| R3 | 5 markdown files carried committed conflict markers (the M-120/N-210 class): worklog.md:12069-12168, CROSS-REF-MATRIX.md:39-46, FIXED-LEDGER.md:656-669, I-048 FIXED file:52-62, PARITY-DELTAS.md:4677-4863 (nested) | N-219 repairs (racing lane, 15c59ee7 + follow-ups) | `git grep '^(<<<<<<< \|>>>>>>> )'` → only the sanctioned M-120 evidence file remains |
| R4 | D-152 triple-claimed in PARITY-DELTAS.md (### :4218 + ## :4656 + ## :4678) | racing seat-renumber repair | `grep -oE '^#{2,3} D-[0-9]+' | uniq -d` → only D-52 (pre-existing, see F6) |

## OPEN — doc-side drift (Phase 5 fixes; doc wrong, code right)

| # | Drift | Evidence |
|---|-------|----------|
| D1 | API.md:97 + API-SURFACE-AUDIT.md:33: "92 `CR_UI_API` lines − 3 #defines" — the tree has 94 lines; the derivation was supposed to be corrected by 497b1277's count repair but both sites still read 92 (the repair landed the "91 exports" headline but not the line-count derivation) | include/cr/cr_ui.h grep -c CR_UI_API = 94 |
| D2 | ABI-CONTRACT.md:506: "`rrecov --list-carve-formats`" — leading-dash typo; the verb is `rrecov list-carve-formats` | docs/ABI-CONTRACT.md:506 vs dispatch |
| D3 | cr_ui.h:1021-1023 documents `ERR_NOT_FOUND` for `cr_ui_session_last_autosave_path` — **no ERR_NOT_FOUND exists in the enum**; the implementation returns ERR_RANGE_OUT_OF_BOUNDS (0x50020004, cr_ui.cpp:3647) and the test pins 0x50020004 (test_cr_ui.cpp:5189-5190 region) | include/cr/cr_error.h (no such member); cr_ui.cpp:3647 |
| D4 | Journal doc claims: "rrecov-apply.ifj beside the undo log, probed at every startup" (PARITY-DELTAS.md:4226, modules/apply.md:107, CROSS-REF-MATRIX.md:21) — the code's default journal is `rrecov-apply-inflight.jrn` (or `<in>.inflight`), probed only inside the apply flow (rrecov.cpp:3361/:3368; undo-apply at :3612-3618); no startup probe exists | rrecov.cpp:3361/:3368 vs the three doc sites |
| D5 | CORE-API-CURRENT.md:14-16: "91 `extern "C" CR_UI_API` definitions in src/ui/cr_ui.cpp, recount matches" — now TRUE post-a47419c7 (91), but the same line's derivation "94 lines − 3 = 91" should be stated once, consistently with API.md/API-SURFACE-AUDIT (D1) | cr_ui.cpp census = 91 |
| D6 | ABI-CONTRACT.md §6's `cr_ui_event` copy (:260-307) missing the `autosave_rc` field that cr_ui.h:269-271 carries — contradicting its own §7h | ABI-CONTRACT.md §6 vs include/cr/cr_ui.h:269-271 |
| D7 | BUILD-SUPPRESSIONS.md:7 "This file intentionally carries ZERO entries" vs row S-1 at :19 (cr_thumbnail.cpp stb block) — the header's claim is false about its own file | docs/BUILD-SUPPRESSIONS.md:7/:19 |
| D8 | error-fix-registry.md:19 F13 "no decryption code at all" — false since M-033: XTS-AES in-stack decrypt (src/core/crypto/cr_aes.cpp, include/cr/crypto/cr_aes.h) + `--bitlocker-fvek` CLI flag (rrecov.cpp:16/:670/:688) | the code cites |
| D9 | architecture.md:48 "no OS headers" (in core) — cr_lastscan.cpp:12 includes windows.h under `#ifdef _WIN32` (M-154's landing) | src/core/session/cr_lastscan.cpp:11-14 |
| D10 | CI-CURRENT.md:54/:153 cite `tools/gen_fat16_junk_fixture.py` + `fat16-junk.img` — the real pair is `tools/gen_junk_gate_fixture.py` + `tests/data/images/junk-gate-fat16.raw` (checks.yml references the real ones) | tools/ dir listing; gen_junk_gate_fixture.py:4/:16 |
| D11 | CONTRIBUTING.md CI section still describes the deleted local-mirror/TSan/UBSan/fuzz CI matrix lanes; checks.yml has no local-mirror job (4 windows-2022-only workflows) | CONTRIBUTING.md:13-64 vs .github/workflows/ |
| D12 | PARAMETERS.md: scan `--part` (fix-only), bench `--no-rda` (removed surface), `undo` as a verb; missing sessions/driver/image/archive-list + `--filter-folder`/`--autosave-dir`/`--pause-stdin` | docs/PARAMETERS.md vs rrecov.cpp flags |
| D13 | GAP-P1-1 status disagreement: CORE-GAP-LIST.md:42 CLOSED (464d8e5, LZNT1 landed — verified: include/cr/compress/cr_decomp.h:32 kLznt1) vs README.md:118 "remaining gaps: … GAP-P1-1" and CAPABILITY-MATRIX.md:33 "not implemented / MISSING" | all three sites + the code |
| D14 | `-h` block omits 6 live scan flags (--autosave-dir/--label/--pause-stdin/--user-signs/--bitlocker-dict/--check) and omits image+archive-list from the driver-gated list (per the S-040 landing's own four-site-lock intent) | rrecov.cpp -h block vs the scan parse loop |
| D15 | docs/API.md verb table lists 11 verbs; the dispatch carries 13 (archive-list, image missing from the table; exit-5 now documented by R2) | docs/API.md:27-48 vs dispatch :3914 region |

## OPEN — behavior-adjacent (filed, not fixed — see api-findings.md)

- F1 (closed by R1): phantom export — recorded for the history.
- F2: enumerate_volumes rc dropped at rrecov.cpp:819 (print_disks) vs the
  file's own FB-K01 principle that enumeration failure is never exit-0 —
  fixing changes observable output; filed.
- F3: N-145 "every startup" journal probe absent (apply-scoped only) — the
  commit d7856484 message and three doc sites claim an every-startup probe;
  adding one changes CLI startup behavior; filed.
- F4: fuzz campaign drift (fuzz/Makefile): `fuzz_reghive` built but never
  run; `run-long` drops `fuzz_image`; corpus mkdir misses `corpus/thumbnail`
  — script-only, but changes what the fuzz campaign verifies; filed for the
  tooling decision.
- F5: D-52 flagged by the census regex — VERIFIED FALSE on first-hand read
  (:1684 `### D-52` + :4340 `### D-52 ADDENDUM` is one seat plus its
  addendum, not a collision). Recorded so the next session doesn't re-chase.
