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

Fix landings (same session):
- A-lane S-007 NICHE BATCH 1 LANDED (e018bf4): 27 og-only formats / 31
  registry rows / 26 validators - MID OGG RM MXF DMG(BZh1) X3F MRW(x3)
  WPG WPD PSP SIT GZ APE AU AIFF HLP(x3) ICO CUR WMF-PL WMF-STD TTF MDB
  TAR EPS AI DXF 3GP. Every key re-read first-hand at its RAWSearchDLL
  registration site before the row landed; WPG bounds {20, 0x5F5E100} and
  MDB bounds {0x1000, 0xBEBC200} verified at their FileInfo ctors; the
  WMF-placeable checksum convention decoded (the stored word = XOR of
  words 0..8). Three census corrections: errata E49 (3F5F03 = Windows
  Help, not NEF), E50 (DXF is a keyed u32 format), E51/E52 (MDB rides the
  u16 0x0100 chain shared with TTF). Shipped mechanical gates:
  static_assert(kTrialCount == FMT_COUNT) (trial-order membership was
  silently unverified), kMaxTrialSlots 96->160, the DMG footer-scan seam
  made forward-progress-safe (an infinite-loop class caught by the first
  test run). Fail-before proven (validators neutralized: 3/3 positive
  RED; restored 4/4); full suite 941/0 gcc-ASan; hostile ASan smoke
  extended with the walk-heavy niche families, 400 mutated iters, zero
  findings. Batch-2 remainder pinned with causes in the S-007 addendum
  (CHM/MNY/EMF/ONE/MPEG-TS/3DS/SWF/JP2/MOBI + the list-layer text
  school). handoff: Agent 1 - the CLI's list-carve-formats surface
  enumerates the registry, so the 27 new formats surface automatically;
  no API-contract change required on your side (the carve_format values
  are append-only stable).


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

