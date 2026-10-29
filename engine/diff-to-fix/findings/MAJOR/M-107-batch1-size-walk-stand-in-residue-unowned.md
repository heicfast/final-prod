
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

This lane's third WPD/WPG window: the independent decode converged with
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
