# worklog.md — the live multi-agent session log

**Archive note:** the 2026-10-28 history bulk (17,690 lines, sessions 1
through the diff-fix waves) was removed from main via PR #13 (980d09c /
92bdff1); every entry remains retrievable from git history
(`git log --diff-filter=A -- worklog.md`). This file is the LIVE log —
each agent session appends its Task-ID entry here per the coordination
contract (handoffs, boundary items, convergence records).


---
Task ID: SCAN-2e-recon (Agent 2 — the S-090/A-011 + N-089 convergent-landing reconciliation)
Agent: Agent 2 (scan-engine lane)

Work Log:
- Collision resolved: the probe lane's A-011+N-089 landing (c899998)
  and this lane's unpushed S-090+N-089 fix (042f363, discarded) fixed
  the SAME two defects discovered independently in the same window
  (this lane's instrument: its first spec-correct BE SQLite fixtures
  went RED; the debug trail isolated the LE reads, then the
  `2ull << 30 / page_size` precedence parse = the 2-page cap).
- The landed A-011 decode supersedes this lane's draft on every
  dimension (the +56 text-encoding gate, the RetDec-export Analyse
  entry FUN_18002e2b0, the encoding-aware marker forms, the 4-page
  scan, the note-propagation mechanism) — no code kept from this
  lane's draft; the draft's S-090 finding file dropped (A-011 stands).
- Landed as THIS lane's reconciliation: the A-011 convergence record
  (the complementary Ghidra-side FUN_18002dfc1 :35404-35432 name-gate
  decode + the second-stage isolation fail-before: fixes held, subtype
  neutralized -> only the FCP-label test RED — the label-only proof
  N-089's severity rests on) + the SCAN-ENGINE-DELTA F2b row (A-011
  form) + this entry. The probe lane's OQ-SA-2 file already carries
  this lane's convergent resolutions (their :1463221 merge note).
- Verified on the landed state: full suite 932 passed / 0 failed
  (linux-release).

