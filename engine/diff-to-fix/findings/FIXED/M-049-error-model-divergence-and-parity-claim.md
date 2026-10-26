# MAJOR M-049 — Error-model divergence: flat 28-code 8-hex-digit enum vs the og's error-object system with 12-digit HRESULT codes; cr_error.h's parity claim is wrong on the documented values

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

- N-061 (engine-log absence — the og's diagnostic `cout` channel and EPMLOG sink are the
  error-consumer surfaces this finding's context struct should feed)
- N-040 (ABI doc truthfulness bundle — the header claim belongs to the same doc-accuracy
  family)

## Open questions

The og's full error-code table (beyond the two documented values) is corpus-UNVERIFIED —
any future decode of the SearchDLL error-emitting sites could pin more values; tracked as
part of this finding, not blocking the recommended option (b).

## Session-11 addendum (2026-11 style, 2026-10-26, DFX11 — header truth + the test; the context struct stays open)

Closed this push: (1) the header's false parity claim REWRITTEN to the
value-space truth (family-prefix convention only, 0x5002..0x5009; numeric
values core-internal — the og has only TWO corpus-documented codes,
12-hex-digit 0x5002200000004/0x5002c0000004; renumbering to them would
break 26 others arbitrarily). Registered D-68. (2) `err_text()` tested
for the first time (Error.ErrTextCoversEveryCode: 18 codes, distinct
non-empty texts, the unknown-code fallback, OK=0). Suite 643/643.

Still open (the plan's step 2, design-gated): the engine-side error-
context subset (module id / operation / retry state) exposed through
cr_ui as optional metadata. NEXT STEP: decide the carrier seam — the
natural fit is the event stream's error-range family (already carries
offset/length/attempts) extended with a module/operation pair at the
orchestrator's emit sites; the og's CErrorManager/CErrorTrackManager
layer stays out of core scope (MOM UI bus).

## Resolution

**Status:** FIXED
**Fixed in commit:** <this-push>
**Verified by:** fail-before RED EXECUTED (attribution recording disabled at the
single retry-device seam: 5/6 RED — ErrorRangeCarriesModuleOperationAttempts,
ProbeBlockFailureAttributesToOrchestrator,
CarveErrorRangeAttributesToCarveModule, Retry.ThreadStampAttributedToRecordedRange,
Retry.OwnerFallbackCoversUnstampedWorkerThreads; restored → all GREEN); suite
**745/745** linux-release. ABI pins: 15 static_asserts in cr_ui.cpp
(CR_ERRMOD_*/CR_ERROP_* == the core enum) + the extended ui_abi_check.c
event_cb (plain-C /W4 /WX readability of the triple).
**Resolution notes:** Session 11 closed the header-truth + err_text halves
(see the addendum above). This session closed the remaining registered
step 2 — the engine-side error-context subset — on the addendum's own
decided seam: the event stream's error-range family. `cr_ui_event` gains
`err_module`/`err_operation`/`err_attempts` (+ an alignment pad — the
8-byte-multiple POD pin stands; v2-era append, no bump per ABI-CONTRACT §9)
with new CR_ERRMOD_* (NONE/ORCHESTRATOR/9 engines/CARVE/LOST/LDM) and
CR_ERROP_* (READ) constants. Attribution is per-range and recorded at
failure time: the retry device stamps each ReadErrorRange with the reading
sub-pass's module — a per-THREAD stamp (`cr::errmod`, cr_errmod.h) for the
orchestrator thread + every pool task it owns (the candidate fan-out
stamps per-candidate families inside scan_candidate), with a per-DEVICE
owner fallback covering engine-internal worker pools (NTFS remnant
fan-out, carve window fan-out); the emit site reads the newest range's
module + attempts. Corpus-verified first-hand this session:
architecture/05-scan-engine-architecture.md:62 (the two 12-digit codes)
and MOM-modules/Error.md (the IErrorModule/IErrorProc registrar census
the module-id set is the engine-side subset of). Session records keep the
geometric trio (wire-compatible both directions; a reloaded range is
honestly kNone — the og's persisted lostpart records carry geometry only,
PRD:20409); coalesced ranges keep the NEWEST constituent's attribution.
D-68 updated (the subset is no longer the open item). The
CErrorManager/CErrorTrackManager MOM-bus layer remains out of core scope
by the finding's own scoping. Tests:
ScanEvents.ErrorRangeCarriesModuleOperationAttempts (FAT-engine traversal
failure in a second cache block — the fixture grew a far subdir),
ScanEvents.ProbeBlockFailureAttributesToOrchestrator (the same dead
sector in block 0 attributes to the probe — the attribution tracks WHO
was reading, not a stage constant),
ScanEvents.CarveErrorRangeAttributesToCarveModule (carve-only scan),
Retry.ThreadStampAttributedToRecordedRange,
Retry.OwnerFallbackCoversUnstampedWorkerThreads (stamp beats owner),
Errmod.GuardSetsAndRestoresTheThreadStamp,
DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce,
DevMap.ModuleNotSerializedWireStaysTrio (44-byte wire pin).
