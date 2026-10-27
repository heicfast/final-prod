# Integration gaps — capabilities the UI expected that the real core does not provide

Honest gaps, not failures: a truthful disabled surface beats a false one.
Every row records the precise scope, the shipped behavior, and the follow-up
plan. Cross-references: `docs/INTEGRATION-PLAN.md` §3.11 (the ratified
register), `contracts/error-codes.json` (the error vocabulary),
`docs/PARITY-DELTAS.md` (deliberate divergences from the reference layout).

**Session update (debug & integration, 2026-10-27).** The user installed the
shipped v0.6.0-rc1 test-signed release on a real machine: the driver
installs and runs, and every functional surface is dead, partial, or fake.
The exhaustive re-read (context sheets `docs/PRODUCT-WIRING.md`,
`docs/INSTALL-FOOTPRINT-CURRENT.md`, `docs/REFERENCE-*.md`) found the
product's seams broken or fake in ways the prior validation could not see
(it exercised the bridge directly and ran the UI's e2e suite against the
test double — never the installed UI against the real engine). This file now
records the **current reality**: the prior-gap status first, then the new-gap
register with severity, scope, and the session plan. Runner-reproduced
evidence lands in `docs/FORENSIC-PROBE.md` as it is captured.

---

## Part 1 — prior-gap register: what closed, what is still open

| # | Prior gap | Status after this session's diagnosis |
|---|---|---|
| G-1 | Standalone repair workbench (dryRun/undoLog/undo) throws typed `not-implemented`; consent flow in-scan shipped | **Partly dishonest in practice.** The typed refusals are real, but the Repair *page* renders a **canned fake queue** on top of them (see NG-10). The gap statement stays OPEN for the ABI trio (b); the fake page is a new defect. |
| G-2 | `currentPath` ticker always null; UI hides it truthfully | OPEN, unchanged (engine has no per-item event; the reference does not expose one at this granularity either). |
| G-3 | `dataFoundBytes` live counter absent; exact value latched at completion | OPEN, unchanged, compatible follow-up. |
| G-4 | Rendered previews (pdf/av/office) null with truthful hints | OPEN, unchanged (feature, not wiring). |
| G-5 | `keepEmptyFolders`/`fileRepair` recovery options truthfully disabled | OPEN, unchanged. |
| G-6 | Engine version string null in About | OPEN, unchanged, compatible follow-up. |
| G-7 | Bridge-side results query for live scans and sessions | **Compounded by NG-1** (the real engine's node model defeats the bridge's path-tree listing entirely — the query never gets a tree to filter). |
| G-8 | Device disconnect synthesis from heartbeat | OPEN, unchanged. |
| G-9 | Device bus kinds (`sd`/`optical`) unmappable from the `removable` flag | **Still open and now visible as a dead surface**: the SD-card page filters `kind==='sd'` that the type map can never produce (NG-14). |
| G-10 | Pre-scan lost-partition hints absent; badge surface future wiring | **Compounded by NG-9**: the Lost-Partition page is a canned fake rather than a truthful surface over the real scan-time capability. |
| G-11 | Folder-scoped scans implemented as bridge-side path scoping | OPEN, unchanged, documented semantics delta. |

---

## Part 2 — new-gap register (this session's findings, all source-cited)

Severity scale: **C** = critical (surface dead or fake), **H** = high,
**M** = medium, **L** = low/hygiene. Unless marked *user decision*, every
gap below is scheduled to be **fixed in this session**; each fix cites its
break point in `docs/FORENSIC-PROBE.md` and lands with a test that failed
before and passes after.

### The scan pipeline (user symptom: "40% forever, no results ever")

