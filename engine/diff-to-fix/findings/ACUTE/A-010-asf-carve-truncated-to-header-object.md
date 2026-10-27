# ACUTE A-010 — ASF/WMV/WMA carves are truncated to the ASF Header Object: every recovered file of the family is a header-only fragment

**Severity:** ACUTE
**Module / Surface:** carve / raw-signature engine (modules/carve.md)
**Parameters affected:** A28 (carving validator logic — end-of-file determination), B2 (recovery correctness — recovered bytes vs reference), C19 (the ASF header-object GUID), X47-adjacent (recovered size)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/SearchDLL/rawsearch-carving--deep-dive.md:87-91` — the BruteForce family contract, naming `BruteForce::CAsfAnalyser` explicitly: "parsers that **walk forward to EOF validating structure at every step** (vs. signature-snap recognizers). Used where a format has no crisp tail marker: **the analyser keeps consuming until the grammar breaks, then reports the last valid position as the file size**... no premature truncation." · `rawsearchdll--analyser-registry-census.md:105` (CAsfAnalyser in the @BruteForce namespace) · `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:1914-1923` — the registry static-initializer: key `0x75b22630` (the ASF header GUID's first dword, LE) → `BruteForce::CAsfAnalyser::vftable`, registry id 6 · `:41950-41965` CAsfRawFileInfo's constructor carrying the four ASF header sub-object GUIDs (file-properties/stream-properties/header-extension/codec-list) — the structural vocabulary the og validator walks
**Core references:** `src/core/carve/cr_analysers.cpp:1838-1860` — `validate_asf` (audited commit `dd82db9`): GUID check, `obj_size = rd64(s, hit + 16)` (the **Header Object's** size field), `r.end = hit + obj_size` — the extent ends at the header boundary; `src/core/carve/cr_registry.cpp:228-241` — the sole ASF registration (`FMT_ASF, "wmv", {GUID}, 16, 0, 64, 4ull<<30, 850, validate_asf`); `src/core/carve/cr_carve.cpp:326,333` — `file_len = cr.end - cand_abs` gates emission and becomes the carved node's extent (the recovered file's size); no other ASF path exists (`rg validate_asf src/ include/` → these three sites only)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** newly discovered (the family-semantics artifacts — rawsearchdll--family-bruteforce.md, rawsearch-carving--deep-dive.md — were NEVER-CITED by any prior finding per the session's corpus inventory; S-007's format additions were evidenced from the registry census, not the family walk semantics)

## Summary

The core's ASF validator accepts a candidate at the ASF header-object GUID and ends the carve extent at the **Header Object's own size field** — so every carved WMV/WMA/ASF file is a header-only fragment (typically a few KiB of the file-properties/stream-description objects, none of the media payload). The reference's CAsfAnalyser is a BruteForce-family walker: it keeps consuming ASF objects forward until the grammar breaks and reports the last valid position as the file size — the "maximize salvage, no premature truncation" contract the corpus states verbatim for this family. The recovered bytes for the entire ASF/WMV/WMA family are therefore wrong (truncated), with the files still presented as successful recoveries.

## What the reference does

The corpus's carving deep-dive defines the BruteForce family (CAsfAnalyser among the named members) as forward-walking structural validators: "walk forward to EOF validating structure at every step... the analyser keeps consuming until the grammar breaks, then reports the last valid position as the file size" (deep-dive:87-91). ASF is exactly the case the family exists for — a container with no crisp tail marker, whose real length lives in the data-object sequence after the header. The registry initializer binds key `0x75b22630` (the header GUID dword) to CAsfAnalyser (decompile:1914-1923), and CAsfRawFileInfo's constructor seeds the four header sub-object GUIDs (file properties, stream properties, header extension, codec list — decompile:41950-41965), the structural vocabulary of the header walk.

## What our implementation does

`validate_asf` (`cr_analysers.cpp:1840-1860`) checks the 16-byte header GUID, reads the Header Object's size at `hit+16`, applies sanity gates (`30 ≤ obj_size ≤ 2 GiB`, header object count 1..128), and sets `r.end = hit + obj_size` (capped at the hit window). The carve dispatcher (`cr_carve.cpp:333`) turns `cr.end - cand_abs` into the emitted node's extent — the recovered file's bytes. **No object walk past the header exists**: the data objects (the actual audio/video payload, the overwhelming majority of any real ASF file) are never included. The registration (`cr_registry.cpp:228-241`) is the only ASF path in the tree (verified by grep), so the truncation is not compensated elsewhere.

## Gap

Falsifiable claim: on any fixture containing an ASF/WMV/WMA file with data objects, the core's carved extent equals the Header Object's size field (± window cap) while the reference's carve extent extends to the last structurally-valid ASF object; the core's recovered file cannot contain media payload.

## Impact

- **Recovery correctness (B2)**: every ASF-family carve result is a header-only fragment presented as a recovered video/audio file — unplayable, wrong bytes, wrong size. This is silent wrong-result behavior from the user's perspective (the file "recovered successfully").
- **Recall**: the truncation also suppresses nothing — the fragment is emitted — so occupancy/dedup marking may additionally shadow later correct hits inside what would have been the file's real extent.
- **Scope**: the whole ASF family (WMV video, WMA audio) in every deep-scan carve pass.

## Reproduction / verification

Decompiled evidence (strength-2): corpus deep-dive:87-91 (the family walk contract, quoted verbatim above) + census:105 + decompile:1914-1923, 41950-41965 (the registry binding + the FileInfo's header-GUID vocabulary). Core evidence (strength-3, quoted exactly): `cr_analysers.cpp:1855-1858` — `r.ok = true; r.end = hit + obj_size;` — with `obj_size` read from `hit+16` (the Header Object size field). An executed comparison (build a small ASF fixture with a padded data object; carve with the core; compare the extent against the object sequence) is specified in the fix plan's acceptance criteria — the static evidence is already conclusive for the divergence itself.

## Fix plan (for the engineering team)

1. Extend `validate_asf` into an object walker: after the header object, iterate ASF objects at the current position — read each object's GUID (16 B) + size (8 B, includes the 24-B header), sanity-gate the size (≥ 24, ≤ remaining window), optionally recognize the four known header/extension sub-object GUIDs from the FileInfo vocabulary, and advance; stop at the first grammar break (unreadable, undersized, or non-advancing object) and set `r.end` to the last valid position (the family contract — no premature truncation, maximize salvage).
2. Keep the existing accept gates (GUID, header count) as the entry predicate; the walk only replaces the END determination.
3. Tests: (a) a synthetic ASF (header + 3 data objects + garbage) — extent must cover all valid objects; (b) header-only ASF (grammar break right after) — extent = header, unchanged; (c) torn final object — extent = last complete object (salvage semantics); (d) the registry sanity gates unchanged (count 0/>128 still rejects).
4. Contract/doc updates: the carve module row + CAPABILITY-MATRIX's ASF row move from "signature+header" to "structural walk (BruteForce family parity)".
5. Acceptance criteria: a byte-for-byte extent comparison on the fixtures above; the deep-dive's family contract quoted in the validator's comment.

## Tests to add

- `AsfCarveExtentCoversDataObjects` (fails before: extent == header size).
- `AsfCarveStopsAtGrammarBreak` (garbage after header → extent = header).
- `AsfCarveTornTailSalvagesLastCompleteObject`.
- `AsfHeaderCountGatesUnchanged` (0 / >128 reject).

## Related findings

- **S-007** (FIXED) — the common-formats family this validator belongs to; the format's PRESENCE was audited, its walk semantics were not (this finding's gap).
- **N-070** (FIXED) — the overlap-scoring/accept accumulation this walker must feed correctly.
- **S-037** — the same family artifacts' never-cited roster gap (FH11/CPT).

## Open questions

None.
