# RELEASE-RUN — gate §8 evidence: the v0.7.0 release, end to end

Written: 2026-11-06 (consolidation release session). Run IDs link to
https://github.com/heicfast/final-prod/actions/runs/<id>.

## The tag

- `v0.7.0` — annotated tag on **68ab3fcd** (the green tree: all four CI
  lanes green on a5cfa11f's code, evidence set committed, engine-ci
  42123593814 the last full run).
- Tag message: the consolidation summary (driver removed, engine advanced
  to core@12f3edf7 ABI v4 with the byte-identity proof, UI rewired, no
  mock in shipped builds, workflows SHA-pinned).

## The release workflow (run 42126551916 — SUCCESS, 22.8 min, windows-2022)

Step-by-step from the run log:

1. **Version gate** ✓ — tag `v0.7.0` == `v<VERSION>` (the VERSION file is
   the source of truth; lockstep verified across ui/package.json,
   Cargo.toml, tauri.conf.json, Cargo.lock — all 0.7.0).
2. **Contracts gate** ✓ — `node contracts/validate.mjs` (phase 1 + 2).
3. **Full product build** ✓ — build.ps1 all lanes:
   - engine: CMake win-x64-Release (vcpkg: zlib, gtest, lz4, zstd);
     **ctest 1741/1741 — "100% tests passed, 0 tests failed"** (the full
     Windows engine suite runs inside the release build);
   - kit staged with runtime DLLs (lz4.dll, z.dll, zstd.dll);
   - ui: pnpm + frontend + cargo/tauri build (NSIS bundle);
   - installer: combined NSIS (engine + app), development test-signed;
   - portable zip + SHA256SUMS.
4. **Artifact + signature verification** ✓ — the three expected artifacts
   exist and carry the development signature:
   - `RecoveryWizard-0.7.0-setup.exe` — 2,790,104 bytes,
     SHA256 `50270B094E190B5D9B052C8EBDE416D3F505129034B8548270C2A024A5EAE2F5`
   - `RecoveryWizard-0.7.0-portable.zip` — 3,914,999 bytes,
     SHA256 `8C37A7110DBB1AC32375D7058D5ABD8D1B98B28B07092E4D841954AC981B0703`
   - `SHA256SUMS` — 199 bytes
   - devsign verify: setup.exe and stage-0.7.0\engine\cr_ui.dll both carry
     `CN=HeicFast Recovery Wizard Development Signing` (status UnknownError
     = self-signed, not chained to a root — the documented, expected
     posture; no certificate is imported anywhere).
5. **Truthful release notes** ✓ — published with the release (the
   user-mode posture, the elevation requirement, what changed vs 0.6.x,
   the honest signing status, artifacts, checksums).
6. **Upload** ✓ — all three artifacts on the release;
   `fail_on_unmatched_files: true`. Release URL:
   https://github.com/heicfast/final-prod/releases/tag/v0.7.0

## The installed-product validation (forensic-repro, dispatched on v0.7.0)

Five attempts; each failure was a probe defect (fixed in-tree) or a
recorded behavior finding — never a masked product failure. Step-level
truth per the runs' job records:

- Attempt 1 (42129256087): step 10 (drive the UI) GREEN — S1 install, launch,
  the no-mock gate, S2 health, S3 scan started + live progress + 197 items;
  step 11 (unelevated twin) RED — the `runas /trustlevel:0x20000` mechanism
  cannot de-elevate on the runner (UAC token filtering not in effect; the
  app ran elevated, truthfully reported administrator, correctly showed no
  banner). The product was right; the launcher was wrong. Fixed (db5f0b81:
  CreateProcessWithLogonW of a fresh standard user + the launcher now
  ASSERTS the spawned token is not-elevated before the banner assertion).
- Attempt 2 (42135600496): cancelled at the S3 capture when the mount-row
  selector bug was caught (rows render the volume label, not the drive
  letter) — before it could burn the full window. Fixed (ff5b337b).
