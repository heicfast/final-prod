# FORENSIC PROBE — what the shipped product actually does (captured evidence)

The cross-layer forensic probe of the installed product. Every observation
below is captured evidence from a real Windows runner (GitHub-hosted
`windows-2022`, elevated, Secure Boot OFF, image built with TESTSIGNING ON),
running the **shipped v0.6.0-rc1 release build** (downloaded from the release
page, checksum-verified) against seeded disks. Evidence artifacts live in the
forensic-repro workflow runs (URLs cited); the curated copies are
`docs/INTEGRATION-VALIDATION/forensic/`.

Reproduction runs (chronological):

| Run | Tree | Outcome |
|---|---|---|
| 40463774709 / 40464046107 / 40464495050 | 5cb10f29→7517866b | probe-tooling iterations (checksum hex case; Unicode window text; step job-object kill) — S1 captured from run 3 on |
| **40465852724** | 7517866b | **GREEN — the full S1/S2/S4 capture; S3 scan phase re-run below** |
| (scan-phase rerun) | 1f45b88c | the scan capture (quick→advanced, the 40% crawl, the results tree) |
| **40470191039** | a6e9e470 | **GREEN — the definitive S3 capture: the blocking consent modal on the HEALTHY seed disk (the smoking gun below); probe did not answer it (probe defect, fixed)** |

The probe suite is `tools/forensic/` + `.github/workflows/forensic-repro.yml`
(diagnostic tooling only — no product code is modified; the UI is driven
through WebView2's CDP port on the real installed app, exactly what a user
sees).

## Machine and fixture context (run 40465852724)

- windows-2022 runner, elevated; Secure Boot `false`; `testsigning Yes`
  (facts-preinstall.json) — the same posture the release notes instruct for
  the test-signed driver.
- Seeded disks (seed-disks.ps1; `seed-manifest.json`): 64 GB GPT/NTFS vdisk
  (`X:`, label `SeedNTFS`, 190 files, 61 deleted, real JPEG/PNG/PDF/ZIP/MP4
  signatures) + 8 GB MBR/exFAT vdisk (`Y:`, label `SeedEXFAT`, 60 files, 15
  deleted). Plus the runner's own OS disks (C:, D:).
- Install: exit 0 in 15.7 s; 15 files placed; RDA service **Running**;
  driver file present; dev cert in LocalMachine Root + TrustedPublisher;
  uninstall key registered (facts-postinstall.json).

## S1 — the installer's visible PowerShell/consoles: REPRODUCED

Captured in `install-process-events.jsonl` + `install-window-events.jsonl`
(400 ms sampling of every spawned process and every top-level window during
the whole install):

- `powershell.exe -NoProfile -ExecutionPolicy Bypass -File "C:\Program
  Files\HeicFast\Recovery Wizard\driver\install.ps1" ...` — **26 samples ≈
  10.4 s of visible console window** (class `ConsoleWindowClass`, titles
  `C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe` and
  `Administrator: ...powershell.exe`).
- `certutil -addstore -f Root ...rda-testcert.cer` and `certutil -addstore
  -f TrustedPublisher ...` — visible console titled
  `C:\Windows\SYSTEM32\certutil.exe`.
- 28 conhost.exe samples — the console hosts for the above.

Root cause (source): `installer/recoverywizard.nsi:301,307,349` (install) and
`:608,649,651` (uninstall) — `ExecWait` on console executables. Confirmed
independently by the context sheets (`docs/INSTALL-FOOTPRINT-CURRENT.md` §F).

## S2 — SMART surface: REPRODUCED (panel empty on every drive; percentage on
real hardware is UI-fabricated)

The Health page (driven through the real UI, screenshots 02-06):

- Device enumeration is **not** the failure on this runner: the drive
  selector lists all four disks — `Windows(C:)`, `Temporary Storage(D:)`,
  `SeedNTFS(X:)`, `SeedEXFAT(Y:)` (evidence.json → health.initial.buttons).
- For **every** drive the panel below the selector renders the
  not-available EmptyState: no status, no percentage, no temperature, no
  power-on hours, **no attribute table** (`attrRows: 0`, `statusWords: []`,
  `tables: []` for all four; screenshots 03-06). On virtual disks the engine
  truthfully refuses (`ERR_HEALTH_UNAVAILABLE` — no SMART transport for
  vdisks), and the pre-fix page had nothing truthful to show.
