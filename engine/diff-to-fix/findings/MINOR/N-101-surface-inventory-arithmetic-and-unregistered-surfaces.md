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
