# MINOR N-108 — Product docs/PARAMETERS.md §3 still carries the pre-fix constants; the SCAN-ENGINE-DELTA "fixed this session" claim is false

**Severity:** MINOR
**Module / Surface:** Product documentation accuracy (engine-side constants row set)
**Parameters affected:** Q68 (documentation accuracy), C-parameters (the documented values for the carve window + user-sign caps)
**Corpus references:** the corrected values' sources: `diff-to-fix/findings/FIXED/S-039-user-signs-caps-reject-og-euuserdata.md:17-20` (the v3 caps: 16→48 bytes, 8→12 signs, 512→1024 offset, 63→80 mime) and `findings/FIXED/S-037`/`S-007` lineage for the carve window; the og's own caps measured on the shipped DB (corpus `userdata-remoteconfig--signature-and-flags.md` + the S-039 measurement)
**Core references:** audited core `35adcb9`: `docs/PARAMETERS.md:78` — "| Carve window | 4–8 MiB scan windows; window-aware read view (M-025, D-101 stateless validator architecture) | cr_carve.h |" and `:79` — "| Raw-sign table | 139 formats + user signs (caps: 256 entries / 8 signs / 16 bytes / offset ≤ 512, M-048) | cr_rawsigns.h, cr_usersigns.h |". The TRUE code constants: `src/core/carve/cr_carve.cpp:35-36` — `constexpr u64 WINDOW = 1ull << 20; /* 1 MiB */` / `constexpr u64 OVERLAP = 4096;` (header comment `include/cr/carve/cr_carve.h:4`: "Pipeline: 1 MiB sequential windows (+4 KiB overlap)"); `include/cr/carve/cr_usersigns.h:40-47` — `kMaxSignBytes=48`, `kMaxEntries=256`, `kMaxSignsPerEntry=12`, `kMaxOffset=1024`, `kMaxExtLen=8`, `kMaxMimeLen=80`. Adjacent: `include/cr/carve/cr_rawsigns.h:7` comment "Our grammar validators (30 formats)" predates S-007 batch-1's +26 validators (`e018bf4`). The false trail claim: `diff-to-fix/SCAN-ENGINE-DELTA.md:204-207` and `:264-265` — "Two stale rows found in docs/PARAMETERS.md §3 … — fixed this session (formats/parameters phase)" — `git log -- docs/PARAMETERS.md` proves no commit after `41a2c56` (2026-10-26) touched the file; the fix never landed.
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered — the SCAN-2f session's completion claim was false; the rows remain stale at every tree since (verified at `35adcb9` and re-verified on the post-`9e5dedc` rebased tree).

## Summary

The product's engine-constants doc row set still documents the pre-S-039/pre-M-025 values: the carve window as "4–8 MiB" (code: 1 MiB + 4 KiB overlap) and the user-sign caps as "8 signs / 16 bytes / offset ≤ 512" (code: 12 signs / 48 bytes / offset ≤ 1024 / mime 80). The audit trail's own SCAN-ENGINE-DELTA claims these rows were "fixed this session" — git history proves they were not (the file is untouched since 2026-10-26). An integrator reading docs/PARAMETERS.md today builds against constants that are wrong by 2-4× on five values.

## What the reference does

The audit's corrected record (S-039's resolution table, the S-007/S-037/M-025 lineage) pins the true constants — the corpus-measured caps (with headroom, atomic-parse proven on the og's own DB) and the landed window model. The docs row is supposed to mirror the code.

## What our implementation does

`docs/PARAMETERS.md:78-79` carry the stale values (quoted in Core references); the code constants are unambiguous and triple-verified (code + findings + tests agree); only the doc row disagrees. The rawsigns.h "30 formats" comment is the same class (stale by batch-1's 26 new validators).

## Gap

Falsifiable: read `docs/PARAMETERS.md:78-79`, read `cr_carve.cpp:35-36` and `cr_usersigns.h:40-47`, observe five value mismatches; read `SCAN-ENGINE-DELTA.md:204-207`, run `git log -- docs/PARAMETERS.md`, observe the "fixed" claim has no corresponding commit.

## Impact

Integrator-facing doc drift (Q68): the caps row understates the actual limits (an integrator truncating signs to 16 bytes / 8 entries loses legal input the core accepts; the window row misdescribes the read model). The false completion claim in the audit trail misleads future sessions into believing the row set is current.

## Reproduction / verification

First-hand this session: the doc rows read; the code constants read (quoted); git log on the file (last touch `41a2c56`, pre-dating the SCAN-2f session by two days); the SCAN-ENGINE-DELTA claim read. Re-verified on the final rebased tree.

## Fix plan (for the engineering team)

1. `docs/PARAMETERS.md:78`: "| Carve window | 1 MiB sequential windows + 4 KiB overlap (M-025/D-101 stateless validator architecture) | cr_carve.h |".
2. `docs/PARAMETERS.md:79`: "| Raw-sign table | 139 formats + user signs (caps: 256 entries / 12 signs / 48 bytes / offset ≤ 1024 / mime ≤ 80, M-048+S-039/M-067) | cr_rawsigns.h, cr_usersigns.h |".
3. `include/cr/carve/cr_rawsigns.h:7`: refresh the validator count comment to the current FMT roster (or drop the count from the comment — the enum is self-describing).
4. Audit-trail honesty (this repo's writable surface): annotate SCAN-ENGINE-DELTA.md:204-207 with the correction — the claim was false, N-108 owns the product-side fix.
5. Acceptance: a docs-vs-code spot-check grep (the row's numbers appear in cr_usersigns.h/cr_carve.cpp).

## Tests to add

- `DocsParameterRows-Match-Code-Constants` (optional cheap grep gate)

## Related findings

- **S-039/M-067/M-025** (the landed constants the docs lag)
- **N-095/N-104** (the other doc-truth clusters — different sites; this file owns the §3 constants rows)

## Open questions

None.
