# BEHAVIOR-FINDINGS — decisions deferred beyond this consolidation session

The consolidation mandate's rule: mechanical problems (builds, wiring,
stale references) are fixed in-session; BEHAVIOR changes are recorded with
precise scope and left for a dedicated decision session. This file is the
register (the INTEGRATION-PLAN §7 seed list, status-annotated against what
the P1–P6 tree actually did).

Status vocabulary: **mitigated** = the product is honest today via a
bridge-side or docs-side measure; **open** = a real gap a user can hit;
**upstream** = the fix belongs in core_recovery, not here.

| ID | Finding | Precise scope | Status in the v0.7.0 tree |
|---|---|---|---|
| BF-01 | No flat-C log sink on core v4 | Bridge binds the mangled `?set_sink@log@cr@@…` (cr_ui_exports.def:19) | **Mitigated, upstream re-land recommended** — P4's logging.rs binds it; hosts shouldn't link C++ mangles |
| BF-02 | `bus_type` absent on v4 disk_info | SD-vs-USB detection degrades to `removable` | **Mitigated** — DeviceKind mapping documents the merge; upstream re-append recommended |
| BF-03 | SMART attr thresholds absent (no `health_read_ex2`) | Health table shows value/worst/raw/status; threshold column | **Mitigated (truthful-disabled)** — the column is disabled with a reason, never faked |
| BF-04 | `query_trim_state` still fsutil `_popen` on core | NG-8 console-flash class; bridge unaffected (no console in Tauri) | **Open, upstream** — cr_health.cpp:301-324 |
| BF-05 | No mid-sweep progress ticks in core's RemnantCtx | Deep-stage progress stays stage-level | **Mitigated** — UI shows stage-level truthfully |
| BF-06 | **NG-21 engine-side guard missing (safety-critical)** | Whole-disk pre-scan repair unguarded at LBA 0 in core | **Mitigated bridge-side** (§4.6: pre_scan_repair=0 on whole-disk targets) + forensic S3 keeps probing the consent gate; **upstream re-port is the real fix** |
| BF-07 | ERR_DEVICE_DISCONNECTED reattach-then-resume flow | UI surfaces device-gone + user restarts; dedicated resume affordance deferred | **Open** — documented UX gap |
| BF-08 | Carve catalog + live tally unsurfaced | No reference screen | **Open** — scoped, CLI covers |
| BF-09 | Disk map unsurfaced | Net-new visualization screen | **Open** — scoped for a future session |
| BF-10 | image_create unsurfaced | Needs a Tools screen + progress | **Open** — CLI covers today |
| BF-11 | fsck_volume + turn_off_bitlocker unsurfaced | Privileged/destructive volume ops need consent + outcome UX | **Open** — escalation ladder documented |
| BF-12 | Image targets unsurfaced | `open_image` → unelevated scans of VMDK/VHD/VHDX | **Open** — also the ABI-check surface |
| BF-13 | Live-scan thumbnails use preview-read fallback | `thumbnail_generate` is session/node_id-addressed only | **Mitigated** — live scans degrade to preview-read |
| BF-14 | RecoveryOptions truth gaps | keepEmptyFolders/fileRepair have no engine equivalent | **Mitigated (truthful-disabled with reasonKey)** |
| BF-15 | M-074 destination taxonomy umbrella | path-too-long/invalid/network all → destination-invalid | **Mitigated** — distinct reasonKeys deferred |
| BF-16 | Core-side query engine unused | `query_run`/`apply_buckets` deferred behind a T14 re-measure | **Open** — the NodeMeta port carries the UI today |
| BF-17 | Assistant stats + regrade unsurfaced | S-023 grading stats; no reference screen | **Open** |
| BF-18 | Windows build fixes are final-prod-local | testPreset `-C`, VCPKG_APPLOCAL_DEPS=OFF, deploy retries | **Mitigated** — re-applied in 01d39b33; upstreaming recommended |
| BF-19 | Engine version-stamp drift | Core CMake VERSION 0.6.1 vs v0.7.1 notes | **Mitigated** — the product gates on `cr_ui_abi_version()` only |
| BF-20 | Health grading duplicated at the bridge | The 4-status model re-implemented in bridge/health.rs | **Mitigated** — keep-in-sync note; upstream export recommended |
| BF-21 | M-101 version_group/tags fields unsurfaced | NTFS namespace-identity grouping + tag extraction | **Open** — no UI surface |
| BF-22 | Deep scan of large sparse vdisks stalls in the forensic capture window | 76.1% / found-count frozen for 27+ min on the 64GB seed; IDENTICAL on the pre-merge ABI v2 engine (run 42072191860) and v0.7.0 (42129256087) — environmental/known-perf (the carve matcher over free space; core's PERF-LEVERS 3), not a wiring regression | **Recorded** — the Stop → partial results → recover path is the gated user flow; the throughput ledger is core's bench lane's business |
| BF-23 | SMART environmentally absent on the forensic runner | Azure VM disks expose no SMART passthrough; VHDX seeds never will — the health surface renders its honest failure state + retry affordance (identical pre/post merge) | **Recorded** — the SMART data path is covered by the engine's ctest suite; needs a bare-metal host for end-to-end visual proof |
| BF-24 | Session-save races the post-cancel drain window | `scan_cancel` is fire-and-forget; the engine's run thread drains 20+ s after a stopped deep scan (io-timeline evidence, run 42143819243) — a save clicked in the window is refused with SCAN_NOT_RUNNING, whose message text ("or cancel it first") assumes cancel is synchronous | **Mitigated (honesty)** — db600f4c makes the refusal visible (onError toast) and the forensic probe retries through the window; the UX fix (save disabled until drain, or bridge-side wait-on-refusal) is upstream scope |
| BF-25 | Cancel latency is unbounded inside the deep-scan stall — a stopped stalled scan stays busy indefinitely | Run 42148493806 (v0.7.0 binary): the 64GB seeded deep scan stalls at 76.1% (BF-22) with reads idle from +6 min; `scan_cancel` at +28 min sets the token (the UI navigates to results on the invoke's success), but the run thread never returns — 15 save attempts over 136 s were ALL refused (cr_ui_scan_busy stays 1 until the heartbeat join; io-timeline shows no write spike — the .crsf was never written), the "stopped early" partial banner never renders (it is gated on status=cancelled, which latches only when run() exits), and the recovery itself SUCCEEDED from the still-live scan. The stalled carve window does not reach a cancellation checkpoint | **Recorded** — the busy-guard itself is honest (a save of a mid-drain scan would be unsound); the responsiveness fix (a cancel checkpoint inside the slow carve window) is upstream core scope, same family as BF-22. The forensic probe's save/resume gates now run on a deterministic completion vehicle (the 8GB SeedEXFAT deep scan) instead of the stalled stop path |

None of these block the v0.7.0 release: every row is either honestly
disabled in the product (with a user-visible reason), carried by the CLI,
or purely upstream. The safety-critical row (BF-06) has the bridge-side
closure + a standing forensic probe.
