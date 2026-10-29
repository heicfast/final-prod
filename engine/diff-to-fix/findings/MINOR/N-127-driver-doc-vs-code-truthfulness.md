# MINOR N-127 — rda-driver doc-vs-code truthfulness defects: a claimed admin gate with no code, stale sector-length prose, dead code the trail says was removed

**Severity:** MINOR
**Module / Surface:** transport (the rda-driver contract docs vs the driver code)
**Parameters affected:** the security-posture claims (global-handle WRITE gating), the sector-length contract prose (CORRECTIONS-2 lineage), dead-code hygiene
**Corpus references:** `artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md` §4 (the legacy global-handle modes — no admin gate in the reference); `eudcpepm-euedkepm--hook-drivers.md` (the session protocol's semantics)
**Core references (rda-driver @ 323e75a):** `src/shared/inc/rda_ioctl.h:360-368` (the admin-gate claim), `src/driver/rda_ioctl.c` (grep: no privilege check), `docs/requirements.md:51`, `docs/ground-truth-contracts.md` §2.2, `src/driver/inc/rda_trace.h:30`, `src/driver/rda_ioctl.c` (RdaSessionDispatch default), `docs/audit/2026-10-22-security-review.md` (the 56/57 checks claim)
**Status:** PUSHED-FORWARD (driver-repo-resident; the two-repo grant of every fix session since S10 excludes heicfast/rda-driver — the OQ-S10-3 boundary record owns the lane)
**Prior audit check:** newly discovered (the SA-9 lane's full driver read; the SCAN-2f D2-partial handoff covered the driver doc drifts generally — this finding owns the specific falsified claims). **Renumbered N-091 → N-127 at the push-time reconciliation** (the session-8 parallel lanes had taken N-091 for parity-deltas-registration-gaps; this lane's N-091 filing was the only ID-colliding finding whose content was NOT convergent — everything else this lane filed merged into the session-8 board as addenda: S-042 WMF, M-081 CRW, S-041 batch-2, M-086, M-087, N-107, N-093).

## Session-86 disposition note (2026-10-29, S86 fix lane — re-verified, pushed forward)

Boundary re-affirmed (the OQ-S10-3 record): all six items' fix surfaces live in
heicfast/rda-driver (`rda_ioctl.h`, `requirements.md`, `ground-truth-contracts.md`
§2.2, `rda_trace.h`, `rda_ioctl.c`'s unreachable default, the S12
security-review count) — untouchable under the two-repo grant. The corpus
posture re-read first-hand this session at `be0cfe9`:
`artifacts/PartitionRecovery/eprdtdrv-sys--driver-internals.md:75-90` — the
§4 IOCTL table + the explicit "No ACL on the device — access control comes
from: (a) signed-driver install …, (b) handle-validation behavior in the
GetFileObjName IOCTL" line: the reference's raw-disk security model is the
install-time + handle-validation posture, NOT caller privilege — confirming
the finding's "Option A (og-parity): delete the admin-gate sentence" as the
parity-faithful resolution when the driver-repo lane takes it. The
Option-A-vs-B decision itself stays with the driver-repo engineering team
(N-127's own open-questions line). No core-side change owed; the next concrete
step stands unchanged (OQ-S10-3's order: M-087 → M-089 → N-107/N-127 prose
sweeps → M-086).

## Summary

Six doc-vs-code mismatches in the rda-driver repo, none behavior-changing
but each a falsified contract claim: (1) `rda_ioctl.h:367` says the legacy
global-handle WRITE "is gated to admin callers" — no privilege check
exists anywhere in `rda_ioctl.c`; (2) `requirements.md:51` still says
session lengths are "in 512-byte sectors" (the pre-CORRECTIONS-2 phrasing
— the code takes byte lengths and returns `Information = Length×512`);
(3) `ground-truth-contracts.md` §2.2 retains pre-G2 prose about empty
device names that contradicts the code's actual handling; (4) the S12
security-review claim "dead code removed" is false — `RDA_DP_COMPONENT`
(`rda_trace.h:30`) is still present and still unparenthesized; (5)
`RdaSessionDispatch`'s final `default: STATUS_SUCCESS` is unreachable
(the explicit 19-code match precedes it); (6) the security review's
"56/57 checks" summary does not document which check is the 57th or its
failing condition.

## What the reference does

The og's EPMNtDrv family implements the legacy global-handle modes
(the 4-byte WRITE storing the caller's handle into a driver global, the
READ resolving it) **with no admin gate** — the reference's security
model for the raw-disk path is the PID-token handshake at CREATE, not
caller privilege. The corpus documents these modes as-is.

## What our implementation does

1. `rda_ioctl.h:360-368` documents the legacy modes and states: "RDA keeps
   the read-name half and gates the store half to admin callers."
   `grep -n "SeCheck\|ZwQueryInformation\|Token\|PRIVILEGE\|admin\|Admin"
   src/driver/rda_ioctl.c` — zero hits. The claimed gate does not exist;
   the code matches the og (ungated), the doc claims a hardening that is
   not there. This is the inverse of the usual drift: an invented claim
   of security rather than an actual regression — but a future security
   review reading the header would credit a control that is absent.
2. `requirements.md:51`: "length in 512-byte sectors" — superseded by the
   CORRECTIONS-2 lineage (byte lengths in, sectors out in `Information`);
   the code and `ground-truth-contracts.md` §5.2's gates agree on bytes.
3. `ground-truth-contracts.md` §2.2: pre-G2 prose about the empty-name
   CREATE handling contradicts the code's G2 behavior (the empty-name
   grammar fix).
4. `rda_trace.h:30`: `#define RDA_DP_COMPONENT 0x1` style macro present
   without the parenthesization the codebase's own macro rules require,
   and the security review's S12 says the dead code it belonged to was
   removed.
5. `rda_ioctl.c` `RdaSessionDispatch`: after the explicit per-code switch
   (G28's 19 cases), a trailing `default: return STATUS_SUCCESS` can
   never execute (the caller `RdaIsSessionCode` already filtered).
6. `docs/audit/2026-10-22-security-review.md`: "56/57 checks" — the
   summary does not name the 57th check or its failure mode; the script
   it summarizes asserts a different count.

## Gap

Each item is a falsifiable doc claim: (1) the gate: absent in code;
(2)-(3) the prose: contradicted by the code paths it describes;
(4) the removal claim: the macro is present; (5) the default: provably
unreachable; (6) the count: undocumented.

## Impact

No behavioral defect. The costs: a security-review reader credits a
control that does not exist (item 1 is the one that matters — if the
admin gate is *desired*, this finding converts into a code TODO; if the
og-parity posture is desired, the doc claim must go); the sector-length
prose misleads integrators computing buffer sizes; the rest is hygiene.

## Reproduction / verification

1. The greps and file reads quoted above (all first-hand this session).
2. The og's posture: eprdtdrv-sys--driver-internals.md §4 + the EPMNtDrv
   PID-handshake contract (CREATE-time token gate, no privilege checks).

## Fix plan (for the engineering team)

1. Decide item 1 first — it is a posture decision, not a typo:
   - Option A (og-parity): delete the "gates the store half to admin
     callers" sentence; the PID-token CREATE gate is the documented
     security model (cite the trio artifact).
   - Option B (real hardening): implement the gate — `SeAccessCheck`/
     `SeSinglePrivilegeCheck(SeDebugPrivilege/SeLoadDriverPrivilege)` or
     an explicit admin-token check in the store path, with a test.
   Either resolves the falsified claim; Option A is the parity target,
   Option B needs a PARITY-DELTAS row.
2. Files: `src/shared/inc/rda_ioctl.h` (the claim), `docs/requirements.md`
   (the sector prose), `docs/ground-truth-contracts.md` (§2.2), 
   `src/driver/inc/rda_trace.h` (remove or parenthesize + use the
   macro), `src/driver/rda_ioctl.c` (drop the unreachable default or
   convert it to an explicit `ASSERT`-style comment), 
   `docs/audit/2026-10-22-security-review.md` (name the 57th check or
   fix the count).
3. Tests (Option B only): an admin vs non-admin global-handle WRITE
   probe on the integration lane.
4. Doc updates: the security-review addendum records the S12 correction.

## Tests to add

- Option B: `DriverSecurity.GlobalHandleWriteRequiresAdmin` (fails before,
  passes after).
- `Lint.NoDocOnlySecurityClaims` (optional): a grep gate pairing every
  "gated to" claim in headers with a symbol reference.

## Related findings

- N-107 — the same repo's count drift (the parallel lane's filing; supersedes this lane's convergent 12-site list).
- I-026 (INFORMATIONAL) — the unsigned-by-design posture record this
  security-posture question sits next to.
- N-040 (FIXED) — the core repo's doc-truthfulness precedent.

## Open questions

- Item 1's Option A vs B is an engineering/security decision (registered
  above, not an unknown). Everything else is mechanical.
