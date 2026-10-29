# S83-L1 — the X3F two-generation chain + the dispatcher-mechanism refutation, decoded first-hand from the raw asm (M-156's ground truth)

**Session-83 fix lane (2026-10-29).** This sheet arbitrates the X3F surface
for the FOURTH time (S71 → S78 → M-155/S81/S82) — this time with the
dispatcher plumbing followed into the engine, and with two claims of the
prior landings corrected against byte-exact evidence. Method: `objdump -d`
over the full .text of `og-codebase/RAWSearchDLL.dll` (118,700 instruction
lines, image base 0x180000000); vtables read straight from .rdata via
pefile (byte-exact, not decompiler output). The Ghidra decompile the prior
lanes cited (`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c`)
is CI-generated and NOT COMMITTED to the corpus repo — every prior lane's
":NNNNN" line cites are unverifiable from the corpus; the DLL is the only
arbitrator, and this sheet's cites are instruction ADDRESSES.

## 1. The registry binding (unchanged across all four lanes — verified again)

`0x180003174`–`0x1800031f4`: `alloc(0x18)` → the modern Brance::X3FAnalyser
node (vtable 0x18006cbc0, idx 0x6c) with `alloc(0x10)` → the legacy
FileX3FAnalyser child (vtable 0x18006e198, idx 0x6c) at node+0x10;
`map['FOVb' = 0x62564f46] = node`.

## 2. The two generations (both real; both fully decoded)

**The modern walk** — the node's vtable slots (byte-exact from .rdata):

| Slot | Function | Role |
|---|---|---|
| +0x00 | 0x180040010 | dtor |
| +0x10 | **FUN_180040080** | tier-1: version bytes +5/+7 == 0 (the BINARY u32 gate) |
| +0x38 | **FUN_1800400a0** | factory: `new(0x120)` walker, vtable 0x18006cc10, cap [obj+0x118] = **0x6400000 = 100 MiB** (0x1800400ee), extent field [obj+0x110] = 0 |
| +0x40 | **0x18000bae0** | `xor eax,eax; ret` — the RETURN-0 STUB (see §3) |

The walker's vtable 0x18006cc10: +0x20 = FUN_180040160 (the per-chunk
feeding main; caller-verified return-2-while-scanning /
return-1-on-completion) → FUN_1800404a0 (the sector-fed walk): the
chunk-tail dword == 'SECd' → extent = the chunk end (0x1800404f9-0x180040501);
the tail == 0 → the byte-exact 'SECd' search (FUN_18003ffc0, the generic
dword finder over the global needle [0x180093900]) plus the half-back
2-sector overlap retry (0x180040596-0x180040610); found → **extent =
SECd + 16 + count\*12** (count = u32@SECd+8; the math at
0x180040541-0x180040571 and 0x1800405cf-0x180040609, clamped via cmova);
no directory in range → **the extent field is NEVER written** (the ctor
zero at 0x1800400e7; the cap-exit at 0x1800404dc jumps straight to the
epilogue) — the modern declines.

**The legacy walk** — the child's vtable 0x18006e198 slots: +0x08 =
FUN_1800535b0 (the walk), +0x10 = FUN_180053590 (tier-1: any readable
header), +0x38 = 0x18000ec90 (a self-factory class), +0x40 = the stub.
FUN_1800535b0: direct device reads via FUN_180042930, sector-stepped; the
tail dword before each sector boundary; **extent = SECd + 0x4C in all
three found branches** (0x18005375e/0x180053752/0x180053746); the
loop-exit at 0x180053762 stamps **the walked prefix** (the salvage, never
a refusal); stamps the chain idx 0x6c unconditionally; returns 1.

## 3. THE DISPATCHER REFUTATION — the S82/N-217 mechanism does NOT fire for X3F

The N-217 finding's decisive claim ("the registry node's vtbl[+0x40]
GetFallback consults the File*Analyser SECONDARY FIRST … the Brance
count-walk never contributes to a carved X3F extent") is built on a real
dispatcher mechanism — but one that CANNOT fire for the X3F node:

1. The mechanism is real: the dispatch sites (0x18000d237,
   0x18000d31a, 0x18000d4ca — the magic-key map path inside
   FUN_18000cf60, the full-registry sweep, and the third site) all call
   `analyser->vt[+0x40]`; a NON-NULL result routes to the fallback's
   vtable slot 0 with the full argument set (the candidate buffer + the
   result object), and the primary's `vt[+0x10]` runs only as the 2-arg
   second chance.
2. **But the fallback consult requires the node's class to OVERRIDE
   +0x40 with the real getter.** The real getter is FUN_180013700
   (`lea rax,[rcx+0x10]` — returns the +0x10 subobject). A full .rdata
   scan of every vtable-like run (120 runs of ≥9 slots) finds exactly 16
   vtables with a non-stub +0x40 — 8 of them node classes with
   FUN_180013700. **The X3F node's vtable (0x18006cbc0) is NOT among
   them: its +0x40 slot is 0x18000bae0 = `xor eax,eax; ret`, byte-exact
   from the PE. The DMG node (0x18006cd90, the M-090 precedent the lanes
   leaned on) carries the same stub.**
