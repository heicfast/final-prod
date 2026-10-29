
---

## Session-87 addendum (the S87-continuation lane's WPD convergence record)

This lane independently decoded and landed the WPD member in the same
window as the racing S84/89 lane's landing (the concession seat per
SA-9 — their earlier push holds it). The two decodes AGREE on every
material point: the three-pair tier-1 {(0x01020a01, 0x00050200)→0x44,
(0x00021601, 0x00030000)→0x45, (0x00010f0f, 0x00030000)→0x3d}, the
extent = u32@+0x14 (the WPC common header's own file-size field), the
unknown-pair decline, the +4 product-word gate's retirement, and the
0xFFFFFFFF row max (the og's registry node carries no min/max). The
convergent corroboration this lane adds: the raw .rdata dump of the
36-byte table at 0x18006ef40 (the byte-level ground truth behind the
pair list) and the engine's extent!=0 acceptance gate at
0x18000d7e1 (the zero-size decline — the racing landing's `end <= hit`
check implements the same semantic). This lane's five-test battery
rides as convergent verification on top of the racing lane's two-test
proof: WpdVersionTableWalkSetsExtent / WpdOverCapSizeCarvesFull (the
72 MiB over-cap carve — the truncation class the finding filed) /
WpdVersionTableGateDeclinesUnknownVersion / WpdZeroSizeFieldDeclines /
WpdSecondAndThirdRecordsCarve (the records-2/3 variants carve through
the same walk). The member count after both windows: members 1-8 of
11 landed (X3F, CRW, DWG gen-1+2, PSP, WPG, WPD, DXF, EPS, AI);
remaining: SIT, APE, SWF-RECT + the DWG gen-3 R2007 remainder.

---

## Session-90 addendum (the S90 lane's final convergence record — the WPG tier-1 corroboration)

**Two factual corrections to this finding's own text (this lane's re-read;
the 0x44c CSL fold is already landed above as M-112 — the enumeration stands
at twelve member sites + M-112's):** (a) AI **has its own validator**
(`validate_ai` in cr_analysers.cpp, FMT_AI row in cr_registry.cpp) — the
finding's "no separate AI validator exists" parenthetical is wrong (the
stand-in substance holds: the validator's size walk is the DWG-pattern
bounded extent, not the og's); (b) the EPS/AI stand-ins are **16 MiB**, not
the title's blanket 64 MiB — already so at the filing baseline (verified via
the filing commit's tree). Cosmetic to the finding's own text; recorded so
the fix wave does not re-derive the wrong bounds.
---

## Session-43 addendum (2026-10-29 — the N-146 re-homing: the FH11 site joins this enumeration)

**N-146 resolved the one un-owned stand-in this finding's enumerations missed:** the
FH11 row's zero-end stand-in (`cr_analysers.cpp`'s `validate_fh11`, `end = hit +
(32ull << 20)` — the S-037-era landing, its owner purged with the S41 sweep) is now
REGISTERED here: the twelfth site, a DIFFERENT stand-in class than the eleven batch-1
members (the og's FUN_180029f30 :32612-32633 stamps the CZeroEndRawFileInfo id 0x1d
with NO size walk — the pending-object engine's zero-end metric carries the extent,
the D-101-registered architecture decline; the 32 MiB cap is the zero-end family's
salvage posture, not the batch-1 64 MiB DWG pattern). The constant is PINNED
(`Carve.Fh11BoundedExtentPinned` — the exact 32 MiB extent on an unclamped 40 MiB
device; the pin's fail-before proven by mutation: 32→16 MiB goes RED). The site's
disposition: stand-in stays (the D-101 decline covers it); the registration gap that
was N-146's actual defect is closed.

---

## Session-78 fix-lane addendum (2026-10-29 — the per-format landings begin; X3F landed — **SUPERSEDED-BY-S71 for the X3F member**: this lane's count-walk decode read FUN_1800404a0, the Brance::X3FAnalyser streaming chunk callback — the wrong authority; the S71 lane's re-decode of FileX3FAnalyser FUN_1800535b0 (the File*Analyser secondary, the M-090 DMG precedent's established size-walk authority) is the corpus-true walk: SECd + 0x4C in all three found-branches, loop-exit salvage, the count never read. The M-155 repair resolved the ed501caa push's committed conflict markers in this lane's favor; this lane's three X3F tests were replaced by corpus-true pins.)

**Per-format status (the fix plan's landing-order record):**
1. **X3F — LANDED (this push, S78 lane):** the section-directory walk
   (Brance::X3FAnalyser, RAWSearchDLL FUN_1800404a0 :48851-48910 + the
   magic finder FUN_18003ffc0 :48624-48638, both re-read first-hand this
   session): the directory is SEARCHED for ("SECd" = 0x64434553, the same
   literal the walk's chunk-tail probe compares — Sigma writes the
   directory at a variable position after the section data); the entry
   count at magic+8; the extent = magic + 16 + count×12 (the end of the
   12-byte-stride entry table behind the 16-byte directory header; the
   caller's entry[0] reach-through at :48741/:48770 corroborates the
   layout), clamped by the row max + device end (the og's own clamp at
   :48904-48906/:48745-48750). No directory in the window = the og's
   decline (the return-1 family) — the blind 64 MiB stand-in is gone.
   The og's streaming chunk-tail gate is its chunked-feeding model's
   artifact (documented divergence of model, not semantics). Tests:
   X3fSectionDirectoryWalkSetsExtent / X3fNoSectionDirectoryDeclines /
   X3fGarbageHugeCountClampsToWindow — RED first-hand via the stash
   dance (2/3 FAILED pre-fix: the stand-in's blind extent + blind
   carve); the S-007 batch re-pinned (the plant now carries the real
   directory; the compact payload keeps the batch slot). The FileRepair
   artifact's directory-first grammar corroborated
   (x3f-repair--section-checkers-walk.md §2).
2. **CRW — NEXT** (FUN_180052160, the CIFF heap chain).
3. DWG, then WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT — pending.

---

## Session-71 addendum — the X3F landing (the finding's own decode-then-land order, member 1 of 11)

**X3F FIXED** (this session; the remaining ten members stay with the finding): the og's X3F size walk is decoded first-hand from FileX3FAnalyser FUN_1800535b0 (RAWSearchDLL :63457-63560, read in full this session): a forward scan for the "SECd" section-directory marker (0x64434553), sector-stepped with overlapping 2-sector windows up to the device cap, with the extent = SECd_position + 0x4C in ALL THREE found-branch shapes (the sector-aligned hit `uVar7 += 0x4c; break;`, and both in-buffer search hits' `piVar5 + ... + 0x4c` forms — the directory's fixed 76-byte tail, which by construction IS the file's end: the X3F writer appends the directory last, corroborated by the FileRepair artifact x3f-repair--section-checkers-walk.md's directory-first repair model). The core's landing: `validate_x3f` scans 256 KiB windows (3-byte overlap — the straddle guard the og's 2-sector windows serve) from the header to the probe limit; found → `SECd + 0x4C` (clamped by the row bound); torn (no SECd in range) → the walked prefix at the row cap with the reduced confidence class (the og's loop-exit salvage shape — the scan stops at its cap with the last offset, never a refusal). The 64 MiB DWG-pattern stand-in and its "recorded follow-up" comment are GONE for X3F. Tests: `Carve.X3fSectionDirectoryWalkCarvesTrueEnd` (a 1 MiB true end on a 100 MiB device — the stand-in carved 64 MiB; RED by construction pre-fix) + `Carve.X3fTornSalvagesTheWalkedPrefix` (the loop-exit salvage at the row cap). Suite 1570/1570. Remaining members: CRW (FUN_180052160, the heap chain) next per the finding's order, then DWG/WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT.

---

## Session-80 addendum (this lane) — the dispatcher decode: the authority dispute settled first-hand (M-155's resolution CONFIRMED; the census's "modern first" prose corrected)

This lane opened at `04fb5e88` with main RED (the ed501caa markers — N-217) and
independently re-read BOTH og sites before touching anything: FUN_180040160
(the Brance::X3FAnalyser chunk processor, vftable-proven at RAWSearchDLL:48700)
with its walk core FUN_1800404a0 (:48851-48910, extent = SECd + 16 + count×12,
clamped, return-1 decline), and FUN_1800535b0 (FileX3FAnalyser :63457-63560,
extent = SECd + 0x4C in all three found-branches, loop-exit walked-prefix
salvage, the id 0x6C stamped unconditionally). An initial hypothesis — that the
registry census's two-generation prose ("the modern analyser first, the legacy
fallback") made the Brance count-walk the primary authority — was CORRECTED by
the dispatcher decode before any push:

**The scan dispatcher (RAWSearchDLL :9850-9876, the magic-key path; the same
shape in the full-registry sweep at :9900-9935) calls the SECONDARY first.**
The map node's registered analyser (`plVar7` = the Brance object) is queried
via `vtbl[+0x40]` (the GetFallback accessor returning the +0x10 secondary —
the FileX3FAnalyser, per the dual registration at :1786-1806); when a
secondary exists, the dispatcher invokes **the secondary's slot-0 method with
the candidate buffer AND the result object** (`(**(code**)*puVar4)(puVar4,
param_2, param_3, ...)` — the result object being the +0x10-size/+0x18-id
shape FUN_1800535b0 writes), and emits on its acceptance; the Brance primary
runs only as a 2-arg second-chance (`vtbl[+0x10](plVar7, param_2)`) that
emits with a null analyser. Since FileX3FAnalyser never declines (FUN_1800535b0
stamps the extent + id unconditionally at :63556-63558), **the Brance
count-walk never contributes to a carved X3F extent** — M-155's resolution
(the S71 FileX3FAnalyser walk kept, the S78 count-walk superseded) is the
corpus-true semantics, now confirmed by the dispatch decode rather than by
precedent alone. The analyser-registry census's "modern first, legacy
fallback" prose is CONTRADICTED by the actual dispatch order — recorded here
as a census erratum (the prose was an inference from the registration
structure, not a decode of the dispatch).

Residual model divergences (documented, deliberate): (a) the core's 256 KiB
windowed search vs the og's sector-stepped 2-sector windows — every found is
the og's in-buffer shape; the og's sector-aligned-tail branch stamps
`uVar7 + 0x4C` where its own arithmetic would give `uVar7 + sector − 4 +
0x4C` (an og quirk the window model never reproduces); (b) the og's walk cap
is `*(FUN_180001350() + 0x70) >> 1` (the singleton's undecoded field — the
halved bound), approximated by the row-max probe limit. X3F's member status
stands as LANDED per the S71/M-155 resolution; the remaining ten members (CRW
next) stay with this finding. N-217 carries the build-repair record.

---

## Session-83 addendum — the X3F member's FOURTH landing (M-156: the ASCII gate's live bug + the dispatcher-mechanism refutation + the chain router)

The S82-close tree still carried two defects on the X3F surface that the
four-lane arbitration left behind (filed + fixed this session as
**M-156**, findings/FIXED/): (1) **the ASCII version gate rejects every
real Sigma X3F file** — real files carry a BINARY u32 version
(0x00020001), the gate demanded an ASCII digit + point, so real RAWs
never carved at all (the og's chain never jointly rejects a readable
FOVb: the modern tier-1 FUN_180040080 accepts exactly the binary shape,
the legacy tier-1 FUN_180053590 accepts any readable header); (2) **the
N-217 dispatcher mechanism does not fire for the X3F node** — the
secondary-first consult requires the node's class to override vtable
+0x40 with the real getter (FUN_180013700), and a full .rdata scan finds
exactly 8 node classes that do; the X3F node (vtable 0x18006cbc0) and
the DMG node (0x18006cd90 — the M-090 precedent) both carry the return-0
STUB at +0x40, byte-exact from the PE. The magic-key dispatch path
(FUN_18000cf60's map route, 0x18000d1e4-0x18000d2c9) provably wires the
MODERN path for binary-version files: the node's own +0x10 binary gate
accepts, the invocation core calls the factory (+0x38 → the 0x120-byte
walker, cap 0x6400000), and the walker registers for the feeding
protocol (+0x20 = FUN_180040160's return-2/return-1 state machine →
FUN_1800404a0's count-walk, extent = SECd + 16 + count*12, decline on
no-directory). The full decode, instruction-address-cited:
`evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md`.

The S83 landing: the version gate is GONE (the union acceptance); the
version shape routes the generation (binary → the modern count-table
walk; anything else → the legacy fixed-tail walk — the four-lane
convergent semantics preserved for the non-binary population; torn → the
legacy walked-prefix salvage); the row bound carries the og walker's own
cap constant (0x6400000 = 100 MiB, raw-asm at 0x1800400ee — the 64 MiB
stand-in value leaves the row). Tests: the five legacy-path pins (all
ASCII plants) stand unchanged; five new pins cover the modern path and
the router — X3fModernCountWalkSetsExtent /
X3fModernGarbageHugeCountClampsToBound /
X3fModernTornFallsBackToLegacySalvage / X3fChainRoutesByVersionShape /
X3fRealSigmaBinaryVersionCarves (the headline RED-at-open pin: the
binary-version plant does not carve under the ASCII gate). The prior
lanes' Ghidra line cites are recorded as unverifiable-from-corpus (the
decompile is CI-generated, not committed) — this addendum's cites are
instruction addresses against the corpus binary itself.

The honest residue (M-156's open questions, named with their decisive
next probe): the engine's +0x20 feed-loop caller was not located; the
registry's container merge (the sweep list's membership) is undecoded —
so for a binary-version file with a directory the og emits EITHER the
count-walk extent OR the legacy constant; the wiring evidence favors the
count-walk (this landing), three convergent prior lanes favor the
constant (the prior). The disagreement is recorded, not papered over.
**X3F's member status: LANDED (the chain, fourth landing); the remaining
ten members (CRW's CIFF heap chain next) stay with this finding.**

---

## Session-83 addendum 2 — the CRW member LANDED (member 2 of 11: the CIFF heap size walk)

**CRW FIXED (this session; the remaining nine members stay with the
finding).** The walk decoded first-hand from the raw asm
(objdump over RAWSearchDLL.dll; evidence:
`evidence/s83-lane-sheets/s83-l2-crw-ciff-walk-decode.md`): the CRW
child's vtable 0x18006e008 slot +0x08 = FUN_180052150 → FUN_180052160
(BOTH the walker's table-builder AND the walk — the prior lanes' cite
pointed here). The walk: sector-stepped from the file start with the
walker's OWN cap **0x3200000 = 50 MiB** (raw-asm 0x18005219d);
non-zero-tailed sectors skipped (the streaming chunk-tail gate — the
registered model-divergence class); zero-tailed sectors searched for CIFF
directory entries **{u16 type == 0x2807, u32 length ∈ {0xa8, 0xb2,
0xe4}}** (the three 6-byte signatures 07 28 XX 00 00 00); a hit stamps
**extent = entry_position + TAIL[length]** with the walker's own table
**{0xa8 → 0x46, 0xb2 → 0x3c, 0xe4 → 0x32}** (the map built at
0x1800521dc-0x18005234b); no entry anywhere → the loop-exit stamps the
CAP itself (never a refusal — the same salvage class as X3F's legacy
walk).

The landing: `validate_crw` keeps the M-081 grammar gates (verified
byte-exact against FUN_180052000) and replaces the blind 64 MiB
stand-in with the walk (the windowed entry search with the straddle
overlap; the hit stamps entry + TAIL; torn → the walked bound at reduced
confidence). Both registry rows (II and MM) carry the og walker's OWN
cap constant (0x3200000 — the 64 MiB stand-in value gone). RED→GREEN
first-hand via the stash dance: `CrwDirectoryEntryWalkSetsExtent` and
`CrwWalkCapIsTheOgsOwnConstant` both RED at the stand-in tree (the
stand-in carved 64 MiB / the entry walk did not exist), GREEN with the
walk; the 174-test carve suite green. Tests: CrwDirectoryEntry-
WalkSetsExtent (all three tail constants pinned) /
CrwTornSalvagesTheWalkedBound / CrwWalkCapIsTheOgsOwnConstant. The
S-007 batch's CRW plant (header-only) exercises the torn-salvage path —
its ≥ assertion holds unchanged. Remaining members: DWG (the
section-locator map) next per the fix plan's order, then
WPG/WPD/PSP/SIT/APE/EPS/DXF/SWF-RECT.


---

## Session-89 addendum (renumbered from the born-collided S86 seat — the racing window re-homed S79→S86 with the earlier claim) — this lane's DWG landing CONCEDED to the racing S85 window per SA-9; the two riders carried forward (2026-10-29/30)

**The seat conceded.** This lane decoded the DWG walk independently
(FUN_18002f320, the CDWGRawInfo feed, RAWSearchDLL_decompiled.c:
36303-36461 + the raw asm 0x18002f3a7-0x18002f480, objdump over
og-codebase/RAWSearchDLL.dll) and landed it unpushed (ab5bbb24) while
the racing S85 window landed the same member (ddab68b4 + the f7e49eaf
gen-2 struct-offset repair). Per SA-9 the racing seat stands; the
convergent findings on both sides:

- AGREED (both decodes): the three-generation state machine keyed by
  the 6-byte version; gen-1 = the section-locator walk (u32 count at
  file+0x15 — the birth cursor 0x15 IS the row floor the factory wrote
  into the walk-state slot; 9-byte records {u8 id, u32 off@+1, u32
  size@+5} at +0x19; the id-sequence gate; the monotonic max of
  off+size; the torn walked-prefix salvage). The racing decode went
  FURTHER: gen-2 (the rand-LCG struct at +0x80 bytes, the six gates,
  extent = u64@0x34 + 0x6c) landed where this lane recorded it as the
  follow-up — the racing seat's superiority is the concession's own
  justification.
- THIS LANE'S UNIQUE CONTRIBUTIONS (the riders, landed separately this
  session): (1) the ROW FLOOR — the racing landing kept min 64 (the
  S-007 superset); the og's registered floor is 0x15 = 21 (the factory
  FUN_18002ff20's qword write at +0x110 — puVar1[0x22] = 0x15, the same
  write that seeds the walk cursor; the PNG/ONE ctor pattern
  [0x22]=min/[0x23]=max), and the gen-1 walk's residual 512-byte floor
  (inherited from the stand-in era) contradicts it; (2) the
  ID-SEQUENCE GATE — the racing landing compares rec[0] != u8(i)
  (truncated); the og compares the byte against the FULL 32-bit counter
  (cmp %ecx,%eax at 0x18002f40a, eax = movzbl of the id, ecx = the
  running counter) so the walk SELF-LIMITS at 256 records (no byte can
  match counter >= 256) — a count>256 fixture with wrapped ids is
  accepted by the racing port and refused by the og. Both riders:
  RED at the racing tree (DwgRowCarriesTheOgsRegisteredFloor — the
  21-byte walked extent refused by the 512 floor AND the min-64 row;
  DwgWalkSelfLimitsAt256Records — the 260-record wrapped-id fixture
  carved), GREEN with the riders.

---


---

- **The tier-1 is the EXACT 32-byte signature** (memcmp 0x20):
  "Paint Shop Pro Image File" + 0x0a + 0x1a + 5 NULs — the core's old
  16-byte prefix accepted files the og refuses (a discovered defect,
  pinned by PspExactSignatureRequired).
- **The walk**: from 0x24, each block is {u32 magic 0x004b427e
  ("~BK\0"), u32 length @ +6}; the position advances 10 + length; the
  extent = the walked END — the first non-~BK block, the 20 MB row
  bound, or the device end all stop the walk there (never a refusal
  once the signature held; the torn straddle salvages the walked end).

RED→GREEN via the stash dance: PspBkBlockChainWalkSetsExtent +
PspExactSignatureRequired both RED at the stand-in validator (the blind
extent; the short-signature acceptance), GREEN with the walk. The
S-007 batch-1 PSP plant re-pinned (the og's exact 32-byte signature +
a real two-block chain; the old 26-byte plant lacked the 0x0a/0x1a).
The carve suite 183/183. Remaining members: WPG, WPD, SIT, APE, EPS,
DXF, SWF-RECT (seven) + the DWG gen-3 remainder.

---

## Session-85 addendum 3 — the WPG member LANDED (member 5 of 11: the zero-tail walk)

**WPG FIXED (this session; six members remain).** The chain decoded
first-hand: the "wpg\0" tier-1 FUN_180014030 (:15660-15683, id 0x45),
the factory FUN_180014090 (the 0x120 WPGRawFileInfo child: min 20, max
100 MB — already in the row), and the child's +0x20 processor
FUN_180014160 (raw asm 0x180014160-0x1800142b0): **extent = chunk
offset + zero_tail(chunk)**, returning 1 (done) after the first walked
chunk. The zero-tail walk is FUN_180042b30 (:50703-50750): a binary
search for the FIRST position whose remainder is all zeros (+1); the
whole buffer zero → 0 (the decline class); no zeros → the buffer
length. The chunk length the driver passes is the DEVICE SECTOR SIZE
(ctx+0xF0, the storage device's sector query at :9192-9195, default
0x200; the +0x20 dispatch at 0x18000d7ae).

The model translation (the registered divergence class, the CRW/X3F
stance): the core stamps the trailing-zero-run position of the hit's
own 512-byte window — the og's semantic (the extent ends at the file's
last non-zero byte within the first sector window) without the
streaming artifacts. **The deferral residue, recorded:** the
processor's gate `[result+0x14] >= 0x10` (the result-object field is
undecoded — the same field gates GZ at >= 8 and the third zero-tail
caller at >= 8) — if it defers the walk to a later sector, the extent
shifts by multiples of 512; the landing takes the first window (the
gate's most coherent reading: the candidate's available bytes in the
feed). A future decode of the result object's +0x14 field settles the
window position; the walk itself and the extent formula are
byte-exact.

