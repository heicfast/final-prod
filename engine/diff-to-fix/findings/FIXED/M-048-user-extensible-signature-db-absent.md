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

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's M-048 landing (DFX33; sha in the worklog entry)
**Verified by:** `UserSigns.ParseTheOgsShape` (the og's EUUserData JSON shape parsed from real corpus entries incl. the "123"/cpl/epub rows; offset/hex/charset/unknown-key/trailing-garbage/empty-signs all rejected ATOMICALLY) + `CarveEngine.UserSignatureProbedAtStride` (fail-before EXECUTED: RED with the user-sign layer disabled — DEADBEEF at a 512-stride is invisible to every built-in, then exactly one .xyz record with the user tag 0xFFFE0000, confidence 600, the MIME on carve_note, the correct extent) + `CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition` (the collision rule: a user sign matching a built-in magic at the same stride never double-emits — the built-in PNG owns the position) + `CrUi.UserSignRegistrationSurface` (the ABI round-trip: register/count/info, atomic bad payloads, replace-on-re-register) + full suite **869/869** linux-release (incl. win_syntax_check + cr_ui_abi_check over the new exports).
**Resolution notes:** All five fix-plan items landed with one corpus-true scope correction on item 4: the og's EUUserData is an INSTALL-TREE CONFIG FILE re-read per scan — NOT session state — so the core matches that posture (the file-load path re-reads per run; sessions do not carry the set; re-registration is one flag, exactly the og's config-reload model; registered in the Resolution instead of a session field). (1) Schema: the og's shape verbatim (extension -> signs["offset,HEX"] + mime); (2) the runtime registration surface at the cr_ui boundary (ABI-since 2.1: cr_ui_register_user_signs / cr_ui_user_sign_count / cr_ui_user_sign_info — atomic install, replace-on-re-register, mutex-guarded) + the CLI --user-signs <file> loader (honest failure: unreadable/invalid aborts the scan, never a silent no-signatures run) + the pure parse/validate module (targeted JSON-subset reader for the og's exact shape — no general JSON dependency; caps: 256 entries / 8 signs / 16 bytes / offset ≤ 512; extension charset [A-Za-z0-9] folded lowercase); (3) the carve wiring: the user-sign layer probes at the raw-sign 512-stride AFTER the grammar layer's claims (emitted_at) and BEFORE the built-in raw-sign table — a user sign fires only where the built-ins did not (the finding's built-in-wins collision rule, proven by the collision test); emission carries the user's extension (the namer chain), the MIME on carve_note (the og's classification field), extent to the device end (user signs are header-only markers — no size grammar, no tail walk), confidence 600 (the FIXED documented value — EUUserData carries no confidence and the og's consumer confidence for them is undecoded; PARITY-DELTAS); the format tag 0xFFFE0000 (the user-signature space, distinct from the raw-sign 0xFFFF0000+fmt tags); (4) session posture per the corpus truth above; (5) all four named tests + the ABI round-trip. Error reporting follows D-68 (family convention only): rejections are ERR_BAD_PARAMETER with the diagnostic at the call site.
