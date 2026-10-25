# MAJOR M-055 — the Windows-only platform layer is UNCOMPILABLE at audit time: FOUR independent structural breaks across `cr_destio.cpp` (N-015's `free_bytes` nested inside `write()`) and `cr_transport.cpp` (N-013's functions outside `namespace cr::win`; N-041's stray namespace closer + deleted `heal_mu_` member with its use kept)

**Severity:** MAJOR (NEW-FINDING, discovered during the session bootstrap: the win-layer parse gate this finding lands caught FOUR build breaks no prior session had executed — two landed before this session's first pull, two more revealed by the parallel N-041/S-029/S-030 landings absorbed mid-session) · **Module:** writer / transport — the entire `src/win` platform layer's buildability · **Parameters affected:** Q67 (build integrity — every claimed Windows-lane guarantee since `3b5397a` was unverified: /W4/WX, DLL smoke, destio + bitlocker test binaries, the Win7 import gate all require cr_win to compile), F62-adjacent (the N-013 destination-arming functions were declared but never defined — the linker surface for SafeWriter arming was absent)
**Corpus references:** none — this is a build-integrity finding, not an og-parity finding; the corpus's role is confined to what the N-013/N-015 fix commits (whose landings broke the layer) cited
**Core references:** `src/win/cr_destio.cpp:50-66` pre-fix (`return OK;` of `write()` followed directly by `u64 WinDestIo::free_bytes(...) const {` — a member definition nested inside another member's body, with the orphaned `}` at the old line 66); `src/win/cr_transport.cpp` pre-fix ×3: (a) `} // namespace win` + `} // namespace cr` closing the namespaces BEFORE `volume_extents`/`destination_volume_letter`'s definitions while `include/cr/win/cr_transport.h:77-82` declares them INSIDE `namespace cr::win`; (b) a stray `} // namespace rda` closer left by the N-041 RdaClient deletion that closes `namespace win` mid-file (everything after it — the whole public transport API — resolves nothing: `rda::`, `probe_geometry`, `RdaTargetDevice` all error); (c) `RdaTargetDevice`'s `std::mutex heal_mu_;` member deleted by the same N-041 sweep while `read_at`'s heal lock kept using it. Introducing commits: `3b5397a` (N-013, 2026-10-25 21:07), `915bce3` (N-015, 21:18), `d8f6808` (N-041, 23:0x) — audited current `main` at session-19/22 bootstrap (`11afbfa` → `93d7fa6` through the parallel-lane pulls)
**Status:** NEW-FINDING — filed and FIXED in the same session, together with the Linux-side parse gate that makes the whole class of break visible on every future push

## Summary

The Windows-only platform layer (`src/win/*.cpp` — the only code the MSVC lane
compiles that the Linux lane does not) has been uncompilable since
2026-10-25 21:07, through FOUR independent defects introduced by three
concurrent fix lanes:

1. **`915bce3` (N-015)** inserted `WinDestIo::free_bytes` into
   `cr_destio.cpp` after `write()`'s `return OK;` but before `write()`'s
   closing brace — `free_bytes` became a member function defined inside
   another member function's body, which is a hard C++ syntax error
   (`qualified-id in declaration before '(' token` under g++; the
   equivalent MSVC diagnostic is C2601/C2086-class). Every MSVC build of
   `cr_win` fails at this file.

2. **`3b5397a` (N-013)** appended `volume_extents` (and, in the same
   trailing block, `destination_volume_letter`) to `cr_transport.cpp`
   AFTER the `} // namespace win` / `} // namespace cr` closers, while the
   header declares both inside `namespace cr::win`. Two consequences:
   the definition's unqualified `VolExtent` reference has no meaning at
   global scope (hard compile error), and even if it parsed, the
   header-declared `cr::win::volume_extents` / `cr::win::destination_volume_letter`
   would remain undefined — the N-013 destination arming the commit claimed
   to land could never link.

3. **`d8f6808` (N-041)** deleted the RdaClient class and, in the same
   sweep, left (a) a stray `} // namespace rda` closer that closes
   `namespace win` mid-file — every public transport function after it
   fails to resolve `rda::compute_pid_token`, `probe_geometry`,
   `RdaTargetDevice` (exactly the errors the parse gate prints), and
   (b) deleted `RdaTargetDevice`'s `std::mutex heal_mu_;` member while
   keeping `read_at`'s heal lock on it — an "undeclared identifier" on a
   guard whose member existed at `d8f6808^` (the landing's own diff
   shows it at the class tail).

The layer's test binaries (`cr_destio_tests`, `cr_bitlocker_tests`), the
`rrecov.exe` CLI, and `cr_ui.dll` all statically link `cr_win` — every
Windows-lane stage (build /W4/WX, tests, DLL smoke, the Win7 import
allowlist gate) fails at the compile step. The GitHub Actions windows job
has been observed queued-`in_progress` without executing throughout this
window (runner capacity), which is why the breaks survived every push
between `3b5397a` and this session: nothing that compiles `cr_win` has
actually run since 2026-10-25 21:07.

## What the reference does

Not an og-parity finding. The corpus's role: both breaking commits were
parity fixes whose corpus citations (N-013: UIUX architecture §7.3's
`\\.\X:` arming; N-015: RecoveryModule :47033-47038's free-space probe)
describe behavior those landings intended to add — the code below the
citations was never compiled by anything.

## What our core did (pre-fix)

As above — four uncompilable-translation-unit-class defects inside a
static library that every Windows artifact links (cr_destio.cpp one break;
cr_transport.cpp three). Additionally, the audit-trail discipline was
falsely green: the sessions that landed `3b5397a`/`915bce3`/`d8f6808`
recorded them as verified fixes while the layer they modified could not
compile; the GitHub Actions windows job's runs over this window show the
build step failing (e.g. run 40242066043's windows job: "Configure +
build (Debug + Release, manifest vcpkg): failure") — the red was visible
and unaddressed.

## Gap

Falsifiable and falsified in-session: `g++ -std=c++20 -fsyntax-only` with a
declaration-only SDK stub over `src/win/*.cpp` produces exactly two failing
files pre-fix (`cr_destio.cpp:52` `qualified-id in declaration before '('
token`; `cr_transport.cpp:1129` `'VolExtent' was not declared in this
scope; did you mean 'cr::win::VolExtent'?`) and zero post-fix. The same
gate wired as `ctest` test `win_syntax_check` reproduces the failure with
the source fixes stashed (fail-before) and passes with them applied
(pass-after).

## Impact

- The Windows platform layer's every guarantee was unverified for the
  entire `3b5397a`..session-19 window: no /W4/WX proof, no destio/
  bitlocker contract tests, no DLL smoke, no Win7 import gate.
- The N-013 destination-arming feature those sessions recorded as FIXED
  was in fact absent from any built artifact (declared, never defined).
- No wrong-results risk on Linux (the pure core never compiles src/win);
  the defect class is "claimed-but-unbuilt Windows surface".

## Reproduction / verification

Executed this session (strongest evidence class — executed comparison):

```
$ git stash push src/win/cr_destio.cpp src/win/cr_transport.cpp   # revert to pre-fix
$ ./tools/win_syntax_check.sh
win_syntax_check: FAIL: cr_destio.cpp does not parse
  cr_destio.cpp:52:26: error: qualified-id in declaration before '(' token
win_syntax_check: FAIL: cr_transport.cpp does not parse
  cr_transport.cpp:1129:49: error: 'VolExtent' was not declared in this scope ...
  ... (11 more errors)
gate exit: 1
$ git stash pop                                                    # re-apply fixes
$ ./tools/win_syntax_check.sh
win_syntax_check: ok: cr_bitlocker.cpp ... ok: cr_transport.cpp     # all six
gate exit: 0
```

Corroborating history: `git log -S free_bytes -- src/win/cr_destio.cpp`
(only `915bce3`); `git show 915bce3 -- src/win/cr_destio.cpp` (the hunk
inserts the function between `return OK;` and `}`); `git log -S
volume_extents` (only `3b5397a`, born after the namespace closers).

## Fix plan (executed this session)

1. `cr_destio.cpp`: close `write()` before `free_bytes`'s definition; drop
   the orphaned brace that previously sat after `free_bytes`'s body.
2. `cr_transport.cpp`: move the namespace closers to the true end of file
   so `volume_extents` and `destination_volume_letter` sit inside
   `namespace cr::win`, matching `cr_transport.h`'s declarations (this
   fixes BOTH the compile error and the link-time absence).
3. `cr_transport.cpp`: delete the stray `} // namespace rda` closer
   (namespace rda closes once, correctly labeled); restore
   `RdaTargetDevice::heal_mu_` (the `std::mutex` member N-041's sweep
   dropped while keeping `read_at`'s heal lock on it).
4. Land the permanent Linux-side parse gate: `tools/win_syntax_check.sh`
   (`g++ -std=c++20 -fshort-wchar -D_WIN32 -fsyntax-only` over
   `src/win/*.cpp`) + declaration-only SDK stubs (`tests/win-stub/`:
   windows.h, winioctl.h, ntddscsi.h, shlwapi.h — SDK-faithful shapes,
   declarations only, never linked or run) wired as ctest `win_syntax_check`
   on non-Windows lanes (Linux gcc only; MSVC compiles the real files).
5. The gate is the regression test: it fails on any future structural
   corruption of the win layer at push time, on the lane every session
   actually executes.

## Tests to add

- `win_syntax_check` (ctest, Linux gcc lane) — the fail-before/pass-after
  run above IS the test's proof; it now runs in the full suite sweep.

## Related findings

- N-056 (WinDestIo failed-rename temp leak) — same file; its fix lands on
  top of this one (the finding's own fix plan was unexecutable while the
  file didn't parse).
- N-013 / N-015 (the two parity fixes whose landings broke the layer) —
  both stand as findings-correct/code-landing-broken; this finding is the
  proof the landings never compiled.

## Open questions

None. (The Windows Actions runner capacity issue is an infrastructure
observation recorded here for the audit trail; the parse gate makes Linux
coverage independent of it.)

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's M-055 commit (brace repair + namespace relocation + the win-stub parse gate)
**Verified by:** fail-before/pass-after executed this session and quoted in Reproduction above (`win_syntax_check` exit 1 pre-fix with the two exact diagnostics, exit 0 post-fix, all six win-layer files parsing); full linux-release suite green including the new gate test; the same errors are deterministic for MSVC (member-in-member definition and unqualified namespace-scope lookup are ill-formed in both compilers).
**Resolution notes:** Four structural repairs (write() closed before free_bytes; volume_extents/destination_volume_letter relocated inside namespace cr::win matching the header contract; the stray N-041 `} // namespace rda` closer removed; RdaTargetDevice::heal_mu_ restored with its use intact) plus the permanent gate: tools/win_syntax_check.sh + tests/win-stub/ (four declaration-only SDK-faithful stub headers; -fshort-wchar for MSVC's 16-bit wchar_t, -D_WIN32 for the layer's platform branches) wired as ctest win_syntax_check on the Linux gcc lane. The stubs declare, never link or run — a new SDK symbol consumed by src/win grows the stub by design (that failure is the gate working). Discovery context recorded for the audit trail: all four breaks were introduced by concurrent parity-fix landings (3b5397a, 915bce3, d8f6808) and survived because no Linux-visible check parsed the win layer; the windows Actions job's failures over this window (e.g. run 40242066043, build step failure) were visible and unaddressed — future sessions must treat the win layer's buildability as proven only from the parse gate forward (or an actually-green MSVC run). The gate already absorbed S-029's IOCTL_DISK_UPDATE_PROPERTIES and the S-030 cr_image_win.cpp file during this session's parallel pulls.
