# MAJOR M-034 — No NAS/remote device layer (og: CRemoteDevice/CDoScanRemoteDevice + EuMrx redirector)

**Severity:** MAJOR (scope UNVERIFIED — depends on whether NAS scanning is in the product roadmap)
**Module:** storage
**Parameters affected:** A8 (multi-volume), Q54/Q55 (module boundaries/dependency direction), Q46 (contract documentation)
**Corpus references:** `re-workspace/rtti_classes.json` (DRWUI section: classes `CRemoteDevice`, `CDoScanRemoteDevice`); `architecture/05-scan-engine-architecture.md:124` (§8: "NAS: EuMrx redirector", `X:\EuShare.com\…` paths); `artifacts/DRWUI-exe--main-ui-architecture.md:214-215` (EuMrx/NAS scan flow).
**Core references:** `include/cr/storage/` (5 headers — device, slice, cached, retry, devmap; no remote class; grep-verified). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-018` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og product can scan NAS shares through a remote-device layer (CRemoteDevice/CDoScanRemoteDevice wrapping an EuMrx redirector that mounts shares as drive paths). Our storage layer has no remote device concept at all. If NAS scanning is in scope, the layer must be added; if not, the exclusion must be documented as an explicit parity exclusion (currently it is documented nowhere).

## What the reference does

RTTI proves both classes in DRWUI; the architecture doc places EuMrx as the §8 transport alternative for remote volumes; the DRWUI artifact documents the NAS scan flow.

## What our core does

All five storage headers are local-device implementations (physical, slice, cache, retry, devmap). No remote transport, no EuMrx equivalent.

## Gap

Capability absent. Falsifiable: core cannot scan a NAS share at all (`\\server\share` or a mapped drive pointing at SMB storage is refused or unsupported by the device layer).

## Impact

Bounded by scope: NAS recovery is a real og capability; its absence is user-observable only for users who need it. The finding's action is primarily a scope decision + documentation truth.

## Reproduction / verification

Corpus-derived (RTTI + docs). Core: directory/code read. UNVERIFIED-BY-EXECUTION (needs a NAS/SMB environment).

## Fix plan

1. Product decision first: NAS in or out of core scope.
2. If in: add `RemoteDevice : IStorageDevice` over an SMB client (or the OS network path), following the og layer order (physical → cache → remote wrapper → slice); the EuMrx interop itself is og-internal plumbing we do not need to replicate — the device contract is the parity surface.
3. If out: add an explicit "NAS scanning — excluded by scope" row to docs/PARITY-DELTAS.md and this finding's Resolution note.

## Tests to add

- (If in) `Storage.RemoteDeviceReadsThroughCache` — mock transport; assert read/caching behavior matches the local path.
- (Either way) `Docs.ParityDeltasListsNasExclusion` — doc-truth pin.

## Related findings

M-033 (BitLocker — the other §8 transport sibling), I-017 (slice supersets), F-VPV-3 in the evidence file.

## Open questions

Whether the og's NAS scan actually recovers deleted files from shares (server-side undelete semantics differ from raw block access) — a product-level question the corpus does not answer; the RTTI proves the scan-path plumbing only.


## Resolution

**Status:** FIXED (scope registration — the finding's fix plan step 3)
**Fixed in commit:** (this session's M-034 commit — DFX24)
**Verified by:** `Storage.ParityDeltasListsNasExclusion` — the doc-truth pin (fail-before by construction: neither the D-88 row nor the cr_device.h block existed); suite 893/893 linux-release post-union.
**Resolution notes:** The product decision the finding demanded: NAS scanning is OUT of core scope, REGISTERED as PARITY-DELTAS D-88. The registration records the og's evidence (CRemoteDevice/CDoScanRemoteDevice RTTI, the EuMrx redirector, architecture/05 §8), the core's posture (five local-device layers; the IStorageDevice contract is transport-agnostic — a host MAY layer an SMB-backed device; the win layer's volume path reads mapped drives through the OS redirector for file-level recovery), and the rationale: the EuMrx interop is og-internal plumbing (the finding's own fix plan: "the device contract is the parity surface"); the corpus proves scan-path plumbing only — whether the og's NAS scan actually recovers deleted files from shares is undecoded (the finding's own open question, now the row's REVISIT TRIGGER); a network stack (credentials, timeouts, offline caching) is host policy, not pure-core surface. The finding existed to force exactly this disposition — the capability was never registered anywhere before.
