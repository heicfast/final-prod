# MINOR N-163 — NTFS runlist malformed-input tolerance: the og truncates-and-keeps (and its pre-validator rejects sparse-first); the core rejects the whole fragment — corrupt-runlist salvage differs in both directions

**Severity:** MINOR
**Module:** ntfs (runlist decode)
**Parameters affected:** recovery correctness edge (2), fragment handling (27), error recovery (35)
**Corpus references:** `NTFSSearchDLL_decompiled.c:41853-41970` (`FUN_1800347d0` — the decoder: len_width>8 → break + return SUCCESS at :41891; off_width>8 → success :41992-94; zero-length runs accepted; buffer exhaustion → success :41875/:41888), `:23175-23210` (`FUN_18001d5f0` — the pre-validator REJECTS first-header-byte nibble==0, i.e. sparse-first $DATA, at :23184; enforced at 5 call sites :23412/:23506/:23533/:24337/:24643)
**Core references:** `src/core/ntfs/cr_runlist.cpp:77-86` (len_width==0/>8, run_len==0 → ERR_BAD_PARAMETER; truncation → ERR_TRUNCATED), `src/core/ntfs/cr_mftwalk.cpp:561-567` (a failed fragment contributes no runs); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

On a $DATA runlist torn mid-entry (a garbage width byte after valid runs), the og's
decoder KEEPS the runs decoded so far and returns success — the file recovers its
prefix content. The core rejects the whole fragment (ERR_BAD_PARAMETER) and the file
surfaces with no extents. Conversely, a sparse-FIRST $DATA (fully sparse file) is
REJECTED by the og's pre-validator at five attribute-parse sites, while the core accepts
and represents it. Salvage posture differs in both directions.

## What the reference does

The decoder's malformed-input branches (cites above): every width violation and
buffer-exhaustion path returns SUCCESS with the partial run set; the separate
pre-validator (FUN_18001d5f0, :23184) rejects the sparse-first/zero-width-header shape
BEFORE the decoder runs, at the five $DATA parse sites.

## What our core does

Strict decode (cr_runlist.cpp:77-86): any width violation or zero-length run fails the
whole fragment; the mftwalk drops the fragment's runs (:561-567). No sparse-first
rejection anywhere (the core represents sparse runs natively).

## Gap

Torn runlist: og → partial extents; core → no extents. Sparse-first: og → attribute
rejected; core → accepted. Both classes are corrupt-input salvage postures, not
valid-input behavior.

## Impact

Deleted-file recovery on volumes with torn MFT records: the core loses recoverable
prefixes the og salvages (the salvage-over-purity doctrine the audit has consistently
pushed). The sparse-first direction is a core superset (registered nowhere). Low
frequency, real recovery targets.

## Reproduction / verification

Corpus-derived (lane A first-hand: the four decoder branches + the validator gate with
all five call sites — cites above). Core-derived (the reject branches read directly).
B2's "runlist codec MATCH" verdict covered the VALID-input codec only — the malformed
branches were never compared (PARITY-DELTAS: no row).

## Fix plan

1. Torn-tolerance: in `RunList::decode`, a width violation AFTER ≥1 valid run returns
   OK with the runs so far (the og's break-and-keep), setting a `torn` flag on the
   result so the walker can grade the file (PartialRisk) rather than silently pretend
   completeness. Zero-length FIRST header byte still rejects (that is the og validator's
   class — keep the core's sparse-first acceptance and REGISTER it).
2. Files: `src/core/ntfs/cr_runlist.cpp`, `cr_mftwalk.cpp` (grading), `include/cr/ntfs/`
   (the flag).
3. Registration: one PARITY-DELTAS line for the sparse-first superset.
4. Tests: below.
5. Acceptance: the torn fixture (valid runs + garbage width) yields the prefix extents
   with PartialRisk; the fully-garbage runlist still yields none; valid runlists
   unchanged (existing NTFS corpus tests green).

## Tests to add

- `RunList.TornAfterValidRunsKeepsPrefix` (fails today)
- `RunList.TornPrefixGradedPartialRisk`
- `RunList.SparseFirstAcceptedAndRegistered` (superset pin)
- `RunList.AllGarbageYieldsNothing` (regression)

## Related findings

- N-162 (the same lane's numeric-hygiene family)
- D-14/F7 (the guard family this does not touch)

## Open questions

None.
