# MAJOR M-156 — the landed X3F surface rejects every real Sigma file: the ASCII version gate survives the four-lane arbitration, and the modern generation's count-walk never runs — the S83 correction (the dispatcher-mechanism refutation + the chain router)

**Severity:** MAJOR (user-observable: real Sigma X3F RAW files do not carve at all — the gate rejects the binary version form every real camera writes)
**Module / Surface:** carve (src/core/carve/cr_analysers.cpp validate_x3f, the FMT_X3F row in cr_registry.cpp)
**Parameters affected:** A25 (the X3F size walk), A27 (the carved extent end), A47-adjacent (the row bound vs the og walker's own 100 MiB cap), B1-adjacent (the acceptance gate)
**Corpus references:** the corpus DLL itself, disassembled first-hand this session — `diff-to-fix/evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md` (every cite below is an instruction address in that sheet). Note: the Ghidra decompile the four prior lanes cite (`RAWSearchDLL_decompiled.c :NNNNN`) is CI-generated and NOT COMMITTED to the corpus repo — those line cites are unverifiable from the corpus; the DLL bytes are the only arbitrator.
**Core references:** the landed `validate_x3f` at the S82-close tree (`2d614cf2`-lineage): the ASCII gate `h[4] < '0' || h[4] > '9' || h[5] != '.'`; the legacy-only walk (SECd + 0x4C + salvage); the row bound 64 MiB; the five-test legacy-path set (all ASCII-header plants).
**Status:** FIXED (this session — the S83 correction, riding the M-107 X3F member's fourth landing)

## Summary

The four-lane X3F arbitration (S71 → S78 → M-155/S81 → S82) settled the
WALK question (the legacy FileX3FAnalyser's SECd + 0x4C + torn salvage
landed as the sole semantics) on the strength of N-217's dispatcher decode
("vtbl[+0x40] GetFallback consults the secondary FIRST … the Brance
count-walk never contributes"). Two defects survived that settlement:

1. **The live bug:** the landed ASCII version gate rejects every real
   Sigma X3F file. Real files carry a BINARY u32 version (0x00020001 =
   bytes `01 00 02 00`) — the gate demands an ASCII digit + point and
   refuses the binary form outright. **Real Sigma X3F RAWs do not carve.**
   The og's net acceptance across the chain is "readable header": the
   modern generation's tier-1 (FUN_180040080) accepts exactly the binary
   shape; the legacy's tier-1 (FUN_180053590) accepts any readable header;
   the registry keys on FOVb alone. No tier-1 pair ever jointly rejects.
2. **The refuted mechanism:** N-217's dispatcher claim does not fire for
   the X3F node. The mechanism is real (the dispatch sites at
   0x18000d237/0x18000d31a/0x18000d4ca call `vt[+0x40]`; a non-NULL result
   routes to the fallback first) — but it requires the node's class to
   OVERRIDE +0x40 with the real getter (FUN_180013700 `lea rax,[rcx+0x10]`).
   A full .rdata scan (120 vtable-like runs) finds exactly 8 node classes
   with the real getter — **the X3F node's vtable 0x18006cbc0 carries the
   return-0 STUB (0x18000bae0) at +0x40, byte-exact from the PE; the DMG
   node (0x18006cd90, the M-090 precedent) carries the same stub.** The
   +0x40-overriding classes use a different registration shape (an
   embedded fallback vtable stored directly at node+0x10, e.g. the
   0xd302020 node at 0x1800034f3-0x180003533) — not the X3F/DMG
   separately-allocated child-pointer shape.

## What the reference does (the wired path, verified first-hand)

For an FOVb candidate with a binary version: the magic-key map path inside
FUN_18000cf60 (0x18000d1e4-0x18000d2c9) looks the node up, the +0x40 stub
returns 0, **the node's own +0x10 binary gate (FUN_180040080) runs and
accepts**, and the MODERN NODE is returned; the invocation core
(0x18000ca8d-0x18000ccfc) calls the node's factory (+0x38 =
FUN_1800400a0: the 0x120-byte walker, cap [obj+0x118] = 0x6400000 =
100 MiB, extent field zeroed) and registers it into the scan's
active-walker list — the walker's +0x20 (FUN_180040160) is a per-chunk
state machine (return 2 = "feed me more", return 1 = "walk complete")
whose walk (FUN_1800404a0) computes **extent = SECd + 16 + count*12**
(count = u32@SECd+8, clamped) and declines on no-directory (the extent
field is never written). The legacy walk (FUN_1800535b0: SECd + 0x4C +
the loop-exit salvage, permissive tier-1) serves everything the modern's
gate refuses. The full decode: the S83 evidence sheet.

## What our core did (at the open tree)

The legacy-only walk for ALL files + the ASCII gate (rejecting real
files) + the 64 MiB row bound. The count-walk never ran for any
population; real Sigma files never carved.

## Gap

Falsifiable two ways at the open tree: (1) a plant with the binary
version 0x00020001 and a real directory DOES NOT CARVE (the ASCII gate
refuses h[4]=0x01) — `Carve.X3fRealSigmaBinaryVersionCarves` is RED
there and GREEN post-fix; (2) the same plant under a hypothetical
gate-removal-only fix carves at SECd + 0x4C, while the wired path
(§What the reference does) computes the count-table end — the router
test `Carve.X3fChainRoutesByVersionShape` pins the two-generation split.

## Fix plan (executed this session)

1. Remove the ASCII gate entirely — the chain's union acceptance (the
   modern's binary form OR the legacy's permissive read = any readable
   FOVb header).
2. Route by version shape: binary → the modern count-table walk
   (SECd + 16 + count*12, clamped; the unreadable-count corner = the
   og's chunk-tail shape, the window end); anything else → the legacy
   fixed-tail walk (SECd + 0x4C — the four-lane convergent semantics,
   preserved for the non-binary population); torn → the legacy
   walked-prefix salvage at reduced confidence.
3. The row bound takes the og walker's OWN cap constant (0x6400000 =
   100 MiB, raw-asm at 0x1800400ee) — the 64 MiB stand-in value leaves
   the row.
4. Tests: the five legacy-path tests (all ASCII plants) stand unchanged;
   five new tests pin the modern path and the router:
   X3fModernCountWalkSetsExtent / X3fModernGarbageHugeCountClampsToBound
   / X3fModernTornFallsBackToLegacySalvage / X3fChainRoutesByVersionShape
   / **X3fRealSigmaBinaryVersionCarves (the headline RED-at-open pin)**;
   the torn row-bound re-pin (64 MiB → the device end under the 100 MiB
   cap) and the noise test's bad-version X3F re-pin (the chain's
   acceptance shape — the og never refuses a readable FOVb).

