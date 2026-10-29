# MAJOR M-136 — recovery-time reads bypass the retry/cache pipeline the scan built: one transient read error becomes a permanent zero-filled span

**Severity:** MAJOR
**Module:** writer / recovery + orchestrator (file-reconstruction focus battery)
**Parameters affected:** #35 Error recovery; #47/#48 Throughput/latency (the recovery pass's read path); #60 Bad-sector handling
**Corpus references:** `architecture/04-recovery-workflow.md:147-150` (the og's export reads flow through the engine/scan-session stream abstraction — IFileProperties/IInWindowStream-mediated); `re-workspace/artifacts/SearchDLL/ntfs-engine--deep-dive.md:146` (the CStorageDeviceWithCache reader the engines own); `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:873629-873671` (`FUN_140540670`: the content-object `props+0x30` → `vt+0x78` GetStream dispatch — the writer reads through the stream, not the raw device)
**Core references:** `src/core/orchestrator/cr_scan.cpp:155-194` (the scan's read chain: SliceShim → RetryingDevice(3 attempts, Skip mode) → CachedReader — the M-058 single construction point); `src/app/rrecov.cpp:2080-2082` at the audited pin / `:2326` at the merged HEAD (the CLI hands `src.device` **raw** to the SafeWriter; re-pinned at the merged HEAD: `rrecov.cpp:2326` after the window's A-013/M-116 CLI landings); `src/ui/cr_ui.cpp:4027` (the ABI does the same); `src/core/recovery/cr_safewriter.cpp:498-503` (the zero-fill-on-failure that a retry would have survived) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The scan builds a layered read pipeline (slice shim, retrying device with 3 attempts in Skip mode, cached reader) and runs every scan read through it; the recovery pass — the one place where bytes become the user's recovered files — hands the writer the bare device. A single transient read failure during recovery (a hiccup the scan would have retried past) is treated as permanent: the span is zero-filled, the item commits Salvaged, real data that was readable a moment later is never fetched. The og's export reads go through the engine/scan-session stream abstraction, i.e. the same cached/retrying substrate the scan used.

## What the reference does

The og's export loop resolves each item's content through the node's content object (`props+0x30`, `vt+0x78` `GetStream` — `:873629-873671`, verified first-hand this session) and reads via the stream's reader vtables (`vt+0x10` read / `vt+0x28` sector size in the runlist path `:875672-875699`); architecture/04 §7.5 states the mediation explicitly ("source reads come from the engine/scan-session abstraction … which for damaged volumes read through EPMNtDrv raw sector IO"). The engines' reader is the `CStorageDeviceWithCache` family (the same cached-reader class the deep-scan sweep installs its volume bitmap into — see the s42 scan-mode worksheet's `:919095-919125`); whether the og's export path additionally retries is UNDECODED (no retry loop visible in the runlist path's failure branch — it truncates on first failure), so the og's *retry* posture is not claimed here. What is claimed and verified: recovery reads are **stream-mediated**, cache-adjacent, and separate from any raw-device handle the destination uses.

## What our core does

- The scan's `build_read_pipe` (`cr_scan.cpp:155-194`) constructs SliceShim → RetryingDevice (3 attempts, Skip mode) → CachedReader and scans through it (M-058's pinned contract).
- The recovery entry points bypass it entirely: `rrecov.cpp:2326` (re-pinned; was :2080-2082 at the audited pin) constructs `recovery::SafeWriter writer(io, src.device.get(), wcfg)` — the **raw** `IStorageDevice*`; `cr_ui.cpp:4027` does the same for the ABI.
- The writer's failure policy then converts any single read miss into a permanent zero span (`cr_safewriter.cpp:498-503`), with no retry, no cache warmth, and no read-through to the slice shim's basis mapping the scan enjoyed.

## Gap

Stated falsifiably: a sector that reads successfully on attempt 2 during the scan (RetryingDevice) reads as a zero span during recovery of the very same file (no retry layer); the scan and the export disagree about the readability of the same byte range within one session on one device.

## Impact

Recovered-output integrity under transient errors (cable/USB flakiness, busy virtual disk, SMR pause): the exact conditions recovery products face most. The core's Salvaged/zero-fill accounting hides the loss (M-135), and the bad-sector policy diverges from the scan's own behavior. MAJOR: robustness-of-recovery divergence, happy path unaffected, interaction with M-135 makes it user-visible as silently-zeroed data.

## Reproduction / verification

