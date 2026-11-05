# MINOR N-170 — stale pre-erratum claims batch: 8 core-tree comments/docs still carry the refuted pre-E-A/E-B reading of EUUserData/GetRawUserInfoMap (+2 corpus-side residues)

**Severity:** MINOR
**Module:** carve (user-sign channel docs) / doc-truth family
**Parameters affected:** documentation accuracy (68), registry truthfulness
**Corpus references:** the erratum itself: `re-workspace/artifacts/CORRECTIONS.md` #9 (EUUserData feeds CHKRepairFileSign, NOT the RAW engine; GetRawUserInfoMap = the per-scan carve-statistics map) — core-side echo `diff-to-fix/CORPUS-ERRATA.md` E-A/E-B
**Core references:** (1) `include/cr/carve/cr_usersigns.h:8` ("The og's carve layer consults it alongside raw_signs.json" — the refuted reading); (2) `cr_usersigns.h:86-87` ("built-ins were already probed by the earlier layers" — false per S-055); (3) `cr_usersigns.h:42-43` (kMaxEntries comment "the og ships 111" — the cap is enforced against total SIGNS, og ships 190/191; 256 is 1.34× headroom, not 2.3×); (4) `cr_usersigns.h:46` ("longest ext is 4" — actually 8, "manifest"; the cap is correct); (5) `docs/PARITY-OG-CORE-INVENTORY.md:84` ("GetRawUserInfoMap feeds the RAW scan-setup trigger surface" — refuted: it is scan OUTPUT); (6) `docs/PARITY-DELTAS.md` D-52 (:1176-77) (groups GetRawUserInfoMap with the registry "seeds" — it seeds nothing); (7) D-60f (:1838) ("the engine reads … EUUserData" — the ENGINE never reads it; DRWUI does); (8) `diff-to-fix/SCAN-ENGINE-DELTA.md` C4 (:89) ("+ EUUserData MIME" in the og classification chain — no binary consumer reads the mime member); audited at core `8d0d88b8
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Eight in-tree claims still describe the pre-erratum architecture (EUUserData as an
engine-side carve input; GetRawUserInfoMap as a setup feed). Two corpus-side artifacts
(`config-files/userdata-remoteconfig` §1; `rawsearch-carving--deep-dive` §3 item 2)
carry the same refuted claims and await the errata protocol's fold-on-next-touch.

## What the reference does

The erratum (CORRECTIONS.md #9 = E-A/E-B) is the corpus's own truth record; the core's
CORPUS-ERRATA.md mirrors it. The claims above predate or ignore it.

## What our core does

The eight claims stand as listed (all read this session by lane C; the load-bearing
ones re-read by the lead auditor — items 1/2/5/6/7).

## Gap

Doc-vs-truth: each item contradicts the decoded architecture.

## Impact

Future sessions and hosts reading these comments/docs inherit the refuted model —
exactly the class that produced the M-048/D-104 mis-framing originally. Item 2 also
masks S-055's inversion.

## Reproduction / verification

Each item read at its cited location this session; the erratum read at both ends.

## Fix plan

1. One doc-truth commit: rewrite items 1-8 to the E-A/E-B truth (EUUserData → CHK repair
   chain only; GetRawUserInfoMap → per-scan output map; the sign census 111 keys/190
   signs deduped, 112/191 raw; ext max 8; the kMaxEntries headroom corrected).
2. Files: `include/cr/carve/cr_usersigns.h`, `docs/PARITY-OG-CORE-INVENTORY.md`,
   `docs/PARITY-DELTAS.md` (D-52/D-60f), `diff-to-fix/SCAN-ENGINE-DELTA.md` (C4).
3. Corpus-side: fold the two artifact residues per the errata protocol (corpus repo
   maintainers).
4. Tests: none (comment/doc only); the doc-truth gate script's pattern list may gain the
   phrases "consults it alongside" / "scan-setup trigger" as forbidden strings.
5. Acceptance: grep for the refuted phrases returns zero in core docs.

## Tests to add

- Doc-gate pattern additions (optional, per the N-135 gate convention)

## Related findings

- S-055 (item 2's code-level counterpart)
- LC-9/I-035 (the decode that supersedes these claims)
- N-135/N-148/N-150 (the doc-truth batch family)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-170 landing, S46 MINOR-first lane — post-push SHA in the ledger row and worklog entry)
**Verified by:** the acceptance grep — `consults it alongside` / `scan-setup trigger` → zero hits in src/include/docs/diff-to-fix (the finding's own acceptance criterion). Doc-only (no code change); the suite green at the landing (1275 ran / 1233 pass / 42 env-skips / 0 fail). Items 2 and 4 were ALREADY re-truthed by the parallel windows' landings (item 2 by the S-055 fix — the built-in-first text at the current :92-94; item 4 by N-179 — the kMaxExtLen = 8 "manifest" comment): both verified FIXED-ALREADY at the current tree and excluded from this commit's edit set.
**Resolution notes:** Six sites re-truthed to the E-A/E-B reading (CORRECTIONS.md #9 re-read first-hand this session): (1) cr_usersigns.h's header block — EUUserData never reaches the og's RAW engine (sole og consumer: DRWUI's CHKRepairFileSign, the CHK repair chain, :1005387/:1005818/:721520-721680; zero references in RAWSearchDLL's 80,614 lines); the core's carve-side consumption is the registered deliberate superset D-104; (3) kMaxEntries — the cap counts TOTAL SIGNS (the loader's out.signs.size() gate), the og ships 190 deduped / 191 raw (N-169), 256 = 1.34×; (5) PARITY-OG-CORE-INVENTORY's OQ-SA-3 resolution — GetRawUserInfoMap is the per-scan carve-statistics OUTPUT map (E-B), the N-140 surface's og counterpart; (6) D-52 — GetRawUserInfoMap removed from the "raw-dual-registry seeds" grouping; (7) D-60f — EUUserData removed from the engine's read list (the ENGINE never reads it; DRWUI does, for CHK repair); (8) SCAN-ENGINE-DELTA C4 — the "+ EUUserData MIME" claim struck (no binary consumer of the mime member). The two corpus-side residues (userdata-remoteconfig §1; rawsearch-carving--deep-dive §3 item 2) await the corpus maintainers' fold-on-next-touch — unchanged, read-only repo.
