# MINOR N-095 — Audit-trail and docs doc-truth batch: five stale texts carrying superseded readings

**Severity:** MINOR
**Module:** docs / audit-trail (transport, carve, driver docs, parity docs, ABI contract)
**Parameters affected:** Q68 (documentation accuracy), C19-adjacent (the IOCTL label), D14-adjacent (the errata propagation)
**Corpus references:** the corrections each item's source supersedes: `re-workspace/artifacts/CORRECTIONS.md` #5 (RawConfig 34 toggles; FileFilter 1,481 unique of 1,489); `diff-to-fix/CORPUS-ERRATA.md` E-A/E-B + D-104 (the GetRawUserInfoMap reading); I-011 #8 (0x60002400 = GetProcName); `Device_decompiled.c:7793/50628` ("\\\\.\\EPRDTDRV\\%d\\%d" — the 2-segment disk template); `diff-to-fix/evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md:34-36`
**Core references:** audited `35adcb9`. (1) `docs/REFERENCE-DRIVER-BEHAVIOR.md:92` + `:329` still label `0x60002400` "set/resolve global target", citing the artifact I-011 #8 superseded. (2) `docs/parity/PARITY-CORE-REC.md:125` "37 extension toggles" (corrected value: 34); `docs/parity/PARITY-ARCH.md:54,141,223-225` + `PARITY-UI.md:205` "1,489 extensions" without the 1,481-unique nuance. (3) `diff-to-fix/modules/carve.md:546-548` still carries OQ-SA-3's superseded "GetRawUserInfoMap … capability-configuring, MAJOR stands" text with no D-104 annotation. (4) `diff-to-fix/modules/transport.md:6` quotes the og's disk grammar as 3-segment `\\.\EPRDTDRV\<pid>\<disk>\<part>` vs the decompile's 2-segment template. (5) `docs/ABI-CONTRACT.md` §7f cites `cr_ui.h:823-865` and §7g `867-886` — the blocks are now at 853-895 / 897-916 after W1c's appends.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Five doc-truth defects where a corrected reading exists elsewhere in the trail but the text was never updated: two driver-doc labels superseded by corpus errata, three parity-doc count errors (the CORRECTIONS #5 tail), one module file carrying a refuted verdict without the supersession note, one grammar misquote against the decompile, and a block of stale line cites in ABI-CONTRACT. Each is verified against its own trail evidence.

## What the reference does

The corrections are already on file (the corpus CORRECTIONS entries, the trail's CORPUS-ERRATA rows, I-011, and the s4 evidence file) — this finding is purely about propagation.

## What our core does

The five stale texts persist (cites above).

## Gap

Each named site contradicts the trail's own corrected record. Falsifiable per site: read the site, read the correction, observe the mismatch.

## Impact

Future sessions and engineers reading these docs inherit refuted readings (the I-011 #8 case caused a real mislabel in its day; the carve.md case still reads as if a MAJOR finding stands that D-104 superseded).

## Reproduction / verification

All five verified first-hand this session (each site read; each correction's source read; the ABI cite drift confirmed by reading the actual cr_ui.h block boundaries).

## Fix plan (for the engineering team)

1. `docs/REFERENCE-DRIVER-BEHAVIOR.md:92/:329`: relabel 0x60002400 to GetProcName (I-011 #8's reading; cite the corrected artifact).
2. `docs/parity/PARITY-CORE-REC.md:125`: 37→34; add the 1,489/1,481 elements-vs-unique nuance to the PARITY-ARCH/PARITY-UI sites.
3. `diff-to-fix/modules/carve.md:546-548`: append the D-104 supersession note (the OQ-SA-3 correction — the map is per-scan statistics, the "MAJOR stands" verdict is superseded).
4. `diff-to-fix/modules/transport.md:6`: quote the og's 2-segment disk template per the decompile (the 3-segment form is the volume grammar / the RDA side's composition).
5. `docs/ABI-CONTRACT.md` §7f/§7g: re-pin the cr_ui.h ranges (853-895 / 897-916).

## Tests to add

- None (documentation). A `docs-cites-fresh` lint pass would prevent class (5) — optional.

## Related findings

- N-094 (the same class; separate file for its lock-rule implication).
- I-011/OQ-SA-3/D-104/CORRECTIONS#5 (the correction lineages).

## Open questions

None.
