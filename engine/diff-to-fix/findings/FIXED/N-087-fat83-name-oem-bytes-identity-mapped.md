# MINOR N-087 — FAT 8.3 short names are identity-mapped to U+00XX instead of persisted as raw OEM-codepage bytes: non-ASCII 8.3 names display and restore with different glyphs than the reference

**Severity:** MINOR
**Module / Surface:** session / FAT engine name fidelity (modules/session.md, modules/fat.md)
**Parameters affected:** B10 (filename/Unicode handling — 8.3 OEM-codepage semantics), D11 (node fields — the short-name carrier)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/SaveScan/ifileproperties-part1--ntfs-fat-exfat.md:121` — the FAT name field: "8.3 name, **OEM-codepage bytes** — observed mangled control-byte names (`\x05\x01\x04\x01…`) = recovered deleted dir entries whose first char(s) were 0xE5/0x05-marked" (the og persists the raw bytes; the UI's codepage does the interpretation)
**Core references:** `src/core/fat/cr_lfn.cpp:113-129` (audited commit `dd82db9`) — `short_name_to_u16`: `s.push_back(u16(name[i]))` — a byte→U+00XX identity map for both the 8-char base and the 3-char extension; no OEM codepage conversion exists in the FAT name path (`rg -i "oem|cp437|codepage" src/core/fat/` → zero hits)
**Status:** FIXED — see Resolution below
**Prior audit check:** not in prior scope (N-029/FIXED covered UTF-8 replacement residuals on other engines' names; the FAT 8.3 OEM-bytes semantics were never dispositioned — part1 was cited once, for N-024, not the name field)

## Summary

The og persists FAT 8.3 names as raw OEM-codepage bytes (the codepage interpretation belongs to the display layer). The core identity-maps every byte to the code-unit of the same numeric value (byte 0x93 → U+0093), so any 8.3 name containing high-half OEM bytes (accented letters, box-drawing, the codepage's national glyphs — common on non-English Windows installs) renders and restores with different characters than the reference produces. ASCII names (the overwhelming majority) are unaffected.

## What the reference does

The part1 field census records the FAT name carrier as raw OEM-codepage bytes, with observed control-byte names from deleted entries (0xE5/0x05-marked first chars) surviving verbatim (`\x05\x01\x04\x01…`, part1:121). The og makes no transformation — the bytes travel from directory entry to session record to UI unchanged, and the OEM codepage (the mounting system's) supplies the glyphs.

## What our implementation does

`short_name_to_u16` (`cr_lfn.cpp:113-129`) builds the display name by widening each byte with `u16(name[i])` — the identity map. For bytes ≥ 0x80 the result is the Latin-1-range code unit of the same value (U+0080..U+00FF), which is NOT the glyph the OEM codepage assigns (e.g. CP437 0x93 = `ô` U+00F4, but the core yields U+0093, a C1 control character). The name then persists through the session and into recovery naming.

## Gap

Falsifiable claim: for a FAT 8.3 name containing any byte ≥ 0x80, the core's persisted name is the identity-mapped U+00XX sequence while the reference's is the raw byte sequence (interpreted as OEM codepage downstream); the two differ for every such name.

## Impact

- Name fidelity on non-ASCII 8.3 names only: display in results, session round-trip, and the recovered file's name all carry the wrong glyphs (control characters may even render as boxes/replacement).
- ASCII 8.3 names and all LFN names (UTF-16 carriers) are unaffected; no size/time/extent fields are touched.
- Real-world frequency is low (8.3-only names with high-half OEM bytes — legacy/non-English systems), hence MINOR.

## Reproduction / verification

Corpus doc evidence (strength-3): part1:121 quoted above. Core evidence (strength-3): the `short_name_to_u16` body quoted in Core references — the identity map is the entire transformation. An executed comparison would need an OEM-codepage fixture (a FAT image with a high-half 8.3 name); the static evidence is conclusive for the mapping difference.

## Fix plan (for the engineering team)

1. Decide the semantics: the og defers interpretation (raw bytes). The core's session/UI model is UTF-16 — the faithful equivalent is an OEM-codepage→Unicode conversion at the boundary (CP437 for US-market images is the corpus-observable default; the mounting system's codepage is the general answer). Apply it in `short_name_to_u16` behind the existing name-building call, keeping the 0x05-first-byte KANI marker behavior unchanged (N-029 lineage).
2. If raw-byte fidelity is chosen instead (matching the og exactly), the node name carrier needs a bytes-mode flag — larger change, registered as deliberate.
3. Tests: a FAT12/16 fixture with `0x93 0x41 0x42` (ôAB in CP437) in the 8.3 base → persisted name `ôAB`; the 0xE5-deleted first byte path unchanged; ASCII names byte-identical before/after.
4. Acceptance criteria: the fixture test green; the part1 citation added to the name-field comment.

## Tests to add

- `FatShortNameOemBytesDecoded` (fails before: yields U+0093).
- `FatShortNameAsciiUnchanged`.
- `FatDeletedFirstByte0x05MarkerUnchanged`.

## Related findings

- **N-029** (FIXED) — the UTF-8 replacement residuals on other engines (the same name-fidelity axis).
- The anchor-datum tracked-open item (this session) — the same part1 artifact's other under-consumed fields.

## Open questions

Which OEM codepage the og's UI actually applies (CP437 vs CP1252 vs system-default) is not decoded in part1 — the fix's step 1 default (CP437) may need a one-probe verification against the og UI's rendering before shipping; the divergence itself does not depend on it.

## Resolution (FIXED — the CP437 boundary decode; fail-before proven)

`short_name_to_u16` (`src/core/fat/cr_lfn.cpp`) now widens high-half
bytes through a full standard-verified CP437 table (128 entries,
byte-exact against the reference mapping; 0x20..0x7E pass through so
ASCII names are byte-identical) instead of the identity map — the
finding's step-1 semantics: the og defers to the mounting codepage; the
core's UTF-16 carrier gets the conversion at the boundary. The
0x05-first-byte KANJI escape and the deleted-marker rewrite are caller
behavior (cr_fat_quick.cpp:202-203, unchanged) — a 0x05-led live dentry
now decodes as 0xE5 → CP437 sigma (U+03C3), not the pre-fix U+00E5.

**Tests (fail-before PROVEN on the stash-reverted pre-fix build — both
behavior tests RED):**
- `FatEngine.ShortNameOemBytesDecoded` — byte 0x93 decodes to
  o-circumflex U+00F4 (the finding's own example); no name contains the
  identity-mapped U+0093.
- `FatEngine.ShortNameAsciiUnchanged` — DEL/INNER byte-identical.
- `FatEngine.ShortNameKaniMarkerPathUnchanged` — the full 0x05 → 0xE5 →
  CP437 U+03C3 path (the pre-fix identity map produced U+00E5).
- Suite after the fix: fully green (see the commit message for counts).

The open question (which codepage the og's UI applies) is preserved in
the code comment — CP437 is the corpus-observable default and the
divergence fix does not depend on it; a UI-rendering probe against the
og remains a future refinement if a non-CP437 target market appears.

**Landed:** commit "fix(fat): N-087 - the 8.3 short-name CP437 boundary
decode (the og's raw OEM bytes -> the core's UTF-16 carrier)".
