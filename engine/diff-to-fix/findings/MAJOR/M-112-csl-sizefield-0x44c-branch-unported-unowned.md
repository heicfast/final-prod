# MAJOR M-112 — the CSL size branch (dword@0x44c + 0x480) is decoded-but-unported and carries NO active board owner: 'csl'-marked XML files are sized by the generic </svg> walk / 1 MiB cap instead of the og's own size field read

**Severity:** MAJOR
**Module:** carve (the XML/SVG chain's CSL leg) — modules/carve.md
**Parameters affected:** A27 (fragment handling — the carve extent for a whole format family), A28 (carving validator logic — the size-assignment branch), A47-adjacent (throughput — the 1 MiB cap truncates files the og sizes to their true extent)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:49749-49828` (FUN_180041950, the SVG/CSL size walker — first-hand re-read this session): the `LAB_1800419b2` branch computes `uVar4 = (ulonglong)(*(int *)(lVar3 + param_2) + 0x480)` where `lVar3` is the buffer offset of file-offset **0x44c** (the gate: `if ((uVar7 < 0x44c) && (0x44c < (lVar5+1)*lVar6))` — the chunk must span 0x44c; the dword read requires `lVar6 >= lVar5+0x450` so the full dword is in-buffer; the result must not exceed the +0x118 cap field, else the branch declines); the size is assigned verbatim at `*(ulonglong *)(param_1 + 0x10) = uVar4`. The probe-side 'csl' marker (DAT_180093968, the Corel symbol-library string that routes generic-XML id 0x13) is decoded in the landed M-093 chain.
**Core references:** audited `743d7b8d` — `src/core/carve/cr_analysers.cpp:2886-2889` (the validator's own comment: "Residual (recorded, the M-106 stand-in discipline): the walker's 0x44c branch (the CSL size = dword@0x44c + 0x480 for files long enough to span it) is decoded only partially and stays unported"); the `validate_xmlsvg` body (`:2890+`): the size assignment is `r.end = marker_end` (the `</svg>` two-case walk) or the bounded fallback `r.end = limit` (the 1 MiB cap) — the only `0x44c`/`0x480` occurrences in the entire core are the comment lines themselves (`grep -rn "0x44c|0x480" src/ include/ tests/` → `:2887-2888` only, verified this session).
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (session-11B push-time reconciliation):** filed as M-111 by this lane; renumbered **M-112** — the parallel S11 lane's M-111 (the Explorer-partition-browse capability finding) pushed first and keeps the ID per the SA-9 rule. All cross-references updated (this file's Related-findings section references M-107 by ID, unaffected).
**Prior audit check:** the M-093 landing (ca701711) recorded the residual in its (now retired) Resolution and in the code comment; **no active finding owns it**: M-107 enumerates batch-1's eleven stand-in sites only (X3F/CRW/WPG/WPD/PSP/SIT/APE/EPS/AI/DXF/DWG/SWF — verified: zero mention of 0x44c/CSL/'csl' in M-107's file), M-106 is ONE/3DS only, S-049 is the naming contract, M-092/M-093 are retired. This filing is the M-107 class's extension to the M-093 landing's own residual — the "recorded outside an active finding, no board owner" defect that class names.

## Summary

The og's SVG/CSL size walker reads a size field at file offset 0x44c (dword + 0x480) for files long enough to span that offset, and uses it as the carve extent. Our core decoded this branch (the comment says so) but never ported it: every 'csl'-marked (Corel symbol-library) XML file is instead sized by the generic `</svg>` end-tag walk or truncated at the 1 MiB family cap. The residual is recorded only in a code comment and the retired M-093 Resolution — no active finding owns it, which is exactly the unowned-stand-in defect class M-107 established for batch-1.

## What the reference does

FUN_180041950 (`RAWSearchDLL_decompiled.c:49749`) is the per-chunk callback that finalizes sizes for the SVG/CSL family. When a chunk spans file offset 0x44c, it reads the little-endian dword at 0x44c, adds 0x480, validates the result against the cap field (the analyser's +0x118 max), and assigns it verbatim as the file size (`*(param_1 + 0x10) = dword@0x44c + 0x480`). Files shorter than 0x44c+4 (or whose computed size exceeds the cap) decline this branch and fall to the walker's other sizing. The og thereby carves Corel symbol-library files to their own recorded extent — no end-tag scan needed for this family.

## What our core does

`validate_xmlsvg` (cr_analysers.cpp:2890+) probes the four u32 keys, walks `</svg>` two-case up to the 1 MiB cap, and falls back to the bounded cap when no marker is found. For 'csl'-routed files (the generic-XML id 0x13 leg) the og's dedicated size read has no counterpart: the comment at :2886-2889 honestly records the residual under "the M-106 stand-in discipline", but M-106 is the ONE/3DS finding and M-107's enumeration does not include this site — the stand-in is orphaned on the active board.

## Gap

Falsifiable: carve a file whose first bytes carry the `<?xml` declaration gate and the 'csl' marker (routing it to the generic-XML id), whose dword@0x44c + 0x480 is between 1 MiB and the family cap (e.g. dword = 0x100000 → size 0x100480 ≈ 1.06 MiB), and whose body contains no `</svg>`. The og carves it to 0x100480 bytes; our core truncates at the 1 MiB cap (`r.end = limit`, the bounded fallback) — or, if a `</svg>` appears earlier, at the marker. The size field the og reads at 0x44c is never consulted.

## Impact

Every carved Corel symbol-library ('csl'-family) XML file longer than the 1 MiB cap (or whose true extent differs from the first `</svg>` marker) is recovered truncated or mis-sized — the A-027/A-028 carve-extent class. The frequency is lower than the mainstream formats (M-093's own note: 'csl' is the minority leg of the XML chain), which holds this at MAJOR not SEVERE. Secondary impact: board truth — an unowned recorded residual is the exact defect class M-107 filed against batch-1, and leaving this one unowned repeats it.

## Reproduction / verification

Corpus-derived (protocol §6.2): FUN_180041950 read first-hand this session at RAWSearchDLL_decompiled.c:49749-49828 — the 0x44c-span gate, the +0x480 arithmetic, the cap check, and the verbatim assignment all quoted above. Core side verified by grep (only the comment cites 0x44c) and by reading the validator's size-assignment paths. The M-093 landing's own Resolution (retired file, history at `608665fb^`) records the same decode.

## Fix plan (for the engineering team)

1. No architectural change — one branch inside `validate_xmlsvg`.
2. Files: `src/core/carve/cr_analysers.cpp` (the validator), `tests/unit/test_carve.cpp`.
3. Algorithm: after the probe routes a hit to the generic-XML id (the 'csl' leg), read `dword@hit+0x44c` when `s->size() - hit >= 0x450`; if `dword + 0x480` is within the family cap (1 MiB — the registry row's max; the og's cap field is the analyser's own +0x118, which for this family is the same 0x100000 bound decoded with M-093), assign `r.end = hit + dword + 0x480` with note `csl-sizefield@0x44c` and skip the end-tag walk; otherwise fall through to the existing walk/fallback. Mirror the og's declination semantics exactly (out-of-cap → the walk, not an error).
4. Tests: (a) `Carve.CslSizeFieldExtendsPastCap` — a 'csl' file with dword@0x44c sizing it past 1 MiB (no `</svg>`) carves to the field extent, RED pre-fix (truncates at cap); (b) `Carve.CslSizeFieldDeclinesOutOfCap` — dword + 0x480 > cap falls to the bounded fallback; (c) `Carve.CslShortFileFallsToWalk` — files shorter than 0x450 never read the field; (d) the SVG leg unchanged (dword@0x44c is only read on the 'csl' route).
5. Contract/doc: `modules/carve.md` row update; the comment at :2886-2889 rewritten to point at this finding (resolved) instead of "stays unported".
6. Acceptance: the four tests green; a re-run of the M-093 fixtures shape-identical (the SVG-leg files must not change size).

## Tests to add

- `Carve.CslSizeFieldExtendsPastCap` (the RED-able core case)
- `Carve.CslSizeFieldDeclinesOutOfCap`
- `Carve.CslShortFileFallsToWalk`
- `Carve.SvgLegUnaffectedByCslBranch`

## Related findings

M-107 (the stand-in-residue class this extends — its enumeration should gain this site by addendum), M-093 (the landing that recorded the residual; retired), S-041 (the batch-2 umbrella that closed with this leg unowned), N-135 (the bookkeeping class: a recorded residual outside an active finding).

## Open questions

The og's cap-field value for this analyser (+0x118) is assumed to be the same 0x100000 the M-093 decode registered for the family bounds — the ctor site for the CSL walker's cap was not separately re-read this session. If the engineering team finds the og's cap differs, the declination boundary in test (b) adjusts accordingly; the branch logic is unaffected.
