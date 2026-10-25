# MAJOR M-043 — Error-model divergence: flat 28-code 8-hex-digit enum vs the og's error-object system with 12-digit HRESULT codes; cr_error.h's parity claim is wrong on the documented values

**Severity:** MAJOR
**Module:** err (+ ABI/CLI surfaces)
**Parameters affected:** C23 (error codes — same classes, same mapping: FAIL on the two documented values), D14 (error-map shape — no error objects/contexts), F35 (error recovery — no error-tracking semantics), Q46 (documentation accuracy — the header's "reproduces the reference engine convention" claim is false as stated)
**Corpus references:** `architecture/05-scan-engine-architecture.md:62` ("engine error codes are HRESULT-style `0x5002xxxx` (e.g. **0x5002200000004** open-fail, **0x5002c0000004** read-fail)"); `re-workspace/artifacts/MOM-modules/Error.md` (error-object architecture: CErrorManager/CErrorTrackManager + IErrorBase/IErrorManager/IErrorModule/IErrorProc/IErrorTrackManager interfaces + CErrorObject in the MOM skeleton + per-module CMomError/CMyErrorProc registrars — RTTI census)
**Core references:** `include/cr/cr_error.h:4` ("Reproduces the reference engine convention (HRESULT-style 0x5002xxx family…"), `:22-33` (ERR_OPEN_FAILED = 0x50020001, ERR_READ_FAILED = 0x50020002, … 8-hex-digit values), `src/core/err/cr_error.cpp` (flat mapping), `err_text` untested — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's engine error surface is (a) a set of 12-hex-digit HRESULT-shaped codes — the corpus
documents two concrete values (open-fail 0x5002200000004, read-fail 0x5002c0000004) — and (b)
an error-object architecture (managers, trackers, per-module registrars) that carries error
context across the MOM bus. Our core is a flat 28-code enum with 8-hex-digit values whose
header explicitly claims to "reproduce the reference engine convention" — the family prefix
matches (0x5002) but the documented og values are 8 digits longer and our values differ from
both documented constants. The error-object layer has no counterpart at all.

## What the reference does

- Code values (`05-scan-engine-architecture.md:62`, quoted verbatim in Corpus references):
  engine error codes are HRESULT-style with the family in the high word — 0x5002200000004 for
  open-failure, 0x5002c0000004 for read-failure. The shape is
  `0x5002_<subcode>_<flags/severity>` — 12 hex digits total.
- Error objects (`MOM-modules/Error.md` census): CErrorManager and CErrorTrackManager
  top-level managers; the IErrorBase/IErrorManager/IErrorModule/IErrorProc/IErrorTrackManager
  interface family; CErrorObject in the MOM skeleton; per-module CMomError/CMyErrorProc
  registrars. Errors flow as objects with module/proc context across the MOM bus to the UI's
  error surfaces (error-map display family).

## What our core does

`cr_error.h:22+`: `ERR_OPEN_FAILED = 0x50020001`, `ERR_READ_FAILED = 0x50020002`, … 28 codes
in 8 families (0x5002 open/transport through 0x5009), each an 8-hex-digit u32. The header
comment (`:4`) claims: "Reproduces the reference engine convention (HRESULT-style 0x5002xxx
family…". There is no error object, no context carrier, no per-module registration; `cr_error.cpp`
is a code→text table (`err_text`), which has no test.

## Gap

Falsifiable claims: (a) the og's documented open-fail code is 0x5002200000004 — the core's
`ERR_OPEN_FAILED` is 0x50020001; the values differ and the shapes differ (12 vs 8 hex digits);
(b) the core has no error-object/context surface — `err_text()` is the entire consumer-facing
error surface beyond the raw u32.

## Impact

- Any consumer mapping our codes against og-era logs/support tooling (the 12-digit codes
  appear in the og's `cout` diagnostic channel per 05-scan:62) cannot correlate.
- The header's parity claim is false as written — a documentation-accuracy defect (Q46) that
  will mislead the next implementer.
- The error-context richness (which module, which proc, retry state) that the og's UI error
  map shows has no core equivalent — errors surface as bare codes/strings.

## Reproduction / verification

- Corpus: both citations read first-hand this session (05-scan:62 verbatim; Error.md census).
- Core: cr_error.h read first-hand (values + the claim comment).
- Cross-check: sessions 1-4 never scored C23/D14 against the two documented values (the
  citation map shows 05-scan-engine-architecture.md cited once, for a different section).

## Fix plan (for the engineering team)

1. Decide the value policy: either (a) renumber to the og's 12-digit shape for the two
   documented codes (open-fail/read-fail) and keep the family discipline for the rest,
   documenting the UNVERIFIED remainder; or (b) keep the core's own u32 space and REWRITE the
   header claim to "family-prefix convention only; numeric values are core-internal". Option
   (b) is recommended — the core's ABI owns its error space, and the og's full code table is
   not corpus-documented (2 values only).
2. Add an error-context struct at the internal boundary (module id, operation, retry state)
   populated at emit sites, exposed through cr_ui as optional metadata — the engine-side
   subset of the og's error objects (the manager/track-manager UI layer stays out of scope).
3. Test `err_text()` for every code (currently untested).
4. Update the parity docs: register the deliberate value-space decision; correct the header
   comment.

## Tests to add

- `Error.ErrTextCoversEveryCode`
- `Error.CodeFamiliesAreContiguousAndDocumented`
- `Error.ContextCarriesModuleAndOperation` (after step 2)

## Related findings

- N-058 (engine-log absence — the og's diagnostic `cout` channel and EPMLOG sink are the
  error-consumer surfaces this finding's context struct should feed)
- N-040 (ABI doc truthfulness bundle — the header claim belongs to the same doc-accuracy
  family)

## Open questions

The og's full error-code table (beyond the two documented values) is corpus-UNVERIFIED —
any future decode of the SearchDLL error-emitting sites could pin more values; tracked as
part of this finding, not blocking the recommended option (b).
