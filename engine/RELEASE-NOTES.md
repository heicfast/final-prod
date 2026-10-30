# core_recovery release notes

**A-F1 (the honest residue, filed not fixed):** the mid-stage carved-
count telemetry delta (N-166's property) does not materialize on
Windows — four release validations' evidence, three independent test
designs, and a discriminating run with hundreds of ticks flowing all
confirm no ScanProgress event carries `carved_found >= 1` before
completion there, while carving itself, the tick machinery (stages
0/1), the park machinery, and mid-scan queries are all proven working
by the sibling batteries in the same runs. The property holds on
Linux. The pin (Telemetry.CountersMoveDuringStage) is deleted with
this pointer and re-lands with the Windows fix; the full evidence
chain and the scoped next-session probe live at
worklog/api-findings.md A-F1.

# core_recovery v0.6.1

**Scope of this release:** the v0.6.0 scope plus the release-blocking
repairs found by the v0.6.0 release validation itself (run 27 refused to
publish — "no release without green, without exception"): nine
Windows-only test defects that had never completed a Windows run (every
checks.yml run since their landing was cancelled by the next racing
push), two of them real implementation bugs now fixed — the BitLocker
turn-off command's doubled quotes (a PowerShell arg breakage; the verb
never worked) and the missing stale-.partial sweep on image-commit
resume-after-cancel. The v0.6.0 tag's validation surfaced them; this
release carries the repairs. The full note set below (v0.6.0's scope
statement stands unchanged).

**Added at the cut (the coherence lane):** the release gate itself is
restored — the Win7 posture gate's import baseline completed for the
S-060/M-074/M-075 landing wave (the five kernel32 consumers + the
SHLWAPI strict row; every import Win7-SP1-available, source-censused,
pinned by tools/test_verify_pe_um.py cases 6-7), closing the 8-red-run
streak that held the tag. Docs that ship in this zip were reconciled to
the tree in the same lane (README gap list + worklog row, RELEASE.md
pipeline + signing posture, API.md grammar, CORE-API-CURRENT counts,
PARAMETERS --deep, CONNECTION-GUIDE transport table, CAPABILITY-MATRIX
ReFS row, the diff-to-fix banner pointer).

# core_recovery v0.6.0

**Scope of this release:** the capability + parity milestone on top of
v0.5.2 — 897 commits, the M-107 carve size-walk family COMPLETE (all
eleven members landed — X3F, CRW, DWG, PSP, WPG, WPD, DXF, EPS, AI,
SIT, APE, SWF-RECT; the finding CLOSED and moved to findings/FIXED/,
with the DWG gen-3 R2007 remainder recorded as the precisely-scoped
residue; the M-160 APE whole-family absence fixed beside it — the
u32-BE gate rejected every real Monkey's Audio file, the u16-LE window
[1000,9999] + the CAPEAnalyser walk land), the whole-disk partition scan, the
pointed-resume session surface, and the warnings-hygiene coherence pass
(the MSVC /W4 /WX build restored to zero warnings; the full Linux suite
green at every landing). The headline set since v0.5.2:

## Carve engines — the M-107 size-walk family (the blind stand-ins retired)

