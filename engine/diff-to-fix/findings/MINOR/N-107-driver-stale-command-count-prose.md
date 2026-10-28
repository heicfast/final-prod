# MINOR N-107 — Driver-repo command-count prose is stale at 14 current-tense sites: "15/18 commands" where the code carries 19

**Severity:** MINOR
**Module / Surface:** Driver wire contract documentation (rda-driver repo docs + source comments)
**Parameters affected:** C22 (IOCTL codes and ABI numbers — the documented count of the session-code set), Q68 (documentation accuracy)
**Corpus references:** `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md:52,61-75` (the corpus table lists the prior 15 codes — the historical baseline the prose froze at); the G28 widening documented driver-side `docs/audit/2026-10-22-decompile-reverification.md:190-192`
**Core references:** audited driver `323e75a`: the authoritative count — 19 `#define`s `src/shared/inc/rda_ioctl.h:183-201`, 19 `case` labels in the recognition switch `src/driver/rda_ioctl.c:47-66`, 19 values in `scripts/verify/verify_reference_binaries.py:214-232`; **zero sites in the driver repo state 19**. The 14 stale sites: `README.md:31` ("the 15-command copy-session protocol") and `:42` ("15-command session engine (`0x800C2AC8 … 0x800C2B38`)"); `docs/architecture.md:19` and `:72`; `docs/ground-truth-contracts.md:194` + the §5.2 command table `:202-219` (15 rows only — discovery + RRWR rows absent); `docs/requirements.md:56`; `src/shared/inc/rda_ioctl.h:11` ("15-command") and `:154` ("decoded the FULL command set (18 codes)" — its own table at `:159-177` lists 19); `src/driver/inc/rda_p.h:13`; `src/driver/rda_registry.c:14`; `scripts/verify/verify_reference_binaries.py:30` ("all 18 command codes" — the code checks 19); `tests/unit/test_compat.c:229` ("the full 18-command set" — its CHECKs `:230-243` cover 19); `src/driver/rda_ioctl.c:6-7` (the routing comment's range "0x800C2AC8..0x800C2B38" — the switch spans 0x800C2AC0..0x800C2B42). The CORE repo already states 19 correctly (`docs/DRIVER-CONTRACT.md:131,:202`).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** "expanded on prior finding" the D3 handoff — the core trail recorded "seven driver-repo sites"; this session's sweep finds **14 current-tense sites** (+2 historical worklog lines). This filing owns the full inventory.

## Summary

The driver's session-code set has carried 19 codes since the G28 widening (discovery + the RRWR family), and the raw-binary probe confirms all 19 dwords in the reference EUDCPEPM.sys — but the driver repo's own docs and comments still say "15 commands" (9 sites) or "18 codes" (3 sites), a stale routing-comment range (1), and a 15-row command table missing the discovery/RRWR rows. The core repo's DRIVER-CONTRACT.md states the truth; the driver repo's own ground-truth doc is the one that's wrong.

## What the reference does

The corpus table (eudcpepm--hook-drivers.md:52,61-75) documents the 15-code baseline the prose froze at; the widening to 19 (G28) is documented in the driver repo's own decompile-reverification doc. The count is a wire-contract parameter (C22) — an integrator reading ground-truth-contracts.md §5.2 would implement 15 codes and miss the discovery + RRWR families.

## What our implementation does

The 14 stale sites listed in Core references persist (each read first-hand this session; the exact stale text captured). Note the asymmetry: several sites' own adjacent content is correct — `rda_ioctl.h:154` says "18" directly above a 19-row table; `test_compat.c:229` says "18" above CHECKs covering 19; `verify_reference_binaries.py:30` says "18" while the code checks 19 values.

## Gap

Falsifiable per site: read the site, count the codes the code defines/dispatches (19), observe the site stating 15 or 18 (or a range ending at 0x800C2B38).

## Impact

Doc drift on a wire-contract parameter: integrators and future sessions reading the driver repo's ground-truth docs inherit the wrong count; the §5.2 command table's missing rows (discovery + RRWR) hide live surface. No behavioral defect (the code is consistent at 19).

## Reproduction / verification

Repo-wide grep this session for the count phrases + the range; the 19-count verified three ways in source (defines, switch cases, verifier tuple) and a fourth way by the raw-binary dword probe of EUDCPEPM.sys (all 19 present, 1-2 sites each, including 0x800C2AC0 and 0x800C2B3E/B40/B42).

## Fix plan (for the engineering team)

1. One doc-sweep commit in the driver repo: all 14 sites to "19" (or the exact range `0x800C2AC0…0x800C2B42` where a range is shown); the ground-truth-contracts.md §5.2 table gains the 4 missing rows (discovery + RRWR ×3) with their codes and semantics from `rda_ioctl.h:159-177`.
2. Consider deriving the count in the docs from the header (a comment "19 = the rda_ioctl.h table" rather than a literal) — or a tiny CI grep gate asserting no "15-command|18-command|18 codes" phrase remains.
3. The two historical worklog lines stay (dated records).

## Tests to add

- `NoStaleCommandCountProse` (the optional grep gate)

## Related findings

- **M-089** (the phantom-zero probe list — the same wire-truth family; fix together)
- **M-086/M-087** (the other driver-lane findings)

## Open questions

None.
