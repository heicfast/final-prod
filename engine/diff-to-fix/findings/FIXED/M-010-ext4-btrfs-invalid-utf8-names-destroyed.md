# MAJOR M-010 — ext4/Btrfs names with invalid UTF-8 bytes are destroyed (U+FFFD baked); the og preserves the raw bytes

**Severity:** MAJOR (edge-case coverage: permanent loss of the original name bytes for
names that are not valid UTF-8; content unaffected; ZFS-side behavior matches the og)
**Module:** ext4, btrfs (zfs verified MATCH on this edge)
**Parameters affected:** B10 (filename/Unicode handling — invalid-sequence fidelity),
D11 (node name field — information content), D12/D13 (session/report carry the baked
replacement permanently)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/EXT4SearchDLL_imports.txt`
(no MultiByteToWideChar/WideCharToMultiByte — probe re-run this session, exit 1);
`re-workspace/reversed/ghidra-searchdll/BtrfsSearchDLL_imports.txt` (same absence);
`re-workspace/reversed/ghidra-searchdll/ZfsSearchDLL_decompiled.c` :24995+ (FUN_18001c630
calls `MultiByteToWideChar(0xfde9, 0, …)` — CP_UTF8 with dwFlags=0 → U+FFFD per invalid
sequence, matching the core); `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md`
:326 (`char name[name_len]` — raw UTF-8 byte string, reader null-terminates)
**Core references:** `include/cr/cr_text.h` :49-93 (`utf8_to_u16` — every invalid lead
byte becomes U+FFFD, resync one byte ahead; RFC 3629 strict), :3-15 (policy header);
`src/core/ext4/cr_ext4_engine.cpp` :404-406 (`text::utf8_to_u16` call site, comment
"sequences become U+FFFD (never byte-lift)"); `src/core/btrfs/cr_btrfs_engine.cpp` :712;
`src/core/zfs/cr_zfs_engine.cpp` :1162/:1173/:1291; `include/cr/session/cr_records.h` :32
(session stores names UTF-16LE — the baked U+FFFD is permanent) — audited commit `8574039`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

For filenames that are **not valid UTF-8** (legacy Latin-1/CP1252-era Linux names, raw
byte names written by some tools), the og's EXT4 and Btrfs engines pass the raw bytes
through unchanged into the `.rsf` session — the original name is preserved byte-for-byte.
Our core's strict decoder replaces each invalid sequence with U+FFFD at decode time, and
because both the session and the report store names as UTF-16, the original bytes are
permanently unrecoverable: the file recovers under a different name than it had on disk.
The ZFS side of the same edge **matches** the og (both produce U+FFFD) and is pinned, not
divergent.

## What the reference does

- **EXT4/Btrfs**: the decompiled engines import no byte↔wide conversion API at all
  (`EXT4SearchDLL_imports.txt` / `BtrfsSearchDLL_imports.txt` — grep for
  `MultiByteToWideChar|WideCharToMultiByte` returns nothing, re-verified this session).
  Whatever bytes the directory entry carries are copied into the result node's name field,
  and the session writer serializes that field as `char name[name_len]`
  (`rsf-format--scan-session.md` :326 — a raw UTF-8-labeled byte string). A name that is
  invalid UTF-8 therefore survives to the session byte-identically.
- **ZFS**: `ZfsSearchDLL_decompiled.c` FUN_18001c630 :24995+ converts every name with
  `MultiByteToWideChar(0xfde9, 0, …)` — CP_UTF8, `dwFlags=0` (no MB_ERR_INVALID_CHARS).
  With flags 0, each invalid sequence maps to U+FFFD. The og ZFS engine therefore bakes
  replacements exactly like our core does — this side is parity, not divergence.

## What our core does

`utf8_to_u16` (`cr_text.h` :49-93) is a strict RFC 3629 decoder: overlong forms, surrogate
halves, code points above U+10FFFF, and any torn/invalid sequence decode as U+FFFD (one
per bad lead byte, resync one byte ahead — the policy is deliberate and documented in the
header, and it is the correct policy for the *valid* UTF-8 population after S-005's fix).
All three engines call it: ext4 at `cr_ext4_engine.cpp` :404-406, Btrfs at
`cr_btrfs_engine.cpp` :712, ZFS at `cr_zfs_engine.cpp` :1162/:1173/:1291. The result flows
into UTF-16 node names, a UTF-16LE session record (`cr_records.h` :32), and the report
layer. Nothing anywhere records the original bytes for a name that failed to decode.

## Gap

Falsifiable claim: on an ext4 (or Btrfs) volume containing a file whose name is, e.g.,
`"r\xe9sum\xe9.txt"` in Latin-1 (bytes `0x72 0xE9 0x73 …` — invalid UTF-8), the og's
session contains the name bytes `72 E9 73 75 6D E9 …` exactly, and our core's session
contains `U+0072 U+FFFD U+0073 U+0075 U+006D U+FFFD …` with the original bytes
unrecoverable from any core output.

## Impact

- Users with legacy non-UTF-8 filenames (pre-migration Linux systems, certain archival
  tools) recover those files under mangled, irreversible names. The name difference also
  breaks og-vs-core inventory comparisons for such volumes (same class of mechanical
  divergence S-001 caused before its fix).
- ZFS is unaffected (verified parity on this edge).
- NTFS/FAT/exFAT are unaffected (names are UTF-16 on disk; the engines never run this
  decoder for them).

## Reproduction / verification

- Corpus-derived (all re-verified first-hand this session):
  - `grep -i 'multibytetowidechar|widechartomultibyte' EXT4SearchDLL_imports.txt
    BtrfsSearchDLL_imports.txt` → no hits (raw passthrough).
  - `ZfsSearchDLL_decompiled.c` :24995+ FUN_18001c630 body: two
    `MultiByteToWideChar(0xfde9,0,…)` calls (length probe + convert).
  - `rsf-format--scan-session.md` :326: `char name[name_len]` raw field.
- Core-derived: `cr_text.h` :85-88 (`cp = 0xFFFDu; extra = 0;` — the resync/replacement),
  `cr_records.h` :32 (UTF-16LE session names).
- Executed: not run this session (fixture with deliberately invalid-UTF-8 names is
  specified in Tests below; the current behavior is fully determined by the pure
  function `utf8_to_u16` and is pinned by its unit tests).

## Fix plan (for the engineering team)

1. Keep the strict RFC 3629 decode for well-formed input, but make invalid bytes
   **lossless** instead of destructive for the two raw-byte engines: decode each byte
   that cannot start/continue a valid sequence as `U+E000 + byte` (private-use plane;
   MUTF-8-style lossless mapping). The mapping is a bijection, so the original raw name
   is reconstructible (`unit − 0xE000 = original byte`).
2. Split the decoder surface: `utf8_to_u16_lossless()` for ext4/Btrfs call sites;
   `utf8_to_u16()` (U+FFFD) stays for ZFS — **that asymmetry is the og parity**: ZFS's
   og engine converts via CP_UTF8/flags-0 and bakes replacements, ext4/Btrfs's og
   engines do not convert at all.
3. Pass-through discipline: session, report, and CLI output must not "clean" private-use
   units (they already round-trip arbitrary UTF-16 units; add a pin test so nobody adds
   a sanitizer later).
4. Document the private-use convention in `cr_text.h` and the report schema note, and
   register the ext4/Btrfs lossless mapping in PARITY-DELTAS (it is a *fidelity* superset
   over the observable session bytes: og keeps raw bytes, core keeps a reversible
   encoding of them).
5. Files to modify: `include/cr/cr_text.h`, `src/core/ext4/cr_ext4_engine.cpp` :406,
   `src/core/btrfs/cr_btrfs_engine.cpp` :712 (call-site swap), `tests/unit/test_text.cpp`,
   `tests/unit/test_ext4_volume.cpp`, `tests/unit/test_btrfs_volume.cpp`,
   `tests/unit/test_zfs_volume.cpp`.
6. Acceptance criteria: (a) an ext4/Btrfs fixture with a Latin-1 name round-trips the
   original bytes out of the session; (b) a ZFS fixture with the same name yields U+FFFD
   (og parity pinned); (c) no regression in the existing valid-UTF-8 S-005 tests; (d)
   full suite green + ASan.

## Tests to add

- `Text.InvalidByteLosslessMapping` (unit, pure function: `"\xE9"` → `U+E0E9`; torn
  3-byte lead → each byte mapped; valid sequences untouched).
- `Ext4Engine.Latin1NameRoundTripsRawBytes` (fixture dir entry with bytes `72 E9 …`;
  assert session name decodes back to the original byte string).
- `BtrfsEngine.Latin1NameRoundTripsRawBytes` (same shape).
- `ZfsEngine.InvalidUtf8NameDecodesReplacement` (**pin**: U+FFFD — guards the
  engine-asymmetry against someone "unifying" the decoders).

## Related findings

- S-005 (the parent mojibake finding — fixed for valid UTF-8; this is its residual edge,
  split out per the S-005 resolution note).
- N-009c (CLI filter ASCII mangling — same family of name-fidelity issues).

## Open questions

None. The ZFS side was resolved this session by the FUN_18001c630 read (CP_UTF8,
flags 0 → replacement, matching the core).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — "fix: M-010 — lossless invalid-UTF-8 name mapping (U+E000+byte) for ext4/Btrfs; ZFS keeps U+FFFD (og asymmetry)")
**Verified by:** `Text.InvalidByteLosslessMapping` + `Text.LosslessRoundTripRecoversRawBytes` (the bijective mapping + the inverse; torn sequences, lone continuations, 0xF8+ leads each covered) + `Text.ZfsDecoderStaysReplacement` (the engine-asymmetry pin: ZFS keeps U+FFFD per the og's `MultiByteToWideChar(CP_UTF8, 0)` decode, FUN_18001c630 :24995+) + `Btrfs.Latin1NameRoundTripsRawBytes` (fixture dir item with Latin-1 "r\xE9sum\xE9.txt": the name surfaces as `r U+E0E9 s u m U+E0E9 . t x t` and `lossless_u16_to_utf8` returns the original bytes byte-for-byte) + the updated `Ext4Engine.NonAsciiNamesDecodedNotByteLifted` / `Btrfs.NonAsciiNamesDecodedNotByteLifted` (their invalid-lead assertions flipped from U+FFFD to the lossless form — the pre-fix green U+FFFD assertions are the fail-before witness: the fix changed exactly this behavior) + `Zfs.NonAsciiNamesDecodedNotByteLifted` (unchanged and still green: the ZFS U+FFFD pin at engine level, fixture with an invalid name). Full suite 462/462 Linux ASan.
**Resolution notes:** Implemented per the fix plan: `utf8_to_u16_lossless()` added to cr_text.h (strict RFC 3629 for well-formed input; every byte that cannot start or continue a valid sequence maps to U+E000 + byte — MUTF-8-style, bijective; a torn multi-byte lead keeps its LEAD byte losslessly and resyncs, continuation bytes map individually; lone continuations and 0xF8+ leads map directly) + `lossless_u16_to_utf8()` (the inverse: private-use units [U+E000, U+E0FF] return their byte, valid codepoints re-encode, foreign private-use input is refused). ext4 (`cr_ext4_engine.cpp`) and Btrfs (`cr_btrfs_engine.cpp`) call sites swapped to the lossless decoder; ZFS call sites unchanged (the asymmetry IS the og parity: EXT4/Btrfs DLLs import no conversion API — re-verified this session, zero hits in both imports files — while ZFS converts CP_UTF8/flags-0 and bakes replacements). The private-use convention is documented in cr_text.h (including the "do NOT clean private-use units downstream" discipline); the session/report layers already round-trip arbitrary UTF-16 units (the whole-pipeline name_utf8 projections in the updated tests exercise this end to end). The finding's acceptance criteria: (a) Latin-1 round-trip ✓ (Btrfs engine test + Text inverse test); (b) ZFS U+FFFD pin ✓ (Text + existing engine test); (c) the S-005 valid-UTF-8 tests unchanged and green ✓; (d) full suite green + ASan ✓.
