# RELEASE-RUN — gate §8 evidence: the v0.7.0 + v0.8.0 releases, end to end

Written: 2026-11-06 (consolidation release sessions). Run IDs link to
https://github.com/heicfast/final-prod/actions/runs/<id>.

## The v0.8.0 release (this session)

### The tag

- `v0.8.0` — annotated tag on **f1c0418f** (the version-bump commit; the
  product fix db600f4c + the probe re-vehicle 95d5d1de beneath it).
- CI at the tag: ui-ci 42166263292 GREEN, ui-e2e 42166263357 GREEN on
  f1c0418f; contracts validated locally + inside the release gate; engine
  unchanged since its last green lane.
- Tag message: the release summary (the session-save visibility fix as
  the product delta; BF-25 recorded — unbounded cancel latency inside the
  deep-scan stall, upstream core scope).

### The release workflow (run 42167758364 — SUCCESS, 22.0 min, windows-2022;
job 127728096420, every step green)

1. **Version gate** ✓ — tag `v0.8.0` == `v<VERSION>`; lockstep held
   (ui/package.json, ui/src-tauri/Cargo.toml, tauri.conf.json, Cargo.lock —
   all 0.8.0).
2. **Contracts gate** ✓ — `node contracts/validate.mjs` (phase 1 + 2,
   44 commands).
3. **Full product build** ✓ — build.ps1 all lanes; **ctest 1741/1741 —
   "100% tests passed, 0 tests failed"** (log line 4401); kit staged
   (lz4/z/zstd DLLs), UI + Tauri NSIS bundle, combined installer, portable
   zip, SHA256SUMS.
4. **Artifact + signature verification** ✓ — devsign verify on the
   installer AND stage-0.8.0\engine\cr_ui.dll (both
   `CN=HeicFast Recovery Wizard Development Signing`, status UnknownError =
   self-signed, the documented posture):
   - `RecoveryWizard-0.8.0-setup.exe` — 2,790,696 B, SHA256
     `AF797684917CA26268B12A9A5576D671F2D9C737FBD67934E0E91C8F93A591D4`
     (byte-verified by an independent download of the published asset)
   - `RecoveryWizard-0.8.0-portable.zip` — 3,915,357 B, SHA256
     `4A26D3154FEDA68E32532D6F5BF6C482233D879D3AFB18FE728FAE85C6D1EBE4`
   - `SHA256SUMS` — 199 B
5. **Truthful release notes** ✓ — the 0.8.0 story: the session-save
   visibility fix, "everything else is the proven 0.7.0 product", the
   0.7.0 driver-removal history pointer, the honest signing status.
6. **Upload** ✓ — all three assets on the release
   (https://github.com/heicfast/final-prod/releases/tag/v0.8.0,
   published 2026-11-06T05:41:34Z); `fail_on_unmatched_files: true`.

---

## The v0.7.0 release

## The tag

- `v0.7.0` — annotated tag on **68ab3fcd** (the green tree: all four CI
  lanes green on a5cfa11f's code, evidence set committed, engine-ci
  42123593814 the last full run).
- Tag message: the consolidation summary (driver removed, engine advanced
  to core@12f3edf7 ABI v4 with the byte-identity proof, UI rewired, no
  mock in shipped builds, workflows SHA-pinned).

## The release workflow (run 42126551916 — SUCCESS, 22.8 min, windows-2022) — v0.7.0

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

### The v0.8.0 re-validation (forensic-repro dispatched on v0.8.0)

**Run 18 (42170466748) — RED at the S5 resume gate, with the run's own
green evidence and two findings (step-level: steps 1–9 green; step 10
red):**

- The v0.8.0 binary's no-mock gate ✓; S2 health ✓ (4 selections);
  **the vehicle scan completed in 11.8 min** and **save attempt 1 landed
  the 'Search saved' toast** — the db600f4c onError/save path proven
  GREEN end-to-end on the shipped release (the S3b recover leg never
  ran; the failure preceded it).
- Finding 1 (product, mechanical — fixed this session): the Backup
  screen rendered the saved session as **"(unreadable session)"** — a
  VALID whole-disk session with an empty volume label hit the bridge's
  `valid && !label.is_empty()` conditional (BF-26; fixed in render_row:
  "(no volume label)").
- Finding 2 (probe, mechanical — fixed this session): the S5 gate
  demanded a "Resume scan" affordance — but the Backup screen renders
  Resume ONLY for incomplete sessions; a completed session's row carries
  just "Open". The gate was asking the product for an affordance its
  design never shows on that row. The vehicle was redesigned: stop the
  8GB scan mid-deep (healthy), GATE the drain (the "stopped early"
  banner — the run-thread-returned signal), save the INCOMPLETE session,
  gate the Resume affordance + live-scan URL, then gate the resumed
  scan's completion (fresh-scan baseline 11.8 min).