RED→GREEN via the stash dance: all three tests RED at the stand-in
(WpgZeroTailWalkSetsExtent / WpgZeroTailFullWindowIsTheBoundary /
WpgAllZeroWindowDeclines — the stand-in's blind 64 MiB and its
all-zero acceptance), GREEN with the walk. The carve suite 186/186.
Remaining: WPD, SIT, APE, EPS, DXF, SWF-RECT (six) + the DWG gen-3
remainder.

---

## Session-84 convergence record (the DWG member's convergent repair — the gen-2 struct offset)

The DWG member was decoded and landed CONVERGENTLY by two lanes in the
same window: the racing S85 lane's landing (8272daf2 — the seat, the
structure: the gen-1 locator walk, the gen-2 rand-XOR struct walk, the
gen-3 recorded remainder, the version-table correction
{12,13,14,15,18,21,24,27,32}) and this lane's S84 decode (evidence:
`evidence/s84-lane-sheets/s84-l1-dwg-walk-decode.md` — the same
three-generation model, the same factory cursor 0x15 at 0x18002ff59, the
same keystream's first byte 41). **The one material delta, arbitrated
by the raw asm: the gen-2 encrypted struct's offset.** The landing read
`hit + 0x20`; the asm at 0x18002f5a0 is `lea rdx,[rdi+0x80]` with rdi =
the chunk base (set once at 0x18002f342, never reloaded — the phantom
`mov edi,0x6c428d44` in naive disassembly is the tail byte of
`lea rcx,[rbp-0x41]` at 0x18002f593, a mid-instruction start). The Ghidra
decompile's `param_2 + 0x20` is INT* arithmetic = **0x80 BYTES** — the
landing ported the unscaled constant. Repaired: the read, both test
plants, and this finding's gen-2 bullet now carry +0x80 with the asm
citation. The offset matches the real R2004 format (the section page
map at 0x80) — the convergent external corroboration.