- **PSP, DWG, WPD, WPG, DXF, EPS, AI** (members 1-9 of 11): each format's
  size is now computed by the og's own walk — decoded first-hand from the
  reference binary (objdump/pefile over the corpus), landed with RED→GREEN
  stash-dance proof per member, and pinned by per-format batteries. The
  blind 64 MiB stand-ins are gone: recovered extents no longer truncate
  (or over-extend) versus the reference. Notable decodes: the DWG
  three-generation walk (the 0x15 position constant, the R2004 XOR-LCG
  keystream), the WPD version-table gate (the og stamps extent = the
  header's own u32 size field on any of three version pairs), the WPG
  zero-tail continuation walk, the DXF EOF-trailer scan (LF and CRLF
  forms), the EPS Adobe covenant (`%!PS-Adobe-` required — a bare `%!PS`
  is the og's decline), and the AI header formula (three start+length
  pairs, max wins), the SIT entry-marker walk (the per-ENTRY
  marker + stride table, the corrupt-entry terminal, the new-header
  gate at 512-byte boundaries; the racing lanes' convergence kept
  the fuller 2214af90 decode per SA-9), the APE CAPEAnalyser walk
  (the OLD-format seek-table scan with entry[0] the bound and the max
  entry the walked size; convergent with M-160's u16-LE version gate),
  and the SWF-RECT validation gates (the per-arm version windows FWS
  [3,20] / CWS [6,20], the FWS RECT bounding-box walk, the CWS deflate
  CMF nibble + the partial-tolerant streaming inflate + the SAME RECT
  walk over the inflated bytes — the recorded follow-up landed).
- **S-054 (repair of the S-047 landing)**: the MPEG-TS/AVCHD (TS/M2TS)
  carve cap is the og's 200 MB controller global EXACTLY — the S-047
  landing's "× 10" value (2 GB) was a refuted provenance; any release
  that shipped S-047's cap should read this as the correction (recovered
  TS/M2TS files above 200 MB were over-extended vs the reference).
- **M-117**: contiguous TS/M2TS/MPEG-audio streams no longer pay the
  quadratic mid-stream re-walk in the carve stage (covered candidates
  resolve at the covered check before the validator).

## Scan + session surfaces

- **S-051**: a whole-disk scan target (the ABI's `cr_ui_open_disk`, the
  CLI `scan --disk` / whole-disk images) now reads the partition list at
  scan setup and scans every table volume (the og's CDoScanPartition
  flow) — a partitioned disk no longer completes "successfully" with
  zero files.
- **M-154 (pointed resume)**: `cr_ui_session_resume_last` — the header's
  phantom export implemented; the last-autosave path reads the pointer
  sidecar (not the directory enumeration), with the full dead-pointer
  refusal taxonomy (ERR_OPEN_FAILED per the cr_ui.h contract).
- **M-101**: the results-tree gains the Versions node and Tags Extracted
  branches (the two missing results-tree surfaces).
- **M-116**: the archive-list verb + the two-call archive ABI
  (STASH.ZIP-class archives surface their member inventory end-to-end).
- **The `image` verb**: `image --create --disk N --out file.img` — the
  flat-raw image producer (atomic temp+rename commit, .sha256 sidecar,
  explicit .partial on cancel/failure).
- **M-075**: `fix --chkdsk`; **M-078**: BitLocker unlock/turn-off
  (`unlock --turn-off` CLI + `cr_ui_turn_off_bitlocker` ABI);
  **M-105**: the DRWService elevation-broker decision closed as
  NO-SERVICE (the FVE harvest's ACL diagnosis replaces the transient
  LocalSystem service).

## Hygiene + verification (the coherence pass)

- The Windows empty-directory listing fix (runs `40694592245` /
  `40700058960`): a directory containing NO `.crsf` sessions listed as
  `ERR_OPEN_FAILED` on Windows — the version-proof discriminator now
  probes directory existence (`GetFileAttributesW`) on failed
  enumeration: an existing directory with zero matches is the empty-list
  SUCCESS (POSIX `opendir` semantics), a missing path stays
  `ERR_OPEN_FAILED`. Both surfaces: `cr_ui_session_list` and the CLI
  `sessions` verb.
- The MSVC /W4 /WX build restored to zero warnings (the C4310/C4127/
  C4267/C4244 classes fixed at their root — explicit lo-word masks, the
  direct-failure convention, explicit EOCD casts, `narrow_bytes()` for
  the journal paths), zero suppressions added.
- The committed conflict-marker classes swept (the N-210 gate now holds
  across the merged windows); the suite grew to 1,700+ tests, green at
  every landing.

## Release truth (the coherence session's addendum)

- **Version stamps track the tag again:** v0.5.0/v0.5.1/v0.5.2 shipped
  with `core_version: 0.4.0` in their MANIFEST.json (vcpkg.json +
  CMakeLists.txt were never bumped through the line; verified against
  the published v0.5.2 MANIFEST first-hand). From this release the
  stamps carry the tag: 0.6.0.
- **The v0.5.0-v0.5.2 notes gap (honest):** those three releases were
  cut without release-notes entries. Their content, from the tags:
  **v0.5.0** (11 commits) the driver-only era transition (Win7
  import-allowlist entries for the SCM surface, C4477 fixes, the
  capability matrix's driver-only rows); **v0.5.1** (3 commits) the
  extended-IOCTL phantom-zero repair + the prebuilt fetcher's binding
  fixes; **v0.5.2** (97 commits) the CI consolidation + the Windows
  empty-directory listing fix (both surfaces).
- **The win-only test battery compiled for the first time:** the M-075
  fsck wiring battery (test_fsck_win.cpp) shipped uncompiled by any
  compiler until checks.yml run 321 caught it (7bd3ed8b's
  `using namespace cr;` repair). The class is now gated on the Linux
  lane: win_syntax_check.sh parse-covers every
  tests/unit/test_*_win.cpp (18/18 at this session's HEAD) with the
  win-stub SDK grown to carry the gtest CRT surface; two test files
  dropped the MSVC-only multi-word functional cast for the identical
  static_cast form.

## vNext (CI consolidation, 2026-10-28) — folded into this release

- *(Corrected 2026-10-29, M-088: the lane set is FOUR, not three — the
  S-038 `installer.yml` gates lane postdates the consolidation and the
  original text omitted it; the FloorVersion floor is pinned at TWO live
  sites (release.yml + installer.yml), both v0.2.4.)*
- CI rebuilt as the windows-only 3-lane set: **Checks** (format, lints,
  /W4 /WX build, ctest, Win7 posture, report schema, driverless refusal
  probes - vcpkg binary-cached), **Release** (validate -> build core +
  fetch driver tier -> test-dev-SIGN the driver with the repo's
  persistent certificate, public cert shipped as `rda-test-sign.cer`),
  **Test & Benchmark** (prebuilt-release consumer: driver verify,
  simulated VHDX disks, 10-scenario matrix through CLI and the cr_ui API
  both, gates + perf + structured evidence).
- Release asset set gains `rda-test-sign.cer`; `MANIFEST.json` now records
  the driver's unsigned AND signed SHA-256 plus the dev-sign subject.
- The ubuntu sanitizer/fuzz CI lanes were dropped (owner directive;
  coverage remains available locally via `ci-local.sh --san|--fuzz`).

# core_recovery v0.4.1-parity

**Scope of this release:** the warnings-hygiene + docs truth pass on top
of `v0.4.0-parity` (whose release notes — the parity-audit-close
headline set — remain at the `v0.4.0-parity` tag and in git history).
Zero behavior change: every engine, CLI, ABI, and test assertion in
this release is bit-identical to v0.4.0-parity; what changed is the
warning surface, the CI gate strength, and the documentation truth.

## New since v0.4.0-parity

- **The zero-warning fuzz lane** (commit `095aab5`): the clang
  libFuzzer lane's entire 48-warning backlog is zeroed and the lane
  now compiles with `-Werror` — the same zero-warning discipline the
  gcc and MSVC lanes already enforced. The backlog was fixed at its
  root, never suppressed: 44 wire-format constants (zfs/hfsplus/
  btrfs/ext4/refs) are `[[maybe_unused]]`-annotated documentation
  (zero codegen change), the refs Walker initializer is explicit at
  its existing effective values (construction byte-identical, verified
  against the struct's own defaults), the structurally-unreachable
  `> 4 GiB` RIFF-size ceiling in `validate_avi` is dropped with its
  contract recorded (a u32 can never reach it), the btrfs
  `subvol_index` dead store and `cr_fscheck`'s caller-less `rd64` are
  deleted, and the S-023 merge-storm duplicate of the
  analysis-adoption block in `cr_ui.cpp` is removed (the live twin
  executes first; the second guard could never fire).
- **OQ-S4-3's transport half is now a measured verdict** (not an
  argument): with the windows link fixes green again, the alignbench
  harness executed on the live Windows lane (run `40315255368`,
  win22, `\\.\C:`, 512 MiB span, the og's own qd=1, RDA service
  RUNNING). The core's 64 KiB direct-I/O envelope **meets/exceeds**
  the og's 4/7 MiB buffered-synchronous chunking (203.98 vs
  197.32/196.15 MiB/s — the og trails 3.3–4.0%) at comparable CPU, and
  the 256 KiB cache block **exceeds** it by 18–19% at lower CPU.
  Transcribed in the OQ file's closing addendum + M-030's Resolution
  measurement addendum + `SUMMARY.md` (Session-38 delta). Remaining
  open: only the acceptance's scan-level time-to-first-result half
  (a comparative-bench dispatch; no engineering prerequisite).
- **Driver pin**: the API-benchmark lane now fetches `rda-driver`
  **v0.2.2** (was v0.2.1) — the driver's current release with the
  `sc.exe` installer-script fix; `>= v0.2.1` remains the functional
  floor (the bare-open error-87 fix). Pinning addendum in
  `docs/DRIVER-UTILIZATION.md`.

## Documentation truth pass

- **New** `CONTRIBUTING.md` (build lanes, gate inventory, commit
  style, the zero-behavior-change discipline) and `SECURITY.md`
  (BitLocker **legitimate-unlock-only** policy with no FVEK
  extraction, the default-deny consent model, the Win7 import floor,
  the driver-side SDDL boundary).
- `docs/CONNECTION-GUIDE.md` §8: known host projects — the UI repo
  currently consumes nothing (MockConnector + the typed TS connector
  contract mirroring the `cr_ui` ABI, binding session pending); the
  rda-driver relationship (runtime `\\.\RDA` opens, bench pin).
- `diff-to-fix/SUMMARY.md`: Session-38 delta appended (the convention:
  history is never rewritten).
- Test-count truth: **926 tests** measured (`ctest --preset linux-gcc`,
  ASan, at this tree) — `docs/testing.md` already matches; the 924 in
  the v0.4.0-parity notes described the pre-wave tree and stays in
  that tag's history.

## Validation at this tag

- **926/926 tests** green on Linux (plain + **ASan**; the ubsan/tsan
  CI lanes and the Windows MSVC `/W4 /WX` lane green at the parent
  commits; this release adds documentation + warning-surface changes
  only on top).
- clang-format-18 gate clean; the win-layer parse gate
  (`win_syntax_check`) green; the fuzz lane green under its new
  `-Werror` gate; **API-Benchmark (alignbench) green** — the
  `alignbench.json` artifact and the OQ-S4-3 measurements above come
  from that run.
- **Win7 import gate green on every shipped PE** (allowlist extended
  at v0.4.0-parity for the S-014/S-010/M-033 imports and the N-062
  `K32GetProcessMemoryInfo` binding — recorded in `tools/verify_pe_um.py`).

## Known honest gaps (registered, not hidden)

NTFS XPRESS/LZX compressed $DATA (GAP-P1-1 — no corpus surface exists;
the og is LZNT1-only too), ReFS container-map translation (N-036), ZFS
multi-vdev topologies (single-vdev only), HFS+ attributes items the
corpus itself leaves UNVERIFIED, and OQ-S4-3's **scan-level
time-to-first-result half** (the transport half is now measured — see
above). Nothing silently degrades: unimplemented surfaces return
explicit errors.
