# BUILD-SUPPRESSIONS — every warning/error suppression, with rationale and scope

> Policy (mandate §7): no suppression may hide a real issue. Every entry is a
> deferred fix with an owner and a scope — never a permanent silencing. A
> suppression that is not in this table is a bug.

**Current state: exactly ONE registered entry (S-1 below — the vendored
stb_image.h include block).** The ZERO-entries headline this file carried
after the warning-clean campaign predates S-1's landing and was stale: the
S-1 row has been in this table since the thumbnail lane, and the table is
the registry the policy above demands. All first-party code builds
warning-clean under gcc `-Wall -Wextra -Werror -Wshadow` and MSVC `/W4 /WX`
(global flags in `CMakeLists.txt`; every preset build is the Werror build).
The historical warnings-as-errors fixes (the MSVC `/W4` red waves, the clang
fuzz lane's 48-warning backlog zeroing, C4244/C4459-class repairs) are
recorded in the repo worklog and `docs/error-fix-registry.md` — fixed, not
suppressed. (Session-84 corrected this headline; the contradiction —
"ZERO entries" one paragraph above the S-1 row — was the doc-side drift.)

When a suppression becomes genuinely necessary (e.g. a third-party header
warning under a new toolchain), it lands here with:

| ID | Suppression (file:line / flag) | Rationale | Scope | Deferred-fix tracking |
|---|---|---|---|---|
| S-1 | `src/core/preview/cr_thumbnail.cpp` — `#pragma warning`/`#pragma GCC diagnostic` push-ignored (`-Wmaybe-uninitialized`, `-Wunused-but-set-variable`, `-Wshadow`, `-Wpedantic`; MSVC 4100/4189/4244/4245/4267/4456/4457/4701/4702/4706) wrapping ONLY the vendored `third_party/stb/stb_image.h` include+implementation | The public-domain stb decoder is vendored upstream code: it must compile byte-identical to upstream (no local edits), and its internal warnings under our global Werror gates are upstream-known, not defects. The pragma scope closes immediately after the include; all first-party code in the file compiles under full strictness. `STBI_NO_STDIO` + `STBI_MAX_DIMENSIONS=32768` are upstream-supported configuration macros, not edits. | One include block in one file; the first-party API/impl in the same TU is outside the push/pop | None — pinned configuration, re-evaluated on stb version bumps (upstream upgrades re-run the fuzz + test suites) |
