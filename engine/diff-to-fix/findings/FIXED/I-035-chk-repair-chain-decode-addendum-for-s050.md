# INFORMATIONAL I-035 — the CHK repair chain decode addendum: rename-to-identified-ext semantics, the hex-prefix adapter registry, the 36-KiB read buffer, and the "tif" exclusion — enrichment for S-050's evidence

**Severity:** INFORMATIONAL
**Module:** repair (CHK chain — S-050's scope)
**Parameters affected:** documentation accuracy (68); S-050's fix-plan inputs
**Corpus references:** `DRWUI_decompiled.c:726699-706` (the rename formats `"%s.%s"` / `"%s_%s.%s"` — the .chk fragment renamed to its identified extension), `:1008918-1009407` (the adapter registry keyed by hex prefix: "47"=TS, "4D5A"=PE, "504B0304"=Office-zip, "D0CF11E0A1B11AE1"=Office-OLE, TIFF keys), `:772543-553` (the 36-KiB match read buffer / 2-MiB cap), `:726583` (the explicit "tif" exclusion)
**Core references:** the chain's absence is owned by S-050 (SEVERE, active — its corpus references already name CHKRepairFileSign/CORRECTIONS #9); audited at core `8d0d88b8`
**Status:** READ-ONLY REGISTER — no fix applied

## Summary

This session's local-DB lane decoded the og's ACTUAL EUUserData consumer (the CHK repair
chain) to a new depth. The findings belong to S-050's open scope; this register records
the new decode detail so the S-050 fix plan can consume it without re-decoding:
identified fragments are RENAMED to their detected extension (two format strings), the
repair dispatch is keyed by a five-entry hex-prefix adapter registry, matching reads a
36-KiB buffer (2-MiB cap), and "tif" is explicitly excluded from the rename flow.

## What the reference does

The four decode elements above, each cited to the decompile (lane C first-hand; the
lead auditor re-read the adapter-registry region).

## What our core does`

S-050's gap stands (no repair line). Nothing to compare beyond what S-050 records.

## Gap

None new — evidence enrichment only.

## Impact

S-050's fix plan gains precise constants (buffer sizes, adapter keys, rename formats)
— no additional user-visible divergence.

## Reproduction / verification

All cites read first-hand this session (lane C sheet: evidence/s12-lane-sheets/lane-C-localdb.md).

## Fix plan

1. Fold these constants into S-050's finding file as an addendum when the repair-line
   decision lands (doc-side; S-050's owner).
2. No code action.

## Tests to add

- None (S-050's plan owns the test set).

## Related findings

- S-050 (the owner)
- LC-3/LC-5 → M-128/N-168 (the parser quirks that apply to this chain's identify flow)

## Open questions

None.

## Resolution

**Status:** FIXED (ACKNOWLEDGED — decode folded into S-050 as the fix plan directs)
**Fixed in commit:** (this session's I-035 landing, S48 INFO-first lane — SHA recorded in the worklog entry)
**Verified by:** all four corpus cites re-read first-hand this session: DRWUI:726699-706 (both rename format strings `"%s.%s"` / `"%s_%s.%s"` confirmed in the 0x1000 stack buffer), :1008918-1009407 (the hex-prefix adapter registry — CHKOffice504B0304 keyed "504B0304" [8 chars] + CHKOfficeD0CF11E0A1B11AE1 keyed "D0CF11E0A1B11AE1" [0x10 chars] confirmed on-disk), :772543-553 (the 0x9000/0x200000 buffer constants in the FUN_1404f8e60 call), :726583 (`_stricmp(_Str1,"tif")` — the explicit TIFF exclusion). The fold-in landed as a dated addendum in S-050's finding file — `grep -c "I-035 fold-in" findings/SEVERE/S-050-file-repair-line-absent-and-unregistered.md` = 1 post-landing.
**Resolution notes:** The register's fix plan executed: the four decode constants (the two rename format strings + the 4 KiB rename buffer, the five-entry hex-prefix adapter registry, the 36-KiB/2-MiB match read buffer, the "tif" exclusion) folded into S-050's finding file as a session-48 addendum feeding the step-3 eligibility-seam contract, so the S-050 owner (the SEVERE lane) consumes them without re-decoding. No code action per the finding's own verdict; S-050 remains ACTIVE on its own file and owns the repair-line decision. The related parser quirks ride M-128/N-168 (both open).
