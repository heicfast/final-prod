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