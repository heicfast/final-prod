# MINOR N-098 — Enumeration refresh contract sentence absent: nothing documents that each `cr_ui_enumerate_disks` call re-queries the OS (the og's refresh flow equivalent)

**Severity:** MINOR
**Module:** ABI / storage-enumeration / docs
**Parameters affected:** Q68 (documentation of the contract), P42-adjacent (the pull-model equivalence statement)
**Corpus references:** `re-workspace/artifacts/UIUX/13-ui-core-wiring-state.md:14` ("| `FUN_14076df60` | device manager (enumerator) | view-manager slots 7/8/14 (`FUN_14076e1d0(dev,1)` start refresh, `FUN_14076e0c0`), refresh dialog flow |"); `UIUX/07-home-phase-deep-dive.md:11` (refresh is user-triggerable via the modal flow: `QDialog::exec` + `refreshFinished()`)
**Core references:** audited `35adcb9`. Behavior verified always-fresh first-hand: `src/ui/cr_ui.cpp:1631-1671` (`enumerate_disks_impl` constructs a local `win::PhysicalDiskList`, calls `win::enumerate_disks(200, list)` per invocation); `src/win/cr_enumerate.cpp:139-159` (the numbered PhysicalDrive loop + cfgmgr32 pass, each call; zero cache — grep "cache" over src/win → 0). No doc states the equivalence: `include/cr/cr_ui.h:442-471` documents the two-call pattern and failure semantics only; `docs/ABI-CONTRACT.md` has no always-fresh sentence (grep "fresh|refresh" → only carve-table/per-scan notes :461-469). `win::refresh_disk_properties` (IOCTL_DISK_UPDATE_PROPERTIES, S-029) is C++-only (`cr_transport.cpp:974-999`), documented as such at `docs/REFERENCE-API-GAP.md:329`.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og has a user-triggerable refresh flow (a modal re-enumeration). The core's equivalent — calling `cr_ui_enumerate_disks` again — is real (every call re-queries, no caching anywhere in the path, verified first-hand) but undocumented: no header sentence, no ABI-CONTRACT line tells a host that re-calling IS the refresh. The plan-of-record's remaining half for R-86 is exactly this sentence plus a pinning test.

## What the reference does

The og's home page exposes refresh as an explicit modal flow that re-runs the same enumeration machinery (UIUX/13:14, UIUX/07:11).

## What our core does

Pull-model enumeration, always fresh (the citations above), with the refresh-properties IOCTL available C++-side for the update-disk-metadata case (S-029).

## Gap

The contract is behaviorally complete but textually unstated: no documentation says "each call re-queries; there is no cache; `list` is the refresh". Falsifiable: grep the header + ABI-CONTRACT for the statement — absent.

## Impact

Host-contract clarity only. A host might cache rows believing the API memoizes, or ask for a refresh export that the model makes unnecessary.

## Reproduction / verification

First-hand: the always-fresh path read (cr_ui.cpp:1631-1671; cr_enumerate.cpp:139-159); the doc greps (no statement); the corpus refresh-flow cites read.

## Fix plan (for the engineering team)

1. `include/cr/cr_ui.h` enumeration section: one contract sentence — "Each call re-queries the OS; there is no caching — calling this again IS the refresh (the og's modal refresh flow re-runs the same enumeration; the modal chrome is host-side)."
2. Mirror sentence in `docs/ABI-CONTRACT.md`'s enumeration subsection; flip the REFERENCE-API-GAP:329 row to point at the statement.
3. Test: a pinning unit test asserting no state is retained between calls (fresh list construction per call — trivially true today, pinned against future regression) +, on the Windows lane where fixtures allow, a two-consecutive-calls-reflect-change row.

## Tests to add

- `Enumerate.NoStateRetainedBetweenCalls` (the pin; fails only if someone adds a cache without updating the contract).

## Related findings

- M-076 (this session — the other enumeration-row gap, the TRIM fields).
- S-029 (FIXED — the refresh-properties IOCTL).

## Open questions

None.

---

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-098 contract batch — the session-38 minor-first lane)
**Verified by:** `CrUi.EnumerationRefreshContractPinned` (new) — EXECUTED RED on the pre-fix header (the contract sentence absent: both the "RE-QUERIES the OS" and "no cache" assertions failed), GREEN after; full suite green (linux-release, gcc Werror); clang-format 18.1.8 clean.
**Resolution notes:** Re-verified at current main: the always-fresh behavior stood (enumerate_disks_impl constructs a local `win::PhysicalDiskList list;` per invocation, zero cache — read first-hand) while no doc stated it (the header's enumeration section, ABI-CONTRACT, and R-86's row were all silent; grep-verified). The fix per the plan: (1) the contract sentence landed in cr_ui.h's enumeration section (each call RE-QUERIES the OS — no cache, no memoization, no retained state; calling again IS the refresh; the og's modal flow re-runs the same machinery, the chrome is host-side; the update-disk-metadata case is the C++-only S-029 IOCTL); (2) the mirror sentence landed in CORE-API-CURRENT's b.3 row and API.md's census row (ABI-CONTRACT has no enumeration subsection — the finding assumed one; the mirror lives where the enumeration contract actually lives, noted here); (3) R-86's gap row flipped from PARTIAL to PRESENT (pull model, registered) pointing at the statement; (4) the pinning test asserts the header sentence, the local-list implementation shape (and the absence of static state inside enumerate_disks_impl), and R-86's statement — so a future cache cannot land silently. The Windows-lane two-consecutive-calls runtime probe the finding suggested remains the MSVC lane's smoke addition; the source-level pin covers the regression class cross-platform.
