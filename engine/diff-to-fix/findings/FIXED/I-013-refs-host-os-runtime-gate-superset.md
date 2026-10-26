# INFORMATIONAL I-013 — ReFS host-OS runtime gate: og refuses to run below Windows 8; core has no host gate (deliberate, cross-platform scope)

**Severity:** INFORMATIONAL (deliberate, documented scope exclusion — reviewed and accepted)
**Module:** refs
**Parameters affected:** B1 (detection correctness — environment gating), Q70 (Win7-inclusive posture, inverse direction)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c` — FUN_1800010d0 :37–49: `GetVersion()`; `if (major < 6 || minor < 2) return 0;` (gates the engine DLL at runtime); import `KERNEL32.DLL::GetVersion` (ReFSSearchDLL_imports.txt:1); architecture doc `architecture/05-scan-engine-architecture.md` §5 "ReFSSearchDLL | OS-version gating" (the doc's "OS-version gating" is this host gate)
**Core references:** `src/core/refs/cr_refs_engine.cpp` + `src/core/refs/cr_refs_detect.cpp` @ `34b3441` — no host-OS check anywhere in the refs module (the core is platform-portable; `scan_refs` runs on any host with an `IStorageDevice`)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's ReFS DLL self-gates on the *host* Windows version — it refuses to run below Windows
8 (6.2), consistent with ReFS first shipping read-capable in Win8/Server 2012. Our core has
no host gate: it scans ReFS volumes on any platform (including the Linux bench lanes). This
is a deliberate portability property of the core, recorded so parity benches on non-Windows
hosts are not counted as og-divergent, and so the architecture doc's "OS-version gating" is
not misread as an on-disk version gate (the on-disk gates are I-004 + N-026 (N-013b)).

## What the reference does

`FUN_1800010d0:37–49` — called with `param_2 == 1` from the DLL's engine-availability path:
`GetVersion()`; `(byte)ver < 6` → refuse; `(byte)(ver >> 8) < 2` → refuse. I.e. Windows ≥ 6.2
(Win8 / Server 2012) required. No on-disk meaning — pure host gating.

## What our core does

No host-OS check exists in the refs module (or any core engine): `scan_refs`/`detect_refs`
depend only on `IStorageDevice`. The core's Win7 posture work (Q70) is an import-allowlist
concern on the Windows build, not a scan-time gate.

## Gap

Host-environment superset: the core attempts ReFS scans where the og would refuse to load.
No on-disk behavior differs.

## Impact

None on recovery correctness. Benches: any host-vs-host delta is environmental, not parity.
(If the og's gate ever misleads a bench harness into "no og results" on an old-Windows lane,
this row explains it.)

## Reproduction / verification

Decompiled citation + import census line; core absence grep-verified (no `GetVersion`/
version check in the refs module; the core does not import Win32 version APIs on any
platform path).

## Fix plan (for the engineering team)

None needed. Register one line in `docs/parity/PARITY-DELTAS.md` ("ReFS engine host gate:
og Win8+; core host-agnostic — deliberate") so future sessions don't re-derive it.

## Tests to add

None (no behavior to pin).

## Related findings

- I-004 + N-026 group item N-013b (the *on-disk* gates — do not confuse with this host gate).
- N-007 (same module, detection family).

## Open questions

None.

## Resolution

**Status:** ACKNOWLEDGED
**Verified by:** first-hand re-read of FUN_1800010d0 :37-49 (GetVersion; major<6||minor<2 -> refuse) + grep-verified absence of any host-OS gate in the core refs module.
**Resolution notes:** Deliberate portability superset, preserved: the core parses ReFS on any host (the Win7-inclusive posture cuts BOTH ways - Q70 keeps the import gate while the ReFS engine drops the og's Win8+ host gate). The gate exists og-side because ReFS client-read support is OS-gated; a user-mode parser has no such dependency. Future work must NOT add a host gate.

**Status:** ACKNOWLEDGED (deliberate portability superset — preserved)
**Verified by:** re-verification this session: corpus
`ReFSSearchDLL_decompiled.c` FUN_1800010d0 (:37-49) — `GetVersion()`,
refuse below 6.2 (host gate; re-read directly). Core: the only
"GetVersion" reference in the tree is `kIoctlGetVersion` — an RDA-driver
IOCTL (`src/win/cr_transport.cpp:489`), not a host-OS check; no
`VerifyVersionInfo`/`GetVersion` host gate exists in any engine. The core
scans ReFS on any host with an IStorageDevice.
**Resolution notes:** rationale correct; parity benches on non-Windows
hosts are not og-divergent by this row. Registered as PARITY-DELTAS
**D-59f**. Do not add a host gate.
