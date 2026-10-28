# SEVERE S-039 — the user-signature surface rejects the og's own database: 15 of 111 EUUserData entries violate the core's caps, the atomic parse loads ZERO, and the CLI aborts the scan

**Severity:** SEVERE
**Module / Surface:** carve / user-extensible signature DB (modules/carve.md; the M-048 feature's grammar half)
**Parameters affected:** C20-adjacent (the DB grammar's field bounds), A28-adjacent (which signs can ever load), B1-adjacent (detection coverage via user signs)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` (the EUUserData grammar — NEVER-CITED before this session); the og's own shipped DB `og-codebase/EUUserData` (16,891 bytes, 111 entries — **measured first-hand this session**): max sign length **44 bytes** (e.g. `hqx` `0,28546869732066696C…`, `img` `0,000100005374616E64…`, `eps` `0,252150532D41646F62…` — 11 entries over 16 B), max signs per entry **11** (`mpeg`), max offset **526** (`zip` `526,504B537058`), max mime length **73** (3 entries) — **15 of 111 entries violate at least one core cap**
**Core references:** audited commit `dd82db9` — `include/cr/carve/cr_usersigns.h:34-41` (the caps: `kMaxSignBytes = 16` with the comment "the og's longest entry is 8 bytes" — **factually false**, `kMaxSignsPerEntry = 8`, `kMaxOffset = 512`, `kMaxMimeLen = 63`); `src/core/carve/cr_usersigns.cpp:95-124` (the parser: `n > kMaxSignBytes * 2 → return false` — **atomic**: any violating entry rejects the whole file); `src/app/rrecov.cpp:922-931` (the CLI: parse failure → the error message naming "the og's EUUserData JSON shape" → **`return 1` — the scan aborts**)
**Prior audit check:** expanded on prior finding M-048 (FIXED — the user-extensible signature aid's surface); M-048's verification parsed only 3 compliant entries (the false "og's longest" comments date from it) — the caps-vs-og's-own-data contradiction is newly discovered by this session's first read of the grammar artifact + first measurement of the og's DB

## Resolution (FIXED — the caps raised to the measured og maxima; fail-before-proven)

The caps in `include/cr/carve/cr_usersigns.h` now carry the og's own measured
extremes with honest headroom:

| Constant | Before | After | The og's measured extreme |
|---|---|---|---|
| `kMaxSignBytes` | 16 | **48** | 44 (`img`/`hqx`/`eps`) |
| `kMaxSignsPerEntry` | 8 | **12** | 11 (`mpeg`) |
| `kMaxOffset` | 512 | **1024** | 526 (`zip`) |
| `kMaxMimeLen` | 63 | **80** | 73 (`pptx` OOXML string) |

- All four false justification comments replaced with the measured values
  (the "the og's longest entry is 8 bytes" and "signs live inside the probe
  stride" rationales were factually false; the og has no stride bound on
  sign offsets — the scan's `match_at` avail check is the only bound).
- `parse_sign`'s digit-count comment updated (offset ≤ 1024 is four digits).
- **Atomicity kept** (the finding's own verdict: honest caps + atomic reject
  is the right posture; a hostile/corrupt file still fails loudly).
- The `rrecov --user-signs` CLI path is unchanged — with truthful caps the
  og's own DB now parses, installs (111 entries / 190 signs), and the scan
  proceeds (the pre-fix exit-1 abort happened only on the over-cap reject).

**Tests (fail-before PROVEN on the stash-reverted pre-fix build):**
- `UserSigns.ParseOgShippedExtremes` — RED pre-fix (the og's 44-byte sign,
  11-sign entry, offset-526 sign, and 73-char mime together rejected the
  whole atomic parse); GREEN post-fix with per-extreme byte assertions
  (the real `mpeg` sign strings + the real `img` 44-byte sign + the real
  `zip` 526-offset sign straight from `og-codebase/EUUserData`).
- `UserSigns.HostileBlobStillAtomic` — the hostile-input contract at the new
  bounds: a 98-hex-digit sign (past 96), a 99999 offset, a 13-sign entry,
  and an 81-char mime all still reject the whole file atomically.
- `UserSigns.RegistrationRoundTripProceedsToScan` — RED pre-fix (the parse
  of the og's own `zip` extreme entries failed → the CLI-abort path);
  GREEN post-fix: parse → install → count surfaced → the offset-526 sign
  fires at the stride and carves with the MIME in the note.
- Full suite: 914 tests — 910 passed, 4 platform-skips (unchanged), the
  85-test Carve/RawSigns block fully green.

**Landed:** commit "fix(carve): S-039 — user-sign caps raised to the og's
measured EUUserData maxima (the og's own DB now loads; fail-before proven)"
(files: `include/cr/carve/cr_usersigns.h`, `src/core/carve/cr_usersigns.cpp`,
`tests/unit/test_carve.cpp`).
