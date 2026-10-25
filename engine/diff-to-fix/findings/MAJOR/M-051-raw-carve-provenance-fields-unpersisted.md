# MAJOR M-051 — RAW carve provenance fields not persisted: the og's carved records carry a timestamp string, analyser checksum, note field, tail anchor, and a flag; the core's carve props set none

**Severity:** MAJOR
**Module:** carve + session
**Parameters affected:** D11 (node fields — five og RAW-record fields with no core carrier), D12 (session persistence — RAW records lose provenance), B9 (timestamp semantics — the og's carved timestamp is a formatted wall-clock string, not the file's time), Q46 (contract documentation)
**Corpus references:** `re-workspace/artifacts/SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md:77-90` (CRAWFileProperties grammar: `+0x1A` file size; `+0x22` FLAGS 0x0020; `+0x25` marker=1; **`+0x56` u32 analyser checksum/validation — 0xFFFFFFFF = invalid marker, 12,030/12,147 nonzero**; **second string `+0x5A` — "usually empty; analyser note family"**; **`+0x8C` qword header offset / carve anchor A**; **`+0x94` qword tail offset / carve anchor B**; **`+0xF4` u32 boolean**; `+0xF8` type-registry ID; runlist unconditionally present)
**Core references:** `src/core/carve/cr_carve.cpp:148-161` (props->name / fs_kind / logical_size / carve_format / carve_confidence / extents — nothing else set), `include/cr/tree/cr_node.h` FileProperties (no provenance fields), `src/core/session/cr_records.cpp` (no provenance serialization) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's carved-file record (CRAWFileProperties) is a provenance-rich structure: a formatted
timestamp string (written at carve time via `"%d-%d-%d %d-%d-%d"` from a FILETIME), the
analyser's validation checksum (with an explicit 0xFFFFFFFF invalid marker), a second
note-family string, the tail anchor offset, and a boolean flag — 12,030 of 12,147 sampled
records carry a nonzero checksum. The core's carve path sets none of these; the carved node
carries format id + confidence only, and `times` serialize as zeros.

## What the reference does

Per `ifileproperties-part3--btrfs-zfs-raw-generic.md:77-90` (the RAW record section — first
full read by any session): each carved file persists —

| Field | Offset | Semantics (corpus, census-verified) |
|---|---|---|
| formatted timestamp string | +0x5A-area (std::string) | written by `FUN_140c41aa0` via `"%d-%d-%d %d-%d-%d"` (SYSTEMTIME from a FILETIME arg); the SOURCE FILETIME is not persisted (corpus correction §5) |
| analyser checksum/validation | +0x56 (u32) | range 1..0xFFFFFFFF; **0xFFFFFFFF = invalid/unknown marker**; 12,030/12,147 nonzero |
| second string | +0x5A | "usually empty; analyser note family" |
| tail offset / anchor B | +0x94 (qword) | 261/12,147 nonzero (3,273..757,417) |
| boolean | +0xF4 (u32) | 2,431/12,147 = 1 |
| header offset / anchor A | +0x8C (qword) | 1,245/12,147 nonzero |

plus the name (+0x12), size (+0x1A), FLAGS (+0x22, see S-031), marker (+0x25), type-registry
ID (+0xF8, shared ID space with NTFS +0x60), and an unconditionally-present runlist.

## What our core does

`cr_carve.cpp:148-161` sets: name (namer), fs_kind=Raw, logical_size, carve_format (sig.id),
carve_confidence (averaged), one extent, optional sha128. `props->times` is never touched
(zero on the wire); there is no analyser-checksum field, no note field, no tail-anchor field,
no provenance boolean. The session writer has no carriers for them even if they were set.

## Gap

Falsifiable claim: for every carved file the og session carries a formatted carve timestamp,
a validation checksum (explicitly marked when invalid), and anchor fields; a core session
carries none of the five — reloaded carved records are indistinguishable from
freshly-carved ones and carry zero provenance.

## Impact

Post-resume surfaces lose provenance: the og's UI shows the carve timestamp on carved items
(when the file was found), the checksum supports support/forensics workflows ("was this
validated or marker-only?"), and the anchors support re-verification without re-scanning.
Support and power-user workflows that rely on them degrade silently. Also a S-023
interaction: the og's grading path (analyser checksum = an input to trust decisions in the
RAW record family) reads the checksum the core never records.

## Reproduction / verification

- Corpus: part3's RAW-record section read first-hand this session (the field table above is
  the artifact's own, with its census counts).
- Core: the carve emit path + FileProperties field list read first-hand; negative space
  verified (no provenance fields, times zero).

## Fix plan (for the engineering team)

1. Add optional provenance fields to FileProperties: `carve_time` (the core already has
   `times` — set `changed` to the carve wall-clock and document the mapping to the og's
   formatted-string field), `carve_checksum` (u32; 0xFFFFFFFF = invalid marker — populate
   from the analyser's validation state), `carve_note` (empty string carrier), `carve_anchor_b`
   (tail offset), `carve_flag` (boolean). Append-only; default zero/empty keeps old sessions
   valid.
2. Populate at emit time in `cr_carve.cpp` (checksum from the grammar validator's
   accept/reject detail; anchor B from tail-paired raw-sign hits; note from the analyser
   family where the og's analysers emit notes).
3. Serialize in `cr_records.cpp` (append-only block after carve_confidence).
4. Surface in the tree --json / report as informational fields (document them in the frozen
   contract update note).

## Tests to add

- `Carve.ProvenanceFieldsPopulated` (grammar hit → checksum nonzero; tail-paired hit →
  anchor B set)
- `Carve.ProvenanceInvalidMarker` (validator-reject-then-sign-hit path → 0xFFFFFFFF)
- `Carve.ProvenanceSessionRoundTrip`

## Related findings

- S-031 (FLAGS 0x20 — same record, same emit path; land as one carve-record wave)
- S-032 (inventory fields — same record family; same wave)
- S-023 (grading — the checksum is a grading input og-side)

## Open questions

The exact producer semantics of the +0xF4 boolean (2,431/12,147 set) is census-level only —
the fix leg should carry it as opaque and document it as UNDECODED rather than guessing.