## Open questions (carried honestly)

(a) The engine's feed loop driving walker->vt[+0x20] was not located
(the state machine's protocol is the evidence it exists; the
registered-to-be-fed inference is strong but the caller was not found).
(b) The registry's container merge (the singleton's six containers; which
objects the +0x00/+0x10/+0x18 views hold) was not decoded to completion —
the legacy child's precise reachability path (the sweep's membership)
is open. (c) Consequently, for a binary-version file with a directory,
the og emits EITHER the count-walk extent OR the legacy 0x4C — the
wiring evidence favors the count-walk (this landing), three convergent
prior lanes favor the legacy constant (the prior landing); the
disagreement is recorded, and the next session's decisive probe is named:
locate the +0x20 feed caller in CRAWSearchController's scan loop.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's S83 correction commit
**Verified by:** the five legacy-path X3F tests green (unchanged) + the five new modern-path/router tests green (X3fRealSigmaBinaryVersionCarves RED at the open tree — the ASCII gate refused the binary form — and GREEN post-fix; the full X3F battery 10/10) + the S007 niche batch + noise tests green + the full suite green at the post-fix tree + the evidence sheet's byte-exact cites (pefile reads of the vtable slots; the .rdata scan over 120 vtable runs)
**Resolution notes:** The ASCII gate is gone (real Sigma files carve); the chain router lands the modern count-walk for binary versions on the wiring evidence, preserving the four-lane legacy semantics for everything else; the row bound carries the og walker's own cap constant. The N-217 dispatcher mechanism is corrected in the trail (real mechanism, wrong node class — the X3F/DMG nodes' +0x40 is the stub; only the 8 embedded-fallback classes override it). The prior lanes' Ghidra line cites are recorded as unverifiable-from-corpus (CI-generated, not committed) — this session's cites are instruction addresses against the corpus binary. The remaining open questions (the feed-loop caller, the container merge, the extent disagreement) are named with their decisive next probe in the finding and the evidence sheet §6.
