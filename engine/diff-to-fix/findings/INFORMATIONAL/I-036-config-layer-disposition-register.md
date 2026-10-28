# INFORMATIONAL I-036 — The config-layer disposition register: the four surfaces the I-034 census missed (shellexe.ini, TreeType, themdark.ini, DRWLog.log) — all OOS-UI/product-frame; the census claim corrected

**Severity:** INFORMATIONAL
**Module:** gate docs (the config-layer registers) / cross-cutting
**Parameters affected:** P68 (documentation accuracy — the surface census), P69 (the never-dispositioned sweep)
**Corpus references:** `og-codebase/config/{shellexe.ini, themdark.ini, config.ini, version.ini}`; `og-codebase/log/DRWLog.log`; `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (:1561098-1561240 the FUN_140a09c40 TreeType gate; :338209-338290 the FUN_1401d1ef0 reader; :320128-320131 the co-gate); `re-workspace/artifacts/config-files/{filtering-priority--config-layer.md §5, config-data-reference.md:69}`; `re-workspace/inventory.json` :233/:588
**Core references:** audited `869da779`. No core surface exists for any of the four (the merged product has no Explorer integration, no Qt UI, no theme engine, no launcher log) — the register records the dispositions and the census correction.
**Status:** ACKNOWLEDGED AT FILING (the INFO contract: intentional scope exclusions, preserved for future work; retired per the purge convention with this file in history and the content LIVE in `evidence/decompiled-quotes/s42-config-layer-dispositions.md`)

## Summary

I-034's 750-entry og-file census claimed "no undispositioned product surface". Four surfaces had zero trail citations at this lane's open: `config/shellexe.ini` (the Explorer context-menu verb's 25-locale taglines), `config/version.ini`'s `TreeType` gate (the DRWUI results-view layout boolean, shipped=1), `config/themdark.ini` (the Qt theme tables), and `log/DRWLog.log` (the empty launcher channel). All four are dispositioned this session — all OOS-UI / product-frame, no engine-side parameter, no core defect.

## What the reference does

Full decode with citations in `evidence/decompiled-quotes/s42-config-layer-dispositions.md`:
shellexe.ini is the shell-extension host's locale tagline table (byte-identical install/PE copies, no in-tree consumer binary); TreeType is a two-reader view-layout gate (FUN_140a09c40's "absent-or-zero" boolean AND a context byte at +0x1c8, consumed only by Qt item-view internals; FUN_1401d1ef0's settings-default override) — the sibling of the already-dispositioned scanext4 gate; themdark.ini is 118 Qt stylesheet fragments per theme; DRWLog.log is a 0-byte channel.

## What our core does

Nothing — correctly. No Explorer integration, no Qt UI, no theme engine, no launcher log exist in the merged product's scope; the four surfaces carry no engine/session/ABI/CLI parameter.

## Gap

None (register). The I-034 census-claim correction: the "no undispositioned product surface" line was wrong by these four — the artifacts *mentioning* them were cited, but the surfaces never got disposition rows. Falsifiable: `rg 'shellexe|TreeType|themdark|DRWLog'` over the pre-S42 trail → 0 hits.

## Impact

Navigation only: the census is now truthful; future lanes will not re-derive these surfaces.

## Reproduction / verification

Executed this session: the trail-wide grep (0 hits at open), the byte reads of all four files, the decompile decode of both TreeType readers with the co-gate, the inventory sha256 cross-ref (install ≡ PE).

## Fix plan (for the engineering team)

None required (register-only). The content lives in the evidence file; the SURFACE-INVENTORY §-update records the rows.

## Tests to add

None (informational).

## Related findings

**I-034** (the census this corrects — acknowledged/retired session 40) · **I-027** (the data-asset register — the DRWLog channel row extends its log-channel census) · **I-035** (the Explorer-verb register row this sharpens with the tagline file) · **I-016** (the scanext4 sibling gate — the TreeType disposition completes the version.ini gate family)

## Open questions

None.
