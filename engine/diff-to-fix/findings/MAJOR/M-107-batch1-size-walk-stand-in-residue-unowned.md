
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

## Session-93 addendum — the convergent-lane concession record + the TIER-1 CENSUS correction (a rider the seat-holder's landing carries a false prose claim about)

**The concession.** This lane independently decoded the DWG member
(the full gen-1 section-locator walk and the gen-2 rand()-XOR
security-header walk — count u32@+0x15, the 9-byte records, the
full-width id counter, the running max, the torn salvage; the +0x80
decrypt offset, the field-gate set, qword@+0x34 + 0x6c) and built an
eight-test battery plus the S-007 plant re-pin — all convergent with the
racing window's landing (`ddab68b4` + the `f7e49eaf` gen-2 offset
repair + the `d35e1cbb` S86 riders). Per SA-9 the earlier push holds
the seat; this lane's code and tests conceded at the merge. The
convergence itself is the strongest verification class the house
records: two independent instruction-level decodes agreeing on every
material point.

**The census rider (the unique carry — a corpus-truth correction the
seat-holder's comment gets wrong).** The landed comment above
validate_dwg claims the og's tier-1 table (FUN_18002fe30) is "exactly
these nine" versions {12,13,14,15,18,21,24,27,32}. The raw asm says
TEN: the stack table runs "AC10"+{32,27,24,21,18,15,14,13,12} and then
one more entry — `mov DWORD PTR [rbp-0x18],0x39303031` (0x18002feae:
bytes 31 30 30 39 = "AC10" + "09") = **AC1009**, and the loop bound is
`cmp edx,0xa; jl` (0x18002fede-0x18002fee1) — indices 0..9, TEN
entries checked. So the og's tier-1 ACCEPTS AC1009 (stamps the result
id 0x18 at 0x18002fef7) — and then the walk callback (FUN_18002f320)
declines it, because AC1009 matches no generation table (gen-1
{12,13,14,15} at 0x18002f345-0x18002f39f, gen-2 {18,24,27,32} at
0x18002f478-0x18002f4d3, gen-3 {21}) — the size stays 0, no emission.
The CARVE BEHAVIOR of the landed nine-version gate is correct (AC1009
declines either way); the MECHANISM prose is wrong: AC1009 is
tier-1-accepted-but-walk-declined, not tier-1-rejected. The correction
lands as: the comment re-truthed (ten entries, the AC1009
tier-1/walk-split recorded), and the seat-holder's
`DwgVersionGateIsTheOgsNine` test re-named to what it actually proves
(the gate matches the og's three generation tables; AC1009 declines
through them).

The AC1009 tier-1 acceptance has one consequence worth recording for
future censuses: the og's magic-key registry census (the
analyser-registry doc's DWG row) derives from the TIER-1 table, so any
future count of "DWG versions the og recognizes" must say TEN, with the
carve-effective set being the nine (the walk's three tables).
