# MAJOR M-146 — probe_family's NTFS gate family is not the og analyser's: the core demands bps-range + spc-POW2 and skips the og's zero-byte sweeps and record/index shape gates — divergence in BOTH accept directions, and the core's comment misattributes the getters

**Severity:** MAJOR
**Module:** orchestrator (probe_family dispatch) / ntfs
**Parameters affected:** B1 (detection correctness — non-pow2-spc volumes never dispatch), B3 (false-positive dispatch — spoofed DBRs the og analyser declines), A25 (engine algorithm steps), Q68 (the in-code comment's citation accuracy)
**Corpus references:** `re-workspace/reversed/ghidra-mo/NTFSFileSystemAnalyser_decompiled.c` FUN_180004900 (:3568-3577 — the analyser's boot gates) and FUN_180005470 (:4292-4307 — the record/index shape gates feeding the record-0 probe); the getter identities FUN_180004830/FUN_180004870 (record/index sizes, +0x40/+0x44) confirmed at their definitions this session
**Core references:** audited `34d6cf10` — `src/core/orchestrator/cr_scan.cpp:792` (`bps >= 512 && bps <= 0x1000 && (bps % 512) == 0 && spc != 0 && (spc & (spc - 1)) == 0 && mft_lcn != 0`) and the comment at `:770-779` ("the boot gates (bps 512-multiple <= 0x1000, SPC pow2 nonzero; FUN_180005470 :4292-4307)")
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og analyser's NTFS boot gate family is: OEM, bps%512==0, three reserved-zero sweeps, spc != 0 (ANY value — no pow2 requirement), and — at the record-0 probe stage — record-size a 512-multiple ≤ 4096 and index-block a 4096-multiple (the +0x40/+0x44 getters, with the 0x400/0x1000 zero-defaults). The core's probe_family demands bps in [512,4096], spc a POWER OF TWO, and mft_lcn != 0, applies no zero sweeps and no record-shape gates — and its comment claims FUN_180005470's gates are "bps/SPC" gates, misattributing getters that actually read +0x40/+0x44. Volumes with non-pow2 spc (real formatter output) never dispatch in the core; spoofed DBRs the og declines do dispatch.

## What the reference does

The analyser's boot gates (FUN_180004900 :3568-3577, re-read first-hand):

```c
if (((CONCAT35(uStack_210,local_218._3_5_) == 0x202020205346544e)   /* "NTFS    " */
   && ((uStack_20d & 0x1ff) == 0))                                   /* bps % 512 == 0 */
&& (((short)(uStack_20d >> 0x18) == 0)                               /* +0x0F == 0 */
  && ((((char)(uStack_20d >> 0x10) != '\0' && ((char)local_208 == '\0'))  /* spc(+0x0D) != 0; +0x10 == 0 */
    && (bytes +0x11/+0x12/+0x13 == 0 && +0x14 == 0 && +0x16 == 0 && +0x17 == 0)))
&& (+0x20..0x23 == 0))) {
```

— spc != 0 as a raw nonzero test (no pow2 anywhere); the +0x20..0x23 zero sweep (NTFS's unused BPB band). Then FUN_180005470 (:4292-4307) gates the follow-on probe: `record_size & 0x800001ff == 0` (512-multiple), `record_size <= 0x1000`, `index_block != 0 && index_block & 0x80000fff == 0` (4096-multiple) — where record/index sizes come from the +0x40/+0x44 getters (FUN_180004830/870, definitions re-read this session; they are NOT bps/spc readers).

## What our core does

`cr_scan.cpp:792` (quoted in the header) — bps-range, bps%512, spc!=0, **spc pow2**, mft_lcn!=0. No zero sweeps; no record/index shape gates (the record-0 USA probe follows, which the og also has — that part is parity). The comment at :770-779 attributes "bps 512-multiple <= 0x1000, SPC pow2 nonzero" to FUN_180005470 :4292-4307 — but that function gates RECORD/INDEX sizes; the bps/pow2 reading is the core's own invention recorded as if it were the corpus's.

## Gap

Both directions, both falsifiable:
(a) a volume with spc = 3/5/6/9… (any oddball but real formatter output — some third-party formatters and resized volumes produce non-pow2 spc): every og layer accepts (raw nonzero test); the core's probe_family declines → the volume is never engine-scanned. 
(b) a spoofed DBR with OEM "NTFS    ", pow2 spc, plausible mft_lcn, but nonzero reserved bands (+0x0E/+0x10..0x17/+0x20..0x23) or record sizes outside the 512/4096-multiple bands: the og's analyser declines before the record probe; the core dispatches (the engine's own open gates may catch it, but the og never reaches the engine — dispatch-time divergence is observable in the family-recognition log and the lost-volume scoring that consumes probe results).

## Impact

(a) is a detection-rate divergence on exotic-but-real geometry — the false-negative direction the audit weighs most. (b) is a false-positive-dispatch divergence absorbed downstream but still observable. The comment misattribution is a citation-integrity defect: future lanes reading `cr_scan.cpp` will trust a corpus claim that is not in the corpus.

## Reproduction / verification

Decompiled evidence, lead-verified first-hand this session: the analyser conjunction quoted above; FUN_180005470's record/index shape gates; the getter definitions (FUN_180004830 reads +0x40, FUN_180004870 reads +0x44 — not bps/spc). Core side: the gate line and comment quoted at `34d6cf10`.

## Fix plan (for the engineering team)

1. Replace probe_family's NTFS geometry gate with the analyser's actual family: OEM (already), `bps % 512 == 0` (keep the bps<=4096 range as registered core hardening or drop to og form — decide and register), the reserved-zero sweeps (+0x0E-0x0F, +0x10-0x14, +0x16-0x17, +0x20-0x23), `spc != 0` (drop pow2 — no og site requires it; keep a note if the engine open depends on pow2 elsewhere, which it does not: `cluster_bytes = bps * spc` works for any spc).
2. Add the record/index shape gates before the record-0 USA probe: record_size % 512 == 0 && <= 4096; index_block % 4096 == 0 (via the same getter semantics including the 0x400/0x1000 zero-defaults per M-138).
3. Correct the :770-779 comment: FUN_180005470 gates record/index sizes; the bps/spc attribution was a misread.
4. Files: `src/core/orchestrator/cr_scan.cpp`; tests in the orchestrator recognition home.
5. Acceptance criteria: non-pow2-spc fixture dispatches and scans (content identical to a pow2-equivalent control); spoofed-reserved-band fixture declines at dispatch; the comment matches the cited function.

## Tests to add

- `Recognition.NtfsNonPow2SpcDispatches` — RED today (declined).
- `Recognition.NtfsReservedBandNonzeroDeclinesAtDispatch`.
- `Recognition.NtfsRecordSizeShapeGatesAtDispatch` (2048-byte odd record declined before the USA probe).
- `Recognition.NtfsPow2VolumesUnchanged` (control).

## Related findings

- M-138 (the 0x400/0x1000 defaults the shape gates must adopt).
- M-139 (the 55AA question — the og analyser has none; probe_family already agrees).
- M-141 (the mft_lcn bound — the third gate in the same family).

## Open questions

Whether the core's bps<=4096 range cap should stay (the og has no bps upper bound at the analyser; the core's cap is P1-3/P1-4-adjacent hardening) — recommend keeping it and REGISTERING it rather than silently diverging either way.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push — the S65 lane's third landing)
**Verified by:** `NtfsVolume.RecognitionNonPow2SpcDispatches` (RED pre-fix: pow2-declined), `RecognitionReservedBandNonzeroDeclinesAtDispatch` + `RecognitionUnusedTailBandNonzeroDeclinesAtDispatch` (RED pre-fix: dispatched), `RecognitionRecordSizeShapeGateDeclinesAtDispatch` + `RecognitionIndexBlockShapeGateDeclinesAtDispatch` (RED pre-fix: dispatched — 5/7 RED pre-fix, the 2 boundary pins already passing), `RecognitionUnsweptBandsStayAccepted` (the +0x0F/+0x15/+0x18-0x1F/+0x24 boundary pin), `RecognitionShapeGateZeroDefaultsPassAtDispatch` (the M-147 defaults in-band); full suite 1496/1496 green post-fix.
**Resolution notes:** CONFIRMED-DIVERGENCE, with one annotation correction: the corpus re-read (FUN_180004900 :3568-3577, the frame-offset derivation re-done first-hand at the fix: uStack_20d covers +0x0B..+0x0E so the `>> 0x18` byte is **+0x0E, not +0x0F** — the finding's "+0x0E-0x0F" sweep overstated by one byte; +0x15 (media descriptor) is also outside the sweep, consistent with the finding's own list). The landed gate family: OEM, bps%512==0, the zero sweeps {+0x0E, +0x10..+0x14, +0x16..+0x17, +0x20..+0x23}, spc != 0 raw (the pow2 requirement dropped — no og site has it), then FUN_180005470's shape gates before the record-0 USA probe (record % 512 == 0 && <= 4096, index != 0 && index % 4096 == 0) via the M-147 getter twins — `index_block_size_from_dbr` added to cr_ntfs_wire.h (the FUN_180004870 decode with the 0x1000 zero-default; the P1-4 exponent guard kept as the registered hardening the record twin carries). The misattributed comment re-truthed (FUN_180005470 gates record/index sizes; the bps/spc attribution was the core's own invention). Registered hardening kept per the open question's recommendation: the bps [512,4096] range (the D-52 registration the engine ladder carries). The mft_lcn != 0 gate dropped (no og site; mft_lcn 0 self-declines at the record probe). Two fixtures' unrealistic cib=2 (512B clusters with 1024B index blocks — out of the og's own band) corrected to the realistic cib=8 (test_ntfs_volume build_image + the whole-disk FS-marker fixture) — the fixtures were wrong, not the gate.
