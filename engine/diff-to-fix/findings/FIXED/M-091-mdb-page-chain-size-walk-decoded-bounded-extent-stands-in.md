# MAJOR M-091 — the og's MDB size is a page-chain walk (0x1000 strides, page-type word <= 0x901, cap 0x6400000) — the landed row's bounded-extent stand-in carves wrong sizes for nearly every real Jet database, and the "page-chain walk is undecoded" comment is now falsifiable

**Severity:** MAJOR
**Module / Surface:** carve / the S-007 batch-1 MDB row (modules/carve.md)
**Parameters affected:** A2-adjacent (carve extent correctness), A28-adjacent (the size-decoding walk), 65-adjacent (the recorded follow-up now actionable)
**Corpus references:** `easeus-drw-research/re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:52001-52039` (FUN_1800447c0, the MDB CalculateSize walk — **read verbatim this session**): starts at 0x1000, strides 0x1000, reads the page-type word via the device read helper, requires it nonzero and **<= 0x901** (`if ((0x901 < *puVar3) || (uVar4 = uVar4 + 0x1000, 0x63fffff < uVar4)) break;`), caps at 0x6400000, and stamps size = the walked offset; the keyed gates (:16509-16517: 00 01 00 00 + "Standard Jet DB\0" 20-byte signature, type-id 0x27=39; the "dard ACE" variant → 0x4c) and ctor bounds {0x1000, 0xBEBC200, step 0x1000} (:16538-16541) — all verified this session and matching core.
**Core references:** audited commit `ff879f9` — `src/core/carve/cr_analysers.cpp:3488-3502` (`validate_mdb`): keyed gates correct (00 01 00 00 + "Standard Jet DB\x00"), but `r.end = std::min(hit + (64 << 20), probe_limit(...))` — a 64 MiB bounded extent — with the comment "page-chain walk is undecoded — the Jet header + the og's own bounds carry the row, bounded extent stands in" (:3488-3489). The og cap is 0x6400000 = 100 MiB (not 64 MiB), and the walk is now decoded.
**Status:** FIXED (session-10 parallel lane)
**Prior audit check:** the S-007 batch-1 landing (e018bf4) registered the bounded-extent stand-in under the recorded-follow-up convention ("formats whose og size-walk is undecoded use the DWG-pattern bounded extent … the size-decoding walks are the recorded follow-ups") — the follow-up decode is this session's contribution; no prior finding carries it.

## Summary

The landed MDB row detects correctly (the Jet signature gates match the og exactly, trial-order TTF-before-MDB verified) but sizes every carve as a 64 MiB bounded extent because the og's page-chain walk was recorded as undecoded. This session decoded the walk first-hand: stride 0x1000 from the first page, page-type word must be nonzero and <= 0x901 (the Jet page-type family), cap 0x6400000. Real Access databases are typically a few MB, so the stand-in over-carves up to 64 MiB of trailing data for small files (padding the carved artifact with unrelated bytes) and under-carves beyond 64 MiB where the og allows 100 MiB. The fix is direct and the recorded follow-up now has its grammar.

## What the reference does

FUN_1800447c0 (:52001-52039) is the MDB size provider: it walks 0x1000-byte pages from offset 0x1000, reading the page-type word of each; the run ends at the first page whose word is zero or > 0x901 (the Jet b-tree/data page-type family), the accumulated offset becomes the carve size, and the walk caps at 0x63fffff+0x1000 = 0x6400000. Combined with the ctor bounds (min 0x1000, max 0xBEBC200, step 0x1000) the og sizes a healthy MDB at its true page-chain length.

## What our core does

validate_mdb (cr_analysers.cpp:3488-3502) checks the keyed signature correctly and then takes `min(hit + 64 MiB, probe_limit)` as the end — no page walk. The registry row (cr_registry.cpp:856-866) carries the og's {0x1000, 0xBEBC200} bounds faithfully; only the validator's extent leg and the 64 MiB stand-in diverge. The comment records the stand-in honestly under the batch-1 convention, but its premise ("undecoded") is now false.

## Gap

Falsifiable: a Jet database with N valid pages (each page-type word in (0, 0x901]) followed by a zero/invalid page carves in the og with size N x 0x1000; core carves min(64 MiB, probe_limit) — over-carve for small N, wrong boundary data included; a 90-page (0x5A000) database carves as 64 MiB in core vs 0x5A000 in the og.

## Impact

Recovery quality: carved .mdb files carry up to 64 MiB of unrelated trailing bytes (a corrupt tail that confuses Access and downstream parsers) or are truncated at 64 MiB where the og walks to 100 MiB. Confidence/naming are unaffected. Audit-trail currency: the "undecoded" comment invites a fix that now has its full grammar; leaving it would let the wrong sizes ship as if the og matched.

## Reproduction / verification

Decompile-direct: the walk body (:52001-52039) was read verbatim this session (quoted in the corpus field, including the 0x901 word gate and the 0x63fffff cap). Core side read at cr_analysers.cpp:3488-3502. Fix verification: synthesize a 4-page Jet image (signature + 3 typed pages + a zero page) and require size == 0x4000.

## Fix plan (for the engineering team)

1. Implement the page-chain walk in validate_mdb: from hit+0x1000, stride 0x1000, read the page-type word (u16 at the page start per the og's device-read helper usage), continue while nonzero and <= 0x901, cap the walk at 0x6400000, size = walked offset (clamped by probe_limit and the row's 0xBEBC200 max). 2. Keep the signature gates exactly as landed. 3. Replace the "undecoded" comment with the decoded grammar citation (RAWSearchDLL_decompiled.c:52001-52039) and update the S-007 FIXED addendum's MDB row.

## Tests to add

- `Carve.MdbPageChainExtent` — signature + 3 typed pages + zero page → size 0x4000; RED pre-fix (64 MiB stand-in), GREEN post-fix.
- `Carve.MdbPageTypeGate` — a page with type word 0x902 or 0 terminates the run.
- `Carve.MdbWalkCapAt100MiB` — 26,000 typed pages still bounded by the walk cap.
- `Carve.MdbSignatureGatesUnchanged` — regression guard on the keyed gates.

## Related findings

S-007 (batch-1 parent; the recorded follow-up now executed); S-042/M-081/M-090 (the other session-8 batch-1 verification findings).

## Open questions

None — the walk is fully decoded; the only un-decoded MDB tail (the og's "dard ACE" variant analyzer, id 0x4c at :16515-16517) is a separate keyed row decision (it is a distinct ACE-signature database family the og also carves — recorded here as a potential batch-3 candidate, out of this finding's scope).


## Resolution

**Status:** FIXED
**Fixed in commit:** 16475b8
**Verified by:** Carve.M091MdbPageChainWalk (pre-fix: every MDB carved as the flat 64 MiB bounded extent; post-fix: the 4-page chain carves as 0x4000, the 0x902 gate page ends the run at that page, the unbroken run walks to the probe limit); full suite 962/962 green (gcc linux-release), clang-format 18.1.8 clean.
**Resolution notes:** The og walk FUN_1800447c0 (:52001-52039, read verbatim this session before the fix): 0x1000 strides from the first page, the page-type u16 at each page start must be nonzero and <= 0x901 (the Jet b-tree/data page-type family), the run ends at the first breaking page (zero or out-of-family word), cap 0x6400000 (the 0x63fffff inner check), also bounded by the row ctor global 0xBEBC200 and the probe limit. The "page-chain walk is undecoded" comment premise is falsified and removed.
