# OPEN-ITEMS — the small-bug hunting session (2026-11-26), precisely-scoped deferrals

Live document. Every item below was found, evidenced, and classified in this
session's audit sheets (`worklog/SMALL-BUGS-SOURCE.md`, `worklog/WIRING-AUDIT-SMALL-BUGS.md`,
`worklog/NUMERIC-DEFECTS.md`) and the consolidated `worklog/DEFECT-LEDGER.md`;
each carries its file:line and fix direction there. Fixed items live in
`worklog/FIXES/<defect-id>/` with before/after evidence.

## Fixed this session (see worklog/FIXES/)

| ID | Fix | Evidence |
|---|---|---|
| SB-B-02 (P0) | allow-destroy grant + the exit-guard close race + capability gate test | worklog/FIXES/SB-B-02/ |
| SB-B-01 (P1) | Save-and-exit: stop → bounded drain → the REAL named save (queries-layer flow) | worklog/FIXES/SB-B-01/ |
| SB-E-01..05 (P1/P2) | the premature progress family (FAT/exFAT/ext4/btrfs/HFS+ ticks) + the sibling-subdir regression pin | worklog/FIXES/SB-E-01/ |
| SB-A-01 (P1) | metas_snapshot Arc swap (no per-query deep clone) | worklog/FIXES/SB-A-01/ |
| SB-A-08 (P2) | onError surfacing for stop/pause/resume/cancel + refusal-toast pin | worklog/FIXES/SB-A-08/ |
| SB-B-04 (P2) | session-load-progress hex-keyed event name + charset pins (TS + Rust) | worklog/FIXES/SB-B-04/ |

## Open items (precise scope, ready to execute)

