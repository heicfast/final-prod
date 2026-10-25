# MINOR N-041 — Dead shipped code: RdaClient structured copy-session protocol (zero callers) and volume_letter_to_disk (zero callers)

**Severity:** MINOR
**Module:** transport (new module row) / abi
**Parameters affected:** P69 (stub/TODO/dead-code scan — none in shipped paths), P54-adjacent (module boundaries — an entire unexercised protocol layer), P55-adjacent (dependency direction)
**Corpus references:** `re-workspace/reversed/ghidra-mo/Device_decompiled.c` (the og's client does NOT use a copy-session protocol for reads — grammar + synchronous ReadFile only; the EUDCPEPM copy-session contract the core modeled exists in the corpus driver family but the recovery client never drives it), `re-workspace/reversed/ghidra-drivers/` (EUDCPEPM/EUEDKEPM driver decompiles — the copy-session family), `AUDIT-ARCH.md:213` already registers "wire-or-delete"
**Core references:** `src/win/cr_transport.cpp:380-609` (RdaClient: OPEN_TARGET + full copy-session protocol — session_open/StartBackupIn 0x28/GetNextBlock 0x1E/0x800C2AC0..0x800C2B42 codes, 0x840 OpenSession pack(4) overlay), `include/cr/win/cr_rda.h:77-124` ("preferred over the legacy grammar when the driver advertises it" — not acted on), `cr_transport.cpp:919-962` (`volume_letter_to_disk` — zero callers; `cr_transport.h:57-60` keep-note), production paths use only the grammar-path free functions (`:617-664`, `:718-728`) and `volume_extent` (cr_ui.cpp:2254-2265, rrecov.cpp:990-995); audited commit `63a9167`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

Two implemented-but-never-called surfaces ship in the win layer: the `RdaClient` structured protocol (structured OPEN_TARGET plus the full EUDCPEPM-style copy-session state machine — 20 IOCTL codes, 0x840-byte OpenSession overlay, StartBackupIn/GetNextBlock structures) has zero callers repo-wide; and `volume_letter_to_disk` has zero callers (the F12 destination-overlap preflight uses `volume_extent` only). The header narrative claims RdaClient is "preferred over the legacy grammar" — nothing prefers it. The og's recovery client itself never drives a copy-session for reads, so the dead surface is also unanchored to reference behavior.

## What the reference does

The og's user-mode read client (Device.mo) uses only the grammar open
(`\\.\EPRDTDRV\<mangled-pid>\<disk|Volume<n>>`) with synchronous sector-granular
ReadFile — no copy-session IOCTLs on the recovery read path (evidence:
`evidence/decompiled-quotes/s4-driver-client-and-factory-contract.md` §1-§3). The
copy-session family (EUDCPEPM/EUEDKEPM drivers in the corpus) exists for the backup/
imaging product line, not the recovery read path.

## What our core does

- `RdaClient` (cr_transport.cpp:380-609) implements the structured protocol faithfully
  (codes 0x800C2AC0..0x800C2B42, session states 0/1/3/4/5, the 0x840 OpenSession
  overlay with 4-mod-8 field offsets, StartBackupIn 0x28, GetNextBlock 0x1E) — and
  nothing calls it. `grep -rn "RdaClient" src/ tools/ bench/ tests/ fuzz/` → the class
  definition + its own methods only.
- `volume_letter_to_disk` (cr_transport.cpp:919-962) — intended as writer-overlap
  preflight; the actual F12 arming uses `volume_extent` (cr_ui.cpp:2254-2265;
  rrecov.cpp:990-995).
- The session-3 quality row scored P69 MATCH (0 TODO/FIXME/stub strings — true), but
  dead *implemented* code is the same defect class by another name: unexercised
  protocol surface that silently rots (no tests, no CI path, no consumer).

## Gap

Falsifiable claims: (a) `RdaClient` has zero callers (grep-verified); (b)
`volume_letter_to_disk` has zero callers (grep-verified); (c) the header's "preferred"
narrative (cr_rda.h:77-79) is not acted on anywhere.

## Impact

- Maintainability: ~230 lines of untested protocol code shipping in every build; wire
  format drift between the dead client and the live driver would never be caught.
- Truthfulness: the header narrative misleads readers about the production path.

## Reproduction / verification

- Executed greps this session (the zero-caller claims); direct reads of both surfaces;
  AUDIT-ARCH.md:213 cross-read (the repo's own audit already flags it — this finding
  registers it formally in diff-to-fix with a decision deadline).

## Fix plan (for the engineering team)

1. Decision required (AUDIT-ARCH.md:213's "wire-or-delete"): either
   (a) wire `RdaClient` into an imaging/verification feature with tests, or
   (b) delete it (and `volume_letter_to_disk`) in a dedicated commit, keeping the
   grammar path as the single production surface. Recommend (b) unless the imaging
   feature is scheduled — dead protocol code with a live driver counterpart is a
   liability.
2. **Files to modify**: `src/win/cr_transport.cpp`, `include/cr/win/cr_rda.h`,
   `AUDIT-ARCH.md:213` (mark resolved), this finding → FIXED/.
3. **Tests to add**: if wired — `RdaClient.SessionLifecycleAgainstMockDriver`;
   if deleted — `Transport.GrammarPathIsSoleSurface` (symbol absence lint).
4. **Acceptance criteria**: zero unexercised protocol surfaces in src/win; the header
   narrative matches production; suite green.

## Tests to add

`RdaClient.SessionLifecycleAgainstMockDriver` (if wired) or
`Transport.GrammarPathIsSoleSurface` (if deleted).

## Related findings

- **M-030** — the transport-lane audit that surfaced this; the grammar-path facts
  there assume the single-surface posture.

## Open questions

None.

## Resolution

**Status:** FIXED (fix-plan option (b): DELETE — the recommended path)
**Fixed in commit:** this session's dead-surface batch (DFX12)
**Verified by:** `Transport.GrammarPathIsSoleSurface` implemented as the CI dead-surface absence lint (.github/workflows/ci.yml lint job — `grep -e 'RdaClient::' -e 'class RdaClient' -e 'volume_letter_to_disk'` over src/include/tests/tools/bench/fuzz must find zero matches; executed locally: zero matches); the zero-caller greps re-executed at deletion time (RdaClient, volume_letter_to_disk, rda_bind_structured, wrap_grammar_target, structured_ — all absent from the tree); Linux suite 584/584 (the win layer compiles on the MSVC lane — the deletion is pure-cut + comment edits, verified seam-by-seam). clang-format clean on all three touched files (with the two pre-existing 23-vs-18 churn hunks reverted to HEAD bytes so the CI's format-18 gate sees no churn).
**Resolution notes:** executed the finding's recommended option (b) in full: the RdaClient class (cr_rda.h declaration + cr_transport.cpp method bodies), the structured-open device plumbing it alone reached (rda_bind_structured, the structured_/kind_/disk_/part_/volume_ members, the OPEN_TARGET re-issue branch in reopen_and_rebind_, the orphaned wrap_grammar_target helper), and volume_letter_to_disk (cr_transport.h W4 KEEP-note + declaration + body) — ~16 KB / 511 diff lines removed. The wire structs/constants stay in cr_rda.h as the client-side mirror of the DRIVER's documented protocol (the same ground truth the driver-side rda_ioctl.h records) — marked with N-041 notes so no code path claims to issue them. THE KEEP RULING IN PARITY-REFACTOR-PLAN.md D-A WAS SUPERSEDED with evidence: its cited TRANSPORT-CONTRACT §8 no longer exists in the tree, and its "UI reads info()/ioctl_base()/session_* through it" claim is disproven by the exhaustive zero-caller grep (cr_ui links the ENGINE, never src/win; no test/tool/bench/fuzz consumer). AUDIT-ARCH.md ARCH-DEAD-1 marked RESOLVED (deleted), ARCH-DEAD-2 re-annotated (wire structs kept BY DESIGN as contract documentation), PARITY-ARCH.md:152's refuted claim corrected (the grammar path is the sole production surface), CAPABILITY-MATRIX/DRIVER-UTILIZATION/PARITY-ORCH rows updated, FB-T06/FB-T07 moot (the slop paths died with the class). The grammar path (open_rda_partition / open_rda_volume — D-B's KEEP) is untouched.
