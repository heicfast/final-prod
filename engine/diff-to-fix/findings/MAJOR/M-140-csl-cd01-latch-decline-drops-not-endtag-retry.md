# MAJOR M-140 — The CSL/CD01 walker: the og's per-candidate latch permanently binds CD01 candidates to the 0x44c sizing (declined reads drop the candidate); the core's fall-through carves them via the end-tag walk — the M-112 landing's "retries other sizing" claim is contradicted by the decompile

**Severity:** MAJOR
**Module:** carve (the Brance::SVGAnalyser CD 01 00 00 leg — FMT_SVG_CD01 / the M-112 size branch)
**Parameters affected:** A1 (detection correctness — the CD01 population's acceptance set), B1 (false-positive rate — the core carves files the og drops), A25 (the walker's control flow), B28 (carving validator logic — the accept/reject criteria)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` FUN_180041950 :49749-49828 (the walker with the +0x120 latch — read first-hand this session), FUN_180041880 :17291-17306 (the SVGRawFileInfo ctor: `puVar1[0x23] = 0x100000` = the +0x118 cap field, the id `*(undefined4 *)(puVar1 + 3) = 0x5b` at +0x18, the size slot +0x10 left unset), FUN_180041b40 (the end-tag branch the latch bypasses)
**Core references:** audited `34d6cf10` (HEAD at filing) — `src/core/carve/cr_analysers.cpp` the M-112 block :3777-3840 (the 0x44c branch at :3817-3827 parity-verified; the decline fall-through to `scan_tag_two_case(s, hit, limit, "</svg>")` at :3830-3838 and the bounded fallback at :3839-3840; the landed claim at :3814-3816 "Declination ... falls to the end-tag walk exactly as the og's pending model retries other sizing"); landed by 73a134eb (M-112)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The M-112 landing ported the og's CD01 size branch (dword@0x44c + 0x480,
the 1 MiB cap) correctly, but added a fall-through the og does not have:
when the 0x44c read declines (the file too short to span 0x44c, the size
out of cap, or a zero result), the core retries sizing via the `</svg>`
end-tag walk and then a bounded fallback — and carves the candidate. In
the og, the first walker window that starts with the CD01 key sets a
**per-candidate latch** (+0x120) that permanently routes every subsequent
call to the 0x44c branch; a declined 0x44c read never assigns the size
slot, and the unsized candidate is dropped. The core therefore carves a
CD01-key population the og refuses — a false-positive divergence — and the
landed comment mis-states the og's control flow.

## What the reference does

FUN_180041950 (full quotes in
`evidence/decompiled-quotes/s50-landing-divergences.md` §3):

```c
if (param_2 == (int *)0x0) goto EXIT;
if (*(int *)(param_1 + 0x120) == 0) {              /* the per-candidate LATCH */
    if (*param_2 == DAT_180093918) {               /* window starts CD 01 00 00 */
        *(undefined4 *)(param_1 + 0x120) = 1;       /* latch ON - permanent     */
        goto LAB_1800419b2;                        /* -> the 0x44c branch, NOW */
    }
    /* ... FUN_180041b40 = the </svg> end-tag branch, then the
           +0x110/+0x118 size assignment ... */
}
else {
LAB_1800419b2:                                     /* the 0x44c branch */
    uVar4 = 0;
    ...
    if ((uVar7 < 0x44c) && (0x44c < (ulonglong)((lVar5 + 1) * lVar6))) {
        ...
        uVar4 = (ulonglong)(*(int *)(lVar3 + (longlong)param_2) + 0x480);
        if ((lVar6 < lVar5 + 0x450) ||
            (*(longlong *)(param_1 + 0x118) < (longlong)uVar4))  /* the 1 MiB cap */
            goto EXIT;                              /* declined - NO size, NO retry */
    }
    if (uVar4 == 0) goto EXIT;                      /* declined - NO size, NO retry */
    *(ulonglong *)(param_1 + 0x10) = uVar4;         /* size = dword + 0x480 */
}
EXIT: return;
```

The og's block scan is sector-unit, so a real CD01 candidate's first walker
window starts at the candidate and the latch fires on the first call —
from then on the end-tag branch is dead code for that candidate. A declined
0x44c read (file < 0x450 bytes readable at the window, dword+0x480 > 1 MiB,
or zero) leaves the size slot (+0x10) unset on every call, and the ctor
(FUN_180041880) initializes only the cap (+0x118 = 0x100000) and the id
(+0x18 = 0x5b) — the pending model drops the unsized candidate.

## What our core does

The M-112 block at cr_analysers.cpp:3777-3840 (audited tree): the 0x44c
branch itself is parity (the :3817-3827 read + cap + the FMT_SVG stamp) —
but on decline the code falls through:

```cpp
    }                                              /* <- the 0x44c decline lands here */
