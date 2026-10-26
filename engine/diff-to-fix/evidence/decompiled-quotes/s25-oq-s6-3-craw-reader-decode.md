# s25 — OQ-S6-3 decode: the CRAW reader's +0xF4 / +0x94 / +0x56 consumption (DRWUI)

Session 25 (DFX25), 2026-10-26. Corpus `03ad46c`. All quotes first-hand from
`re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` + the vtable census
`re-workspace/raw-evidence/props-vtables.txt` (CRAWFileProperties vftable
@ 0x140f5f528).

## 1. The reader (FUN_140c40d80, :1875575) — pass-through, no validation

The reader loads every CRAW field opaquely through the stream vtable +0x70
(`read(dst, n)`) and NEVER branches on any of the three fields:

```c
(**(code **)(*param_2 + 0x70))(param_2,param_1 + 0x56,4);   /* checksum — loaded, NO verify */
...
(**(code **)(*param_2 + 0x70))(param_2,param_1 + 0x8c,8);   /* anchor A */
(**(code **)(*param_2 + 0x70))(param_2,param_1 + 0x94,8);   /* anchor B */
(**(code **)(*param_2 + 0x70))(param_2,param_1 + 0xf4,4);   /* the boolean */
(**(code **)(*param_2 + 0x70))(param_2,param_1 + 0xf8,4);   /* type-registry ID */
uVar2 = FUN_14051e440(param_2);                              /* DataRunList — unconditional */
```

There is no comparison, gate, or read-bounding on +0x56/+0x94/+0xF4 anywhere
in the reader. The runlist is consumed unconditionally (the artifact's
"carved files always carry ≥1 run" observation).

## 2. The +0xF4 producer (FUN_140c41450, :1875961) — a PER-ANALYSER-TYPE marker

The record factory that copies an analyser result into a fresh
CRAWFileProperties sets the boolean iff the analyser's type id is 0xA8:

```c
iVar5 = *(int *)(param_2 + 0x18);        /* the analyser's format/type id */
if (iVar5 == 0x2a) {                     /* type 42: tail-extent folding */
    lVar9 = *(longlong *)(param_2 + 0xe0);
    if ((*(longlong *)(param_2 + 0xd8) != 0) && (lVar9 != 0)) {
        FUN_14063f890(local_f8, *(longlong *)(param_2 + 0xd8), lVar9 + 0x1ffU >> 9);
        lVar9 = *(longlong *)(param_2 + 0x10) + lVar9;      /* size += tail len */
        *(longlong *)(param_2 + 0x10) = lVar9;
        *(longlong *)(param_1 + 0x1a) = lVar9;
    }
}
if (iVar5 == 0xa8) {                     /* type 168: THE BOOLEAN */
    *(undefined4 *)(param_1 + 0xf4) = 1;
}
*(int *)(param_1 + 0xf8) = iVar5;        /* the type id itself is persisted */
```

So **+0xF4 = "this carved file is analyser-type 168"** — a per-format flag
in the same id space as +0xF8 (the sample: 2,431/12,147 = the type-168
population; the corpus does not decode which format 168 names — the id
space is the RAWSearchDLL analyser registry, e.g. 35 = the shot*.jpg set).
None of the OQ's three hypotheses (tail-confirmed / truncated /
inline-vs-extent) is correct.

Bonus decode (the anchor-B PRODUCER side): type 42 (0x2A) analyser results
carry a tail extent pair (analyser +0xd8 offset / +0xe0 length) that the
factory appends to the runlist and folds into the size — the tail-pairing
model the core's raw-sign path already implements (M-051's anchor-B
population "ONLY on the raw-sign tail-paired path" is the same shape).

## 3. The getters are DEAD accessors (vtable proof)

- `FUN_140c413e0` (get +0x56 checksum), `FUN_140c41430` (get +0x8C AND
  +0x94 — one dual-anchor getter), `FUN_140c41780` (get +0xF4): **zero
  call sites** in the whole DRWUI decompile (grep-verified).
- The CRAWFileProperties vtable (props-vtables.txt :42-60) contains NONE of
  them — the 17 slots (0x00..0x88) hold 140c41300/140c41c40/140c412f0/
  140c41320/1400d6270/140c41cc0/140c41330/140c41000/140c41a90/140c413a0/
  140c41070/140c41370/140c41340/140c41420/140c41fb0/140c41260/140c41350/
  140c41360 — name, size, flags, +0xEC, timestamps, runlist accessors).
  Being outside the vtable AND uncalled = no virtual dispatch consumer
  either.
- The setters (FUN_140c41df0 checksum, FUN_140c41fc0 anchors A+B) are
  called only from the carve-record factories (:1880336/:1881288 etc.).

## 4. Consequences for the core (all three questions answered)

1. **+0xF4**: a type-168 marker, produced per-analyser-type, round-tripped
   opaquely, NEVER consumed after load. The core's `carve_flag` carrier
   (M-051: "carried and round-tripped, never invented") is the correct
   posture; the core has no og-analyser-type-id space, so no producer is
   added (inventing one would be hallucination). The semantics are now
   documented at the carrier (cr_node.h).
2. **+0x94 anchor B**: WRITE-ONLY provenance at load — the reader never
   bounds reads or verifies tails with it; the dual-anchor getter is dead.
   The core's folding of the tail anchor into extent length loses NO
   reader behavior; M-051's `carve_anchor_b` persistence is provenance
   parity, exactly as landed.
3. **+0x56 checksum**: informational-only at load — the reader performs NO
   validation and the getter is dead. The core's `carve_checksum`
   provenance carrier (CRC32 re-verification value) is the right shape;
   adding a session-load validator verdict would be INVENTED behavior (the
   og has none).

**OQ-S6-3 RESOLVED — no code change required** (the core's M-051 design is
confirmed correct on all three fields; the only landing is the decoded
semantics in the carrier's comment + this evidence file).