- The runtime **console flash** is captured: the app process spawned
  `cmd.exe /c fsutil behavior query disabledeletenotify 2>nul` during the
  health reads (io-timeline.jsonl, children field) — the `_popen("fsutil
  ...")` at `engine/src/win/cr_health.cpp:301-325` (now replaced with the
  native IOCTL).
- The user's machine (real ATA/NVMe drives) additionally showed a health
  **percentage**: that number is UI-fabricated —
  `ui/src/features/health/health-page.tsx:100-106` rendered
  good→`max(88, 100−wear)` (≥96%), caution→72, bad→38 **even with zero
  attributes** (removed in fix batch 1). The NVMe-only refusal
  (`health.rs:43-48` misreading `available==0`) made NVMe drives show
  "not available" instead of their real NVMe health log — also fixed.
- The "only ONE drive reported" half of the symptom is the **Home page**
  filter: `home-page.tsx` rendered only `kind === 'internal'` devices;
  removable disks were hidden behind a `?media` URL param that **no
  affordance in the UI ever sets** (verified: zero references to `media=`
  anywhere in `ui/src`). All USB/SD/external disks are invisible on Home.
  Fixed in batch 1 (both groups always rendered).

## S3 — scan pipeline: 40% crawl + no results

Source-level break points (all verified against the shipped tree; the scan
phase capture is the run at 1f45b88c):

1. **40% is the quick→deep stage-weight boundary by construction** —
   `engine/include/cr/orchestrator/cr_scan.h:71-75` (default weights
   40/35/25): quick done ⇒ overall = exactly 40%, deep runs 40→75%.
2. **Deep-scan progress stalls by design on large volumes** — progress
   ticks only when a parallel 4 MiB chunk completes
   (`engine/src/core/tree/cr_mftwalk.cpp:2117-2164`); a single-threaded
   remnant sweep emits nothing, and the heartbeat repeats the standing
   permille (`engine/src/ui/cr_ui.cpp:875-916`). On a multi-TB disk this is
   hours at 40.x% — the user's "40.2%, 40.4%, …".
3. **Results never appear mid-scan** — `ui/src/queries/scan.ts:41,55-60`
   refetches the results tree only when `floor(overall*10)` changes; during
   the entire 40→50% band (the whole deep stage on big disks) nothing
   refreshes even when nodes arrive.
4. **The reference behavior** (`docs/REFERENCE-BEHAVIOR.md` §B): results
   are visible DURING the scan, the tree is browsable mid-scan, "Recover
   All" is offered mid-scan, and advanced scanning shows sector-linear
   progress ("Reading sector X/Y") with a remaining-time estimate. Our
   product showed none of these.
5. Quick-pass probe failures yield an empty-but-OK quick stage
   (`cr_scan.cpp:1345-1348`) and driver read failures degrade to silent
   bad-sector ranges — neither surfaces to the UI.

### S3 smoking gun — the false repair offer that blocks every whole-disk scan
(run 40470191039, evidence `evidence.json` phases.scan.result.bodyText +
screenshots 09→65 byte-identical for 28 minutes; modal verified visible by
VLM inspection of `09-scan-poll-0s.png`)

The scan of the **healthy, freshly-formatted 64 GiB GPT seed disk** (disk-2,
SeedNTFS X:) never entered the quick stage at all: within a second of
"Search for Lost Data" the engine's **pre-scan repair analysis** proposed a
repair and the run thread **blocked in the consent callback** — the UI showed
"Quick Scanning 0%" behind a centered modal for 28 minutes (IO flat at zero
the entire window, io-timeline.jsonl). The captured modal text:

> Pre-scan repair request … File system: ntfs, Repair actions: 1,
> Report-only findings: 2 — *"no recognizable family in the primary or
> backups; rebuild the DBR from the only boot-shaped sector in the 64 MiB
> window (FixByScanSector)"*

Root cause chain (source-verified, each link cited):

