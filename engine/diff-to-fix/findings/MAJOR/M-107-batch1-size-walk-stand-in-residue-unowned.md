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
