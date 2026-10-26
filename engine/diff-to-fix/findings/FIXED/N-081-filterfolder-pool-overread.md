# MINOR N-081 — the M-018 filterfolder landing reads its semicolon pools as C strings: global-buffer-overflow under ASan (the pools have a trailing delimiter and NO NUL terminator)

**Severity:** MINOR (NEW-FINDING, discovered during M-030's ASan verification: the linux-gcc lane's `cr_ui_tests`/`cr_tests` ASan binaries abort with a global-buffer-overflow in core code shipped by the merge-storm window's M-018 landing) · **Module:** orchestration (the junk filter's static set build) · **Parameters affected:** sanitizer cleanliness (the linux ASan CI lane), P46-adjacent (the M-018 surface's own hygiene)
**Corpus references:** none — a quality defect in our own landing, not an og-parity divergence; the pools are the M-018 lane's decoded copies of the og's filterFolder string tables (the og consumes them from PE resources with explicit length plumbing)
**Core references:** `src/core/orchestrator/cr_filterfolder.cpp:31` `split_into` — `strchr`/`strlen` over `reinterpret_cast<const char*>(pool)`; `src/core/orchestrator/cr_filterfolder_data.cpp` — `kFolderPool` (51 bytes), `kFilePool` (108), `kFatNamePool` (256): semicolon-delimited, TRAILING delimiter, deliberately no NUL byte; first reproduced on the merged tree during this session's M-030 ASan sweep (`SUMMARY: AddressSanitizer: global-buffer-overflow ... cr_filterfolder.cpp:31 in split_into`), re-verified pre-existing with this lane's changes stashed
**Status:** NEW-FINDING — filed and FIXED in the same session (the ASan lane does not stay red)

## Summary

`JunkSets::split_into` parses the three filter pools as C strings
(`while (*p)` + `strchr`), but the pools are fixed-size `u8` arrays that
end with their final `;` and carry no terminator. Every scan-process
startup (the junk set is a magic-static built on first use) reads past
the end of a global — ASan aborts the whole test binary on the first
touch.

## Gap

Falsifiable and falsified in-session: the ASan `cr_ui_tests` binary
aborts with `global-buffer-overflow ... in split_into` on the unmodified
merged tree; post-fix the identical binary exits 0 with the full suite
green.

## Fix

Export the byte lengths (`kFolderPoolLen`/`kFilePoolLen`/`kFatNamePoolLen`
= `sizeof(pool)`, defined beside the arrays, declared in the header with
the N-081 note) and parse with `std::string_view` bounded by the length:
identical tokens for the well-formed data (the trailing `;` yields no
empty tail), no read past the array, ever.

## Tests to add

The ASan suite itself is the detector (any junk-filter test startup
touched the overflow); the post-fix ASan exits (0 across `cr_tests` and
`cr_ui_tests`) plus the unchanged FilterFolder token pins are the proof.

## Related findings

- M-018 (the junk-filter feature this bug rode in on — its own tests stay
  green, the fix changes no tokens) · N-077/N-079 (the same
  ASan-lane-red defect class)

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's ASan-repair commit — `git log` "fix(N-081/N-082)")
**Verified by:** the ASan-lane evidence: pre-fix `./build/linux-gcc/tests/cr_ui_tests` aborts with `SUMMARY: AddressSanitizer: global-buffer-overflow ... cr_filterfolder.cpp:31 in split_into` (reproduced first-hand on the merged tree with this lane's changes stashed — pre-existing from the M-018 landing); post-fix both ASan binaries (`cr_tests`, `cr_ui_tests`) exit 0 with zero summaries, and the release lane + ctest 895/895 stay green.
**Resolution notes:** CONFIRMED (a quality defect in our own landing — the pools are deliberately terminator-free data arrays). Fix at the cause: the byte lengths are now exported (`kFolderPoolLen`/`kFilePoolLen`/`kFatNamePoolLen` = `sizeof(pool)`, defined beside the arrays) and `split_into` parses a `std::string_view` bounded by the length — no C-string reads on non-C-string data, identical tokens for the well-formed pools (the trailing `;` produces no empty tail; the FilterFolder token pins are unchanged and green). The og consumes its equivalent tables from PE resources with explicit length plumbing; our bounded parse now matches that shape.