---

## Session-84 addendum 4 — the WPD member LANDED (member 6 of 11: the pair-table tier-1 + the declared size)

**WPD's walk decoded first-hand from the raw asm** (evidence:
`evidence/s84-lane-sheets/s84-l2-wpd-pair-walk-decode.md`). The
analyser: **FileWPDAnalyser** (the registry key 0x435057ff 'FF WPC', the
node vtable 0x18006cff0 idx 0x44; the +0x10 slot FUN_180041f50 = the
File*Analyser one-function shape: tier-1 + size).

- **The tier-1 pair table**: (u32@+8, u32@+0xE) ∈ {(0x01020a01,
  0x00050200), (0x00021601, 0x00030000), (0x00010f0f, 0x00030000)} —
  the three compare groups at 0x180041f8a/0x180041fa8/0x180041fc7
  (the table at 0x18006ef40); an unknown pair returns 0 — the DECLINE
  (0x180041ff0). The S-007-era 'nonzero product word at +4' gate was
  invented (the og never reads +4 as a gate) — GONE.
- **The size**: extent = u32@+0x14 (0x18004200c: `mov ecx,[r9+0x14]`
  → the result's +0x10) — the declared document size. Never a walk,
  never a refusal once the pair held.
- (The adjacent vtable 0x18006cfa0's 0x180041eb0 walk is
  FileRIFFAnalyser's under the 'RIFF' key — NOT WPD; the attribution
  resolved by the registry leas.)

RED→GREEN first-hand: WpdPairTableDeclaredSizeSetsExtent RED at the
stand-in tree (the 64 MiB device-end extent) → GREEN (the declared
size); WpdUnknownPairDeclines pins the decline. The S-007 batch's
plant re-pinned. The carve suite 288/288. **WPD's member status:
LANDED (members 1-6 of 11; remaining: SIT APE EPS AI DXF SWF-RECT).**

---

## Session-85 addendum 4 — THREE members LANDED (DXF, EPS, AI: members 6, 7, 8 of 11)

**DXF FIXED (member 6).** The walk decoded first-hand: the
CDXFRawFileInfo child's +0x20 processor FUN_1800379c0 (:42395-42432)
— the line ending from byte[3] ('\n' → the LF pattern "  0\nEOF\n"
(8B), else the CRLF form "  0\r\nEOF\r\n" (10B), built from the
qword/short stores at :42397-42404); FUN_180042ab0's backward pattern
search per chunk (returns past-the-pattern); the extent ends past the
FIRST trailer; **the walk cap is the RAW singleton field 0xbebc200 =
200 MB** (:42426-42431, no multiplier — unlike DWG's ×100); no trailer
→ the walked bound (the og's extent += len accumulation). The row max
now carries 0xbebc200.

**EPS FIXED (member 7) — with a DISCOVERED DIVERGENCE.** The og's
tier-1 (FUN_180032b50 :38746-38762) accepts ONLY "%!PS-Adobe-"
headers (the qword "%!PS-Ado" + short "be" + '-'): **a bare "%!PS"
file RETURNS 0 — the og declines it.** The pre-fix core's 4-byte gate
accepted every "%!PS" — the discovered defect, pinned in both
directions (EpsAdobeCovenantRequired). The Adobe-key child is a
CZeroEndRawFileInfo (FUN_180032c50, cap singleton+0x70 × 100 = 20 GB);
its walk's "%%EOF" backward search (the general-ctx arm :41446-41449):
extent = past "%%EOF" + 2 (the DSC trailer's EOL); torn → the walked
bound. The 0x20-ctx arm's -0x10 form (:41439-41442) is the recorded
residue (the ctx-window variant, undecoded).

**AI FIXED (member 8) — with a DISCOVERED DIVERGENCE.** The og's AI
arm (FUN_180032b50 :38765-38790) is a HEADER FORMULA: extent = max of
the three (start + length) pairs — (dw@4 + dw@8), (dw@0xC + dw@0x10),
(dw@0x14 + dw@0x18) — capped at singleton+0x70 × 100 = 20 GB; the
factory's non-Adobe path returns the PLAIN base child (no walk
refinement) — the tier-1's formula IS the size. **The pre-fix core's
"version word 1..64" gate was a stand-in-era invention** (dw@4 is a
block START in the formula, not a version) — dropped; the S-007 noise
plant re-pinned to the genuinely-degenerate all-zero form (the
"version 999" plant is an ACCEPT under the corpus-true semantics).

RED→GREEN via the stash dance: five of the seven new tests RED at the
stand-in tree (DxfEofTrailerWalkSetsExtent / DxfLfFormUsesTheLfTrailer
/ EpsAdobeCovenantRequired / EpsEofTrailerWalkSetsExtent /
AiHeaderFormulaWalkSetsExtent), GREEN with the walks; the torn and
degenerate arms pin the salvage/decline semantics. The carve suite
193/193. Remaining: WPD, SIT, APE (three) + the DWG gen-3 remainder.

### The riders landed (the S86 rider commit, this session)

Rider A (the row floor): the row min 64 -> 0x15 (21), the gen-1 walk's
residual 512-byte floor removed (cr_analysers.cpp's gen-1 tail +
cr_registry.cpp's row). Rider B (the id gate): `rec[0] != u8(i)` ->
`u32(rec[0]) != i` (the full 32-bit counter; the walk self-limits at 256
records). RED→GREEN at the pre-rider tree, the stash dance: rider A's
`DwgRowCarriesTheOgsRegisteredFloor` RED (the 21-byte walked extent
refused by the 512 floor AND the min-64 row), rider B's
`DwgWalkSelfLimitsAt256Records` RED with the de-confounded fixture (300
records, each end 0x900 >= 512 so the 512 floor never refuses - the
first RED attempt passed at the pre-rider tree for the WRONG reason,
the 320-byte extent tripping the 512 floor; the fixture grew to 2304
per-record ends so the counter gate is the ONLY refusal under test).
The racing landing's own six DWG tests stay green under the riders (8/8
at the landed tree). Corpus: FUN_18002f320's loop 0x18002f400-0x18002f441
(cmp %ecx,%eax at 0x18002f40a) + FUN_18002ff20's +0x110 qword write.

---

## Session-91 addendum — the WPD member LANDED (member 6 of 11: the version-table + declared-size contract)

**CONCEDED to the racing windows per SA-9 (earlier-push-wins):** the
racing windows' convergent WPD landings (the same decode: the same three-pair
table, the same u32@0x14 extent, the same unknown-pair decline — an
independent confirmation of this lane's raw-asm read) pushed first and
keeps the seat; this lane's WPD validator/tests withdrew at the merge.
This lane's riders kept: the two directions the racing battery lacks —
WpdZeroSizeFieldDeclines (the zero-size generic-child decline) and
WpdZeroProductWordWithGoodPairCarves (the invented product-word gate's
removal pin) — plus this lane's evidence sheet
(evidence/s91-lane-sheets/s91-l1-wpd-version-table-decode.md) whose
decode of FUN_1800264d0 independently re-verified the landed S-042 WMF
grammar byte-for-byte and corrected the census's 0x90001/0x9ac6cdd7
grouping (the WMF keys, not "WordPerfect"/"thumbnail cache").

**The decode record (this lane's independent read, converging with the
racing landing):** the tier-1 decoded
first-hand from the raw asm (objdump over RAWSearchDLL.dll; evidence:
`evidence/s91-lane-sheets/s91-l1-wpd-version-table-decode.md`):
the "\xFFWPC" registry binding (0x180003ccb-0x180003cde, the pipelined
cascade calibrated against the FOVb/X3F and II/CRW anchors) maps to the
node {vtable 0x18006cff0, idx 0x44} whose +0x40 is the return-0 stub —
no secondary consult, the node's own +0x10 (FUN_180041f50) is the size
authority. The grammar: a THREE-entry version table gates the file —
the pair (u32@8, u32@0xe) must be one of
{(0x01020a01, 0x00050200)→id 0x44, (0x00021601, 0x00030000)→id 0x45,
(0x00010f0f, 0x00030000)→id 0x3d} — and on match the extent is the
header-declared **u32@0x14 verbatim** (0x18004200c-0x180042015). A
zero size field routes to the generic pending child whose walk
declines (0x18000b760: `xor al,al; ret`) — no carve.

**The member's own premise corrected by the decode:** this finding's
"What the reference does" prose ("WPD gates on the product-version
word and walks the object header chain") was the audit's hypothesis —
the og gates on the version PAIR at (+8, +0xe) (never reading the
product word at +4) and takes the size from a direct header field (no
chain walk). The corpus wins; the landing follows the decode.

**A census erratum corrected in the same decode:** the
analyser-registry census's row grouping "\xFFWPC / D7 CD C6 9A →
FileWPDAnalyser (WordPerfect)" is half wrong — the 0x9ac6cdd7
(Aldus placeable WMF) and 0x00090001 (bare standard WMF: type=1 +
headersize=9 as the KEY) keys map to a different node {vtable
0x18006b200, idx 0x46} whose tier-1 (FUN_1800264d0) is the WMF
grammar — decoded this session and matching the core's landed S-042
validators byte-for-byte (an independent re-verification of that
landing). The census's "01 00 09 00 → Windows thumbnail cache" label
is wrong for the same reason.

The landing: `validate_wpd` gates on the og's three-entry version-pair
table, requires the declared size nonzero, and takes the extent =
u32@0x14 clamped by the row bound + device end (probe_limit — the
X3F/CRW registered approximation posture for the og's undecoded
chain-row caps; the og's three chain ids 0x44/0x45/0x3d share the
core's single FMT_WPD row). The old "zero product word" gate is GONE
(it existed nowhere in the og's chain). RED→GREEN first-hand via the
stash dance: WpdVersionTableSetsExtentFromTheDeclaredSize (the
stand-in carved the wrong extent) + WpdZeroProductWordWithGoodPairCarves
(the invented gate declined a real version-pair file) + the re-pinned
S-007 batch plant (the old filler-byte plant never carried a real
pair) all RED at the stand-in tree (3 FAILED), GREEN with the
contract; WpdUnknownVersionPairDeclines + WpdZeroSizeFieldDeclines pin
the decline directions. The 190-test carve suite green.

---

This lane's third WPD/WPG window (the racing S90 concession lane's record, absorbed at the S91 merge): the independent decode converged with
BOTH racing landings (the S84 d20a8483 pair-table seat and the S87
1ff8ae24 version-table walk + 0xFFFFFFFF row max — all three decodes
agree on every material point; the row-max rider this lane built at
4 GiB is conceded to the S87 lane's earlier-pushed 0xFFFFFFFF, the more
exact u32 ceiling). This lane's one UNIQUE record, the corroboration no
other window carried: **the WPG tier-1 decode closes the S85 zero-tail
landing's own honest residue.** Byte-exact (FUN_180014030 @ 0x180014030,
objdump first-hand): `mov eax,[rdx]; cmp 0x677077; jne reject; movl
$0x45,0x18(%r8); mov $1,%eax; ret` — the tier-1 stamps ONLY the id
0x45, NO size. The size==0 acceptance cannot finalize in the scan loop
(the :9564 `plVar12[2] != 0` emit gate fails), so the candidate rides
the continuation path where the +0x20 processor (FUN_180014160, the S85
landing's zero-tail walk) computes the extent — the pending machinery
(:9564-9591, the 16-candidate force-finalize) is the racing processor's
calling context. The S85 landing's recorded residue ("the processor's
ctx gate [result+0x14] >= 0x10 — undecoded; if it defers the walk to a
later sector the extent shifts by multiples of 512") is thereby
EXPLAINED: the "deferral" is the pending-list re-feed cadence (each
re-feed advances the chunk by one sector), not an undecoded gate in the
walk itself. The tier-1 id-only stamp is WHY the +0x20 processor exists.

---

## Session-91 addendum 2 — the SIT member LANDED (member 7 of 11: the next-entry walk)

**SIT FIXED (this session; three members remain: APE, SWF-RECT + the
DWG gen-3 R2007 remainder).** The walk decoded first-hand from the raw
asm (objdump over RAWSearchDLL.dll; evidence:
`evidence/s91-lane-sheets/s91-l2-sit-entry-walk-decode.md`):

- The registry: key "Stuf" (0x66757453, the insert at 0x180003a7c) →
  node {primary vtable 0x180067890, idx 0x5d, secondary at +0x10
  {vtable 0x18006d5a8, idx 0x5d}}; the primary's +0x40 is the
  return-0 stub — no secondary consult.
- The tier-1 (FUN_180015a20): the full 7-byte "StuffIt" gate ("Stuf" +
  u16@4 == 0x4966 + byte@6 == 't') — the core's existing gate,
  byte-exact — stamps ONLY the id 0x5d (NO size): the WPG pattern, the
  extent comes from the child processor.
- The factory (FUN_180015a90): the 0x120 child {vtable 0x180067828,
  min 0x5000 (20480) at +0xd8, max 0xbebc200 (200 MB) at +0x118} — the
  N-093-decoded caps in their native slots.
- The processor (FUN_180015d70, the child's +0x20, fed per 512-byte
  chunk): the first chunk reads the format byte @7 (' ' → size 0; '!'
  → the bang flag; both continue); the later chunks gate the terminator
  at the CHUNK START (sector-aligned): (a) the classic 8-byte
  "StuffIt "/"StuffIt!" next-archive header, or (b) the wrapper family
  {u32@0 ∈ {"Stuf","WEBB"}, "AIKE" at +4, four NULs, the 0x7bbd74a3
  checksum at +0xc}. The walk cap = 10 × max = 2 GB → FINALIZE with
  the walked chunk end (an extent the row gate then rejects — a
  no-terminator SIT does not carve in the og). On FINALIZE:
  **size = (chunk_index − hit_index + 1) × sector_size** — the end of
  the sector containing the terminator.

The landing: `validate_sit` keeps the 7-byte tier-1 gate and replaces
the blind 64 MiB stand-in with the sector-stepped terminator search
(512-byte steps from hit+512, both terminator families, the
sector-alignment requirement pinned); found → the walked sector's
end; the device-end torn case → the walked-bound salvage (the
X3F/CRW loop-exit family posture — the og's own 2 GB cap path
row-rejects, the registered model translation in the evidence sheet's
open question (c)). RED→GREEN first-hand via the stash dance:
SitNextEntryWalkSetsExtent + SitUnalignedNextEntryDoesNotStopTheWalk +
SitWebbAikeTerminatorStopsTheWalk all RED at the stand-in tree (3
FAILED — the stand-in's blind extents), GREEN with the walk;
SitTornAtDeviceEndSalvagesTheWalkedBound pins the torn salvage
(agrees with the stand-in at the device end — the regression pin);
SitMinimumIsTheOgFloor unchanged and green (the device-end salvage
lands exactly on the 20480 floor). The 206-test carve suite green.
The honest residue: the bang-format sub-check FUN_180015b60 (gated on
ctx->[0x14] >= 5 + child->[0x110]) was read only at its gate structure
— its inner entry-index parse is recorded as the evidence sheet's open
question (a); the landing's terminator model is complete without it
(it can only finalize earlier with the same walked-sector formula).

---


---

## Session-89 addendum — the SIT member LANDED (member 10 of 11: the FileSITAnalyser entry-marker walk + the acceptance-window repair)

**SIT FIXED (this session; one member remains besides APE/SWF-RECT... — the
enumeration after this landing: APE + SWF-RECT remain).** The chain decoded
first-hand from the raw asm (objdump over og-codebase/RAWSearchDLL.dll):

- **The acceptance gate is the family union the og's processor enforces**
  (the primary processor 0x180015d70's gate, raw-asm 0x180015df3-0x180015e67:
  `sub $0x20,%al; ja` — byte[7] must be 0x20 or 0x21 — plus the u16@4
  0x4966 / byte@6 't' checks; the WEBBAIKE arm: u32@0 "WEBB" +
  u64@4 0xB4EB6666454B4941 + u32@12 0x7bbd74a3). The node's own 7-byte
  gate (the wrapper vtable 0x18006d5a8 slot+0x10 = FUN_180044940, the
  tier-1 "Stuf"+"fI"+'t' OR the 16-byte WEBBAIKE key) is WIDER than the
  processor's — a "StuffItX" candidate passes the node gate, is refused by
  the processor, and rides the continuation until it dies: never carved.
  The pre-fix core accepted ANY byte[7] with the blind extent (the
  discovered acceptance defect, the M-156 X3F-ASCII-gate class inverted):
  **FIXED — byte[7] ∈ {' ', '!'} or refuse.**
- **The '!' form's walk = the FileSITAnalyser** (FUN_180044a10, vtable
  0x18006d5b0 slot-0 — the File*Analyser one-function tier-1+size shape,
  the M-090/WPD authority precedent; the +0x40 stub on the SIT node means
  the secondary-first consult never fires on the magic-key path — the walk
  is the whole-file expression of the same marker machinery the primary's
  +0x110 pending flag triggers): the tier-1's own byte[7]=='!' gate
  (0x180044a93 `cmpb $0x21,0x7(%rax)`); the windowed entry-marker search
  — the 3-byte prefix 02 D7 0A (FUN_180044890's byte-scan) + the +4 u32
  0x8842f698 validation; a full hit stamps **extent = marker + 8 +
  stride** (stride: XX==0x19 → 4, XX==';' (0x3b) → 5, else 6; raw-asm
  0x180044cab-0x180044cd2; the .rdata pattern table at 0x180067848 lists
  the three 8-byte forms {0x10, 0x3b, 0x19}); a 3-byte prefix hit with a
  mismatched +4 u32 is the **corrupt-entry terminal** (extent = the
  512-aligned sector start containing it); the **new-header gate** (a
  following 7-byte "StuffIt" or the 16-byte WEBBAIKE key at a 512-byte
  boundary, FUN_180044960 — raw-asm-verified both arms) ends the walk at
  that boundary; torn → the walked bound (the loop-exit stamps the
  position; the og's cap is the singleton+0x70 halved bound the row max
  200,000,000 mirrors — the X3F stance).
- **The classic ' ' form**: extent 0 at the first feed
  (raw-asm 0x180015dd6) with the size eventually stamped by the feeding
  epilogue `(offset - stored + 1) * len` (0x180015f42) — a streaming
  artifact with no decoded whole-file semantic. The honest stand-in stays
  for that population (reduced confidence, the note naming the recorded
  follow-up) — the same posture as the DWG gen-3 remainder.

RED→GREEN via the stash dance (the validator reverted alone, the tests
kept): **5 of 7 RED at the stand-in tree** (SitEntryMarkerWalkSetsExtent /
SitMarkerStrideTable / SitNewHeaderAtSectorBoundaryEndsTheWalk /
SitCorruptEntryTerminalStampsTheSectorStart /
SitByteSevenOutsideTheWindowIsRefused), GREEN with the walk; the two
pre-existing pins (SitMinimumIsTheOgFloor, the classic-form stand-in pin)
pass both ways by design. The S-007 batch's SIT plant re-pinned (the '!'
magic + the real 0x19-stride marker at bytes-12; the old filler's byte[7]=7
would now correctly refuse). The full suite **1711/1711** (1705 + the six
new tests). Remaining members: **APE, SWF-RECT**.

---

## Session-87 addendum 3 (the S87-continuation lane's SIT convergence record — the seat conceded per SA-9)

This lane independently decoded and implemented the SIT member in the
same window as the racing S89 lane's landing (`2214af90` — the earlier
push holds the seat). The two decodes AGREE on: the 7-byte "StuffIt"
tier-1, the processor's byte@+7 window ('!' = the SIT5 search arm; ' '
= the classic arm; any other byte never finalizes — the racing lane's
"acceptance-window repair" and this lane's routing are the same
decode), the marker family 02 D7 0A XX 98 F6 42 88 (this lane's
first-hand .data dump at 0x180067858/68/78 vs the racing lane's
stack-constant read at 0x180044b17-0x180044b1e), and the WEBB AIKE
66 66 EB B4 A3 74 BD 7B family form. The racing lane's walk is the
FULLER decode: FUN_180044ae0's per-ENTRY marker walk (the stride table
{0x19→4, 0x3B→5, else 6} past the marker end, the corrupt-entry
terminal at the 512-aligned sector start, the new-header gate at the
512-byte boundaries) — this lane had stopped at the processor's
FUN_180015b60 chunk-level magic search and translated it directly;
the racing seat's walk supersedes.

**The convergence deltas this lane contributes for the future
arbitration lane (both first-hand, neither contradicts the seat):**
1. **The walk cap:** this lane's decode of the heuristic gate at
   0x180015e69-0x180015e81 reads `len × rel < max × 10` — the child's
   200,000,000 max scaled by ten (the asm's `lea (%rax,%rax,4)+add`)
   = a 2 GB force-stamp boundary; the racing seat's bound is the
   singleton-halved form. The two bounds differ by 10×; a future lane
   reading FUN_180044ae0's own loop exit (vs the processor's
   heuristic) settles which cap the og's walk actually carries.
2. **The classic ' ' form's walk:** this lane decoded the processor's
   else-branch re-validation as the ' ' form's ONLY terminator (the
   next full StuffIt/WEBB-family header) — a real walk, not a
   stand-in; the racing seat keeps the honest 64 MiB stand-in at
   reduced confidence with the "primary's feeding walk" as the
   recorded follow-up. A future lane decoding the primary chain's
   feeding path settles whether the classic form walks.
3. The stream-end drop (a candidate whose walk never completes before
   the device ends) — this lane's translation (the decline) matches
   the seat's torn-salvage posture only when the bound is reached;
   the difference is unobservable below whichever cap is right.

This lane's five SIT tests were withdrawn with the concession (they
pin the processor-level translation the seat's fuller walk
supersedes); the racing seat's seven-test battery stands as the
member's proof.

