# MINOR N-129 — PARITY-OG-CORE-INVENTORY numeric and citation defects: api-ms count 44→40, CORE_RL 19→24, .mo 25→30, the DRWReport cite :84→:97, the EuOfficeTools case mismatch, and a stale self-nested row

**Severity:** MINOR (doc-truth; the register's coverage verdicts are unaffected — re-verified)
**Module / Surface:** gate docs (PARITY-OG-CORE-INVENTORY.md — the og-tree disposition register)
**Parameters affected:** P68 (documentation accuracy), P69-adjacent (register hygiene)
**Corpus references:** `og-codebase/` tree at `be0cfe9` (counts re-executed this session); `re-workspace/inventory.csv` (the corpus's own count: 40 api-ms)
**Core references:** audited commit `84a1832` — `docs/PARITY-OG-CORE-INVENTORY.md:53` ("(44 DLLs)"), `:267` ("CORE_RL_* (19)"), `:253` ("all **25** .mo runtime modules"), `:73` (cites "SURFACE-INVENTORY.md:84"), `:30` ("EUOfficeTools.exe"), `:99` ("og-codebase/ (dir)" row)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber reconciliation (S11-lead, 2026-10-29):** the N-129 ID collision was resolved by the session-11B trail lane's push (accepted at the S11-lead rebase): THIS file keeps **N-129** — its lane-B push (0e279e43, 17:32:28Z) preceded the wave-3 bookkeeping file's (f6c00177, 17:33:36Z) by 68 seconds, so the wave-3 file renumbers to N-135. The S11-lead's draft repair had renumbered THIS file (the wrong direction, based on commit-graph position rather than the file-addition timestamps); corrected at the rebase reconciliation.

**Prior audit check:** The register was built by the dev-sign session and currency-passed by sessions 8/9; the counts were never re-executed against the tree. The session-10 completeness sweep (1470 files vs 112 rows) is the first full re-count.

## Summary

Six numeric/citation defects in the og-tree disposition register, found by the session-10 full-tree sweep (1470 files / 167 top-level entries vs the register's 112 rows). All six are count/cite drift; no coverage verdict changes (the sweep confirmed 1468/1470 files carry a disposition — the 2 exceptions are N-130/OQ-S10-1).

## What the reference does

n/a — this is a core-side register-truth finding. The og tree at `be0cfe9` contains: 40 api-ms-win-* DLLs at root (25 core + 15 crt; 40 unique basenames tree-wide across 200 copies in 5 payload roots — the corpus's own inventory.csv agrees: 40); 24 `CORE_RL_*.dll` files in EUFileRepair/; 30 `.mo` files in PartitionRecovery/ (all 30 have register rows); the DRWReport OUT-OF-SCOPE row lives at SURFACE-INVENTORY.md:97 (line 84 is the Imaging row); the file is `EuOfficeTools.exe` (lower-case u's — the register's "EUOfficeTools.exe" matches no file); there is no `og-codebase/og-codebase/` self-nested directory.

## What our core does

The register carries the six stale values listed in Core references. The `.mo` miscount is the most consequential of the six: "all 25 .mo runtime modules" understates the module census by five — a future session reconciling module coverage against the register's count would mis-bucket.

## Gap

Falsifiable per item: `ls og-codebase/*.dll | grep -c "api-ms-win-"` → 40, register says 44. `ls og-codebase/EUFileRepair/ | grep -c CORE_RL` → 24, register says 19. `ls og-codebase/PartitionRecovery/*.mo | wc -l` → 30, register says 25. The DRWReport cite at :73 points at the Imaging row. The EuOfficeTools casing matches no file. The :99 row describes a directory that does not exist.

## Impact

Register consumers (every future parity session's scope determination). Count drift compounds: the sweep's own arithmetic (§8 of SURFACE-INVENTORY) leans on these numbers. No behavioral impact.

## Reproduction / verification

Executed count re-runs this session (the parallel sweep agent's Section B table, lead-spot-verified on the .mo and api-ms counts). The exact-match controls that PASSED in the same sweep (Dict 228, FileFilter 1489, raw_signs 139, userdata 111/190, BUILDPE 0/0) prove the method.

## Fix plan (for the engineering team)

1. `:53` 44→40 (25 core + 15 crt; note the 200-copies×5-roots distribution).
2. `:267` 19→24 CORE_RL_*.dll.
3. `:253` 25→30 .mo (all 30 dispositioned — the coverage claim strengthens).
4. `:73` cite → SURFACE-INVENTORY.md:97.
5. `:30` `EUOfficeTools.exe` → `EuOfficeTools.exe`.
6. `:99` delete or repoint the stale "og-codebase/ (dir)" RESEARCH-COPY row.
7. Acceptance: re-run the count commands in this finding; every register number matches.

## Tests to add

None (doc-only). A `tools/verify_inventory_counts.py`-style re-count script would prevent recurrence, but that is the repo owner's call (read-only session).

## Related findings

**N-130/OQ-S10-1** (the two genuinely undispositioned files the same sweep found) · **N-101/N-120** (the register-arithmetic drift family) · **N-108** (stale-constants class)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-129 landing — recorded post-push in the ledger row)
**Verified by:** the count re-runs executed against corpus `be0cfe9` this session (the acceptance the fix plan demanded): `ls og-codebase/*.dll | grep -c api-ms-win-` → 40 (25 core + 15 crt; 40 unique basenames, the corpus's inventory.csv agrees); `ls og-codebase/EUFileRepair/ | grep -c CORE_RL` → 24; `ls og-codebase/PartitionRecovery/*.mo | wc -l` → 30 (all 30 dispositioned — the coverage claim strengthens); the DRWReport row confirmed at SURFACE-INVENTORY.md:97; `EuOfficeTools.exe` confirmed present on disk with the lower-case-u name (the register's "EUOfficeTools.exe" casing matched nothing). Every register number now matches the tree.
**Resolution notes:** All six defects repaired: (1) :53 api-ms count 44→40 with the 25+15 split and the 200-copies/5-roots distribution note; (2) :267 CORE_RL_* 19→24; (3) the .mo census 25→30 (the "all rows exist" claim re-verified at 30); (4) the DRWReport cite :84→:97 (the Imaging row → the crash-reporter row); (5) the EuOfficeTools casing corrected to the on-disk "EuOfficeTools.exe" — NOTE: this lane's first pass misread item 5 as a non-existent file and dropped the entry; the re-verification against the tree caught the over-correction and restored the entry with the correct casing (the finding's own text was right; the fix follows it exactly); (6) the stale self-nested "og-codebase/ (dir)" row deleted. Doc-only — no code change; the counts are the verification.


## N-160 erratum (2026-10-29, the S56 lane)

The file's two "OQ-S10-1" references (the summary line "the 2 exceptions
are N-130/OQ-S10-1" and the related-findings line) mis-point: the
sfc/VideoFileRepairer exception was **OQ-S10-2** (the VideoViewer/sfc
tracked question, superseded by N-138's decode — SURFACE-INVENTORY:220 and
the ledger's OQ-S10-2 row); OQ-S10-1 is the thumbnail-codec-bound-formats
question (tracked-open). Recorded per N-160 item 3; the original text
stands above as history.
