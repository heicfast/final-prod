# MAJOR M-035 — Engine-side FileFilter/extension classification used in lost-partition validation; core validates structure only

**Severity:** MAJOR (user-observable in edge cases: false ghosts accepted / real ghosts rejected)
**Module:** lost-partition, query (filter)
**Parameters affected:** B3 (false-positive rate — the og rejects ext-inconsistent ghosts), A30 (candidate validation), B16 (filter model — engine-side consumption)
**Corpus references:** exFATSearchDLL_decompiled.c — FileFilter.xml parse :47102-47560 (moduledir + "FileFilter.xml" sprintf :47165; Graphic/iconIndex==0 filter :47279-47290; "TIFF" description filter :47324; _strupr ext set :47428-47440); classifier `FUN_180024fe0` :30785-30960 (170-entry x9-byte hardcoded ext table DAT_18005a090 :30824; RAW-set membership :30843-30854; MOV/MP4/JPG + JPG/JPEG specials :30858-30920; SUBSTRING matching); **validation scorer `FUN_180025390` :30973-31156** — per file: data-start = `cluster_size*(start_cluster-2)+base` :31057-31063, signature-map lookup `FUN_1800225a0` :28535-28566, match counting, `ratio = matched/(considered-unmatched)` :31150, **reject when `considered/2 <= failed && ratio <= 0.3 && failed != 0`** :31152-31155; called from ≥5 sites (:38364, :38665, :39688, :40096, :42733). Evidence: `evidence/decompiled-quotes/s3-filefilter-health.md` §A2.
**Core references:** `src/core/lost/cr_candidates.cpp:69-140` (BPB/DBR structural evidence only); `docs/PARITY-DELTAS.md:663` (post-scan-only filter, by design). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-019` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's exFAT engine consumes FileFilter.xml (plus a hardcoded 170-entry extension table) as part of lost-partition validation: it samples found files, checks name-extension ↔ content-signature consistency, and rejects a candidate when more than half the sampled files mismatch and the match ratio is ≤ 0.3. Our core validates candidates structurally only (BPB/DBR probes) — a valid-BPB ghost full of extension-mismatched content passes our validation and is rejected by the og's.

## What the reference does

The full mechanism is quoted: parse → classify (substring ext match against the hardcoded table + RAW set + specials) → score per candidate (data-start lookup in the signature map, count matches) → the ≤0.3-ratio + majority-failed reject rule, wired at five call sites in the engine.

## What our core does

Structure-only scoring; no content sampling during candidate validation; no extension table anywhere in the engine layer.

## Gap

Missing validation signal. Falsifiable: a lost-partition image with a valid BPB but >50% of signature-bearing files whose extensions don't match their content — og suppresses/rejects it; ours reports it (and ranks it by structure).

## Impact

False-positive ghosts survive our filter (user wades through junk candidates); conversely a content-poor but real ghost is unaffected by us but might score differently. Bounded to the lost-partition path (the scorer is engine-side).

## Reproduction / verification

Corpus-derived (all five components quoted). UNVERIFIED-BY-EXECUTION (synthetic ghost fixture with mismatched extensions — generatable).

## Fix plan

1. Add a content-consistency pass to candidate validation: after structural probes pass, sample up to N found files (the og's sample path uses the file-set binding from the CalcPartition machinery — our quick-scan tree walk provides the same data), classify each file's extension via the M-023 (parallel leg) category table, and compare against the carve signature at the file's data start.
2. Implement the reject rule verbatim: `considered/2 <= failed && ratio <= 0.3 && failed != 0`.
3. Decide the substring-vs-exact classification semantics (og uses SUBSTRING; exact is safer against false matches — document the choice).

## Tests to add

- `Lost.ContentInconsistentGhostRejected` — valid BPB + mismatched content; assert rejection (fails today).
- `Lost.ContentConsistentGhostAccepted` — matched content; assert acceptance (regression guard against over-rejection).
- `Lost.RatioBoundaryThirtyPercent` — ratio exactly 0.3 with majority failed → reject (pins the boundary).

## Related findings

M-023 (parallel leg) (the filter table this consumes), M-017 (parallel leg) (scoring — the content counts also feed ranking), I-010 (OQ-T9 context).

## Open questions

~~Whether the same scorer exists in the OTHER SearchDLLs (NTFS/FAT/Btrfs/…) or is exFAT-only — byte-search the 170-entry table pattern (x9 stride) in each binary; blast radius depends on the answer (tracked OQ-FLT-1).~~

**ANSWERED (DFX30, OQ-FLT-1 resolution, 2026-10-26):** the scorer is
**exFAT-only** — zero `<= 0.3` compare sites in every other engine decompile
(first-hand scan; evidence
`diff-to-fix/evidence/decompiled-quotes/s30-oq-flt1-extension-table-distribution.md`).
The fix scope of this finding is the exFAT lost-partition validation path
alone. NOTE for the implementer: the 169-entry extension table itself is a
4-engine shared vocabulary (live in FAT/exFAT/RAW as the `+0x18`
extension-INDEX -> name mapping, gate `< 0xAA`; dead copy in NTFS; absent in
EXT4/Btrfs/ReFS/HFS+/ZFS) and is set-identical to DRWUI's grading table —
the core's single shared table (`cr_grading.cpp kGradingExtensions`) can
serve the index mapping too if the fix needs index->name resolution.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's `fix(M-035)` push)
**Verified by:** 15 new tests (test_consistency.cpp): the finding's three named
tests (Lost.ContentInconsistentGhostRejected, Lost.ContentConsistentGhostAccepted,
Lost.RatioBoundaryThirtyPercent) + 9 scorer/helper unit tests + 2 tree-detach
tests + the two end-to-end whole-disk scans. Fail-before EXECUTED (the reject
wiring temporarily gated off: Lost.WholeDiskScanDropsContentInconsistentExfatGhost
RED — the ghost's 6 files surfaced; restored → GREEN). Suite 808/808
linux-release. Corpus re-verified FIRST-HAND this session before implementing:
FUN_180025390 :30973-31156 (the scorer loop, counters, data-start math, the
out-of-bounds invalidation, the reject compare), FUN_180024fe0 :30785-30960
(classifier: the 170-entry table strstr + MOV/MP4/JPG prefix specials + the two
memicmp literals), FUN_180025d60 :31566-31680 (the extension extraction +
filter-set membership + TIFF/JPEG specials), and the call site :38340-38380
(ExFatCalcPartition: accept iff valid && ratio > 0.3).
**Resolution notes:** Landed the engine-side content-consistency validation as
`cr::lost` scorer (cr_consistency.h/.cpp): per candidate file — considered
(non-directory, the og's 0x10 skip), filterable (extension in the checkable
vocabulary = the carve registry's families + the TIFF/JPEG specials — the
shared-vocabulary decision OQ-FLT-1 blessed, camera-RAW families without a core
signature stay uncheckable, bounded vocabulary registered), matched (the
first registry signature whose magic matches at the file's data start, registry
order = the carve dispatch precedence; classifier = substring strstr over the
uppercase ASCII name projection + the MOV/MP4/JPG family specials). Reject rule
verbatim from the decompile: considered/2 <= filterable && ratio < 0.3 &&
filterable != 0 — CORRECTION to the finding text: the decompile's
`<= 0.3 && != 0.3` is a STRICT less-than; ratio == 0.3 does NOT reject (pinned
by Lost.RatioBoundaryThirtyPercent). A data start beyond the volume end
invalidates outright (the og's *param_3 path). Wiring: scan_candidate scores
the walked files after the engine dispatch (the og scores inside the walk; ours
emit-then-score is the same observable outcome) and on reject detaches the
candidate's subtree (ResultTree::detach_subtree — new: removes nodes/roots/
indexes, releases props refs once per node, severs the root's links so later
subtree walkers are safe), zeroes the file counts, bumps
ScanStats::lost_content_rejected, logs through the N-048 seam. The candidate
itself stays listed (the og's per-hit persistence — M-042; the reject drops the
candidate's FILES, exactly the CalcPartition accept/reject observable).
ExFAT-only per OQ-FLT-1; applied uniformly at the candidate-shaped seam (lost
fan-out + LDM volume scans — the og's LDM-path wiring is corpus-UNVERIFIED,
documented). Fix-plan fidelity: step 1 (content pass over the quick-scan tree
walk) done; step 2 (reject rule verbatim) done with the strict-< correction;
step 3 (substring semantics kept, og-faithful — the "exact is safer" option was
NOT taken: the decompile says substring, substring it is, with the
family-specials decoded). No safety hardenings touched.
