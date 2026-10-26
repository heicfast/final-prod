# MINOR N-082 — the M-052 exFAT recycle-pairing test copies 15 UTF-16 units from a 14-unit literal: one-past-terminator read under ASan

**Severity:** MINOR (NEW-FINDING, discovered during M-030's ASan verification) · **Module:** exfat (the M-052 pairing test's $I fixture) · **Parameters affected:** sanitizer cleanliness (the linux ASan CI lane)
**Corpus references:** none — a test-fixture defect in our own M-052 landing, not an og-parity divergence
**Core references:** `tests/unit/test_exfat_volume.cpp:1880` (`ExfatEngine.RecyclePairingMirrored`): `const char16_t* path = u"C:\\photo.jpg"` (13 chars + NUL = 14 units) copied with `for (i = 0; i < 15; ++i)` — `path[14]` reads one past the terminator; first reproduced on the merged tree during this session's ASan sweep (`SUMMARY: AddressSanitizer: global-buffer-overflow ... :1880 in ExfatEngine_RecyclePairingMirrored_Test::TestBody()`), re-verified pre-existing with this lane's changes stashed
**Status:** NEW-FINDING — filed and FIXED in the same session

## Summary

The $I-record fixture plants a 15-unit path field but seeds it from a
14-unit string literal with a hard-coded 15-iteration copy loop — the
fifteenth read runs past the literal's NUL. ASan aborts the test binary.

## Gap

Falsifiable and falsified in-session: the ASan `cr_tests` binary aborts
with the global-buffer-overflow above on the unmodified merged tree;
post-fix the identical binary exits 0.

## Fix

Bound the copy by the string: `const std::u16string path = ...` with
`for (size_t i = 0; i < path.size() && i < 15; ++i)` — the field's
remaining units stay zero (the image is zero-initialized), which is the
on-disk shape a real formatter writes (a NUL-padded path field).

## Tests to add

The ASan binary itself is the detector; post-fix `ExfatEngine.
RecyclePairingMirrored` passes under ASan (the pairing behavior it
proves is unchanged — the fixture's declared pathlen still covers the
field).

## Related findings

- M-052 (the recycle-pairing feature the fixture proves — its behavior
  is untouched) · N-081 (the same sweep's other ASan red)

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's ASan-repair commit — `git log` "fix(N-081/N-082)")
**Verified by:** the ASan-lane evidence: pre-fix `./build/linux-gcc/tests/cr_tests` aborts with `SUMMARY: AddressSanitizer: global-buffer-overflow ... :1880 in ExfatEngine_RecyclePairingMirrored_Test::TestBody()` (reproduced first-hand with this lane's changes stashed — pre-existing from the M-052 landing); post-fix the identical binary exits 0 and `ExfatEngine.RecyclePairingMirrored` passes under ASan — the pairing behavior the test proves is unchanged.
**Resolution notes:** CONFIRMED (a test-fixture defect). The copy loop is now bounded by the string (`std::u16string` + `i < path.size() && i < 15`); the path field's remaining units stay zero — the NUL-padded shape a real formatter writes. No core code changed.
