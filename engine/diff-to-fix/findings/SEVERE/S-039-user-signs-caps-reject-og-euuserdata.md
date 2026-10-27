# SEVERE S-039 — the user-signature surface rejects the og's own database: 15 of 111 EUUserData entries violate the core's caps, the atomic parse loads ZERO, and the CLI aborts the scan

**Severity:** SEVERE
**Module / Surface:** carve / user-extensible signature DB (modules/carve.md; the M-048 feature's grammar half)
**Parameters affected:** C20-adjacent (the DB grammar's field bounds), A28-adjacent (which signs can ever load), B1-adjacent (detection coverage via user signs)
**Corpus references:** `easeus-drw-research/re-workspace/artifacts/config-files/userdata-remoteconfig--signature-and-flags.md` (the EUUserData grammar — NEVER-CITED before this session); the og's own shipped DB `og-codebase/EUUserData` (16,891 bytes, 111 entries — **measured first-hand this session**): max sign length **44 bytes** (e.g. `hqx` `0,28546869732066696C…`, `img` `0,000100005374616E64…`, `eps` `0,252150532D41646F62…` — 11 entries over 16 B), max signs per entry **11** (`mpeg`), max offset **526** (`zip` `526,504B537058`), max mime length **73** (3 entries) — **15 of 111 entries violate at least one core cap**
**Core references:** audited commit `dd82db9` — `include/cr/carve/cr_usersigns.h:34-41` (the caps: `kMaxSignBytes = 16` with the comment "the og's longest entry is 8 bytes" — **factually false**, `kMaxSignsPerEntry = 8`, `kMaxOffset = 512`, `kMaxMimeLen = 63`); `src/core/carve/cr_usersigns.cpp:95-124` (the parser: `n > kMaxSignBytes * 2 → return false` — **atomic**: any violating entry rejects the whole file); `src/app/rrecov.cpp:922-931` (the CLI: parse failure → the error message naming "the og's EUUserData JSON shape" → **`return 1` — the scan aborts**)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** expanded on prior finding M-048 (FIXED — the user-extensible signature aid's surface); M-048's verification parsed only 3 compliant entries (the false "og's longest" comments date from it) — the caps-vs-og's-own-data contradiction is newly discovered by this session's first read of the grammar artifact + first measurement of the og's DB

## Summary

The core's user-signature parser imposes caps that the reference's own shipped database violates on 15 of 111 entries — a 44-byte sign vs the 16-byte cap, 11 signs vs 8, a 526 offset vs 512, a 73-char mime vs 63 — and the parse is atomic, so the og's EUUserData loads **zero** entries. The CLI path (`rrecov --user-signs EUUserData`) then aborts the entire scan with exit 1, in an error message that explicitly advertises "the og's EUUserData JSON shape". The feature is therefore unusable with the very database it names, and the header's "the og's longest entry is 8 bytes" justification comments are factually false against the og's actual data.

## What the reference does

The og ships `EUUserData` (16,891 bytes) in the install tree (and a pristine copy in the PE payload) with 111 extension entries; the grammar artifact documents the sign/mime/flags structure. The measured extremes (this session, first-hand): signs up to 44 bytes, up to 11 signs per entry, offsets up to 526, mime strings up to 73 chars. The og's engine consults this DB alongside its raw-signs table (the consumption-mode nuance is tracked in OQ-SA-3's related probe).

## What our implementation does

`cr_usersigns.h:34-41` sets `kMaxSignBytes = 16` (comment: "the og's longest entry is 8 bytes"), `kMaxSignsPerEntry = 8`, `kMaxOffset = 512`, `kMaxMimeLen = 63`. The parser (`cr_usersigns.cpp:95-124`) rejects any entry violating these — and a single rejection fails the whole parse (the function returns false; no partial-load mode exists). The CLI (`rrecov.cpp:922-931`) propagates parse failure to a hard scan abort. Feeding it the og's own DB: the first over-cap entry (e.g. `epub`'s 8-byte sign is fine, but `hqx`'s 44-byte sign is not) rejects the file — 0 of 111 entries load.

## Gap

Falsifiable claim: `rrecov scan --user-signs <og-codebase/EUUserData> <target>` exits 1 with the invalid-database message and performs no scan; the og loads all 111 entries from the same file. (Verified statically: the caps, the measured violators, the atomic parser, the abort path — an executed run needs a Windows host; the chain is fully pinned.)

## Impact

- **The M-048 feature's og-interop path is dead on arrival**: the error message names EUUserData; a user who supplies the og's actual DB (the obvious migration path for an EaseUS user) gets a refused scan, not a partial load or a skipped entry.
- **False justification comments** (`cr_usersigns.h:36-41`'s "the og's longest…") will mislead future maintainers into keeping the caps.
- The `zip` entry's offset-526 sign is additionally a semantic loss even under a fixed-cap parse: the core's "signs live inside the probe stride" rationale for 512 is not the og's model (the og has no such stride bound).

## Reproduction / verification

Executed measurement (strength-1, this session): the 111-entry census of `og-codebase/EUUserData` with the four maxima and the 15 violating entries (the census script's output is quoted in the citations; the entries are enumerable by anyone with the file). Core evidence (strength-3, quoted): the cap constants + their comments, the atomic `return false` chain, the CLI abort. The corpus grammar artifact is the third leg (NEVER-CITED before — its bounds were never consulted when the caps were invented).

## Fix plan (for the engineering team)

1. Raise the caps to the measured og maxima with honest margins: `kMaxSignBytes = 48` (og max 44 + headroom), `kMaxSignsPerEntry = 12` (og 11), `kMaxOffset = 1024` (og 526; keeps a hostile-blob bound without the false stride rationale — rewrite the comment), `kMaxMimeLen = 80` (og 73), and fix all four justification comments to cite the measured values.
2. Decide the partial-load posture: the og consults its DB whole-file; with honest caps the og file parses clean, but a hostile/corrupt file should still fail loudly — keep atomicity, it is the right posture once the caps are truthful.
3. Tests: (a) parse the og's actual EUUserData (ship a trimmed fixture or the measured extremes: one 44-byte sign, an 11-sign entry, a 526-offset sign, a 73-char mime) — all load, count = entries; (b) a post-cap hostile blob (a 65,536-byte sign) still rejects atomically; (c) the CLI round-trip: `--user-signs <fixture>` proceeds to the scan with the registered count printed.
4. Acceptance criteria: the fixture test is fail-before red (it cannot pass today — the og's own extremes are over-cap); the comments carry the measured numbers.

## Tests to add

- `UserSignsParseOgShippedExtremes` (fails before).
- `UserSignsHostileBlobStillAtomic`.
- `UserSignsCliRoundTripProceedsToScan` (fails before: exit 1 today).

## Related findings

- **M-048** (FIXED) — the feature this extends; its verification gap (3 compliant entries) is why this survived.
- **M-065** (this session) — the missing default set, the same surface's other half.

## Open questions

None on the caps themselves (measured). The consumption-mode nuance (secondary typing vs. capability-creating signs) belongs to M-065's probe.
