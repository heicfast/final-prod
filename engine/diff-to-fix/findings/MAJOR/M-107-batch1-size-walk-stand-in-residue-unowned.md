
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
