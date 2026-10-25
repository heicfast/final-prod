# SEVERE S-005 — ext4/btrfs/ZFS filenames byte-lifted to UTF-16 instead of UTF-8 decoded (mojibake)

**Severity:** SEVERE
**Module:** ext4, btrfs, zfs (cross-engine)
**Parameters affected:** B10 (filename/Unicode handling), D11 (node fields), D13 (report schema — name correctness)
**Corpus references:** `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md` :326 — "char name[name_len] (buffer at node+0x1D8; **UTF-8**, reader null-terminates)"; agent-E PE import census (`scratch/probes/`): EXT4SearchDLL imports **no** MultiByteToWideChar/WideCharToMultiByte (passthrough of on-disk UTF-8 bytes)
**Core references:** `src/core/ext4/cr_ext4_engine.cpp:400–403` (`for i in name_len: u16 c = p[off+8+i]; de.name.push_back(c)`), `src/core/btrfs/cr_btrfs_engine.cpp:708–709` (`de.name.push_back(u16(di[DI_NAME + i]))`), `src/core/zfs/cr_zfs_engine.cpp:1291–1292` (`for (char ch : e.name) p->name.push_back(u16(u8(ch)))`) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

ext4, Btrfs, and ZFS store filenames as UTF-8 bytes. The og passes those bytes through its
narrow-string props pipeline into the UTF-8 `.rsf` name field — names arrive intact. Our
core lifts each **byte** into one UTF-16 code unit. Every non-ASCII filename on those three
filesystems becomes mojibake in our results ("é" → "Ã©", "文件" → "æ–‡ä»¶"), and is
double-encoded when the report layer converts the u16 name back to UTF-8.

## What the reference does

The .rsf record-4 grammar stores node names as UTF-8 `char` arrays (rsf-format :326). The
ext4 engine (and by the same shared-props construction btrfs/ZFS) holds the on-disk name
bytes in a narrow string without conversion — EXT4SearchDLL's import table contains no
conversion API (agent-E probe, `scratch/probes/EXT4SearchDLL.dll-pe.txt`), so the bytes
flow: disk (UTF-8) → props (char*) → .rsf (UTF-8) → UI (decoded correctly at display).
Filenames with CJK, accents, emoji, or any non-ASCII content survive end-to-end.

## What our core does

Per-byte lifts (quoted above from all three engines). Example: the UTF-8 bytes of "é"
(0xC3 0xA9) become the two code units U+00C3 U+00A9 — the classic Latin-1 mojibake
"Ã©". The report writer then encodes that damaged u16 string to UTF-8, producing
double-encoded output bytes.

## Gap

Falsifiable claim: for an ext4/Btrfs/ZFS file named "照片.jpg" (UTF-8: 12 bytes), the og's
persisted name is the correct UTF-8 byte sequence and our core's persisted name decodes to
"ç§ç‰‡.jpg" (mojibake). File content is unaffected; the name is wrong in results,
reports, sessions, and the UI.

## Impact

- Any user with non-ASCII filenames on Linux filesystems (extremely common — CJK locales
  are the primary EaseUS market for these engines) sees garbage names.
- Recovered-file naming: files are written under the mangled name — user-visible defect in
  the recovered output tree itself.
- Session round-trips persist the mojibake (permanent in saved sessions).
- Bench inventories that key on names fail to match ground truth for non-ASCII fixtures.

## Reproduction / verification

- Corpus-derived: rsf-format :326 quote; PE import census (absence of conversion APIs in
  EXT4SearchDLL).
- Core-derived: three code quotes (byte-lift loops).
- Executed: not run against og; the delta is fully determined by the quoted code paths.
  Unit repro: `parse_dir_block` on a dir entry with UTF-8 name bytes → assert decoded
  u16 equals the true code points (fails before).

## Fix plan

1. Add `std::u16string utf8_to_u16(std::string_view)` to a shared location
   (`include/cr/cr_bits.h` or a new `include/cr/cr_text.h`) — strict UTF-8 decoder with
   U+FFFD replacement for invalid sequences (the report layer already uses U+FFFD for its
   output; reuse the same policy). Do NOT use locale-dependent conversions.
2. Replace the three byte-lift loops:
   - ext4 `cr_ext4_engine.cpp:400–403` → `de.name = utf8_to_u16({p+off+8, name_len})`
   - btrfs `cr_btrfs_engine.cpp:708–709` → same for `di+DI_NAME`
   - zfs `cr_zfs_engine.cpp:1291–1292` → same for `e.name`
3. Audit the orchestrator's synthesized names (lost-partition volume labels, "InodeNNNN")
   for the same defect class (D-sheet flagged the orchestrator-synthesized set).
4. Files: 3 engine files + 1 shared header; tests in the three volume test files.
5. Tests: each engine gets a `NonAscii_Name_Decoded_Not_ByteLifted` case with CJK + accent +
  invalid-sequence inputs (assert U+FFFD on invalid, not garbage).
6. Report-layer check: `tree --json` output for a CJK-named ext4 file must contain the
   correct UTF-8 bytes (regression through the whole pipeline).

## Tests to add

(fix plan steps 5–6 — all fail before, pass after)

## Related findings

- Q68 (docs): `docs/recovery-report-schema.md` should document the name encoding contract
  (u16 internally, UTF-8 on the wire) once fixed.

## Open questions

None.