Task ID: PARITY-FIX-5 (installer-lane retry fix + OQ-S4-3's TTFR harness)
Agent: main session

Work Log:
- installer.yml choco step: the windows-2022 image ships Inno Setup 6.7.1
  PREINSTALLED - the pinned 6.4.3 install downgrade-failed (run
  40682340640). Now detects ISCC first, choco unpinned only when absent;
  the 6.4.3 references swept from ISS/build-installer/README/PACKAGING.
  Gate 3 (InstallerOSGateMatrix) was GREEN on that run - the sync pin +
  the full envelope matrix passed on the runner.
- Next: the OQ-S4-3 TTFR harness on the staged-scenarios lane (option 1
  of the 97cc3b2 probe-target correction - no engineering prerequisite,
  the event stream carries the timestamps).

Stage Summary:
- Retrying the round-trip job; TTFR extraction next.


---
Task ID: CI-REBUILD-WIN (the owner-ordered windows-only 3-lane consolidation)
Agent: the CI rebuild session (context recovery protocol, API scope)

Work Log:
- Full context recovery first: 5 parallel explore agents (build system,
  CI autopsy, CLI+API contracts, driver+bench infrastructure, research
  corpus) - every contract re-verified first-hand against the tree before
  a line was written (zero-invention bar).
- WIPED: ci.yml (7 jobs incl. 4 ubuntu lanes), core-install.yml,
  driver-install.yml, staged-scenarios.yml, the old release.yml,
  .github/scripts/{stage-disk.ps1,verify-scenario.ps1},
  .github/scenarios/seed-manifest.json, .github/actions/install-driver
  (the owner directive: windows runners only; the ubuntu sanitizer/fuzz
  lanes are documented drops, coverage stays local via ci-local.sh).
  KEPT: installer.yml (the S-038 lane landed mid-session on the moved
  main - fresh active work, independent of anything wiped).
- LANDED the 3-lane set: checks.yml (lint fast-fail -> build+test with
  vcpkg binary caching + Win7 posture + G-09 + driverless refusal probes
  + the new PS-AST and workflow-YAML parse gates), release.yml (validate
  via workflow_call -> resolve -> build-core + driver-tier -> assemble
  + publish; PERSISTENT dev-cert signing replacing the ephemeral
  job-local cert, public cert shipped as rda-test-sign.cer), and
  test-benchmark.yml (single elevated windows-2022 job: release fetch ->
  driver trust/install/verify -> api_harness.c compiled against the
  release SDK -> New-TestDisk (diskpart-only fixed VHDX, MBR+GPT,
  FAT32/NTFS/exFAT, dual-partition) -> Run-Benchmark orchestrator over
  benchmark-matrix.json: deterministic 23-format corpus, 10 scenarios,
  CLI + API lanes, floor/discovery/observe gates, apply probe,
  cancel+resume probe, bench verb + clean-volume timings) with
  Teardown-Benchmark enforcing the M-061/M-069 contracts and the
  structured 00..99 evidence artifact.
- Secrets installed: DEV_SIGN_CERT_PFX_B64 + DEV_SIGN_CERT_PASSWORD
  (CN=RRECOV-DEV-SIGN, RSA-2048 SHA-256, codeSigning EKU, 3-year).
- Docs reconciled: CI-CURRENT.md rewritten for the new lane set;
  CI-SCENARIOS.md pointed at benchmark-matrix.json; RELEASE-NOTES entry.

Stage Summary:
- Validation pending: the checks dispatch, then a release publish
  (auto next-patch), then the auto-dispatched test-benchmark against it -
  the empirical loop the lane set itself enforces (a rewritten composite
  with zero dispatches is an unverified composite - the M-068 lesson).

Task ID: PARITY-FIX-6 (OQ-S4-3's TTFR harness — the event-stream probe)
Agent: main session

Work Log:
- rrecov scan: TtfrProbe + ttfr_event_fn — ONE log line at the first
  positive FilesFound event (run-elapsed ms from the event's
  pause-compensated timestamp_ms; the honest granularity: counters
  coalesce at stage boundaries). Set before the ScanController
  construction (the controller copies cfg — caught by review); atomic
  once-flag (worker threads); no new CLI flag (the existing --log).
- verify-scenario.ps1: --log engine-log.txt + the ttfr block in the
  evidence JSON + the printed line; the evidence upload covers it.
- OQ-S4-3 tracked-open file: the harness section appended; the
  measurement is the lane's next dispatch (execution-bound).
- Gate trail: win_syntax_check green (it caught two relocation defects
  before the push — the parse gate working as designed); ps_sanity OK.

Stage Summary:
- The last code-side item this session could land without colliding with
  the parallel lanes' pinned queues. Watching: the installer round-trip
  job + the CI lane at 624db00.

---
Task ID: PARITY-FIX-6b (the TTFR extraction port to the rebuilt CI lane)
Agent: main session

Work Log:
- The owner's CI consolidation (566c483) landed mid-flight: checks/release/
  test-benchmark replace the 5-workflow set; staged-scenarios.yml +
  verify-scenario.ps1 are GONE. The rebase: verify-scenario.ps1 deleted
  (their side wins - the new lane owns the harness); the rrecov probe kept
  (harness-agnostic).
- The extraction PORTED to Run-Benchmark.ps1: the per-mode scan block
  (which already ran every scan with --log stage-<mode>.log) now extracts
  ttfr_ms/ttfr_first_files into $cleanPerf.scans[] -> the evidence tree's
  clean-volume-benchmark.json (+ the stage logs already land in evidence).
- The OQ-S4-3 file's harness section corrected to name the new lane.

Stage Summary:
- The TTFR probe + extraction now ride the owner's lane of record. The
  measurement = its next dispatch.

---
Task ID: PARITY-FIX-7 (gate-1d quote-canonicalization fix — 14/14 install-side assertions GREEN)
Agent: main session

Work Log:
- Run 40682713693 (the installer round-trip at the fixed Inno step): gates
  1a-1c + 2 ALL GREEN — install exit 0, version key write+verify
  (1.0.0-gate.3), ARP UninstallString + QuietUninstallString (ends
  " /SILENT") + DisplayName/DisplayVersion/Publisher/InstallLocation,
  driver service RDA RUNNING with the Boot File System group (the
  PACKAGING section-10 internal step proven end-to-end). The build, ISCC
  compile, and /VERYSILENT zero-prompt install all work.
- The single failure: gate 1d's exe parse. Inno writes UninstallString
  QUOTED ("C:\Program Files\..."); my ISS built '"' + quoted + '"' =
  ""path"" /SILENT; the gate's Substring parse then produced an empty
  FilePath. Fixed BOTH sides: the ISS strips UninstallString's surrounding
  quotes before composing the canonical single-quoted form; run-gates
  normalizes doubled quotes defensively, splits exe/args robustly (the
  $args automatic-variable trap avoided), and validates the exe exists
  before Start-Process.

Stage Summary:
- Gate 1d/1e (uninstall + residue) re-proven on the next dispatch — the
  only unproven half of the round trip.

---
Task ID: PARITY-FIX-8 (the fuzz-lane red repair: the stb EOF protocol + the UBSan nonnull)
Agent: main session

Work Log:
- Run 40682713696's fuzz job (the thumbnail landing's first fuzz
  exercise; the earlier CI red at 40680931476 never reached the campaign):
  (1) fuzz_thumbnail TIMEOUT at stbi__skip_jpeg_junk_at_end
  (stb_image.h:3393) - the mechanism: StbioSource::read_cb returned 0 on
  budget exhaustion WITHOUT setting eof_; stb's junk-skip loop keys on
  at_eof, so a budget-stopped stream that keeps reporting not-at-EOF
  while reads return 0 spins forever. Fixed: budget exhaustion now sets
  eof_ (budget exhaustion IS an EOF for stb's stream protocol).
  (2) fuzz_preview.cpp:41 UBSan nonnull - memcpy(buf, data_.data()+off,
  0) with an empty data_ (null data()); guarded by take > 0.
- Both files g++ -fsyntax-only clean under the repo's full warning set.
  The next fuzz dispatch (the new Checks lane) is the proof.

Stage Summary:
- All three lanes of the earlier red repaired (windows/local-mirror build,
  fuzz build, fuzz campaign). Watching the installer round-trip at
  1c4db82 (gate 1d/1e proof) + the new Checks lane.

---
Task ID: PARITY-FIX-9 (S-038 step 4 ACCEPTED — the board's last active SEVERE closed)
Agent: main session

Work Log:
- Run 40684304129 (1c4db82): the installer lane's first fully-green
  dispatch - gate 3 (sync pin + 17 probes) AND gates 1+2 (all 18
  assertions: install/version-key/ARP-both-strings/driver-internal-step/
  quiet-uninstall/no-residue). The full list is in S-038's Resolution.
- S-038 moved SEVERE/ -> FIXED/ with the step-4 Resolution; RESOLVED-INDEX
  row added; SUMMARY's board: 0 active, 246 FIXED, 2 tracked-open;
  PACKAGING.md status -> ACCEPTED.

Stage Summary:
- Session board outcome: the last active SEVERE closed; every engine-side
  recall gap verified fixed + registers truthful; the TTFR harness landed;
  three CI-red classes repaired. Remaining tracked-open items are owned
  by the parallel lanes (SCAN-2f's origin-LCN carrier) or execution-bound
  (the TTFR measurement on the rebuilt lane's next dispatch).

---
Task ID: PARITY-FIX-10 (session close-out)
Agent: main session (senior-dev parity-fix)

Work Log:
- Test & Benchmark lane (run 40684560877): the failure is the lane's own
  documented expected state - the latest release (v0.5.1) predates the
  combined era and lacks rda-test-sign.cer; the error message itself
  prescribes the remediation (publish the first combined release per
  docs/RELEASE.md and re-run). An owner tag-push action, not a defect.
- Session ledger (all pushed): d0a102c (GAP-S6-5 scorer + register truth),
  3e6a8fa (S-038 steps 2-3), d9b6862 (CI-red repair #1 + YAML colon),
  2a556e5 (M-068 move + testing.md), 47598bf/3307980 (TTFR harness +
  port), 1c4db82 (gate-1d quote canonicalization), 7a6374a (fuzz red
  repair), 51633dd (S-038 FIXED close-out). Plus the rebase-resolved
  merges with the SCAN-2f / api-exp-1 / CI-rebuild lanes.
- Session outcome: the board's last active SEVERE closed (S-038, all
  three gates green, run 40684304129); all seven engine-side recall gaps
  verified fixed with truthful registers; the physical-ceiling scorer +
  TTFR harness landed; three CI-red classes repaired; board = 0 active /
  246 FIXED / 2 tracked-open (both owned: SCAN-2f's carrier queue; the
  rebuilt lane's TTFR measurement dispatch).

Stage Summary:
- Handoffs left on the board: (a) the origin-LCN carrier (SCAN-2f F-lane),
  (b) the TTFR transcription after the next test-benchmark dispatch,
  (c) the first combined release (owner action; unblocks the
  test-benchmark lane + the installer payload's signed-driver posture).
Task ID: SCAN-3 (Agent 2 — the scan-engine context-recovery re-run + the OQ-SA-1 NTFS decode + the fix queue execution)
Agent: Agent 2 (scan-engine lane)

Work Log:
- Context recovery per protocol: four parallel read-legs (research quick/
  advanced spec + NTFS +0x64 probe; research carve census; core state audit;
  rda-driver scan surfaces). The refreshed Scan Engine Inventory / Delta
  Report is diff-to-fix/SCAN-ENGINE-DELTA.md (session-3 resolution log +
  updated fix queue; every load-bearing claim re-verified first-hand in the
  decompiles before acting).
- OQ-SA-1 NTFS +0x64 — DECODED, instruction-decisive: +0x64 = low u32 of the
  FIRST $DATA RUN'S STARTING LCN (the vtable+0x30 location-anchor family —
  NTFS sibling of the landed FAT +0x3e / exFAT +0x50 carriers); +0x60 = low
  u32 of TOTAL $DATA RUN LENGTH IN CLUSTERS (sparse-zeroed). The N-050/N-059
  "+0x60 type-registry id" census reading is REFUTED three independent ways
  (encoder outputs at every writer site; the +0x20 vtable getter is a
  return-0 stub for 8 of 10 per-FS classes; the census's per-extension value
  multiplicity is allocated-clusters behavior). Decode chain: FUN_1800336f0
  :41028/:41043 -> FUN_1800069e0 :5126-5127 -> FUN_180003020 :1987-1988 ->
  attach twin FUN_180033250 :2264-2265 -> serializer FUN_180003440 :2362.
  The prior session's "local_41 -> +0x53" trace premise was the wrong stack
  slot (local_41 @ raw+0x47 feeds +0x58; the parent ref exits via *param_4,
  never stored). OQ-SA-1 CLOSED (all three anchor halves).
- handoff: corpus-lane — the corpus artifacts carrying the refuted reading
  need the correction rows (ifileproperties-part1--ntfs-fat-exfat.md
  +0x60/+0x64/+0x40 rows; rsf-format--scan-session.md :385;
  ntfs-engine-algorithms-v2--deleted-viability.md's vtable+0x28
  "record_size" label — it is the SECTOR size). Core-side errata rows are in
  diff-to-fix/CORPUS-ERRATA.md (this session's batch).
- handoff: api/packaging lane — board hygiene: M-068's finding file still
  sits in findings/MAJOR/ while its own text records the fix (43f4962 +
  convergence 38ccecb); SUMMARY's board count (1 active · 244 FIXED) does
  not include it. Move to FIXED/ + amend the count. No scan-engine content
  involved.
- handoff: driver lane — rda-driver verified clean at v0.2.4 (323e75a):
  19 live session codes vs the README's stale "15 commands" prose (header +
  switch authoritative); the versioned image suffix derives from PE version
  1.0 (rda10.sys), not the v0.2.4 tag. Zero NAS/Cloud paths confirmed. No
  core scan-engine change required.
- S-007 remainder scope locked: the s3 carve census re-verified the niche
  work list against the registry decode and absorbed 8 census-artifact
  corrections (DXF="  0\r" u32 + u16 "99"; EPS/AI=C5D0D3C6 + u16 "%!";
  OneNote=E4 52 5C 7B id 0x31; II/MM\x1A=CRW not MRW (MRW=\0MRM id 0x66);
  placeable WMF=D7CD C69A same analyser as std WMF; ICO/CUR keys 00 00 01 00
  / 00 00 02 00 (both id 0x22); 00 01 00 00=TTF; u16 MM=3DS id 0x01; WPG
  id 0x33 with 0x45 = the SIZE stamp, erratum E48). Full key/grammar table
  in SCAN-ENGINE-DELTA session-3 log. Fix order: quick scan first (the NTFS
  carrier, landing next commit), then the per-school carve batches.

- Rebase reconciliation: the parallel SCAN-2f A2 leg (7bda022) decoded the
  SAME +0x64 value in the same window — the two legs' evidence merged in
  SCAN-ENGINE-DELTA.md (their DRWUI-twin cite + resident-wrapper caveat
  kept; their "+0x60 = DRWUI wire-side overwrite" phrasing CORRECTED by
  this leg's re-read of FUN_140520d20 DRWUI:853463-464 — the twin
  serializer persists the ENGINE-POPULATED +0x64/+0x60 verbatim, no
  overwrite exists at that site; the census values are
  allocated-clusters behavior). Their A4 count reconciliation (131 = 106
  analysers + 7 FileInfo + 18 infra) folded in. Their queue pinning
  (A-lane niche batch -> F-lane NTFS carrier) merged into this session's
  execution order.
- Also swept in this rebase: the stray '>>>>>>> 32a14d1' marker the TTFR
  lane left inside worklog.md on origin/main (same class as the orphaned
  marker 47598bf swept before; no content lost — the surrounding API-FIX-4
  entry is intact).

- OQ-SA-1 LANDED (1285be3, rebased to eac8e1d): tree::FileProperties::
  ntfs_origin_lcn + both walk stamps (main + GUID-dump) + clone() carry +
  nflags bit-8 persistence; fail-before proven first-hand (stamps
  neutralized -> engine tests RED, session round-trip green); full suite
  1010/1010 green pre-merge, 1012/1012 at the merged tree (gcc-13 ASan,
  this container). OQ-SA-1 is now FULLY closed: decoded, carried,
  persisted, tested.

Stage Summary:
- Delta report refreshed + committed; OQ-SA-1 closed at the decode level;
  the remaining scan-engine work is exactly the S-007 niche-format batches
  (advanced/deep), the two stale docs/PARAMETERS.md §3 rows (formats/
  parameters), and the execution-bound OQ-S4-3 (stays tracked). Combination
  layer audit-verified present — no action.

---
Task ID: API-EXP-1 (W1a landing record — R-56 the session list + summary)
Agent: api-exp lane (this session)

Fix landings:
- Core half `78a6552` (feat(session)): the VolumeInfo v5 tail (u64
  save_unix - our mirror of the og record-2's SYSTEMTIME scanTime,
  rsf-format §5.2 +0x6C; v1..v4 files read 0 = "not recorded"), the
  save-path stamp + the load pass-through, Reader::peek_record (the
  header-only type peek - same offsets table + error codes as
  read_record), ScanController::read_summary (light records only;
  heavy payloads skipped BY TYPE before any payload I/O; the legacy
  no-VolumeInfo rule = all-stages configured), the SessionSummary
  carrier, and the crsf-format.md VOLUME_INFO grammar section (the
  doc had NO volume-info section at all - now v1..v5 with every
  tail's length-carries-version rule). Tests: SaveUnixVTailRoundTrip
  (the 1024B v5 pin + the v4/v1 cut points), PeekRecordHeaderOnly,
  ReadSummaryLightRecordsOnly (2MB tree record present; summary
  fields exact), LegacySessionWithoutVolumeInfo; the
  VolumeInfoIdentityCardRoundTrip v4-size pin moved to the v5 total
  (1036B) with the wire.
- Surface half `eb16335` (feat(api)): cr_ui_session_list (cr_ui.h:808-
  850, two-call size pattern == snapshot_dfs, byte-sorted caller-owned
  POD rows, error rows for unreadable files, path-truncation rule,
  ERR_OPEN_FAILED fails loud on the directory), the rrecov `sessions
  [--dir D] [--json]` verb (FROZEN schema; unreadable rows are data
  never failure; enumeration failure exits 1), the C-consumer gate
  block, the parse-gate stub growth (WIN32_FIND_DATAW + the
  FindFirstFileExW family + MSVC-CRT localtime_s - the gate's own
  new-SDK-symbol rule), the three-site verb lock honored in one
  change (rrecov.cpp header + API.md §1 + CORE-API-CURRENT §c, c.3 =
  sessions, c.4..c.10 renumbered), ABI-CONTRACT §7f, every count site
  (79->80 exports, 10->11 verbs), and 39 cite re-pins (18 cr_ui.h +
  21 rrecov.cpp, every anchor re-verified first-hand, script-asserted).

Verification: 1009/1009 green locally (gcc 14.2 Release, incl. the
win_syntax_check parse gate), clang-format-18 clean, secrets/lints
clean. Board: API-DELTA §5.2 row S3-W1a flipped DONE. Next: W1b
(R-54 the session load progress + cancel - the corpus contract is
SaveScan:344-376, restore 40+50*done/(total+done) + per-child
IUIProgressInfo cancel; the N-060 save mirror).

---
Task ID: API-EXP-1 (W1b landing record — R-54 the session load progress + cancel)
Agent: api-exp lane (this session)

- Landed `ce6bcb0` (feat(session)): the load-side twin of the N-060 save
  machinery, corpus contract SaveScan:344-376 verified first-hand (the
  restore progress window + the per-child IUIProgressInfo IsCancelled
  teardown). Core: session::DeserializeProgress per attached node,
  ERR_CANCELLED propagating through the container layer (the
  REC_RESULT_TREE case explicitly refuses to flatten a cancel into
  SESSION_FORMAT). ABI: cr_ui_session_load_with_events (the caller-owned
  zero-init cancel flag IS the og's polled progress object - set from
  any thread; *out untouched on abort) + CR_EVT_SESSION_LOAD_PROGRESS
  (type 11, coalesced). Docs: ABI-CONTRACT §7g + the §6 event row +
  81-export counts + the cr_ui.h cite set re-mechanized for the +15/+21
  header inserts (all anchors grep-verified). Tests: 2 unit + 1 ABI +
  the C-gate NULL/NULL contract smoke; suite 1012/1012.
- Board: API-DELTA §5.2 row S3-W1b flipped DONE. Next: W1c (R-52 the
  auto-save-on-completion) - the corpus contract is SaveScan:443-451 +
  section 5.5 (auto_<vol>_<M>_<D>_<H>_<m>_<s> naming, the N-061
  auto_session_name already landed; what is missing is the ScanConfig
  trigger + the completion write + the event/stats surfacing).

---
Task ID: API-EXP-1 (W1c landing record + the Wave-1 completion note)
Agent: api-exp lane (this session)

- Landed `c95335f` (feat(scan)): R-52 the completion auto-save. Wave 1
  of the plan of record is now COMPLETE - W1a (R-56 the session list,
  78a6552/eb16335), W1b (R-54 the load progress + cancel, ce6bcb0),
  W1c (R-52, c95335f) - all three SaveScan-parity rows flipped DONE on
  the S3 board, every landing corpus-cited and locally green.
- The remaining plan-of-record board for the next session/lane: Wave 2
  (S3-W2a the query path_substring + state_mask axes, R-22; S3-W2b the
  scan-time folder filter, R-14), Wave 3 (R-41 dest codes, R-65 chkdsk,
  R-86 doc sentence, R-63 TRIM facet), Wave 4 (R-34 preview caps
  defaults, R-93 image create), Wave 5 (R-88 BitLocker turn-off).
  Session state at handoff: suite 1016/1016 on the local gcc-14 lane,
  all counts truthful (81 exports / 11 verbs / 39+OK), the three-site
  CLI lock held on every verb/flag change.

---
Task ID: SA-8/T1 (session-8 open — baseline record)
Agent: session-8 full parity audit (this session)

Baseline (cloned fresh this session):
- core_recovery @ 35adcb93cf433af4339ee7bdb4c7f5e3e13e8fcf (main) — merged
  product repo (confirmed: core engine + win layer + audit trail in one tree;
  driver ships as prebuilt rda.sys release asset).
- rda-driver @ 323e75acf4cd745960813af4e7fd586a4a34d1a0 (main, v0.2.4 — the
  M-064 remainder landed).
- corpus easeus-drw-research @ be0cfe95ecf22ac67e0f3a823dd003163b196d38 (main)
  — ONE commit past the trail's standing 03ad46c: CORRECTIONS.md #9
  (EUUserData -> CHKRepairFileSign; GetRawUserInfoMap = per-scan
  carve-statistics map). Absorption verified first-hand: CORPUS-ERRATA E-A/E-B
  + PARITY-DELTAS D-104 + SCAN-ENGINE-DELTA F2 row already carry it.

Session scope determination (from the trail + corpus state, not the prompt):
the board reads 0 active / 249 FIXED / 1 INFORMATIONAL (I-026) / 1 tracked-open
(OQ-S4-3, Windows-execution-bound). New audit surface since the last audited
baselines: (a) product code landed since the dev-sign close and the SCAN-2f/3
context legs — API-EXP W1a/W1b/W1c (78a6552/eb16335, ce6bcb0, c95335f),
SCAN-2f/3 OQ-SA-1 carrier (eac8e1d/1285be3), S-007 niche batch-1 (e018bf4),
thumbnail engine (05e6786), installer S-038 steps 2-3 (3e6a8fa), CI rebuild
(566c483) — each claims corpus contracts; this session verifies those claims
read-only. (b) The S-007 batch-2 remainder (~14 corpus-documented analyser
formats absent, causes pinned in the FIXED addendum — a live recall gap
recorded outside an active finding). (c) The driver-lane handoffs D3/D4
(recorded not fixed). (d) Gate-doc currency: SURFACE-INVENTORY/PARAMETERS cite
corpus 03ad46c; the corpus moved.

Prompt saved redacted: worklog/session-8-parity-audit-prompt.md. Read twice.

---
Task ID: SA-8/T2 (session-8 wave 2 — the continuation mandate: find what prior
sessions missed, verify the new landings, mark every remaining capability as
a finding)
Agent: session-8 full parity audit (this session, second leg)

Session mandate (user): "look for other things which session 1 missed and
continue working and remove the fixed things and go more deep, do more
comparison on og-codebase and our core and mark all remaining features,
implementations, functions, as finding too." Two-repo scope THIS session:
core_recovery (ff879f9 lineage, read-only) + easeus-drw-research (be0cfe9).
The rda-driver lane handoffs D3/D4 stay OUT of this session's scope (the
two-repo mandate; they remain that lane's items on its own repo).

Wave-2 lanes (from the T1 scope determination + the continuation mandate):
- V-lane: read-only verification of the post-dev-sign landings' corpus
  claims (API-EXP W1a/W1b/W1c; OQ-SA-1 carrier eac8e1d; S-007 batch-1
  e018bf4; thumbnail 05e6786; installer S-038 steps 2-3; CI rebuild 566c483).
- R-lane: the S-007 batch-2 remainder becomes PROPER FINDINGS (the live
  recall gap currently recorded outside an active finding) — decode what is
  decodable at the og registration sites first (CHM/MNY/EMF/ONE/MPEG-TS/
  3DS/SWF/JP2/MOBI/LZH + the text school + FCPEvt/OfficeTagFromXML/CRW).
- G-lane: the gap hunt — corpus artifacts + og-codebase surfaces the prior
  sessions' waves never dispositioned; core capabilities without corpus
  basis (superset register); thumbnail engine vs corpus preview evidence.
- C-lane: gate-doc currency (SURFACE-INVENTORY/PARAMETERS/README corpus
  pin 03ad46c -> be0cfe9; board counts; CROSS-REF-MATRIX new rows).

Baselines: core ff879f9 (suite green per the landing lanes' records),
corpus be0cfe9 (CORRECTIONS #9 absorbed per T1). Board at open: 0 active /
249 FIXED / 1 INFO (I-026) / 1 tracked-open (OQ-S4-3). ID space continues:
next A-012+, S-040+, M-071+, N-090+, I-027+.

---
Task ID: CI-REL-1 (the CI-red repair + the v0.5.2 release cut)
Agent: senior-dev session (this session)

Work Log:
- Context: every Checks run since c95335f red on the MSVC lane; the
  release lane red at validate since 40690974024 (three separate
  build breaks fixed by 66404ae/6ab676f/35adcb9 - C4305/C4804,
  clang-format pin drift, getpid C4996); the test-benchmark lane red
  at fetch (40684560877: v0.5.1 predates the combined era, no
  rda-test-sign.cer asset).
- The last remaining red: run 40694592245 (35adcb9), test 980
  CrUi.AutoSaveOnCompletion at test_cr_ui.cpp:396 - the FIRST block's
  cr_ui_session_list on the just-created EMPTY autosave dir returns
  0x50020001 (ERR_OPEN_FAILED) on MSVC. Root cause: the Windows
  list_crsf_files maps FindFirstFileExW's INVALID_HANDLE_VALUE to
  false unconditionally; on an empty directory the API fails with
  ERROR_FILE_NOT_FOUND ('searched, nothing matched') which is the
  empty-list SUCCESS - POSIX opendir on the same dir succeeds, hence
  gcc-green/msvc-red (the dominant msvc-only-red class).
- The fix (both surfaces of the seam): cr_ui.cpp's list_crsf_files
  returns GetLastError()!=ERROR_FILE_NOT_FOUND on the invalid handle
  (ERROR_PATH_NOT_FOUND/access-denied stay failures); rrecov.cpp's
  print_sessions takes the same branch - an empty dir prints zero
  rows and exits 0 (mirroring the ABI contract), a genuinely missing
  directory stays FB-K01 exit 1.
- Verification in this container: gcc 14.2 linux-release build clean
  (-Wall -Wextra -Werror), full suite 1020/1020 green including the
  win_syntax_check parse gate (7.05s - the _WIN32 branches of both
  edited files parse against the win-stub SDK) and the AutoSave test
  itself; clang-format 18.1.8 (the pinned CI version) dry-run clean
  on both files.
- RELEASE-NOTES.md vNext gains the fix bullet (the honest surface for
  the v0.5.2 consumer).

Stage Summary:
- Next: push, watch Checks go green (the last known red), then cut
  v0.5.2 (the first release of the combined era with the persistent
  dev-sign: rda-test-sign.cer ships, which is exactly the asset the
  test-benchmark lane's fetch demands), and the release's own dispatch
  continues the test+benchmark pass.

---
Task ID: CI-REL-1b (the 40700058960 correction - the error-code theory died on real windows)
Agent: senior-dev session (this session, second leg)

Work Log:
- The first fix (ERROR_FILE_NOT_FOUND -> empty success) was WRONG on
  real windows-2022: run 40700058960 came back with the observed
  matrix INVERTED from the documented one - the nonexistent
  no-such-dir-w1a/no-such-dir-cgate dirs report ERROR_FILE_NOT_FOUND
  (my key wrongly mapped them to success, breaking test 981 :514 and
  the abi C-gate :570, both pinning ERR_OPEN_FAILED) while the
  existing EMPTY autosave dir reports a DIFFERENT code (test 980
  stayed red at :396). 3 red, worse than the 1 we started with.
- The lesson recorded in the code comment: the FindFirstFileExW error
  code is NOT a reliable empty-vs-missing axis across real systems;
  the version-proof discriminator is the DIRECTORY-EXISTS probe. The
  landed fix (dd225da): on a failed enumeration, GetFileAttributesW
  decides - existing directory = the empty-list success (POSIX opendir
  semantics, the W1c first-block contract OK + n==0), missing path or
  a plain file handed as a dir (POSIX ENOTDIR) = the genuine
  ERR_OPEN_FAILED. Known accepted divergence noted in the comment: a
  dir with denied-list-but-readable-attributes (a rare ACL shape)
  reports empty where POSIX opendir would fail.
- Both surfaces again: the ABI seam and the CLI sessions verb (missing
  dir stays FB-K01 exit 1). Local: gcc 1020/1020 incl. the parse gate,
  format 18.1.8 clean. RELEASE-NOTES vNext bullet corrected to the
  final truth.

Stage Summary:
- Watching run for dd225da; on green, cut v0.5.2 (the combined-era
  release with the persistent dev-sign) - the release dispatch then
  continues the test+benchmark lane against the fresh tag.
---
Task ID: SA-8/T2-EXEC (the T2 plan's execution record — the five lanes ran in this
session; the entries below are the results. The product tree moved under the
audit mid-flight (the CI-REL lane's dd225da et al.); every load-bearing absence
claim was RE-VERIFIED on the final rebased tree before push — see the entry's
tail and the s8 evidence file.)
Task ID: SA-8 (the session-8 deep audit — the second-session continuation; findings filed, boards updated, close-out)
Agent: session-8 full parity audit (this session)

Scope executed (the T1-determined plan + the user's second-session mandate:
find what session 1+ missed, verify/remove the fixed things, go deeper on the
og-codebase comparison, mark all remaining features as findings):

1. **Five parallel read lanes** (og-codebase never-audited asset sweep; the
   W1a/W1b/W1c/thumbnail landing verification; S-007 batch-1 + NTFS carrier +
   FIXED-truth spot-check; the 9 R-row evidence packages; the two NEVER-CITED
   driver docs + the MOM decode candidates). Every load-bearing claim
   re-verified FIRST-HAND by the lead before filing (the M-018 regression
   grep census, the volume_label fill-site census, the CRW registry+validator
   read, the thumbnail budget guards, the session-list filter read, the CLI
   -h block, the batch-2 absence loop, the DeviceManager decompile greps).
   Evidence: diff-to-fix/evidence/s8-verification-records.md.

2. **25 items filed** (the session's diff-to-fix/ output):
   SEVERE: S-040 (image create absent, R-93), S-041 (the S-007 batch-2
   remainder, ~24 formats — previously recorded outside an active finding).
   MAJOR: M-071 (the M-018 regression — the N-064 rrecov.cpp reconstruction
   dropped the junk gates; git-proven), M-072..M-078 (the R-row remainder:
   query axes, scan-time folder filter, dest codes, chkdsk, TRIM facet,
   preview caps, BitLocker turn-off), M-079 (auto-save label never filled),
   M-080 (thumbnail budget starves ordinary photos), M-081 (CRW
   misattribution + the PRD gate rejects genuine CRW), M-082 (DeviceManager
   offline-OS identification — the CheckVersion verdict re-scoped).
   MINOR: N-090..N-098. INFORMATIONAL: I-027 (the data-asset ground-truth
   register). TRACKED-OPEN: OQ-S8-1 (CWindowsFS consumer).

3. **Landing verification results** (the T1 scope items): W1a/W1b/W1c TRUE
   at the load-bearing points; the NTFS origin-LCN carrier TRUE on all
   parameters; batch-1 27/27 magics byte-exact (bounds → N-093); the
   installer steps 2-3 verified by the lane's own green dispatch record (run
   40684304129); the CI rebuild docs-only. FIXED spot-check: 9/10 CONFIRMED,
   M-018 PARTIAL → M-071.

4. **Boards updated:** SUMMARY (the session-8 delta + the new counts + the
   recommended fix order), CROSS-REF-MATRIX (the session-8 detail rows),
   API-DELTA §5.2 (row→finding map), SCAN-ENGINE-DELTA A4 (the batch-1
   landed/batch-2-owned correction), SURFACE-INVENTORY (the corpus-currency
   + the driver-doc discharge + the decode-candidate dispositions), README
   (the session-8 audited-commit row), 13 module files (session-8 sections).

Board at close: **25 active** (2 SEVERE · 12 MAJOR · 9 MINOR · I-027; I-026
informational) · 249 FIXED · tracked-open 2 (OQ-S4-3 unchanged; OQ-S8-1
new). Recommended first fixes: M-071 (2 lines, a regression), M-081 (small,
self-contained), then S-041's batches, M-079 (one function), M-080 (decide
before the ABI handoff), S-040 (the largest feature).

Driver-lane handoffs (recorded not fixed — the rda-driver repo is out of
this audit's two-repo scope; for that lane): (a) the READ-length semantics
nuance (og Length = sectors vs rda Length = bytes; the Information×512
invariant holds — an og-style client through rda.sys would under-read; no
in-product effect, core is rda's only client); (b) the disk.sys-comparison
field reading (trio artifact's DriverName vs DRIVER-CONTRACT's ServiceKeyName
— the decompile's `*(DO->DriverObject+0x30)+0x18` chase matches neither
cleanly; flagged to S7); plus the standing D3 (the 15/18-vs-19 session-code
prose, seven driver-repo sites) and D4 (regdrv-rda.reg hex(7) UpperFilters
still invalid).

No source code modified. Every changed path is under diff-to-fix/ + worklog*.
