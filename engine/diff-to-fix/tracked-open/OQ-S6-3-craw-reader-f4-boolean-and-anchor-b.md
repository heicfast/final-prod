# OQ-S6-3 — CRAW reader semantics: the +0xF4 boolean and the anchor-B tail offset (session/record-4 layer)

**Filed by:** session-6 deep-continuation audit (agent B session-format read)
**Status:** RESOLVED (session 25, DFX25 — full decode, no code change required; evidence: `../evidence/decompiled-quotes/s25-oq-s6-3-craw-reader-decode.md`)
**Blocking:** (was: weakens/strengthens the tail-anchor sub-item of S-031 and the RAW payload field set of N-050 — resolved in the core's favor)

## RESOLUTION (session 25, 2026-10-26)

All three questions answered first-hand from DRWUI_decompiled.c
(FUN_140c40d80 :1875575 reader; FUN_140c41450 :1875961 producer;
props-vtables.txt :42 vtable census):

1. **+0xF4 = a PER-ANALYSER-TYPE marker: set to 1 ⟺ the analyser's type
   id == 0xA8 (168)** (FUN_140c41450: `if (iVar5 == 0xa8) *(u32*)(+0xf4)
   = 1`), in the same id space as the persisted type-registry ID +0xF8.
   None of the three filed hypotheses (tail-confirmed / truncated /
   inline-vs-extent) is correct. The corpus does not decode which format
   id 168 names; the sample's 2,431/12,147 = the type-168 population.
   The getter FUN_140c41780 has ZERO callers and is NOT in the vtable —
   no post-load consumer exists.
2. **+0x94 anchor B is WRITE-ONLY provenance at load**: the reader loads
   it without branching; the dual-anchor getter (FUN_140c41430, returns
   +0x8C AND +0x94) has zero callers and no vtable slot. The core's
   folding of the tail anchor into extent length loses NO reader
   behavior; M-051's carve_anchor_b persistence is provenance parity,
   exactly as landed. (Producer side decoded as a bonus: type 0x2A
   analyser results carry a tail extent pair at analyser +0xd8/+0xe0
   that the factory appends to the runlist and folds into the size —
   the same tail-pairing shape the core's raw-sign path implements.)
3. **+0x56 checksum is informational-only at load**: the reader performs
   NO validation; the checksum getter (FUN_140c413e0) is a dead
   accessor. The core's `carve_checksum` provenance carrier is the
   correct posture; a load-time validator verdict would be INVENTED
   behavior (the og has none) — not added.

The core's M-051 design is CONFIRMED correct on all three fields. The
only landing: the decoded semantics documented at the `carve_flag`
carrier (include/cr/tree/cr_node.h) + the evidence file. Related
findings S-031/N-050/N-075: no behavioral change required by this
decode (their landed shapes stand).

## The precise questions

1. **What is the u32 boolean at +0xF4 of the carved-file record (2,431/12,147 = 1 in the sample)?** (ifileproperties-part3:88.) Hypotheses: (a) a "tail-confirmed" flag (the file's tail anchor was validated); (b) a truncated/salvaged marker; (c) an inline-vs-extent data flag. The distribution (20%) does not obviously select any of these.
2. **Is the +0x94 anchor-B (tail offset) a reader-consumed field or write-only provenance?** If the reader consumes it (e.g., to bound reads or verify tails), the core's folding of the tail anchor into extent length loses behavior; if write-only, M-041's fix need not add a separate field.
3. **Does the reader's `FUN_140c40d80` validate the analyser checksum (+0x56) on load**, or is it informational-only? (Affects whether the core should persist a validator verdict for session-integrity checking.)

## The corpus artifacts that must be examined next

- `SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md` (the field table + the reader function name `FUN_140c40d80`).
- The CRAW/RAWSearchDLL decompile tree under `re-workspace/reversed/` at `FUN_140c40d80` — read the +0xF4/+0x94/+0x56 consumers.

## Why it was not resolved this session

The reader-side semantics require the decompile walk; the artifact documents the writer-side field census only.

## Related findings

S-031 (carve emission — the validation-payload/tail-anchor items), M-051+N-075 (node-field gaps — the RAW payload field set).

---

## Session-14 addendum (DFX14 — partially superseded by M-051's landing)

M-051 landed carve_anchor_b (the tail-paired exact position) with the og
semantics registered as the deliberate superset for the flag carrier. The
remaining open half (the CRAW reader's +0xF4 boolean semantics,
FUN_140c40d80) is unchanged; no new evidence this session.
