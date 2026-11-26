# DEFECT-LEDGER — Small-Bug Hunting & Stability Polish session (2026-11-26)

Consolidated from the three Phase-0 audit sheets (`SMALL-BUGS-SOURCE.md` Agent A,
`WIRING-AUDIT-SMALL-BUGS.md` Agent B (session-scoped: the standing `WIRING-AUDIT.md` from a prior audit is preserved untouched), `NUMERIC-DEFECTS.md` Agent E) — 64 Rust/TS/engine files
read by Agent A, 86 chains traced by Agent B, ~115 numeric sites recomputed by
Agent E. Every row carries its evidence in the source sheet. This ledger adds the
session-level classification: impact class, severity, verification status, and the
disposition plan (fix-in-session with Linux-runnable proof / Windows-runner proof
via CI lane / scoped OPEN-ITEM).

**Verification status legend** — `verified-static`: the defect is fully visible in
code and re-checked by the session owner (not just the agent); `agent-confirmed`:
agent-verified with file:line, owner spot-checked the site; `needs-runtime`: the
static window exists but the runtime magnitude/trigger needs a Windows capture.

## Master table

| ID | One-line | Class | Sev | Status | Disposition |
|---|---|---|---|---|---|
| SB-B-02 | Close button dead: `core:window:allow-destroy` missing while exit-guard + Tauri wrapper auto-destroy call `destroy()`; plus guard's `preventDefault` races the wrapper's post-handler check (`void onRequested(event)` not awaited) | broken button (close) + stuck window | **P0** | verified-static (owner re-read capability file + guard; wrapper semantics re-checked against installed @tauri-apps/api source) | **FIX-1** |
| SB-B-01 | "Save and exit" can never succeed: bridge `session_save` refuses while busy though the engine supports mid-run saves (`has_ctrl` is the only gate); mock allows it (mock-divergence) | stuck dialog / broken button | P1 | verified-static (owner re-read sessions.rs + cr_ui.cpp:2987) | **FIX-2** |
| SB-E-01/02 | FAT + exFAT quick pass: per-directory `(done,done)` tick → stage permille 1000‰ after the FIRST directory; gauge "Quick 100%"/overall 75% stuck for the whole scan (honest value at that moment: ~2‰) | wrong number (progress) | P1 (high) | agent-confirmed + owner re-read tick sites | **FIX-3** (engine) |
| SB-E-03 | ext4: zero ticks during live walk (0% stuck), then terminal `(groups,groups)` tick BEFORE the deleted-inode sweep (100% stuck; sweep's honest 500‰ clamped up) | wrong number (progress) | P1 | agent-confirmed | **FIX-3** (engine) |
| SB-E-04 | btrfs: `progress(4,4)` per SUBVOLUME → 1000‰ after subvol 1 of N | wrong number (progress) | P2 | agent-confirmed | **FIX-3** (engine) |
| SB-E-05 | HFS+: hard-coded 1024-node denominator (phantom total; ≤29% cap or premature 100% at node 1025) | wrong number (progress) | P2 | agent-confirmed | **FIX-3** (engine) |
| SB-A-01 | `metas_snapshot` deep-clones the entire `Vec<NodeMeta>` (2 Strings/row) on EVERY results page/tree/count/detail query + every progress-bucket invalidation — O(tree) alloc/copy per 200-row page | memory/CPU spike | P1 | agent-confirmed (state.rs:826-842 + 4 consumers) | **FIX-4** (Arc swap) |
| SB-A-02 | `scan_close` has no TS caller — every scan/session_load/resume leaks engine handle + open device + metadata cache + channel/thread for the whole app run | memory/handle leak | P1 | verified-static (owner grepped: no `closeScan` anywhere in ui/src) | **FIX-5** (wire closeScan) |
| SB-A-08 | stop/pause/resume/cancel mutations have no `onError` (no global MutationCache handler either) — a refused stop is a silent no-op: dialog closes, scan keeps running, zero feedback | swallowed error / broken stop button | P2 | verified-static (owner re-read queries/scan.ts:150-175 + providers.tsx) | **FIX-6** |
| SB-B-03 | Unlock dialog's FVEK field is a dead control — `fvek` state never read after input; only passwords sent; the real key-open path (`bitlockerFvek` → `cr_ui_open_volume_with_key`) has zero UI callers | dead wire / broken control | P1 | agent-confirmed | **OPEN-ITEM** (feature-level wiring; precise scope in docs/OPEN-ITEMS.md) |
| SB-B-04 | `session-load-progress:<raw Windows path>` event name violates the Tauri event-name charset → listen rejects → W1b load spinner dead on every real install | dead wire (event) | P2 | agent-confirmed (events.rs:112 + tauri-connector.ts:455) | **FIX-7** (id-keyed event name) |
| SB-A-03 | recover-all mid-scan pages a shifting sorted window across snapshot refresh generations → silent skip/duplicate of files; enqueue uses pre-loop `snap_ptr` | wrong result set | P1* | needs-runtime (static window real; arena-order stability mitigates the wrong-file half) | **OPEN-ITEM** (needs Windows harness with live scan; scope in OPEN-ITEMS) |
| SB-A-04 | lost-candidate cap (4096): event pairs NEW lba with OLD last row's sectors/fs/score; `count` saturates ("count-exact" contract broken) | wrong number (rows) | P2 | agent-confirmed (events.rs:412-431) | **OPEN-ITEM** |
| SB-A-05/06/07 | scan_start / recovery_start / session_resume error paths leak opened device + raw pipe box + created handles (retry loops compound) | resource leak on fallible path | P2 | agent-confirmed | **OPEN-ITEM** (RAII guards; engine/bridge needs Windows build to prove; scope precise) |
| SB-A-09 | Icon (tile) view hard-caps `rows.slice(0, 120)`, no virtualization, no loadMore — results past tile 120 unreachable | broken view mode | P2 | agent-confirmed (virtual-file-list.tsx:428) | **OPEN-ITEM** (virtualize; UI test harness exists) |
| SB-A-10 | Imaging same-disk overlap gate silently skipped when `enumerate_volumes()` fails (`.unwrap_or_default()`) — backstop is a corrupt image | swallowed error disabling safety gate | P2 | agent-confirmed (imaging.rs:225) | **OPEN-ITEM** |
| SB-E-06 | Recovery live counters add non-disjoint engine stats (`done+salvaged`, `errors+hash_mismatch`) — "10/10 +2 failed +1 skipped" (13 outcomes for 10 files; can read 12/10) | wrong number (counters) | P2 | agent-confirmed with engine-side verification (cr_safewriter.cpp) | **OPEN-ITEM** |
| SB-E-07 | Recovery "size to recover" = count × hard-coded 2,400,000 B average posing as measured; gates the out-of-space refusal (4,000× under for big files; spurious blocks for small) | wrong number (size) + wrong gate | P2 | agent-confirmed (recovery-page.tsx:80-95 → system.rs:230) | **OPEN-ITEM** |
| SB-E-08 | Scan ETA/throughput = current-sub-pass bytes ÷ whole-run elapsed (mismatched windows; sub-pass restart → ETA balloons to 1e7 s; oscillating "Remaining") | wrong number (ETA) | P2 | agent-confirmed (events.rs:540-554 vs cr_scan.cpp:554-565) | **OPEN-ITEM** (correct Δbytes/Δt pattern exists at events.rs:622-632) |
| SB-E-12 | Lost-partition Size column = `sectorCount × 512` hard-coded → 8× under on 4Kn disks (3.81 GiB shown as 488 MB) | wrong number (size) | P2 | agent-confirmed (lost-partition-page.tsx:216) | **OPEN-ITEM** |
| SB-A-11 | All 13 connector `on*` disposers `void pending.then(unlisten)` without `.catch` — listen rejection = unhandled rejection + silent no-op subscription | swallowed error | P3 | agent-confirmed | **OPEN-ITEM** (fold into SB-B-04 fix lane if budget) |
| SB-A-12 | App-log writer thread spawn failure swallowed by `.ok()` — the product log silently becomes a no-op | swallowed error | P3 | agent-confirmed (logging.rs:236-252) | **OPEN-ITEM** |
| SB-A-13 / SB-E-09 | Sessions rows fill `dataFoundBytes` with `bytes_scanned` (wrong axis; latent — no renderer yet, contract-level wrongness) | mislabeled field | P3 | agent-confirmed (sessions.rs:217) | **OPEN-ITEM** |
| SB-A-14 | Session list ids `sess-<unix-seconds>` collide for same-second saves (React key identity) | id collision | P3 | agent-confirmed (sessions.rs:197) | **OPEN-ITEM** |
| SB-A-15 | Consent re-hydration re-sets `setRepair` with fresh object identity every 500 ms poll → modal re-render churn during a pend | render churn | P3 | needs-runtime | **OPEN-ITEM** |
| SB-B-05 | `image_target_list` registered, zero callers; manifest text misstates reality | dead command (info) | P2-info | agent-confirmed | **OPEN-ITEM** (doc truth) |
| SB-B-06 | `repair` domain (4 ops) is a typed refusal — documented G-1 gap, honestly rendered | documented gap | P2 | confirmed (documented) | no action (truthful) |
| SB-B-07 | Dead route `/tools` (no link targets it) | dead route | P3 | confirmed | **OPEN-ITEM** (one-line removal candidate) |
| SB-E-10 | `sectorSize ?? 512` fallbacks pre-telemetry (scan page, image dialog) — the exact class P-DEEP-01 bans | wrong number (transient) | P3 | agent-confirmed | **OPEN-ITEM** |
| SB-E-11 | Stop-confirm "Time" falls back to ETA (remaining) when state not loaded — 11:00 shown for 05:00 elapsed | wrong number (transient) | P3 | agent-confirmed | **OPEN-ITEM** |
| SB-E-13 | Recoverability ring counts all-files population while engine verdict score uses the 20-format gauge population — contradictory "recoverability" numbers | population mismatch | P3 | agent-confirmed (semantics) | **OPEN-ITEM** |
| — (obs) | `progress_display_remap` (og's non-linear display curve) is dead code with zero callers — if ever wired it violates no-fake-progress | dead code | P3 | agent-confirmed (grep) | **OPEN-ITEM** (delete-or-gate recommendation) |

## Fix batches for THIS session (in user-impact order)

- **FIX-1 (SB-B-02, P0)** — close button + window destroy: add `core:window:allow-destroy`
  to `capabilities/default.json`; fix the exit-guard async race so `preventDefault()`
  lands before the wrapper's post-handler check; gate test: a repo test that walks every
  window-API usage in `ui/src` and asserts the matching capability grant (fails before,
  passes after). Wrapper semantics verified against the installed `@tauri-apps/api`
  source (see worklog/FIXES/SB-B-02/).
- **FIX-2 (SB-B-01, P1)** — save-and-exit: remove the bridge's busy-refusal for live
  scans (the engine's `scan_save_session_impl` gates on `has_ctrl` only and explicitly
  designs for mid-run saves — N-060 adapter + `Snapshot::capture(tree, running)`);
  keep the honest refusals (loaded-session rows; has_ctrl=false). Proof: mock-vs-real
  divergence closes; bridge behavior documented; Windows CI lanes green on push.
- **FIX-3 (SB-E-01..05, P1/P2)** — engine progress honesty: remove/repair the
  loop-scoped terminal ticks (FAT/exFAT per-directory `(done,done)`, btrfs per-subvol
  `(4,4)`, ext4 pre-sweep `(groups,groups)`, HFS+ phantom 1024 denominator) so no
  stage permille reaches 1000‰ before the true pass end; per-pass regression tests in
  the engine suite asserting permille stays < 1000 mid-pass and hits 1000 only at the
  terminal tick.
- **FIX-4 (SB-A-01, P1)** — `metas_snapshot` clone → `Arc` (generation-stable under the
  snapshot mutex; consumers borrow the Arc). Proof: cargo check/test + memory
  arithmetic documented before/after (alloc count per page fetch: O(n) strings → 1 Arc).
- **FIX-5 (SB-A-02, P1)** — wire `closeScan` into the connector contract + call it when
  a scan/session entry leaves scope (results-route exit / new-scan replace), keeping
  the busy/job guard semantics. Proof: contract validator + RTL coverage of the exit
  path + registration count parity.
- **FIX-6 (SB-A-08, P2)** — onError surfacing for stop/pause/resume/cancel (hook-level
  toast following the codebase's established onError pattern). Proof: RTL test driving
  a failing cancel → toast + state honest (fails before, passes after).
- **FIX-7 (SB-B-04, P2)** — session-load-progress event keyed by a bridge-minted load
  id (charset-legal) instead of the raw Windows path; both sides + the TS listener.
  Proof: contract/event mapping updated + validator + unit tests on name shape.

Every item NOT fixed this session lands in `docs/OPEN-ITEMS.md` with precise scope (the
audit sheets already carry file:line + fix direction for each; OPEN-ITEMS references
them). Windows-only runtime proof (freezes/captures, CI sweep, RC validation) is
explicitly out of this sandbox's reach and listed there with the exact capture
procedure, ready to run.

## Environment truth for this ledger

- All static evidence above is code-level (file:line cited in each source sheet; the
  P0/P1 entries re-read by the session owner).
- Linux-runnable proof lanes in this sandbox: `contracts/validate.mjs`, engine suite
  (CMake, historically 1682 green on Linux), UI vitest/RTL suite (pnpm), `cargo check`
  for the bridge where the crate compiles on Linux.
- Windows-runner proof lanes: GitHub Actions on this repo's pushes (engine-ci,
  ui-ci, bridge-integration, ui-e2e — windows runners). The session polls their real
  status via the GitHub API after pushing; whatever the state at handoff is reported
  honestly (never fabricated).
- NOT possible in this sandbox: live Windows UI captures, perf-counter profiling,
  handle/memory growth measurement on real Windows, clean-Windows RC validation.
  Scoped in `docs/OPEN-ITEMS.md` with the exact procedure.