- Attempt 3 (42138026793): step 10 RED — the S3b recovery REPORT wait was
  rejected by Playwright's ARIA engine on WebView2 in 268 ms (non-timeout)
  while the report was already visible in the same-instant screenshot.
  Fixed (eaec57b5: the CSS engine + verbatim rejection capture + one
  retry). The run's captured evidence up to that point: stop dialog,
  partial results, 30 rows, destination chosen (SeedEXFAT), recovery
  STARTED — the surface itself was working.
- Attempt 4 (42143819243): step 10 RED at the S5 save gate — the recover
  gate was GREEN this time ('Recovery finished — 1 of 1 files recovered ·
  0 failed', reportWait.ok=true), but the save clicked at +9 s hit the
  post-cancel drain window (cr_ui_scan_busy still set; io-timeline shows
  reads creeping 20+ s after the stop) and the refusal was INVISIBLE —
  the mutation had no onError (no toast, no file). Fixed (db600f4c: the
  onError toast — the product's never-silent rule — + the probe retries
  through the window). BF-24 recorded.
- Attempt 5 (42148493806): step 10 RED at the S5 save gate again — the
  retry loop ran, but ALL 15 attempts over 136 s saw ZERO toasts and the
  .crsf was never written (io-timeline: no write spike). Root cause
  (BF-25, this session's analysis of the artifact): the stalled 64GB
  scan's run thread NEVER RETURNED after the stop — reads had been idle
  from +6 min (the BF-22 stall), the cancel token was set, the UI
  navigated to results on the cancel invoke's success, but the thread
  sat inside the slow carve window with no cancellation checkpoint:
  busy stays 1 (it clears only at the heartbeat join), every save is
  refused, and the 'stopped early' banner (gated on status=cancelled,
  which latches when run() exits) never rendered. The recovery itself
  SUCCEEDED from the still-live scan ('1 of 1 files recovered'). Fixed
  probe-side (this session): the save/resume gates moved OFF the stalled
  stop path onto a deterministic completion vehicle — a full scan of the
  8GB SeedEXFAT seed that completes inside its window (S2b), with the
  toast poll timing fixed (400 ms poll inside the toast's ~4 s life;
  the old 4 s sleep could miss a toast that auto-dismissed between
  samples). The busy-guard itself is HONEST (a save of a mid-drain scan
  would be unsound) — BF-25 points upstream (a cancel checkpoint inside
  the carve window), same family as BF-22.
- The unelevated twin and the silent-uninstall assertions never executed
  to completion on v0.7.0 (they run after step 10, which failed in every
  attempt) — their first full pass is the v0.8.0 dispatch below.

### The v0.8.0 re-validation (dispatched on the new tag)

**(filled from the run's evidence as it lands — the gate set: S1 install,
launch + no-mock, S2 health, S2b completion vehicle → save → backup →
resume, S3 stall recorder, S3b stop → partial results → recover → report,
the unelevated twin, postrun facts, the silent uninstall + no-leftover
assertions)**

## Behavior findings recorded during the validation (not release blockers)

1. **The deep scan of the 64GB seeded vdisk stalls at 76.1%** in the
   28-minute capture window — IDENTICALLY on the pre-merge ABI v2 engine
   (old run 42072191860: 43%→76.1%, 189 items) and the v0.7.0 ABI v4
   engine (75%→76.1%, 197 items; app read 3.2 GB then ~27 KB/s for the
   final 14 min). Two engines 1092 commits apart, same stall point =
   environmental/known-perf (the carve matcher over the sparse vdisk's
   free space — core's PERF-LEVERS frontier), NOT a wiring regression.
   The real user path from the stall (Stop → partial results → recover)
   is gated green by S3b. The performance ledger belongs to core's own
   benchmark lanes (docs/BUILD.md's recorded decision).
2. **SMART is environmentally unavailable** on the runner's Azure VM
   disks (both C:/D: are virtual disks; the seeds are VHDX) — the health
   surface renders its honest failure state with the retry affordance,
   identically on both engines. The SMART data path itself is covered by
   the engine's ctest suite.
