# MINOR N-131 — the og's FS-native parent anchors are not persisted: NTFS parent-reference qword (+0x40), FAT parent-directory u32 (+0x42), exFAT +0x54 — the core keeps structural parent_id only

**Severity:** MINOR (representational datum; the hierarchy itself round-trips — the N-100 precedent class)
**Module / Surface:** session (record persistence model)
**Parameters affected:** P11 (node fields), P12 (session persistence format — field equivalence)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md:84` (NTFS qword +0x40 "parent reference" — serialized), `:129` (FAT u32 +0x42 "parent-directory anchor" — serialized), `:148` (exFAT u32 +0x54 — serialized; the anchor-twin reading per the vtable+0x30 family pattern, its census "type-registry ID" label undermined by the OQ-SA-1 refutation of the NTFS +0x60 twin); the OQ-SA-1 mask-read context `:61-65` (the parent-reference mask `*param_1 & 0xffffffffffff` — decode work that never dispositioned the PERSISTENCE of these slots)
**Core references:** audited commit `84a1832` — `src/core/session/cr_records.cpp:175` (`put32(out, parent_ids[i])` — the structural tree-index parent, the ONLY parent datum persisted), `:183` (record_ref — own identity, not parent), `:258-259`/`:268-269` (the OWN-file anchors that DID land: fat_origin_cluster bit7, ntfs_origin_lcn bit8); absence greps (first-hand): `rg "parent_anchor|parent_reference|parent_frn" src/ include/` → **0 hits**; `include/cr/tree/cr_node.h:209-216` (the +0x60 decline rationale — the analogous decision, N-100)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** OQ-SA-1 (FIXED) decoded the ANCHOR-DATUM family semantics (+0x3e/+0x50/+0x64 — the own-file anchors) and closed with those landings; the PARENT-anchor slots (+0x40/+0x42/+0x54) appear in its corpus references but its probe scope never covered their persistence. N-011a (FIXED) landed short_name; N-100 (ACTIVE) owns the +0x60 twin. The session-10 field-parity table (`evidence/s10-field-parity-table.md`) is the first per-field enumeration that isolates these three slots as unpersisted.

## Summary

The og persists, per node, the FS-native parent datum (NTFS: the parent FILE-reference qword; FAT: the parent directory's first-cluster u32; exFAT: the +0x54 twin) alongside the structural tree linkage. The core persists only the structural parent index (parent_id) — the tree hierarchy round-trips perfectly, but the FS-native lineage datum is dropped, so a reloaded session cannot cross-validate the reconstructed tree against the filesystem's own parent linkage (orphan re-derivation, cycle sanity checks, forensic lineage).

## What the reference does

part1's serialized grammars carry the parent anchors as first-class fields: NTFS +0x40 (qword, the parent-reference mask family OQ-SA-1 read at :61-65), FAT +0x42 (u32, parent-directory anchor), exFAT +0x54 (u32, the anchor twin). Whatever tree reconstruction the og performed at scan time, the session file retains the FS's own parent truth per node.

## What our core does

`serialize_tree` writes `parent_ids[i]` — an index into the same session's node array (cr_records.cpp:175). No FS-native parent datum is carried (grep: 0 hits for any parent-anchor concept). The own-file anchors landed (fat_origin_cluster, ntfs_origin_lcn — the OQ-SA-1 halves); the parent halves did not. The precedent decision for the adjacent +0x60 datum is recorded at cr_node.h:209-216 ("derivable… its vtable getter is a 0-stub for 8/10 classes") — but +0x40/+0x42 are NOT derivable from the persisted set (parent_id is an index, not an FS datum; the parent's own anchor would identify it, but the parent's record_ref for NTFS is… also not persisted as a parent linkage).

## Gap

Falsifiable: save a session for an NTFS volume, reload it, and ask "what was this node's parent MFT reference?" — the og's session answers (+0x40); the core's session cannot (only the tree index, meaningful only within the same session file).

## Impact

Session-file informational completeness (P11/P12). No recovery-correctness impact — recovery walks the structural tree, which round-trips. The lost capability is reload-time cross-validation and forensic lineage (which MFT record was the parent of this recovered file). MINOR per the N-100 precedent (the same class of declined datum).

## Reproduction / verification

Corpus-doc (part1's serialized grammar tables, cited) + core-source + grep (both directions, first-hand; the same pass re-verified the anchors that DID land, proving the method). Full table: `evidence/s10-field-parity-table.md`.

## Fix plan (for the engineering team)

1. Decide the disposition per N-100's pattern: either (a) land the carriers — `ntfs_parent_ref` u64, `fat_parent_cluster` u32, `exfat_parent_twin` u32 as nflags-gated append-only blocks (the bit-fires-only-when-nonzero convention, bits 512/1024/2048 — the cr_records.cpp:258-269 pattern), populated at walk time (the NTFS parent reference is already in hand where record_ref is stamped; FAT's parent first-cluster is the dentry-walk state) — or (b) register the decline in PARITY-DELTAS with the same derivability analysis cr_node.h:209-216 uses for +0x60 (note: unlike +0x60, these are NOT derivable — the register must say "declined: lineage datum, not consumed by recovery").
2. Tests if landed: `Session.NtfsParentRefRoundtrips`, `Session.FatParentClusterRoundtrips` (fail-before: field absent).
3. Update the field-parity table row in the module doc.

## Tests to add

(If landed) the two round-trip tests above + the bit-fires-only-when-nonzero shape test against a pre-fix session file.

## Related findings

**N-100** (the +0x60 twin — the precedent decision this finding mirrors) · **OQ-SA-1 (FIXED)** (the anchor-family decode; its scope boundary is why these slots fell through) · **M-050 (FIXED)** (the wrapper-scalar deliberate set — the same "carry or decline" discipline) · **erratum E69** (the exFAT/ReFS family's [INFERRED] label reliability)

## Open questions

The exFAT +0x54 semantics (anchor twin vs type-registry ID) rest on the family-pattern inference — the serializer trace that would settle it is the same trace E69 queues. The carrier should land only after that trace (fix-plan step 1's (a) path is gated on it).