const u64 limit = probe_limit(s, hit, cap);
const u64 marker_end = scan_tag_two_case(s, hit, limit, "</svg>");   /* the retry */
...
if (marker_end != 0) { r.ok = true; r.end = marker_end; ... return r; }
r.ok = true; r.end = limit; r.confidence = 500;      /* the bounded fallback */
```

The landed comment (:3814-3816): "Declination (short file / out-of-cap /
zero result) falls to the end-tag walk exactly as the og's pending model
retries other sizing." The decompile shows the og has no such retry for
latched (i.e. all real, sector-aligned) CD01 candidates.

## Gap

Falsifiable: on a CD01-key candidate whose 0x44c read declines — e.g. a
`CD 01 00 00`-led file shorter than 0x450 bytes containing `</svg>`, or one
whose dword@0x44c + 0x480 exceeds the 1 MiB cap — the og assigns no size and
drops the candidate; the core carves it (end-tag extent, or the 1 MiB
fallback at confidence 500).

## Impact

False-positive-rate divergence on the CD01-key population: the core's
results contain files the og refuses (extra rows in the SVG category with
`svg-endtag-walk`/`svg-bounded-cap` provenance notes). The direction is
one-sided (the core never drops an og-carved CD01 file — the 0x44c
success path is parity), so recovery correctness is not lost; but the
acceptance set diverges, and the mis-stated comment teaches the next
engineer a control flow the reference does not have. MAJOR per the
false-positive axis (users observe different result sets), consistent with
the M-081 attribution precedent (classification-level divergence, content
largely real).

## Reproduction / verification

Decompiled-evidence class, both sides read first-hand this session:
`evidence/decompiled-quotes/s50-landing-divergences.md` §3 (the walker
quotes with line numbers, the ctor, the latch's permanence); the sweep
record `evidence/executions/s50-landing-parity-sweep.md` row 10 (the
DIVERGENCE verdict on the M-112 landing — the branch itself PARITY, the
fall-through the divergence).

## Fix plan (for the engineering team)

1. Architectural: mirror the og's two-tier control flow inside the analyser
   — the CD01-key candidates size via the 0x44c branch ONLY; the
   end-tag/bounded paths serve the non-CD01 SVG legs (`<svg`, the
   "csl"-content leg) exactly as today. No new state machine needed: the
   candidate's first byte dword (0x000001CD) IS the latch condition (the
   core is stateless per call, so "latched" = "the window starts with the
   key", which the existing `u32_from_bytes_le(win) == 0x000001cdu` test
   already selects).
2. Files: `src/core/carve/cr_analysers.cpp` (the M-112 block), the block
   comment.
3. Algorithm: inside the `if (n >= 4 && u32_from_bytes_le(win) == 0x000001cdu)`
   branch, on decline return the not-ok result (r.ok stays false; no
   fall-through to the end-tag walk). Re-truth the comment: "Declination
   drops the candidate — the og's +0x120 latch permanently binds CD01
   candidates to the 0x44c sizing (FUN_180041950 :49766-49771); the
   end-tag walk serves the OTHER SVG legs."
4. Tests (`tests/unit/test_carve.cpp`, the CSL/M-112 family):
   - `Carve.CslShortCd01DeclinedIsDropped` — a CD01-led file of 0x200 bytes
     containing `</svg>`: expect NOT carved, RED before.
   - `Carve.CslCd01OutOfCapDeclinedIsDropped` — a CD01-led fixture whose
     dword@0x44c+0x480 > 1 MiB with `</svg>` present: expect NOT carved,
     RED before.
   - `Carve.SvgPlainLegStillWalksEndTag` — the `<svg `-led leg keeps the
     end-tag walk (guards against over-tightening).
   - keep the existing M-112 tests (the success path) green.
5. Contract/doc: the carve.md module row + the FIXED/M-112 Resolution gain
   the erratum note (the "retries other sizing" claim refuted).
6. Acceptance: the three new tests green; the CD01 success-path tests
   unchanged-green; the plain-SVG legs unchanged.

## Tests to add

The three named tests above; the first two RED before by construction.

## Related findings

M-112 (FIXED — the landing this rode), M-093 (the FMT_SVG_CD01 row's own
lineage), S-049 (the naming contract — the dropped candidates also never
get names, so the FP is confined to results that should not exist).

## Open questions

None — the latch decode is unambiguous (the `goto LAB_1800419b2` makes the
control flow explicit in the decompile).