- The unelevated twin + uninstall legs remain unexecuted on a v0.8.0
  tag (step 10 red precedes them) — their first full pass moves to the
  v0.8.1 dispatch (the fixed probe + fixed bridge).

### The v0.8.1 release (the mislabel fix)

- Tag `v0.8.1` on **97893c15** (the BF-26 bridge fix 73419713 + the
  version bump; CI: ui-ci 42173490484, ui-e2e 42173490489, contracts-ci
  42173490533 — all GREEN).
- Release run **42174434974 — SUCCESS, 18.8 min** (job 127749513646,
  06:26:26Z→06:45:14Z): version + contracts gates green, **ctest 1741/1741
  "100% tests passed, 0 tests failed"** (log line 4401), dev-sign verified
  on the installer and cr_ui.dll, all three assets published
  (2026-11-06T06:45:08Z):
  - `RecoveryWizard-0.8.1-setup.exe` — 2,790,224 B, SHA256
    `F316F7EC3FB85D29A5DA46FA851F13547298FCCBAA3B2A51AD526D9E5230721B`
    (byte-verified by an independent download of the published asset)
  - `RecoveryWizard-0.8.1-portable.zip` — 3,915,146 B, SHA256
    `E268156357D478C8059AC04A7B6EF9DF5A386850BFAF655A3D1B45B69B6864C3`
  - `SHA256SUMS` — 199 B
- Notes: the 0.8.1 truth (the Backup-list label fix, the carried save
  visibility fix, "everything else is the proven 0.7.0 product").

### The v0.8.1 forensic dispatch — attempt 1 (run 19, 42176600928)