---

## Session-91 addendum 3 — the SIT arbitration: the classic-form walk folded into the racing seat (N-229's repair record)

The racing window landed a DEEPER SIT decode (the .data end-magic
constants at 0x180067858/68/78, the stride table {0x19: 4, 0x3B: 5,
0x10: 6}, the corrupt-entry terminal, the WEBB 16-byte key as the
four-dword form {0x42424557, 0x454b4941, 0xb4eb6666, 0x7bbd74a3}) —
their push keeps the seat per SA-9. **But their landing shipped RED**
(N-229, findings/FIXED/): the unused `flavor` parameter broke the
Linux build (-Werror), so their seven-test battery NEVER RAN — and on
first execution three tests failed while two pins contradicted each
other on the classic-' '-form semantics (the stand-in vs the
next-header walk — both pinned in the same push).

The arbitration (this lane's repair, corpus-first):
1. The classic ' ' form walks to the next SIT-family header at a
   sector boundary (this lane's FUN_180015d70 decode: the first-chunk
   gate sets NO search flag for ' '; the racing lane's own
   SitNextHeaderTerminatesTheWalk pins the same edge) — the stand-in
   branch and its pin superseded (rewritten to
   SitClassicSpaceFormWalksToTheNextHeader).
2. The stream-end drop: a no-terminator SIT DECLINES (both decodes
   agree: the og's 2 GB force-stamp extent row-rejects; the pending
   candidate drops at the stream end) — the torn-salvage fall-through
   and the stale 200 MB constant removed.
3. The racing marker walk (the '!' form) kept as landed: the entry
   marker (02 D7 0A + the u32@+4 validation 0x8842f698, the stride
   table), the corrupt-entry terminal (the sector start), the
   new-header boundary — with the shared next-header gate now serving
   both forms.
4. The flavor repair: the plant uses its parameter with the
   stride-adapted marker position; Sit5AllThreeMagicVariantsCarve now
   actually exercises the three variants (was: three copies of the
   0x19 plant under a broken build).

The SIT battery 12/12 at the arbitration head; the carve suite
213/213; the full suite 1720/1720. This lane's four processor-level
SIT tests withdrawn (superseded by the arbitrated battery). **SIT's
member status: LANDED (the arbitration); two members remain: APE,
SWF-RECT + the DWG gen-3 R2007 remainder.**
---
## Session-87 addendum 4 — the APE member's decode progress (the gate landed separately as M-160; the walk remains)

**APE's size walk NOT yet landed (the member remains; the decode
progress recorded here for the future lane).** This session's
first-hand decode of the og's APE chain (BruteForce::CAPEAnalyser,
the "MAC " key 0x2043414d → idx 0x51):

