# INFORMATIONAL I-017 — Slice-layer deliberate supersets: byte units, eager validation, rich per-call errors (+ no sticky error, no null guard)

**Severity:** INFORMATIONAL (deliberate differences ledger for the storage layer)
**Module:** storage
**Parameters affected:** C20 (units), Q43 (lifetimes/validation), S35 (error model)
**Corpus references:** HFSPlusSearchDLL_decompiled.c — sector units throughout (ctor :7113-7138, read :7219-7264); no ctor validation; sticky last-error + null-buffer −2 (:7234-7237); evidence: `evidence/decompiled-quotes/s3-virtual-volume-device.md`.
**Core references:** `src/core/storage/cr_slice.cpp:15-41` (eager validation: null/zero/alignment/overflow), `cr_slice.h:26-28` (byte units, sector-aligned starts, ragged tails), `cr_error.h:23-32` (per-call u32 codes, HRESULT-styled), `cr_devmap.cpp` (persistent ReadErrorMap — a clean-room addition the og lacks); no null-buffer guard, no sticky error. Audited commit `7fd00e0`.
**Status:** READ-ONLY RECORD
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `I-012` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.
 — deliberate supersets (the one behavioral divergence, tail reads, is M-032)

## Summary

Five deliberate differences in the slice layer, recorded as the parity ledger: (1) byte-granular units with enforced alignment (strict superset of the og's sector granularity); (2) eager ctor validation (og defers errors to read time); (3) rich per-call error codes + persistent ReadErrorMap (og: sticky last-error with 3 values, no map); (4) no sticky-error channel (per-call is strictly better in our contract); (5) no null-buffer guard (og has one — trivial to add if wanted). Tail-read semantics (clamp+zero-fill vs refusal) is the only behavioral divergence and is tracked as M-032.

## Action

No change required. Optionally add the null-buffer guard (one line) for robustness parity.

## Related findings

M-032 (the behavioral divergence), N-045/N-046 (the same layer's minor ledger).

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** cr_slice.cpp eager validation (null/zero/alignment/overflow) re-read this session; byte units + per-call error codes + persistent ReadErrorMap confirmed; the og's sector-units/sticky-error/null-guard model cited in evidence/s3-virtual-volume-device.md.
**Resolution notes:** Deliberate storage-layer supersets, preserved: byte anchoring (F1), eager ctor validation, per-call HRESULT-styled errors, the clean-room ReadErrorMap. The og's sticky last-error and null-buffer -2 semantics are NOT ported (weaker contract). Future work must keep the eager validation.
