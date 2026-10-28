# SEVERE S-054 — N-013 regression: the multi-extent destination arming was clobbered back to first-extent-only at both production call sites (unrelated commit 1ebd1bcc); the F12 overlap refusal is silently defeated for spanned/RAID and junction destinations while the published contract claims all-spans coverage

**Severity:** SEVERE
**Module:** writer, win/transport, ui
**Parameters affected:** F62/P62 (destination overlap refusal — hard refusal, same semantics), B8 (multi-volume / spanned volumes), R43/R57 (contract truthfulness — the header documents the requirement the code no longer meets), Q65 (test coverage — no test pins the production wiring; the suite is green over the regression)
**Corpus references:** `architecture/04-recovery-workflow.md` §7.5 (destination validation: the og ALWAYS opens `\\.\X:` for drive-letter destinations and validates with `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` + lock/dismount); `re-workspace/reversed/ghidra-partition/RecoveryModule_decompiled.c:60381-60444` (the og's `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS` consumption — ALL extents iterated for the overlap/validation logic)
**Core references:** audited `c7fa527c` — `src/app/rrecov.cpp:2084-2088` (the recover path arms `writer.set_destination(dstart, dlen)` from the FIRST extent only via `win::volume_extent`); `src/ui/cr_ui.cpp:4045-4051` (the ABI path: same first-extent-only `set_destination`, drive-letter-prefix paths only); `include/cr/win/cr_transport.h:124-126` (the published contract: "N-013: reports the FIRST extent only - kept for the source-disk lookup call sites; **destination arming must use volume_extents() (all spans)**") and `:129-131` (the `volume_extents` declaration — zero production consumers, grep-verified); `src/core/recovery/cr_safewriter.cpp:29-34` (`set_destination_spans` — the machinery, unconsumed); `src/win/cr_transport.cpp:1037` (`destination_volume_letter` — the junction/relative-path resolver from the N-013 landing, zero production consumers); git: `3b5397a1` (the N-013 fix landing — rrecov.cpp 52 lines, cr_ui.cpp 37 lines, transport +72, safewriter +34) and `1ebd1bcc` (the UNRELATED M-036 NVMe fix that clobbered both call sites back to the pre-N-013 shape; the removal diff preserves the full spans loop in git history)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The N-013 fix (all-extents destination arming + any-resolvable-path resolution, landed complete at `3b5397a1` with tests and a published header contract) was silently reverted at both production call sites by the unrelated M-036 NVMe commit `1ebd1bcc`, which was evidently authored on a pre-N-013 tree. At the audited HEAD the CLI recover path and the `cr_ui` ABI path both arm the SafeWriter with the destination volume's FIRST extent only, and only for direct drive-letter paths — the `volume_extents()` + `set_destination_spans()` + `destination_volume_letter()` machinery still exists, still carries its tests, and has ZERO production consumers. The SafeWriter's overlap refusal (`cr_safewriter.cpp:135-152`, which itself correctly iterates every span) is therefore armed with an incomplete span set: a file whose extents intersect the destination's second-or-later span is NOT refused and is written — and if that later span lives on the source disk, the write lands on the source disk being recovered.

## What the reference does

The og's destination validation (architecture/04 §7.5, decompiled at RecoveryModule_decompiled.c:60381-60444) always opens `\\.\X:` for drive-letter destinations, queries `IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS`, and iterates the FULL extents result (multi-extent-aware: a spanned/RAID volume's every disk placement participates in the validation/overlap logic), then locks (`FSCTL_LOCK_VOLUME`) and dismounts (`FSCTL_DISMOUNT_VOLUME`) the destination. The overlap guarantee the core's F12 models — "the destination volume extent must not intersect the source" — is enforced by the og over every extent of the destination volume, not its first.

## What our core does

```cpp
// src/app/rrecov.cpp:2029-2034 (audited c7fa527c)
if (to_dir.size() >= 2 && to_dir[1] == L':') {
    u64 dstart = 0, dlen = 0;
    u32 ddisk = 0;
    if (win::volume_extent(to_dir[0], &dstart, &dlen))   // FIRST extent only
        writer.set_destination(dstart, dlen);             // single-span arming
}

// src/ui/cr_ui.cpp:3863-3870 — the same first-extent-only, drive-letter-only shape
if (utf8_path[0] && utf8_path[1] == ':' && r->dev->disk_known &&
    std::strcmp(r->dev->transport, "image") != 0) {
    if (win::volume_extent(wchar_t(utf8_path[0]), &ddisk, &dstart, &dlen))
        r->writer->set_destination(dstart, dlen);
}
```