- **The version gate — LANDED as M-160** (the discovered whole-family
  defect: the og gates the **u16 LE at +4 against [1000, 9999]**
  (FUN_180033180's first check); the core's u32-BE [300, 5000] read
  rejected every real APE file. Fixed this session with the two-test
  battery.)
- **The walk's own cap:** `uVar4 = *(longlong *)(singleton + 0x70) * 10`
  = 0xbebc200 × 10 = **2 GB** (the same singleton×N class as DWG's
  ×100); the final clamp `if (uVar4 < extent) extent = uVar4`.
- **The old form (< 3980):** the 32-byte header; the format flags
  (u16 LE@+8): bit 2 → the data start 0x24 else 0x20; bit 4 (seek
  elements) → a u32 read at the data start + 4; the u32@+0x10
  (nHeaderBytes) nonzero → + 0x2c (the 44-byte WAV header); the
  alignment = channels (u16 LE@+0xA) × bytes-per-sample (bit 3 → 3,
  bit 0 → 8, else 2); the gates: alignment ≠ 0 AND channels ≤ 2 else
  the extent-0 decline.
- **The new form (≥ 3980):** the descriptor (the u32@+8
  nDescriptorBytes → the header at +52); the u64 at header+0x10 =
  nBitsPerSample | nChannels<<16 | nSampleRate<<32; the alignment =
  (bitsPerSample >> 3) × channels with the same ≠ 0 / channels ≤ 2
  gates; the data start = the descriptor's size-field sum.
- **The seek-table walk (LAB_180033421):** the u32 entries from the
  data start; each entry 0 or ≥ 2 GB breaks; the MAX accumulates at
  +0x120 — the audio end. A zero/oversized first entry at the data
  start → the extent-0 decline (no seek table).
- **The APETAGEX tail scan:** backward from the chunk end for the u64
  0x5845474154455041 ("APETAGEX"); on hit: the u32@+12 (the tag size)
  and the flags byte @+0x17 bit 5 (the header-at-front); the extent
  adjusts to include the tag (the old form: footer_pos + the tag
  fields; the new form: the aligned sum); no tag → the block-alignment
  rounding.

The remaining landing work: the static translation of the seek walk +
the tag scan over the exact field offsets above (the +0x118
table-bound arithmetic is the one decode point still open — the loop's
own 0/≥2 GB break is the walk's termination in the static model).
## Session-90 addendum 3 (the S90 lane's SIT concession — the ×10-gate corroboration + the marker-scope correction)

This lane's SIT landing (e0acb546, unpushed at the pull) was CONCEDED to
the racing window's 2214af90 (the earlier push holds the seat per SA-9).
The concession analysis: **the racing decode found the correct SIZE
AUTHORITY** — the FileSITAnalyser secondary's one-function walk
FUN_180044a10 (the M-090/M-155 File*Analyser authority precedent), whose
entry-marker walk uses the SAME 8-byte marker constants this lane
decoded. This lane had targeted the PRIMARY's pending-feed processor
(FUN_180015d70) — the wrong authority for the extent (the same class of
mistake the S78 X3F lane made and M-155 corrected).

This lane's unique carries, recorded as corroboration:
1. **The ×10 stay-pending gate** (raw-asm 0x180015e69-0x180015ea6, the
   `lea (%rax,%rax,4),%rcx; add %rcx,%rcx` pair = row_max × 10 over the
   SITRawFileInfo ctor's 200,000,000): the PRIMARY's feed-processor cap
   is 2 GB — a true fact about the pending machinery the racing landing's
   comment complements (their torn cap: "the singleton+0x70 halved bound
   — the SITRawFileInfo row max 200,000,000"). The primary's cap and the
   secondary's torn bound coexist: the primary's gate bounds the PENDING
   feeding, the secondary's walk bounds the EXTENT.
2. **The marker-scope correction (this lane's own error, honestly
   recorded):** this lane's "the three stop markers are provably INERT"
   claim was TRUE only of FUN_180015b60's single short-circuited call
   site (0x180015e98 — one caller, the always-true size-high arm) — the
   marker CONSTANTS are alive in FUN_180044a10's entry walk (the racing
   decode). The inert claim is scoped to the dead gate, not the markers.
3. The byte[7] ' '/'!' split: independently decoded both sides; the
   racing landing went further (the acceptance window — any other byte
   never finalizes in the og; the core refuses it outright).

The racing landing's row max (200,000,000) stands — the secondary's
walk is the extent authority and its torn bound IS the ctor value; this
lane's 2 GB row was the primary's gate constant applied to the wrong
surface. This lane's two SIT tests and the batch-caps re-pin were
dropped as superseded; the racing battery (SitEntryMarkerWalkSetsExtent
/ SitMarkerStrideTable / SitNewHeaderAtSectorBoundaryEndsTheWalk /
SitCorruptEntryTerminalStampsTheSectorStart /
SitByteSevenOutsideTheWindowIsRefused) owns the coverage.

---

---

## Session-92 addendum (renumbered from the born-collided S92) — the WPD member's S92 SUB-FORMAT RIDER (the engine-side name split; the S87 landing's 'report-layer' framing corrected)

**The rider (this lane's unique delta over the racing S87 seat):** the
racing 1ff8ae24 landing kept the single FMT_WPD family carve for all
three tier-1 rows, framing the id split as "the og's report-layer
routing ... the registered model-divergence stance, the CRW precedent"
— but the framing is factually wrong and the registration never landed
in any trail file (PARITY-DELTAS.md carries no WPD row; the S87
addendum's text names no divergence — the stance exists only in the
commit message; the trail-truth class this session's doc sweep hunts).

The first-hand decode that corrects it: the og's ENGINE assigns the
sub-format name. The plain CRawFileInfo child's **+0x28 processor
(0x18000b7a0)** — decoded this session: `mov eax,[rcx+0x18]` (the id
the tier-1 stamped), `lea r9,[rax+rax*8]` + `0x18008f070` (the
9-byte-stride name table), the strlen walk, the string-assign call —
maps the stamped id (0x44/0x45/0x3d) to the name string WPD/WPG/SHW
inside the engine's own result object. The "CRW precedent" citation is
also a mis-citation: CRW II/MM are distinct 4-byte KEYS (registry
dispatch), while the WPC split is post-key discrimination — the true
precedents are N-111 (JP2 brands via format_override) and N-115/N-158
(the office typing set).

**The landing:** the WPG pair {0x00021601, 0x00030000} carries via
`format_override` onto the existing FMT_WPG row (the "wpg\0"-keyed row
keeps its own zero-tail-walk entry semantics — two paths, one format
id, the og's shape); the SHW pair {0x00010f0f, 0x00030000} takes the
appended **FMT_SHW** row (ext "shw", the og name table's id 0x3d) — an
override target in the N-115 pattern, its kTrialOrder slot keeping the
exhaustive-table static_assert true. The WPD row itself (min 16, max
4 GiB — the racing S90 rider's house-pattern form) is untouched. The
strengthened `WpdSecondAndThirdRecordsCarve` asserts both sub-format
ids + both sizes: RED first-hand via the validator-only stash dance
(the pre-rider tree stamps everything FMT_WPD), GREEN with the rider;
the full WPD battery 7/7; the full suite 1705/1705.

**The convergence record (this lane's seat history):** this lane
independently decoded the WPD member pre-window (the three-pair
tier-1, the u32@+0x14 extent, the +4 gate retirement — all agreeing
with the racing S84/S87 landings; the local landing dropped at the
rebase per SA-9 with the concession recorded in the worklog's S92
entry). The racing S87 seat added the zero-size decline gate (the
engine's extent!=0 gate at 0x18000d7e1) this lane's decode missed —
their seat is the better base and holds. This rider is the one delta
that survives: the sub-format split the seat's own message saw
("-> type 0x44 / 0x45 / 0x3d") but routed away on the mistaken
report-layer framing. The divergence registration the S87 message
promised is thereby MOOT — the split is implemented, not diverged.

---

## Session-87 addendum 5 — the SWF-RECT member LANDED (member 11 of 11: the RECT validation gate; the open question 3 resolved)

**SWF-RECT FIXED (this session — the last enumerated member; the APE
walk and the DWG gen-3 R2007 remainder stay as the finding's recorded
remainders).** The walk decoded first-hand (the FileSWFAnalyser chain):
**FUN_1800268a0** (:29970-29996, the analyser): the key gate accepts
"FWS" (u16 0x5746 + 'S') or "CWS" (u16 0x5743 + 'S') — ZWS is NOT
accepted; on the deep gate's success it stamps type 0x3e and **extent
= u32@+4 (the FileLength) VERBATIM** — the finding's open question 3
RESOLVED: the RECT walk is VALIDATION-ONLY, the extent never comes
from it (the old core's FileLength source was already right).

- **FUN_180026940** (the FWS deep gate, raw asm 0x180026940+): version
  byte@+3 ∈ [3,20]; FileLength > 8; the RECT at +8: Nbits = byte@8 >>
  3 with the Nbits > 1 gate; four signed fields (a sign bit +
  Nbits-1 magnitude bits, MSB-first — the og's sign-magnitude read of
  the two's-complement fields); the acceptance: ALL FOUR coordinates
  non-negative, both maxes strictly positive, Xmin < Xmax AND Ymin <
  Ymax — a sane positive rectangle.
- **FUN_180026b80** (the CWS deep gate): version ∈ [6,20] ((v-6) <
  0xf — TIGHTER than the FWS arm); the deflate CMF nibble (byte@+8 &
  0xF) == 8; the zlib inflate from +8 into a 512-byte window; ≥ 16
  inflated bytes; the SAME RECT gate on the inflated bytes.

The landing: the shared `swf_rect_is_sane` bit-walk helper + the
per-arm version windows + the CWS deflate-nibble/stream/inflated-RECT
gates (a partial-tolerant streaming inflate — only Z_DATA_ERROR/
Z_MEM_ERROR decline; the truncated-but-valid prefix yields partial
output, the og's own streaming shape). The extent = FileLength
verbatim (the RECT never sizes); the row max carries the u32 field's
own ceiling 0xFFFFFFFF (the WPD precedent — the 64 MiB stand-in
gone); both rows (FWS and CWS) updated. The plants re-pinned: the FWS
plant writes a REAL sane RECT (Nbits=9, 0/100/0/100 — the MSB-first
bit packer, byte-verified 48 00 C8 00 32 00); the CWS plant writes a
REAL zlib stream (compress2 of the RECT payload — the og validates
the INFLATED RECT).

RED→GREEN first-hand via the stash dance: five tests RED at the
pre-RECT validator (SwfRectGateRequiresSaneRectangle — the inverted
and negative-coordinate RECTs accepted; SwfNbitsUnderTwoDeclines —
Nbits=1 accepted; SwfCwsVersionWindowStartsAtSix — the CWS arm's
version-4 accepted under the shared [3,20] window;
SwfCwsDeflateNibbleAndCorruptStreamDecline — a non-deflate nibble and
a garbage stream accepted; SwfCwsInflatedRectIsGated — an inflated
Nbits=1 RECT accepted), GREEN with the gates; the sixth
(SwfCwsRealStreamCarvesAtFileLength) is the happy-path pin (the
FileLength extent, both arms' unchanged semantics). The 216-test
carve suite green. **The member enumeration closes: 11 of 11 landed
(the APE walk and the DWG gen-3 remainder are the finding's recorded
remainders, each with its decode progress).**
## Session-92 addendum — the APE member's decode IN PROGRESS (the frame walk's first half decoded first-hand; the precise continuation probes recorded)

**APE's chain decoded this session (objdump over og-codebase/RAWSearchDLL.dll;
RTTI via the COL walk):** the "MAC " key 0x2043414d (raw asm
0x18000358b, the rolling alloc-then-bind) → the **BruteForce::CAPEAnalyser**
node (vtable 0x18006bf88, RTTI-verified, id 0x51 = the name table's
'APE'). The tier-1 (node+0x10 = FUN_180033090) is the MINIMAL form:
`mov [r8+0x18], 0x51; mov eax, 1; ret` — accepts UNCONDITIONALLY,
stamps the id, never touches result+0x10; the magic key IS the gate.
The factory (node+0x38 = FUN_1800330a0) allocates a **0x138-byte
specialized child**: the plain CRawFileInfo base ctor (0x18000b520)
then the APE vtable override (0x18006bfd8) with +0x110/+0x118/+0x120
zeroed (the walk's accumulators). The node's +0x40 is the return-0
stub — no secondary; the child's +0x20 (FUN_180033180) is the frame
walk, the authority.

**The walk's decoded first half (0x180033180-0x180033478):**
- **The version window gate** (:180033200-:180033240): u16@chunk+4
  must satisfy 1000 ≤ v < 10000 (the 3920-3990 3.9x family fits
  verbatim); outside → `mov [rbx+0x10], 0; return 1` — the extent-0
  DECLINE.
- **The cap**: singleton+0x70 × 10 = **2,000,000,000** (:1800331e7-:1800331f4,
  `lea r12,[rcx+rcx*4]; add r12,r12` — the same cap family the SIT
  walk decoded).
- **The seek-table walk** (:180033440-:180033476): 4-byte entries from
  the chunk's descriptor-referenced table, each entry < the cap and
  nonzero, accumulating the running MAX into child+0x120.
- **The extent stamp** (:180033494-:18003349b): child+0x120 (the
  seek-table max) → result+0x10.
- **The frame-header parse** (:18003329e-:180033321): the SIMD read of
  the chunk's head, the descriptor offset s32@chunk+8, and the frame
  size formula (u16@frame >> 3) × u16@frame+2 → child+0x12c.

**The precise continuation probes (the next lane's start):**
1. The frame loop at :1800334c4 onward (the `[r13+0x14] > r15d` gate,
   the 0x18006bf70 .data constant, the byte-compare loop at
   :1800334f6) — the per-frame advance and the walk's natural end.
2. The second-chunk continuation at :180033421 (the child+0x118
   path — the chunk-feeding protocol's resync).
3. The terminal states: :1800335c1 (the cap boundary), :1800335c6
   (the decline), :1800335d7 (the return), :180033604 (the post-gate
   fall-through).
The stand-in stays WITH this addendum as the owner (the honest
posture until the walk lands whole).

---

## Session-90 addendum 4 — the SWF-RECT member LANDED (member 11 of 11: the FWS RECT validation gate — a VALIDATION-ONLY landing, the finding's open question 3 answered)

**SWF-RECT FIXED (this session, the S90 lane; APE + the DWG gen-3 R2007
remainder stay with the finding).** The walk decoded first-hand from the
raw asm (objdump over RAWSearchDLL.dll):

- **The caller (FUN_1800268a0 :29970-29996)**: FWS ("FW"+'S') or CWS
  ("CW"+'S') only — ZWS is NOT handled; the deep gate runs
  (FUN_180026940 for FWS, FUN_180026b80 for CWS) and **on acceptance
  the caller stamps size = u32@+4 (the FileLength) unconditionally** —
  the RECT walk NEVER changes the extent. **M-107's open question 3 is
  answered: the RECT parse is VALIDATION-ONLY — the "member" downgrades
  to a gate, exactly as the question anticipated.** The core's extent
  (FileLength) was already correct.
- **The FWS deep gate (FUN_180026940 :30001-30130, raw-asm
  0x180026940-0x180026b79)**: version byte@+3 ∈ [3,20]
  (0x180026953-0x180026963); FileLength ≥ 9 (0x180026969); **Nbits =
  byte8 >> 3** (the top 5 bits, MSB-first), **Nbits < 2 rejects**
  (0x180026980-83); four signed fields of [1 sign bit + (Nbits−1)
  magnitude bits], MSB-first starting at stream bit 5 (the first sign
  bit = LSB-index bit 2 — the asm's `and $0x4,%bl` sign check at
  0x180026a03; the first magnitude bit = LSB bit 1, the traced `shr $1`),
  sign-magnitude decoded (neg/cmove) — for every file the gate ACCEPTS
  all four fields are non-negative, where sign-magnitude and the spec's
  two's complement agree; **accept iff Xmin < Xmax && Ymin < Ymax &&
  Xmin ≥ 0 && Ymin ≥ 0 && Xmax > 0 && Ymax > 0**
  (0x180026b3e-0x180026b5b) — a bounding-box sanity gate.
- **The CWS deep gate (FUN_180026b80) inflates the compressed body
  before validating** (the zlib-family calls at 0x180026c25/0x180026c4c/
  0x180026c7e) — the finding's explicit recorded remainder; the CWS row
  keeps the shallow gates + the FileLength extent (which the og's
  caller also stamps for CWS). Pinned by
  `SwfCwsDeepGateIsTheRecordedFollowUp` so the future inflate-validation
  landing flips the pin deliberately.

The landing: validate_swf grows the FWS RECT gate (`swf_rect_valid`, the
exact bit-walk, 25-byte header window covering every Nbits ≤ 31); the
"RECT bit-walk is the recorded follow-up" comment is GONE. The S-007
plant re-pinned (a valid RECT via the bit-writer helper). RED→GREEN
first-hand via the stash dance: SwfRectValidationRejectsInvertedBox /
SwfRectValidationRejectsNegativeField / SwfRectNbitsBelowTwoRejects all
RED at the pre-fix tree (the core accepted every readable FWS header),
GREEN with the gate; SwfRectValidationAcceptsSaneBoxAtFileLength pins
the positive + the FileLength extent. Suite 1724/1724.
---

---

## Session-86 S86 addendum — the DWG convergence record (the seat conceded per SA-9; the ten-pair gate truth + the gen-3 routing riders landed on the seat) (2026-10-30)

This lane independently decoded the DWG three-generation walk from the
raw asm (its own decode sheet:
`evidence/s86-lane-sheets/s86-dwg-three-generation-walk-decode.md`)
before the pulls revealed the racing S84/S85 landings — the convergence
is total (cursor 0x15, records from 0x19, the full-u32 id equality at
0x18002f406, the running-max at 0x18002f41f, the mismatch decline at
0x18002f46b, the torn salvage at 0x18002f429, the rand-LCG
de-obfuscation at 0x18002f670, the six gates at 0x18002f5b0, extent =
u64[0x34]+0x6c at 0x18002f5dd, the gen-3 bounded remainder). The seat
stands; this lane's six duplicate tests were dropped.

**Rider (a) — the tier-1 table is TEN pairs, not nine:** the asm's
stack table runs [rbp-0x50]..[rbp-0x18] with the tenth entry's suffix
u16 "09" at rbp-0x16 (the high half of the 0x39303031 constant at
0x18002feae) and the loop bound `cmp edx,0xa; jl` at 0x18002fede.
AC1009/R12 is gate-PASSED (id 0x18 stamped) but un-walked by every
generation — the og's size stays the ctor's 0, so the DECLINE is the
og-true observable. Landed: the gate list carries the tenth pair; the
test renamed DwgVersionGateIsTheOgsTenPairs with the third arm.

**Rider (b) — the gen-3 branch was a fall-through:** the explicit
"21" match (the og's gen-3 table: the u16 0x3132 compare at
0x18002f4eb-0x18002f505) restores the routing; a gated-but-un-walked
version keeps the ctor's 0 and declines. The third test arm was RED
first-hand at the rider tree without it.

**Rider (c) — the WPD duplicate-test red main:** convergent with the
window's N-230 (their delete kept the S87 seat's test; this lane's
rename form conceded per SA-9 and is recorded in N-230's Resolution).

**Remaining members: APE's frame walk, SWF-RECT** (+ the DWG gen-3
R2007 remainder named on the seat). The merged DWG+WPD battery 17/17
first-hand; the full two-pass suite green.
## Session-89 addendum — the APE member LANDED (member 10 of the batch-1 eleven by the finding's own enumeration; the board's live-defect headline of this landing)

**APE FIXED (this session; ONE member remains: SWF-RECT + the DWG gen-3
recorded remainder).** The walk decoded first-hand from the raw asm
(objdump over og-codebase/RAWSearchDLL.dll; evidence:
`evidence/s89-lane-sheets/s89-l1-ape-cape-walk-decode.md`): the chain is
the registry key "MAC " (0x2043414d) → the tier-1 FUN_180033090 that
stamps ONLY the id 0x51 (the WPG class — no size at tier-1) → the
CAPERawFileInfo +0x20 processor FUN_180033180 (0x180033180-0x180033603):

- **The version gate — the discovered live defect:** the og reads the
  **u16 LE at +4, gated [1000, 9999]** (raw-asm $0x3e8/$0x2710 at
  :228/:232 — the version×100 encoding). The core's stand-in read a
  **u32 BE** and gated **[300, 5000]** — a reading under which every
  real Monkey's Audio file (u16 LE 3800..4400) was REJECTED: the
  format keyed correctly, real files never carved (the M-156 ASCII-gate
  class, third occurrence). The S-007 batch's own APE plant was tuned to
  the wrong gate (a u32 BE 3980 that the invented window accepted) and
  is re-pinned to the real grammar (the CRW/PSP plant precedent), as is
  the bad-variant fixture (the BE 999999 read as LE u16 3840 would now
  carve; re-pinned to u16 LE 999).
- **The format split at 3980:** OLD (< 3980) — the seek-table dword scan
  (base = 0x20 | flags&4, +4 on flags&0x10, +0x2c when u32@16 != 0; the
  align = u16@10 × mult gates; entry[0] = the scan bound with the
  version≤3800 aux subtraction; stop at 0 or ≥ cap; the max entry is the
  walked size). NEW (≥ 3980) — the descriptor arm: align = (u16@desc >>
  3) × u16@desc+2; **size = u32@8+12+16+20+24+32** (the og's own
  arithmetic — the dword@32 MD5 term included, the registered quirk).
- **The epilogue:** the finalizing chunk is the 512 bytes ending at
  round_up(size, 512); the backward "APETAGEX" scan (.rdata 0x18006bf70,
  read byte-exact from the PE) with the footer gates (items ≤ 0xFFFF,
  the signed tag_size−0x20 ≤ 0x1000000, flags bit29 clear); OLD+tag →
  extent = the footer end (the span algebra cancels — raw-asm
  0x180033593-:59e); NEW+tag → round_up(size + span, 512); no tag →
  round_up(accum − zero_tail, 512) + 512 (the og's own +512 slack,
  kept).
- **The cap:** singleton+0x70 × 10 = **2,000,000,000** (raw-asm
  0x1800331ec-:1f4) — the row max re-pinned (256 MiB gone) and the row
  min re-pinned to 0 (the CAPERawInfo factory's [0x22] = 0).
- **Torn:** the readable extent ends before the finalizing window → the
  walked size clamps at the reduced confidence (the SIT/X3F salvage
  class); the og's pending force-finalize is the streaming machinery's
  job (the registered model divergence — the CRW/WPG stance).

RED→GREEN first-hand: **5 of 6 RED at the stand-in tree**
(ApeVersionGateIsTheOgsExactWindow / ApeOldFormatTagEndsAtTheFooter /
ApeNewFormatTagExtentIsHeaderSumPlusSpan / ApeNoTagExtentIsTheRoundUpPlusSlack
/ ApeTornSalvagesTheWalkedSize; the channel-gate pin passed both ways —
the old gate also rejected the fixture, for the wrong reason), GREEN with
the walk; the S-007 batch's APE row re-pinned (the plant's real grammar +
the footer-end extent 0x3E0). The full suite **1722/1722** (the N-229
red-main repair rides the same push — the WPD convergence's duplicate
test body deduped). Remaining members: **SWF-RECT only** + the DWG gen-3
R2007 recorded remainder.

---

## Session-87 addendum 6 — the SWF-RECT member COMPLETED (the CWS deep gate lands over the racing FWS seat); the enumeration closes

**SWF-RECT COMPLETE (this session — the racing window's FWS seat
(their `swf_rect_valid` + the five-test battery + the
SwfCwsDeepGateIsTheRecordedFollowUp pin) + this lane's CWS deep-gate
completion, landed in one convergence merge):**

- **The CWS version window [6,20]** — FUN_180026b80's own `(v-6) <
  0xf` head gate, TIGHTER than the FWS arm's [3,20] (the compressed
  form postdates SWF 6); the pre-completion core applied the shared
  [3,20] window to both arms — a CWS header with version 4 was
  accepted the og refuses.
- **The deflate CMF nibble gate** — `(byte@+8 & 0xF) == 8` (the
  zlib CM field must be deflate; the og checks it before the
  inflate).
- **The zlib inflate + the inflated-RECT validation** — the og
  inflates from +8 into a 512-byte window and runs the SAME
  four-field RECT walk over the INFLATED bytes, with a >= 16
  inflated-byte gate. The completion: a partial-tolerant streaming
  inflate (only Z_DATA_ERROR/Z_MEM_ERROR decline — a
  truncated-but-valid prefix yields partial output, the og's own
  streaming shape), the >= 16 gate, and `swf_rect_valid` over the
  inflated buffer (the helper refactored to take the RECT pointer —
  the FWS raw +8 window and the CWS inflated buffer feed the same
  walk).
- **The follow-up pin FLIPPED**: the racing seat's
  SwfCwsDeepGateIsTheRecordedFollowUp test — which pinned the
  shallow-gates contract "until the inflate-validation landing flips
  this pin deliberately" — now pins the deep-gate contract (a CWS
  header whose +8 bytes are a raw-written broken RECT, a corrupt
  zlib stream, declines). The flip is the landing the pin
  anticipated.

The convergence proof set: the racing seat's five FWS tests + this
lane's four CWS completion tests (SwfCwsVersionWindowStartsAtSix /
SwfCwsDeflateNibbleAndCorruptStreamDecline /
SwfCwsInflatedRectIsGated / SwfCwsRealStreamCarvesAtFileLength — the
first three RED at the pre-completion seat, proven via the stash
dance before the merge resolution) + the flipped pin; the plants
re-pinned both arms (the FWS real RECT via swf_write_rect; the CWS
real compress2 stream — the og validates the INFLATED RECT). The
carve suite green.

**THE ENUMERATION CLOSES: all eleven members landed** (X3F, CRW, DWG
gen-1+2, PSP, WPG, WPD, DXF, EPS, AI, SIT, APE, SWF-RECT — twelve
carve sites across eleven member seats). The ONE recorded remainder:
the DWG gen-3 R2007 (AC1021) de-interleave record chain — the bounded
extent stays for that generation alone, precisely registered in the
S85 addendum (the 0x3d8 accumulate + the 3×239 stride-3 de-interleave
+ FUN_18002ef50's record chain, the decode scope for the future lane).

## Resolution

**Status:** FIXED (the eleven-member enumeration closed; the DWG gen-3
R2007 remainder is the recorded, precisely-scoped residue)
**Fixed in commit:** the member landings across the S78-S87 windows —
X3F (S78/S71/S84 arbitration), CRW (S83), DWG gen-1+2 (S84/S85 +
the +0x80 offset repair f7e49eaf), PSP (S85), WPG (S85), WPD (the
S84/S89/S87 convergence), DXF/EPS/AI (S89), SIT (the S89/S87
convergence + the N-229 red-tree repair), APE (the S89/S87
convergence — the M-160 gate + the full walk), SWF-RECT (the S91/S87
convergence — the FWS seat + this lane's CWS completion)
**Verified by:** each member's RED→GREEN battery in its landing
addendum (the stash-dance proofs); the carve suite's growth
174→216 tests across the landings; the full suite green at every
push; the standing "recorded follow-up"/"stands in" grep now clean
except the two DELIBERATE registered stand-ins (the DWG gen-3
remainder and the NCSD/CCI leg's M-106-registered deviation)
**Resolution notes:** The finding's own acceptance criteria met: the
stand-in comments are gone (grep-verified) or owned by explicitly
registered remainders; the over-cap plants carve og-exact; the
batch-1 suites stayed green throughout. The discovered defects fixed
along the way (each with its own finding): M-160 (the APE gate's
whole-family absence), the PSP 16-byte-prefix acceptance, the DWG
phantom-30/missing-13 tier-1 table, the AI invented version gate, the
EPS bare-%!PS acceptance, the SWF CWS shared-window acceptance. The
DWG gen-3 R2007 remainder keeps its bounded extent with the precise
decode scope registered — the never-silently rule honored.
## Session-86 addendum 2 — the gen-3 R2007 remainder: the decode PROGRESS (the rotation transform identified first-hand; the next lane's head start) (2026-10-30)

The DWG gen-3 chain's remaining pieces, decoded this session to the depth
a landing window allowed (all against the corpus decompile; the raw-asm
arbitration stays the next lane's first step):

1. **CONFIRMED (the accumulate state machine)** — the factory target
   [+0x120] = 0x3d8 (FUN_18002ff20); the first chunk contributes from
   buffer+0x80 (0x18002f52b-0x18002f540), later chunks whole; when the
   remainder goes non-positive the parser FUN_18002fc50 runs
   (0x18002f573).
2. **CONFIRMED (the de-interleave)** — FUN_18002fc50 copies the 0x3d8
   accumulation, then the 3x0xef strided pass at 0x18002fc24-0x18002fce2:
   out[s*239 + i] = in[i*3 + s], s in 0..2, i in 0..238 - 717 bytes
   (0x2cd).
3. **IDENTIFIED THIS SESSION (the per-record rotation)** — FUN_18002eb10
   (@ 0x18002eb10) is NOT a record parser: it is the one-byte ROTATION
   transform, size-parameterized (param_3 bytes rotated right by one,
   with a 32-byte-per-iteration fast path for sizes > 0x1f - the
   `param_3 >> 5` unrolling at the head). Every R2007 secmap record's
   bytes are stored rotated by one; the size-class byte's position
   within the record is the rotation's offset. This is the missing
   puzzle piece between the size-class grammar head (the FUN_18002ef50
   decode in this lane's evidence sheet) and the record fields.
4. **The remaining unknowns (the next probes, named):** (a) the
   caller's post-de-interleave flow - the `dword@0x18 < 1` gate and the
   16-byte-stride records copy into the local_508 object
   (FUN_18002fc50's mid section, decompile :36680-36732) - needs the
   raw-asm pass (the decompile's puVar7 aliasing is not resolvable
   from the C output alone); (b) the final size write
   `*(u64*)(obj+0x10) = uStack_500` - which local_508 field accumulates
   the max (the +0x08 field per the stack layout) and what feeds it;
   (c) whether the 0x17 special record / the 0xFF skip runs
   (FUN_18002ef50's grammar) contribute to the extent directly.

The bounded extent REMAINS for gen-3, pinned by the seat's tests. The
evidence sheet carries the full head decode:
evidence/s86-lane-sheets/s86-dwg-three-generation-walk-decode.md.

---

## Session-89 addendum 2 — the SWF-RECT member LANDED (member 11 of 11: the validation-only downgrade the finding's own open question 3 prescribed)

**SWF-RECT FIXED (this session; the finding's open question 3 answered
first-hand: the RECT parse is VALIDATION-ONLY — the extent is the
FileLength u32@4 in BOTH paths, the caller's stamp at :29987-88; the walk
never contributes to the extent).** The chain decoded first-hand from the
raw asm:

- **FWS (FUN_180026940)**: version byte@3 ∈ [3,20] (0x18002695b), the
  FileLength >= 9 gate (0x180026969), then the RECT walk — nbits =
  byte[8]>>3 must be >= 2 (0x18002697d); the four fields (Xmin Xmax Ymin
  Ymax) each read sign-bit-first then (nbits-1) magnitude bits MSB-first
  (the sign from the field's own first bit — byte[8]&4 for Xmin, raw-asm
  0x180026ad1-0x180026ae8); value = sign ? -magnitude : magnitude (the
  og's sign-magnitude arithmetic: the -0 form reads as 0 and PASSES the
  gate — the og's own edge, pinned in the battery); the geometry gate
  (0x180026b3e-0x180026b5b): both axes 0 <= min < max (min >= 0 via `js`,
  max > 0 via `jle`, min < max via `jge` — any violation declines).
- **CWS (FUN_180026b80)**: the version window is [6,20] (0x180026bb2
  `sub $0x6; cmp $0xe` — the compressed form exists only from SWF 6; the
  pre-fix core accepted [3,20] for both sigils — the discovered
  divergence, FIXED), the zlib CMF low nibble must be 8 (0x180026bc4 —
  the deflate method; byte 8 is the zlib stream's first byte in a CWS
  file, FIXED), FileLength >= 9. The og then INFLATES the body and runs
  the RECT walk on the decompressed stream (the zlib calls at
  0x180054a90/0x180054c10) — the core keeps the GZ no-inflate precedent:
  the inflate validation is the REGISTERED follow-up, the version + CMF
  gates are the honest header-validation floor.

RED→GREEN via the stash dance: **5/5 RED at the pre-fix tree**
(SwfRectGeometryGateRejectsDegenerateRect /
SwfRectNegativeFieldDeclines / SwfNbitsFloorIsTwo /
SwfCwsVersionGateIsSix / SwfCwsZlibCmfNibbleRequired), GREEN with the
walk; the SWF plants re-pinned (the FWS plant carries the real nbits=11
RECT 0,640/0,480; the CWS plant carries the 0x78 0x9C zlib CMF/FLG
pair). The full suite **1721/1721**. Remaining members: **APE only**.

The rider swept in the same change: the S91 racing window's duplicate
WpdZeroSizeFieldDeclines definition (the N-210 class, NINTH occurrence —
the rider survived its own concession's test-drop sweep; cr_tests could
not build: `redefinition of Carve_WpdZeroSizeFieldDeclines_Test`,
first-hand at the pre-fix tree) — the duplicate merged into the original
:3434 definition with both rationales kept.
---

## Session-89 addendum 3 — the SIT RECONCILIATION (the corpus-true arbitration of the two convergent landings: the ARCHITECTURE decides, not the push order)

The S89 lane's SIT landing (this file's Session-89 addendum) and the racing
S87/S91 lane's SIT5 landing (their 11a819c1) decoded DIFFERENT authorities:
the FileSITAnalyser secondary (FUN_180044a10 — the stride table, the
corrupt-entry terminal, the walked-bound torn salvage) vs the primary
processor (FUN_180015d70 + the search FUN_180015b60 — the three full 8-byte
end magics, the stream-end drop, the ' '-form next-header-only walk, the
max×10 = 2 GB cap). Their merge (72b96a93) conceded the seat to the S89
decode ("the fuller decode") — but the concession's premise was wrong, and
this lane's reconciliation corrects it with the ARCHITECTURAL proof:

**The SIT node's wrapper vtable (RawSpeed::SITAnalyser::vftable =
0x180067890) carries the RETURN-0 STUB at +0x40** (the full .rdata
node-vtable enumeration — only 8 node classes override +0x40 with the real
getter FUN_180013700; SIT is not among them; the M-156/S83 precedent's own
census re-verified). The magic-key dispatcher's secondary-first consult
NEVER FIRES for a "Stuf" candidate: the FileSITAnalyser is never asked.
The PRIMARY processor (the walker child's +0x20 slot, FUN_180015d70) is the
scan-path authority — the racing lane's decode was the corpus-true one on
every point the two landings disagreed:

1. **The extent formula: the found magic's END** (the full 8-byte magic
   search, FUN_180015b60's three compare loops over the .data constants at
   0x180067858/68/78) — NOT the FileSIT stride table (marker + 8 + stride).
   The stride table is the SECONDARY's own semantics — preserved in this
   record, withdrawn from the scan path.
2. **The stream-end drop**: a walk that finds neither magic nor next header
   never stamps — the pending candidate drops (NO carve). The FileSIT's
   loop-exit stamp (the walked bound) is again the secondary's detail.
3. **The ' ' (pre-SIT5) form: the next-header-only walk** (the magic-search
   flag [child+0x110] is set only on '!') — the S89 lane's "honest
   stand-in" for that population is retired; the stream-end drop applies
   when no header follows.
4. **The 2 GB cap** (the child's 200,000,000 max × 10, the asm's
   lea (%rax,%rax,4)+add at 0x180015e77-7b) — the force-stamp boundary; the
   racing lane's row-max constant stands.

The S89 landing's own contributions that survive the reconciliation: the
**acceptance-window repair** (byte[7] ∈ {' ', '!'} — the processor's own
gate; the pre-fix core accepted any byte[7]) and the **512-aligned
next-header translation** (the chunk-head re-validation at the chunk
granularity — both landings' readings agree in the whole-buffer model).

The landing (this lane's rewrite on top of the merged tree): validate_sit
now implements the primary's semantics with the surviving repairs; the
corrupted merge artifact (the auto-merge had mangled the unified plant's
body — `img]` lines, first-hand) rewritten as the flavor-aware plant (the
magic at bytes-8). The batteries: the racing lane's five tests pass
unchanged (Sit5MagicWalkSetsExtent / Sit5AllThreeMagicVariantsCarve /
Sit5NoMagicNoHeaderDeclines / SitNextHeaderTerminatesTheWalk /
SitWebbHeaderTerminatesTheWalk) + this lane's five rewritten pins
(SitEntryMarkerWalkSetsExtent [the magic's END] /
SitNewHeaderAtSectorBoundaryEndsTheWalk / SitBrokenMagicDrops [the
corrupt-terminal withdrawal pin] / SitByteSevenOutsideTheWindowIsRefused /
SitClassicSpaceFormDropsWithoutNextHeader) + the N-093 floor pin — **11/11
green; the full suite 1729/1729**. The racing lane's withdrawn
WpdZeroSizeFieldDeclines duplicate rider (the N-210 class, NINTH
occurrence — cr_tests could not build at the merged tree, first-hand)
merged into the original definition with both rationales kept.

---

## Session-94 addendum — the fourth convergent-lane concession record (the S94 lane; the WPD zero-size COVERAGE restoration filed as N-234)

This lane independently decoded the DWG member's full walk (gen-1's
section-locator table: count u32@+0x15, the 9-byte records, the
full-width id counter, the running max, the mismatch decline, the torn
salvage; gen-2's rand()-XOR security header: the +0x80 offset, the six
gates, qword@+0x34 + 0x6c) and separately re-derived the TIER-1 CENSUS
correction (the table is TEN pairs — `0x39303031` at 0x18002feae = the
AC1009 entry; the `cmp edx,0xa` bound — AC1009 gate-passed but
un-walked, the decline og-true) — both convergent with the racing
windows' landings and riders (`ddab68b4`/`f7e49eaf`/`d35e1cbb` +
`47c849bb`'s three-rider set, whose TenPairs test + third arm hold the
seats). This lane's DWG code, tests, census-rider form, and its own
N-229-class red-main filing all conceded/withdrew across three window
rebases. The convergence itself — multiple independent
instruction-level decodes agreeing — is the strongest verification class
the house records.

**The unique carry (N-234, filed+fixed): the WPD zero-size COVERAGE
restoration.** The windows' arbitration records state the zero-size
coverage was preserved three separate times (`8301c72b`: "the S87
seat's test keeps its name ... both batteries' coverage preserved";
`47c849bb`: "the second renamed ... both green"; this finding's own
closing text: "merged into the original definition with both rationales
kept") — yet the tips (`84a3670e`, `69a9a437`) carried ZERO tests for
the behavior anywhere in the tree (the whole-tree search, first-hand).
The extent!=0 engine-gate behavior (the landed validator's
`end <= hit` decline, corpus-decoded at 0x18000d7e1) shipped unpinned: a
future regression would pass the suite green. The S94 landing restores
the S87 seat's test verbatim against the landed validator — the
seat-holder's own recorded intent, re-implemented:
findings/FIXED/N-234. The class lesson: arbitration records are not
tree state.

**M-107's final state (the windows' close, recorded here for the
history):** all eleven members landed across the S71-S93 lanes (X3F's
chain router, CRW's CIFF heap walk, DWG's three generations, PSP's
~BK chain, WPG's zero-tail walk, WPD's version-table declared size,
SIT's entry-marker walk, EPS's %%EOF trailer scan, AI's header
formula, DXF's EOF-trailer scan, APE's CAPE walk via M-160 + the S87
continuation, SWF-RECT's validation gate) + the DWG gen-3 R2007
remainder resolved on the seat — the finding retired to FIXED/.
