# MINOR N-101 — Surface-inventory arithmetic corrections + four unregistered og-codebase surfaces (AutoUpdate, Eaolog channel, DataFile.ini/DRWReport.ini/auto.txt, BuildRaw/DelFat log channels)

**Severity:** MINOR
**Module:** audit-trail (SURFACE-INVENTORY truth) / orchestration-adjacent log channels
**Parameters affected:** C68-adjacent (documentation accuracy of the gate doc), N-048's log-channel census completeness
**Corpus references:** `architecture/00-INDEX.md:64-65` (GetDriverInfo + burn rows); `artifacts/PartitionRecovery/getdriverinfo--tb-module-decode.md` (the never-dispositioned surface — see N-106); `artifacts/UIUX/30-interaction-driver-ground-truth.md:14` (progressive carve-tree growth — dispositioned this session as MATCH: the core's carve stage attaches nodes live via carve_slice, cr_scan.cpp:1877-88, and counters refresh at progress ticks); `og-codebase/DRWReport.ini` (attaches `log\BuildRaw.log` "Raw FileCount" + `log\DelFat.log` "Fat heighClu"); `og-codebase/Eaolog.log` (CEaoActiveOnline licensing log — the 4th root-level log channel, listed by the qa-logs artifact but missed by N-048's census); `og-codebase/AutoUpdate.exe` + `Upgrade.ini` (killDRWUI=1/URL — decoded at deepening-batch3 §4, one-lined at config-data-reference:75); `og-codebase/DataFile.ini` (UTF-16 [HEAD_DATA] state stub)
**Core references:** audited `ff879f9` — `diff-to-fix/SURFACE-INVENTORY.md` §8 ("78 cited / 204 never-cited"; "per-binary 113/114 never-cited"; §1 "EuShare (5 artifacts, cited)"; §4 "EFIBoot* (cited)"), `findings/FIXED/M-024-*.md:6,15` (see N-102)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The inventory's coverage arithmetic undercounts citations (~95 cited
of 282, not 78: brace-group, alias, and short-stem citation forms were
not folded in); two classification rows are wrong (per-binary is
112/114 never-cited — deepening-batch1 is cited by M-064; EuShare's 2
artifacts are both never-cited — the NAS exclusion stands on other
evidence; EFIBoot.md is never-cited — M-040 cites the decompile, not
the artifact); and four og-codebase surfaces have no registered
disposition at all: the AutoUpdate.exe+Upgrade.ini updater pair, the
Eaolog.log licensing channel (the 4th log channel), the
DataFile.ini/DRWReport.ini/auto.txt state stubs, and the
BuildRaw.log/DelFat.log engine run-log channels that DRWReport.ini
references.

## What the reference does / What our core does

These are registration/disposition gaps, not behavior gaps: the four
unregistered surfaces are all OUT-OF-SCOPE by product-frame reasoning
(updater, licensing log, state stubs; the two engine log channels are
og-internal run logs whose data the core surfaces through its own
telemetry). The inventory exists so no surface is silently dropped —
these four were silently dropped. **Reconciliation note (push-time,
over the parallel deep-audit lane):** that lane's I-027 data-asset
register independently dispositioned DataFile.ini (PARITY-OG-CORE-
INVENTORY row 67), DRWReport.ini (row 73), and Upgrade.ini (row 78), and
its N-097 decoded auto.txt as an FVE key snapshot — so three of the four
"unregistered" items now have owners. This finding's remaining scope:
the §8 arithmetic corrections, the three classification-row corrections
(per-binary / EuShare / EFIBoot.md), and the two channels still without
a registration line (Eaolog.log — the 4th root-level log channel, which
extends N-048's census but was not in that lane's register either; and
log\BuildRaw.log + log\DelFat.log, the engine run-log channels that
DRWReport.ini attaches).

## Gap

Falsifiable: grep the trail for "AutoUpdate" / "Eaolog" /
"BuildRaw.log" / "DataFile.ini" — zero disposition hits (verified this
session); the inventory's own §5/§6 rows do not contain them.

