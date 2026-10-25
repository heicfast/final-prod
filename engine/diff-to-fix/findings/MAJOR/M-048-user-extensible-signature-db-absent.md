# MAJOR M-048 — User-extensible signature database (EUUserData) has no core counterpart

**Severity:** MAJOR
**Module:** carve (+ query extension surfacing)
**Parameters affected:** B1 (detection correctness — user-defined signatures never probed), A28 (carving validator logic — no user-signature grammar), D16 (filter model — per-signature extension/mime has no core field), Q46 (documentation of the contract — the capability is undocumented in core docs)
**Corpus references:** `og-codebase/EUUserData` (the file itself: 16,891-byte JSON, 111 entries, byte-verified this session — sample entry below); `re-workspace/artifacts/config-files/config-data-reference.md:95` (EUUserData documentation); `:122`-adjacent PARITY-DELTAS F-A3-13 (core's own parity doc registers the gap as a delta line only)
**Core references:** `src/core/carve/` (no custom-signature path: grep for any user-signature/config-load surface is empty), `include/cr/carve/*.h` (no API to register signatures at runtime) — audited commit `7cf6754`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og ships a user-extensible signature database: `EUUserData` is a JSON map of 111
extensions, each carrying user-defined file signatures (`"signs": ["<offset>,<hex bytes>"]`)
and a MIME type. The og's raw-carve layer consults it alongside raw_signs.json, so users can
teach the scanner new file types. Our core has no mechanism to load, register, or probe
user-defined signatures — the capability does not exist at any layer.

## What the reference does

`og-codebase/EUUserData` (read and parsed this session):

```json
{
  "123":  { "signs": ["0,00001A00051004"], "mime": "application/vnd.lotus-1-2-3" },
  "cpl":  { ... }, "epub": { ... }, "ttf": { ... }, "gz": { ... },
  "hqx":  { ... }, "doc":  { ... }, "mxf": { ... }, ...   // 111 entries total
}
```

Each entry maps an extension to one or more signatures — a signature is an `"offset,hexbytes"`
pair (offset from the candidate start, then the hex byte string) — plus a MIME type for
classification. `config-data-reference.md:95` documents it as the user-extensible signature
store; the og's carve pipeline (RAWSearchDLL raw-sign layer + the config singleton's filter
vectors) consumes signature sources beyond the compiled-in tables.

## What our core does

Nothing. The carve module has exactly two signature sources, both compile-time:
`cr_registry.cpp` (35 grammar formats) and `cr_rawsigns_data.cpp` (generated from the og's
139-entry raw_signs.json). There is no runtime signature registration API, no config-file
signature load, and no `EUUserData` reader. `docs/parity/PARITY-CORE-REC.md` F-A3-13 records
the file's existence as a delta line, but no finding has ever carried the capability gap.

## Gap

Falsifiable claim: a user adds a signature for a proprietary format (extension `xyz`, sign
`0,DEADBEEF`) to EUUserData in the og → the scanner carves `xyz` files. The same user on our
core has no equivalent action at any layer; `xyz` files are invisible to every scan mode.

## Impact

Power users (the og's audience for this feature — corporate formats, instrument formats,
game assets) lose the ability to extend recovery to their file types. The og's marketing and
UI surface custom-signature recovery; parity claims cannot cover the carve module while this
is absent. It is also the natural companion gap to M-024 (the RawConfig runtime-toggle
surface) — both are the og's runtime-configurable carve model.

## Reproduction / verification

- Corpus: the file itself parsed this session (111 entries; structure quoted above) +
  `config-data-reference.md:95`.
- Core: negative grep over `src/core/carve/` + `include/cr/carve/` for any
  signature-registration surface (none); the two compile-time sources enumerated.

## Fix plan (for the engineering team)

1. Define the core's user-signature schema (adopt the og's shape verbatim: extension →
   {signs: ["offset,hex"], mime}) — keep JSON for toolability; document it in the carve
   contract docs.
2. Add a runtime registration API at the cr_ui boundary (e.g. `cr_ui_register_user_signs`)
   AND a file-load path (default location mirroring the og's install-tree placement);
   validate entries (offset bound, hex length, extension charset) with honest rejection.
3. Wire user signatures into the carve pipeline as a third signature source AFTER the grammar
   registry and BEFORE/alongside rawsigns (probe at the raw-sign stride; emit with the
   user's extension and MIME in the carve format metadata).
4. Session/reload: user signatures must be re-registerable on resume (store the active set
   in the session header — append-only field).
5. Tests: registration validation, carve hit on a user signature, extension/mime surfacing,
   session resume with signatures, conflict behavior when a user signature collides with a
   built-in (built-in wins; user signature logged).

## Tests to add

- `Carve.UserSignatureRegisteredAndProbed`
- `Carve.UserSignatureValidationRejectsBadOffsetAndHex`
- `Carve.UserSignatureSessionResume`
- `Carve.UserSignatureCollisionWithBuiltIn`

## Related findings

- M-024 (RawConfig runtime toggles — the same runtime-configurable carve model; land as one
  wave)
- S-007 (missing built-in formats — the static counterpart of this dynamic gap)
- M-029 (query model — user signatures surface per-extension in the type filter)

## Open questions

The og's exact load-time precedence between EUUserData and raw_signs.json (does a user
signature for an existing raw-sign extension override or supplement?) is not documented in
the corpus — the fix leg should pick "supplement, built-in wins" and document it as a
deliberate choice if the corpus cannot answer (config-data-reference.md does not).
