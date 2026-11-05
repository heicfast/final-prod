# MINOR N-137 — the thumbnail ABI header contradicts its own implementation: "ERR_TRUNCATED never returned" vs the DecodeFailed→ERR_TRUNCATED map, and the inert allow_embedded knob with an unenforced "validated 0/1" claim

**Severity:** MINOR (contract-truth in a shipped header; consumer-behavior implications, no recovery-data impact)
**Module:** ABI / preview (the thumbnail surface)
**Parameters affected:** P43 (ownership/lifetimes — the contract's truthfulness), P46 (documentation of the contract), P24-adjacent (error-code mapping as documented)
**Corpus references:** `DRWUI_decompiled.c:652395-652410` (the og's 256-px GDI+ thumbnail path); `DRWUI-subsystems--assistant-grading-preview.md:46` (the setter pin); the og has no equivalent ABI header (the divergence is core-internal truth, discovered during the S11/T2 verification of the M-102/M-083 landing)
**Core references:** audited `7990a86d` — `include/cr/cr_ui.h:1272-1274` (the "never returned" claim), `:1242-1245` (the allow_embedded "validated … 0/1" claim), `:1246-1252` (the caps struct); `src/ui/cr_ui.cpp:3677-3678` (the DecodeFailed→ERR_TRUNCATED map), `:3713-3714` (the inert mapping); `docs/ABI-CONTRACT.md:607` (§7i states the truth the header denies); `tests/unit/test_cr_ui.cpp:3508` (the test pins the truth: `0x50030004u` expected on the refused decode); `src/core/preview/cr_thumbnail.cpp:1413-1415` (the engine validates edges/pixels, not the allow_embedded boolean)
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (S11-lead):** drafted as N-136, renumbered at the rebase reconciliation — the remote lanes' N-136 (the session-list cwd-dir finding, itself renumbered from N-128) pushed first and keeps the ID.

**Prior audit check:** newly discovered by the session-11 independent verification of the M-102/M-083 landing (`evidence/s11-board-truth.md` §1 residuals); not covered by M-102/M-083 (both retired PARITY-VERIFIED), N-092 (the refusal matrix — filed before the ABI existed), or M-109 (the 32 MiB cap residual).

## Summary

The thumbnail ABI surface landed by `e56baac7` is functionally correct (S11-executed verification, 40/40 checks), but its shipped header contradicts its own implementation in two places: (1) `cr_ui.h` tells C consumers that `0x50030004 ERR_TRUNCATED` is "never returned for thumbnails," while the implementation maps `DecodeFailed → ERR_TRUNCATED`, ABI-CONTRACT §7i documents that mapping, and the ABI test asserts it; (2) the caps field `allow_embedded` is documented as "validated … 0/1" but the implementation treats 0 as "default (ON)" and any nonzero as ON — there is no way to disable the embedded fast path through the ABI, and out-of-range values like 7 are silently accepted rather than refused.

## What the reference does

n/a — the og has no C ABI for thumbnails (the per-row worker is internal to DRWUI;
`DRWUI_decompiled.c:652395-652410`). The governing standard is the core's own
documented contract: ABI-CONTRACT §7i (`docs/ABI-CONTRACT.md:581-617`) states the
status mapping — decode failure crosses as `ERR_TRUNCATED` (`:607`) — and the og's
thumbnail subsystem always prefers the embedded preview when present (the
`allow_embedded` default-ON behavior the core mirrors). The reference-relevant fact
is the og's discipline the core inherited: the surface's behavior and its documented
contract must agree, which is the same standard every ABI-CONTRACT section is held to.

## What our core does

- `include/cr/cr_ui.h:1272-1274`: the section comment states `0x50030004
  ERR_TRUNCATED` "never returned for thumbnails; truncation is disclosed in
  `info->truncated` as data" — the second half is true, the first half is false.
- `src/ui/cr_ui.cpp:3677-3678`: the status map sends `DecodeFailed` to
  `ERR_TRUNCATED` — so a corrupt/short image that the strict decoders refuse (e.g.
  a truncated PNG) DOES return `0x50030004` from `cr_ui_thumbnail_generate`.
- `tests/unit/test_cr_ui.cpp:3508`: `CrUi.ThumbnailTruncationDisclosed`'s refusal
  branch expects exactly `0x50030004u` — the test pins the truth the header denies.
- `docs/ABI-CONTRACT.md:607`: §7i documents the refusal mapping the header denies.
- `include/cr/cr_ui.h:1242-1245`: `allow_embedded` documented "validated … 0/1";
  `src/ui/cr_ui.cpp:3713-3714` maps 0 → default (ON) and any nonzero → ON; the
  engine (`cr_thumbnail.cpp:1413-1415`) validates max_edge/max_pixels but not the
  boolean — 7 is silently accepted as ON.

## Gap

Falsifiable two ways: (1) call `cr_ui_thumbnail_generate` on a node whose image is a
truncated PNG (strict family) → the return is `0x50030004`, contradicting
`cr_ui.h:1272-1274`'s "never returned" claim verbatim; (2) call it with
`caps->allow_embedded = 7` → the call succeeds (no `BadParameter`), contradicting
the header's "validated" claim; with `= 0` the embedded path still runs (0 is
"default", not "off") — the documented knob has no off position.

## Impact

C consumers reading only the header (the ABI's stated audience — the two-call
contract is designed for plain-C hosts, `tests/ui_abi_check.c`) will not handle the
`ERR_TRUNCATED` refusal branch and may treat a refused generation as a hard ABI
error rather than a per-node recoverable condition. The `allow_embedded` overclaim
misleads hosts that need to force full decode (e.g. for forensic pixel evidence)
into believing they can disable the embedded path — they cannot. No recovery-data
impact; the defect is contract truthfulness in a shipped header, which the ABI
policy (§9 of ABI-CONTRACT) treats as load-bearing (headers are the compatibility
surface; no version bump can fix a lying comment retroactively).

## Reproduction / verification

Executed + code-read (S11/T2, `evidence/s11-board-truth.md` §1): the truncated-PNG
DecodeFailed branch was executed in the standalone engine harness (the strict-family
refusal is the root-cause behavior the landing's own fail-before discovered); the
`0x50030004` map read at `cr_ui.cpp:3677-3678`; the test's expectation read at
`test_cr_ui.cpp:3508`; the header claims read at `cr_ui.h:1272-1274` and
`:1242-1245`; the caps mapping read at `:3713-3714` and the engine's validation
surface at `cr_thumbnail.cpp:1413-1415`.

## Fix plan (for the engineering team)

1. Correct the header comment at `include/cr/cr_ui.h:1272-1274` to state the truth:
   decode refusal returns `0x50030004 ERR_TRUNCATED` (with `info->truncated`
   disclosing in-band truncation on the Ok path) — one comment block, no code
   change, no version bump (comments are not part of the ABI surface).
2. Decide the `allow_embedded` semantics and make header and implementation agree
   (recommended: keep the og-parity always-prefer-embedded behavior but change the
   header text to "reserved, must be 0; the embedded fast path is always preferred
   (og parity)" and refuse nonzero with `BadParameter` — or, if a disable path is
   wanted, map 0→OFF explicitly and document it; the og-parity default-ON must
   survive either way).
3. Add the two doc-truth pins the surface lacks: a `ThumbnailHeaderContract` test
   asserting the header's error list matches §7i's (grep-able count/phrase check,
   the pattern used by the other ABI sections' pin tests, e.g.
   `test_cr_ui.cpp:3189`), and a caps-validation test asserting the chosen
   `allow_embedded` contract (refuse-or-accept, pinned).
4. Acceptance: `grep -n "never returned" include/cr/cr_ui.h` returns nothing for the
   thumbnail section; the new pins pass; the C-consumer block in
   `tests/ui_abi_check.c` gains the refusal-branch handling example.

## Tests to add

- `CrUi.ThumbnailHeaderMatchesContract` (doc-truth pin: the header's error-code
  claims equal ABI-CONTRACT §7i's).
- `CrUi.ThumbnailCapsAllowEmbeddedContract` (the chosen semantics pinned: either
  nonzero → `BadParameter`, or 0 → OFF with the documented default restored when
  the caps block is NULL).

## Related findings

M-102 / M-083 (the landing this residual was discovered verifying — both retired
PARITY-VERIFIED with this residual filed rather than absorbed); M-109 (the other
M-080-lineage residual, the 32 MiB cap); N-092 (the refusal-matrix doctrine this
header comment contradicts).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-137 landing — recorded post-push in the ledger row)
**Verified by:** `CrUi.ThumbnailCapsAllowEmbeddedValidated` (0 and 1 accepted; 7 refused with ERR_BAD_PARAMETER 0x50030003 — RED pre-fix: the out-of-range value was silently accepted as ON), `CrUi.ThumbnailHeaderMatchesContract` (the doc-truth pin: the "never returned" denial is gone from the section, the refusal mapping is stated, the caps note records the no-off-position design — RED pre-fix on the header text), plus the whole thumbnail family staying green (cr_ui_tests 61/61; cr_tests 1082/1082, 1040 pass / 42 env skips / 0 fail).
**Resolution notes:** Both contract-truth defects repaired: (1) the section comment's "ERR_TRUNCATED never returned" denial replaced with the truth ABI-CONTRACT §7i and the existing test already pinned (`ThumbnailTruncationDisclosed`'s 0x50030004 expectation) — a decode REFUSAL returns ERR_TRUNCATED with info->truncated also set, while in-band truncation stays an Ok-path disclosure; (2) the caps field's "validated 0/1" claim is now ENFORCED: values above 1 are refused with ERR_BAD_PARAMETER, 0 = default-ON and 1 = explicit-ON (the og-parity always-prefer-embedded behavior is preserved and now DOCUMENTED in the caps comment — the knob has no off position by design, exactly as the finding's recommended option). Comments are not ABI-surface (no version bump); the >1 refusal is a strict tightening of a previously-silent acceptance (any consumer passing 0 or 1 — the documented values — is unaffected; only contract violations change behavior). The finding's third ask (the C-consumer refusal-branch example in tests/ui_abi_check.c) is noted as riding the next abi-check touch — the header/test pair here carries the contract truth.