## Impact

Trail truth only. Future sessions walking the inventory will believe
the citation map is smaller than it is, and the four unregistered
surfaces could be re-litigated from scratch.

## Reproduction / verification

The parallel-agent G-B full walk this session: per-artifact basename
citation search across the whole trail; every ambiguous match
context-verified; the four unregistered surfaces confirmed zero-hit.

## Fix plan (for the engineering team)

1. Correct SURFACE-INVENTORY §8 arithmetic (~95/~187) and the three
   wrong rows (per-binary 112/114; EuShare 2 never-cited, exclusion
   re-anchored to architecture/05:124 + DRWUI-exe--main-ui:214-15 +
   rtti_classes.json; EFIBoot.md never-cited with the decompile cite
   noted).
2. Add the two remaining OUT-OF-SCOPE registration lines (Eaolog.log —
   licensing channel, extends N-048's census; BuildRaw.log/DelFat.log —
   og engine-internal run-log channels, telemetry-equivalent data already
   in the core). The other three items ride the parallel lane's I-027
   rows and N-097.
3. Fold the GetDriverInfo row into N-106's disposition.

## Tests to add

None (documentation).

## Related findings

N-102 (M-024 propagation), N-106 (GetDriverInfo), N-103/N-104
(truth-pass siblings).

## Open questions

None.


---

## Session-8 lead-lane reconciliation addendum (2026-10-28, the push-window merge)

Three more pointer/arithmetic drifts in the same trail class, from the lead lane's
citation-gap sweep (kept as N-120 in the merged numbering): (a) the I-010 finding's D-26
pointer over-claims — its list includes Installer, Licensing/quota, and runtime-families
under "the D-26 family", but D-26 (PARITY-DELTAS:639-646) contains none of those (licensing
= D-32; installer = S-038/PACKAGING; runtime-families has NO PARITY-DELTAS row at all —
per the register's own rule (:6-7) a missing row is a bug); (b) SURFACE-INVENTORY:85-86's
exclusion pointers target CAPABILITY-MATRIX §3 for FileRepair/Cloud/licensing/languages/
runtime, but §3 registers only Cloud/NAS/remote, BitLocker-derivation, signing,
.rsf-import — the real registers are PARITY-CORE-REC.md:163, OGI:30/:39/:79, D-26/D-32
(wrong-register pointers make registered exclusions look silent); (c) PARITY-CORE-REC's
repair-chain exclusion text sits at :163, not :164 (the off-by-one repeated across three
findings' cites).

---

## Resolution

**Status:** FIXED (one live residue) + FIXED-ALREADY (the rest, by the parallel session-9/SA-10B lanes)
**Fixed in commit:** (this session's N-101 doc-truth batch — the session-38 minor-first lane)
**Verified by:** the live-tree reproduction of each already-fixed half (reads cited below) + the one remaining arithmetic item fixed and re-verified.
**Resolution notes:** Re-verified at current main, item by item: the §8 arithmetic corrections (92-by-basename/110-any-form/134-never-cited + the corrected globs) were ALREADY landed by the session-9 currency pass (SURFACE-INVENTORY:22-27 — read this session); the EuShare 2-artifact row correction at :40 and the never-cited note at :125 — ALREADY landed; the three classification-row corrections — the per-binary row carried the one LIVE residue (it still said "113/114 never-cited; libvmdk ×1" while deepening-batch1 is also cited by M-064 at SI:41/ PARAMETERS:152 — now 112/114 with both cites named); GetDriverInfo — the N-106 disposition row EXISTS at SI:86 (that finding's own resolution); the Eaolog.log + BuildRaw.log/DelFat.log registration lines — ALREADY landed at SI:132-133; the DataFile.ini/DRWReport.ini/Upgrade.ini/auto.txt rides — I-027's rows + this session's N-097 auto.txt correction. The one live edit this session: the per-binary 112/114 arithmetic. Everything else was verified as already landed and is recorded here with its reproduction.