- **NG-1 [C] The results tree can never render against the real engine.**
  The engine hangs all results under virtual roots (Found / Deleted / Lost /
  carved) whose node `attributes == 0` (`engine/src/core/tree/cr_node.cpp:109-127`),
  while the bridge decides directory-ness via `attributes & 0x10`
  (`ui/src-tauri/src/bridge/state.rs:636`) and builds directory paths from
  components that all contain `/` (`state.rs:625-634`), which the depth-1
  path-tree listing then filters out (`ui/src-tauri/src/commands/results.rs:701-715`).
  Net effect: an empty tree on every completed scan. The UI's e2e/unit
  suites stayed green only because they run against the test double, whose
  fixture tree (`ui/tests/fakes/test-connector.ts:164-175`) has top-level
  directories — structurally impossible with the real engine.
  **Fix:** the bridge's directory/path model must consume the engine's real
  node model (virtual roots + attribute-flag directories) — bridge +
  connector contract, with a contract test that runs the REAL engine fixture
  (a real cr_ui scan of a test image) instead of the double.
- **NG-2 [H] Results only refetch on 10%-bucket changes.**
  `ui/src/queries/scan.ts:41,55-60` refetches the tree when
  `floor(overall*10)` advances; between 40% and 50% — the entire deep stage
  on a big disk — nothing refreshes even if data arrives.
  **Fix:** event/timer-driven refetch while a scan is live.
- **NG-3 [H] Deep-scan progress stalls by design on large volumes.**
  Deep-scan progress ticks only when a parallel chunk completes
  (`engine/src/core/tree/cr_mftwalk.cpp:2117-2164`) and the heartbeat repeats
  the standing permille (`engine/src/ui/cr_ui.cpp:875-916`), so a
  single-threaded remnant sweep over a multi-TB volume shows hours at
  40.x% with no forward signal. The reference shows sector-linear progress
  with a "Reading sector X/Y" ticker and a remaining-time estimate
  (`docs/REFERENCE-BEHAVIOR.md` §B).
  **Fix:** engine emits fine-grained deep-scan progress (sector-cursor based
  across workers), bridge maps it, UI displays sector ticker + estimate.
- **NG-4 [H] Quick-pass probe failure yields an empty-but-OK scan.**
  An unknown-FS probe failure produces an empty quick pass without a
  surfaced reason (`engine/src/core/orchestrator/cr_scan.cpp:1345-1348`);
  driver read failures degrade to silent bad-sector ranges rather than
  surfaced errors. The reference surfaces scan-blocking errors.
  **Fix:** engine error propagation for probe/read failure classes; UI
  shows the truthful state instead of an empty 40%-forever scan.

### The SMART / health surface (user symptom: "one drive, percentage only")

- **NG-5 [C] The health percentage is UI-fabricated.**
  `ui/src/features/health/health-page.tsx:100-106` renders good→`max(88, 100−wear)` (≥96%),
  caution→72%, bad→38% — including when `attributes: []`. This is fake data
  in a shipped surface.
  **Fix:** render only real report fields (status grading, temperature,
  power-on hours, wear (NVMe/SSD), reallocated/pending counts, attribute
  table); no synthetic percentage. If no data → truthful unavailable state.
- **NG-6 [C] NVMe drives are refused by an `available==0` misread.**
  `ui/src-tauri/src/commands/health.rs:43-48` treats `report.available == 0`
  as no-SMART, but `available` is the ATA-SMART-table flag; NVMe health data
  is carried by `nvme_available` (`engine/src/ui/cr_ui.cpp:3697-3701`). On
  NVMe-only machines (most modern laptops) the health surface is dead.
  **Fix:** accept ATA-attributes OR NVMe health; grade per transport.
- **NG-7 [C] Only one drive listed / others invisible.**
  Home lists only `kind === 'internal'` disks
  (`ui/src/features/…/home-page.tsx:79-84`); the health surface lists
  physical disks only via `use-physical-devices.ts:21` (volumes never
  shown); unlettered volumes never enumerate
  (`engine/src/win/cr_enumerate.cpp:224-230`); probe failures collapse to
  `permission-denied` (`ui/src-tauri/src/commands/devices.rs:77-127`). The
  reference lists **all physical disks** in its health selector
  (`docs/REFERENCE-BEHAVIOR.md` §A).
  **Fix:** enumerate every physical disk (internal + removable + vdisk) for
  the health selector and Home; per-drive transport failure surfaces as a
  truthful per-drive state, never as absence.
