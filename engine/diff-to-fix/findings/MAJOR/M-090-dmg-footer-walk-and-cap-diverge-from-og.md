# MAJOR M-090 — the landed DMG row diverges from the og on both the extent-walk target and the cap: the og walks the "koly" UDIF trailer (+0x200, DMG-specific bound 0xC800000) — core walks the bzip2 EOS magic (+10, global cap 0xBEBC200), truncating real DMG carves before the required footer

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-1 DMG row (modules/carve.md)
**Parameters affected:** A2-adjacent (carve extent correctness — the koly footer is cut off), A28-adjacent (the extent walk), 47-adjacent (cap semantics)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:1762-1783` (registration: key 0x31685a42 "BZh1" → Brance::DMGAnalyser/FileDMGAnalyser id 0x71); `:49410-49415` (the block-pi gate: 10-byte "BZh1"+"1AY&SY" check); `:49436-49440` (**DMGRawFileInfo ctor bounds: max = 0xC800000 = 209,715,200** — read verbatim this session); `:63806` (the FileDMGAnalyser extent walk needle **0x796c6f6b = "koly"**, the UDIF trailer signature) with the sector walk's 8-zero pre-gate (:63843-63844) and **size = hit + 0x200** (:63847); `:9049` (the GLOBAL carve cap 0xBEBC200 = 200,000,000 — a different constant the landing attributed to DMG). The "koly" string is present in the shipped og-codebase/RAWSearchDLL.dll (binary grep, offset 0x53222).
**Core references:** audited commit `ff879f9` — `src/core/carve/cr_registry.cpp:695-707` (FMT_DMG row: key BZh1, min 32, **cap 0xBEBC200** — the global constant, not DMG's own 0xC800000) and `src/core/carve/cr_analysers.cpp:2996-3031` (`validate_dmg`: block-pi gate correct; the extent walk searches the **bzip2 EOS magic {0x17,0x72,0x45,0x38,0x50,0x90} and sizes EOS+10** (:3006) — not the og's koly trailer walk). The landing commit (e018bf4) claimed "stream-footer walk, cap mirrors the og's own 0xBEBC200".
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** the S-007 batch-1 landing (e018bf4) — the DMG row's walk and cap were mis-decoded (the global singleton cap read as the DMG bound; the bzip2 EOS read as the og's footer target). No prior finding covers the divergence.

## Summary

The landed DMG validator matches the og on the keyed gate (BZh1 + block-pi) but diverges on the extent: the og's FileDMGAnalyser walks to the "koly" UDIF trailer and sizes the carve as trailer+0x200 with a DMG-specific 0xC800000 bound; core walks to the bzip2 EOS magic and sizes EOS+10 with the global 0xBEBC200 cap. For a real UDIF image the bzip2 stream ends before the koly footer, so core truncates the carve — the carved file loses the required UDIF footer (and anything after the stream) and typically will not mount. The cap is also under-attributed by ~9.7 MB.

## What the reference does

Registration :1762-1783 keys DMG on BZh1 with a 10-byte block-pi gate (:49410-49415) — core matches this exactly. The og's DMGRawFileInfo ctor (:49436-49440) carries DMG's own bounds with max 0xC800000 (209,715,200). The FileDMGAnalyser extent walk (:63782-63868) searches for the "koly" signature (0x796c6f6b at :63806) with an 8-zero pre-gate, and sizes the carve at the trailer hit + 0x200 — the standard 512-byte UDIF footer — so the carved file is byte-complete including the mandatory footer.

## What our core does

validate_dmg (cr_analysers.cpp:2996-3031) re-verifies the BZh1 gate correctly but then walks the bzip2 end-of-stream magic {0x17,0x72,0x45,0x38,0x50,0x90} and takes EOS+10 as the end (:3006); the registry row caps at 0xBEBC200 (:704). The koly trailer — the structure a UDIF consumer needs — is never searched for, so a real DMG carves truncated (stream end, footer lost). The landing's commit message records "stream-footer walk, cap mirrors the og's own 0xBEBC200": both halves of that claim are mis-decodes (the og's walk targets koly; 0xBEBC200 is the singleton cap at :9049, not DMG's ctor bound).

## Gap

Falsifiable: a synthetic UDIF image = [BZh1 bzip2 stream][padding][koly trailer at offset K] carves in the og with size K+0x200; core carves with size = (bzip2 EOS)+10 < K — the koly trailer and the trailing region are cut off, and the DMG-specific cap 0xC800000 (209,715,200) admits candidates in (200 MB, 209.7 MB] that core's row refuses.

## Impact

Recovery correctness: carved DMGs are truncated before the mandatory UDIF footer — macOS will typically refuse to mount them, so the recovered artifact is unusable despite being "recovered". Candidates in the 200-209.7 MB window are dropped outright. Audit-trail truth: the batch-1 commit's verification claims ("stream-footer walk", "cap mirrors the og's own 0xBEBC200") are both factually wrong against the decompile.

## Reproduction / verification

Decompile-direct: DMG's ctor bound (:49436-49440), the koly needle (:63806), the +0x200 size (:63847), and the global singleton cap (:9049) were each read this session and are quoted in the corpus field; the "koly" constant cross-checks against the shipped binary (og-codebase/RAWSearchDLL.dll at 0x53222). Core side read at cr_registry.cpp:695-707 and cr_analysers.cpp:2996-3031. Fix verification: synthesize a BZh1+koly fixture and require size == koly_offset + 0x200.

## Fix plan (for the engineering team)

1. Change validate_dmg's extent walk to target the "koly" trailer (needle 0x796c6f6b with the og's 8-zero pre-gate), size = hit + 0x200, keeping the bzip2 EOS walk only as a fallback when no koly is found within the cap (record the fallback as a heuristic note, the batch-1 convention). 2. Raise the FMT_DMG row cap to the og's DMG-specific 0xC800000 (cr_registry.cpp:704). 3. Fix the comment and the S-007 FIXED addendum (the "cap mirrors the og's own 0xBEBC200" sentence is wrong — that constant is the global carve cap).

## Tests to add

- `Carve.DmgKolyFooterExtent` — a BZh1 stream + koly trailer at K carves with size K+0x200; RED pre-fix (EOS+10 truncation), GREEN post-fix.
- `Carve.DmgCapAtDmgSpecificBound` — a 205 MB candidate within cap carves (was dropped at the 200 MB global cap).
- `Carve.DmgNoKolyFallsBackToEos` — footer-less bzip2 stream still carves bounded (the fallback, with the heuristic note).
- `Carve.DmgBlockPiGateUnchanged` — regression guard on the keyed gate.

## Related findings

S-007 (batch-1 parent; addendum needs the walk/cap erratum); S-042/M-081/M-091 (the other session-8 batch-1 verification failures).

## Open questions

None material — the DAT_180093910 search constant used by the Brance twin walk (FUN_180041440 :49522-49553) is presumed "koly" by structural twin with FileDMG's explicit needle; if a follow-up wants belt-and-braces, a .data hexdump of that dword settles it (recorded, not blocking).