3. The classes that DO override +0x40 use a DIFFERENT registration
   shape: an EMBEDDED fallback vtable stored directly at node+0x10 (e.g.
   the 0xd302020 node at 0x1800034f3-0x180003533: `node->vt =
   0x18006c2d0; node->[0x10] = 0x18006c288` — a vtable address, not a
   child pointer). The X3F/DMG shape stores a separately-allocated child
   POINTER at +0x10 and does NOT override the getter — the dispatcher
   never consults their children through the +0x40 mechanism.

**Conclusion:** the secondary-first mechanism exists but is a property of
8 specific node classes; the X3F node is not one of them. The prior lanes'
"the Brance count-walk never contributes" does not follow from the
dispatcher — it was over-generalized from a mechanism that does not apply.

## 4. What the dispatcher ACTUALLY does for an FOVb candidate (verified)

FUN_18000cf60 (the candidate→analyser lookup, called from the invocation
core at 0x18000cadc): (a) an extension-path layer (string building,
0x18000cfe5-0x18000d1c6); (b) **the magic-key map path
(0x18000d1e4-0x18000d2c9): the RB-tree lookup over [registry+0x10] → the
map node's value at +0x28 = the MODERN node → node->vt[+0x40]() → the
stub returns 0 → the node's OWN vt[+0x10] (FUN_180040080, the binary
version gate) with (buf, result) → on acceptance the hit is recorded
(FUN_18000d920) and the MODERN NODE is returned**; (c) a further layer
(FUN_18000d3e0); (d) the full-registry sweep (0x18000d2f3-0x18000d391:
iterates the registry's list, each entry's +0x40 then +0x10).

The invocation core (0x18000ca8d-0x18000ccfc) then: the returned
analyser's factory (+0x38) → the walker (FUN_1800400a0 for the modern
node: the 0x120 object) → the walker's vt[+0x08] direct-walk check
(FUN_180012540 = `return 1` for the modern walker — a no-op accept) →
the walker is registered into the scan's active-walker list
(0x18000cb40-0x18000cb84) with its format id, base, and node back-pointer
— **the registration exists to feed it: the walker's +0x20
(FUN_180040160) is a per-chunk state machine whose return 2 = "feed me
more" and return 1 = "walk complete"** (verified in FUN_180040160's
control flow: 0x18000d1c0/0x18000d1de return 2 while scanning,
0x180004069 returns 1 on completion).

**The wired path for a real Sigma file (binary version): the magic-key
map lookup returns the modern node → its binary tier-1 passes → the
factory creates the count-walker → the walker registers for the feeding
protocol → FUN_1800404a0's count-walk computes the extent.** The legacy
child is reachable only through layers not fully decoded this session
(the registry's map/list projections — the singleton at 0x1801181a0
holds six containers; which objects the +0x00/+0x10/+0x18 views hold
after the merge step was not traced to completion).

## 5. The two live bugs the prior landings left (M-156's substance)

1. **The ASCII version gate rejects every real Sigma X3F file.** The
   landed code gated `h[4] ∈ ['0'..'9'] && h[5] == '.'` — real files
   carry binary u32 versions (0x00020001 = bytes 01 00 02 00 → h[4] =
   0x01, not an ASCII digit) → REJECTED. The og's net acceptance across
   the chain is "readable header" (the modern's binary gate accepts real
   files; the legacy's permissive tier-1 accepts everything else; the
   registry keys on FOVb alone). Verified: FUN_180040080 accepts exactly
   the binary shape; FUN_180053590 accepts any.
2. **The modern count-walk never ran for any file** under the
   legacy-only landing — while the magic-key path provably wires it for
   binary-version files (§4). The count-table extent (SECd + 16 +
   count\*12) is the modern generation's output for exactly the
   real-file population.

## 6. The correction (M-156) and its honest limits

`validate_x3f` now routes by version shape: binary → the modern
count-table walk; anything else → the legacy fixed-tail walk (the prior
landings' semantics, preserved); torn → the legacy walked-prefix salvage.
The version gate is gone (the union acceptance). The row bound carries
the og walker's own cap constant (0x6400000).

**Open questions (registered in M-156, honestly):**
(a) the engine's feed loop that drives walker->vt[+0x20] was not located
(the +0x20 state machine's protocol is the evidence it exists; the
walkers-are-registered-to-be-fed inference is strong but the caller was
not found); (b) the deeper dispatch layers (the container merge, the
sweep list's membership — nodes vs children) were not decoded to
completion; (c) consequently: for a binary-version file whose directory
is present, the og emits EITHER the count-walk extent OR the legacy
0x4C extent — the wiring evidence (§4) favors the count-walk, three
convergent prior lanes favor the legacy constant, and this correction
lands the wiring reading with the disagreement recorded. The next
session's decisive probe: locate the +0x20 feed caller in the engine
(search the CRAWSearchController's scan loop for the indirect +0x20
dispatch on the active-walker list entries).
