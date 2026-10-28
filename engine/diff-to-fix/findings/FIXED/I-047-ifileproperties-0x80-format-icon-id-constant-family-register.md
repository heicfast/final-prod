# INFORMATIONAL I-047 — The IFileProperties +0x80 per-class format/icon-ID constant family (NTFS 0x2FF / FAT 0xFF / RAW 0x1CF): no core carrier, og consumption undecoded — the register (acknowledged at filing)

**Severity:** INFORMATIONAL
**Module:** tree / query (the display-datum family); the ifileproperties-part1 census
**Parameters affected:** D11 (node fields — a display datum the core does not carry), Q46 (contract documentation — the census register)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md` §1 slot +0x80 (the per-class constant family, [CODE]-backed); DRWUI-side first-hand: `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` `FUN_1405209f0 { return 0x2ff; }` (:853378-853381) and `FUN_140c41350 { return 0x1cf; }` (:1875864-1875873) — the statically linked class vtables' +0x80 slots (read first-hand this session)
**Core references:** audited `34d6cf10` — no carrier: `rg "0x2FF|0x1CF" src/` -> 0 hits; the core's display axis is the FileFilter extension category (`src/core/query/cr_query.cpp:289-293`, the M-029 derivation; `include/cr/cr_ui.h:1037`)
**Status:** ACKNOWLEDGED AT FILING (the INFO-tier convention) — the register, not a code change

## Summary

The og's IFileProperties vtable carries a +0x80 slot returning per-class
constants (NTFS 0x2FF, FAT 0xFF, RAW 0x1CF — a format/icon-ID family, per
the part1 artifact). The DRWUI-side copies of the class vtables return the
constants verbatim (decoded first-hand). The core has no equivalent
carrier — its display/type axis is the FileFilter extension category
deliberately (M-029's derivation). The og's exact CONSUMPTION of the
constant remains undecoded (part1 does not decode the consumer; the +0x80
call sites found in DRWUI return bytes — other vtables), so per the
no-hallucination rule the disposition is this register with a precise open
question, not a divergence finding.

## What the reference does

The part1 artifact documents slot +0x80 as a per-class constant getter
([CODE]-backed): 0x2FF for the NTFS class, 0xFF for the FAT class, 0x1CF
for the RAW class. The DRWUI decompile confirms two of the three constants
first-hand (FUN_1405209f0 = 0x2ff; FUN_140c41350 = 0x1cf). The consumer
that reads this slot (format/icon selection in the UI layer?) is not
decoded in any part1-3 artifact, and my +0x80 caller grep in DRWUI found
only byte-returning call sites (other vtables).

## What our core does

No carrier (grep-verified). The user-visible Type axis is derived from the
extension via the FileFilter table (M-029), and the trail's icon-adjacent
rows are D-31 (asset licensing, an explicit scope exclusion).

## Gap

A datum the og's props interface exposes that the core's does not — with
the consumption undecoded, the parity verdict is honestly open, and the
core's own display axis (M-029) is the registered deliberate equivalent.

## Impact

None owed now: the merged product's UI is the host's, and the core's Type
derivation covers the results-tree semantics the slot plausibly feeds. The
register exists so a future UI-parity session knows the constant family is
decoded on the og side and unclaimed on the core side.

## Reproduction / verification

Decompiled + census evidence class, first-hand this session:
`evidence/s50-ifileproperties-part1-census.md` (the +0x80 row, the two NEW
items section), the DRWUI function quotes above.

## Fix plan (for the engineering team)

None owed (register). If a future UI-parity session decodes the slot's
consumer and it feeds a non-UI-pure behavior (e.g. the rsf round-trip or
the query layer), re-open as a proper finding with the consumer cite.

## Tests to add

None (register).

## Related findings

M-029 (the Type-derivation landed — the deliberate equivalent), D-31 (the
icon-family scope exclusion), the part1 census (N-192's sibling register).

## Open questions

The og's consumer of the +0x80 constant: part1 leaves it undecoded; the
precise probe is a DRWUI-side xref of FUN_1405209f0's vtable-slot callers
(the IFileProperties-family vtables in the DRWUI static copies — the
byte-typed +0x80 call sites found are other classes). Low priority; no
core behavior depends on it.

## Resolution (acknowledged at filing, S50 lane)

Registered as the deliberate-equivalent posture (the M-029 derivation; the
undecoded og consumption recorded as the open question above). No code
change. The ledger row + this file are the record; the census evidence
sheet carries the decode.