### 1. SB-B-03 (P1) — the unlock dialog's FVEK field is a dead control
Scope: `ui/src/features/device-selection/unlock-volume-dialog.tsx:37-52` renders
the FVEK input and enables Unlock when only it is filled, but `onUnlock` sends
`{ volumeId, passwords }` only — the `fvek` state is never read. The real
key-open chain exists end-to-end with zero UI callers: `contract.ts:211
ScanParams.bitlockerFvek` → `tauri-connector.ts:268-275` →
`commands/scan.rs:183-198 cr_ui_open_volume_with_key` → engine M-033.
Fix: carry the entered FVEK into the scan start's `bitlockerFvek` param for
that volume (the unlock dialog's natural consumer), keep the dictionary
fallback for passwords; contract + mock + tests (the mock currently unlocks
unconditionally — mock-connector.ts:653-669 — tighten it to parity so the
lane can see the wire). ~1 focused session.
Interim truth: a FVEK-only user gets a guaranteed dictionary failure toast
today (the unlock attempts passwords only).

### 2. SB-A-02 (P1) — scan_close has no TS caller: per-scan entry leak
Scope: `commands/scan.rs:819-852` implements the teardown (registered at
`lib.rs:57`); the connector contract has no `closeScan` op (grep: zero
callers). Every scan_start/session_load/session_resume leaves its registry
entry — engine handle, open device, metadata cache, channel — for the app
run. Fix: (a) add `scan.closeScan` to `contract.ts` + `command-surface.json`
+ the tauri connector + the mock; (b) call it when a results scope leaves
the scan (results-route exit / before a new scan starts on the same target),
keeping the busy/job refusals honest. Care: results browsing needs the
snapshot alive — close on the route's scope-exit, not on navigation within
it. Contract parity + RTL coverage of the exit path + registration count
(59→60) via validate.mjs. ~1 session.
NOTE: the Arc change of SB-A-01 already removed the per-query COPY cost;
this item bounds the retained CACHE itself.

### 3. SB-A-03 (P1*) — recover-all mid-scan pages a shifting window
Scope: `commands/recovery.rs:145-151` (snap_ptr captured once) + `202-252`
(the paged loop) + `bridge/state.rs:640-676` (refresh_snapshot installs a new
generation at ≥1500 ms). Needs a Windows harness with a live scan +
recover-all + an instrumented enqueue set to prove the skip/duplicate (the
wrong-file-by-index half is mitigated by arena-order stability —
`cr_snapshot.cpp:28-63`). Fix direction: pin the generation (freeze the
snapshot at loop start) for both query and enqueue, or loop until `total`
stops changing and restart from 0 when it grows. ~1 session + the harness.

### 4. SB-A-05/06/07 (P2) — scan_start / recovery_start / session_resume error-path leaks
Scope (each: RAII guard or explicit close-on-error before every early `?`):
- scan_start: `commands/scan.rs:183-346` (+ the image twin `357-495`) —
  leaks the opened device, the raw ConsentPipe box, and the scan handle.
- recovery_start: `commands/recovery.rs:127-285` — leaks the created
  CrUiRecovery handle + boxed sink sender on set_destination/enqueue refusal
  (the realistic bad-destination retry loop compounds it).
- session_resume: `commands/sessions.rs:529-624` — leaks the fresh scan
  handle + consent pipe on every early error except the identity-mismatch
  branch (which gets it right — mirror it).
Rust-only; compile-proof rides the Windows CI lanes (no local toolchain in
this sandbox — documented in worklog/FIXES/SB-A-01/README.md).

### 5. SB-A-09 (P2) — icon (tile) view truncates at 120 rows
Scope: `ui/src/components/ui/virtual-file-list.tsx:428` —
`rows.slice(0, 120)`, no virtualization, no loadMore wiring; the details
view already has the virtualizer + infinite-scroll loader in the same file.
Fix: virtualize the grid with the existing virtualizer or wire loadMore +
a truncation hint. RTL/e2e coverage: results > 120 rows reachable in icon
view. ~half a session.

### 6. SB-A-10 (P2) — imaging overlap gate silently disabled on enumeration failure
Scope: `commands/imaging.rs:225` —
`enumerate_volumes().unwrap_or_default()` swallows the failure and the
same-disk overlap gate opens (backstop = a corrupt image, not an error).
Fix: on enumeration failure keep the gate closed with the honest message
(the adjacent free-space probe already degrades honestly — mirror it).
`system.rs:190` swallows the same way but the engine's enqueue refusal
0x50050001 backstops it — leave that one, note it. Rust-only.

### 7. SB-E-06 (P2) — recovery live counters add non-disjoint engine stats
Scope: `bridge/events.rs:653-656` — `done_count = stats.done + stats.salvaged`
(salvaged ⊂ done) and `failed_count = stats.errors + stats.hash_mismatch`
(hash_mismatch items still commit and count in done). Worked example in
NUMERIC-DEFECTS.md: 10 items can render "10/10 + 2 failed + 1 skipped" (13
outcomes; can read 12/10). Fix: `done_count = stats.done`, `failed =
stats.errors` (mismatches surfaced per-item — the report rows in
commands/recovery.rs already classify disjointly). Rust-only.

### 8. SB-E-07 (P2) — recovery size estimate poses as measured + gates space
Scope: `ui/src/features/recovery-queue/recovery-page.tsx:80-95` —
`avgFileBytes = filesFound>0 ? dataFound/filesFound : 2_400_000` (a
hard-coded phantom average), `neededBytes = count × avg` — presented as
"N files (size)" and sent to `system_validate_destination` (system.rs:230)
as the out-of-space gate. Fix: sum the actual per-node sizes (the tree's
snapshot has them — one query pass over the selection), render "≈" while
an estimate, and stop gating on the estimate (the engine's N-015 pre-check
is the truthful gate). ~1 session incl. RTL.

### 9. SB-E-08 (P2) — scan ETA window mismatch
Scope: `bridge/events.rs:540-554` — `bps = bytes_scanned/elapsed_s` where
bytes_scanned restarts per deep sub-pass (cr_scan.cpp — deliberate) while
elapsed is whole-run; sub-pass restart → ETA balloons to the 1e7 s range.
Fix direction: Δbytes/Δt between samples (the correct pattern already
exists at events.rs:622-632 for recovery/image jobs — port it). Rust-only.

### 10. SB-E-12 (P2) — lost-partition Size column hard-codes 512 B sectors
Scope: `ui/src/features/lost-partition/lost-partition-page.tsx:216` —
`sectorCount * 512` (8× under on 4Kn; the device row's real sectorSize is
in scope). Fix: use the device's sectorSize. One-liner + a 4Kn test fixture.

### 11. SB-A-04 (P2) — lost-candidate cap: mismatched pairing + saturating count
Scope: `bridge/events.rs:412-431` — at the 4096 cap the event pairs the NEW
lba with the OLD last row's sectors/fs/score, and `count` (len()) saturates
while state.rs:37-38 documents "count-exact". Fix: an exact
`lost_candidate_total` counter + emit null/only-matched geometry for
non-pushed candidates. Rust-only.

### 12. SB-B-05 (P2-info) + SB-B-07 (P3) — dead registered command + dead route
Scope: `image_target_list` registered with zero callers (manifest claims
dev-panel/diagnostics consumers — they never invoke it; fix the manifest
text or wire the dev panel); router.tsx:33's `/tools` route has no inbound
link (remove or wire the sidebar). Trivial.

### 13. SB-A-11 (P3) — connector on* disposers drop listen() rejections
Scope: `tauri-connector.ts` — all 13 `on*` methods' disposers do
`void pending.then(unlisten)` with no `.catch`. Fix: `.catch` + surface via
the logger (the SB-B-04 lane already fixed this file's load-progress path —
apply the same to the other 12). ~an hour + the connector-tauri test
extension.

### 14. SB-A-12 (P3) — app-log writer spawn failure turns the log into a no-op
Scope: `bridge/logging.rs:236-252` — thread spawn `.ok()` swallows; on
failure every subsequent line is silently discarded. Fix: fall back to a
direct-file write (or stderr once) on spawn failure. Rust-only.

### 15. SB-A-13/SB-E-09 (P3) — sessions row dataFoundBytes carries bytes_scanned
Scope: `commands/sessions.rs:217` — the light ABI row has no found-bytes
axis; the bridge substitutes scanned bytes into a field named
dataFoundBytes (latent — no renderer consumes it yet). Fix: null the field
(the honest absent axis). One-liner + contract type note. Pairs with
SB-A-14 (same-second session id collisions, sessions.rs:197 — make the id
`sess-<unix>-<seq>`).

### 16. SB-E-10 (P3) + SB-E-11 (P3) — pre-telemetry 512 fallbacks + ETA-as-elapsed
Scope: scan-progress-page.tsx:144 `sectorSize ?? 512`,
create-image-dialog.tsx:131 same (render '—' when unknown instead of the
4Kn-8× wrong fallback); scan-progress-page.tsx:283-287 stop-dialog "Time"
falls back to the ETA when startedAt hasn't loaded — fall back to '—'.

### 17. SB-E-13 (P3) — recoverability ring population mismatch
Scope: `recoverability-card.tsx:24` counts all-files matched/total while
the engine verdict (`cr_grading.cpp:600-602`) uses the og's 20-format
gauge population — two contradictory "recoverability" numbers. Product
decision needed (which population is truth for the card), then align.

### 18. SB-A-15 (P3*) — consent re-hydration re-render churn
Scope: `queries/scan.ts` consent bridge — `pendingConsent` re-fires
`setRepair` with fresh object identity on every 500 ms poll while a prompt
pends. Fix: deep-compare or key the set on content. Needs a runtime
perceptibility check first (the audit's needs-runtime list).

### 19. The systemic mock-divergence lane (P1-class)
Three confirmed mock-green/real-red defects this session (SB-B-01/03/04)
escaped CI because the entire browser-mode test lane runs the mock, which
is more permissive than the real bridge. Fix (structural): a
contract-conformance test double — the mock should REFUSE where the bridge
refuses (saveSession-while-busy, unlock-unconditionally, event-name rules)
— or a `tauri dev` smoke lane on a Windows runner exercising one scan
lifecycle against the REAL bridge. Until then every "green wire" is only
as trustworthy as the mock's fidelity.

### 20. Windows-only runtime captures (this sandbox cannot produce them)
- SB-B-02 live proof: the 30-second X-click check (build once, click the
  title-bar X with and without a scan running; the static gates now pin the
  permission + the race, and ui-e2e runs the built app on the Windows
  runner — the manual click-through remains the final capture).
- SB-A-02's growth rate: N scans in one app run → handle/memory growth
  curve on real Windows (the registry-retention is static-certain; only
  exhaustion timing needs measurement).
- The Phase-0 agents C (runtime profiling) and D (UI interaction) sheets:
  CPU/memory/thread/handle/frame-time captures at 1 s intervals over a
  seeded full exercise; every button in every state. This sandbox has no
  Windows desktop — the procedures are in the session prompt and the
  audit sheets' needs-runtime lists.

### 21. progress_display_remap dead code (P3)
Scope: `engine/src/core/query/cr_query.cpp:579-619` — the og's non-linear
display-remap curve is ported with ZERO callers. It is the one place a
"displayed ≠ actual" percent could enter the product. Delete it or gate it
behind an explicit never-wired note (the no-fake-progress rule).