RED at the S2b DRAIN gate — and the red is a REAL generalization of
BF-25, not a probe bug alone: the probe stopped the vehicle scan
"15 s into advanced" assuming mid-DEEP, but on the exFAT seed the deep
stage is nearly empty (the remnant sweep is NTFS-only), so the stop
landed 0.6% into CARVE. The io-timeline: reads ceased ~90 s after the
cancel (06:50), yet the run thread had not returned by 06:52:07 (the
180 s drain window) — the carve matcher grinds on in memory for minutes
with no cancellation checkpoint (the thread observes the token only
when the matcher finishes; the fresh-scan baseline for the whole pass
was 11.8 min in run 18). Two probe corrections (this is tooling — the
v0.8.1 binary needs no change, the probe runs from main):
1. The drain window is sized to the measured reality: 180 s → 15 min
   (the matcher's remaining grind after a ~2-min-in stop is ~9-10 min).
2. NEW during-drain save probe: a save clicked while the thread is still
   draining MUST show the refusal toast (the db600f4c never-silent
   proof — never before exercised on a shipped binary; silence = red).
   The S3 stall-recorder window is trimmed 28 → 15 min (the full 28-min
   corpus exists in runs 42072191860 + 42148493806; the frozen-at-76.1%
   signature still records) so the longer vehicle fits the 90-min job.
The green evidence run 19 did capture before its gate: install, no-mock
gate, S2 health (4 selections), the vehicle started, advanced stage,
the stop dialog + confirm + navigation, and (io) the reads ceasing on
cancel — the product's stop path itself behaved exactly as designed.

### The v0.8.1 forensic dispatch — attempt 2 (run 20, 42178485694)

RED at the same S2b drain gate — and this time the artifact CONVICTED A
REAL PRODUCT BUG (the v0.8.2 fix):

- The during-drain save (NEW this run) returned **"Search saved" within
  3 s of the stop** — the engine's run thread had ALREADY returned
  (busy=0; the carve pool watcher caught the cancel immediately this
  time — run 19's 180 s+ drain and run 20's ~3 s drain bracket the
  checkpoint-timing variance).
- Yet the "stopped early" banner NEVER rendered in 15 minutes. If the
  thread returned, the terminal latch says 'cancelled' and the banner's
  condition (status === 'cancelled') is met — unless the UI never
  FETCHED the terminal state. It didn't: `useScanState`'s refetchInterval
  polled only while `running`/`starting`; the first post-stop poll
  returns **'stopping'** (the drain), the interval turns off, and the
  query freezes at 'stopping' FOREVER. The banner could never render
  after ANY stop. The TEST DOUBLE's synchronous cancel
  (mock-connector.ts:482 — 'cancelled' set immediately, no stopping
  phase) is why all 174 unit + 14 e2e proofs stayed green while the
  shipped product never showed the banner.
- The fix (v0.8.2): the interval keeps polling through 'stopping' until
  the terminal state lands; pinned by a new regression test
  (scan-state-drain.test.tsx — scripted stopping→stopping→cancelled;
  verified to FAIL on the old interval and PASS on the fix; 175/175).

### The v0.8.2 release (the stopped-scan banner fix)

- Tag `v0.8.2` on **ff9e4c13** (the poller fix f76c9941 + the bump; CI:
  ui-ci 42181863436, ui-e2e 42181863373 — GREEN, 175/175 unit incl. the
  new drain pin).
- Release run **42182876659 — SUCCESS** (~22 min): all gates green,
  ctest 1741/1741, dev-sign verified, three assets published
  (2026-11-06T08:03:34Z):
  - `RecoveryWizard-0.8.2-setup.exe` — 2,790,576 B, SHA256
    `64153954250C4B1B1DF9A9176CB2FBB617292E458D2D272F898243C97DA2D923`
  - `RecoveryWizard-0.8.2-portable.zip` — 3,915,312 B, SHA256
    `C91CE54695236CF3CB838BFD204D1E8B925BD9E17DE032819F765D2005A10974`
  - `SHA256SUMS` — 199 B

### The v0.8.2 forensic dispatch (run 21, 42185123973)

RED at the S5 resume-navigation gate — with BOTH v0.8.2 fixes PROVEN
GREEN on the shipped binary first:

- The during-drain save: "Search saved" within 3 s of the stop (the
  engine thread returned fast — the carve checkpoint variance brackets
  ~3 s to minutes) ✓
- **The drain banner: VISIBLE** — the v0.8.2 poller fix works on the
  installed release (the exact failure of runs 19/20) ✓
- The incomplete-session save attempt 1: "Search saved" ✓
- The Backup screen: rows render **"(no volume label)"** (the v0.8.1
  BF-26 fix proven on a shipped binary) with **Resume scan** affordances
  on both incomplete rows ✓
- THEN the resume click never navigated within 60 s. Root cause (a REAL
  product bug, the third the probe has caught): the backup page
  navigated `/scan?scanId=<id>` — **a route the router does not define**
  (the only scan route is `scan/:deviceId/live`); the catch-all fallback
  swallowed the navigation and the resumed scan ran INVISIBLY (the
  session loaded, the engine re-ran — no page showed it). The probe's
  own waitForURL pattern (`/\/scan\//`) could never match either —
  both fixed.

### The v0.8.3 release (the invisible-resume fix)

- Tag `v0.8.3` on **939dd483** (the route fix 1cd9bc64 + the bump; CI:
  ui-ci 42187695792, ui-e2e 42187695816, contracts-ci 42187695975 —
  GREEN, 176/176 unit incl. the route pin).
- Release run **42188868989 — SUCCESS** (~23 min): all gates green,
  ctest 1741/1741, dev-sign verified, three assets published
  (2026-11-06T08:59:03Z):
  - `RecoveryWizard-0.8.3-setup.exe` — 2,790,176 B, SHA256
    `859A945F7923E18EC600ED614ED2D2A66AD6B533ECF6C9B11E815FA8C0B78FBC`
  - `RecoveryWizard-0.8.3-portable.zip` — 3,915,423 B, SHA256
    `08D7BD19D09C1BD415E3451DA88252C582D42703CA435EC4C4D111E9751CF436`
  - `SHA256SUMS` — 199 B

### The v0.8.3 forensic dispatch (run 22, 42192131100 + run 23, 42195967961)

Run 22: the S5 resume gate timed out silently again (no toast, no
navigation, no IO) — with no diagnostics captured. The instrumentation
commit (a5e6ae7a: the probe's URL+toast polling, the app.log capture,
the session lifecycle logging) followed; CI green
(ui-ci 42194741077, ui-e2e 42194741132, contracts-ci 42194741122).

Run 23 (the instrumented probe, v0.8.3 binary): the diagnostic net
CAUGHT IT — the toast, verbatim: **"the session's source device is not
attached — resume needs the original volume/disk"**. The 'hang' was
never a hang: the invoke rejects fast with the typed DEVICE_GONE
refusal; runs 21/22's probes simply never sampled toasts (the ~4 s toast
life). Root cause (BF-27): every Azure VM disk reports model "Virtual
Disk" with an empty serial → the model/serial match is 4-way ambiguous;
the volume pin demands a lettered volume whose offset+bytes equal the
session's slice, but the vehicle's scan is WHOLE-DISK (the slice IS the
disk); the unique-match fallback then refuses. The fix (v0.8.4): among
the model/serial matches, the disk whose total size equals the
whole-disk slice is picked when unique — the boot-hash verify remains
the authority for a wrong pick (the typed session-corrupt refusal).

### The v0.8.4 release (the whole-disk re-attach fix)

**(filled from the runs' evidence as they land — tag, release workflow,
forensic dispatch on v0.8.4: the full gate set incl. the during-drain
save, the drain banner, the incomplete save, the Backup Resume landing
on the LIVE-SCAN route, the resumed completion, the unelevated twin,
and the silent uninstall)**

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
