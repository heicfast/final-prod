# MINOR N-156 — Register-truth batch: I-034's census scope label + phantom-script citations, N-129's OQ-S10-1/S-10-2 cross-ref swap, the inventory row 79/104 gaps, I-033 §12's overstated invariant, and the M-117 ID collision record

**Severity:** MINOR
**Module:** audit-trail (the diff-to-fix registers' own truthfulness)
**Parameters affected:** P68 (documentation accuracy — the registers), P65-equivalent (the audit's own reproducibility: a phantom citation breaks it), P46 (the trail's contract with future lanes)
**Corpus references:** n/a — the defects are in the trail's own registers; the ground-truth checks executed against the corpus this session are cited per item.
**Core references:** audited `869da779`. (1) `diff-to-fix/findings/INFORMATIONAL/I-034-never-cited-artifact-register.md:6` ("750 entries, top+1 deep") and `:46` ("script: `scripts/s41_never_cited_census.sh` … `scripts/s41_ogfile_census.py`"); (2) `diff-to-fix/findings/FIXED/N-129-parity-og-core-inventory-numeric-defects.md:15`/`:53` ("the 2 exceptions are N-130/OQ-S10-1"); (3) `docs/PARITY-OG-CORE-INVENTORY.md` row 79 (the EUFileRepair bundle's enumeration) and row 104 (SaveScan/*.rsf); (4) `diff-to-fix/findings/INFORMATIONAL/I-033-config-data-asset-register-extension-batch.md:67-71` (§12's RawConfig invariant); (5) the M-117 collision: `diff-to-fix/FIXED-LEDGER.md:436` (the retired suppress_overlap M-117 row) vs the S41 lane's DBX filing (renumbered **M-118** this session).
**Status:** READ-ONLY FINDING — no fix applied (the M-117→M-118 renumber itself was performed audit-side this session per the standing renumber convention; the remaining items are fix-lane work)

## Summary

Seven register-truth defects found by the S42 verification passes (the folder-census re-walk and the config deep-diff), all doc-level: the I-034 register mislabels its own census scope (the "750 entries, top+1 deep" claim is wrong on both numbers' relationship — 555 entries live at top+1; 750 is files at depth ≤2, leaving 720 deeper files outside the labeled scope) and cites two census scripts that do not exist anywhere in the repo (the census is not reproducible from the citation); the retired N-129 swaps OQ-S10-1 for OQ-S10-2 in its exceptions note; the PARITY-OG-CORE-INVENTORY's row 79 under-enumerates the EUFileRepair bundle (2 of its 5 executables missing from the row's own list) and row 104 carries no cross-note to the active S-048/S-049/M-103 session-tree findings; I-033 §12's RawConfig invariant overstates coverage (STL/VPJ have no core carve rows — inert both sides — and nine further `[Filter]` keys have zero engine-side presence in the og); and the M-117 ID collision (two findings filed under one ID — the retired suppress_overlap row and the S41 DBX filing) needed the SA-9 renumber executed this session. None changes a coverage verdict; all degrade the trail's navigability and reproducibility.

## What the reference does

n/a (the "reference" is the trail's own truth standard: the message-truth rule, the SA-9 earlier-push-wins rule, the phantom-SHA/phantom-cite class the trail has repaired repeatedly — the N-129/N-135/N-148 lineage).

## What our core does

n/a (register defects). Per item, the verified truth (all first-hand this session):

1. **I-034's scope label:** a full recursive walk of `og-codebase/` counts 1,538 entries (68 dirs + 1,470 files); entries at depth ≤1 = **555**; **files at depth ≤2 = 750** (root + first-level dirs + BUILDPE/EaseUS-x64). The register's "750 entries, top+1 deep" conflates the two. The consequence: ~654 diskhealth QML files + ~68 BUILDPE nested files (720 total at depth ≥3) sit outside the census's literal scope and rest on wholesale family lines (the diskhealth UI-exclusion line; BUILDPE row 57) — the "no undispositioned product surface" verdict survives (the S42 walk re-verified every one of them), but the census is weaker than labeled.
2. **I-034's phantom scripts:** `find . -name 's41_ogfile_census.py' -o -name 's41_never_cited_census.sh'` over the whole audit tree → **zero hits**. The register's "Executed this session" line cites scripts that were never committed; the census cannot be reproduced from the citation.
3. **N-129's cross-ref swap:** the file says the session-10 sweep's two undispositioned exceptions are "N-130/**OQ-S10-1**" — the sfc/VideoFileRepairer exception was **OQ-S10-2** (the VideoViewer/sfc tracked question, superseded by N-138's decode; SURFACE-INVENTORY:220, FIXED-LEDGER:294). OQ-S10-1 is the thumbnail-codec question. The retired file's note mis-points a future reader.
4. **Row 79 (EUFileRepair):** the row names EUImgRepairFile/EuPDFRepairFile/EuOfficeRepairFile + runtimes; the folder also ships **EUAdobeRepair.exe (431,192 B)** and **EUArchiveRepair.exe (599,128 B)** — covered by rows :76/:77 and corrected in prose by I-033 entry 10, but the row's own list is incomplete (the S42 folder census enumerated all 87 files).
5. **Row 104 (SaveScan/*.rsf):** the row says "VERIFIED" (accurate for the container grammar — the s10 executed evidence) but carries no cross-note to the ACTIVE S-048/S-049/M-103 session-tree findings; a lane reading only the register could mistake it for full session-tree parity.
6. **I-033 §12's invariant:** the register claims RawConfig's keys minus "core's grammar-covered set (heic/webp/m1v/m2ts/mobi/7z/webm/m2t/stl/vpj)" equal exactly the open findings' set. Verified truth: STL/VPJ have no core carve rows AND no og analyser (inert both sides — they are not "grammar-covered"); M2T rides the TS family; M4P is covered only as an MP4 brand alias; and nine further keys (INSP/INSV/CRM/FFF/DPX/MUSX/RVT + STL/VPJ) have zero engine-side presence in the og. The invariant as written would not trip on the drift it claims to catch; the actual uncovered-but-typed remainder is N-154's five branches.
7. **The M-117 ID collision (record; repair executed):** the S41 lane filed the DBX analyser finding as M-117 at `9cc6a03e` (2026-10-28T19:20:03Z) with "M-117 and I-034 confirmed free" — 2.5 minutes after `0f5d57fb` (19:17:34Z) had renumbered the suppress_overlap finding M-116→M-117 and retired it with the ledger row (`FIXED-LEDGER.md:436`). Two findings shared one ID until this session's renumber (the DBX finding is now **M-118**; the retired row keeps M-117 per SA-9 earlier-push-wins). The freedom check raced the push window — the same class as the lane's own N-151→N-152 double renumber, but this one was never caught because the collided-with row was retired rather than active.

## Gap

Falsifiable per item: (1) count the og-codebase tree's depth-≤1 entries (555 ≠ the labeled "750, top+1"); (2) `find` the two cited scripts (zero hits); (3) read N-129:15 against SURFACE-INVENTORY:220 (the OQ number is swapped); (4) list `og-codebase/EUFileRepair/*.exe` (5 executables vs the row's 3); (5) read row 104 (no S-048 cross-note); (6) enumerate RawConfig's 34 keys against the og registry census (the invariant's set arithmetic fails); (7) `git log --format='%ci' 0f5d57fb 9cc6a03e` (the 2.5-minute window) + read FIXED-LEDGER:436 (the collision).

## Impact

Audit-trail navigability and reproducibility. The phantom-script citation is the worst offender: it claims reproducibility that does not exist (the class N-135's own fix plan guards against for SHAs, unguarded for script paths). The scope-label defect weakens the "no undispositioned product surface" headline for a future reader who checks the arithmetic. The OQ swap mis-points a probe. None affects product behavior; all cost future lanes time.

## Reproduction / verification

Executed this session (the evidence file §3 carries the command transcripts): the recursive census counts, the find-negative on the scripts, the file reads, the commit-timestamp check on the collision pair, and the RawConfig key enumeration (against `config/RawConfig.ini` + the trail's registry census).

## Fix plan (for the engineering team)

1. **I-034 repair (doc-only):** re-label the census scope ("750 files at depth ≤2; the 720 deeper files — diskhealth's QML tree + BUILDPE's nested payload — rest on the wholesale family lines, re-verified S42"), and either commit the two census scripts or replace the citations with the inline method description (the S42 evidence file §3's commands).
2. **N-129 erratum note (doc-only):** a one-line addendum in the FIXED file ("the sfc exception was OQ-S10-2, not OQ-S10-1 — the S42 repair").
3. **Row 79/104 repairs (doc-only):** add the two missing executables to row 79's list (with the :76/:77 cross-refs); add the S-048/S-049/M-103 cross-note to row 104.
4. **I-033 §12 rewrite (doc-only):** restate the invariant with the verified inert-key set (STL/VPJ + the nine no-analyser keys as "inert display-name keys, no engine presence either side") and point the uncovered-but-typed remainder at N-154.
5. **The collision class (process):** the next-free-ID check must run against the LEDGER (resolved rows included), not just the active board — the S41 lane's freedom check missed a retired row. One sentence in README's conventions block.
6. Acceptance: every check in the Gap section passes (the counts match the labels; the scripts exist or the citation is method-inline; the OQ numbers are right; the rows enumerate their folders; the invariant's arithmetic holds).

## Tests to add

Doc-truth (no unit tests): the audit's own verification gates — a `doc_truth_gate`-style check that every `scripts/*.py|sh` cited in diff-to-fix/ exists (the phantom-citation guard), and an ID-uniqueness check across active findings + ledger rows (the collision guard).

## Related findings

N-135 (the fix-wave bookkeeping-truth class — this is its audit-side sibling); N-150 (the doc-truth batch); N-129/N-134 (the retired registers the defects live in); M-118 (the renumbered finding whose collision is item 7); I-034/I-033 (the registers carrying items 1/2/6).

## Open questions

None. (Item 7's repair is already executed audit-side; items 1-6 are fix-lane doc work.)