1. **The analysis unit was the DISK, not the partition.**
   `cr_scan.cpp` run() handed the whole-disk slice to
   `repair::analyze()` (`engine/src/core/orchestrator/cr_scan.cpp:918`
   pre-fix) — while the corpus contract is partition-scoped:
   `CRawFixer::CheckPartition → per-FS fixers on damaged VOLUMES`
   (`easeus-drw-research/re-workspace/artifacts/PartitionRecovery/
   rawfixer-fs-repair.md:13-16, :23-24`).
2. **A GPT disk's LBA 0 is a protective MBR** — no NTFS/exFAT/FAT boot
   shape, no family. The family-agnostic **FixByScanSector terminal**
   (`cr_rawfixer.cpp:868-912` pre-fix) then scanned the first 64 MiB of the
   DISK, found the NTFS volume boot sector at ~1 MiB (the partition's
   healthy boot), and proposed **writing it at disk offset 0 — over the
   GPT protective MBR**. On a healthy disk. Approving it would have
   destroyed the partition table.
3. **The consent gate is blocking by design** (S-008: the engine's run
   thread waits in the callback) — the modal was visible and correctly
   rendered (VLM-verified: title, plan details, both buttons), but a user
   who misses it sees a "scan" at 0% forever, and the forensic probe (which
   never answered) reproduced exactly the reported symptom shape: no
   results, no progress, nothing completes.
4. The **tree root was mislabeled** "Windows (C:)" during the disk-2 scan —
   the label was hard-coded in `ui/src/i18n/en.ts`
   (`results.tree.existingFiles: 'Windows (C:)'`), not derived from the
   scan target (NG-22, fixed with the target's real label).

Fixes (batch 4): the orchestrator now analyzes **per-partition** on
whole-disk scans (the corpus unit) with per-partition rebasing;
`analyze()` gained an independent **partition-table guard** (protective-MBR
entry / GPT header at LBA 1 / plausible MBR entries ⇒ clean S3 refusal,
never a write); the tree root label comes from scan state's target. Five
fail-before/pass-after engine tests pin it
(`WholeDiskHealthyPartitionsNeverOfferRepair`,
`WholeDiskDamagedPartitionOfferedInPlaceNeverAtTable`,
`GptDiskNeverOfferedBootRestore`, `MbrDiskNeverOfferedBootRestore`,
`GptHeaderAloneRefusesEvenWithoutProtectiveEntry`); the probe now ANSWERS
the modal (Deny) if one ever appears and records its plan text verbatim
(`tools/forensic/drive-ui.mjs`).

## S4 — other surfaces: dead or fake (screenshots 09-13)

- **Lost Partition page: 100% canned** — fake partition specs, a fake
  progress sequence, and `setStep('scan-completed')` without any scan
  (`ui/src/features/lost-partition/lost-partition-page.tsx:48-172`).
- **Repair page: canned queue** over the typed `not-implemented` refusals
  (`repair.rs:41-66`) — the page fabricates a queue UI.
- **SD Card page: dead by construction** — filters `kind === 'sd'`, a value
  the device type map can never produce (G-9).
- **Home folder dialog: hard-coded subfolder lists** (`home-page.tsx:385-437`).
- App logs: the shipped app writes **no log files anywhere** (app-stderr.log
  and app-stdout.log are empty; the shell logs only via `eprintln`,
  `ui/src-tauri/src/lib.rs:76-83`) — an installed-app failure leaves no
  evidence trail.

## Product defects discovered by the probe itself

- **`rdacli.exe` (shipped in the install) cannot talk to the shipped
  driver**: it opens `\\.\EPRDTDRV` (gle=2, file not found) while the engine
  addresses `\\.\RDA` (`engine/src/win/cr_transport.cpp:306-323`,
  `driver/src/client/rdacli.c:32-36`). facts-postinstall.json `rdacli`
  field. Fix scheduled with the driver tooling lane.
- WebView2 font fetch warnings in console.jsonl (Tracking Prevention) —
  cosmetic, no JS exceptions captured (`pageerror.jsonl` empty).

## What the probe proves about the driver

- The RDA service is **Running** after install (facts-postinstall.json) and
  the engine is RDA-only (`cr_transport.cpp:5-8`, no fallback) — every raw
  read during a scan flows through the driver by construction; the app's
  read counters during the probe window (+6.4 GB) confirm real IO through
  the app while surfaces are exercised (io-timeline.jsonl).