- **NG-8 [H] Every `health_read` spawns `fsutil` via `_popen`.**
  `engine/src/win/cr_health.cpp:307-325` shells out to `fsutil` for TRIM
  detection — a visible console flash per query (twice per drive from the
  bridge's two-call read). No shipped surface may spawn consoles.
  **Fix:** native `IOCTL_STORAGE_QUERY_PROPERTY` (DeviceTrimData) in the
  engine; delete the `_popen`.
- **NG-9 [H] Zeroed SMART tables pass as "good" with an empty table.**
  All-zero attribute slots (id==0, `engine/src/core/health/cr_smart_ata.cpp:134-137`)
  and the never-re-checked second ATA call (`health.rs:50-54`) yield status
  "good" + zero rows. The reference shows Unknown/"--------" for drives
  with no decodable table.
  **Fix:** engine marks the table undecodable (available=0 with reason);
  bridge maps to a truthful per-drive state.
- **NG-10 [M] Hard-coded cosmetic values in the health page.**
  "512B"/"2%" literals (`health-page.tsx:403,434`).
  **Fix:** real sector size from device info; remove or derive the rest.

### Other surfaces (user symptom: "everything else dead or fake")

- **NG-11 [C] Lost-Partition page is 100% canned.**
  `ui/src/features/lost-partition/lost-partition-page.tsx:48-172` renders
  fake specs and jumps to `setStep('scan-completed')` without any scan. The
  engine HAS the real capability (lost-partition probes + candidates +
  apply, `engine/include/cr/lost/cr_lostpart.h`).
  **Fix:** wire the page to a real lost-partition scan over the selected
  disk with truthful progress/candidates/apply, or ship it disabled with an
  honest message until wired. Faking is not acceptable.
- **NG-12 [C] Repair page renders a canned queue over `not-implemented`.**
  The bridge truthfully refuses (`repair.rs:41-66`), but the page shows a
  fabricated queue/spec. **Fix:** truthful empty/disabled state wired to the
  real pre-scan repair consent surface (G-1 (a)); no canned data.
- **NG-13 [M] Home folder dialog shows hard-coded subfolder lists.**
  `home-page.tsx:385-437`. **Fix:** derive from real scan results or remove.
- **NG-14 [M] SD-card page filters `kind==='sd'` — a value the type map can
  never produce (G-9).** Dead surface by construction.
  **Fix:** engine `cr_ui_disk_info` gains the bus/interface field
  (append-only ABI), map sd-card membership from it; page shows truthful
  empty state meanwhile.
- **NG-15 [H] No file logging anywhere in the shipped app.**
  `ui/src-tauri/src/lib.rs` logs only via `eprintln` (lost in a windowed
  app); `contracts/README.md` claims a bridge logger writing
  `ProgramData\...\logs` — the directory exists but nothing writes it.
  **Fix:** implement the diagnostic log seam (bridge + engine log callback
  → rotating file under ProgramData), referenced from the installer docs;
  errors become traceable on customer machines.

### Installer & packaging (user symptom: "PowerShell popup, unfinished")

- **NG-16 [C] The installer spawns six visible consoles.**
  Install: `certutil` ×2 (`recoverywizard.nsi:301,307`) + `powershell.exe
  -File install.ps1` (`:349`); uninstall: `powershell` (`:608`) +
  `certutil` ×2 (`:649,651`). All via `ExecWait` (visible windows).
  **Fix:** full installer replacement per the session mandate — see
  `docs/INSTALLER-DECISION.md` (research) and `docs/INSTALLER.md` (the
  shipped behavior). The driver install becomes a native, silent, logged
  mechanism with correct elevation, repair, update, and uninstall.
- **NG-17 [H] `devsign.pfx` + password uploaded as a workflow artifact.**
  `release.yml:176-182` uploads all of `build/out` including the ephemeral
  signing PFX (14-day retention, not published to the release).
  **Fix:** exclude the PFX from artifacts (security hygiene).
- **NG-18 [M, CLOSED this session] Portable builds claimed a session probe
  that did not exist.** `state.rs:513-522` checked only the ProgramData env
  var (always set on Windows), so portable builds silently wrote sessions to
  ProgramData while README-PORTABLE promised beside-the-exe placement.
  **Fix (batch 5):** the deterministic marker probe — `portable.ps1` writes
  `recovery-wizard.portable` beside the exe (never in installed builds);
  `sessions_dir()`/`logs_dir()` check it first (bridge `util::portable_mode`,
  unit-tested). A marker beats the previously documented writability probe
  (which would misdetect elevated installs); contracts/README-PORTABLE now
  describe the marker. Portable zip gains exactly one file.
- **NG-19 [M] Orphan paths in the current uninstaller.**
  Abort path strands the imported cert + inner uninstall entry; `%TEMP%`
  lifecycle files remain; empty ProgramData tree survives. Folded into the
  installer replacement (complete uninstall requirement).
- **NG-20 [L] ~20 engine ABI exports never called by the UI** (image scan
  targets, BitLocker unlock, the query family, resume, apply,
  `cr_ui_last_transport_error`). Not user-visible defects by themselves, but
  `cr_ui_last_transport_error` is the missing diagnostics hook for NG-15.
  Fold into the respective fixes where they close gaps.
- **NG-21 [CRITICAL, CLOSED this session] The pre-scan repair analysis ran
  on the WHOLE DISK — a catastrophic false offer on every healthy
  GPT/MBR disk, and the S3 smoking gun.** Forensic run 40470191039: a
  whole-disk scan of the healthy 64 GiB seed disk handed the DISK slice to
  `repair::analyze()`; a GPT protective MBR is "no recognizable family,"
  and the family-agnostic FixByScanSector terminal then proposed writing
  the partition's healthy NTFS boot sector at **disk offset 0 — over the
  partition table** — blocking the run thread in the consent callback
  ("Quick Scanning 0%" for 28 minutes, IO flat at zero; modal captured
  verbatim, VLM-verified visible). The corpus contract is partition-scoped
  (`CRawFixer::CheckPartition` → per-FS fixers on damaged VOLUMES,
  rawfixer-fs-repair.md:13-16). **Fix (batch 4):** the orchestrator
  analyzes per-partition on whole-disk scans (merged plans, per-partition
  rebasing, caller-declared `whole_disk` as the discriminator so a
  FAT-volume-misparsed-as-MBR can never misroute); `analyze()` gained an
  independent partition-table guard (protective entry 0xEE / "EFI PART" at
  LBA 1 / plausible MBR entries ⇒ S3 clean refusal, never a write). Five
  fail-before/pass-after engine tests pin both layers
  (`WholeDiskHealthyPartitionsNeverOfferRepair` — the S3 regression,
  `WholeDiskDamagedPartitionOfferedInPlaceNeverAtTable`,
  `GptDiskNeverOfferedBootRestore`, `MbrDiskNeverOfferedBootRestore`,
  `GptHeaderAloneRefusesEvenWithoutProtectiveEntry`). See
  `docs/FORENSIC-PROBE.md` §S3 smoking gun.
- **NG-22 [M, CLOSED this session] The results-tree root label was the
  hard-coded fake `'Windows (C:)'`** (`ui/src/i18n/en.ts
  results.tree.existingFiles`) — every scan target's tree root claimed to
  be the Windows volume (captured: a SeedNTFS(X:) scan labeled "Windows
  (C:)", run 40470191039 end-state). **Fix (batch 4):** the explorer passes
  scan state's `target.label` into the tree; the neutral translated
  "Existing Files" is only the unresolved-target fallback. Pinned by
  `ui/tests/results-tree-label.test.tsx`.

### Not gaps (verified this session, for the record)

- The engine's scan, health, and enumeration cores are REAL and substantial
  (926 engine tests; the T14 measurements ran through the mandatory RDA
  driver). The breakage is at the seams (bridge path model, UI rendering,
  installer UX) — not a missing core.
- The driver installs, loads, and services IOCTLs (T15 run 40442056802:
  service RUNNING, `\\.\RDA` opens; to be re-proven on this session's run).
- The RDA transport is genuinely the only raw-disk path (no silent fallback
  masks driver failures) — `engine/src/win/cr_transport.cpp:5-8`.

---

## Open questions requiring a user decision (recorded, not decided)

None at this time. (Any fix that would change user-facing behavior beyond
restoring real functionality will be recorded here with options and
trade-offs before implementation.)