Stage Summary:
- The convergent landing reconciled with zero duplication: one code
  path (A-011's), one finding id per defect, both lanes' evidence
  merged into the record. The A2 session's net queue after this:
  OQ-SA-1's NTFS +0x64 half (narrowed probe), OQ-S4-3's TTFR half
  (execution-bound), the S-007 remainder. SCAN-ENGINE-DELTA.md is
  current with the board.

---
Task ID: API-FIX (the api-fix session — context recovery + the driver-era ABI drift batch + three CI-red repairs)
Agent: main engineer (API-focused remediation)

Work Log:
- Context recovery per protocol: three parallel read-legs (corpus /
  core / driver) produced the API ground-truth pack
  (/home/z/my-project/api-ctx/*.md, path:line-cited); the consolidated
  45-row present|inconsistent|missing|superset delta board is
  committed as diff-to-fix/API-DELTA.md.
- API-FIX-0 (1117ac9): repaired the two CI-red breakages the parallel
  lanes left (MSVC C4244 in the FH11 extent check -> integer drift
  check; clang-format violations in 9 files). Verified green on the CI
  lane's exact config (gcc-13+ASan; a matching local toolchain was
  bootstrapped under /home/z/gcc13 — gcc-14 mis-compares warnings).
- API-FIX-1..3 (c169332): the driver-era ABI-contract drift batch —
  the phantom-zero extended-IOCTL literal tail swept from 12 sites
  (0x6000244x -> 0x600244x, 3bda8e2's residue); cr_ui.h's phantom
  CR_UI_ERR_INVALID_PARAM and the unreachable "direct" transport
  value removed; ABI-CONTRACT §4 gains the ERR_DRIVER_* 0x500B
  family, §6 gains CR_EVT_FS_CHECK(10) + the M-041 struct tail, §7e
  rewritten driver-only; API.md 78->79 exports / v3 / 3 missing rows;
  CORE-API-CURRENT SUPERSEDED banner; API-SURFACE-AUDIT headline
  61->79@v3. Four doc-truth pins hold it (test_cr_ui.cpp).
- Format tail (630df04): the OQ-SA-1 lane's test_fat_volume.cpp
  comment alignment (the second format miss caught by local-mirror).
- Double-start repair (605d682): the v0.2.4 rda-driver installer
  (M-064's %s%d batch) starts the service itself; the
  install-driver composite action re-issued sc.exe start and failed
  the staged-scenarios lane on 1056 — the start step is now
  idempotent (RUNNING-query first; any other start failure stays
  fatal per the v0.2.3 exit-code lesson).
- Driver repo state (read-only): rda-driver's own doc drifts (D2 stale
  0x6000244x in docs/architecture.md, D3 stale 15/18-command counts
  vs the 19-code set, D4 invalid hex(7) UpperFilters data) recorded in
  the delta pack — that repo's lane owns them (it has active lanes;
  single-repo commit discipline held here).

Stage Summary:
- The API surface's normative docs now match the driver-only code they
  describe, with 4 regression pins; three CI-red states repaired
  across the session; the remaining API-scope board: S-038 step-2+
  (installer implementation, packaging lane), A-ABI-6 (codeql policy —
  owner call), rda-driver D2/D3/D4 doc drifts. Suites verified green:
  969/969 (my batch), CI success on 1117ac9 + 630df04.

---
Task ID: API-FIX-2 (api-fix session 2 — the post-A-011 CLI/CRSF doc truth pass)
Agent: api-fix lane (this session)

Work Log:
- Context-recovery protocol re-run, API scope only: three parallel
  read legs (corpus / core / driver) -> the session-2 delta board in
  diff-to-fix/API-DELTA.md §4 (Session 2).
- Session-1 board re-verified first-hand: driver wire byte-exact vs
  rda_ioctl.h (identity 0x60002400/04/08 + extended 0x6002440/44/48 +
  all 19 session codes + F11 x512); 79 cr_ui exports recounted;
  39+OK error codes recounted; CLI dispatch = 10 verbs (code truth).
- Rebase reconciliation: the remote lanes landed M-064 FULLY FIXED
  upstream (rda-driver v0.2.4 323e75a: -Layout Versioned %s%d
  coexistence + -Uninstall verb + versioned-aware teardown + PS AST
  CI gate) and the A-011/N-089 convergence (fc74677) while this
  session verified — the board row reconciled; core's driver-status
  discovery confirmed unaffected (the service carries the binPath).
- Four NEW verified drift items (all doc-vs-code, zero code defects):
  A: docs/crsf-format.md omits REC_FS_CHECK type 9 (M-041) and its
  payload grammar; B: the rrecov.cpp header comment lists 8 of 10
  verbs + misses scan --image/--deep/--repair-undo/--user-signs and
  recover --image; C: docs/API.md §1 the same 8-of-10 verb gap + a
  phantom [plan] value on --repair (bare flag at rrecov.cpp:1089) +
  a duplicate exit-code row 3; D: docs/CORE-API-CURRENT.md §c says
  9 verbs (omits driver, stale wmain cite) and c.7/c.8/c.9 still
  document the REMOVED --no-rda flag + the "direct" transport label
  (only "rda" exists, rrecov.cpp:1992) + stale function-range cites.
- Fixes A-D landing as separate commits (the three-site verb-list
  pin: docs/API.md + CORE-API-CURRENT §c + the rrecov.cpp header
  comment must move together from now on).

Stage Summary:
- The API lane's remaining board after this session: S-038 steps 2-4
  (packaging lane, owner decisions pending), A-ABI-6 (owner policy),
  OQ-SA-1/OQ-S4-3 (engine-scope probes). Everything API-surface-shaped
  is now doc-consistent with the code at the tree this session pins.

---
Task ID: API-FIX-2 (landing record — the four fix commits)
Agent: api-fix lane (this session)

Fix landings:
- A `5c3fb32`: docs/crsf-format.md — REC_FS_CHECK type 9 row + the full
  FS_CHECK payload grammar (fields verified against serialize/deserialize
  _fs_check at src/core/check/cr_fscheck.cpp:1009-1089 + the CHK_* enum
  0x1001..0x100E at cr_fscheck.h:80-97).
- B+C `3c9a747`: the three-site CLI truth pass, first two sites —
  rrecov.cpp header comment (10 verbs in dispatch order + the complete
  scan flag surface + the full exit-code map; comment-only, parse-safe)
  and docs/API.md §1 (both missing verb lines, scan --image/--deep/
  --repair-undo, the phantom [plan] value removed, recover --image,
  --boot-fix added to the consent list per its M-040 shared-apply gate,
  duplicate exit row 3 collapsed).
- D `8ff17fc`: docs/CORE-API-CURRENT.md — §c 10-verb truth + the stderr
  exception (require_driver is the only fwprintf(stderr) surface) + all
  function-range cites re-pinned + REMOVED --no-rda dropped + the
  rda-only transport label; the fix grew into the §a/§b/§d refresh
  (phantom prefer_rda signature, deleted rda_available/win_pe_environment
  rows, 51 stale cr_ui.h cites mechanically re-pinned; the header now
  pins the live-section cite refresh, §e/§f keep their 22e9657-era
  cites under the banner).
- Board: diff-to-fix/API-DELTA.md §4 items A-D flipped FIXED with the
  commit refs; the three-site verb-list pin stands
  (CliVerbListsAreThreeSiteLocked).

Verification: ci-local doc-links / architecture lint (the driver-only
blocklist — which independently confirms the rda_available /
win_pe_environment / prefer_rda removals this session's §a edits made) /
secrets / commit-convention lanes all green; cmake + clang-format are
absent in this container so the build/test lanes could not run locally —
the only code-file change is comment-only and parse-verified (the
windows.h include is the sole expected parse stop on Linux).

---
Task ID: API-FIX-4 (the M-070 convergence note — this lane's diagnostic half)
Agent: main engineer (api-fix session)

Work Log:
- Reproduced the 40673882330 failure shape end-to-end: staged-truth.json's
  "manifest_version": null + "expected_recoverable": null (the if/else
  @() unwrap to $null, proven in pwsh 7.4.6) + 0.02 s seed-window =
  $manifest was null-or-string on the runner; every static reproduction
  of the invocation (hashtable splat, relative path, LF+CRLF, dot-source
  topology) parsed 11 files, isolating the cause to the script body.
- The api-fix-2 lane landed the mechanism (89dd7ad): `$manifest` is
  `$Manifest` case-insensitively - the SAME variable as the [string]
  param, whose type constraint coerced the parsed PSCustomObject back to
  a STRING on assignment (.files/.version -> null). My loud-gate variant
  (47c7ee5, discarded in the rebase) was the complementary guard; the
  rename is the root fix and the lane's finding file is canonical.
- This lane's residual contribution: the mechanism-isolation evidence
  (the null-vs-empty output-shape distinction + the empty-string
  ConvertFrom-Json silently-yields-null proof) now lives in the finding
  trail via this entry.

Stage Summary:
- M-070 closed by the api-fix-2 lane's rename (89dd7ad); staged-scenarios
  should now stage 11 files - the lane's next dispatch is the proof.

---
Task ID: PARITY-FIX-1 (senior-dev session — the recall-gap register truth pass + GAP-S6-5's scorer action)
Agent: main session (25+yr senior-dev persona, full 3-repo context recovery done)

Work Log:
- Context recovery protocol executed: 6 parallel agents over the research
  corpus (architecture docs / artifacts / og-codebase / reversed+raw-evidence),
  core_recovery (full tree + board), rda-driver (all 66 files). Board at
  session start: 1 active SEVERE (S-038 steps 2-4) + 2 tracked-open
  (OQ-S4-3, OQ-SA-1) + I-026 informational.
- Verified bench/root-causes.json (filed 2026-10-24) against current code:
  ALL SEVEN engine-side recall gaps are already fixed in code with tests
  (GAP-S6-1/2 = 9fc82b0; S6-3/4 = 7797bd6; S6-6/7/8 = 03ddcd2 + F-1 cd9ea16)
  - the register, CORE-GAP-LIST.md rows and PERFORMANCE-PROFILE.md rows
  were all STALE (predate the landings).
- GAP-S6-5's action landed (the only open item): the physical-ceiling
  metric. volume.ps1 gains the HashRanges composite C# op (one locked
  pass, per-file extent-range SHA-256, short-read honesty) +
  Invoke-PhysicalCeilingProbe; apply-scenario's decay branch emits
  physical-ceiling.json (seed-time extents are valid post-decay addresses
  - deletion never moves bytes); the cluster-size resolution is extracted
  to Resolve-ClusterBytes shared by partial_overwrite + decay (logic
  byte-identical, BENCH-F4c/F5c lessons kept); compute_metrics --ceiling
  adds the physical_ceiling block (compute_ceiling(), recall judged
  against the physically-intact set); compare.py surfaces the ceiling on
  BELOW rows; tools/test_ceiling_metric.py (5 cases, 16 assertions) wired
  into ci-local step 5.
- Truth pass: root-causes.json every entry now carries a resolution field
  with the landing commit (original evidence kept verbatim; the decay
  entry carries GAP-S6-5's root-cause CORRECTION - physical destruction,
  not an engine defect); CORE-GAP-LIST.md S6-1..S6-8 all flipped CLOSED
  with commit refs; PERFORMANCE-PROFILE.md rows annotated fixed-in-code
  (numbers stand until the next bench dispatch re-measures).

Stage Summary:
- The engine-side recall-gap register is fully closed in code AND the
  registers now tell the truth. Remaining session targets: S-038 steps
  2-4 (installer, per PACKAGING.md defaults), M-068 board hygiene move,
  testing.md count truth, OQ-SA-1 NTFS +0x64 decompile probe, OQ-S4-3
  TTFR probe target.
---
Task ID: SCAN-2f (Agent 2 — scan-engine lane: the context-recovery re-run + the OQ-SA-1 full closure + the S-007 niche batch)
Agent: Agent 2 (scan-engine lane)

Work Log:
- Context recovery per protocol: FOUR parallel read legs (corpus
  carve-family census/dual-registry; corpus NTFS decompile trace; core
  carve-state audit; rda-driver scan-surface audit). SCAN-ENGINE-DELTA.md
  re-verified + refreshed at this tree (A4 count reconciliation: 131
  roster rows = 106 analysers + 7 FileInfo classes + 18 infra; the
  "67 of 131" residue figure replaced by the precise ~30-format work
  list; N-068/N-069/N-070/S-032/S-037 sub-items re-verified FIXED; F2's
  D-104 registration recorded; C9's probe-target correction folded in).
- OQ-SA-1 NTFS +0x64 — DECODED, instruction-decisive, first-hand: the
  low dword of the FIRST $DATA extent's starting LCN (NOT parent-ref,
  NOT own record number). Producer chain: FUN_1800069e0 :5126-5127
  (src+0x53 <- first pair start from FUN_1800336f0 :41028; src+0x4f <-
  Sum pair seconds = total clusters) -> FUN_180003020 :1986-1988
  (dst+0x64/dst+0x60); the $FILE_NAME path FUN_18002a340 :33912 ZEROES
  the slot (local_39 = src+0x4f/+0x53 as one u64). Engine-side +0x60 =
  total data-run clusters (the "+0x60 type-registry id" census reading
  is the DRWUI wire-side overwrite, part1:86). The FAT +0x3e / exFAT
  +0x50 / NTFS +0x64 anchor triple closes; .rsf persists +0x64
  immediately before the type id (DRWUI_decompiled.c:853462-853464).
  Core carrier + nflags bit-8 wire + fail-before tests = the F-lane fix
  this session; corpus erratum row queued (part1:85 "secondary ID").
- Driver leg (read-only): rda-driver v0.2.4 wire byte-stable (identity
  0x60002400/04/08 + extended 0x6002440/44/48 + all 19 session codes
  re-verified; F11 x512; NAS/Cloud exclusion holds). Driver-repo D3/D4
  doc drifts + the FileVersion-1.0.0.0-vs-tag note recorded as handoffs
  in SCAN-ENGINE-DELTA.md (single-repo commit discipline held).
- handoff: PARITY-FIX-1 lane — your "remaining session targets" list
  carries "OQ-SA-1 NTFS +0x64 decompile probe": that probe is RESOLVED
  by this lane (the decode above; the Delta Report Q11 row + the
  tracked-open file update land with this lane's F-lane fix). No
  further +0x64 work needed; OQ-S4-3's TTFR half stays
  execution-bound (the probe-target correction 97cc3b2 governs).

Stage Summary:
- Scan-engine board after context recovery: quick lane clean (zero open
  defects); A-lane work = the S-007 niche batch (keyed set, corpus-verified
  magics, batch-1 scope pinned in the queue); C-lane clean except the
  execution-bound TTFR half; F-lane work = the NTFS origin-LCN carrier.
  Fixes land next in the mandated order: A-lane batch -> F-lane carrier.


---
Task ID: PARITY-FIX-2 (S-038 steps 2-3 — the app installer + the three gates)
Agent: main session (senior-dev parity-fix continuation)

Work Log:
- The two PACKAGING.md owner decisions closed at the contract's own
  documented defaults (the owner may overturn either by reopening the row):
  section 2 = Inno Setup (evidence-pinned, the 13-parameter contract maps
  1:1), section 8 = omit (no telemetry endpoint; no stub verbs).
- installer/core-recovery.iss: the 13-parameter implementation. Parameters
  1-5/7/10 native Inno; parameter 6 the admin manifest; parameter 9 the
  frozen single-instance AppId; parameters 11-12 intentionally OMIT (the
  recorded decision); parameter 8 (ARP QuietUninstallString, the reference's
  exact ' /SILENT' form) + parameter 13 (the version key at
  HKLM\SOFTWARE\heicfast\core_recovery\Version) written AND read-back-verified
  by [Code] (the M-064 write-and-hope lesson); the version key's removal
  owned by the uninstaller's [Code] hook (Inno only uninstalls [Registry]
  entries). OS gate (section 6 modernized envelope, Win7 SP1 -> Server 2025,
  x64-only) in InitializeSetup; driver deployer (section 10) as the default
  'driver' component - install.ps1 stays the SCM source of truth, exit codes
  propagate (failed driver install fails the app install), teardown before
  file deletion.
- Caught + fixed pre-push: the [Code] ARP-key string must use the raw GUID
  (AppIdRaw define) - the {{-escaped AppId define would build a wrong
  registry key.
- installer/osgate.ps1: the shared decision module (injectable version
  tuples; the OSGATE-V1 marker is the sync pin); installer/run-gates.ps1:
  gates 1+2 executable (silent round trip + version key + no-residue:
  files/ARP/version-key/service/driver-image sweep); installer/
  build-installer.ps1: payload staging + ISCC; installer/README.md.
- tools/test_installer_osgate.py: gate 3 - the sync-pin check (comment-style
  agnostic) + the injectable matrix (9 in-envelope ADMITS incl. Win7 SP1
  floor + Server 2025, 5 below-floor REFUSES incl. Win7 RTM/SP0, 3
  wrong-arch REFUSES incl. ARM64/Itanium).
- .github/workflows/installer.yml: the gate lane (gate 3 fast job + PS AST
  parse; the full build -> fetch driver -> job-local test-sign -> ISCC ->
  round-trip job on windows-2022, Inno pinned 6.4.3 via choco). The
  installer PS files joined scripts/ps_parsecheck.ps1.
- PACKAGING.md: status -> IMPLEMENTED; sections 2/7/8/12 record the closed
  decisions + the registered version-key path.
- S-038 finding: steps 2-3 Resolution appended; step 4 (acceptance) flips
  when the lane's first dispatch is green.

Stage Summary:
- S-038 steps 2-3 landed (11 files). Next: dispatch installer.yml and watch
  the three gates; then the M-068 hygiene move + testing.md count truth +
  OQ-SA-1 NTFS +0x64 decompile probe.

<<<<<<< HEAD

---
Task ID: PARITY-FIX-3 (CI red repair + the installer-lane YAML fix)
Agent: main session

Work Log:
- installer.yml: the step name carried an unquoted colon ('the v0.2.3
  lesson: a broken script...') - YAML mapping-value error, zero jobs, the
  lane's first dispatch died instantly (run 40682041579). Quoted.
- CI red repair (run 40680931476, three lanes red on the parallel
  thumbnail landing 05e6786 - not this session's code, repaired per the
  repo's ci-restore-green precedent):
  (1) windows/local-mirror: cr_thumbnail.cpp:479 MSVC C4244-as-error -
  the TIFF bps loop narrowed vector<u64> elements into a u32 range-for
  var; now compares in u64 and narrows once at the assignment with the
  8/16 refusal gate as the safety rationale;
  (2) fuzz: cr_thumbnail.cpp:36 - '-Wmaybe-uninitialized' is a GCC-only
  warning group; under clang -Werror it fails as unknown-warning-option;
  now guarded by __GNUC__ && !__clang__ (clang's equivalent lives under
  -Wconditional-uninitialized; the vendored-stb pragma region keeps the
  rest).
- g++ -fsyntax-only -Wall -Wextra -Wpedantic -Wshadow -Werror: the fixed
  file is clean.

Stage Summary:
- Pushing the repair + re-dispatching the installer lane.

---
Task ID: PARITY-FIX-4 (board hygiene: the M-068 move + testing.md count truth)
Agent: main session

Work Log:
- M-068 moved findings/MAJOR/ -> findings/FIXED/ (git mv, history kept):
  the fix landed in the SA-6 wave (43f4962 + 38ccecb) but the file never
  moved; Resolution section appended; RESOLVED-INDEX row added.
- SUMMARY.md: the PARITY-FIX-3/4 delta entry on top (board: 245 FIXED, 1
  active S-038 awaiting the lane's green dispatch, 2 tracked-open).
- docs/testing.md: the stale '926 tests' -> the current truth (1,014
  TEST/TEST_F macros across tests/unit, 61 files; latest executed green
  1,003/1,003 release+ASan at the thumbnail landing 05e6786).

Stage Summary:
- Board hygiene closed. Watching the installer lane (run 40682340640) and
  the CI validation lane (run 40682340755); next: OQ-SA-1's engine-side
  NTFS origin-LCN carrier if the SCAN-2f lane has not landed it.

---
Task ID: API-EXP-1 (api-fix session 3 — the corpus-parity expansion: context recovery + the plan-of-record board)
Agent: api-exp lane (this session)

Work Log:
- Context-recovery protocol run 3, API scope only: three parallel read
  legs (corpus be0cfe9 / core 89dd7ad / driver 323e75a) -> the session-3
  board committed as diff-to-fix/API-DELTA.md §5.
- Standing state re-verified first-hand: 79 exports / v3 / 39+OK / 10
  verbs all stand; driver wire byte-exact (the SAME 19 session-code
  values both sides — the driver worklog's "18-code" phrasing counts the
  dispatch set minus the AC0 discovery probe; reconciled in the board).
- Reconciliation: the S-036 fail-loud + FloorVersion close-out the rda
  driver worklog names as "core_recovery-side" is ALREADY landed here
  (fetch-release.ps1 semver floor gate :59-65, install-driver composite
  default fail-loud + FloorVersion v0.2.3, release/staged floors) — the
  driver lane's line is stale, recorded for its next sync. A-ABI-6's
  RELEASE-NOTES half is gone (no CodeQL claim at this tree); the stale
  half is the audit row itself (S3-1). M-068: the hygiene move was
  landed by the board-hygiene lane at this rebase (2a556e5 — the FIXED/
  move + the Resolution + the RESOLVED-INDEX row); folded, no action.
- Rebase note: while this session filed, the S-038 lane landed steps 2-3
  (3e6a8fa, the Inno installer + the three test gates) and SCAN-2f
  decoded the OQ-SA-1 NTFS +0x64 half (7bda022) — both folded into this
  lane's board (API-DELTA §5.1) at the rebase.
- A fresh local verification lane was bootstrapped in this container
  (cmake 4.4.4 via pip, gtest 1.15.2 FetchContent, gcc 14.2.0 Release,
  clang-format 18.1.8): full suite 977/977 green at the base tree.
- The remaining corpus-parity API work re-based onto the plan of record:
  PLAN-API-EXPANSION Waves 1-5 = 12 ops (S3-W1a..S3-W5a), every one
  verified ABSENT first-hand at 89dd7ad. This session executes them in
  the plan's landing order (W1a session-list first), each op = C++ + ABI
  + CLI + contract text + tests, read-on-demand corpus verification per
  op, committing every 2-3 files.

Stage Summary:
- Board filed (rebase-adjusted); execution starts at W1a (R-56 session
  list + summary). This lane's queue: S3-1 (the A-ABI-6 audit-row truth
  pass) then Waves 1-5; NOT touching S-038's installer lane, SCAN-2f's
  engine probes, or API-FIX-4's CI lane.

