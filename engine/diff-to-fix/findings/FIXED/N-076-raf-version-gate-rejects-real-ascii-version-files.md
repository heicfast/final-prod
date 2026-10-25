# MINOR N-076 — NEW-FINDING (discovered during N-054 verification): validate_raf's version gate rejected every real-world RAF (ASCII version field read as binary)

**Severity:** MINOR
**Module:** carve
**Parameters affected:** B1 (detection — the canonical RAF row was dead for every real file), A28 (validator logic)
**Corpus references:** `re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md:55` (the FUJI 4CC keys the camera-RAW chain, which validates further); the RAF layout itself: 16-byte magic "FUJIFILMCCD-RAW " then a 4-byte version string at +16 (ASCII digits, e.g. "0100"/"0201" — Fuji RAF format references: exiftool FujiFilm tags, kladd/raw's `FormatVersion [4]byte` header struct, freedesktop libopenraw Fuji RAF page)
**Core references:** `src/core/carve/cr_analysers.cpp` validate_raf (pre-fix: `const u32 ver = rd32(s, hit + 16); if (ver == 0 || ver > 0x2000) return r;`) — discovered at session-12 commit `0fc9994`
**Status:** DISCOVERED + FIXED in the same change as N-054 (the finding whose verification exposed it)

## Summary

The pre-fix validate_raf read the 4-byte version field at +16 as a little-endian
binary integer and rejected anything above 0x2000. Real-world RAF files carry the
version as a 4-character ASCII string ("0100", "0201", …) — read LE those are
0x30313030-style values (~8.08e8), far above the cap. Every real RAF file was
therefore REJECTED by the canonical path, and — combined with the 16-byte magic
N-054 describes — the FMT_RAF row had been unreachable for every real-world file
since its introduction. No RAF test existed (zero fixture coverage, which is how
the defect survived).

## What the reference does

The og keys RAF on the FUJI 4CC and its camera-RAW chain "validates further"
(census :55) — the chain's exact depth is corpus-undecoded, but the og's RAW
recovery demonstrably surfaces real .RAF files, so its chain accepts the real
ASCII-version layout.

## What our core does (pre-fix)

`ver == 0 || ver > 0x2000` — rejects ASCII versions; only a small-binary version
field (never seen in real RAF files) passed.

## Gap

Falsifiable: a byte-exact real RAF header (magic + "0201" + section table) is
rejected by the pre-fix core — `Carve.RafRealAsciiVersionAccepted` is RED on the
pre-fix engine (executed: fail-before stash-isolated rebuild, this session).

## Impact

Complete RAF recall loss on the grammar path (the format row existed but could
never fire on a real file). MINOR severity per the scale only because carve
recall for RAF would in practice fall back to... nothing — the 16-byte magic also
gated dispatch, so RAF files were not carved at all. The N-054 magic fix alone
would have LEFT this defect live for canonical files (the variant path requires
only a nonzero version, but the canonical path still carried the binary cap) —
which is exactly how the verification pass caught it.

## Reproduction / verification

Byte-level layout cross-checked against three independent public RAF format
descriptions (exiftool FujiFilm tag docs, the kladd/raw Go header struct, the
freedesktop libopenraw page: magic[16] + FormatVersion[4]). Fail-before executed
this session on the stash-isolated pre-fix engine.

## Fix (landed with N-054)

Canonical acceptance: version is 4 ASCII digits OR a small binary value
(≤ 0x2000) — both writer populations covered; the variant path requires any
nonzero version. `Carve.RafRealAsciiVersionAccepted` is the regression pin.

## Related findings

- N-054 (the RAF/JPEG magic-strictness pair whose verification exposed this)
- S-007 (format coverage context)

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's carve-magic-quartet batch (DFX12) — filed and fixed in the same change (the discovered-divergence SOP: announced in the commit message)
**Verified by:** `Carve.RafRealAsciiVersionAccepted` — fail-before executed (RED on the stash-isolated pre-fix engine; green post-fix; the fixture is a byte-exact real-layout RAF header: "FUJIFILMCCD-RAW " + "0201" + section table). Full suite 549/549 linux-release.
**Resolution notes:** the canonical version gate now accepts 4-ASCII-digit version strings OR small binary values (≤ 0x2000) — the real writer population (ASCII, per exiftool/kladd/freedesktop layouts) and the legacy binary assumption are both covered; the variant-header path (N-054) requires any nonzero version. Detected during N-054's verification pass when building the RAF fixtures exposed that NO fixture for the format had ever existed — the row was dead for real files twice over (magic + version gate).
