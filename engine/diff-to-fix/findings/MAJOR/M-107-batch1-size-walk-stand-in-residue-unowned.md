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