The refusal consumer is intact and multi-span-aware:

```cpp
// src/core/recovery/cr_safewriter.cpp:135-137
for (const auto& e : pr->extents) {
    for (const auto& span : dest_spans_) { /* N-013: every span */
        if (intersects(e.offset, e.length, span.first, span.second)) { ... ERR_DEST_OVERLAP ... }
```

but `dest_spans_` is only ever fed one span by the two production call sites. `grep -rn "volume_extents\|set_destination_spans" src/` (excluding the two provider files) returns nothing — the machinery is dead code at HEAD. The header contract (`cr_transport.h:124-126`) still instructs integrators that "destination arming must use volume_extents() (all spans)" — the code contradicts its own published contract.

**The git trail (the regression's proof, first-hand):**
- `3b5397a1` "fix(N-013): destination overlap armed for any resolvable path, ALL extents, junctions resolved to target volume" — landed the spans loop (`std::vector<win::VolExtent> ve; win::volume_extents(dletter, &ve); ... writer.set_destination_spans(spans);`), the `destination_volume_letter` resolution, and tests (`test_destio_win.cpp` +67, `test_cr_ui.cpp` +21, `test_report.cpp` +35).
- `1ebd1bcc` "fix(M-036): NVMe health full field surface" — an unrelated health fix whose diff REMOVES the spans loop and the junction resolution from both `rrecov.cpp` and `cr_ui.cpp` and restores the pre-N-013 single-extent/drive-letter-only shape (visible in `git show 1ebd1bcc -- src/app/rrecov.cpp`: the `-` side carries `volume_extents`/`set_destination_spans`, the `+` side carries `volume_extent`/`set_destination`).

## Gap

The core's destination-overlap refusal (P62/F12) is armed with the destination's first extent only, and only for direct drive-letter paths, at both production call sites — while (a) the og enforces the guarantee over every extent, (b) the core's own published header contract asserts the all-spans requirement, and (c) the FIXED-LEDGER records N-013 as resolved. Falsifiable claim: at `c7fa527c`, no production code path calls `volume_extents()`, `set_destination_spans()`, or `destination_volume_letter()`; both arming sites pass a single span.

## Impact

- **Silent data loss window (the F12 safety parameter):** recovering to a spanned/RAID (dynamic) destination volume whose second-or-later span resides on the source disk — the refused-overlap write is NOT refused; it proceeds onto the source disk, overwriting the very content being recovered.
- **Junction/relative destination paths:** the N-013 landing's any-resolvable-path arming is equally dead — a destination expressed as a junction or relative path (resolving to a volume on the source disk) arms nothing at all, so the overlap check silently no-ops.
- **Contract truth:** every host integrating per `cr_transport.h:124-126` or the `cr_ui` destination API inherits the incomplete guarantee with no error signal.
- **Process truth:** the suite is green (1117/1117 at this HEAD) over the regression because the surviving tests pin the helpers, not the wiring — the N-013 acceptance tests were clobbered with the wiring and the helper tests cannot fail when the wiring is absent.

## Reproduction / verification

Executed (git + grep, this session, first-hand):
1. `git show 3b5397a1 --stat` — the N-013 landing touches rrecov.cpp (52 lines), cr_ui.cpp (37), cr_transport.cpp (+72), cr_safewriter.cpp (34), plus three test files.
2. `git show 1ebd1bcc -- src/app/rrecov.cpp` — the M-036 diff removes the `volume_extents`/`set_destination_spans` block and restores `volume_extent`/`set_destination` (the `-`/`+` sides quoted in `evidence/decompiled-quotes/s12-destarming-regression.md`).
3. `grep -rn "volume_extents|set_destination_spans" src/ --include="*.cpp" | grep -v "cr_transport.cpp|cr_safewriter.cpp"` — zero hits (rc=1).
4. `sed -n '2084,2088p' src/app/rrecov.cpp` and `sed -n '4045,4051p' src/ui/cr_ui.cpp` — the first-extent-only calls, quoted above.
5. Full suite at the same tree: 1117/1117 green (`evidence/executions/s12-build-execution.md`) — the regression is invisible to the suite.

## Fix plan (for the engineering team)

1. Architectural: none — the machinery (`volume_extents`, `set_destination_spans`, `destination_volume_letter`) already exists and is correct; this is a re-landing of the two call sites plus a regression fence.
2. Files: `src/app/rrecov.cpp` (the recover path's destination-validation + arming block), `src/ui/cr_ui.cpp` (the ABI destination path). Restore the `3b5397a1` shape: resolve the destination volume letter via `destination_volume_letter` (any resolvable path, junctions included), fetch `volume_extents()`, print the per-span diagnostics, and arm `set_destination_spans(spans)`.
3. Algorithm/constant: none beyond the restoration; keep the image-source exemption and the disk-known gating exactly as `3b5397a1` had them.
4. Tests: the N-013 acceptance tests must be re-landed WITH a wiring pin — a test that fails if the production path stops calling the spans API (e.g. a fake `SafeWriter`/transport seam asserting multi-span arming end-to-end from the CLI/`cr_ui` entry, not just the helpers). Add a regression test naming this finding: `DestArming.AllSpansWired` (a 2-span fake volume arming both spans) and `DestArming.JunctionPathArmed`.
5. Contract/doc: no change needed — the header already states the requirement; after the fix the code matches it again. Add a line to `docs/ground-truth-contracts.md` only if the junction-resolution semantics need re-stating.
6. Acceptance: `grep` for production consumers of `set_destination_spans` returns the two call sites; the wiring-pin tests pass; a manual smoke on a spanned destination (or the fake-volume test) shows per-span refusal when a later span intersects the source.

## Tests to add

- `DestArming.AllSpansWired` — a fake 2-span destination volume: a file whose extents intersect ONLY the second span is refused (`ERR_DEST_OVERLAP`). Fails at HEAD, passes after.
- `DestArming.JunctionPathArmed` — destination expressed as a junction to a volume on the source disk: refused. Fails at HEAD, passes after.
- `DestArming.WiringPinned` — an end-to-end arming assertion through the `cr_ui` destination entry (fake transport), failing if the spans API stops being called (the fence that would have caught `1ebd1bcc`).

## Related findings

- N-013 (the original, retired-FIXED at `3b5397a1` — this finding re-opens its defect with the regression provenance)
- M-074/S-010-class destination-validation family (the lock/dismount halves)
- N-135 (the bookkeeping-truth batch class — the ledger row for N-013 should gain a re-opened note)
- N-154 (bookkeeping batch #2 — the post-rebase truth sweep)

## Open questions

None — the regression is git-proven; the only judgment call is the re-landing shape, which `3b5397a1` already defines.
## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-054 restore; the ledger row carries the final SHA)
**Verified by:** DestArming.ProductionWiringCallsSpansApi + DestArming.HeaderContractStillClaimsAllSpans (the wiring fence that would have caught 1ebd1bcc: the source-level pins fail the day either production path stops calling destination_volume_letter/volume_extents/set_destination_spans - the FAIL-BEFORE proven by mutation: stripping the wiring from the scratch source copy failed the fence; the restored tree is green) + the surviving pre-existing pins (Report.MultiExtentSpannedVolumeRefused portable-core writer behavior; DestArming.VolumeExtentsCarryAllSpans/NetworkPathNeverResolves/JunctionResolvesToTargetVolume/VolumeLetterResolvesAnyLocalPath on the Windows lane). Full suite 1094/1094 + cr_ui 61/61 (Linux release) at the restored tree.
**Resolution notes:** The 3b5397a1 shape is restored verbatim at both production call sites (re-verified against the git history first-hand: the rrecov block with the source-disk warning + the (extent(s)) diagnostic; the cr_ui block with the any-resolvable-path resolution and the image-source exemption preserved, the disk_known gate dropped exactly as N-013 had it). The helpers were intact and correct at HEAD (volume_extents, set_destination_spans, destination_volume_letter - zero production consumers was the only defect; the regression was pure wiring, so the fix is a pure wiring restore, no new machinery). The junction/relative-path arming is live again (the destination_volume_letter resolution replaces both drive-letter gates). Suite green; the Windows-lane live-path tests cover the resolution semantics the Linux lane cannot execute (the fence pins the wiring for both lanes). Provenance note: this re-landing is the S-054 finding's own fix plan step 2 executed exactly; the original N-013 ledger row keeps its landing (3b5397a1) with the re-open record in this finding.
