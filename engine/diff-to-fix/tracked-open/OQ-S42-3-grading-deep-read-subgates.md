# OQ-S42-3 — the grading deep-read sub-gates: FUN_1401bc5a0's predicate, the +0x72-bit-7 / +0x74-bit-2 flags, vt+0x28's sector-size source, vt+0xb8 / vt+0x100's meaning, and the run-unit value local_b8[0]

**Origin:** S42 continuation lane, grading focus battery (M-121's decode residue)
**Status:** RESOLVED-IN-PART (session-48: 5/6 sub-gates decoded first-hand — Q1 addr-set membership, Q2 the claimed flag, Q3 the path-analysis name flag, Q4 GetSectorSize device geometry, Q6 the engine run-unit; Q5 narrowed to the C<X>VolumeAnalyze family with the no-binary blocker; only Q5's slot-23/32 backing fields remain)

## The precise questions

M-121 decoded `FUN_14038c470` (the og's sector-content grade validation) structurally; five sub-gates inside and around it remain undecoded, and M-121's fix plan needs them (or a deliberate ship-without):

1. **`FUN_1401bc5a0(mgr, node, addr)`** — the first element of the param_3==0 three-part pre-check (`DRWUI:605355-361` region per the call-site decode): what does it test (an addr-map membership? a device liveness probe)?
2. **Bit 7 of node+0x72** — the flag the pre-check ANDs (it is ALSO the QuickScanDeleteExtend increment condition at `:331164-170` — same bit, two consumers; what sets it? deletion-state? scan-phase?).
3. **Bit 2 of node+0x74** — the deep-read-skip bit (its setter `FUN_140389570` sets bit 1; the getter reads bit 2 — who sets bit 2?).
4. **`vt+0x28` on the reader chain** — the sector-size source `FUN_14038c470` uses at `:606353` (device geometry? a fixed 512?).
5. **`vt+0xb8` / `vt+0x100` on the NTFS device object** — the grade-0 sub-rule at `:605901-907` (`vt+0xb8()!=0 || vt+0x100()==0xa8 → grade 0`); `vt+0x100` is also compared to `0x23` in `FUN_1401c76e0` (`:330987`) — a device/type code space (0xa8 is also the partition-struct size stride elsewhere — coincidence suspected but unproven).
6. **The run-unit `local_b8[0]`** (`:605939`) — the multiplier in the device-walk's address key (the core's `kDeviceUnit = 2^40` is a registered stand-in; the og's true value changes only key scaling, not collision sets).

## Next probe (exact)

All sites are in `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`: read `FUN_1401bc5a0`'s body; find the writers of node+0x72 bit 7 and node+0x74 bit 2 (grep the setters); read the reader chain's `vt+0x28` implementor (the CStorageDeviceWithCache family); read the two NTFS vtable slots' implementors (the NTFSSearchDLL controller class); read `FUN_1401bb580`'s out-param fill for `local_b8`. Each is a single function-body read.

## Consumers

M-121 (its fix plan's step 4), M-120 (the unit value), N-158 (the bit-7 QSDE semantics), M-119 (the vt+0x100 device-code space if it overlaps the boundary semantics there).

---

## Resolution (session-48 lane, 2026-10-29 — five of six sub-gates DECODED first-hand; the sixth narrowed to a material blocker)

**Status:** RESOLVED-IN-PART (5/6 answered; Q5's slot semantics narrowed with the object family identified and the precise blocker recorded). The OQ stays open ONLY on Q5's vt+0xb8/vt+0x100 backing-field semantics; the decode products below are the fix-plan inputs M-121's record lacked.

### Q1 — FUN_1401bc5a0: ANSWERED (addr-set membership, not liveness)

Body read at `:322676-322714`: when the caller's addr == -1, `FUN_1401bc410(mgr, node, &addr, …)`
resolves the node's address (the same resolver the QSDE path calls at `:331323`); then
`FUN_1401b6b90(mgr)` walks the singleton (`FUN_1401b19d0`) via its vtable+0x78 getter to a
container, and `FUN_14035abe0(container, node, addr)` (`:575161-575177`) tests
`FUN_1404ec7d0(container->+0x10, addr)` — the SAME set-membership primitive the QSDE dedup
uses at `:331146` (not-in-set → process + count). **It is an addr-map membership probe
(the claimed/seen-address set), not a device liveness test.** A null container returns 0.

### Q2 — node+0x72 bit 7: ANSWERED (the node-claimed flag; deletion-state independent)

The runtime SETTER is `FUN_140638400` (`:1037337-1037372`): when a node passes the
collector's predicate, it is attached (collector+0x10 = node; a map insert keyed by
node+0x50) and `node+0x72 |= 0x80`. The bit is PERSISTED: serializer `FUN_140088940`
writes it (`:133357` `>> 7`); deserializer `FUN_140088af0` restores it (`:133393`, `:133401`);
node init zeroes the byte (`:132966/:132969`). The getter `FUN_1401b3550` (`:317506`) is
read at 6 sites: the M-121 pre-check element 2 (`:606358`), the QSDE counter split
(`:331164` — claimed nodes bump +0x1bc, unclaimed bump +0x1b8), `:319323`, `:330644`,
`:612174`, and the serializer. **It is the claimed-by-a-view/collection flag — set at
attach time, independent of deletion state and scan phase.** (N-158's "QSDE semantics"
question: the QSDE increment counts CLAIMED vs UNCLAIMED nodes — the deleted-vs-found
split is carried by the OTHER counters, not this bit.)

### Q3 — node+0x74 bit 2: ANSWERED (the name-set-by-path-analysis flag)

Bit 2 has exactly TWO runtime setters, both immediately after `FUN_140629120(lVar,
node)` — the function that converts a wide path to UTF-8 and pushes it through the
node+0x30 setter chain (`:1027882-1027939`): `:1028061` (inside `FUN_140629260`'s
path-processing) and `:1034509` (inside `FUN_140633be0`'s wide-char `0x3a`+`0x5c` = `:\`
drive-letter scan). The bit is copied on clone (`:133508-133510`, mask 4). The getter
`FUN_1403895a0` (`:605061`) has ONE caller: the M-121 pre-check element 3 (`:606359`,
skip the deep read when set). **Bit 2 = "this node's name was (re)derived by the
path-analysis flow" — the deep-read skip marks nodes whose identity was already
re-established by name.** Bit 1 (set by FUN_140389570 at `:606414`, called when grade==1
and param_4) is the SEPARATE "deep-read validated, grade 1 confirmed" flag — the OQ's
setter/getter bit asymmetry resolved: they are different flags in the same byte.

### Q4 — vt+0x28: ANSWERED (IStorageDevice slot 5 = GetSectorSize; device geometry, 4Kn-capable)

`local_1090` is the node's device, resolved through the singleton chain
(`FUN_1401b54f0` primary; `FUN_1401b4fd0`/`FUN_1401b50a0`/`FUN_1401b50f0(mgr, node)`
fallbacks, `:606316-606330`); `local_1088 = device->vt[+0x28]()` (`:606353`). The usage
PROVES it is the sector size: `local_1084 = local_1088 - 0x10` then
`memcmp(buf + local_1084, zeros, 0x10)` (`:606403-405`) indexes the LAST 16 bytes of the
one-sector read (`device->vt[+8]` = slot 1 = Read, `:606380/:606396`). **Device geometry
via the IStorageDevice slot-5 getter (the A4 row's mapping holds); NOT a fixed 512 —
the tail compare is 4Kn-correct by construction.**

### Q5 — vt+0xb8 / vt+0x100: NARROWED (the object family decoded; the slots' backing fields blocked at the corpus's material limit)

The object is the per-volume **C<X>VolumeAnalyze family**: `CNtfsVolumeAnalyze`'s
constructor (`:794466-794482`) allocates 0x78 bytes, stamps +8 = 0xE (the pre-parse
default), boot-parses, and on NTFS detection rewrites +8 = **3** (the NTFS FS code,
the Q1 factory space) — so **vt+0xa0 (slot 20) = the FS-code getter** (the grading
`== 3` test). The 100-byte region at +0xc is the UTF-16 VOLUME NAME, filled by
`FUN_140c4a2c0`'s $VOLUME_NAME (type 0x60) MFT-record parse (`:1881864-1881882` via
`FUN_140c4a060`'s MFT-geometry read). The CFatVolumeAnalyze twin is allocated in the
same factory loop (`:794495+`). The sub-rule itself re-confirmed on-disk
(`:605901-907`): `vt+0xb8()!=0 || vt+0x100()==0xa8 → grade 0`, preceded by the
slot-22 name-keyed lookup returning 0..8 → grade 2. **What remains undecoded: slots
23/32's backing fields and the 0xa8/0x23 code-space semantics.** The corpus ships NO
DRWUI.exe binary (re-workspace/reversed/DRWUI.exe is the rabin2/strings artifact
DIRECTORY; the vtable layout lives in the binary's .rdata, which no corpus artifact
dumps — Ghidra's flat decompile does not recover vtable slot tables). **Next probe
(exact):** a Ghidra/r2 re-run WITH vtable recovery on the real DRWUI.exe (not in
corpus — requires the upstream binary), or the DRWUI_blocks.txt gaining a vtable
section. Until then the M-121 record treats slot 23 as "a per-volume bad/declined
state" and slot 32's 0xa8/0x23 as "a per-volume code space" — both sufficient for the
core's grading parity (the core's own sub-rules are the C5 family: size<0x400 → 2,
kind {1,4} → −2 — all already implemented).

### Q6 — the run-unit local_b8[0]: ANSWERED (the engine's run granularity, NOT a constant)

The address key formula decoded at BOTH sites: `local_88 = extent_start + device_base +
run_index * run_info[0]` (`:605939` and `:605932`). `FUN_1401bb580(node, &extent_start,
run_info_out, &device_base)` (`:322031-322066`) fills them: device_base = the node's
device vt+0x68 slot; extent_start/run_info = the ENGINE's extent resolver
(`engine->vt[+0x30]`, engine = `FUN_1401b65b0(mgr, node->handle)`), with the node's own
vt+0x10 getter as fallback. **local_b8[0] = run_info[0] = the per-node run/extent unit
(cluster/extent granularity) from the engine's resolver — a variable, not a fixed
multiplier.** The core's `kDeviceUnit = 2^40` registered stand-in therefore changes only
key scaling exactly as suspected — collision sets are identical for any unit ≥ the max
run span (M-120's record stands; the stand-in is now decode-anchored).

**Evidence bar:** every cite above re-read first-hand in
re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c this session (the line numbers are
the on-disk lines). Consumers: M-121/M-120 (FIXED — this decode is the supplementary
anchor), N-158 (the QSDE semantics answered under Q2 — the bit is the claimed flag, the
deleted split rides the other counters), M-119 (UNAFFECTED — the vt+0x100 code space is
the VolumeAnalyze per-volume family, not the partition-boundary semantics).

## Session-52 update (2026-10-29 — the M-134 landing)

The deep read itself LANDED (findings/FIXED/M-134): the reader, the two
threshold rules, the graded-bit one-shot, and the full-tree re-grade shape.
What remains for this OQ is exactly the pre-check trio at :606355-361 —
`FUN_1401bc5a0(mgr, node, addr) ∧ bit-7-of-+0x72 ∧ ¬bit-2-of-+0x74` → the
presumptive grade-0 WITHOUT reading. This session's control-flow decode adds
two constraints for the next probe: (1) the trio's failure path goto-target
is the read logic (LAB_14038c622) — i.e., a failing pre-check is a SKIP of
the presumptive demote, not a skip of the whole function; (2) the "graded"
mark (FUN_140389540) sits at the epilogue — the presumptive path DOES fall
through to it. Next probe sites: FUN_1401bc5a0's predicate body (the manager
lookup keyed on the node), and the +0x72/+0x74 flag setters (search the
props-builder for the byte writes — the bit-2-of-+0x74 "deep-read-skip"
readiness flag and the bit-7-of-+0x72 candidate flag).
---

## Session-50 disposition (2026-10-29 — RESOLVED by the continuation audit lane's probe)

**RESOLVED — the full sub-gate list decoded** (first-hand, DRWUI_decompiled.c
:606291-606421 + the two callers; the evidence sheet:
`evidence/decompiled-quotes/s50-grading-deep-read-subgates.md`):

- **G0** — early-outs: null file / the session-restored graded bit
  (+0x73 bit 2, FUN_1401c5da0) / unresolvable device -> return ungraded.
- **G1** — the no-read grade-0 gate (mode 0): the offset in the global
  key-set (+ flag bit7 of +0x72 set + bit2 of +0x74 clear) -> grade 0
  without any read.
- **G2** — the prev-sector tail gate: ONE sector read BEFORE the file's
  first sector; its last 16 bytes all-zero -> tentative grade 2 (mode 0),
  else tentative 1.
- **G3** — the first-sector zeroed gate: the first 0x20 bytes all-zero ->
  grade 0.
- **G4** — the first-sector 0xFF gate: the first 4 bytes all-0xFF ->
  grade 0.
- **G5** — the param_4 flag hook: tentative 1 + param_4 -> set bit1 of
  +0x74.
- **G6** — the commit: MAX-ratchet (a committed 2 is never downgraded to
  0 — FUN_140089630 :134188-134204) + the deep-read-done bit (+0x73 bit 3).
  Mode 1 (the ReFS preview pass, FUN_1401c7d80 :331272-331289) can only
  upgrade 0->1.

Caller A (FUN_14038ac90 :605726-605800) re-decoded with the twin-key
confirmation: the -2 index shift applies to FS types 1/4 (FAT16 and the
EXT2-family) exactly as the M-133/M-138 filings decoded (:605753-756), and
the PREVIOUS twin on collision is upgraded to grade 2 — the deep read runs
only for non-twins.

**Consumers closed:** M-134 (LIVE per the battery) now has the decoded
sub-gate content for its fix plan (the absent deep-read validation is the
finding; this decode is the specification). The OQ's own question is
answered — the tracked-open seat closes; M-134 carries the remaining work.

**S50/S52 reconciliation (at the rebase):** the S52 landing's M-134 fix and
this lane's decode rode the same evidence — convergent. The S52 update's two
open constraints are ANSWERED by this lane's decode above: (1) the failing
pre-check's goto-target LAB_14038c622 is the commit path (the presumptive
demote is skipped, the function continues — exactly the S52 note's
reading); (2) the graded mark sits at the epilogue and the presumptive path
falls through to it (confirmed at :606416-417). The predicate body is
decoded: FUN_1401bc5a0 (:322676-322715) resolves the node's offset key and
tests membership via FUN_14035abe0 (:575161-575178) -> FUN_1404ec7d0
(:820130-820143, the vtable+0x20 contains(key)); the flag getters are
FUN_1401b3550 (:317506-317510, bit 7 of +0x72) and FUN_1403895a0
(:605061-605065, bit 2 of +0x74). **The OQ's remaining scope narrows to the
flag-SETTER origins** (which pipeline stage writes bit7-of-+0x72 and
bit2-of-+0x74 — the S52 note's props-builder byte-write search); the
sub-gate decode itself is closed on both lanes' evidence.