- **Core evidence (first-hand):** the pipe construction at `cr_scan.cpp:155-194`; the raw-device handoff at `rrecov.cpp:2326` + `cr_ui.cpp:4027`; the zero-fill at `cr_safewriter.cpp:498-503`.
- **Corpus evidence:** the stream-mediated export reads (`:873629-873671` + the reader-vtable calls at `:875672-875699`); arch/04 §7.5's statement; the CStorageDeviceWithCache class family. Worksheet: `diff-to-fix/evidence/decompiled-quotes/s42-reconstruction-export-chain.md`.
- **Execution status:** the retry-count divergence is constructively proven (the writer demonstrably has no retry call — there is no retry API on the raw device path it holds); an end-to-end fault-injection run (a flaky mock device failing first-read-only) is prescribed for the fix lane's acceptance.

## Fix plan (for the engineering team)

1. Thread the scan's existing read pipe (or a recovery-grade equivalent) into the writer: `SafeWriter`'s constructor takes the `RetryingDevice`/`CachedReader` chain instead of the bare device (the chain already exists at `cr_scan.cpp:155-194` — reuse `build_read_pipe`'s output, or accept an `IStorageDevice*` that already wraps it).
2. Keep the destination side raw (unchanged — only the source reads change).
3. Add a fault-injection mock (first-attempt-fails device) to the writer tests; assert the span recovers on retry and the item commits `Done`, not `Salvaged`.
4. Revisit M-135's policy with retries in place: post-retry hard failures are the ones that should truncate-and-keep (the og's shape) rather than zero-fill.
5. Acceptance: a mock device failing every first read recovers byte-identical files; the zero-fill path only fires after all 3 attempts fail.

## Tests to add

- `Writer.ReadsRetryOnTransientFailure` (fails today: no retry layer in the writer's path).
- `Writer.CachedReaderSharedWithScan` (the same reader instance serves both phases — the construction seam).

## Related findings

- **M-135** — the failure policy that consumes the misses this finding eliminates.
- **S-059** — the zero-fill tail policy (composes with retries).
- **M-058** (retired) — the read-pipe construction contract this fix reuses.

## Open questions

Whether the og's export path retries internally (no retry loop observed in the runlist failure branch; the cache-mediation is verified, the retry posture is not) — one focused read of `CStorageDeviceWithCache`'s read path in the corpus decompiles would settle it; tracked with OQ-S42-3's worksheet as a secondary probe.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — the S72 MAJOR-first lane's first landing)
**Verified by:** `Writer.RecoveryEntryPointsConstructThroughBuildReadPipe` (RED pre-fix at the stash-revert state: both grep pins npos — the CLI and ABI entry points carried no pipe construction; GREEN post-fix), `Writer.ReadsRetryOnTransientFailure` (the one-shot transient-fault device recovers byte-identical through the pipe — Done, zero Salvaged), `Writer.BareDeviceZeroFillsWhatThePipeRetries` (the negative control pinning the removed behavior: bare device + the same transient fault → Salvaged zero-span, the finding's falsifiable scan-vs-export disagreement), `Writer.RecoveryCacheServesSharedExtents` (two items over one extent read the device once — the CachedReader is in the writer's chain); full suite 1507/1507.
**Resolution notes:** CONFIRMED-DIVERGENCE. The corpus re-verified first-hand this session: architecture/04-recovery-workflow.md §7.5 ("source reads come from the engine/scan-session abstraction") and the GetStream dispatch at DRWUI:873629-873671 — export reads are stream-mediated, never raw-handle. The core fix threads the scan's own construction seam into both entry points: the CLI recover verb and the ABI recovery_open now build `orchestrator::ReadPipe` via `build_read_pipe(src, 8 MiB, 256 KiB)` (the M-058 single construction point, identical constants) and hand the writer `rp.pipe` (SliceShim identity → RetryingDevice ×3 Skip-mode → CachedReader); the chain is owned by the entry context (a local before the writer in the CLI; a `read_pipe` member declared before `writer` in CrRecovery) and provably outlives it. Destination-side IO is untouched (only source reads changed). The finding's open question (whether the og's export path retries internally) stays answered-as-undecoded — the fix does not claim og retry parity, only stream-mediation + the core's own scan-grade retry posture, which is the verified claim. The LDM basis devices keep raw member reads in BOTH the scan (scan_candidate over the bare LdmVolumeDevice, cr_scan.cpp:1875) and the export — no scan-vs-export disagreement exists on that surface (consistency preserved; the og's candidate-side CStorageDeviceWithCache is the scan's own posture, out of this finding's export-path scope). No safety hardening was near this site; the F12/N-013 destination arming is unchanged.
